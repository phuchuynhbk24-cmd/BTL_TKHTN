#include "display_manager.h"
#include "ili9341.h"

/* Display Manager implementation:
 * Modern high-contrast dark theme, large Scale-2 sensor typography,
 * seamless touch hit-testing, physical button override display,
 * and anti-flicker partial redraws.
 *
 * Rules:
 * - Rule 0.1: Layer isolation (no HAL/registers in Application layer)
 * - Rule 0.2: Zero float/double (100% fixed-point integer math)
 * - Rule 0.3: Zero stdio/snprintf/malloc overhead
 * - RAM: <= 500 bytes budget (Actual: ~50 bytes static)
 */

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} rect_t;

/* Coordinate Definitions */
static const rect_t s_mode_rect = {12, 32, 216, 32};
static const rect_t s_dev_rects[4] = {
    {12,   80, 104, 48}, /* FAN */
    {124,  80, 104, 48}, /* LIGHT 1 */
    {12,  134, 104, 48}, /* LIGHT 2 */
    {124, 134, 104, 48}  /* DEHUMIDIFIER */
};

/* Sensor Card Rectangles (2x2 Grid) */
static const rect_t s_sensor_rects[4] = {
    {12,  198, 104, 46}, /* TEMP */
    {124, 198, 104, 46}, /* HUMI */
    {12,  248, 104, 46}, /* LIGHT */
    {124, 248, 104, 46}  /* PIR */
};

static const char *s_dev_labels[4] = {"FAN", "LIGHT 1", "LIGHT 2", "DEHUM"};

/* State cache for partial redraw */
static uint8_t s_cached_states[4] = {0, 0, 0, 0};
static SystemMode_t s_cached_mode = MODE_MANUAL;
static int16_t s_cached_temp = -999;
static uint16_t s_cached_humi = 9999;
static uint16_t s_cached_light = 9999;
static uint8_t s_cached_pir = 0xFF;

/* Ultra-light integer formatting helpers (prevents dragging libc printf/malloc) */
static void format_fixed1(int16_t val_x10, char *buf)
{
    int16_t integer = val_x10 / 10;
    int16_t frac = val_x10 % 10;
    if (frac < 0) frac = -frac;
    uint8_t idx = 0;
    if (integer < 0) {
        buf[idx++] = '-';
        integer = -integer;
    }
    if (integer >= 100) {
        buf[idx++] = (char)('0' + (integer / 100));
        integer %= 100;
        buf[idx++] = (char)('0' + (integer / 10));
        integer %= 10;
    } else if (integer >= 10) {
        buf[idx++] = (char)('0' + (integer / 10));
        integer %= 10;
    }
    buf[idx++] = (char)('0' + integer);
    buf[idx++] = '.';
    buf[idx++] = (char)('0' + frac);
    buf[idx] = '\0';
}

static void format_u3(uint16_t val, char *buf)
{
    buf[0] = (char)('0' + ((val / 100) % 10));
    buf[1] = (char)('0' + ((val / 10) % 10));
    buf[2] = (char)('0' + (val % 10));
    buf[3] = '\0';
}

static void format_u4(uint16_t val, char *buf)
{
    buf[0] = (char)('0' + ((val / 1000) % 10));
    buf[1] = (char)('0' + ((val / 100) % 10));
    buf[2] = (char)('0' + ((val / 10) % 10));
    buf[3] = (char)('0' + (val % 10));
    buf[4] = '\0';
}

static inline uint8_t is_inside(uint16_t tx, uint16_t ty, const rect_t *r, int16_t pad)
{
    int32_t x_min = (int32_t)r->x - pad;
    int32_t x_max = (int32_t)(r->x + r->w) + pad;
    int32_t y_min = (int32_t)r->y - pad;
    int32_t y_max = (int32_t)(r->y + r->h) + pad;

    if (x_min < 0) x_min = 0;
    if (y_min < 0) y_min = 0;

    return ((int32_t)tx >= x_min && (int32_t)tx <= x_max &&
            (int32_t)ty >= y_min && (int32_t)ty <= y_max);
}

void display_manager_init(void)
{
    ili9341_init();
}

