#include "rtc_clock.h"
#include "clock.h"
#include "stm32f103.h"

/* F103 хранит в RTC не календарь, а 32-битный счётчик секунд.
   Предделитель LSE: 32768 Гц / (32767+1) = 1 Гц.
   Домен резервного копирования переживает Stop, Standby и пропадание VDD,
   если на VBAT есть батарея. Магия в BKP_DR1 не даёт сбрасывать его при каждом старте. */

#define RTC_MAGIC 0xA55Au

static int g_ok;
static int g_lse;
static int g_boot_alarm;
static volatile int g_alarm_irq;

static int wait_rtoff(void) {
    for (uint32_t i = 0; i < 4000000u; i++) {
        if (RTC->CRL & RTC_CRL_RTOFF) {
            return 1;
        }
    }
    return 0;
}

static int wait_sync(void) {
    /* RSF сбрасывается записью нуля и поднимается, когда теневые регистры обновлены. */
    RTC->CRL &= (uint16_t)~RTC_CRL_RSF;
    for (uint32_t i = 0; i < 4000000u; i++) {
        if (RTC->CRL & RTC_CRL_RSF) {
            return 1;
        }
    }
    return 0;
}

static void backup_access(void) {
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR->CR |= PWR_CR_DBP;
}

static void config_begin(void) {
    wait_rtoff();
    RTC->CRL |= RTC_CRL_CNF;
}

static void config_end(void) {
    RTC->CRL &= (uint16_t)~RTC_CRL_CNF;
    wait_rtoff();
}

static void clear_alarm_flag(void) {
    if (wait_rtoff()) {
        RTC->CRL &= (uint16_t)~RTC_CRL_ALRF;
    }
    if (EXTI->PR & (1u << 17)) {
        EXTI->PR = (1u << 17);
    }
}

void rtc_init(uint32_t fallback_epoch) {
    backup_access();
    if (BKP->DR1 == RTC_MAGIC) {
        g_lse = (BKP->DR2 == 1u) ? 1 : 0;
        g_ok = wait_sync();
        g_boot_alarm = (RTC->CRL & RTC_CRL_ALRF) ? 1 : 0;
        if (g_boot_alarm) {
            clear_alarm_flag();
        }
        return;
    }

    RCC->BDCR |= RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;

    RCC->BDCR |= RCC_BDCR_LSEON;
    uint32_t start = DWT_CYCCNT;
    int lse = 0;
    while ((DWT_CYCCNT - start) < (SystemCoreClock * 3u)) {
        if (RCC->BDCR & RCC_BDCR_LSERDY) {
            lse = 1;
            break;
        }
    }

    uint16_t prescaler;
    if (lse) {
        RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL_MASK) | RCC_BDCR_RTCSEL_LSE | RCC_BDCR_RTCEN;
        prescaler = 32767u;
        g_lse = 1;
    } else {
        /* Кварц 32 кГц не стартовал. LSI около 40 кГц, ход будет плавать. */
        RCC->CSR |= RCC_CSR_LSION;
        start = DWT_CYCCNT;
        while ((DWT_CYCCNT - start) < SystemCoreClock) {
            if (RCC->CSR & RCC_CSR_LSIRDY) {
                break;
            }
        }
        RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL_MASK) | RCC_BDCR_RTCSEL_LSI | RCC_BDCR_RTCEN;
        prescaler = 39999u;
        g_lse = 0;
    }

    if (!wait_sync()) {
        g_ok = 0;
        return;
    }
    config_begin();
    RTC->PRLH = 0u;
    RTC->PRLL = prescaler;
    RTC->CNTH = (uint16_t)(fallback_epoch >> 16);
    RTC->CNTL = (uint16_t)fallback_epoch;
    RTC->ALRH = 0xFFFFu;
    RTC->ALRL = 0xFFFFu;
    config_end();
    BKP->DR1 = RTC_MAGIC;
    BKP->DR2 = (uint16_t)(g_lse ? 1u : 2u);
    g_ok = 1;
    g_boot_alarm = 0;
}

int rtc_ok(void) {
    return g_ok;
}

int rtc_uses_lse(void) {
    return g_lse;
}

uint32_t rtc_epoch(void) {
    uint16_t high;
    uint16_t low;
    uint16_t again;
    do {
        high = RTC->CNTH;
        low = RTC->CNTL;
        again = RTC->CNTH;
    } while (high != again);
    return ((uint32_t)high << 16) | low;
}

void rtc_set_epoch(uint32_t epoch) {
    if (!g_ok) {
        return;
    }
    config_begin();
    RTC->CNTH = (uint16_t)(epoch >> 16);
    RTC->CNTL = (uint16_t)epoch;
    config_end();
}

void rtc_irq_init(void) {
    EXTI->IMR |= (1u << 17);
    EXTI->RTSR |= (1u << 17);
    EXTI->PR = (1u << 17);
    nvic_enable(3);    /* RTC: секунда, переполнение, будильник */
    nvic_enable(41);   /* EXTI17, именно эта линия будит Stop и Standby */
}

int rtc_take_boot_alarm(void) {
    int fired = g_boot_alarm;
    g_boot_alarm = 0;
    return fired;
}

int rtc_consume_irq(void) {
    if (!g_alarm_irq) {
        return 0;
    }
    g_alarm_irq = 0;
    return 1;
}

static void alarm_isr(void) {
    uint32_t guard = 100000u;
    while (guard-- && (RTC->CRL & RTC_CRL_RTOFF) == 0u) {
    }
    if (RTC->CRL & RTC_CRL_RTOFF) {
        RTC->CRL &= (uint16_t)~RTC_CRL_ALRF;
    }
    if (EXTI->PR & (1u << 17)) {
        EXTI->PR = (1u << 17);
    }
    g_alarm_irq = 1;
}

void RTC_IRQHandler(void) {
    alarm_isr();
}

void RTCAlarm_IRQHandler(void) {
    alarm_isr();
}

void rtc_alarm_schedule(int enable, int hour, int minute) {
    if (!g_ok) {
        return;
    }
    if (!enable) {
        if (wait_rtoff()) {
            RTC->CRH &= (uint16_t)~RTC_CRH_ALRIE;
        }
        clear_alarm_flag();
        return;
    }
    if (hour < 0) {
        hour = 0;
    }
    if (hour > 23) {
        hour = 23;
    }
    if (minute < 0) {
        minute = 0;
    }
    if (minute > 59) {
        minute = 59;
    }
    uint32_t now = rtc_epoch();
    uint32_t day = now - (now % 86400u);
    uint32_t target = day + (uint32_t)hour * 3600u + (uint32_t)minute * 60u;
    /* Совпадение в текущую минуту уже отыграло: следующий заход — завтра. */
    if (target <= now + 45u) {
        target += 86400u;
    }
    config_begin();
    RTC->ALRH = (uint16_t)(target >> 16);
    RTC->ALRL = (uint16_t)target;
    RTC->CRH |= RTC_CRH_ALRIE;
    config_end();
    clear_alarm_flag();
}
