 The Reuters Foundation API (RFA) is a key part of the RMDS API strategy. It provides a data-neutral, low-level, multi-threaded API that applications can use with Reuters systems to publish and consume data.

Using the RFA API, applications can be written once to operate in several different market data environments. For developers familiar with the TIB API, MDAPI or the SSL 'C' API, the RFA provides a similar level of data access where data is received as a generic buffer of information. Applications then have the freedom to parse and manage the data as required. 


This table defines market codes for various markets:

	1> select to 10 mCode,name from Market
	2> go
	 mCode       name
	 ----- ----------------------
		 0 TIB_IDN_SelectFeed
		 1 TIB_MultiContrib
		 2 TIB_EBS
		 3 EBS_Live
		 4 EBS_AI
		 5 Reuters_D3000
		 6 CME
		 7 Bloomberg
		 8 AVT
		 9 REU


This lists instrument names, subscriptions are made using this table:

	1> select * from MDATIBFeedInstrument where instrument like '%CAD%'
	2> go
	 id        source   instrument
	 --------  -------- --------------
		 4001  RSF      CAD=D2
		 5001  RSF      CAD=EBS

This maps exchange instrument names to common names:

	1> select * from ExchangeInstrument where exchangeInstrument like 'CAD%'
	2> go
	 id   exchangeName  exchangeInstrument  marketCode  marketInstrument
	 ---- ------------- ------------------- ----------- --------------------
		4 RSF           CAD=D2                        0 USDCAD
		5 RSF           CAD=                          1 USDCAD
		6 RSF           CAD=EBS                       2 USDCAD



Class MultiInstrumentMap (m_instrumentMap) implements the logic of mapping exchange instrument to market instrument based on ExchangeInstrument.


Main application logic in class MyApplication. MyApplication constructor:

1. initialiseContracts

	boost::shared_ptr<MDAQuoteContractImpl> idn;
	boost::shared_ptr<MDAQuoteContractImpl> multiContrib;
	boost::shared_ptr<MDAQuoteContractImpl> ebs;

	idn.reset(new MDAQuoteContractImpl(m_instrumentMap, priceCache,
		MDAHelper::instance().marketNameFromMDAIdentifier(TIB_IDN_SELECT_FEED)));

	multiContrib.reset(new MDAQuoteContractImpl(m_instrumentMap, priceCache,
		MDAHelper::instance().marketNameFromMDAIdentifier(TIB_MULTI_CONTRIB)));

	ebs.reset(new MDAQuoteContractImpl(m_instrumentMap, priceCache,
		MDAHelper::instance().marketNameFromMDAIdentifier(TIB_EBS)));

2. start Reuters service

	boost::shared_ptr< ReutersService > reutersService(new ReutersService());
	reutersService->startListening();

3. register MDATIBFeedInstrument listener

	tibFeedInstrumentCacheObserver =
		Cache < MDATIBFeedInstrument >::instance().addObserver(
			boost::bind(&MyApplication::onMDATIBFeedInstrumentChange, this, _1, _2));

3. subscribe to instruments listed in MDATIBFeedInstrument

	Cache < MDATIBFeedInstrument >::List instruments;
	Cache < MDATIBFeedInstrument >::instance().values(instruments);
	foreach(iter, instruments) {
		subscribeTo(PricedItemId((*iter)->getSource().str(), (*iter)->getInstrument().str()));
	}

	void subscribeTo(PricedItemId const &item) {
		std::pair < PriceListeners::iterator, bool > result = 
			priceListeners.insert(PriceListeners::value_type(item, boost::shared_ptr < RfaCallback >()));
		result.first->second.reset(new RfaCallback(m_instrumentMap, bleater));
		if (RedundancyService::isMaster())
			result.first->second->enablePublishing();
		reutersService->subscribe(result.first->second, item);
	}

So for each subscription, RfaCallback, when Reuters service receives a tick, it calls RfaCallback::onPrice to handle it:

	RfaCallback::onPrice(const PricedItemId& item, const Prices& prices)

	deque< InstrumentQuote> quotes;
	for( uint32 i=0; i<prices.getNumPrices(); ++i) {
		quotes.push_back(
			InstrumentQuote(
				prices[i].getId(),
				Decimal( prices[i].getPrice()), 				// rate
				(int64) prices[i].getQty(),						// volume
				prices[i].getSide()==Price::BID ? Side::BID : Side::ASK,	// side
				InstrumentQuote::TRADEABLE,						// type, allways tradeable
				prices[i].hasHiddenDepth()));					// hidden depth
	}


	MultiInstrumentMap::MarketInstruments mktInstrs;
	m_instrumentMap.lookup(item, mktInstrs);
	foreach (iter, mktInstrs) {
		const MarketInstrument & instrument = *iter;
		MDAHelper::instance().onInstrumentUpdate(
			m_dispatcher, 
			instrument.getMarket(),
			instrument.getInstrument().asciiString(), // instrument
			"SP", 									  // tenor
			quotes);
	}

For each RSF/PHP= from Reuters, fx_mda_tib_feed will send two ticks

	1> select * from ExchangeInstrument where marketInstrument like 'USDPH%'
	2> go
	 id        exchangeName   exchangeInstrument  marketCode  marketInstrument
	 --------- -------------- ------------------- ----------- ---------------------
	    512114 RSF            PHP=                          1 USDPH3
	     47001 RSF            PHP=                          1 USDPHP


	2014-11-04 11:31:14.119898 INFO RFA: Ticking RSF/PHP= : Bid 44.960000 Ask 44.980000
	2014-11-04 11:31:14.119936 INFO MDA: Ticking: RSF/PHP=  BID 44.96 ASK 44.98 for [ MarketInstrument: market=1 instrument='USDPHP' ]
	2014-11-04 11:31:14.120006 INFO MDA: Ticking: RSF/PHP=  BID 44.96 ASK 44.98 for [ MarketInstrument: market=1 instrument='USDPH3' ]
