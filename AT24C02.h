#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "stdio.h"
#include <string.h>
#include <stdlib.h>
#include "stdbool.h"

#ifndef AT24C02_H_
#define AT24C02_H_

#define AT24C_ADDR_7BIT (0x50<<1)

/* ===================== Настройка I2C ===================== */
#define SCL_PIN GPIO_Pin_10
#define SDA_PIN GPIO_Pin_11
#define I2C_PORT GPIOB

#define I2C_APB1_MHZ 36U
#define I2C_CCR_100K 180U
#define I2C_TRISE_VAL 37U

#define I2C_TIMEOUT 100

/* Раскомментируйте одну периферию I2C */
//#define I2C_PERPH_1
#define I2C_PERPH_2
#ifdef I2C_PERPH_1
#define I2C_PERPH I2C1
#define I2C_ER_IRQn I2C1_ER_IRQn
#elif defined(I2C_PERPH_2)
#define I2C_PERPH I2C2
#define I2C_ER_IRQn I2C2_ER_IRQn
#endif

void Init_AT24(void);
bool AT24C02_Write(uint8_t AddressDevice, uint8_t AddressByte, uint8_t Value);
bool AT24C02_Read(uint8_t AddressDevice, uint8_t AddressByte, uint8_t *data);
bool I2C_Check(uint32_t event);
void I2C_ResetBus(void);
void I2C_Reset(void);
uint8_t read_i2c_response(uint8_t target_address, char *buffer, size_t buf_size, size_t *bytes_read);
uint8_t send_i2c_command(uint8_t target_address, const char *command);
u8 i2c_wait_flag(uint32_t flag, u8 set, uint32_t timeout_ms);

#endif /* AT24C02_H_ */
