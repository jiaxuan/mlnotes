###################################################################################

# Background

About trade source:


	// The FX Trade Source
	class FXTradeSource version 3.0 apiexposed

	// source enum
	enum Source
		SOURCE_GUI,
		SOURCE_FIX,
		SOURCE_OTHER,
		SOURCE_FXALL,				// fxall_gateway
		SOURCE_RISKTRANSFER,		// fx_sierra_trade_interface
		SOURCE_CFX,					// cfx_gateway
		SOURCE_ORDER_FILL_MANUAL,	// fx_order_capture_server
		SOURCE_SALES_BOOK_SWEEP,
		SOURCE_ORDER_FILL_AUTO,		// fx_order_capture_server
		SOURCE_TRADE_AGGREGATOR,	// retired/fx_trade_aggregator
		SOURCE_MARKUPRISKPOSX,		// fx_trade_server (bookReverseTrade as result of FXTraceContract::replaceSpotForward, or 
									// bookMarkupPositionTransferTrade as the result of bookTrade)
		SOURCE_FXWEB_PNLROLL,		// bana_fix_gateway if quoteId="PLROLL", see SessinOrderManager_NewOrderRequest.cpp
		SOURCE_ACE,					// retired/ace_gateway
		SOURCE_BO_OPS,				// retired/mercury_interface
		SOURCE_FIX_BANA,			// bana_fix_gateway
		SOURCE_PORTAL,
		SOURCE_OSE_FX_ESP,
		SOURCE_IMIX,				// CFETS, bana_fix_gateway, see CFETSTradeProcessor.cpp
		SOURCE_FXTRANSACT,
		SOURCE_FIX_ESP,				// bana_fix_gateway, MarketMaker
		SOURCE_SMART_AUTO,			// fx_smart_order_capture_server
		SOURCE_SMART_MANUAL			// fx_smart_order_capture_server


Used to keep track of progress:

	enum STEP
		START =0,
		VALUEDATE_VALIDITY,
		SANITY,
		TIME_EXPIRY,
		QTY_VERIFY,
		QUOTE_VERIFY,
		PERMISSIONS,
		TRADING_OPEN,
		LIMITS,
		PROFIT,
		RISK,
		CREDIT,
		AXE_BROADCAST,
		PERSIST,
		BROADCAST,
		TRADING_GROUP_VERIFY,
		QUOTING_GROUP_VERIFY,
		END_MARKER

	// used when processing a trade to hint at what checks to do in relation
	// to the relate delay parameter.

	enum DELAY_RATE_CHECK
		NO_RATE_DELAY_CHECK = 0,
		DELAY_RATE_DO_OTHER_CHECKS,
		DO_RATE_CHECK_ONLY

Markup book 

	class MarkupBook version 1.0
		groupId // group id
		bookId 	// markup book for spots and forwards

Error book to keep track of cancel and replace trades:

	class ErrorBook version 1.0
		groupId // group id
		bookId 	// error book for cancel and replace trades

This is used in checking:

	// This class allows a trading group to have the pricing group overridden on a per currency pair basis
	class PricingGroupMappingOverride version 1.0
		ccyPair 		// currency pair
		tradingGroupId	// the trading group
		pricingGroupId 	// the pricing group (the configuration used to generate a shared stream price)


## Internally generated trades

Internally generated offset sales trades are marked by **client ID**:

	FXTradeContractImpl::generateOffsetSalesTrade(...)
		salesAccountTrade->setClientId(Format("RskTrd-%lld-TF", originalTrade->getId()));

Internally generated canel and replace trades are marked by special values of userRef2:

	const std::string userRef2ForAmend="<MLFX:Amend>";
	const std::string userRef2ForCancel="<MLFX:Cancel>";

	FXSpotForwardTrade::Ptr FXTradeContractImpl::getReverseTrade(...)
		reversalTrade->setUserRef2(Nullable<String>(userRef2ForCancel.c_str()));
		reversalTrade->setUserRef3(getCancelReplaceUserRef3(originalTrade));

where userRef3 has the format of "first=<tradeId>; pref=<originalTradeId>", see
`FXTradeContractImpl::getCancelReplaceUserRef3`.

## Programming style

Some functions, such as bookTrade, is invoked multiple times in a single trade 
flow, to perform different tasks depending on state information. A very poor style.



####################################################################################

# Allocation (fx_trade_server/Allocation.rfc)

## Introduction:

Allocation is the ability to split a trade among a number of settlement accounts. 
This is done by booking the position out of a parent take onto a number of child trades. 

There are two types of allocations - pre allocation and post allocation.

A pre-allocation is the booking of a parent trade and its childern all at the same time.

Post allocation is the booking of childern trade onto a parent trade after it has been booked but before its settlement.

This document covers restrictions, trade flow logic, known issues, and feature needed for AVT retirement.

All functionality covered in this document applies to both Outright and Swaps.


## Restrictions:

1. Total sum criteria:  net position of all childern trades == net position of parent trade.
The sum of childern trade amount must exactly match that of the parent trade. The sum 
implies the direction of each trade is taken into account. Rounding logic is in place to ensure 
that childern amounts are rounded in a fair statistical approach while adhering to the total sum criteria.
This rounding logic is cover in section D.
	
2. Settlement Account Location criteria: The settlement account of parent and childern trades must 
managed by the same location. This is a restriction imposed by the FXTS system.
  * Pre Allocation specific restrictions:
    - N/A
  * Post Allocation:
    - Cut off Date criteria: Current trading date < settlement date. For swaps the near leg settlement date is used.
	- State criteria: The Parent Trade must have a State of FXTS CONFIRMED or FXTS AMENDED to before a post allocation is initiated.
	

## Booking Methods:	

- Previously quoted
  * MLFX GUI :
    1. Pre Allocations through :
	  a. Quick Deal Screen
	  b. Trade Loader Screen
	2. Post Allocations through the blotter:
  * FXall:
    1. Pre Allocations
    2. Post Allocations NOT SUPPORTED for FXall/AVT clients.
        
## Rounding:
 
All trade quantity amounts must be rounded. 
The rounding of childern trade amounts can cause the total sum to no longer add up to the parent trade.
 	
Therefore the following rounding adjust logic is in place:
Let minUnit equal the minimum representable rounded value.     		

When rounding: 

1.	Keep track to the childern with the most rounded up/down amounts.
2.	Calculate the sum of remainders of the childern rounding operations. Use +ive for rounding up and -ive for down.
3.	If the sum of remainders is -ive then for each minUnit in the sum add minUnit to the most rounded down childern amounts. 
	A childern amount is only adjusted once.
	Vice verse for +ive sum of remainders.


