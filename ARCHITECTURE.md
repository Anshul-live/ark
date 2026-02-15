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

### Blob

A Blob represents the contents of a file. When you add a file, Ark:

1. Reads the file content
2. Creates a blob with Git-style header: `blob <size>\0<content>`
3. Computes SHA-256 hash of the content
4. Compresses using zlib
5. Stores at `.ark/objects/<first-2-chars>/<remaining-hash>`

```
Format: blob <size>\0<content>
Example: blob 12\0Hello World
```

### Tree

A Tree represents a directory snapshot. It contains:
- File mode (100644, 100755, 120000)
- Object hash (blob or subtree)
- Filename

```
Format: <mode> <hash> <filename>
Example: 100644 abc123...  file.txt
         040000 def456...  subdir/
```

### Commit

A Commit represents a snapshot of the entire repository. It contains:
- Tree hash - root directory structure
- Parent commit hash(es) - for history and merging
- Author - name and email from config
- Committer - same as author
- Timestamp
- Commit message

```
Format:
tree <tree-hash>
parent <parent-hash>
author <name> <email> <timestamp>
committer <name> <email> <timestamp>

<commit-message>
```

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

## Core Classes

### Repository (`repository.h`)

Manages repository paths and utilities:

```cpp
class Repository {
    std::string root();           // Repository root path
    std::string objectsDir();    // .ark/objects
    std::string indexPath();      // .ark/index
    std::string configPath();     // .ark/config
    std::string headPath();      // .ark/HEAD
    std::string refsDir();       // .ark/refs
    
    bool isValid();              // Check if .ark exists
    std::string getMode();       // Get file mode (100644, etc.)
    bool isIgnored();            // Check .arkignore
};
```

### Ref (`ref.h`)

Manages HEAD and branches:

```cpp
class Ref {
    std::string getHeadCommit();      // Get current commit hash
    std::string getCurrentBranchName(); // Get branch name
    bool isDetached();                // Is HEAD detached?
    
    void setHeadToCommit();           // Move HEAD to commit
    void setHeadToBranch();           // Switch to branch
    
    bool branchExists();               // Check if branch exists
    void createBranch();               // Create new branch
    void deleteBranch();               // Delete branch
    std::vector<std::string> listBranches();
};
```

### Index (`index.h`)

Manages staging area:

```cpp
class Index {
    void load();           // Read .ark/index
    void save();           // Write .ark/index
    
    void stage(path, hash, mode);  // Add to staging
    void unstage(path);           // Remove from staging
    
    bool contains(path);   // Is file staged?
    get(path);            // Get index entry
};
```

### Config (`config.h`)

Manages user configuration:

```cpp
class Config {
    void load();          // Load from .ark/config
    void save();          // Save to .ark/config
    
    getUserName();        // Get configured name
    getUserEmail();       // Get configured email
    hasUserConfig();      // Check if configured
};
```

### Logger (`logger.h`)

Provides logging functionality:

```cpp
class Logger {
    void init(path);           // Initialize with log file
    void setLevel(LEVEL);      // Set min log level
    void setConsoleOutput(bool); // Enable/disable console
    
    void debug(msg);
    void info(msg);
    void warning(msg);
    void error(msg);
    void fatal(msg);
};
```

---

## Storage Format

### .ark/objects/

Objects stored as: `objects/<prefix>/<hash>`

- First 2 chars of hash as directory name
- Remaining 62 chars as filename
- Content compressed with zlib
- Format: `<type> <size>\0<data>`

### .ark/index

Text format: `<mode> <hash> <filename>`

```
100644 abc123...  file.txt
100755 def456...  script.sh
040000 ghi789...  subdir/
```

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
│   └── InvalidObjectException
├── IndexException
│   └── IndexCorruptedException
├── BranchException
│   ├── BranchNotFoundException
│   └── BranchExistsException
├── ConfigException
├── FileSystemException
│   ├── FileNotFoundException
│   └── PermissionDeniedException
```

---

## Testing

### Unit Tests (Google Test)

```bash
cd build
./test_ark
```

- 40 unit tests covering all core classes
- Tests for Logger, Config, Repository, Ref, Index, Objects

### End-to-End Tests (Shell)

```bash
./tests/test_e2e.sh
```

- 25 comprehensive workflow tests
- Tests init, add, commit, branch, switch, log, status

---

## Project Structure

```
ark/
├── src/
│   ├── main.cpp              # Entry point
│   ├── commands/            # CLI commands
│   │   ├── init.cpp
│   │   ├── add.cpp
│   │   ├── commit.cpp
│   │   ├── branch.cpp
│   │   ├── switch.cpp
│   │   └── ...
│   └── utils/              # Core utilities
│       ├── objects.cpp     # Blob, Tree, Commit
│       ├── repository.cpp  # Repository class
│       ├── ref.cpp         # Ref class
│       ├── index.cpp       # Index class
│       ├── config.cpp      # Config class
│       ├── logger.cpp      # Logger class
│       └── compress.cpp    # zlib compression
├── include/                # Headers
├── tests/                 # Test files
│   ├── test_*.cpp         # Unit tests
│   └── test_e2e.sh        # E2E tests
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
