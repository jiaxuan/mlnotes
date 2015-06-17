#include <bitset>
#include <chrono>
#include <thread>
#include "bubblenet/BubbleNet.hpp"
#include "detail/subscription/outright/FXOutrightRequestInfo.hpp"
#include "detail/subscription/outright/axe/AXEBaseSubscription.hpp"
#include "detail/subscription/outright/axe/AXEOutrightSubscription.hpp"
#include "detail/subscription/outright/axe/AXEOutrightMassSubscription.hpp"
#include "detail/subscription/outright/axe/AXEBubbleNetContext.hpp"

#include "detail/subscription/outright/axe/graph/ApplyAXEFastMarket.hpp"
#include "detail/subscription/outright/axe/graph/ApplyAXEInterpolation.hpp"
#include "detail/subscription/outright/axe/graph/ApplyAXESalesSpread.hpp"
#include "detail/subscription/outright/axe/graph/ApplyGroupTradeVolume.hpp"
#include "detail/subscription/outright/axe/graph/ApplySpotMidConstraint.hpp"
#include "detail/subscription/outright/axe/graph/InitializeTick.hpp"
#include "detail/subscription/outright/axe/graph/AutoQuoteConditions/VerifyAXEFastMarketData.hpp"
#include "detail/subscription/outright/axe/graph/AutoQuoteConditions/VerifySalesSpreadData.hpp"
#include "detail/subscription/outright/axe/graph/AutoQuoteConditions/VerifyInvertedPrice.hpp"
#include "detail/subscription/outright/axe/graph/AutoQuoteConditions/VerifySpotLimit.hpp"
#include "detail/subscription/outright/axe/graph/AutoQuoteConditions/VerifyPostCalculate.hpp"
#include "detail/subscription/outright/axe/graph/CrossedSpot/ApplyAXECrossSpotRate.hpp"
#include "detail/subscription/outright/axe/graph/CrossedSpot/VerifyCrossSpotRates.hpp"
#include <fx/tpsclient3/FXOutrightRequestFactory.hpp>

#include "audit/ReasoningStore.hpp"
#include "InputData.hpp"

// #define DEBUG 1

typedef std::chrono::microseconds usec_t;
usec_t usecs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
}

// #define CLEAR() code = FXOutrightQuote::E_NONE; isValid = true; reasoning->clear();
// #define CLEAR() code = FXOutrightQuote::E_NONE; isValid = true; 
#define CLEAR()



using namespace ml::common::bubblenet;
using namespace ml::common::util;
using namespace ml::spec::fx::fwd;
using namespace ml::spec::fx::fwd::ndf;
using namespace ml::spec::fx::tps::quote;
using namespace ml::spec::fx::trade;

using ml::panther::serialize::types::Decimal;
using ml::panther::serialize::types::String;
using ml::common::math::DecimalDecNumber;

const int NUM_VOLUMES = 1;

std::string to_string(std::bitset<MAX_NODES>& mask)
{
    std::string s = mask.to_string();
    std::reverse(s.begin(), s.end());
    return s;
}

boost::shared_ptr<FXOutrightRequest> createRequest(int64 requestId, int64 userId, int64 groupId, 
                                                   std::string ccyPair, std::string tenor) 
{
    return ml::fx::createFXOutrightRequestPtr(FXRequestId(userId, requestId),
                                              groupId,
                                              ccyPair,
                                              tenor,
                                              ml::common::math::Decimal(1000),
                                              FXOutrightRequest::BASE_VOLUME,
                                              FXOutrightRequest::BOTH_SIDES,
                                              FXOutrightRequest::SALES_RATE,
                                              FXOutrightRequest::FULL,
                                              mlc::util::Nullable<Decimal>::Null,
                                              true,//inCompetition
                                              std::deque < ml::spec::fx::tps::quote::FXTPSQualifier >(),
                                              mlc::util::Nullable<ml::spec::fx::fwd::ndf::FXNDFFixingInfo>::Null,
                                              0,//republication interval
                                              mlc::util::Nullable<Decimal>(Decimal::ZERO),
                                              ml::common::util::Nullable< ml::spec::fx::trade::FXSettlementAccount >::Null
                                             );
}

FXSpotTickWithDepth::Ptr createSpotTick(double bid, double ask) 
{
    std::deque< FXSpotUnifiedQuote > rateStack;
    rateStack.push_back(FXSpotUnifiedQuote{500000,Decimal(bid),Decimal(ask)});
    rateStack.push_back(FXSpotUnifiedQuote{5000000,Decimal(bid+0.1),Decimal(ask+0.1)});
    rateStack.push_back(FXSpotUnifiedQuote{10000000,Decimal(bid+0.2),Decimal(ask+0.2)});
    return  boost::make_shared<FXSpotTickWithDepth>(Time().usecs(),
                                                    6001,
                                                    "EURUSD",
                                                    FXSpotTickStatus::NORMAL,
                                                    Nullable<Decimal>(bid),
                                                    Nullable<Decimal>(ask),
                                                    Nullable<Decimal>(bid),
                                                    Nullable<Decimal>(ask),
                                                    Nullable<Decimal>((bid+ask)*0.5),
                                                    Nullable<String>(),
                                                    Nullable<String>(),
                                                    rateStack,
                                                    "",
                                                    FXSpotTickWithDepth::ROUND_NORMAL,
                                                    FXSpotTickWithDepth::PUBLISH_NORMAL,
                                                    Time().usecs(),
                                                    20000000LL,
                                                    Nullable<String>("externalid"),
                                                    FXSpotTickWithDepth::INSTINCT,
                                                    Decimal(bid),
                                                    Decimal(ask));
}


//////////////////////////////////////////////////////////////////////////////////////////
// for subscription testing

struct SpotEvent : public ml::fx::tps::Event
{
    typedef boost::shared_ptr<SpotEvent>    Ptr;

    static const int SPOT_EVENT_ID_BASE = 100;

    SpotEvent( const FXSpotTickWithDepth::Ptr & d, FXSpotTickWithDepth::Ptr & t) : data(d), target(t) { }

    // Override from Event
    const int id() const
    {
        return SPOT_EVENT_ID_BASE + 1;
    }

    const char * name() const { return "SpotEvent"; }
    void onProcess() { target = data; }

    void populateAsString(std::string &text) const
    {
        text = data->toString();    
        return;
    }

    ml::spec::fx::spot::FXSpotTickWithDepth::Ptr getData() const { return data; }

    private:
    ml::spec::fx::spot::FXSpotTickWithDepth::Ptr data;
    ml::spec::fx::spot::FXSpotTickWithDepth::Ptr & target;
};

const char* getEventName(int32 kind)
{
    switch(kind)
    {
        case SPOT_INDICATIVE_EVENT:
            return "SpotIndicativeEvent";
        case FWD_INDICATIVE_EVENT:
            return "FwdIndicativeEvent";
        case SPOT_FAST_MARKET_EVENT:
            return "SpotFastMarketEvent";
        case FWD_FAST_MARKET_EVENT:
            return "FwdFastMarketEvent";
        case SPOT_TRADER_SPREAD_EVENT:
            return "SpotTraderSpreadEvent";
        case FWD_TRADER_SPREAD_EVENT:
            return "FwdTraderSpreadEvent";
        case TRADER_SPREAD_THRESHOLD_EVENT:
            return "TraderSpreadThresholdEvent";
        case FWD_SALES_SPREAD_EVENT:
            return "FwdSalesSpreadEvent";
        case SLA_EVENT:
            return "SLAEvent";
        case CLIENT_PENALTY_EVENT:
            return "ClientPenaltyEvent";
        case CUTOFF_EVENT:
            return "CutOffEvent";
        case ENTITY_CUTOFF_EVENT:
            return "EntityCutOffEvent";
        case SPOT_LIMIT_EVENT:
            return "SpotLimitEvent";
        case FWD_LIMIT_EVENT:
            return "FwdLimitEvent";
        case GROUP_FWD_AMG_INDICATIVE_EVENT:
            return "GroupFwdAmgIndicativeEvent";
        case FUNDING_CASH_RATE_EVENT:
            return "FundingCashRateEvent";
        case GROUP_TRADE_VOLUME_EVENT:
            return "GroupTradeVolumeEvent";
        case AXE_FAST_MARKET_EVENT:
            return "axeFastMarketEvent";
        case FXGROUP_DATA_EVENT:
            return "FXGroupDataEvent";
        case SKEW_MIN_SPREADS_DATA_EVENT:
            return "SkewMinSpreadsDataEvent";
        case SPREAD_MULTIPLIER_DATA_EVENT:
            return "SpreadMultiplierDataEvent";
        case SPOT_TRADER_MARKUP_DATA_EVENT:
            return "SpotTraderMarkupDataEvent";
        case SPOT_SALES_MARKUP_EVENT:
            return "SpotSalesMarkupDataEvent";
        case CROSS_FWD_SPREAD_FACTOR_EVENT:
            return "CrossFwdSpreadFactorEvent";
        case AUTO_HEDGE_VOLUME_EVENT:
            return "AutoHedgeVolumeEvent";
    }
    return "<UnknownDataEvent>";
}

template <class T>
struct DataEvent : public ml::fx::tps::Event
{
    typedef boost::shared_ptr< DataEvent<T> > Ptr;

    DataEvent(const boost::shared_ptr<T> & d, boost::shared_ptr<T> & t, int32 k)
        : data(d), target(t), kind(k) { }

    // Override from Event
    const int id() const
    {
        return kind;
    }

    DataEvent( const DataEvent& other)
        : data(other.d)
          , target(other.t)
          , kind(other.k)
    {
        //done
    }

    const char * name() const
    {
        return getEventName(kind);
    }

    virtual const char * toString() const { return data->toString().c_str(); }

    void onProcess() { target = data; }

    typename boost::shared_ptr<T> data;
    typename boost::shared_ptr<T> & target;
    int32 kind;
};