## State transitions:
 
### Pre Allocation:
 	
	Parent:	CREATED        -> EXECUTED              -> ORDEROFFLAY      -> SENT_BACK_OFFICE -> FXTS_ALLOCATED
	Child:  CREATED        -> EXECUTED              -> SENT_BACK_OFFICE -> FXTS_CONFIRMED
 	
### Post Allocation:
 	
	Parent: FXTS_CONFIRMED -> ALLOCATION_REQUESTED  -> FXTS_ALLOCATED
	Child:  CREATED	       -> EXECUTED              -> SENT_BACK_OFFICE -> FXTS_CONFIRMED


		
## Risk Management:
 
### Position
 	
Parent Trades are risk managed the same way as non allocated trades.
Childern trades should have no impact on panther positions and are passed straight though to FXTS.
 	
### Location:
		
The settlement account of parent and childern trades must be managed by the same location. 
This is a restriction imposed by the FXTS system.
		
When a parent trade is post allocated, all new childern trades are sent to the same location 
where the parent and childern are managed. This is done regardless of the current active 
region. This is the reason for the post allocation cut off date criteria.
 	


## Known Issues:
   
Post Allocations NOT SUPPORTED for FXall/AVT clients.


   
## Required Changes:
 
Post Allocations need to be supported for certain FXall/AVT clients before AVT can be retired.
 


#########################################################################################

# Services 

It provides the following services:

- FXTradeContractStub
- FXTradeSplitServerContractStub
- FXTradeBulkContractStub


	contract FXTradeContract version 1.0 apiexposed

        // book a single spot forward trade
        void bookSpotForwardAsync(FXSpotForwardIns instruction);

        // book at market single spot forward trade
        void bookAtMarketSpotForwardAsync(FXSpotForwardIns instruction , string valueDate);

	    // book a single swap trade
        void bookSwapTradeAsync(FXSwapTradeIns instruction);

        // book a single swap trade
        void bookAtMarketSwapAsync(FXSwapTradeIns instruction);

     	// for UI, book a single spot forward trade
        FXSpotForwardTrade bookSpotForward( FXSpotForwardIns instruction);

        // book a single spot forward AtMarket trade
        FXSpotForwardTrade bookAtMarketSpotForward( FXSpotForwardIns instruction , string valueDate);

        // book a single swap trade
        FXSwapTrade bookSwapTrade(FXSwapTradeIns instruction);

        // book a single swap trade for roll operation
        FXSwapTrade bookSwapTradeForRoll(FXSwapTradeIns instruction, int64 parentAggId);

        // book a single swap AtMarket trade
        FXSwapTrade bookAtMarketSwap(FXSwapTradeIns instruction);
        
        // book pre -split spot forward trades 
        FXSpotForwardTrade bookSplitSpotForwardTrades(  
             FXSpotForwardIns instruction, list<FXSpotForwardTradeAllocation> acctQtylist );  

        // book pre -split Swap trades 
        FXSwapTrade bookSplitSwapTrades(  
             FXSwapTradeIns instruction, list<FXSwapTradeAllocation> acctQtylist );
       
        // book pre -split Swap trades 
        FXSwapTrade bookSplitSwapTradesWithSpotForwardAllocations(  
             FXSwapTradeIns instruction, list<FXSwapTradeAllocation> acctQtyListSwap, list<FXSpotForwardTradeAllocation> acctQtyListNear, list<FXSpotForwardTradeAllocation> acctQtyListFar );
       
        // book pre -split spot forward at market trades 
        FXSpotForwardTrade bookAtMarketSplitSpotForwardTrades(  
             FXSpotForwardIns instruction, string valueDate, list<FXSpotForwardTradeAllocation> acctQtylist );
        
         // book pre -split spot forward at market trades 
        FXSwapTrade bookAtMarketSplitSwapTrades(  
             FXSwapTradeIns instruction, list<FXSwapTradeAllocation> acctQtylist );

        // book sales trade between account and book
        FXSpotForwardTrade bookOffsettingSalesAccountTrade(
             int64 tradeId, FXSettlementAccount salesAccount, int64 overrideAccountId, string valueDate, decimal custSpot, 
             decimal traderAllin, decimal custAllin, decimal preTradeMid, boolean holdTrade);

        // book sales trade between account and book
        void bookOffsettingSalesAccountTradeAsync(
             int64 tradeId, FXSettlementAccount salesAccount, int64 overrideAccountId, string valueDate, decimal custSpot, 
             decimal traderAllin, decimal custAllin, decimal preTradeMid, boolean holdTrade);
		
		// do a credit check of an offset trade
        FXSpotForwardTrade checkOffsetCredit(
             int64 tradeId, FXSettlementAccount salesAccount, int64 overrideAccountId, string valueDate, decimal custSpot, 
             decimal traderAllin, decimal custAllin, decimal preTradeMid, boolean holdTrade);
		
        // replace a single spot forward trade
        void replaceSpotForwardAsync(int64 tradeId, FXSpotForwardIns instruction);

        // asynch version - book a pre-split spot forward trade
        void bookSplitSpotForwardTradesAsync(FXSpotForwardIns instruction, list<FXSpotForwardTradeAllocation> acctQtylist);

        // asynch version - book a pre-split Swap trade 
        void bookSplitSwapTradesAsync(FXSwapTradeIns instruction, list<FXSwapTradeAllocation> acctQtylist );

		// asynch version - near and far leg can be split separately 
		void bookSplitSwapTradesWithSpotForwardAllocationsAsync(FXSwapTradeIns instruction, list<FXSwapTradeAllocation> acctQtyListSwap, list<FXSpotForwardTradeAllocation> acctQtyListNear, list<FXSpotForwardTradeAllocation> acctQtyListFar);

        // asynch version - book at market single spot forward trade
        void bookSplitAtMarketSpotForwardAsync(FXSpotForwardIns instruction , string valueDate, list<FXSpotForwardTradeAllocation> acctQtylist);

	    // book a preAlloc market swap trade
        void bookSplitAtMarketSwapTradeAsync(FXSwapTradeIns instruction, list<FXSwapTradeAllocation> acctQtylist);  

        // on broadcast of a single spot forward trade
        void forceCreditPassSpotForwardTradeAsync(FXSpotForwardTrade trade, list<FXSpotForwardTrade> splitTrades);
        				
        // on broadcast of a single swap trade
        void forceCreditPassSwapTradeAsync(FXSwapTrade trade , list<FXSwapTrade> splitTrades);




	// This contract represents the interface exposed by the FX Trade Split Server.
	contract FXTradeSplitServerContract version 1.0 
		// Perform a async post split on the FXSpotForwardTrade then unlocks the trade.
        void postSplitFXSpotForwardTradeAsync(
                FXSpotForwardTrade trade,
                list<FXSpotForwardTradeAllocation> spotFwdAlloclist,
                RequesterId requester, string region);

        // Perform a async even/uneven post allocation split on an FXSwapTrade then unlocks the trade.
        void postSplitFXSwapTradeAsync(
                FXSwapTrade trade,
                list<FXSwapTradeAllocation> evenSwapAlloclist,
                list<FXSpotForwardTradeAllocation> nearOnlySpotAlloclist,
                list<FXSpotForwardTradeAllocation> farOnlySpotAlloclist,
                RequesterId requester, string region);

		// Perform a post split on a FXSpotForwardTrade then unlocks the trade
        PostSplitOperationStatus postSplitFXSpotForwardTrade(
                FXSpotForwardTrade trade, list<FXSpotForwardTradeAllocation> acctQtylist, 
                RequesterId requester, string region);

        // Perform a sync even/uneven post allocation split on an FXSwapTrade then unlocks the trade.
        PostSplitOperationStatus postSplitFXSwapTrade(
                FXSwapTrade trade,
                list<FXSwapTradeAllocation> evenSwapAlloclist,
                list<FXSpotForwardTradeAllocation> nearOnlySpotAlloclist,
                list<FXSpotForwardTradeAllocation> farOnlySpotAlloclist,
                RequesterId requester, string region);


	// This contract used by UI to book bulk trades
	contract FXTradeBulkContract version 1.0
        // booking a bulk list of spot forward trades 
        void bookBulkSpotForwardAsync( list< FXSpotForwardIns > instructions);

        // booking a bulk list of swap trades 
        void bookBulkSwapTradeAsync( list< FXSwapTradeIns > instructions);
        
        // booking a bulk list of spot forward splits
        void bookBulkSplitSpotForwardAsync( list< BulkSpotForwardSplit > instructions );

        // booking a block trade with block quote
        void bookBlockAsync( list< BulkSpotForwardSplit > instructions, FXBlockQuote quote );

        // booking a bulk list of swap splits
        void bookBulkSplitSwapTradeAsync( list< BulkSwapSplit > instructions );                            

        // booking a bulk list of spot forward trades 
        list< FXSpotForwardTrade > bookBulkSpotForward( list< FXSpotForwardIns > instructions);

        // booking a bulk list of swap trades 
        list< FXSwapTrade > bookBulkSwapTrade( list< FXSwapTradeIns > instructions);
        
        // booking a bulk list of spot forward splits
        list< FXSpotForwardTrade > bookBulkSplitSpotForward( list< BulkSpotForwardSplit > instructions );

        // booking a block trade with block quote
        list< FXSpotForwardTrade > bookBlock( list< BulkSpotForwardSplit > instructions, FXBlockQuote quote );

        // booking a bulk list of swap splits
        list< FXSwapTrade > bookBulkSplitSwapTrade( list< BulkSwapSplit > instructions );                            


