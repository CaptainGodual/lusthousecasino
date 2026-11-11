# PlayFab Classic CloudScript bridge

This is the authenticated entry point for both slot games and LUSTVEND. Coins, XP, missions, bonus state and free spins are now authoritative in Nakama and commit transactionally with each settlement. PlayFab no longer writes SugarLustProfile after cutover. Its old profile is historical migration data only.

Read [the mandatory maintenance/migration deployment guide](../SETTLEMENT_DEPLOYMENT.md) before uploading either backend file. Deploy the matching Nakama index.js and this cloudscript.js together; there is no legacy settlement fallback. Existing players need a frozen import staged with the private helper. Set server-only Title Internal Data SugarLustEconomyMigrationReady=transactional-v1 only after draining old requests and staging imports.

The handler names and public response shapes are unchanged: GetOrMigratePlayerProfile, GetProgressiveJackpots, Spin, NeonNightsSpin, PickBonusCard, ClaimDailyBonus, ClaimMissionReward, and the existing LustVend handlers. DebugTriggerBonus is removed. Identity always comes from currentPlayerId; client balances, migration seeds and arbitrary target players are not accepted.

Keep SugarLustNakamaGatewayUrl HTTPS and SugarLustNakamaHttpKey private in Title Internal Data. Do not put server keys in the client or a public repository.
