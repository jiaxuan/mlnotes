
<!-- ################################################################################# -->

# Introduction

A request (`OutrightInput.hpp`) may be of different types:

- full request: `requestType == FXOutrightRequest::FULL` 
- points request: `requestType == FXOutrightRequest::POINTS` 

A request may also ask for different rates:

- trader rate: `rateType == FXOutrightRequest::TRADER_RATE` 
- sales rate: `rateType == FXOutrightRequest::SALES_RATE`

Different pricing model may be used, indicated by model:

- direct spot: `model & DIRECT_SPOT`
- crossed spot: `model & CROSSED_SPOT`
- direct forward: `model & DIRECT_FWD`
- crossed forward: `model & CROSSED_FWD`

ROE, LVN, NDF, PM are decided by ccyPairType.

Tenor types are decided in the following manner:

	bool isTDY()		tenorType == Tenor::TT_TDY
	bool isTOM()		tenorType == Tenor::TT_TOM
	bool isSpot()		tenorType == Tenor::TT_SPOT
	bool isPostSpot()	tenorType == Tenor::TT_POST_SPOT

	bool isFwd()		!isSpot()
	bool isPreSpot()	isTDY() || isTOM()

IMM dates have the format of:

	IM1, IM2, IM3, IM4
or

	IMM1, IMM2, IMM3, IMM4


## Unified Pricing 2.0_v3.0.docx

Apply a set of bid / ask skews to the tiered bid and ask spot prices for each core pricing algorithm. This shifts the mid points up or down as a function of overall currency holdings to encourage trading in a particular direction:

- Skew engine will not be part of MLFX. **Skew generating logic will live in AXE**, which will generate the skew values and skew quantities for every **autohedged** currency pair. 
- The output of the AXE skew engine will be sent to MLFX **Spot Server** where bid/ask skews will be calculated in pips and applied to the tiered bid and ask spot prices output from the spot server.
- AXE will have the functionality to generate more than one instance of bid/ask skews per currency pair.  


TPS related settings in table `Configuration`:

	select * from Configuration where keyName like 'TPS%'

	id    keyName                          value             description                                                                                 
	----- -------------------------------- ----------------- ------------------------------------------------------------------------------------------- 
	38016 TPS_DualPricingAuditTimeInterval 30000000          GUS Spider Dual pricing - frequency of audit logs                                           
	38014 TPS_DualPricingEnabled           true              GUS Spider Dual pricing logic will be enabled if this parameter is set to TRUE              
	38017 TPS_DualPricingGoodTicksNeeded   2                 GUS Spider Dual pricing - number of ticks needed before switch back to primary is to happen 
	38015 TPS_DualPricingSecondarySource   DefaultLabProfile GUS Spider Dual pricing - name of the secondary spot pricing source                         
	60031 TPS_FMFactorMidModelThreshold    3.0               Threshold to control Volatility Factor application                                          


<!-- ################################################################################# -->

`TradeablePriceServer` structure:

	FwdCachePrefetch::Ptr 			m_fwdCachePrefetch;

	// observers 
	mlc::pattern::AutoObserver::Ptr m_calendarObserver;
	mlc::pattern::AutoObserver::Ptr	m_spotProviderObserver;

	// upstream data: spot, forward, data, skew
	SpotProviderInterface::PtrList	m_spotProviderList;
	SpotProviderInterface::Ptr		m_rvSpotProvider;
	FwdProvider::Ptr				m_fwdProvider;
	DataProvider					m_dataProvider;
	fx_unified_tps::SkewProvider	m_skewProvider;	// subscription to the skew library

    // handle dual source providers
    local::DualPricingConfig        m_dualPricingConfig;

	// calculation
	QuoteCalculator					m_quoteCalculator;
	QuoteCache						m_quoteCache;

	// subscription
	SubscriptionManager				m_subscriptionManager;
	QuotePublisher					m_quotePublisher;


It provides the following services:

- Spot/Forward/Swap subscription
- TPS Audit
- Trade broadcast


The following APIs are used to select spot provider from TPS:

	SpotProviderInterface::Ptr TradeablePriceServer::spotProvider(const string& profileName, const bool useGusPricing) const
		if(useGusPricing)
			if(m_dualPricingConfig.size() != 0)
				return m_dualPricingConfig.getDualProvider(profileName);
		return spotProvider(profileName);

	/* Note this implmenetation is a little different than on the GUI TPS, here we use spot 
	 * profile name and on the GUI it uses spot Algo Name. 
	 */
	SpotProviderInterface::Ptr TradeablePriceServer::spotProvider(const string& profileName) const
		foreach( providerItr , m_spotProviderList )
			SpotProviderInterface::Ptr  provider = *providerItr ;
			if( provider && provider->getProfile() && provider->getProfile()->getName() == profileName ) 
				return provider;
		return m_rvSpotProvider;

In most cases, the RV provider is used. Non-RV providers are used only for AXE managed currency pairs:

	void AXEOutrightSubscription::reload()
		string labProfileName <- FXGroup[spotProfile] ("DefaultLabProfile" if not found) for m_request.getGroupId()
		bool isGroupDualPriced <- check if FXSpotPricingAlgoProperties[corePricingType] is GUS_PRICING for labProfileName

		const bool gusPricingEnabled (
			ConfigurationSettings::isDualPricingActive()	// Configuration[TPS_DualPricingEnabled]
			&& isGroupDualPriced							// FXSpotPricingAlgoProperties[corePricingType]
			&& isCurrencyDualPriced(m_request.getCcyPair())); // AXEManagedCcyPairs[isGusPricingEnabled]

		if(!gusPricingEnabled)
			if(isGroupDualPriced)
				// fall back to the default profile
				labProfileName = ConfigurationSettings::secondaryDualPricingSource();

		DataProvider& dp = m_tps.dataProvider();
		SkewProvider & skp = m_tps.skewProvider();
		FwdProvider & fp = m_tps.fwdProvider();
		SpotProviderInterface& sp = m_tps.spotProvider(labProfileName, gusPricingEnabled);

<!-- ################################################################################# -->

# Providers

These are interfaces to upstream data servers.

<!-- ===================================================== -->
## Spot Providers

There are two sets of spot providers:

	SpotProviderInterface::PtrList	m_spotProviderList;	// multiple from FXSpotServerProfile
	SpotProviderInterface::Ptr		m_rvSpotProvider;	// single "RV" provider

	void TradeablePriceServer::startAllSpotProviders()
		foreach( configIter in table FXSpotServerProfile )
			SpotProviderInterface::Ptr newP(new SpotProvider((*configIter))); [
				SpotProvider::SpotProvider(const FXSpotServerProfile::Ptr& profile):m_profile(profile)
					// note that pricingAlgoName is used as spot clients profile name
					m_spotClient.reset( new UnifiedSpotClient(*this, m_profile->getPricingAlgoName()));
			]
			m_spotProviderList.push_back( newP );

		FXSpotServerProfile::Ptr rvProfile(new FXSpotServerProfile("RV", "RV" ,  "RV" , "RV"));
		m_rvSpotProvider.reset(new SpotProvider(rvProfile));
	 
		m_spotProviderObserver.reset( new mlc::pattern::AutoObserver(Cache<FXSpotServerProfile>::instance(), 
			boost::bind(&TradeablePriceServer::onSpotServerProfileChange, this, _1, _2)) );


From `global_db::FXSpotServerProfile`
	
	id   name              pricingAlgoName spreadAlgoName skewAlgoName 
	---- ----------------- --------------- -------------- ------------ 
	1000 DefaultLabProfile Lab1            LabSpotSpread  LabSkew      
	2000 Lab2Quants        Lab2            Lab2SpotSpread Lab2Skew     
	3000 Lab3Quants        Lab3            Lab3SpotSpread Lab3Skew     
	5001 Lab4              Lab4            Lab2SpotSpread Lab2Skew     
	6000 spider            spider          LabSpotSpread  LabSkew      
	6001 spider2           spider2         LabSpotSpread  LabSkew      
	6002 spider2_gui       spider2_gui     LabSpotSpread  LabSkew      
	6003 spider2_rej       spider2_rej     LabSpotSpread  LabSkew      
	6004 spider2_rtl       spider2_rtl     LabSpotSpread  LabSkew      

From `global_db::FXSpotPricingAlgoProperties`

	id   name        corePricingType 
	---- ----------- --------------- 
	1000 Lab1        0               
	2000 Lab2        0               
	3000 Lab3        0               
	5001 Lab4        0               
	6000 spider      2               
	6001 spider2     2               
	6002 spider2_gui 2               
	6003 spider2_rej 2               
	6004 spider2_rtl 2   

The following pricing types are defined:

	enum CorePricingAlgoType (LAB_PRICING , RV_PRICING, GUS_PRICING)

There are three spot servers:

- fx_unified_spot_server
- fx_unified_rv_spot_server     
- fx_gus_spot_server

From `global_db::FXSpotSpreadAlgoProperties`

	id   name           spotTierType 
	---- -------------- ------------ 
	2000 Lab2SpotSpread 0            
	3000 Lab3SpotSpread 0            
	1000 LabSpotSpread  0            

From `global_db::FXSpotSkewAlgoProperties`

	id   name     skewAlgoType 
	---- -------- ------------ 
	2000 Lab2Skew 0            
	3000 Lab3Skew 0            
	1000 LabSkew  0            



