# Lust House Casino master release board

Reviewed 2026-11-11T23:42:22.170Z. This is the single game implementation board. Edit systems.json and regenerate this view; keep Character Studio and Character Production as linked specialist records.

**Full release: NOT READY. Focused checkpoint: NOT READY.**

Inventory: 35 systems; 3 working, 17 incomplete, 7 planned, 8 blocked. Working always includes an evidence level and does not automatically mean release accepted.

## Active baseline

- Project: SugarLustUE.uproject; Unreal 5.8; application 0.0.2.
- Current runtime source fingerprint: 6979603885715a556921e3e6889dad767b188145981ee973395fc010f0d0bfad.
- Source-control identity: Resolved source commit aa4b4c970d1c2156f3e2ca3b6d7f5c87a501d724
- Preserved pre-change manifest: [Saved/ReleaseAudit/2026-11-11/baseline-source-manifest.json](../../Saved/ReleaseAudit/2026-11-11/baseline-source-manifest.json).
- Machine-readable current checks: [Saved/ReleaseAudit/current.json](../../Saved/ReleaseAudit/current.json).

## Status meanings

- **working**: Executed successfully at the explicitly stated evidence level. Release acceptance is tracked separately.
- **incomplete**: Implementation exists, but required behavior or acceptance evidence is unfinished.
- **planned**: No integrated implementation was found for the intended player outcome.
- **blocked**: The next required implementation or verification step has a concrete unmet dependency.

## Current verification

| Check | Result | Evidence |
| --- | --- | --- |
| project-config | passed | Engine 5.8; configured versions 0.0.2, 0.0.2, 0.0.2, 0.0.2 [SugarLustUE.uproject](../../SugarLustUE.uproject) [Config/DefaultGame.ini](../../Config/DefaultGame.ini) [Config/DefaultEngine.ini](../../Config/DefaultEngine.ini) |
| git-head | passed | Resolved source commit aa4b4c970d1c2156f3e2ca3b6d7f5c87a501d724 [Saved/ReleaseAudit/2026-11-11/baseline-source-manifest.json](../../Saved/ReleaseAudit/2026-11-11/baseline-source-manifest.json) |
| runtime-bundle | passed | Runtime bundle verified. [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/runtime-bundle.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/runtime-bundle.log) |
| settlement | passed | PASS new players use server defaults, never a supplied seed PASS frozen imports cannot be overwritten and migrated accounts cannot be reseeded 32 transactional economy integration tests passed. [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/settlement.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/settlement.log) |
| free-spins | passed | PASS: already-settled PlayFab version-3 responses recover without another debit, credit or RPC. PASS: Neon Muse reel frequency, 3/4/5-reel payouts, Wild substitution and losing combinations. PASS: Neon rules-4 receipts recover under rules 5 without reroll, double pay or duplicate free-spin consumption. [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/free-spins.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/free-spins.log) |
| mega-pot | passed | PASS Mega and Sugar transactions preserve one shared wallet PASS Mega RTP is exactly stake-linear across the minimum and maximum bet 16 Mega Pot transactional integration tests passed. [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/mega-pot.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/mega-pot.log) |
| setup-workflow | passed | PASS Live handler failure blocks enabling and redacts credentials and gateway URLs PASS missing key fails before any requests 6 setup workflow tests passed. No live requests made. [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/setup-workflow.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/setup-workflow.log) |
| lustvend | passed | PASS earning storage failure rolls back entire spin and grant PASS client cannot submit activity or arbitrary gem credits 19 LUSTVEND Nakama/PlayFab tests passed. [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/lustvend.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/lustvend.log) |
| release-policy | passed | PASS duplicate IDs and cyclic dependencies are rejected PASS unknown dependencies and invalid completed states are rejected 14 release readiness policy tests passed. [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/release-policy.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/release-policy.log) |
| native-policy | passed | PASS the CLI cannot invent exit zero when the native exit code is missing PASS malformed fresh report JSON is rejected by the CLI 13 native verification policy tests passed. [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/native-policy.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/native-policy.log) |
| lustvend-python (optional demo) | blocked | Ran 11 tests in 1.063s  FAILED (errors=1) [Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/lustvend-python.log](../../Saved/ReleaseAudit/run-2026-11-11T23-42-22-170Z/lustvend-python.log) |
| editor-build | passed | SugarLustUEEditor Win64 Development: Native target build succeeded. [Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorBuild-evidence.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorBuild-evidence.json) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/editor-build.log](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/editor-build.log) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/editor-build-console.log](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/editor-build-console.log) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorBuild-assessment.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorBuild-assessment.json) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/BuildConfiguration-EditorBuild.xml](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/BuildConfiguration-EditorBuild.xml) |
| shipping-build | passed | SugarLustUE Win64 Shipping: Native target build succeeded; selected compiler is allowed but not preferred. [Saved/ReleaseAudit/REL02-2026-11-11/native-current/ShippingBuild-evidence.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/ShippingBuild-evidence.json) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/shipping-build.log](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/shipping-build.log) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/shipping-build-console.log](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/shipping-build-console.log) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/ShippingBuild-assessment.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/ShippingBuild-assessment.json) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/BuildConfiguration-ShippingBuild.xml](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/BuildConfiguration-ShippingBuild.xml) |
| editor-automation | blocked | Unreal did not produce a fresh automation report; an exit code of zero does not verify test execution. [Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorTests-evidence.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorTests-evidence.json) [Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorTests-assessment.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorTests-assessment.json) |
| windows-package | not_run | Run Verify-Project.ps1 -Mode WindowsCheckpoint and import its evidence.  |

An Editor process exiting with code zero without a nonempty test report remains unverified. Isolated backend tests use fake accounts and storage. No live wagers or deployment changes are performed by this audit.

## Master task queue

