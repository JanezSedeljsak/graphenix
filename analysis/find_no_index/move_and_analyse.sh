#!/bin/bash

SIZES=("$@")
dir="analysis/find_no_index"

for size in "${SIZES[@]}"; do
    cp "analysis/find_index/result_$size.txt" "$dir/iresult_$size.txt"

    { while IFS=$'\t' read -r value1 value2; do
        result=$(echo "scale=2; $value1 / $value2" | bc)
        echo "$result"
      done } < <(paste "$dir/result_$size.txt" "$dir/iresult_$size.txt") > "$dir/speedup_$size.txt"
done