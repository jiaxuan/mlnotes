# Startup

Bootstrap -> Lifecycle

# Lifecycle.java

The actual client starts with Lifecycle.main().

Internal client lifecycle steps are defined in 

java/mercuryinternal/projects/mercuryinternal-release/resources/config/application/parameters.xml


	<map module="core" name="core-init-steps">
		<element name="LOGVERSIONS"
				 value="com.ml.panther.platform.core.lifecycle.steps.LogVersionsStep"/>
		<element name="PRELOGINCOREINIT"
				 value="com.ml.panther.platform.core.lifecycle.steps.PreLoginCoreInitStep"/>
		<element name="POSTLOGINCOREINIT"
				 value="com.ml.panther.platform.core.lifecycle.steps.PostLoginCoreInitStep"/>  
		<element name="LOGIN"
				 value="com.ml.panther.platform.core.lifecycle.steps.LoginStep"/> 
		<element name="INITMODULES"
				 value="com.ml.panther.platform.core.lifecycle.steps.InitModulesStep"/> 
		<element name="INITMODULESUI"
				 value="com.ml.panther.platform.core.lifecycle.steps.InitModulesUIStep"/> 		             
		<element name="WAITMODULESTATECHANGES"
				 value="com.ml.panther.platform.core.lifecycle.steps.WaitModuleStateChanges"/> 	
		<element name="INITDONE"
				 value="com.ml.panther.platform.core.lifecycle.steps.InitDoneStep"/> 		             
		<element name="PRELOGINCOREINIT"
				 value="com.ml.panther.platform.core.lifecycle.steps.PreLoginCoreInitStep"/>
		<element name="LAFINIT"
				 value="com.ml.panther.platform.desktop.internal.lifecycle.steps.LAFInitStep"/>
		<element name="DESKTOPINIT"
				 value="com.ml.panther.platform.desktop.internal.lifecycle.steps.DesktopInitStep"/>
		<element name="SHOWDESKTOPUI"
				 value="com.ml.panther.platform.desktop.internal.lifecycle.steps.ShowDesktopUIStep"/>
		<element name="SPECINIT"
				 value="com.ml.panther.spec.lifecycle.steps.SPECRegistrationsStep"/>
	</map>

	<!-- 
	   List of initialization steps for normal startup. This variant initializes
	   the user interface, if there is one.
	-->
	<list module="core" name="core-init-sequence-normal">
		<value>PRELOGINCOREINIT</value>
		<value>SPECINIT</value>
		<value>LOGVERSIONS</value>
		<value>LAFINIT</value>
		<value>LOGIN</value>
		<value>POSTLOGINCOREINIT</value>
		<value>DESKTOPINIT</value>
		<value>INITMODULESUI</value>
		<value>SHOWDESKTOPUI</value>
		<value>WAITMODULESTATECHANGES</value>
		<value>INITDONE</value>
	</list>



For internal GUI, the menu is defined in:

java/mercuryinternal/projects/mercuryinternal-gui/resources/config/mercuryinternalgui/commands.xml


Template for menu item FX Deal/Fast Deal Montage is defined in:

java/fx/projects/fxgui/resources/templates/fxgui/InstrumentGrid.xml


## The Forward Pricing Setup Panel

main class

	class STRTForwardPricingSetupPanel
		// last selected panel
		private STRTForwardPricingPanel selectedCard;
		// cached panels
		private Map<String, STRTForwardPricingPanel> panelCache = ...;

		// model
		STRTForwardPricingMainPM pm;

		// model change listener
		PMPropertyChangeListener pmPropertyChangeListener = new PMPropertyChangeHandler();

Split pane with JTree on the left with a tree of two levels:

 - top level, currency groups, STRTForwardPricingGroupPanel
 - second level, currencies of each group, STRTForwardPricingCurrencyPanel

In addition:

 - both panel implements the STRTForwardPricingPanel interface
 - they are both containers with multiple tabs for multiple regions
 - actual implementations are ForwardPricingCurrencyPanel and ForwardPricingGroupPanel

Tree selection listener is setup as

	tree.addTreeSelectionListener(new TreeSelectionHandler());