#########################################################################################

# Checkers

## Credit Checker

	bool CreditChecker::run(...)
	
		checkExecuted = false;

		check registry "/ENVIRONMENT/fx_trade_server/", "disableCreditCheck" , DISABLE_CREDIT_CHECK);
		if( disableCreditCheck )
			return true;

		if (trade.getSettleAccount().getAccountType() == FXSettlementAccount::TRADERBOOK) // from trade book
			if (trade.settleAccountOverrideId != NULL) // is there an override ?
				BOSettlementAccount::Ptr overrideAccount <- BOSettlementAccount[trade.settleAccountOverrideId];
				if (overrideAccount && overrideAccount->getType() == BOSettlementAccount::TRADERBOOK)
					return true;
			else
				return true;

		// the original quote
		FXOutrightQuote& tradeQuote = trade.getQuote();

		// pass for certain quotes
		if (tradeQuote.getType() in [NEGOTIATED, MANUAL, CREDIT_FAIL_OVERRIDEN])
			return true;

		checkExecuted = true;

		// check allocations
		if (allocations.size() > 0)
			// create requests
			CreditCheckRequest::PtrList requests;
			foreach(alloc, allocations)
				CreditCheckRequest::Ptr request = getCreditCheckRequest(trade, *alloc, carveOut);
				requests.push_back(request);
			// send requests, using tradeId as bulkId
			return sendBulkRequest(requests, response, responses, Format::int64ToString(trade.getId()), log, checkExecuted);

		//If no allocations, just check the trade
		CreditCheckRequest::Ptr request = getCreditCheckRequest(trade, FXSpotForwardTradeAllocation::Ptr(), carveOut);
		return CreditCheckUtility::checkCredit(*request) == CreditCheckResponseCode::SUCCESS)

CreditCheckUtility uses credit server service, **synchronously**:

	checkCredit(...)
		return CreditClient::getInstance().checkCredit(...)

	cancel(...)
		return CreditClient::instance().cancel(...)

## Economic Checker

What is reject pricing ?

	EconomicChecker::Result EconomicChecker::run(FXSpotForwardTrade& trade, logs)
		TradeProperties<FXSpotForwardTrade> properties(trade, TradePropertiesBase::SPOT_FORWARD);

		// check market arbitrage
		if (!doMktArbitrageCheck(trade, logs, properties))
			...
			return EconomicChecker::Result(profitSuccess, rateSuccess, tpsError, risk, mktArbitrageCheckPassed, ttlExpired);

		if (properties.useRejectPricing())
			try {
				// Alternative pricing Check
				LazySpotPrice lazyMarketPrice(trade, properties);
				return runImplRejectPricing(trade, logs, properties, lazyMarketPrice);
			} catch(... &e)
				...
				return EconomicChecker::Result(profitSuccess, rateSuccess, tpsError, risk, mktArbitrageCheckPassed, ttlExpired);

		LazySpotPrice lazyMarketPrice(trade, properties);
		return runImpl(trade, logs, properties, lazyMarketPrice);


	EconomicChecker::Result runImpl(const Trade& trade,
			std::deque<ml::spec::fx::trade::FXTradeInfo>& logs,
			const TradeProperties<Trade> properties,
			LazyMarketPrice<Trade>& lazyMarketPrice)

		// Do both checks even if the first fails, so we can analyse results

		// check profit limit, ProfitLimitAlgorithms.cpp
		const bool profitSuccess = performProfitLimitCheck(properties, trade, lazyMarketPrice, logs);

		// check rate tolerance, RateToleranceAlgorithms.cpp
		const bool rateSuccess = performRateToleranceCheck(properties, trade, lazyMarketPrice, logs);

		const bool tpsError = (!profitSuccess || !rateSuccess) && lazyMarketPrice.isPricingError();
		const bool ttlExpiredError = lazyMarketPrice.isTTLExpiryError();
		enum EconomicChecker::Result::eRisk risk = EconomicChecker::Result::RISK_UNKNOWN;
		const bool mktArbitrageCheckPassed = true;

		if (properties.algorithmChoice == TradePropertiesBase::MID_BASED_ALGORITHM)
			risk = properties.isTradeRiskDecreasing() 
				? EconomicChecker::Result::RISK_DECREASING
				: EconomicChecker::Result::RISK_INCREASING;
		return EconomicChecker::Result(profitSuccess, rateSuccess, tpsError, risk, mktArbitrageCheckPassed, ttlExpiredError);


