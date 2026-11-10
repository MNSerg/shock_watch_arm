
#include "AT24C02.h"
#include "stm32f10x_i2c.h"
#include "misc.h"

void Init_AT24(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
#ifdef I2C_PERPH_1
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
#elif defined(I2C_PERPH_2)
	RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
#endif

	{
		GPIO_InitTypeDef scl;
		scl.GPIO_Mode = GPIO_Mode_AF_OD;
		scl.GPIO_Pin = SCL_PIN;
		scl.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(I2C_PORT, &scl);

		GPIO_InitTypeDef sda;
		sda.GPIO_Mode = GPIO_Mode_AF_OD;
		sda.GPIO_Pin = SDA_PIN;
		sda.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(I2C_PORT, &sda);
	}

	I2C_PERPH->CR2 = I2C_APB1_MHZ;
	I2C_PERPH->CCR = I2C_CCR_100K;
	I2C_PERPH->TRISE = I2C_TRISE_VAL;
	I2C_PERPH->CR1 |= I2C_CR1_ACK;
	I2C_PERPH->CR1 |= I2C_CR1_PE;
	NVIC_EnableIRQ(I2C_ER_IRQn);
}

bool AT24C02_Write(uint8_t AddressDevice, uint8_t AddressByte, uint8_t Value)
{
	I2C_PERPH->CR1 |= I2C_CR1_ACK;
	uint32_t i=1000; uint8_t error = 0;
	while(I2C_GetFlagStatus(I2C_PERPH, I2C_FLAG_BUSY) && (--i)) {};
	error = (i>0) ? error : 1;
	I2C_AcknowledgeConfig(I2C_PERPH, ENABLE);

	I2C_GenerateSTART(I2C_PERPH, ENABLE);i=1000;
	while(!(I2C_PERPH->SR1 & I2C_SR1_SB) && (--i)){};
	error = (i>0) ? error : 1;
		(void) I2C_PERPH->SR1; // wait Generate Start

	I2C_Send7bitAddress(I2C_PERPH, AddressDevice, I2C_Direction_Transmitter);i=1000;
	while(!(I2C_PERPH->SR1 & I2C_SR1_ADDR) && (--i)){};
	error = (i>0) ? error : 1;
		(void) I2C_PERPH->SR1;
		(void) I2C_PERPH->SR2; // wait send Address

	I2C_SendData(I2C_PERPH, AddressByte);i=1000;
	while(!(I2C_PERPH->SR1 & I2C_SR1_TXE) && (--i)){}; // wait send Address Byte
	error = (i>0) ? error : 1;

	I2C_SendData(I2C_PERPH, Value);i=1000;
	while(!(I2C_PERPH->SR1 & I2C_SR1_BTF) && (--i)){};
	error = (i>0) ? error : 1;
	I2C_PERPH->CR1 |= I2C_CR1_STOP;i=1000;
	while(I2C_GetFlagStatus(I2C_PERPH, I2C_FLAG_STOPF) && (--i)) {}; // wait Generate Stop
	error = (i>0) ? error : 1;
	I2C_PERPH->SR1 &= ~I2C_SR1_AF;

	if (error) {
		I2C_Reset();
	}
	delay(25000);
	return error;
}

void I2C_Reset(void) {
    I2C_Cmd(I2C_PERPH, DISABLE);
    I2C_DeInit(I2C_PERPH);
    Init_AT24();
}

