#ifndef BUTTON_H
#define BUTTON_H

#include "sys_types.h"
#include <stdint.h>

typedef enum {
    BTN_FAN = 0,
    BTN_LIGHT1,
    BTN_LIGHT2,
    BTN_DEHUMIDIFIER,
    BTN_COUNT
} button_id_t;

sys_status_t button_init(void);
void button_scan_10ms(void);
uint8_t button_has_event(button_id_t btn);
uint8_t button_is_down(button_id_t btn);

#endif /* BUTTON_H */