struct SubsTestSubscription : public AXEOutrightSubscription
{
   SubsTestSubscription()
        : AXEOutrightSubscription("subscriberId", 
                                  createRequest(1, 0, 1, "EURUSD", "SP"),
                                  "topic", 
                                  true, 
                                  true, 
                                  "EURUSD") {
    }

   virtual void publish() override {}
   virtual void publish(Mutex::scoped_lock& datalock) const { datalock.unlock(); }
  
   void test(uint32 cnt) {
       OutrightSubscriptionContext& ctx = getContextAt(0);

       printf("== reload\n");
       reload();

       // bool& isValid = m_bnContexts[0]->m_tick->m_isValid;
       // FXOutrightQuote::Error& code = m_bnContexts[0]->m_tick->m_ErrorData.code;
       // boost::shared_ptr<StringBuilder>& reasoning = m_bnContexts[0]->m_tick->m_output.reasoning;

       InternalTPSTick tick;

       boost::shared_ptr< DataEvent<TPSGroupData> > groupDataEvent = boost::make_shared< DataEvent<TPSGroupData> >(boost::make_shared<TPSGroupData>(true, "valid reason", false, false, 1.0, true, true, true, true, true, true, true, true, true), ctx.m_input.groupData, FXGROUP_DATA_EVENT);

       boost::shared_ptr< DataEvent< TPSSpotSalesMarkupData > > spotSalesMarkupDataEvent = boost::make_shared< DataEvent< TPSSpotSalesMarkupData> >(boost::make_shared<TPSSpotSalesMarkupData>(true, "valid reason", SpreadPairData(1.0, 1.0, SpreadMode::PIPS), Nullable<SpreadPairData>(SpreadPairData(1.0, 1.0, SpreadMode::PIPS))), ctx.m_input.spot.spotSalesMarkup, SPOT_SALES_MARKUP_EVENT);

       boost::shared_ptr< DataEvent<TPSAXEFastMarketData> > fmDataEvent = boost::make_shared< DataEvent<TPSAXEFastMarketData> >(boost::make_shared<TPSAXEFastMarketData>(true, "valid reason", 1.2, "fm audit", false), ctx.m_input.spot.axeFastMarket, AXE_FAST_MARKET_EVENT);

       boost::shared_ptr< DataEvent<TPSSpotTraderMarkupData> > spotTraderMarkupDataEvent = boost::make_shared< DataEvent<TPSSpotTraderMarkupData> >(boost::make_shared<TPSSpotTraderMarkupData>(true, "valid reason", 0.7), ctx.m_input.spot.spotTraderMarkup, SPOT_TRADER_MARKUP_DATA_EVENT);

       boost::shared_ptr< DataEvent<TPSTradingLimitData> > tradingLimitDataEvent = boost::make_shared< DataEvent<TPSTradingLimitData> >(boost::make_shared<TPSTradingLimitData>(true, "valid reason", 0, 10000000000LL), ctx.m_input.spot.tradingLimits,  SPOT_LIMIT_EVENT);

       boost::shared_ptr< DataEvent<TPSGroupTradeVolumeData> > groupTradeVolumeDataEvent = boost::make_shared< DataEvent<TPSGroupTradeVolumeData> >(boost::make_shared<TPSGroupTradeVolumeData>(true, "valid reason", 0,0,0,0), ctx.m_input.spot.groupTradeVolume, GROUP_TRADE_VOLUME_EVENT);

       boost::shared_ptr< DataEvent<TPSIndicativeData> > indicativeDataEvent = boost::make_shared< DataEvent<TPSIndicativeData> >(boost::make_shared<TPSIndicativeData>(true, "valid reason", false, "not indicative"), ctx.m_input.spot.indicative, SPOT_INDICATIVE_EVENT);

       printf("== Ticking static data...\n");

       ctx.addEvent(groupDataEvent);
       ctx.addEvent(spotSalesMarkupDataEvent);
       ctx.addEvent(fmDataEvent);
       ctx.addEvent(spotTraderMarkupDataEvent);
       ctx.addEvent(tradingLimitDataEvent);
       ctx.addEvent(groupTradeVolumeDataEvent);
       ctx.addEvent(indicativeDataEvent);

       CLEAR();
       consistentCalculate();

       printf("== Ticking spot...\n");

       boost::shared_ptr<SpotEvent> spotEvent[2];
       spotEvent[0] = boost::make_shared<SpotEvent>(createSpotTick(1.4401,1.4402), ctx.m_input.spot.tick);
       spotEvent[1] = boost::make_shared<SpotEvent>(createSpotTick(1.5501,1.5502), ctx.m_input.spot.tick);
       m_expectedConfigId = spotEvent[0]->getData()->getConfigId();
       uint64 ts = Time().usecs();

       for (uint32 i = 0; i < cnt; ++i) {
           ctx.addEvent(spotEvent[i&1]);
           CLEAR();
           consistentCalculate();
       }
       uint64 ts2 = Time().usecs();
       printf("Total: %lld (cnt=%u, avg=%g)\n", ts2-ts, cnt, (ts2-ts)*1.0/cnt);
   }
};

//////////////////////////////////////////////////////////////////////////////////////////
// for BubbleNet testing

struct BnTestSubscription : public AXEBaseSubscription
{
    BnTestSubscription()
        : AXEBaseSubscription("subscriberId", "topic", true, true, 
                              boost::shared_ptr<OutrightSubscriptionReqInfo>(new FXOutrightRequestInfo(boost::make_shared<FXOutrightRequest>()))),
        m_bn("TestBubbleNet", NUM_VOLUMES) {
            for (int i = 0; i < NUM_VOLUMES; ++i) {
                m_bnContexts.push_back(boost::make_shared<AXEBubbleNetContext>(this, i, &m_bn));
                // m_bnContexts[i]->m_tick = new InternalTPSTick();
            }

            std::string ccyPair("EURUSD");
            std::string valueDate("SP");

            m_request.setCcyPair(ccyPair);
            m_request.setValueDate(valueDate);

            std::deque<FXOutrightRequest> requests;
            int64 userId = 1;
            int64 groupId = 1;

            for (int i = 0; i < NUM_VOLUMES; ++i) {
                requests.push_back(FXOutrightRequest(FXRequestId(userId, 1000+i),             // requestId
                                                     groupId,                                 // groupId
                                                     ccyPair,                                 // ccyPair
                                                     valueDate,                               // valueDate
                                                     (i+1) * 10000,                           // volume
                                                     FXOutrightRequest::BASE_VOLUME,          // volumeType
                                                     FXOutrightRequest::BOTH_SIDES,           // sideType
                                                     FXOutrightRequest::SALES_RATE,           // rateType
                                                     FXOutrightRequest::FULL,                 // requestType
                                                     Nullable<Decimal>(),                     // spotOverrideVolume
                                                     true,                                    // inCompetition
                                                     std::deque<FXTPSQualifier>(),            // qalifiers
                                                     Nullable<FXNDFFixingInfo>(),             // ndfFixingInfo
                                                     Nullable<Decimal>(),                     // antiSweepVolume
                                                     Nullable<FXSettlementAccount>(),         // settlementAcct
                                                     true,                                    // isAuditable
                                                     Nullable<FXOutrightSpreadPtsOverride>(), // spreadPtsOverride
                                                     Nullable<Decimal>()                      // spotRolloverRate
                                                    ));
            }
            m_request.setRequests(requests);
        }

    virtual int getNumVolumes() { return NUM_VOLUMES; }
    virtual bool isQuoteGoodToRepublish(SubscriptionContext* ctx) const { return true; }
    virtual void resetAudit() {}
    virtual void consolidateQuote(OutrightSubscriptionContext& ctx) {}
    virtual int64 getQuoteId() const { return 0; }
    virtual bool doSubscriptionManagerReload(const SubscriptionManager::DoReload& reload) const { return true; }
    virtual void addRequestNode(int ctxId, XmlNode& quote) const {}
    virtual void publish(Mutex::scoped_lock& datalock) const {}
    virtual void addQuoteNode(int ctxId, XmlNode& quote) const {}
    virtual ml::spec::fx::tps::quote::FXOutrightRequest& getRequestAt(uint32 idx) { return m_request.getRequests().at(idx); }

