# Lust House Casino

The active C++ project is **SugarLustUE.uproject**, configured for Unreal Engine **5.8** and application version **0.0.2**. The November 11 review found engine **5.8.4** installed and successfully rebuilt the Editor target using a project-local Unreal Build Accelerator cache. The older Windows and Android packages are candidates from October 31; they do not verify the newer source or current device behavior.

## One implementation board

[Open the master release board](Docs/Release/MASTER_BOARD.md). It covers every major implemented and planned system with a status, evidence level, dependency, next action and completion check. Edit [systems.json](Docs/Release/systems.json), then regenerate the board. Character Studio and Character Production remain specialist character records rather than competing game queues.

`working` means the system ran successfully at the stated evidence level. It does not automatically mean it passed packaged or live-service release acceptance. The full release gate includes expansion systems; the focused checkpoint is an intermediate milestone.

## Repeatable verification

Run from the project root with Node and Python available:

```powershell
node Tools/Release/audit-release.js
```

This fingerprints source, configuration, backend code, content and media, runs isolated backend and release-policy checks, and updates the board plus `Saved/ReleaseAudit/current.json`. Exit 2 means release evidence remains incomplete or blocked; exit 1 means a required automated check failed. The optional Python demo HTTP test may be blocked by a sandbox that denies loopback sockets; this is reported separately from the production Nakama checks.

Build and verify native code using a common evidence directory:

```powershell
$reviewFingerprint = (Get-Content -Raw Saved/ReleaseAudit/current.json | ConvertFrom-Json).sourceFingerprint
$reviewOutput = Join-Path $PWD 'Saved/ReleaseAudit/native-current'
./Tools/Release/Verify-Project.ps1 -Mode EditorBuild -OutputDirectory $reviewOutput -SourceFingerprint $reviewFingerprint
./Tools/Release/Verify-Project.ps1 -Mode EditorTests -OutputDirectory $reviewOutput -SourceFingerprint $reviewFingerprint
./Tools/Release/Verify-Project.ps1 -Mode WindowsCheckpoint -OutputDirectory $reviewOutput -SourceFingerprint $reviewFingerprint
node Tools/Release/audit-release.js --reuse-checks --unreal-evidence Saved/ReleaseAudit/native-current
```

The wrapper keeps caches and logs in the project and preserves existing release packages by archiving a new Windows checkpoint. It never treats an Editor exit code of zero without a nonempty automation report as a passing test run. A packaged build still requires interactive, account, restart, performance and service acceptance against that exact artifact. Fill `Saved/ReleaseAudit/acceptance.json` from the generated template only with actual test evidence.

## Implemented paths and outstanding work

Sugar Lust, Neon Nights, Candy Jar, free spins, progressives, missions, daily rewards, and gem earnings have client and authoritative backend implementations. Mega Pot has local online implementation and isolated transaction tests; its deployment guide records that the new bundle still needs remote deployment verification. Spin Duel remains labeled practice. Reward media, wearable equipment, character deployment, time-of-day presentation, Pepper, and deeper social/VIP/store features have explicit unfinished records in the master board.

PlayFab authenticates the player. Nakama owns online settlement and durable receipts. Local demo logic must not become a fallback for online currency. See [settlement deployment](Backend/SETTLEMENT_DEPLOYMENT.md), [Mega Pot deployment](Backend/MEGA_POT_ONLINE_DEPLOYMENT.md), and [LUSTVEND](Backend/LustVend/README.md).

The local Git metadata did not resolve a commit during the review. Preserve `Saved/ReleaseAudit/2026-11-11/baseline-source-manifest.json` and the binary backups until a valid checkout is restored and compared. Do not initialize a new repository over unresolved metadata or reset player state to repair deployment problems.

