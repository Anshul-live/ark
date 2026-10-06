# Ark Architecture

This document explains how Ark works internally - the design decisions, object model, and data flow.

## Overview

Ark is a Git-like version control system implemented in C++. It demonstrates how version control systems work by implementing core concepts:

1. **Content-addressable storage** - Objects are stored and retrieved by their SHA-256 hash
2. **Object model** - Blob, Tree, and Commit objects
3. **Staging area** - Index file tracking changes to be committed
4. **Reference system** - HEAD and branches pointing to commits

---

## Object Model

Every object is stored zlib-compressed at `.ark/objects/<first 2 hex chars>/<remaining 62>`. Its id is the SHA-256 of the uncompressed frame:

```
<type> <body size in bytes>\0<body>
```

`type` is `blob`, `tree` or `commit`.

### Blob

The body is the raw file content. Example frame: `blob 11\0Hello World`.

### Tree

The body has one line per entry, sorted by name. Each line is `<mode> <type> <hash> <name>`, where `type` is `blob` or `tree`:

```
100644 blob 3f2a...e1 file.txt
040000 tree 9c0b...77 subdir
```

### Commit

The body is written by `Commit::serialize`, in this order:

```
tree <tree-hash>
parent <parent-hash>        (one line per parent, may be absent)
committer <name> <<email>> <unix-time> <utc-offset>
<message>
```

There is no author line. A root commit written by `ark commit` currently records a zero hash as its parent (known bug).

---

## Data Flow

### Adding Files (`ark add`)

```
User runs: ark add file.txt
           │
           ▼
┌─────────────────────────────┐
│ 1. Read file content       │
│ 2. Create Blob object     │
│ 3. Compute SHA-256 hash   │
│ 4. Compress with zlib     │
│ 5. Store in objects/      │
│ 6. Add to Index          │
└─────────────────────────────┘
           │
           ▼
Index (.ark/index) contains:
100644 abc123...  file.txt
```

### Creating Commits (`ark commit`)

```
User runs: ark commit
           │
           ▼
┌─────────────────────────────┐
│ 1. Read Index entries     │
│ 2. Build Tree from Index  │
│ 3. Create Commit object   │
│ 4. Store Commit object   │
│ 5. Update branch ref     │
└─────────────────────────────┘
           │
           ▼
Branch (.ark/refs/heads/main) points to new commit
```

### Switching Branches (`ark switch`)

```
User runs: ark switch feature
           │
           ▼
┌─────────────────────────────┐
│ 1. Read target commit     │
│ 2. Compare trees (diff)  │
│ 3. Update working dir   │
│ 4. Update Index         │
│ 5. Update HEAD          │
└─────────────────────────────┘
```

---

## Classes

Public headers live in `include/ark/<module>/`, sources in `src/<module>/`, and one file per command in `src/cli/commands/`.

| Module | Classes and functions |
|---|---|
| `core` | `ObjectHash`, `ObjectType`, `FileMode`, `RepoPath`, `RefName` / `BranchName`, exception hierarchy |
| `storage` | `ObjectStore` (read, write, `hashOf`), `frameObject` / `parseObjectFrame`, zlib `compressObject` / `decompressObject`, `sha256` |
| `objects` | `Blob`, `Tree`, `Commit`, `CommitStore`, `CommitDAG` (`historyFrom`), `FlatTree` and `flattenTree`, `TreeBuilder` / `writeTreeFromIndex`, `diffTrees` |
| `repo` | `Repository`, `Index`, `IndexFile`, `Config`, `RefStore`, `IgnoreRules`, `WorkingTree`, `Checkout`, `computeStatus` / `printStatus` |
| `cli` | `cmd_*` command functions, `CommitMessageEditor` |
| `util` | `Logger`, string helpers, filesystem helpers, time helpers |

### Dependency injection

`main` builds one `Repository` with `Repository::discover()` and passes it to every command as `int cmd_x(const Repository& repo, const std::vector<std::string>& args)`; only `cmd_init` takes no repository. `Index`, `Config` and `RefStore` take the `Repository` in their constructor and keep a reference to it. `Repository` resolves its root lazily from the current directory, so a missing repository is reported (`Not inside an Ark repository`, exit 1) when a command first needs a path.

