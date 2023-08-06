#!/bin/bash

# other args are sizes
SIZES=("$@")

SCRIPTS=("graphenix_bulk_insert" "mysql_bulk_insert" "raw_sqlite_insert")

for size in "${SIZES[@]}"; do
    echo "Running for size $size"
    for script_name in "${SCRIPTS[@]}"; do
        python3 -m "analysis.join.$script_name" "$size"
    done

done