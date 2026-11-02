#ifndef ILI9341_H
#define ILI9341_H

#include "sys_types.h"
#include <stdint.h>

#define ILI9341_WIDTH   240
#define ILI9341_HEIGHT  320

/* 16-bit RGB565 Colors */
#define COLOR_BLACK          0x0000
#define COLOR_WHITE          0xFFFF
#define COLOR_RED            0xF800
#define COLOR_GREEN          0x07E0
#define COLOR_BLUE           0x001F
#define COLOR_YELLOW         0xFFE0
#define COLOR_CYAN           0x07FF
#define COLOR_MAGENTA        0xF81F
#define COLOR_GRAY           0x8410
#define COLOR_DARKGRAY       0x2124
#define COLOR_NAVY           0x000F
#define COLOR_DARKGREEN      0x03E0

/* Modern Professional Theme Colors */
#define COLOR_BG_DARK        0x0821  /* Deep slate velvet background */
#define COLOR_CARD_BG        0x18C3  /* Dark charcoal card body */
#define COLOR_CARD_BORDER    0x31A6  /* Subtle slate border */
#define COLOR_HEADER_BG      0x0A54  /* High-tech deep blue header */
#define COLOR_HEADER_LINE    0x047A  /* Header accent line */
#define COLOR_ACTIVE_BG      0x0344  /* Vibrant emerald green card body */
#define COLOR_ACTIVE_BORDER  0x2FE6  /* Bright neon green card border */
#define COLOR_ACCENT_AMBER   0xFD20  /* Warm amber for temperature */
#define COLOR_ACCENT_CYAN    0x07FF  /* Electric cyan for humidity */
#define COLOR_ACCENT_GOLD    0xFFE0  /* Gold for light sensor */
#define COLOR_ACCENT_PURPLE  0x915C  /* Electric purple for Auto mode */
#define COLOR_ACCENT_BLUE    0x2BDF  /* Cyan blue for Manual mode */
#define COLOR_MUTED_TEXT     0x7BEF  /* Slate gray muted text */

sys_status_t ili9341_init(void);
void ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ili9341_fill_screen(uint16_t color);
void ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void ili9341_draw_char(uint16_t x, uint16_t y, char ch, uint16_t fg, uint16_t bg, uint8_t scale);
void ili9341_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale);

#endif /* ILI9341_H */