### Repository

Paths and root discovery: `root()`, `arkPath()`, `objectsDir()`, `indexPath()`, `configPath()`, `headPath()`, `refsDir()`, `isValid()`.

### RefStore

HEAD and branches: `readHead`, `resolveHead`, `currentBranch`, `isHeadDetached`, `moveHeadTo`, `attachHeadTo`, `readRef`, `writeRef`, `branchTarget`, `branchExists`, `createBranch`, `deleteBranch`, `listBranches`.

### Index

The staging area, held as a `FlatTree` (path to `FileEntry{hash, mode}`): `load`, `save`, `stage`, `stageAll`, `replaceAll`, `erase`, `contains`, `find`, `entries`, `size`, `clear`.

### CommitDAG

`historyFrom(start)` walks parents breadth-first and returns the hashes in the order `ark log` prints them. There is no visited set, so a commit reachable along two paths appears twice (known bug).

### Config

`load`, `save`, `getUserName`, `getUserEmail`, `setUserName`, `setUserEmail`, `hasUserConfig`.

### Logger

Singleton: `init(path)`, `setLevel`, `setConsoleOutput`, and `debug` / `info` / `warning` / `error` / `fatal`, usually through the `LOG_*` macros.

---

## Storage Format

### .ark/objects/

Objects are stored as described under Object Model.

### .ark/index

Written by `IndexFile::write`, one line per path, sorted by path: `<mode> <hash> <path>`.

```
100644 abc123...  file.txt
100755 def456...  script.sh
```

Directories are not stored; only files and symlinks. The reader splits on whitespace, so paths with spaces do not round-trip (known bug).

### .ark/HEAD

Points to current branch or commit:

```
ref: refs/heads/main        # On a branch
abc123...                   # Detached HEAD
```

### .ark/refs/heads/<branch>

Contains commit hash (64 hex chars):

```
abc123def456789012345678901234567890123456789012345678901234
```

### .ark/config

INI-style format:

```ini
[user]
name = Your Name
email = you@example.com
```

---

## File Modes

Ark supports Git-style file modes:

| Mode | Meaning |
|------|---------|
| 100644 | Regular file |
| 100755 | Executable file |
| 040000 | Directory |
| 120000 | Symbolic link |

---

## Error Handling

Ark uses custom exception classes for different error types:

```
ArkException (base)
├── RepositoryException
│   └── NotARepositoryException
├── ObjectException
│   ├── ObjectNotFoundException
│   ├── ObjectFormatException
│   └── InvalidObjectException
├── IndexException
│   └── IndexCorruptedException
├── BranchException
│   ├── BranchNotFoundException
│   └── BranchExistsException
├── ConfigException
└── FileSystemException
```

---

## Testing

```bash
cd build
./test_ark
```

The suite has 184 tests in 30 suites: unit tests that call the classes in-process inside a temporary repository, CLI tests that run the real `ark` binary, and randomized branch-history tests. 142 pass and 42 fail; the failures are tests that describe correct behaviour for bugs that are still open. See `tests/README.md` for the layout and the list of known failures.

---

## Project Structure

```
ark/
├── include/ark/
│   ├── core/       # value types and exceptions
│   ├── storage/    # object store, framing, compression, sha256
│   ├── objects/    # blob, tree, commit, commit store, commit DAG, tree diff
│   ├── repo/       # repository, index, config, refs, working tree, checkout, status
│   ├── cli/        # command declarations, commit message editor
│   └── util/       # logger, strings, fs, time
├── src/
│   ├── main.cpp    # entry point and command table
│   ├── <module>/   # implementations matching include/ark/<module>
│   └── cli/commands/   # one file per command (merge.cpp is not built yet)
├── tests/
│   ├── support/    # sandbox, process runner, independent oracle
│   ├── unit/
│   ├── cli/
│   └── property/
└── CMakeLists.txt
```

---

## Key Design Decisions

1. **SHA-256 for content-addressing** - Ensures unique identification and deduplication
2. **zlib compression** - Reduces storage space
3. **Object format compatibility** - Similar to Git for interoperability
4. **Text-based index** - Human-readable for debugging
5. **Exception-based error handling** - Clean error propagation
6. **Logger for debugging** - Easy to trace issues
