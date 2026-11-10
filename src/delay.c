#include "delay.h"
#include "clock.h"
#include "stm32f103.h"

void delay_us(uint32_t us) {
    uint32_t cycles = us * (SystemCoreClock / 1000000u);
    if (cycles < 2u) {
        cycles = 2u;
    }
    if (DWT_CTRL & DWT_CYCCNTENA) {
        uint32_t start = DWT_CYCCNT;
        while ((DWT_CYCCNT - start) < cycles) {
        }
        return;
    }
    while (cycles--) {
        __asm volatile("nop");
    }
}

void delay_ms(uint32_t ms) {
    while (ms--) {
        delay_us(1000u);
    }
}

int wait_flag(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_ms) {
    uint32_t start = DWT_CYCCNT;
    uint32_t limit = timeout_ms * (SystemCoreClock / 1000u);
    while ((DWT_CYCCNT - start) < limit) {
        if ((*reg & mask) != 0u) {
            return 1;
        }
    }
    return (*reg & mask) ? 1 : 0;
}