| ID | System | Status | Column | Priority | Scope | Next action |
| --- | --- | --- | --- | --- | --- | --- |
| [REL-01](#rel-01) | Project identity and source control | incomplete | Verify | P0 | checkpoint | Verify a fresh origin/master clone with Git LFS, rebuild the native target from that checkout, and record the exact source revision and logs in source-baseline acceptance. |
| [REL-02](#rel-02) | Unreal native build | blocked | Verify | P0 | checkpoint | Run Tools/Release/Verify-NativeRelease.ps1 in a normal project PowerShell terminal to execute all 11 required tests and refresh native evidence. Review its report before closing REL-02. |
| [REL-03](#rel-03) | Windows packaged build and launch | blocked | Ready | P0 | checkpoint | Complete the checkpoint in an environment permitted to stage with the installed Unreal engine, then verify the exact packaged executable with fresh and returning accounts. |
| [REL-04](#rel-04) | Guest login and profile persistence | incomplete | Ready | P0 | checkpoint | Verify login failure states, save permissions, returning identity, profile load, and restart with a dedicated account. |
| [REL-05](#rel-05) | Authoritative coins and settlement | working | Verify | P0 | checkpoint | Verify deployed runtime and bridge hashes, then run dedicated-account replay, contention, and response-loss checks in staging. |
| [REL-06](#rel-06) | Sugar Lust base game | incomplete | Ready | P0 | checkpoint | Run the complete Sugar Lust loop in the new checkpoint and fix each observed defect before adding expansion gameplay. |
| [REL-07](#rel-07) | Neon Nights base game | incomplete | Ready | P1 | expansion | Verify Neon separately using the same release loop and preserve one shared wallet across game changes. |
| [REL-08](#rel-08) | Candy Jar bonus | incomplete | Verify | P0 | checkpoint | Test three picks, repeated cards, response loss on the final pick, and relaunch during the bonus. |
| [REL-09](#rel-09) | Free spins and Neon Heat | incomplete | Verify | P0 | checkpoint | Verify queued free spins and Neon Heat completion on a dedicated account without forced live rewards. |
| [REL-10](#rel-10) | Progressive jackpots | incomplete | Verify | P1 | checkpoint | Verify shared pool reads and deterministic staging jackpot awards with real storage; reconcile all displayed tiers. |
| [REL-11](#rel-11) | Daily rewards and streak | incomplete | Verify | P1 | checkpoint | Verify first claim, repeat claim, missed day, server rollover and countdown in staging. |
| [REL-12](#rel-12) | XP levels and missions | incomplete | Verify | P1 | checkpoint | Complete and claim one mission, verify duplicate protection, and compare XP across game changes and restart. |
| [REL-13](#rel-13) | Gem earnings and LUSTVEND transactions | working | Verify | P1 | checkpoint | Verify one supported purchase, duplicate submission, collection, gem milestone and restart against staging. |
| [REL-14](#rel-14) | Reward library and actual content | incomplete | Ready | P1 | checkpoint | Attach one approved supported reward, cook it, and verify opening, navigation, mute, repeat viewing and return. |
| [REL-15](#rel-15) | Cosmetic ownership and equipment | planned | Backlog | P2 | expansion | Implement server-validated entitlements and equip selections with compatible avatar slots and a packaged preview. |
| [REL-16](#rel-16) | Main menu and navigation | incomplete | Ready | P1 | checkpoint | Connect runtime avatar and event state, expose a shared event selector, and visibly label destinations whose actual systems are not ready. |
| [REL-17](#rel-17) | Casino floor movement seating and cabinets | incomplete | Verify | P1 | checkpoint | Run route, seating and widget lifetime tests, then test walking, sitting, play, standing and return on the actual floor. |
| [REL-18](#rel-18) | Spin Duel practice event | incomplete | Ready | P2 | expansion | Verify the entire labeled practice run, exit/resume, win/loss, held reels and return before defining any online rewards. |
| [REL-19](#rel-19) | Mega Lust Pot online game | blocked | Ready | P2 | expansion | Verify the prepared versioned bundle, deploy through controlled staging access, and test sync, wager replay, feature continuation and reconnect. |
| [REL-20](#rel-20) | Jester character production and deployment | incomplete | Ready | P1 | checkpoint | Complete one stable Jester record, inspect UVs and deformation, texture it, and connect the approved export to the real menu or scene. |
| [REL-21](#rel-21) | Time of day character presentation | planned | Backlog | P1 | checkpoint | Implement two configurable local-time windows, development preview, event-host precedence and asset fallback at safe menu transitions. |
| [REL-22](#rel-22) | Hot Streak with Pepper event | incomplete | Ready | P1 | checkpoint | Stage the matching runtime and bridge, enable a separate internal Sugar Lust event, and verify a three-win sequence, Mild title claim/equip, response-loss recovery and packaged presentation. |
| [REL-23](#rel-23) | Audio settings scaling and accessibility | incomplete | Ready | P1 | checkpoint | Verify settings persistence, mute behavior, payout priority, phone safe areas and every release-critical input path. |
| [REL-24](#rel-24) | Updater versioning and public distribution | blocked | Ready | P0 | checkpoint | Prepare a manifest from the accepted artifact/version and publish it only after those downloads exist and the release is approved. |
| [REL-25](#rel-25) | Android signed package and device behavior | blocked | Ready | P1 | expansion | Build with the retained signing identity and test the exact APK on the Motorola Edge 2024 without erasing an unrecovered guest account. |
| [REL-26](#rel-26) | Performance stability and release soak | blocked | Ready | P0 | checkpoint | Measure launch, floor, character, repeated spins, rewards, background/resume and repeated transitions on the exact candidate. |
| [REL-27](#rel-27) | Backend deployment operations and recovery | blocked | Ready | P0 | checkpoint | Verify deployed hashes and service health, staging access, private administrative endpoints, backups and a tested restore procedure. |
| [REL-28](#rel-28) | Friends and network social presence | planned | Backlog | P2 | expansion | Implement a bounded first social slice with requests, accepted friends, presence, block/remove and two-account verification. |
| [REL-29](#rel-29) | Inbox and notifications | planned | Backlog | P2 | expansion | Implement server-authored announcements with stable IDs, read/dismiss state, empty/loading/error states and expiry. |
| [REL-30](#rel-30) | VIP progression and rewards | planned | Backlog | P2 | expansion | Freeze one versioned VIP progression model and implement trusted progress, tier display and one durable reward claim. |
| [REL-31](#rel-31) | Store coin purchases and piggy bank | planned | Backlog | P2 | expansion | Choose the release channel and product model, then implement sandbox billing with server-verified idempotent delivery before enabling any purchase UI. |
| [REL-32](#rel-32) | Tournaments and additional games | planned | Backlog | P2 | expansion | Define and implement one versioned tournament with trusted scoring, deterministic tie-breaks, an end boundary and one claim path; scope each additional game as its own board record. |
| [REL-33](#rel-33) | Asset provenance and release content | blocked | Ready | P0 | checkpoint | Create an asset manifest linking each shipped asset to its source, terms, approval, AI provenance where applicable and intended platform; replace anything without a valid release path. |
| [REL-34](#rel-34) | Client interrupted-request recovery | incomplete | Ready | P0 | checkpoint | Verify the immutable-request fix, then persist the complete account-scoped request before send and reconcile it after login before new play. |
| [REL-35](#rel-35) | Master board and release verification workflow | working | Done | P0 | checkpoint | Keep this board current after each implementation and import real native, packaged and acceptance evidence. |

## Implementation order

1. P0 baseline: REL-01, REL-02, REL-03, REL-35. Resolve source identity, retain the baseline, run actual automation, and produce a new package.
2. P0 gameplay: REL-05, REL-34, REL-04, REL-06, REL-08, REL-09, REL-10, REL-11, REL-12, REL-13. Prove money, requests, persistence and the complete loop.
3. P1 presentation: REL-16, REL-17, REL-20, REL-23, then REL-21. Connect one approved character before expanding the roster.
4. P1 event slice: REL-14 and REL-22. Freeze the rules, deliver one supported Mild reward, and verify earning and claims in staging and the package.
5. Checkpoint acceptance: REL-03, REL-24, REL-26, REL-27, REL-33. Fix observed defects, verify operations/content, and review the exact build.
6. Full product completion: REL-07, REL-15, REL-18, REL-19, REL-25, REL-28, REL-29, REL-30, REL-31, REL-32. Each remains in this board until its real acceptance is working; none is silently removed from full release readiness.

Only one implementation task and one bounded character/art task should be In Progress. The focused checkpoint is an intermediate milestone; it does not convert planned expansion systems to working.

## Completion checks by system

## REL-01

**Project identity and source control** — incomplete; P0; Verify.

Evidence level: All 14 source commits were published to origin/master and installed locally at aa4b4c970d1c2156f3e2ca3b6d7f5c87a501d724. Local index matches 2,374 prepared paths; 2,068 LFS asset objects passed installation integrity checks. Original source commits remain on refs/heads/backup/pre-lfs-6cf1c73f97bc..

Source evidence: [SugarLustUE.uproject](../../SugarLustUE.uproject), [Source/SugarLustUE.Target.cs](../../Source/SugarLustUE.Target.cs), [Docs/Release/observations.json](../../Docs/Release/observations.json), [Docs/Release/SOURCE_UPLOAD.md](../../Docs/Release/SOURCE_UPLOAD.md), [Tools/Release/chunked-source-upload.js](../../Tools/Release/chunked-source-upload.js), [Tools/Release/chunked-source-upload.test.js](../../Tools/Release/chunked-source-upload.test.js), [Saved/ReleaseAudit/REL01-2026-11-11/chunked-upload/published.json](../../Saved/ReleaseAudit/REL01-2026-11-11/chunked-upload/published.json), [Saved/ReleaseAudit/REL01-2026-11-11/chunked-upload/installed.json](../../Saved/ReleaseAudit/REL01-2026-11-11/chunked-upload/installed.json).

Dependencies: none.

Current constraint: The upload and Git metadata blockers are resolved. A fresh independent clone with downloaded LFS content and a native rebuild has not yet been verified; source-baseline release acceptance remains pending.

Next action: Verify a fresh origin/master clone with Git LFS, rebuild the native target from that checkout, and record the exact source revision and logs in source-baseline acceptance.

- [ ] Retain the published.json and installed.json receipts and the original local backup branch.
- [ ] Clone origin/master independently, download its LFS content, and verify the source and asset inventory against the retained manifest.
- [ ] Rebuild the native target from that checkout and record matching commit, engine version, source fingerprint, and build evidence.

Acceptance: Project path, engine version, branch, exact commit, and source manifest agree; clean rebuild from a second checkout reproduces the candidate.

Required acceptance record: source-baseline. Release acceptance: pending.

## REL-02

**Unreal native build** — blocked; P0; Verify.

Evidence level: Current Unreal 5.8.4 Editor Development and standalone Win64 Shipping target checks succeeded on the working source fingerprint 6979603885715a556921e3e6889dad767b188145981ee973395fc010f0d0bfad. Both targets were up to date. Required automation inventory contains 11 tests; 13 native evidence policy tests and 14 readiness policy tests pass. Runtime automation has not executed in this session..

Source evidence: [Source/SugarLustUE/SugarLustUE.Build.cs](../../Source/SugarLustUE/SugarLustUE.Build.cs), [Source/SugarLustUEEditor.Target.cs](../../Source/SugarLustUEEditor.Target.cs), [Source/SugarLustUE.Target.cs](../../Source/SugarLustUE.Target.cs), [Config/UnrealBuildTool/BuildConfiguration.xml](../../Config/UnrealBuildTool/BuildConfiguration.xml), [Tools/Release/Verify-Project.ps1](../../Tools/Release/Verify-Project.ps1), [Tools/Release/Verify-NativeRelease.ps1](../../Tools/Release/Verify-NativeRelease.ps1), [Tools/Release/inspect-native-evidence.js](../../Tools/Release/inspect-native-evidence.js), [Docs/Release/NATIVE_VERIFICATION.md](../../Docs/Release/NATIVE_VERIFICATION.md), [Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorBuild-evidence.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorBuild-evidence.json), [Saved/ReleaseAudit/REL02-2026-11-11/native-current/ShippingBuild-evidence.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/ShippingBuild-evidence.json), [Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorTests-evidence.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/EditorTests-evidence.json), [Saved/ReleaseAudit/REL02-2026-11-11/native-current/toolchain-review.json](../../Saved/ReleaseAudit/REL02-2026-11-11/native-current/toolchain-review.json).

Dependencies: REL-01.

Current constraint: Editor startup inside this execution environment fails in Unreal machine-ID storage before logging initializes; no fresh automation report is created, despite exit zero. A debugger captured SetStoredValue -> LogfImpl during CSV profiler initialization. MSVC 14.51.36260 is allowed by the installed engine policy but is not preferred; its warning remains recorded.

Next action: Run Tools/Release/Verify-NativeRelease.ps1 in a normal project PowerShell terminal to execute all 11 required tests and refresh native evidence. Review its report before closing REL-02.

- [ ] Retain current Editor/Shipping receipts, the exact SDK configuration, compiler review, and the startup debugger diagnostic.
- [ ] Run the native verification command outside the restricted execution session and require fresh successful records for every discovered project test.
- [ ] Review the visible nonpreferred-toolchain warning; optionally select a serviced preferred compiler, rerun verification, and retain the real versions.
- [ ] Record final native/toolchain acceptance against the matching source and packaged candidate when REL-03 supplies that artifact.

Acceptance: Current source builds successfully, required automation tests execute, and toolchain versions are recorded.

Required acceptance record: toolchain-review. Release acceptance: pending.

## REL-03

**Windows packaged build and launch** — blocked; P0; Ready.

Evidence level: Current Editor and Windows Shipping targets compile. UAT reports cook completion; a new package is blocked during staging by an engine-owned intermediate file write..

Source evidence: [Tools/Release/Build-WindowsDemo.ps1](../../Tools/Release/Build-WindowsDemo.ps1), [Backend/V0.0.2_RELEASE_STATUS.md](../../Backend/V0.0.2_RELEASE_STATUS.md).

Dependencies: REL-02.

Current constraint: AutomationTool tries to write Engine/Intermediate/Build/BuildCookRun/StagedBuild_SugarLustUE.ini under Program Files, which this session cannot write. No new archive or interactive launch is verified.

Next action: Complete the checkpoint in an environment permitted to stage with the installed Unreal engine, then verify the exact packaged executable with fresh and returning accounts.

- [ ] Preserve the original v0.0.2 candidate.
- [ ] Build under a new Checkpoint directory with a source and artifact fingerprint.
- [ ] Test on a clean extraction; verify prerequisites, content cooking, launch, exit, and restart.

Acceptance: A versioned Shipping artifact matching current source launches and completes the release smoke checklist without a blocker.

Required acceptance record: windows-packaged-loop. Release acceptance: pending.

## REL-04

**Guest login and profile persistence** — incomplete; P0; Ready.

Evidence level: PlayFab CustomID login and profile restore exist; historical live verification is from October 31.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Backend/PlayFabCloudScript/cloudscript.js](../../Backend/PlayFabCloudScript/cloudscript.js).

Dependencies: REL-03, REL-05.

Current constraint: No current packaged fresh-account, relaunch, or guest-account recovery acceptance record.

Next action: Verify login failure states, save permissions, returning identity, profile load, and restart with a dedicated account.

- [ ] Test offline launch and unavailable login without silently enabling a live-wallet fallback.
- [ ] Test unwritable local identity storage.
- [ ] Define a supported account recovery or link path before promising cross-device accounts.

Acceptance: Fresh and returning accounts keep the same server-owned balances and pending features; access limitations are visible and no guest ID is erased.

Required acceptance record: live-account-loop. Release acceptance: pending.

## REL-05

**Authoritative coins and settlement** — working; P0; Verify.

Evidence level: 32 isolated bridge-to-gateway transaction tests passed on November 11.

Source evidence: [Backend/NakamaRuntime/economy-settlement.js](../../Backend/NakamaRuntime/economy-settlement.js), [Backend/NakamaRuntime/economy-domain.js](../../Backend/NakamaRuntime/economy-domain.js), [Backend/Tests/settlement.test.js](../../Backend/Tests/settlement.test.js).

Dependencies: none.

Current constraint: Current deployed hashes, real database concurrency, and load behavior are not attested by local tests.

Next action: Verify deployed runtime and bridge hashes, then run dedicated-account replay, contention, and response-loss checks in staging.

- [ ] Retain the permanent receipt and version-checked account model.
- [ ] Verify all enabled games against one authoritative wallet.
- [ ] Record real-database evidence before granting release acceptance.

Acceptance: Wager, payout, feature state, XP, gems, and durable receipt commit together; duplicate and concurrent requests never double-settle.

Required acceptance record: live-transaction-recovery. Release acceptance: pending.

## REL-06

**Sugar Lust base game** — incomplete; P0; Ready.

Evidence level: 5 by 3 and 20-payline client and authoritative rules exist; backend regressions pass.

Source evidence: [Source/SugarLustUE/SugarLustSlotMachine.cpp](../../Source/SugarLustUE/SugarLustSlotMachine.cpp), [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Backend/NakamaRuntime/index.core.js](../../Backend/NakamaRuntime/index.core.js).

Dependencies: REL-03, REL-04, REL-05, REL-34.

Current constraint: Current packaged controls, reels, visual payouts, low funds, and return flow lack direct acceptance evidence.

Next action: Run the complete Sugar Lust loop in the new checkpoint and fix each observed defect before adding expansion gameplay.

- [ ] Verify minimum and maximum wager and active-line changes.
- [ ] Compare displayed win highlights with the settled result.
- [ ] Record a normal spin, a loss, a win, low funds, a bonus, and return to menu.

Acceptance: Wagers, wins, wilds, paylines, controls, autoplay stop, rejection, navigation, and restart match authoritative results.

Required acceptance record: sugar-lust-loop. Release acceptance: pending.

## REL-07

**Neon Nights base game** — incomplete; P1; Ready.

Evidence level: 5 by 4 all-ways rules, feedback tests, and server settlement exist.

Source evidence: [Source/SugarLustUE/Tests/NeonNightsFeedbackTests.cpp](../../Source/SugarLustUE/Tests/NeonNightsFeedbackTests.cpp), [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp).

Dependencies: REL-06.

Current constraint: Packaged win feedback, Muse artwork, hold feature and device layout need verification.

Next action: Verify Neon separately using the same release loop and preserve one shared wallet across game changes.

- [ ] Run all Neon feedback automation tests in the actual rebuilt module.
- [ ] Test repeated entry and exit and cross-game balance updates.
- [ ] Verify symbols and payout audio on both release platforms.

Acceptance: Ways payouts, wilds, Muse, highlights, autoplay, feature transitions and resume match server results.

Required acceptance record: neon-nights-loop. Release acceptance: pending.

## REL-08

**Candy Jar bonus** — incomplete; P0; Verify.

Evidence level: Server picks and payout concurrency are covered by settlement tests.

Source evidence: [Backend/NakamaRuntime/economy-domain.js](../../Backend/NakamaRuntime/economy-domain.js), [Source/SugarLustUE/SugarLustSlotMachine.cpp](../../Source/SugarLustUE/SugarLustSlotMachine.cpp).

Dependencies: REL-06.

Current constraint: Current packaged pick animation, duplicate input, final collection, and restart recovery need playback verification.

Next action: Test three picks, repeated cards, response loss on the final pick, and relaunch during the bonus.

- [ ] Keep picks and payouts server-owned.
- [ ] Confirm presentation cannot accept input before the trusted response.
- [ ] Verify exit and resume at each pick count.

Acceptance: Only valid picks reveal; final reward pays once; pending cards and payout survive restart.

Required acceptance record: candy-jar-resume. Release acceptance: pending.

## REL-09

**Free spins and Neon Heat** — incomplete; P0; Verify.

Evidence level: Free wager, locking, retry, retrigger and bonus isolation backend checks passed.

Source evidence: [Backend/Tests/free-spins.test.js](../../Backend/Tests/free-spins.test.js), [Backend/NakamaRuntime/economy-domain.js](../../Backend/NakamaRuntime/economy-domain.js), [Source/SugarLustUE/Tests/NeonNightsFeedbackTests.cpp](../../Source/SugarLustUE/Tests/NeonNightsFeedbackTests.cpp).

Dependencies: REL-06, REL-07.

Current constraint: Visible pause, payout hold, cross-game pending state, and relaunch behavior remain unverified in the current package.

Next action: Verify queued free spins and Neon Heat completion on a dedicated account without forced live rewards.

- [ ] Test automatic and manual pause/resume.
- [ ] Verify Candy Jar and Neon Heat temporarily suspend free-spin continuation.
- [ ] Record restart during a feature and final completion.

Acceptance: Locked wagers, remaining counts and pending payouts persist; retries do not consume another round.

Required acceptance record: free-spin-resume. Release acceptance: pending.

## REL-10

**Progressive jackpots** — incomplete; P1; Verify.

Evidence level: Award, reset and credit atomicity pass isolated settlement tests.

Source evidence: [Backend/NakamaRuntime/index.core.js](../../Backend/NakamaRuntime/index.core.js), [Backend/JACKPOT_SYMBOLS_DEPLOYMENT.md](../../Backend/JACKPOT_SYMBOLS_DEPLOYMENT.md), [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp).

Dependencies: REL-05, REL-06.

Current constraint: Live pool consistency, meter refresh, feature presentation, and payout math review are outstanding.

Next action: Verify shared pool reads and deterministic staging jackpot awards with real storage; reconcile all displayed tiers.

- [ ] Keep progressive awards separate from paytable RTP.
- [ ] Verify delayed meter responses cannot overwrite newer data.
- [ ] Measure contribution and jackpot behavior from the locked release configuration.

Acceptance: Contribution, award, pool reset, wallet credit and receipt are consistent; presentation uses the server tier and amount.

Required acceptance record: progressive-staging. Release acceptance: pending.

## REL-11

**Daily rewards and streak** — incomplete; P1; Verify.

Evidence level: Authoritative daily claim versus wager race passes isolated testing.

Source evidence: [Backend/NakamaRuntime/economy-domain.js](../../Backend/NakamaRuntime/economy-domain.js), [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp).

Dependencies: REL-04, REL-05, REL-16.

Current constraint: Current menu claim, UTC reset display, duplicate tap and relaunch need verification.

Next action: Verify first claim, repeat claim, missed day, server rollover and countdown in staging.

- [ ] Keep event Heat separate from the existing daily streak.
- [ ] Test claim while a wager settles on another device.
- [ ] Verify an unavailable service leaves a useful retry state.

Acceptance: One eligible reward is granted per server day and the UI reflects authoritative eligibility and streak.

Required acceptance record: daily-reward-loop. Release acceptance: pending.

## REL-12

**XP levels and missions** — incomplete; P1; Verify.

Evidence level: Mission claim races, XP settlement and server-owned mission records are implemented.

Source evidence: [Backend/NakamaRuntime/economy-domain.js](../../Backend/NakamaRuntime/economy-domain.js), [Source/SugarLustUE/SugarLustSlotMachine.cpp](../../Source/SugarLustUE/SugarLustSlotMachine.cpp), [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp).

Dependencies: REL-05, REL-16.

Current constraint: Packaged progress feedback, daily reset, level display, and mission claim persistence lack current acceptance evidence.

Next action: Complete and claim one mission, verify duplicate protection, and compare XP across game changes and restart.

- [ ] Verify local demo missions never write the online economy.
- [ ] Confirm claim errors recover with the original claim identity.
- [ ] Document and verify any level-based access gates before advertising them.

Acceptance: Only trusted progress and rewards apply; displayed levels and mission states survive relaunch.

Required acceptance record: missions-and-xp. Release acceptance: pending.

## REL-13

**Gem earnings and LUSTVEND transactions** — working; P1; Verify.

Evidence level: 19 Nakama and PlayFab tests passed; Python HTTP transport has a separate socket blocker.

Source evidence: [Backend/NakamaRuntime/test_lustvend.js](../../Backend/NakamaRuntime/test_lustvend.js), [Backend/NakamaRuntime/index.core.js](../../Backend/NakamaRuntime/index.core.js), [Source/SugarLustUE/LustVendWidget.cpp](../../Source/SugarLustUE/LustVendWidget.cpp).

Dependencies: REL-05.

Current constraint: Local HTTP transport and real packaged purchase/reconnect acceptance are not complete.

Next action: Verify one supported purchase, duplicate submission, collection, gem milestone and restart against staging.

- [ ] Preserve original purchase IDs through uncertain responses.
- [ ] Keep ownership and currency changes atomic.
- [ ] Retest socket transport in an environment that permits loopback networking.

Acceptance: Server prices, gem debit, ownership and receipt reconcile exactly; empty rewards cannot be presented as delivered content.

Required acceptance record: lustvend-live-loop. Release acceptance: pending.

## REL-14

**Reward library and actual content** — incomplete; P1; Ready.

Evidence level: Ownership-gated image and video viewer exists; configured media slots are empty.

Source evidence: [Source/SugarLustUE/LustVendWidget.cpp](../../Source/SugarLustUE/LustVendWidget.cpp), [Backend/LustVend/REWARD_LIBRARY.md](../../Backend/LustVend/REWARD_LIBRARY.md), [Config/DefaultGame.ini](../../Config/DefaultGame.ini).

Dependencies: REL-13, REL-33.

Current constraint: Approved reward media and packaged playback evidence have not been supplied.

Next action: Attach one approved supported reward, cook it, and verify opening, navigation, mute, repeat viewing and return.

- [ ] Use stable SKU-to-content records.
- [ ] Validate video paths and supported formats.
- [ ] Do not sell a placeholder as completed media.

Acceptance: Owned rewards show actual approved content on both platforms without another debit; missing content stays clearly unavailable.

Required acceptance record: reward-content-playback. Release acceptance: pending.

## REL-15

**Cosmetic ownership and equipment** — planned; P2; Backlog.

Evidence level: Avatar and theme packages currently offer previews; no wearable equipment path found.

Source evidence: [Backend/LustVend/REWARD_LIBRARY.md](../../Backend/LustVend/REWARD_LIBRARY.md), [Config/DefaultGame.ini](../../Config/DefaultGame.ini).

Dependencies: REL-13, REL-20.

Current constraint: Typed cosmetic catalog, equipment slots and compatible approved assets are missing.

Next action: Implement server-validated entitlements and equip selections with compatible avatar slots and a packaged preview.

- [ ] Define RewardId, RewardKind, EquipSlot and CompatibleAvatarIds.
- [ ] Commit equipment changes with a durable idempotent request.
- [ ] Keep the first Pepper reward on a supported presentation path until wearable production is complete.

Acceptance: Only owned compatible items equip, persist across restart and render correctly without changing currency twice.

Required acceptance record: cosmetic-equipment. Release acceptance: pending.

## REL-16

**Main menu and navigation** — incomplete; P1; Ready.

Evidence level: Runtime hit targets and primary destinations exist; event entry is hardwired to Spin Duel.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Content/Art/MainMenu/T_MainMenu.uasset](../../Content/Art/MainMenu/T_MainMenu.uasset).

Dependencies: REL-03.

Current constraint: Selected-avatar presentation and event selector are absent; several menu buttons lead to notices.

Next action: Connect runtime avatar and event state, expose a shared event selector, and visibly label destinations whose actual systems are not ready.

- [ ] Preserve access to Spin Duel practice.
- [ ] Verify Daily Rewards and casino-floor entry from the actual menu artwork.
- [ ] Test every destination and close/back path in the packaged build.

Acceptance: Every visible active destination works; unavailable states are explicit; player numbers are runtime text; keyboard and touch navigation return safely.

Required acceptance record: menu-navigation. Release acceptance: pending.

## REL-17

**Casino floor movement seating and cabinets** — incomplete; P1; Verify.

Evidence level: Venue routes, seating and input automation exist; local floor is a prototype.

Source evidence: [Source/SugarLustUE/SocialCasinoPrototype.cpp](../../Source/SugarLustUE/SocialCasinoPrototype.cpp), [Source/SugarLustUE/CasinoFloorRoute.h](../../Source/SugarLustUE/CasinoFloorRoute.h), [Source/SugarLustUE/CasinoCabinetControls.cpp](../../Source/SugarLustUE/CasinoCabinetControls.cpp).

Dependencies: REL-16, REL-06.

Current constraint: Current packaged stage routes, cabinet framing, touch targets and repeated enter/leave need playback verification.

Next action: Run route, seating and widget lifetime tests, then test walking, sitting, play, standing and return on the actual floor.

- [ ] Verify streaming and authored map cooking.
- [ ] Measure collisions and camera framing on device.
- [ ] Separate prototype NPCs from any claim of network multiplayer.

Acceptance: No clipping or softlock on supported routes; cabinet controls retain account session and safe focus; floor exits restore the menu.

Required acceptance record: casino-floor-loop. Release acceptance: pending.

## REL-18

**Spin Duel practice event** — incomplete; P2; Ready.

Evidence level: Practice encounter rules and actor/streamed grove implementation exist.

Source evidence: [Source/SugarLustUE/SpinDuelRun.cpp](../../Source/SugarLustUE/SpinDuelRun.cpp), [Source/SugarLustUE/SpinDuelRules.h](../../Source/SugarLustUE/SpinDuelRules.h), [Source/SugarLustUE/Tests/SpinDuelRulesTests.cpp](../../Source/SugarLustUE/Tests/SpinDuelRulesTests.cpp).

Dependencies: REL-16, REL-17.

Current constraint: Practice flow is not an online seasonal reward event; packaged grove loading and mobile controls need verification.

Next action: Verify the entire labeled practice run, exit/resume, win/loss, held reels and return before defining any online rewards.

- [ ] Keep practice entry independent of the featured event.
- [ ] Test missing-map fallback and cleanup.
- [ ] Treat online event economy and rewards as a separately versioned implementation if later required.

Acceptance: Practice works end to end with clear temporary scoring and cannot debit or grant the online wallet.

Required acceptance record: spin-duel-practice. Release acceptance: pending.

## REL-19

**Mega Lust Pot online game** — blocked; P2; Ready.

Evidence level: 16 isolated transactional tests pass; online client recovery and rule simulation exist.

Source evidence: [Source/SugarLustUE/MegaLustPotOnline.cpp](../../Source/SugarLustUE/MegaLustPotOnline.cpp), [Backend/MEGA_POT_ONLINE_DEPLOYMENT.md](../../Backend/MEGA_POT_ONLINE_DEPLOYMENT.md), [Backend/Tests/mega-pot.test.js](../../Backend/Tests/mega-pot.test.js).

Dependencies: REL-05, REL-27.

Current constraint: Deployment guide explicitly states the prepared Mega Pot bundle has not been enabled remotely; final art and device tests remain.

Next action: Verify the prepared versioned bundle, deploy through controlled staging access, and test sync, wager replay, feature continuation and reconnect.

- [ ] Preserve the existing deployed economy and receipts.
- [ ] Verify runtime and bridge compatibility and versioned math.
- [ ] Complete art, cabinet fit and platform acceptance before enabling public entry.

Acceptance: Online pot counters and locked wagers persist per bet with one shared wallet; no local practice fallback can settle currency.

Required acceptance record: mega-pot-live-loop. Release acceptance: pending.

## REL-20

**Jester character production and deployment** — incomplete; P1; Ready.

Evidence level: JesterTest and JesterTestV2 meshes, skeletons and animations plus a test map are present.

Source evidence: [Content/Art/Characters/JesterTestV2/SK_LHC_Jester_Clean.uasset](../../Content/Art/Characters/JesterTestV2/SK_LHC_Jester_Clean.uasset), [Content/Maps/Tests/JesterMovementTest.umap](../../Content/Maps/Tests/JesterMovementTest.umap), [Source/SugarLustUE/SocialCasinoPrototype.cpp](../../Source/SugarLustUE/SocialCasinoPrototype.cpp).

Dependencies: REL-33.

Current constraint: Current main-game code uses other prototype people; final textures, UV/deformation review, approved asset version and performance evidence are absent.

Next action: Complete one stable Jester record, inspect UVs and deformation, texture it, and connect the approved export to the real menu or scene.

- [ ] Link Character Studio and Production IDs instead of creating another roster.
- [ ] Review hands, feet, arm spacing, hips, shoulders, wrists and skeleton compatibility.
- [ ] Choose one approved export; leave test variants out of the public character selection.

Acceptance: One approved character renders, animates and fits the real scene, with reproducible source/rig/material records and measured device cost.

Required acceptance record: jester-visual-and-performance. Release acceptance: pending.

## REL-21

**Time of day character presentation** — planned; P1; Backlog.

Evidence level: No integrated schedule or safe-boundary host swap found.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Source/SugarLustUE/SugarLustGameInstance.cpp](../../Source/SugarLustUE/SugarLustGameInstance.cpp).

Dependencies: REL-20, REL-16.

Current constraint: One approved deployable character and presentation record are prerequisites.

Next action: Implement two configurable local-time windows, development preview, event-host precedence and asset fallback at safe menu transitions.

- [ ] Use player-local time for presentation and server time for reward/event deadlines.
- [ ] Begin with one character and two looks if that is the available approved art.
- [ ] Persist user selection and entitlement rules where required.

Acceptance: Both windows, midnight boundaries, event override and missing assets are predictable; swaps do not interrupt spins or mandatory presentations.

Required acceptance record: presentation-windows. Release acceptance: pending.

## REL-22

**Hot Streak with Pepper event** - incomplete; P1; Ready.

Evidence level: Local mechanics, atomic Heat and title claims, account subsystem, shared event panel, slot/cabinet HUDs and queued text reactions implemented; 30 event backend tests and 12 Unreal automation tests passed; staging acceptance remains pending.

Source evidence: [Backend/NakamaRuntime/hot-streak-rules.js](../../Backend/NakamaRuntime/hot-streak-rules.js), [Backend/NakamaRuntime/economy-domain.js](../../Backend/NakamaRuntime/economy-domain.js), [Source/SugarLustUE/HotStreakSubsystem.cpp](../../Source/SugarLustUE/HotStreakSubsystem.cpp), [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Docs/HOT_STREAK_IMPLEMENTATION.md](../HOT_STREAK_IMPLEMENTATION.md).

Dependencies: REL-05, REL-06, REL-14, REL-16.

Current constraint: Backend and disabled internal configuration require staging deployment and real storage/reconnect acceptance. Pepper portraits and wearable rewards remain pending.

Next action: Stage the matching runtime and bridge, enable a separate internal Sugar Lust event, and verify a three-win sequence, Mild title claim/equip, response-loss recovery and packaged presentation.

- [ ] Use the existing implementation plan; add atomic progress and permanent claim receipts to authoritative settlement.
- [ ] Choose a supported owned media or presentation reward before offering an unimplemented wearable.
- [ ] Validate staging and packaged play before adding Neon support or the full reward ladder.

Acceptance: W W W W L yields Heat 0 0 1 2 2; free and bonus rounds freeze progress; restart/replay/concurrency and deadlines never duplicate rewards.

Required acceptance record: pepper-first-claim. Release acceptance: pending.

## REL-23

**Audio settings scaling and accessibility** — incomplete; P1; Ready.

Evidence level: Music/SFX controls, scaling, procedural payout audio and cues exist.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Source/SugarLustUE/Tests/NeonNightsFeedbackTests.cpp](../../Source/SugarLustUE/Tests/NeonNightsFeedbackTests.cpp).

Dependencies: REL-16, REL-17.

Current constraint: Actual audio playback, safe areas, readable UI and keyboard/touch focus are not verified in current packages.

Next action: Verify settings persistence, mute behavior, payout priority, phone safe areas and every release-critical input path.

- [ ] Test 1920 by 1080, wide phones and cabinet overlays.
- [ ] Verify settings survive rebuilds and relaunch.
- [ ] Profile UI and avoid critical information baked into artwork.

Acceptance: Audio respects settings without overlaps; text and controls remain usable at target sizes; motion and voiced dialogue have suitable controls/subtitles when present.

Required acceptance record: audio-input-layout. Release acceptance: pending.

## REL-24

**Updater versioning and public distribution** — blocked; P0; Ready.

Evidence level: Local 0.0.2 differs from public update.json 0.1.0 verified on November 11.

Source evidence: [Source/SugarLustUE/SugarLustUpdatePolicy.cpp](../../Source/SugarLustUE/SugarLustUpdatePolicy.cpp), [Config/DefaultGame.ini](../../Config/DefaultGame.ini), [Backend/V0.0.2_RELEASE_STATUS.md](../../Backend/V0.0.2_RELEASE_STATUS.md), [Docs/Release/observations.json](../../Docs/Release/observations.json).

Dependencies: REL-03, REL-33.

Current constraint: The live manifest advertises a different version; candidate URLs and final release selection must be coordinated.

Next action: Prepare a manifest from the accepted artifact/version and publish it only after those downloads exist and the release is approved.

- [ ] Run update policy automation against malformed versions and unsafe URLs.
- [ ] Verify clean install and update guidance.
- [ ] Keep preparation separate from public publishing until the review build passes.

Acceptance: Project, Android version, packaged executable and published manifest agree; only allowed HTTPS downloads are offered.

Required acceptance record: distribution-manifest. Release acceptance: pending.

## REL-25

**Android signed package and device behavior** — blocked; P1; Ready.

Evidence level: October 31 release APK and signature audit exist; no current physical-device result.

Source evidence: [Tools/Release/Build-AndroidDemo.ps1](../../Tools/Release/Build-AndroidDemo.ps1), [Backend/V0.0.2_RELEASE_STATUS.md](../../Backend/V0.0.2_RELEASE_STATUS.md), [Config/DefaultEngine.ini](../../Config/DefaultEngine.ini).

Dependencies: REL-03, REL-23.

Current constraint: Current release certificate, device acceptance and exact release channel requirements need verification; guest identity must survive installation handling.

Next action: Build with the retained signing identity and test the exact APK on the Motorola Edge 2024 without erasing an unrecovered guest account.

- [ ] Keep private keys/passwords outside source and public artifacts.
- [ ] Verify the target channel SDK requirements before store submission.
- [ ] Retain artifact hashes and signing verification with the matching source fingerprint.

Acceptance: Signed non-debuggable package installs through the intended channel, includes data, and passes gameplay, feature resume, layout, relaunch and performance checks.

Required acceptance record: android-device-loop. Release acceptance: pending.

## REL-26

**Performance stability and release soak** — blocked; P0; Ready.

Evidence level: Historical crashes and builds exist; no measured current soak or target-device performance report.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Source/SugarLustUE/SocialCasinoPrototype.cpp](../../Source/SugarLustUE/SocialCasinoPrototype.cpp).

Dependencies: REL-03, REL-20, REL-23.

Current constraint: A chosen test device, accepted frame/memory budget and instrumented current checkpoint are needed.

Next action: Measure launch, floor, character, repeated spins, rewards, background/resume and repeated transitions on the exact candidate.

- [ ] Record the device and graphics settings.
- [ ] Use profiling evidence to choose LOD, material and texture changes.
- [ ] Re-run the soak after changes affecting assets, streaming or presentation.

Acceptance: No crashes, balance defects or softlocks during the agreed soak; measured frame time and memory stay inside the recorded device budget.

Required acceptance record: performance-soak. Release acceptance: pending.

## REL-27

**Backend deployment operations and recovery** — blocked; P0; Ready.

Evidence level: Deployment and migration helpers exist; no current service access or restore evidence.

Source evidence: [Backend/SETTLEMENT_DEPLOYMENT.md](../../Backend/SETTLEMENT_DEPLOYMENT.md), [Tools/Backend/finish-settlement-setup.js](../../Tools/Backend/finish-settlement-setup.js), [Backend/NakamaRuntime/README.md](../../Backend/NakamaRuntime/README.md).

Dependencies: REL-05.

Current constraint: Current deployment access, monitoring, limits, backups and real restore results are unavailable locally.

Next action: Verify deployed hashes and service health, staging access, private administrative endpoints, backups and a tested restore procedure.

- [ ] Use dedicated tester accounts and never log service credentials.
- [ ] Keep supported forward fixes for authoritative data.
- [ ] Record a maintenance and incident runbook with evidence of recovery.

Acceptance: The exact backend release is known, monitored, limited and recoverable without resetting accounts or permanent receipts.

Required acceptance record: service-operations. Release acceptance: pending.

## REL-28

**Friends and network social presence** — planned; P2; Backlog.

Evidence level: Friends button explicitly says multiplayer is not connected.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Source/SugarLustUE/SocialCasinoPrototype.cpp](../../Source/SugarLustUE/SocialCasinoPrototype.cpp).

Dependencies: REL-04, REL-27.

Current constraint: Account linking, social data model, permissions and real network room authority are not implemented.

Next action: Implement a bounded first social slice with requests, accepted friends, presence, block/remove and two-account verification.

- [ ] Choose one authoritative social backend.
- [ ] Implement authenticated requests and user privacy controls.
- [ ] Add room synchronization only after friend and presence state work.

Acceptance: Two real test accounts can connect and disconnect consistently; private data and blocked users remain protected; local NPCs are never described as live players.

Required acceptance record: friends-two-account. Release acceptance: pending.

## REL-29

**Inbox and notifications** — planned; P2; Backlog.

Evidence level: Inbox currently displays an empty notice without an inbox data model.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp).

Dependencies: REL-04, REL-27.

Current constraint: Persistent message records, delivery identity and read/dismiss APIs are missing.

Next action: Implement server-authored announcements with stable IDs, read/dismiss state, empty/loading/error states and expiry.

- [ ] Start with announcements rather than unrestricted player messaging.
- [ ] Use a separate durable claim transaction for any attached reward.
- [ ] Verify pagination, duplicate delivery and read-state synchronization.

Acceptance: Messages persist correctly per account and cannot grant a reward twice; relaunch and expiry preserve consistent unread state.

Required acceptance record: inbox-delivery. Release acceptance: pending.

## REL-30

**VIP progression and rewards** — planned; P2; Backlog.

Evidence level: VIP destination is a coming-soon notice; Mint is a character concept rather than an integrated VIP system.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp).

Dependencies: REL-05, REL-13, REL-15.

Current constraint: VIP qualification rules, authoritative progress, reward schedule and supported content need design and implementation.

Next action: Freeze one versioned VIP progression model and implement trusted progress, tier display and one durable reward claim.

- [ ] Keep VIP progression distinct from character-host scheduling.
- [ ] Add the supported reward path before advertising benefits.
- [ ] Test downgrade, rollover and rule-version migration where applicable.

Acceptance: Progress and benefits are reproducible from trusted activity, correctly displayed and granted once across retries and restart.

Required acceptance record: vip-progression-claim. Release acceptance: pending.

## REL-31

**Store coin purchases and piggy bank** — planned; P2; Backlog.

Evidence level: Store, coin purchase and piggy-bank buttons currently show notices.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp).

Dependencies: REL-05, REL-27, REL-33.

Current constraint: Product definitions, platform billing, server receipt verification and delivery/recovery are absent.

Next action: Choose the release channel and product model, then implement sandbox billing with server-verified idempotent delivery before enabling any purchase UI.

- [ ] Keep unavailable purchases visibly disabled in the checkpoint.
- [ ] Implement a piggy-bank ledger only after its earning and release rules are frozen.
- [ ] Use billing sandbox evidence and reviewed product disclosures before public activation.

Acceptance: Only verified receipts credit the correct account once; replay, cancellation, pending payment, restoration and channel-specific reversals are tested.

Required acceptance record: billing-sandbox. Release acceptance: pending.

## REL-32

**Tournaments and additional games** — planned; P2; Backlog.

Evidence level: Tournament and extra game cards are coming-soon notices.

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp).

Dependencies: REL-05, REL-22, REL-28.

Current constraint: No integrated tournament rules, ranking, tie-break, settlement or approved additional game specification.

Next action: Define and implement one versioned tournament with trusted scoring, deterministic tie-breaks, an end boundary and one claim path; scope each additional game as its own board record.

- [ ] Capture individual future games in this board rather than treating a card as implementation.
- [ ] Test multiple accounts and event boundaries in staging.
- [ ] Keep new game math and reward cost separate from presentation art.

Acceptance: Ranking and final rewards remain correct under simultaneous play, late results, replay and restart; each enabled game has its own full-loop acceptance.

Required acceptance record: tournament-finalization. Release acceptance: pending.

## REL-33

**Asset provenance and release content** — blocked; P0; Ready.

Evidence level: Imported art, music, voice and third-party characters exist; no complete release asset manifest was found.

Source evidence: [Content/Art](../../Content/Art), [Backend/V0.0.2_RELEASE_STATUS.md](../../Backend/V0.0.2_RELEASE_STATUS.md).

Dependencies: none.

Current constraint: Approved release assets, redistribution evidence and final content/channel classifications must be supplied or reviewed.

Next action: Create an asset manifest linking each shipped asset to its source, terms, approval, AI provenance where applicable and intended platform; replace anything without a valid release path.

- [ ] Keep editable character sources separate from approved game exports.
- [ ] Exclude test-only character variants and unused placeholder rewards from the public roster.
- [ ] Record the intended distribution channel and its content disclosures.

Acceptance: Every shipped asset has a retained usable provenance record and approved public presentation; advertised rewards match actual content.

Required acceptance record: release-content-review. Release acceptance: pending.

## REL-34

**Client interrupted-request recovery** — incomplete; P0; Ready.

Evidence level: Original spin parameters are now retained for Sugar Lust and Neon manual retries, and pending wagers are locked. Server receipts are durable; restart recovery remains incomplete..

Source evidence: [Source/SugarLustUE/SugarLustRootWidget.cpp](../../Source/SugarLustUE/SugarLustRootWidget.cpp), [Source/SugarLustUE/MegaLustPotOnline.cpp](../../Source/SugarLustUE/MegaLustPotOnline.cpp), [Backend/Tests/settlement.test.js](../../Backend/Tests/settlement.test.js).

Dependencies: REL-05.

Current constraint: Primary slot pending requests are still held in widget memory. Durable journal recovery, definitive rejection handling and current packaged acceptance remain unfinished.

Next action: Verify the immutable-request fix, then persist the complete account-scoped request before send and reconcile it after login before new play.

- [ ] Implemented locally: retain original wager parameters and prevent wager/line changes while a request is unresolved; compile and run PendingSpinIdentity automation.
- [ ] Add the durable account-scoped journal and a trusted definite-rejection contract; keep ambiguous failures pending.
- [ ] Test result-parse failure, commit-then-response-loss, application restart, corrupted journal and cross-account callbacks.

Acceptance: Disconnect, wager changes, scene change and application restart always recover the original request; no changed fingerprint, duplicate charge or lost unresolved operation.

Required acceptance record: client-request-recovery. Release acceptance: pending.

## REL-35

**Master board and release verification workflow** — working; P0; Done.

Evidence level: 35 system records validated; executable audit generates the single board; 13 release policy tests pass and prevent false release acceptance..

Source evidence: [Docs/Release/systems.json](../../Docs/Release/systems.json), [Tools/Release/audit-release.js](../../Tools/Release/audit-release.js), [Tools/Release/Verify-Project.ps1](../../Tools/Release/Verify-Project.ps1).

Dependencies: none.

Current constraint: Full product acceptance is still blocked by the explicitly recorded system dependencies; the board workflow itself is locally verified.

Next action: Keep this board current after each implementation and import real native, packaged and acceptance evidence.

- [ ] Use systems.json as the task record and MASTER_BOARD.md as its generated readable view.
- [ ] Keep specialist character records linked from the board.
- [ ] Limit active implementation to the highest-priority unresolved dependency and one bounded art task.

Acceptance: Every system has a status, priority, dependency, next action and acceptance evidence; the full release gate remains blocked until every system and required check is verified.

Required acceptance record: board-review. Release acceptance: pending.

## Updating this board

Run `node Tools/Release/audit-release.js` to execute local checks and regenerate this board. Exit 2 means release evidence is still blocked; exit 1 means a required automated check failed. Use `--unreal-evidence Saved/ReleaseAudit/<run>` to import native build, automation and checkpoint evidence. `--reuse-checks` reuses recorded checks without refreshing their timestamps; changed source or stale evidence still blocks readiness.

Use `Tools/Release/Verify-Project.ps1 -Mode EditorBuild`, `-Mode EditorTests`, and `-Mode WindowsCheckpoint` with the same output directory and the audit source fingerprint. Copy acceptance.template.json to acceptance.json only when recording real tests. Each passed acceptance needs a tester, UTC timestamp, matching package fingerprint and retained evidence. A template is never acceptance.

Historical deployment notes remain reference evidence. They are not parallel game task boards. The original November 11 advancement plan and November 9 Pepper design remain design inputs.
