/*  h9_sysex.h
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

#ifndef h9_sysex_h
#define h9_sysex_h

#include "libh9.h"

#define H9_SYSEX_EVENTIDE 0x1C
#define H9_SYSEX_H9       0x70

// Forward declarations
typedef struct h9 h9;

typedef enum h9_enforce_sysex_id {
    kH9_RESTRICT_TO_SYSEX_ID = 0U,
    kH9_RESPOND_TO_ANY_SYSEX_ID,
} h9_enforce_sysex_id;

#ifdef __cplusplus
extern "C" {
#endif

// SYSEX Preset Operations

/*
 * Attempts to parse and apply the indicated sysex data.
 *
 * This data can be:
 *    - a preset (as either a command TO the pedal or as a dump FROM the pedal),
 *    - a sysvars dump FROM the pedal (in which case the appropriate system settings are updated),
 *    - a system value response for a single config value
 * Other values will, at present, be harmlessly ignored with the appropriate response status.
 *
 * Validates:
 *    - that it is intended for an H9
 *    - if enforce_sysex_id is set to kH9_RESTRICT_SYSEX_ID, that the message is intended for THIS H9
 *    - that the checksum is correct,
 *    - that the bit of sysex is of the appropriate type (supported types are value dump, sysvars dump, and preset)
 *    - that the sysex is properly formatted,
 *    - that the values contained are reasonable and supported by this software.
 * If successful, the state of the h9 object is updated to reflect the settings in in the sysex.
 *
 * Return value indicates whether the operation was successful.
 */
h9_status h9_parse_sysex(h9* h9, uint8_t* sysex, size_t len, h9_enforce_sysex_id enforce_sysex_id);

/*
 * Generates a complete sysex message encapsulating the specified h9 object's current state.
 * This sysex can be sent to an H9 with the matching sysex id to load into the working preset space
 *   (preset 0, regardless of the sysex-contained preset id) for immediate preview.
 *
 * Return value is length of the entire sysex blob, inclusive of 0xF0/0xF7 terminators.
 * Note 1: If this is >= max_len, truncation occurred and the sysex should be considered invalid.
 *         Regardless of update_sync_dirty, if truncation occurred, the dirty flag will NOT be updated.
 * Note 2: This is NOT a string. There is no guarantee of a NULL after the final 0xF7.
 */
size_t h9_dump(h9* h9, uint8_t* sysex, size_t max_len, bool update_dirty_flag);

// SYSEX Generation (syncing and device inquiry)

size_t h9_sysexGenRequestCurrentPreset(h9* h9, uint8_t* sysex, size_t max_len);
size_t h9_sysexGenRequestSystemConfig(h9* h9, uint8_t* sysex, size_t max_len);
size_t h9_sysexGenRequestConfigVar(h9* h9, uint16_t key, uint8_t* sysex, size_t max_len);
size_t h9_sysexGenWriteConfigVar(h9* h9, uint16_t key, uint16_t value, uint8_t* sysex, size_t max_len);

void h9_sysexRequestCurrentPreset(h9* h9);
void h9_sysexRequestSystemConfig(h9* h9);
void h9_sysexRequestConfigVar(h9* h9, uint16_t key);
void h9_sysexWriteConfigVar(h9* h9, uint16_t key, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* h9_sysex_h */