    void print() {
        printNodes();
        printNodeIndices();
        printRecalcMaps();
    }
    void printNodes() {
        std::cout << "=============== Nodes =================\n";
        std::vector< UntypedNode::Ptr >& nodes = m_bn.getNodes();
        for (int i = 0; i < (int)nodes.size(); ++i) {
            UntypedNode::Ptr& node = nodes[i];
            std::cout << "[" << std::setw(2) << i << "] " 
                << std::setw(30) << node->getName() 
                << ", id=" << std::setw(2) << node->getId() 
                << ", depth=" << std::setw(2) << node->getDepth();

            std::bitset<MAX_NODES>& depMask = *node->getDependencyMask();
            std::cout << to_string(depMask);

            std::bitset<MAX_NODES>& audMask = *node->getAuditMask();
            std::cout << " | " << to_string(audMask);

            std::cout <<  " ->";
            for (auto& d : node->getEffectNodes())
                std::cout << " " << d->getName();
            std::cout << "\n";
        } 
    }
    void printRecalcMaps() {
        std::cout << "============== RecalcMaps ==============\n";

        std::bitset<MAX_NODES>& gm = m_bn.getGlobalRecalcMap();
        std::cout << "GMP " << to_string(gm) << "\n";

        std::vector< std::bitset<MAX_NODES> >& recalcMaps = m_bn.getRecalcMaps();
        for (int i = 0; i < (int)recalcMaps.size(); ++i) {
            std::cout << "[" << i << "] " << to_string(recalcMaps[i]) << "\n";
        }
    }
    void printNodeIndices() {
        std::cout << "=============== Indices ================\n";
        int* indices = m_bn.getNodeIndices();
        for (int i = 0; i < (int)m_bn.getSize(); ++i)
            std::cout << "[" << i << "] " << indices[i] << "\n";
    }
    void printResults(std::vector< UntypedNode* >& results) {
        std::cout << "Results: ";
        for (int i = 0; i < (int)results.size(); ++i) {
            if (results[i]) {
                std::cout << "[" << results[i]->getName();

                const InternalTPSTick& bnOutput = *static_cast<ComputeNode*>(results[i])->doGet(*m_bnContexts[i]);
                // const InternalTPSTick& bnOutput = *m_bnContexts[i]->m_tick;

                if (bnOutput.getIsValid()) {
                    if (m_bnContexts[i]->m_output.m_output.m_error.code != FXOutrightQuote::E_NONE) {
                        std::cout << "{" << FXOutrightQuote::ErrorToString(m_bnContexts[i]->m_output.m_output.m_error.code);
                        std::cout << ":" << m_bnContexts[i]->m_output.m_output.m_error.reason << "}";
                    }
                    std::cout << "]";

                    // if (bnOutput.m_output.m_error.code != FXOutrightQuote::E_NONE) {
                    //     std::cout << "{" << FXOutrightQuote::ErrorToString(bnOutput.m_output.m_error.code);
                    //     std::cout << ":" << bnOutput.m_output.m_error.reason << "}";
                    // }
                    // std::cout << "]";

                } else {
                    if (bnOutput.m_ErrorData.code != FXOutrightQuote::E_NONE) {
                        std::cout << "(" << FXOutrightQuote::ErrorToString(bnOutput.m_ErrorData.code);
                        std::cout << ":" << bnOutput.m_ErrorData.reason;
                    } else {
                        std::cout << "(" << FXOutrightQuote::ErrorToString(bnOutput.m_RFQErrorData.code);
                        std::cout << ":" << bnOutput.m_RFQErrorData.reason;
                    }
                    std::cout << ")]";
                }
            } else {
                std::cout << "[_]";
            }
        }
        std::cout << "\n";

        std::cout << "\nAudit: ";
        for (int i = 0; i < (int)results.size(); ++i) {
            std::cout << "[" << i << "] ";
            if (results[i]) {
                std::cout << "lastNode: " << results[i]->getName();
                const InternalTPSTick& bnOutput = *static_cast<ComputeNode*>(results[i])->doGet(*m_bnContexts[i]);
                // const InternalTPSTick& bnOutput = *m_bnContexts[i]->m_tick;
                std::cout << ", reasoning:\n";
                if (m_bnContexts[i]->m_output.m_output.reasoning)
                    std::cout << m_bnContexts[i]->m_output.m_output.reasoning->str();
                // std::cout << bnOutput.m_output.reasoning->str();
            }
            std::cout << "\n";
        }

        std::cout << "\nNewAudit: ";
        for (int i = 0; i < (int)results.size(); ++i) {
            std::cout << "[" << i << "] ";
            if (results[i]) {
                std::cout << "lastNode: " << results[i]->getName();
                BnetReasoning* bnReasoning = ReasoningStore::allocBnet();
                m_bn.getNodeReasoning(*m_bnContexts[0], results[i], bnReasoning);
                char buf[64000];
                std::size_t len = 64000;
                char* p = buf;
                bnReasoning->getReasoning(p, len);
                buf[64000-len] = '\0';
                std::cout << ", reasoning:\n";
                std::cout << buf;
                ReasoningStore::freeBnet(bnReasoning);
            }
            std::cout << "\n";
        }

    }

    BubbleNet m_bn;
    std::vector< boost::shared_ptr<BubbleNetContext> > m_bnContexts;
    FXOutrightMassQuoteRequest m_request;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *                                                   InternalTPSTick
 *                                                          |
 *                                                          |
 *                                                          |
 *                      TPSGroupData                initializeTick                     TPSSpotSalesMarkupData
 *                            |                             |                                 |   |
 *                            |_____________________________|_________________________________|   |
 *                                                          |                                     |
 * TPSAXEFastMarketData  TPSSpotTraderMarkupData  verifySalesSpreadData                           |
 *          |                       |                       |                                     |
 *          |_______________________|_______________________|                                     |
 *                                  |                                                             |
 *                        verifyAXEFastMarketData          TPSTradingLimitData                    |
 *                                  |                            | |                              |
 *                                  |____________________________| |                              |
 *                                                 |               |                              |
 *              TPSGroupTradeVolumeData     verifySpotLimit        |                              |
 *                         |                       |               |                              |
 *                         |_______________________|_______________|                              |
 *                                                 |                                              |
 *                                       applyGroupTradeVolume      SpotTickWithDepth             |
 *                                                 |                       |                      |
 *                                                 |_______________________|                      |
 *                                                            |                                   |
 *                                                   applyAXEInterpolation                        |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                    applyAXEFastMarket                          |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                   applySpotMidConstraint                       |
 *                                                            |                                   |
 *                                                            |___________________________________|
 *                                                                         |
 *                                                                applyAXESalesSpread 
 *                                                                         |
 *                                                                         |
 *                                                                         |
 *                                                                verifyInvertedPrice      SpotIndicative
 *                                                                         |                      |
 *                                                                         |______________________|
 *                                                                                    |
 *                                                                            verifyPostCalculate
 */
struct TestDirect : public BnTestSubscription
{
    TestDirect() : BnTestSubscription() {
        buildGraph();
    }

    virtual void consolidateContextQuote(OutrightSubscriptionContext& ctx) {}
    virtual void consolidateFinalQuote() {}

    void buildGraph() {
        // source ndoes
        m_sInternalTPSTick = m_bn.addSourceNode<InternalTPSTick::CPtr>("InternalTPSTick");
        m_sSpotIndicative = m_bn.addSourceNode<TPSIndicativeData::Ptr>("SpotIndicative", TPSIndicativeData::Ptr());
        m_sSpotTickWithDepth = m_bn.addSourceNode<FXSpotTickWithDepth::Ptr>("SpotTickWithDepth", FXSpotTickWithDepth::Ptr());
        m_sTPSTradingLimitData = m_bn.addSourceNode<TPSTradingLimitData::Ptr>("TPSTradingLimitData", TPSTradingLimitData::Ptr());
        m_sTPSAXEFastMarketData = m_bn.addSourceNode<TPSAXEFastMarketData::Ptr>("TPSAXEFastMarketData", TPSAXEFastMarketData::Ptr());
        m_sTPSGroupData = m_bn.addSourceNode<TPSGroupData::Ptr>("TPSGroupData", TPSGroupData::Ptr());
        m_sTPSGroupTradeVolumeData = m_bn.addSourceNode<TPSGroupTradeVolumeData::Ptr>("TPSGroupTradeVolumeData", TPSGroupTradeVolumeData::Ptr());
        m_sTPSSpotTraderMarkupData = m_bn.addSourceNode<TPSSpotTraderMarkupData::Ptr>("TPSSpotTraderMarkupData", TPSSpotTraderMarkupData::Ptr());
        m_sTPSSpotSalesMarkupData = m_bn.addSourceNode<TPSSpotSalesMarkupData::Ptr>("TPSSpotSalesMarkupData", TPSSpotSalesMarkupData::Ptr());
        m_sTPSAutoHedgeVolumeData = m_bn.addSourceNode<TPSAutoHedgeVolumeData::Ptr>("TPSAutoHedgeVolumeData", TPSAutoHedgeVolumeData::Ptr());

        ////////////////////////////////////////////////////////////////////////////////////////

        m_cInitializeTick = m_bn.addComputeNode1<InternalTPSTick::CPtr>(*m_sInternalTPSTick, 
                                                                        InitializeTick(), 
                                                                        "initializeTick");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            BaseQuantity spotQty;
            BaseQuantity fwdQty;
            Decimal denomMultiplier(1.0);
            auto nodeCtx = boost::make_shared<InitializeTickContext>(req, 
                                                                     1,
                                                                     spotQty, 
                                                                     fwdQty, 
                                                                     "USD", 
                                                                     "USD", 
                                                                     denomMultiplier,
                                                                     true, 
                                                                     true, 
                                                                     "USDEUR", 
                                                                     Nullable<Decimal>(), 
                                                                     true, 
                                                                     true, 
                                                                     false,
                                                                     false,
                                                                     false
                                                                    );
            ctx->setNodeContext(*m_cInitializeTick, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cVerifySalesSpreadData = m_bn.addComputeNode3<InternalTPSTick::CPtr>(*m_cInitializeTick, 
                                                                               *m_sTPSGroupData,
                                                                               *m_sTPSSpotSalesMarkupData,
                                                                               VerifySalesSpreadData(), 
                                                                               "verifySalesSpreadData");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            auto nodeCtx = boost::make_shared<VerifySalesSpreadDataContext>(req,                            // request
                                                                            false,                          // isInternalSubscription
                                                                            true,                           // isSpotDirect
                                                                            ml::fx::common::Tenor::TT_SPOT, // tenorType
                                                                            false,                          // applySaleSpreadOverride
                                                                            false                           // applyRFQRateOverride
                                                                           );
            ctx->setNodeContext(*m_cVerifySalesSpreadData, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cVerifyAXEFastMarketData = m_bn.addComputeNode3<InternalTPSTick::CPtr>(*m_cVerifySalesSpreadData,
                                                                                 *m_sTPSAXEFastMarketData, 
                                                                                 *m_sTPSSpotTraderMarkupData,
                                                                                 VerifyAXEFastMarketData(), 
                                                                                 "verifyAXEFastMarketData");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            auto nodeCtx = boost::make_shared<VerifyAXEFastMarketDataContext>(req);
            ctx->setNodeContext(*m_cVerifyAXEFastMarketData, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }


        //
        m_cVerifySpotLimit = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cVerifyAXEFastMarketData, 
                                                                         *m_sTPSTradingLimitData, 
                                                                         VerifySpotLimit(), 
                                                                         "verifySpotLimit");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            Decimal usdSpotVolume = req.getVolume();
            auto nodeCtx = boost::make_shared<VerifySpotLimitContext>(req, usdSpotVolume);
            ctx->setNodeContext(*m_cVerifySpotLimit, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }


        //
        m_cApplyGroupTradeVolume = m_bn.addComputeNode3<InternalTPSTick::CPtr>(*m_cVerifySpotLimit,
                                                                               *m_sTPSGroupTradeVolumeData, 
                                                                               *m_sTPSTradingLimitData, 
                                                                               ApplyGroupTradeVolume(), 
                                                                               "applyGroupTradeVolume");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            OutrightInput input;
            auto nodeCtx = boost::make_shared<ApplyGroupTradeVolumeContext>(req,
                                                                            input,
                                                                            true,	// isVolumeInBase
                                                                            false	// isCBLREnabled
                                                                           );
            ctx->setNodeContext(*m_cApplyGroupTradeVolume, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cApplyAXEInterpolation = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cApplyGroupTradeVolume, 
                                                                               *m_sSpotTickWithDepth,
                                                                               ApplyAXEInterpolation(), 
                                                                               "applyAXEInterpolation");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            Decimal requestVolume = req.getVolume();
            auto nodeCtx = boost::make_shared<ApplyAXEInterpolationContext>(req, 
                                                                            requestVolume,
                                                                            true,			// isVolumeInBase
                                                                            false,			// isLegSubscription
                                                                            false,			// isGroupWithAlerts
                                                                            false			// isCBLREnabled
                                                                           );
            ctx->setNodeContext(*m_cApplyAXEInterpolation, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }


        //
        m_cApplyAXEFastMarket = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cApplyAXEInterpolation, 
                                                                            *m_sTPSAutoHedgeVolumeData,
                                                                            ApplyAXEFastMarket(), 
                                                                            "applyAXEFastMarket");
        m_quotedVolumes.quotedCcy = "EUR";
        m_quotedVolumes.volumeInBase = true;
        m_quotedVolumes.isCrossCcyPair = false;
        m_quotedVolumes.baseVolume = DecimalDecNumber(1000);
        m_quotedVolumes.termsVolume = DecimalDecNumber(1000);

        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            auto nodeCtx = boost::make_shared<ApplyAXEFastMarketContext>(req, false, true, true, m_quotedVolumes);
            ctx->setNodeContext(*m_cApplyAXEFastMarket, nodeCtx);
        }


