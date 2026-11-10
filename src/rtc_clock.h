#ifndef RTC_CLOCK_H
#define RTC_CLOCK_H

#include <stdint.h>

void rtc_init(uint32_t fallback_epoch);
int rtc_ok(void);
int rtc_uses_lse(void);
uint32_t rtc_epoch(void);
void rtc_set_epoch(uint32_t epoch);
void rtc_irq_init(void);
int rtc_take_boot_alarm(void);
int rtc_consume_irq(void);
void rtc_alarm_schedule(int enable, int hour, int minute);

#endif
