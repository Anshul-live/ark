#!/bin/bash
set -e
source tests/_assert.sh

start_test 3 "add-dot"

cleanup() {
  rm -rf temp-repo
}
# trap cleanup EXIT

rm -rf temp-repo
mkdir temp-repo
cd temp-repo

ark init >/dev/null

# Blob hash calculator (MUST match ark)
calc_blob_hash() {
  local file="$1"
  local size=$(wc -c <"$file" | tr -d ' ')
  printf "blob %s\0" "$size"
  cat "$file"
}

blob_hash() {
  calc_blob_hash "$1" | shasum -a 256 | awk '{print $1}'
}

mkdir -p src/utils
mkdir -p data/images

echo "alpha" >a.txt
echo "beta" >src/b.txt
echo "gamma" >src/utils/c.txt
echo "delta" >data/d.txt
echo "epsilon" >data/images/e.txt

# expected hashes (correct Git-style blob)
H_A=$(blob_hash a.txt)
H_B=$(blob_hash src/b.txt)
H_C=$(blob_hash src/utils/c.txt)
H_D=$(blob_hash data/d.txt)
H_E=$(blob_hash data/images/e.txt)

# Perform add .
ark add . >/dev/null

assert_file ".ark/index"

# filename checks
grep -q "a.txt" .ark/index || fail_test "missing a.txt"
grep -q "src/b.txt" .ark/index || fail_test "missing b.txt"
grep -q "src/utils/c.txt" .ark/index || fail_test "missing c.txt"
grep -q "data/d.txt" .ark/index || fail_test "missing d.txt"
grep -q "data/images/e.txt" .ark/index || fail_test "missing e.txt"

# hash checks
grep -q "$H_A" .ark/index || fail_test "hash mismatch for a.txt"
grep -q "$H_B" .ark/index || fail_test "hash mismatch for b.txt"
grep -q "$H_C" .ark/index || fail_test "hash mismatch for c.txt"
grep -q "$H_D" .ark/index || fail_test "hash mismatch for d.txt"
grep -q "$H_E" .ark/index || fail_test "hash mismatch for e.txt"

# object file existence checks
assert_file ".ark/objects/${H_A:0:2}/${H_A:2}"
assert_file ".ark/objects/${H_B:0:2}/${H_B:2}"
assert_file ".ark/objects/${H_C:0:2}/${H_C:2}"
assert_file ".ark/objects/${H_D:0:2}/${H_D:2}"
assert_file ".ark/objects/${H_E:0:2}/${H_E:2}"

pass_test
