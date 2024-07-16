# Project Overview

This project is organized to ensure efficient compilation and testing of the 
`pkgmain` and `btide` binaries. It includes multiple folders for source files, 
headers, tests, and resources. Below is a detailed description of each component
and instructions on how to run tests.

# Organization of Software

`Makefile` file: contains compilation flags and dependencies to ensure binaries 
are correctly linked and compiled

`include` folder: contains header files for specific sections of code; this is
automatically included during compilation in the `Makefile`

`p1tests` folder: contains all the tests for part 1

`p2tests` folder: contains all the tests for part 2

`resources` folder: contains all the intial resources given when cloning git
repo; contains sample format for `.bpkg` files, and several binaries

`src` folder: contains all the `.c` source files

`part1sel.txt` folder: used for autograding on Ed

`config.cfg` folder: automatically committed when cloning git, contains a sample
format for the config file needed to run `btide` e.g. `./btide config.cfg`

`high_performance` folder: containing a script to run the benchmark, a `.data` 
and a `.bpkg` file for testing. Lastly there are some `csv` files generated 
from the testing, as well as a python script which creates some graphs of the
data.

# Tests

There are 2 folders `p1tests` and `p2tests` which contain the tests for part 1
and part 2 respectively.  Each folder contains the necessary test files and 
scripts to validate the functionalities of the respective parts. 

There is a corresponding `p1test.sh` and `p2test.sh` in the home directory 
of this assignment. 

For `p1tests`, make sure `pkgmain` is in the home directory 
For `p2tests`, make sure `btide` is in the home directory

## OPTION 1 BASH SCRIPT
For `p1tests`, run `make p1tests`
For `p2tests`, run `make p2tests`

## OPTION 2 MAKEFILE
For `p1tests`, run `bash p1test.sh`
For `p2tests`, run `bash p2test.sh`

## NOTE

Note that for `p2tests`, I have used a Python script to create 2-3 running
instances of `btide`. However, because the order of the `.in` files will always
change, there will be a race condition. This might result in occasional 
mismatches in the output, even though there is a specific order I want 
them to run in.

They may also deadlock, so you may need to CTRL C and run again

# High Performance Merkle

# CHANGES

I have set out to
- minimise the time required for the I/O bound problem of large volumes 
of data to load and hash using multiple threads

I have done this by incorporating 3 threads, each having to do 1/3 of the work
a single thread would've had to do. This was achieved with a `ThreadData` struct
which contains the work each thread needs to do. 

This will minimise time as data can be read  3 times as fast. 
I also have created a `set_error()` function that will tell me if the threads 
have completed their work succesfully, and if not, then I can  propagate 
an error to end the program.

Lastly I join the threads all together before completing the merkle tree 
as usual.

## TESTING

To test the efficiency, I have created a large sample `.dat` file using

`cat /dev/urandom | head -c 2000000 > benchmark1.bpkg`

and created a corresponding `.bpkg` file which will be used as the entry point

`./pkgmake benchmark1.data --nchunks 10000 --output benchmark1.bpkg`

Next, in the make file I have created two binaries, `pkgmain` using the normal 
`merkletree.c` and `pkgmain_parallel` using `merkletree_parallel.c`. This will
create two identical binaries only differing on their merkle tree 
implementation.

Then I created a bash script which will run these binaries on the data and log
the time. I have two variables in the script, one is `TEST_ITERATIONS` which
is the number of times the script itself is repeated, and each time
`NUM_ITERATIONS` specifies the number of times the binary is run on the data.
Thus the total number of times the binary is run on the data is 
`TEST_ITERATIONS * NUM_ITERATIONS`

I have done this because `NUM_ITERATIONS` will make the program run longer, as
both programs finishes relatively quickly and there will be to much variance.

I write the results of the bash script to a csv file, and lastly I use a 
Python script using `pandas` and `matplotlib` to create a graph of the times
for ease of comparing.

I have included two tests, `10_5_results.csv` which is run using 
`TEST_ITERATIONS = 10`
`NUM_ITERATIONS = 5`

And the other is `10_15_results.csv` which is run using
`TEST_ITERATIONS = 10`
`NUM_ITERATIONS = 15`

## RESULTS

For the first test, in total it is run `10 * 5 = 50` times, and looking at the 
graph, it does look like the parallel program runs slightly faster but there
is so much variability that I cannot say this with confidence. With a lower 
number of test iterations, there is high overhead in getting everything setup
so the result here cannot be relied too much on

