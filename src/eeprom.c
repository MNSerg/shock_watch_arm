#include "eeprom.h"
#include "board.h"
#include "clock.h"
#include "delay.h"

/* AT24C02: 256 байт, страница 8 байт, цикл записи до 5 мс.
   Карта:
     0..7  настройки (магия C1 0C, версия, флаги, будильник, таймаут, CRC)
     8..15 последняя записанная эпоха, на случай пустого домена VBAT
   Адрес в DR — 8-битный (0xA0 / 0xA1). Повторный сдвиг, как в старом
   драйвере через I2C_Send7bitAddress, уводил транзакцию на чужой адрес. */

#define EE_MAGIC0 0xC1u
#define EE_MAGIC1 0x0Cu
#define EE_VERSION 1u
#define EE_SET_ADDR 0u
#define EE_EPOCH_ADDR 8u

static int g_i2c_ready;

static void i2c_pin_af(void) {
    gpio_mode(GPIOB, 10u, GPIO_AF_OD50);
    gpio_mode(GPIOB, 11u, GPIO_AF_OD50);
}

static void i2c_hw_init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_I2C2;
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C2;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C2;

    uint32_t pclk = clock_pclk1_hz();
    uint32_t mhz = pclk / 1000000u;
    if (mhz < 2u) {
        mhz = 2u;
    }
    if (mhz > 36u) {
        mhz = 36u;
    }
    uint32_t ccr = pclk / (100000u * 2u);
    if (ccr < 4u) {
        ccr = 4u;
    }
    I2C2->CR1 = 0u;
    I2C2->CR2 = (uint16_t)mhz;
    I2C2->CCR = (uint16_t)ccr;          /* standard mode, 100 кГц */
    I2C2->TRISE = (uint16_t)(mhz + 1u);
    I2C2->CR1 = I2C_CR1_PE | I2C_CR1_ACK;
    g_i2c_ready = 1;
}

static void i2c_bus_recover(void) {
    /* Девять тактов SCL снимают слейва, который зажал SDA посередине байта. */
    I2C2->CR1 &= ~I2C_CR1_PE;
    gpio_mode(GPIOB, 10u, GPIO_OUT_OD50);
    gpio_mode(GPIOB, 11u, GPIO_OUT_OD50);
    gpio_set(GPIOB, 10u);
    gpio_set(GPIOB, 11u);
    delay_us(5u);
    for (int i = 0; i < 9; i++) {
        gpio_reset(GPIOB, 10u);
        delay_us(5u);
        gpio_set(GPIOB, 10u);
        delay_us(5u);
    }
    gpio_reset(GPIOB, 11u);
    delay_us(5u);
    gpio_set(GPIOB, 10u);
    delay_us(5u);
    gpio_set(GPIOB, 11u);
    delay_us(5u);
    i2c_pin_af();
    i2c_hw_init();
}

static int sr1_wait(uint16_t mask) {
    for (uint32_t i = 0; i < 100000u; i++) {
        uint16_t sr = I2C2->SR1;
        if (sr & mask) {
            return 0;
        }
        if (sr & I2C_SR1_ERR) {
            return -1;
        }
    }
    return -1;
}

static int wait_idle(void) {
    for (uint32_t i = 0; i < 100000u; i++) {
        if ((I2C2->SR2 & I2C_SR2_BUSY) == 0u) {
            return 0;
        }
    }
    return -1;
}

static void clear_addr(void) {
    (void)I2C2->SR1;
    (void)I2C2->SR2;
}

static int start_addr(uint8_t addr8, int check_busy) {
    /* Повторный START приходит, пока шина ещё занята мастером.
       Ждать BUSY==0 в этот момент нельзя: условие не наступит. */
    if (check_busy && wait_idle() != 0) {
        return -1;
    }
    I2C2->CR1 |= I2C_CR1_START;
    if (sr1_wait(I2C_SR1_SB) != 0) {
        return -1;
    }
    I2C2->DR = addr8;
    if (sr1_wait(I2C_SR1_ADDR) != 0) {
        I2C2->SR1 &= (uint16_t)~I2C_SR1_AF;
        I2C2->CR1 |= I2C_CR1_STOP;
        return -1;
    }
    clear_addr();
    return 0;
}

static int write_page(uint8_t mem, const uint8_t *data, unsigned len) {
    if (start_addr(EE_ADDR_WRITE, 1) != 0) {
        return -1;
    }
    if (sr1_wait(I2C_SR1_TXE) != 0) {
        return -1;
    }
    I2C2->DR = mem;
    for (unsigned i = 0; i < len; i++) {
        if (sr1_wait(I2C_SR1_TXE) != 0) {
            return -1;
        }
        I2C2->DR = data[i];
    }
    if (sr1_wait(I2C_SR1_BTF) != 0) {
        return -1;
    }
    I2C2->CR1 |= I2C_CR1_STOP;
    return 0;
}

