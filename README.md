# Ark

Ark is a small Git-like version control system written in C++17. It stores files as content-addressed objects (blob, tree, commit) hashed with SHA-256 and compressed with zlib, keeps a text index as the staging area, and supports branches. It was written to learn how Git works, and the code is small enough to read in an afternoon.

This page covers what Ark does and does not do, how to build and try it, how to run the tests, and the open problems. Details are in [USAGE.md](USAGE.md) and [ARCHITECTURE.md](ARCHITECTURE.md).

## What Ark does not do

- No merge. `src/cli/commands/merge.cpp` exists but is not built and not wired into `src/main.cpp`.
- No remotes: no clone, fetch or push.
- No diff command, reset, tag, stash or rebase.
- No packfiles. Every object is one zlib-compressed file.
- No glob patterns in `.arkignore` (exact paths and directory prefixes only).
- No compatibility with Git's on-disk format. Hashes are SHA-256, and tree and commit bodies differ from Git's.

## Quick start

Build (more detail in [USAGE.md](USAGE.md#building)):

```bash
cmake -S . -B build
cmake --build build -j4
export PATH="$PWD/build:$PATH"
```

Try it in a scratch directory:

```bash
mkdir demo && cd demo
ark init
printf '[user]\nname = Ada\nemail = ada@example.com\n' > .ark/config

echo "hello" > hello.txt
ark add hello.txt
ark commit          # opens $GIT_EDITOR, $VISUAL, $EDITOR or vi; type a message
ark log

ark branch feature
ark switch feature
```

Notes on this sequence:

- Name files explicitly. `ark add .` currently stages `.ark/` and `.ark.log` too (see Known issues).
- `ark commit` always opens an editor. For scripts, set `GIT_EDITOR` to an executable that writes the message into the file passed as its first argument.
- Ark prints some `[INFO]` log lines on stdout.

## Features

- Commands: `init`, `add`, `commit`, `status`, `log`, `branch`, `switch`
- Plumbing commands: `hash-object`, `cat-file`, `write-tree`, `commit-tree`, `update-ref`
- Regular and executable file modes recorded in trees
- `.arkignore` support
- The repository is found by walking up from the current directory, so commands work from subdirectories
- `switch` rewrites the working tree and the index to match the target branch

## Requirements

- C++17 compiler
- CMake 3.16 or newer
- OpenSSL (SHA-256 through the EVP API)
- zlib

CMake fetches GoogleTest v1.14.0 at configure time, so the first configure needs network access.

## Testing

```bash
cmake --build build -j4
cd build
ctest --output-on-failure          # one process per test
./test_ark                         # or everything in one process
./test_ark --gtest_filter='Branching.*'
```

The suite has 187 tests. At the time of writing 146 pass and 41 fail. Each failing test checks behaviour that is still broken (see below). [tests/README.md](tests/README.md) describes the layout and maps failing tests to issues.

## Known issues

These are open at the time of writing. Each has at least one failing test.

- `ark switch` can discard uncommitted work. It moves HEAD first, then overwrites or deletes files, without checking for staged or modified files.
- `ark add .` stages `.ark/` and `.ark.log`, and `ark status` lists them as untracked.
- `.ark.log` is written to the current directory, not to `.ark/`, and is created even outside a repository.
- Ref and branch names are not validated by the commands. `ark branch ../x` writes outside `refs/heads/`, and `ark update-ref` accepts any path and any string as a hash.
- `ark add` accepts paths outside the repository.
- `ark commit -m` is not supported. The message always comes from the editor, through a fixed temp file name, and the commit body has no blank line between the header and the message.
- Commits with nothing staged are allowed.
- The global config files are never read. The code opens the literal paths `~/.arkconfig` and `~/config/ark/config` without expanding `~`. Only `.ark/config` works.
- Deletions cannot be staged. A removed file stays in the index, and `status` does not report it.
- The root commit records a zero hash as its parent (`parent 000...0`).
- Symlinks are followed when staging, so the target's contents are stored as a regular file.
- `switch` compares object hashes only. It ignores mode changes, and it mishandles a file replaced by a directory or the reverse.
- `ark log` can print a commit more than once when history has merges, and it refuses to run on a detached HEAD.
- Some failures exit with status 0: a missing identity in `commit`, a missing file in `hash-object`, a missing path in `add`.
- `commit-tree` reads its arguments at the wrong positions, so parents and message are dropped or read out of range.
- INFO log lines go to stdout and mix into command output.

## License

MIT