void display_manager_update_mode(SystemMode_t mode)
{
    s_cached_mode = mode;
    uint16_t border = (mode == MODE_AUTO) ? COLOR_ACCENT_PURPLE : COLOR_ACCENT_BLUE;
    uint16_t bg = COLOR_CARD_BG;

    ili9341_fill_rect(s_mode_rect.x, s_mode_rect.y, s_mode_rect.w, s_mode_rect.h, bg);
    /* 2px Accent Border */
    ili9341_fill_rect(s_mode_rect.x, s_mode_rect.y, s_mode_rect.w, 2, border);
    ili9341_fill_rect(s_mode_rect.x, s_mode_rect.y + s_mode_rect.h - 2, s_mode_rect.w, 2, border);
    ili9341_fill_rect(s_mode_rect.x, s_mode_rect.y, 2, s_mode_rect.h, border);
    ili9341_fill_rect(s_mode_rect.x + s_mode_rect.w - 2, s_mode_rect.y, 2, s_mode_rect.h, border);

    if (mode == MODE_AUTO) {
        ili9341_draw_string(s_mode_rect.x + 18, s_mode_rect.y + 11, "MODE: [ AUTO ]  (TOUCH TO SW)", COLOR_ACCENT_GOLD, bg, 1);
    } else {
        ili9341_draw_string(s_mode_rect.x + 14, s_mode_rect.y + 11, "MODE: [ MANUAL ] (TOUCH TO SW)", COLOR_WHITE, bg, 1);
    }
}

void display_manager_update_device(uint8_t id, uint8_t state)
{
    if (id >= 4) return;
    s_cached_states[id] = state;

    uint16_t x = s_dev_rects[id].x;
    uint16_t y = s_dev_rects[id].y;
    uint16_t w = s_dev_rects[id].w;
    uint16_t h = s_dev_rects[id].h;

    uint16_t bg = state ? COLOR_ACTIVE_BG : COLOR_CARD_BG;
    uint16_t border = state ? COLOR_ACTIVE_BORDER : COLOR_CARD_BORDER;

    ili9341_fill_rect(x, y, w, h, bg);
    ili9341_fill_rect(x, y, w, 2, border);
    ili9341_fill_rect(x, y + h - 2, w, 2, border);
    ili9341_fill_rect(x, y, 2, h, border);
    ili9341_fill_rect(x + w - 2, y, 2, h, border);

    /* Label in Scale 1 */
    uint16_t label_color = state ? COLOR_WHITE : COLOR_MUTED_TEXT;
    ili9341_draw_string(x + 10, y + 8, s_dev_labels[id], label_color, bg, 1);

    /* State in large Scale 2 */
    if (state) {
        ili9341_draw_string(x + 38, y + 24, "ON", COLOR_ACCENT_GOLD, bg, 2);
    } else {
        ili9341_draw_string(x + 32, y + 24, "OFF", COLOR_GRAY, bg, 2);
    }
}

