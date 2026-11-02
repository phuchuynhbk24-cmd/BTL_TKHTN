#include "ili9341.h"
#include "font5x7.h"
#include "stm32f1xx_hal.h"

/* Hardware Pin Definitions:
 * CS:   PA4
 * SCK:  PA5 (SPI1)
 * MOSI: PA7 (SPI1)
 * DC:   PB0
 * RST:  PB1
 * [MUST] Fast GPIO via direct register bit clear/set (BRR/BSRR = 1 CPU cycle)
 */
#define TFT_CS_LOW()    (GPIOA->BRR  = GPIO_PIN_4)
#define TFT_CS_HIGH()   (GPIOA->BSRR = GPIO_PIN_4)
#define TFT_DC_CMD()    (GPIOB->BRR  = GPIO_PIN_0)
#define TFT_DC_DATA()   (GPIOB->BSRR = GPIO_PIN_0)
#define TFT_RST_LOW()   (GPIOB->BRR  = GPIO_PIN_1)
#define TFT_RST_HIGH()  (GPIOB->BSRR = GPIO_PIN_1)

static SPI_HandleTypeDef s_hspi1;

static inline void spi1_write_byte(uint8_t byte)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    *((__IO uint8_t *)&SPI1->DR) = byte;
    while (SPI1->SR & SPI_SR_BSY);
}

static inline void tft_write_cmd(uint8_t cmd)
{
    TFT_DC_CMD();
    TFT_CS_LOW();
    spi1_write_byte(cmd);
    TFT_CS_HIGH();
}

static inline void tft_write_data(uint8_t data)
{
    TFT_DC_DATA();
    TFT_CS_LOW();
    spi1_write_byte(data);
    TFT_CS_HIGH();
}

void ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    tft_write_cmd(0x2A); /* Column Address Set */
    TFT_DC_DATA();
    TFT_CS_LOW();
    spi1_write_byte((uint8_t)(x0 >> 8));
    spi1_write_byte((uint8_t)(x0 & 0xFF));
    spi1_write_byte((uint8_t)(x1 >> 8));
    spi1_write_byte((uint8_t)(x1 & 0xFF));
    TFT_CS_HIGH();

    tft_write_cmd(0x2B); /* Page Address Set */
    TFT_DC_DATA();
    TFT_CS_LOW();
    spi1_write_byte((uint8_t)(y0 >> 8));
    spi1_write_byte((uint8_t)(y0 & 0xFF));
    spi1_write_byte((uint8_t)(y1 >> 8));
    spi1_write_byte((uint8_t)(y1 & 0xFF));
    TFT_CS_HIGH();

    tft_write_cmd(0x2C); /* Memory Write */
}

sys_status_t ili9341_init(void)
{
    /* 1. Clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* 2. Control GPIOs */
    GPIO_InitTypeDef gpio = {0};

    /* PA4: CS */
    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    TFT_CS_HIGH();

    /* PB0: DC, PB1: RST */
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    TFT_RST_HIGH();

    /* PA5 (SCK), PA7 (MOSI) */
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* 3. SPI1 Hardware Init: Prescaler 4 gives 18MHz @ 72MHz APB2 */
    s_hspi1.Instance = SPI1;
    s_hspi1.Init.Mode = SPI_MODE_MASTER;
    s_hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    s_hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    s_hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    s_hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    s_hspi1.Init.NSS = SPI_NSS_SOFT;
    s_hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    s_hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    s_hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    s_hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    if (HAL_SPI_Init(&s_hspi1) != HAL_OK) {
        return SYS_ERROR;
    }
    __HAL_SPI_ENABLE(&s_hspi1);

    /* 4. Hardware Reset */
    TFT_RST_LOW();
    HAL_Delay(20);
    TFT_RST_HIGH();
    HAL_Delay(100);

    /* 5. Software Reset & Wakeup */
    tft_write_cmd(0x01); /* SW Reset */
    HAL_Delay(100);
    tft_write_cmd(0x28); /* Display OFF */

    /* Power Control */
    tft_write_cmd(0xCB);
    tft_write_data(0x39); tft_write_data(0x2C); tft_write_data(0x00); tft_write_data(0x34); tft_write_data(0x02);

    tft_write_cmd(0xCF);
    tft_write_data(0x00); tft_write_data(0xC1); tft_write_data(0x30);

    tft_write_cmd(0xE8);
    tft_write_data(0x85); tft_write_data(0x00); tft_write_data(0x78);

    tft_write_cmd(0xEA);
    tft_write_data(0x00); tft_write_data(0x00);

    tft_write_cmd(0xED);
    tft_write_data(0x64); tft_write_data(0x03); tft_write_data(0x12); tft_write_data(0x81);

    tft_write_cmd(0xF7);
    tft_write_data(0x20);

    tft_write_cmd(0xC0); /* Power Control 1 */
    tft_write_data(0x23);

    tft_write_cmd(0xC1); /* Power Control 2 */
    tft_write_data(0x10);

    tft_write_cmd(0xC5); /* VCOM Control 1 */
    tft_write_data(0x3E); tft_write_data(0x28);

    tft_write_cmd(0xC7); /* VCOM Control 2 */
    tft_write_data(0x86);

    tft_write_cmd(0x36); /* Memory Access Control: Portrait 240x320 */
    tft_write_data(0x48); /* MX=1, BGR=1 */

    tft_write_cmd(0x3A); /* Pixel Format 16-bit RGB565 */
    tft_write_data(0x55);

    tft_write_cmd(0xB1); /* Frame Rate Control */
    tft_write_data(0x00); tft_write_data(0x18);

    tft_write_cmd(0xB6); /* Display Function Control */
    tft_write_data(0x08); tft_write_data(0x82); tft_write_data(0x27);

    tft_write_cmd(0x11); /* Sleep Out */
    HAL_Delay(120);

    tft_write_cmd(0x29); /* Display ON */
    HAL_Delay(20);

    ili9341_fill_screen(COLOR_BLACK);
    return SYS_OK;
}

void ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT) || (w == 0) || (h == 0)) return;
    if ((x + w) > ILI9341_WIDTH)  w = ILI9341_WIDTH - x;
    if ((y + h) > ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    ili9341_set_window(x, y, x + w - 1, y + h - 1);

    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);
    uint32_t total = (uint32_t)w * h;

    TFT_DC_DATA();
    TFT_CS_LOW();
    for (uint32_t i = 0; i < total; i++) {
        spi1_write_byte(hi);
        spi1_write_byte(lo);
    }
    TFT_CS_HIGH();
}

void ili9341_fill_screen(uint16_t color)
{
    ili9341_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT) return;
    ili9341_set_window(x, y, x, y);
    TFT_DC_DATA();
    TFT_CS_LOW();
    spi1_write_byte((uint8_t)(color >> 8));
    spi1_write_byte((uint8_t)(color & 0xFF));
    TFT_CS_HIGH();
}

void ili9341_draw_char(uint16_t x, uint16_t y, char ch, uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (ch < 32 || ch > 126) ch = ' ';
    uint8_t c_idx = (uint8_t)(ch - 32);

    if (scale == 1) {
        ili9341_set_window(x, y, x + 5, y + 7);
        TFT_DC_DATA();
        TFT_CS_LOW();
        for (uint8_t row = 0; row < 8; row++) {
            for (uint8_t col = 0; col < 6; col++) {
                uint16_t pixel = bg;
                if (col < 5 && (font5x7[c_idx][col] & (1 << row))) {
                    pixel = fg;
                }
                spi1_write_byte((uint8_t)(pixel >> 8));
                spi1_write_byte((uint8_t)(pixel & 0xFF));
            }
        }
        TFT_CS_HIGH();
    } else {
        for (uint8_t col = 0; col < 5; col++) {
            uint8_t line = font5x7[c_idx][col];
            for (uint8_t row = 0; row < 8; row++) {
                uint16_t pixel = (line & (1 << row)) ? fg : bg;
                ili9341_fill_rect(x + col * scale, y + row * scale, scale, scale, pixel);
            }
        }
        /* 1-column spacing */
        ili9341_fill_rect(x + 5 * scale, y, scale, 8 * scale, bg);
    }
}

void ili9341_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale)
{
    uint16_t cur_x = x;
    uint16_t cur_y = y;
    uint8_t char_w = 6 * scale;
    uint8_t char_h = 8 * scale;

    while (*str) {
        if (*str == '\n') {
            cur_x = x;
            cur_y += char_h + 2;
        } else {
            if (cur_x + char_w > ILI9341_WIDTH) {
                cur_x = x;
                cur_y += char_h + 2;
            }
            ili9341_draw_char(cur_x, cur_y, *str, fg, bg, scale);
            cur_x += char_w;
        }
        str++;
    }
}
