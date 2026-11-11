# LUSTVEND implementation

**Gem earning update is Live:** [GEM_EARNING.md](GEM_EARNING.md) describes the 10/25/50-spin milestones, daily/weekly bonuses, and once-per-account tutorial. Nakama was restarted and PlayFab revision 19 is Live. In-game earning progress and the +30 tutorial reward were verified against the existing signed-in account (10 → 40 gems).

**Current production architecture:** see [NAKAMA_DEPLOYMENT.md](NAKAMA_DEPLOYMENT.md). The game now reuses its existing PlayFab → Nakama bridge; the older separate-service/`ServiceUrl` instructions below describe the standalone reference implementation and no longer apply to the game client. Nakama upload/restart and PlayFab revision 18 Live publishing are complete. The updated game has been verified loading the signed-in account's 10-gem wallet and six-product catalog in the enlarged machine screen. A funded purchase/collect test against live accounts remains outstanding; local transaction tests pass.

## Current in-game player flow

The lobby now places one clickable 215×360 LUSTVEND cabinet at `(852.5,510)` on its 1920×1080 canvas, on the floor in front of the central stage. Clicking it opens a dedicated full-screen native rewards scene over the existing lobby, with the stage backdrop, a fixed cabinet, six selectable product sprites inside its glass, a highlighted selection, an in-game confirmation panel, server-authorized dispensing, and a clickable collect chute. Back to Lobby closes the scene. Repeated clicks do not stack multiple screens. This is a native 2D scene in the current canvas-based game, not a browser or a new 3D level.

Players **never enter a ticket** in-game. `HandleGemsPlusButton` (also bound to the cabinet) passes the current signed-in player's existing PlayFab session to `ULustVendWidget`. The service validates that session and derives account ownership itself. Wallet reads at login and purchase responses update the lobby gem meter. Pending requests remain scoped to the signed-in PlayFab ID, and uncollected rewards recover from that account's server inventory. The standalone browser ticket field is developer test tooling only; it is not part of the player journey.

**Deployment remains required:** `ServiceUrl` is intentionally empty until the authenticated service is deployed. Point it at your HTTPS LUSTVEND service and set `PLAYFAB_TITLE_ID`/`PLAYFAB_SECRET_KEY` only on its server. The local `--demo` server's shared `demo-player` account must never be used for real in-game player accounts. No shared demo token or title secret is included in the game client. Without a deployed endpoint the in-game scene opens but displays an availability message instead of issuing rewards. The existing PlayFab/Nakama slot service is not automatically configured to host this separate rewards endpoint.

This supersedes the earlier native-screen description below: product selection is now on the cabinet, confirmation uses an overlay, and the dispensed product uses the selected reward art with drop/fade animation. The native scene has a Back to Lobby action. The browser reference remains available for service development.

Animation alignment correction: the runtime machine frames now use a complete 215×360 cabinet cropped at `(8,0)` and held pixel-identical across all eight states. Only the 98×47 chute interior at local `(30,277)` changes for states 4–6. This supersedes the original equal-cell 192×360 extraction guidance below. Browser rendering uses native source size, removes the image blur/glow filter, and preloads versioned frames; Unreal also uses the native brush size and prevents upward whole-screen scaling. `Tools/LustVend/check-registration.ps1` verifies every cabinet pixel outside the animated chute. The original dense atlas remains the resolution limit; these changes remove crop drift and unnecessary enlargement rather than inventing missing image detail.

The original conversation attachment was recovered and extracted into `Content/Art/LustVend` and `web/assets`. This feature adds a native Unreal screen opened by the lobby gem **+**, and a standalone browser reference UI. Neither has been deployed to your live servers. Existing PlayFab coins, jackpots, and slot math are untouched.

