#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARK_BIN="${SCRIPT_DIR}/../build/ark"
TEST_DIR="/tmp/ark_e2e_test"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASSED=0
FAILED=0

setup() {
    rm -rf "$TEST_DIR"
    mkdir -p "$TEST_DIR"
    cd "$TEST_DIR"
}

cleanup() {
    rm -rf "$TEST_DIR"
}

pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((PASSED++))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((FAILED++))
}

section() {
    echo ""
    echo -e "${YELLOW}=== $1 ===${NC}"
}

assert_file_exists() {
    if [ -f "$1" ]; then
        pass "File exists: $1"
    else
        fail "File not found: $1"
    fi
}

assert_file_not_exists() {
    if [ ! -f "$1" ]; then
        pass "File does not exist: $1"
    else
        fail "File should not exist: $1"
    fi
}

assert_dir_exists() {
    if [ -d "$1" ]; then
        pass "Directory exists: $1"
    else
        fail "Directory not found: $1"
    fi
}

assert_equals() {
    if [ "$1" == "$2" ]; then
        pass "$3"
    else
        fail "$3 - Expected: '$1', Got: '$2'"
    fi
}

assert_contains() {
    if echo "$1" | grep -q "$2"; then
        pass "$3"
    else
        fail "$3"
    fi
}

run_ark() {
    "$ARK_BIN" "$@" 2>&1
}

section "Test 1: Init Repository"
setup
run_ark init
assert_dir_exists "$TEST_DIR/.ark"
assert_file_exists "$TEST_DIR/.ark/HEAD"
assert_file_exists "$TEST_DIR/.ark/index"
assert_file_exists "$TEST_DIR/.ark/config"
assert_file_exists "$TEST_DIR/.ark/refs/heads/main"
assert_file_exists "$TEST_DIR/.ark/ark.log"
assert_equals "ref: refs/heads/main" "$(cat $TEST_DIR/.ark/HEAD)"

section "Test 2: Add Single File"
echo "Hello World" > "$TEST_DIR/test.txt"
run_ark add test.txt
assert_file_exists "$TEST_DIR/.ark/index"
assert_contains "$(cat $TEST_DIR/.ark/index)" "test.txt"

section "Test 3: Commit Creation"
echo -e "[user]\nname = Test User\nemail = test@test.com" > "$TEST_DIR/.ark/config"
echo "test commit" > /tmp/commit_msg.txt
EDITOR="cat" run_ark commit < /tmp/commit_msg.txt
assert_dir_exists "$TEST_DIR/.ark/objects"
HASH=$(cat "$TEST_DIR/.ark/refs/heads/main")
if [ "$HASH" != "0000000000000000000000000000000000000000000000000000000000000000" ]; then
    pass "Commit created with hash: $HASH"
else
    fail "Commit hash is still null"
fi

section "Test 4: Log Command"
LOG_OUTPUT=$(run_ark log 2>&1 || true)
if echo "$LOG_OUTPUT" | grep -q "commit:"; then
    pass "Log shows commit"
else
    fail "Log does not show commit"
fi

section "Test 5: Status Command"
STATUS_OUTPUT=$(run_ark status 2>&1 || true)
if echo "$STATUS_OUTPUT" | grep -q "Changes to be committed:"; then
    pass "Status shows changes"
else
    fail "Status output unexpected"
fi

section "Test 6: Branch Creation"
BRANCH_OUTPUT=$(run_ark branch feature 2>&1)
run_ark branch
assert_contains "$(run_ark branch 2>&1)" "feature"

section "Test 7: Add Multiple Files"
echo "File 1" > "$TEST_DIR/file1.txt"
echo "File 2" > "$TEST_DIR/file2.txt"
mkdir -p "$TEST_DIR/subdir"
echo "File 3" > "$TEST_DIR/subdir/file3.txt"
run_ark add file1.txt file2.txt subdir/file3.txt
INDEX_CONTENT=$(cat "$TEST_DIR/.ark/index")
assert_contains "$INDEX_CONTENT" "file1.txt"
assert_contains "$INDEX_CONTENT" "file2.txt"
assert_contains "$INDEX_CONTENT" "file3.txt"

