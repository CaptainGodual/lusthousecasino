# Neon Muse: Neon Nights rules version 5

The supplied transparent PNG is imported at /Game/Art/NeonNights/neon-symbol-muse. It is regular symbol ID 18, appended after the four existing progressive IDs. One ordinary Ace per 44-stop reel strip is replaced. Wild substitutes; three/four/five consecutive reels from the left pay 10/30/100 times Bet per winning way. Multiple matching rows multiply ways. This adds a new regular payout and changes overall return; it is not a certified RTP configuration. Scatter, Free Spins and jackpot positions and exact feature trigger probabilities remain unchanged. Sugar Lust stays at rules version 4.

## Activation order

1. Back up the deployed index.js. Upload Backend/NakamaRuntime/index.js to /opt/nakama/data/modules/index.js and restart Nakama.
2. Publish Backend/PlayFabCloudScript/cloudscript.js as the Live Classic CloudScript revision.
3. Use the freshly compiled client. Package Android and Windows again before distributing this content. An older Neon client expects version 4 and will reject version 5; do not test older demo downloads against this rollout.

The current runtime explicitly attests historical Neon version-3 and version-4 receipts. CloudScript settles the original payout once and returns the new response contract. Already-settled PlayFab responses replay without another payment. No receipt deletion or reroll is required. Sugar Lust keeps its existing version-3-to-4 recovery.

## Verification

node Backend/Tests/free-spins.test.js
Unreal automation: SugarLust.NeonNights.WinFeedback

Checks cover Muse wins/losses/Wild substitution, original feature probabilities, existing jackpots, and pending version-4 receipts during the version-5 rollout. Live deployment is not performed by the local build/import steps.
