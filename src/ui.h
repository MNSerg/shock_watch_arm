#ifndef UI_H
#define UI_H

#include <stdint.h>

/* Снимок, который экраны только читают. Живые часы и черновик
   редактора сюда подставляет main: рисование не ходит в RTC и EEPROM. */
typedef struct {
    int year, month, day, wday;
    int hour, minute, second;
    int hour12;
    int alarm_on;
    int alarm_h, alarm_m;
    int timer_run;
    uint32_t timer_ms;
    int bat_mv;
    int bat_pct;
    int sleep_s;
    int rtc_lse;
    int cpu_mhz;
    int mem_ok;
    int sel;
    int field;
    int blink;
} Ui;

void ui_face(const Ui *ui, int full);
void ui_status(const Ui *ui, int full);
void ui_menu(const Ui *ui, int full);
void ui_alarm(const Ui *ui, int full);
void ui_timer(const Ui *ui, int full);
void ui_clock(const Ui *ui, int full);
void ui_power(const Ui *ui, int full);
void ui_ring(const Ui *ui, int full);

#endif