Profit limit:

	std::string performProfitLimitCheckAgainstBidOffer(const Properties& properties,
													  const Trade& trade,
													  const Decimal& marketPrice,
													  enum TradePropertiesBase::eConfigurationField cfField)
		const Decimal dealPrice = properties.lookupMarketDealPrice(); // simple, look at quote
		const Decimal minimumProfitInUSD = properties.lookupProfitLimit(cfField) [
			// check the following tables
			FXUserProfitLimits
			FXGroupProfitLimits
			FXChannelProfitLimits
			FXGlobalProfitLimits
		]
		const Decimal profitInUSD = calculateProfitInUSD(...) [
			diff(tradeValue, marketValue) * quantity
			then convert to USD by looking up conversion rate in localdb::FXSpotUSDRate
		]
		const bool passed = (profitInUSD >= minimumProfitInUSD);


	// the limits are defined like

	class FXUserProfitLimits version 1.0
		userId 		 		// user id
		groupId 	 		// group id
		volumeBandId		// volume band
		usdMinimumProfit 	// positive minimum profit, negative maximum loss in usd per trade		
    	usdMinimumProfitRiskIncreasing // Profit Limit in $USD for trades 'risk increasing'
    	usdMinimumProfitRiskDecreasing // Profit Limit in $USD for trades 'risk decreasing'


Rate tolerance:

   std::string performRateToleranceCheckAgainstBidOffer(const TradeProperties<FXSpotForwardTrade>& properties,
                                                        LazyMarketPrice<FXSpotForwardTrade>& lazyMarketPrice,
                                                        const FXSpotForwardTrade& trade)

    	const bool clientBuys = properties.getMarketDirection();
    	const Decimal dealPrice = properties.lookupMarketDealPrice();
    	const Decimal marketPrice = lazyMarketPrice.lookupTradeSpecificMarketPrice();
    	const Decimal basisPoints = properties.lookupRateTolerance(TradePropertiesBase::CF_BIDOFFER_FIELD) [
    		// checks the following tables
    		FXUserRateTolerance
    		FXGroupRateTolerance
    		FXChannelRateTolerance
    		FXGlobalRateTolerance
    	]
    	const std::string marketPriceStr = clientBuys ? "traderAsk" : "traderBid";
    	const Decimal tolerance = marketPrice * basisPoints / Decimal::TEN_THOUSAND;
    	const bool passed = clientBuys ? (dealPrice >= marketPrice - tolerance)
    	                               : (dealPrice <= marketPrice + tolerance);

    	// Rate tolerance looks like this
		class FXUserRateTolerance version 1.0
			userId 		 		// user id
			groupId 	 		// group id
			bpRateTolerance 	// positive for min profit, if negative max loss per trade
			delayRateToleranceCheck // ms the trade must exist before check,
									// 0 means check immediately.
	    	bpRateToleranceRiskIncreasing // in basis points for trades 'risk increasing'
	    	bpRateToleranceRiskDecreasing // in basis points for trades 'risk decreasing'


`TradeProperties::isTradeRiskDecreasing` consults skew provider:

	bool SkewProvider::isTradeRiskDecreasing(const FXSpotForwardTrade& trade, std::string& explanation) const
		// A trade is considered "risk decreasing" if two conditions apply:
		// - it's volume is less than or equal to that of the max tier volume
		// - the sign of the skewfactor for the *first* tier is positive (for a clientBuy) or negative (for a clientSell)

		const SkewSubscription & subscription = subscriptions.lookupSubscription(marketCurrencyPair);
		const bool volumeWithinMaxTier = subscription.getSkewFactor(volume);
		if (volumeWithinMaxTier) { // within max ?
			const boost::optional<SkewFactor> firstTierSkewFactor = subscription.getSkewFactor(0);
			const Decimal skewFactor = marketDirectionClientBuys ? firstTierSkewFactor->second : firstTierSkewFactor->first;
			const bool riskDecreasing = marketDirectionClientBuys
									   ? skewFactor.isNegative()	// ask skew factor
									   : skewFactor.isPositive(); 	// bid skew factor
		}

#########################################################################################

# Book spot forward

	// to handle incoming requests
	FXTradeContractImpl tradeContract(*dispatcher.getDispatcher(), backOfficeAcknowledger, 
		swapBackOfficeAcknowledger, regionStr);

Incoming request, spot/forward:

	void FXTradeContractImpl::bookSpotForwardAsync(FXSpotForwardIns& instruction)
		bookSpotForward( instruction );

	boost::shared_ptr<FXSpotForwardTrade>
	FXTradeContractImpl::bookSpotForward(FXSpotForwardIns& instruction)
		
		ml::spec::fx::trade::FXSpotForwardTrade::Ptr result;
		result = fxTradeGenerator::instance().getTrade< FXSpotForwardTrade >();

		// broadcast trade instruction
		if (m_broadcastTradeIns)
			m_broadcastProxy.bookSpotForward( instruction);

		std::string error;
		try {
			const Nullable<FXSpotForwardTradeAllocation::PtrList> sfAllocsNearFarNull;
			processTrade<...>(instruction , result, false, FXSpotForwardTradeAllocation::PtrList(), 
				sfAllocsNearFarNull, sfAllocsNearFarNull );
		} catch (...) { error <- ... }
		
		if (!error.empty()) 
			result = fxTradeGenerator::instance().getTrade<FXSpotForwardTrade>();
			...
			FXSpotForwardTrade::PtrList emptyList;
			tradeBroadcaster.run( *result, emptyList); // broadcast error result

		return result;


