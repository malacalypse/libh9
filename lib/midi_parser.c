//
//  midi_parser.c
//  max-external
//
//  Created by Malacalypse on 2020-04-05.
//

#include "midi_parser.h"

#include <stdlib.h>
#include <string.h>

#include "debug.h"

#ifndef MAX_SYSEX_BUFFER_SIZE
#define MAX_SYSEX_BUFFER_SIZE  8192 // Adjust to taste. Some sample sysex can be MB.
#endif

#define INIT_SYSEX_BUFFER_SIZE 256 // Large enough for many implementations, will auto-scale.

static void midi_reset(midi_parser *parser);
static void midi_sysex_complete(midi_parser *parser);
static void midi_cc_complete(midi_parser *parser, uint8_t n);

midi_parser * midi_parser_new(void *context, midi_sysex_callback sysex_callback, midi_cc_callback cc_callback) {
    midi_parser *parser = malloc(sizeof(*parser));

    if (parser == NULL) {
        return NULL;
    }

    // Init a reasonable sysex buffer
    parser->buffer_len = INIT_SYSEX_BUFFER_SIZE;
    parser->sysex_buffer = malloc(sizeof(*parser->sysex_buffer) * parser->buffer_len);
    if (parser->sysex_buffer == NULL) {
        parser->buffer_len = 0;
    }

    // Clear the preamble buffer of any cruft
    parser->sysex_preamble_len = 0;
    parser->sysex_preamble = NULL;

    // Zero any important other variables
    parser->state = kIDLE;
    parser->sysex_len = 0;
    parser->listening_channel = 0;

    // Setup callbacks
    parser->sysex_callback = sysex_callback;
    parser->cc_callback = cc_callback;

    parser->context = context;

    return parser;
}

void midi_parser_filter_channel(midi_parser *parser, uint8_t channel) {
    if (channel <= 0xF) {
        parser->listening_channel = channel;
    }
}

void midi_parser_filter_sysex(midi_parser *parser, uint8_t *expected_preamble, size_t preamble_len) {
    if (preamble_len > parser->sysex_preamble_len) {
        uint8_t *new_preamble;
        if (parser->sysex_preamble != NULL) {
            new_preamble = realloc(parser->sysex_preamble, preamble_len);
        } else {
            new_preamble = malloc(preamble_len * sizeof(*parser->sysex_preamble));
        }
        if (new_preamble == NULL) {
            return;
        }
        parser->sysex_preamble = new_preamble;
    }
    memcpy(parser->sysex_preamble, expected_preamble, preamble_len);
    parser->sysex_preamble_len = preamble_len;
}

void midi_parser_delete(midi_parser *parser) {
    free(parser->sysex_preamble);
    free(parser->sysex_buffer);
}

