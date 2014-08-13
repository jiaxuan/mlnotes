# Subscription Structure

FwdPointProcessor holds all ccypair subscriptions and a callback

	CcyPairSubscriptionMap m_ccyPairSubscriptions; // keyed on ccypair
	Callbacker::Ptr m_callback; // passed in in ctor, propogated to all subscriptions

CcyPairBaseSubscription holds curves and a list of final (ccypair, valueDate) subscriptions

	struct Node {
		std::string m_vd; // value date
		FwdPointCcyConfig::Ptr m_dn; // subscription
	};

	FwdPointsGenerator::Ptr m_priCurve; // this is where ticks originate
	FwdPointsGenerator::Ptr m_refCurve;
	std::map< Date, Node > m_subscriptions; 
	Callbacker::Ptr m_callback; // same as containing FwdPointProcessor

FwdPointCcyConfig is the actual subscription, its business logic is mostly in the bubblenet
of selector:

	FwdPointsSelector::Ptr m_selector;

FwdPointsSelector

	boost::shared_ptr< BubbleNet > m_bn; // this may also trigger ticks for static data change
	Callbacker::Ptr m_callback; // again, inherited from FwdPointCcyConfig

Data flow is setup in the following manner:

	FwdPointProcessor::m_callback             = FXFwdServer::onProcessorUpdate
		CcyPairBaseSubscription::m_callback   = FXFwdServer::onProcessorUpdate
			FwdPointsSelector::m_callback     = FXFwdServer::onProcessorUpdate


Ticks from FwdPointsGenerator:

	// the various implementations of generators eventually call this to publish ticks
	FwdPointsGenerator::publish(tick) { 
		m_staleListener->processTick(tick); // run it thu StalingProcessor first, which checks FwdPointsStale table
		m_callback(tick);
	}
		FwdPointsGenerator::m_callback = CcyPairBaseSubscription::onPrimaryTick  for CcyPairBaseSubscription::m_priCurve
		FwdPointsGenerator::m_callback = CcyPairBaseSubscription::onReferenceTick  for CcyPairBaseSubscription::m_refCurve

	// for primary curve, the following happens, the second path is to handle broken date
	CcyPairBaseSubscription::onPrimaryTick 
		-> CcyPairBaseSubscription::processCurveTicks -> FwdPointsSelector::onPrimaryTick
		-> CcyPairBaseSubscription::processLeftRightPriTick -> FwdPointsSelector::onLeftRightNodePriTick
	// for reference curve
	CcyPairBaseSubscription::onReferenceTick 
		-> CcyPairBaseSubscription::processCurveTicks -> FwdPointsSelector::onReferenceTick
		-> CcyPairBaseSubscription::processLeftRightRefTick -> FwdPointsSelector::onLeftRightNodeRefTick
											
At FwdPointSelector, the ticks are fed into the bubblenet:

	onPrimaryTick: m_priSourceNode->tick(tick)
	onReferenceTick: m_refSourceNode->tick(tick)
	onLeftRightNodePriTick: m_leftPriSourceNode->tick(tick) or m_rightPriSourceNode->tick(tick)
	onLeftRightNodeRefTick: m_leftrefSourceNode->ticl(tick) or m_rightRefSourceNode->tick(tick)

The bubblenet ends at:

	FwdPointsSelector::onTick(tick) {
		// ... set m_lastTick
		// calls canFilterOut(...) to do filtering 
		publish();
	}
	FwdPointsSelector::publish() {
		InternalFwdTick tick(*m_lastTick);
		// detect region schedule issue
		publishInternal(tick);
	}
	FwdPointsSelector::publishInternal(tick) {
		tick.m_sData->m_tickId = apr_time_now(); // tick id
		tick.m_sData->m_timeBucket = m_bucket; // value date
		(*m_callback)(tick); // m_callback is FXFwdServer::onProcessorUpdate
	}

Now we reached the final callback:

	FXFwdServer::onProcessorUpdate





# Entry Points