The code:

	class SpotProvider : public UnifiedSpotClientCallback, SpotProviderInterface

	FXSpotServerProfile		m_profile;		// profile
	UnifiedSpotClient::Ptr 	m_spotClient;	// spot client

	struct Binding
		tps::EventListener::WPtr	listener;
		FXSpotTickWithDepth::Ptr	&target;

	// tick with subscriber list
	struct TickData 
		FXSpotTickWithDepth::Ptr	tick;
		std::list<Binding>			bindings;

	unordered_map<string, Decimal>	m_multiplierMap;
	unordered_map<string, TickData> m_dataMap; // ccyPair -> TickData

	SpotProvider::SpotProvider(const FXSpotServerProfile::Ptr& profile):m_profile(profile)
		m_spotClient.reset( new UnifiedSpotClient(*this, m_profile->getPricingAlgoName()) );


Implementing the `SpotProviderInterface`:

	virtual void start();
	virtual void stop();
	virtual FXSpotServerProfile::Ptr& getProfile();
    virtual const std::vector< std::pair<int64, String> > getSpotServerProfileName() const;
	virtual FXSpotTickWithDepth::Ptr getSpotTick(const std::string & ccyPair);

    // regular provider always provides raw, unskewed data
    virtual bool providerCanSkewData() const { return false; }

    // note there is no unregister
	virtual void registerListener(const CurrencyPair& ccyPair, 
		const tps::EventListener::Ptr & listener, FXSpotTickWithDepth::Ptr& target)

		Binding binding(listener, target);

		TickDataMap::iterator iter = m_dataMap.find( ccyPair );
		// not the first subscriber
		if( iter != m_dataMap.end() ) // add listener to list and notify
			TickData & data = iter->second;
			data.bindings.push_back( binding );
			notify( binding, data.tick );
			return;

		// create a new ccypair in data map
		TickData& data = m_dataMap[ ccyPair ];
		// put an empty entry into the mapping 
		data.tick.reset(new FXSpotTickWithDepth(...));
		data.bindings.push_back( binding );
	    // no notify, wait for a real tick from the spot server


	// denomination currency is the base currency in a transaction, or the currency
	// in which a financial asset is quoted.
	virtual bool getDenominationMultiplier(const std::string& quotedCcy, const std::string& denomCcy, Decimal& multiplier)

		int32 quotedRank, denomRank;
		FXCacheUtils::getCurrencyRank(quotedCcy, quotedRank);
		FXCacheUtils::getCurrencyRank(denomCcy, denomRank);

		if( quotedRank == denomRank )
			assert( quotedCcy == denomCcy );
			multiplier = ml::common::math::Decimal::ONE;
			return true;

		Nullable<ml::common::math::Decimal> mid;
		bool result = false;

		// 1. check out map to see if there is a direct spot tick for the two
		if( quotedRank > denomRank ) // quoted ccy is the base currency
			mid = getTickRawMid( getSpotTick(quotedCcy + denomCcy) );
			multiplier = *mid;
		else // quoted ccy is the terms currency
			mid = getTickRawMid( getSpotTick(denomCcy + quotedCcy) );
			multiplier = ml::common::math::Decimal::ONE / *mid;

		if multiplier already set
			return true;

		//  2. still use our map, try to go through USD or EUR
		result = getCrossDenominationMultiplier(quotedCcy, quotedRank, denomCcy, denomRank, "USD", multiplier);
		if( !result )
			result = getCrossDenominationMultiplier(quotedCcy, quotedRank, denomCcy, denomRank, "EUR", multiplier);

		// If successful, store it in our multiplier map
		if( result )
			m_multiplierMap[ quotedCcy + denomCcy ] = multiplier;
		// 3. try to use one that's already in the multiplier cache
		else
			MultiplierMap::const_iterator iter = m_multiplierMap.find(quotedCcy + denomCcy);
			if( result = (iter != m_multiplierMap.end()) )
				multiplier = iter->second;

			// 4. as a last resort, use FXSpotUSDRate to convert between the two
			if( !result )
				// If we use an initial volume of 1, the resultant volume will be the same as the multiplier to use
				result = FXCacheUtils::convertBetweenCcys(ml::common::math::Decimal::ONE, quotedCcy, denomCcy, multiplier);
		return result;

Spot event handling:

	// connected, get a snapshot and notify
	void SpotProvider::onSpotConnect()
		boost::shared_ptr< FXSpotTickWithDepth::PtrList > ticks = m_spotClient->getAllSnapshots();
		foreach( tickIter, * ticks.get() )
			const FXSpotTickWithDepth::Ptr & newTick = * tickIter;
			TickData & data = m_dataMap[ newTick->getCcyPair() ];
			if( !data.tick || data.tick->getTickId() < newTick->getTickId() )
				data.tick = newTick;
				notify( data.bindings, data.tick );

	// disconnected, stale quote for all existing subscription
	void SpotProvider::onSpotDisconnect()
		foreach( dataIter, m_dataMap )
			TickData & data = const_cast<TickData &>(dataIter->second);
			data.tick.reset(new FXSpotTickWithDepth(0, -1, ...));
			notify( data.bindings, data.tick );

	// FXSpotTickWithDepth.configId is FXSpotServerProfile.id
	// FXSpotTickWithDepth.tickId is timestamp (us) of when the tick was sent

	// got a tick, note this has the effect of subscribing as well
	void SpotProvider::onSpotTick(const FXSpotTickWithDepth& newTick)

		const std::string & ccyPair = newTick.getCcyPair().str();
		TickDataMap::iterator iter = m_dataMap.find( ccyPair );

		if( iter != m_dataMap.end() )
			TickData & data = iter->second;
			if( !data.tick || data.tick->getTickId() < newTick.getTickId() )
				data.tick = FXSpotTickWithDepth::PtrCreate(newTick);
				notify( data.bindings, data.tick );
			return;

		// First time this currency pair has been accessed, add it to map
		TickData & data = m_dataMap[ ccyPair ];
		// Do this check here anyway in case we got something between release the
		// read lock and obtaining the write lock
		if( !data.tick || data.tick->getTickId() < newTick.getTickId() )
			data.tick = FXSpotTickWithDepth::PtrCreate(newTick);
			notify( data.bindings, data.tick );



	void SpotProvider::notify(Binding & binding, const FXSpotTickWithDepth::Ptr & tick)
	if( tps::EventListener::Ptr ptr = binding.listener.lock() )
		SpotEvent::Ptr event( new SpotEvent(tick, binding.target) );
		ptr->onEvent( boost::static_pointer_cast<tps::Event, SpotEvent>(event) );



When registering listener, a listener provided target ref is saved:

	struct Binding
		tps::EventListener::WPtr	listener;
		FXSpotTickWithDepth::Ptr	&target;
		Binding(const tps::EventListener::Ptr & l, FXSpotTickWithDepth::Ptr & t) : listener(l), target(t) { }

	void SpotProvider::registerListener(ccyPair,
										const tps::EventListener::Ptr & listener,
										FXSpotTickWithDepth::Ptr & target)
		Binding binding(listener, target);


When creating the event, both target ref and tick data are used, and the tick data is simply 
copied to target in `onProcess`. The usage pattern is that event handler would alway expect 
event data through pre-determined data member.

	struct SpotEvent : public tps::Event
		SpotEvent( const FXSpotTickWithDepth::Ptr & d, FXSpotTickWithDepth::Ptr & t) : data(d), target(t) { }
		void onProcess() { target = data; }
		ml::spec::fx::spot::FXSpotTickWithDepth::Ptr getData() const { return data; }
		
		ml::spec::fx::spot::FXSpotTickWithDepth::Ptr data;
		ml::spec::fx::spot::FXSpotTickWithDepth::Ptr & target;

Similarly,

	struct FwdEvent : public tps::Event
		FwdEvent( const FXFwdTick::Ptr & d, FXFwdTick::Ptr & t ) : data(d), target(t) { }
		void onProcess() { target = data; }
		
		ml::spec::fx::fwd::FXFwdTick::Ptr data;
		ml::spec::fx::fwd::FXFwdTick::Ptr & target;

	template <class T>
	struct DataEvent : public tps::Event
		DataEvent(const boost::shared_ptr<T> & d, boost::shared_ptr<T> & t, int32 k)
			: data(d), target(t), kind(k) { }
		void onProcess() { target = data; }

		typename boost::shared_ptr<T> data;
		typename boost::shared_ptr<T> & target;
		int32 kind;


