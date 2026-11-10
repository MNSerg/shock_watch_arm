#include "pins.h"
#include "board.h"

void pins_prepare(void) {
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPA | RCC_APB2ENR_IOPB | RCC_APB2ENR_IOPC;
    /* PB3 — CS дисплея, а после сброса это JTDO. JTAG выключаем, SWD оставляем. */
    AFIO->MAPR = (AFIO->MAPR & ~(7u << 24)) | (2u << 24);

    GPIOA->CRL = 0u;
    GPIOA->CRH &= (0xFFu << 20);   /* PA13/PA14 не трогаем: это SWD */
    GPIOB->CRL = 0u;
    GPIOB->CRH = 0u;
    GPIOC->CRL = 0u;
    GPIOC->CRH &= 0xFF000000u;     /* PC14/PC15 принадлежат LSE */

#if BOARD_HAS_LED
    gpio_mode(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_OUT_PP2);
    led_set(0);
#endif
}

void led_set(int on) {
#if BOARD_HAS_LED
    int level = on ? 1 : 0;
    if (BOARD_LED_ACTIVE_LOW) {
        level = !level;
    }
    if (level) {
        gpio_set(BOARD_LED_PORT, BOARD_LED_PIN);
    } else {
        gpio_reset(BOARD_LED_PORT, BOARD_LED_PIN);
    }
#else
    (void)on;
#endif
}
