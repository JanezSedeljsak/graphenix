#!/bin/bash

TEST_FILE="tests.out"

g++ -DIS_TESTING tests.cpp -o "$TEST_FILE"
"./$TEST_FILE"
status=$? # mark exit status the success of tests
rm -f "$TEST_FILE"

exit $status