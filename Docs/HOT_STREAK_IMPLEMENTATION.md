# Hot Streak with Pepper

This implements the first playable mechanics from Brian's Hot Streak with Pepper Implementation Plan. The code is ready for an internal test event. Deployment, real database concurrency checks, final Pepper art and production activation remain separate work.

## Playable behavior

- The third consecutive qualifying paid win earns 1 Heat. Each further consecutive paid win earns another 1.
- Any positive authoritative `BaseWin` counts, including a payout below the wager. A paid loss resets the current streak and preserves Heat and the best streak.
- Sugar Lust and Neon Nights share the event streak when both are configured as eligible. Free spins, Candy Jar picks, Neon Heat respins, Spin Duel, Mega Lust Pot and offline demonstrations cannot earn or reset it.
- Heat thresholds are Mild 1, Hot 5, Extra Hot 15 and Inferno 30. The rule module does not change reel odds or multiply payouts.
- Mild grants a supported permanent presentation title, **Pepper's Mild**, which can be equipped on the event HUD and from My Reward Library. The three wearable tiers are previews with collection disabled until their content is available.
- The shared event panel is accessible from the main menu, Events navigation, both slot HUDs and the cabinet control deck. Spin Duel practice remains available from the panel.
- Pepper currently uses text reactions with the approved Mild, Hot, Extra Hot and Inferno badges on tier unlocks. The same badges appear on the four event collection cards. Reels reveal trusted counters; bonus controls, payout counting, jackpots and big-win presentations take priority over Pepper. Ordinary reactions have a 15-second cooldown. Opening the panel stops autoplay and waits for the current result without deleting its pending spin request.

## Pepper tier artwork

The four supplied 1254-by-1254 PNGs are preserved under `Content/Art/HotStreak`, alongside their imported UI textures. They have opaque black backgrounds; the cards and unlock cue use matching black panels. `HotStreakArt` maps the fixed event tier IDs to local assets and retains text fallbacks. `Tools/Art/import_pepper_tier_symbols.py` provides repeatable imports with UI compression, sRGB, no mipmaps and no streaming. The existing `/Game/Art` cook directory includes these textures. These badges do not change reel symbols, Heat rules or reward readiness.

## Persistence and authority

`hot-streak-rules.js` supplies pure ES5 rules and a small allowlisted reward catalog. `economy-domain.js` applies scoring only to validated paid base outcomes. `economy-settlement.js` commits the wager, event progress and permanent receipt in one conditional batch.

The private `event_config/hot_streak` row retains definitions under `Events`, with `ActiveEventId` selecting the featured season. A request pins its server admission time and season before storage retries. Earning and claims validate the configuration version in the conditional commit. Existing scoring, dates, tier IDs and reward IDs cannot be edited under an existing EventId; operational earning and claim switches remain separate.

Claim grants share the existing LUSTVEND ownership account. A deterministic player/event/tier grant ledger, typed title ownership, claimed marker and claim receipt commit together. Different ClaimIds racing for the same tier still grant once. Same-ID receipt recovery works after the deadline; a new late claim fails. Repeat ownership explicitly keeps the existing reward without a coin or gem replacement.

Spin receipt replays return their original event delta, a fresh profile snapshot and a separate `Replayed` marker. The client never earns Heat locally. Its game-instance subsystem rejects older revisions and callbacks from another login generation, bounds presentation IDs, and persists pending claim IDs before sending them. The shared account transport preserves existing spin retry IDs and explicitly supports bounded claim retries.

## Preparing an internal event

The checked-in fixture is disabled and uses a future placeholder schedule. No live service or player account is changed by building the project.

Prepare a private gateway request locally:

```powershell
node Tools/Backend/prepare-hot-streak-event.js --starts-at 2026-12-01T00:00:00Z --event-id pepper_hot_streak_test_01
```

This writes `Saved/HotStreakVerification/configuration-request.json`, with seven days of earning, a further 48-hour claim window, Sugar Lust eligibility and both operational switches disabled. The example date is an input example, not a launch commitment. Supply the intended UTC start before staging. Add `--include-neon` for a separate test definition that includes Neon Nights.

The tool can prepare explicit earning/claim switch values with `--earning-enabled` and `--claims-enabled`; it never sends the request. `--previous-config` retains older season definitions when adding a new EventId. `--expected-version` supplies the actual storage version when updating an existing configuration; `*` is create-only. Switch-only changes to an already staged season must retain its immutable definition and use the current storage version.

After backing up the existing runtime, bridge and storage, deploy the generated `Backend/NakamaRuntime/index.js` and matching PlayFab `cloudscript.js` to staging. Apply the reviewed configuration through the private authenticated gateway operation `ConfigureHotStreak`. There is no player CloudScript handler for configuration. Use an internal EventId and tester accounts before any public season.

## Validation

```powershell
node Tools/Backend/build-runtime.js --check
node Backend/Tests/hot-streak.test.js
node Backend/Tests/settlement.test.js
node Backend/Tests/free-spins.test.js
node Backend/Tests/mega-pot.test.js
node Backend/NakamaRuntime/test_lustvend.js
```

The new backend suite covers the documented sequence, excluded rounds, boundaries, atomic failures, concurrent spins and claims, lost responses, old receipt recovery, configuration changes, title equipment, retained ownership and season changes. Existing settlement, feature and vending regressions are also covered.

Unreal automation includes `SugarLust.HotStreak.ClientState`: strict parsing, deferred display, duplicate/replayed deltas, revision ordering, account isolation, persisted claim recovery, cooldown, mandatory presentation ordering and the four-tier panel. `-HotStreakScreenshots` on the automation command line exports the actual event panel and tier unlock cue to `Saved/HotStreakVerification/event-panel.png` and `pepper-unlock.png` for layout inspection.

## Remaining production work

Use staging to verify real Nakama conditional-write behavior, simultaneous devices and lost network replies. Review the initial Sugar Lust slice before activating additional games. Import approved Pepper portraits, expressions and wearable collection artwork. Wearable claims require the avatar attachment, outfit/effect and equipment compatibility pipeline. Tune thresholds with authoritative reel simulations before freezing a public RulesRevision, then check packaged Windows and Android presentation and controls. No public event dates or wearable availability have been enabled here.
