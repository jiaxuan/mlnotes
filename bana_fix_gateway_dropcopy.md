
<!-- ========================================================================================= -->

# Existing 

## DropCopyRTNSSession

	dropcopy::MFXTradeSenderPtr m_spMFXTradesRecovery;	// history snapshot
	dropcopy::BlotterListenerPtr m_spBlotterListener; 	// listen for blotter updates

	DropCopyProcessor::Ptr m_spDropCopyProcessor;
	DropCopyBlotterProcessor::Ptr m_dropCopyBlotterProcessorPtr;

	DropCopyTridentListener::Ptr m_dropCopyTridentListenerPtr;
	dropcopy::TridentTradeSenderPtr m_spTridentTradesRecovery;


## Init

On gaining mastership:

	void DropCopyRTNSSession::onGainMastership() const
		// start FXBlotterOutright listener ASAP (we can request 'past' trades only for today)
		m_spBlotterListener->start();
		// start trades processing (FXBlotterOutright/cache updates) even before establishing FIX connection to Client
		// trade infos will be just persisted and after Logon will be send to Client
		m_dropCopyBlotterProcessorPtr->init();
		m_dropCopyTridentListenerPtr->init();
		m_dropCopyBlotterProcessorPtr->start() {
			start a new thread to run getTradesAndProcess
		}

