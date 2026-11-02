#include "app_main.h"
#include "sys_types.h"
#include "button.h"
#include "xpt2046.h"
#include "display_manager.h"
#include "mode_control.h"

/* [MUST] Rule 0.1: Application software layer
 * Strictly NO #include "stm32f1xx_hal.h" and NO direct register access.
 * Communicates exclusively through System Layer APIs and internal Application modules.
 * Zero float/double. Zero dynamic allocation. Local stack <= 64 bytes.
 */

static uint8_t s_device_states[4] = {0, 0, 0, 0}; /* 0: FAN, 1: LIGHT1, 2: LIGHT2, 3: DEHUM */

/* Test sensor values (DHT11/ADC/PIR simulated until sensor driver module attached) */
static int16_t s_test_temp_x10 = 265;   /* 26.5 C */
static uint16_t s_test_humi_x10 = 620;  /* 62.0 % */
static uint16_t s_test_light_adc = 1680;
static uint8_t s_test_pir = 0;

void app_init(void)
{
    /* 1. Initialize System Hardware drivers */
    button_init();
    display_manager_init();
    xpt2046_init();

    /* 2. Initialize Application logic */
    mode_control_init();

    /* 3. Render initial base UI */
    display_manager_draw_screen(mode_control_get(), s_device_states);
}

void app_loop(uint32_t current_tick_ms)
{
    static uint32_t last_btn_tick = 0;
    static uint32_t last_touch_tick = 0;
    static uint32_t last_sensor_tick = 0;
    static uint32_t touch_lockout_until = 0;
    static uint32_t touch_cooldown_until = 0;
    static uint8_t touch_consecutive_press = 0;
    static uint8_t touch_was_down = 0;

    /* 1. [MUST] Rule 2.2: Periodic Button Scan (10ms) - HIGHEST PRIORITY */
    if ((uint32_t)(current_tick_ms - last_btn_tick) >= 10) {
        last_btn_tick = current_tick_ms;
        button_scan_10ms();

        uint8_t any_physical_down = 0;

        for (uint8_t i = 0; i < BTN_COUNT; i++) {
            if (button_is_down((button_id_t)i)) {
                any_physical_down = 1;
            }

            if (button_has_event((button_id_t)i)) {
                /* Rule: Any user interaction (including physical button) forces MANUAL mode immediately */
                if (mode_control_handle_user_interaction()) {
                    display_manager_update_mode(MODE_MANUAL);
                }

                /* Physical button priority: toggle device state and force UI display */
                s_device_states[i] ^= 1;
                display_manager_update_device(i, s_device_states[i]);
                display_manager_show_physical_btn(i, s_device_states[i]);

                /* Cancel any pending touch state */
                touch_was_down = 0;
                touch_consecutive_press = 0;

                /* Lock out touch for 500ms so physical button takes full precedence */
                touch_lockout_until = current_tick_ms + 500;
            }
        }

        /* If any physical button is actively held down, continuously hold touch lockout */
        if (any_physical_down) {
            touch_lockout_until = current_tick_ms + 500;
        }
    }

    /* 2. Periodic Touch Processing (30ms non-blocking) - LOWER PRIORITY THAN PHYSICAL BUTTONS */
    if ((uint32_t)(current_tick_ms - last_touch_tick) >= 30) {
        last_touch_tick = current_tick_ms;

        /* Touch is only processed when physical button lockout has elapsed */
        if ((uint32_t)(current_tick_ms - touch_lockout_until) < 0x80000000U) {
            uint16_t tx = 0, ty = 0;

            if (xpt2046_read(&tx, &ty) == SYS_OK) {
                if (touch_consecutive_press < 255) {
                    touch_consecutive_press++;
                }

                ui_touch_target_t target = display_manager_hit_test(tx, ty);

                /* Update touch coordinates and recognized target on UI */
                display_manager_update_touch_pos(tx, ty, target);

                /* Resistive touch debounce:
                 * Trigger action only once on 2nd stable sample (60ms) and
                 * only when cooldown (350ms) has expired
                 */
                if (!touch_was_down && touch_consecutive_press >= 2) {
                    if ((uint32_t)(current_tick_ms - touch_cooldown_until) < 0x80000000U) {
                        touch_was_down = 1;
                        touch_cooldown_until = current_tick_ms + 350;

                        if (target == UI_TOUCH_MODE) {
                            /* Explicit Mode Button: toggles between AUTO and MANUAL */
                            mode_control_toggle();
                            display_manager_update_mode(mode_control_get());
                        } else if (target >= UI_TOUCH_FAN && target <= UI_TOUCH_DEHUM) {
                            /* Rule: Any user interaction on devices forces switch to MANUAL mode immediately */
                            if (mode_control_handle_user_interaction()) {
                                display_manager_update_mode(MODE_MANUAL);
                            }

                            uint8_t dev_id = (uint8_t)(target - UI_TOUCH_FAN);
                            s_device_states[dev_id] ^= 1;
                            display_manager_update_device(dev_id, s_device_states[dev_id]);
                        }
                    }
                }
            } else {
                /* Touch released - reset debounce counters */
                touch_consecutive_press = 0;
                touch_was_down = 0;
            }
        }
    }

    /* 3. Periodic Sensor UI Refresh (500ms as per Requirement_Spec.pdf) */
    if ((uint32_t)(current_tick_ms - last_sensor_tick) >= 500) {
        last_sensor_tick = current_tick_ms;
        display_manager_update_sensors(s_test_temp_x10, s_test_humi_x10, s_test_light_adc, s_test_pir);
    }
}