section "Test 8: Branch Switching"
echo "feature work" > "$TEST_DIR/feature.txt"
run_ark add feature.txt
echo "feature commit" > /tmp/commit_msg.txt
EDITOR="cat" run_ark commit < /tmp/commit_msg.txt
run_ark switch feature
WORKDIR_FILES=$(ls "$TEST_DIR" | grep -v "^\.")
if echo "$WORKDIR_FILES" | grep -q "feature.txt"; then
    pass "Feature file exists on feature branch"
else
    fail "Feature file missing on feature branch"
fi
if echo "$WORKDIR_FILES" | grep -q "test.txt"; then
    fail "Main branch file should not exist on feature branch"
else
    pass "Main branch file correctly not on feature branch"
fi

section "Test 9: Switch Back to Main"
run_ark switch main
WORKDIR_FILES=$(ls "$TEST_DIR")
if echo "$WORKDIR_FILES" | grep -q "test.txt"; then
    pass "Main branch file restored"
else
    fail "Main branch file missing"
fi

section "Test 10: Add . (Current Directory)"
rm -f "$TEST_DIR/.ark/index"
touch "$TEST_DIR/newfile.txt"
cd "$TEST_DIR"
run_ark add .
INDEX_CONTENT=$(cat "$TEST_DIR/.ark/index")
assert_contains "$INDEX_CONTENT" "newfile.txt"

section "Test 11: Object Storage"
cd "$TEST_DIR"
echo "object test" > "$TEST_DIR/object.txt"
run_ark add object.txt
OBJECT_COUNT=$(find "$TEST_DIR/.ark/objects" -type f | wc -l)
if [ "$OBJECT_COUNT" -gt 0 ]; then
    pass "Objects stored: $OBJECT_COUNT files"
else
    fail "No objects stored"
fi

section "Test 12: Cat File"
BLOB_HASH=$(run_ark hash-object "$TEST_DIR/test.txt" 2>/dev/null | tail -1)
if [ -n "$BLOB_HASH" ]; then
    CAT_OUTPUT=$(run_ark cat-file "$BLOB_HASH" 2>&1)
    if echo "$CAT_OUTPUT" | grep -q "Hello World"; then
        pass "Cat-file works correctly"
    else
        fail "Cat-file output incorrect"
    fi
else
    fail "Hash object failed"
fi

section "Test 13: Status After Multiple Commits"
echo "updated content" >> "$TEST_DIR/test.txt"
run_ark add test.txt
STATUS_OUTPUT=$(run_ark status 2>&1)
assert_contains "$STATUS_OUTPUT" "Changes to be committed:"

section "Test 14: Create Nested Directory Structure"
mkdir -p "$TEST_DIR/src/utils"
mkdir -p "$TEST_DIR/src/include"
echo "code" > "$TEST_DIR/src/main.cpp"
echo "header" > "$TEST_DIR/src/include/header.h"
run_ark add src
INDEX_CONTENT=$(cat "$TEST_DIR/.ark/index")
assert_contains "$INDEX_CONTENT" "src/main.cpp"
assert_contains "$INDEX_CONTENT" "src/include/header.h"

section "Test 15: Ignore File"
echo "ignored.txt" > "$TEST_DIR/ignored.txt"
echo "*.log" > "$TEST_DIR/.arkignore"
run_ark add .
INDEX_CONTENT=$(cat "$TEST_DIR/.ark/index")
if echo "$INDEX_CONTENT" | grep -q "ignored.txt"; then
    fail "Ignored file was staged"
else
    pass "Ignored file correctly not staged"
fi

