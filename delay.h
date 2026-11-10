#ifndef __DELAY_H
#define __DELAY_H


#include "stm32f10x.h"
#include "stm32f10x_rcc.h"

#define    DWT_CYCCNT    *(volatile uint32_t *)0xE0001004
#define DWT_CONTROL *(volatile unsigned long *)0xE0001000
#define SCB_DEMCR   *(volatile unsigned long *)0xE000EDFC
#define k 100 //Коэффициент для частоты 72 МГц

uint8_t init_sysclk(void);
void DWT_Init(void);
void delay_us(u32 us);
void delay_ms(u32 us);

#endif
