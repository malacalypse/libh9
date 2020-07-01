//
//  h9_sysex.h
//  h9
//
//  Created by Studio DC on 2020-06-30.
//

#ifndef h9_sysex_h
#define h9_sysex_h

#include "h9.h"

#define H9_SYSEX_EVENTIDE 0x1C
#define H9_SYSEX_H9       0x70

// Forward declarations
typedef struct h9 h9;

#ifdef __cplusplus
extern "C" {
#endif

// SYSEX Preset Operations

/*
 * Attempts to load the sysex as an H9 preset.
 * Validates:
 *    - that it is intended for an H9 (but does not check sysex id for THIS H9),
 *    - that the checksum is correct,
 *    - that the sysex is properly formatted,
 *    - that the values contained are reasonable,
 *    - that the indicated module and algorithm are supported by this software version.
 * If successful, the state of the h9 object is updated to reflect the settings in the preset.
 *
 * Return value indicates whether the operation was successful.
 */
h9_status h9_load(h9* h9, uint8_t* sysex, size_t len);

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

#ifdef __cplusplus
}
#endif

#endif /* h9_sysex_h */
