###################################################################################

# Main
 
	// init
    FIX::SessionSettings settings( file );

    Application application;
    FIX::FileStoreFactory storeFactory( settings );
    FIX::SocketInitiator initiator( application, storeFactory, settings );

	// connecting
    initiator.start();

	// running
    application.run();

	// shutdown
    initiator.stop();

###################################################################################

# Initialization

Setting parsing:

	FIX::SessionSettings settings(file);
	
Settings are parsed into multiple **sessions**. Every `SESSION` in the configuration is saved in `SessionSettings` by `SessionID`:

	typedef std::map < SessionID, Dictionary > Dictionaries;
	std::set < SessionID > getSessions() const;

	Dictionaries m_settings;
	Dictionary m_defaults;


Each session is identified by (beginString, senderCompID, targetCompID):

	SessionID(const std::string& beginString,
         	  const std::string& senderCompID,
              const std::string& targetCompID,
              const std::string& sessionQualifier = "" )
	  : m_beginString( BeginString(beginString) ),
	    m_senderCompID( SenderCompID(senderCompID) ),
	    m_targetCompID( TargetCompID(targetCompID) ),
	    m_sessionQualifier( sessionQualifier ),
		m_isFIXT(false)
	{
	    toString(m_frozenString);
			if( beginString.substr(0, 4) == "FIXT" )
				m_isFIXT = true;
	}

