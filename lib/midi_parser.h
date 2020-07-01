//
//  midi_parser.h
//  max-external
//
//  Created by Malacalypse on 2020-04-05.
//

#ifndef midi_parser_h
#define midi_parser_h

#include <stdint.h>
#include <stdio.h>

typedef enum midi_state {
    kIDLE,
    kCC_NUM,
    kCC_VAL,
    kSYSEX_PENDING,
    kSYSEX_IGNORE,
    kSYSEX_ACQUIRE,
    // note and clock ignored for now.
} midi_state;

struct midi_parser;
typedef struct midi_parser midi_parser;

typedef void (*midi_cc_callback)(midi_parser *parser, uint8_t channel, uint8_t cc, uint8_t val);
typedef void (*midi_sysex_callback)(midi_parser *parser, uint8_t *sysex, size_t len);

typedef struct midi_parser {
    midi_state state;
    void *     context;  // can be used to point to a parent or whatever

    // Filter configuration
    uint8_t  listening_channel;
    uint8_t *sysex_preamble;
    uint8_t  sysex_preamble_len;

    // Data buffers
    uint8_t  cc_num;
    uint8_t *sysex_buffer;
    size_t   sysex_len;
    size_t   buffer_len;

    // Callbacks
    midi_cc_callback    cc_callback;
    midi_sysex_callback sysex_callback;
} midi_parser;

void         midi_parse(midi_parser *parser, uint8_t n);
void         midi_parser_filter_channel(midi_parser *parser, uint8_t channel);
void         midi_parser_filter_sysex(midi_parser *parser, uint8_t *expected_preamble, size_t preamble_len);
void         midi_parser_delete(midi_parser *parser);
midi_parser *midi_parser_new(void *context, midi_sysex_callback sysex_callback, midi_cc_callback cc_callback);

#endif /* midi_parser_h */
