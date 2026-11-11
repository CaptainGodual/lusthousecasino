# Current LUSTVEND activation: existing Nakama server

**Activation verified:** the user restarted Nakama after the script upload, its HTTPS healthcheck returned HTTP 200, and PlayFab Game Manager shows **Revision 18 (live)** with the uploaded matching CloudScript. Revision 17 remains available for rollback. The updated Unreal game signed into the existing account automatically, displayed **10 gems** in the lobby, and opened the enlarged LUSTVEND screen with the same **10 gems**, six catalog prices and the ready-to-select message. No browser ticket or separate service URL was used. Screenshot: `Saved/LustVendDeployment/game-live-wallet.png`. A full funded purchase/collect test remains unperformed against live accounts; the authenticated wallet/catalog path is verified, and local transaction tests cover purchase/collect/retry behavior.

The production game now uses **Unreal → existing PlayFab login/Classic CloudScript → existing Nakama RPC**. The Python/browser demo is separate development tooling, not the production wallet. `ServiceUrl` is no longer needed or read by the game. No new hostname, listener, client key, or player-entered ticket is required.

## Completed locally / uploaded

- Enlarged close-up cabinet: 430×720 design units within a 1000×900 responsive layout. Source artwork remains registered at 215×360; product sprites and touch regions scale together.
- Solid dark glass and product tile backgrounds prevent the atlas's original products showing through the new selectable rewards. Stage backdrop is dimmed using a solid brush.
- Windows Unreal Editor build succeeded: `Saved/LustVend-nakama-build.log`.
- Twelve local Nakama/PlayFab contract tests passed: `node Backend/NakamaRuntime/test_lustvend.js`.
- Live `/opt/nakama/data/modules/index.js` was downloaded via the existing WinSCP session and compared with the local source; only the LUSTVEND dispatch and handler additions differed.
- Previous live file is preserved in `Saved/LustVendDeployment/before/index.js` (14,759 bytes).
- Updated `Backend/NakamaRuntime/index.js` (19,660 bytes) was uploaded to `/opt/nakama/data/modules/index.js` through WinSCP. Remote listing confirmed the new size. This does **not** prove that Nakama has loaded it.
- Matching PlayFab source is staged at `Saved/LustVendDeployment/upload/cloudscript.js` and maintained at `Backend/PlayFabCloudScript/cloudscript.js`.

## Activation / verification procedure

1. Restart the existing Nakama service using its normal SSH/deployment method. Identify the actual service/unit before restarting; don't reboot the VPS or replace its configuration unnecessarily. Verify service health and runtime logs. Restore the rollback script and restart if runtime loading fails.
2. In the existing PlayFab title's **Classic CloudScript**, upload `Backend/PlayFabCloudScript/cloudscript.js` as a new revision. Preserve/export the old Live revision and compare any changes made outside this repository before publishing. Set the new revision **Live** after the Nakama runtime loads successfully. New functions: `LustVendState`, `LustVendPurchase`, `LustVendCollect`.
3. Existing server-only Title Internal Data entries `SugarLustNakamaGatewayUrl` and `SugarLustNakamaHttpKey` are reused. Do not place them in Unreal, screenshots, or client configuration.
4. Restart/reopen the newly built game module, sign in normally, and enter the lobby machine. Confirm the gem balance/catalog load. If it still fails, check PlayFab CloudScript execution logs; distinguish missing function/revision, missing internal settings, and Nakama runtime errors. Do not inject a shared demo token.
5. Test a purchase with a funded nonproduction account, retry its original request after a dropped response, collect the receipt twice, and check a second account cannot access its inventory. Verify Sugar Lust and Neon Nights still work.

The agent used the existing SFTP file-transfer connection through WinSCP. Direct OpenSSH was rejected before authentication because no trusted host key was configured for that client; no host verification bypass was used. The user performed the service restart, PlayFab revision 18 is confirmed Live, and the live in-game wallet/catalog path is verified.

## Wallet behavior

`LustVendState` makes a trusted `Daily` call: once per UTC day, the signed-in account receives **10 gems**. Refreshing, reconnecting, or using a second device does not duplicate the grant. Actual wallets start at zero; the standalone demo's 420-gem test seed is never imported. Six fixed cosmetics cost 60–600 gems. Additional mission/weekly earning integration remains future work.

Nakama keeps private storage in collection `lustvend`, owned by the same system UUID used by the existing slot runtime:

- `account_<PlayFabId>`: balance, owned SKU list, uncollected receipt list.
- `receipt_<PlayFabId>_<request UUID>`: durable, payload-bound purchase receipt.
- `ledger_<PlayFabId>_<request UUID>`: negative purchase ledger entry.
- `ledger_<PlayFabId>_daily_<UTC date>`: unique positive daily ledger entry.

`storageWriteRetry` conditionally commits all affected objects atomically and recomputes on version conflict. Object read/write permissions are both zero. Receipts are not trimmed. Collection only marks presentation acknowledged; ownership was already committed with the gem debit. PlayFab builds the Nakama request using `currentPlayerId`, never a player ID, price, balance, or grant amount from client arguments. Direct Nakama user-context calls to the shared RPC remain rejected.

Official storage behavior: https://heroiclabs.com/docs/nakama/concepts/storage/collections/

## Rollback

Restore `Saved/LustVendDeployment/before/index.js` to `/opt/nakama/data/modules/index.js`, restart Nakama, and select the prior PlayFab Live revision. Retain the `lustvend` storage records for audit/recovery; don't delete wallets or receipts. The earlier slot runtime doesn't use those new records.
