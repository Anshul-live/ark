# Ark test suite

This document explains how the tests are organised, how to run them, and which failing tests correspond to which open issue. The tests are written against correct behaviour, so a failing test means Ark has a bug, not that the test is out of date.

## Counts

At the time of writing the suite has 187 tests (`./test_ark --gtest_list_tests`, parameterised cases counted individually). 146 pass and 41 fail. Every failure is listed in the table below.

## Running

```bash
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -j8 --output-on-failure      # each test in its own process
./build/test_ark --gtest_filter='Branching.*'        # or run the binary directly
```

CMake fetches GoogleTest v1.14.0 at configure time. The build also compiles the `ark` binary; the CLI tests run it, and `ARK_BIN` is set to its path by `CMakeLists.txt`.

With sanitizers (AddressSanitizer and UBSan, applied to the test binary and to every `ark` process it spawns):

```bash
cmake -S . -B build-asan -DARK_SANITIZE=ON && cmake --build build-asan -j && ./build-asan/test_ark
```

Randomised tests:

```bash
ARK_SEEDS=200 ./build/test_ark --gtest_filter='*RandomHistory*'     # more seeds
ARK_SEED=387276917 ./build/test_ark --gtest_filter='*RandomHistory*' # replay one seed
ARK_KEEP_TMP=1 ./build/test_ark ...                                  # keep the sandbox directories
```

## Layers

| Directory | What it tests | How |
|---|---|---|
| `support/` | Shared helpers: `Sandbox`, process runner, and an independent oracle (SHA-256, inflate, tree/commit/index parsers) | Not tests; used by everything below |
| `unit/` | Compression, core types, objects, object store, trees and diffs, index, config, refs, repository, logger | Calls Ark's classes in-process inside a temporary repository |
| `cli/` | `init`, plumbing commands, `add`, `commit`, `status`, `branch`/`switch`, `log` | Runs the real `ark` binary and checks exit code, stdout, stderr and the files on disk |
| `property/` | Random branch histories | Generates random trees, commits on several branches, switches between them in random order and compares every snapshot byte for byte |

## Why the results can be trusted

- Expected hashes come from `blobHash()` and `sha256Hex()` in `support/ark_test.cpp`, which call OpenSSL directly. Objects are read back with the suite's own inflate and parsers, and `readObject` checks that each object's content matches its id. Ark's code is never used to compute an expected value.
- Each test gets a fresh `mkdtemp` directory with its own `$HOME`, `$TMPDIR` and a scripted `$GIT_EDITOR`, so tests do not share paths and `ctest -j` is safe.
- `Sandbox::ark()` forks and execs the binary with stdin on `/dev/null`, captures stdout and stderr separately, and enforces a 30 second timeout.
- Tests compare whole maps and snapshots, not substrings.

## Failing tests and the issues behind them

Issue numbers refer to the maintainer's local bug list (`tmp/bugs/`, which is not part of the repository). Each test that targets a bug has a `// tmp/bugs/NN` comment above it, so `grep -rn "tmp/bugs/13" tests/` finds them.

| Issue | Failing tests |
|---|---|
| 03 `switch` discards uncommitted work | `Branching.SwitchRefusesToOverwriteUncommittedChanges`, `Branching.SwitchNeverDropsStagedChanges` |
| 06 `add .` stages `.ark/` and `.ark.log` | `Add.DotNeverStagesRepositoryMetadata`, `StatusMetadata.NeverListsRepositoryMetadata`, `RepositoryTest.WorkingDirectoryScanNeverIncludesArkMetadata` |
| 07 `.ark.log` written to the working directory | `Init.DoesNotCreateLogFilesInTheWorkingDirectory`, `StatusTest.SameResultFromASubdirectory` (lists `d/.ark.log` as untracked) |
| 08 ref names not validated | `RefTest.RejectsBranchNamesThatEscapeRefs`, `Branching.RejectsBranchNamesThatEscapeRefs`, `Plumbing.UpdateRefRejectsPathsOutsideRefs`, `Plumbing.UpdateRefRejectsHashesThatAreNotCommits` |
| 09 `add` accepts paths outside the repo | `Add.RejectsPathsOutsideTheRepository` |
| 10 / 12 failures that exit 0 | `Plumbing.HashObjectOfMissingFileFails`, `Commit.FailsWithoutIdentityAndLeavesHeadUnchanged` |
| 11 `commit-tree` argument indexing | `Plumbing.CommitTreeRecordsTheGivenParent` |
| 13 global config never loaded | `ConfigTest.ReadsGlobalConfigFromHome`, `ConfigTest.ReadsXdgConfig`, `ConfigTest.RepoConfigOverridesGlobal`, `Commit.UsesGlobalIdentityFromHome` |
| 14 deletions cannot be staged | `Add.StagesDeletionOfATrackedFile`, `Add.DotStagesDeletions`, `StatusTest.ShowsDeletedFiles` |
| 16 empty commits allowed | `Commit.NothingToCommitIsRefused`, `Commit.NothingStagedInFreshRepoIsRefused` |
| 17 INFO logs on stdout | `Logger.ConsoleOutputNeverGoesToStdout`, `Plumbing.WriteTreePrintsOnlyTheHash` |
| 18 `switch` ignores mode and type changes | `Branching.SwitchAppliesModeOnlyChanges`, `Branching.SwitchHandlesFileReplacedByDirectory` |
| 21 `log` repeats commits, fails when detached | `Log.WorksOnDetachedHead`, `Log.MergeHistoryListsEachCommitOnce` |
| 22 commit message handling | `Commit.WhitespaceOnlyMessageAborts`, `Commit.MessageThatLooksLikeAHeaderIsNotParsedAsOne` |
| 25 root commit records a zero parent | `Commit.RootCommitSnapshotsTheIndexAndHasNoParent` |
| 14 and 18 (inferred) | `Seeds/RandomHistory.SwitchingReproducesEveryBranchExactly/0` to `/7` (all eight seeds) |

The random-history failures were checked by hand on two seeds only. Seed 0 fails because `switch` does not apply an executable-bit change (18). Seed 3 fails because a file deleted in the working tree is still in the commit (14). The other six were not inspected.

## If you change a format

The suite parses on-disk formats in exactly two places, both in `support/ark_test.cpp`: `readIndex()` for the index (`<mode> <hash> <path>` per line), and `parseTree()` / `parseCommit()` for object bodies. If you change a format, update those functions; no individual test should need to change.
