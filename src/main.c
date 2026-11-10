#include "adc.h"
#include "board.h"
#include "clock.h"
#include "eeprom.h"
#include "input.h"
#include "lcd.h"
#include "pins.h"
#include "power.h"
#include "rtc_clock.h"
#include "time_civil.h"
#include "ui.h"

/* Наручные часы.
   Лицо: короткое B — статус, долгое B — меню, долгое A — Stop.
   В меню короткое A открывает пункт, короткое B листает, долгое B возвращает.
   Stop гасит панель и оставляет RTC; Standby доступен только из POWER
   и после него программа стартует заново. Будильник будит оба этих режима. */

static Settings set;
static int screen = SCR_FACE;
static int built = -1;
static int sel;
static int field;
static int dirty = 1;
static int mem_ok;
static int bat_mv;
static uint32_t bat_at;
static uint32_t idle_at;
static int ring;
static uint32_t ring_at;
static int blink;
static int timer_run;
static uint32_t timer_acc;
static uint32_t timer_mark;
static uint32_t frame_at;
static int ed_y, ed_mo, ed_d, ed_h, ed_mi, ed_h12;
static int ed_ah, ed_am, ed_aon;

static uint32_t timer_elapsed(void) {
    uint32_t value = timer_acc;
    if (timer_run) {
        value += millis() - timer_mark;
    }
    return value;
}

static void note_activity(void) {
    idle_at = millis();
}

static void enter_stop(void) {
    lcd_sleep();
    buttons_suppress_until_release();
    power_enter_stop();
    buttons_suppress_until_release();
    lcd_panel_init();
    built = -1;
    dirty = 1;
    note_activity();
}

static void enter_standby(void) {
    (void)eeprom_save(&set);
    if (rtc_ok()) {
        (void)eeprom_save_epoch(rtc_epoch());
    }
    rtc_alarm_schedule(set.alarm_on, (int)set.alarm_h, (int)set.alarm_m);
    lcd_sleep();
    power_enter_standby();
    lcd_panel_init();
    built = -1;
    dirty = 1;
    note_activity();
}

static void remember_settings(void) {
    if (eeprom_save(&set) == 0) {
        mem_ok = 1;
    } else {
        mem_ok = 0;
    }
}

static void clamp_edit_day(void) {
    int dim = days_in_month(ed_y, ed_mo);
    if (ed_d > dim) {
        ed_d = dim;
    }
    if (ed_d < 1) {
        ed_d = 1;
    }
}

static void load_clock_edit(void) {
    if (rtc_ok()) {
        Civil now;
        civil_from_epoch(rtc_epoch(), &now);
        ed_y = now.year;
        ed_mo = now.month;
        ed_d = now.day;
        ed_h = now.hour;
        ed_mi = now.minute;
    } else {
        ed_y = 2026;
        ed_mo = 11;
        ed_d = 10;
        ed_h = 12;
        ed_mi = 0;
    }
    ed_h12 = set.hour12 ? 1 : 0;
    field = 0;
}

static void save_clock_edit(void) {
    clamp_edit_day();
    uint32_t epoch = epoch_from_civil(ed_y, ed_mo, ed_d, ed_h, ed_mi, 0);
    rtc_set_epoch(epoch);
    (void)eeprom_save_epoch(epoch);
    set.hour12 = (uint8_t)(ed_h12 ? 1 : 0);
    remember_settings();
    rtc_alarm_schedule(set.alarm_on, (int)set.alarm_h, (int)set.alarm_m);
}

static void bump_clock_field(void) {
    switch (field) {
    case 0:
        ed_y = (ed_y >= 2099) ? 2000 : (ed_y + 1);
        clamp_edit_day();
        break;
    case 1:
        ed_mo = (ed_mo >= 12) ? 1 : (ed_mo + 1);
        clamp_edit_day();
        break;
    case 2: {
        int dim = days_in_month(ed_y, ed_mo);
        ed_d = (ed_d >= dim) ? 1 : (ed_d + 1);
        break;
    }
    case 3:
        ed_h = (ed_h + 1) % 24;
        break;
    case 4:
        ed_mi = (ed_mi + 1) % 60;
        break;
    default:
        ed_h12 ^= 1;
        break;
    }
}

static void save_alarm_edit(void) {
    set.alarm_h = (uint8_t)ed_ah;
    set.alarm_m = (uint8_t)ed_am;
    set.alarm_on = (uint8_t)(ed_aon ? 1 : 0);
    remember_settings();
    rtc_alarm_schedule(set.alarm_on, ed_ah, ed_am);
}

