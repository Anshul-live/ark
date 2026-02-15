# Ark

A minimal Git-like version control system written in C++ to understand how Git works under the hood.

## What is Ark?

Ark implements the core concepts of version control:
- **Content-addressable storage** using SHA-256 hashes
- **Object model** (Blob, Tree, Commit)
- **Staging area** (index)
- **Branching** and commit history

## Quick Start

```bash
# Build
mkdir build && cd build
cmake ..
make

# Initialize repository
./ark init

# Configure user (required for commits)
echo -e "[user]\nname = Your Name\nemail = you@example.com" > .ark/config

# Add files and commit
./ark add .
./ark commit

# Create a branch
./ark branch feature
./ark switch feature

# View history
./ark log
```

## Documentation

- [USAGE.md](USAGE.md) - Detailed usage guide and command reference
- [ARCHITECTURE.md](ARCHITECTURE.md) - Internal architecture and design

## Features

| Feature | Description |
|---------|-------------|
| Repository | Initialize and manage `.ark` directory |
| Objects | Blob, Tree, Commit with zlib compression |
| Staging | Index-based staging with file modes |
| Branches | Create, list, switch branches |
| History | View commit log and status |
| Ignore | `.arkignore` file support |

## Requirements

- C++17 compiler
- CMake >= 3.16
- OpenSSL
- zlib

## Building

See [USAGE.md](USAGE.md#building) for detailed build instructions.

## Testing

```bash
# Unit tests
cd build && ./test_ark-end tests
./

# End-totests/test_e2e.sh
```

## License

MIT