### Dual (Spot) Provider


	class DualPricingConfig

    const local::TradeablePriceServerInterface     &m_tpsInterface;
    std::map< int64, SpotProviderInterface::Ptr >   m_dualSourceProvider;

	// start any configuration found to be type spider
	void DualPricingConfig::startDualProviders()
		// check global table Configuration[kayName="TPS_DualPricingEnabled"]
	    if(ConfigurationSettings::isDualPricingActive())
	        startProviders();

	// start any configuration found to be type spider
	void DualPricingConfig::startProviders()

		// load FXSpotServerProfile into profiles
	    FXSpotServerProfile::PtrList profiles;
	    loadDBConfiguration(profiles);
	    // get secondary provider
	    const std::string secondaryProviderProfileName(ConfigurationSettings::secondaryDualPricingSource());
	    SpotProviderInterface::Ptr secondary = m_tpsInterface.spotProvider(secondaryProviderProfileName);
	    if(secondary)
	    	// find all profiles with a pricingAlgo type of GUS to be primary 
	        foreach( profile, profiles )
	            std::pair<bool,FXSpotPricingAlgoProperties::Ptr> kvpair = getAlgoProperties((*profile)->getPricingAlgoName());
	            if ( !kvpair.first )
	                continue;
	            FXSpotPricingAlgoProperties::Ptr algoType = kvpair.second;
	            SpotProviderInterface::Ptr primary = m_tpsInterface.spotProvider((*profile)->getPricingAlgoName().toString());
	            m_dualSourceProvider[(*profile)->getId()] = new DualSourceProvider(primary, secondary);


	// check if a profiles pricingAlgo is of type GUS
	std::pair<bool, FXSpotPricingAlgoProperties::Ptr>
	DualPricingConfig::getAlgoProperties(const String& name) const
	    FXSpotPricingAlgoProperties::Ptr algoType = Cache<FXSpotPricingAlgoProperties>::instance()[name];
	    if(algoType && algoType->getCorePricingType() == FXSpotPricingAlgoProperties::GUS_PRICING)
	        return std::pair<bool, FXSpotPricingAlgoProperties::Ptr>(true, algoType);
	    return std::pair<bool, FXSpotPricingAlgoProperties::Ptr> (false, FXSpotPricingAlgoProperties::Ptr());


The actual dual provider:

	class DualSourceProvider : public SpotProviderInterface
		SpotProviderInterface::Ptr m_primary;
		SpotProviderInterface::Ptr m_secondary;

	void DualSourceProvider::registerListener(const CurrencyPair& ccyPair, 
		const tps::EventListener::Ptr& listener, FXSpotTickWithDepth::Ptr& target)
		// listener must be mixer, such as AXEOutrightSubscription::m_eventMixer
		tps::event::Mixer::Ptr mixer = boost::static_pointer_cast<tps::event::Mixer>(listener);
		// provide some more log entry detail to the mixer
		mixer->setAdditionalInfomation(ccyPair);
		m_primary->registerListener(ccyPair, mixer->get(tps::event::Flow::PRIMARY), target);
		m_secondary->registerListener(ccyPair, mixer->get(tps::event::Flow::SECONDARY), target);


<!-- ===================================================== -->
## Data Provider

`detail/provider/data/DataProvider.hpp`

	class DataProvider :
		public ml::spec::fx::tps::data::UnifiedTPSDataListener,
		public ml::spec::fx::tps::data::UnifiedTPSGeneralDataListener,
		public mlc::util::HeartbeatTimer

	struct BindingData { std::list<Binding> bindings; ... } // libs/fx/eps_framework/provider/Provider.hpp
	typedef std::map<std::string, BindingData> BindingDataMap;
	BindingDataMap m_dataMap;	// ccypair -> subscriber list


<!-- ===================================================== -->
## Skew Provider

This is used only for AXEOutrightSubscription.



<!-- ################################################################################# -->

# Subscription Manager

Provides subscription services

	WorkQueue<SubscriptionQueueItem, ConsolidatedQueue<SubscriptionQueueItem>> m_queue;
	list< Task::Ptr > m_threads;

	map<string, map<FXRequestId, Subscription::Ptr>> m_subscriptions; // clientId -> [tpsRequestId -> Subscription]
	map< FXRequestId, int64 >	m_SubscriptionSeqNoMap; // tpsRequestId -> seqNo

	// heartbeat
	Subscription::Ptr m_heartbeatSubscription;
	ClientHeartbeatTracker m_clientHeartbeatTracker;

	// handles incoming client requests
	FXOutrightProviderImpl::Ptr m_provider;			// outright
	FXSwapProviderImpl::Ptr 	m_swapProvider;		// swap

	// the stat manager publishes subscription related events by FXTpsStatsListenerProxy,
	// the data is used by libs/fx/tpsclient3/connection/TpsDynamicLoadBalancedConnectionManager
	// maybe we can use this to monitor TPS subscriptions ?
	TpsStatsManager_ptr m_tpsStatsManager_ptr;


## Subscription

Create client interfaces:

	void SubscriptionManager::start()
		m_provider.reset( new FXOutrightProviderImpl(instanceName) );
		m_swapProvider.reset( new FXSwapProviderImpl(instanceName) );


The client interfaces forward actual work to manager (leaky):

	void FXOutrightProviderImpl::subscribe(subscriberId, requests, topic, seqNo)
		TradeablePriceServer & tps = TradeablePriceServer::instance();
		tps.subscriptionManager().subscribe(subscriberId, requests, topic, seqNo);


### New subscription:

	// what is the purpose of seqNo ?
	void SubscriptionManager::subscribe(subscriberId, requests, topic,seqNo)
		SubscriptionMap & submap = m_subscriptions[ subscriberId ];			
		foreach( request, requests )
			// create subscription
			const FXRequestId & requestId = request->getRequestId(); // tps req id
			Subscription::Ptr & ptr = submap[ requestId ];
			if( ptr ) ptr.reset();	// no clean up necessary ?
			ptr = SubscriptionFactory::createInstance( subscriberId, * request, topic );
			enqueueForReloading(ptr);
			// track seqNo
			m_SubscriptionSeqNoMap[ requestId ] = seqNo;
			// heartbeat tracking
			m_clientHeartbeatTracker->clientConnected(subscriberId, bind(&SubscriptionManager::clientDisconnected...));
			// publish stat event
			m_tpsStatsManager_ptr->onSubscribe(subscriberId,request);

Creating subscription from request:

	Subscription::Ptr SubscriptionFactory::createInstance(subscriberId,request, topic)
		// Determine if the curreny pair is a reciprocal and adjust, here we will allow failure to find a ccy pair
		// to be propagated from the main error handling.  We assume a best case we are focused on reciprocal
		FXOutrightRequest userRequest(request);
		if(isRequestReciprocal(request)) userRequest = getInvertedRequest(request);

		// set republication interval
		bool republicationIntervalActive = RegistrySettings::isRepublicationIntervalActive();
		// check if we are allowed to republish quotes
		if( republicationIntervalActive )
			// if the end user has no time specified then turn off republish on this subscription
			if (userRequest.getRepublicationInterval() == 0 )
				republicationIntervalActive = false;
			else if (userRequest.getRepublicationInterval() < RegistrySettings::getMinRepublicationInterval())
				// too low - reset the interval to the minimum value
				userRequest.setRepublicationInterval(RegistrySettings::getMinRepublicationInterval());

		// Turn on the publication delay for a better pricing ?
		const bool betterFirstPrice = RegistrySettings::getBetterPricingMarker();

		// If AXE managed currency, create AXEOutrightSubscription
		if (AXEManagedCcyPairs[userRequest.getCcyPair()]) 
			return new AXEOutrightSubscription(...);
		else
			return new RVOutrightSubscription(...);