Data for the main panel is setup as

	pm.addPropertyChangeListener(pmPropertyChangeHandler);

Group panels are created as:

	private STRTForwardPricingPanel createGroupPanel(String group) {

		// setup model
		STRTForwardPricingGroupPM groupModel = new STRTForwardPricingGroupPM(group);
		// load, this is before adding the currency models
		groupModel.load();
		groupModel.addPropertyChangeListener(pmPropertyChangeHandler);
		pm.addModel(groupModel);

		// create panel
		STRTForwardPricingPanel groupPanel = new STRTForwardPricingGroupPanel(group, groupModel);
		cardPanel.add(group, (Component) groupPanel);

		// load currency models for the group, one model per currency
		List<String> ccys = pm.getValidCurrenciesForGroup(group);
		for (String currency : ccys) {
			STRTForwardPricingCurrencyPM ccyModel = createCurrencyPanelModel(currency);
			groupModel.addCcyModel(ccyModel);        		
		}
		
		loadForwardPanelState(groupPanel);

		groupPanel.selectDefaultRegion();
		return groupPanel;
	}

	private STRTForwardPricingCurrencyPM createCurrencyPanelModel(String currency) {
		FXFwdCcyRegion r = FXFwdCcyRegions.getInstance().getRegion(currency, FXRegionUtils.findActiveRegion(currency));
		STRTForwardPricingCurrencyPM ccyModel = new STRTForwardPricingCurrencyPM(currency, r, this);
		ccyModel.addPropertyChangeListener(pmPropertyChangeHandler);
		// the currency model is added to global model as well !!!
		pm.addModel(ccyModel);
		// load !!!
		ccyModel.load();
		return ccyModel;
	}

	private STRTForwardPricingPanel createCurrencyPanel(String currency) {
		STRTForwardPricingCurrencyPM ccyModel = (STRTForwardPricingCurrencyPM) pm.getModelByItemName(currency);
		STRTForwardPricingPanel ccyPanel = new STRTForwardPricingCurrencyPanel(currency, ccyModel);
		cardPanel.add(currency, (Component) ccyPanel);
		ccyPanel.selectDefaultRegion();
		ccyPanel.fillFields();
		return ccyPanel;
	}


### Tree selection handler

	void valueChanged(...) 
		previousPanel.unsubscribeSpot();
		previousPanel.unSubscribeAllFwdPreview();

		selectedPanel.subscribeSpot();

### Model and change handler

 - For STRTForwardPricingSetupPanle, its STRTForwardPricingMainPM
 - For ForwardPricingGroupPanel, its STRTForwardPricingGroupPM
 - For ForwardPricingCurrencyPanel, its STRTForwardPricingCurrencyPM

The main model contains:

 - the underlying data representing the currency groups and their currencies of the tree view:
	ForwardPricingTreeModel treeModle
 - the model for the various groups
	List<STRTForwardPricingPM> models;

The group models are added by STRTForwardPricingMainPM::addModel when creating group panels in STRTForwardPricingSetupPanle::createGroupPanel.

Similarly, the currency models are added by STRTForwardPricingGroupPM::addCcyModel when creating group panels in STRTForwardPricingSetupPanle::createGroupPanel:

	private final String currentGroup;
	private final Set<String> regions = new LinkedHashSet<String>();

	private final Map<String, STRTForwardPricingCurrencyPM> ccyModels = new LinkedHashMap<String, STRTForwardPricingCurrencyPM>();

	private final Map<String, EventList<STRTCcyRowPM>> rowsByRegion = new HashMap<String, EventList<STRTCcyRowPM>>();

	public void addCcyModel(STRTForwardPricingCurrencyPM model) {
		ccyModels.put(model.getItemName(), model);
		model.addPropertyChangeListener(dirtyPropertyListener);

		fxFwdAmgRuleDao.getAll().addEventListListener(fxFwdAmgRuleDaoEventListener);

		for (String regionName: regions) {
			FXFwdCcyRegion region = FXFwdCcyRegions.getInstance().getRegion(model.getItemName(), regionName);
			if(region == null) {
				continue;
			}
			STRTForwardPricingPerRegionData dataPerRegion = model.getDataPerRegion(region);
			String ccyName = dataPerRegion.getRegion().getCcy();

			FXFwdAmgRule fxFwdAmgRule = getFxFwdAmgRule(ccyName);

			STRTCcyRowPM newRow = new STRTCcyRowPM(dataPerRegion, fxFwdAmgRule);
			newRow.addPropertyChangeListener(dirtyPropertyListener);
			newRow.setStale(FwdPointsStaling.getInstance().isStale(region.getCcy(), region.getRegion()));
			newRow.clearStaleDirty();
			newRow.clearThresholdDirty();
			newRow.clearThrottleDirty();
			newRow.clearCcyGlobalDirty();

			EventList<STRTCcyRowPM> rows = rowsByRegion.get(regionName);
			rows.add(newRow);
		}
	}	