Process trade:

	template<...>
	bool FXTradeContractImpl::processTrade( const Inst& tradeInstruction,
											typename T::Ptr & trade,
											bool withSplits,
											TAllocation::PtrList& acctQtyPtrList,
											FXSpotForwardTradeAllocation::PtrList& sfAllocsNear,
											FXSpotForwardTradeAllocation::PtrList& sfAllocsFar)

	if (!checkUserIdAndDup<>(tradeInstruction, trade) )
		return false;

	// turn off checking for smart order
	if (tradeInstruction.getSource() == [ FXTradeSource::SOURCE_SMART_AUTO, FXTradeSource::SOURCE_SMART_MANUAL])
		performChecks = false;

	STEP step = START;
	return bookTrade<...>(trade, tradeInstruction, errorCode, withSplits, acctQtyPtrList, 
		sfAllocsNear, sfAllocsFar, step, performChecks, DELAY_RATE_DO_OTHER_CHECKS);


Check user id and duplicate trade, note that when user id is invalid, the code returns
without broadcasting any result (bug ? to verify, send a trade instruction with user id set to -1
in bana_fix_gateway):

	bool FXTradeContractImpl::checkUserIdAndDup( const Inst& tradeIns ,typename T::Ptr & trade)

	if( tradeIns.getRequest().getUser() == -1 )
		...
		return false; // bug ?

	// Build a Trade object from the instruction
	setTradeAttributes( tradeIns, trade);
	trade->setOrderType(tradeIns.getOrderType());

	std::string clientId = trade->getClientId().c_str();
	bool  dupCheckRes  = getDupChecker(trade).run(*trade, log) [
		DuplicateChecker::run(trade, ...) {
			TradeCache::instance().exists(trade) [
				uses separate caches for spot and swap
				each trade is indexed by (trade.tradeDate, trade.clientId, trade.userId)
			]
		}
	]

	if ( !dupCheckRes )
		...
		typename T::PtrList emptyList;
		tradeBroadcaster.run( *trade, emptyList);
		return false;
	return true;

