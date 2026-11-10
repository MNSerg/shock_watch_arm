#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

void delay_us(uint32_t us);
void delay_ms(uint32_t ms);
int wait_flag(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_ms);

#endif
