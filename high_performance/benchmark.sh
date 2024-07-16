#!/bin/bash

NUM_ITERATIONS=15
OUTPUT_FILE="15_15_results.csv"
benchmark() {
    local binary=$1
    local flag=$2
    local total_time=0
    local path="benchmark1.bpkg"

    for ((i = 1; i <= NUM_ITERATIONS; i++)); do
        local start=$(date +%s.%N)
        ./$binary $path $flag > /dev/null 2>&1
        local end=$(date +%s.%N)
        local elapsed=$(echo "$end - $start" | bc)
        total_time=$(echo "$total_time + $elapsed" | bc)
    done
    echo "$total_time"
}

TEST_ITERATIONS=15
for ((i = 0; i <= TEST_ITERATIONS; i++)); do
    echo "Benchmarking pkgmain..."
    pkgmain_time=$(benchmark "../pkgmain" "-all_hashes")
    echo "pkgmain total time: $pkgmain_time seconds"
    echo "pkgmain,$pkgmain_time" >> $OUTPUT_FILE

    echo "Benchmarking pkgmain_parallel..."
    pkgmain_parallel_time=$(benchmark "../pkgmain_parallel" "-all_hashes")
    echo "pkgmain_parallel total time: $pkgmain_parallel_time seconds"
    echo "pkgmain_parallel,$pkgmain_parallel_time" >> $OUTPUT_FILE
done