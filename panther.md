<!-- ################################################################################### -->


QA7 user

	fxqas07

Build 32bit with cmake:

	cmake -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32 ../


To update spec only

	cd cpp/src/libs/spec
	make

To use a sync proxy:

	// use DispatcherAutoSubscriber to set response topic in RV header and to auto-subscribe
	// to the response topic
	ml::panther::serialzie::protocol::panther::DispatcherAutoSubscriber das("rv_ps.MyContractReply");
	MyContractProxy proxy(*das.getDispatcher());
	Rsp rsp = proxy.getSnapshot();


FIX tags are defined in

	libs/ext/fix/StandardFixTags.hpp

<!-- ################################################################################### -->

# Utilities

## Namespaces

in `<common/common.hpp>`:

	namespace mlc = ml::common;


## Strings

	std::string s = common::text::Format(“%s %d”, “The sum is”, 5);

	mlc::text::StringTools {
		static bool startsWith...
		static bool endsWith...
		static bool isDate...
		static bool is???...
		static std::string loadTextFile(const std::string &fname);
		static std::string toLowerCase(const std::string&);
		static std::string toUpperCase(const std::string&);
		static std::string trim...(const std::string&);
		static std::string replaceAll(const std::string& s, const std::string& pattern, const std::string& replacement);
	}

