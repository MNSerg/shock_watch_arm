#include "power.h"
#include "board.h"
#include "clock.h"
#include "input.h"

/* Три режима, пока часы не на руке:
   Sleep  — WFI в главном цикле: ядро стоит, такт и экран живы.
   Stop   — SLEEPDEEP + LPDS: HSE/PLL погашены, RAM цел, будят обе кнопки и будильник.
   Standby — SLEEPDEEP + PDDS: содержимое RAM теряется, будит только нарастающий PA0
             и будильник RTC. После пробуждения программа стартует заново. */

int power_woke_from_standby(void) {
    int woke = (PWR->CSR & PWR_CSR_SBF) ? 1 : 0;
    PWR->CR |= PWR_CR_CSBF | PWR_CR_CWUF;
    return woke;
}

void power_enter_stop(void) {
    SYST_CSR = 0u;
    PWR->CR |= PWR_CR_CWUF;
    PWR->CR = (PWR->CR & ~PWR_CR_PDDS) | PWR_CR_LPDS;
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    cpu_dsb();
    cpu_wfi();
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    clock_restore();
    systick_start();
}

void power_enter_standby(void) {
    /* Пока PA0 держится в единице, фронта для следующего пробуждения не будет,
       а часть ревизий выходит из Standby сразу. Ждём отпускания. */
    while (gpio_read(BOARD_BTN1_PORT, BOARD_BTN1_PIN)) {
    }
    SYST_CSR = 0u;
    PWR->CSR |= PWR_CSR_EWUP;
    PWR->CR |= PWR_CR_CSBF;
    PWR->CR |= PWR_CR_CWUF;
    while (PWR->CSR & PWR_CSR_WUF) {
        PWR->CR |= PWR_CR_CWUF;
    }
    PWR->CR |= PWR_CR_PDDS;
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    cpu_dsb();
    cpu_wfi();
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    clock_restore();
    systick_start();
}
