# ark

## this readme is currently being put together and can have issues.

A tiny Git-like version control system I’m building in C++ to learn how Git works under the hood.

Right now it can:

* Make a repository folder (`.ark`)
* Turn a file into a blob object (with Git-style header + content)
* Store it compressed by its hash
* Read an object back by hash and print its contents
* Add files to the repository (staging area)

It’s minimal — just enough to explore the core ideas.

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
    src/main.cpp src/commands/*.cpp src/utils/compress.cpp \
    -L$(brew --prefix openssl@3)/lib -lssl -lcrypto -lz -o build/ark

# Linux
 g++ -std=c++17 -Iinclude src/main.cpp src/commands/*.cpp src/utils/compress.cpp -lssl -lcrypto -lz -o build/ark
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
    src/main.cpp src/commands/*.cpp src/utils/compress.cpp \
    -L<openssl_lib> -L<zlib_lib> -lssl -lcrypto -lz -o build\ark.exe
```

The binary will be at `build/ark` (or `build\Release\ark.exe` on Windows).

---

## Usage

Initialize a repo:

```bash
./build/ark init
```

Hash a file:

```bash
./build/ark hash-object README.md
```

Add a file to the repository:

```bash
./build/ark add README.md
```

Read an object back:

```bash
./build/ark cat-file <hash>
```

Quick demo:

```bash
./build/ark init
./build/ark add README.md
HASH=$(./build/ark hash-object README.md)
./build/ark cat-file "$HASH"
```

---

## How it works

* A blob object is: `blob <size>\0<file content>`
* It’s hashed with SHA-256
* Stored compressed under `.ark/objects/<first-2-chars>/<rest>`
* `cat-file` decompresses and prints the content
* `add` updates the staging area with file references

---

## Project layout

```
src/main.cpp                 -> CLI entry
src/commands/init.cpp        -> creates .ark
src/commands/hash-object.cpp -> stores blobs
src/commands/cat-file.cpp    -> reads blobs
src/commands/add.cpp         -> stages files for commit
src/utils/compress.cpp       -> zlib helper
include/                     -> headers
```

---

## License

MIT
