#!/bin/bash

# store first arg as group and shift args
group=$1
shift

# other args are sizes
SIZES=("$@")

SCRIPTS=("graphenix" "alchemy_sqlite" "alchemy_mysql" "raw_sqlite" "raw_mysql")
COUNT=5

for size in "${SIZES[@]}"; do
    if [ -f "analysis/$group/result_$size.txt" ]; then
        rm "analysis/$group/result_$size.txt"
    fi

    echo "Running for size $size"
    for script_name in "${SCRIPTS[@]}"; do
        sum=0
        echo "  Running script $script_name"

        for ((i = 0; i < $COUNT; i++)); do
            result=$(python3 -m "analysis.$group.$script_name" "$size")
            sum=$(bc <<< "$sum + $result")
        done

        if [ $COUNT -gt 0 ]; then
            average=$(bc <<< "scale=2; $sum / $COUNT")
            echo "$average" >> "analysis/$group/result_$size.txt"
            echo "Average for $group $size was $average"
        fi

    done

done

# python3 -m "analysis.$group.graph"