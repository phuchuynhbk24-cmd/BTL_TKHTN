#include "mode_control.h"

/* Mode Control implementation:
 * Requirement_Spec.pdf:
 * - RAM: <= 20 bytes (Actual: 1 byte static)
 * - Execution time: <= 1ms
 * - Any user interaction in AUTO mode switches to MANUAL immediately.
 */
static SystemMode_t s_mode = MODE_MANUAL;

void mode_control_init(void)
{
    s_mode = MODE_MANUAL;
}

SystemMode_t mode_control_get(void)
{
    return s_mode;
}

void mode_control_set(SystemMode_t mode)
{
    s_mode = mode;
}

void mode_control_toggle(void)
{
    s_mode = (s_mode == MODE_MANUAL) ? MODE_AUTO : MODE_MANUAL;
}

uint8_t mode_control_handle_user_interaction(void)
{
    if (s_mode == MODE_AUTO) {
        s_mode = MODE_MANUAL;
        return 1; /* Mode changed, caller must update UI */
    }
    return 0;
}

uint8_t mode_control_handle_physical_button(void)
{
    return mode_control_handle_user_interaction();
}
