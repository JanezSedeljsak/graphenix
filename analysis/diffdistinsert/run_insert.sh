#!/bin/bash

# other args are sizes
SIZES=("$@")

SCRIPTS=("graphenix_bulk_index" "graphenix_bulk" "mysql_bulk_index" "mysql_bulk" "raw_sqlite_index" "raw_sqlite")

for size in "${SIZES[@]}"; do
    echo "Running for size $size"
    for script_name in "${SCRIPTS[@]}"; do
        python3 -m "analysis.diffdistinsert.$script_name" "$size"
    done

done