On FIX logon

	void DropCopyRTNSSession::preProcessLogon(const FixMessage_ptr &message) {
		LINFO("Got logon response from RTNS");
		scheduleAllowSendingDCMessages(m_tmDelayDCAfterLogon) {
			schedule DropCopyRTNSSession::allowSendingDCMessages
		}


	void DropCopyRTNSSession::allowSendingDCMessages()
		controlExecutionReportFlow(true);
		LINFO("Starting recovery for FixSession %lld", getFixSession().getId());
		// stop normal flow (if it is already started) before recovery
		m_dropCopyBlotterProcessorPtr->stop();	// note: we will accumulate coming FXBlotterOutrights
		m_spMFXTradesRecovery->recovery();			///@TODO when we should stop it? (if disconnected)
		m_spTridentTradesRecovery->recovery();	///@TODO when we should stop it? (if disconnected)
		m_dropCopyBlotterProcessorPtr->start();	// start normal flow only after recovery complete

		ml::common::application::Application::instance().setReadyToService();

## Recovery

### MFX

	void MFXTradeSender::recovery()
		// stuff from db
		Trades notSentTradesFromDB;
		getTradesFromDB(notSentTradesFromDB);
		sendTrades(notSentTradesFromDB) {
			basically calls DropCopyBlotterProcessor::sendAndPersist
		}
		// stuff from snapshot
		processTradesFromBlotterSnapshot() {
			get snapshot
			DropCopyBlotterProcessor::onBlotterTrades(...)
		}

	void MFXTradeSender::processTradesFromBlotterSnapshot()
		get the latest entry from db
		if not found return
		set snapshot starting time based on timestamp of the entry
		requestPastTrades(startingTime) {
			/PROCESS/trades_req_tcp_port_os  (tcp port offset)
			/PROCESS/trades_req_timeout_ms   (response timeout)
			calls BlotterTradeClient::getTradesByPrincipalId(...)

			MFXTradeSender::onBlotterTradeTimeout: add empty data to m_bltData
			MFXTradeSender::onBlotterTrades: add data to m_bltData
		}
		WorkQueue<FXBlotterOutright>::m_bltData.get(bltData) // this blocks
		// filter out those already captured in db
		MFXOverlapFilter::removePastTradesFromSnapshot(bltData) {
			foreach trade : bltData
				// for swap, all legs have same tradeId/tradeVersion
				vector<DCMFXTrade> lst <- getFromDB(trade.tradeId)
				if lst.empty()
					next
				if trade.state == EXECUTED || trade.state == CONFIRMED
					remove // all entries in db already EXECUTED or CONFIRMED ?
				foreach i : lst
					if i.version == trade.version 
						remove
				next

			in summary, it will be processed if:
			1. theres no entry with the same tradeId in db
			2. its not EXECUTED or CONFIRMED and its version diff from those in db
		}
		// filter out those also received over streaming
		MFXOverlapFilter::removeStreamedDataFromSnapshot(bltData) {
			map.clear()
			foreach i : m_streamQueue
				map <- i by (tradeId, tradeVersion)
			foreach i : bltData
				if in map remove it

			// this process moves streamed data to the filter
		}
		get streamed data from MFXOverlapFilter and send by DropCopyBlotterProcessor::onBlotterTrades(..., false)
		send bltData by DropCopyBlotterProcessor::onBlotterTrades(..., true)

Notes:

- Snapshots are provided by fx_blotter: see fx_blotter/snapshot/SnapshotServer.cpp

### Trident
	
	void TridentTradeSender::recovery()
		Trades notSentTradesFromDB;
		getTradesFromDB(notSentTradesFromDB);
		// just send all 'isSent=N && isHeld=N' trades from db (no need to do deduplication/overlapping check)
		sendTrades(notSentTradesFromDB) {
			calls DropCopyTridentListener to process each message
			basically the same as DropCopyTridentListener::onMessage
		}

Notes:

- fx_trident_listener receives data from Darwin and saves it in FXDropCopyTridentTrade for
  drop copy to use. No one else uses the table.

## Data Processing

### MFX

	DropCopyBlotterProcessor::onBlotterTrade(
		FXBlotterOutright::Ptr trade,
		FXBlotterOutright::PtrList& unmatchedNear,
		FXBlotterOutright::PtrList& unmatchedFar,
		std::map<String, FXBlotterOutright::PtrList>& unmatchedOption,
		ml::common::concurrent::Mutex& mutex,
		bool useSnapshotMode)

		// ignore fxt trades
		if trade.getSource == SOURCE_FXTRANSACT return
		// ignore if aggregated
		if trade.getAggregationStatus == AGG_PARENT return
		// ignore allocation parent
		if trade.getIsAllocationParent == true return

		// check number of option trades(legs and hedge)
		if (trade.getOptionTradesCount == 0) { // non-option

			// check aggreate state
			if trade.getTradeAggState in [AGG_STATE_HOLDABLE, AGG_STATE_HELD, AGG_STATE_LOCKED]
				return
			
			if trade.getParentAggId().isNull() && trade.getTradeAggState in 
				[AGG_STATE_AGGREGATED, AGG_STATE_RELEASED, AGG_STATE_UNHELD] &&
				trade.getState not_in [CONFIRMED, AMENDED, CANCELLED]
				return

			// if from order book, need to be final
			if trade.getSource in [SOURCE_ORDER_FILL_MANUAL, SOURCE_ORDER_FILL_AUTO] and
			   trade.getState not_in [CONFIRMED, AMENDED, CANCELLED]
			   return

			if (trade.getState == EXECUTED && trade.getTradeState == TRADE_EXECUTED or
				trade.getState == CANCELLED or
				trade.getState == AMENDED) {
				goto doIt
			} else {
				if (!snapshotMode)
					return

				if (trade.getState in [CONFIRMED, ALLOCATED, SETTLED])
					goto doIt
				return
			}
		doIt:
			// check FXDropCopyDisabledProduct to get account specific settings
			// on what products are disabled for drop copy
			bool NoNDF,NoSPOT,NoOUTRIGHT,NoSWAP,NoVANILLA,NoMULTILEG = false;
			string mnemonic = m_dropCopyProcessorPtr->getMnemonicFromTrade(trade...) {

				// check if special mnemonic for precious metal
				if (trade.ccy1 is PreciousMetal or trade.ccy2 is PreciousMetal) and
				   accnt <- BOSettlementAccount[trade.boAccountId] and
				   CDSSierraMnemonics[accnt] notnull
				   return CSSierraMnemonics[accnt]
				if trade.shortNameOverride set, return trade.shortNameOverride
				return trade.settlementAccountInfo.boAccName
			}
			if !mnemonic.empty()
				// set all the flags based on FXDropCopyDisabledProduct
				processDisabledProduct(NoNDF, NoSPOT, NoOUTRIGHT, NoSWAP, NoVANILLA, NoMULTILEG, mnemonic);

			switch (trade->getLegType())
			case NDF && !NoNDF:
			case SPOT && !NoSpot:
			case OUTRIGHT && !NoOutright:
				filterCreateSendAndPersist(trade)
				break

			// for SWAP, need both legs
			case SWAP_NEAR && !NoSwap
				far = findInAndDelFromList(trade.tradeId, unmatchedFar)
				if far
					filterCreateSendAndPersist(far, trade) // bug ?
				else
					unmatchedNear.push_back(trade)
			case SWAP_FAR && !NoSwap
				near = findInAndDelFromList(trade.tradeId, unmatchedNear)
				if near
					filterCreateSendAndPersist(near, trade)
				else
					unmatchedFar.push_back(trade)

		} else { // option



		}


	void DropCopyBlotterProcessor::filterCreateSendAndPesist(
		FXBlotterOutright::Ptr& trade, FXBlotterOutright::Ptr& farTrade)
		int64 nBankId;
		if (! isNeedToSend(trade, nBankId))
			return;
		
		DCMFXTradePtr trade2 = farTrade ? 
			boost::make_shared<DCMFXTrade >(trade, farTrade, fixSessionId) : 
			boost::make_shared<DCMFXTrade >(trade, fixSessionId);

		// fixes issue when cancel not sent for amend by blotter. This is done only for streaming blotter
		// this will generate a cancel for amend that doesnt have a matching cancel
		applyCancelForAmendFix(trade2, nBankId);

		// NOTE: PERSISTED TWICE
		trade2->persist();		// persist as not_sent first
		sendAndPersist(trade2, nBankId);
	}



	DropCopyBlotterProcessor::isNeedToSend(trade, bankId)
		if filter == null return false
		return filter.isNeedToSend(trade, bankId)

	MFXTradeFilter::isNeedToSend(trade, bankId) 
		1. if trade.holdTradeInBackoffice == true return false

		2. check dropycopy trident account settings

		if trade.shortNameOverride != null {

			// List of non-MFX Accounts for which we need DropCopy. 
			// For non-MFX trades (done not through MFX system, e.g. voice trades), filtering is based on info
			// from TridentOutbound xml. Note that only 'not held' trades will be processed 
			// (trades with <TridentOutbound><trade><heldFlag>N</heldFlag>)

			class FXDropCopyTridentAccount version 1.0
				// for search, taken from <TridentOutbound><trade> first <Leg><bankAcctShortName> 
				// (also known as 'Sierra mnemonic')
				bankAcctShortName varchar(100);

				// for search, taken from <TridentOutbound><trade><externalSourceSystem> 
				// (also known as 'Channel' - 'ANY' matches all)
				externalSourceSystem varchar(100);

				// for search, taken from <TridentOutbound><trade><brokerCode> 
				// (also known as 'brokerCode' - 'ANY' matches all)
				brokerCode varchar(100);

				// link to session through which we will DropCopy trades for this account
				session int64 foreign key FXDropCopySession(id);
				
				// link to related MFX bank for this record (used for FIX counterparty population)
				bankId int64 foreign key Bank(id);

			accnt <- FXDropCopyTridentAccount[bankAcctShortName = trade.shortNameOverride]

			bankAcctShortName externalSourceSystem session bankId 
			----------------- -------------------- ------- ------ 
			SCXL-VTI          ANY                  1       1      
			GAZPMLX-LON       ANY                  1       2      
			SOGP-CUR          ANY                  1       2      
			NYYDF-CPH         ANY                  3007    2      

			dcsess <- FXDropCopySession[accnt->session]

			id   clientType fixSessionId name       
			---- ---------- ------------ ---------- 
			1    0          2314002      RTNS       
			3007 1          2314003      LOGICSCOPE 


			id      name               logonFixID 
			------- ------------------ ---------- 
			2314002 RTNS-DropCopy      12372005   
			2314003 Logiscope-DropCopy 12111009 

			if dcsess.fixSessionId == getFixSessionId
				nBankId = accnt.bankId
				return true
		}

		3. check regular dropcopy account

		accntInf <- trade.settlementAccountInfo
		accntInf == null: return false

		accnt <- FXDropCopyAccount[accntInf.name, accntInf.groupId]
		if accnt == null: return false

		dcsess <- FXDropCopySession[accnt.session]
		if dcsess.fixSessionId != getFixSessionId
			return false
		nBankId = trade.settlementAccountInfo.bankId
		return true


	// if its an cancel, record its trade id, if its an amend, 
	// match it against recorded cancels, if not found, generate a cancel
	DropCopyBlotterProcessor::applyCancelForAmendFix(DCMFXTrade trade, int64 bankId)
		// if its cancel, record it
		if trade.leg1.state == CANCELLED
			m_cancelIds.insert(trade.tradeId)
			return

		if trade.leg1.state != AMENDED
			return

		// an amend
		if trade.tradeId in m_cancelIds
			remove trade.tradeId from m_cancelIds
			return

		// get from db
		DCMFXTrade origTrade <- db[tradeId]
		if origTrade.isSwap()
			cancel <- DCMFXTrade(origTrade.leg1, origTrade.leg2, fixSessionId)
		else
			cancel <- DCMFXTrade(origTrade.leg1, fixSessionId)
		cancel.leg1.state <- CANCELLED
		cancel.persist()
		sendAndPersist(cancel, bankId)


	void DropCopyBlotterProcessor::sendAndPersist(DCMFXTradePtr trade, int64 bankId)
		bool created = isSent = false;

		// DCMFXTrade previous trade is inited in constructor by:
    	// if m_spTradeNotification1->getState() in [AMENDED, CANCELLED]
    	//		m_spPreviousTrade = DCMFXTrade::getLastTrade(m_spTradeNotification1->getTradeId())

        FXBlotterOutright prevTrade;
		if trade->getPreviousTrade()
			prevTrade = trade->getPreviousTrade()->getLeg1();

		if (trade->getLeg2())
			isSent = DropyCopyProcessor::onDropCopyTradesSWAP(trade.leg1, trade.leg2, prevTrade, bankId, created);
		else
			isSent = DropyCopyProcessor::onDropCopyTradesSPOTFWD(trade.leg1, prevTrade, bankId, created);

		if (isSent)
			trade->setSent();
		else if (! created)
			// there is some error with trade - mark it as invalid (so we won't try to resend it again later)
			trade->invalidate();

		trade->persist();


