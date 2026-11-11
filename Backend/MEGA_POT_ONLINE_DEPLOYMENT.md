# Mega Lust Pot — Royal Lust online integration

Approved direction: 96% target RTP, medium-high volatility. Rules version 1.

The client now has separate **MEGA LUST POT • ONLINE** and **PRACTICE** lobby entries. Online mode calls PlayFab `MegaLustPotState` and `MegaLustPotSpin`, backed by the existing Nakama account transaction. Online mode never calls the local practice outcome generator or grants local credits. Guided outcomes and practice reset are disabled online.

## Balance result
30,000,000 paid spins returned **95.95956%** of stakes, including complete free-spin, royal-multiplier and hold/respin feature sequences. An independent 10-million-spin holdout returned **96.01717%**, with approximate 95% block interval **95.72%–96.32%**. These are simulation estimates around the 96% target, not an exact RTP proof or certification.

The holdout recorded:
- Paid-cycle hit rate: 23.05% (including the funded spin's subsequent free/hold steps).
- Return standard deviation: 7.02 total bets per paid-funded cycle.
- Mega feature starts: approximately once per 45.34 paid spins.
- Full-board Mega awards: 45 in 10 million paid spins (rare-event estimate).
- Maximum observed funded-cycle return: 1,144× total bet.

Final v1 symbol probabilities per cell: regular Wild 3.5%, free-spin Wild 9.5%, Heart Coin 2.664%, Crimson Crest 2.5%, Royal Crest 0.8%; remaining probability is uniform across the 13 paying symbols. Hold coin probability per empty cell is 3.5% per respin, collector probability 1%, capped at 10×. All 20 paylines, payouts, counter thresholds and fixed feature tiers are defined in `Backend/NakamaRuntime/mega-pot-rules.js`. Mini/Major/Mega are fixed feature awards, not withdrawals from the shared progressive pools.

## Persistence and transaction behavior
Pot counters persist **separately for every bet level**. Free spins, Royal spins and Hold lock their originating bet. One conditional account transaction commits the debit, award, feature state and permanent spin receipt. Receipts retain an argument fingerprint: retries replay the original result; conflicting reuse is rejected. Expected state revisions prevent stale or simultaneous distinct requests consuming the same feature step.

A spin recovery journal is saved locally before submission. Transient retries reuse the same receipt. After an uncertain response, online play locks until **SYNC / RECOVER** completes. Synchronization atomically advances the game's revision; a late uncommitted request using the old revision then fails. A request already committed before synchronization is reflected in the restored server wallet/state. This prevents an old request settling after the client has discarded its journal. Closing the game never settles or resets server features.

Live responses are validated before the client changes its wallet or board. Unknown rules, malformed grids, invalid phase/counters or unsafe balances fail closed. The client updates the existing shared casino session from the authoritative profile.

## Deployment status
Implementation and isolated tests are complete locally. **This bundle has not been uploaded or enabled on the remote server.** This session has no configured PlayFab deployment secret, Nakama HTTP key, or SSH deployment connection. No live-wallet wagers were submitted.

Upload bundle contents:
- `Nakama/index.js`: the single generated runtime, preserving existing Sugar Lust, Neon Nights and LUSTVEND logic.
- `PlayFab/cloudscript.js`: matching allowlisted bridge handlers.
- `Mega-Pot-Balance-Report.json`: configuration and simulation evidence.
- `SHA256SUMS.txt`: file integrity hashes.

Deploy the Nakama runtime first, then publish the matching PlayFab CloudScript revision as Live. Back up current scripts and the database. Keep Mega Pot unavailable during deployment; no profile reset, wallet migration or reseeding is needed. New state is created lazily in existing profiles. Upload only the generated `index.js` as the runtime module, not its build inputs alongside it.

Verify with a dedicated development account: State sync, one paid spin, receipt replay, reconnect, persisted pot progress, and a feature continuation. Confirm other games still settle normally. Never downgrade/reset Mega Pot account state to recover from a deployment issue; keep the game unavailable and preserve receipts/state for a forward fix.

Reproduce the offline simulation from the project root:
`node Tools/Backend/simulate-mega-pot.js 10000000 987654321`

Regression checks: `node Backend/Tests/mega-pot.test.js`, `node Backend/Tests/settlement.test.js`; Unreal automation `SugarLust.MegaPot`, `SugarLust.Lounge.WidgetLifetime`, `SugarLust.NeonNights.WinFeedback`.

Artwork remains replaceable. Practice mode retains its earlier generous exploration math; its credits and feature previews never cross into online mode. Android device testing and remote deployment smoke tests remain outstanding.
