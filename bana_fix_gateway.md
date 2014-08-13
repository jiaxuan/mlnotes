###################################################################################

# Setup

	FixApplication 
		id=814016 name='DEV-BANAFIX0' fixVersion=4.3

	FixGateway 
		id=533014 identifier='DEV-BANAFIX0' osAccountName='zk8xmsb'

	FixEnginePair 
		id=601014 name='DEV-BANAFIX0' rmiPort=19500
		ha1Host=sglsgfxapd5.sg.ml.com ha1ClusterPort=19400
		ha2Host=localhost ha2ClusterPort=19400
		ha1BAckUpHost=sglsgfxapd5.sg.ml.com ha2BackUpHost=sglsgfxapd5.sg.ml.com

	FixSession 
		id=3342006
		name='DEV-BANAFIX0-QTS' 
		applicaitonId=814016
		gatewayId=533014
		enginePairId=601014 enginePairClientPort=19100 engineClientConnType=0 engineGatewayPort=19300
		dialect=BANAStandard
		logonFixID=15174004
		targetFixID=999

	FixID (Logon)
		id=15174004 compId=DEV-BANAFIX0-QTS subId='' locId='' onBehalfOfRequired=N permissions=3 requestType=0

	FixID (Target)
		id=999 compId=ML-FIX-FX

	ExternalEntity (Logon) 
		id=21563004 externalId=15174004 externalType=1 principleId=44188028 principleType=0

	ExternalEntity (Target)
		id=9056004 externalId=999 externalType=1 principleId=34239003 principleType=0

	PantherUser (Logon)
		id=44188028 userName=DEV-BANAFIX0 fullName='banafix testUser (0)' 
		userType=2 defaultRegion='' loginUserName=DEV-BANAFIX0

	PantherUser (Target)
		id=34239003 userName=GALBRAITH-FIX-USER fullName='FIX - GALBRAITH' 
		userType=2 defaultRegion='' loginUserName='GALBRAITH-FIX-USER'

	PantherUserRole (Logon)
		id=64503752 userId=44188028 roleId=10 (trading client)

	GroupUser
		id=45266026 userId=44188028 groupId=15847006

	PantherGroup
		id=15847006 name=DEV-BANAFIX-0-FIX counterpartyId=12970003 assetId=10 channelId=20
		isPricingGroup=Y isTradingGroup=Y countryCodeId= primeBrokerId=

	Counterparty
		id=12970003 bankId=2 name=DEV-BANAFIX0-CPTY internal=N clientType=

	Channel
		id=20 name=FIX apiType='FIX 4.3' userOnBoarding=Y

	Asset
		id=10 name=FX description=GFX rank=2

	Bank
		id=2 name='Bank of America, N.A.' shortName=BANA usiPrefix=1030201141 (Unique Swap Identifier)

	PantherGroupPricingStream
	FXSettlementAccout
	FXGroupTradingLimits


###################################################################################

# Initialization

For gateway identifier `DEV-BANAFIX0`:


	void GatewayMain::init()
		initGateway();
		initSessions();
		// register callbacks on cluster master election
		registerCallbacks();

Load table `FixGateway` by `identifier=<identifier> and osAccountName=<username>`:

	[FixGateway]

	void GatewayMain::initGateway
		FixGateway::SearchCriteria criteria;
		criteria.isValid = 1;

		// gateway name comes from cmd line --identifier=...
		m_gateway = Cache<FixGateway>::instance().get(FixGateway::Index::by_name(gatewayName, username), criteria);

Load sessions:

	[FixGateway::id] -> [FixSession::gatewayId]

	name                logonFixID targetFixID enginePairId engineClientPort engineGatewayPort 
	------------------- ---------- ----------- ------------ ---------------- ----------------- 
	DEV-BANAFIX0-QTS    15181004   999         604005       19100            19300             
	DEV-BANAFIX44-0-QTS 15181006   999         604005       19100            19301             
	DEV-BANAFIX-ECN0-A  15181009   999         604005       19100            19302             
	DEV-BANAFIX-ECN0-B  15181010   999         604005       19100            19303             
	DEV-BANAFIX0-ORD    15181005   999         604005       19100            19304             
	DEV-BANAFIX44-0-ORD 15181007   999         604005       19100            19305             
	DEV-BANAFIX0-CNX    15181025   999         604005       0                19306             

	dialect          applicationId 
	---------------- ------------- 
	BANAStandard     817007        
	BANAStandardV4.4 817006        
	BANAStandard     817007        
	BANAStandard     817007        
	BANAStandard     817007        
	BANAStandardV4.4 817006        
	CNXStandard      817005        

	[FixSession::enginePairId] -> [FixEnginePair::id] -> EngineConnection

	name         ha1Host               ha2Host   ha1ClusterPort ha2ClusterPort rmiPort 
	------------ --------------------- --------- -------------- -------------- ------- 
	DEV-BANAFIX0 sglsgfxapd5.sg.ml.com localhost 19400          19400          19500   

	ha1BackUpHost         ha2BackUpHost         
	--------------------- --------------------- 
	sglsgfxapd5.sg.ml.com sglsgfxapd5.sg.ml.com 

These settings correspond to how CameronFIX is configured:

	$PANTHER_FIX_DIR/DEV-BANAFIX0/config/config.xml

	<Application rmiName="DEV-BANAFIX0" rmiPort="19500">

		<HighAvailabilityCluster id="DEV-BANAFIX0_cluster"
			transactionTimeout="3000" primaryDeterminationTimeout="5000">
			<Server name="sglsgfxapd5_DEV-BANAFIX0" ip="sglsgfxapd5.sg.ml.com" port="19400" />
			<Server name="localhost_DEV-BANAFIX0" ip="localhost" port="19400"/>
		</HighAvailabilityCluster>

		<Parties>
			<Party compId="DEV-BANAFIX0-QTS">
				<Session fixversion="4.3" heartbeat="60" compid="ML-FIX-FX" dictionaryName="BANAStandard"
					useDateMilliseconds="true" ...>
					<MessageFactory id=...>
						<ToFIX>...</ToFIX>
						<FromFIX>...</FromFIX>
					</MessageFactory>
					<Persister>...</Persister>
				</Session>

				<Connections>
					<SocketConnection>listenport="19100"/>
				</Connections>

				<SessionManager>
					<SourceMessageProcessors>
						<PantherSocketAdapter id="sock_1" port="19300" enablePasswordManagement="false">
							<Properties>...</Properties>
						</PantherSocketAdapter>
					</SourceMessageProcessors>
					<ListenMessageProcessor>
						<PantherSocketAdapter id="sock_1"/>
					</ListenMessageProcessor>
				</SessionManager>
			</Party>

			<Party compId="DEV-BANAFIX44-0-QTS">
				<Session fixversion="4.3" heartbeat="60" compid="ML-FIX-FX" dictionaryName="BANAStandardV4.4" ...>
				</Session>

				<Connections>
					<SocketConnection>listenport="19100"/>
				</Connections>

				<SessionManager>
					<SourceMessageProcessors>
						<PantherSocketAdapter id="sock_2" port="19301" enablePasswordManagement="false">
						</PantherSocketAdapter>
					</SourceMessageProcessors>
					<ListenMessageProcessor>
						<PantherSocketAdapter id="sock_2"/>
					</ListenMessageProcessor>
				</SessionManager>
			</Party>

			...
		</Parties>

	</Application>

Note that each party is identified by a different value of "compid". This corresponds to the standard Fix CompID fields: SenderCompID and TargetCompID. In fact, a party can be further defined by a "subid" and "locationid" (corresponding to the Fix Sender/TargetSubID  and Sender/TargetLocationID fields). So a party entry could look like:
 
	<Party compid="PartyX" subid="123" locationid="Desk A">

Implementation details:

	void GatewayMain::initSessions

		1. load sessions by  FixGateway::id == FixSession::gatewayId, each session corresponds to a party in cameron config
		2. load engine pair by FixEnginePair::id == FixSession::enginePairId
		3. create an engine connection for each engine session

			EngineConnection_ptr connection(new EngineConnection(*engine, *session, 
				DialectFactory::getDialect(FixSession::dialect)));
			m_engineConnections.push_back(connection);

		   Note that connections are created as:

			m_primaryHost =  InetAddress(enginePair.getHa1Host().str(), session.getEngineGatewayPort());
			m_backupHost  =  InetAddress(enginePair.getHa2Host().str(), session.getEngineGatewayPort());

These engine connections become active once the gateway becomes cluster master:

	void GatewayMain::onGainMastership()

		// update gatewayIdentifier in localdb::LoggedInPrincipal to this gateway
		ml::panther::accesscontrol::AccessControl::instance().resetLoggedInUsers( 
				ml::spec::panther::session::LoggedInFrom::FIX_GATEWAY, getIdentifier() );

		m_mastershipObservable.mastershipChanged(true);		// notify before other actions

		foreach(engine, m_engineConnections)
			(*engine)->start();
		
		m_gatewayContract.start();
		...

###################################################################################

# Engine Connection

Communication with Cameron Engine is handled by `bana_fix_gateway/client/EngineConnection.hpp`,
which implements Camerons TCP adaptor interface.

	// each engine connection has a dialect-specific session. the
	// session is created from the provided dialect provider, for bana, this would be 
	//     dialect/bana/BANAProvider
	//     client/BANA/BANASession

	EngineConnection::EngineConnection
		, m_sendQueue(new ConsolidatingFixMessageQueue()) // its own consolidating send queue
		, m_dialect(dialect)
		// note that
		// 1. the connection has its own session
		// 2. the send queue is passed to the session
		, m_sessionPtr(m_dialect.createSession(m_fixSession, m_sendQueue, *this)) 


Upon initialization, it looks up FixApplication by

	[FixSession::applicationId] -> [FixApplication]

	FixApplication::id == FixSession::applicationId

and loads application name and FIX version from it. It also sets up the consolidating message queue:

	ml::ext::fix::ConsolidatingFixMessageQueue_ptr m_sendQueue;
	ml::common::pattern::AutoObserver              m_consolidationObserver;

	m_consolidationObserver.reset(*m_sendQueue, boost::bind(&EngineConnection::onMessageConsolidation, this, _1, _2));

	void EngineConnection::onMessageConsolidation(ConsolidatingFixMessageQueue &, Message_ptr &message)
		GLINFO(m_appName.c_str(), ">> Dropped: %s", message->toString().c_str());

Upon start, it starts three worker threads:

	ml::common::concurrent::Task  m_connectTask;
	ml::common::concurrent::Task  m_readTask;
	ml::common::concurrent::Task  m_writeTask;

	m_connectTask.run(boost::bind(&EngineConnection::taskEstablishConnection, this));
	m_readTask.run(boost::bind(&EngineConnection::taskReadMessage, this));
	m_writeTask.run(boost::bind(&EngineConnection::taskWriteMessage, this));

