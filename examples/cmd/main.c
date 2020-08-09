/*  main.c
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

#include "main.h"

#include "libh9.h"
#include "midi_parser.h"
#include "utils.h"

void sysex_found(midi_parser *parser, uint8_t* sysex, size_t len);

int main(int argc, char* argv[]) {
    h9* h9 = h9_new();
    midi_parser* mp = midi_parser_new(h9, sysex_found, NULL);

    if (argc == 2) {
        char* fname = argv[1];
        printf("Parsing Sysex from %s...\n", fname);
        FILE *fptr;
        fptr = fopen(fname, "r");
        if (fptr == NULL) {
            printf("Could not open %s, aborting.\n", fname);
            return 2;
        }
        char current_char;
        while ((current_char = getc(fptr)) != EOF) {
            midi_parse(mp, current_char);
        }
        printf("Completed parsing.\n");

        printf("Dumping loaded information back out:\n");
        uint8_t sysex[1000];
        size_t len = 0U;
        len = h9_dump(h9, sysex, 1000, true);
        if (len > 1000) {
            printf("Buffer too small, truncated!!\n");
        } else {
            char sysex_buffer[1000];
            size_t bytes_written = hexdump(sysex_buffer, 1000, sysex, len);
            printf("%zu bytes of Sysex generated:\n%*s\n", len, (int)bytes_written, sysex_buffer);
            if (bytes_written >= 1000) {
                printf("(Truncated)\n");
            }
        }
    } else {
        printf("Usage: %s <sysex filename>\n", argv[0]);
        return 1;
    }
    return 0;
}

void sysex_found(midi_parser *parser, uint8_t* sysex, size_t len) {
    h9_status status = h9_load(parser->context, sysex, len);
    if (status != kH9_OK) {
        printf("H9 Load failed: %d\n", status);
    }
    printf("Loaded preset %s\n", ((h9 *)parser->context)->preset->name);
}
