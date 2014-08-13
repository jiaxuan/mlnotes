# Background

From UIEP_CreditCheck_v0.1.pdf

In the old Merrill Lynch world, the following things apply:

- MLFX used to be the sole platform for Merill Lynch, i.e. MLB entity trading
- The counterparty reference system for all of Merrill Lynch is based upon CoPeR IDs
- The credit checking system is DAP
  * However, a request/response approach to credit checking with DAP was very slow
  * To remediate this, MLFX implemented its own credit server that cached credit data from DAP to allow credit checking to be much quicker
  * MLFX then updates its credit data cache with intraday updates from DAP (every 30mins)

In the old Bank Of America world, the following things used to apply:

- FXTransact was the sole platform for retail and institutional business
- The counterparty reference system for all of the Bank Of America was based upon GCI IDs
- The credit check system is ICE

As part of the unification of the two banks, a decision had been made to retire the ICE credit check system and integrate DAP into the FXTransact system architecture.

The ICE Retirement project at a high level had the following objectives:

- Updating all the Bank Of America counterparties such that they had a CoPeR ID
  * As this is the static data source for DAP)
- Implementing all the credit check functionality that ICE used to do, but are currently missing in DAP
- Make the request/response approach to credit checking faster

As far as the UIEP Programme was concerned, the strategy was to:

- Wait for the ICE retirement project
- Re-engineer MLFX connectivity to DAP in a request/response model and decommission the MLFX credit server


# Overview

Credit Server is a process that runs to ensure all the credit checks for a trade.
The credit checks include:

1. Exposure Checks (PE Checks)
2. Settlement Limit Checks (MDD Checks)
3. Tenor Checks
4. Net Operating Profit (NOP) Checks

Steps in Credit Server:

1. Loading of the feed files: creditFeedMonitor
2. Moving the data from the feed tables to the cached tables: ProcessCreditFeed procedure called by credit_server
3. Processing done by credit_server: credit_server

From here I will assume that the data has been loaded to the Feed tables by creditFeedMonitor.sh

1. The caches are loaded.
a. There are many tables and a cache container which contains all the caches.
b. The cache container is
2. The work is done on caches.
3. Then the cache is written to the databases as needed.