The worker threads use state (awaitState, not a good idea, better use non-blocking
state machine) to synchronize activities among them:

	bool EngineConnection::taskEstablishConnection()
		awaitState(STATE_OPEN)
		m_socket = defaultProtocolFactory()->create(m_socketProtocol, socket);
		m_sendQueue->enable();
		changeState(STATE_OPENING, STATE_OPEN)
		m_socket->addObserver(boost::bind(&EngineConnection::onClientConnectionStateChange, this, _1, _2));
		...

	bool EngineConnection::taskReadMessage()
		
		awaitState(STATE_OPEN)
		
		boost::shared_ptr < FixMessageSocket > socket(m_socket);
		Message_ptr message;
		socket->pollMessage(message, MESSAGE_READ_TIMEOUT) // recieve from network

		FixMessage_ptr messageObj = m_dialect.fromMessageString(*message); [
			libs/fx/fx_dialect/FXDialectProvider::fromMessageString(Message& messageString)
				
				FieldMap fields = createFieldMap(messageString)

				// check various fields: 
				//    senderCompId, senderSubId, senderLocationId,
				//    targetCompId, targetSubId, targetLocationId,
				//    onBehalfOfCompId, OnBehalfOfSubId

				// check sendingTime

				// translate msgTypeString to msgType

				FixParty sender, target
				init sender and target with comp/sub/loc/onBehalfOf ids

				switch (msgType)
				case Message::LOGON:
					Logon msg = getMessageFactory().createLogon()
					msg->setSender(...)
					msg->setTarget(...)
					set sendingTime if necesssary
					getIncomingDialect().processLogon(msg, messageString, fields) ...
				case Message::QUOTE_REQUEST:
					...
					getIncomingDialect().processFXQuoteRequest(...)...

			BANAProvider::getIncomingDialect() returns BANAIncoming
			BANAIncoming::processFXQuoteRequest() ...
		]
		m_sessionPtr->receiveMessage(messageObj);

		// note that we have two stages of checking/processing
		// 1. BANAProvider::fromMessageString, BANAIncoming::process???
		// 2. BANASession::receiveMessage


	bool EngineConnection::taskWriteMessage()
		awaitState(STATE_OPEN)
		ConsolidatingFixMessageQueue::Entry_ptr entry;
		boost::shared_ptr < FixMessageSocket > socket(m_socket);

		// this calls ConsolidatingFixMessageQueue::get(...), very
		// bad concurrency, a lot of room for improvement

		if (m_sendQueue->get(entry, 5000))
			message = entry->message;
			m_FixOptionalTagsFilter.filter(*message);
			tagTransManager.applyTransformations( m_fixSession.getId(), *message );
		else
			hb->setSender(m_sessionPtr->getSender());
			hb->setTarget(m_sessionPtr->getSessionTarget());
			message = m_dialect.createHeartbeat(hb);
		
		socket->writeMessage(*message);



	Session::sendMessage(msg)
		msgString = m_dialect.toMessageString(msg) [
			libs/fx/fx_dialect/FXDialectProvider::toMessageString(msg)
				switch (msg.getMessageType())
				case Message::LOGON:
					const Logon& _msg = dynamic_cast<const Logon&>(msg);
					getOutgoingDialect().processLogon(_msg);

				BANAProvider::getOutogingDialect() returns BANAOutgoing
				BANAOutgoing::processLogout() ...
		]
		m_queue->put(msg.getTarget(), msgString)

	Session::sendConsolicatedMessage(msg, key)
		msgString = m_dialect.toMessageString(msg);
		m_queue->put(msg.getTarget(), msgString, key);

	// note the inconsistencies: 
	//    Session: sendMessage,  sendConsolidatedMessage
	//    ConsolidatingFixMessageQueue  put(target, str), put(target, str, key)


Note that `BANAIncoming` and `BANAOutgoing` are basically stateless and
can be optimized away.

