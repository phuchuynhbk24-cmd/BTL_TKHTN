#include "xpt2046.h"
#include "ili9341.h"
#include "stm32f1xx_hal.h"

/* Hardware Pin Definitions:
 * IRQ:  PB11 (Active-LOW)
 * CS:   PB12 (Active-LOW)
 * SCK:  PB13 (SPI2)
 * MISO: PB14 (SPI2)
 * MOSI: PB15 (SPI2)
 */
#define TOUCH_CS_LOW()     (GPIOB->BRR  = GPIO_PIN_12)
#define TOUCH_CS_HIGH()    (GPIOB->BSRR = GPIO_PIN_12)
#define TOUCH_IS_PRESSED() ((GPIOB->IDR & GPIO_PIN_11) == 0)

/* Default 12-bit calibration range */
#define CAL_X_MIN 200
#define CAL_X_MAX 3800
#define CAL_Y_MIN 200
#define CAL_Y_MAX 3800

#define CMD_READ_X 0xD0 /* 10110000: 12-bit, Differential, X position */
#define CMD_READ_Y 0x90 /* 10010000: 12-bit, Differential, Y position */

static SPI_HandleTypeDef s_hspi2;

static uint8_t spi2_transfer(uint8_t byte)
{
    while (!(SPI2->SR & SPI_SR_TXE));
    *((__IO uint8_t *)&SPI2->DR) = byte;
    while (!(SPI2->SR & SPI_SR_RXNE));
    return (uint8_t)(SPI2->DR);
}

static uint16_t xpt2046_read_raw(uint8_t cmd)
{
    TOUCH_CS_LOW();
    spi2_transfer(cmd);
    uint8_t b1 = spi2_transfer(0x00);
    uint8_t b2 = spi2_transfer(0x00);
    TOUCH_CS_HIGH();

    return (((uint16_t)b1 << 8) | b2) >> 3;
}

sys_status_t xpt2046_init(void)
{
    /* Clock configuration */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* PB12 (T_CS) */
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    TOUCH_CS_HIGH();

    /* PB11 (T_IRQ): Input with Pull-up */
    gpio.Pin = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* PB13 (SCK), PB15 (MOSI) */
    gpio.Pin = GPIO_PIN_13 | GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* PB14 (MISO) */
    gpio.Pin = GPIO_PIN_14;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* SPI2 Init: Prescaler 32 gives ~1.1MHz @ 36MHz APB1 */
    s_hspi2.Instance = SPI2;
    s_hspi2.Init.Mode = SPI_MODE_MASTER;
    s_hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    s_hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    s_hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    s_hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    s_hspi2.Init.NSS = SPI_NSS_SOFT;
    s_hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    s_hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    s_hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    s_hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    if (HAL_SPI_Init(&s_hspi2) != HAL_OK) {
        return SYS_ERROR;
    }
    __HAL_SPI_ENABLE(&s_hspi2);

    /* Wake up controller into power-down mode with IRQ enabled */
    TOUCH_CS_LOW();
    spi2_transfer(0x00);
    TOUCH_CS_HIGH();

    return SYS_OK;
}

sys_status_t xpt2046_read(uint16_t *x, uint16_t *y)
{
    if (!TOUCH_IS_PRESSED()) {
        return SYS_ERROR;
    }

    /* 4-sample averaging per requirement */
    uint32_t sum_x = 0;
    uint32_t sum_y = 0;
    const uint8_t SAMPLES = 4;

    for (uint8_t i = 0; i < SAMPLES; i++) {
        if (!TOUCH_IS_PRESSED()) return SYS_ERROR;
        sum_x += xpt2046_read_raw(CMD_READ_X);
        sum_y += xpt2046_read_raw(CMD_READ_Y);
    }

    uint16_t raw_x = (uint16_t)(sum_x / SAMPLES);
    uint16_t raw_y = (uint16_t)(sum_y / SAMPLES);

    if (raw_x < CAL_X_MIN || raw_x > CAL_X_MAX || raw_y < CAL_Y_MIN || raw_y > CAL_Y_MAX) {
        return SYS_ERROR;
    }

    /* [MUST] Rule 0.2: 100% integer mapping, no float allowed */
    int32_t px = (int32_t)(raw_x - CAL_X_MIN) * ILI9341_WIDTH / (CAL_X_MAX - CAL_X_MIN);
    int32_t py = (int32_t)(raw_y - CAL_Y_MIN) * ILI9341_HEIGHT / (CAL_Y_MAX - CAL_Y_MIN);

#if TOUCH_INVERT_X
    px = (ILI9341_WIDTH - 1) - px;
#endif

#if TOUCH_INVERT_Y
    py = (ILI9341_HEIGHT - 1) - py;
#endif

#if TOUCH_SWAP_XY
    int32_t tmp = px;
    px = py;
    py = tmp;
#endif

    if (px < 0) px = 0;
    if (px >= ILI9341_WIDTH) px = ILI9341_WIDTH - 1;
    if (py < 0) py = 0;
    if (py >= ILI9341_HEIGHT) py = ILI9341_HEIGHT - 1;

    *x = (uint16_t)px;
    *y = (uint16_t)py;
    return SYS_OK;
}
