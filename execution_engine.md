## Init

Exchange names are read from table ***ExecConfig***:

- Exchanges

Whether an exchange is GUS depends on if it appears in these in ***ExecConfig***:

- GUSMarkets
- GUSRates

which are lists of Market::mCode.

For each exchange, its settings are again saved in ***ExecConfig*** as

	<market_name>-<config_name>

Such as:

	GUS_LN-fixBasedMDA
	GUS_LN-heartbeatStyle


Virtual currencies are loaded from table ***VirtualCurrency***:

	ccy1	ccy2	parentCcyPair	name
	USD		A01		USDAR0			ARS 1M
	USD		CS1		USDCN0			CNY 1MS

Execution routing rules are loaded from ***ExecutionRouting***

	class ExecutionRouting version 1.0
		// account (user id)
		account varchar(50) readOnly;

		// the market code from Market
		mktCode int32 foreign key Market(mCode);

The setup:

	agg1 ----+
	         |                                   +-- ExchangeOrderContract
	agg2 ----+--- ExecutionContract   xchange1 --+-- ExchangeOrderCallback
	         |                                   +-- MDAQuote(WithDepth)Callback
	agg3 ----+                        ...

## Exchanges

## Instruction Tracker

## Aggregated Pricer

## DAO

- instructions (ExecutionInstruction, ExecInstructionParam)
- orders (ExchangeOrder)
- deals (ExchangeDeal)
- processed messages (ProcessedMDAMessage)

## Workflow