For bana, the session is created as

	Session_ptr BANAProvider::createSession(FixSession& session,
						ConsolidatingFixMessageQueue_ptr& queue, // this is engine connections outgoing queue
						EngineConnection & engine)
		const int64 id = session.getId();
		FXPositionTracker_ptr fxPositionTracker = findFXPositionTracker(id, m_fxPositionTrackerMap);
		FXQuoteStoreManager_ptr fxQuoteStoreManager = FXQuoteStoreManager::createQuoteStore(*fxPositionTracker, session.getName());
		BigOrderTracker_ptr bigOrderTracker_ptr = findBigOrderTracker(id, m_bigOrderTrackerMap);

		BANASession* pSession = new BANASession(session, engine, queue, ...


	class BANASession: public ml::ext::fix::protocol::Session
	class Session
		ml::ext::fix::ConsolidatingFixMessageQueue_ptr m_queue; // outgoing queue of engine connection


	FXPositionTracker_ptr m_positionTracker_ptr;		// one per fix session id
	BigOrderTracker_ptr m_bigOrderTracker_ptr;			// one per fix session id
	FXQuoteStoreManager_ptr m_quoteStoreManager_ptr;	// one per session

	// these members are per session

	ThrottleManager m_throttleManager;
	DispatcherAutoSubscriber m_RFQDispatcher;

	BANASessionQuoteManager m_quoteManager;
	BANASessionOrderManager m_orderManager;
	BANASessionScheduleManager m_scheduleManager;

	// static global data
	static int sqm_count; // session count/id
	static set<BANASession*> m_sessions; // existing sessions


	// reply subject is
	//          gw: "rv_ps.fix_gateway_session_reply"
	//         das: "rv_ps.FIX.<pid>.Orders.<client_comp_id>"
	//         rfq: "RFQ.<session_count>"

	BANASession::BANASession( ...
			ConsolidatingFixMessageQueue_ptr &queue, // outgoing queue of EngineConnection
			FXPositionTracker_ptr & fxPositionTracker_ptr,
			FXQuoteStoreManager_ptr & fxQuoteStoreManager_ptr,
			BigOrderTracker_ptr& bigOrderTracker_ptr ... )
		:Session (session, queue, dialect)

		, m_positionTracker_ptr(fxPositionTracker_ptr)
		, m_bigOrderTracker_ptr(bigOrderTracker_ptr)
		, m_quoteStoreManager_ptr(fxQuoteStoreManager_ptr)
		
		, m_quoteManager(*this, m_RFQDispatcher.getDispatcher())
		, m_orderManager(*this, SettlementAccountGetterPtr(new GlobalSettlementAccountGetter))
		, m_scheduleManager(*this)

		, m_throttleManager(session.getId(), session.getApplicationId())
		, m_RFQDispatcher( makeTopicString( "RFQ", sqm_count ) )
		, m_dispatcher(ContractTopics::instance().makeTopicString(
								RouterFactory::TIBRV_PUBSUB, "fix_gateway_session_reply") )
		, m_das(ContractTopics::instance().makeTopicString(
								RouterFactory::TIBRV_PUBSUB, createReplyChannelName(getClientCompId())))
	{
		m_keyOnChangeFixID = Cache<FixID>::instance().addObserver(boost::bind(&BANASession::onChangeFixID, this, _1, _2) );
		m_keyOnChangeFixSession = Cache<FixSession>::instance().addObserver(boost::bind(&BANASession::onChangeFixSession, this, _1, _2) );
		sqm_count++;

		m_asyncRequest = RegistryHelper::getRegistryBool("/PROCESS", "allow_aync_request_handling", false);
		if(m_asyncRequest) {
		  m_requestHandlingThreadPool_size = RegistryHelper::getRegistryInteger("/PROCESS", "async_request_thread_pool_size", 3);
		  m_requestHandlingThreadPool.reset(new ml::common::concurrent::ThreadPool(m_requestHandlingThreadPool_size, "async_request_thread_pool"));
		}
		// add self to session list
		BANASession::add(this);


and incoming messages are handled as:


	void BANASession::receiveMessage(const FixMessage_ptr &message)
		// sanity checking
		if (message->getMessageType() == ml::ext::fix::Message::HEARTBEAT)
			return;

		if (message->getMessageType() == ml::ext::fix::Message::LOGON)
			processLogonMessage(message);

		handleMessageByType(message);


	void BANASession::handleMessageByType(const FixMessage_ptr &message)
		switch (message->getMessageType())
		case Message::LOGON:  			connect();
		case Message::QUOTE_REQUEST:    doHandleQuoteRequest(message);
		case Message::NEW_ORDER :       handleOrderRequest(FXNewOrderSingle::fromMessage(message));
		case Message::NEW_ORDER_MULTI_LEG : m_orderManager.handleNewOrderMultiLeg(FXNewOrderMultiLeg::fromMessage(message));
		case Message::ALLOCATION :      handleTradeAllocRequest(FXTradeAllocRequest::fromMessage(message));
		case Message::REJECT :          doHandleReject(message);
		case Message::LOGOFF :          disconnect();


	void BANASession::handleQuoteRequest(const FixMessage_ptr & message)
		m_quoteManager.handleQuoteRequest(FXQuoteRequest::fromMessage(message));

	void BANASession::handleOrderRequest(const FXNewOrderSingle_ptr & message)
		m_orderManager.handleNewOrderSingle(message);

###################################################################################

# Position Tracker

Keeps track of quotes and orders

	map<string, PositionInfo> quoteMap; 	// quoteId -> positionInfo
	map<string, OrderInfo> orderMap;		// orderId -> orderInfo

	PositionInfo { bidSize/initBidSize/bidPrice, offer...}

	// add a new quote iff the quoteId is new
	// Note that bidSize is how much the buyer can buy
	//      but offerPrice is the price the buyer pays for 
	void addQuote(quoteId ...)
		// there are two quote conventions
		if(isInTermsOfBase)
			quoteMap.insert(QuoteMap::value_type(quoteId, PositionInfo(bidSize, offerSize, bPrice, oPrice)))
		else
			quoteMap.insert(QuoteMap::value_type(quoteId, PositionInfo(offerSize/oPrice, bidSize/bPrice, bPrice, oPrice)))


	// remove
	retireQuote(quoteId)

	// check if an order should be accepted or rejected
	bool addOrder(quoteId, orderId, quantity, side, isInTermsOfBase, multiplier, limit, usdRate)
		quote = quoteMap[quoteId]
		remaining = currentSize - quantity


		if (remaining < -1e-09) { // more than available
			isSmallOrder = true
			if (quantity <= initialSize && usdRate > 0)
				initSizeInUsd = initSize * usdRate
				if (initSizeInUsd > limit * multiplier)
					isSmallOrder = false
				else 
					if (initSizeInUsd > limit)
						finalLiquidity = limit * multiplier / usdRate
					else
						finalLiquidity = initSize * multiplier
					if (finalLiquidity + remaining - initSize < -1e-09)
						isSmallOrder = false

			if (!isSmallOrder)
				return false
		}

		// udpate quote remaining size
		quote.?Size = remaining
		orderMap[orderId] = OrderInfo(quantity, side, quoteId, isInTermsOfBase)
		return true

`addQuote` and `addOrder` are called to register an order with the position tracker
before sending a order to the trade server. When a success response is received from
the trade server, the following is called:

	void addExecution(string& orderId)
		update quote bid or offer size accrordingly
		remove the order from order map

If it is a failure, the following is called:

	void rejectExecution(string& orderId) 
		update quote bid/offer size
		remove the order from order map



###################################################################################

# Quote Manager

Important members:

	typedef map<string, Subscription> SubscriptionMap;

	SubscriptionMap m_subscriptionMap	// FIX requestId -> Subscription
	map<int64, SubscriptionMap::iterator> m_requestMap	// TPS requestId -> (FIX requestId -> subscription)
	set<string> m_usedSubscriptionIDs	// Existing FIX requestIds
	set<string> m_subscribedQuotes		// Existing subscriptions tracked by (Symbol, Currency, Tenor, SenderCompId, Side)

	LocalQuoteLifecycleManager& m_localQuoteLifecycleManager
	SubscriptionAmoutChecker& m_subscriptionAmoutChecker

	Cache<CurrencyPair> m_currencyCache

Note that its base class `SessionQuoteManager` has the **same private** members such
as m_subscriptionMap and m_requestMap.

It uses its own tps client:

	BANASessionQuoteManager::BANASessionQuoteManager(...)
		, m_tpsClient(new tpsclient2::TpsClient(*this..)) // self as callback

		// set these from registry /PROCESS/...
		m_allowEspSwap = ...
		m_allowEspForward = ...
		m_allowNDFSwap = ...
		m_asyncQuote = ...
		m_allowMismatchedTenors = false; // set to true by Currenex
		if async, init thread pool

It uses tps client callback to receive quotes:

	class BANASessionQuoteManager : 
		public SessionQuoteManager,
		public ResponseHandlerInterface // manual quotes from RFQ server

	// quotes from TPS
	class SessionQuoteManager : public ml::fx::tpsclient2::TpsClientCallback   // tpsclient 

	// this interface is for handling manual quotes from RFQ server
	class ResponseHandlerInterface
		//called when a manual spot quote is provided by RFQ Server
		virtual void onQuote(FXRequestId& requestId,

		//called when a manual swap quote is provided by RFQ Server
		virtual void onQuote(FXRequestId& requestId,...

		//called when a manual block quote is provided by RFQ Server
		virtual void onQuote(FXRequestId& requestId,...
		
		//called when a quote is cancelled. The implementor is expected to withdraw the
		//latest quote sent to client. If incase of an autoQuoted stream, the Implementor is responsible
		//to unsubscribe from tps as the request will become manual quoted.
		virtual void onQuoteCancel( FXRequestId& requestId ) = 0;
		
		//called when a stream is cancelled by trader/RFQ server. 
		virtual void onRequestCancel( FXRequestId& requestId, ...

		//the sales person has overriden credit failure - go to TPS for auto quoting
		virtual void creditOverriden( FXRequestId& requestId ) = 0;

## Quote request

	void BANASessionQuoteManager::handleQuoteRequest(const ml::fx::fix_dialect::message::FXQuoteRequest_ptr &request)
		//validate symbol (remove /, uppercase and look it up in CurrencyPair)
		if(!validateSymbol(m_currencyCache, *request->getSymbol()))
			sendQuoteRequestReject(request, ...;

		if (request->getStreamingDuration() == FXQuoteRequest::CANCEL)
			handleCancelQuoteRequest(request);
		else
			handleNewQuoteRequest(request);

### New request:

	void BANASessionQuoteManager::handleNewQuoteRequest

		validate sender against PantherUser and PantherGroup

		validate streaming duration, can be either RFS or ESP, RFS stops if any of the
		following happens: 1. after a preconfigured period. 2. a trade has been made
		3. it is cancelled. For ESP, 1 and 2 dont apply.

		if ESP, block quote not supported, so must not have noLegs specified, check

		check currency matches symbol

		if throttleManager has quote request limit 
			check quote request count is within limit

		// the quote request limit comes from FixSessionOption
		select * from FixSessionOption where sessionId=(select id from FixSession where name='DEV-BANAFIX0-QTS')
		id      optionTypeId sessionId defaultValue defaultFlag 
		------- ------------ --------- ------------ ----------- 
		1605770 24           3345006   19.00000000  N           
		1605771 40           3345006   1.00000000   Y           

		select * from FixOptionType 
		id name                       defaultValue     defaultFlag isValueRequired isFlagRequired 
		-- -------------------------- ---------------- ----------- --------------- -------------- 
		1  Quote Request Limit        0E-8             N           Y               N              
		2  Account Required           0E-8             N           N               Y              
		3  Minimum Streaming Duration 60000.00000000   N           Y               N              
		4  Maximum Streaming Duration 3600000.00000000 N           Y               N              
		5  Stale Quote TTL            4000.00000000    N           Y               N              

		if ESP, check subscription amount:
			m_subscriptionAmountChecker->isSubscriptionLimitCheckOk(request, groupId) {
				convert requested amount to usd
				1. check against FXGroupSubscriptionLimit using groupId

				select * from FXGroupSubscriptionLimits where groupId=1318001
				id      groupId ccy tenor minLimit maxLimit  
				------- ------- --- ----- -------- --------- 
				2486001 1318001 AUD SN    (null)   100000000 
				2487001 1318001 AUD TOM   (null)   100000000 
				2489001 1318001 NZD SN    (null)   100000000 
				2488001 1318001 NZD TOM   (null)   100000000 

				2. check against FXDefaultSubscriptionLimits
				the table seems to be empty
			}
			
		check quoteReqId: 1. not null, 2. not empty. 3. len less than 65

		check side
		check qty

		check tenor and value date
		if (request->getNoLegs().isNull()) {
			tenor = request->getTenorValue()
			valueDate = request->getTenor()
		} else {
			// block trades, use spot tenor
			tenor = "SP"
		}
		1. check if tenor and valueDate are present
		2. validate tenor
		3. validate and convert value date, this may update tenor and valueDate

		request->setTenorValue(tenor)
		request->setTenor(valueDate)

		check throttleManager if one-way request should be rejected (FixSessionOption::id=32)
			if so, check if value date is later than spot date, if yes,
			send a quote request reject. do the same for each leg if its a block quote req

		if its swap, check qty, tenor, valueDate etc of the far leg

		if both tenor and valueDate are present, check if they are consistent
		if mismatch, check if m_allowMismatchedTenors is set (Currenex, CNXSession, sets this to true)

		if ESP
			reject if its swap and m_allowEspSwap is false
			reject if its fwd and m_allowEspForward is false

		if one way request and option send-2way-price-for-1way-req (FixSessionOption::id=33) is set
			modify the request by adding the opposite side

		call SpecialCcyUtil.s.cpp::validateAndConvertSpecialProduct(...) to handle special cases for:
			NDF, ROE/LVN, PM, OST

		that call may lead to data change, so apply them
		request->setSecurityType(securityType);
		request->setSymbol(convertedSymbol);
		request->setSettlCurrency(settlCurrency);

		// select * from Currency
		isoCode rank isLCT qtyPrecision roundMode ccySet         mapCcy ccySalesGroup type longName      
		------- ---- ----- ------------ --------- -------        ------ ------------- ---- ------------- 
		EUR     9000 N     2            0         Default        (null) G7            0    Euro          
		GBP     8900 N     2            0         Default        (null) G7            0    British Pound 
		XT2     8680 N     3            0         PreciousMetals XPT    PRECIOUS      7    Platinum Zurich 
		FJ3     8600 N     2            0         Default        FJD    ROE/LVN       4    Fijian Dollar 
		KWD     8495 N     2            0         European       (null) EMEA          0    Kuwaiti Dinar 
		ARS     8266 N     2            0         Default        (null) NDF           0    Argentina Peso 
		AR_     8264 N     2            0         LatAm          ARS    NDF           1    Argentina Peso 
		PEN     8240 N     2            0         Default        (null) NDF           0    Peruvian Nuevo Sol 
		PE3     8235 N     2            0         Default        PEN    LATAM         4    Peruvian Nuevo Sol 

		enum RoundMode (ROUND_UP, TRUNCATE);
		enum CurrencyType (
			NORMAL, NDF, ROE, LVN_SELL, LVN_BUY_SELL, OST, METAL_LONDON, METAL_ZURICH);

		certain currency types use currency names different from isoCode.
		mapCcy maps non-standard ccy values to isoCode, see FXCurrencyUtils.cpp::getSpecialCcy(...)

		// NDF-specific logic
		if NDF {
			if swap check m_allowNDFSwap
			check NDF fixing date
			if swap {
				check fixing date 2
				check tenor
			}
			// NOTE that when fixing date mismatch with tenor (including swap), if
			// m_allowMismatchedTenors is set (CNX), the request is queued for
			// execution after a day roll, no reject or quote will be sent out
				m_session.addMismatchedRequest(request)
				return

		}

		// convert ccy field
		ccy = request->getValueCurrency()
		convertFromExternalCurrencyToInternalCurrency(ccy, isNDF, isROE, isPM, pmLocation, isOST)
		request->setValueCurrency(ccy)

		// set current spot date for future use
		FXCalendar::instance().getCurrentTradingDay(CcyPair, today)
		spotDate = FXCalendar::instance().getSpotDate(ccyPair, today)
		request->setCurrentSpotDate(spotDate)

		if (request->getNoLegs())
			subscribeForQuoteBlock()
		else if (request->getFarVolume())
			validateSwapQuoteRequest(request)
			subscribeForQuotes<FXSwapRequest, FXSwapQuote, CreditCheckSwapRequest, CreditCheckSwapResponse>(...)
		else
			subscribeForQuotes<FXOutrightRequest, FXOutrightQuote, CreditCheckRequest, CreditCheckResponse>(...)

		// send lifecycle event, exes/fx/diag/quote_lifecycle_monitor can be used to monitor these events
		m_localQuoteLifecycleManager.sendLifeCycleEventOnQuoteRequest(request);


### Subscribe for Quotes

	// a template method that works for both outrights and swaps
	void BANASessionQuoteManager::subscribeForQuotes(...forceManualQuoting...)
		QuoteRequestAccessChecker validator = new QuoteRequestAccessChecker(request, principal)
		validator->validate() {
			// check PantherGroup::isPricingGroup
			select * from PantherGroup where name='DEV-BANAFIX-0-FIX'
			id       name                  counterpartyId assetId channelId isPricingGroup isTradingGroup countryCodeId primeBrokerId 
			-------- --------------------- -------------- ------- --------- -------------- -------------- ------------- ------------- 
			16041006 DEV-BANAFIX-0-FIX     13074003       10      20        Y              Y              (null)        (null)        
		}
		
		createFXQuoteRequest(tpsRequest, ...) {		// create TPS request from request
			// a lot of duplicate sanity checking is performed here

			// getValueCurrency() -> Currency (tag=15)
			if (*request->getValueCurrency() == *request->getCcy1())
				isInTermsOfBase = true;
			else if (*request->getValueCurrency() == *request->getCcy2())
				isInTermsOfBase = false;

			// volumn and side

			if (request->getQuoteType() == FXQuoteRequest::INDICATIVE) {
				volume = ml::common::math::Decimal::ZERO;
				volumeSide = FXOutrightRequest::BASE_VOLUME;
				side = FXOutrightRequest::BOTH_SIDES;
			} else {
				// side
				volumeSide = (isInTermsOfBase) ? FXOutrightRequest::BASE_VOLUME : FXOutrightRequest::TERMS_VOLUME;
				switch(request->getSide()) {
				case ml::fx::common::Side::ASK:
					side = volumeSide == FXOutrightRequest::BASE_VOLUME ? FXOutrightRequest::SELL : FXOutrightRequest::BUY
				case ml::fx::common::Side::BID:
					side = volumeSide == FXOutrightRequest::BASE_VOLUME ? FXOutrightRequest::BUY : FXOutrightRequest::SELL
				default:
					side = FXOutrightRequest::BOTH_SIDES
				}

				// check volumn rounding based on  Currency::roundMode
				Decimal roundedVolume = request->getVolume();
				FXCurrencyUtils::roundCurrencyValue(roundedVolume, request->getValueCurrency())
				if roundedVlume != request->getVolume()
					reject
			}

			// get settlement account
			FXSettlementAccount sa = getFXSettlementAcct(groupId, subId, 
				preciousMetal ? FXSettlementAccount::PM : FXSettlementAccount::FX) {
					1. search FXSettlementAccount by (groupId, senderSubId) on (groupId, boAccountId)
					2. if not found, search by (groupId, isDefault=true)
				}


			select * from FXSettlementAccount where groupId=16041006

			id       groupId  name    description copperId isCLS isDefault accountType boAccountId boAccName  
			-------- -------- ------- ----------- -------- ----- --------- ----------- ----------- ---------- 
			34958008 16041006 default default     132198   N     Y         0           167370001   EVERBA-FLO 
			
			bankId channelAcctRef boAccountType branchCode branchName isVoiceAccount 
			------ -------------- ------------- ---------- ---------- -------------- 
			2      default        0                                   N              


			//	Type of the account
			enum AccountType (	SETTLEMENTACCOUNT, TRADERBOOK );	

			// Type of BO account
			enum BoAccountType (FX, PM);

			// CoperId - Credit identifier
			copperId varchar(25) displayWidth(25);	

			//Whether the account is support in the CLS (Continually Linked Settlement) System
			isCLS boolean;

			//reference to back office settlement account table.
			boAccountId int64;

			// if not empty should have value from fix field of "SenderSubID"
			// so trader server could lookup settlement account with SenderSubID	
			channelAcctRef varchar(50);


			select * from BOSettlementAccount where name='EVERBA-FLO'
			
			id        backOfficeId accountId name       longName                bankId copperId isCLS showInOrderBook 
			--------- ------------ --------- ---------- ----------------------- ------ -------- ----- --------------- 
			167370001 2            83683001  EVERBA-FLO EVERBANK (JACKSONVILLE) 2      132198   N     N               
			
			type ownerTypeId boAccountType branchCode branchName 
			---- ----------- ------------- ---------- ---------- 
			0    1           0                                   

		}

		tpsRequestList.push_back(tpsRequest)		// enqueue TPs request

		// prepare terminator, rejector and callback

		Subscription::Terminator terminator =
		if RFS
			bind(onRFSSubscriptionEnd ...)
		else
			bind(onSubscriptionEnd ...)

		Subscription::Rejector rejector = bind(sendQuoteRequestReject ...)
		Subscription::SubscriptionCallback firstQuoteTimeoutCallback = bind(onInitialQuoteTimout...)

		// create subscription
		Subscription::Ptr subscription(new Subscription(..., m_tpsClient, terminator, rejector ...))
		
		// set isRFS, isSwap, fixRequestId, tpsRequestId ...
		// set throttle
		subscription->quoteThrottle = getQuoteThrottle(*request, m_session.getThrottleManager(), m_quoteThrottleTimer) {
			create FixIntervalThrottle based on settings in FixSessionOption, related option types are:

			id name                        defaultValue   defaultFlag isValueRequired isFlagRequired 
			-- --------------------------- -------------- ----------- --------------- -------------- 
			14 Min Outright Quote Interval 30000.00000000 N           Y               N              
			15 Min Swap Quote Interval     30000.00000000 N           Y               N              
		}


		BANASessionQuoteManager::SubRejectReason subscriptionResult = SUBSCRIPTION_PASS;

		// register the subscription 
		subscriptionResult = onSubscriptionStart(subscription) {
			m_usedSubscriptionIDs.insert(requestId)
			m_subscriptionMap.insert(requestId, subscription)
			m_requestMap.insert(tpsRequestId, requestId)
			string id = subscription->getUniqueSubData() {
				 a string by concat (Symbol, Currency, Tenor, SenderCompId, Side)
			}
			m_subscribedQuotes.insert(id)
		}

		if (subscriptionResult != OK) sendReject

		// setup quote permission revocation listener
		subscription->permissionRevokationObserver =
			acl::AccessControl::instance().addQuotePermissionRevocationListener(
				tpsRequest->getGroupId(),tpsRequest->getCcyPair().str(),
				quotePermissionRevokeCallback (inited with onRFSSubscriptionEnd/onSubscriptionEnd, sendQuoteRequestReject ),
				subscription->isSwap );

		// check credit for RFS
		if RFS {
			isCreditOkay = validateCreditCheckOk<...>(...) {
				// again. check requestId, groupId etc
				FXSettlementAccount sa // get settlement account the same way as above

				// pre-trade credit checking required ? 
				// doCreditCheckOnQuote checks the FXGroup::doCreditCheck table by groupId
				// select * from FXGroup where groupId=16041006
				doCreditCheck useFwdHigherQuotePrecision doCreditCheckRFSOnGUI doSubscriptionAntiSweep ... ... ...
				------------- -------------------------- --------------------- ----------------------- 
				0             N                          2                     N                       

				if (CreditCheckHelper::doCreditCheckOnQuote(groupId)) {
					CreditCheckHelper checker(*m_RFQDispatcher)
					string leg("0")
					// create request, including a unique client_id of the form RFQ_<userId>_<timestamp>
					// a unqiue request_id of the form RFQ_OUT<userId>_<fixRequestId>
					checker.createCreditCheckRequest(....,sa, leg)

					// check credit with any of
					// 1. credit_server using CreditCheckContractProxy (MLFXRTCMCreditClientImpl)
					// 2. fx_dap_interface using FXDAPCreditCheckContractProxy (DAPRTLCCreditClientImpl)
					// 3. diabled (DisabledCreditClient)
					// 

					rsp = checker.doCheck<...>(req)

					To decide the type of credit checking to perform, 
					it calls libs/panther/creditcheck/CreditClient.cpp::getCreditClientImplFunc(...) {
						checks FXCreditConfigOverride using the following criteria
							(coperId, bandId, productType)
							(coperId, bandId, ALL        )
							(coperId, NULL  , productType)
							(coperId, NULl  , ALL        )
							(ALL    , NULL  , productType)
							(ALL,   , bankId, ALL        )
							(ALL,   , NULL  , ALL        )
						finally, check registry /creditcheck/defaultCreditCheck
					}

					//Credit Config Override: contains credit check flags for coper 
					class FXCreditConfigOverride {
						enum CreditCheckOverrideType ( DAP_RTLC, MLFX_RTCM, DISABLED );
						enum ProductType ( ALL, SPOT, FWD, SWAP, NDF, PM );

						// id for which we need to override global config 
						coperId varchar(25) readOnly displayName("CoPeR Id" );

						// Bank id
						bankId int64 readOnly null displayName("Bank id") foreign key Bank(id);

						//Credit Check Type
						creditCheckType refEnum CreditCheckOverrideType displayName("Credit Check Type" );

						//Product Type
						productType refEnum ProductType displayName("Product Type" );
						...
					}
				}
			}
		}

		// now start subscription

		subscription->start(tpsRequestList, !forceManualQuoting) {
			schedule the initial quote timeout task to run
			if (!forceManualQuoting || !this->isRFS)
				tpsClient->subscribe(tpsRequestList);

			// so if its manual quoting, we only schedule 
			// timeout checking w/o sending tps request, the actual
			// quoting is handled by the logic below
		}

		if isCreditOkay != CC_PASS
			subscription->disableQuoting(...)

		if isCreditOkay == CC_NO_ACCOUNT
			onSubscriptionEnd(subscription...)
			sendReject

		// note this, necessary ?
		validator->validate()
		if failed
			onSubscriptionEnd(subscription...)
			sendReject		

		if isRFS && !shouldSuppressRFS
			// notify RFQ service
			RequestHandler::instance().onNewRequest<...>(tpsRequest, this, m_RFQDispatcher)

		if (isRFS)
			if(!isCreditOkay)
				subscription->switchToManualQuoting("...", FXOutrightQuote::E_CREDIT_FAILURE_DEPRECATED, code);
			else if(forceManualQuoting)
				// Credit check passed but NDF fixing date is invalid.
				subscription->switchToManualQuoting("...", FXOutrightQuote::E_PRICE_MANUALLY, code);

### Subscribe for Quote Block

	// this is in file SubscribeForQuotesBlock.cpp
	// SAME -> same as subscribeForQuotes
	void BANASessionQuoteManager::subscribeForQuotesBlock(principal, request) {
		check if pricing group by QuoteRequestAccessChecker // SAME

		FXOutrightRequest::PtrList tpsRequestList
		FXOutrightRequest::Ptr tpsRequest

		createFXQuoteRequest(tpsRequest, principal, request...) // SAME
		tpsRequestList.push_back(tpsRequest) 
		Subscription::Ptr subscription = new Subscription(...) //SAME
		subscription.quoteThrottle = getQuoteThrottle(...)
		...
		subscriptionResult = onSubscriptionStart(subscription) // SAME
		register permission revokation listeners


		if FXCurrencyUtils::isCurrencyPairSpecial(*request->getCcyPair() && 
			/ENVIRONMENT/bana_fix_gateway[NDFROEBlockQuotingEanbled] == false
			reject // shouldn't this be done earlier ?

		FXInstrumentLegs iLegs = request->getBulkLegs()
		deque<FXBlockLegRequest> fxLegs

		map<string, FXBlockLegRequest> blrMap
		int requestId = tpsRequest.getRequestId().getRequest() + 1

		for (each leg in iLegs) {
			tenor <- leg.tenor
			account <- leg.account
			FXBlockLegRequest blr <- look up by tenor
			if not found
				blr <- new FXBlockLegRequest(requestId++)
				insert (tenor, blr) to blrMap
			FXSpotForwardTradeAllocation alloc
			alloc.setQuantityCcyUsed(qtyCcy)
			alloc.setClientId(leg.getRefID())
			FXSettlementAccount accoutPtr = FXSettlementUtils::getAccountByChannelRef(
				tpsRequest.getGroupId(), leg.getAccount())
			if (accountPtr == NULL) fail // shouldn't this be done earlier ?
			alloc.setSettlementAcct(accountPtr)

			blr.setSideType(...)
			blr.setVolume(...)
			blr.setValueDate(...)
			blr.getAllocations().push_back(alloc)
		}

		push all FXBlockLegRequest in blrMap into fxLegs

		FXBlockRequest::Ptr blockPtr = new FXBlockRequest(
			...
			FXBlockRequest::SALES_RATE, 
			fxLegs,
			true /* in competition */
			tspRequest->getSettlementAcct())

		// shall we do pre-trade credit checking ?
		if (CreditCheckHelper::doCreditCheckOnQuote(...)
			CreditCheckHelper::instance().check(*blockPtr)
			ccResponseCode = ...

		if isCreditOkay == CC_NO_ACCOUNT
			sendQuoteRejectAndEndSubscription(...) return

		// Notify RFQ service for new RFS stream
		RequestHandler::instance().onNewRequest ...

		// each subscription has its own block quote manager
		subscription.getBlockQuoteManager()->settParentSide(...)

		// get snapshot
		pair<FXOutrightQuote::Error, string> snapshotResult
			= getLegSnapshots(principal, request, subscription->getBlockQuoteManager(), isCcy1Primary)
		if got snapshot
			subscription->getBlockQuoteManager()->setLegSnapshotReady()

		subscription->start(tpsRequestList, isCreditOkay == CC_PASS)

		// run validator again in case permission has been revoked
		if revoked
			reject and end subs

		if isCreditOkay != CC_PASS // credit check failed
			subscription->switchToManualQuoting(...)
		else
			// credit checking is okay, do we have snapshot
			if first snapshot failed
				subscription->switchToManualQuoting(...)
	}

	pair<FXOutrightQuote::Error, Nullable<string> > BANASessionQuoteManager::getLegSnapshots(
		principal, FXQuoteRequest request, BlockQuoteManager bqm, isBaseCurrency) {

		start <- apr_time_now()
		PointInfoMap& pointInfoMap = bqm.getPointInfoMap()

		// populate point map, aggregate legs of same tenor
		for each leg in request legs
			PointInfo pointInfo
			if leg->tenor not in pointInfoMap, add (tenor, new PointInfo)
			++pointInfo.legCount

			if leg->getOrderQty != FXCurrencyUtils::roundCurrencyValue(leg->getOrderQty(), request->getValueCurrency)
				reject quote since volume is subject to rounding /// !!!

			increate/decreasae pointInfo.volume based on leg side
			pointInfo.fxInstrumentLeg <- leg

		// request for snapshot prices for all the tenors
		uint64 tmout1 <- m_session.getThrottleManager.getBlockQuoteAutoTimeout
		uint64 tmout2 <- /bana_fix_gateway/BlockQuote[MaxTimeToWaitPerLeg], 3000

		foreach pointInfo in pointInfoMap
			if now - start > tmout1: fail
			FXOutrightRequest req
			leg <- pointInfo.fxInstrumentLeg
			createFXQuoteRequest(req, principal, request, leg, pointInfo)
			pointInfo.quote = getSnapshot(req, tmout) {
				m_tpsClient->getSnapshot(req, tmout)
			}
			if failed
				set state, reason and break
			// set forward points if necessary
			spotDate <- request.getCurrentSpotDate()
			valueDate <- leg.getSettleDate()
			if spotDate != valueDate {
				FXOutrightRate salesRate <- pointInfo.quote.getSalesRate()
				if salesRate.getOutrightBidRate && salesRate.getSpotBidRate
					salesRate.setFwdBitPts(
						FXPricingUtils::calculateMathPoints(
							request.getCcyPair,
							salesRate.getOutrightBidRate,
							salesRate.getSpotBidRate
							)
						)
				if salesRate.getOutrightAskRate && salesRate.getSpotAskRate
					salesRate.setFwdBitPts(
						FXPricingUtils::calculateMathPoints(
							request.getCcyPair,
							salesRate.getOutrightAskRate,
							salesRate.getSpotAskRate
							)
						)
			}

		foreach leg in request legs
			find point info based on tenor
			FXOutrightQuote quote <- new FXOutrightQuote(pointInfo.quote)
			if quote.legCout > 1
				set forward points accordingly
			bqm->updateLegSnapshot(leg->getRefID, quote)
	}

## Quotes from TPS

	// tpsClient registered this with rv as message handler
	BANASessionQuoteManager::onQuote(const FXRequestId& requestId,
									 const FXOutrightQuote& quote)
		handleQuote( requestId, modifiedQuote);

	void BANASessionQuoteManager::handleQuote(FXRequestId & requestId,
										   const QuoteType& constQuote,
										   const int32 timeToLive )
		// requestMap[requestId] -> Subscription
		Subscription::Ptr subscription = getSubscription(requestId.getRequest());
		subscription->handleQuote(constQuote, timeToLive);


	void Subscription::handleQuote(const FXOutrightQuote & inQuote, const int32 timeToLive )
		if (isEnabled)
			if(isBlock)
				// The block manager will call the handleQuoteHelper for a block quote if necessary.
				getBlockQuoteManager()->handleQuote(inQuote, timeToLive);
			else
				handleQuoteHelper(inQuote, timeToLive, resend_reschedule_flag, message, quoteId, ignoreQuoteThrottle);			
		else
			update latestOutrightQuote and latestOutrightQuoteTimeToLive

		if (resend_reschedule_flag)
			handleQuoteThrottle(inQuote, timeToLive, resend_reschedule_flag, message, quoteId, ignoreQuoteThrottle);

	void Subscription::handleQuoteHelper(quote, ...)
		// special handlling of the 1st quote
		if !isQuoted
			if (quoteType == TRADEABLE || quoteType == NEGOTIATED) && reasonCode == OK
				isQuoted = true
				stopTSPValidQuoteWaitTimer()
			else 
				setLastBadQuote(quote)
				...
				return

		if !valid return // ignore
		if isManualRequestSent && quoteType != NEGOTIATED return // ignore
		// check quote id sequence order
		if lastTpsQuoteId > quote.getQuoteId() return // igore

		setLastTpsQuoteId(quote.getQuoteId())

		if needToResendQuoteTimerTask != NULL cancel it
		if m_rfqTerminationTimerTask != NULL cancel it
		noQuotesReceived = false // diff between this and 

		// check if the last quote is stale using FXQuoteStoreManager
		bool wasStale = quoteStoreManager.template wasLastQuoteStale<QuoteType>(
			session.getClientPantherId(), tpsRequestId.getRequest());
		// check if its valid
		bool isQuoteValid = checkQuoteValidity(quote);

		// check if expired (using quote.getOriginSysTime) and calc TTL
		bool quoteAlreadyExp = checkIfQuoteExpAndCalcTTL(...);

		if !isQuoteInvalid
			quoteId = quoteStoreManager.storeQuote(request, tpsRequestId, quote, 
				session.getClientPantherId(), tpsRequestId.getRequest(),
				throttleManagerUtils.getStaleQuoteTtl(session,timeToLive), 
				calculatedTTL, isInTermsOfBase, streamType);

		if has quote error or stale
			switch quoteError
				... // mostly reject, but may also switch to manual quoting

		cancel initial quote timeout timer
		// create fix quote message from quote
		FixDialectMessageHelper.populateMessageFromQuote(message, quote...)

		if it should be sent immediately
			sendConsolidatedQuote(message, quoteId, timeToLive);
		else 
			// see above, this will lead to handleQuote to call handleQuoteThrottle on
			// the quote, bad design
			resend_reschedule_flag = true;

		if RFS and first quote
			RequestHandler::instance().onQuoted(tpsRequestId, convert(inQuote));
			schedule sendQuoteUpdateToBlotter // why only the first ?



	void Subscription::sendConsolidatedQuote(QuoteType& message, std::string & quoteId, int32 timeToLive)
		session.sendConsolidatedMessage(message, *request->getRequestId());
		localQuoteLifecycleManager.sendLifeCycleEventUpdateOnQuote(message, lastQuoteId);


	libs/ext/fix/protocol/Session.cpp
	void Session::sendConsolidatedMessage(FixMessage_ptr &message, string &key)
		message->setSendingTime(now + m_timeOffset);
		ml::ext::fix::Message_ptr messageString = m_dialect.toMessageString(*message);
		// remember m_queue is basically EngineConnection's outgoing message queue
		m_queue->put(message->getTarget(), messageString, key);

What is the difference between stale and expired ?


################################################################################

# Block

## Manual quoting

	void Subscription::switchToManualQuoting(QuoteType quote, CreditCheckResponseCode respCode)
		bool isBlockManualQuotingEnabled = RegistryHelper::getRegistryBool(
			"/ENVIRONMENT/bana_fix_gateway", "manualBlockQuotingEnabled", true);

		// until manual quoting of block trades is fully supported, send a reject
		if (isBlock && !isBlockManualQuotingEnabled) ...

		// we might have already sent a request for this
		if (!isManualRequestSent)
			// if credit check failure the sales person may override the quote.
			// in this case we dont unsubscribe from TPS but just disable quoting
			// so that new quote is not sent to client
			if(!quote.isNull() && respCode != ml::spec::panther::credit::CreditCheckResponseCode::SUCCESS)
				disableQuoting("Credit Check Failure, Sales person may override", -1);
			else
				unsubscribeFromTps();

			RequestHandler::instance().createRFQ(tpsRequestId, convert(tempQuote));
			isManualRequestSent = true;

		if (m_blotterQuoteUpdateSenderTask)
			m_blotterQuoteUpdateSenderTask->cancel();


### Send request to RFQ server:

	libs/fx/requestmanagement/RequestHandler.hpp

	class RequestHandler {
		typedef mlc::concurrent::AsyncFlow< RequestItem::Ptr >  RequestItemProcessorFlow;
		RequestItemProcessorFlow m_requestItemFlow;

		// On receiving a E_PRICE_MANUALLY from tps, call this method
		// This method will ensure you get a quoteCancel so that previous quotes can be withdrawn(if any)
		// This will also ensure the request is sent to RFQ Server
		void createRFQ(FXRequestId &reqId, QuoteType quote)
			m_requestItemFlow.put(new CreateRFQRequestItem<QuoteType>(reqId, quote));

	RequestHandler::RequestHandler()
	: m_requestItemFlow(boost::bind(&RequestHandler::processRequestItem, this, _1))
		m_requestItemFlow.start();

	void RequestHandler::processRequestItem(const RequestItem::Ptr &item)
		item->handle();

The actual work is done in:

	libs/fx/requestmanagement/item/RequestItem.hpp

	template<typename QuoteType>
	struct CreateRFQRequestItem : public RequestItem
		FXRequestId reqId;
		QuoteType quote;

		void handle()
			const ClientRequestItem::Ptr item = ClientRequestCache::instance().get(reqId);
			if(item)
				item->handler->onQuoteCancel(reqId);
				if(!quote.isNull() && isCreditCheckFailure<QuoteType>(quote->getReasonCode()))
					item->waitingForCreditOverrideStatus = true;
					CreditOverrideHandler::instance().handleCreditCheckFailure(reqId...);
				else
					bool rc = RFQSenderInstance::instance().getRFQServerSenderInstance()->createNewRFQ(reqId, quote);
					if (!rc) 
						item->quoteRequestType = QuoteRequest::RFQ;
						RequestBlotterUtil::onNewRFQ(reqId);
						CreditOverrideHandler::instance().requestEnded(reqId);

## Quote Cancel




###################################################################################
# Quote Store

## FXQuoteStore

Code in `FXQuoteStore.hcpp`. Important members

	struct Quote {
		string quoteId  // FIX quote id ?
		FXQuoteRequest request
		FXRequestId requestId // TPS request id
		quote
		Date quoteDate
		Time staleTime
		int64 quoteInstance
		StreamType streamType
	};

	std::string storeId
	uint64 m_uniqueId // internal sequence number
	FXPositionTracer m_positionTracker
	map<string, deque<Quote>> m_quoteMap

	FXQuoteStore::storeQuote(...) 
		clientId = (userPantherFixID ++ tpsRequestId)
		quoteId = (m_uniqueId ++ quote.getSourceTickId() ++ quote.getQuoteId() ++ userPantherFixID ++ tpsRequestId ++ storeId)
		history = m_quoteMap[clientId]
		if (!history.empty()) 
			history.back().staleTime = now + Time(staleTtl * 1000) // notice this
			cleanupStaleQuotes(history)
		get current trading day from FXCalendar for the ccyPair
		calc quote stale time for the quote
		history.push_back(new Quote(...))


From `FXOutrightQuote` contract:

	// This id will be unique to a particular subscription and tps instance
	quoteId int64;

	// reference id for the source data that generated this quote. May be zero.
	sourceTickId int64;		


###################################################################################

# Order Manager

Implements `FXTradeBroadcastContractStub` to receive trades:

	class BANASessionOrderManager : public SessionOrderManager
	class SessionOrderManager : public FXTradeBroadcastContractStub

## New Order Single

	BANAIncoming::processFXNewOrderSingle
		// check OrdType
		if (orderType == OrdTypeConsts::FOREX_MARKET || orderType == OrdTypeConsts::MARKET)
			...
		else if orderType == OrdTypeConsts::PREVIOUSLY_QUOTED
			...
		// swap
		if (OrderQty2 != NULL)
			...

	void BANASession::handleOrderRequest(const FixMessage_ptr & message)
		m_orderManager.handleNewOrderSingle(FXNewOrderSingle::fromMessage(message));

	void BANASessionOrderManager::handleNewOrderSingle(const FXNewOrderSingle_ptr &orderRequest)
		// find the corresponding quote from quote store manager (via the session object)
		const Status quoteStatus = lookupQuote(m_session, quoteId, quoteRequest, tpsRequestId, 
			quoteDate, outrightQuote, swapQuote, streamType);
		// add the stream request type to the sender's FixParty in order to look up the correct user
		enrichSenderWithStreamType(orderRequest, streamType);

		// check user
		// check group
		// notify RFQ server
		// send lifecycle event
		// check ClientOrderID <= 64

		// check if it is market swap
		// farVolume -> swap
		// quoteId -> market
		if (farVolume notnull && quoteId isnull) {
			// check /PROCESS/allow_market_swap_orders
		}

		if (farVolume notnull)
			handleNewSwapOrderSingle(...)

		if (orderType in [LIMIT, STOP, STOP_LIMIT])
			// reject

		handleNewOutrightOrderSingle(...)

	// there are two main tasks other than validation:
	// 1. create the trade instruction
	// 2. create the allocation list
	void BANASessionOrderManager::handleNewOutrightOrderSingle(
		const FXNewOrderSingle_ptr &request,	// (in) order request
		Nullable<std::string> & quoteId,		// (in) quote id
		FXQuoteRequest_ptr & quoteRequest,		// (in) original quote request, null for market order
		FXRequestId & tpsRequest,				// (in) corresponding TPS request for the quote
		Date & quoteDate,						// (in) quote date
		FXOutrightQuote::Ptr & quote,			// (in) quote
		const Status quoteStatus)				// (in) returned by lookupQuote above
	{
		boost::shared_ptr<ml::spec::fx::trade::FXSpotForwardIns>  tradeInstruction;

		// check order type has to be in [MARKET, FOREX_MARKET, PREVIOUSLY_QUOTED, FOREX_PREVIOUSLY_QUOTED]
		// if market order, check if is allowed

		// check if this is a negotiated quote
		if not market order && quoteId isnull
			if quoteId == "NIL"
				dummyQuoteNeeded = true
			if quoteId == "PLROLL"
				dummyQuoteNeeded = true
				clientManaged = true

		// access control
		if (!AccessControlUtils::validUser(principalPtr)) ...

		// if quoteStatus not OK, check reason and reject

		// set value date and pmLocation

		// here a diff logic is used to check if it's market order
		// if no original quoteRequest, then market order

		if(quoteRequest) {
			valueDate = quoteRequest->getTenor();
			pmLocation = quoteRequest->getPMSettlementLocation();
		} else {
			// is market order
			valueDate = request->getSettlementDate();
			pmLocation = request->getPMSettlementLocation();
		}

		// validate and convert special product
		if(!validateAndConvertSpecialProduct(
			groupId, securityType, symbol, settlCurrency, valueDate,
			isNDF, isROE, isPM, isOST, isNDFError, isROEError, isPMError, isOSTError,
				prodType, // isSwapRequest=false: since this is called in handle outright quote method
				errorMessageNDFROEconversion,
				pmLocation
			)) { ... }


		if isNDF {
			// check, convert and set fixing date
			// set fixing source
		}

		// symbol conversion for precious metals
		if (oldSymbol != symbol && isPM)
			request->setPMSettlementLocation(pmLocation)

		if (dummyQuoteNeeded) {
			bool byPassTPS = clientManaged
			quote = createDummyQuote(...)
			request.setQuoteId(quote.getQuoteId())
		} else if (marketOrder) {
			if (!validateTrading(...)) return
			// fill in some dummy data
			quote.reset(new FXOutrightQuote())
			tpsRequest <- FXRequestId(principalId, 0)
		} else {
			// if quoteStatus not OK, reject
			// if quoteType not in [TRADEABLE, NEGOTIATED], reject
			if (!validateTradingAgainstQuote(request, principal, ...)) return [
				// in FixUtils.hpp
				validUser()
				validateTrading(...)
				validateObjectForOrder(...)
				validatePermission(...)
			]

			if (!shareStreamValidation(...)) return [
				// in FixUtils.hpp

				// quote group is proxy if is pricing group but not trading group
				bool isQuoteGroupProxyAccount = quoteGroup->getIsPricingGroup() && ! quoteGroup->getIsTradingGroup();
				if( isQuoteGroupProxyAccount ) {
					// this is for shared stream
					// check if quoteGroup is pricing group or trading group
					bool isTradeGroupRealAccount = ! orderGroup->getIsPricingGroup() && orderGroup->getIsTradingGroup();
					if ( ! isTradeGroupRealAccount ) 
						reject
				} else {
					// not shared, order group and quote group must agree with each other
					if (orderGroup != quoteGroup)
						reject
				}
			]

			// check symbols and dates 
		}

		if (marketOrder)
			// check if settlementDate and tenor match

		volumeCurrency = ...
		isInTermsOfCcy1 = ...
		isBuyTrade = ...
		volume = ...
		direction = ... // "buy" or "sell"

		minBand = quote.getMinVolume()
		maxBand = quote.getMaxVolume()

		...

		FXSettlementAccount::Ptr settlementAccount;
			if (request->getAccount().isNull()) 
				if (*isPM)
					settlementAccount = FXSettlementUtils::getDefaultAccount(groupId, FXSettlementAccount::PM);
				else
					settlementAccount = FXSettlementUtils::getDefaultAccount(groupId);
			else
				if (*isPM)
					settlementAccount = FXSettlementUtils::getAccountByChannelRef(groupId, *request->getAccount(), FXSettlementAccount::PM);
				else
					settlementAccount = FXSettlementUtils::getAccountByChannelRef(groupId, *request->getAccount());

		// throttling
		bool applyThrotlles = true;
		// if manual or negotiated, no throttling
		if(quote.get() != NULL && (quote->getType() == FXOutrightQuote::MANUAL || quote->getType() == FXOutrightQuote::NEGOTIATED))
			applyThrotlles = false;

		if (applyThrotlles) 
			if (m_throttleManager.hasOrderVolume()) 
				...

		// try to get trader account
		if (!request->getTraderSource().isNull())
			traderAccount <- FixTraderAccount[traderSource]

		orderTimeout = ...

		// check credit fail override
		Subscription::Ptr subscription = m_banaQuoteMgr.getSubscription(tpsRequest.getRequest());
		if(subscription && subscription->isCreditFailOverriden && !subscription->isManuallyPriced())
			quote->setType(FXOutrightQuote::CREDIT_FAIL_OVERRIDEN);

		// create the trade instruction object
		tradeInstruction.reset(
			new FXSpotForwardIns(
					tpsRequest,                                                // user id & request
					groupId,                                                   // group id
					pricingGroupId == -1 ? Nullable<int64>() : pricingGroupId, // pricing group id
					*request->getClientOrderId(),                              // client id
					now,                                                       // enter time
					instrument.toString(),                                     // ccy pair
					tradeDate,                                                 // trade date
					direction,                                                 // direction of ccy 1
					*settlementAccount,                                        // settlement account
					quantityCcy1,                                              // qty ccy
					...);

		// now, allocations
		FXSpotForwardTradeAllocation::PtrList allocList;
		const OrderAllocations & allocsFromReq = request->getAllocations();
		foreach allocIt in allocsFromReq
			...
			FXSpotForwardTradeAllocation::Ptr allocPtr( new FXSpotForwardTradeAllocation(
					*allocAccount, (*allocIt).getIndividualId(), allocQuantityCcy1, allocQuantityCcy2,
					volumeCurrency, directionAlloc, userRef1, userRef2, userRef3, *usiFields));
			allocList.push_back(allocPtr);

		// set quantities
		if( allocList.size() != 0)
			if( isInTermsOfCcy1 )
				tradeInstruction->setQuantityCcy2(abs(calcCcyAmount));
			else 
				tradeInstruction->setQuantityCcy1(abs(calcCcyAmount));

		// position tracking, only if not market order
		if (!marketOrder) 
			// 1.
			buySell = ...
			multiplier = ...
			limit = ...
			m_positionTracker.addQuote(...)
			if (!m_positionTracker.addOrder(...))
				// order exceeds liquidity
				reject
			...
			// 2. 
			if (!m_bigOrderTracker.add(...))
				// big order limit exceeded
				reject
			
		// finally, send it
		asyncSendOrderOutright(request, tradeInstruction, allocList, marketOrder);


	void BANASessionOrderManager::asyncSendOrderOutright(
			const FXNewOrderSingle_ptr  &request,
			const ml::spec::fx::trade::FXSpotForwardIns::Ptr &tradeInstruction,
			const ml::spec::fx::trade::FXSpotForwardTradeAllocation::PtrList& allocs,
			bool marketOrder)
	{
		// calc timeout value
		Time timeout = tradeInstruction->getEnteredTime()->time();
		timeout += m_throttleManagerUtils.getOrderTimeout(m_session) * (Time::USECS_PER_SECOND / 1000);

		// schedule the timeout task and add the trade to awaiting trades

		// for market orders, since they are not tracked in position tracker,
		// this is the only place they are tracked
		TimerTask_ptr timeoutTask(new TimerTask(boost::bind(&BANASessionOrderManager::asyncSendOrderTimeout...));

		AwaitingTrade trade = { request, timeoutTask, false };
		orderInsertResult = m_asyncAwaitingTrades.insert(... trade);

		bool isPreAlloc = (allocs.size() > 0);
		FXTradeContractProxy proxy(*m_das.getDispatcher());

		// update liquidity, this also goes to trade server which is liquidity provider
		// see fx_trade_server/contracts/liquidity

		// 1. check table Configuration[keyName='TradeServerLiqViolationCheckEnabled'] to see if it's enabled
		// 2. check FXGroup[liquidityProfileId] notnull || PantherGroup[channelId] -> FXChannel[liquidityProfileId] notnull
		if (ml::fx::tools::FXLiquidityViolationUtils::doSendLiquidityUpdate(tradeInstruction->getGroupId()))
			FXLiquidityPoolContractProxy proxyLiquidityPool(m_LiquidityUpdateDispatcher);
			FXLiquidityUpdate liqUpdate;
			ml::fx::tools::FXLiquidityViolationUtils::getFXLiquidityUpdateFromTrade(*tradeInstruction, liqUpdate, marketOrder);
			proxyLiquidityPool.updateLiquidityPool(liqUpdate);

		// now send the trade instruction to the trade server
		if (marketOrder)
			if(isPreAlloc)
				proxy.bookSplitAtMarketSpotForwardAsync(
						*tradeInstruction,
						TenorNameChanges::translateTenorName(*request->getSettlementDate()),
						allocs);
			else
				MarketOrderVWAPManager& vwap_mngr = MarketOrderVWAPManager::instance();
				std::string ccyUsed = tradeInstruction->getQuantityCcyUsed().str();
				Decimal volume(
						vwap_mngr.incVolume(tradeInstruction->getGroupId(),
											tradeInstruction->getCcyPair().str(),
											tradeInstruction->getDirection().str(),
											ccyUsed,
											(ccyUsed == "1") ? tradeInstruction->getQuantityCcy1()
															 : tradeInstruction->getQuantityCcy2()));
				tradeInstruction->setVwapVolume(volume);
				std::string tenor = TenorNameChanges::translateTenorName(*request->getSettlementDate());
				proxy.bookAtMarketSpotForwardAsync(*tradeInstruction, tenor);
		else
			if(isPreAlloc)
				proxy.bookSplitSpotForwardTradesAsync(*tradeInstruction, allocs);
			else
				proxy.bookSpotForwardAsync(*tradeInstruction);

		m_asyncTimeoutTimer.schedule(timeoutTask);
	}


Notes:
- Position tracking is not performed for market orders.

## Response from trade server

	contract FXTradeBroadcastContract version 1.0
		void onSpotForward(FXSpotForwardTrade trade, list<FXSpotForwardTrade> splitTrades);
		void onSwapTrade(FXSwapTrade trade, list<FXSwapTrade> splitTrades);
		void onSwapTradeWithSpotForwardAllocations(FXSwapTrade trade, list<FXSwapTrade> splitSwapTrades,
			list<FXSpotForwardTrade> splitSpotForwardTrades);


	void BANASessionOrderManager::onSpotForward(FXSpotForwardTrade& trade, FXSpotForwardTrade::PtrList& splits)
		// flow management
		RequestHandler::instance().onTrade(trade);

		// handle it only if it is executed, failed and not modified by bana_fix_gateway
		if ((trade.getState() != FXTradeState::EXECUTED &&
			trade.getState() != FXTradeState::FAILED) ||
			trade.getModifiedBy() == BlotterNotification::BFGW_MODIFIED_BY)
			return;

		ml::fx::fix_dialect::message::FXNewOrderSingle_ptr request;
		bool tradeRejectionWasSent = false;

		// clear it from awaiting trades
		AsyncAwaitingTradesMap::iterator finder = m_asyncAwaitingTrades.find(...);
		if (finder != m_asyncAwaitingTrades.end())
			request = finder->second.request;
			finder->second.timeoutTimer->cancel();
			tradeRejectionWasSent = finder->second.rejected;

			// if market order, update position
			if(trade.getOrderType() == FilledOrderType::MKT)
				MarketOrderVWAPManager& vwap_mngr = MarketOrderVWAPManager::instance();
				vwap_mngr.decVolume(...);

			m_asyncAwaitingTrades.erase(finder);

		// check for expiration error, (is this quote expiration ? market orders not tracked by m_positionTracker)
		if(trade.getState() == FXTradeState::FAILED && trade.getErrorNumber() == FXSpotForwardTrade::E_TIME_EXPIRED_ERROR)
			m_positionTracker.rejectExecution(trade.getClientId().str());
			return;

		// this is a very important case !!!

		// we have already sent a reject back to the client due to the timeout threshold being met
		// but the trade_server did not reject it!
		if (request && tradeRejectionWasSent)
			if (trade.getState() != FXTradeState::FAILED)
				GLCRIT(TIMEOUT_GROUP, "OR[%s] : Outright trade was rejected to client due to timeout but trade PAN-%lld-C is STILL ACTIVE in system. ACTION REQUIRED: PAN-%lld-C needs to be manually unwound. ", trade.getClientId().c_str(), trade.getId(), trade.getId());
			else
				LINFO("OR[%s] : Finally received failed trade from fx_trade_server, already sent failure to client.", trade.getClientId().c_str());
			return;

		if (request)
			int status = trade.getErrorNumber();
			std::string reason = trade.getReason().str();
			handleExecutionReportSpotForwardNew(request, trade, splits, status, reason);
		// Else trade belongs to someone else, ignore it

`SessionOrderManager_NewOrderRequest.cpp`:

	void BANASessionOrderManager::handleExecutionReportSpotForwardNew(
			const FXNewOrderSingle_ptr &request,
			const FXSpotForwardTrade &trade,
			const FXSpotForwardTrade::PtrList& splits,
			int status,
			const std::string &reason)
	{
		// If its a hit on RFS close the RFS subscription first
		// EPS subscriptions are closed by explicit QuoteRequest
		if (!request->getQuoteId().isNull())
			endRFSSubscription(*request->getQuoteId(), banagw::Outright);

		FXOrderExecutionReport_ptr message(...);
		// populate message ...
		if (trade.getState() == FXTradeState::FAILED)
			m_positionTracker.rejectExecution(request.clientOrderId);
			// ...
		else
			m_positionTracker.addExecution(request.clientOrderId);
			//...
		m_session.sendMessage(message);
		sendLifeCycleEventUpdateOnExecutionReport(request, message);

##########################################################################

# Quote store manager

`libs/ext/fix/protocol/FXQuoteStoreManager`

	FXPositionTracker& m_positionTracker;
	FXQuoteStore<FXOutrightQuote> m_outrightQuoteStore;
	FXQuoteStore<FXSwapQuote> m_swapQuoteStore;
	FXQuoteStore<FXBlockQuote> m_blockQuoteStore;

	Status getQuote(FXQuoteRequest_ptr &,FXRequestId &,FXOutrightQuote::Ptr &, ...
	Status getQuote(FXQuoteRequest_ptr &,FXRequestId &,FXSwapQuote::Ptr &, ...

	// note these calls are not type specific and are implemented
	// by calling remove on all 3 stores, slow

	void removeQuote(const int64 fixId, int64 reqId, uint32 msTimeout);
	void staleQuote(const int64 fixId, int64 reqId);

`libs/ext/fix/protocol/FXQuoteStore`

	struct Quote {
		std::string quoteId;
		ml::fx::fix_dialect::message::FXQuoteRequest_ptr request;
		ml::spec::fx::tps::quote::FXRequestId requestId; // tps request
		T_Ptr quote;
		ml::common::util::Date quoteDate;
		ml::common::util::Time staleTime;
		ml::ext::fix::StreamType streamType;
	};
	
	// note history is saved in deque
	typedef typename std::deque<Quote> QuoteHistory;
	typedef typename std::map<std::string, QuoteHistory> QuoteMap; // keyed by (fixId, tpsReqId)

	QuoteMap m_quoteMap;
	
	ml::common::util::Timer m_removalTimer;
	uint64 m_removalDelay;
	uint64 m_uniqueId;
	FXPositionTracker& m_positionTracker;

	std::string storeId;

	// search history (deque), slow O(N)
	Status getQuote(
		ml::fx::fix_dialect::message::FXQuoteRequest_ptr &,
		ml::spec::fx::tps::quote::FXRequestId &,
		T_Ptr &,
		ml::common::util::Date &,
		const std::string &quoteId,
		ml::ext::fix::StreamType &streamType) const;

	// this is fast, O(1)
	void getLastQuote(const int64 fixId, int64 reqId, T_Ptr &) const;

	// put the quote in quote history and generate quote id for end user
	std::string storeQuote(
		const ml::fx::fix_dialect::message::FXQuoteRequest_ptr &,
		const ml::spec::fx::tps::quote::FXRequestId &,
		const T_Ptr &,
		const int64 fixId, int64 reqId,
		const uint32 staleTtl, const uint32 validUntilTime,
		const ml::ext::fix::StreamType streamType) {

		sets staling time
		clear staled quotes from quoteHistory // !!!
	}

###################################################################################

# Schedule Manager

	class BANASessionScheduleManager : public SessionScheduleManager
	class SessionScheduleManager
		virtual void onDayRoll(DayRoll& dayroll, DayRollData&  newTradingDay) = 0;


###################################################################################

# Option Manager



###################################################################################

# Subscription

###################################################################################

# Request Flow Management 

ag 'RequestHandler::instance'


###################################################################################

# TPS Client

It provides two sets of APIs:

* Subscription
* Server stub to receive quotes

for both outright and swap:

	class FXSwapCallback : public ml::spec::fx::tps::quote::FXSwapListenerStub
		virtual void onQuote(FXRequestId& requestId, FXSwapQuote& quote) = 0;

		void subscribe(FXSwapRequest::PtrList& requests);
		void unsubscribe(FXRequestId::PtrList& requestIds);
		void unsubscribeAll();

	class FXOutrightCallback : public ml::spec::fx::tps::quote::FXOutrightListenerStub
		virtual void onQuote(FXRequestId& requestId, FXOutrightQuote& quote) = 0;

		void subscribe(FXOutrightRequest::PtrList& requests);	
		void unsubscribe(FXRequestId::PtrList& requestIds);
		void unsubscribeAll();

	class TpsClient : public FXOutrightCallback, public FXSwapCallback {
		FXOutrightQuote::Ptr getSnapshot(FXOutrightRequest::Ptr& request, uint32 timeout);
		FXSwapQuote::Ptr getSnapshot(FXSwapRequest::Ptr& request, uint32 timeout);

		TpsServerInfo::Ptr getActiveServer(FXOutrightRequest & request) const;
		TpsServerInfo::Ptr getActiveServer(FXSwapRequest & request) const;

		void onFXGroupChange(FXGroup::Cache & cache, FXGroup::Cache::Event & evt);
		void onConfigurationChange(Configuration::Cache & cache, Configuration::Cache::Event & evt);

To subscribe:

	void TpsClient::subscribe(const FXOutrightRequest::PtrList& requests)
		excludeFromAntiSweep(requests);
		applyAntiSweepVolumes (m_requests, requests, dirtyRequests, false, m_excludeFromAntiSweep);

		foreach(request, requests)
			const FXRequestId & requestId = (*request)->getRequestId();
			m_requests[requestId] = *request;

		//dirtRequests contains all requests (new + antisweep) that needs to be subscribed
		FXOutrightCallback::subscribe(dirtyRequests);

	void FXOutrightCallback::subscribe(const FXOutrightRequest::PtrList& requests)
		TpsServerInfo::Ptr const &serverInfo = TpsConnectionManager::instance().getTpsServerInfo();
		FXOutrightProviderProxy proxy(m_dispatcher, 
			serverInfo->m_outrightProviderKey, serverInfo->m_outrightProviderTopic);
		proxy.subscribe( m_subscriberId, requests, m_topic, apr_time_now() );

To handle incoming quotes:

	void TpsClient::onQuote(const FXRequestId& requestId, const FXOutrightQuote& quote)
		TpsConnectionManager::instance().onMessage();

		int64 &prevQuoteId = m_quotes[ requestId ];
		if( prevQuoteId > quote.getQuoteId() )
			return;

		prevQuoteId = quote.getQuoteId();
		FXOutrightCallback::m_callback.onQuote(requestId, quote);

To trace TpsClient creation:

	grep "TpsClient::TpsClient()"


To trace connection

	grep 'TpsClient::onTpsConnect' | grep 'from instance'
	grep 'TpsClient::onTpsDisconnect' | grep 'from instance'

To trace subscription:

	grep 'OUTRIGHT subscription request received'
	grep 'New Outright Subscription request'
	grep 'Subscribing outright request'
	grep 'Unsubscribing outright request'

	grep 'SWAP subscription request received'
	grep 'Subscribing swap request'
	grep 'Unsubscribing swap request'

To trace quotes:

	grep 'Received quote' | grep 'for quote outright request'
	grep 'Received quote' | grep 'for quote request'



###################################################################################

# Message Processing

The general pipeline in `EngineConnection::taskReadMessage` is:

- `FXDialectProvider::fromMessageString`
	- `BANAIncoming::process...`
- `BANASession::receiveMessage`
	- `BANASession::handleMessageType`

## Logon

- `BANAIncoming::processLogon`: check for field `HeartBtInt`
- `BANASession::processLogonMessage`:
	// if any of the checking fails, send a logout to cameron
	check fix version (as in FixApplication)
	check if already logged on
	check compId,subId,locId
	check if its FixID is valid
	validate panther user (user id is ExternalEntity::principalId/PantherUser::id)
	validate panther group
	GatewayMain::instance().loginFix() // do login with login server
	Sleep for 1 sec /// !!! BAD
- `BANASession::handleMessageByType`:
	connect() {
		m_orderManager.disconnect()
	}


## Logout

- `BANAIncoming::processLogout`: nothing
- `BNANSession::handleMessageByType`:
		disconnect() {
			m_queue->clear() // clear cameron outgoing queue
			1. GatewayMain::instance().clientLoggedOff(...)
			2. send logout to cameron // note this, not the same for logon
			3. AccessControl::instance().logOut(...)

			m_quoteManager.disconnect()
			m_scheduleManager.disconnect()
			m_orderManager.disconnect()
		}

## Heartbeat

- `BANASession::receiveMessage`: ignore the message

## Reject

## Quote Request

- `BANAIncoming::processFXQuoteRequest`
		set QuoteReqID, NoRelatedSym, Symbo
		// check if its block quote request
		if NoLegs > 0
			create an FXInstrumentLeg for each leg
			set side
			set tenor and do tenor name translation
			set settlement date
			validate tenor against settlement date if both are present
			validate tenor
			validate and convert settlement date

		QuoteRequestType (can be CANCEL, FOREVER(for ESP) or RFS)
		process currency pair
		process OrderQty, TenorValue, FutSettDate
		set valueCurrency of message to value of Currency
		for swap, process OrderQty2, TenorValue2, FutSettDate2
		for NDF, process MaturityDate, FixingDate, FarLegFixingDate, SettlCurrency, SecurityType
		for PM (precious metal), set PMSettlementLocation

- `BANASession::handleQuoteRequest`:
		m_quoteManager.handleQuoteRequest()

## New Order Single

## New Order Multi-Leg

## Order Modification Request

## Order Status Request

## Order Cancel Request

## Order Mass Cancel Request

## Execution Report

## Trading Session Status

## Quote Ack

## Dont-Know Trade

## Application Ping

## Application Ping Reply

## Order Timeout

## User Response

## Cancel All Rates

## Business Message Reject

## Execution Report Ack

## Quote Status Report

## Order Mass Status Request

## Allocation


###################################################################################

# Issues


## Cache loading

- The following caches take a long time to load:

	SierraSettlementAccount
	BOSettlementAccount
	PSAFloorCode

- Each cache is loaded multiple times




####################################################################################

# Summary (Using outright as example)

Major data components: subscription/quotes and order/trades

Market orders do not have corresponding quotes.

## Subscription/Quotes

- Starts with receiving a QuoteRequest ("R") from client. Quote may have different
  streaming durations RFS or ESP. RFS stops if any of the following happens: 1. after 
  a preconfigured period. 2. a trade has been made 3. it is cancelled. For ESP, 
  1 and 2 dont apply. The client indicates type of quotes through QuoteRequestType(303):
  - 102: RFQ (RFS)
  - 103: RFS (ESP)
  - 101: cancel 

- The quote request is a FXQuoteRequest  is handled by the quote manager (`subscribeForQuotes`):
  - create TPS request object (type FXOutrightRequest)
  - create Subscription object (type Subscription)
  - perform the following book keeping:
    - m_usedSubscriptionIDs <- FIX request ID
	- m_subscriptionMap.insert <- (FIX request ID, subscription)
	- m_requestMap.insert <- (tpsRequestId, iterator of the subscription in m_subscriptionMap)
	- m_subscribedQuotes <- (a string by concat Symbol, Currency, Tenor, SenderCompId, Side)
  - send request to TPS 

- Whan a new quote from TPS is received, the quote manager looks up the subscription by
  TPS request id and pass the quote to it for handling:
    - store the quote in quote store manager, which:
      - add quote to quote history map by (fixId, tpsReqId)
      - generate quoteId
    - send the quote to client 

## Order/Trades 

- When a new order request comes in, it is passed to the order manager, which
  first looks up the corresponding quote request and TPS quote if this is not 
  a market order. Since it has no direct access to the quote store, this is 
  done through the session object.
- Create the trade instruction object (FXSpotForwardIns for outright)
- Create allocations if the request has pre-trade allocations
- If it is not a market order
  - add the quote to position tracker
  - add the order to position tracker
- Schedule order timeout checking
- Add the order to m_asyncAwaitingTrades
- Send the trade to the trade server

- When a response from the trade server is received, remove the corresponding
  trade from m_asyncAwaitingTrades first
- Depending on trade status, either 
  - m_positionTracker.rejectExecution(request.clientOrderId) or
  - m_positionTracker.addExecution(request.clientOrderId)
- Send execution report the client

Note that no explicit action is taken to remove quotes from position tracker.
cleanupStaleQuotes is called as part of storeQuote to clear stale quotes.

If an order response corresponds to an RFS subscription, the subscription 
is stopped (see `BANASessionOrderManager::handleExecutionReportSpotForwardNew`).

Another way an RFS subscription may stop is through timeout, see 
`Subscription::sendConsolidatedQuote`. Whenever a new quote is received:

- cancel the current m_rfqTerminationTimerTask
- reset m_rfqTerminationTimerTask with a new timeout and schedule it

Thus if a new quote is not received before the timer task fires, the 
subscription is stopped.

EPS subscriptions are always explicitly stopped by QuoteRequest.



## Quote Store TTL

FIX quote store TTL depends on three knobs:  TTL1, TTL2,  removalDelay

A quote is kept in the store for (TTL1+TTL2+removalDelay). Stale status checking is based on (TTL1+TTL2)

TTL1

- Check table FixSessionOption for optionTypeId=5 (stale_quote_ttl in ms), if present, 
  useit, current value 2000 only set for fix session id 1 2 3
- Otherwise take value from registry key `stale_quote_ttl`  with default value 2000

TTL2

- Check registry key `TTL_IgnoreOnQuote`
    - if false, set TTL2 to (quote.originSysTime + quote.timeToLive – now)
    - if true
- Check table FixSessionOption for optionTypeId=17 (valid_until_time in ms), if present, 
use it, current value 30000 only set for session BLUETREND-QTS
- Otherwise take value from registry key `valid_until_time` with default value 30000

There is a way to use an override value to short-circuit the above logic which is currently 
unused with the override value is set to -1. When set, both TTL1 and TTL2 are set to that value instead.

removalDelay is from registry key `expired_quote_removal_delay` with a default value of 4000 ms
