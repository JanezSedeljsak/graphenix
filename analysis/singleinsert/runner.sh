#!/bin/bash

SIZES=(1000 10000 100000 1000000)
SCRIPTS=("graphenix.py" "alchemy_sqlite.py" "alchemy_mysql.py" "raw_mysql.py" "raw_sqlite.py")
COUNT=10

for size in "${SIZES[@]}"; do
    if [ -f "result_$size.txt" ]; then
        rm "result_$size.txt"
    fi

    echo "Running for size $size"
    for script_name in "${SCRIPTS[@]}"; do
        sum=0
        echo "  Running script $script_name"

        for ((i = 0; i < $COUNT; i++)); do
            result=$(python3 "$script_name" "$size")
            sum=$(bc <<< "$sum + $result")
        done

        if [ $COUNT -gt 0 ]; then
            average=$(bc <<< "scale=2; $sum / $COUNT")
            echo "$average" >> "result_$size.txt"
        fi

    done

done