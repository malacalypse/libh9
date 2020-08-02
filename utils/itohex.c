/*  itohex.c
    This file is part of libh9, a library for remotely managing Eventide H9
    effects pedals.

    Copyright (C) 2020 Daniel Collins

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Usage: %s <infile> [<outfile>]\n", argv[0]);
        return 1;
    }

    char* infile = argv[1];
    FILE *inptr;
    if ((inptr = fopen(infile, "r")) == NULL) {
        printf("Could not open %s. Aborting.\n", infile);
    }

    FILE *outptr = NULL;
    if (argc == 3) {
        char* outfile = argv[2];
        if ((outptr = fopen(outfile, "w")) == NULL) {
            printf("Could not open %s. Aborting.\n", outfile);
        }
    }

    printf("Begin processing...\n");
    int i;
    while (fscanf(inptr, "%d", &i) == 1) {
       if (outptr != NULL) {
           fprintf(outptr, "%02x", i);
       }
       printf("%02x", i);
    }
    printf("\n");
    
    printf("Processing complete.\n");
}