bool AT24C02_Read(uint8_t AddressDevice, uint8_t AddressByte, uint8_t *data)
{
	uint8_t ReceiveData = 0;
	uint32_t i=1000;
	uint8_t error = 0;
	while(I2C_GetFlagStatus(I2C_PERPH, I2C_FLAG_BUSY) && (--i));
	error = (i>0) ? error : 1;
	i=1000;
	I2C_GenerateSTART(I2C_PERPH, ENABLE);
	while(!(I2C_PERPH->SR1 & I2C_SR1_SB) && (--i)){};
	error = (i>0) ? error : 1;
		(void) I2C_PERPH->SR1; // wait Generate Start
		i=1000;
	I2C_Send7bitAddress(I2C_PERPH, AddressDevice, I2C_Direction_Transmitter);
	while(!(I2C_PERPH->SR1 & I2C_SR1_ADDR) && (--i)){};
	error = (i>0) ? error : 1;
		(void) I2C_PERPH->SR1;
		(void) I2C_PERPH->SR2;
	 // wait send Address Device
		i=1000;
	I2C_SendData(I2C_PERPH, AddressByte);
	while(!(I2C_PERPH->SR1 & I2C_SR1_TXE) && (--i)){};// wait send Address Byte
	error = (i>0) ? error : 1;
	i=1000;
	I2C_GenerateSTART(I2C_PERPH, ENABLE);
	while(!(I2C_PERPH->SR1 & I2C_SR1_SB) && (--i)){};
	error = (i>0) ? error : 1;
		(void) I2C_PERPH->SR1; // wait Generate Start
		i=1000;
	I2C_Send7bitAddress(I2C_PERPH, AddressDevice, I2C_Direction_Receiver);
	while(!(I2C_PERPH->SR1 & I2C_SR1_ADDR) && (--i)){};
	error = (i>0) ? error : 1;
		(void) I2C_PERPH->SR1;
		(void) I2C_PERPH->SR2; // wait Send Address Device As Receiver
		i=1000;	I2C_NACKPositionConfig(I2C_PERPH, I2C_NACKPosition_Current); // send not acknowledge
		I2C_AcknowledgeConfig(I2C_PERPH, DISABLE);// disable acknowledge
		I2C_PERPH->CR1 &= ~I2C_CR1_ACK;
	while( !I2C_CheckEvent(I2C_PERPH, I2C_EVENT_MASTER_BYTE_RECEIVED)  && (--i)) {}; // wait Receive a Byte
	error = (i>0) ? error : 1;
	ReceiveData = I2C_ReceiveData(I2C_PERPH);


	i=1000;
	I2C_GenerateSTOP(I2C_PERPH, ENABLE);
	while (I2C_GetFlagStatus(I2C_PERPH, I2C_FLAG_STOPF) && (--i)); // wait Generate Stop Condition
	error = (i>0) ? error : 1;
	I2C_GenerateSTOP(I2C_PERPH, ENABLE);
	I2C_AcknowledgeConfig(I2C_PERPH, DISABLE);// disable acknowledge
	I2C_PERPH->SR1 &= ~I2C_SR1_AF;

	*data = ReceiveData;
	if (error) {
		I2C_Reset();
	}

	delay(25000);
    return error;
}

// ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ I2C
bool I2C_Check(uint32_t event) {
    uint32_t timeout = I2C_TIMEOUT;
    uint32_t flag1 = 0, flag2 = 0;

    while(timeout--) {
        flag1 = I2C_PERPH->SR1;
        flag2 = I2C_PERPH->SR2;
        flag2 = flag2 << 16;

        // ˜˜˜˜˜˜˜˜ ˜˜ NACK
        if(flag1 & I2C_SR1_AF) {
        	I2C_PERPH->SR1 &= ~I2C_SR1_AF;
        	I2C_PERPH->CR1 |= I2C_CR1_STOP;
            return false;
        }

        // ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
        if((flag1 | flag2) & event) {
            return true;
        }
    }

    // ˜˜˜˜˜˜˜ - ˜˜˜˜˜ ˜˜˜˜
    I2C_ResetBus();
    return false;
}

void I2C_ResetBus(void) {
	I2C_PERPH->CR1 |= I2C_CR1_SWRST;
    I2C_PERPH->CR1 &= ~I2C_CR1_SWRST;
    Init_AT24();
}



// ˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜
#define I2C_BUSY_TIMEOUT_MS      100
#define I2C_FLAG_TIMEOUT_MS      50
#define I2C_BYTE_TIMEOUT_MS      10

// ˜˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
uint32_t get_tick(void) {
    return DWT->CYCCNT / (SystemCoreClock / 1000);
}

// ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜
u8 i2c_wait_flag(uint32_t flag, u8 set, uint32_t timeout_ms) {
    uint32_t start = get_tick();
    while (1) {
        if (set ? (I2C_PERPH->SR1 & flag) : !(I2C_PERPH->SR1 & flag)) {
            return true;
        }

        // ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜
        if ((get_tick() - start) > timeout_ms) {
            return false;
        }

        // ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜
        if (I2C_PERPH->SR1 & (I2C_SR1_AF | I2C_SR1_ARLO | I2C_SR1_BERR)) {
            return false;
        }
    }
}

// ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜˜
uint8_t send_i2c_command(uint8_t target_address, const char *command) {
    // 1. ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜
	I2C_PERPH->SR1 &= ~I2C_SR1_AF;
	I2C_PERPH->CR1 |= I2C_CR1_ACK;

	I2C_AcknowledgeConfig(I2C_PERPH, ENABLE);
    uint32_t start = get_tick();
    while (I2C_PERPH->SR2 & I2C_SR2_BUSY) {
        if ((get_tick() - start) > I2C_BUSY_TIMEOUT_MS) {
            return 0;
        }
    }

    // 2. ˜˜˜˜˜˜˜˜˜ START
    I2C_PERPH->CR1 |= I2C_CR1_START;
    if (!i2c_wait_flag(I2C_SR1_SB, true, I2C_FLAG_TIMEOUT_MS)) {
        return 0;
    }

    // 3. ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜
    I2C_PERPH->DR = target_address << 1;
    if (!i2c_wait_flag(I2C_SR1_ADDR, true, I2C_FLAG_TIMEOUT_MS)) {
        I2C_PERPH->CR1 |= I2C_CR1_STOP;
        I2C_PERPH->SR1 &= ~I2C_SR1_AF;
        return 0;
    }
    (void)I2C_PERPH->SR2;  // ˜˜˜˜˜ ˜˜˜˜˜ ADDR

    // 4. ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜
    const char *ptr = command;
    while (*ptr) {
        if (!i2c_wait_flag(I2C_SR1_TXE, true, I2C_BYTE_TIMEOUT_MS)) {
        	//I2C_PERPH->SR1 &= ~I2C_SR1_AF;
            I2C_PERPH->CR1 |= I2C_CR1_STOP;
            return 0;
        }

        I2C_PERPH->DR = *ptr++;

        // ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ACK
        if (I2C_PERPH->SR1 & I2C_SR1_AF) {
            //I2C_PERPH->SR1 &= ~I2C_SR1_AF;
            I2C_PERPH->CR1 |= I2C_CR1_STOP;

            return 0;
        }
    }

    // 5. ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜
    if (!i2c_wait_flag(I2C_SR1_TXE, true, I2C_BYTE_TIMEOUT_MS)) {
    	//I2C_PERPH->SR1 &= ~I2C_SR1_AF;
        I2C_PERPH->CR1 |= I2C_CR1_STOP;
        return 0;
    }
    I2C_PERPH->DR = '\r';

    if (!i2c_wait_flag(I2C_SR1_TXE, true, I2C_BYTE_TIMEOUT_MS)) {
    	//I2C_PERPH->SR1 &= ~I2C_SR1_AF;
        I2C_PERPH->CR1 |= I2C_CR1_STOP;
        return 0;
    }
    I2C_PERPH->DR = '\n';

    // 6. ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜
    if (!i2c_wait_flag(I2C_SR1_BTF, true, I2C_FLAG_TIMEOUT_MS)) {
    	//I2C_PERPH->SR1 &= ~I2C_SR1_AF;
        I2C_PERPH->CR1 |= I2C_CR1_STOP;
        return 0;
    }

    I2C_PERPH->CR1 |= I2C_CR1_STOP;
	//I2C_AcknowledgeConfig(I2C_PERPH, DISABLE);// disable acknowledge
	//I2C_PERPH->SR1 &= ~I2C_SR1_AF;
    return 1;
}

// ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜˜
uint8_t read_i2c_response(uint8_t target_address, char *buffer, size_t buf_size, size_t *bytes_read) {
    *bytes_read = 0;

    // 1. ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜
    uint32_t start = get_tick();
    while (I2C_PERPH->SR2 & I2C_SR2_BUSY) {
        if ((get_tick() - start) > I2C_BUSY_TIMEOUT_MS) {
            return 0;
        }
    }

    // 2. ˜˜˜˜˜˜˜˜˜ START
    I2C_PERPH->CR1 |= I2C_CR1_START;
    if (!i2c_wait_flag(I2C_SR1_SB, 1, I2C_FLAG_TIMEOUT_MS)) {
        return 0;
    }

    // 3. ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜˜˜
    I2C_PERPH->DR = (target_address << 1) | 0x01;
    if (!i2c_wait_flag(I2C_SR1_ADDR, 1, I2C_FLAG_TIMEOUT_MS)) {
        I2C_PERPH->CR1 |= I2C_CR1_STOP;
        return 0;
    }
    (void)I2C_PERPH->SR2;  // ˜˜˜˜˜ ˜˜˜˜˜ ADDR

    // 4. ˜˜˜˜˜˜ ˜˜˜˜˜˜
    size_t index = 0;
    bool last_byte = 0;

    while (index < buf_size - 1) {
        // ˜˜˜˜˜˜˜˜˜ ACK/NACK
        if (index < buf_size - 2 && !last_byte) {
            I2C_PERPH->CR1 |= I2C_CR1_ACK;
        } else {
            I2C_PERPH->CR1 &= ~I2C_CR1_ACK;
        }

        // ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜
        if (!i2c_wait_flag(I2C_SR1_RXNE, 1, I2C_BYTE_TIMEOUT_MS)) {
            I2C_PERPH->CR1 |= I2C_CR1_STOP;
            return 0;
        }

        // ˜˜˜˜˜˜ ˜˜˜˜˜
        char c = I2C_PERPH->DR;

        // ˜˜˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜ ˜˜˜˜˜˜
        if (c == '\n') {
            buffer[index] = '\0';
            *bytes_read = index;
            break;
        }

        buffer[index++] = c;

        // ˜˜˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜
        if (index == buf_size - 1) {
            last_byte = 1;
        }
    }

    // 5. ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜
    I2C_PERPH->CR1 |= I2C_CR1_STOP;
    I2C_PERPH->CR1 |= I2C_CR1_ACK;  // ˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ACK

    return 1;
}
