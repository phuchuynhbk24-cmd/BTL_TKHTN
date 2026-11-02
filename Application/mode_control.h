#ifndef MODE_CONTROL_H
#define MODE_CONTROL_H

#include <stdint.h>

/* Operating modes as specified in Requirement_Spec.pdf */
typedef enum {
    MODE_MANUAL = 0,
    MODE_AUTO
} SystemMode_t;

void mode_control_init(void);
SystemMode_t mode_control_get(void);
void mode_control_set(SystemMode_t mode);
void mode_control_toggle(void);

/* Forces switch from AUTO to MANUAL on any user interaction (Physical Button or Touch).
 * Returns 1 if mode was AUTO and switched to MANUAL, 0 otherwise.
 */
uint8_t mode_control_handle_user_interaction(void);
uint8_t mode_control_handle_physical_button(void); /* Kept for backward compatibility */

#endif /* MODE_CONTROL_H */
