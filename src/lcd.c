#include "lcd.h"
#include "board.h"
#include "delay.h"

/* Круглый GC9A01 240x240 по SPI2. Кадр RGB565 уходит сразу в панель:
   буфер 240*240*2 не помещается в 20 КБ RAM.
   Пины взяты из прежнего SPI-драйвера: SCK PB13, MOSI PB15, RES PB12, DC PA8, CS PB3. */

#define PIN_CS_PORT GPIOB
#define PIN_CS 3u
#define PIN_DC_PORT GPIOA
#define PIN_DC 8u
#define PIN_RST_PORT GPIOB
#define PIN_RST 12u

static int stream_wide;

static void spi_frame(int wide) {
    if (stream_wide == wide) {
        return;
    }
    while (SPI2->SR & SPI_SR_BSY) {
    }
    SPI2->CR1 &= (uint16_t)~SPI_CR1_SPE;
    if (wide) {
        SPI2->CR1 |= SPI_CR1_DFF;
    } else {
        SPI2->CR1 &= (uint16_t)~SPI_CR1_DFF;
    }
    SPI2->CR1 |= SPI_CR1_SPE;
    stream_wide = wide;
}

static void cs_low(void) { gpio_reset(PIN_CS_PORT, PIN_CS); }
static void cs_high(void) { gpio_set(PIN_CS_PORT, PIN_CS); }
static void dc_cmd(void) { gpio_reset(PIN_DC_PORT, PIN_DC); }
static void dc_data(void) { gpio_set(PIN_DC_PORT, PIN_DC); }

static void spi_wait(void) {
    while ((SPI2->SR & SPI_SR_TXE) == 0u) {
    }
    while (SPI2->SR & SPI_SR_BSY) {
    }
}

static void lcd_cmd(uint8_t command) {
    spi_frame(0);
    dc_cmd();
    cs_low();
    while ((SPI2->SR & SPI_SR_TXE) == 0u) {
    }
    SPI2_DR8 = command;
    spi_wait();
    cs_high();
}

static void lcd_byte(uint8_t data) {
    spi_frame(0);
    dc_data();
    cs_low();
    while ((SPI2->SR & SPI_SR_TXE) == 0u) {
    }
    SPI2_DR8 = data;
    spi_wait();
    cs_high();
}

static void lcd_cmd1(uint8_t command, uint8_t data) {
    lcd_cmd(command);
    lcd_byte(data);
}

static void lcd_cmdn(uint8_t command, const uint8_t *data, int count) {
    lcd_cmd(command);
    while (count--) {
        lcd_byte(*data++);
    }
}

void lcd_io_init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_SPI2;
    gpio_mode(GPIOB, 13u, GPIO_AF_PP50);
    gpio_mode(GPIOB, 15u, GPIO_AF_PP50);
    gpio_mode(PIN_RST_PORT, PIN_RST, GPIO_OUT_PP50);
    gpio_mode(PIN_DC_PORT, PIN_DC, GPIO_OUT_PP50);
    gpio_mode(PIN_CS_PORT, PIN_CS, GPIO_OUT_PP50);
    cs_high();
    dc_cmd();
    gpio_reset(PIN_RST_PORT, PIN_RST);
#if BOARD_HAS_DISP_EN
    gpio_mode(BOARD_DISP_EN_PORT, BOARD_DISP_EN_PIN, GPIO_OUT_PP50);
    gpio_reset(BOARD_DISP_EN_PORT, BOARD_DISP_EN_PIN);
#endif
    /* Только передача: приём выключен, иначе невычитанный RXNE поднимает OVR. */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
        | (uint16_t)(LCD_SPI_BR << SPI_CR1_BR_SHIFT)
        | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
    SPI2->CR1 |= SPI_CR1_SPE;
    stream_wide = 0;
}

static void panel_wake_power(void) {
#if BOARD_HAS_DISP_EN
    gpio_set(BOARD_DISP_EN_PORT, BOARD_DISP_EN_PIN);
    delay_ms(10u);
#endif
    gpio_reset(PIN_RST_PORT, PIN_RST);
    delay_ms(20u);
    gpio_set(PIN_RST_PORT, PIN_RST);
    delay_ms(120u);
}

