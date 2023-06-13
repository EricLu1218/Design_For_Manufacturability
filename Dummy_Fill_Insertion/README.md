# Dummy Fill Insertion
Propose your approach or implement an existing algorithm to solve the dummy fill insertion problem.

## How to Compile
In `Dummy_Fill_Insertion/src/`, enter the following command:
```
$ make
```
An executable file `Fill_Insertion` will be generated in `Dummy_Fill_Insertion/bin/`.

If you want to remove it, please enter the following command:
```
$ make clean
```

## How to Run
Usage:
```
$ ./Fill_Insertion <input file> <output file>
```

E.g.,
```
$ ./Fill_Insertion ../testcase/3.txt ../output/3.txt
```

## How to Test
In `Dummy_Fill_Insertion/src/`, enter the following command:
```
$ make test $name
```
It will build an executable file, test on testcase `$name`, and run a verifier to verify the answer.

E.g., test on testcase 3 and verify the answer.
```
$ make test 3
```