        //
        m_cApplySpotMidConstraint = m_bn.addComputeNode1<InternalTPSTick::CPtr>(*m_cApplyAXEFastMarket, 
                                                                                ApplySpotMidConstraint(), 
                                                                                "applySpotMidConstraint");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            auto nodeCtx = boost::make_shared<ApplySpotMidConstraintContext>(req, 
                                                                             4,
                                                                             Decimal::RoundIncrement::TENTH,
                                                                             false,
                                                                             ApplyRounding());
            ctx->setNodeContext(*m_cApplySpotMidConstraint, nodeCtx);
        }


        //
        m_cApplyAXESalesSpread = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cApplySpotMidConstraint, 
                                                                             *m_sTPSSpotSalesMarkupData,
                                                                             ApplyAXESalesSpread(), 
                                                                             "applyAXESalesSpread");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            OutrightInput input;
            auto nodeCtx = boost::make_shared<ApplyAXESalesSpreadContext>(req, 
                                                                          input,
                                                                          false, 
                                                                          false
                                                                         );
            ctx->setNodeContext(*m_cApplyAXESalesSpread, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cVerifyInvertedPrice = m_bn.addComputeNode1<InternalTPSTick::CPtr>(*m_cApplyAXESalesSpread, 
                                                                             VerifyInvertedPrice(), 
                                                                             "verifyInvertedPrice");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            OutrightInput input;
            auto nodeCtx = boost::make_shared<VerifyInvertedPriceContext>(req, 
                                                                          true,
                                                                          input
                                                                          );
            ctx->setNodeContext(*m_cVerifyInvertedPrice, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }


        //
        m_cVerifyPostCalculate = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cVerifyInvertedPrice, 
                                                                             *m_sSpotIndicative,
                                                                             VerifyPostCalculate(), 
                                                                             "verifyPostCalculate");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            OutrightInput input;
            auto nodeCtx = boost::make_shared<VerifyPostCalculateContext>(req, 
                                                                          input,
                                                                          false,	// forceTenorCheck
                                                                          false		// applyRFQRateOverride
                                                                         );
            ctx->setNodeContext(*m_cVerifyPostCalculate, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }
        //
        m_bn.commitStructure(m_bnContexts);
    }

    void printTestSummary(const char* name, uint64_t start, uint64_t end, uint32_t cnt) {
        std::cout 
            << std::setw(50) << name 
            << std::setw(12) << cnt 
            << std::setw(12) << (end - start) 
            << std::setw(12) << (end - start) * 1.0 / cnt
            << "\n";
    }

    void prepare() {
        std::cout << "New BubbleNet\n";
        print();

        m_groupData = boost::make_shared<TPSGroupData>(true, "valid reason", false, false, Decimal(1.0), true, true, true, true, true, true, true, true, true);
        m_spotSalesMarkupData = boost::make_shared<TPSSpotSalesMarkupData>(true, "valid reason", 
                                                                           SpreadPairData(1.0, 1.0, SpreadMode::PIPS), 
                                                                           Nullable<SpreadPairData>(SpreadPairData(1.0, 1.0, SpreadMode::PIPS)));
        m_fmData = boost::make_shared<TPSAXEFastMarketData>(true, "valid reason", Decimal(1.2), "fm audit", false);
        m_spotTraderMarkupData = boost::make_shared<TPSSpotTraderMarkupData>(true, "valid reason", 0.7);
        m_tradingLimitData = boost::make_shared<TPSTradingLimitData>(true, "valid reason", 0, 10000000000LL);
        m_groupTradeVolumeData = boost::make_shared<TPSGroupTradeVolumeData>(true, "valid reason", 0,0,0,0);
        m_indicativeData = boost::make_shared<TPSIndicativeData>(true, "valid reason", false, "not indicative");
        m_autoHedgeVolumeData = boost::make_shared<TPSAutoHedgeVolumeData>(true, "valid reason", Decimal(1500), Decimal(1500));


        std::cout << "== Ticking static data...\n";

        for (int i = 0; i < (int)m_bnContexts.size(); ++i) {
            m_bn.tickSourceNode(m_bnContexts, i, *m_sInternalTPSTick, &m_tick);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSGroupData, m_groupData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSSpotSalesMarkupData, m_spotSalesMarkupData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSAXEFastMarketData, m_fmData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSSpotTraderMarkupData, m_spotTraderMarkupData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSTradingLimitData, m_tradingLimitData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSGroupTradeVolumeData, m_groupTradeVolumeData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sSpotIndicative, m_indicativeData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSAutoHedgeVolumeData, m_autoHedgeVolumeData);
        }

        printRecalcMaps();

        m_masks.resize(m_bn.getDepth(), true);
        m_results.resize(m_bn.getDepth());

        CLEAR();
        m_bn.flushTicks(m_bnContexts, m_masks, m_results);

        printResults(m_results);

        std::cout << "== Ticking spot...\n";
        m_spotTick = createSpotTick(1.4401, 1.4402);

        m_bn.tickSourceNode(m_bnContexts, 0, *m_sSpotTickWithDepth, m_spotTick);

        CLEAR();
        m_bn.flushTicks(m_bnContexts, m_masks, m_results);

        printResults(m_results);
        std::cout << "\n\n";
    }

    void test(uint32_t cnt) {
        prepare();

        //
        /*
        // this shows the bug caused by bn not propagating error
        do {
            TPSAXEFastMarketData::Ptr fmData2 = boost::make_shared<TPSAXEFastMarketData>(false, "invalid reason", 1.2, "fm audit", false);
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sTPSAXEFastMarketData, fmData2);
            m_bn.flushTicks(m_bnContexts, masks, results);
            printResults(results);

            m_bn.tickSourceNode(m_bnContexts, 0, *m_sSpotTickWithDepth, spotTick);
            m_bn.flushTicks(m_bnContexts, masks, results);
            printResults(results);
        } while (false);
        */


        // printRecalcMaps();
        // printResults(results);


        std::cout << std::setw(50) << "Updates" << std::setw(12) << "Ticks" <<  std::setw(12) << "usec" << std::setw(12) << "Avg" << "\n";
        usec_t ts = usecs();
        for (uint32_t i = 0; i < cnt; ++i) {
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sSpotTickWithDepth, m_spotTick);
            CLEAR();
            m_bn.flushTicks(m_bnContexts, m_masks, m_results);
        }
        printTestSummary("SpotTickWithDepth", ts.count(), usecs().count(), cnt);

        // printResults(results);

        ts = usecs();
        for (uint32_t i = 0; i < cnt; ++i) {
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sTPSTradingLimitData, m_tradingLimitData);
            m_bn.flushTicks(m_bnContexts, m_masks, m_results);
        }
        printTestSummary("TPSTradingLimitData", ts.count(), usecs().count(), cnt);

        ts = usecs();
        for (uint32_t i = 0; i < cnt; ++i) {
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sTPSSpotSalesMarkupData, m_spotSalesMarkupData);
            CLEAR();
            m_bn.flushTicks(m_bnContexts, m_masks, m_results);
        }
        printTestSummary("TPSSpotSalesMarkupData", ts.count(), usecs().count(), cnt);

        ts = usecs();
        for (uint32_t i = 0; i < cnt; ++i) {
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sSpotTickWithDepth, m_spotTick);
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sTPSTradingLimitData, m_tradingLimitData);
            CLEAR();
            m_bn.flushTicks(m_bnContexts, m_masks, m_results);
        }
        printTestSummary("SpotTickWithDepth+TPSTradingLimitData", ts.count(), usecs().count(), cnt);
    }

    void test_nodes(uint32_t cnt) {
        prepare();

        std::cout << std::setw(50) << "Node" << std::setw(12) << "Updates" <<  std::setw(12) << "usec" << std::setw(12) << "Avg" << "\n";
        std::vector< UntypedNode::Ptr >& nodes = m_bn.getNodes();
        std::vector< std::bitset<MAX_NODES> > recalcMaps;
        for (int i = 0; i < (int)m_bnContexts.size(); i++) {
            recalcMaps.push_back(std::bitset<MAX_NODES>());
            recalcMaps[i].flip();
        }

        for (int i = 0; i < (int)nodes.size(); ++i) {
            UntypedNode::Ptr& node = nodes[i];
            if (node->isRoot())
                continue;
            usec_t ts = usecs();
            for (uint32_t n = 0; n < cnt; ++n) {
                node->tick2(m_bnContexts, recalcMaps, i, m_results);
                // printRecalcMaps();
            }
            printTestSummary(node->getName().c_str(), ts.count(), usecs().count(), cnt);
        } 
    }


    SourceNode<InternalTPSTick::CPtr>::Ptr        m_sInternalTPSTick;
    SourceNode<TPSIndicativeData::Ptr>::Ptr       m_sSpotIndicative;
    SourceNode<FXSpotTickWithDepth::Ptr>::Ptr     m_sSpotTickWithDepth;
    SourceNode<TPSAXEFastMarketData::Ptr>::Ptr    m_sTPSAXEFastMarketData;
    SourceNode<TPSSpotTraderMarkupData::Ptr>::Ptr m_sTPSSpotTraderMarkupData;
    SourceNode<TPSSpotSalesMarkupData::Ptr>::Ptr  m_sTPSSpotSalesMarkupData;
    SourceNode<TPSGroupData::Ptr>::Ptr            m_sTPSGroupData;
    SourceNode<TPSGroupTradeVolumeData::Ptr>::Ptr m_sTPSGroupTradeVolumeData;
    SourceNode<TPSTradingLimitData::Ptr>::Ptr     m_sTPSTradingLimitData;
    SourceNode<TPSAutoHedgeVolumeData::Ptr>::Ptr  m_sTPSAutoHedgeVolumeData;

    ComputeNode::Ptr m_cInitializeTick;
    ComputeNode::Ptr m_cVerifySalesSpreadData;
    ComputeNode::Ptr m_cVerifyAXEFastMarketData;
    ComputeNode::Ptr m_cVerifySpotLimit;
    ComputeNode::Ptr m_cApplyGroupTradeVolume;
    ComputeNode::Ptr m_cApplyAXEInterpolation;
    ComputeNode::Ptr m_cApplyAXEFastMarket;
    ComputeNode::Ptr m_cApplySpotMidConstraint;
    ComputeNode::Ptr m_cApplyAXESalesSpread;
    ComputeNode::Ptr m_cVerifyInvertedPrice;
    ComputeNode::Ptr m_cVerifyPostCalculate;

    TPSGroupData::Ptr m_groupData;
    TPSSpotSalesMarkupData::Ptr m_spotSalesMarkupData;
    TPSAXEFastMarketData::Ptr m_fmData;
    TPSSpotTraderMarkupData::Ptr m_spotTraderMarkupData;
    TPSTradingLimitData::Ptr m_tradingLimitData;
    TPSGroupTradeVolumeData::Ptr m_groupTradeVolumeData;
    TPSIndicativeData::Ptr m_indicativeData;
    TPSAutoHedgeVolumeData::Ptr m_autoHedgeVolumeData;
    InternalTPSTick m_tick;
    FXSpotTickWithDepth::Ptr m_spotTick;

    std::vector< UntypedNode* > m_results;
    std::vector< bool > m_masks;

    OutrightInput::QuotedVolumes m_quotedVolumes;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 *                                                   InternalTPSTick
 *                                                          |
 *                                                          |
 *                                                          |
 *                      TPSGroupData                initializeTick                     TPSSpotSalesMarkupData
 *                            |                             |                                 |   |
 *                            |_____________________________|_________________________________|   |
 *                                                          |                                     |
 * TPSAXEFastMarketData  TPSSpotTraderMarkupData  verifySalesSpreadData                           |
 *          |                       |                       |                                     |
 *          |_______________________|_______________________|                                     |
 *                                  |                                                             |
 *                        verifyAXEFastMarketData          TPSTradingLimitData                    |
 *                                  |                            | |                              |
 *                                  |____________________________| |                              |
 *                                                 |               |                              |
 *              TPSGroupTradeVolumeData     verifySpotLimit        |                              |
 *                         |                       |               |                              |
 *                         |_______________________|_______________|                              |
 *                                                 |                                              |
 *                                       applyGroupTradeVolume  crossedSpotBase  crossedSpotTerm  |
 *                                                 |                     |             |          |
 *                                                 |_____________________|_____________|          |
 *                                                            |                                   |
 *                                                    verifyCrossSpot                             |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                     applyCrossSpot                             |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                   applyAXEFastMarket                           |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                            |                                   |
 *                                                    applySpotRounding                           |
 *                                                            |                                   |
 *                                                            |___________________________________|
 *                                                                         |
 *                                                                applyAXESalesSpread
 *                                                                         |
 *                                                                         |
 *                                                                         |
 *                                                                verifyInvertedPrice      SpotIndicative
 *                                                                         |                      |
 *                                                                         |______________________|
 *                                                                                    |
 *                                                                            verifyPostCalculate
 *
 */
