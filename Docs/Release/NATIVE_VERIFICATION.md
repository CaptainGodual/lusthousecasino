# REL-02: native build verification

REL-02 covers the current Editor target, standalone Shipping compilation,
toolchain identification, and execution of the required project tests.
Windows cooking, staging, packaging, and interactive acceptance remain REL-03.

## Verification command

Run from a normal PowerShell terminal in the project:

    .\Tools\Release\Verify-NativeRelease.ps1

This builds SugarLustUEEditor Win64 Development, checks SugarLustUE Win64
Shipping, runs the SugarLust automation suite, and updates the master board.
Node is located automatically from PATH, a normal installation, or the
Codex runtime. -NodePath accepts an explicit executable; -CheckRuntime
checks discovery without starting builds.

The default evidence directory is:

    Saved/ReleaseAudit/REL02-2026-11-11/native-current

For a focused test rerun after successful compilation:

    .\Tools\Release\Verify-Project.ps1 -Mode EditorTests -OutputDirectory Saved\ReleaseAudit\REL02-2026-11-11\native-current

Then import the evidence and refresh the board:

    .\Tools\Release\Verify-NativeRelease.ps1

Keep the source stable during verification. The verifier calculates the
current fingerprint itself, validates any supplied fingerprint, and checks
again after each phase. Configuration XML now participates in that identity.
Each native receipt records its source commit, engine version, Node version,
available compiler-selection evidence, warnings, and retained logs.

## Build configuration

Config/UnrealBuildTool/BuildConfiguration.xml is the tracked verification
template. UnrealBuildTool actually reads the project override from
Saved/UnrealBuildTool/BuildConfiguration.xml; the verifier copies the template
there and retains the previous file in its output directory.

The SDK setting is correctly nested under WindowsPlatform. The template pins
the installed Windows SDK 10.0.28000.0 and configures artifact reads/writes and
build accelerators explicitly. Cache and log paths remain under the project.
UE 5.8 still uses its UBA local executor when acceleration is disabled;
the engine's ExecutorFactory disables detouring in that case. A UBA local
executor log line is therefore not proof that the disable switch was ignored.

ShippingBuild compiles without cooking or staging. A successful Shipping
compile does not establish a complete Windows package.

## Toolchain review

The installed compiler family is 14.51.36231, with compiler version
14.51.36260. The installed engine policy prefers 14.50.35717-14.50.99999
or 14.44.35207-14.44.99999, but bans the early releases below
14.50.35723 and 14.44.35211 within those families.

The current compiler is above the minimum and outside the banned ranges;
it is not preferred. Its warning remains visible and is recorded in the
native evidence. This is local build evidence, not a claim that Epic has
validated this compiler for release. Selecting a preferred toolchain requires
installing an allowed serviced 14.50 or 14.44 toolset through Visual Studio
Installer and rerunning the checks. No system toolset is changed by these
scripts.

An up-to-date target may report the selected compiler warning without
repeating the SDK/compiler-path line. The evidence preserves that distinction
and does not invent a selected SDK version; the exact verification SDK
configuration is retained alongside the build logs.

## Automation acceptance

The test inventory is discovered from Source/SugarLustUE/Tests. It currently
contains 11 named SugarLust tests. A passing report must be fresh, contain
every required name exactly once, have successful states, and come from a
successful process. Missing, stale, partial, duplicate, malformed, failed,
and not-run results cannot establish acceptance. Zero exit status alone
does not establish execution.

The current Codex execution environment cannot complete Editor startup.
A debugger captured the following call path before any Editor log or test
report was created:

    FLowLevelMemTracker::ProcessCommandLine
    FCsvProfiler::SetMetadata / FCsvProfiler::Get
    FGenericPlatformMisc::GetLoginId / GetMachineId
    FWindowsPlatformMisc::SetStoredValue
    FOutputDevice::LogfImpl (null output device)

The installed engine's SetStoredValue error path dereferences GWarn after a
registry write fails. This session cannot complete that machine-ID registry
write, and the logging device has not yet been initialized at this point.
The process can consequently return zero without running any tests.

Retained diagnostics:

- Saved/ReleaseAudit/REL02-2026-11-11/automation-probe/probe.json
- Saved/ReleaseAudit/REL02-2026-11-11/debugger-startup-failure/console.log

Run the normal-terminal verification command above to produce genuine test
evidence. The verifier does not patch the installed engine, alter machine
identity, or turn the missing report into a pass.

Reference: [Epic's automation command-line documentation](https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine).
