#Organisation of software

The source code is organised into serveral directories and files as followed:
- `/src`: Contains all the source code of the application.
  - `/chk`: Contains the code to load packages and deal with package commands
  - `/crypt`: Contains codes to compute data files into hashes
  - `/tree`: Contains code to initalise merkle tree and perform tree searches
  - `pkgmain.c`: 
- `include`: Contains all headers for the source code of the application.
  - `/chk`: Contains the header file for pkgchk functions
  - `/crypt`: Contains the crypt header file for sha256 functions
  - `config.h`: The header file for config source code
  - `package.h`: The header file for package source code
  - `peer.h`: The header file for peer source code.

The package source and header files were confused with packets so the package files are used for packet transmissions now

##Part One Tests

The tests for part one (merkle tree tests) are located in the folder 'part1_tests' and tests the merkle tree implementation through the functions 'all-hashes', 'hashes-of' and 'min-hashes.' File check and chunk check are not included because they do not use the merkle tree.

The script is located in the root of the project and is called `p1test.sh`.

To run merkle tree tests, type in the terminal at root of the project:
`bash p1test.sh`

It will output in the terminal each test from 1 to 8 with the description of the test and if it passed or not. 
e.g
"Test 1: Test 1 Description"
"Test 1: Passed!"

##Part Two Tests

The tests for part two (networking tests) are located in the folder 'part2_tests' which contains folders for each test. Inside each test, there are at least 3 files, a .in file for commands that the program inputs, .out file for generated output and .expected for expected output. How the tests work is that a python script is used to run two btide programs in parallel. The first btide program called 'client 1' (which is just called 'client' in the scripts) is the program where we are testing the outputs from and the program where the script inputs the commands into. This program uses the config file 'config1.cfg' file which contains the directory path to 'btide_test1' where it only contains 3 bpkg files and incomplete .data files. The second btide program called 'client 2' (which is just called 'server' in the script for simplicity but is just another client) is the client that is being interacted by the first program. This btide program uses the config file called 'config2.cfg' which holds the directory path 'btide_test2' that contains the same 3 bpkg files and all the completed .data files. 

btide_test1 folder contains incompleted .data files for client 1.
btide_test1 folder contains completed .data files for client 2.

In the test folders, each .out file is the output of client 1 and the commands in the .in files are passed into client 1.

The script is a python script using the subprocess library to run two programs in parallel and is located in the root of the project and is called `p2test.py`.

To run merkle tree tests, type in the terminal at root of the project:
`python3 p2test.py`

It will output in the terminal each test from 1 to 7 with the description of hte test and if it passed or not.
e.g
"Test 1: Test 1 Description"
"Test 1: Passed!"