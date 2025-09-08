# Ark

A minimal Git-like version control system built in C++ to understand how Git works under the hood. Ark implements the core concepts of content-addressable storage, object model, and version control in a clean, educational codebase.

## Features

Ark currently supports:

* **Repository Management**: Initialize repositories with `.ark` directory structure
* **Object Model**: Blob, Tree, and Commit objects with Git-style headers
* **Content Addressing**: SHA-256 hashing with compressed storage using zlib
* **Staging Area**: Index-based staging system with file permissions
* **Branching**: Create, list, and switch between branches
* **Commits**: Create commits with parent relationships and merge support
* **History**: View commit history and repository status
* **Ignore System**: `.arkignore` file support for excluding files
* **Working Directory**: Checkout and update working directory from commits

---

## Build

Requirements:

* C++17 compiler (clang, gcc, or MSVC)
* CMake >= 3.16
* OpenSSL (for SHA-256)
* zlib (for compression)

### Linux / macOS

Install dependencies via your package manager, for example:

```bash
# macOS (Homebrew)
brew install cmake openssl@3 zlib

# Ubuntu/Debian
sudo apt update
sudo apt install cmake libssl-dev zlib1g-dev
```

Build using CMake:

```bash
cmake -S /path/to/ark -B /path/to/ark/build \
      -DCMAKE_PREFIX_PATH="$(brew --prefix openssl@3);$(brew --prefix zlib)"
cmake --build /path/to/ark/build -j4
```

Or compile directly with g++:

```bash
# macOS
g++ -std=c++17 -Iinclude -I$(brew --prefix openssl@3)/include \
    src/main.cpp src/commands/*.cpp src/utils/*.cpp \
    -L$(brew --prefix openssl@3)/lib -lssl -lcrypto -lz -o build/ark

# Linux
 g++ -std=c++17 -Iinclude src/main.cpp src/commands/*.cpp src/utils/*.cpp -lssl -lcrypto -lz -o build/ark
```

### Windows

Install dependencies via vcpkg or manually:

```powershell
# Example with vcpkg
vcpkg install openssl zlib
```

Build using CMake:

```powershell
cmake -S C:\path\to\ark -B C:\path\to\ark\build \
      -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build C:\path\to\ark\build --config Release
```

Or compile directly with g++:

```powershell
g++ -std=c++17 -Iinclude -I<openssl_include> -I<zlib_include> \
    src/main.cpp src/commands/*.cpp src/utils/*.cpp \
    -L<openssl_lib> -L<zlib_lib> -lssl -lcrypto -lz -o build\ark.exe
```

The binary will be at `build/ark` (or `build\Release\ark.exe` on Windows).

---

## Commands

### Repository Management
```bash
# Initialize a new repository
./build/ark init

# Show repository status
./build/ark status
```

### Object Operations
```bash
# Create blob from file and get hash
./build/ark hash-object <filename>

# Read object content by hash
./build/ark cat-file <object-hash>

# Create tree from staged files
./build/ark write-tree

# Create commit from tree
./build/ark commit-tree <tree-hash> [<parent1-hash>] [<parent2-hash>] [<message>]
```

### Staging and Commits
```bash
# Stage files for commit
./build/ark add <filename> [<filename>...]

# Create commit with editor
./build/ark commit
```

### Branching
```bash
# List all branches
./build/ark branch

# Create new branch
./build/ark branch <branch-name>

# Switch to branch
./build/ark switch <branch-name>

# Update branch reference
./build/ark update-ref <ref-path> <commit-hash>
```

### History
```bash
# Show commit history
./build/ark log
```

## Usage Examples

### Basic Workflow
```bash
# Initialize repository
./build/ark init

# Stage files
./build/ark add README.md src/main.cpp

# Create commit
./build/ark commit

# Create and switch to new branch
./build/ark branch feature-branch
./build/ark switch feature-branch

# View history
./build/ark log
```

### Object Inspection
```bash
# Get file hash
HASH=$(./build/ark hash-object README.md)
echo "File hash: $HASH"

# Read object content
./build/ark cat-file "$HASH"
```

---

## Architecture

### Object Model
Ark implements Git's object model with three core types:

- **Blob**: Represents file content with Git-style headers (`blob <size>\0<content>`)
- **Tree**: Directory structure containing references to blobs and subtrees
- **Commit**: Snapshot with tree reference, parent commits, and metadata

### Storage System
- **Content Addressing**: Objects identified by SHA-256 hash
- **Compression**: All objects compressed using zlib
- **Directory Structure**: Objects stored as `objects/<first-2-chars>/<remaining-hash>`
- **Index**: Text-based staging area with `mode hash path` entries

### Repository Structure
```
.ark/
├── objects/          # Content-addressable object storage
├── refs/
│   └── heads/        # Branch references
├── HEAD              # Current branch or commit
├── index             # Staging area
└── config            # Repository configuration
```

### File Permissions
Supports Git-style file modes:
- `100644`: Regular file
- `100755`: Executable file  
- `040000`: Directory
- `120000`: Symbolic link

---

## Project Structure

```
ark/
├── src/
│   ├── main.cpp                    # Entry point and CLI interface
│   ├── commands/                   # Command implementations
│   │   ├── init.cpp               # Repository initialization
│   │   ├── hash-object.cpp        # Blob creation
│   │   ├── cat-file.cpp           # Object content retrieval
│   │   ├── add.cpp                # Staging files
│   │   ├── commit.cpp             # Commit creation
│   │   ├── write-tree.cpp         # Tree creation from index
│   │   ├── commit-tree.cpp        # Commit from tree
│   │   ├── branch.cpp             # Branch management
│   │   ├── switch.cpp             # Branch switching
│   │   ├── log.cpp                # Commit history
│   │   ├── status.cpp             # Repository status
│   │   └── update-ref.cpp         # Reference updates
│   └── utils/                     # Core utilities
│       ├── ark.cpp                # Helper functions
│       ├── compress.cpp           # zlib compression/decompression
│       ├── objects.cpp            # Object model implementation
│       └── head.cpp               # HEAD management
├── include/                       # Header files
├── build/                         # Build artifacts
├── CMakeLists.txt                 # Build configuration
```

## Development Status

### ✅ Implemented Features
- Complete object model (Blob, Tree, Commit)
- Repository initialization and management
- Content-addressable storage with compression
- Staging area and index management
- Branch creation and switching
- Commit history and logging
- Working directory synchronization
- File permission handling
- Ignore file support(its not like git, here its exact file and directory matching, no regex support)

### 🚧 In Progress / TODO
- Configuration system completion
- Improved commit message parsing
- Enhanced error handling
- Performance optimizations
- Additional Git compatibility features
---

## License

MIT