static void open_menu_item(int index) {
    sel = index;
    field = 0;
    if (index == 0) {
        ed_ah = (int)set.alarm_h;
        ed_am = (int)set.alarm_m;
        ed_aon = set.alarm_on ? 1 : 0;
        screen = SCR_ALARM;
    } else if (index == 1) {
        screen = SCR_TIMER;
    } else if (index == 2) {
        load_clock_edit();
        screen = SCR_CLOCK;
    } else {
        sel = 0;
        screen = SCR_POWER;
    }
}

static void begin_ring(void) {
    ring = 1;
    screen = SCR_RING;
    ring_at = millis();
    built = -1;
    dirty = 1;
    rtc_alarm_schedule(set.alarm_on, (int)set.alarm_h, (int)set.alarm_m);
}

static void dismiss_ring(void) {
    ring = 0;
    screen = SCR_FACE;
    built = -1;
    dirty = 1;
    led_set(0);
    rtc_alarm_schedule(set.alarm_on, (int)set.alarm_h, (int)set.alarm_m);
    note_activity();
}

static void on_button(uint8_t event) {
    note_activity();
    if (ring) {
        dismiss_ring();
        return;
    }
    if (event == BTN1_LONG) {
        enter_stop();
        return;
    }
    switch (screen) {
    case SCR_FACE:
        if (event == BTN2_SHORT) {
            screen = SCR_STATUS;
        } else if (event == BTN2_LONG) {
            sel = 0;
            screen = SCR_MENU;
        }
        break;
    case SCR_STATUS:
        if (event == BTN1_SHORT || event == BTN2_SHORT) {
            screen = SCR_FACE;
        } else if (event == BTN2_LONG) {
            sel = 0;
            screen = SCR_MENU;
        }
        break;
    case SCR_MENU:
        if (event == BTN2_LONG) {
            screen = SCR_FACE;
        } else if (event == BTN2_SHORT) {
            sel = (sel + 1) % 4;
        } else if (event == BTN1_SHORT) {
            open_menu_item(sel);
        }
        break;
    case SCR_ALARM:
        if (event == BTN2_LONG) {
            save_alarm_edit();
            screen = SCR_MENU;
        } else if (event == BTN2_SHORT) {
            field = (field + 1) % 3;
        } else if (event == BTN1_SHORT) {
            if (field == 0) {
                ed_ah = (ed_ah + 1) % 24;
            } else if (field == 1) {
                ed_am = (ed_am + 1) % 60;
            } else {
                ed_aon ^= 1;
            }
        }
        break;
    case SCR_TIMER:
        if (event == BTN2_LONG) {
            screen = SCR_MENU;
        } else if (event == BTN1_SHORT) {
            if (timer_run) {
                timer_acc += millis() - timer_mark;
                timer_run = 0;
            } else {
                timer_mark = millis();
                timer_run = 1;
            }
        } else if (event == BTN2_SHORT) {
            timer_run = 0;
            timer_acc = 0u;
        }
        break;
    case SCR_CLOCK:
        if (event == BTN2_LONG) {
            save_clock_edit();
            screen = SCR_MENU;
        } else if (event == BTN2_SHORT) {
            field = (field + 1) % 6;
        } else if (event == BTN1_SHORT) {
            bump_clock_field();
        }
        break;
    case SCR_POWER:
        if (event == BTN2_LONG) {
            screen = SCR_MENU;
        } else if (event == BTN2_SHORT) {
            sel = (sel + 1) % 3;
        } else if (event == BTN1_SHORT) {
            if (sel == 0) {
                enter_stop();
            } else if (sel == 1) {
                enter_standby();
            } else {
                static const uint8_t steps[4] = {10u, 20u, 30u, 60u};
                int index = 0;
                while (index < 3 && steps[index] != set.sleep_s) {
                    index++;
                }
                set.sleep_s = steps[(index + 1) % 4];
                remember_settings();
            }
        }
        break;
    default:
        screen = SCR_FACE;
        break;
    }
    dirty = 1;
}

