#!/bin/bashcmake -S . \
-B build \
  -DCMAKE_PREFIX_PATH="$(brew --prefix openssl@3):$(brew --prefix zlib)" &&
  cmake --build build -- -j4