Verified on this machine: Unreal Engine 5.8 Windows Editor Development build succeeded (`Saved/LustVend-build-verified.log`); all 11 Python tests passed, including loopback HTTP authorization/purchase/collect and static asset delivery; browser JavaScript syntax passed. The build required a project-local UBA cache directory and `-NoUBA` to avoid system-cache permissions. The in-app browser could not reach this computer's loopback address, so browser visual/touch QA and native in-editor interaction remain unverified. Sample PNG crops were visually inspected: the supplied atlas has tightly packed/overlapping machine artwork, so common-cell machine crops can trim the outer silhouette; the intact source atlas is retained for manual registration. The individual Vid Chip crop is usable as extracted.

## Run the playable prototype

From the project directory, run:

```powershell
python Backend/LustVend/server.py --demo
```

Open http://127.0.0.1:8787 in a browser **on this computer**, enter `local-demo`, and Connect. The demo seeds 420 gems once, persists its SQLite database across restarts, and supports the complete select → confirmation dialog → authorization → dispense → collect flow. Click either a machine hotspot or product card. Daily adds 10 gems once per UTC day. Each of the six fixed cosmetics can be owned once. The atlas includes additional pack types that are intentionally not sold yet.

The server runs until Ctrl+C. Use `--db path/to/separate.sqlite3` for a fresh isolated demo. There are no Python package dependencies. A convenience `Tools/LustVend/launch-demo.ps1` starts the local demo and opens the browser.

```powershell
python -m unittest discover -s Backend/LustVend -v
```

Tests cover duplicate concurrent requests, distinct concurrent purchases, insufficient funds, catalog mismatch, ownership checks, injected mid-transaction failure, persistent receipts after restart, earning deduplication, and ledger reconciliation.

## Unreal integration

`ULustVendWidget` is a native C++/Slate screen registered through the existing lobby gem + handler. It passes the existing PlayFab session ticket directly to the service. It loads extracted machine frames from staged PNGs, displays server catalog prices, highlights selected prize buttons, asks for confirmation, waits for approval, animates the chute, and acknowledges collection. Closing during a request is safe: the pending request ID is saved per PlayFab player under `Saved/LustVend`, and uncollected purchases are recovered from the server. Native confirmation is inline; the browser reference uses a modal dialog and product art cards/hotspots. Browser collection animates the product into inventory; native collection currently uses the close/reset atlas frame.

Configure `[SugarLust.LustVend] ServiceUrl` in `Config/DefaultGame.ini` to the **HTTPS** endpoint after deploying the service. Shipping builds reject HTTP; development permits only `http://127.0.0.1:8787`. A real Unreal PlayFab ticket will not authenticate against demo mode. To test native UI locally, run the service in PlayFab mode with the server environment below, and use the loopback URL in a Development build. Never replace the existing game login with the shared demo identity.

The lobby's fake 25-gem number is replaced with an unavailable marker. The authoritative gem balance appears inside LUSTVEND. A future shared wallet subsystem should refresh the lobby meter after login/closing vending; do not restore a local simulated balance.

No new module dependencies are required; HTTP, Json, Slate, and Engine already exist. Raw PNG staging is configured with `DirectoriesToAlwaysStageAsUFS`. For editor asset workflows, import the PNGs to `/Game/Art/LustVend`, set UI texture group, sRGB, no mipmaps, and UI compression; use alpha translucency, never black color-key removal. The source has dense red/neon halos: preserve them. Machine cells share 192×360 bounds; they are illustrative frames, not a continuous high-frame-rate animation. Minor source composition shifts should be registered manually for polished production motion.

## Secure service deployment

Set `PLAYFAB_TITLE_ID` and `PLAYFAB_SECRET_KEY` **only on the server**, then run `python server.py --db /persistent/local-disk/lustvend.sqlite3` without `--demo`. It binds loopback; put it behind your existing HTTPS reverse proxy. The game supplies `Authorization: Bearer <PlayFab session ticket>`. The service verifies the ticket with PlayFab's Server/AuthenticateSessionTicket and derives the player ID from the verified response. It never trusts player ID, price, balance, reward quantity, or progress submitted by a client.