static int read_at(uint8_t mem, uint8_t *data, unsigned len) {
    if (len == 0u) {
        return 0;
    }
    if (start_addr(EE_ADDR_WRITE, 1) != 0) {
        return -1;
    }
    if (sr1_wait(I2C_SR1_TXE) != 0) {
        return -1;
    }
    I2C2->DR = mem;
    if (sr1_wait(I2C_SR1_BTF) != 0) {
        return -1;
    }
    if (start_addr((uint8_t)(EE_ADDR_WRITE | 1u), 0) != 0) {
        return -1;
    }
    for (unsigned i = 0; i < len; i++) {
        if (i + 1u == len) {
            I2C2->CR1 &= (uint16_t)~I2C_CR1_ACK;
            I2C2->CR1 |= I2C_CR1_STOP;
        }
        if (sr1_wait(I2C_SR1_RXNE) != 0) {
            return -1;
        }
        data[i] = (uint8_t)I2C2->DR;
    }
    I2C2->CR1 |= I2C_CR1_ACK;
    return 0;
}

static uint8_t checksum(const uint8_t *data, unsigned len) {
    uint8_t sum = 0xA5u;
    for (unsigned i = 0; i < len; i++) {
        sum = (uint8_t)(sum ^ data[i]);
    }
    return sum;
}

void settings_default(Settings *s) {
    s->hour12 = 0u;
    s->alarm_on = 0u;
    s->alarm_h = 7u;
    s->alarm_m = 0u;
    s->sleep_s = 20u;
}

void eeprom_init(void) {
    i2c_pin_af();
    i2c_hw_init();
}

static int guarded_write(uint8_t mem, const uint8_t *data, unsigned len) {
    if (write_page(mem, data, len) == 0) {
        delay_ms(6u);
        return 0;
    }
    i2c_bus_recover();
    if (write_page(mem, data, len) != 0) {
        i2c_bus_recover();
        return -1;
    }
    delay_ms(6u);
    return 0;
}

static int guarded_read(uint8_t mem, uint8_t *data, unsigned len) {
    if (read_at(mem, data, len) == 0) {
        return 0;
    }
    i2c_bus_recover();
    if (read_at(mem, data, len) != 0) {
        i2c_bus_recover();
        return -1;
    }
    return 0;
}

int eeprom_load(Settings *s) {
    uint8_t raw[8];
    settings_default(s);
    if (!g_i2c_ready || guarded_read(EE_SET_ADDR, raw, 8u) != 0) {
        return -1;
    }
    if (raw[0] != EE_MAGIC0 || raw[1] != EE_MAGIC1 || raw[2] != EE_VERSION) {
        return -1;
    }
    if (checksum(raw, 7u) != raw[7]) {
        return -1;
    }
    s->hour12 = (raw[3] & 1u) ? 1u : 0u;
    s->alarm_on = (raw[3] & 2u) ? 1u : 0u;
    s->alarm_h = raw[4];
    s->alarm_m = raw[5];
    s->sleep_s = raw[6];
    if (s->alarm_h > 23u || s->alarm_m > 59u) {
        settings_default(s);
        return -1;
    }
    if (s->sleep_s != 10u && s->sleep_s != 20u && s->sleep_s != 30u && s->sleep_s != 60u) {
        s->sleep_s = 20u;
    }
    return 0;
}

int eeprom_save(const Settings *s) {
    uint8_t raw[8];
    raw[0] = EE_MAGIC0;
    raw[1] = EE_MAGIC1;
    raw[2] = EE_VERSION;
    raw[3] = (uint8_t)((s->hour12 ? 1u : 0u) | (s->alarm_on ? 2u : 0u));
    raw[4] = s->alarm_h;
    raw[5] = s->alarm_m;
    raw[6] = s->sleep_s;
    raw[7] = checksum(raw, 7u);
    return guarded_write(EE_SET_ADDR, raw, 8u);
}

int eeprom_load_epoch(uint32_t *epoch) {
    uint8_t raw[8];
    if (guarded_read(EE_EPOCH_ADDR, raw, 5u) != 0) {
        return -1;
    }
    if (checksum(raw, 4u) != raw[4]) {
        return -1;
    }
    *epoch = (uint32_t)raw[0]
        | ((uint32_t)raw[1] << 8)
        | ((uint32_t)raw[2] << 16)
        | ((uint32_t)raw[3] << 24);
    return 0;
}

int eeprom_save_epoch(uint32_t epoch) {
    uint8_t raw[8];
    raw[0] = (uint8_t)epoch;
    raw[1] = (uint8_t)(epoch >> 8);
    raw[2] = (uint8_t)(epoch >> 16);
    raw[3] = (uint8_t)(epoch >> 24);
    raw[4] = checksum(raw, 4u);
    return guarded_write(EE_EPOCH_ADDR, raw, 5u);
}
