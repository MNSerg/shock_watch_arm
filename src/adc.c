#include "adc.h"
#include "board.h"
#include "clock.h"
#include "delay.h"

/* PA4 = ADC1_IN4. Опорное принимается равным BOARD_VREF_MV:
   у F103 нет калиброванного измерения Vdda без внешнего делителя.
   Чтение усредняется, между выборками АЦП выключается. */

void adc_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_ADC1;
    gpio_mode(GPIOA, 4u, GPIO_ANALOG);
    ADC1->SMPR2 = (7u << 12);          /* канал 4, 239.5 такта — высокое сопротивление делителя */
    ADC1->SQR1 = 0u;                   /* одна конверсия */
    ADC1->SQR3 = 4u;
    ADC1->CR2 = ADC_CR2_ADON;
    delay_us(20u);
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL) {
    }
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) {
    }
    ADC1->CR2 &= ~ADC_CR2_ADON;
}

static int adc_raw(uint16_t *out) {
    ADC1->CR2 |= ADC_CR2_ADON;
    delay_us(10u);
    ADC1->CR2 |= ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL_SW | ADC_CR2_SWSTART;
    uint32_t start = DWT_CYCCNT;
    uint32_t limit = SystemCoreClock / 100u;
    while ((ADC1->SR & ADC_SR_EOC) == 0u) {
        if ((DWT_CYCCNT - start) > limit) {
            ADC1->CR2 &= ~ADC_CR2_ADON;
            return -1;
        }
    }
    *out = (uint16_t)ADC1->DR;
    ADC1->CR2 &= ~ADC_CR2_ADON;
    return 0;
}

int adc_battery_mv(void) {
    uint32_t sum = 0u;
    unsigned n = 0u;
    for (int i = 0; i < 4; i++) {
        uint16_t raw = 0u;
        if (adc_raw(&raw) == 0) {
            sum += raw;
            n++;
        }
    }
    if (n == 0u) {
        return 0;
    }
    uint32_t raw = sum / n;
    if (raw < 50u) {
        return 0;                      /* вход висит или делитель не запаян */
    }
    uint32_t pin = raw * BOARD_VREF_MV / 4095u;
    return (int)(pin * (BOARD_BAT_RTOP + BOARD_BAT_RBOT) / BOARD_BAT_RBOT);
}

int adc_battery_pct(int millivolts) {
    if (millivolts <= 0) {
        return -1;
    }
    if (millivolts <= BOARD_BAT_EMPTY_MV) {
        return 0;
    }
    if (millivolts >= BOARD_BAT_FULL_MV) {
        return 100;
    }
    return (millivolts - BOARD_BAT_EMPTY_MV) * 100
        / (BOARD_BAT_FULL_MV - BOARD_BAT_EMPTY_MV);
}
