#!/bin/bash

GREEN="\033[32m"
RED="\033[31m"
NC="\033[0m"

start_test() {
  TEST_NUM=$1
  TEST_NAME=$2
  echo "[$TEST_NAME]"
}

pass_test() {
  echo -e "Test $TEST_NUM : ${GREEN}PASS${NC}"
  exit 0
}

fail_test() {
  echo -e "Test $TEST_NUM : ${RED}FAIL${NC} - $1"
  exit 1
}

assert_dir() {
  if [ ! -d "$1" ]; then
    fail_test "Directory missing: $1"
  fi
}

assert_file() {
  if [ ! -f "$1" ]; then
    fail_test "File missing: $1"
  fi
}

assert_eq() {
  if [ "$1" != "$2" ]; then
    fail_test "Expected '$1' == '$2'"
  fi
}
