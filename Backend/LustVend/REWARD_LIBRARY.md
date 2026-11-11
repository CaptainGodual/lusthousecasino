# Reward library and package opening

The native client now uses existing server-owned SKU entitlements. Collection leads into a short package reveal after inventory refresh. Previously purchased items are available from MY REWARD LIBRARY in the machine and the lobby REWARDS button. Opening is repeatable, has no gem cost, and never removes ownership. Daily Bonus remains available from its dedicated lobby button. No server deployment is required for the library.

Actual reward media has not been supplied. Empty or missing content slots show CONTENT COMING SOON and preserve the package. Existing package product artwork is used for the opening presentation, not passed off as the reward itself.

## Add content later

In Config/DefaultGame.ini, each [SugarLust.RewardContent.SKU] section can contain multiple +Images and +Videos entries. Existing SKUs: vid-chip, holo-disk, pinup-pack, avatar-item, theme-pack, special-item.

- Import photo/cosmetic preview textures into /Game/Art/Rewards. Example: +Images=/Game/Art/Rewards/pinup-01.pinup-01. Art is already included in the project's always-cook directories.
- Place videos under Content/Movies/Rewards. Example: +Videos=Rewards/vid-chip.mp4. Use an Android/Windows-compatible H.264 MP4 with AAC audio and test actual target devices after packaging.
- Each package displays all configured images followed by its videos. Previous/Next, replay and mute controls are provided. Media stops when returning to the library or leaving the widget.
- Avatar/theme slots display previews; applying actual cosmetics will require the corresponding assets and feature-specific integration when those assets are ready.

Only owned SKUs open. Client content configuration never grants ownership, coins, free spins or gems.

## Verification

Unreal automation: SugarLust.LustVend.RewardLibrary. Checks cover ownership gates, reopen, empty media, no extra gem spending, and video path boundaries. Existing server tests: node Backend/NakamaRuntime/test_lustvend.js. Actual image/video playback and final device layout must be verified when the real media is provided.