### Trident

<!-- ========================================================================================= -->

# Info

- All legs of Swap have the same tradeId and tradeVersion

## Ignored Trades

- FXT trades are ignored, trade.getSource == SOURCE_FXTRANSACT
- Aggregated, trade.getAggregationStatus == AGG_PARENT
- Parent of allocated trades, trade.getIsAllocationParent == true
- Spot, with aggreate state AGG_STATE_HOLDABLE or AGG_STATE_HELD or AGG_STATE_LOCKED

<!-- ========================================================================================= -->

# New Design

Completely separate the incoming data flow and the sending process

## Incoming data:

- snapshot

- streaming data
  * MFX
	* through 
	* serialized as FXDropCopyMFXTrade
  * Trident
	* through cache update notification
	* received as ml::spec::fx::trade::dropcopy::FXDropCopyTridentTrade::Cache::Event
	* serialized as FXDropCopyTridentTrade by fx_trident_listener

## Working set:

- insertion
- status update
- caching
- persistence

Special handling for trident

- on startup, load all proper entries from FXDropCopyTridentTrade, add them to working set
  and mark them as sent

## Helper

- turn working set data to execution report
- trun FXDropCopyTridentTrade to working set data

## Notes

- Use execution report ACK to confirm delivery
- FXDropCopyTridentTrade updates are handled by adding it to working set and marking it as sent
- As a preventive measure, also do periodic scanning of FXDropCopyTridentTrade table for unsent
  entries, do this as part of FXDropCopyTridentTradeUpdateListener::onSubscriptionCacheUpdate
- the meaning of isSent in FXDropCopyTridentTrade simply means it has been added to working set
- on gaining mastership, the sender should clear its sending buffer and reload from db or
  the sender should clear its sending buffer on losing mastership
- benchmark speed