Most of processing happens here:

	template<> bool FXTradeContractImpl::bookTrade (
			  T::Ptr &trade
			, TChildren::PtrList &splitTrades
			, FXSpotForwardTrade::PtrList& splitSwapSFTrades
			, const Inst& tradeIns
			, int32 errorCodeIntVal
			, bool withSplits
			, TAllocation::PtrList& acctQtyPtrList
			, FXSpotForwardTradeAllocation::PtrList& sfAllocsNear
			, FXSpotForwardTradeAllocation::PtrList& sfAllocsFar
			, STEP& step
			, const bool performCheck
			, DELAY_RATE_CHECK delayRateCheck
			, const bool creditCheckExecutedBeforeTimer
			, CreditCheckResponseType::PtrList& responses
			, CreditCheckResponse::PtrList& nearResponses
			, CreditCheckResponse::PtrList& farResponses)

	if (performCheck)

		// 1. Don't perform credit, profit, or limit checks for order fill trades, 
		//    which come from fx_trade_capture_server
		// 2. Customer offset trades have clientId ending with "-TF", perform credit check only

		bool isOrderFill       = FXTradingUtils::isOrderFillTrade(*trade);
		bool isCustOffset		 = ends_with(trade->getClientId(),"-TF");

		bool doCreditCheck     = !isOrderFill;
		bool doProfitCheck     = !isOrderFill && !isCustOffset;
		bool doLimitsCheck     = !isOrderFill && !isCustOffset;
		bool doPermissionCheck = !isOrderFill && !isCustOffset;
		bool doQuoteVerify     = !isCustOffset;

		// should rate checking be delayed ? Note that a delayed check is done
		// in two legs. The first leg executes the if block to do all the 
		// checking except for rate and then schedule a timer task to do
		// the second leg, which will execute the else block that does
		// rate checking only


		// 1. first leg of delayed rate check, check everything except for rate

		if (delayRateCheck == DELAY_RATE_DO_OTHER_CHECKS && doProfitCheck)
			const int32 rateToleranceDelay = economicChecker.getDelayMs(*trade);

			if (rateToleranceDelay == 0)
				res = performChecksOnTrade(...);
			else 
				if (performChecksOnTrade(...))
					const int64 remainingDelay = rateToleranceDelay * milliSecondsToMicroseconds - delayed;
					if (remainingDelay > 0)
						// set a timer to do the delayed credit check
						TimerTask_ptr task(new TimerTask(boost::bind(&FXTradeContractImpl::bookTradeFromTimer, this, args), timeout));
						m_timer.schedule(task);
						return false;
					else
						res = performChecksOnTrade(...);
						if(!res)
							if (creditCheckExecuted)
								creditChecker.cancel(*trade, log, acctQtyPtrList, sfAllocsNear, sfAllocsFar);
							creditCheckExecuted = false;


		// 2. second leg of delayed rate check, check rate only

		else if(delayRateCheck == DO_RATE_CHECK_ONLY)
			res = performChecksOnTrade(...);
			// reverse the credit carve-out
			if (!res && creditCheckExecutedBeforeTimer)
				creditChecker.cancel( *trade, log, acctQtyPtrList, sfAllocsNear, sfAllocsFar);
			creditCheckExecuted = false;

		// 3. regular full check without any delay

		else
			res = performChecksOnTrade(...);

		if (!res && creditCheckExecuted)
			creditChecker.cancel( *trade, log, acctQtyPtrList, sfAllocsNear, sfAllocsFar);
			creditCheckExecuted = false;


	// checking done, start booking the trade

	// set execution book id
	if (tradeIns.source in [SOURCE_SMART_AUTO, SOURCE_SMART_MANUAL]) {

		// Use hedge book id as execution book id for smart orders
		trade->setExecutionBookId( RiskRegion[m_region].hedgeBookId );

		// risk region is defined as:
		class RiskRegion version 1.0
			riskRegionCode			// The Risk region - TK , LN , NY  
			backOfficeRegion		// backOfficeRegion
			interRegionShortName	// interRegionShortName
			mercuryQueueName		// mercuryQueueName
			canManageRisk			// True for - TK , LN , NY
			hedgeBookId				// BOSettlementAccountId for Hedge Book
			executionBookId 		// BOSettlementAccountId for execution Book
			eTraderBookId 			// BOSettlementAccountid for eTrader Book
			smartRiskRoutingBook	// Smart risk routing book. used to route risk to Sierra BO.

	} else {

		Nullable<int64> markupBookId;
		if (getMarkupBook(tradeIns..., markupBookId))
			//set markup book without passing this trade to risk manager
			trade->setExecutionBookId(markupBookId);
			trade->setIsPanCcy1RiskMan(false);
			trade->setIsPanCcy2RiskMan(false);
			trade->setManagedBy(FXDeskType::BACK_TO_BACK);
	}

	// in performChecksOnTrade, TradeTimeExpiryChecker (step=TIME_EXPIRY) may
	// set time out error, check for it.

	if ( trade->getErrorNumber() == T::E_TIME_EXPIRED_ERROR  &&
			( trade->getSource() == [ FXTradeSource::SOURCE_FIX || FXTradeSource::SOURCE_FIX_BANA] ) )
		creditChecker.cancel( *trade, log, acctQtyPtrList, sfAllocsNear, sfAllocsFar);
		tradeBroadcaster.run( *trade, splitTrades, splitSwapSFTrades );
		return res;


	// check for duplicate

	if( !withSplits )
		bool  dupCheckRes = getDupChecker(trade).run( *trade, log);
		if(!dupCheckRes)
			creditChecker.cancel( *trade, log, acctQtyPtrList, sfAllocsNear, sfAllocsFar);
			tradeBroadcaster.run( *trade, splitTrades, splitSwapSFTrades );
			return res;


	if( withSplits )
		ml::spec::fx::trade::RequesterId requesterId( trade->getUserId(), trade->getGroupId() );

		// Create the Split Trades
		std::deque<int64> tradeIds;
		createSplitTrades( * trade, acctQtyPtrList, sfAllocsNear, sfAllocsFar, splitTrades, 
			splitSwapSFTrades, tradeIds, responses, nearResponses, farResponses, requesterId);
	else
		foreach (rsp : responses) // CreditCheckResponse.spec
			trade->setCreditCheckApprover(rsp->getCreditCheckApprover());

			// CreditCheckApprover.spec
			enum CreditCheckApprover version 1.0 
				DAP_RTLC,
				MLFX_RTCM,
				MANUAL,
				NONE

	// Broadcast to AXE RM

	FXTradeServerUtil::setState(*trade, res ? FXTradeState::EXECUTED : FXTradeState::FAILED);
	step = AXE_BROADCAST;
	updateIdVersion(*trade);

	if(withSplits)
		// set parentId, quote and state of children trades
		foreach(sIter, splitTrades)
			(*sIter)->setParentId(trade->getId());
			(*sIter)->setQuote(trade->getQuote());

		foreach(sSFIter, splitSwapSFTrades)
			(*sSFIter)->setParentId(trade->getId());
			// (*sSFIter)->setQuote(trade->getQuote());

		FXTradeServerUtil::setSplitState<T>(splitTrades, res ? FXTradeState::EXECUTED : FXTradeState::FAILED);
		FXTradeServerUtil::setSplitState<FXSpotForwardTrade>(splitSwapSFTrades, res ? FXTradeState::EXECUTED : FXTradeState::FAILED);

		trade->setIsAllocationParent(true);
	else
		trade->setIsAllocationParent(false);

	adjustQuoteUsingDealtRateIfConfigured(*trade);

	if (trade->getBatchId().isNull())
		m_axeBroadcaster.run(*trade, splitTrades, splitSwapSFTrades);
	else
		//for bulk trades, broadcast separate bulk trade to AXE if the flag is  set to false
		if ( !FXTradingUtils::isNetSSPAXEBroadcastingEnabled() )
			m_axeBroadcaster.run(*trade, splitTrades, splitSwapSFTrades);


	// Persist the Trade

	step = PERSIST;
	//Restrict post trade allocations on Mercury Aggregatable trades.
	if(FXTradingUtils::isEligibleForAggregation(trade) && FXTradingUtils::isBlotterAggregationEnabled() )
		trade->setTradeAggState(T::AGG_STATE_HOLDABLE);

	TradeEnrichment::run(*trade);
	getTradePersistor(trade).run( *trade, log, false);

	if(	withSplits )
		foreach(sIter, splitTrades)
			TradeEnrichment::run(**sIter);
		foreach(sIter, splitSwapSFTrades)
			TradeEnrichment::run(**sIter);

		FXTradeServerUtil::setSplitState<T>(splitTrades, res ? FXTradeState::EXECUTED : FXTradeState::FAILED);
		FXTradeServerUtil::setSplitState<FXSpotForwardTrade>(splitSwapSFTrades, res ? FXTradeState::EXECUTED : FXTradeState::FAILED);

		getTradePersistor(trade).run(splitTrades);

		if(!splitSwapSFTrades.empty())
			getTradePersistor(*splitSwapSFTrades.begin()).run(splitSwapSFTrades);


	// broadcast

	step = BROADCAST;
	
	// Resend - if no response from back office server.

	if (FXTradeServerUtil::checkState( *trade,  FXTradeState::SENT_BACK_OFFICE) )
		if(splitSwapSFTrades.empty())
			// is this any diff from tradeBroadcaster ?
			getBackOfficeAcknowledger(trade).sendTrade(trade, splitTrades, false); [

				FXTradeBroadcastContractProxy proxy( *dispatcher.getDispatcher());				
				proxy.onSpotForward(*trade, splitTrades);
				if( doMonitor ) monitor(trade, splitTrades);
			]
		else
			getBackOfficeAcknowledger(trade).sendTrade(trade, splitTrades, splitSwapSFTrades, false);
	else
		tradeBroadcaster.run( *trade, splitTrades, splitSwapSFTrades ); [
			FXTradeBroadcastContractProxy proxy( m_dispatcher);
			proxy.onSpotForward( trade, splitTrades);			
		]


	// book markup position transfer trade (???)

	if(res)
		// TODO: what does this do ? 
		return bookMarkupPositionTransferTrade(trade, tradeIns, withSplits);

	return res;


