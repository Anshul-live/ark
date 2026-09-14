# Graph Report - ark  (2026-09-15)

## Corpus Check
- Corpus is ~11,603 words - fits in a single context window. You may not need a graph.

## Summary
- 606 nodes · 1000 edges · 36 communities (26 shown, 2 thin omitted)
- Extraction: 81% EXTRACTED · 18% INFERRED · 0% AMBIGUOUS · INFERRED: 185 edges (avg confidence: 0.83)
- Token cost: 65,063 input · 0 output

## Community Hubs (Navigation)
- Object Model & Tree Diffing
- Architecture & Usage Docs
- Index API & Tests
- Repository Core
- Logger
- Config API & Tests
- End-to-End Tests
- Ref Implementation
- Index Implementation
- Commit Creation Commands
- Repository Unit Tests
- Branch & Object Exceptions
- Ref API & Branch Commands
- Config Implementation
- Header Include Hub
- Ref Unit Tests
- Logger Unit Tests
- Object Unit Tests
- Config Exceptions
- Base & Commit Exceptions
- Filesystem Exceptions
- Log Command
- Blob Working-Tree I/O
- Index Exceptions
- Repository Exceptions
- update-ref Command
- cat-file Command
- hash-object Command

## God Nodes (most connected - your core abstractions)
1. `Logger` - 24 edges
2. `Repository` - 23 edges
3. `Config` - 19 edges
4. `TEST_F()` - 19 edges
5. `Index` - 18 edges
6. `Object` - 17 edges
7. `Ref` - 17 edges
8. `TEST_F()` - 17 edges
9. `Tree` - 16 edges
10. `cmd_switch()` - 16 edges

## Surprising Connections (you probably didn't know these)
- `cmd_switch()` --calls--> `setHeadToBranch`  [INFERRED]
  src/commands/switch.cpp → include/ref.h
- `End-to-End Tests (test_e2e.sh)` --conceptually_related_to--> `test_ark Test Executable Target`  [AMBIGUOUS]
  ARCHITECTURE.md → CMakeLists.txt
- `TEST_F()` --calls--> `setUserName`  [INFERRED]
  tests/test_config.cpp → include/config.h
- `TEST_F()` --calls--> `setUserEmail`  [INFERRED]
  tests/test_config.cpp → include/config.h
- `logBranch()` --calls--> `split`  [INFERRED]
  src/commands/log.cpp → include/config.h

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Git-style Object Model (Blob/Tree/Commit in content-addressed store)** — architecture_blob, architecture_tree, architecture_commit, architecture_object_model, architecture_content_addressable_storage [EXTRACTED 1.00]
- **Stage-and-Commit Workflow** — usage_add, usage_commit, architecture_index, architecture_tree, architecture_commit, architecture_reference_system [INFERRED 0.85]
- **Ark Build Toolchain and Dependencies** — cmakelists_ark, cmakelists_test_ark, cmakelists_openssl, cmakelists_zlib, cmakelists_googletest, cmakelists_cxx17_standard [EXTRACTED 1.00]

## Communities (36 total, 2 thin omitted)

### Community 0 - "Object Model & Tree Diffing"
Cohesion: 0.06
Nodes (62): unordered_map, Object, content, getSha256, hash, isWrittenToDisk, mode, name (+54 more)

### Community 1 - "Architecture & Usage Docs"
Cohesion: 0.05
Nodes (62): ark add Data Flow, Ark Architecture Document, ArkException, Blob Object, BranchException, BranchExistsException, Commit Object, ark commit Data Flow (+54 more)

### Community 2 - "Index API & Tests"
Cohesion: 0.05
Nodes (44): ClearIndex, ContainsFile, GetEntry, GetEntryNotFound, unordered_map, Index, clear, contains (+36 more)

### Community 3 - "Repository Core"
Cohesion: 0.08
Nodes (38): Blob, string, Repository, arkPath, cachedRoot, exists, findRoot, initialize (+30 more)

### Community 4 - "Logger"
Cohesion: 0.07
Nodes (35): exception, LogLevel, Logger, consoleOutput, debug, error, fatal, fileStream (+27 more)

### Community 5 - "Config API & Tests"
Cohesion: 0.07
Nodes (34): DefaultConfigIsEmpty, HasUserConfig, HasUserConfigPartial, Config, data, getUserEmail, getUserName, hasUserConfig (+26 more)

### Community 6 - "End-to-End Tests"
Cohesion: 0.10
Nodes (19): AddDot, AddSingleFile, CatFile, ConfigLoading, HashObject, IndexPersistence, InitRepository, MultipleFilesAdd (+11 more)

### Community 7 - "Ref Implementation"
Cohesion: 0.17
Nodes (22): readBranchRef, readHeadContent, writeBranchRef, headPath, refsDir, root, string, vector (+14 more)

### Community 8 - "Index Implementation"
Cohesion: 0.15
Nodes (19): string, IndexEntry, hash, mode, indexPath, pair, string, unordered_map (+11 more)

