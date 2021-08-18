Hexdiff: A hexadecimal differencing program
===========================================

Hexdiff is a terminal application for differencing two binary files. Its output
format is color-coded hexadecimal, inspired by `radiff2 -x`.

Features
--------
* Arbitrary offsets can be set for each input file.
* A maximum compare length can be specified to limit the amount of compared data.
* All matching lines can be printed to the terminal window, even when they form
  a large contiguous block of matching data.

Installation
------------
Hexdiff relies only on standard C libraries, with the color-coding performed by
ANSI escape sequences. It can be compiled with:

	gcc -o hexdiff hexdiff.c

or

    make

Optimizations can be enabled during compilation, though they seem to lead to
minimal performance improvements.

Usage
-----
The user runs:

	hexdiff [-ahds] [-n len] [-c num] file1 file2 [skip1 [skip2]]

with the command line arguments:
* `-a`: all lines should be printed
* `-h`: show help
* `-d`: dense output
* `-s`: skip same lines
* `-n`: maximum number of bytes to compare
* `-c`: num  number of bytes (columns)
* `-w len  force terminal width`
* `skip1`: offset for `file1`
* `skip2`: offset for `file2`

Tips
----

* Passing to `less -R` is recommened for scrollable output.
* Auto width when passing output to `head`, `grep`, `tail`, `less` use alias:
```
alias hexdiff="hexdiff -w $COLUMNS"
```