Checking spot/outright:

	bool FXTradeContractImpl::performChecksOnTrade(  FXSpotForwardTrade::Ptr& trade,
												const FXSpotForwardIns& tradeIns,
												FXSpotForwardTrade::ErrorNum& errorCode,
												STEP& step,
												bool withSplits,
												const FXSpotForwardTradeAllocation::PtrList& acctQtyPtrList,
												FXSpotForwardTradeAllocation::PtrList& sfAllocsNear,
												FXSpotForwardTradeAllocation::PtrList& sfAllocsFar,
												std::deque<FXTradeInfo>& log,
												CreditCheckResponse::PtrList & responses,
												CreditCheckResponse::PtrList & nearResponses,
												CreditCheckResponse::PtrList & farResponses,
												bool& checkExecuted,
												bool doCreditCheck,
												bool doProfitCheck,
												bool doLimitsCheck,
												bool doQuoteVerify,
												bool doPermissionCheck)

	bMarkUpTrade = getMarkupBook(   tradeIns.getGroupId(),
									tradeIns.getSettlementAccount(),
									tradeIns.getSource(),
									tradeIns.getQuote().getType(),
									tradeIns.getBatchId(),
									withSplits,
									trade->getUserRef2(),
									trade->getUserRef3(),
									markupBookId);

	if( isMarkupPositionTransferTrade(*trade) )
		doCreditCheck 		= false;
		doProfitCheck 		= false;
		doLimitsCheck 		= false;
		doQuoteVerify 		= false;
		doQuantityCheck 	= false;
		doSanityCheck 		= false;
		doPermissionCheck 	= false;
	// customer offset trades - hard code the remaining checks not set in bookTrade
	else if( isCustOffsetTrade(*trade)) // clientId ends with "-TF" ?
		doQuantityCheck = false;
		doSanityCheck = false;
	else
		// check table PantherSettingGroup if the group has the relevant permission
		std::string canAmendFIXTrade = PantherSettingGroup[tradeIns.groupId]["canAmendTrade"], "no";
		if (canAmendFIXTrade == "yes")
			doProfitCheck = false;
			doLimitsCheck = false;

	step  = START;
	// check if this is a trading group
	if (res) 
		step = TRADING_GROUP_VERIFY;
		check PantherGroup[groupId].isTradingGroup

	// check if the group can trade
	const std::string marketCurrencyPair = FXCurrencyUtils::getMarketConventionCurrencyPair(tradeIns.getCcyPair().str());
	if (res)
		step = QUOTING_GROUP_VERIFY;
		if pricingGroupId != groupId check PricingGroupMappingOverride to see if it can trade

	// check the value date
	if (res)
		step = VALUEDATE_VALIDITY;
		...

	// sanity check trade
	if( res && doSanityCheck)
		step = SANITY;
		res  = TradeSanityChecker::run( *trade, withSplits, acctQtyPtrList, log); // a lot of checking
		...

	//check quantity
	if( res && doQuantityCheck)
		step = QTY_VERIFY;
		...

	// validate quote integrity
	if( res && doQuoteVerify)
		step = QUOTE_VERIFY;
		res  = quoteVerifier.run( *trade, log);

	// check permissions
	if( res && doPermissionCheck)
		step = PERMISSIONS;
		// uses PantherAccessControl and FxAccessControl 
		res  = tradePermissionChecker.run( *trade, log);

	// check if trading suspended
	if( res)
		step = TRADING_OPEN;
		// check FXTradingStatus
		res  = tradingOpenChecker.run( *trade, log);

	// check limits
	if( doLimitsCheck && res)
		step = LIMITS;
		res  = tradeLimitsChecker.run( *trade, log);

	// credit check
	if( doCreditCheck && res )
		step = CREDIT;
		res  = creditChecker.run( *trade, response, responses, nearResponses, farResponses,...);
		if( !res )
			...
		else if (response)
			trade->setCreditCheckApprover(response->getCreditCheckApprover());

		// if credit check was overridden manually then code value should be MANUAL
		if (trade->getQuote().getType() == FXOutrightQuote::CREDIT_FAIL_OVERRIDEN)
			trade->setCreditCheckApprover(ml::spec::panther::credit::CreditCheckApprover::MANUAL);

	// profit check
	if (doProfitCheck && res)
		res = economicChecker.run(*trade, log);
		 setRiskProfile(*trade, result.getRisk());

	// risk check
	if(res)
		step = RISK;
		// only run risk check for non-bulk trade or flag is off
		if (trade->getBatchId().isNull() || !FXTradingUtils::isNetSSPAXEBroadcastingEnabled())
			res = RiskManager::run(*trade, log);

	if( res && bMarkUpTrade )
		//set markup book without passing this trade to risk manager
		trade->setExecutionBookId(markupBookId);
		trade->setIsPanCcy1RiskMan(false);
		trade->setIsPanCcy2RiskMan(false);
		trade->setManagedBy(FXDeskType::BACK_TO_BACK);

	// time expiry check trade
	if ( res && doSanityCheck )
		step = TIME_EXPIRY;
		res  = tradeTimeExpiryChecker.run(tradeIns, *trade, log);

	trade->setErrorNumber(errorCode);

	if( errorCode != FXSpotForwardTrade::E_OK)
		trade->setReason(...)
	return res;


#########################################################################################

# Replace spot forward

Two trades will be created:

- Reversal trade to cancel the original trade, with userRef2="<MLFX:Cancel>", when this is booked,
  the original trade userRef2 is also updated to "<MLFX:Cancel>"
- Replacement trade, with userRef2="<MLFX:Amend>"

Details:

	void FXTradeContractImpl::replaceSpotForward(int64 &oldtradeId, FXSpotForwardIns& instruction )

	FXSpotForwardTrade::Ptr reversalTrade;
	FXSpotForwardIns replaceTradeInstruction = instruction;

	// check PantherSettingGroup[groupId, "canAmendTrade"] if the group has the relevant permission
	if no permission
		FXSpotForwardTrade::Ptr reversalTrade(new FXSpotForwardTrade()); ...
		persistAndBroadcastFailedTrade(reversalTrade, FXSpotForwardTrade::E_PERMISSIONS_ERROR, 
			"...", persister, tradeBroadcaster);
		return;

	// if both quantities are 0, book only the reversal trade (how can this happen ?)
	if(instruction.getQuantityCcy1()==0 && instruction.getQuantityCcy2()==0)
		reversalTrade = bookReverseTrade(oldtradeId, instruction.getClientId(), instruction.getClientId());
		return;

	// book the reverse trade		
	reversalTrade = bookReverseTrade(oldtradeId, instruction.getClientId());

	// book the new replacement trade
	replaceTradeInstruction.setUserRef3(reversalTrade->getUserRef3());
	replaceTradeInstruction.setUserRef2("<MLFX:Amend>"); // mark this as amend
	bookSpotForward(replaceTradeInstruction);


