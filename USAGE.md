# Ark usage

This guide covers building Ark, the commands it has, the config and ignore files, and the error messages you can run into. Everything here was checked against `src/main.cpp` and `src/cli/commands/*.cpp`. Where a command misbehaves, the text says so; the full list is in the [README](README.md#known-issues).

## Building

Requirements: a C++17 compiler, CMake 3.16 or newer, OpenSSL, zlib. CMake downloads GoogleTest at configure time, so the first configure needs network access.

macOS (Homebrew):

```bash
brew install cmake openssl@3 zlib
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix openssl@3)"
cmake --build build -j4
```

Ubuntu/Debian:

```bash
sudo apt install cmake libssl-dev zlib1g-dev
cmake -S . -B build
cmake --build build -j4
```

The binary is `build/ark`. Windows has not been tried.

## Quick start

The examples assume `ark` is on your `PATH` (for example `export PATH="$PWD/build:$PATH"`).

```bash
mkdir demo && cd demo
ark init
printf '[user]\nname = Ada\nemail = ada@example.com\n' > .ark/config

echo "hello" > hello.txt
ark add hello.txt
ark commit
ark log
```

`ark init` prints `ark initialised` (or `ark already initialised` if `.ark/` exists). The user name and email in `.ark/config` are required for `commit`.

`ark commit` opens an editor on a temporary file. It uses `$GIT_EDITOR`, then `$VISUAL`, then `$EDITOR`, then `vi`. To commit without an interactive editor, point `GIT_EDITOR` at a script that writes the message into the file it receives:

```bash
cat > /tmp/ark-msg.sh <<'SH'
#!/bin/sh
echo "my message" > "$1"
SH
chmod +x /tmp/ark-msg.sh
GIT_EDITOR=/tmp/ark-msg.sh ark commit
```

A value such as `GIT_EDITOR="echo hi"` does not work: the file name is appended to the command, and `echo` does not write to it, so the message stays empty and the commit is aborted.

## Commands

All commands except `init` must run inside a repository (any subdirectory of a directory containing `.ark/`). With no command, Ark prints `Usage: <program> <command> [args...]` and exits 1. An unknown command prints `Unknown command: <name>` and exits 1.

### init

```bash
ark init
```

Creates `.ark/` in the current directory with `objects/`, `refs/heads/`, `HEAD` (pointing at `refs/heads/main`), an empty `index` and an empty `config`. The branch `main` exists but points at a zero hash until the first commit.

### add

```bash
ark add <path> [<path> ...]
```

Stages files. A directory is walked recursively. Each file's content is stored as a blob and its path, hash and mode are written to `.ark/index`. Files matching `.arkignore` are skipped. With no arguments it prints `Usage:  add <filename>/<dirname> ...` and exits 1.

A path that does not exist prints `<absolute path> does not exist.` to stderr, but the command still exits 0.

Avoid `ark add .`: it also stages `.ark/` and `.ark.log`. Name your files or directories instead.

### status

```bash
ark status
```

Prints three headings, each followed by paths (staged in green, modified and untracked in red):

```
Changes to be committed:
a.txt

Changes not staged for commit:
b.txt (modified)

Untracked files:
c.txt (untracked)
```

Deleted files are not reported, and `.ark/` and `.ark.log` currently show up as untracked.

### commit

```bash
ark commit
```

Writes a tree from the index, then a commit whose parent is the current HEAD commit, and moves the current branch to it. It prints `passed checks` and some log lines. It does not print the new hash; use `ark log`.

There is no `-m` option. Arguments are ignored. If the message is empty, it prints `commit aborted due to empty message` and exits 1. If `user.name` or `user.email` is missing, it prints `please provide the following values user.name and user.email before committing.` and exits with status 0. A commit is made even if nothing is staged or changed.

### log

```bash
ark log
```

Prints one line per commit, newest first, following parents breadth first:

```
commit: 8993d780493d733f026e57f720e7a4d6ffab79037f2a21cbdcb596979e13a1c1
commit: f57415b3d2e5abc8a0083d15946f4acad9f354e377cffdee7c5872f820b7c1a5
```

Only the hash is shown, not the author, date or message. On a detached HEAD it prints `HEAD is detached` and exits 1.

### branch

```bash
ark branch              # list branches
ark branch <name>       # create a branch at the current commit
```

The list marks the current branch with `* ` (name in green) and indents the others by two spaces. Order is the order the filesystem returns, not sorted. There is no delete option.

Creating a branch before the first commit prints `please make a commit first` to stderr and exits 1. An existing name prints `branch already exists` and exits 1.

### switch

```bash
ark switch <branch>
```

Moves HEAD to the branch, rewrites the working tree to match the branch's commit, and replaces the index with that commit's files. For each changed file it prints `creating file ...`, `overwriting file ...` or `deleting file ...`, then `switched branch to <branch>`. There is no `-c` option: create the branch first with `ark branch`.

| Situation | Output | Exit |
|---|---|---|
| No argument | `Usage: ark switch <branch-name>` (stdout) | 1 |
| Branch missing | `branch <name> does not exist` (stderr) | 1 |
| Already on it | `already on branch <name>` (stderr) | 0 |

`switch` does not check for uncommitted work and can overwrite it (see the README, Known issues). Commit your changes before switching.

### Plumbing commands

Low-level commands that work on single objects.

| Command | What it does | Failure |
|---|---|---|
| `ark hash-object <file>` | Prints the blob hash of a file. Stores nothing. | No argument: `Usage: hash-object <filename>`, exit 1. Unreadable file: `Error: cannot open file <file>`, exit 0 |
| `ark cat-file <hash>` | Prints an object's body (without the header) followed by a newline. | `Usage: cat-file <object-hash>`; `Error: invalid object hash` for hashes shorter than 3 characters; `Error: object not found for hash <hash>`; all exit 1 |
| `ark write-tree` | Writes tree objects from the index and prints the root tree hash. | |
| `ark commit-tree <tree> [<parent1> [<parent2>]]` | Writes a commit for a tree and prints its hash. | Broken: it reads parents and message from the wrong argument positions. Avoid it. |
| `ark update-ref <ref-path> <hash>` | Writes the hash into `.ark/<ref-path>`, for example `refs/heads/main`. | No validation of the path or the hash. Missing arguments: `Usage: update-ref <ref-path> <commit-hash>`, exit 1 |

Because of the stdout log lines, `write-tree` and `hash-object` output can have `[INFO]` lines mixed in.

## Not available

`merge` is not available yet. The source file `src/cli/commands/merge.cpp` is not compiled or registered, so `ark merge` prints `Unknown command: merge`. Ark also has no diff, reset, tag, stash, remote or clone commands.

## Working with branches

```bash
ark branch feature
ark switch feature

echo "feature work" > feature.txt
ark add feature.txt
ark commit

ark switch main        # feature.txt is deleted from the working tree
```

`ark switch main` prints `deleting file <path>` for `feature.txt` because it is not in `main`'s commit.

## Ignoring files

Put an `.arkignore` file in the repository root, or in any subdirectory (its patterns are relative to its own directory). One pattern per line. Empty lines and lines starting with `#` are skipped.

```
secret.txt
build/
docs/notes.txt
```

Matching is by exact path, not glob: `*.log` matches a file literally named `*.log`. A pattern ending in `/` ignores that directory and everything under it. Ignored files are skipped by `add` and by `status`.

## Configuration

Only `.ark/config` is used. Format:

```ini
[user]
name = Your Name
email = you@example.com
```

Lines beginning with `;` are comments. `Config::load` also tries `~/.arkconfig` and `~/config/ark/config`, but it opens those names literally without expanding `~`, so they are never found (known issue). Set the identity in each repository's `.ark/config`.

## Log file

Ark writes a log to `.ark.log` in the directory where you ran the command, not inside `.ark/`. Levels are DEBUG, INFO, WARNING, ERROR and FATAL. INFO and above are also printed to stdout, prefixed with a timestamp, for example `[2026-10-24 00:57:44.245] [INFO] Index loaded: 2 entries`. Both behaviours are known issues.

## Error messages

| Message | Meaning |
|---|---|
| `Not inside an Ark repository` | No `.ark/` in the current directory or any parent. Run `ark init`, or `cd` into the repository. |
| `Unknown command: <name>` | Not one of the commands listed above. |
| `please make a commit first` | `ark branch <name>` before the first commit. |
| `branch already exists` | `ark branch <name>` with a name in use. |
| `branch <name> does not exist` | `ark switch` to a missing branch. |
| `commit aborted due to empty message` | The editor left the file empty. Check that `GIT_EDITOR` writes to its first argument. |
| `please provide the following values user.name and user.email before committing.` | Add a `[user]` section to `.ark/config`. |
| `Error: object not found for hash <hash>` | `cat-file` hash is wrong or the object is missing. |
| `Index file is corrupted` | `.ark/index` has a line that is not `<mode> <hash> <path>`. |
| `Editor exited with code <n>` | The editor command returned a non-zero status. |

## Tests

```bash
cd build
ctest --output-on-failure
```

The suite has 187 tests; 146 pass and 41 fail at the time of writing because of the open issues. See [tests/README.md](tests/README.md).

## Example session

```bash
mkdir demo && cd demo
ark init
printf '[user]\nname = Dev\nemail = dev@example.com\n' > .ark/config

echo "# My project" > README.md
mkdir src
echo "int main() { return 0; }" > src/main.cpp

ark add README.md src
GIT_EDITOR=/tmp/ark-msg.sh ark commit      # script from Quick start

echo "// more" >> src/main.cpp
ark add src/main.cpp
GIT_EDITOR=/tmp/ark-msg.sh ark commit

ark log
ark branch develop
ark switch develop
ark switch main
```
