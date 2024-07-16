#!/bin/bash

# USYD CODE CITATION ACKNOWLEDGEMENT
# I declare that the majority of the script and inspiration comes from the following two links
# https://unix.stackexchange.com/questions/243489/bash-command-script-to-diff-all-mytestn-out-and-testn-out
# https://stackoverflow.com/questions/10523415/execute-command-on-all-files-in-a-directory

PKGMAIN="../../pkgmain"
# need to go inside this directory first
cd p1tests
# loops over all the tests in the directory
echo "---------------------------------------------------"

# either run a specific argument from the command line
if [ -n "$1" ]; then
    test_dirs="$1"
# or run all of them at once
else
    test_dirs=$(echo test* | tr ' ' '\n' | sort -V)
fi

for testdir in $test_dirs; do
    # check if directory exists
    if [[ -d "$testdir" ]]; then
        cd "$testdir"
        # extract names
        BPKGFILE="${testdir}.bpkg"
        OUTFILE="${testdir}.out"
        # choose flag depending on which test directory
        if [[ "$testdir" == "test7" || "$testdir" == "test8" ]]; then
            FLAG="-chunk_check"
        elif [[ "$testdir" == "test9" || "$testdir" == "test10" ]]; then
            FLAG="-min_hashes"
        elif [[ "$testdir" == "test11" ]]; then
            FLAG="-hashes_of b3e2ad4c6cdcfe2e7c53a744fd86fa70cf3673c33247b2f16f5394e4b92ea8ef"
        elif [[ "$testdir" == "test12" ]]; then
            FLAG="-hashes_of 4953053bbaf5f5ab3de26242fa78610aaa7ffb7957c0bb45247228fffdf54166"
        elif [[ "$testdir" == "test13" || "$testdir" == "test14" ]]; then
            FLAG="-file_check"
        else
            FLAG="-all_hashes"
        fi
        echo "Running test in directory: $testdir with flag $FLAG"
        # if file exists
        if [ -x "$PKGMAIN" ]; then
            # run file with flag -all_hashes
            OUTPUT=$("$PKGMAIN" "$BPKGFILE" $FLAG)
            echo "$PKGMAIN" "$BPKGFILE" $FLAG
            echo "$OUTPUT" > temp_output.txt
            diff temp_output.txt "$OUTFILE" > /dev/null
            # compare output
            if [ $? -eq 0 ]; then
                echo "Output for $testdir matches expected output."
            else
                echo "Output for $testdir does not match expected output. Differences:"
                diff temp_output.txt "$OUTFILE"
            fi
            # clean up
            rm temp_output.txt
            if [[ "$testdir" == "test13" ]]; then
                rm nonexistent.dat
            fi
        else
            # something went wrong
            echo "Error: '$PKGMAIN' does not exist or is not executable."
            exit 1
        fi
        # go back to orignal directory
        cd ..
    else 
        echo "Error: Test directory '$testdir' does not exist."
    fi
    echo "---------------------------------------------------"
done
echo "All tests completed."