The actual processing:

	SubscriptionManager::onProcess()
		m_queue.get(item)
		item.ptr->reload()

	static const std::string RV_SPOT_SERVER_PROFILE = "RV";

	// detail/subscription/outright/rv/RVOutrightSubscription_Main.cpp
	RVOutrightSubscription::reload()
		DataProvider dp <- m_tps.dataProvider();
		SpotProviderInterface sp <- m_tps.spotProvider(RV_SPOT_SERVER_PROFILE); // use RV 
		FwdProvider fp <- m_tps.FwdProvider();

		// setup event forwarder to forward events to self
		m_eventForwarder <- ...
		// clear old events
		m_events.clear();
		// setup initial invalid quote
		m_quote <- ...
		// clear input data
		m_input.reset()	// this also clear children subscriptions
		// init input from request
		m_input <- ... m_request...
		// clear output data
		m_output ...

		setState(PENDING_DATA);

		// subscribe to DataProvider
		if (!m_registeredSubscription)
			dp.registerSubscription(m_input.ccyPair, Subscription::getAsSubscription<OutrightSubscription>(this));
			m_registeredSubscription = true;

		// set current trading date
		m_cal.getCurrentTradingDay(m_request.getCcyPair().str(), m_input.tradingDay)
		// if reciprocal, no USD should be involved
		if (isReciprocalRate()) {...}

		// check ccypair, tenor, valueDate

		// check if valueDate is a broken date
		if (Tenor::isBrokenDate(m_request.getValueDate())) {

			// Note: broken date is expected to have the format of	ISO_YYYYMMDD
			// the logic of checking is very simple:
			bool Tenor::isBrokenDate( const std::string & isoDate )
				const unsigned int ISO_CCYYMMDD = 8;
				return isoDate.size() >= ISO_CCYYMMDD;

			// the logic of validation is also simply a conversion
			try {
				requestDate = Date(m_request.getValueDate(), Time::ISO_YYYYMMDD, Time::UTC);
			} catch (...) { // failed to convert, error }

			// check if it is a working day
			if (!m_cal.isWorkingDay(ccyPair, requestDate)) { ... }

			tenors = m_cal.getSurroundingTenors(m_input.ccyPair, requestDate, m_input.tradingDay);
			if( tenors.first == tenors.second ) /// a direct tenor
				if( tenors.first == Tenor::SPOT )		m_input.tenorType = Tenor::TT_SPOT;
				else if( tenors.first == Tenor::TDY )	m_input.tenorType = Tenor::TT_TDY;
				else if( tenors.first == Tenor::TOM )	m_input.tenorType = Tenor::TT_TOM;
				else									m_input.tenorType = Tenor::TT_POST_SPOT;
				
				//If TOM and SP are on same day give preference to SP in determining the TENOR for broken date
					if (tenors.first ==  ml::fx::common::Tenor::TOM)
						Date spotDate = m_cal.getSpotDate(m_input.ccyPair, m_input.tradingDay);
						if (spotDate == requestDate )
							m_input.tenorType =  ml::fx::common::Tenor::TT_SPOT;

				if( m_input.isFwd() )
					m_input.fwd.tenor = tenors.first;
					requestTenor = *m_input.fwd.tenor;
			else // otherwise use POST_SPOT !!!
				m_input.tenorType = Tenor::TT_POST_SPOT;
		} else {
			...
		}

		// note that the various request processing results are saved in m_input
		// as m_input holds all the data necessary to calculate the output

		// set quote precision

		// set multipliers ... called the following
		// sp.getDenominationMultiplier(m_input.volumeCcy, "USD" , usdMultiplier )
		// sp.getDenominationMultiplier(m_input.volumeCcy, m_input.denomCcy, m_input.denomMultiplier)
		// sp.getDenominationMultiplier("USD", m_input.ccyPair.baseCcy(), m_input.baseFactorUSD)
		// sp.getDenominationMultiplier("USD", m_input.ccyPair.termsCcy(), m_input.termsFactorUSD)

		// check/set volume
		// check/set band

		Cross currency settings come from two tables:
		FXCcyPairCrosses for spot
		FXCcyPairFwdCrosses for fwd 

		// check for spot crossing, this is always done
		dp.getSpotCrossCurrency(m_input.ccyPair, m_input.spot.crossingCcy) ...
		if ... m_input.model |= DIRECT_SPOT  else ... m_input.model |= CROSSED_SPOT

		if (m_input.isFwd()) 
			// this is done for fwd only
			dp.getFwdCrossCurrency(m_input.ccyPair, m_input.fwd.crossingCcy)
			...
			if ... m_input.model |= DIRECT_FWD else ... m_input.model |= CROSSED_FWD

		// Store whether they're crossing the same way
		m_input.sameCrossingCcy = m_input.spot.crossingCcy == m_input.fwd.crossingCcy;
	
		if( m_input.isSpotCrossed() || m_input.isFwdCrossed())
			... get currency ranks
			m_input.spot.ccyXIsBaseInBaseLeg = spotCrossRank > baseRank;
			m_input.spot.ccyXIsBaseInTermsLeg = spotCrossRank > termsRank;
			...
			m_input.fwd.ccyXIsBaseInBaseLeg = fwdCrossRank > baseRank;
			m_input.fwd.ccyXIsBaseInTermsLeg = fwdCrossRank > termsRank;

		... 

		// register for spot market data
		if( m_input.model & DIRECT_SPOT == DIRECT_SPOT)
			if (!FXCurrencyUtils::isCurrencyPairSpecial(m_input.ccyPair) && m_tps.HANDLE_SPOT_TICK_CROSSING )

				// subs for static data for fastMarket and indicative changes in spot crossing currency candidates
				createDataSubscription( m_input.ccyPair.baseCcy(),  "USD", m_input.spotCrossDollarData.base );
				createDataSubscription( m_input.ccyPair.termsCcy(), "USD", m_input.spotCrossDollarData.terms );

				createDataSubscription( m_input.ccyPair.baseCcy(),  "EUR", m_input.spotCrossEuroData.base );
				createDataSubscription( m_input.ccyPair.termsCcy(), "EUR", m_input.spotCrossEuroData.terms );

			// subscribe to spot provider
			sp.registerListener(m_input.ccyPair, m_eventForwarder, m_input.spot.tick);

		// register for fwd market data
		if( m_input.model & DIRECT_FWD == DIRECT_FWD )
			fp->registerListener(m_input.ccyPair, *m_input.fwd.valueDate, m_eventForwarder, m_input.fwd.tick);

		... more static data subscriptions

		// Create TOM subscription if we're pricing ON !!!!!
		if( m_input.isTDY() )
		{
			FXOutrightRequest requestTOM( m_request );
			requestTOM.setValueDate( Tenor::TOM );
			requestTOM.setRequestType( FXOutrightRequest::POINTS );

			//Remove antisweep volume else it will be added again in the OutrightSubscription constructor.
			sweep::removeAntiSweepVolumeFromRequestVolume(requestTOM);
			// Create our TOM co-subscription with the same request options as ours, except with a different tenor
			m_input.subTOM.reset( new RVOutrightSubscription(requestTOM, 
				this->shared_from_this(), depth(), false, false, false, ignoreFM()) );
			m_tps.subscriptionManager().enqueueForReloading( boost::static_pointer_cast<Subscription, OutrightSubscription>( m_input.subTOM ) );
		}

		// Create leg subscriptions for crossing models

		// m_needDollarRates is set in SubscriptionFactory::createInstance by republicationIntervalActive [
			bool republicationIntervalActive = RegistrySettings::isRepublicationIntervalActive();
			if( republicationIntervalActive )
				// if the end user has no time specified then turn off republish on this 
				// subscription
				if (userRequest.getRepublicationInterval() == 0 )
					republicationIntervalActive = false;
				else if (userRequest.getRepublicationInterval() < RegistrySettings::getMinRepublicationInterval())
					// to low - reset the interval to the minimum value
					userRequest.setRepublicationInterval(RegistrySettings::getMinRepublicationInterval());
			new RVOutrightSubscription(... republicationIntervalActive...);
		]
		// what does it mean ?
		
		switch( m_input.model )
		case DIRECT_SPOT_NO_FWD:
			if( m_needDollarRates ) // ??? 
				createDollarSubscriptions(profileId, false); 
			break;
		case DIRECT_SPOT_DIRECT_FWD:
			break;
		case CROSSED_SPOT_NO_FWD:
			if( m_needDollarRates && *m_input.spot.crossingCcy != USD )
				createDollarSubscriptions(profileId);
			// We do not create subscriptions on POINTS subscription against USD.
			if( m_needLegSubscriptions )
				std::string spotDate = m_input.spot.valueDate->toUTCString();
				// we need to create these subscriptions even if we're also crossing fwds, since the spot rates
				// that come with the fwds won't necessarily line up in valuedate with the cross spot date			
				createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
									  m_input.spotQty.requestVolume, m_input.baseViaSpotCcy.subNearOutright);
				createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
									  m_input.spotQty.requestVolume, m_input.termsViaSpotCcy.subNearOutright);
			break;
		case CROSSED_SPOT_DIRECT_FWD:
			// Direct forward model implies we have a dollar deal pair: No dollar legs required.

			// We do not create subscriptions on POINTS subscription against USD.
			if( m_needLegSubscriptions )
				std::string spotDate = m_input.spot.valueDate->toUTCString();
				// we need to create these subscriptions even if we're also crossing fwds, since the spot rates
				// that come with the fwds won't necessarily line up in valuedate with the cross spot date			
				createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
									  m_input.spotQty.requestVolume, m_input.baseViaSpotCcy.subNearOutright);
				createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
									  m_input.spotQty.requestVolume, m_input.termsViaSpotCcy.subNearOutright);
			break;
		case DIRECT_SPOT_CROSSED_FWD:
			// Our deal pair must be crossed through dollar, use the incumbent dollar forward legs
			std::string baseCcyPair;
			if( 0 == FXCurrencyUtils::toCurrencyPair( m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, baseCcyPair ) )
				throw ErrorData(...);
			Nullable<std::string> baseCrossCcy;
			if( !dp.getSpotCrossCurrency(baseCcyPair, baseCrossCcy) )
				throw ErrorData(...);
			if( !baseCrossCcy.isNull() )
				throw ErrorData(...);

			// Dollar rates are extracted during recalculation of this model
			std::string spotDate = m_input.spot.valueDate->toUTCString();
			std::string fwdDate = m_input.fwd.valueDate->toUTCString();
			
			// Create the appropriate subscriptions for the spot & outright base leg, 
			// noting that the spot one is using the fwd crossing currency
			createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, spotDate, FXOutrightRequest::FULL, 
								  m_input.spotQty.requestVolume, m_input.baseViaFwdCcy.subNearOutright);
			createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, fwdDate, FXOutrightRequest::FULL, 
								  m_input.fwdQty.requestVolume, m_input.baseViaFwdCcy.subFarOutright);

			// Create the subscriptions for the terms leg fwd points, with the assumption here being that since we're 
			// only crossing fwd through USD, by definition we'll be able to get DIRECT fwd POINTS for the legs.
			// We explicitly request for POINTS only here to avoid potential recursive cycle if spot happens to be crossed in the terms leg
			createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, spotDate, FXOutrightRequest::POINTS, 
								  m_input.spotQty.requestVolume, m_input.termsViaFwdCcy.subNearPoints);
			createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, fwdDate, FXOutrightRequest::POINTS, 
								  m_input.fwdQty.requestVolume, m_input.termsViaFwdCcy.subFarPoints);

	
			// If this is an TDY, then we'll also need the terms leg TOM points to correctly perform the fwd crossing.
			// Note that the assumption here is that we can never have mis-aligned TDY. ie, m_input.fwd.valueDate is always
			// going to correspond to TDY for the terms leg
			if( m_input.isTDY() )
				createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, Tenor::TOM, FXOutrightRequest::POINTS, 
								      m_input.fwdQty.requestVolume, m_input.termsViaFwdCcy.subTOMPoints, false);
			break;
		case CROSSED_SPOT_CROSSED_FWD:
			// No need to create the dollar legs coz the legs are available as the crossed fwd 
			// is always through the dollar
			
			std::string spotDate = m_input.spot.valueDate->toUTCString();
			std::string fwdDate = m_input.fwd.valueDate->toUTCString();

			createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, spotDate, FXOutrightRequest::FULL, 
								  m_input.spotQty.requestVolume,
								  m_input.baseViaFwdCcy.subNearOutright);

			createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, spotDate, FXOutrightRequest::FULL, 
								  m_input.spotQty.requestVolume,
							      m_input.termsViaFwdCcy.subNearOutright);

			createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, fwdDate, FXOutrightRequest::FULL, 
								  m_input.fwdQty.requestVolume,
								  m_input.baseViaFwdCcy.subFarOutright);

			createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, fwdDate, FXOutrightRequest::FULL, 
								  m_input.fwdQty.requestVolume,
							      m_input.termsViaFwdCcy.subFarOutright);

			// If we're going through different crossing currencies for spot and fwd, then
			// we'll also need some subscriptions for the base and terms through the spot
			// crossing currency for the spot valuedate
			if( !m_input.sameCrossingCcy )
				createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
								      m_input.spotQty.requestVolume,
									  m_input.baseViaSpotCcy.subNearOutright);
	
				createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
								      m_input.spotQty.requestVolume,
									  m_input.termsViaSpotCcy.subNearOutright);

			break;
		default:
			throw RFQErrorData( FXOutrightQuote::E_SYSTEM, "Unrecognized pricing model" );