Microsoft's API reference: https://learn.microsoft.com/en-us/rest/api/playfab/server/authentication/authenticate-session-ticket?view=playfab-rest

The bundled Python HTTP server is a reference host for local/small controlled testing. For public release, host these store operations behind a production application server with TLS termination, request rate limits per player/IP, body/time limits, monitoring, and bounded workers. Apply the same auth function to all client API routes; do not enable permissive CORS. Keep the browser UI same-origin. Protect filesystem/static paths and disable directory listing in the production host. Keep tickets only in memory, never URL parameters, analytics, or logs. The service does not log request bodies or tickets. Use PlayFab platform authentication for public accounts; the existing project's guest/custom-ID policy still needs its own account security review.

Keep one writable service/database on durable **local** disk; back up using SQLite's backup API. Do not put SQLite WAL on a network share or run independent database copies behind a load balancer. At larger traffic, port the transaction to PostgreSQL using row locks, unique constraints, and an atomic conditional debit. Rate-limit busy/retry storms. Never copy gem balances into a second authoritative PlayFab currency wallet.

## Transaction and API contract

| Route | Request | Result |
|---|---|---|
| GET `/api/state` | authenticated session | balance, catalog/version, owned SKUs, uncollected receipts |
| POST `/api/purchase` | requestId (UUID), sku, version | receipt, authoritative price and balance at purchase, collection state |
| POST `/api/collect` | receipt | idempotent acknowledgment; never grants twice |
| POST `/api/daily` | empty object | once-per-UTC-day +10 earning event |

`BEGIN IMMEDIATE` serializes writes. In one commit: verify catalog version, enforce SKU ownership and funds, debit wallet, append a negative ledger entry, insert unique inventory ownership, and persist the receipt. A failure rolls back all five records. Unique `(player, requestId)` binds a request to its SKU/version; reusing the key for another payload returns conflict. A retried completed request returns its original receipt. Its balance is the historical balance at that purchase: refresh `/state` for current balance. Successful requests and ledger source IDs must remain durable, with no short rolling receipt window.

SQLite transaction reference: https://www.sqlite.org/lang_transaction.html

Persist the request **before sending**. After timeout, HTTP 5xx, restart, or uncertain outcome, resend that same ID. Never automatically manufacture a replacement ID. Business rejections are HTTP 4xx; the UI can refresh and start a new explicit confirmation. Rewards are granted at authorization, not at animation completion. Collect merely acknowledges presentation; killing the client cannot lose a paid reward. On reconnect, `/state.pending` resumes the chute presentation. Two clients collecting the same receipt is harmless. Entitlement ownership is unique per player/SKU in this initial catalog.

The inventory rows represent durable digital entitlements, not downloadable content delivery. Wire your media/cosmetic library to check ownership using the same authenticated account before playback/equipping. Actual video assets, cosmetic definitions, and a library/equip screen are not included. If rewards must be fulfilled in another service, add an outbox row **in the purchase transaction**, retry delivery with the receipt as the destination idempotency key, and distinguish owned/pending-delivery/fulfilled in the UI. Never perform a PlayFab inventory grant and currency debit as unrelated HTTP calls and call them atomic.

`Store.earn(player, source, amount, reason)` is a server-only, deduplicated primitive. It has no public arbitrary-credit route. Wire trusted game-result/mission events to it with stable source IDs. Validate progress in that authoritative producer. Enforce the earning policy below in that producer before crediting; the primitive alone is not a mission validator. Keep a durable event outbox and replay until the ledger acknowledges receipt. No gem earning based on client timers, claimed spins, wager size, or reported wins.

Reconcile `wallet.gems = SUM(ledger.delta)` per player. Audit source IDs and receipt IDs; use compensating ledger entries for refunds, never edit history. Production refund tooling must atomically revoke the entitlement (or explicitly allow retained ownership), mark the purchase refunded, and credit once. Refund tooling, stock-limited sales, real-money gems, random packs, and cash-equivalent rewards are outside this implementation.