For the second test, it is run `10 * 15 = 150` times, and looking at the graph,
it is much clearer that the parallel program runs faster. Taking the average
run time, `pkgmain` runs in `15.020031079700004` whereas `pkgmain_parallel` runs
in `13.585047593799999`. 

This is a `15.020031079700004-13.585047593799999/15.020031079700004 = 0.1`
`10%` speedup which is less than the ideal `33%` speedup possibly due to
1. Overhead in managing multiple threads
2. Limited by speed of I/O operations
3. Fraction of code actually parallelised

A more realistic speedup will be calculating the proportion of code parallelized
and using Amdahl's law to calculate the optimal speedup.

# TREE
── config.cfg
├── high_performance
│   ├── 10_15.png
│   ├── 10_15_results.csv
│   ├── 10_5.png
│   ├── 10_5_results.csv
│   ├── benchmark1.bpkg
│   ├── benchmark1.data
│   ├── benchmark.sh
│   └── plot.py
├── include
│   ├── bytetide
│   │   └── btide.h
│   ├── chk
│   │   └── pkgchk.h
│   ├── config
│   │   └── config.h
│   ├── crypt
│   │   └── sha256.h
│   ├── net
│   │   └── packet.h
│   ├── package
│   │   └── package.h
│   ├── parser
│   │   └── parser.h
│   ├── peer
│   │   └── peer.h
│   └── tree
│       └── merkletree.h
├── Makefile
├── p1tests
│   ├── test1
│   │   ├── test1.bpkg
│   │   ├── test1.dat
│   │   └── test1.out
│   ├── test10
│   │   ├── test10.bpkg
│   │   ├── test10.dat
│   │   └── test10.out
│   ├── test11
│   │   ├── test11.bpkg
│   │   ├── test11.dat
│   │   └── test11.out
│   ├── test12
│   │   ├── test12.bpkg
│   │   ├── test12.dat
│   │   └── test12.out
│   ├── test13
│   │   ├── test13.bpkg
│   │   ├── test13.dat
│   │   └── test13.out
│   ├── test14
│   │   ├── test14.bpkg
│   │   ├── test14.dat
│   │   └── test14.out
│   ├── test2
│   │   ├── test2.bpkg
│   │   ├── test2.dat
│   │   └── test2.out
│   ├── test3
│   │   ├── test3.bpkg
│   │   ├── test3.dat
│   │   └── test3.out
│   ├── test4
│   │   ├── test4.bpkg
│   │   ├── test4.dat
│   │   └── test4.out
│   ├── test5
│   │   ├── test5.bpkg
│   │   ├── test5.dat
│   │   └── test5.out
│   ├── test6
│   │   ├── test6.bpkg
│   │   ├── test6.dat
│   │   └── test6.out
│   ├── test7
│   │   ├── test7.bpkg
│   │   ├── test7.dat
│   │   └── test7.out
│   ├── test8
│   │   ├── test8.bpkg
│   │   ├── test8.dat
│   │   └── test8.out
│   └── test9
│       ├── test9.bpkg
│       ├── test9.dat
│       └── test9.out
├── p1test.sh
├── p2tests
│   ├── test1
│   │   ├── file1.bpkg
│   │   ├── peer1.cfg
│   │   ├── peer2.cfg
│   │   ├── test1peer1.in
│   │   ├── test1peer1.out
│   │   ├── test1peer2.in
│   │   ├── test1peer2.out
│   │   ├── test1.py
│   │   └── tests
│   │       └── file1.data
│   └── test2
│       ├── file1.bpkg
│       ├── file4.bpkg
│       ├── peer1.cfg
│       ├── peer2.cfg
│       ├── peer3.cfg
│       ├── test2peer1.in
│       ├── test2peer1.out
│       ├── test2peer2.in
│       ├── test2peer2.out
│       ├── test2peer3.in
│       ├── test2peer3.out
│       ├── test2.py
│       └── tests
│           ├── file1.data
│           └── file4.data
├── p2test.sh
├── part1sel.txt
├── README.md
├── resources
│   ├── getting_started
│   │   ├── client.c
│   │   └── server.c
│   ├── pkgmake
│   ├── pkgs
│   │   ├── file1.bpkg
│   │   ├── file1.data
│   │   ├── file4.bpkg
│   │   └── file4.data
│   ├── pktval
│   └── sha256chk
└── src
    ├── btide.c
    ├── chk
    │   └── pkgchk.c
    ├── config.c
    ├── crypt
    │   └── sha256.c
    ├── package.c
    ├── parser.c
    ├── peer.c
    ├── pkgmain.c
    └── tree
        ├── merkletree.c
        └── merkletree_parallel.c