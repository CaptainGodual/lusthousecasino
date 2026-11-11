# Free Spins update

Both games use a dedicated Free Spins symbol, separate from Scatter.

- Three symbols anywhere: 10 free spins; four: 15; five or more: 20.
- Three or more Free Spins symbols during the feature award five extra spins. A feature can award at most 50 spins in total, preventing an endless chain.
- Tap SPIN to pause/resume free-spin autoplay, including pausing after the current spin.
- The triggering bet (and Sugar Lust active lines) is locked until the feature ends.
- Free spins cost no coins, pay normal wins, and do not contribute an imaginary paid wager to progressives.
- Candy Jar and Neon Heat continue to use Scatters. They pause queued free spins and resume afterward. Neon Heat also holds the pause through its payout and completion presentation, with a visible pause message.
- Counts, wagers, accumulated wins and last-request receipts live in the PlayFab profile.
- Neon Nights uses its existing promo-free-spins texture. Sugar Lust uses a temporary FREE SPINS text symbol.

## Deploy together

The local Unreal editor build is updated. The live servers have not been changed by this implementation.

1. Upload Backend/NakamaRuntime/index.js to the Nakama data/modules directory and restart Nakama.
2. Publish Backend/PlayFabCloudScript/cloudscript.js as the live Classic CloudScript revision.
3. Use the updated Unreal client. Existing older clients use the previous Sugar Lust strip layout, so coordinate this backend update with the new client rather than leaving older demos connected during rollout.

The spin response now carries RulesVersion 4. The new client refuses older spin results to avoid displaying old reel stops with new symbol positions.

Run server regression checks from the project directory with:

    node Backend/Tests/free-spins.test.js

Run the Unreal automation test SugarLust.NeonNights.WinFeedback for win visualization and free-spin profile parsing.

## Scatter tuning

Sugar Lust now has one Scatter on each 30-stop reel, including reel four. Three or more visible Scatters trigger the existing Candy Jar bonus with an exact long-run probability of 0.856% (about 1 in 117 spins).

Neon Nights now has one Scatter per 44-stop reel instead of an adjacent pair. Its exact long-run trigger chance is about 0.653% (1 in 153 spins).

These are probabilities, not guaranteed intervals. Free Spins symbols and awards are unchanged. Deploy the updated Nakama runtime and Classic CloudScript together with the updated client (RulesVersion 4); no player-profile reset is needed.

See JACKPOT_SYMBOLS_DEPLOYMENT.md for the latest coordinated rollout and matching jackpot symbols.
