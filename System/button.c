#include "button.h"
#include "stm32f1xx_hal.h"

/* Button Pin Definitions:
 * BTN1 (FAN):          PA3
 * BTN2 (LIGHT1):       PB2
 * BTN3 (LIGHT2):       PB3 (Requires AFIO REMAP SWJ_NOJTAG)
 * BTN4 (DEHUMIDIFIER): PB4 (Requires AFIO REMAP SWJ_NOJTAG)
 */
#define BTN_FAN_PORT       GPIOA
#define BTN_FAN_PIN        GPIO_PIN_3

#define BTN_LIGHT1_PORT    GPIOB
#define BTN_LIGHT1_PIN     GPIO_PIN_2

#define BTN_LIGHT2_PORT    GPIOB
#define BTN_LIGHT2_PIN     GPIO_PIN_3

#define BTN_DEHUM_PORT     GPIOB
#define BTN_DEHUM_PIN      GPIO_PIN_4

#define DEBOUNCE_THRESHOLD 2 /* 2 * 10ms = 20ms debounce */

static uint8_t s_stable_state[BTN_COUNT] = {1, 1, 1, 1}; /* 1 = Released (Active-LOW) */
static uint8_t s_debounce_cnt[BTN_COUNT] = {0, 0, 0, 0};
static uint8_t s_event_pressed[BTN_COUNT] = {0, 0, 0, 0};

static uint8_t button_read_raw(button_id_t btn)
{
    switch (btn) {
    case BTN_FAN:
        return (BTN_FAN_PORT->IDR & BTN_FAN_PIN) ? 1 : 0;
    case BTN_LIGHT1:
        return (BTN_LIGHT1_PORT->IDR & BTN_LIGHT1_PIN) ? 1 : 0;
    case BTN_LIGHT2:
        return (BTN_LIGHT2_PORT->IDR & BTN_LIGHT2_PIN) ? 1 : 0;
    case BTN_DEHUMIDIFIER:
        return (BTN_DEHUM_PORT->IDR & BTN_DEHUM_PIN) ? 1 : 0;
    default:
        return 1;
    }
}

sys_status_t button_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* [MUST] Rule 1.2: Protect SWD (PA13/PA14). Only remap SWJ_NOJTAG to free PB3/PB4 */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    /* Configure PA3 */
    gpio.Pin = BTN_FAN_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BTN_FAN_PORT, &gpio);

    /* Configure PB2, PB3, PB4 */
    gpio.Pin = BTN_LIGHT1_PIN | BTN_LIGHT2_PIN | BTN_DEHUM_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);

    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        s_stable_state[i] = button_read_raw((button_id_t)i);
        s_debounce_cnt[i] = 0;
        s_event_pressed[i] = 0;
    }

    return SYS_OK;
}

void button_scan_10ms(void)
{
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        uint8_t raw = button_read_raw((button_id_t)i);
        if (raw != s_stable_state[i]) {
            if (++s_debounce_cnt[i] >= DEBOUNCE_THRESHOLD) {
                s_stable_state[i] = raw;
                s_debounce_cnt[i] = 0;
                /* Active-LOW: Transition from 1 (Released) to 0 (Pressed) */
                if (s_stable_state[i] == 0) {
                    s_event_pressed[i] = 1;
                }
            }
        } else {
            s_debounce_cnt[i] = 0;
        }
    }
}

uint8_t button_has_event(button_id_t btn)
{
    if (btn >= BTN_COUNT) return 0;
    if (s_event_pressed[btn]) {
        s_event_pressed[btn] = 0;
        return 1;
    }
    return 0;
}

uint8_t button_is_down(button_id_t btn)
{
    if (btn >= BTN_COUNT) return 0;
    return (s_stable_state[btn] == 0) ? 1 : 0;
}
