#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "mode_control.h"
#include <stdint.h>

/* Display Manager:
 * Manages TFT ILI9341 contents, touch regions, partial redraws (anti-flicker).
 * RAM Budget: <= 500 Byte (Requirement_Spec.pdf page 9)
 */

typedef enum {
    UI_TOUCH_NONE = 0,
    UI_TOUCH_MODE,
    UI_TOUCH_FAN,
    UI_TOUCH_LIGHT1,
    UI_TOUCH_LIGHT2,
    UI_TOUCH_DEHUM
} ui_touch_target_t;

void display_manager_init(void);
void display_manager_draw_screen(SystemMode_t current_mode, const uint8_t device_states[4]);
void display_manager_update_device(uint8_t id, uint8_t state);
void display_manager_update_mode(SystemMode_t mode);
void display_manager_update_sensors(int16_t temp_x10, uint16_t humi_x10, uint16_t light_adc, uint8_t pir_state);
void display_manager_update_touch_pos(uint16_t tx, uint16_t ty, ui_touch_target_t target);
void display_manager_show_physical_btn(uint8_t btn_id, uint8_t state);
ui_touch_target_t display_manager_hit_test(uint16_t tx, uint16_t ty);

#endif /* DISPLAY_MANAGER_H */