void display_manager_draw_screen(SystemMode_t current_mode, const uint8_t device_states[4])
{
    /* 1. Dark velvet canvas */
    ili9341_fill_screen(COLOR_BG_DARK);

    /* 2. Top Header Bar */
    ili9341_fill_rect(0, 0, ILI9341_WIDTH, 26, COLOR_HEADER_BG);
    ili9341_fill_rect(0, 25, ILI9341_WIDTH, 2, COLOR_HEADER_LINE);
    ili9341_draw_string(22, 8, "SMART ROOM CONTROLLER", COLOR_WHITE, COLOR_HEADER_BG, 1);

    /* 3. Mode Toggle Button */
    display_manager_update_mode(current_mode);

    /* 4. Devices Section Header */
    ili9341_draw_string(12, 69, "DEVICE CONTROLS", COLOR_MUTED_TEXT, COLOR_BG_DARK, 1);
    ili9341_fill_rect(115, 72, 113, 1, COLOR_CARD_BORDER);

    /* 5. 4 Device Buttons */
    for (uint8_t i = 0; i < 4; i++) {
        display_manager_update_device(i, device_states[i]);
    }

    /* 6. Environment Section Header */
    ili9341_draw_string(12, 187, "ENVIRONMENT (500ms)", COLOR_MUTED_TEXT, COLOR_BG_DARK, 1);
    ili9341_fill_rect(138, 190, 90, 1, COLOR_CARD_BORDER);

    /* 7. Draw 4 Sensor Cards Background & Static Labels */
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t sx = s_sensor_rects[i].x;
        uint16_t sy = s_sensor_rects[i].y;
        uint16_t sw = s_sensor_rects[i].w;
        uint16_t sh = s_sensor_rects[i].h;

        ili9341_fill_rect(sx, sy, sw, sh, COLOR_CARD_BG);
        ili9341_fill_rect(sx, sy, sw, 1, COLOR_CARD_BORDER);
        ili9341_fill_rect(sx, sy + sh - 1, sw, 1, COLOR_CARD_BORDER);
        ili9341_fill_rect(sx, sy, 1, sh, COLOR_CARD_BORDER);
        ili9341_fill_rect(sx + sw - 1, sy, 1, sh, COLOR_CARD_BORDER);
    }

    /* Static Titles inside Sensor Cards */
    ili9341_draw_string(s_sensor_rects[0].x + 8, s_sensor_rects[0].y + 6, "TEMPERATURE", COLOR_ACCENT_AMBER, COLOR_CARD_BG, 1);
    ili9341_draw_string(s_sensor_rects[1].x + 8, s_sensor_rects[1].y + 6, "HUMIDITY", COLOR_ACCENT_CYAN, COLOR_CARD_BG, 1);
    ili9341_draw_string(s_sensor_rects[2].x + 8, s_sensor_rects[2].y + 6, "LIGHT ADC", COLOR_ACCENT_GOLD, COLOR_CARD_BG, 1);
    ili9341_draw_string(s_sensor_rects[3].x + 8, s_sensor_rects[3].y + 6, "MOTION (PIR)", COLOR_ACCENT_CYAN, COLOR_CARD_BG, 1);

    /* Initial placeholders */
    s_cached_temp = -999;
    s_cached_humi = 9999;
    s_cached_light = 9999;
    s_cached_pir = 0xFF;

    /* 8. Footer Status Bar */
    ili9341_fill_rect(0, 298, ILI9341_WIDTH, 1, COLOR_CARD_BORDER);
    ili9341_draw_string(12, 305, "Touch: X:--- Y:---  [---]", COLOR_MUTED_TEXT, COLOR_BG_DARK, 1);
}

void display_manager_update_sensors(int16_t temp_x10, uint16_t humi_x10, uint16_t light_adc, uint8_t pir_state)
{
    char num_buf[8];
    uint16_t card_bg = COLOR_CARD_BG;

    /* 1. Partial Redraw Temperature in LARGE Scale 2 */
    if (temp_x10 != s_cached_temp) {
        s_cached_temp = temp_x10;
        format_fixed1(temp_x10, num_buf);

        uint16_t vx = s_sensor_rects[0].x + 8;
        uint16_t vy = s_sensor_rects[0].y + 20;

        ili9341_fill_rect(vx, vy, 88, 18, card_bg);
        ili9341_draw_string(vx, vy, num_buf, COLOR_WHITE, card_bg, 2);
        ili9341_draw_string(vx + 58, vy, "C", COLOR_ACCENT_AMBER, card_bg, 2);
    }

    /* 2. Partial Redraw Humidity in LARGE Scale 2 */
    if (humi_x10 != s_cached_humi) {
        s_cached_humi = humi_x10;
        format_fixed1((int16_t)humi_x10, num_buf);

        uint16_t vx = s_sensor_rects[1].x + 8;
        uint16_t vy = s_sensor_rects[1].y + 20;

        ili9341_fill_rect(vx, vy, 88, 18, card_bg);
        ili9341_draw_string(vx, vy, num_buf, COLOR_WHITE, card_bg, 2);
        ili9341_draw_string(vx + 58, vy, "%", COLOR_ACCENT_CYAN, card_bg, 2);
    }

    /* 3. Partial Redraw Light ADC in LARGE Scale 2 */
    if (light_adc != s_cached_light) {
        s_cached_light = light_adc;
        format_u4(light_adc, num_buf);

        uint16_t vx = s_sensor_rects[2].x + 6;
        uint16_t vy = s_sensor_rects[2].y + 20;

        ili9341_fill_rect(vx, vy, 92, 18, card_bg);
        ili9341_draw_string(vx, vy, num_buf, COLOR_WHITE, card_bg, 2);

        const char *state_str = (light_adc < 1500) ? "DRK" : "BRT";
        ili9341_draw_string(vx + 58, vy + 4, state_str, COLOR_ACCENT_GOLD, card_bg, 1);
    }

    /* 4. Partial Redraw PIR in LARGE Scale 2 */
    if (pir_state != s_cached_pir) {
        s_cached_pir = pir_state;

        uint16_t vx = s_sensor_rects[3].x + 10;
        uint16_t vy = s_sensor_rects[3].y + 20;

        ili9341_fill_rect(vx, vy, 84, 18, card_bg);
        if (pir_state) {
            ili9341_draw_string(vx + 6, vy, "ALERT!", COLOR_RED, card_bg, 2);
        } else {
            ili9341_draw_string(vx + 6, vy, "CLEAR", COLOR_GREEN, card_bg, 2);
        }
    }
}

