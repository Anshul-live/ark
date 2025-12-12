#!/bin/bash
set -e
source tests/_assert.sh

start_test 4 "add-files-dirs"

cleanup() {
  rm -rf temp-repo
}
# trap cleanup EXIT

rm -rf temp-repo
mkdir temp-repo
cd temp-repo

ark init >/dev/null

calc_blob_hash() {
  local file="$1"
  local size=$(wc -c <"$file" | tr -d ' ')
  printf "blob %s\0" "$size"
  cat "$file"
}

blob_hash() {
  calc_blob_hash "$1" | shasum -a 256 | awk '{print $1}'
}

mkdir -p module/sub
echo "A" >a.txt
echo "BB" >module/b.txt
echo "CCC" >module/sub/c.txt

H_A=$(blob_hash a.txt)
H_B=$(blob_hash module/b.txt)
H_C=$(blob_hash module/sub/c.txt)

# add mixture of files + dirs
ark add a.txt module >/dev/null

assert_file ".ark/index"

grep -q "a.txt" .ark/index || fail_test "missing a.txt"
grep -q "module/b.txt" .ark/index || fail_test "missing b.txt"
grep -q "module/sub/c.txt" .ark/index || fail_test "missing c.txt"

grep -q "$H_A" .ark/index || fail_test "hash mismatch for a.txt"
grep -q "$H_B" .ark/index || fail_test "hash mismatch for b.txt"
grep -q "$H_C" .ark/index || fail_test "hash mismatch for c.txt"

assert_file ".ark/objects/${H_A:0:2}/${H_A:2}"
assert_file ".ark/objects/${H_B:0:2}/${H_B:2}"
assert_file ".ark/objects/${H_C:0:2}/${H_C:2}"

pass_test
ass_test