struct TestCross : public BnTestSubscription
{
    TestCross() : BnTestSubscription() {
        buildGraph();
    }

    virtual void consolidateContextQuote(OutrightSubscriptionContext& ctx) {}
    virtual void consolidateFinalQuote() {}

    void buildGraph() {
        // source ndoes
        m_sInternalTPSTick = m_bn.addSourceNode<InternalTPSTick::CPtr>("InternalTPSTick");
        m_sSpotIndicative = m_bn.addSourceNode<TPSIndicativeData::Ptr>("SpotIndicative", TPSIndicativeData::Ptr());
        // m_sSpotTickWithDepth = m_bn.addSourceNode<FXSpotTickWithDepth::Ptr>("SpotTickWithDepth", FXSpotTickWithDepth::Ptr());
        m_sCrossedSpotBase = m_bn.addSourceNode<OutrightOutput>("CrossedSpotBase", OutrightOutput());
        m_sCrossedSpotTerm = m_bn.addSourceNode<OutrightOutput>("CrossedSpotTerm", OutrightOutput());
        m_sTPSTradingLimitData = m_bn.addSourceNode<TPSTradingLimitData::Ptr>("TPSTradingLimitData", TPSTradingLimitData::Ptr());
        m_sTPSAXEFastMarketData = m_bn.addSourceNode<TPSAXEFastMarketData::Ptr>("TPSAXEFastMarketData", TPSAXEFastMarketData::Ptr());
        m_sTPSGroupData = m_bn.addSourceNode<TPSGroupData::Ptr>("TPSGroupData", TPSGroupData::Ptr());
        m_sTPSGroupTradeVolumeData = m_bn.addSourceNode<TPSGroupTradeVolumeData::Ptr>("TPSGroupTradeVolumeData", TPSGroupTradeVolumeData::Ptr());
        m_sTPSSpotTraderMarkupData = m_bn.addSourceNode<TPSSpotTraderMarkupData::Ptr>("TPSSpotTraderMarkupData", TPSSpotTraderMarkupData::Ptr());
        m_sTPSSpotSalesMarkupData = m_bn.addSourceNode<TPSSpotSalesMarkupData::Ptr>("TPSSpotSalesMarkupData", TPSSpotSalesMarkupData::Ptr());
        m_sTPSAutoHedgeVolumeData = m_bn.addSourceNode<TPSAutoHedgeVolumeData::Ptr>("TPSAutoHedgeVolumeData", TPSAutoHedgeVolumeData::Ptr());

        ////////////////////////////////////////////////////////////////////////////////////////

        m_cInitializeTick = m_bn.addComputeNode1<InternalTPSTick::CPtr>(*m_sInternalTPSTick, 
                                                                        InitializeTick(), 
                                                                        "initializeTick");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            BaseQuantity spotQty;
            BaseQuantity fwdQty;
            Decimal denomMultiplier(1.0);
            auto nodeCtx = boost::make_shared<InitializeTickContext>(req, 
                                                                     1,
                                                                     spotQty, 
                                                                     fwdQty, 
                                                                     "USD", 
                                                                     "USD", 
                                                                     denomMultiplier,
                                                                     true, 
                                                                     true, 
                                                                     "USDEUR", 
                                                                     Nullable<Decimal>(), 
                                                                     true, 
                                                                     true, 
                                                                     false,
                                                                     false,
                                                                     false
                                                                    );
            ctx->setNodeContext(*m_cInitializeTick, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cVerifySalesSpreadData = m_bn.addComputeNode3<InternalTPSTick::CPtr>(*m_cInitializeTick, 
                                                                               *m_sTPSGroupData,
                                                                               *m_sTPSSpotSalesMarkupData,
                                                                               VerifySalesSpreadData(), 
                                                                               "verifySalesSpreadData");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            auto nodeCtx = boost::make_shared<VerifySalesSpreadDataContext>(req,                            // request
                                                                            false,                          // isInternalSubscription
                                                                            true,                           // isSpotDirect
                                                                            ml::fx::common::Tenor::TT_SPOT, // tenorType
                                                                            false,                          // applySaleSpreadOverride
                                                                            false                           // applyRFQRateOverride
                                                                           );
            ctx->setNodeContext(*m_cVerifySalesSpreadData, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cVerifyAXEFastMarketData = m_bn.addComputeNode3<InternalTPSTick::CPtr>(*m_cVerifySalesSpreadData,
                                                                                 *m_sTPSAXEFastMarketData, 
                                                                                 *m_sTPSSpotTraderMarkupData,
                                                                                 VerifyAXEFastMarketData(), 
                                                                                 "verifyAXEFastMarketData");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            auto nodeCtx = boost::make_shared<VerifyAXEFastMarketDataContext>(req);
            ctx->setNodeContext(*m_cVerifyAXEFastMarketData, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }


        //
        m_cVerifySpotLimit = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cVerifyAXEFastMarketData, 
                                                                         *m_sTPSTradingLimitData, 
                                                                         VerifySpotLimit(), 
                                                                         "verifySpotLimit");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            Decimal usdSpotVolume = req.getVolume();
            auto nodeCtx = boost::make_shared<VerifySpotLimitContext>(req, usdSpotVolume);
            ctx->setNodeContext(*m_cVerifySpotLimit, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }


        //
        m_cApplyGroupTradeVolume = m_bn.addComputeNode3<InternalTPSTick::CPtr>(*m_cVerifySpotLimit,
                                                                               *m_sTPSGroupTradeVolumeData, 
                                                                               *m_sTPSTradingLimitData, 
                                                                               ApplyGroupTradeVolume(), 
                                                                               "applyGroupTradeVolume");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            OutrightInput input;
            auto nodeCtx = boost::make_shared<ApplyGroupTradeVolumeContext>(req,
                                                                            input,
                                                                            true,	// isVolumeInBase
                                                                            false	// isCBLREnabled
                                                                           );
            ctx->setNodeContext(*m_cApplyGroupTradeVolume, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cVerifyCrossSpot = m_bn.addComputeNode3<InternalTPSTick::CPtr>(*m_cApplyGroupTradeVolume, 
                                                                         *m_sCrossedSpotBase,
                                                                         *m_sCrossedSpotTerm,
                                                                         VerifyCrossSpotRates(), 
                                                                         "verifyCrossSpot");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            Decimal requestVolume = req.getVolume();
            auto nodeCtx = boost::make_shared<VerifyCrossSpotRatesContext>(req, 
                                                                           false			// isGroupWithAlerts
                                                                          );
            ctx->setNodeContext(*m_cVerifyCrossSpot, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cApplyCrossSpot = m_bn.addComputeNode1<InternalTPSTick::CPtr>(*m_cVerifyCrossSpot, 
                                                                         ApplyAXECrossSpotRate(), 
                                                                         "applyCrossedSpot");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            Decimal requestVolume = req.getVolume();
            auto nodeCtx = boost::make_shared<ApplyAXECrossSpotRateContext>(req, 
                                                                            true,           // ccyXIsBaseInBaseLeg
                                                                            false,          // ccyXIsBaseInTermsLeg
                                                                            true,           // needDollarRates
                                                                            true,           // useDollarRateSubscriptions
                                                                            true,           // isSalesRate
                                                                            1,              // modelType
                                                                            true,			// gusPricingEnabled
                                                                            Decimal(0.5)    // crossTraderSpreadFactor
                                                                           );
            ctx->setNodeContext(*m_cApplyCrossSpot, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }


        //
        m_cApplyAXEFastMarket = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cApplyCrossSpot, 
                                                                            *m_sTPSAutoHedgeVolumeData,
                                                                            ApplyAXEFastMarket(), 
                                                                            "applyAXEFastMarket");
        m_quotedVolumes.quotedCcy = "EUR";
        m_quotedVolumes.volumeInBase = true;
        m_quotedVolumes.isCrossCcyPair = false;
        m_quotedVolumes.baseVolume = DecimalDecNumber(1000);
        m_quotedVolumes.termsVolume = DecimalDecNumber(1000);

        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            auto nodeCtx = boost::make_shared<ApplyAXEFastMarketContext>(req, false, true, true, m_quotedVolumes);
            ctx->setNodeContext(*m_cApplyAXEFastMarket, nodeCtx);
        }


        //
        m_cApplySpotRounding = m_bn.addComputeNode1<InternalTPSTick::CPtr>(*m_cApplyAXEFastMarket, 
                                                                                ApplySpotRounding(), 
                                                                                "applySpotRounding");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            auto nodeCtx = boost::make_shared<ApplySpotRoundingContext>(req, 
                                                                        4,
                                                                        Decimal::RoundIncrement::TENTH,
                                                                        false,
                                                                        ApplyRounding());
            ctx->setNodeContext(*m_cApplySpotRounding, nodeCtx);
        }


        //
        m_cApplyAXESalesSpread = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cApplySpotRounding, 
                                                                             *m_sTPSSpotSalesMarkupData,
                                                                             ApplyAXESalesSpread(), 
                                                                             "applyAXESalesSpread");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            OutrightInput input;
            auto nodeCtx = boost::make_shared<ApplyAXESalesSpreadContext>(req, 
                                                                          input,
                                                                          false, 
                                                                          false
                                                                         );
            ctx->setNodeContext(*m_cApplyAXESalesSpread, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }

        //
        m_cVerifyInvertedPrice = m_bn.addComputeNode1<InternalTPSTick::CPtr>(*m_cApplyAXESalesSpread, 
                                                                             VerifyInvertedPrice(), 
                                                                             "verifyInvertedPrice");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            OutrightInput input;
            auto nodeCtx = boost::make_shared<VerifyInvertedPriceContext>(req, 
                                                                          true,
                                                                          input
                                                                          );
            ctx->setNodeContext(*m_cVerifyInvertedPrice, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }


        //
        m_cVerifyPostCalculate = m_bn.addComputeNode2<InternalTPSTick::CPtr>(*m_cVerifyInvertedPrice, 
                                                                             *m_sSpotIndicative,
                                                                             VerifyPostCalculate(), 
                                                                             "verifyPostCalculate");
        for (auto& ctx : m_bnContexts) {
            const FXOutrightRequest& req = getRequestAt(ctx->getId());
            OutrightInput input;
            auto nodeCtx = boost::make_shared<VerifyPostCalculateContext>(req, 
                                                                          input,
                                                                          false,	// forceTenorCheck
                                                                          false		// applyRFQRateOverride
                                                                         );
            ctx->setNodeContext(*m_cVerifyPostCalculate, nodeCtx);
            // nodeCtx->tickStorage = ctx->m_tick;
        }
        //
        m_bn.commitStructure(m_bnContexts);
    }

    void printTestSummary(const char* name, uint64_t start, uint64_t end, uint32_t cnt) {
        std::cout 
            << std::setw(50) << name 
            << std::setw(12) << cnt 
            << std::setw(12) << (end - start) 
            << std::setw(12) << (end - start) * 1.0 / cnt
            << "\n";
    }
    void test(uint32_t cnt) {
        std::cout << "New BubbleNet\n";
        print();

        // bool& isValid = m_bnContexts[0]->m_tick->m_isValid;
        // FXOutrightQuote::Error& code = m_bnContexts[0]->m_tick->m_ErrorData.code;
        // boost::shared_ptr<StringBuilder>& reasoning = m_bnContexts[0]->m_tick->m_output.reasoning;


        InternalTPSTick tick;
        TPSGroupData::Ptr groupData = boost::make_shared<TPSGroupData>(true, "valid reason", false, false, 1.0, true, true, true, true, true, true, true, true, true);
        TPSSpotSalesMarkupData::Ptr spotSalesMarkupData = boost::make_shared<TPSSpotSalesMarkupData>(true, "valid reason", 
                                                                                                     SpreadPairData(1.0, 1.0, SpreadMode::PIPS), 
                                                                                                     Nullable<SpreadPairData>(SpreadPairData(1.0, 1.0, SpreadMode::PIPS)));
        TPSAXEFastMarketData::Ptr fmData = boost::make_shared<TPSAXEFastMarketData>(true, "valid reason", 1.2, "fm audit", false);
        TPSSpotTraderMarkupData::Ptr spotTraderMarkupData = boost::make_shared<TPSSpotTraderMarkupData>(true, "valid reason", 0.7);
        TPSTradingLimitData::Ptr tradingLimitData = boost::make_shared<TPSTradingLimitData>(true, "valid reason", 0, 10000000000LL);
        TPSGroupTradeVolumeData::Ptr groupTradeVolumeData = boost::make_shared<TPSGroupTradeVolumeData>(true, "valid reason", 0,0,0,0);
        TPSIndicativeData::Ptr indicativeData = boost::make_shared<TPSIndicativeData>(true, "valid reason", false, "not indicative");
        TPSAutoHedgeVolumeData::Ptr autoHedgeVolumeData = boost::make_shared<TPSAutoHedgeVolumeData>(true, "valid reason", Decimal(1500), Decimal(1500));

        std::cout << "== Ticking static data...\n";

        for (int i = 0; i < (int)m_bnContexts.size(); ++i) {
            m_bn.tickSourceNode(m_bnContexts, i, *m_sInternalTPSTick, &tick);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSGroupData, groupData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSSpotSalesMarkupData, spotSalesMarkupData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSAXEFastMarketData, fmData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSSpotTraderMarkupData, spotTraderMarkupData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSTradingLimitData, tradingLimitData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSGroupTradeVolumeData, groupTradeVolumeData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sSpotIndicative, indicativeData);
            m_bn.tickSourceNode(m_bnContexts, i, *m_sTPSAutoHedgeVolumeData, autoHedgeVolumeData);
        }

        printRecalcMaps();

        std::vector< UntypedNode* > results;
        std::vector< bool > masks(m_bn.getDepth(), true);
        results.resize(m_bn.getDepth());

        CLEAR();
        m_bn.flushTicks(m_bnContexts, masks, results);

        printResults(results);

        std::cout << "== Ticking spot...\n";
        FXSpotTickWithDepth::Ptr spotTick = createSpotTick(1.44011, 1.4402);

        // m_bn.tickSourceNode(m_bnContexts, 0, *m_sSpotTickWithDepth, spotTick);

        CLEAR();
        m_bn.flushTicks(m_bnContexts, masks, results);

        printResults(results);

        /*
        // this shows the bug caused by bn not propagating error
        do {
            TPSAXEFastMarketData::Ptr fmData2 = boost::make_shared<TPSAXEFastMarketData>(false, "invalid reason", 1.2, "fm audit", false);
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sTPSAXEFastMarketData, fmData2);
            m_bn.flushTicks(m_bnContexts, masks, results);
            printResults(results);

            m_bn.tickSourceNode(m_bnContexts, 0, *m_sSpotTickWithDepth, spotTick);
            m_bn.flushTicks(m_bnContexts, masks, results);
            printResults(results);
        } while (false);
        */


        // printRecalcMaps();
        // printResults(results);

        std::cout << "\n\n";

        std::cout << std::setw(50) << "Updates" << std::setw(12) << "Ticks" <<  std::setw(12) << "usec" << std::setw(12) << "Avg" << "\n";
        usec_t ts = usecs();
        for (uint32_t i = 0; i < cnt; ++i) {
            // m_bn.tickSourceNode(m_bnContexts, 0, *m_sSpotTickWithDepth, spotTick);

            CLEAR();
            m_bn.flushTicks(m_bnContexts, masks, results);
            // printRecalcMaps();
        }
        printTestSummary("SpotTickWithDepth", ts.count(), usecs().count(), cnt);

        // printResults(results);

        ts = usecs();
        for (uint32_t i = 0; i < cnt; ++i) {
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sTPSTradingLimitData, tradingLimitData);
            m_bn.flushTicks(m_bnContexts, masks, results);
        }
        printTestSummary("TPSTradingLimitData", ts.count(), usecs().count(), cnt);

        ts = usecs();
        for (uint32_t i = 0; i < cnt; ++i) {
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sTPSSpotSalesMarkupData, spotSalesMarkupData);
            CLEAR();
            m_bn.flushTicks(m_bnContexts, masks, results);
        }
        printTestSummary("TPSSpotSalesMarkupData", ts.count(), usecs().count(), cnt);

        ts = usecs();
        for (uint32_t i = 0; i < cnt; ++i) {
            // m_bn.tickSourceNode(m_bnContexts, 0, *m_sSpotTickWithDepth, spotTick);
            m_bn.tickSourceNode(m_bnContexts, 0, *m_sTPSTradingLimitData, tradingLimitData);
            CLEAR();
            m_bn.flushTicks(m_bnContexts, masks, results);
        }
        printTestSummary("SpotTickWithDepth+TPSTradingLimitData", ts.count(), usecs().count(), cnt);
    }

    SourceNode<InternalTPSTick::CPtr>::Ptr        m_sInternalTPSTick;
    SourceNode<TPSIndicativeData::Ptr>::Ptr       m_sSpotIndicative;
    // SourceNode<FXSpotTickWithDepth::Ptr>::Ptr     m_sSpotTickWithDepth;
    SourceNode<OutrightOutput>::Ptr               m_sCrossedSpotBase;
    SourceNode<OutrightOutput>::Ptr               m_sCrossedSpotTerm;
    SourceNode<TPSAXEFastMarketData::Ptr>::Ptr    m_sTPSAXEFastMarketData;
    SourceNode<TPSSpotTraderMarkupData::Ptr>::Ptr m_sTPSSpotTraderMarkupData;
    SourceNode<TPSSpotSalesMarkupData::Ptr>::Ptr  m_sTPSSpotSalesMarkupData;
    SourceNode<TPSGroupData::Ptr>::Ptr            m_sTPSGroupData;
    SourceNode<TPSGroupTradeVolumeData::Ptr>::Ptr m_sTPSGroupTradeVolumeData;
    SourceNode<TPSTradingLimitData::Ptr>::Ptr     m_sTPSTradingLimitData;
    SourceNode<TPSAutoHedgeVolumeData::Ptr>::Ptr  m_sTPSAutoHedgeVolumeData;

    ComputeNode::Ptr m_cInitializeTick;
    ComputeNode::Ptr m_cVerifySalesSpreadData;
    ComputeNode::Ptr m_cVerifyAXEFastMarketData;
    ComputeNode::Ptr m_cVerifySpotLimit;
    ComputeNode::Ptr m_cApplyGroupTradeVolume;
    // ComputeNode::Ptr m_cApplyAXEInterpolation;
    ComputeNode::Ptr m_cVerifyCrossSpot;
    ComputeNode::Ptr m_cApplyCrossSpot;
    ComputeNode::Ptr m_cApplyAXEFastMarket;
    // ComputeNode::Ptr m_cApplySpotMidConstraint;
    ComputeNode::Ptr m_cApplySpotRounding;
    ComputeNode::Ptr m_cApplyAXESalesSpread;
    ComputeNode::Ptr m_cVerifyInvertedPrice;
    ComputeNode::Ptr m_cVerifyPostCalculate;

    OutrightInput::QuotedVolumes m_quotedVolumes;
};