## Environment Variables

	mlc::util::Environment {
		static bool isDefined(const char*);
		static void undefine(const char*);
		static std::string getString(const char*);
		static std::string getString(const char*, const char* dftVal);
		...Bool...
		...Int32...

## Command Line Arguments

	ml::common::application::CmdLine

	CmdLine cmdLine;
	CmdLine::ArgVector opts = cmdLine.argv();
	for (CmdLine::ArgVector::const_iterator it = ++opts.begin(); it != opts.end(); ++it) {
		const std::string& opt = *it;
	}

## Working with Settings (registry)

Settings are loaded in
	
	Application::Application() {
		applyRegShortcuts(values in $APPLICATION_REG_SHORTCUTS )
		// if --registry-shortcut, apply
		// if --registry-file, apply
		// if --registry-override, apply 
		applyRegFiles( files listed in $APPLICATION_REG_FILES )
		applyRegOverride(values in $APPLICATION_REG_OVERRIDES )
		// if /PROCESS doesn’t exist, apply the shortcut "/PROCESS=/ENVIRONMENT/<appname>/<id>"
		// where <id> is "" or --identifier=...
	}

Retrieve values through `ml::common::registry::RegistryHelper`

	bool getRegistryInteger(int& v, const std::string& key, const std::string& attrib);
	int getRegistryInteger(const std::string& key, const std::string& attrib, const int defaultValue);
	...Integer64...
	...String...
	...Bool...

To limit scope to `PROCESS` and `ENVIRONMENT`:

	int getRegistryIntegerLibOrEnv(…)


The setup of `$APPLICATION_REG_FILES` is done in `config/sh/panther.env`:

	export APPLICATION_REG_FILES="${PANTHER_CONFIG}/registry/panther.registry.xml"

	if [ -f ${PANTHER_CONFIG}/registry/environments/${PANTHER_ENVIRONMENT}.registry.xml ]
	then
	    export APPLICATION_REG_FILES="${APPLICATION_REG_FILES}:${PANTHER_CONFIG}/registry/environments/${PANTHER_ENVIRONMENT}.registry.xml"
	fi
	 
	if [ -f ${PANTHER_CONFIG}/registry/hosts/${HOSTNAME}.registry.xml ]
	then
	    export APPLICATION_REG_FILES="${APPLICATION_REG_FILES}:${PANTHER_CONFIG}/registry/hosts/${HOSTNAME}.registry.xml"
	fi

	if [ -f ${PANTHER_CONFIG}/registry/users/${USER}.registry.xml ]
	then
	    export APPLICATION_REG_FILES="${APPLICATION_REG_FILES}:${PANTHER_CONFIG}/registry/users/${USER}.registry.xml"
	fi

`panther.bashrc` sources `panther.env` to setup environ. On my machine:

	APPLICATION_REG_FILES=
		/export/work/zk8xmsb/src/trunk/source/config/registry/panther.registry.xml:
		/export/work/zk8xmsb/src/trunk/source/config/registry/environments/sgp_dev.registry.xml:
		/export/work/zk8xmsb/src/trunk/source/config/registry/users/zk8xmsb.registry.xml

Shortcut:

	APPLICATION_REG_SHORTCUTS=/ENVIRONMENT=/sgp_dev

So the process

	bana_fix_gateway --identifier=DEV-BANAFIX0 --daemon=yes

Would pick up `/PROCESS` from:

	<reg:root>
		<reg:key reg:name="sgp_dev"> <!-- alias to /ENVIRONTMENT -->
			<reg:key reg:name="bana_fix_gateway">
				<reg:key reg:name="DEV-BANAFIX0"> <!-- alias to /PROCESS -->

Overrides:

	APPLICATION_REG_OVERRIDES=...
	/ENVIRONMENT/sierra_mbu/reportDatabase@dataServerBCP=SYBEMA_SIE_UAT_DS06
	/ENVIRONMENT/sierra_mbu/reportDatabase@dbName=boaapp
	/ENVIRONMENT/housekeeper_server/database/NY/local@password=b1ackcat
	/ENVIRONMENT/housekeeper_server/database/LDN/local@password=b1ackcat
	/ENVIRONMENT/housekeeper_server/database/TKY/local@password=b1ackcat
	/ENVIRONMENT/housekeeper_server/database/LDN/global@password=b1ackcat
	/ENVIRONMENT/database/global_ro@username=panther_dbo
	/ENVIRONMENT/database/global_ro@password=b1ackcat
	/ENVIRONMENT/database/global_ro@dataServer=SYBASA_GFX_DEV_DS01
	/ENVIRONMENT/database/global_ro@dbName=dev_globaldb_19
	/ENVIRONMENT/failedWrite@location=/export/work/zk8xmsb/panther/trunk/logs/failed_write
	/ENVIRONMENT/database@isMaster=true

See also:

- `libs/panther/registry/RegistryUpdater`: update registry at runtime

<!-- ################################################################################### -->

# Logging

Set in registry as:

	<reg:key reg:name='logging' level='INFO' location='<stdout>' syslog='false'/>

Location can be `<stdout>`, `<stderr>` or a `dir path`


`common/RegistryVariables.hpp` defines keys to control logging:

	/ENVIRONMENT/logging
	/PROCESS/logging

<!-- ################################################################################### -->

# Concurrency

Thread

	mlc::concurrent::Thread {
		static Thread::Identifier self();
		Thread::Identifier id() const;

		void start();
		void stop();
		void join();

		Thread::State state() const;

		virtual void onThreadStart() {}
		virtual bool onProcess() = 0; // return true to stop the thread, false to continue
		virtual void onThreadStop() {}

		private void* run(void* pvThread) {
			Thread& thread = * static_cast<Thread *>(pvThread);
			While ( (thread.state() != TS_STOPPING) && (thread.onProcess() == Thread::CONTINUE)) ;


To wrap a task in thread (no generic task management, can only
do one-off task with a thread)

	mlc::concurrent::Task task(function_or_functor); 

or 

	mlc::concurrent::Task task;
	task.run(function_or_functor);

	Task : private Thread {
	bool onProcess() { return function_or_functor(); }

<!-- ################################################################################### -->

# Caching

libs/spec/cache

<!-- ################################################################################### -->

# DB


libs/ext/db

	// ensure the globaldb pool is initialized
	ml::spec::panther::messaging::ContractTopicDBBroker::Connection dummyConnection;
	ml::spec::panther::messaging::ContractTopicDBBroker::ROConnection dbConnection;
	
	Procedure proc( dbConnection, "gvContractTopic");	
	Dataset ds( proc.execute(false));

	value_ref<int32>  routerType(   ds["routerType"] );
	value_ref<string> routingKey(   ds["routingKey"] );

	while (ds.next()) {
		// access value by *routerType, *routingKey
	}


libs/panther/db


<!-- ################################################################################### -->

# RPC

== libs/panther/messaging


== libs/panther/serialize

## Transport

### Tibco RV

* when an rvrd starts up, it joins the "global"  mcast group for the region and connects to
  its peers as specified in config
* rvrd listens for rvg messages in the "global" mcast group and forwards those to neighbors


* when a panther process starts up, it looks for a running rvd on the same machine and starts
  one if none is found
* rvd joins the "local" and "global" groups for that region


* on local machines, "ft" service is used for heartbeat and "local" for all other traffic.

### Basic Router Interface

The transport is pub/sub style and is captured in the interface `ml::panther::messaging::interface::Router`:

	class Router {
	public:
		class Response {}
		class Topic {}

		void addListener(topic);

		void startListening();
		void stopListening();

		Response send(topic, msg, len, replyTopic);

		// router specific pub/sub
		void addSubscriber(cb, topic);
		void rmSubscriber(cb, topic);
		void rmSubscriber(cb);

The interface itself is mostly virtual except for this:

	bool Router::Topic::isGlobalTopicStr(const std::string& messagingTopic ) {
		RouterFactory::Type type = RouterFactory::getType(messagingTopic.substr(0, messagingTopic.find(".")));
		switch( type) {
			Default:
				FTHROW( RuntimeException, "No such type %d", type);		

			case RouterFactory::TIBRV_PUBSUB:			// "rv_ps"
			case RouterFactory::TIBRV_QUEUE:			// "rv_q"
			case RouterFactory::TIBRV_PQUEUE:			// "rv_pqueue"
			case RouterFactory::MULTICAST_PUBSUB:		// "mcc_ps", MCCRouter class
	        case RouterFactory::SERVER_P2P:             // "server_p2p", ServerRouter class
			case RouterFactory::INVALID:				// invalid
				return false;

			case RouterFactory::TIBRV_PUBSUB_GLOBAL: 	// "rvg_ps", TibrvRouter class
			case RouterFactory::P2P_TCP:				// "p2p_tcp", TcpRouter class
				return true;

Note that topics with transport type *rvg_ps* or *p2p_tcp* are **global**.

### Pub/Sub Router Interface

The actual pubsub implementations use an adapted router interface:

	class ml::panther::messaging::pubsub::interface::Router : public ...Router {
		struct WorkElement {
			std::string topic;
			std::string replyTopic;
			std::string payload;
			unsigned int sz;
		};

		typedef std::set< boost::shared_ptr<...RouterCallback> > Subscribers;
		typedef std::map< std::string, Subscribers> Subscriptions;
		typedef ml::common::container::WorkQueue< WorkElement> WorkQueue;

		Subscriptions subscriptions;	// topic -> [callback]

		WorkQueue workQueue;
		std::queue< InboundDecoupler*> threadPool;

It adds subscriber-independent pub/sub logic:

	virtual void subscribeTopic(const std::string& item) = 0;
	virtual void unsubscribeTopic(const std::string& item) = 0;

The original subscriber-specific pub/sub logic is implemented by

	void addSubscriber(cb, topic) {
		// 1. if new topic, subscribe
		// 2. add cb to subscribers
	}
	... remove is similar

It handles incoming data by:

	void onMessage(topic, replyTopic, payload, sz) {
		if( numThreads == 0) {
			// calls onMessageInternal which
			// passs data to callbacks by RouterCallback::onMessage(...)
		} else { 
			WorkElement 	ele;	// save callback data in ele
			workQueue.put( ele);
			// threads in threadPool pool workQueue (uses std::deque by default)
			// and invoke Router::onMessageInternal.
		}
	}

This part has the potential to be optimized with Disruptor.


### TCP Router

Related registry

	/ENVIRONMENT/messaging/tcp
		basePort
		domain
	 	timeout


Topic format:  

	p2p_tcp.:<host>:<port_offset>:

Actual port is base port plus port offset. TCP router topics

	std::string TcpRouter::makeTcpTopic( const std::string& topic )
		return ContractTopics::instance().makeTopicString( RouterFactory::P2P_TCP, 
			topic.substr(topic.find(".") + 1, string::npos));  // !!! Note

	// remote topic format:  p2p_tcp.:<host>:<port_offset>:

	std::string TcpRouter::getRemoteTopic( const std::string& host, int32 portOffset )
		if( host == "localhost" || host.empty() )
			topicHost = localhost;
		else
			topicHost = host;

		string remoteP2PTopic(  delim + topicHost + delim + port + delim);
		return ContractTopics::instance().makeTopicString( RouterFactory::P2P_TCP, remoteP2PTopic);

	// private topic format:  p2p_tcp.:<localhost>:<port_offset>:

	std::string TcpRouter::getPrivateTopic( int32 portOffset )
		string privateP2PTopic(  delim + localhost + delim + port + delim);
		return ContractTopics::instance().makeTopicString( RouterFactory::P2P_TCP, privateP2PTopic);

On startup, it creates thread pool and start the listening thread:

	TcpRouter::TcpRouter(int numThreads) : Router( numThreads)
		m_connection_listener.reset(new InboundConnectionListener(*this));

Subscribe/unsubscribe:

	p2p_tcp:<host>:<port>:<contract>

	void TcpRouter::subscribeTopic( const std::string& item)
		// listen on port
		// add item to topic list

	void TcpRouter::unsubscribeTopic( const std::string& item)
		// erase item from topic list

		// note no checking is done to see if listening
		// on port is still necessary, bad api design

Send:

	ml::panther::messaging::interface::Router::Response TcpRouter::send(topic, msg, sz, replyTopic)
		TcpConnection::TcpConnection_Ptr conn;
		// check m_outbound_connections for existing connection to destination
		// if not found, create a new one and add it to m_outbound_connections
		conn->send(topic, msg, sz, replyTopic);

Data is sent as:

	uint32 topicSz = strlen(topic);
	uint32 replyTopicSz = strlen(replyTopic);
	uint32 totalSz 	= sz+topicSz+replyTopicSz + (sizeof(uint32) * 2);
	
	writeUint32( totalSz );
	writeUint32( topicSz );
	writeUint32( replyTopicSz );
	
	write(  topic, topicSz );
	write(  replyTopic, replyTopicSz );
	write(  msg, sz );									

Take the contract `FXOutrightProvider` as an example, the settings are saved
in `ContractTopic` as:

	id    contractName       routingKey routerType topic          
	----- ------------------ ---------- ---------- -------------- 
	1000  FXOutrightProvider            4          :localhost:0:  
	15812 FXOutrightProvider lb-1       4          :localhost:10: 
	15813 FXOutrightProvider lb-2       4          :localhost:11: 
	15814 FXOutrightProvider lb-3       4          :localhost:12: 


The registry as the following

	<reg:key reg:name='messaging'>
		<reg:key reg:name='tcp' basePort='50810' timeout='-1' />
	</reg:key>

My `sh/zk8xmsb.panther.processes` has

	ALL_BOXES:fx_unified_tps --identifier=lb-1
	ALL_BOXES:fx_unified_tps --identifier=lb-2
	ALL_BOXES:fx_unified_tps --identifier=lb-3

When these servers run, they'll be providing the server side of contract
on port 50820, 50822, 50823 respectively. For clients trying to subscribe to
the contract, they would connect to these ports on localhost.

### Rv Router 

The actual Rv based implementation is `ml::panther::messaging::pubsub::tibrv::TibRvRouter`:

	class TibRvRouter : public ml::panther::messaging::pubsub::interface::Router {
		TibrvTransport* 	tibTransport;
		TibrvQueueGroup* 	tibQueueGroup;
		TibrvQueue*			tibQueue;		// created with policy TIBRVQUEUE_DISCARD_NONE
		TibrvDispatcher*	tibDispatcher;

		// constructor
		TibRvRouter(numThreads, config) {}

Note that `config` in the constructor plays two roles:

1. it sets transport type of the router based on naming conventions

		"global" => RouterFactory::TIBRV_PUBSUB_GLOBAL;
		"local"  => RouterFactory::TIBRV_PUBSUB;
		 others  => RouterFactory::TIBRV_PUBSUB;

	See also the notes on `RouterFactory::create()` below for the same logic.

2. it is used to get settings from the registry at
	
		/ENVIRONMENT/tibrv/networks/<config>

	to initialize `TibrvTransport` by:

		TibrvStatus TibrvTransport::create(
			const char* service = NULL,		// port 
			const char* network = NULL,		// multicase addr
			const char* daemon = NULL );	// how and where to find the Rendezvous daemon
											// see RV Concepts, page 110

	also, the description of the transport is set to:

		Panther-pubsub-<config>

The logic of mapping config name to transport type is duplicate in `TibRvRouter` and
`RouterFactory`. It should stay in `RouterFactory` and `TibRvRouter` can be refactored as:

	TibRvRouter::TibRvRouter(int numThreads, RouterFactory::Type type, const string& config) {}


`TibRvRouter` uses the `TibrvDipatcher` convenience class to dispatch in a separate thread. 
Note that `TibrvDispatcher` calls `TibrvDispatchable::timedDispatch(timeout)` in a loop. 
It's a **blocking** call and the value of timeout used is not documented. 

**Potential bottleneck** ? Make sense to do custome dispatch using `TibrvDispatchable::poll()` ?

	void startListening() {
		tibDispatcher = createDispatcher();
		// which does this:
		// TibrvDispatcher* disp = new TibrvDispatcher;
		// disp->create( tibQueueGroup, TIBRV_WAIT_FOREVER));

`TibRvRouter` sends message by

	// application level data should already be encoded as msg/sz
	Response send(const char* rvTopic, const char* msg, unsigned int sz, const char* replyRvTopic = "") {
		// wrap data with TibrvMsg::addOpaque() and send

it handles incoming data by

- New topics are added by `subscribeTopic`, which creates a listener with `tibrv::OnEvent` as handler
- `OnEvent` implements `TibrvMsgCallback` by calling `TibRvRouter::forwardMessage`
- `TibRvRouter::forwardMessage` calls `Router::onMessage`

Note we can eliminate `OnMessage` by implementing `TibrvMsgCallback` directly in `TibRvRouter`

### Topic Based Transport Selection and Router Factory

The formation of a final transport level topic follows this convention:

	transport_type.app_level_topic

Transport is automatically chosen based on `transport_type`. Transport *rvg_ps* and *p2p_tcp* 
are global (see interface `Router` above). Consequently, a topic can also be *global* or *local*, 
depending on its transport type.

For SPEC contracts, contract names are used as application topic names. 

Transport types are defined as:

	class RouterFactory
	{
	public:
		// integers are used in DB
		enum Type { 
			TIBRV_PUBSUB 		= 1,
			TIBRV_QUEUE			= 2, 
			MULTICAST_PUBSUB	= 3, 
			TIBRV_PUBSUB_GLOBAL = 4,
			P2P_TCP 			= 5,
			SERVER_P2P        	= 6,
			TIBRV_PQUEUE      	= 7,
			INPROC				= 8,
			INVALID	 			= 100
		};
		static const string MULTICAST_PUBSUB_STR  		= "mcc_ps";
		static const string TIBRV_PUBSUB_STR  	  		= "rv_ps";
		static const string TIBRV_QUEUE_STR   	  		= "rv_q";
		static const string TIBRV_PERSISTENT_QUEUE_STR  = "rv_pqueue";
		static const string TIBRV_PUBSUB_GLOBAL_STR 	= "rvg_ps";
		static const string P2P_TCP_STR 				= "p2p_tcp";
		static const string SERVER_P2P_STR         		= "server_p2p";
		static const string INVALID_STR 				= "invalid";

The class provides mapping between router type and string:

	static const Type getType( const std::string&)
	static const std::string & getStringName(Type)

It also provides factory method for routers:

	static boost::shared_ptr<Router> create(int numThreads, Type t = TIBRV_PUBSUB) {}

	static boost::shared_ptr<Router> create(int numThreads, const std::string& type) {
		// "rv_ps" has special logic to allow custom routers
		TIBRV_PUBSUB_STR           => new pubsub::tibrv::TibRvRouter(numThreads, "local");
		TIBRV_PUBSUB_STR.<config>  => new pubsub::tibrv::TibRvRouter(numThreads, <config>);

		// the rest do strict match
		TIBRV_PUBSUB_GLOBAL_STR    => new pubsub::tibrv::TibRvRouter(numThreads, "global");
		TIBRV_QUEUE_STR            => new distqueue::tibrv::TibRvRouter(numThreads, "local");
		TIBRV_PERSISTENT_QUEUE_STR => new distqueue::tibrvp::TibRvRouter(numThreads, "local");
		MULTICAST_PUBSUB_STR       => new pubsub::multicast::MCCRouter(numThreads);
	    P2P_TCP_STR                => new point2point::tcp::TcpRouter(numThreads);
		SERVER_P2P_STR             => new serverp2p::tcp::ServerRouter(numThreads);
	}

Note that:

1. For Rv routers, the router factory translates topic strings to 
network configuration names:

		rv_ps            => local
		rv_ps.<config>   => <config>
		rvg_ps           => global

2. Topics used by the router factory are used only to set router type and
network configuration. It doesn't lead to any actual subscription. These
topics are not the same as the actual application level topics, as discussed below.

### Router Manager

`ml::panther::messaging::interface::Routers` implements router management:

	typedef mlc::pattern::Singleton< RoutersImpl> Routers;

	class RoutersImpl {}

It has facilities to register callbacks for router thread startup notifications:

	Callbacks	m_callbacks;
	void registerOnStartupCallback( const RouterOnStartupCallback::Ptr& callback );
	void unRegisterOnStartupCallback( const RouterOnStartupCallback::Ptr& callback );
	void callOnStartupCallbacks( const RouterFactory::Type routerType );

For instance, the pub/sub router has the following thread pool:

	std::queue< InboundDecoupler*> threadPool;

And the worker thread look like this:

	class InboundDecoupler : public ml::common::concurrent::Thread {
		Router& router;
	public:
		InboundDecoupler( Router& router);
		void onThreadStart()  {
			...Routers::instance().callOnStartupCallbacks(router.getRouterType());
		}

It also organizes routers in two groups:

	typedef std::map< std::string, boost::shared_ptr<Router> > Routers; // topic -> router
	typedef std::map< unsigned int, Routers > RouterGroups;             // id -> group
	RouterGroups m_routers;

Group 0 is for general use and group 1 is used for contract implementations.

It's pub/sub interface is:

	void addSubscriber(cb, topic, group);
	void rmSubscriber(topic, cb,  group);
	void rmSubscriber(cb, group);

Routers are created on-demand when subscribers are added:

	void addSubscriber(cb, topic, group) {
		type = getType(topic)
		// search group for router with type, ceate if not found
		// add cb to the router by topic
		// Note routers are indexed in a group by type
	}

`getType` translates an application level topic to a router level topic:

	xxx          => xxx
	xxx.yyy      => xxx
	xxx.yyy^zzz  => xxx.yyy


To summarize, a topic is mapped to a router in the following manner, using
Rv as examples:

	1. Routers::getType,  app_topic => router_topic

		a) rv_ps           => rv_ps
		b) rv_ps.abc       => rv_ps
		c) rv_ps.xyz       => rv_ps

		d) rv_ps.mmm^part1 => rv_ps.mmm
		e) rv_ps.mmm^part2 => rv_ps.mmm

	2. RouterFactory::create, router_topic => (router_type, net_config_name)

		a) rv_ps            => (TIBRV_PUBSUB, "local")
		b) rv_ps.mmm        => (TIBRV_PUBSUB, "mmm")

	3. TibRvRouter::TibRvRouter

		a) set router type to TIBRV_PUBSUB
		b) create transport based on registry 
			/ENVIRONMENT/tibrv/networks/local
			/ENVIRONMENT/tibrv/networks/mmm


### Client Proxy

`libs/panther/serialize/contract/ContractProxy`: base class of client side contract impl

*Initial buffer size* set in `/PROCESS/SPEC@init_buffer_size`

*Response timeout* set in `/PROCESS/SPEC@response_timeout`

Uses `Dispatcher` to handle transport.

	ContractProxy::ContractProxy(...) {
		protocol::panther::Dispatcher& 	m_dispatcher;	// dispatcher to send data
	}

	void ContractProxy::setContractName(const string& name) {
		m_contractName = name;
		// no transport, uses default transport to get:  rv_ps.<name>
		m_messagingTypeAndTopicName = ContractTopics::instance().getTypeAndTopic(m_contractName);

	class TestContractProxy : public ContractProxy {
		TestContractProxy(Dispatcher& d) : ContractProxy(d) {
			// Use contract name as application level topic, final topic "rv_ps.TestContract"
			setContractName("TestContract"); ...
		}
	}

Also, a random reply topic is generated to receive reponse if necessary:

	string ContractProxy::generateUniqueTopic() const {
		return "server." + uuid().toString();
	}

A client side RPC call is implemented/generated as:

	String methodName("sayHello");
	const uint32 numArgs = ...;

	// serialize headers for a panther FMT_ContractCall message
	string replyTopic = initiateCall(methodName, numArgs);

	// serialize RPC call parameters
	... stream = getCallMessage().getStream();
	stream.write(...);
	stream.write(...);

	bool expectResponse = ... // depends on if the call returns a value

	// send the message
	makeCall(expectResponse);

	if (expectResponse) {
		waitResponse(size, methodName, replyTopic);

		// the waitReponse will only decode headers, need to
		// decode actual return value ourselves
		...BoundedStream bs(getReplyMessage().getStream()...);

		T rc;
		bs.read(rc);
		return rc;
	}

The logic of `ContractProxy` response handling is:

	makeCall(expectRsp) {
		if (expectRsp)
			m_dispatcher.registerService( m_myself, replyTopic, true);
		//send message
		m_dispatcher.send(...);

	waitForResponse(...) {
		m_condition.wait( ... tmout);
		m_dispatcher.unregisterService(m_myself, true);
		if timeout, throw
		get data from m_replymessage
		decode header and do sanity checking

	void onServiceRequest(Message& message)
		if topic does not match expected, ignore
		m_replyMessage     = message;	
		m_responseReceived = true;	
		m_condition.notifyAll();


### Server Stub

`libs/panther/serialize/contract/ContractService`: base class of server side contract impl

	// unlike ContractProxy, a ContractService may have multiple dispatchers
	std::deque< boost::shared_ptr< protocol::panther::Dispatcher> > 	m_dispatchers;


libs/fx/discovery: `ServiceDiscoveryContract`


### Contract Topics

Contract topics are stored in DB (`spec/definitions/panther/messaging/ContractTopic.spec`)

	// SPEC
	class ContractTopic version 1.0 
	{
		... for global_db
	    enum RouterType ( 
	    	TIBRV_PUBSUB, 
	    	TIBRV_DISTQUEUE,
	    	MCAST, 
	    	TIBRV_PUBSUB_GLOBAL,
	    	P2P_TCP,
	    	TIBRV_PERSISTENT_DISTQUEUE
	    );

		// contract name
		contractName varchar(128) readOnly;

		// routing key, empty string indicates not to use a key
		routingKey varchar(64) readOnly;

		// routing subsystem
		routerType refEnum RouterType; 

	     // messaging topic
		topic varchar(255);
	}

And can be accessed via `ContractTopicsImpl` (libs/panther/serialize/contract/ContracTopics.h/cpp)

	// all the details on loading topics from db
	ContractTopicsImpl::ContractTopicImpl() {

		// default router is in /ENVIRONMENT/spec@default_router:
		//
		// <reg:key reg:name='/ENVIRONMENT' envID='255'>
		// 	<reg:key reg:name='spec' default_router='rv_ps'/>

		RegKey& rootKey = Registry::instance().key( "/ENVIRONMENT/spec");
		defaultRouterName = rootKey[ "default_router"];

		// ensure the globaldb pool is initialized
		ml::spec::panther::messaging::ContractTopicDBBroker::Connection dummyConnection;
		ml::spec::panther::messaging::ContractTopicDBBroker::ROConnection dbConnection;
		
		Procedure proc( dbConnection, "gvContractTopic");	
		Dataset ds( proc.execute(false));
		...
	}

Based on whether routing key is set, `ContractTopicsImpl` organize topics in two categories:

	// default, no routingKey specified:
	//       (contractName, routerType) -> topic
	m_defaultContractMap[contractName][routerType] = topic;

	// routingKey specific:
	//       (contractName, routingKey, routerType) -> topic
	m_contractMap[contractName][routingKey][routerType] = topic;


Note that  `routerType` (should really be transport type) and `topic` (rv specific) are transport level controls.
`contractName` and `routingKey` are application level controls. A better conceptual categorization of 
the above should be:

   		(contractName) -> (topic, transport)
   		(contractName, key) -> (topic, transport)

or simply:

   		(contractName, key) -> (topic, transport)

as the first is the special case where `key == ''`.

`ContractTopicsImpl` forms the final RV topic by combining transport and topic:

	string makeTopicString(const string& messagingType, const string& topicName) {
		return messagingType + "." + topicName;
	}

`ContractTopicsImpl` also provides methods to query topics (bad code):

	// search by (contract, key, transport) in m_contractMap
	std::string getTypeAndTopic(contract, key, transport);

	// search by (contract, transport) in m_defaultContractMap
	std::string getTypeAndTopic(contract, transport);

	// search by (contract, key) in m_contractMap, the final list of (transport, topic)
	// are returned as encoded final RV topic strings.
	std::deque< std::string> getAllTypeAndTopics(contract, key);

	// search by (contract) in m_defaultContractMap and optionally m_contractMap as well
	std::deque< std::string> getAllTypeAndTopics(contract, bool includeEntriesWithRoutingKey = true);

	// same as above, except the routing key for each RV topic is also returned as
	// the first item of a pair. For entries from the default map, they are ""
	std::deque< std::pair < std::string, std::string> >
	getAllTypeAndTopicsWithKey(contract, bool includeEntriesWithRoutingKey = true);

	// search by (contract) in m_defaultContractMap:
	//    a. if found
	//		 i. if unique, return it
	//       ii. if multiple, error
	//    b. if not found, return <default_transport>.contract
	std::string getTypeAndTopic(contract);
	
	// search by (contract, key) in m_contractMap:
	// 1. if found
	//    a. if unique, return it
	//    b. if multiple, error
	// 2. if not found, search by (contract) in m_defaultContractMap:
	//    a. if found
	//		 i. if unique, return it
	//       ii. if multiple, error
	//    b. if not found, return <default_transport>.contract
	std::string getTypeAndTopic(contract, key);

	
- how are the table `ContractTopic` used
- relationship among `ContractTopic`'s fields
- relationship between the actual RV topic and `ContractTopic`

Cache update messages are broadcasted with the following subjects:

	CacheHelper<T>::getBroadcastTopic() {
		return Router::Topic( ContractTopics::instance().makeTopicString( T::BROADCAST_ROUTERTYPE, typeid(T).name()) );


## In-process RPC

RPC can be made in-proc if both proxy and stub are running in the same process.
First instruct spec to generate in-proc code:

	contract FXOutrightListener
	generate
		c++ client server inproc code in ...

Stub:
- the original on inproc code is unchanged, when an encoded method call message comes, it
  checks the method name and calls handle_onQuote, which in turn calls onQuote
- registerService will register an instance of the stub with the Routers::instance()

Proxy:
- in constructor, it checks ContractTopics to see if in-proc mode should be used
	static const bool isInProcCall = ml::panther::serialize::contract::ContractTopics::instance().isInProc("FXOutrightListener");
  if yes, it tries to grab a copy of the registered stub instance directly
- in onQuote, it directly invokes stub->onQuote

<!-- ################################################################################### -->

# Domain

libs/fx/common: CurrencyPair, MidRate, Price, Rate, Side, Spread, Tenor


Tables in global db

- ContractTopic
- FixApplication
- FixDialect
- Tenor
- VolumeBand


<!-- ################################################################################### -->

# SPEC

For contract `HelloContract`, the following are generated:

	// interface def
	class HelloContract {
		// pure virtual for all methods
		static const std::string getName() { return std::string("HelloContract"); }
	};

	// client side
	class HelloContractProxy : virtual public HelloContract, public ContractProxy {
	};

	// server side
	class HelloContractStub : virtual public HelloContract, public ContractService {
	};

 
 `ml::panther::serialize::spec::BrokerRegistrator`


Methods that either return a value or *throw* are executed synchronously:

	class MyContractProxy : ...

	void sayHelloWithException(...) throw(...) {
	int sayHelloWithReturn(...) {
		...
		makeCall(true);
		uint32 size;
		waitResponse(size, methodName, replyTopic);

## Message encoding

Proxies are derived from `ContractProxy` and encode outgoing message as:

	std::string replyTopic = initiateCall(methodName, numArgs);
	ml::panther::serialize::stream::ByteArrayStream& stream = getCallMessage().getStream();
	stream.write(...);
	stream.write(...);
	...
	makeCall();

The details are in the base class:

	class ContractProxy : public protocol::panther::Service

	protocol::panther::Dispatcher& m_dispatcher;
	protocol::panther::Message m_callMessage;

	Message& getCallMessage() { return m_callMessage; }

Message is:

	class Message

	std::string messagingReplyTopic;
	header header;
	boost::shared_ptr< stream::ByteArrayStream > byteStream;

	ByteArrayStream& getStream() { return * byteStream; }

The key is the `ByteArrayStream` class:

	class ByteArrayStream : public MessageStream

	char* m_buffer;
	unsigned int m_readPos;
	unsigned int m_sz;		// written
	unsigned int m_maxSz;	// capacity

	// these two are the key
	virtual void writeRaw(const void*, unsigned int sz);
	virtual void readRaw(void*, unsigned int sz);

	void writeRaw(const void* data, unsigned int dataSz) 
		unsigned int requiredSz = m_sz + dataSz;
		while (requiredSz > m_maxSz) {
			m_maxSz *= 2;
			m_buffer = (char*)realloc(m_buffer, m_maxSz);
		}
		memcpy(m_buffer + m_sz, data, dataSz);
		m_sz += dataSz;

	// these are provided to help encode len/value

	class BlockSizeMarker
		unsigned int writePosition;
		unsigned int blockStartPosition;
		ByteArrayStream* stream;

	BlockSizeMarker getBlockSizeMarker()
		BlockSizeMarker marker;
		unsigned int tmp = 0xFFFFFFFF;
		marker.writePosition = m_sz;
		marker.stream = this;
		writeBlockSize(tmp);
		marker.blockStartPosition = m_sz;
		return marker;


	void writeBlockSizeMarker(const BlockSizeMarker& marker)
		unsigned int blockSize = m_sz - marker.blockStartPosition;
		unsigned int netOrdered = htonl(blockSize);
		memcpy(m_buffer + marker.writePosition, &netOrdered, sizeof(netOrdered));

	// not very efficient, can be replaced with:
	char startBlock();
	void endBlock(char blockId);
	// an array can be used for block offsets:
	int m_blockOffsets[32];
	int m_numBlocks;
	// index is used as block id



<!-- ################################################################################### -->

# Topic

The concept of *topic* plays an extremely important role in both the underlying
framework and the application space. However, the logic to handle topics at 
different layers is quite messy. This section tries to clarify its usage.

Each Rendezvous message has a subject name. It is used to designate message 
end points within a network service. It also uses subjects for special handling 
and restricted internal use. Special subject names begin with an underscore.
A message may optionally carry a reply subject.

A subject name may consist of multiple elements separated by dot.  This design of naming
scheme allows subjects to form hierarchical namespaces. Wildcard
characters can be used to match subject name patterns, which makes effective 
message filtering possible.

SPEC-generated contract proxy implementations `HelloContractProxy` set 

	ContractProxy::m_contractName

to contract name `HelloContract`. The Rv subject 

	ContractProxy::m_messagingTypeAndTopicName

is set to

	ContractTopics::instance().getTypeAndTopic(m_contractName);

This will search the db for predefined settings, if not found, it returns
`<default_transport>.<contract_name>`, which is `rv_ps.HelloContract` in this case.

When making a call:

- Set topic in panther message header to `HelloContract`
- Set reply topic in header to `server.<uuid>`
- Register service with dispatcher:
		
		Dispatcher.registerService("HelloContract"...)
		// and if response expected
		Dispatcher.registerService("server.<uuid>"...)

  Note that `Dispatcher.registerService` only adds the handlers to its
  callback table using service names as provided.
- Send message by

		Dispatcher.send(ContractProxy::m_messagingTypeAndTopicName ...) {
			Routers::instance().send( 
				ContractProxy::m_messagingTypeAndTopicName, ...
				Dispatcher::messagingTypeAndTopicName);

	Note that Rv subject comes from `ContractProxy`, but Rv reply subject 
	is `Dispatcher`'s own, which defaults to `"none"` if not provided on construction.

		class RoutersImpl 
		// Hence normal use should use the group 0, and group 1 is used by contract implementations.
		Router::Response send(subject, msg, sz, replySubject = "", group = 0);

	**Note that documentation states that group 1 should be used, but the default
	value 0 is used here**. While sending data through `RouterImpl`, it translates
	the topic to a router object name (See Router Manager for details):

		rv_ps.HelloContract => rv_ps

	Since the router cannot be found, "rv_ps" is passed to `RouterFactory` to create
	the router. `RouterFactory` maps the router object name to transport type and
	network configuration name (if transport type is TIBRV_PUBSUB). 

	In this case, "rv_ps" will lead to an instance of `TibRvRouter` created with type 
	`TIBRV_PUBSUB` and network configuration settings "local".

	Finally the router object is added to `RoutersImpl` by key `rv_ps` and 
	message sent through the router object.

Note that:

1. in the above process, no subscription is made at tibrv, response, if expected,
   cannot be received without further action. `DispatcherAutoSubscriber` can be used:

		DispatcherAutoSubscriber::~DispatcherAutoSubscriber() // unsubscribe from topic
		shared_ptr<Dispatcher> DispatcherAutoSubscriber::getDispatcher() // subscribe to topic

		// this does two things:
		// 1. set Dispatcher::messagingTypeAndTopicName to rv_ps.HelloContract, which
		//    will result in outgoing messages having the right replySubject
		// 2. subscribe to rv_ps.HelloContract so that response can be received
		DispatcherAutoSubscriber das(ContractTopics::instance().makeTopicString(RouterFactory::TIBRV_PUBSUB, "HelloContract"));
		HelloContractProxy proxy(das.getDispatcher());

	This is bad api. `HelloContractProxy`'s transport type and name are leaked.

2. When making contract proxy calls, the topic names are closely related to the way 
  `RoutersImpl` manages router objects and the way router objects are created, the
  topic names can only be of the following format:

  		a. <transport_name>
  		b. <transport_name>.<contract_name>
  		c. rv_ps.<contract_name>^<routing_key>

  	1. a and b create routers named <transport_name> in RoutersImpl
  	2. b creates routers named rv_ps.<contract_name> in RoutersImpl
  	3. c is an extention to b, but such an extension only applies to "rv_ps"
  	4. the reason for 3 is that <transport_name> and rv_ps.<contract_name> are passed
  	   to `RouterFactory` to create router objects:
  	   1. Transport types other than Tibrv, only <transport_name> can be used. It
  	      it is directly mapped to types of transport router objects.
  	   2. For tibrv, the following mapping is applied to provide parameters to
  	      `TibRvRouter` constructor:

  	   			rvg_ps              => (TIBRV_PUBSUB_GLOBAL, "global")
				rv_ps               => (TIBRV_PUBSUB, "local")
				rv_ps.<name>        => (TIBRV_PUBSUB, "<name>")


So `global` and `local` identifies default settings for global and local tibrv
services and the name convention `<transport>.<name>^<routing>` allows custom tibrv 
settings but it applies to local services only.

From `registry/panther.registry.xml`:

    <!-- ======================================================== -->
    <!--             TIB RV                                            -->
    <!-- ======================================================== -->

    <reg:key reg:name='tibrv'>
        <reg:key reg:name='networks'>
            <reg:key reg:name='global' network='' service='' daemon='' reliability='60'/>
            <reg:key reg:name='local'  network='' service='' daemon='' reliability='60'/>
            <reg:key reg:name='tick_data'     network='' service='' daemon='' reliability='6'/>
            <reg:key reg:name='gateway_data'  network='' service='' daemon='' reliability='6'/>
            <reg:key reg:name='ft' network='' service='' daemon='' reliability='20'/>
        </reg:key>
    </reg:key>

And from global db, note the lack of consistency: 

	use dev_globaldb_19
	select * from ContractTopic where topic like '%^%' order by topic

	contractName 				routingKey 	routerType 	topic
	----------------------------------------------------------------------------------------------
	ServiceHeartbeatContract				0			ft^ServiceHeartbeatContract
	ServerHeartbeatContract					0			ft^server_heartbeat
	AggIndicatorCallback					0			gateway_data^AggIndicatorCallback
	AggregatedQuoteListener		Aggregated	0			gateway_data^AggregatedQuoteListener
	AggregatedQuoteListener					0			gateway_data^AggregatedQuoteListener.general
	FXFwdTick					normal		0			tick_data^FXFwdTick.normal
	FXFwdTick					preview		0			tick_data^FXFwdTick.preview
	FXFwdTickListener			normal		0			tick_data^FXFwdTickListener.normal
	FXFwdTickListener			preview		0			tick_data^FXFwdTickListener.preview
	FXFwdTickProvider			normal		0			tick_data^FXFwdTickProvider.normal
	FXFwdTickProvider			preview		0			tick_data^FXFwdTickProvider.preview
	... many more tick_data


Once the client side has been setup properly, the server side logic is quite
simple. `HelloContractStub` subscribes to `rv_ps.HelloContract` to receive 
requests. Responses are sent to replySubject of requests. At application level, 
panther request header's topic and replyTopic becomes response header's replyTopic
and topic.

These are/use contract names:

- ContractProxy::m_contractName
- Dispatcher::registerService()
- ContractTopics::getTypeAndTopic()


These are/use tibrv subjects:

- ContractProxy::m_messagingTypeAndTopicName
- Dispatcher::send()
- RoutersImpl::send()


<!-- ################################################################################### -->

# Tips

##### User Permissions for Currency Pair 

Permissions are specified in

	select id, name, assetId, description, hasAction, internal, clientPrivilege, serverPrivilege from Permission

	id	name						description							assetId	hasAction	internal	clientPrivilege	serverPrivilege
	1	Everything					Permit access to all static data	5		Y			Y			Y				Y
	10	EXECUTE_TRADES				Execute trades						10		N			N			N				Y
	12	EXECUTE_TRADES_FOR_GROUP	Execute trades for group			10		N			N			N				Y
	14	EXECUTE_TRADES_FOR_CPARTY	Execute trades for counterparty		10		N			N			N				Y
	16	EXECUTE_TRADES_FOR_BANK		Execute trades for bank				10		N			N			N				Y
	20	VIEW_BLOTTER				View blotter trades					10		N			N			Y				Y
	22	VIEW_BLOTTER_FOR_GROUP		View blotter trades for group		10		N			N			Y				Y
	26	VIEW_BLOTTER_FOR_BANK		View blotter trades for bank		10		N			N			Y				Y


Currency pair classifications are defined in:

	class CurrencyPairClassification version 1.0

	// Currency pair
	ccyPair char(6) readOnly foreign key CurrencyPair(name);

	// classificationType - type 
	groupName varchar(32) readOnly foreign key CurrencyPairGroups(groupName);
	
	// A rank may be used for any other purposes like sorting etc
	rank int32 null;

	// note, same currency pair in multiple groups allowed
	with primary key (ccyPair, groupName)


	select * from CurrencyPairClassification

	id	ccyPair	groupName	rank
	8	AUDUSD	Majors		900	
	19	AUDUSD	G10			300	
	5	EURCHF	Majors		600	
	12	EURDKK	Majors		1300
	4	EURGBP	Majors		500	
	2	EURJPY	Majors		300	



CcyPairGroupPermissions links Permissions to CurrencyPairClassifications

	class CcyPairGroupPermissions version 1.0 

    //CurrencyPairClassification Id
    ccyPairClassificationId int64 readOnly foreign key CurrencyPairClassification(id);
    
    //Permission Id
    permissionId int64 readOnly foreign key Permission(id);

So the granularity of control is at currency pair level as CurrencyPairClassification is per currency pair. To create group
level control, create one-to-many mapping from Permission to CurrencyPairClassifications in CcyPairGroupPermissions
by mapping the same permission id to all entries of the same group in CurrencyPairClassifications.

RolePermission links Role to Permission. Note that Read/Write is part of this mapping instead of Permission. Bad design.

	class RolePermission version 1.0

	//RoleID
	roleID int64 readOnly foreign key PantherRole(id);
	
	//PermissionID
	permissionID int64 readOnly foreign key Permission(id);
			
	//W - Write , R - Read, B - Both, N - None
	actions char(1);


PantherUserRole links PantherUser to PantherRole

	class PantherUserRole version 1.0 

	//User Id
	userId int64 readOnly foreign key PantherUser(id);
	
	//Role Id
	roleId int64 readOnly foreign key PantherRole(id);


The mappings

	PantherUser <-- PantherUserRole --> PantherRole <-- RolePermission --> Permission <-- CcyPairGroupPermissions --> CurrencyPairClassification


When a user logs in, the login server returns the list of user permissions in response. 
Thus, user role update will not take effect before the next login.


##### Cameron

To reset message sequence number for each logon, add this to config:

	<SessionManager resetOnLogon="true">

##### Sybase

Check stored procedure content

	sp_helptext "storedProcName"


To drop a column from table, need to enable "select into" option first

	master..sp_dboption dev_globaldb_19, "select into", true
	alter table ...

##### Failed to start rvd

When accessing rv service, a client may result in a local rvd started
automatically if it is not already running. This creates implicit dependencies
among rv clients. If various clients require different options for rvd, 
clients requiring more specific options may fail to start if clients with
more general options started before them.


##### Prevent Stripping

	export STRIP_EXE=0


##### Tracing TCP Connections

	void InboundConnectionListener::listenOn(uint32 port)
		LINFO( "Tcp Router Listening on port: %d", port);	

So use

	grep "Tcp Router Listening" 

to get server sockets.



##### Tracing Dispatcher Service Registrations

To trace subject registration:

	grep registerService logs/<app_name>.latest

Meta level logging should be on. See 

	void Dispatcher::registerService(sp, pantherTopicName, isProxy ) {
	    LMETA("****** registerService %s *******" , pantherTopicName.c_str());

Note these are dispatcher level callback registration, they only
lead to callbacks registered with the dispatcher. No subjects 
are added to the underlying TibRvRouter.


##### Tracing Tibrv Subjects


To show Tibrv subscriptions:

	grep "Subscribed to" apps/<app_name>.latest

See

	void TibRvRouter::subscribeTopic(const string& subject) {
		LINFO( "Subscribed to: %s", subject.c_str());

##### Tracing Contract Calls

To trace client side contract proxy calls:

	grep "Sending method call" logs/<app_name>.latest

See

	void ContractProxy::makeCall(bool expectingResponse) {
		LTRCE(
			"(%s) Sending method call to messaging topic %s",
			replyTopic.c_str(), m_messagingTypeAndTopicName.c_str());

To trace server side contract service responses:

	grep "Sending to messaging topic" logs/<app_name>.latest

Trace level logging should be on. See

	void ContractService::sendReply(messagingReplyTopic, header, exception) {	
		LTRCE( 
			"Sending to messaging topic='%s' data\n%s", messagingReplyTopic.c_str(),
			Format::toTraceString(sendBuffer.data(), sendBuffer.size()).c_str());


To trace incoming messages:

	grep "Received request on" logs/<app_name>.latest

Each request will have two trace messages, with one showing reply topic the other
message data. Or

	grep "Received message on panther" logs/<app_name>.latest

This will show only one trace for each message. See

	void Dispatcher::onMessage(messagingTopic, replyTopic, payload, payloadSize) {
		LTRCE( 
			"Received request on topic %s reply to '%s'",
			messagingTopic, replyTopic==0 ? "<none>" : replyTopic);
		
		LTRCE( 
			"Received request on topic %s data\n%s",
			messagingTopic, Format::toTraceString(bas.data(), bas.size()).c_str());

		LTRCE( 
			"(%s) Received message on panther topic %s", 
			callerId.c_str(), messagingTypeAndTopicName.c_str());


#### Print stack trace

	ml::common::diagnostic::StackTrace st;
	LINFO("%s", st.toString().c_str());


<!-- ################################################################################### -->

# Issues

## Cache 

Caching logic always reload after the initial read:

	CacheImpl::doLoad(...)
		conn = broker.getROConnection();
		DataSet ds = broker.getAll(*conn);
		add ds
		scheduleRefresh(INITIAL_DELAY) { // 15 seconds
			if (firsttime)
				create timed task for CacheImpl::reload, which calls doLoad again
		}
what purpose does this serve ? 

Some caches take a long time to load. For example, `CreditSettlementAvblty` in
credit_server takes 10-20 seconds to load.

## BANA FIX Gateway

An identifier is used to select the dialect of FIX as well as its related settings:

	bana_fix_gateway --identifier=LMAX

The identifier needs to have a matching entry in `FixGateway` in global db:

	select * from FixGateway where identifier='LMAX'

	id       identifier     osAccountName
	-----------------------------------------
	83009    LMAX           ...


It contains a tag transformer to translate FIX tags if necessary:

	FixTagTransformationManager::instance().init();

The gateway connects to Cameron (the *Engine*) using socket adapter. The setting
of the engine is stored in table `FixEnginePair` in the global db:

	select * from FixEnginePair where name='LMAX'

	name    ha1Host   ha1ClusterPort   ha2Host  ha2ClusterPort  rmiPort ...
	-------------------------------------------------------------------------
	LMAX

	void GatewayMain::initSessions() {
		FixEnginePair::Ptr engine = Cache<FixEnginePair>::instance()[session->getEnginePairId()];
		m_engineConnections.push_back(new EngineConnection(*engine, *session, dialect));

## TpsSnapshotClient

It blocks on `getSnapshot`, the entire code flow may involve multiple blocking calls.



# Spot Profile Selection

## Server Side

### spot_gen/spot server

The --identifier cmdline option corresponds to FXSpotServerProfile::pricingAlgoName 
and decides the configId of the FXSpotTickWithDepth generated.


## Client Side