void lcd_panel_init(void) {
    static const uint8_t gamma_f0[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    static const uint8_t gamma_f1[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    static const uint8_t reg70[] = {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03};
    static const uint8_t reg62[] = {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70};
    static const uint8_t reg63[] = {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70};
    static const uint8_t reg64[] = {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07};
    static const uint8_t reg66[] = {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00};
    static const uint8_t reg67[] = {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98};
    static const uint8_t reg74[] = {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00};
    static const uint8_t reg90[] = {0x08, 0x08, 0x08, 0x08};
    static const uint8_t regff[] = {0x60, 0x01, 0x04};
    static const uint8_t regdf[] = {0x21, 0x0C, 0x02};

    panel_wake_power();
    /* Заводская последовательность GC9A01. Без блока гаммы клоны панелей остаются чёрными. */
    lcd_cmd(0xEF);
    lcd_cmd1(0xEB, 0x14);
    lcd_cmd(0xFE);
    lcd_cmd(0xEF);
    lcd_cmd1(0xEB, 0x14);
    lcd_cmd1(0x84, 0x40);
    lcd_cmd1(0x85, 0xFF);
    lcd_cmd1(0x86, 0xFF);
    lcd_cmd1(0x87, 0xFF);
    lcd_cmd1(0x88, 0x0A);
    lcd_cmd1(0x89, 0x21);
    lcd_cmd1(0x8A, 0x00);
    lcd_cmd1(0x8B, 0x80);
    lcd_cmd1(0x8C, 0x01);
    lcd_cmd1(0x8D, 0x01);
    lcd_cmd1(0x8E, 0xFF);
    lcd_cmd1(0x8F, 0xFF);
    lcd_cmd(0xB6);
    lcd_byte(0x00);
    lcd_byte(0x20);
    lcd_cmd1(0x3A, 0x05);              /* RGB565 */
    lcd_cmd1(0x36, LCD_MADCTL);
    lcd_cmdn(0x90, reg90, 4);
    lcd_cmd1(0xBD, 0x06);
    lcd_cmd1(0xBC, 0x00);
    lcd_cmdn(0xFF, regff, 3);
    lcd_cmd1(0xC3, 0x13);
    lcd_cmd1(0xC4, 0x13);
    lcd_cmd1(0xC9, 0x22);
    lcd_cmd1(0xBE, 0x11);
    lcd_cmd(0xE1);
    lcd_byte(0x10);
    lcd_byte(0x0E);
    lcd_cmdn(0xDF, regdf, 3);
    lcd_cmdn(0xF0, gamma_f0, 6);
    lcd_cmdn(0xF1, gamma_f1, 6);
    lcd_cmdn(0xF2, gamma_f0, 6);
    lcd_cmdn(0xF3, gamma_f1, 6);
    lcd_cmd(0xED);
    lcd_byte(0x1B);
    lcd_byte(0x0B);
    lcd_cmd1(0xAE, 0x77);
    lcd_cmd1(0xCD, 0x63);
    lcd_cmdn(0x70, reg70, 9);
    lcd_cmd1(0xE8, 0x34);
    lcd_cmdn(0x62, reg62, 12);
    lcd_cmdn(0x63, reg63, 12);
    lcd_cmdn(0x64, reg64, 7);
    lcd_cmdn(0x66, reg66, 10);
    lcd_cmdn(0x67, reg67, 10);
    lcd_cmdn(0x74, reg74, 7);
    lcd_cmd(0x98);
    lcd_byte(0x3E);
    lcd_byte(0x07);
    lcd_cmd(0x35);
    lcd_cmd(LCD_INVERT ? 0x21 : 0x20);
    lcd_cmd(0x11);
    delay_ms(120u);
    lcd_cmd(0x29);
    delay_ms(20u);
}

void lcd_sleep(void) {
    lcd_cmd(0x28);
    delay_ms(5u);
    lcd_cmd(0x10);
    delay_ms(20u);
#if BOARD_HAS_DISP_EN
    gpio_reset(BOARD_DISP_EN_PORT, BOARD_DISP_EN_PIN);
#endif
}

void lcd_stream_begin(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) {
        return;
    }
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    lcd_cmd(0x2A);
    lcd_byte((uint8_t)(x >> 8));
    lcd_byte((uint8_t)x);
    lcd_byte((uint8_t)(x1 >> 8));
    lcd_byte((uint8_t)x1);
    lcd_cmd(0x2B);
    lcd_byte((uint8_t)(y >> 8));
    lcd_byte((uint8_t)y);
    lcd_byte((uint8_t)(y1 >> 8));
    lcd_byte((uint8_t)y1);
    lcd_cmd(0x2C);
    /* Ширину кадра меняем при снятом CS, иначе панель увидит обрыв такта. */
    spi_frame(1);
    dc_data();
    cs_low();
}

void lcd_stream_push(uint16_t color) {
    while ((SPI2->SR & SPI_SR_TXE) == 0u) {
    }
    SPI2->DR = color;
}

void lcd_stream_end(void) {
    spi_wait();
    cs_high();
    spi_frame(0);
}

void lcd_fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x >= LCD_W || y >= LCD_H || w <= 0 || h <= 0) {
        return;
    }
    if (x + w > LCD_W) {
        w = LCD_W - x;
    }
    if (y + h > LCD_H) {
        h = LCD_H - y;
    }
    lcd_stream_begin(x, y, w, h);
    uint32_t count = (uint32_t)w * (uint32_t)h;
    while (count--) {
        lcd_stream_push(color);
    }
    lcd_stream_end();
}

void lcd_clear(uint16_t color) {
    lcd_fill_rect(0, 0, LCD_W, LCD_H, color);
}

static int isqrt_int(int value) {
    if (value <= 0) {
        return 0;
    }
    int x = value;
    int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + value / x) / 2;
    }
    return x;
}

void lcd_fill_circle(int cx, int cy, int radius, uint16_t color) {
    if (radius < 0) {
        return;
    }
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = isqrt_int(radius * radius - dy * dy);
        lcd_fill_rect(cx - dx, cy + dy, dx * 2 + 1, 1, color);
    }
}