void display_manager_update_touch_pos(uint16_t tx, uint16_t ty, ui_touch_target_t target)
{
    char x_buf[4];
    char y_buf[4];
    format_u3(tx, x_buf);
    format_u3(ty, y_buf);

    ili9341_fill_rect(12, 305, 216, 10, COLOR_BG_DARK);

    ili9341_draw_string(12, 305, "Touch: X:", COLOR_MUTED_TEXT, COLOR_BG_DARK, 1);
    ili9341_draw_string(12 + 54, 305, x_buf, COLOR_ACCENT_GOLD, COLOR_BG_DARK, 1);
    ili9341_draw_string(12 + 76, 305, " Y:", COLOR_MUTED_TEXT, COLOR_BG_DARK, 1);
    ili9341_draw_string(12 + 96, 305, y_buf, COLOR_ACCENT_GOLD, COLOR_BG_DARK, 1);

    const char *target_str = "[---]";
    switch (target) {
    case UI_TOUCH_MODE:   target_str = "[MODE]"; break;
    case UI_TOUCH_FAN:    target_str = "[FAN]"; break;
    case UI_TOUCH_LIGHT1: target_str = "[LIGHT1]"; break;
    case UI_TOUCH_LIGHT2: target_str = "[LIGHT2]"; break;
    case UI_TOUCH_DEHUM:  target_str = "[DEHUM]"; break;
    default: break;
    }
    ili9341_draw_string(12 + 130, 305, target_str, (target != UI_TOUCH_NONE) ? COLOR_ACCENT_CYAN : COLOR_MUTED_TEXT, COLOR_BG_DARK, 1);
}

void display_manager_show_physical_btn(uint8_t btn_id, uint8_t state)
{
    if (btn_id >= 4) return;

    ili9341_fill_rect(12, 305, 216, 10, COLOR_BG_DARK);
    ili9341_draw_string(12, 305, "Phys BTN: ", COLOR_ACCENT_AMBER, COLOR_BG_DARK, 1);
    ili9341_draw_string(12 + 60, 305, s_dev_labels[btn_id], COLOR_WHITE, COLOR_BG_DARK, 1);
    ili9341_draw_string(12 + 115, 305, state ? "-> ON (PRIO)" : "-> OFF (PRIO)", state ? COLOR_GREEN : COLOR_GRAY, COLOR_BG_DARK, 1);
}

ui_touch_target_t display_manager_hit_test(uint16_t tx, uint16_t ty)
{
    const int16_t PAD = 6; /* Generous +6px margin for effortless finger touch */

    /* Mode button hitbox */
    if (is_inside(tx, ty, &s_mode_rect, PAD)) {
        return UI_TOUCH_MODE;
    }

    /* 4 Device buttons */
    for (uint8_t i = 0; i < 4; i++) {
        if (is_inside(tx, ty, &s_dev_rects[i], PAD)) {
            return (ui_touch_target_t)(UI_TOUCH_FAN + i);
        }
    }

    return UI_TOUCH_NONE;
}
