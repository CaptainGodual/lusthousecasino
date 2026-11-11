# Recovering the first source upload

The active project is SugarLustUE.uproject. The original local root is
7d5c2fe525ad4dcc938842e8aab7c69e267d040c. That root commit contains
2,369 files and 5,957,392,317 bytes of uncompressed content (about 5.55 GiB).
Seven Unreal packages exceed GitHub's 100 MiB raw-file limit. Repeatedly pushing
the same commit cannot resolve those individual-file limits. The current local branch is main at
6cf1c73f97bcc0771c3d7b93cc6231c0291e7458, including a second local commit
with the initial uploader. Both commits are retained in the input history.

## Prepared replacement

Tools/Release/chunked-source-upload.js prepares an independent commit chain
under Saved/ReleaseAudit/REL01-2026-11-11/chunked-upload. It uses the original
commit as a read-only source and checks the original bytes before converting
Unreal packages and production media into canonical Git LFS pointers.

The first commit contains project code, configuration, backend code, and LFS
attributes. Asset commits target 512 MiB of original payload each. A single
asset larger than that target stays whole in its own commit; the largest
current file is about 612 MiB. The last commit brings the release notes and
upload tooling up to date. The prepared UPLOAD_PLAN.md records each exact
commit and payload size.

Preparation verifies original asset hashes, preservation of every original
path, the new commit order, absence of oversized raw Git blobs, and official
Git LFS integrity checks. It preserves the active HEAD, index, asset files,
and player data. The new attributes file is kept in the prepared history until
installation, so the current raw-asset checkout is not changed prematurely.

## Upload and install

Close any other running push for this project. From a normal PowerShell
terminal in the project root, run:

    .\Tools\Release\Publish-SourceChunks.ps1

The launcher locates Node on PATH, in a standard installation, or in the
existing Codex runtime. It does not require a separate Node installation when
that bundled runtime is available. For another runtime location, supply
-NodePath followed by the full path to node.exe. Add -CheckRuntime to check
runtime discovery without starting an upload.

This command uploads LFS objects and one Git commit at a time to origin/master.
It checks the remote after each commit. Run the same command again after an
interruption; it resumes from the actual remote head. It stops if that branch
contains unrelated work and never force pushes. The existing origin/main
updater branch remains unchanged.

After the final remote commit is verified, the command preserves the original
input tip on backup/pre-lfs-6cf1c73f97bc, installs the prepared history
and LFS object cache, selects local master, and configures it to track
origin/master. The original local main remains at its input tip. It updates the index without checking out or deleting working
files. Do not push all local branches: the backup intentionally retains the
original oversized root.

The local script.bat is preserved as user work and is excluded from the upload.
It uses a 50,000-commit limit and force-pushes main; it does not split the large
root commit or convert its oversized assets. Use the PowerShell publisher.

Git LFS must be installed and the local Git account must have push access.
An LFS allowance or authentication failure is reported by Git and stops the
upload; it must be resolved before resuming. The tool does not alter billing.
All original content is retained in this recovery. Later asset organization
and dependency cleanup require a separate Unreal reference review.

## Evidence and remaining REL-01 work

The previous missing-commit blocker is resolved locally. The failed monolithic
upload is being replaced by the prepared LFS history. This execution
environment can read but cannot write the active .git directory, and its
local Git process cannot resolve github.com. The connected GitHub app also
reports read-only access. Therefore this environment can prepare and verify
the replacement, but cannot apply it to the active Git metadata or publish it.
The tests exercise byte preservation, changed-work guards, batch boundaries,
resume decisions, and rejection of corrupt LFS objects. The complete local
upload/install/clone integration test is skipped when the Windows sandbox
blocks Git's local transport; a skip is not a passing integration result.

Retain plan.json, verification.json, publish-progress.json, published.json,
and installed.json under the prepared directory as they become available.
Only the last two establish successful remote publication and local installation.
Then verify a fresh clone with Git LFS, reproduce the native build, and record
that exact source revision in the checkpoint. REL-01 remains incomplete until
those checks have evidence.

For another initial preparation, use:

    node Tools/Release/chunked-source-upload.js prepare
    node Tools/Release/chunked-source-upload.js verify
    node --test Tools/Release/chunked-source-upload.test.js

The older Repair-SourceControl.ps1 was for the missing-commit condition only.
It refuses to replace an existing valid history and is not the upload repair.

References:

- [GitHub push-size troubleshooting](https://docs.github.com/en/get-started/using-git/troubleshooting-the-2-gb-push-limit)
- [GitHub large-file limits](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github)
- [Git LFS specification](https://github.com/git-lfs/git-lfs/blob/main/docs/spec.md)
