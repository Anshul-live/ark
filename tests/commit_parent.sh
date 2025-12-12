#!/bin/bash
set -e
source tests/_assert.sh

start_test 6 "commit-parent"

cleanup() { rm -rf temp-repo; }
trap cleanup EXIT

rm -rf temp-repo
mkdir temp-repo
cd temp-repo

ark init >/dev/null

# required config
echo "[user]" >.ark/config
echo "name = anshul" >>.ark/config
echo "email = anshul@gmail.com" >>.ark/config

cp ../tests/_fake_editor.sh fake_editor.sh
chmod +x fake_editor.sh
export EDITOR="$(pwd)/fake_editor.sh"

# ===== FIRST COMMIT =====
echo "v1" >a.txt
ark add a.txt >/dev/null

export COMMIT_MESSAGE="commit1"
ark commit >/dev/null

REF=".ark/refs/heads/main"
COMMIT1=$(awk '{print $1}' "$REF")
assert_file ".ark/objects/${COMMIT1:0:2}/${COMMIT1:2}"

# ===== SECOND COMMIT =====
echo "v2" >a.txt
ark add a.txt >/dev/null

export COMMIT_MESSAGE="commit2"
ark commit >/dev/null

COMMIT2=$(awk '{print $1}' "$REF")

[[ "$COMMIT2" != "$COMMIT1" ]] || fail_test "second commit hash == first commit hash"

OBJ2=".ark/objects/${COMMIT2:0:2}/${COMMIT2:2}"
assert_file "$OBJ2"

# read commit body using Ark's own decompressor
ark cat-file "$COMMIT2" >BODY
COMMIT2_BODY=$(cat BODY)

# verify parent pointer
PARENT2=$(echo "$COMMIT2_BODY" | awk '/^parent / { print $2 }')

[[ "$PARENT2" = "$COMMIT1" ]] || fail_test "second commit parent incorrect"

# verify tree exists
TREE2=$(echo "$COMMIT2_BODY" | awk '/^tree / { print $2 }')
assert_file ".ark/objects/${TREE2:0:2}/${TREE2:2}"

pass_test
