Hexdiff: A hexadecimal differencing program
===========================================

Hexdiff is a terminal application for differencing two binary files. Its output
format is color-coded hexadecimal, inspired by `radiff2 -x`.

Features
--------
* Arbitrary offsets can be set for each input file
* A maximum compare length can be specified to limit the amount of compared data
* All matching lines can be printed to the terminal window, even when they form
  a large contiguous block of matching data.

Installation
------------
Hexdiff relies only on standard C libraries, with the color-coding performed by
ANSI escape sequences. It can be compiled with:

	gcc -o hexdiff hexdiff.c

Optimizations can be enabled during compilation, though they seem to lead to
minimal performance improvements.

Usage
-----
The user runs:

	hexdiff [-a] [-n max_len] file1 file2 [skip1 [skip2]]

with the command line arguments:
* `-a`: all lines should be printed
* `-n`: specify a maximum output length
* `skip1`: offset for `file1`
* `skip2`: offset for `file2`