To summarise tick subscriptions, for spot, there are two cases:

 - DIRECT_SPOT
   1. sp.registerListener(m_input.ccyPair, ..., m_input.spot.tick)
 - CROSS_SPOT
   1. createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
   						m_input.spotQty.requestVolume, m_input.baseViaSpotCcy.subNearOutright);
   2. createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
						m_input.spotQty.requestVolume, m_input.termsViaSpotCcy.subNearOutright);

For fwd (everything non-spot is fwd, including TDY and TOM), there are four:

 - DIRECT_SPOT | DIRECT_FWD
   1. this: spot (m_input.ccyPair, m_input.spot.tick)
   2. this: fwd  (m_input.ccyPair, *m_input.fwd.valueDate, m_input.fwd.tick)

 - DIRECT_SPOT | CROSS_FWD
   1. this: spot (m_input.ccyPair, m_input.spot.tick)
   2. createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, spotDate, FXOutrightRequest::FULL, 
   					  m_input.spotQty.requestVolume, m_input.baseViaFwdCcy.subNearOutright);
   3. createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, fwdDate, FXOutrightRequest::FULL, 
   					  m_input.fwdQty.requestVolume, m_input.baseViaFwdCcy.subFarOutright);
   4. createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, spotDate, FXOutrightRequest::POINTS, 
   					  m_input.spotQty.requestVolume, m_input.termsViaFwdCcy.subNearPoints);
   5. createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, fwdDate, FXOutrightRequest::POINTS, 
   					  m_input.fwdQty.requestVolume, m_input.termsViaFwdCcy.subFarPoints);
  
 - CROSS_SPOT | DIRECT_FWD
   1. fp.registerListener(m_input.ccyPair, *m_input.fwd.valueDate, ..., m_input.fwd.tick)
   2. createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
   						m_input.spotQty.requestVolume, m_input.baseViaSpotCcy.subNearOutright);
   3. createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
						m_input.spotQty.requestVolume, m_input.termsViaSpotCcy.subNearOutright);

 - CROSS_SPOT | CROSS_FWD (1, 2, only if spot.crossingCcy != fwd.crossingCcy)
   1. createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
   						m_input.spotQty.requestVolume, m_input.baseViaSpotCcy.subNearOutright);
   2. createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.spot.crossingCcy, spotDate, FXOutrightRequest::FULL, 
						m_input.spotQty.requestVolume, m_input.termsViaSpotCcy.subNearOutright);
   3. createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, spotDate, FXOutrightRequest::FULL, 
   					    m_input.spotQty.requestVolume, m_input.baseViaFwdCcy.subNearOutright);
   4. createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, spotDate, FXOutrightRequest::FULL, 
   					    m_input.spotQty.requestVolume, m_input.termsViaFwdCcy.subNearOutright);
   5. createLegSubscription(m_input.ccyPair.baseCcy(), *m_input.fwd.crossingCcy, fwdDate, FXOutrightRequest::FULL, 
   					    m_input.fwdQty.requestVolume, m_input.baseViaFwdCcy.subFarOutright);
   6. createLegSubscription(m_input.ccyPair.termsCcy(), *m_input.fwd.crossingCcy, fwdDate, FXOutrightRequest::FULL, 
   					    m_input.fwdQty.requestVolume, m_input.termsViaFwdCcy.subFarOutright);

And for TDY,  the above would be doubled as TOM is also subscribed.

Data subscription logic:

	struct CcyPairData
		CurrencyPair ccyPair;
		TPSIndicativeData::Ptr indicative;
		TPSFastMarketData::Ptr fastMarket;
	    TPSAXEFastMarketData::Ptr axeFastMarket;
		bool subscribed;

	struct CcyPairDataLegs
		CcyPairData base;
		CcyPairData terms;

	void RVOutrightSubscription::createDataSubscription(legCcy, crossCcy, CcyPairData & data)

		std::string ccyPair;
		FXCurrencyUtils::toCurrencyPair( crossCcy, legCcy, ccyPair);
		CurrencyPair::Ptr ccyPairPtr;
		FXCacheUtils::getCurrencyPair(ccyPair, ccyPairPtr);

		// In order to set the volume correctly, we need to convert from the
		// original volume in the request to the denominated volume ccy
		ml::common::math::Decimal denomCcyMultiplier(0);
		m_tps.spotProvider(RV_SPOT_SERVER_PROFILE)->getDenominationMultiplier(
			m_input.volumeCcy, ccyPairPtr->getOutrightDenomCcy(), denomCcyMultiplier);
		denomCcyMultiplier *= m_input.spotQty.requestVolume;
		data.ccyPair = ccyPair;
		DataProvider& dp = m_tps.dataProvider();
		dp.registerListener(data.ccyPair, *m_input.spot.valueDate, m_input.spot.tenor,
		      denomCcyMultiplier, m_eventForwarder, data.indicative, &data.fastMarket, 0);
		data.subscribed = true;


Incoming events (calculating):

	// with the following id bases, event type can be derived from event id
	static const int SPOT_EVENT_ID_BASE = 100;
    static const int FWD_EVENT_ID_BASE = 200;
 	static const int DATA_EVENT_ID_BASE = 300;
 	static const int CHILD_EVENT_ID_BASE = 400;
	static const int SKEW_EVENT_ID_BASE	= 500;

	OutrightSubscription::onEvent(tps::Event::Ptr& event)
		Subscription::onEvent(event) [
			m_events.insert(event) ...
			if (m_updateList) m_eventList.push_back(event)
		]
		m_tps.quoteCalculator().enqueueForCalculation(this) [
			add this to work queue 
			QuoteCalculator::onProcess 
				get item from work queue and call its calculate method
		]

	OutrightSubscription::calculate()
		consistentCalculate()

	RVOutrightSubscription::consistentCalculate()
		Event::PtrList events

		// RVOutrightSubscription set m_updateList to true in constructor,
		// so events are in m_eventList. event processing is done by calling
		// process on each event, which copies the update data to their target,
		// and rerun the calculation process to update the quote

		m_eventList.swap(events)
		foreach (event : events)
			event->process();

		// check consistency
		if (!areSpotRatesConsistent(*this))
			return;

		// init m_output
		// checking
		// add reasoning

		switch( m_input.model )
		case DIRECT_SPOT_NO_FWD:
		case DIRECT_SPOT_DIRECT_FWD:
			calculateDirectSpotDirectFwd();
			break;
		case DIRECT_SPOT_CROSSED_FWD:
			if (!calculateDirectSpotCrossedFwd())
				setAbandoned(true);
				return;
			setAbandoned(false);
			break;
		case CROSSED_SPOT_NO_FWD:
		case CROSSED_SPOT_DIRECT_FWD:
			calculateCrossedSpotDirectFwd();
			break;
		case CROSSED_SPOT_CROSSED_FWD:
			if (!calculateCrossedSpotCrossedFwd())
				setAbandoned(true);
				return;
			setAbandoned(false);
			break;
		
		// TODO: a lot of stuff going on here

		if (!m_isInternalSubscription)
			...
			m_tps.quotePublisher().enqueueForPublishing(this)
		else
			// TODO: how this came about and how it works ?
			// TODO: 
			// internal subscription, publish to parent
			parent.onEvent(new ChildEvent(...))		