## Initial gem economy (design targets, tune with telemetry)

Gems are a nontransferable cosmetic loyalty currency, separate from wager coins. No cash-out, conversion to wagering coins, gem purchases, randomized rewards, gem-pack purchases with gems, or pay-to-win boosts. The atlas's gem pack/coin pack/XP/mission-skip/random-reward artwork is extracted but excluded from the active catalog to avoid circular faucets and untested game-balance effects.

| Trusted earning source | Gems | Limit / verification |
|---|---:|---|
| Daily check-in | 10 | One per server UTC day; implemented `/daily` |
| Three distinct daily missions | 5 each | Once per mission/day; verify on existing server mission completion |
| Finish daily mission set | 5 | Once/day after all three grants |
| Weekly activity goal | 30 | Once/week, 5 distinct completed daily sets; no wager target |
| Onboarding completion | 30 | Once/account, validated tutorial state |

The full designed daily ceiling is 30; weekly ceiling is 240, excluding one-time onboarding. A casual check-in player gets 70/week. A mission player completing four days gets 120/week; a seven-day active player can reach 240/week. Native/web currently implement only check-in; the remaining sources require trusted producer integration, with separate stable ledger IDs (`mission:<day>:<id>`, `set:<day>`, `weekly:<week>`, `onboarding:v1`). Do not add login streak multipliers or unlimited spin farming.

| Fixed reward | Gems | Days at 10/day | Days at 30/day (rounded up) |
|---|---:|---:|---:|
| Vid Chip | 60 | 6 | 2 |
| Holo Disk | 90 | 9 | 3 |
| Pinup Pack | 120 | 12 | 4 |
| Avatar Item | 180 | 18 | 6 |
| Theme Pack | 300 | 30 | 10 |
| Special Item | 600 | 60 | 20 |

Catalog total is 1,350 gems: about 5.6 weeks at the full weekly ceiling or 19.3 weeks at check-in only, before the proposed onboarding grant. This finite catalog needs new cosmetics over time; it is not a perpetual sink. Track median weekly earning, time to first redemption, 7/30-day redemption rate, wallet percentiles, completion rates, and failed purchase reasons. Start with these targets, measure for two weeks, then adjust future catalog prices with a new version and fresh player confirmation. Avoid changing the value of already purchased entitlements.

## Atlas extraction / animation

Re-run `Tools/LustVend/extract.ps1 -Atlas '<path to original PNG>'`. Pixel rectangles are saved in `Content/Art/LustVend/atlas_rects.json`. Working crops include 8 machine states, 11 individual products, and 10 chute states. Printed captions are excluded as far as the dense source permits; inspect crop edges at final UI scale. No generated substitute artwork was used.

Use machine frames 0/1 for idle at 2 fps; frame 2 for selection/button press; frame 3 while waiting (loop/pulse, no dispense); only after an approved receipt play 4→5→6, roughly 0.4 seconds each. Hold 6 until collection. Frame 7 closes/resets. The browser demonstrates product fall and collect transforms; honor reduced-motion preferences. For Paper2D, import each PNG as a sprite and build separate nonlooping dispense and reset flipbooks; for UMG use the same frame list and update a brush. Chute frame 9 is close; the source labels repeat “ITEM SLIDE 2,” so use numeric indices, not caption text. For non-Vid rewards, use an empty chute crop plus the selected product sprite to avoid depicting a Vid Chip for every product. The initial whole-machine sequence depicts the atlas's Vid Chip sample.

Before shipping: compile and package the native widget for Windows/Android, confirm PNG staging/touch scale and safe areas, test valid/expired PlayFab sessions against a nonproduction title, test two devices on the same account and dropped responses, verify restored entitlements in the consuming library, wire mission producers, and deploy the production host. The standalone demo does not require those live-service steps.
