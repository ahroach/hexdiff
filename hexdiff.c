/*
 * hexdiff - hexadecimal differencing program
 *
 * Copyright 2018 Austin Roach <ahroach@gmail.com>
 * Copyright 2021 Radomír Polách <rp@t4d.cz>
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
 */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>

#define BUFFER_SIZE 256

// ANSI escape sequences
static const char ansi_green[] = "\x1B""[32m";
static const char ansi_red[] = "\x1B""[31m";
static const char ansi_reset[] = "\x1B""[0m";
static const char empty_str[] = "";


static int sigint_recv = 0;

static void sigint_handler(int signum)
{
    sigint_recv = 1;
}


static void show_help(char **argv, int verbose)
{
    fprintf(stderr,
            "Usage: %s [-ahds] [-n len] [-c num] [-w len] file1 file2 [skip1 [skip2]]\n",
            argv[0]);
    if (verbose) {
        printf(" -a      print all lines\n"
               " -h      show help\n"
               " -d      dense output\n"
               " -s      skip same lines\n"
               " -n len  maximum number of bytes to compare\n"
               " -c num  number of bytes (columns)\n"
               " -w len  force terminal width\n"
               " skip1   starting offset for file1\n"
               " skip2   starting offset for file2\n");
    }
    exit(EXIT_FAILURE);
}


static void printicize(uint8_t * buf, unsigned long long n)
{
    // Convert non-ASCII printable values to '.'
    for (int i = 0; i < n; i++) {
        if ((buf[i] < 0x20) || (buf[i] > 0x7e)) {
            buf[i] = '.';
        }
    }
}


static void print_same(uint8_t *buf1, uint8_t *buf2, unsigned long long n, unsigned long long skip1, unsigned long long skip2, unsigned long long cnt, int dense)
{
    // Print the left side
    printf("%s0x%010llx  ", ansi_reset, skip1 + cnt);
    for (int i=0;i<n;++i) {
        printf("%02hhx", buf1[i]);
        if (!dense) printf(" ");
    }
    printf(" ");
    printicize(buf1, n);
    for (int i=0;i<n;++i) printf("%c", buf1[i]);
    printf("    ");

    // Print the right side
    printf("0x%010llx  ", skip2 + cnt);
    for (int i=0;i<n;++i) {
        printf("%02hhx", buf2[i]);
        if (!dense) printf(" ");
    }
    printf(" ");
    printicize(buf2, n);
    for (int i=0;i<n;++i) printf("%c", buf2[i]);
}


static void print_diff(uint8_t *buf1, uint8_t *buf2, unsigned long long n, unsigned long long skip1, unsigned long long skip2, unsigned long long cnt, int dense)
{
    const char *color[BUFFER_SIZE];
    const char *color_last;

    // Assign escape sequences as appropriate for each byte
    for (int i = 0; i < n; i++) {
        color[i] = buf1[i] == buf2[i] ? ansi_green : ansi_red;
    }

    // Remove many redundant escape sequences
    color_last = color[0];

    if ((color[0] == ansi_red) && (color[7] == ansi_red)) {
        // Beginning of each section is preceded by the address
        // (always red), or by the last element of a preceding
        // section. As long as the beginning and ending elements are
        // both red, we can get rid of the escape sequence at the
        // beginning of the section.
        color[0] = empty_str;
    }

    for (int i = 1; i < n; i++) {
        if (color[i] == color_last) {
            color[i] = empty_str;
        } else {
            color_last = color[i];
        }
    }

    // Print the left side
    printf("%s0x%010llx  ", ansi_red, skip1 + cnt);
    for (int i=0;i<n;++i) {
        printf("%s%02hhx", color[i], buf1[i]);
        if (!dense) printf(" ");
    }
    printf(" ");
    printicize(buf1, n);
    for (int i=0;i<n;++i) printf("%s%c", color[i], buf1[i]);
    printf("    ");

    // Print the right side
    printf("%s0x%010llx  ", ansi_red, skip2 + cnt);
    for (int i=0;i<n;++i) {
        printf("%s%02hhx", color[i], buf2[i]);
        if (!dense) printf(" ");
    }
    printf(" ");
    printicize(buf2, n);
    for (int i=0;i<n;++i) printf("%s%c", color[i], buf2[i]);
    printf("%s", ansi_reset);
}