Book a reverse trade

	FXSpotForwardTrade::Ptr FXTradeContractImpl::bookReverseTrade(oldTradeId, failedTradeClientId, clientId)

	// get the original trade from localdb (FXSpotForwardTrade)
	FXSpotForwardTrade::Ptr originalTrade = tradeDBBroker.getById(oldTradeId);

	// sanity check originalTrade 
	// 1. not null 
	// 2. errorNumber is E_OK, 
	// 3. clientId ok 
	// 4. userRef2 no "<MLFX:Cancel>"

	// create the reverse trade from the original trade
	reversalTrade = getReverseTrade(originalTrade,reversalTrade);

	// get markup book id or error book id
	bMarkupBookConfigured = getMarkupBook(originalTrade->getGroupId(),
				originalTrade->getSettleAccount(), originalTrade->getSource(),
				originalTrade->getQuote().getType(), originalTrade->getBatchId(),
				false, reversalTrade->getUserRef2(), reversalTrade->getUserRef3(),
				groupMarkupBookId);
	// get FXSettlementAccount by [ groupId, boAccountId, accountType=TRADERBOOk ]
	groupMarkupAccount = getSettlementAccount(m_markupGroupId, groupMarkupBookId);

	//Set markup book on original trade as execution book
	reversalTrade->setManagedBy(FXDeskType::BACK_TO_BACK);
	reversalTrade->setExecutionBookId(groupMarkupBookId);

	// create a hedger trade

	hedgerTrade.reset(new FXSpotForwardTrade(*reversalTrade));
	std::string hedgerClientID = reversalTrade->getClientId().toString();
	hedgerClientID += "-CH";
	hedgerTrade->setUserRef3(reversalTrade->getUserRef3());
	hedgerTrade->setClientId(hedgerClientID);
	hedgerTrade->setSettleAccount(*groupMarkupAccount); // <---
	hedgerTrade->setExecutionBookId(Nullable<int64>());
	hedgerTrade->setGroupId(m_markupGroupId);
	hedgerTrade->setUserId(m_markupUserId);
	hedgerTrade->setSource(FXTradeSource::SOURCE_MARKUPRISKPOSX);
	hedgerTrade->getQuote().setValueDate(valueDate);
	hedgerTrade->setTenor(String("SP"));

	resetTradeId(reversalTrade);

	tradingOpenChecker.run( *reversalTrade, log);

	// book the reversal trade
	FXSpotForwardTrade::Ptr reverseTrade = bookSpotForwardTrade(reversalTrade);

	// update original trade's userRef2 and broadcast it

	originalTrade->setUserRef2(Nullable<String>(userRef2ForCancel.c_str()));
	tradeDBBroker.asyncWrite(*originalTrade, AsyncDBWriteBroadcastType::ON_DB_WRITE);
	tradeBroadcaster.run( *originalTrade, emptyList);

	// book the hedge trade at market rate

	resetTradeId(hedgerTrade);
	hedgerTrade->setHedgeOnBehalf(reverseTrade->getGroupId());
	hedgerTrade->setHedgeDelayType(FXTradeHedgeDelay::WASH_TRADE);
	hedgerTrade->setBatchId(Nullable<String>(ml::common::text::Format::int64ToString(originalTrade->getId())));
	bookAtMarketSpotForwardTrade(hedgerTrade, reversalTrade->getId());

	return reversalTrade;


Create a trade to reverse an original trade

	FXSpotForwardTrade::Ptr
	FXTradeContractImpl::getReverseTrade(FXSpotForwardTrade::Ptr originalTrade,
										 FXSpotForwardTrade::Ptr failedTrade)

	FXSpotForwardTrade::Ptr reversalTrade;
	reversalTrade.reset(new FXSpotForwardTrade(*originalTrade));

	// get various rates using reversed trade directions

	if(originalTrade->getDirection()=="buy")
		reversalTrade->setDirection("sell");
		traderSpotRate     = originalTrade->getQuote().getTraderRate().getSpotAskRate();
		...
	else
		reversalTrade->setDirection("buy");
		traderSpotRate     = originalTrade->getQuote().getTraderRate().getSpotBidRate();
		...

	// set quote with various reversed rates obtained above
	reversalTrade->getQuote().getTraderRate().setSpotAskRate(traderSpotRate);
	reversalTrade->getQuote().getTraderRate().setSpotBidRate(traderSpotRate);
	...

	// set trade date, value date and tenor
	// set checksum, entered time and order type

	// the original trade should have userRef1=userRef2=userRef3=NULL,
	// thus the userRef3 of reversalTrade will be set to "first=<origTradeId>;prev=<origTradeId>"
	reversalTrade->setUserRef3(getCancelReplaceUserRef3(originalTrade));
	reversalTrade->setUserRef2("<MLFX:Cancel>"); // mark the trade as cancel
	return reversalTrade;


Find markup book or error book for the quote:

	bool FXTradeContractImpl::getMarkupBook(const int64 groupId,
										const FXSettlementAccount& account,
										const int32 tradeSource,
										const FXOutrightQuote::QuoteType quoteType,
										const Nullable<String> batchId,
										const bool withSplits,
										const Nullable<String>& userRef2,
										const Nullable<String>& userRef3,
										Nullable<int64>& markupBookId)
	// No markup captured if:
	// 1. from trader book
	// 2. order fill (from fx_order_capture_server)
	// 3. P&L roll
	// 4. negotiated quote
	// 5. manual quote

	if (account.accountType == FXSettlementAccount::TRADERBOOK ||
		FXTradingUtils::isOrderFillTrade(tradeSource )  || 
		tradeSource == FXTradeSource::SOURCE_FXWEB_PNLROLL ||
		quoteType == FXOutrightQuote::NEGOTIATED || 
		quoteType == FXOutrightQuote::MANUAL)
		return false;

	if (!userRef2.isNull())

		if (userRef2->toString()== "<MLFX:Cancel>" || userRef2->toString()== "<MLFX:Amend>")

			// Bypass the error book if the trading date is still same as the date of the original trade

			bool bypassErrorBook = false;

			// extract original trade id from userRef3
			Nullable<std::string> strOriginalTradeId = ...;

			// get the original trade object
			FXSpotForwardTrade::Ptr originalTrade = tradeDBBroker.getById(strOriginalTradeId);

			Date currentTradingDate;
			string marketCurrencyPair = FXCurrencyUtils::getMarketConventionCurrencyPair(originalTrade->getCcyPair());
			FXCalendar.getCurrentTradingDay(marketCurrencyPair, currentTradingDate)
			if (originalTrade->getTradeDate() == currentTradingDate)
				bypassErrorBook = true;

			if (!bypassErrorBook)
				ErrorBook::Ptr ErrorBookItem = ErrorBook[groupId];
				if(ErrorBookItem != NULL)
					markupBookId = ErrorBookItem->getBookId();
					return true;

	MarkupBook::Ptr markupBookItem <- MarkupBook[groupId];
	if(markupBookItem != NULL)
		markupBookId = markupBookItem->getBookId();
		return true;
	return false;

Book a spot/forward trade:

	FXSpotForwardTrade::Ptr FXTradeContractImpl::bookSpotForwardTrade(FXSpotForwardTrade::Ptr trade)
		
	// create trade instruction
	FXSpotForwardIns tradeInstruction(FXRequestId(trade->getUserId(), Time().usecs()), trade->getGroupId(), ...);
	bookTrade<...>(trade, tradeInstruction, errorCode, false, FXSpotForwardTradeAllocation::PtrList() , 
			sfAllocsNearFarNull, sfAllocsNearFarNull, step, false /* performCheck */, NO_RATE_DELAY_CHECK);
	return trade;

# TPS subscription

When the trade server subscribes to TPS, it uses userId/groupId from registry


