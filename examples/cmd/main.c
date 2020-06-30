//
//  main.c
//  h9
//
//  Created by Studio DC on 2020-06-28.
//

#include "main.h"

#include "h9.h"
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
