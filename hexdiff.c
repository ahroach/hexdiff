/*
 * hexdiff - hexadecimal differencing program
 *
 * Copyright 2018 Austin Roach <ahroach@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// ANSI escape sequences
char * ansi_green = "\x1B""[32m";
char * ansi_red = "\x1B""[31m";
char * ansi_reset = "\x1B""[0m";


void printicize(char * buf) {
	// Convert non-ASCII printable values to '.'
	for (int i = 0; i < 8; i++) {
		if ((buf[i] < 0x20) || (buf[i] > 0x7E)) {
			buf[i] = '.';
		}
	}
	return;
}


void print_same(char * buf1, char * buf2, unsigned long long int skip1,
                unsigned long long int skip2, unsigned long long int cnt)
{
	// Print the left side
	printf("%s0x%010llx  %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx ",
	       ansi_reset, skip1 + cnt, buf1[0], buf1[1], buf1[2], buf1[3],
	       buf1[4], buf1[5], buf1[6], buf1[7]);
	printicize(buf1);
	printf("%c%c%c%c%c%c%c%c    ", buf1[0], buf1[1], buf1[2], buf1[3],
	       buf1[4], buf1[5], buf1[6], buf1[7]);

	// Print the right side
	printf("0x%010llx  %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx ",
	       skip2 + cnt, buf2[0], buf2[1], buf2[2], buf2[3], buf2[4],
	       buf2[5], buf2[6], buf2[7]);
	printicize(buf2);
	printf("%c%c%c%c%c%c%c%c\n", buf2[0], buf1[1], buf2[2], buf2[3],
	       buf2[4], buf2[5], buf2[6], buf2[7]);
	return;
}


void print_diff(char * buf1, char * buf2, unsigned long long int skip1,
                unsigned long long int skip2, unsigned long long int cnt)
{
	char * color[8];
	
	// Assign escape sequences as appropriate for each byte
	// TODO: This whole thing would probably run faster if we didn't
	// have repeating escape codes for multiple characters of the
	// same color
	for (int i = 0; i < 8; i++) {
		color[i] = buf1[i] == buf2[i] ? ansi_green : ansi_red;
	}

	// Print the left side
	printf("%s0x%010llx  "
	       "%s%02hhx%s%02hhx%s%02hhx%s%02hhx"
	       "%s%02hhx%s%02hhx%s%02hhx%s%02hhx ",
	       ansi_red, skip1 + cnt, color[0], buf1[0], color[1], buf1[1],
	       color[2], buf1[2], color[3], buf1[3], color[4], buf1[4],
	       color[5], buf1[5], color[6], buf1[6], color[7], buf1[7]);
	printicize(buf1);
	printf("%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c    ", color[0], buf1[0],
	       color[1], buf1[1], color[2], buf1[2], color[3], buf1[3],
	       color[4], buf1[4], color[5], buf1[5], color[6], buf1[6],
	       color[7], buf1[7]);

	// Print the right side
	printf("%s0x%010llx  "
	       "%s%02hhx%s%02hhx%s%02hhx%s%02hhx"
	       "%s%02hhx%s%02hhx%s%02hhx%s%02hhx ",
	       ansi_red, skip2 + cnt, color[0], buf2[0], color[1], buf2[1],
	       color[2], buf2[2], color[3], buf2[3], color[4], buf2[4],
	       color[5], buf2[5], color[6], buf2[6], color[7], buf2[7]);
	printicize(buf2);
	printf("%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c\n", color[0], buf2[0],
	       color[1], buf2[1], color[2], buf2[2], color[3], buf2[3],
	       color[4], buf2[4], color[5], buf2[5], color[6], buf2[6],
	       color[7], buf2[7]);
	printf("%s", ansi_reset);
	return;
}


int main(int argc, char **argv)
{
	int opt, show_all, final;
	unsigned long long int max_len, skip1, skip2, cnt, eq_run;
	char *fname1, *fname2;
	FILE *file1, *file2;

	unsigned char buf1[8], buf2[8];

	// TODO: Trap SIGINT so that we always clean up with a 
	// reset character, and don't leave the terminal in red
	// or green mode

	// Parse the input arguments
	show_all = 0;
	max_len = 0;
	while ((opt = getopt(argc, argv, "an:")) != -1) {
		switch (opt) {
		case 'a':
			show_all = 1;
			break;
		case 'n':
			max_len = strtoull(optarg, 0, 0);
			break;
		default:
			fprintf(stderr,
			        "Usage: %s [-a] [-n max_len] file1 file2"
				" [skip1 [skip2]]\n",
				argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// Get the filenames and any skip values
	switch (argc - optind) {
	case 2:
		fname1 = argv[argc-2];
		fname2 = argv[argc-1];
		skip1 = 0;
		skip2 = 0;
		break;
	case 3:
		fname1 = argv[argc-3];
		fname2 = argv[argc-2];
		skip1 = strtoull(argv[argc-1], 0, 0);
		skip2 = 0;
		break;
	case 4:
		fname1 = argv[argc-4];
		fname2 = argv[argc-3];
		skip1 = strtoull(argv[argc-2], 0, 0);
		skip2 = strtoull(argv[argc-1], 0, 0);
		break;
	default:
		fprintf(stderr,
		        "Usage: %s [-a] [-n max_len] file1 file2"
			" [skip1 [skip2]]\n",
			argv[0]);
		exit(EXIT_FAILURE);
	}

	// Open the files and seek to the appropriate spots
	if ((file1 = fopen(fname1, "r")) == 0) {
		fprintf(stderr, "Error opening %s for reading.\n", fname1);
		exit(EXIT_FAILURE);
	}
	if ((file2 = fopen(fname2, "r")) == 0) {
		fprintf(stderr, "Error opening %s for reading.\n", fname2);
		fclose(file1);
		exit(EXIT_FAILURE);
	}

	if (fseeko(file1, skip1, 0) != 0) {
		fprintf(stderr,
		        "Error seeking to 0x%x in %s.\n", skip1, fname1);
		fclose(file1);
		fclose(file2);
		exit(EXIT_FAILURE);
	}

	if (fseeko(file2, skip2, 0) != 0) {
		fprintf(stderr,
		        "Error seeking to 0x%x in %s.\n", skip2, fname2);
		fclose(file1);
		fclose(file2);
		exit(EXIT_FAILURE);
	}

	// Print the header
	printf("%s   offset      0 1 2 3 4 5 6 7 01234567    "
	       "   offset      0 1 2 3 4 5 6 7 01234567\n",
	       ansi_reset);
	
	final = 0;
	cnt = 0;
	eq_run = 0;
	while ((final == 0) && ((cnt < max_len) || (max_len == 0))) {
		// If we fail to fill the buffer due to EOF, we want
		// the residual values to be 0
		memset(buf1, 0, 8);
		memset(buf2, 0, 8);

		// TODO: Probably less I/O overhead if we read more than
		// 8 bytes at a time. Or, threads...
		if (fread(buf1, 1, 8, file1) != 8) {
			final = 1;
		}
		if (fread(buf2, 1, 8, file2) != 8) {
			final = 1;
		}
		
		if (memcmp(buf1, buf2, 8) == 0) {
			if ((eq_run == 0) || (show_all == 1)) {
				print_same(buf1, buf2, skip1, skip2, cnt);	
			} else if (eq_run == 1) {
				printf("...\n");
			}
			eq_run++;
		} else {
			print_diff(buf1, buf2, skip1, skip2, cnt);
			eq_run = 0;
		}
	
		cnt += 8;
	}

	fclose(file1);
	fclose(file2);

	return 0;
}

