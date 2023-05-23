#!/bin/bash

TEST_FILE="tests.out"

g++ -DIS_TESTING tests.cpp -o "$TEST_FILE"
"./$TEST_FILE"
rm -f "$TEST_FILE"