Per currency, STRTForwardPricingCurrencyPanel

	public STRTForwardPricingCurrencyPanel(String currency, STRTForwardPricingCurrencyPM pm) {
		...
		this.pm = pm;
		this.currency = currency;

		regionTabs = new HashMap<String, ForwardPricingCurrencyPanel>();
		List<FXFwdCcyRegion> regions = FXFwdCcyRegions.getInstance().getRegionsByCcy(currency);

		// add regions
		for (FXFwdCcyRegion region : regions) {
			ForwardPricingCurrencyPanel regionPanel = new ForwardPricingCurrencyPanel(region, pm);
			strtTabbedPane.addTab(region.getRegion(), regionPanel);
			regionTabs.put(region.getRegion(), regionPanel);
		}
		...
	}

Per currency per region, ForwardPricingCurrencyPanel

	// region
	FXFwdCcyRegion region;
	// model
	STRTForwardPricingCurrencyPM pm;
	// blinking manager
	TableBlinkingManager blinkingManager;

	// data display
	DataObjectListProviderGridPanel thresholdsGridPanel;
	// the above is held in
	JPanel thresholdsPanel;

	// note that pm is passed in from STRTForwardPricingCurrencyPanel
	public ForwardPricingCurrencyPanel(final FXFwdCcyRegion region,
			 final STRTForwardPricingCurrencyPM pm) {
		 this.region = region;
		 this.pm = pm;
		 createUI();
		 this.pm.addPropertyChangeListener(new StreamingDataRepaintHandler());
		 this.pm.addPropertyChangeListener(new DirtyPropertyListener());
		 this.pm.addPropertyChangeListener(new OverrideModePropertyListener());
	 }

	 // the grid panel is created here
	 private JPanel getThresholdPanel() {
		thresholdsPanel = new JPanel(new BorderLayout());
		thresholdsGridPanel = new DataObjectListProviderGridPanel() {
			protected Grid createGrid() {
				Grid g =  new Grid() {
					@Override
					public String getToolTipText(int rowIndex, int columnIndex) {
						... tooltip logic goes here
					}
				};
				g.addPropertyChangeListener(...);
			}

		// grid definition is loaded here !!!
		GridDefinition gridDef = GridDefinitionLoader.getGridDefinition(
				getClass(),
				ndf ? "strt-fwd2-thresholds-ndf-grid-definition.xml"
						: "strt-fwd2-thresholds-grid-definition.xml");

		// set grid content
		thresholdsGridPanel.setContent(null,pm.getThresholdsObjectList(region), gridDef);
		...

		// setup blinking manager
		blinkingManager = new TableBlinkingManager(thresholdsGridPanel.getGrid().getMainTable());
		pm.addBlinkingManager(blinkingManager, region);
		
		for (String c : ROW_BLINKING_COLUMN_NAMES) {
			TableColumn column = thresholdsGridPanel.getGrid().getMainTable().getColumn(c);
			if (column != null) {
				TableCellRenderer columnRenderer = column.getCellRenderer();
				TableCellRenderer renderer = new RowBlinkingRenderer(columnRenderer, blinkingManager);
				thresholdsGridPanel.getGrid().getMainTable().getColumn(c).setCellRenderer(renderer);
			}
		}
		
		for (String c : CELL_BLINKING_COLUMN_NAMES) {
			TableColumn column = thresholdsGridPanel.getGrid().getMainTable().getColumn(c);
			if (column != null) {
				TableCellRenderer columnRenderer = column.getCellRenderer();
				TableCellRenderer renderer = new CellBlinkingRenderer(columnRenderer, blinkingManager);
				thresholdsGridPanel.getGrid().getMainTable().getColumn(c).setCellRenderer(renderer);
			}
		}
		...
	 }

Cell renderers (header/cell) can be customized in the grid defintion xml:

	<column-definitions>

		<column-definition name="stale" sortable="false" pref-width="40">
		</column-definition>    

		<column-definition name="tenor" editable="false" pref-width="40" min-width="20">
			<attributes>
				<font>fxforwardpricingsetup.grid.body.boldfont</font>
				<alignment>centre</alignment>                
			</attributes>
			<cell-renderer>com.ml.panther.fxgui.controls.grid.renderers.STRTThresholdTenorRenderer</cell-renderer>          
		</column-definition>

		<column-definition name="valueDate" editable="false" min-width="20" pref-width="80">
			<attributes>
				<font>fxforwardpricingsetup.grid.body.boldfont</font>
			</attributes>
		</column-definition>

		<column-definition name="primMid" editable="false" pref-width="65" min-width="20">
			<attributes>
				<font>fxforwardpricingsetup.grid.body.boldfont</font>
				<alignment>centre</alignment>   
			</attributes>
			<cell-renderer>com.ml.panther.fxgui.controls.grid.renderers.STRTRateCellRenderer</cell-renderer>
			<header-renderer>com.ml.panther.fxgui.admin.strt.renderers.MultiLineHeaderRenderer</header-renderer>
		</column-definition>
		...

Row color can be customized using coloring rules in the grid defintion xml:

	<colour-rules>
		<!-- Use a different background colour to indicate stale rows -->
		<colour-rule test-column="stale">
			<test test-operator="EQUAL" test-value="true"/>
			<fg-colour>fwdpricingsetup.stale.fg</fg-colour>
		</colour-rule>
		<colour-rule test-column="ccyStale">
			<test test-operator="EQUAL" test-value="true"/>
			<fg-colour>fwdpricingsetup.stale.fg</fg-colour>
		</colour-rule>	    	
		...



# Bubblenet

A bubblenet is a collection of connected nodes with names:

	// class BubbleNet
	private Map<String, UntypedNode<ConcurrencyPolicy>> nodes;

A node may have signal, which is basically a listener list:

	// class Node
	private Signal<T> tickSignal;

	// class Signal
	public interface Slot<T> { void signal(T value); }
	Set<Slot<T>> slots;

	public void connect(Slot<T> slot) { slots.add(slot); }
	public void disconnect(Slot<T> slot) { slots.remove(slot); }
	public void signal(T value) { for (Slot<T> slot : slots) slot.signal(value); }

A node has some internal state/value stored as its content. A node may also have 
a set of upstream nodes called causes and a set of downstream nodes called effects. 

There are two types of nodes: source and compute. A compute node has a functor to
calculate its content from the content of its upstream nodes. The nodes are added
to the bubblenet as:

	// class BubbleNet
	public SourceNode createSourceNode(String name, T initValue) {
		SourceNode node = new SourceNode(this, initValue,name);
		addNadedNode(node);
		return node;
	}
	public ComputeNode createComputeNode1(String name, Node arg0, F1 f) {
		ComputNode1 node = new ComputeNode1(this, arg0, f, name);
		arg0.addEffect(node);
		return node;
	}
	public ComputeNode createComputeNode2(String name, Node arg0, Node arg1, F2 f) {
		ComputNode2 node = new ComputeNode2(this, arg0, arg1, f, name);
		arg0.addEffect(node);
		arg1.addEffect(node);
		return node;
	}
	...

A bubblenet is driven by ticking a source node:

	// class SourceNode
	public void tick(T value) {
		getNet().getConcurrencyPolicy().sourceTick(value, this);
	}

	// class SequentialConcurrencyPolicy
	public void sourceTick(T value, SourceNode node) {
		node.doSet(value);					// set value on source node
		tickAll(node.getEffectNodes());		// tick its downstream nodes
	}

	public void tickAll(Set nodes) {
		TreeSet pendingNodes; // pending nodes are ordered by their rank
		pendingNodes.addAll(nodes);
		while (pendingNodes not empty) {
			node <- remove next node from pendingNodes
			node.tick();
			// its important to set proper rank for all the nodes
			// to ensure the proper order of ticking. the logic
			// of rank assignment is in BubbleNet.rerank(). it really
			// should be in concurrency policy.
			pendingNodes.addAll(node.getEffectNode());
		}
	}


For compute node, ticking is simple:

	// class SequentialConcurrencyPolicy
	poublic void computeTick(ComputeNode node) { node.recompute(); }

	// class ComputeNode1
	public void tick() { getNet().getConcurrencyPolicy().computeTick(this); }
	public void doRecompute() { doSet(f1.execute(arg0.doGet())); }
	// class ComputeNode2
	public void doRecompute() { doSet(f2.execute(arg0.doGet(), arg1.doGet())); }


# Forward subscription in Java TPS

For RVFXOutrightSubscription, registration is made in reload through
outright.DataTools.registerForMarketData. Forward tick data target is set to

	input.fwd.getTickReference()

	// class OutrightInput
	public FwdData fwd = new FwdData();

For AXEOutrightSubscription, if dual pricing inactive, the same as above; otherwise
a mixer is used. In either case, input.fwd is set with new tick data.

When events arrive, consistentCalculate is scheduled to run. For AXEOutrightSubscription,
its bubblenet is updated through tickSourceNode(TPSEvent event)

	private void tickSourceNode(TPSEvent event) throws DataException {
		switch (event.getEventId()) {
			case Constants.EVENT_FWD_TICK:
				if (fwdTick_sourceNode != null)
					fwdTick_sourceNode.tick(input.fwd.getTick());
				break;


# RPC

Java clients connect to gateway, the connection is called a session on both Java side and 
gateway side. 

For Java client, such a connection is represented as:

	public class PantherSession extends Session {
		PantherMessageProtocolHandler messageProtocolHandler = new PantherMessageProtocolHandler(this);
		StreamConnectionManager connectionManager; 
		SessionAuthenticator authenticator;

		OutputStream os;
		Thread writerThread;
		InputThread inputThread;

		void connect(ConnectionManager cm, SessionAuthenticator sa) {
			this.connectionManager = cm;
			this.authenticator = sa;
			...
			// call StreamConnectionManager.connect
			this.connectionManager.connect(...);
			...
			this.os = cm.getOutputStream();
			this.inputThread = new InputThread(cm.getInputStream());
			this.inputThread.start();
		}

		void send(Message message) {
			// send message down to this.os as a proper SPEC message
		}

		class InputThread extends Thread {
			...
			final byte[] signature = new byte[1];
			...
			signature[0] = // read
			ProtocolHandler handler = getProtocolHandler(signature);
			handler.handleMessage(PantherSession.this, signature, cis, bdis);
			...
		}
	}

The setup of this is in PreLoginCoreInitStep.java:

	public void initialize(Lifecycle lifecycle) throws Exception {
		...
		registerSessions(lifecycle);
		...
	private void registerSessions(Lifecycle lifecycle) throws Exception {
		Map map = Resources.getModuleParameters().getMap(
												CoreConstants.CORE_MODULE_NAME,		// core
												CoreParameters.CORE_SESSIONS,		// core-sessions
												Collections.EMPTY_MAP);
		for (Iterator iter = map.entrySet().iterator(); iter.hasNext();) {
			...
			SessionFactory.registerSession(sessionName, className);

	// from mercurial internal client's parameters.xml
	<!-- Sessions to be registered for this application -->
	<map module="core" name="core-sessions">
		<element name="panther" value="com.ml.panther.pantherclient.protocol.PantherSession"/>
	</map>


So incoming SPEC messages are dispatched based on the first byte of a SPEC message, which 
has the following format:

	+-------------+---------------+-------------+
	| ContentType | ContentLength | ContentData |
	+-------------+---------------+-------------+


For Java, the content type definitions are in:

	// com.ml.panther.platform.server.serialize.protocol.ProtocolConstants
	public final class ProtocolConstants {
		public static final byte MSG_PANTHER_SPOT_RATES_PROTOCOL = 16;
		public static final byte MSG_PANTHER_FWD_RATES_PROTOCOL = 17;
		public static final byte MSG_FX_SPOTTICKDEPTH_PROTOCOL = (byte)0x13;

For C++, the definitions are in:

	// libs/panther/serialize/protocol/ProtocolConstants.hpp
	class ProtocolConstants {
		static const unsigned char MSG_PantherProtocol				= 0x00;
		...
		static const unsigned char MSG_FXSpotRateProtocol			= 0x10;
		static const unsigned char MSG_FXFwdRateProtocol			= 0x11;
		static const unsigned char MSG_FXSpotRateWithDeptProtocol	= 0x12;
		static const unsigned char MSG_FXSpotTickDepthProtocol		= 0x13;
		static const unsigned char MSG_FXSkewProtocol				= 0x14;
		...


When forward server sends fwd ticks, the marshaller sets up the proper content type:

	FXFwdRateMarshaller::FXFwdRateMarshaller()
	: ml::panther::serialize::protocol::rates::FXRateMarshaller(
		PROTOCOL_NAME,
		ml::panther::serialize::protocol::rates::defaults::DEFAULT_TICK_SIZE,
		ProtocolConstants::MSG_FXFwdRateProtocol) {}

For Panthe SPEC method invokation, messages are sent with content type MSG_PantherProtocol.
This message has in its header Topic and ReplyTopic. ReplyTopic is set only for methods
that return responses. The header also contains an optional credential header of the
format (principalType, principalId, appId), which is used by the gateway to associate
a message with its originating session. 

For instance, when Java client invokes 

	int FXFwdTickSnapshotProvider::subscribeFwdSnapshot(...)

The original Java SPEC message will have 

	Topic: FXFwdTickSnapshotProvider
	ReplyTopic: 1407831878986
	Payload: Encoding of the contract call: name, parameters etc.

After setting the credential header of the message to that of the originating session,
the gateway sends it out using tibrv with:

	RVSubject: rv_ps.FXFwdTickSnapshotProvider
	RVReplySubject: rv_ps.:sglsgfxapd5.sg.ml.com:50:
	Payload: the original SPEC message with modified credential header

The gateway has special logic for SPEC topic LoginContract and LogoutContract.
The forward server receives this and sends a response:

	RVSubject: rv_ps.:sglsgfxapd5.sg.ml.com:50:
	Payload: the SPEC message response with the same credential header of the request

The gateway sets up MessagingListener to intercept these messages:

	boost::shared_ptr<RouterCallback> routerCallback((RouterCallback*)manager.getMessagingListener());

	Routers::instance().addSubscriber(routerCallback, publicForwardingTopic, 1);
	Routers::instance().addSubscriber(routerCallback, privateP2PTopic, 1);
	Routers::instance().addSubscriber(routerCallback, privateLocalForwardingTopic, 1);
	Routers::instance().addSubscriber(routerCallback, privateGlobalForwardingTopic, 1);

The gateway uses the credential header of the SPEC message to match an active
session, if found, send the SPEC message to the session. The client side of Java code
is implemented as:

	// AbstractBinarySPECClientContractProxy.java
	public InputStream sendSPECContractCallWithReturn(String contractName,
													  String methodName,
													  int numArgs,
													  byte[] encodedParameters,
													  BoundedInputStream bdis) ...
		byte[] encodedCall = getEncodedContractCall(methodName, numArgs,
													encodedParameters);
		Payload payload = new Payload(encodedCall);
		// the unique reply topic is generated by PantherSession
		Message message = new Message(ProtocolConstants.MSG_PANTHER_PROTOCOL,
									  contractName,
									  getSession().getUniqueReplyTopic(),
									  payload);
		applyServerCredential(message);
		applyMessagingTopic(message);
		applyRoutingKey(message);
		// this calls Session.sendAndWait
		byte[] response = getSession().getPublisher().sendAndWait(message, false);
		// checks that rsp msg type is FMT_ContractResponse and its methodName matches the request
		checkContractResponse(bdis, response, methodName, true);
		return bdis;

	// Session.java
	byte[] sendAndWait(Publisher publisher, Message message, boolean priority)
		String replyTopic = message.getReplyTopic();
		Subscriber subscriber = getSubscriber(replyTopic, null);
		ResponseTopicListener tl = new ResponseTopicListener();
		subscriber.addTopicListener(tl);

		send(message, priority);

		long remaining = getResponseWaitTime(message);
		while (remaining > 0 && tl.getMessage() == null) {
			tl.wait(remaining);
			...
		}
		subscriber.removeTopicListener(tl);
		removeSubscriber(replyTopic, null);

		if (tl.getMessage() != null)
			return tl.getMessage().getPayload().getMessage();


The logic to match SPEC content type with a handler is in Session:

	Session {
		ProtocolHandler getProtocolHandler(byte[] signature) {
			for (ProtocolHandler h : protocolHandlers)
				if (h.handlesProtocol(this, signature))
					return h;
			return null;
		}
		// protocol handlers are added via this.
		// TPSManager uses
		//		session.registerProtocolHandler(new FXPricingProtocolHandler());
		//
		// FXTraderGUIModule uses
		//		session.registerProtocolHandler(new FXAggregatedQuoteProtocolHandler());
		//
		void registerProtocolHandler(...) {}
	}

This is what TPSManager uses to setup protocol handlers:

	FXPricingProtocolHandler() {
		AbstractSPECObjectBroker spotTickBroker;
		AbstractSPECObjectBroker fwdTickBroker;
		...

		FXPricingProtocolHandler() {
			... factory = SPECContractManager.getInstance().getFactory();
			spotTickBroker = factory.getSPECObjectBroker("FXSpotTick");
			fwdTickBroker = factory.getSPECObjectBroker("FXFwdTick");
			...
		}

		boolean handlesProtocol(Session session, byte[] sig) {
			return sig.length == 1 &&
			(sig[0] == FX_FWD_RATES_PROTOCOL ||
			 sig[0] == FX_SPOT_RATES_PROTOCOL ...);
		}

		void handleMessageImpl(Session session, byte[] sig, InputStream is, BoundedInputStream bdis) {
			switch(sig[0]) {
				case FX_FWD_RATES_PROTOCOL:
					...
					FXFwdTick tick = (FXFwdTick)fwdTickBroker.decode(is, factory);
					...
			}
		}
	}

Spot is setup in:

	TPSManager {
		void initialise() {
			SPECContractManager.getInstance().registerSPECServerContractHandler(
					FXSpotTickListener.class, SpotTickListener.class);

			SPECContractManager.getInstance().registerSPECServerContractHandler(
					FXSpotTickWithDepthListener.class,
					SpotTickWithDepthListener.class);
			
			// Skew listener implementation
			SPECContractManager.getInstance().registerSPECServerContractHandler(
					FXSkewTickListener.class, SkewTickListener.class);

		}
	}

Foward is setup in

	class ForwardClient {
		static {
			SPECContractManager.getInstance().registerSPECServerContractHandler(
					FXFwdTickListener.class, FwdTickListener.class);
		}

		public static class FwdTickListener implements FXFwdTickListener {

			private boolean isPreview(FXFwdTick tick) {
				return tick.getType() == FXFwdTick.TickType.PREVIEW_TICK;
			}

			@Override
			public void onFwdTick(FXFwdTick tick) {

				if (isPreview(tick)) {
					ForwardClient.getInstance().onFwdTickPreview(tick);
				} else {
					ForwardClient.getInstance().onFwdTick(tick);
				}
			}
		}
	}

For Java server side, there is no need to go through the gateway and the C++ impl of Routers
is used through JNI. Such a connection is represented as:

	public class ServerSession extends Session {
		MessageRouter router;
		ContractTopicsManager topics;

		void send(Message message) {
			MessageDetails details = ProtocolUtils.extract(message);
			router.send(details, 0);
		}

		void messageReceived(Message m) {
			RoutingKey routingKey = RoutingKey.decode(m.getSenderBlob());
			onMessage(m, routingKey.getRoutingKey());
		}
	}

	Session {
		void onMessage(Message message, String routingKey) {
			String topic = message.getTopic();
			// find subscriber by topic and routingKey
			subscriber.onMessage(message);
		}
	}

The setup is done in ServerUtils.java:

	//Wires up the SPEC framework, Adds mappings for all contracts in
	// - client-model and 
	// - fx-model
	public static synchronized ServiceFactory initSPEC() {

		SPECContractManager specContractMgr = SPECContractManager.getInstance();
		ServiceFactory sf = getServiceFactory();

		// use ServerSession for session
		ServerSession serverSess = new ServerSession(getRegistry());
		specContractMgr.setDefaultSession(serverSess);

		// use BinaryEncoderDecoderFactory for codec
		EncoderDecoderFactory.registerEncoding("binary", BinaryEncoderDecoderFactory.class.getName());
		specContractMgr.setDefaultFactory(EncoderDecoderFactory.getFactory("binary"));

		// use BaseMessageHandler as handler
		BaseMessageHandler bmh = new BaseMessageHandler(specContractMgr);
		MessageRouter messageRouter = sf.get(MessageRouter.class);
		messageRouter.addListener(bmh);

		ContractTopicsManager contractTopics = sf.get(ContractTopicsManager.class);

		// Get the environment we are running in
		EnvironmentSettings es = new EnvironmentSettings(getRegistry());

		ProtocolUtils.setContractTopics(contractTopics);
		ProtocolUtils.setEnvironmentId(es.getEnvironmentId());

		// Pop the services onto the session
		serverSess.setRouter(messageRouter);
		serverSess.setTopics(contractTopics);

		// setup mapping between contract names and their actual implementations (class names)
		srs.registerSPECMappings((SPECRegistry) Class.forName("config.pantherclient.SPECRegistrationsImpl").newInstance(),
								 specContractMgr.getDefaultFactory());
		srs.registerSPECMappings((SPECRegistry) Class.forName("config.fxmodel.SPECRegistrationsImpl").newInstance(),
								 specContractMgr.getDefaultFactory());
		
		return serviceFactory;
	}

	// config.pantherclient.SPECRegistrationsImpl and config.fxmodel.SPECRegistrationsImpl are auto-generated
	public class SPECRegistrationsImpl implements SPECRegistry {
		public Map<String, String> getSPECObjectBrokerClassMap() {
			Map<String, String> map = new HashMap<String, String>();
			map.put("ExecutionInstructionRequest", 
					/* __CLS__ */ "com.ml.panther.mda.execution.binarybroker.ExecutionInstructionRequestBroker");
			...
			return map;
		}

		public Map<String, String> getSPECContractServerProxyMap() {
			Map<String, String> map = new HashMap<String, String>();
			map.put(/* __CLS__ */ "com.ml.panther.mda.FXMDAOrderContract",
					/* __CLS__ */ "com.ml.panther.mda.proxy.FXMDAOrderContractServerProxy");
			...
			return map;
		}

		Map<String, String> map = new HashMap<String, String>();
			map.put(/* __CLS__ */ "com.ml.panther.mda.FXMDAOrderContract",
					/* __CLS__ */ "com.ml.panther.mda.proxy.FXMDAOrderContractClientProxy");
			...
			return map;
		}

		public Map<String, String[]> getSPECObjectHandlerClassMap() {
			Map<String, String[]> map = new HashMap<String, String[]>();
			map.put("ExecutionInstructionRequest",
					new String[] {
						/* __CLS__ */ "com.ml.panther.mda.execution.ExecutionInstructionRequest",
						/* __CLS__ */ "com.ml.panther.mda.execution.handler.ExecutionInstructionRequestHandler",
					});
			...
			return map;
		}

		public Map<String, String> getSPECObjectManagerClassMap() {
			Map<String, String> map = new HashMap<String, String>();
			map.put(/* __CLS__ */ "com.ml.panther.pantherclient.dataobjects.identity.Channel",
					/* __CLS__ */ "com.ml.panther.pantherclient.dataobjects.identity.manager.ChannelManager");
			...
			return map;
		}
	}






