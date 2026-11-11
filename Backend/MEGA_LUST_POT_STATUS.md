# Mega Lust Pot — Royal Lust

## Available now
Open the main lobby and choose **MEGA LUST POT • PRACTICE**. This is an independent playable C++ prototype with its own temporary practice credits. It never submits PlayFab spins, changes the live coin/gem balance, grants inventory, or increments live missions.

Implemented:
- 5 reels × 3 rows, 20 unique left-to-right paylines, best wild substitution paid once per line.
- Royal Lust ruby/black/gold interface with animated reel cycling, paid-cell highlights, rules/paytable, bet controls, max bet and auto-play.
- Crimson: 12 dedicated crests award eight extra-wild free spins.
- Royal: 16 dedicated crests award three spins with independently selected 2×–10× line-win multipliers.
- Mega: six Golden Heart Coins in one result, or 24 accumulated heart coins, start Hold & Respin.
- At least six starting coins, three respins, persistent locks, each new coin resets to three, collectors raise the final coin multiplier up to 10×.
- Highest achieved feature tier: nine hearts Mini 25×, twelve hearts Major 100×, all fifteen Mega 1,000× total feature bet, plus the locked-coin sum × collector multiplier.
- Bets remain locked through queued features; free/multiplier spins resume after the hold bonus.
- Actual session counters, guided feature previews, close/reopen retention, and resettable practice credits.

## Explicit prototype decisions
The concept leaves trigger frequencies and exact payouts open. All thresholds, symbol weights, free-spin counts, multipliers and tier values above are provisional design choices. The tiers are fixed feature awards, not claims on the casino's existing progressive pools. The Queen Wild is currently a text placeholder; final symbol illustrations, pot vessels, character art, audio, cabinet paint and polished transitions remain art/production work.

Counters currently persist only for the life of this practice widget and are shared across practice bet levels. A production design must decide per-bet progress and migration rules before economy activation. Guided feature previews deliberately construct triggering practice outcomes and are explicitly labeled in the UI. They must not be exposed in a live-wallet game.

## Validation
Windows editor build passes. Automated rules tests cover paylines, wild payment, rejected malformed outcomes without debits, free-spin debit behavior, multiplier range, bet locking, both hold triggers, respin resets, preserved locks, collector application, full-grid payout, and duplicate-collection prevention. A deterministic 10,000-step run checks feature-state and meter invariants. Existing widget-lifetime and Neon Nights win-feedback checks also pass.

The editor play-through checks lobby entry, hold bonus, respins, result, collection, reopen retention, help, and return to lobby. Android packaging/touch has not been tested for this new view.

## Required before live-wallet release
1. Approve the provisional math, simulate the full paid/free/hold/collector process for RTP and volatility, and lock a versioned configuration. The 10,000-step invariant run is not an RTP certification or balancing exercise.
2. Implement these versioned rules in the Nakama transactional economy; persist pot counters, queued features and locked bets in the account transaction. Use stable spin IDs and fingerprinted receipts for replay/retry without double settlement.
3. Add PlayFab allowlisted request/response fields and a fail-closed client path. Never use the local practice engine as a live-wallet fallback.
4. Test transaction retries, concurrent sessions, reconnect/resume, out-of-order replies and feature recovery before deployment.
5. Replace placeholders with authored art/audio, fit the cabinet screens, then assign floor cabinets and test Windows/Android performance.

The existing Sugar Lust and Neon Nights production gameplay and floor assignments were not replaced by this prototype.