section "Test 16: Update Ref"
COMMIT_HASH=$(cat "$TEST_DIR/.ark/refs/heads/main")
run_ark update-ref refs/heads/main "$COMMIT_HASH"
NEW_HASH=$(cat "$TEST_DIR/.ark/refs/heads/main")
assert_equals "$COMMIT_HASH" "$NEW_HASH" "Update ref works"

section "Test 17: Write Tree"
TREE_HASH=$(run_ark write-tree 2>&1 | tail -1)
if [ -n "$TREE_HASH" ] && [ ${#TREE_HASH} -eq 64 ]; then
    pass "Write tree returned valid hash"
else
    fail "Write tree failed"
fi

section "Test 18: Commit Tree"
echo -e "[user]\nname = Test\nemail = t@t.com" > "$TEST_DIR/.ark/config"
TREE_HASH=$(run_ark write-tree 2>&1 | tail -1)
COMMIT_HASH=$(run_ark commit-tree "$TREE_HASH" "" "" "manual commit" 2>&1 | tail -1)
if [ -n "$COMMIT_HASH" ] && [ ${#COMMIT_HASH} -eq 64 ]; then
    pass "Commit tree created: $COMMIT_HASH"
else
    fail "Commit tree failed"
fi

section "Test 19: Multiple Branches"
run_ark branch branch1
run_ark branch branch2
BRANCH_LIST=$(run_ark branch 2>&1)
assert_contains "$BRANCH_LIST" "branch1"
assert_contains "$BRANCH_LIST" "branch2"

section "Test 20: Log on Different Branch"
run_ark switch branch1
echo "branch1 content" > "$TEST_DIR/branch1.txt"
run_ark add branch1.txt
echo "branch1 commit" > /tmp/commit_msg.txt
EDITOR="cat" run_ark commit < /tmp/commit_msg.txt
run_ark switch main
LOG_MAIN=$(run_ark log 2>&1)
run_ark switch branch1
LOG_BRANCH=$(run_ark log 2>&1)
if [ "$LOG_MAIN" != "$LOG_BRANCH" ]; then
    pass "Different branches have different history"
else
    fail "Branch history should differ"
fi

section "Test 21: Untracked Files Detection"
echo "untracked.txt" > "$TEST_DIR/untracked.txt"
STATUS_OUTPUT=$(run_ark status 2>&1)
assert_contains "$STATUS_OUTPUT" "untracked"

section "Test 22: Modified Files Detection"
echo "modified" >> "$TEST_DIR/test.txt"
STATUS_OUTPUT=$(run_ark status 2>&1)
assert_contains "$STATUS_OUTPUT" "modified"

section "Test 23: Empty Repository Status"
cd "$TEST_DIR"
rm -rf .ark
run_ark init
STATUS_OUTPUT=$(run_ark status 2>&1 || true)
if echo "$STATUS_OUTPUT" | grep -q "Untracked files:"; then
    pass "Empty repo shows untracked files"
else
    fail "Empty repo status unexpected"
fi

section "Test 24: Config Loading"
echo -e "[user]\nname = ConfigTest\nemail = config@test.com" > "$TEST_DIR/.ark/config"
run_ark init 2>/dev/null || true
CONFIG_NAME=$(grep "name" "$TEST_DIR/.ark/config" | cut -d'=' -f2 | tr -d ' ')
assert_equals "ConfigTest" "$CONFIG_NAME" "Config name loaded"

section "Test 25: Index Preservation Across Operations"
echo "persistent" > "$TEST_DIR/persistent.txt"
run_ark add persistent.txt
run_ark status > /dev/null 2>&1
INDEX_PERSISTS=$(cat "$TEST_DIR/.ark/index")
if echo "$INDEX_PERSISTS" | grep -q "persistent.txt"; then
    pass "Index preserved across operations"
else
    fail "Index not preserved"
fi

echo ""
echo "========================================"
echo -e "${GREEN}PASSED: $PASSED${NC}"
echo -e "${RED}FAILED: $FAILED${NC}"
echo "========================================"

cleanup

if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