void midi_parse(midi_parser *parser, uint8_t n) {
    switch (parser->state) {
        case kSYSEX_PENDING:
            debug_info("Midi Parser: kSYSEX_PENDING: Parsing %u\n", n);
            if ((n > 0x80) && (n <= 0xF7)) {
                // Transmission ended before we positively identified it was for us
                midi_reset(parser);
                return;
            } else if (n < 0x80) {
                // validate as we go
                if (parser->sysex_preamble[parser->sysex_len] == n) {
                    // we're still matching the preamble, so keep acquiring
                    parser->sysex_buffer[parser->sysex_len++] = n;
                    if (parser->sysex_len == parser->sysex_preamble_len) {
                        // We've matched the entire preamble, so it's intended for us
                        debug_info("Midi Parser: kSYSEX_PENDING->kSYSEX_ACQUIRE\n");
                        parser->state = kSYSEX_ACQUIRE;
                    }
                } else {
                    // We didn't match the preamble, so ignore the rest of this sysex
                    debug_info("Midi Parser: kSYSEX_PENDING->kSYSEX_IGNORE\n");
                    parser->state = kSYSEX_IGNORE;
                }
            } else {
                // Real-time status byte, we can safely ignore it.
            }
            break;
        case kSYSEX_ACQUIRE:
            debug_annoy("Midi Parser: kSYSEX_ACQUIRE: Parsing %u\n", n);
            if (n < 0x80) {
                // It is a data byte, add it to the pending sysex batch...
                parser->sysex_buffer[parser->sysex_len++] = n;
                // check for overrun
                if (parser->sysex_len >= parser->buffer_len) {
                    size_t new_len = parser->buffer_len * 2;
                    if (new_len > MAX_SYSEX_BUFFER_SIZE) {
                        new_len = MAX_SYSEX_BUFFER_SIZE;
                    }
                    if (new_len <= parser->buffer_len) {
                        // We can't make the buffer any bigger, so send on the current sysex and reset.
                        midi_sysex_complete(parser);
                        return;
                    }
                    // Otherwise, we can ask for more memory
                    uint8_t *new_buffer = realloc(parser->sysex_buffer, sizeof(*parser->sysex_buffer) * new_len);
                    if (new_buffer == NULL) {
                        // We didn't get it, so abort with what we've got
                        midi_sysex_complete(parser);
                        return;
                    }
                    parser->sysex_buffer = new_buffer;
                    parser->buffer_len = new_len;
                    return;
                }
            } else {
                // It is a status byte...
                if (n > 0xF7) {
                    // it is a realtime status byte, ignore. They're allowed in the middle of sysex.
                } else {
                    // per MIDI spec, any non-realtime status byte is valid for ending sysex
                    midi_sysex_complete(parser);
                    if (n < 0xF7) {
                        midi_parse(parser, n); // status byte still needs independent processing.
                    }
                }
            }
            break;
        case kSYSEX_IGNORE:
            debug_info("Midi Parser: kSYSEX_IGNORE: Parsing %u\n", n);
            // We're just waiting for the end of the transmission
            if ((n >= 0x80) && (n <= 0xF7)) {
                // we're done!
                midi_reset(parser);
                return;
            }
            break;
        case kCC_NUM:
            debug_info("Midi Parser: kCC_NUM: Parsing %u\n", n);
            if (n >= 0x80) {
                midi_reset(parser);
                midi_parse(parser, n);
                return;
            }
            parser->cc_num = n;
            debug_info("Midi Parser: kCC_NUM->kCC_VAL\n");
            parser->state = kCC_VAL;
            break;
        case kCC_VAL:
            debug_info("Midi Parser: kCC_VAL: Parsing %u\n", n);
            if (n >= 0x80) {
                midi_reset(parser);
                midi_parse(parser, n);
                return;
            }
            midi_cc_complete(parser, n);
            break;
        default: // should be kIDLE, but just in case...
            parser->state = kIDLE;
            debug_annoy("Midi Parser: kIDLE: Parsing %u\n", n);
            if (n == 0xF0) {
                if (parser->sysex_preamble_len > 0) {
                    debug_info("Midi Parser: kIDLE->kSYSEX_PENDING\n");
                    parser->state = kSYSEX_PENDING;
                } else {
                    debug_info("Midi Parser: kIDLE->kSYEX_ACQUIRE\n");
                    parser->state = kSYSEX_ACQUIRE;
                }
            } else if (n == (0xB0 + parser->listening_channel)) {
                debug_info("Midi Parser: kIDLE->kCC_NUM\n");
                parser->state = kCC_NUM;
            } else {
                debug_info("Midi Parser: %u not valid in current state, ignoring.\n", n);
                // currently unsupported right now.
            }
            break;
    }
}

static void midi_reset(midi_parser *parser) {
    debug_info("Midi Parser: midi_reset\n");
    parser->state = kIDLE;
    parser->sysex_len = 0;
}

static void midi_cc_complete(midi_parser *parser, uint8_t n) {
    debug_info("Midi Parser: midi_cc_complete\n");
    if (parser->cc_callback != NULL) {
        parser->cc_callback(parser, parser->listening_channel, parser->cc_num, n);
    }
    midi_reset(parser);
}

static void midi_sysex_complete(midi_parser *parser) {
    debug_info("Midi Parser: midi_sysex_complete\n");
    if (parser->sysex_callback != NULL) {
        parser->sysex_callback(parser, parser->sysex_buffer, parser->sysex_len);
    }
    midi_reset(parser);
}