Subscribe:

	bool FxFwdServer::onSubscribe(	string& ccyPairName,
									string& timeBucket,
									string& failReason ) {
		return enqueueSubscriptions( ccyPairName, timeBucket, std::string(""), std::string(""), std::string(""), failReason );

	bool FxFwdServer::onPreviewSubscribe(string& region,
										 string& ccyPairName,
										 string& source,
										 string& curve,
										 string& timeBucket,
										 string& failReason ) {
		return enqueueSubscriptions( ccyPairName, timeBucket, region, source, curve, failReason );


	bool FxFwdServer::enqueueSubscriptions(	string& ccyPairName,
			  	  	  	  	  	  	  	  	string& timeBucket,
			  	  	  	  	  	  	  	  	string& region,
			  	  	  	  	  	  	  	  	string& source,
			  	  	  	  	  	  			string& curve,
			  	  	  	  	  	  			string& failReason )
		const CurrencyPair::Ptr& ccyPair = CacheUtils::getCurrencyPair(ccyPairName);
		bool isWorkingDay = false;
		... check working day ...
		if (isWorkingDay)
			putMessage(Message_ptr(new ClientSubscriptionMessage(ccyPair, timeBucket, region, source, curve)));


Unsubscribe:

	void FxFwdServer::onUnsubscribe(const std::string& ccyPairName, const std::string& timeBucket)
	    const CurrencyPair::Ptr& ccyPair = CacheUtils::getCurrencyPair(ccyPairName);
        putMessage(Message_ptr(new ClientUnsubscriptionMessage(ccyPair, timeBucket)));

	void FxFwdServer::onPreviewUnsubscribe( string& region,
											string& ccyPairName,
											string& source,
			 	 	 	 	 	 	 	 	string& curve,
			 	 	 	 	 	 	 	 	string& timeBucket )
		const CurrencyPair::Ptr& ccyPair = CacheUtils::getCurrencyPair(ccyPairName);
		putMessage(Message_ptr(new ClientUnsubscriptionMessage( ccyPair,timeBucket, region,	source,	curve, true)));


The actual work is done by FwdPointsProcessor.

# FwdPointsProcessor

The setup:

	void FxFwdServer::createProcessor()
	    m_processorCallback.reset(new Callbacker(
			"ProcessorCallbackFunctor",
			boost::bind(&FxFwdServer::onProcessorUpdate,make_weak_ptr(shared_from_this()), _1)));
	    m_processor.reset(new FwdPointsProcessor(m_processorCallback));


So processor results are handled in:

	void FxFwdServer::onProcessorUpdate(const InternalFwdTick & tick)

    	const ml::fx::fwdclient::TickDetails& tickDetails =	*(tick.m_sData->m_tickDetails);
    	ml::spec::fx::fwd::FXFwdTick::Ptr fxFwdTickPtr = tick.toFXFwdTick();

    	if (!fxFwdTickPtr) {
    		GLFWDWARN(false, "FXFwdTick is null (tick: %s)", tick.toString().c_str());
    		return;
    	}

    	if ( tick.isPreviewModeOn() ) {
			m_clientPreviewSide->sendPreviewTick(fxFwdTickPtr);
    	} else {
    		if (tick.isBidOrAskStale())
				m_clientSide->sendStale(fxFwdTickPtr, tickDetails, tick.m_sData->m_mlfxTradingDay);
			else
				m_clientSide->sendTick(fxFwdTickPtr, tickDetails, tick.m_sData->m_mlfxTradingDay);
    	}


The subscriptions:

	// FwdPointsProcessor.hpp

    typedef std::tr1::unordered_map<std::string, CcyPairClientSubscription_ptr> CcyPairSubscriptionMap;
    typedef std::tr1::unordered_map<std::string, CcyPairPreviewSubscription_ptr> CcyPairPreviewSubscriptionMap;

    CcyPairSubscriptionMap                     m_ccyPairSubscriptions;
    CcyPairPreviewSubscriptionMap              m_ccyPairPreviewSubscriptions;


	void FwdPointsProcessor::subscribe(const CurrencyPair::Ptr & ccyPair,
							   	   	   const std::string & timeBucket,
							   	   	   const std::string & region,
							   	   	   const std::string & sourceType,
							   	   	   const std::string & curve,
							   	   	   const bool isPreview)
		CcyPairBaseSubscription_ptr subscription = getCcyPairSubscription(ccyPair->getName().str(), sourceType, curve, region, isPreview);
		subscription->subscribe(timeBucket);

	void FwdPointsProcessor::unsubscribe( const CurrencyPair::Ptr & ccyPair,
								 	 	  const std::string & timeBucket,
								 	 	  const std::string & region,
								 	 	  const std::string & sourceType,
								 	 	  const std::string & curve,
								 	 	  const bool isPreview )
		CcyPairBaseSubscription_ptr subscription = getCcyPairSubscription(ccyPair->getName().str(), sourceType, curve, region, isPreview);
		subscription->unsubscribe(timeBucket);


	CcyPairBaseSubscription_ptr FwdPointsProcessor::getCcyPairSubscription( const std::string& ccyPair,
																			const std::string& sourceType,
																			const std::string& curve,
																			const std::string& region,
																			bool isPreview)
		CcyPairBaseSubscription_ptr subscription;

		if(isPreview) {
			std::string ccySourceKey = makeCcySourceKey(ccyPair, sourceType, curve, region);
			CcyPairPreviewSubscriptionMap::iterator iter = m_ccyPairPreviewSubscriptions.find(ccySourceKey);

			if (iter != m_ccyPairPreviewSubscriptions.end()) {
				subscription = iter->second;
			} else {
				CcyPairPreviewSubscription_ptr typePtr(new CcyPairPreviewSubscription( ccyPair, sourceType, curve, region,
																			  m_callback, m_processingThreadPool));
				m_ccyPairPreviewSubscriptions[ccySourceKey] = typePtr;
				subscription = typePtr;
			}
		} else {
			// for normal subscription, it must already exist (created in init)
			CcyPairSubscriptionMap::iterator iter = m_ccyPairSubscriptions.find(ccyPair);

			if (iter != m_ccyPairSubscriptions.end()) {
				subscription = iter->second;
			} else {
				FTHROW(RuntimeException, "CurrencyPair %s not in price FwdPointsProcessor",
					ccyPair.c_str());
			}
		}
	    return subscription;


Preview subscriptions are based on (ccyPair, sourceType, curve, region), note tenor is not part of it:

    std::string makeCcySourceKey( const std::string& ccyPair,
    							  const std::string& sourceType,
    							  const std::string& curve,
    							  const std::string& region )
    {
    	return (ccyPair + "-" + sourceType + "-" + curve + "-" + region);
    }



Internal structure of subscriptions:

	CcyPairBaseSubscription

    typedef boost::shared_ptr<Node> NodePtr;
    typedef std::map<mlc::util::Date, NodePtr> DateSubscriptions;

    std::string 								m_ccy;
    std::string 								m_ccyPair;
    std::string 								m_activeRegion;
    ml::common::util::Date 						m_tradingDay;

    DateSubscriptions 							m_subscriptions;	// tenors

    FwdPointsGenerator::Ptr						m_priCurve;			// primary curve
    FwdPointsGenerator::Ptr						m_refCurve;			// reference curve

    bool										m_previewMode;
    bool										m_refCurveEnabled;

Subscribe for preview:

	void CcyPairPreviewSubscription::subscribeThreadSafe(const std::string& timeBucket )
		FwdPointsCcyConfig::Ptr dateNode;

		if(!m_priCurve)
			initialise();

		std::string stdTenor, lhsTenor, rhsTenor;
		Date valuedate;
		bool isbrokenDate;

		if ( !Utils::getDate( m_ccyPair, timeBucket, m_tradingDay, valuedate, stdTenor, lhsTenor, rhsTenor, isbrokenDate) )
			return;

		stdTenor = lhsTenor = rhsTenor = timeBucket;

		// check date subscription
		DateSubscriptions::iterator srcIter = m_subscriptions.find(valuedate);
		if ( srcIter == m_subscriptions.end() )
		{
			// Creating new preview subscription node

			std::string lT(lhsTenor), rT(rhsTenor);
			bool hasValidSpreads = m_spreadsRangeExtender->extendRangeForTenors(valuedate, lT, rT);
			dateNode.reset( new FwdPointsCcyConfig( m_ccy,
												m_ccyPair,
												m_activeRegion,
												timeBucket,
												valuedate,
												stdTenor,
												lT,
												rT,
												m_tradingDay,
												m_callback,
												m_processingThreadPool,
												false,
												hasValidSpreads,
												m_skewDispatcher,
												m_sourceType,
												m_curveName,
												true) );

			NodePtr pNode(new Node(timeBucket, dateNode));
			m_subscriptions.insert(std::make_pair(valuedate, pNode));

			if ( m_priCurve )
				dateNode->initialise(m_priCurve);

		} else
			dateNode = srcIter->second->m_dn;

		if (dateNode)
			dateNode->getSelector()->activate();


	void CcyPairPreviewSubscription::initialise()
		STRTRank rank 		= STRTRank::PRIMARY;
		bool hasOverrides 	= true;

		CallbackFunctor callback = CcyPairBaseSubscription::getCallbackFunctor(rank);

		if ( m_priCurve ) {
			m_priCurve->stop();
			m_priCurve.reset();
		}

		m_priCurve = GeneratorCreator::createGenerator(   m_sourceType,
														  m_ccyPair.c_str(),
														  m_activeRegion,
														  rank,
														  m_curveName,
														  m_tradingDay,
														  hasOverrides,
														  callback,
														  true );



Subscribe for normal clients:

	void CcyPairClientSubscription::subscribe(const std::string & timeBucket)
        FwdPointsCcyConfig::Ptr dn = subscribeThreadSafe(timeBucket, m_subscriptions, true);
        if (dn)
            dn->getSelector()->activate();

    FwdPointsCcyConfig::Ptr CcyPairClientSubscription::subscribeThreadSafe( const std::string & timeBucket,
                                                                     	 	DateSubscriptions & subscriptions,
                                                                     	 	bool reportErrorToClient )
        std::string stdTenor, lhsTenor, rhsTenor;
        Date valuedate;
        bool isbrokenDate;

        if ( !Utils::getDate( m_ccyPair, timeBucket,
        					  m_tradingDay, valuedate, stdTenor, lhsTenor, rhsTenor, isbrokenDate) )
            return FwdPointsCcyConfig::Ptr();

        // correcting std tenor for broken date timebucket
        if (stdTenor.empty() && CacheUtils::isStandardTimeBucket(m_ccyPair, m_activeRegion, timeBucket))
        	stdTenor = lhsTenor = rhsTenor = timeBucket;

        return subscribeInternal(	timeBucket,
        						  	valuedate,
        						  	stdTenor,
        						  	lhsTenor,
        						  	rhsTenor,
        						  	isbrokenDate,
        						  	m_callback,
        						  	subscriptions,
        						  	reportErrorToClient );

  	FwdPointsCcyConfig::Ptr CcyPairClientSubscription::subscribeInternal(const std::string & timeBucket, const Date & valuedate,
  	                                                               const std::string & stdTenor,
  	                                                               const std::string & lhsTenor,
  	                                                               const std::string & rhsTenor, const bool & isbrokenDate,
  	                                                               const Callbacker::Ptr & newCallback,
  	                                                               DateSubscriptions & subscriptions,
  	                                                               bool reportErrorToClient)
  	    FwdPointsCcyConfig::Ptr dateNode;

  	    // checking value date to avoid subscriptions for past dates
  	    if (valuedate < m_tradingDay)
  	        return dateNode;

  	    std::string lT(lhsTenor), rT(rhsTenor);

  	    bool hasValidSpreads = m_spreadsRangeExtender->extendRangeForTenors(valuedate, lT, rT);

        DateSubscriptions::iterator dvIter = subscriptions.find(valuedate);

        if (dvIter == subscriptions.end()) {
            dateNode.reset(new FwdPointsCcyConfig( m_ccy,
                        		   	   	   	   	   m_ccyPair,
                        		   	   	   	   	   m_activeRegion,
                        		   	   	   	   	   timeBucket,
                        		   	   	   	   	   valuedate,
                        		   	   	   	   	   stdTenor,
                                                   lT,
                                                   rT,
                                                   m_tradingDay,
                                                   newCallback,
                                                   m_processingThreadPool,
                                                   m_refCurveEnabled,
                                                   hasValidSpreads,
                                                   m_skewDispatcher));

            NodePtr pNode(new Node(timeBucket, dateNode));
            subscriptions.insert(std::make_pair(valuedate, pNode));

            if ( m_priCurve )
            	dateNode->initialise(m_priCurve);

            if ( m_refCurve )
            	dateNode->initialise(m_refCurve);

        } else {
            dateNode = dvIter->second->m_dn;
            FwdPointsSelector::Ptr & selector = dateNode->getSelector();

            const Callbacker::Ptr & existingCallback = selector->getCallback();
            if (*existingCallback != *newCallback && (*existingCallback == *getNullCallbacker() || *newCallback != *getNullCallbacker()))
                selector->setCallback(newCallback);
        }
        ...

  	    return dateNode;


    void CcyPairClientSubscription::initialise()
    {
    	initialiseFwdPointsGenerator(m_priCurve, STRTRank::PRIMARY);
    	initialiseFwdPointsGenerator(m_refCurve, STRTRank::REFERENCE);

    	// Update existing subscriptions.
        foreach(dvIter, m_subscriptions )
        {
            FwdPointsCcyConfig::Ptr ccyConfig = dvIter->second->m_dn;
            if (ccyConfig)
                ccyConfig->changeRegion(m_activeRegion, m_priCurve, m_refCurve);
        }

        m_refCurveEnabled = ( (m_priCurve && m_refCurve) ? true : false );
        CcyPairBaseSubscription::setRefCurveStateOnSelectors(m_refCurveEnabled);

        // Subscribe to std tenors if reqd
        if ( m_subscriptions.empty() )
        	subscribeStdTenors();


	void CcyPairClientSubscription::initialiseFwdPointsGenerator(FwdPointsGenerator::Ptr & curve, const STRTRank & rank)
	{
		// for normal subscription, get source info from STRTSchedule
		STRTSchedule::Ptr schedule;
		CacheUtils::getSTRTSchedule(schedule, m_ccy, m_activeRegion, rank);

		if (schedule) {
	        const std::string & type 	= schedule->getType().str();
	        const std::string & source 	= schedule->getSource().str();

	        if (curve) {
	        	if (source != curve->getSource()) {
	        		// reinitialize curve
	        		reinitialiseCurve(curve, rank, type, source);
	        	}
			} else {
				// reinitialize curve
				reinitialiseCurve(curve, rank, type, source);
			}
		} else {
			if (curve) {
	    		curve->stop();
				curve.reset();
			}
		}
	}


	void CcyPairClientSubscription::reinitialiseCurve(FwdPointsGenerator::Ptr & curve, const STRTRank & rank,
		const std::string & type, const std::string & source)
	{
		bool reinitialize = false;

		if (!curve) {
			reinitialize = true;
		} else if (source != curve->getSource()) {
			reinitialize = true;
			curve->stop();
			curve.reset();
		}

		if (reinitialize) {
			// Need to create new curve.
			// We override only PRI curves
			bool hasOverrides = (rank == STRTRank::PRIMARY);
	        CallbackFunctor callback = getCallbackFunctor(rank);

	        FwdPointsGenerator::Ptr newCurve = GeneratorCreator::createGenerator(type,
													   m_ccyPair.c_str(),
													   m_activeRegion.c_str(),
													   rank,
													   source,
													   m_tradingDay,
													   hasOverrides,
													   callback);
	        if (newCurve)
	        	curve = newCurve;
		}
	}


When calling GeneratorCreator::createGenerator, callbacks are set up using:

	CallbackFunctor CcyPairBaseSubscription::getCallbackFunctor(const STRTRank & rank)
	    switch (rank.get())
	        case STRTRank::PRIMARY:
	            return boost::bind(&CcyPairBaseSubscription::onPrimaryTick, make_weak_ptr(shared_from_this()), _1);
	        case STRTRank::REFERENCE:
	            return boost::bind(&CcyPairBaseSubscription::onReferenceTick, make_weak_ptr(shared_from_this()), _1);

So ticks are handled by:

	void CcyPairBaseSubscription::onPrimaryTick(const InternalFwdTick & tick)
	    InternalFwdTick oTick(tick);
	    if ( m_previewMode ) {
	        oTick.setPreviewMode(true);
	        oTick.m_sData->m_tickDetails->region = m_activeRegion;
	    }

	    SelectorDispatch selectorDispatch = boost::bind(&FwdPointsSelector::onPrimaryTick, _1, _2);
	    m_processingThreadPool.execute(
	                    WorkItem(
	                             m_ccyPair,
	                             m_ccyPair + "." + oTick.m_sData->m_valueDate.toUTCString() + "."
	                                       + Format::int64ToString(tick.m_sData->m_tickId),
	                             boost::bind(&CcyPairBaseSubscription::processCurveTicks,
	                            		 this->shared_from_this(), oTick, selectorDispatch)));

	void CcyPairBaseSubscription::onReferenceTick(const InternalFwdTick & tick)
	    SelectorDispatch selectorDispatch = boost::bind(&FwdPointsSelector::onReferenceTick, _1, _2);
	    m_processingThreadPool.execute(
	                    WorkItem(m_ccyPair,
	                             m_ccyPair + "." + tick.m_sData->m_valueDate.toUTCString() + "."
	                                       + Format::int64ToString(tick.m_sData->m_tickId),
	                             boost::bind(&CcyPairBaseSubscription::processCurveTicks,
	                            		 this->shared_from_this(), tick, selectorDispatch)));


	void CcyPairBaseSubscription::processCurveTicks( InternalFwdTick const &tick,
													 SelectorDispatch const &selectorDispatch )
		// getSelector find in m_subscriptions the one with a matching value date
		// and returns its FwdPointsSelector
        FwdPointsSelector::Ptr sn = getSelector(tick.m_sData->m_valueDate);
        if (sn)
        	// this calls FwdPointsSelector::onPrimaryTick or FwdPointsSelector::onReferenceTick
            selectorDispatch(sn, tick);


The selector:

	void FwdPointsSelector::onPrimaryTick(const InternalFwdTick & tick)
        if (!m_lastPrimaryTick)
            m_lastPrimaryTick.reset(new InternalFwdTick());
        *m_lastPrimaryTick = tick;
	    m_priSourceNode->tick(tick);

	void FwdPointsSelector::onReferenceTick(const InternalFwdTick & tick)
	   	m_refSourceNode->tick(tick);

# Architecture

FxFwdServer provides the facade to subscription service:

	onSubscribe(...)
	onUnsubscribe(...)
	onPreviewSubscribe(...)
	onPreviewUnsubscribe(...)

The actual subscriptions are in FwdPointsProcessor:

	// normal subscriptions are indexed by ccyPair
	typedef std::tr1::unordered_map<std::string, CcyPairClientSubscription_ptr> CcyPairSubscriptionMap;
	CcyPairSubscriptionMap                     m_ccyPairSubscriptions;

	// previews are indexed by (ccyPair, sourceType, curve, region)
	typedef std::tr1::unordered_map<std::string, CcyPairPreviewSubscription_ptr> CcyPairPreviewSubscriptionMap;
	CcyPairPreviewSubscriptionMap              m_ccyPairPreviewSubscriptions;


Each client/preview subscription (CcyPairBaseSubscription) contains a primary curve, a reference curve
and a list of subscriptions for different tenors. The tenor-specific subscription is implemented in
FwdPointsCcyConfig, which uses FwdPointsSelector to do much of the actual processing:

    typedef boost::shared_ptr<Node> NodePtr;
    typedef std::map<mlc::util::Date, NodePtr> DateSubscriptions;

    FwdPointsGenerator::Ptr						m_priCurve;			// primary curve
    FwdPointsGenerator::Ptr						m_refCurve;			// reference curve
    DateSubscriptions 							m_subscriptions;	// tenors


The point generators are responsible for generating forward points, there are two types:

	BrokerPageFwdPointsGenerator
	RioFwdPointsGenerator

CcyPairBaseSubscription registers itself as handlers of forward points. When a tick
is generated, it finds the FwdPointsCcyConfig in m_subscriptions with the matching
date and dispatches the tick to its FwdPointsSelector.






# Tick Generators



Curves are tick sources:

	// not Thread
	class BrokerPageFwdPointsGenerator : public FwdPointsGenerator

	// responsible for generating market data
	BrokerPageGeneratorInternal::MarketDataLayerPtr m_marketData;

	void onStart() {
		...
		m_marketData->start();
	}
	void onStop() {
		...
		m_marketData->stop();
	}




Market data layer:


	// Not Thread
	class MarketDataLayer

	std::string m_ccyPair;
	std::string m_region;
	std::string m_pageId;
	mlc::util::Date m_tradingDay;

	bool m_isActive;

	MarketDataSource m_mdSource;

	// MarketDataSource inited in ctor as
	m_mdSource(ccypair, region, pageId, tradingDay, sRank, boost::bind(&MarketDataLayer::onNewMarketData, this, _1, _2, _3), mode)

	// keep track of last points for subscribed tenors
	std::map<mlc::util::Date, MarketPointPtr> m_pointsMap;

	void initialize() {
		// subscribing to STD tenors
		CacheUtils::StandardTenorList stdTenors;
		CacheUtils::getCurrencySpecificTenors(stdTenors, m_ccyPair, m_region);

		foreach (iter, stdTenors) {
			MarketPointPtr point;
			mlc::util::Date date;
			// call MarketDataSource::subscribe based on tenor type
			switch (stdTenor.timeBucketType)
				case TimeBucketType::TENOR:
					point = m_mdSource.subscribe(stdTenor.tenor);
				case TimeBucketType::BROKENDATE:
					...
				case TimeBucketType::IMM:
					...
		}
	}

	void onNewMarketData(Data& date, Side& side, Price& price) {
		// do some checking and finally call
		PointMap::iterator pos = m_pointsMap.find(date);
		if (pos == m_pointsMap.end()) ...
		MarketPointPtr& point = pos->second;
		if (point->setPrice(side, price)) { // true only if price has changed
			if (it is spot) {
				if (!point->getRate().isAnyStale()) {
					// recalculate points for all 
					foreach(iter, m_pointsMap) {
						if (!iter->second->getRate().isAnyStale())
							processPoint(iter->second);
					}
				}
			} else 
				processPoint(point);
		}
	}

	void processPoint(MarketPointPtr& point) {
		...
	}




Market data source:


	// Not Thread
	class MarketDataSource

	ml::panther::silo::SiloCallback * m_decoupledCallback;

	std::multimap<SiloRic, BrokerSourcePtr> m_ric2Source;
	SiloRic m_referenceRic;

	// ctor
	m_decoupledCallback = Utils::getSiloDecoupler(m_mode.isBasicPreview()).insertCallback(this, ccypair, source);

	// dtor
	write_synchronized(Utils::getSiloDecoupler(m_mode.isBasicPreview()).getMutex()) {
		Utils::getSiloDecoupler(m_mode.isBasicPreview()).removeCallbackThreadSafe(this, m_ccyPair, m_source);
	}


	// there are 3 different subscribes that are used by MarketDataLayer

	// for standard tenor
	MarketPointPtr subscribe(Tenor& tenor) {

		// get subscription info from BrokerPageSubscription table
		BrokerPageSubscription::PtrList subscriptionsList;
		CacheUtils::getTenorSubscriptions(subscriptionsList, m_ccyPair, tenor, m_source);
		BrokerPageSubscription::Ptr subscription;

		...
		// creating BrokerSource object
		BrokerSourcePtr brokerSource;
		brokerSource.reset(new BrokerSource(
				m_decoupledCallback,       // <--- notice this
				subscription->getSubscriptionString().str(),
				tenor.getId(), date, m_mode.isBasicPreview()));		
		...
		// there are 2 subscriptions, one for bid, one for ask
		m_ric2Source.insert(std::make_pair(brokerSource->getBidRic(), brokerSource));
		m_ric2Source.insert(std::make_pair(brokerSource->getAskRic(), brokerSource));
		// save the bid Ric
		m_referenceRic = brokerSource->getBidRic();
		...
	}

	// for broken date, we simply return a stale point
	MarketPointPtr subscribe(const mlc::util::Date & date) {
		MarketPointPtr point;
		// we don't have any RICs configured for a broken date that's why we're creating a staled point
		point.reset(new MarketPoint(date, 0, m_hasOutrights));
		point->setPrice(Side::BOTH, Price::STALE, "No RIC configured for this tenor. Please contact support team");
		return point;
	}

	// for IMM
	MarketPointPtr subscribe(Date& date, int immId) {
		...
	}

	// for callback
	void onTick(SiloRic & ric, double& value) {
		...
		processTick(ric, price);
	}
	void onStale(SiloRic& ric) {...}


	// note we are calling sub/unsub on each BrokerSource twice
	// as each has two entries in our map, bad !!!

	void forceSubscribeAll()
		foreach (iter, m_ric2Source)
			iter->second->subscribe();

	void forceUnsubscribeAll()
		foreach (iter, m_ric2Source)
			iter->second->unsubscribe();




Broker source:

	SiloRic                                     m_bidRic;
	SiloRic                                     m_askRic;
	ml::panther::silo::SubscriptionToken_ptr    m_bidSub;
	ml::panther::silo::SubscriptionToken_ptr    m_askSub;

	void subscribe() {
		...
		m_bidSub = BrokerSource::getSiloReutersImpl(m_preview).subscribe(m_bidRic, m_callback);
		m_askSub = BrokerSource::getSiloReutersImpl(m_preview).subscribe(m_askRic, m_callback);
	}

	void unsubscribe() {
		...
		m_bidSub.reset();	// SubscriptionToken::~SubscriptionToken() does unsub
		m_askSub.reset();
	}




SiloReutersImpl:

	// it uses a ReutersProvider to interface with Reuters
	ml::panther::connectivity::reutersrfa::ReutersProvider_ptr m_reuters;

	/**
	 * Holds information about the ask value for each RIC which is used to determine in
	 * which order to tick back the bid and ask values to avoid the calling code having
	 * to deal with inverted rates unnecessarily.
	 */
	std::map<RfaRic, ml::common::util::Nullable<double> > m_previousAsks;

	/**
	 * Holds all the currently active subscriptions form e.g.:
	 *   RSF/GBP= -> RSF/GBP=/BID
	 *   RSF/GBP= -> RSF/GBP=/ask
	 */
	 std::multimap<RfaRic, SiloRic> m_activeSubs;

	/**
	 * Holds all the HISTORICAL subscriptions form e.g.:
	 *   RSF/GBP= -> RSF/GBP=/BID
	 *   RSF/GBP= -> RSF/GBP=/ask
	 * This map is cleared down only when there are no more active mappings.  It will be used to delete all the cached values.
	 */
	std::map<RfaRic, int64> m_historicalSubs;
	
	/*
	 * Holds the previous tick sequence number for each RFA message received.
	 */
	SequenceMap m_sequences;


ReutersProvider

	class ReutersProvider : private ml::common::concurrent::Thread

	ml::panther::pricefeed::reutersrfa::ReutersRfaListener<ReutersProviderEventForwarder> m_rfaListener;

	// subscription management
	void subscribe(const Ric& ric);
	void unsubscribe(const Ric& ric);
	bool pushTick(const Ric& ric);

	ReutersProvider::ReutersProvider(Callback *callback) : m_callback(callback), m_rfaListener(new ReutersProviderEventForwarder(this))
		start();
    	m_rfaListener.startListening(); 



ReutersRfaListener

	class ReutersRfaListener : public ml::panther::pricefeed::interface::Listener

    ml::panther::pricefeed::reutersrfa::OnEventBase* pOnEvent;
	ml::panther::pricefeed::reutersrfa::Source*      pSource;
	ml::panther::pricefeed::reutersrfa::Subscriber*  pSubscriber;

	ReutersRfaListener<T>::ReutersRfaListener(T* event) :
		pOnEvent(0),
		pSource(0),
		pSubscriber(0)
	{
		rfa::common::Context::initialize();
		pOnEvent	= event;		
        pSource     = new ml::panther::pricefeed::reutersrfa::Source;                    
        pSubscriber = new ml::panther::pricefeed::reutersrfa::Subscriber( *pSource, *pOnEvent);
        pOnEvent->setEventListener( *this);


Source

	class Source

    DispatchThread*                     dispatchThread;	

	rfa::sessionLayer::Session*			pSession;
	rfa::common::EventQueueGroup*		pEventQueueGroup;
	rfa::common::EventQueue*			pEventQueue;


	// it creates a session and a queue group, the queue is not used
        pEventQueueGroup = createEventQueueGroup();
        pEventQueue = createEventQueue();
        pEventQueueGroup->addEventQueue( *pEventQueue); 
		pSession = createSession();

	// it also creates a thread to drive the events

	Source::stopPumping()
        delete dispatchThread;
        dispatchThread = 0;

	Source::startPumping()
        dispatchThread = new DispatchThread( *pEventQueueGroup);

	class DispatchThread : public Thread
	virtual bool onProcess()
		long rtn = queueGroup.dispatch( 1000);

Subscriber

	// the subscriber manages subscriptions on top of the source object

	ml::panther::pricefeed::reutersrfa::OnEventBase& m_onEvent;			// RFA callback class			
	ml::panther::pricefeed::reutersrfa::Source&      m_source;
	rfa::common::EventQueue* 				 	     m_pEventQueue;
	rfa::sessionLayer::MarketDataSubscriber* 	     m_pMktDataSubscriber;

	// using its own event queue but Source group and session
		m_pEventQueue = createEventQueue();		
        m_source.getEventQueueGroup().addEventQueue( *m_pEventQueue);
		m_pMktDataSubscriber = createMarketDataSubscriber();
			
	MarketDataSubscriber* Subscriber::createMarketDataSubscriber()
		MarketDataSubscriber* pSubscriber = 
			m_source.getSession().createMarketDataSubscriber("mktDataSubscriber", false);
	    MarketDataSubscriberInterestSpec interestSpec( true);
	    pSubscriber->registerClient(*m_pEventQueue, interestSpec, m_onEvent, 0);
		return pSubscriber;

	void Subscriber::subscribeSymbol(const PricedItemId& itemId)
		if (m_handles.find(itemId) != m_handles.end())
			FTHROW( RuntimeException, "Symbol already subscribed: %s", itemId.toString().c_str() );
	    if( !m_onEvent.hasSource( itemId.getSource()))
	        FTHROW( RuntimeException, "Source unavailable: %s", itemId.getSource().c_str());
	   
		Handle* pHandle = m_pMktDataSubscriber->subscribe(*m_pEventQueue, itemSub, m_onEvent, 0);
		if (!pHandle)
			FTHROW( RuntimeException, "Failed to subscribe symbol %s", itemId.toString().c_str());
		m_handles[itemId] = pHandle;


We have two decouplers:

	SiloDecoupler& Utils::getSiloDecoupler(bool preview)
		static SiloDecoupler previewDecoupler;
		static SiloDecoupler liveDecoupler;

		if (preview)
			return previewDecoupler;
		return liveDecoupler;

We also have two SiloReutersImpl:

	ml::panther::silo::SiloReutersImpl & BrokerSource::getSiloReutersImpl(bool preview)
		static SiloReutersImplWrapper previewSiloReutersInstance;
		static SiloReutersImplWrapper liveSiloReutersInstance;

		if (preview)
			return previewSiloReutersInstance.get();
		return liveSiloReutersInstance.get();




ReutersProvider keeps track of subscriptions with map and set:

	std::map<Ric, SubsAckTrackingData_ptr> PendingAckSubscriptionMap;
	std::set<Ric> ActiveSubscriptionSet;

	void ReutersProvider::subscribe(const Ric &ric) {
		// NO checking for duplicate subscription here !
	}



## Relationships:

	// fwd server side

	(ccypair, region, tradingDay, source, rank)

                                  owns 1                   owns 1
	BrokerPageFwdPointsGenerator -------- MarketDataLayer -------- MarketDataSource


	// market data side
                     owns 1                          owns 1                     owns 1
	SiloReutersImpl -------- ReutersProvider (Thr) -------- ReutersRfaListener -------- Source (Thr)



	MarketDataSource adds itself to SiloDecoupler by (ccyPair, source)

	Note that source ticks only depend on (ccyPair, source) 

	For each tenor, two RIC subscriptions are made, ask and bid:

	MarketDataSource::subscribe(tenor) -----> BrokerSource -------> 2 SiloReutersImpl::subscribe(tenor)

	The driver (decoupler) of the BrokerSource depends on preview/non-preview
	The type of SiloReutersImpl used depends on basic-preview/non-basic-preview






## The Subscription Chain

Subscription management

Snapshot management


## The Callback Chain:

Driver: Source::dispatchThread

	1. DispatchThread::onProcess() { long rtn = queueGroup.dispatch( 1000); }
	2. ReutersProviderEventForwarder::processEvent(const rfa::sessionLayer::MarketDataItemEvent& event)
	3. void ReutersProvider::processEvent(const MarketDataItemEvent& event)
	4. void ReutersProvider::processMarketData(const MarketDataItemEvent& event, bool isTick)
	5. void SiloReutersImpl::onTick(int64 sequence, const RfaRic& rfaRic, const RfaMsg& rfaMsg)
	6. void SiloWithManagedSubscriptions::sendTick(const Ric &ric, const double &value, SiloCallback *) // bid
	   void SiloWithManagedSubscriptions::sendTick(const Ric &ric, const double &value, SiloCallback *) // ask
	   	
	   	SubscriptionMapIterPair range = subscription_map.equal_range(ric);
    	for (SubscriptionMap::iterator pos = range.first; pos != range.second; ++pos)
	   		SiloCallback *callback_confirm = pos->second.silo_callback;
	   		callback_confirm->onTick(ric, value);

	7. void SiloDecoupler::Couple::onTick(const SiloRic &ric, const double &value)
	8. void SiloDecoupler::onTick(const Tick_ptr& tick)
			m_tickQueue.put(tick);


Driver: SiloWithManagedSubscriptions::m_initialTickTask
	
	1. bool SiloWithManagedSubscriptions::InitialTickProcessor()
	2. void SiloDecoupler::Couple::onTick(const SiloRic &ric, const double &value)
	3. void SiloDecoupler::onTick(const Tick_ptr& tick)
			m_tickQueue.put(tick);


Subscription initialization may result in two ticks:

The first one will come from the decoupler thread:

Driver: the subscription thread -> ReutersProvider internal thread -> decoupler thread

	1. void FwdPointsGenerator::start() <- subscriber thread
	2. void BrokerPageFwdPointsGenerator::onStart()
	3. void MarketDataLayer::start()
	4. void MarketDataLayer::initialise()
	5. MarketPointPtr MarketDataSource::subscribe(const ml::fx::common::Tenor & tenor)
	6. void BrokerSource::subscribe()
	7. void SiloWithManagedSubscriptions::doSubscribe(const Ric &ric, SiloCallback *silo_callback)
	8. void SiloReutersImpl::onFirstSubscribe(const SiloRic& siloRic)
	9. bool ReutersProvider::pushTick(const Ric &ric)
		    m_tickQueue.put(tick);
	10. bool ReutersProvider::onProcess() <- ReutersProvider thread
	11. void SiloReutersImpl::onTick(int64 sequence, const RfaRic& rfaRic, const RfaMsg& rfaMsg)
	12. void SiloWithManagedSubscriptions::sendTick(const Ric &ric, const double &value, SiloCallback *) // bid
	13. void SiloDecoupler::Couple::onTick(const SiloRic &ric, const double &value)
	14. void SiloDecoupler::onTick(const Tick_ptr& tick)
			m_tickQueue.put(tick); <- to be sent by the decoupler thread


The second one comes from the subscription thread itself:

	1. void FwdPointsGenerator::subscribe(const mlc::util::Date & valueDate, const std::string & stdTenor)
	2. void BrokerPageFwdPointsGenerator::publishSnapshot(const mlc::util::Date & valueDate, const std::string & stdTenor)
	3. void UserSubscriptionsHandler::publishSnapshot(const mlc::util::Date & valueDate, const std::string & stdTenor)
	4. void ShortTermDatesExtrapolator::onTick(const InternalTick & tick)
	5. void BrokerPageFwdPointsGenerator::onTick(const BrokerPageGeneratorInternal::InternalTick & tick)


Similar when stopping, a tick with reason "Staled with de-activation" is pushed from the calling thread

	1. void FwdPointsGenerator::stop()
	2. void BrokerPageFwdPointsGenerator::onStop()
	3. void UserSubscriptionsHandler::stop()
	4. void ShortTermDatesExtrapolator::onTick(const InternalTick & tick)
	5. void BrokerPageFwdPointsGenerator::onTick(const BrokerPageGeneratorInternal::InternalTick & tick)

Driver: SiloDecoupler

	1. bool SiloDecoupler::onProcess() { callback->onTick(tick->ric, tick->value); }
	2. void MarketDataSource::onTick(const SiloRic & ric, const double & value) // BPGL1
	3. void MarketDataSource::processTick(const SiloRic & ric, const ml::fx::common::Price & price)
	4. void MarketDataLayer::onNewMarketData(Date & date, Side & side, Price & price) // BPGL2
	5. void MarketDataLayer::processPoint(const MarketPointPtr & point)
	6. UserSubscriptionsHandler::onTick
	7. ShortTermDatesExtrapolator::onTick
	8. void BrokerPageFwdPointsGenerator::onTick(const BrokerPageGeneratorInternal::InternalTick & tick)




# Subscription Management at Gateway

The gateway manages each client as a Session, which is identified by CredentialHeader.

GUI client sends FXFwdTickProvider messages, which are forwarded to FwdServer by the gateway.
The messages are handled by ForwardClientServerImpl. In addition to creating new subscriptions
when necessary, it also calls GatewayContract::addFwdRouting/addFwdPreviewRouting/addFwdPreviewRouting2
to setup routing in Gateway, credentials are passed back in these calls.

On Gateway, addFwdRouting/addFwdPreviewRouting/addFwdPreviewRouting2, new subscriptions are added
to RateSubscriptionManager

	void
	RateSubscriptionManager::addFwdSubscription(const CredentialHeader& credentials,
												const FwdCCyPairTenor& CCyTenPair,
												bool previewMode )
	{
		// get client session by credentials
		Session_Ptr  session = manager.getCredentialManager().getSession(credentials);

		// get all the sessions for the (ccypair, tenor)
		set<Session_Ptr>& sessions = m_FwdSubscriptions[CCyTenPair];
		...
		// add the session to session set
		sessions.insert(session);

		// add the (ccypair, tenor) to the session
		SessionRateSubscriptions::Ptr ptr = session;
		ptr->getFwdCCyTenorSubscriptions().insert(CCyTenPair);
	}




# FwdPointsSelector bubblenet


For live subscription:

	pri_source_node   ref_source_node   *threshold_limit_cfg*    mode_source_node       
	      |                  |                     |                     |
	      +------------------+----------+----------+---------------------+
	                                    |
	                                    |    *lhs_spread_cfg*    *rhs_spread_cfg* 
                                        |            |                   |
                                        |            +---------+---------+
                                        |                      |
                                        |             spread_interpolator  *fast_market_cfg*
                                        |                      |                  |
                                        |                      +---------+--------+
                                        |                                |
                                pri_ref_selector                   fm_multiplier
                                        |                                |
                                        +----------------+---------------+
                                                         |
      *fwd_points_stale_cfg* (specific tenor)      spread_applier     
                    |                                    |
                    +------------------+-----------------+
                                       |
                              sanity_controller   skew_source_node
                                       |                 |
                                       +---------+-------+
                                                 |
                                         skew_applier_node
                                                 |
      *fwd_points_stale_cfg* (tenor "ALL")  rounding_node  
                    |                            |
                    +--------------+-------------+
                                   |
                           region_stale_node
                                   |
                       FwdPointsSelector::onTick
                                                                                                                              
 

	pri_source_node:        InternalFwdTick, primary tick
	ref_source_node:        InternalFwdTick, reference tick
	*threshold_limit*:      ThresholdLimitMutableSourceTickerConfig, threshold limit
	mode_source_node:       bool, reference curve status
	*fast_market_cfg*:      FastMarketMutableSourceTickerConfig, fast market cfg
	*lhs_spread_cfg*:       SpreadMutableSourceTickerConfig, near tenor spread cfg
	*rhs_spread_cfg*:       SpreadMutableSourceTickerConfig, far tenor spread cfg
	*fwd_points_stale_cfg*: FwdPointsStaleMutableSourceTickerConfig, forward points stale cfg
	skew_source_node:       Decimal, forward skew

	pri_ref_selector:       InternalFwdTick, PriRefSelector, selector
	spread_interpolator:    SpreadValue, SpreadInterpolater, spread interpolator
	fm_multiplier:          SpreadValue, ApplyFM, fast market multiplier
	spread_applier:         InternalFwdTick, ApplySpread, apply spread
	sanity_controller:      InternalFwdTick, SanityController, sanity check
	skew_applier_node:      InternalFwdTick, ApplySkew, apply skew
	rounding_node:          InternalFwdTick, TickRounder, rounding
	region_stale_node:      InternalFwdTick, RegionStaleNode, region stale

The graph is different for basic preview or when sanity checking is turned off. See FwdPointSelector.cpp 
for details.

