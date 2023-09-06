#!/bin/bash

GRAPHENIX_DIR="./../graphenix"

CPP_LINES=$(find "$GRAPHENIX_DIR" \( -type d -name "tests" -prune \) -o -type f \( -name "*.hpp" -o -name "*.h" -o -name "*.cpp" \) -exec cat {} \; | wc -l)
PY_LINES=$(find "$GRAPHENIX_DIR" \( -type d -name "tests" -prune \) -o -type f -name "*.py" -exec cat {} \; | wc -l)

echo "Py  lines $PY_LINES"
echo "C++ lines $CPP_LINES"