static void fill_ui(Ui *ui) {
    Civil now;
    ui->hour = -1;
    ui->minute = 0;
    ui->second = 0;
    ui->year = 2026;
    ui->month = 11;
    ui->day = 10;
    ui->wday = 2;
    if (rtc_ok()) {
        civil_from_epoch(rtc_epoch(), &now);
        ui->year = now.year;
        ui->month = now.month;
        ui->day = now.day;
        ui->wday = now.wday;
        ui->hour = now.hour;
        ui->minute = now.minute;
        ui->second = now.second;
    }
    ui->hour12 = set.hour12 ? 1 : 0;
    ui->alarm_on = set.alarm_on ? 1 : 0;
    ui->alarm_h = (int)set.alarm_h;
    ui->alarm_m = (int)set.alarm_m;
    ui->timer_run = timer_run;
    ui->timer_ms = timer_elapsed();
    ui->bat_mv = bat_mv;
    ui->bat_pct = adc_battery_pct(bat_mv);
    ui->sleep_s = (int)set.sleep_s;
    ui->rtc_lse = rtc_uses_lse();
    ui->cpu_mhz = (int)(SystemCoreClock / 1000000u);
    ui->mem_ok = mem_ok;
    ui->sel = sel;
    ui->field = field;
    ui->blink = blink;

    if (screen == SCR_ALARM) {
        ui->alarm_h = ed_ah;
        ui->alarm_m = ed_am;
        ui->alarm_on = ed_aon;
    } else if (screen == SCR_CLOCK) {
        ui->year = ed_y;
        ui->month = ed_mo;
        ui->day = ed_d;
        ui->hour = ed_h;
        ui->minute = ed_mi;
        ui->hour12 = ed_h12;
    }
}

static void draw_screen(const Ui *ui) {
    int full = (built != screen);
    if (screen == SCR_FACE) {
        ui_face(ui, full);
        built = SCR_FACE;
        return;
    }
    if (!dirty && !full) {
        return;
    }
    switch (screen) {
    case SCR_STATUS:
        ui_status(ui, full);
        break;
    case SCR_MENU:
        ui_menu(ui, full);
        break;
    case SCR_ALARM:
        ui_alarm(ui, full);
        break;
    case SCR_TIMER:
        ui_timer(ui, full);
        break;
    case SCR_CLOCK:
        ui_clock(ui, full);
        break;
    case SCR_POWER:
        ui_power(ui, full);
        break;
    case SCR_RING:
        ui_ring(ui, full);
        break;
    default:
        ui_face(ui, 1);
        screen = SCR_FACE;
        break;
    }
    built = screen;
    dirty = 0;
}

static void maybe_sleep(void) {
    if (ring || timer_run) {
        return;
    }
    uint32_t limit = (uint32_t)set.sleep_s * 1000u;
    if (limit < 5000u) {
        limit = 5000u;
    }
    if ((millis() - idle_at) >= limit) {
        enter_stop();
    }
}

int main(void) {
    uint32_t fallback;

    (void)power_woke_from_standby();
    pins_prepare();
    buttons_init();
    eeprom_init();
    if (eeprom_load(&set) != 0) {
        settings_default(&set);
        mem_ok = (eeprom_save(&set) == 0);
    } else {
        mem_ok = 1;
    }

    fallback = epoch_from_civil(2026, 11, 10, 12, 0, 0);
    {
        uint32_t stored = 0u;
        if (eeprom_load_epoch(&stored) == 0) {
            fallback = stored;
        }
    }
    rtc_init(fallback);
    adc_init();
    lcd_io_init();
    lcd_panel_init();
    systick_start();
    rtc_irq_init();
    if (rtc_take_boot_alarm() && set.alarm_on) {
        begin_ring();
    } else {
        rtc_alarm_schedule(set.alarm_on, (int)set.alarm_h, (int)set.alarm_m);
    }
    idle_at = millis();
    bat_at = 0u;

    for (;;) {
        uint8_t event = buttons_get();
        if (event != BTN_NONE) {
            on_button(event);
        }
        if (rtc_consume_irq() && set.alarm_on && !ring) {
            begin_ring();
        }
        if (ring && (millis() - ring_at) >= 60000u) {
            dismiss_ring();
        }
        if ((bat_at == 0u) || ((millis() - bat_at) >= 2000u)) {
            bat_at = millis();
            bat_mv = adc_battery_mv();
            if (screen == SCR_STATUS) {
                dirty = 1;
            }
        }
        {
            int phase = (int)((millis() / 400u) & 1u);
            if (phase != blink) {
                blink = phase;
                if (screen == SCR_ALARM || screen == SCR_CLOCK || screen == SCR_RING) {
                    dirty = 1;
                }
            }
        }
        if (screen == SCR_TIMER && timer_run && (millis() - frame_at) >= 100u) {
            frame_at = millis();
            dirty = 1;
        }
        if (ring) {
            led_set(blink);
        }

        {
            Ui view;
            fill_ui(&view);
            draw_screen(&view);
        }
        maybe_sleep();
        cpu_wfi();
    }
}