int fit_bytes(int width, int dense) {
    return ((width/2*2) - (12+2+1)*2 - 3)/2/(3+!dense);
}

int main(int argc, char **argv)
{
    int opt, show_all = 0, input_end, bytes_w = 16, dense = 0, skip_same = 0, print_w = 0, size_w = 0;
    unsigned long long max_len = 0, skip1, skip2, cnt, eq_run;
    char *fname1, *fname2;
    FILE *file1, *file2;
    struct sigaction sigint_action;
    uint8_t buf1[BUFFER_SIZE], buf2[BUFFER_SIZE];
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    size_w = w.ws_col;

    // Parse the input arguments
    while ((opt = getopt(argc, argv, "ahdsw:c:n:")) != -1) {
        switch (opt) {
        case 'a':
            show_all = 1;
            break;
        case 'h':
            show_help(argv, 1);
            break;
        case 'd':
            dense = 1;
            break;
        case 's':
            skip_same = 1;
            break;
        case 'c':
            size_w = 0;
            bytes_w = strtoull(optarg, NULL, 0);
            if (bytes_w < 1) bytes_w = 16;
            if (bytes_w > 256) bytes_w = 256;
            break;
        case 'w':
            size_w = strtoull(optarg, NULL, 0);
            break;
        case 'n':
            max_len = strtoull(optarg, NULL, 0);
            break;
        default:
            show_help(argv, 0);
        }
    }

    if (size_w) {
        bytes_w = fit_bytes(size_w, dense);
        if (bytes_w < 1) bytes_w = 1;
    }

    // Get the filenames and any skip values
    if ((argc - optind) < 2) show_help(argv, 0);
    fname1 = argv[optind++];
    fname2 = argv[optind++];
    skip1 = (optind < argc) ? strtoull(argv[optind++], NULL, 0) : 0;
    skip2 = (optind < argc) ? strtoull(argv[optind++], NULL, 0) : 0;
    if (optind < argc) show_help(argv, 0); //Leftover arguments

    // Open the files and seek to the appropriate spots
    if ((file1 = fopen(fname1, "r")) == NULL) {
        fprintf(stderr, "fopen: %s: %s\n", fname1, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if ((file2 = fopen(fname2, "r")) == NULL) {
        fprintf(stderr, "fopen: %s: %s\n", fname2, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fseeko(file1, skip1, 0) != 0) {
        fprintf(stderr,
                "fseek to 0x%llx in %s: %s\n", skip1, fname1,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (fseeko(file2, skip2, 0) != 0) {
        fprintf(stderr,
                "fseek to 0x%llx in %s: %s\n", skip2, fname2,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set up signal handler for SIGINT
    sigint_action.sa_handler = sigint_handler;
    sigaction(SIGINT, &sigint_action, NULL);

    input_end = 0;
    cnt = 0;
    eq_run = 0;
    while ((input_end == 0) && ((cnt < max_len) || (max_len == 0)) &&
           (sigint_recv == 0)) {
        // If we fail to fill the buffer due to EOF, we want
        // the residual values to be 0
        memset(buf1, 0, bytes_w);
        memset(buf2, 0, bytes_w);

        // TODO: Probably less I/O overhead if we read more than
        // 8 bytes at a time. Or, threads...
        if (fread(buf1, 1, bytes_w, file1) != bytes_w) {
            input_end = 1;
        }
        if (fread(buf2, 1, bytes_w, file2) != bytes_w) {
            input_end = 1;
        }

        if (memcmp(buf1, buf2, bytes_w) == 0) {
            if ((!skip_same && eq_run == 0) || show_all == 1) {
                print_same(buf1, buf2, bytes_w, skip1, skip2, cnt, dense);
                printf("\n");
            } else if (eq_run == 1) {
                printf("...\n");
            }
            eq_run++;
        } else {
            print_diff(buf1, buf2, bytes_w, skip1, skip2, cnt, dense);
            printf("\n");
            eq_run = 0;
        }

        cnt += bytes_w;
    }

    fclose(file1);
    fclose(file2);

    return EXIT_SUCCESS;
}

// vim: ts=2 fdm=marker syntax=c expandtab sw=2
