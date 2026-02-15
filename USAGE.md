# Ark Usage Guide

This guide covers how to use Ark for version control.

## Building

### Requirements

- C++17 compiler (clang, gcc, or MSVC)
- CMake >= 3.16
- OpenSSL (for SHA-256)
- zlib (for compression)

### macOS (Homebrew)

```bash
brew install cmake openssl@3 zlib

cmake -S . -B build \
      -DCMAKE_PREFIX_PATH="$(brew --prefix openssl@3);$(brew --prefix zlib)"
cmake --build build -j4
```

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install cmake libssl-dev zlib1g-dev

cmake -S . -B build
cmake --build build -j4
```

### Windows

```powershell
# Using vcpkg
vcpkg install openssl zlib

cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

---

## Quick Start

### 1. Initialize Repository

```bash
./build/ark init
```

Creates `.ark/` directory with:
```
.ark/
├── objects/
├── refs/heads/
├── HEAD
├── index
├── config
└── ark.log
```

### 2. Configure User (Required for Commits)

```bash
# Create config file
echo -e "[user]\nname = Your Name\nemail = you@example.com" > .ark/config
```

Or manually:
```bash
cat > .ark/config <<EOF
[user]
name = Your Name
email = you@example.com
EOF
```

### 3. Add Files

```bash
# Add single file
./build/ark add filename.txt

# Add multiple files
./build/ark add file1.txt file2.txt

# Add directory (recursively)
./build/ark add src/

# Add all files in current directory
./build/ark add .
```

### 4. Create Commit

```bash
# Opens editor for commit message
./build/ark commit

# Or use environment variable
export GIT_EDITOR="echo 'My commit message'"
./build/ark commit
```

### 5. View History

```bash
./build/ark log
```

---

## Commands Reference

### Repository Commands

#### init
Initialize a new repository.

```bash
./ark init
```

#### status
Show working tree status.

```bash
./ark status
```

Output shows:
- Changes staged for commit (green)
- Changes not staged (red, modified)
- Untracked files (red)

---

### Staging Commands

#### add
Stage files for commit.

```bash
./ark add <file> [files...]
./ark add .          # Stage all files
./ark add <dir>/    # Stage directory contents
```

Files matching `.arkignore` are not staged.

---

### Commit Commands

#### commit
Create a new commit from staged changes.

```bash
./ark commit
```

Requires:
- Staged files (`ark add`)
- User configuration (name and email)
- Non-empty commit message

#### commit-tree
Low-level commit creation (advanced).

```bash
./ark commit-tree <tree-hash> [-p <parent>] [-m <message>]
```

---

### Branching Commands

#### branch

```bash
# List branches
./ark branch

# Create branch (points to current commit)
./ark branch <branch-name>

# Delete branch
./ark branch -d <branch-name>
```

#### switch

```bash
# Switch to branch
./ark switch <branch-name>

# Create and switch
./ark switch -c <branch-name>
```

When switching:
- Working directory updated to match branch
- Index updated with branch's files
- HEAD points to new branch

---

### Object Commands

#### hash-object
Compute object ID without storing.

```bash
./ark hash-object <filename>
```

Returns 64-character SHA-256 hash.

#### cat-file
Display object content.

```bash
./ark cat-file <object-hash>
```

#### write-tree
Create tree object from index.

```bash
./ark write-tree
```

Returns tree hash.

---

### Reference Commands

#### update-ref
Update branch reference.

```bash
./ark update-ref refs/heads/<branch-name> <commit-hash>
```

---

## Working with Branches

### Creating a Feature Branch

```bash
# Start from main
./ark branch feature

# Or create and switch
./ark switch -c feature

# Make changes
echo "feature work" > feature.txt
./ark add feature.txt
./ark commit

# Switch back to main
./ark switch main
```

### Merging (Coming Soon)

Not yet implemented.

---

## Ignoring Files

Create `.arkignore` in repository root:

```bash
# Ignore specific files
secret.txt

# Ignore patterns (exact match)
*.log
*.tmp

# Ignore directories
build/
node_modules/
```

Note: Currently supports exact path matching, not glob/regex.

---

## Configuration

### Config File Location

- Repository: `.ark/config`
- User home: `~/.arkconfig`
- System: `~/config/ark/config`

### Config Format

```ini
[user]
name = Your Name
email = you@example.com
```

### Reading Config

Config is automatically loaded when needed. Check with:

```bash
cat .ark/config
```

---

## Troubleshooting

### "Not inside an Ark repository"

Run `ark init` first, or navigate to repository root.

### "No commits found"

Make at least one commit before creating branches.

### "Branch already exists"

Choose a different branch name or delete existing.

### "Permission denied"

Check file permissions on `.ark/` directory.

---

## Testing

### Unit Tests

```bash
cd build
./test_ark
```

40 tests covering:
- Logger
- Config
- Repository
- Ref (branches)
- Index
- Objects

### End-to-End Tests

```bash
./tests/test_e2e.sh
```

25 workflow tests covering all commands.

---

## Log File

Ark logs to `.ark/ark.log`:

```bash
tail -f .ark/ark.log
```

Log levels:
- DEBUG
- INFO
- WARNING
- ERROR
- FATAL

---

## Examples

### Complete Workflow

```bash
# Initialize
./ark init

# Configure
echo "[user]
name = Dev
email = dev@example.com" > .ark/config

# Create project
echo "# My Project" > README.md
mkdir src
echo "void main() {}" > src/main.cpp

# Stage and commit
./ark add .
./ark commit  # Enter message: "Initial commit"

# Make changes
echo "int x = 1;" >> src/main.cpp
./ark add src/main.cpp
./ark commit  # Enter message: "Add variable"

# View history
./ark log

# Create feature branch
./ark branch develop
./ark switch develop

# Work on feature
echo "feature" > feature.txt
./ark add feature.txt
./ark commit  # Enter message: "Add feature"

# Switch back
./ark switch main
```

### Single File Workflow

```bash
echo "hello" > file.txt
./ark add file.txt
./ark commit  # "Add file"
echo "world" >> file.txt
./ark add file.txt
./ark commit  # "Update file"
./ark log
```
