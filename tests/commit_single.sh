#!/bin/bash
set -e
source tests/_assert.sh

start_test 5 "commit-single"

cleanup() { rm -rf temp-repo; }
trap cleanup EXIT

rm -rf temp-repo
mkdir temp-repo
cd temp-repo

ark init >/dev/null

# required config for commit
echo "[user]" >.ark/config
echo "name = anshul" >>.ark/config
echo "email = anshul@gmail.com" >>.ark/config

# fake editor for commit messages
cp ../tests/_fake_editor.sh fake_editor.sh
chmod +x fake_editor.sh

# add a file
echo "hello" >a.txt
ark add a.txt >/dev/null

export EDITOR="$(pwd)/fake_editor.sh"
export COMMIT_MESSAGE="first commit"

ark commit >/dev/null

# read commit hash
REF=".ark/refs/heads/main"
COMMIT1=$(awk '{print $1}' "$REF")

assert_file ".ark/objects/${COMMIT1:0:2}/${COMMIT1:2}"

# EXTRACT commit body exactly through Ark
ark cat-file "$COMMIT1" >BODY

COMMIT_BODY=$(cat BODY)

# validate commit fields
echo "$COMMIT_BODY" | grep -q "^tree " || fail_test "missing tree line"
echo "$COMMIT_BODY" | grep -q "^parent " || fail_test "missing parent line"
echo "$COMMIT_BODY" | grep -q "^committer " || fail_test "missing committer line"
echo "$COMMIT_BODY" | grep -q "first commit" || fail_test "missing commit message"

# validate parent hash for first commit
Z64="0000000000000000000000000000000000000000000000000000000000000000"
PARENT=$(echo "$COMMIT_BODY" | awk '/^parent / { print $2 }')

[ "$PARENT" = "$Z64" ] || fail_test "first commit parent should be zero hash"

# validate tree object exists
TREE_HASH=$(echo "$COMMIT_BODY" | awk '/^tree / { print $2 }')
assert_file ".ark/objects/${TREE_HASH:0:2}/${TREE_HASH:2}"

pass_test