Publishing:

	void QuotePublisher::enqueueForPublishing(const Subscription::Ptr & ptr)
		m_queue.put(ptr);

	bool QuotePublisher::onProcess()
		if( !m_queue.get(item) )
			return Thread::STOP;
		ptr->publish();	
		return Thread::CONTINUE;

	OutrightSubscription::publish()
		if (m_quote.getQuoteId() == m_lastPublishedQuoteId && m_request.getRepublicationInterval() == 0)
			return;
		...
		m_tps.quotePublisher().publish(topic(), subscriberId(), m_request, quote);
		storeQuote();
		const Time now;
		setLastPublishedTime(now);


	void OutrightSubscription::storeQuote()
		if (!m_tps.ENABLE_AUDITING)
			return;
			
	    Time now;
		// Decay previous quote in 3 seconds
		m_auditQuote.reschedule( now + Time(3, 0) );

		// m_audit is a shallow copy of the subscription object and is
		// set in RVOutrightSubscription::consistentCalculate as:
		//
		// 		m_audit.reset(new RVOutrightSubscription(*this));
		// 		m_audit->clone(); // clone reasoning

		if (m_audit)
			// what we put in cache is a function to call generateAuditTrail on m_audit
			// to create the required audit on-demand, neat
			m_auditQuote = m_tps.quoteCache().put( m_request.getRequestId().getUser(), 
				m_request.getRequestId().getRequest(), m_quote.getQuoteId(), 
				boost::bind( &generateAuditTrail,  m_audit));
			m_audit.reset();



<!-- ################################################################################# -->

# Quote Publisher


The class

	void publish(Router::Topic & topic, string & subscriberId,
				 FXOutrightRequest & request, FXOutrightQuote & quote);
	void publish(Router::Topic & topic, const std::string & subscriberId,
				 FXSwapRequest & request, FXSwapQuote & quote);
	void publishStats();

	list<Task::Ptr > m_threads;
	Dispatcher m_quoteDispatcher;


There are four related contracts for publishing data:

	FXOutrightProvider
	FXSwapProvider
	FXTspAuditProvider
	FXTradeBroadcastContract

Contract settings in `ContractTopic`:

	id    contractName       routingKey routerType topic          
	----- ------------------ ---------- ---------- -------------- 
	1000  FXOutrightProvider            4          :localhost:0:  
	15812 FXOutrightProvider lb-1       4          :localhost:10: 
	15813 FXOutrightProvider lb-2       4          :localhost:11: 
	15814 FXOutrightProvider lb-3       4          :localhost:12: 
	8007  FXSwapProvider                4          :localhost:4:  
	15815 FXSwapProvider     lb-1       4          :localhost:15: 
	15816 FXSwapProvider     lb-2       4          :localhost:16: 
	15817 FXSwapProvider     lb-3       4          :localhost:17: 
	7004  FXTpsAuditProvider            4          :localhost:3:  
	15821 FXTpsAuditProvider lb-1       4          :localhost:25: 
	15822 FXTpsAuditProvider lb-2       4          :localhost:26: 
	15823 FXTpsAuditProvider lb-3       4          :localhost:27: 


`Subscription` publishes through publisher, for example:

	void OutrightSubscription::publish()
		m_tps.quotePublisher().publish( topic(), subscriberId(), m_request, quote );

Publishing is straightforward:

	void QuotePublisher::publish(Router::Topic & topic, subscriberId, request, quote)
		FXOutrightListenerProxy proxy(m_quoteDispatcher, subscriberId, topic); or
		FXSwapListenerProxy proxy(m_quoteDispatcher, subscriberId, topic);
		proxy.onQuote(request.getRequestId(), quote);




<!-- ################################################################################# -->

# Clients of Upstream Servers 

## Spot Client 

Settings:

	// Global spot server configuration.
	// There should only be one valid row in this table
	class FXSpotGlobalConfig version 1.0
		// Heartbead period in milliseconds
		heartbeatPeriod int32;	// used to set heartbeat timeout for UnifiedSpotClientInterface
		
		// Pricing sticky factor (range between 0.00 and 1.00) 
		stickyFactor decimal(1,2);	// doesn't seem to be used by panther
									// see it only in fx_unified_spot_audit_server for logging
		
		// Default precautionary volatility factor
		defaultVolFactor decimal(2,2); // couldn't find reference in panther


### Utils related to spot subscription

	const std::string NORMAL_MODE_IDENTIFIER("normal");
	const std::string PROFILE_MODE_IDENTIFIER("profile");

	// Return the routingKey for the spot server, depending on mode.
	inline std::string getRoutingKey(bool isProfileMode)
		return isProfileMode ? PROFILE_MODE_IDENTIFIER : NORMAL_MODE_IDENTIFIER;


	// Build a gateway subscription key.
	inline std::string getSubscriptionKey(std::string ccyPair,
										  std::string profileId,
										  bool isProfileMode )
		if isProfileMode
			return ccyPair + "." + profileId + "." + getRoutingKey(true);
		else
			return ccyPair;

	const std::string TICK_PROTOCOL_CONTRACT     = "FXSpotTickWithDepth";

	// Get the name of the contract on which normal or raw spot ticks should be sent.
	inline std::string getContractName()
		return TICK_PROTOCOL_CONTRACT;

### TicksListener

`TicksListener` provides the basic logic to sub/unsub to spot events:

	class TicksListener : public RouterCallback

	UnifiedSpotClientInterface & m_callback;	// callback to upper layer 
	const bool m_isRepublishThrottleEnabled;

Subscription is done through static methods:

	static void registerListener( const TicksListener::Ptr& listener, const std::string& profileName );
	static void unRegisterListener( const TicksListener::Ptr& listener );

	void TicksListener::registerListener(  const TicksListener::Ptr& listener, const std::string& profileName)
		// topic is tick_data^FXSpotTickWithDepth_<profileName>
		const std::string tickProtocolTypeTopic =
			( ContractTopics::instance().getTypeAndTopic(getContractName())+ "_" + profileName );
		shared_ptr<RouterCallback> callback( dynamic_pointer_cast< RouterCallback, TicksListener>(listener) );
		Routers::instance().addSubscriber( callback, tickProtocolTypeTopic, 1);

	void
	TicksListener::onMessage( const char* topic,  const char* replyTopic,
							  const char* payload, unsigned int sz )
		FXSpotTickWithDepth tick;
		FXSpotTickDepthMarshaller::readTick(tick, payload, sz);

		if(tick.getPublishState() == FXSpotTickWithDepth::PUBLISH_PREV_THROTTLED)
			// configured by /unifiedspotclient_lib[republishThrottled]
			if(m_isRepublishThrottleEnabled)
				m_callback.onSpotTick(tick, payload, sz);
		else
			m_callback.onSpotTick(tick, payload, sz);


### UnifiedSpotClientInterface

This is the base of spot client:

- it uses `TicksListener` to get spot events
- it adds snapshot capabilities

	class UnifiedSpotClientInterface : public FXSpotTickWithDepthListenerStub ...
	// snapshot
	virtual ... getSnapshots(const std::deque< String >& ccyPairs);
	virtual ... getAllSnapshots();
	// ticks
    virtual void onSpotTickWithDepth( FXSpotTickWithDepth& tick );

    // callback for TicksListener
    virtual  void onSpotTick( const FXSpotTickWithDepth& tick,
    						  const char* data, unsigned int sz) = 0;

	std::string 		m_profileName;		// profile name
	TicksListener::Ptr 	m_listener;			// for ticks, with self as callback
	DispatcherAutoSubscriber  m_dispatcher;	// for snapshot, inited as

		m_dispatcher(ContractTopics::instance().makeTopicString(
			RouterFactory::TIBRV_PUBSUB, "UnifiedSpotClient.spotsnapdepth.result"))


	boost::shared_ptr< FXSpotTickWithDepth::PtrList >
	UnifiedSpotClientInterface::getSnapshots(const std::deque< String >& ccyPairs)
		FXSpotTickWithDepthProviderProxy proxy( *m_dispatcher.getDispatcher(), m_profileName );
		return proxy.getSnapshots( ccyPairs );


### UnifiedSpotClient

The final client is a simple extension with heartbeat and status notification:

	class UnifiedSpotClient : public UnifiedSpotClientInterface
		UnifiedSpotClient(UnifiedSpotClientCallback & callback, const std::string& profileName);
		UnifiedSpotClientCallback & 		m_callback;


	void UnifiedSpotClient::onSpotTick(const FXSpotTickWithDepth& tick, const char* data, unsigned int sz)
		cancelHeartbeatTimer();
		if( m_connected.compare_exchange(0, 1) == 0 )
			m_callback.onSpotConnect();
		m_callback.onSpotTick(tick);
		restartHeartbeatTimer();

It provides service through callbacks:

	class UnifiedSpotClientCallback
		virtual void onSpotConnect() = 0;
		virtual void onSpotTick(const ml::spec::fx::spot::FXSpotTickWithDepth& tick) = 0;
		virtual void onSpotDisconnect() = 0;

TPS `SpotProvider` uses `UnifiedSpotClient` to interface with upstream spot servers.


## Forward Client 

See fx_fwd_server.md for details.

TPS `FwdProvider` uses `ForwardClient2` to interface with the forward server 



## Skew Client 


The use of `SkewClient` to interface with upstream skew provider (api_gateway ?) is
quite involved here:


<!-- ################################################################################# -->

# Subscription

