#!/bin/bash

CPP_FILES=("tests.cpp" "tests_size_3.cpp")
EXECUTABLES=()

for file in "${CPP_FILES[@]}"; do
    executable="${file%.*}"
    EXECUTABLES+=("$executable")

    g++ -DIS_TESTING "$file" -o "$executable"
    if [ $? -ne 0 ]; then
        echo "Compilation failed for $file"
        status=1
        break
    fi
done

if [ "$status" != 1 ]; then
    for executable in "${EXECUTABLES[@]}"; do
        ./"$executable"
        if [ $? -ne 0 ]; then
            echo "Execution failed for $executable"
            status=1
            break
        fi
    done
fi

rm -f "${EXECUTABLES[@]}"

exit "${status:-0}"