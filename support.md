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