During initialization, all client sessions are added to the initiator:

	SocketInitiator::SocketInitiator( Application& application,
	                                  MessageStoreFactory& factory,
	                                  const SessionSettings& settings )
	: Initiator( application, factory, settings ),
	  m_connector( 1 ), m_lastConnect( 0 ) ...


	Initiator::Initiator(...) { initialize(); }
	Initiator::initialize() {
  		SessionFactory factory( m_application, m_messageStoreFactory,
                          m_pLogFactory );
		for each client session ID i with ConnectionType "initiator"
			// add session id to session id set
			m_sessionIDs.insert( *i );
			// create the session
			m_sessions[ *i ] = factory.create( *i, m_settings.get( *i ) );
			// set the session as disconnected
			setDisconnected( *i );
	

	Session* SessionFactory::create( const SessionID& sessionID,
                                 const Dictionary& settings ) {
  		Session* pSession = new Session( m_application, m_messageStoreFactory,
                         sessionID, dataDictionaryProvider, sessionTimeRange,
                         heartBtInt, m_pLogFactory );
		// set other properties based on settings

	void Initiator::setDisconnected( const SessionID& sessionID ) { 
		m_pending.erase( sessionID );
		m_connected.erase( sessionID );
		m_disconnected.insert( sessionID );
		

Initiator manages sessions by:

	  typedef std::set < SessionID > SessionIDs;
	  typedef std::map < SessionID, int > SessionState;
	  typedef std::map < SessionID, Session* > Sessions;
	
	  Sessions m_sessions;			// all sessions, indexed by session id
	  SessionIDs m_sessionIDs;		// all session ids
	  SessionState m_sessionState;	// session states

	  SessionIDs m_pending;			// pending sessions
	  SessionIDs m_connected;		// connected sessions
	  SessionIDs m_disconnected;	// disconnected sessions

The important thing to note is that the same initiator handles all client sessions in the configuration.

###################################################################################

# Connecting

The basic process is in `Initiator`:

	void Initiator::start() {
	  m_stop = false;
	  onConfigure( m_settings );	// pure virtual
	  onInitialize( m_settings );	// pure virtual
	  HttpServer::startGlobal( m_settings );
	  thread_spawn( &startThread, this, m_threadid );
	
	THREAD_PROC Initiator::startThread( void* p )
	  Initiator * pInitiator = static_cast < Initiator* > ( p );
	  pInitiator->onStart();	 		// pure virtual

Implementation details in `SocketInitiator`:

	void SocketInitiator::onConfigure( const SessionSettings& s ) {
		// setup stuff like buffer size, SO_NODELAY etc.

	void SocketInitiator::onInitialize( const SessionSettings& s ) {
		// nothing

	void SocketInitiator::onStart()

		// the following calls: Initiator::connect()
		// which invokes doConnect() for each session in m_disconnected
		//
		// create sockets for all sessions, call connect for them
		// add them to monitor and create SocketConnections for them
		connect();

		// main I/O loop
		while ( !isStopped() )
			// this is basically: m_connector.select( pIOHandler )
			m_connector.block( *this );
	
		// wait for up to 5 seconds to logout
		time_t start = time_t now = 0;
		::time( &start );
		while ( isLoggedOn() ) {
			m_connector.block( *this );
			if( ::time(&now) -5 >= start )
				break;
		}
	}
	
	void SocketInitiator::doConnect( const SessionID& s, const Dictionary& d )
		int socket = m_connector.connect( address, port, m_noDelay, m_sendBufSize, m_rcvBufSize );

		// moves the session from disconnected to pending
		setPending( s );

		// pending connections are indexed by socket descriptor
		// The SocketConnection class is a wrapper for socket. This
		// establishes the relationship between a Session and a socket:
		//    1. Session uses m_pResponder to send outgoing data
		//    2. SocketConnection implements the Responder interface
		//    3. SocketConnection constructor sets itself to be the
		//       m_pResponder of the Session identified by s
		// 
		// Note that the SocketConnection also keeps a reference 
		// of the connector's monitor
		m_pendingConnections[socket] = new SocketConnection(*this, s, socket, &m_connector.getMonitor() );

	int SocketConnector::connect( const std::string& address, int port, bool noDelay, int sendBufSize, int rcvBufSize )
		// create the socket and calls connect on it
		int socket = ...
		m_monitor.addConnect( socket );
		return socket;	// returns the socket file descriptor

`SocketConnector::m_monitor` is `SocketMonitor`, which wraps the logic of `select`:

	class SocketMonitor {

		Sockets m_connectSockets;
		Sockets m_readSockets;
		Sockets m_writeSockets;

		bool addConnect( int socket );
		bool addRead( int socket );
		bool addWrite( int socket );

		bool drop( int socket );

		void block( Strategy& strategy, bool poll = 0, double timeout = 0.0 );

		// its inner class defines callback interface
		// SocketInitiator implements this interface
		class Strategy {
		public:
		    virtual void onConnect( SocketMonitor&, int socket ) = 0;
		    virtual void onEvent( SocketMonitor&, int socket ) = 0;
		    virtual void onWrite( SocketMonitor&, int socket ) = 0;
		    virtual void onError( SocketMonitor&, int socket ) = 0;
		    virtual void onError( SocketMonitor& ) = 0;
		    virtual void onTimeout( SocketMonitor& ) {}
		}

		// note that it uses a socket pair to manage write set
	
		// adding a socket to write set is done indirectly,
		// it's sent using m_signal, when received over
		// m_interrupt, it's added to m_writeSockets
		void signal( int socket );

		// erase the socket from m_writeSockets, this is done directly
		void unsignal( int socket ); 

		int m_signal;
		int m_interrupt;

		SocketMonitor::SocketMonitor(...)

			sockets = socket_createpair();
			m_signal = sockets.first;
			m_interrupt = sockets.second;
			socket_setnonblock(m_signal);
			socket_setnonblock(m_interrupt);
			m_readSockets.insert(m_interrupt);

The main loop is in the `SocketInitiator` thread, the FIX state machine is driven by I/O events. 
The state machine starts with one or more connections in pending states:

	void SocketInitiator::onConnect( SocketConnector&, int s )
		SocketConnection* pSocketConnection = find connection for s m_connections[s] = pSocketConnection;
		m_pendingConnections.erase( i );
		setConnected( pSocketConnection->getSession()->getSessionID() );

		// this drives the state machine
		pSocketConnection->onTimeout();

	void SocketConnection::onTimeout()
		if ( m_pSession ) 
			m_pSession->next();

	void Session::next()
		next( UtcTimeStamp() );

	void Session::next( const UtcTimeStamp& timeStamp )
    	if ( !checkSessionTime(timeStamp) ) { reset(); return; }

		// check if we need to logout now
	    if( !isEnabled() || !isLogonTime(timeStamp) ) {
	    	if( isLoggedOn() ) {
	        	if( !m_state.sentLogout() ) {
	          		m_state.onEvent( "Initiated logout request" );
	          		generateLogout( m_state.logoutReason() );
				}
			} else
	        	return;
	    }
	
		// should logon ?
    	if ( !m_state.receivedLogon() ) {
      		if ( m_state.shouldSendLogon() && isLogonTime(timeStamp) ) {
        		generateLogon();
        		m_state.onEvent( "Initiated logon request" );
			} else if ( m_state.alreadySentLogon() && m_state.logonTimedOut()) {
				// check logon timeout
        		m_state.onEvent( "Timed out waiting for logon response" );
        		disconnect();
			}
      		return;
		}

		if ( m_state.heartBtInt() == 0 ) return ;

    	if ( m_state.logoutTimedOut()) {
      		m_state.onEvent( "Timed out waiting for logout response" );
      		disconnect();
    	}

		// heartbeat ?
    	if ( m_state.withinHeartBeat() ) return ;

    	if ( m_state.timedOut() ) {
      		m_state.onEvent( "Timed out waiting for heartbeat" );
      		disconnect();
    	} else {
			// test ?
      		if ( m_state.needTestRequest() ) {
        		generateTestRequest( "TEST" );
        		m_state.testRequest( m_state.testRequest() + 1 );
        		m_state.onEvent( "Sent test request TEST" );
      		} else if ( m_state.needHeartbeat() ) {
        		generateHeartbeat();
      		}
    	}

Incoming data is handled as:


	void SocketMonitor::block( Strategy& strategy, bool poll, double timeout )
		... calls processReadSet on read events

	void SocketMonitor::processReadSet(...)
		for each socket
			if it's m_interrupt
				read socket descriptor and add it to write set
			else
				strategy.onEvent( *this, socket); // note, no recv called

	void ConnectorWrapper::onEvent(...) 
		// m_strategy is out initiator
		if (!m_strategy.onData(m_connector, socket))
			m_strategy.onDisconnect(m_connector, socket);


	bool SocketInitiator::onData(SocketConnector& connector, int sock)
		SocketConnection* conn = lookup in m_connections by sock
		return conn->read(connector)

	// there's also a read(SocketAcceptor&, SocketServer&) for server side logic

	bool SocketConnection::read(SocketConnector& sc)
		call recv on socket to get incoming data
		add incoming data to m_parser
		readMessages(sc.getMonitor()); // remember monitor is a wrapper for the select sets of socket

	void SocketConnection::readMessage(SocketMonitor&)
		std::string msg;
		while (readMessage(msg))
			// drive session state machine with message
			m_pSession->next(msg, UtcTimestamp());


	void Session::next(const Message& msg, const UtcTimeStamp& ts, bool queued) 
		Header& header = msg.getHeader();
		get msgType, beginString, senderCompId and targetCompId
		regular sanity checking
		validate message with data dictionary based on message version

		if this is a session level message that may affect out state machine, handle it accordingly:
			nextLogon(...)
			nextHeartbeat(...)
			nextSequenceRequest(...)
			nextTestRequest(...)
			nextLogout(...)
			nextResendRequest(...)
			nextReject(...)

		// this actually handles the message, bad name
		if (!verify(msg))
			return;
		// update seq no
		m_state.incrNextTargetMsgSeqNum();

	void Session::verify(Message& msg, bool checkTooHigh, bool checkTooLow)
		... checking, validating ...
		m_state.lastReceivedTime(now);
		// invoke incoming side of callback
		fromCallback(...)

	// finally, we get to the application
	void Session::fromCallback(const msgType& msgType, const Message& msg, const SessionID& sid)
		if (Message::isAdminMsgType(msgType))
			m_application.fromAdmin(msg, m_sessionID);
		else
			m_application.fromApp(msg, m_sessionID);

###################################################################################

# Data Dictionary

For the following dictionary:

	<fix major='4' type='FIX' servicepack='0' minor='3'>
		<header>
			<field name='BeginString' required='Y' />
			<field name='BodyLength' required='Y' />
			<field name='MsgType' required='Y' />
			<field name='SenderCompID' required='Y' />
			<field name='TargetCompID' required='Y' />
			...
			<group name='NoHops' required='N'>
				<field name='HopCompID' required='N' />
				<field name='HopSendingTime' required='N' />
				<field name='HopRefID' required='N' />
			</group>
		</header>
		<messages>
			<message name='Heartbeat' msgcat='admin' msgtype='0'>
				<field name='TestReqID' required='N' />
			</message>
			<message name='TestRequest' msgcat='admin' msgtype='1'>
				<field name='TestReqID' required='Y' />
			</message>


For heartbeat, it's added as this:

	DataDictionary::readFromDocument(...)

		addMsgType("0");
		addValueName(32, "0", "Heartbeat");

		// the above add the mapping (35, "0") => "Heartbeat"
		// there's no way to do lookup: "Heartbeat" => "0"

		addMsgField("0", lookupXMLFieldNumber(..., "TestReqID"));