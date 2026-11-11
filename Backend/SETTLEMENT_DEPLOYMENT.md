# Transactional settlement deployment — v0.0.2

Prepared October 31, 2026. Local implementation and isolated tests are complete; production deployment and real-database smoke tests are still required. No Unreal version settings or client code were changed.

## Finish an already-staged migration with one command

For the current development account, the frozen export and staging have already succeeded and the matching runtime/Live bridge are deployed. Keep gameplay stopped, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "C:\Users\Hammy\Documents\Unreal Projects\UnrealPrototype 5.8\Tools\Backend\Finish-SettlementSetup.ps1"
```

Enter only the PlayFab title secret key at the hidden prompt. The launcher obtains the existing gateway and HTTP key from Title Internal Data, verifies/imports the frozen staged account directly, checks the actual Live CloudScript profile response, sets the enable flag, and checks its readback. It does not upload scripts, restage/replace the frozen profile, reset an account or perform test spins. A missing staged import or balance/feature mismatch stops the workflow. Imported accounts are not overwritten on a rerun. Partial completion is preserved safely; do not reset it.

A successful result is recorded without credentials in Saved/SettlementFixVerification/live-setup-result.json. Restart Play in Unreal after SETUP COMPLETE. Verification uses the live server response, not the local cached balance. Actual gameplay smoke tests still follow.

## What changed

PlayFab still authenticates players and exposes the same game handler names. Nakama now owns coins, XP, missions, daily claims, Candy Jar, Neon Heat and free-spin state. Each spin commits its wallet update, permanent request receipt, progressive jackpot changes, and earned-gem progress in one conditional storage batch. Concurrent requests retry from a fresh account rather than overwriting one another. A changed wager under an existing spin ID is rejected. Old spin receipts no longer become payable again after the 20-entry reel cache expires.

The development DebugTriggerBonus handler is absent. The registered gateway blocks the old separate Spin/NeonSpin operations, preventing an old spin bridge from producing new partial settlements. There is no fallback to legacy PlayFab writes. The old PlayFab internal profile becomes historical data, not the current wallet.

## Files

Upload only these runtime files to their respective services:

- Backend/NakamaRuntime/index.js — generated single ES5 bundle for the Nakama modules directory.
- Backend/PlayFabCloudScript/cloudscript.js — matching PlayFab Classic CloudScript; publish a new Live revision.

Do not upload index.core.js, economy-domain.js and economy-settlement.js separately alongside the generated bundle. They are maintainable build inputs, already included in index.js. Rebuild after editing them with `node Tools/Backend/build-runtime.js`; `--check` verifies the bundle is current.

The private migration helper is Tools/Backend/stage-economy-migration.js. Do not package it, its credentials, player list, or exported profiles with the game or commit private snapshots to a public repository.

## Maintenance and cutover order

1. Put the game in maintenance and block gameplay requests at the service/API boundary. Close test clients and drain all existing CloudScript executions before freezing profiles. This includes old daily/mission/bonus handlers; restarting Nakama alone does not drain PlayFab requests. Take protected backups of PlayFab internal profiles, the Nakama database, and both old deployed scripts. Keep requests blocked throughout staging.
2. Leave PlayFab Title Internal Data `SugarLustEconomyMigrationReady` absent/empty. Upload the generated Nakama index.js, restart the actual Nakama service, and confirm the log contains `Transactional settlement v1 enabled; legacy spin RPC disabled.` Existing jackpot pools and gem ownership are retained. Verify the uploaded file hash against the release manifest.
3. Freeze and stage **every existing player that has SugarLustProfile**, using the helper below. It preserves the full profile, including coins, XP, claims, pending bonuses, free spins and last settled responses. Imports are create-only; mismatches stop rather than overwriting progress. A forgotten legacy player will fail closed on login until their frozen profile is staged; they will not be reset to defaults. Include all existing players, not only the developer account. The helper intentionally does not guess or enumerate accounts.
4. Publish the matching cloudscript.js as Live while requests remain blocked. Verify clients use Live; the current native client does. Retire or restrict historical CloudScript revisions as appropriate to your title. Crucially, account initialization reads the frozen Nakama import, never the legacy profile's balance, so a later old-revision write cannot change an already frozen seed.
5. After old requests have drained and all known legacy snapshots are staged, set **Title Internal Data** `SugarLustEconomyMigrationReady` to exactly `transactional-v1`. This flag permits first-time imports/new accounts; it is not a global pause switch for accounts already imported. Keep the existing HTTPS gateway URL and HTTP key private and unchanged unless rotating them deliberately.
6. Allow only a dedicated tester first. Verify migration equals the frozen balance/XP/features, then test both games and the checks below against the actual server/database. Reopen general access only after those checks pass. New players without a legacy record receive the existing 10,050,000-coin/0-XP defaults; no client-provided seed is accepted.

## Private helper usage (Node 18+)

Run from the project root. Use a private shell session; provide credentials through environment variables without putting values in scripts or screenshots. Required for export: PLAYFAB_TITLE_ID and PLAYFAB_SECRET_KEY. Required for staging: NAKAMA_URL (HTTPS base URL) and NAKAMA_HTTP_KEY. The helper never prints credentials or key-bearing URLs.

Create a private JSON file containing the existing players' PlayFab IDs, for example `["PLAYER_ID_1","PLAYER_ID_2"]`. This list is not a list of Nakama UUIDs.

```powershell
node Tools/Backend/stage-economy-migration.js --players Saved/SettlementMigration/players.json --export Saved/SettlementMigration/profiles.json
node Tools/Backend/stage-economy-migration.js --snapshot Saved/SettlementMigration/profiles.json
node Tools/Backend/stage-economy-migration.js --snapshot Saved/SettlementMigration/profiles.json --apply
```

The first command only reads PlayFab and writes a private local frozen snapshot; it refuses to overwrite an existing snapshot. The second previews that exact snapshot. Only the third writes create-only staged imports to Nakama. It does not enable the import gate. Inspect the balances before applying; keep this snapshot protected for recovery. If a batch stops partway, rerunning the same snapshot is safe: matching rows return already_staged; imported accounts return already_migrated. A different snapshot cannot overwrite a staged row. If there are no legacy records at all, no legacy import is needed.

Do not run against a live actively changing old economy and assume the result is a consistent cutover. The export is sequential; maintenance/draining is required. Preserve any pre-cutover discrepancy for reconciliation rather than silently substituting a newer snapshot.

## Old pending requests

A last settled PlayFab response is imported as proof of prior payment and can be replayed without another payment. An older raw Nakama receipt or played marker without a permanent settlement ledger is **not** proof that PlayFab paid it. Such requests stop with `Legacy spin needs settlement reconciliation`. Do not delete receipts, reset free spins, change the request ID to reroll it, or automatically credit its Win. Preserve the frozen profile, original request and old receipt and reconcile that player offline once. There is deliberately no public endpoint that blindly credits an ambiguous old result.

New requests have durable settlement receipts. They are not trimmed with the old reel cache. Replaying an old new-system request returns the original outcome with the current profile snapshot, without charging, granting XP, consuming a free spin or touching pools again. Candy Jar duplicate cards and already-claimed daily rewards continue to reject safely; they are not new retriable request-ID APIs.

## Storage and recovery

Private system-owned `slot_economy` rows:

- account_<PlayFabId>: authoritative profile and revision; shared conditional row for all coin/feature operations.
- legacy_<PlayFabId>: frozen create-only source snapshot.
- receipt_<SHA256(player|action|requestId)>: permanent settlement response and immutable request fingerprint.

The same conditional batch includes sugar_lust progressive/reel receipt rows and applicable lustvend wallet/activity/ledger rows. No HTTP or PlayFab mutation occurs inside the database retry callback. A failed batch leaves all of them unchanged. Pool and gem conflicts also trigger recomputation.

Back up these rows with the rest of Nakama. Do not prune settlement receipts or frozen imports as routine cache cleanup. Monitor growth and contention before a public scale-up. The global progressive row remains a serialization point.

**Rollback is not simply uploading the old scripts.** After the first new settlement, PlayFab's old profile is stale. Stop access, preserve the new database and exports, and reconcile/export authoritative balances/features before any deliberate rollback to the old economy. Never delete slot_economy to force migration again. The import flag does not roll back data.

## Verification and acceptance

Local commands:

```powershell
node Tools/Backend/build-runtime.js --check
node Backend/Tests/settlement.test.js
node Backend/Tests/free-spins.test.js
node Backend/NakamaRuntime/test_lustvend.js
```

The new tests execute the actual bridge, registered gateway, domain and reel helpers against an isolated version-checked storage harness. They simulate interleaved transactions, atomic failures, and response loss after commit. This is not a live Nakama/PostgreSQL load test.

Before reopening the service, verify on a dedicated live tester: first import and reconnect; one paid spin per game; A-B-A with unchanged and changed wagers; retry after a lost response; simultaneous cross-game requests; insufficient funds; free-spin consumption and Neon Heat pause/resume; Candy Jar final payout; daily/mission claims; jackpot award/reset; and a gem purchase racing an earning milestone. Confirm exact account/receipt/pool deltas and private storage permissions. Confirm a historical raw Spin RPC fails rather than partially settling. Verify a new account and an unstaged legacy account behave as documented.

Nakama's version-checked storage batch/retry semantics are documented at https://heroiclabs.com/docs/nakama/concepts/storage/collections/ . Actual service version/health still needs verification during deployment.