Base class:

	class Subscription : public tps::EventListener

	const std::string	m_subscriberId;
	const FXRequestId	m_requestId;
	const Router::Topic	m_topic;
  
	tps::Event::PtrSet    m_events; 	// event set
	bool m_updateList; // controls if events should also be added to m_eventList, false by default
	tps::Event::PtrList   m_eventList;	// event list

	mlc::util::Time m_lastPublishedTime;
	bool m_isAuditable;

	enum InternalState {
		NONE,
		PENDING_DATA,
		INVALID,
		PENDING_RATES,
		RFQ_NORATES,
		RFQ_WITHRATES,
		AUTOQUOTE_TRADEABLE,
		AUTOQUOTE_INDICATIVE
	};
	InternalState m_state;

	enum PublicationStates
	{
		NotActive       =0,
		NotPublished    =1,
		PublishHeld     =2,
		PublishHoldExpired = 3,
		PublishDone     =4
	};
	atomic32	m_publicationState;

	virtual void reload()		= 0;
	virtual void calculate()	= 0;
	virtual void publish()		= 0;
	virtual void unsubscribe()	= 0;

	void Subscription::onEvent(const tps::Event::Ptr & event)
		std::pair<tps::Event::PtrSet::iterator, bool>  eventPair =  m_events.insert(event) ;
    	if( eventPair.second == false ) 
    		const_cast<tps::Event::Ptr &>(*eventPair.first) = event ;
    	if( m_updateList) 
    		m_eventList.push_back(event) ;

	struct EventCompare
		// compare by id, for children events, compare by name
		bool operator()(const Event::Ptr& event1, const Event::Ptr& event2) const
			if (event1->id() != event2->id())
				return event1->id() < event2->id();
		  	else
				if (event1->id() == CHILD_EVENT_ID_BASE)
			  		return event1->name() < event2->name();
				else
			  		return false;
	typedef std::set<Event::Ptr, EventCompare>	PtrSet;

Outright subscription:

	class OutrightSubscription : public Subscription

	const FXOutrightRequest m_request;
	// used for reciprocal only to send out the right request details if required
	const FXOutrightRequest m_originalRequest;

	FXOutrightQuote m_quote;
	Subscription::WPtr m_parent;
	int32 m_depth;

	OutrightInput  m_input;	// request, directly or indirectly from the original request
	OutrightOutput m_output;

	shared_ptr<OutrightSubscription> m_audit;	// a shallow copy of the subscription used for audit
 	QuoteCache::Item m_auditQuote;				// key into to quote cache

	int64 m_lastPublishedQuoteId;
	bool m_hasBeenPriced;
	bool m_isInternalSubscription;
	bool m_registeredSubscription;	// subscribed to data provider ?
	...

<!-- ################################################################################# -->

# Quote Cache

Audit quote cache. 

	class QuoteCache

		struct Key
			const int64 userId;
			const int64 requestId;
			const int64 quoteId;

		StalingMap< Key, AuditGeneratorFunc > m_cache;
		Time m_quoteTimeToLive;

		shared_ptr< FXTpsAuditProviderImpl > m_auditProvider;
		shared_ptr< FXTradeBroadcastContractImpl > m_tradeListener;

This is used to add quote to the cache:

	Item put( int64 userId, int64 requestId, int64 quoteId, const AuditGeneratorFunc & func);


It uses `StalingMap`:

	typedef boost::shared_ptr<Resource> Resource_ptr;
	typedef std::map< Key, Resource_ptr, Compare > Collection;
	typedef std::multimap< int64, typename Collection::iterator > PriorityQueue;

	template <typename Key, typename Value, typename Compare = std::less<Key> >
	class StalingMap : public mlc::util::HeartbeatTimer
		Collection 		m_items;		// regular map
		PriorityQueue 	m_priority;		// priority queue on timeout for staling

When an item is added or scheduled, a timeout has to be specified. The item is added
to m_items and the priority queue based on timeout order.

It periodically removes expired entries:

	void StalingMap<K, V, C>::onHeartbeatTimeout() 
		typename PriorityQueue::const_iterator end = m_priority.lower_bound( mlc::util::Time().usecs() );
		typename PriorityQueue::iterator begin = m_priority.begin();
		while( begin != end )
			m_items.erase(begin->second);
			m_priority.erase( begin++ );
		restartHeartbeatTimer();

Checking interval is set to 10ms:

	template <typename K, typename V, typename C>
	StalingMap<K, V, C>::StalingMap()
		int32 usecs = 100000;
		setHeartbeatPeriod( mlc::util::Time(0, usecs) );
		restartHeartbeatTimer();	


<!-- ################################################################################# -->

# The Bubble Net

	// the parent/child relationship is maintained in cause nodes and effect nodes
	// cause nodes are essentially input nodes of compute nodes
	// effect nodes are required to push change top-down, 
	// the set of cause nodes and effect nodes of a node is updated whenever a node
	// is involed in BubbleNet::addComputeNode*
	//
	// so each time a source node is updated, a top-down tree traversal based on 
	// effect nodes is performed, see BubbleNet::flushTicks

	class UntypedNode {
		std::string m_name;
		mutable int m_rank;
		std::set<ComputeNode*> m_effects;

		virtual const std::vector<const UntypedNode*> getCauseNodes() const = 0;
		const std::set<ComputeNode*> getEffectNodes() const { return m_effects; }
	}

	// A node has content
	template <typename T>
	class Node : public UntypedNode {
		boost::optional<T> m_content;

		void doSet(const T& t) { m_content = t; }

		// used for argument validation
		bool testGet() const {
			return m_content;
		}
		const T& doGet() const { 
			if (m_content)
				return *m_content; 
			throw UninitializedNodeException();
		}
	}

	// A source node only has content, no internal logic.
	template <typename T>
	class SourceNode : public Node<T> {
		virtual const std::vector<const UntypedNode*> getCauseNodes() const {
			// always return an empty vec
		}
	}


	class ComputeNode : public Node<InternalTPSTick::CPtr> {
		std::vector<const UntypedNode*> m_causes;
		void addCause(const UntypedNode* cause) { m_causes.push_back(cause); }
		virtual const std::vector<const UntypedNode*> getCuaseNodes() const { return m_causes; }

		virtual bool doRecompute() = 0;
		bool tick(bool _truncateReasoning) {
			this->truncateReasoning = _truncateReasoning;
			return doRecompute();
		}
	}

	int_tps_tick

	int_tps_tick    InternalTPSTick, dummyTick


<!-- ################################################################################# -->

# Misc

## Heartbeat




<!-- ################################################################################# -->

# tps_gen

<!-- ################################################################################# -->

# AXE Outright Subscription Graph for direct


	                                                   InternalTPSTick
	                                                          |
	                                                          |
	                                                          |
	                      TPSGroupData                initializeTick                     TPSSpotSalesMarkupData
	                            |                             |                                 |   |
	                            |_____________________________|_________________________________|   |
	                                                          |                                     |
	 TPSAXEFastMarketData  TPSSpotTraderMarkupData  verifySalesSpreadData                           |
	          |                       |                       |                                     |
	          |_______________________|_______________________|                                     |
	                                  |                                                             |
	                        verifyAXEFastMarketData          TPSTradingLimitData                    |
	                                  |                            | |                              |
	                                  |____________________________| |                              |
	                                                 |               |                              |
	              TPSGroupTradeVolumeData     verifySpotLimit        |                              |
	                         |                       |               |                              |
	                         |_______________________|_______________|                              |
	                                                 |                                              |
	                                       applyGroupTradeVolume      SpotTickWithDepth             |
	                                                 |                       |                      |
	                                                 |_______________________|                      |
	                                                            |                                   |
	                                                   applyAXEInterpolation                        |
	                                                            |                                   |
	                                                            |                                   |
	                                                            |                                   |
	                                                    applyAXEFastMarket                          |
	                                                            |                                   |
	                                                            |                                   |
	                                                            |                                   |
	                                                   applySpotMidConstraint                       |
	                                                            |                                   |
	                                                            |___________________________________|
	                                                                         |
	                                                                applyAXESalesSpread
	                                                                		 |
	                                                                		 |
	                                                                		 |		 
	                                                                verifyInvertedPrice      SpotIndicative
	                                                                         |                      |
	                                                                         |______________________|
	                                                                                    |
	                                                                            verifyPostCalculate





<!-- ################################################################################# -->

# Refactoring

## ExecutionEngine 

- manages all subscription data processing
- has a list of ExecutionWorkers
- allocate subscription id from new subscriptions
- assign new top-level subscription to worker based on load
- assign child-subscription to the same worker as parent subscription


## ExecutionWorker

- maintains an array of subscriptions, Subscription::m_index reflects its position in this array.
  NullSubscription is used to avoid pointer check
- uses ConcurrentQueue to hold events, add, reload, unsubscribe, recalc are handled as normal events


## Event routing and handling

- each worker has a ExecutionWorkerDataDispatcher, which interfaces with DataProvider, SpotProvider etc as usual
- all subscriptions inside a worker subscribe to the worker's own dispatcher
- a subscription maintains an array of event handlers, when making a subscription, the index of
  the event handler is passed to ExecutionWorkerDataDispatcher
- ExecutionWorkerDataDispatcher records each subscriber's subscription index (it's position inside worker's array)
  and the handler index. when a event arrives, it's sent as <subIndex, hdlrIndex, event>, this makes event
  data small and its dispatching fast

- for spot and forward ticks, subscription management is internalized. there's only a single spot tick subscription
  per worker per currency pair. when a new ticks arrives, only one copy of tick data is passed from spot provider
  into the internal processing loop of worker. the worker maintains an array of <subIndex, hdrlIndex> for the
  currency pair and feed them with the same tick. fast and efficient

## Special notes:

- creation of a normal child subscription inside reload is done synchronously and the child subs is 
  added to the worker synchronously. this avoids the race condition that a subscription is removed its
  children finishes reloading. this applies to real subscription of a shared leg subscription as well
