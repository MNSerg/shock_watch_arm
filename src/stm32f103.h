/* Карта регистров STM32F103, которые использует прошивка.
   Адреса и смещения совпадают с RM0008. HAL, LL и runtime SPL
   не подключаются: обращение к периферии прямое. */
#ifndef STM32F103_H
#define STM32F103_H

#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

/* Ядро Cortex-M3 */
#define SYST_CSR   REG32(0xE000E010u)
#define SYST_RVR   REG32(0xE000E014u)
#define SYST_CVR   REG32(0xE000E018u)
#define SYST_ENABLE   (1u << 0)
#define SYST_TICKINT  (1u << 1)
#define SYST_CLKSRC   (1u << 2)

#define NVIC_ISER(n) REG32(0xE000E100u + 4u * (n))

#define SCB_SCR    REG32(0xE000ED10u)
#define SCB_SCR_SLEEPDEEP (1u << 2)

#define DEMCR      REG32(0xE000EDFCu)
#define DWT_CTRL   REG32(0xE0001000u)
#define DWT_CYCCNT REG32(0xE0001004u)
#define DEMCR_TRCENA (1u << 24)
#define DWT_CYCCNTENA (1u << 0)

/* FLASH */
#define FLASH_ACR  REG32(0x40022000u)
#define FLASH_ACR_LATENCY_2 2u
#define FLASH_ACR_PRFTBE    (1u << 4)

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_TypeDef;

typedef struct {
    volatile uint32_t EVCR;
    volatile uint32_t MAPR;
    volatile uint32_t EXTICR[4];
    volatile uint32_t MAPR2;
} AFIO_TypeDef;

typedef struct {
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
} EXTI_TypeDef;

typedef struct {
    volatile uint16_t CR1;
    uint16_t r0;
    volatile uint16_t CR2;
    uint16_t r1;
    volatile uint16_t SR;
    uint16_t r2;
    volatile uint16_t DR;
    uint16_t r3;
} SPI_TypeDef;

typedef struct {
    volatile uint16_t CR1;
    uint16_t r0;
    volatile uint16_t CR2;
    uint16_t r1;
    volatile uint16_t OAR1;
    uint16_t r2;
    volatile uint16_t OAR2;
    uint16_t r3;
    volatile uint16_t DR;
    uint16_t r4;
    volatile uint16_t SR1;
    uint16_t r5;
    volatile uint16_t SR2;
    uint16_t r6;
    volatile uint16_t CCR;
    uint16_t r7;
    volatile uint16_t TRISE;
    uint16_t r8;
} I2C_TypeDef;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t JOFR1;
    volatile uint32_t JOFR2;
    volatile uint32_t JOFR3;
    volatile uint32_t JOFR4;
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t JSQR;
    volatile uint32_t JDR1;
    volatile uint32_t JDR2;
    volatile uint32_t JDR3;
    volatile uint32_t JDR4;
    volatile uint32_t DR;
} ADC_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CSR;
} PWR_TypeDef;

/* DR1 лежит по смещению 0x04 от базы BKP. */
typedef struct {
    uint32_t reserved0;
    volatile uint16_t DR1;
    uint16_t p1;
    volatile uint16_t DR2;
    uint16_t p2;
    volatile uint16_t DR3;
    uint16_t p3;
    volatile uint16_t DR4;
    uint16_t p4;
} BKP_TypeDef;

typedef struct {
    volatile uint16_t CRH;
    uint16_t p0;
    volatile uint16_t CRL;
    uint16_t p1;
    volatile uint16_t PRLH;
    uint16_t p2;
    volatile uint16_t PRLL;
    uint16_t p3;
    volatile uint16_t DIVH;
    uint16_t p4;
    volatile uint16_t DIVL;
    uint16_t p5;
    volatile uint16_t CNTH;
    uint16_t p6;
    volatile uint16_t CNTL;
    uint16_t p7;
    volatile uint16_t ALRH;
    uint16_t p8;
    volatile uint16_t ALRL;
    uint16_t p9;
} RTC_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x40010800u)
#define GPIOB ((GPIO_TypeDef *)0x40010C00u)
#define GPIOC ((GPIO_TypeDef *)0x40011000u)
#define RCC   ((RCC_TypeDef *)0x40021000u)
#define AFIO  ((AFIO_TypeDef *)0x40010000u)
#define EXTI  ((EXTI_TypeDef *)0x40010400u)
#define SPI2  ((SPI_TypeDef *)0x40003800u)
#define I2C2  ((I2C_TypeDef *)0x40005800u)
#define ADC1  ((ADC_TypeDef *)0x40012400u)
#define PWR   ((PWR_TypeDef *)0x40007000u)
#define BKP   ((BKP_TypeDef *)0x40006C00u)
#define RTC   ((RTC_TypeDef *)0x40002800u)

/* 8-битный доступ к SPI_DR: запись uint16_t в 8-битном кадре
   на F103 иногда выталкивает лишний байт. */
#define SPI2_DR8 (*(volatile uint8_t *)0x4000380Cu)

#define RCC_CR_HSEON  (1u << 16)
#define RCC_CR_HSERDY (1u << 17)
#define RCC_CR_PLLON  (1u << 24)
#define RCC_CR_PLLRDY (1u << 25)

