# Progressive jackpot symbols (rules version 4)

Both games now have separate Mini, Minor, Major and Grand reel symbols. Three or more matching symbols anywhere win that pool once; mixed tiers and Wilds do not substitute. Mystery jackpot rolls have been removed.

Nakama awards and resets the winning shared pool in the same conditional transaction as the spin receipt. PlayFab credits the prize through the existing authoritative balance update. Retrying a request replays the same result. Free spins remain eligible and preserve their locked wager.

The previous per-tier trigger rates are retained as long-run symbol-match probabilities: Mini approximately 1/7,000, Minor 1/30,000, Major 1/100,000, Grand 1/300,000. These are not guaranteed intervals. The new stop weights preserve the exact probabilities of all existing Scatter and Free Spins windows.

Neon Nights uses the supplied four progressive-symbol textures. Sugar Lust uses clearly labeled tier placeholders until its artwork is provided. Matching symbols highlight after the reels stop, and a progressive-win panel waits for Continue before free spins resume. Paid auto spins stop on a jackpot.

Deploy the updated Backend/NakamaRuntime/index.js, publish Backend/PlayFabCloudScript/cloudscript.js as the live revision, and use the updated client together. All spin responses now use RulesVersion 4; older client layouts are incompatible. Existing saved progress and jackpot pools are preserved.

Checks: node Backend/Tests/free-spins.test.js

## Recovery of pending version-3 receipts

Deploy the current Nakama index.js and restart Nakama, then publish the current CloudScript as Live. The updated Nakama runtime explicitly marks version-3 stored results as historical receipts served by version 4. CloudScript accepts only that attested recovery; an unmarked version-3 result remains rejected. It settles the original win and feature count once and preserves the original Nakama receipt. Already-settled PlayFab responses are replayed without another debit, credit or RPC.

Use the rebuilt client for Sugar Lust recovery: historical stops refer to old strips, so the client restores the settled profile and pending bonus without displaying old stops on the new strips. Neon Nights can display its original stored grid directly. Do not delete receipts or create a new spin ID to bypass a failed pending result.

Regression verification: node Backend/Tests/free-spins.test.js. Tests cover both games, original wins, one-time free-spin consumption, unchanged stored receipts, rejected unmarked version-3 results, and already-settled PlayFab responses.
