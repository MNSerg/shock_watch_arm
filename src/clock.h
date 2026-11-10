#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

extern uint32_t SystemCoreClock;

void SystemInit(void);
void clock_restore(void);
uint32_t clock_pclk1_hz(void);
int clock_hse_ok(void);

#endif
