## Given a trade id, find out BFGW and Cameron instances

In localdb:

	select * from FXSpotForwardTrade where id=[tradeid]

the record would contain [userId], which is basically PantherUser::id, 

 - PantherUser::id (globaldb) -> ExternalEntity::principalId
 - ExternalEntity::externalId -> FixID::id
 - FixID::compId -> FixSession::name
 - FixSession:applicationId -> FixApplication::id

Then:

	select * from FixApplication where id in (select applicationId from FixSession where name in (select compId from FixID where id in (select externalId from ExternalEntity where principalId=44836007)))

FixApplication::name is BFGW identifier and Cameron session name:

	ww | grep [name]

If there are multiple instances, use wmg to find out which is the master


## What is CnR ?

See bana_fix_gateway/client/EngineConnection constructor

## How Throttle Works ?

## What is Fast Market ?

## How Credit Checking Works ?

## How is broken date handled ?

First, it needs to have the right format. A broken date has the format 
of YYYYMMDD, the checking is done as:

	bool Tenor::isBrokenDate( const std::string & isoDate )
		return isoDate.size() >= 8;

The logic of validation is simple:

	try {
		... = Date(tenor.str(), Time::ISO_YYYYMMDD, Time::UTC);
	} catch (...) {
		// invalid format
	}

Then check if it is a working day, which is currency pair specific:

	Calendar::isWorkingDay(ccyPair, date)

Try to map it to a standard tenor:

	tenors = Calendar::getSurroundingTenors(m_input.ccyPair, requestDate, m_input.tradingDay);


## How are outright prices calculated ?

From FXPricingUtils.java:

	calculateOutright
	Input: spotBid, spotAsk, fwdBidPnt, fwdAskPnt, pipMultiplier

	For TDY, forward points for TOM are also required:

	TDY: 
		bid = spotBid - (fwdAsk + fwdTNAsk) * pipMultiplier
		ask = spotAsk - (fwdBid + fwdTNBid) * pipMultiplier

	TOM:
		bid = spotBid - fwdAsk * pipMultiplier
		ask = spotAsk - fwdBid * pipMultiplier

	SPOT:
		bid = spotBid
		ask = spotAsk

	POST SPOT:
		bid = spotBid + fwdAsk * pipMultiplier
		ask = spotAsk + fwdBid * pipMultiplier

Question: Why the special logic for TDY and TOM ? 

1. By convention, forward points for TDY (ON) is for over-night, between TDY and TOM, 
   instead of TDY and SP.
2. The forward points for TDY and TOM have the same sign as post-spot points, even
   though they should be in opposite direction. So whey applying them, unlike normal
   post-spot points, we need to subtract them from instead of add them to spot rate.


## Pricing

Trader rate may differ from sales rate on

- spot
- forward points

how are these difference configured/used ?


## Rounding


## How does bana_fix_gateway choose account ?

Take RTF as an example, for sender

	compId: DEV-BANAFIX0-QTS
	subId:

First try to find FixID by an exact match on compId/subId/locId/requestType:

	select * from FixID where compId='DEV-BANAFIX0-QTS' and requestType=0

	id			compId				subId	locId	onBehalfOfRequired	permissions	pricingFIXId	requestType
	20777002	DEV-BANAFIX0-QTS					N					3							0

For normal request (requestType=0), this finds one match. For market order (requestType=2), this 
does not match anything, so a match on the group is performed:

	select groupName from FixSessionGroup where compId='DEV-BANAFIX0-QTS'

	DEV-BANAFIX0

	select * from FixID where compId='DEV-BANAFIX0-QTS' and requestType=2

	id			compId				subId	locId	onBehalfOfRequired	permissions	pricingFIXId	requestType
	20777008	DEV-BANAFIX0						N					3							2

Get ExternalEntity from FixID:

	select * from ExternalEntity where externalId=20777008 or externalId=20777002

	id			externalId	externalType	principalId	principalType
	27335004	20777002	1				47882032	0
	27335010	20777008	1				47882033	0

	select * from PantherUser where id=47882032 or id=47882033

	id			userName			fullName					userType	loginUserName
	47882032	DEV-BANAFIX0		banafix TestUser (0)		2			DEV-BANAFIX0
	47882033	DEV-BANAFIX0_rfs	banafix RFS TestUser (0)	2			DEV-BANAFIX0_rfs


## How to decide if a currency pair is a cross ?

For spot, check table FXCcyPairCrosses.
For forward, check table FXCcyPairFwdCrosses

If an entry is found and crossCcy is not null, it is a cross. If crossCcy is null, it is not.

If an entry is not found, it is crossed by USD.

See tps DataProvider.cpp getCrossCurrency.

## Sides

## Calendar

## Permissions


## How are ROE or LVN decided

By currency pair type: CurrencyPair::[ ROE || LVN_SELL || LVN_BUY_SELL ]

## How is Precious Metal decided

By currency pair type: CurrencyPair::[ METAL_LONDON || METAL_ZURICH ]

## How is NDF decided

By currency pair type: CurrencyPair::NDF


## What is DAP
DAP is the **credit checking engine** used by the Bank. Executed deals are fed into DAP post-execution by the FXTransact/MLFX credit check servers (not by the STP servers).

## What is Seawolf / PTE (Post-Trade Engine)

Seawolf is a middleware component that takes XML tickets generated by upstream systems and enriches them with extra information that is required to interface the deal into PTE for processing.

The main functions of PTE are as follows:

- Determines the trading books that the executed deals need to feed into
- Sends STP acknowledgements back to MLFX (via Seawolf)

## What is Sierra

Sierra is the **risk management system** for all FX Cash products, i.e. Spot, Forward and Swaps.
It provides a counterparty static data feed to MLFX.


## What is Grasshopper

Grasshopper is the in-house Client STP solution for customers trading on the Bank’s ecommerce platforms. Grasshopper receives its deals directly from PTE (rather than Sierra).

## What is Trident

This is the component that deals with STP outbound from Sierra, which includes the cancel and amend workflow relevant to MLFX

## What is Altair

Altair is the risk management system for all vanilla GFX Options and some exotic products.

## What is RAM

RAM is the risk management system for Equities products.