//////////////////////////////////////////////////////////////////////////////////////////////

void addReasoning(StringBuilder& sb, const char * format, ...)
{
    char buffer[4096];
    va_list ap;
    va_start(ap, format);

    int bufSize = vsnprintf(buffer, sizeof(buffer), format, ap);

    va_end(ap);

    if (bufSize > 0)
    {
        sb.append(buffer, bufSize);
        sb.append('\n');
    }
}

void bench_reasoning_store(uint32_t cnt)
{
	ml::common::math::Decimal d1(31400000000000000000.0);
	ml::common::math::Decimal d2(314000000000.0);
	ml::common::math::Decimal d3(314000000000.0);

	uint64_t ts = Time().usecs();
    for (uint32_t i = 0; i < cnt; ++i) { 
        ReasoningAppender* appender = ReasoningStore::allocAppender();
        appender->d1 = d1;
        appender->d2 = d2;
        appender->d3 = d3;
        NodeReasoning* node = ReasoningStore::allocNode();
        node->addAppender(appender);
        BnetReasoning* bnet = ReasoningStore::allocBnet();
        bnet->addNode(node);
        ReasoningStore::freeBnet(bnet);
    }

	uint64_t ts2 = Time().usecs();
    printf("%llu for %u (avg=%g)\n", ts2-ts, cnt, (ts2-ts)*1.0/cnt);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void bench_decimal()
{
    Decimal d(3.1415926);
    Decimal d2(3.1415926);
    Decimal d3(3.1415926);
    uint64 ts = Time().usecs();
    uint32_t count = 1000000;
    // const char* s;
    for (uint32_t i = 0; i < count; ++i) {
        StringBuilder sb;
        addReasoning(sb, "hello, %s %s %s", get_temp_cstr(d), get_temp_cstr(d2), get_temp_cstr(d3));
    }
    printf("bench_decimal: %lld (cnt=%u)\n", Time().usecs()-ts, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////

const uint32 log_count = 1000000;

void logger()
{
    ::usleep(10000);
    std::string msg("this is a message");
    uint64 ts1 = Time().usecs();
    for (uint32 i = 0; i < log_count; ++i)
        LINFO("This is a reasonable test message %d %s", 23434, msg.c_str());
    uint64 ts2 = Time().usecs();
    printf("total=%lld cnt=%u avg=%g\n", ts2-ts1, log_count, (ts2-ts1)*1.0/log_count);
}

void bench_log()
{
    std::thread log1(logger);
    std::thread log2(logger);
    std::thread log3(logger);
    std::thread log4(logger);
    log1.join();
    log2.join();
    log3.join();
    log4.join();
}


void test_reasoning_store()
{
    ::usleep(1000000);
    for (uint32_t i = 0; i < 10000; i++) {
        NodeReasoning* node0 = ReasoningStore::allocNode();

        NodeReasoning* node1 = ReasoningStore::allocNode();
        printf("+++ refcnt1=%d\n", node1->refcnt.load());
        node1->addAppender(ReasoningStore::allocAppender());
        node1->appenders[0]->s1 = std::string("hi");

        NodeReasoning* node2 = ReasoningStore::allocNode();
        printf("+++ refcnt2=%d\n", node2->refcnt.load());
        node2->addAppender(ReasoningStore::allocAppender());
        node2->addAppender(ReasoningStore::allocAppender());
        node2->addAppender(ReasoningStore::allocAppender());
        node2->appenders[0]->s1 = std::string("one");
        node2->appenders[0]->s2 = std::string("two");
        node2->appenders[2]->s2 = std::string("three");

        NodeReasoning* node3 = ReasoningStore::allocNode();
        printf("+++ refcnt3=%d\n", node3->refcnt.load());
        node3->addAppender(ReasoningStore::allocAppender());
        node3->addAppender(ReasoningStore::allocAppender());
        node3->addAppender(ReasoningStore::allocAppender());
        node3->addAppender(ReasoningStore::allocAppender());
        node3->addAppender(ReasoningStore::allocAppender());
        node3->addAppender(ReasoningStore::allocAppender());
        node3->appenders[4]->s2 = std::string("5");
        node3->appenders[4]->s1 = std::string("##$#$#");

        ReasoningStore::freeNodes(&node0, 1);
        ReasoningStore::freeNodes(&node1, 1);
        ReasoningStore::freeNodes(&node2, 1);
        ReasoningStore::freeNodes(&node3, 1);

        ::usleep(1);
    }
}

#define NELEM(a) (sizeof(a)/sizeof(a[0]))
void test_rs() 
{
    std::thread* threads[32];
    for (std::size_t i = 0; i < NELEM(threads); i++)
        threads[i] = new std::thread(test_reasoning_store);

    for (std::size_t i = 0; i < NELEM(threads); i++)
        threads[i]->join();
}

void bn_tester(uint32_t cnt)
{
    TestDirect n1;
    n1.test(cnt);
}

typedef std::thread* PTHREAD;

void test_bn(uint32_t cnt, int numThreads)
{
    PTHREAD* threads = new PTHREAD[numThreads];
    for (int i = 0; i < numThreads; ++i)
        threads[i] = new std::thread(std::bind(bn_tester, cnt));
    for (int i = 0; i < numThreads; ++i) {
        threads[i]->join();
        delete threads[i];
    }
    delete [] threads;
}

void test_bn_nodes(uint32_t cnt)
{
    TestDirect t;
    t.test_nodes(cnt);
}

std::string gen_audit()
{
    return std::string("hello, value");
}

void test_quote_cache()
{
    // typedef boost::function0<std::string> AuditGenFunc;
    // typedef StalingMap2<AuditGenFunc, 500> QuoteCache;
    // QuoteCache cache(200000);
    // cache.init();
    // QuoteCache::Item item;
    // int64 ts = Time().usecs();
    // printf("sizeof(AuditGenFunc)=%d\n", sizeof(AuditGenFunc));
    // printf("sizeof(boost::shared_ptr<...>)=%d\n", sizeof(boost::shared_ptr<AXEBaseSubscription>));
    // printf("sizeof(Item)=%d\n", sizeof(QuoteCache::Item));
    // int cnt = 2000000;
    // for (int i = 0; i < cnt; ) {
    //     for (int j = 0; j < 1000; ++j, ++i) {
    //         cache.schedule(1, (int64)i, gen_audit, Time()+1000000, item);
    //         // if (!cache.get(1, (int64)i, item))
    //         //     printf("######################## lookup failed!!!\n");
    //     }
    //     ::usleep(10);
    // }
    // printf("%d items cached in %lld usec, size=%u\n", cnt, Time().usecs()-ts, cache.size());
}

void printUsage()
{
    printf("bntest [a|b|c|r] <count> [#threads]\n");
    printf("     a     Audit\n");
    printf("     b     BubbleNet\n");
    printf("     n     BubbleNet nodes\n");
    printf("     c     Calculate\n");
    printf("     r     Reasoning\n");
}

#define PACK_DEC(d, p, len)             \
    PACK_INT(d.m_value.digits,p,len);   \
    PACK_INT(d.m_value.exponent,p,len); \
    *p++ = d.m_value.bits, --len;       \
    memcpy(p, d.m_value.lsu, sizeof(d.m_value.lsu)); \
    p += sizeof(d.m_value.lsu);         \
    len -= sizeof(d.m_value.lsu)

#define UNPACK_DEC(d, p, len)           \
    UNPACK_INT(d.m_value.digits,p,len); \
    UNPACK_INT(d.m_value.exponent,p,len);\
    d.m_value.bits = *p++, --len;       \
    memcpy(d.m_value.lsu, p, sizeof(d.m_value.lsu));\
    p += sizeof(d.m_value.lsu);         \
    len -= sizeof(d.m_value.lsu)

#define PACK_SHORT(i, p, len)           \
    *(short*)(p) = i;                   \
    p += sizeof(short);                 \
    len -= sizeof(short)

#define UNPACK_SHORT(i, p, len)         \
    i = *(short*)p;                     \
    p += sizelf(short);                 \
    len -= sizeof(short)

#define PACK_INT(i, p, len)             \
    *(int*)(p) = i;                     \
    p += sizeof(int);                   \
    len -= sizeof(int)

#define UNPACK_INT(i, p, len)           \
    i = *(int*)p;                       \
    p += sizelf(int);                   \
    len -= sizeof(int)

#define PACK_LONG(l, p, len)            \
    *(long*)(p) = l;                    \
    p += sizeof(long);                  \
    len -= sizeof(long)

#define UNPACK_LONG(l, p, len)          \
    l = *(long*)p;                      \
    p += sizelf(long);                  \
    len -= sizeof(long)

#define PACK_STR(s, p, len)             \
    do {                                \
        int sz = s.length();            \
        PACK_SHORT(sz, p, len);         \
        if (sz) {                       \
            memcpy(p, s.data(), sz);    \
            p += sz;                    \
            len -= sz;                  \
        }                               \
    } while (0)

#define UNPACK_STR(s, p, len)           \
    do {                                \
        int sz = s.length();            \
        PACK_INT(sz, p, len);           \
        if (sz) {                       \
            memcpy(p, s.data(), sz);    \
            p += sz;                    \
            len -= sz;                  \
        }                               \
    } while (0)

#define PACK_BOOLS(app, p, len)         \
    *(char*)p = (app.f4 << 3) | (app.f3 << 2) | (app.f2 << 1) | app.f1; \
    ++p; --len

#define UNPACK_BOOLS(app, p, len)       \
    app.f4 = (*p) & 8;                  \
    app.f3 = (*p) & 4;                  \
    app.f2 = (*p) & 2;                  \
    app.f1 = (*p) & 1;                  \
    ++p; --len


void pack_appender(ReasoningAppender& app, char* p, size_t len)
{
    PACK_DEC(app.d1, p, len);
    PACK_DEC(app.d2, p, len);
    PACK_DEC(app.d3, p, len);
    PACK_DEC(app.d4, p, len);
    PACK_INT(app.i1, p, len);
    PACK_INT(app.i2, p, len);
    PACK_LONG(app.l1, p, len);
    PACK_LONG(app.l2, p, len);
    PACK_STR(app.s1, p, len);
    PACK_STR(app.s2, p, len);
    PACK_BOOLS(app, p, len);
}

void test_audit_ser()
{
    int n = 1000000;
    char buf[2048];
    size_t len;
    char* p;
    ReasoningAppender app;
    app.s1 = "hello";
    app.s2 = "world";

    int64 ts = Time().usecs();
    for (int i = 0; i < n; ++i) {
        p = buf;
        len = 2048;
        pack_appender(app, p, len);
    }
    printf("%lld\n", Time().usecs()-ts);
}

int AppMain() { return 0; }
int main(int argc, char* argv[])
{
    ml::common::math::DecimalDecNumber d("3E+302");
    decNumber& v = const_cast<decNumber&>(d.getValue());
    printf("sizeof(decNumberUnit)=%d\n", sizeof(v.lsu[0]));
    printf("sizeof(decNumber.lsu)=%d\n", sizeof(v.lsu));
    printf("sizeof(decNumber)=%d\n", sizeof(decNumber));
    printf("DECNUMUNITS=%d\n", DECNUMUNITS);

    char buf[1024];
    printf("v=%s\n", decNumberToString(&v, buf));
    printf("v=%s\n", decNumberToEngString(&v, buf));

    // test_audit_ser();
    // return 0;

   //  ml::common::math::DecimalDecNumber d;
   //  std::cout << d.toString() << "\n";
   //
   //  decNumber& v = const_cast<decNumber&>(d.getValue());
   //
   //  printf("sizeof(decNumberUnit)=%d\n", sizeof(v.lsu[0]));
   //
   //  v.digits = 6;
   //  v.exponent = -5;
   //  v.bits = 0;
   //  v.lsu[0] = 529;
   //  v.lsu[1] = 111;
   //  v.lsu[2] = 318;
   //  v.lsu[3] = 30;
   //  v.lsu[4] = 645;
   //  v.lsu[5] = 941;
   //  v.lsu[6] = 152;
   //  v.lsu[7] = 11;
   //  v.lsu[8] = 0;
   //  v.lsu[9] = 19279;
   //  v.lsu[10] = 51135;
   //  v.lsu[11] = 6085;
   //  v.lsu[12] = 63478;
   //  v.lsu[13] = 19468;
   //  v.lsu[14] = 51135;
   //  v.lsu[15] = 954;
   //  std::cout << d.toString() << "\n";
   //
   //  v.digits = 48;
   //  v.exponent = 207;
   //  v.bits = 0;
   //  v.lsu[0] = 69;
   //  v.lsu[1] = 962;
   //  v.lsu[2] = 33;
   //  v.lsu[3] = 231;
   //  v.lsu[4] = 199;
   //  v.lsu[5] = 466;
   //  v.lsu[6] = 395;
   //  v.lsu[7] = 61;
   //  v.lsu[8] = 907;
   //  v.lsu[9] = 80;
   //  v.lsu[10] = 95;
   //  v.lsu[11] = 470;
   //  v.lsu[12] = 544;
   //  v.lsu[13] = 948;
   //  v.lsu[14] = 221;
   //  v.lsu[15] = 141;
   //  std::cout << d.toString() << "\n";
   //
   // return 0;

    // test_quote_cache();
    // return 0;

    // printf("sizeof(InputData)=%d\n", sizeof(InputData));
    // InputData i1;
    // i1.model = 1;
    // InputData i2;
    // int64 ts1 = Time().usecs();
    // volatile int n;
    // int total = 0;
    // for (int i = 0; i < 2000000; ++i) {
    //     i2 = i1;
    //     n = i2.model;
    //     total += n;
    // }
    // int64 ts2 = Time().usecs();
    // printf("%lld %g %d\n", ts2-ts1, (ts2-ts1)*1.0/2000000, total); 
    // return 0;

    printf("sizeof(OutrightInput)=%d\n", sizeof(OutrightInput));
    printf("sizeof(OutrightOutput)=%d\n", sizeof(OutrightOutput));
    printf("sizeof(InternalTPSTick)=%d\n", sizeof(InternalTPSTick));
    printf("sizeof(BubbleNetOutput)=%d\n", sizeof(BubbleNetOutput));
    printf("sizeof(BnetReasoning)=%d\n", sizeof(BnetReasoning));
    printf("sizeof(NodeReasoning)=%d\n", sizeof(NodeReasoning));
    printf("sizeof(ReasoningAppender)=%d\n", sizeof(ReasoningAppender));
    // test_rs();
    // return 0;

    // bench_decimal();
    // return 0;
    // bench_log();
    // return 0;

    if (argc < 3) {
        printUsage();
        return 0;
    }

    uint32_t cnt = strtoul(argv[2], 0, 10);

    int numThreads = 1;
    if (argc == 4)
       numThreads = (int)strtoul(argv[3], 0, 10);

    if (argv[1][0] == 'a') {
        // test audit
    } else if (argv[1][0] == 'b') {
        // test bubblenet
        test_bn(cnt, numThreads);
    } else if (argv[1][0] == 'n') {
        // test bubblenet nodes
        test_bn_nodes(cnt);
    } else if (argv[1][0] == 'c') {
        // test calc
        boost::shared_ptr<SubsTestSubscription> subs = boost::make_shared<SubsTestSubscription>();
        subs->test(cnt);
    } else if (argv[1][0] == 'r') {
        bench_reasoning_store(cnt);
    }
    return 0;
}


