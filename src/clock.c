#include "clock.h"
#include "board.h"

uint32_t SystemCoreClock = 8000000u;
static int g_hse_ok;

uint32_t clock_pclk1_hz(void) {
    /* APB1 не выше 36 МГц. SPI2 и I2C2 тактируются от него.
       Делитель таймеров при APB1 /2 удваивается, у SPI — нет. */
    return (SystemCoreClock > 36000000u) ? (SystemCoreClock / 2u) : SystemCoreClock;
}

int clock_hse_ok(void) {
    return g_hse_ok;
}

static void dwt_on(void) {
    DEMCR |= DEMCR_TRCENA;
    if ((DWT_CTRL & DWT_CYCCNTENA) == 0u) {
        DWT_CYCCNT = 0u;
        DWT_CTRL |= DWT_CYCCNTENA;
    }
}

static int spin_set(volatile uint32_t *reg, uint32_t mask, uint32_t spins) {
    while (spins--) {
        if ((*reg & mask) != 0u) {
            return 1;
        }
    }
    return 0;
}

static void use_hsi(void) {
    FLASH_ACR = FLASH_ACR_PRFTBE;
    RCC->CFGR &= ~((0xFu << 4) | (7u << 8) | (7u << 11) | (3u << 14));
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_PLLON);
    SystemCoreClock = 8000000u;
    g_hse_ok = 0;
    dwt_on();
}

static void leave_pll(void) {
    RCC->CFGR = (RCC->CFGR & ~3u) | 0u;
    while (((RCC->CFGR >> 2) & 3u) != 0u) {
    }
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) {
    }
}

static void clock_apply(int want_hse) {
    /* PLL нельзя менять, пока от него тактируется ядро.
       После Stop ядро и так на HSI, а HSE и PLL аппаратно погашены. */
    leave_pll();
    if (!want_hse) {
        use_hsi();
        return;
    }

    RCC->CR |= RCC_CR_HSEON;
    if (!spin_set(&RCC->CR, RCC_CR_HSERDY, 2000000u)) {
        use_hsi();
        return;
    }

    FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~((0xFu << 4) | (7u << 8) | (7u << 11) | (3u << 14)
            | (1u << 16) | (1u << 17) | (0xFu << 18));
    cfgr |= (4u << 8);    /* APB1 /2 ? 36 МГц */
    cfgr |= (2u << 14);   /* ADC /6 ? 12 МГц, потолок АЦП 14 МГц */
    cfgr |= (1u << 16);   /* PLL от HSE без деления */
    cfgr |= ((uint32_t)(PLL_MUL - 2u) << 18);
    RCC->CFGR = cfgr;

    RCC->CR |= RCC_CR_PLLON;
    if (!spin_set(&RCC->CR, RCC_CR_PLLRDY, 2000000u)) {
        use_hsi();
        return;
    }
    RCC->CFGR = (RCC->CFGR & ~3u) | 2u;
    while (((RCC->CFGR >> 2) & 3u) != 2u) {
    }
    SystemCoreClock = HSE_HZ * (uint32_t)PLL_MUL;
    g_hse_ok = 1;
    dwt_on();
}

void SystemInit(void) {
    clock_apply(1);
}

void clock_restore(void) {
    /* Повторно поднимаем только тот источник, который уже взлетел на старте.
       Иначе каждая побудка из Stop секунду ждёт отсутствующий HSE. */
    clock_apply(g_hse_ok);
}