### Community 9 - "Commit Creation Commands"
Cohesion: 0.12
Nodes (20): Commit, create, loadFromDisk, parents, tree, string, vector, setHeadToCommit (+12 more)

### Community 10 - "Repository Unit Tests"
Cohesion: 0.11
Nodes (16): ConfigPath, GetModeForDirectory, GetModeForFile, HeadPath, isValid, IndexPath, IsValidRepository, ObjectsDir (+8 more)

### Community 11 - "Branch & Object Exceptions"
Cohesion: 0.16
Nodes (7): BranchException, BranchExistsException, BranchNotFoundException, string, InvalidObjectException, ObjectException, ObjectNotFoundException

### Community 12 - "Ref API & Branch Commands"
Cohesion: 0.14
Nodes (17): Ref, branchExists, createBranch, deleteBranch, getCurrentBranchName, getHeadBranch, getHeadCommit, isDetached (+9 more)

### Community 13 - "Config Implementation"
Cohesion: 0.20
Nodes (16): configPath, Config::getUserEmail(), Config::getUserName(), Config::load(), Config::loadFromFile(), Config::removeCharacters(), Config::removeExtraSpaces(), Config::replaceChar() (+8 more)

### Community 14 - "Header Include Hub"
Cohesion: 0.22
Nodes (3): string, unordered_map, vector

### Community 15 - "Ref Unit Tests"
Cohesion: 0.14
Nodes (12): BranchExists, GetCurrentBranchName, GetHeadCommit, listBranches, IsDetached, ListBranches, path, testing::Test (+4 more)

### Community 16 - "Logger Unit Tests"
Cohesion: 0.14
Nodes (12): ConsoleOutput, FatalLevel, InitializeLogger, LevelFiltering, LogLevels, MultipleLogCalls, SetLevel, path (+4 more)

### Community 17 - "Object Unit Tests"
Cohesion: 0.18
Nodes (9): BlobContentFormat, BlobHashIsSHA256, CreateBlobFromFile, DifferentContentDifferentHash, path, testing::Test, ObjectsTest, testFile (+1 more)

### Community 18 - "Config Exceptions"
Cohesion: 0.24
Nodes (3): ConfigException, ConfigNotFoundException, InvalidConfigException

### Community 19 - "Base & Commit Exceptions"
Cohesion: 0.20
Nodes (4): ArkException, CommitException, NoCommitsException, runtime_error

### Community 20 - "Filesystem Exceptions"
Cohesion: 0.22
Nodes (3): FileNotFoundException, FileSystemException, PermissionDeniedException

### Community 21 - "Log Command"
Cohesion: 0.32
Nodes (7): split, getBranchHash, cmd_log(), string, vector, logBranch(), Ref::branchExists()

### Community 22 - "Blob Working-Tree I/O"
Cohesion: 0.25
Nodes (7): Blob, createFile, deleteFile, fromFile, loadFromDisk, overwriteFile, writeToWorkingTree

### Community 25 - "update-ref Command"
Cohesion: 0.40
Nodes (4): updateRef, cmd_updateRef(), string, vector

### Community 26 - "cat-file Command"
Cohesion: 0.50
Nodes (3): cmd_catFile(), string, vector

### Community 27 - "hash-object Command"
Cohesion: 0.50
Nodes (3): cmd_hashObject(), string, vector

## Ambiguous Edges - Review These
- `End-to-End Tests (test_e2e.sh)` → `test_ark Test Executable Target`  [AMBIGUOUS]
  ARCHITECTURE.md · relation: conceptually_related_to

## Knowledge Gaps
- **76 isolated node(s):** `save`, `trim`, `toLower`, `removeCharacters`, `data` (+71 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 232 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `End-to-End Tests (test_e2e.sh)` and `test_ark Test Executable Target`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `Logger` connect `Logger` to `Logger Unit Tests`, `Repository Core`?**
  _High betweenness centrality (0.132) - this node is a cross-community bridge._
- **Why does `Repository` connect `Repository Core` to `Object Model & Tree Diffing`, `Index API & Tests`, `Config API & Tests`, `Ref Implementation`, `Index Implementation`, `Repository Unit Tests`, `Config Implementation`?**
  _High betweenness centrality (0.066) - this node is a cross-community bridge._
- **Why does `Commit` connect `Commit Creation Commands` to `Object Model & Tree Diffing`, `Config API & Tests`, `Header Include Hub`?**
  _High betweenness centrality (0.061) - this node is a cross-community bridge._
- **What connects `save`, `trim`, `toLower` to the rest of the system?**
  _76 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Object Model & Tree Diffing` be split into smaller, more focused modules?**
  _Cohesion score 0.05608322026232474 - nodes in this community are weakly interconnected._
- **Should `Architecture & Usage Docs` be split into smaller, more focused modules?**
  _Cohesion score 0.053939714436805924 - nodes in this community are weakly interconnected._