#!/bin/bash

# store first arg as group and shift args
group=$1
shift

# other args are sizes
SIZES=("$@")

SCRIPTS=("graphenix" "alchemy_sqlite" "alchemy_mysql" "raw_sqlite" "raw_mysql" "graphenix_bulk")
COUNT=10

calculate_average() {
    IFS=$'\n' sorted_array=($(sort -n <<< "${array[*]}"))
    unset IFS

    trimmed_array=("${sorted_array[@]:2:$(( ${#sorted_array[@]} - 4 ))}")

    local sum=0
    for number in "${trimmed_array[@]}"; do
        sum=$(awk "BEGIN { print $sum + $number; exit }")
    done
    average=$(awk "BEGIN { printf \"%.2f\", $sum / ${#trimmed_array[@]}; exit }")
}

for size in "${SIZES[@]}"; do
    if [ -f "analysis/$group/result_$size.txt" ]; then
        rm "analysis/$group/result_$size.txt"
    fi

    echo "Running for size $size"
    for script_name in "${SCRIPTS[@]}"; do
        if [ ! -f "analysis/$group/$script_name.py" ]; then
            continue
        fi

        declare -a array=()
        echo "  Running script $script_name"

        for ((i = 0; i < $COUNT; i++)); do
            result=$(python3 -m "analysis.$group.$script_name" "$size")
            array+=("$(bc <<< "$result")")
        done
        
        calculate_average
        # average=$(bc <<< "scale=2; $sum / $COUNT")
        echo "$average" >> "analysis/$group/result_$size.txt"
        echo "  Average was: $average"
        echo ""

    done

done

if [ -f "analysis/$group/graph.py" ]; then
    python3 -m "analysis.$group.graph"
fi