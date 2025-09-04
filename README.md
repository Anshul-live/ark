# ark

A tiny Git-like version control system I’m building in C++ to learn how Git works under the hood.  

Right now it can:  
- make a repo folder (`.ark`)  
- turn a file into a blob object (with Git-style header + content)  
- store it compressed by its hash  
- read an object back by hash and print its contents  

It’s super minimal — just enough to play with the core ideas.  

---

## Build

You’ll need:  
- C++17 compiler (clang/gcc)  
- CMake >= 3.16  
- OpenSSL (SHA-256)  
- zlib (compression)  

On macOS:  

```bash
brew install cmake openssl@3 zlib
```

Build it:  

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix openssl@3);$(brew --prefix zlib)"
cmake --build build -j4
```

The binary will be at `build/ark`.  

---

## Usage

Make a repo:  

```bash
./build/ark init
```

Hash a file:  

```bash
./build/ark hash-object README.md
```

Read an object back:  

```bash
./build/ark cat-file <hash>
```

Quick demo:  

```bash
./build/ark init
HASH=$(./build/ark hash-object README.md | awk '/Object hashed:/ {print $3}')
./build/ark cat-file "$HASH"
```

---

## How it works

- A blob object looks like: `blob <size>\0<file content>`  
- I hash this with SHA-256  
- Store it compressed under `.ark/objects/<first-2-chars>/<rest>`  
- `cat-file` reverses it and prints the content  

---

## Project layout

```
src/main.cpp                 -> CLI entry
src/commands/init.cpp        -> makes .ark
src/commands/hash-object.cpp -> stores blobs
src/commands/cat-file.cpp    -> reads blobs
src/utils/compress.cpp       -> zlib helper
include/                     -> headers
```

---

## License

MIT
