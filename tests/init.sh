#!/bin/bash
set -e
source tests/_assert.sh

start_test 1 "init"

rm -rf temp-repo
mkdir temp-repo
cd temp-repo

ark init >/dev/null

assert_dir ".ark"
assert_dir ".ark/refs"
assert_dir ".ark/refs/heads"
assert_file ".ark/HEAD"
assert_file ".ark/index"
assert_file ".ark/config"

pass_test
