# Ark test suite

```bash
cmake -S . -B build && cmake --build build -j
ctest --test-dir build -j8 --output-on-failure      # each test in its own process
./build/test_ark --gtest_filter='Branching.*'        # or run the binary directly
```

With sanitizers (catches memory errors / UB inside Ark's own code, in-process and in every `ark` the tests spawn):

```bash
cmake -S . -B build-asan -DARK_SANITIZE=ON && cmake --build build-asan -j && ./build-asan/test_ark
```

## Layout

| Dir | What | How |
|---|---|---|
| `support/` | Sandbox, process runner, oracle | shared by everything |
| `unit/` | compress, core types, objects, object store, trees and diffs, index, config, refs, repository, logger | calls Ark's classes in-process inside a temp repo (a fixture owns a `Repository` and passes it to `Index`, `Config`, `RefStore`) |
| `cli/` | init, plumbing, add, commit, status, branch/switch, log | runs the real `ark` binary, checks exit code, stdout, stderr and the on-disk result |
| `property/` | random branch histories | generates trees, commits several branches, then switches between them in random order and checks every snapshot byte for byte |

## Why these tests can be trusted

- **Independent oracle.** Expected hashes come from `blobHash()`/`sha256Hex()` (OpenSSL EVP). Objects are read back with the test's own zlib inflate and parser (`readObject`, `parseTree`, `parseCommit`, `flattenTree`), and `readObject` checks that every object's content matches its id. Ark's code is never used to compute an expected value.
- **Full isolation.** Each test gets a fresh `mkdtemp` dir with its own `$HOME` (so your real `~/.arkconfig` is invisible), its own `$TMPDIR`, and a scripted `$GIT_EDITOR`. No shared fixed paths, so `ctest -j` is safe.
- **Real process boundaries.** `Sandbox::ark()` forks and execs the binary with stdin set to `/dev/null`, stdout and stderr captured separately, and a 30s timeout. A crash shows up as exit code ≥128 instead of looking like an ordinary failure.
- **Exact comparisons.** Tests compare whole maps and snapshots, not "the output contains `test.txt`".
- **Strict helpers.** For example, `isExecutable()` throws on a missing file instead of reading "unknown" permissions as executable. An earlier version of this suite passed a switch test that way while the file had actually been deleted.

## Randomized tests

```bash
ARK_SEEDS=200 ./build/test_ark --gtest_filter='*RandomHistory*'   # more seeds
ARK_SEED=387276917 ./build/test_ark --gtest_filter='*RandomHistory*' # replay a failure
ARK_KEEP_TMP=1 ...                                                  # keep the sandbox dirs
```

## Known-failing tests

These tests describe **correct** behavior and fail until the matching bug in `tmp/bugs/` is fixed. The suite has 184 tests; 142 pass and 42 fail. Fixing #01 and #02 turns most of the switch, status and random-history tests green.

| Bug | Tests |
|---|---|
| 01 header not stripped | `Branching.SwitchReproducesTheTargetSnapshotExactly`, `Branching.StatusIsCleanAfterSwitch`, `Branching.SwitchAppliesModeOnlyChanges`, `StatusTest.CleanRightAfterCommit`, `StatusTest.ShowsUnstagedModificationOfTrackedFile`, `StatusTest.StagedModificationIsReportedOnceAsStaged`, `ObjectsTest.LoadedTreeFlattensToFullPathsOfEveryBlob`, all `RandomHistory` |
| 02 flatten uses basenames | `Branching.IndexMatchesTargetCommitAfterSwitch`, `Branching.CommitAfterSwitchBuildsOnTheTargetTree` (+ most of the 01 list) |
| 03 switch discards work | `Branching.SwitchRefusesToOverwriteUncommittedChanges`, `Branching.SwitchNeverDropsStagedChanges` |
| 04 nondeterministic trees | `ObjectsTest.TreeHashDoesNotDependOnStagingOrder`, `PlumbingDeterminism.TreeHashDoesNotDependOnStagingOrder` |
| 05 spaces in paths | `IndexTest.PathsWithSpacesRoundTrip`, `Add.PathsWithSpacesAreStagedExactly` |
| 06 `.ark` gets staged | `Add.DotNeverStagesRepositoryMetadata`, `StatusMetadata.NeverListsRepositoryMetadata`, `RepositoryTest.WorkingDirectoryScanNeverIncludesArkMetadata` |
| 07 `.ark.log` in cwd | `Init.DoesNotCreateLogFilesInTheWorkingDirectory` |
| 08 ref name validation | `RefTest.RejectsBranchNamesThatEscapeRefs`, `Branching.RejectsBranchNamesThatEscapeRefs`, `Plumbing.UpdateRefRejectsPathsOutsideRefs`, `Plumbing.UpdateRefRejectsHashesThatAreNotCommits` |
| 09 add outside repo | `Add.RejectsPathsOutsideTheRepository` |
| 10 Blob read errors | `Add.UnreadableFileFailsCleanly`, `Plumbing.HashObjectOfMissingFileFails` |
| 11 commit-tree args | `Plumbing.CommitTreeRecordsTheGivenParent`, `Log.MergeHistoryListsEachCommitOnce` |
| 12 exit codes | `Commit.FailsWithoutIdentityAndLeavesHeadUnchanged` |
| 13 global config | `ConfigTest.ReadsGlobalConfigFromHome`, `ConfigTest.ReadsXdgConfig`, `ConfigTest.RepoConfigOverridesGlobal`, `Commit.UsesGlobalIdentityFromHome` |
| 14 deletions | `Add.StagesDeletionOfATrackedFile`, `Add.DotStagesDeletions`, `StatusTest.ShowsDeletedFiles` |
| 15 status from subdir | `StatusTest.SameResultFromASubdirectory` |
| 16 empty commits | `Commit.NothingToCommitIsRefused`, `Commit.NothingStagedInFreshRepoIsRefused` |
| 17 logs on stdout | `Logger.ConsoleOutputNeverGoesToStdout`, `Plumbing.WriteTreePrintsOnlyTheHash` |
| 18 type/mode changes | `Branching.SwitchAppliesModeOnlyChanges`, `Branching.SwitchHandlesFileReplacedByDirectory` |
| 20 dangling ref | `Log.DanglingRefFailsCleanly` |
| 21 log | `Log.WorksOnDetachedHead`, `Log.MergeHistoryListsEachCommitOnce` |
| 22 commit message | `Commit.WhitespaceOnlyMessageAborts`, `Commit.MessageThatLooksLikeAHeaderIsNotParsedAsOne` |
| 25 null parent | `Commit.RootCommitSnapshotsTheIndexAndHasNoParent` |

Every test that targets a bug has a `// tmp/bugs/NN` comment above it, so `grep -rn "tmp/bugs/05" tests/` finds them all.

## If you change a format

The suite parses on-disk formats in exactly two places, both in `support/ark_test.cpp`:
- `readIndex()`: `<mode> <hash> <path…>` per line
- `parseTree()` / `parseCommit()`: the object bodies

If you change the index or tree format while fixing #05, update those functions. No individual test should need to change.
