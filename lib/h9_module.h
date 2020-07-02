//
//  h9_module.h
//  h9
//
//  Created by Studio DC on 2020-07-01.
//

#ifndef h9_module_h
#define h9_module_h

//////////////////// Module Function Declarations
void       h9_reset_knobs(h9* h9);
void       h9_update_display_value(h9* h9, control_id control, control_value value);
h9_preset* h9_preset_new(void);

#endif /* h9_module_h */