#define RCC_APB2ENR_AFIOEN (1u << 0)
#define RCC_APB2ENR_IOPA   (1u << 2)
#define RCC_APB2ENR_IOPB   (1u << 3)
#define RCC_APB2ENR_IOPC   (1u << 4)
#define RCC_APB2ENR_ADC1   (1u << 9)
#define RCC_APB1ENR_SPI2   (1u << 14)
#define RCC_APB1ENR_I2C2   (1u << 22)
#define RCC_APB1ENR_PWREN  (1u << 28)
#define RCC_APB1ENR_BKPEN  (1u << 27)
#define RCC_APB1RSTR_I2C2  (1u << 22)

#define RCC_BDCR_LSEON  (1u << 0)
#define RCC_BDCR_LSERDY (1u << 1)
#define RCC_BDCR_RTCSEL_LSE (1u << 8)
#define RCC_BDCR_RTCSEL_LSI (2u << 8)
#define RCC_BDCR_RTCSEL_MASK (3u << 8)
#define RCC_BDCR_RTCEN  (1u << 15)
#define RCC_BDCR_BDRST  (1u << 16)
#define RCC_CSR_LSION   (1u << 0)
#define RCC_CSR_LSIRDY  (1u << 1)

#define PWR_CR_LPDS (1u << 0)
#define PWR_CR_PDDS (1u << 1)
#define PWR_CR_CWUF (1u << 2)
#define PWR_CR_CSBF (1u << 3)
#define PWR_CR_DBP  (1u << 8)
#define PWR_CSR_WUF (1u << 0)
#define PWR_CSR_SBF (1u << 1)
#define PWR_CSR_EWUP (1u << 8)

#define RTC_CRL_ALRF  (1u << 1)
#define RTC_CRL_RSF   (1u << 3)
#define RTC_CRL_CNF   (1u << 4)
#define RTC_CRL_RTOFF (1u << 5)
#define RTC_CRH_ALRIE (1u << 1)

#define SPI_CR1_MSTR    (1u << 2)
#define SPI_CR1_BR_SHIFT 3
#define SPI_CR1_SPE     (1u << 6)
#define SPI_CR1_SSI     (1u << 8)
#define SPI_CR1_SSM     (1u << 9)
#define SPI_CR1_DFF     (1u << 11)
#define SPI_CR1_BIDIOE  (1u << 14)
#define SPI_CR1_BIDIMODE (1u << 15)
#define SPI_SR_TXE (1u << 1)
#define SPI_SR_BSY (1u << 7)

#define I2C_CR1_PE    (1u << 0)
#define I2C_CR1_START (1u << 8)
#define I2C_CR1_STOP  (1u << 9)
#define I2C_CR1_ACK   (1u << 10)
#define I2C_CR1_SWRST (1u << 15)
#define I2C_SR1_SB   (1u << 0)
#define I2C_SR1_ADDR (1u << 1)
#define I2C_SR1_BTF  (1u << 2)
#define I2C_SR1_RXNE (1u << 6)
#define I2C_SR1_TXE  (1u << 7)
#define I2C_SR1_BERR (1u << 8)
#define I2C_SR1_ARLO (1u << 9)
#define I2C_SR1_AF   (1u << 10)
#define I2C_SR1_OVR  (1u << 11)
#define I2C_SR1_ERR  (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | I2C_SR1_OVR)
#define I2C_SR2_BUSY (1u << 1)

#define ADC_SR_EOC   (1u << 1)
#define ADC_CR2_ADON (1u << 0)
#define ADC_CR2_CAL  (1u << 2)
#define ADC_CR2_RSTCAL (1u << 3)
#define ADC_CR2_EXTTRIG (1u << 20)
#define ADC_CR2_EXTSEL_SW (7u << 17)
#define ADC_CR2_SWSTART (1u << 22)

/* CNF:MODE в младших 4 битах ячейки CRL/CRH. */
#define GPIO_ANALOG   0x0u
#define GPIO_IN_FLOAT 0x4u
#define GPIO_IN_PULL  0x8u
#define GPIO_OUT_PP2  0x2u
#define GPIO_OUT_PP50 0x3u
#define GPIO_OUT_OD50 0x7u
#define GPIO_AF_PP50  0xBu
#define GPIO_AF_OD50  0xFu

static inline void irq_disable(void) { __asm volatile("cpsid i" ::: "memory"); }
static inline void irq_enable(void) { __asm volatile("cpsie i" ::: "memory"); }
static inline void cpu_wfi(void) { __asm volatile("wfi" ::: "memory"); }
static inline void cpu_dsb(void) { __asm volatile("dsb sy" ::: "memory"); }

static inline void nvic_enable(int irqn) {
    NVIC_ISER((unsigned)irqn >> 5) = 1u << ((unsigned)irqn & 31u);
}

static inline void gpio_mode(GPIO_TypeDef *port, unsigned pin, uint32_t cnf_mode) {
    volatile uint32_t *cr = (pin < 8u) ? &port->CRL : &port->CRH;
    unsigned shift = (pin & 7u) * 4u;
    uint32_t value = *cr;
    value &= ~(0xFu << shift);
    value |= (cnf_mode & 0xFu) << shift;
    *cr = value;
}

static inline void gpio_set(GPIO_TypeDef *port, unsigned pin) {
    port->BSRR = 1u << pin;
}

static inline void gpio_reset(GPIO_TypeDef *port, unsigned pin) {
    port->BRR = 1u << pin;
}

static inline int gpio_read(GPIO_TypeDef *port, unsigned pin) {
    return (port->IDR & (1u << pin)) ? 1 : 0;
}

#endif