- a subscription should keep track of its subscriptions and is able to unsubscribe all of them
- a subscription should keep track of its child subscriptions
- when reloading an existing subscription,  should first do unsubscribe for itself and all of its non-shared
  children, and remove all of non-shared children
- when unsubscribing, need to unsubscribe for all children subscription and remove them as well


## Leg subscription management and event dispatching

- each subscription has m_hasSharedLegParent, this is set on real subscription of a leg subs and all of its children
- real subscriptions of leg subs are assigned to workers like usual
- for each subscription, if it has added itself as dependent, need to reverse it when reloading or unsubscribing
- a leg subscription should submit a unsub request for its real sub when deleted


In subscription ctr/dtr add tracing logic to track subscription management to avoid memory leak

Note: checkDependant is called on every publish, slow if a leg has many dependants, consider removing it


- A shared leg is kept as a shared pointer in various dependants. when it ticks, it publishes
  a child event to its dependants which uses tryGetSubscriptionOutput to get the data. This is clearly
  very racy:
  
  - the update of leg and its publishing of events is asyn to its dependants 
  - the handling of these event in dependants are non-deterministic, multiple events may be handled in one
    or more concistentCalculate, the point of tryGetSubscriptionOut is not in sync with the original child event
  - the call of clone in dependants again is async to the leg

- A tick from a shared leg may lead to all of its dependant ticking and cloning. This may lead to
  multiple cloning of the shared leg.

Both issues can be addressed by publishing the audit (clone, which is created anyway before publishing)
of the leg as part of child event. The dependants would then use this for their cloning as well. 

- The hanlding of children events contains many if conditions to many event names against children subscriptions,
this can be optimized.
- There's the danger of of a client exits without properly unsubscribing its subscriptions. This can be
  easily handled for TCP clients. For TIBRV clients, a status checking mechanism is required.



# Performance Improvment

Use a single consolidateQuote(ctx), for MQ, check if quotestack is empty, if yes,
update all, otherwise, update a single volume

## Slow Logging


	// AXEBaseSubscription.cpp 

	bool AXEBaseSubscription::processBubbleNetResult(OutrightSubscriptionContext& ctx, ...)

	const InternalTPSTick& bnOutput = *lastTickedComputeNode->doGet(*m_bnContexts[ctxId]);
	GLDBUG(GRP_MAIN, "output trader %s sales ...")


	// OutrightSubscription_StateTools.cpp

	void OutrightSubscription::setState(OutrightSubscriptionContext& ctx, const InternalState newState)

	GLDBUG(GRP_MAIN, "Outright state update to %s from %s ...")


## BubbleNet Improvement

When flushTicks for FXSpotTickWithDepth, the following is slow:
- applyAXEInterpolation_computeNode
- applyAXEFastMarket_computeNode
- applyAXESalesSpread_computeNode

	Total 58:
	 FXSpotTickWithDepth(0) 
	 applyAXEInterpolation_computeNode(13) 
	 applyAXEFastMarket_computeNode(17) 
	 applySpotMidConstraint_computeNode(4) 
	 applyAXESalesSpread_computeNode(17) 
	 verifyInvertedPrice_computeNode(2) 
	 verifyPostCalculate_computeNode(1)

	Total 56:
	 FXSpotTickWithDepth(0) 
	 applyAXEInterpolation_computeNode(14) 
	 applyAXEFastMarket_computeNode(16) 
	 applySpotMidConstraint_computeNode(5) 
	 applyAXESalesSpread_computeNode(16) 
	 verifyInvertedPrice_computeNode(2) 
	 verifyPostCalculate_computeNode(1)

	Total 60:
	 FXSpotTickWithDepth(0) 
	 applyAXEInterpolation_computeNode(14) 
	 applyAXEFastMarket_computeNode(18) 
	 applySpotMidConstraint_computeNode(6) 
	 applyAXESalesSpread_computeNode(17) 
	 verifyInvertedPrice_computeNode(2) 
	 verifyPostCalculate_computeNode(1)

	Total 81:
	 FXSpotTickWithDepth(0) 
	 applyAXEInterpolation_computeNode(26) 
	 applyAXEFastMarket_computeNode(19) 
	 applySpotMidConstraint_computeNode(6) 
	 applyAXESalesSpread_computeNode(22) 
	 verifyInvertedPrice_computeNode(2) 
	 verifyPostCalculate_computeNode(2)

How to get rid of the extensive copying of InternalTPSTick in bubblenet nodes

	const InternalTPSTick::CPtr ApplyAXEInterpolation::update(ApplyAXEInterpolationContext& ctx,
	                                                          const InternalTPSTick::CPtr & tick, 
	                                                          const FXSpotTickWithDepth::Ptr& spotTick) const {
		on_start2(ApplyAXEInterpolation);
		ctx.tickStorage = *tick;	// copy
		InternalTPSTick::Ptr retTick = &*ctx.tickStorage;

	
	// one option is this:
	1. Add version number to InternalTPSTick
	2. In node context, record version of the dependent tick
	3. for the above code, do this:

	const InternalTPSTick::CPtr ApplyAXEInterpolation::update(ApplyAXEInterpolationContext& ctx,
	                                                          const InternalTPSTick::CPtr & tick, 
	                                                          const FXSpotTickWithDepth::Ptr& spotTick) const {
		// save current local tick version
		uint32 localTickVersion = ctx.tickStorage->version;
		// check if dependant tick has changed
		if (ctx.dependentTickVersion != tick->version) {
			ctx.tickStorage = *tick;
			ctx.dependentTickVesion = tick->version;
		}
		// always update local tick version
		ctx.tickStorage.version = localTickVersion + 1;

		InternalTPSTick::Ptr retTick = &*ctx.tickStorage;
		....
	}


For ComputeNode<InternalTPSTick>, get rid of boost::optional: check performance between optional and raw pointer.

Improve the performance of validateArgs:
- combine testGet and doGet
- for nodes with a single parent, any need to do it at all ?
- for nodes with multiple parents, minimize the impact of validate:
  - if only one parent is updated, there's no need to do it
  - how to check if any parent has been updated since last call ?

## Audit Improvement

For audit cloning,

	resetAudit();	// this is even slower than clone
	m_audit();		// also quite slow


Consider replacing Subscription with SubscriptionAudit:
- it is pool managed
- it should only copy data required for audit
- it uses data versioning on Subscription and previous SubscriptionAudit to save memory


## SpotTick Improvement

Use pool for SpotTick events

## Fundations

### Object pool
- check TCMalloc doc
- check folly IndexedMemPool, optimized it for tcmalloc
- bench

### Logging

See Aereon logbuffer

### String builder

The default string builder expands memory on demand, wasteful for audit. can use small buffers
to hold data.

Check Decimal.toString() performance.

addReasoning(...) with Decimal.toString() seems to be a major bottleneck for bubblenet



Note that each InternalTPSTick takes 22k:

	sizeof(OutrightInput)=2456
	sizeof(OutrightOutput)=6984
	sizeof(InternalTPSTick)=22820
	sizeof(AXEOutrightSubscription)=4880
	sizeof(RVOutrightSubscription)=3940

And there are 30 nodes with local copy, so bubblenet consumes a lot of memory:

	% ag 'optional<InternalTPSTick>' axe/graph/ | grep tickStorage
	
	/axe/graph/ApplyAXESalesSpread.hpp:29:      boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/ApplyAXEInterpolation.hpp:28:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/ApplyDollarRate.hpp:17:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/ApplyFwdFastMarket.hpp:39:       boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/ApplyFwdSalesSpread.hpp:48:      mutable boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/ApplyFwdSpreadMultiplier.hpp:31:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/ApplyGroupTradeVolume.hpp:29:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/ApplySkewInterpolation.hpp:18:    //mutable boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifyAXEFastMarketData.hpp:16:   boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifyFwdLimit.hpp:21:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifyFwdTickData.hpp:16:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifyInvertedPrice.hpp:26:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifyMinSpreadData.hpp:19:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifyPostCalculate.hpp:34:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifySpotLimit.hpp:19:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifySalesSpreadData.hpp:29:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/AutoQuoteConditions/VerifySpreadMultiplierData.hpp:17:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/CreateReciprocalRate.hpp:16:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/CrossSpotCrossFwd/ApplyCrossSpotCrossFwdDollarRate.hpp:14:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/CrossSpotCrossFwd/CalculateCrossFwdPointsForCrossSpot.hpp:59:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/CrossSpotCrossFwd/VerifyCrossSpotRatesForCrossFwd.hpp:15:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/CrossedSpot/VerifyCrossSpotRates.hpp:16: boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/DirectSpot/calculateDirectSpotPoints.hpp:13:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/DirectSpotCrossFwd/ApplyDSCFDollarRate.hpp:23:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/DirectSpotCrossFwd/VerifySpotRatesConsistency.hpp:14:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/DirectSpotCrossFwd/calculateMidSpotBased.hpp:58: boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/DirectSpotDirectFwd/calculateDirectFwdRate.hpp:34:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/InitializeTick.hpp:40:    boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/ApplyFwdToAlignSpot.hpp:29:      mutable boost::optional<InternalTPSTick> tickStorage;
	/axe/graph/PrepareSkewData.hpp:30:    boost::optional<InternalTPSTick> tickStorage;

	% ag 'optional<InternalTPSTick>' axe/graph/ | grep tickStorage | wc -l
	30

