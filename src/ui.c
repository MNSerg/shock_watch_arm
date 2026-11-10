#include "ui.h"
#include "assets.h"
#include "board.h"
#include "lcd.h"

/* Циферблат нарочно пустой: крупные цифры, 12 тихих точек и одна
   точка секунд. Меню и редакторы — список с одной акцентной полосой.
   Подписи латинские: в знакоместе 5x7 нет кириллицы. */

static const char *const WDAY_NAME[7] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};
static const char *const MONTH_NAME[12] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

static const uint8_t *glyph_rows(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 32);
    }
    if (ch < 32 || ch > 'Z') {
        ch = ' ';
    }
    return font5x7[(unsigned)(ch - 32)];
}

static void glyph(int x, int y, char ch, int scale, uint16_t fg, uint16_t bg) {
    const uint8_t *rows = glyph_rows(ch);
    lcd_stream_begin(x, y, 5 * scale, 7 * scale);
    for (int row = 0; row < 7; row++) {
        uint8_t bits = rows[row];
        for (int sy = 0; sy < scale; sy++) {
            for (int col = 0; col < 5; col++) {
                uint16_t color = (bits & (uint8_t)(0x10u >> col)) ? fg : bg;
                for (int sx = 0; sx < scale; sx++) {
                    lcd_stream_push(color);
                }
            }
        }
    }
    lcd_stream_end();
}

static int text_at(int x, int y, const char *s, int scale, uint16_t fg, uint16_t bg) {
    for (int i = 0; s[i] != 0; i++) {
        if (i != 0) {
            lcd_fill_rect(x, y, scale, 7 * scale, bg);
            x += scale;
        }
        glyph(x, y, s[i], scale, fg, bg);
        x += 5 * scale;
    }
    return x;
}

static int text_width(const char *s, int scale) {
    int n = 0;
    while (s[n] != 0) {
        n++;
    }
    if (n == 0) {
        return 0;
    }
    return n * 5 * scale + (n - 1) * scale;
}

static void text_center(int y, const char *s, int scale, uint16_t fg, uint16_t bg) {
    int x = (LCD_W - text_width(s, scale)) / 2;
    if (x < 0) {
        x = 0;
    }
    text_at(x, y, s, scale, fg, bg);
}

static void hint_line(int y, const char *s) {
    lcd_fill_rect(24, y - 1, 192, 10, COL_BG);
    text_center(y, s, 1, COL_DIM, COL_BG);
}

static char *put_s(char *p, const char *s) {
    while (*s) {
        *p++ = *s++;
    }
    *p = 0;
    return p;
}

static char *put_u2(char *p, int value) {
    if (value < 0) {
        value = 0;
    }
    if (value > 99) {
        value = 99;
    }
    *p++ = (char)('0' + value / 10);
    *p++ = (char)('0' + value % 10);
    *p = 0;
    return p;
}

static uint16_t edit_color(const Ui *ui, int field) {
    if (ui->field != field) {
        return COL_FG;
    }
    return ui->blink ? COL_ACCENT : COL_DIM;
}

static void display_hour(const Ui *ui, int *hour, const char **ampm) {
    *ampm = 0;
    if (!ui->hour12 || ui->hour < 0) {
        *hour = ui->hour;
        return;
    }
    if (ui->hour == 0) {
        *hour = 12;
        *ampm = "AM";
    } else if (ui->hour < 12) {
        *hour = ui->hour;
        *ampm = "AM";
    } else if (ui->hour == 12) {
        *hour = 12;
        *ampm = "PM";
    } else {
        *hour = ui->hour - 12;
        *ampm = "PM";
    }
}

static void draw_colon(int x, int y, int scale, uint16_t fg, uint16_t bg) {
    int height = 7 * scale;
    int dot = (scale >= 5) ? 4 : 3;
    lcd_fill_rect(x, y, UI_TIME_COLON, height, bg);
    int cx = x + (UI_TIME_COLON - dot) / 2;
    lcd_fill_rect(cx, y + height / 4 - dot / 2, dot, dot, fg);
    lcd_fill_rect(cx, y + (3 * height) / 4 - dot / 2, dot, dot, fg);
}

static void draw_hhmm(int x, int y, int scale, int hour, int minute,
                      uint16_t hour_color, uint16_t minute_color, uint16_t bg) {
    int cw = 5 * scale;
    int gap = (scale >= 6) ? UI_TIME_GAP : 4;
    int colon = (scale >= 6) ? UI_TIME_COLON : 10;
    if (hour < 0) {
        hour = 0;
    }
    glyph(x, y, (char)('0' + (hour / 10) % 10), scale, hour_color, bg);
    x += cw + gap;
    glyph(x, y, (char)('0' + hour % 10), scale, hour_color, bg);
    x += cw + gap;
    if (scale >= 6) {
        draw_colon(x, y, scale, hour_color, bg);
        x += colon + gap;
    } else {
        glyph(x, y, ':', scale, hour_color, bg);
        x += cw + gap;
    }
    glyph(x, y, (char)('0' + (minute / 10) % 10), scale, minute_color, bg);
    x += cw + gap;
    glyph(x, y, (char)('0' + minute % 10), scale, minute_color, bg);
}

static void battery_icon(int percent) {
    int x = UI_BAT_X + 1;
    int y = UI_BAT_Y + 1;
    lcd_fill_rect(UI_BAT_X, UI_BAT_Y, UI_BAT_W, UI_BAT_H, COL_BG);
    lcd_fill_rect(x, y, 26, 1, COL_FG);
    lcd_fill_rect(x, y + 11, 26, 1, COL_FG);
    lcd_fill_rect(x, y, 1, 12, COL_FG);
    lcd_fill_rect(x + 25, y, 1, 12, COL_FG);
    lcd_fill_rect(x + 26, y + 3, 2, 6, COL_FG);
    if (percent < 0) {
        return;
    }
    int fill = percent * 22 / 100;
    if (fill > 0) {
        uint16_t color = (percent <= 15) ? COL_WARN : COL_FG;
        lcd_fill_rect(x + 2, y + 2, fill, 8, color);
    }
}

static int face_ready;
static int seen_hour = -3;
static int seen_min = -1;
static int seen_sec = -1;
static int seen_date = -1;
static int seen_pct = -1000;
static int seen_alarm = -1;
static int seen_timer = -1;
static int seen_h12 = -1;

void ui_face(const Ui *ui, int full) {
    if (full || !face_ready) {
        lcd_clear(COL_BG);
        for (int i = 0; i < 12; i++) {
            lcd_fill_circle(UI_CX + dial_hour_dx[i], UI_CY + dial_hour_dy[i],
                            UI_HOUR_DOT, COL_TICK);
        }
        text_at(UI_HINT_X, UI_HINT_Y, "B MENU", 1, COL_DIM, COL_BG);
        face_ready = 1;
        seen_hour = -3;
        seen_min = -1;
        seen_sec = -1;
        seen_date = -1;
        seen_pct = -1000;
        seen_alarm = -1;
        seen_timer = -1;
        seen_h12 = -1;
    }

    int show_hour = 0;
    const char *ampm = 0;
    display_hour(ui, &show_hour, &ampm);
    if (ui->hour < 0) {
        if (seen_hour != -2) {
            lcd_fill_rect(UI_TIME_X, UI_TIME_Y, UI_TIME_W, UI_TIME_H, COL_BG);
            text_center(UI_TIME_Y + 12, "NO RTC", 2, COL_WARN, COL_BG);
            seen_hour = -2;
            seen_min = -1;
        }
    } else if (show_hour != seen_hour || ui->minute != seen_min || ui->hour12 != seen_h12) {
        if (seen_hour == -2) {
            lcd_fill_rect(UI_TIME_X, UI_TIME_Y, UI_TIME_W, UI_TIME_H, COL_BG);
        }
        draw_hhmm(UI_TIME_X, UI_TIME_Y, UI_TIME_SCALE, show_hour, ui->minute,
                  COL_FG, COL_FG, COL_BG);
        lcd_fill_rect(UI_AM_X, UI_AM_Y, UI_AM_W, UI_MARK_H, COL_BG);
        if (ampm != 0) {
            text_at(UI_AM_X, UI_AM_Y, ampm, 2, COL_DIM, COL_BG);
        }
        seen_hour = show_hour;
        seen_min = ui->minute;
        seen_h12 = ui->hour12;
    }

    if (ui->alarm_on != seen_alarm || ui->timer_run != seen_timer) {
        lcd_fill_rect(UI_MARK_X, UI_MARK_Y, UI_MARK_W, UI_MARK_H, COL_BG);
        const char *mark = 0;
        if (ui->alarm_on && ui->timer_run) {
            mark = "AL T";
        } else if (ui->alarm_on) {
            mark = "AL";
        } else if (ui->timer_run) {
            mark = "T";
        }
        if (mark != 0) {
            text_at(UI_MARK_X, UI_MARK_Y, mark, 2, COL_ACCENT, COL_BG);
        }
        seen_alarm = ui->alarm_on;
        seen_timer = ui->timer_run;
    }

    int date_key = ui->year * 512 + ui->month * 32 + ui->day;
    if (date_key != seen_date && ui->hour >= 0) {
        char line[11];
        int wday = ui->wday;
        int month = ui->month;
        if (wday < 0 || wday > 6) {
            wday = 0;
        }
        if (month < 1 || month > 12) {
            month = 1;
        }
        const char *wd = WDAY_NAME[wday];
        const char *mo = MONTH_NAME[month - 1];
        line[0] = wd[0];
        line[1] = wd[1];
        line[2] = wd[2];
        line[3] = ' ';
        line[4] = (char)('0' + (ui->day / 10) % 10);
        line[5] = (char)('0' + ui->day % 10);
        line[6] = ' ';
        line[7] = mo[0];
        line[8] = mo[1];
        line[9] = mo[2];
        line[10] = 0;
        text_at(UI_DATE_X, UI_DATE_Y, line, UI_DATE_SCALE, COL_DIM, COL_BG);
        seen_date = date_key;
    }

    if (ui->bat_pct != seen_pct) {
        battery_icon(ui->bat_pct);
        seen_pct = ui->bat_pct;
    }

    if (ui->hour >= 0) {
        int sec = ui->second % 60;
        if (sec < 0) {
            sec = 0;
        }
        if (sec != seen_sec) {
            if (seen_sec >= 0 && seen_sec < 60) {
                lcd_fill_circle(UI_CX + dial_sec_dx[seen_sec],
                                UI_CY + dial_sec_dy[seen_sec], UI_SEC_DOT, COL_BG);
            }
            lcd_fill_circle(UI_CX + dial_sec_dx[sec], UI_CY + dial_sec_dy[sec],
                            UI_SEC_DOT, COL_ACCENT);
            seen_sec = sec;
        }
    }
}

static void status_row(int y, const char *text) {
    lcd_fill_rect(28, y, 184, 16, COL_BG);
    text_at(36, y, text, 2, COL_FG, COL_BG);
}

void ui_status(const Ui *ui, int full) {
    char line[22];
    char *p;
    if (full) {
        lcd_clear(COL_BG);
        text_center(36, "STATUS", 2, COL_DIM, COL_BG);
        hint_line(206, "B BACK");
    }
    p = put_s(line, "BAT ");
    if (ui->bat_mv <= 0) {
        put_s(p, "--");
    } else {
        int volts = ui->bat_mv / 1000;
        int frac = (ui->bat_mv % 1000) / 10;
        if (volts >= 10) {
            *p++ = (char)('0' + (volts / 10) % 10);
        }
        *p++ = (char)('0' + volts % 10);
        *p++ = '.';
        *p++ = (char)('0' + (frac / 10) % 10);
        *p++ = (char)('0' + frac % 10);
        p = put_s(p, "V ");
        if (ui->bat_pct >= 0) {
            if (ui->bat_pct >= 100) {
                p = put_s(p, "100");
            } else {
                p = put_u2(p, ui->bat_pct);
            }
            put_s(p, "%");
        }
    }
    status_row(64, line);

    if (!ui->alarm_on) {
        status_row(88, "ALM OFF");
    } else {
        p = put_s(line, "ALM ");
        p = put_u2(p, ui->alarm_h);
        p = put_s(p, ":");
        p = put_u2(p, ui->alarm_m);
        put_s(p, " ON");
        status_row(88, line);
    }
    status_row(112, ui->hour < 0 ? "RTC FAIL" : (ui->rtc_lse ? "RTC LSE" : "RTC LSI"));
    p = put_s(line, "CPU ");
    if (ui->cpu_mhz >= 10) {
        p = put_u2(p, ui->cpu_mhz);
    } else {
        *p++ = (char)('0' + (ui->cpu_mhz < 0 ? 0 : ui->cpu_mhz));
        *p = 0;
    }
    put_s(p, " MHZ");
    status_row(136, line);
    status_row(160, ui->mem_ok ? "MEM OK" : "MEM FAIL");
    p = put_s(line, "SLP ");
    p = put_u2(p, ui->sleep_s);
    p = put_s(p, "s  ");
    put_s(p, ui->hour12 ? "12H" : "24H");
    status_row(184, line);
}

void ui_menu(const Ui *ui, int full) {
    static const char *const items[4] = {"ALARM", "TIMER", "CLOCK", "POWER"};
    if (full) {
        lcd_clear(COL_BG);
        text_center(36, "MENU", 2, COL_DIM, COL_BG);
        hint_line(188, "A OPEN   B NEXT");
        hint_line(206, "HOLD B HOME");
    }
    for (int i = 0; i < 4; i++) {
        int y = 72 + i * 28;
        int selected = (i == ui->sel);
        lcd_fill_rect(40, y - 4, 160, 24, selected ? COL_SLOT : COL_BG);
        if (selected) {
            lcd_fill_rect(40, y - 4, 3, 24, COL_ACCENT);
        }
        text_at(64, y, items[i], 2, selected ? COL_FG : COL_DIM,
                selected ? COL_SLOT : COL_BG);
    }
}

void ui_alarm(const Ui *ui, int full) {
    if (full) {
        lcd_clear(COL_BG);
        text_center(36, "ALARM", 2, COL_DIM, COL_BG);
        hint_line(188, "A +    B NEXT");
        hint_line(206, "HOLD B SAVE");
    }
    draw_hhmm(UI_TIME_X, 96, UI_TIME_SCALE, ui->alarm_h, ui->alarm_m,
              edit_color(ui, 0), edit_color(ui, 1), COL_BG);
    lcd_fill_rect(70, 156, 100, 20, COL_BG);
    text_center(156, ui->alarm_on ? "ON" : "OFF", 2, edit_color(ui, 2), COL_BG);
}

static void timer_digits(uint32_t timer_ms) {
    uint32_t tenths_total = timer_ms / 100u;
    int tenth = (int)(tenths_total % 10u);
    int sec = (int)((tenths_total / 10u) % 60u);
    int minute = (int)((tenths_total / 600u) % 100u);
    const int scale = 5;
    const int cw = 5 * scale;
    const int gap = 3;
    int x = 28;
    int y = 96;
    glyph(x, y, (char)('0' + minute / 10), scale, COL_FG, COL_BG);
    x += cw + gap;
    glyph(x, y, (char)('0' + minute % 10), scale, COL_FG, COL_BG);
    x += cw + gap;
    glyph(x, y, ':', scale, COL_FG, COL_BG);
    x += cw + gap;
    glyph(x, y, (char)('0' + sec / 10), scale, COL_FG, COL_BG);
    x += cw + gap;
    glyph(x, y, (char)('0' + sec % 10), scale, COL_FG, COL_BG);
    x += cw + gap;
    glyph(x, y, '.', scale, COL_ACCENT, COL_BG);
    x += cw + gap;
    glyph(x, y, (char)('0' + tenth), scale, COL_ACCENT, COL_BG);
}

void ui_timer(const Ui *ui, int full) {
    if (full) {
        lcd_clear(COL_BG);
        text_center(40, "TIMER", 2, COL_DIM, COL_BG);
    }
    timer_digits(ui->timer_ms);
    hint_line(188, ui->timer_run ? "A STOP   B ZERO" : "A START  B ZERO");
    hint_line(206, "HOLD B BACK");
}

static int put_token(int x, int y, const char *s, int scale, uint16_t fg) {
    return text_at(x, y, s, scale, fg, COL_BG);
}

void ui_clock(const Ui *ui, int full) {
    char year[5];
    char month[3];
    char day[3];
    if (full) {
        lcd_clear(COL_BG);
        text_center(32, "CLOCK", 2, COL_DIM, COL_BG);
        hint_line(188, "A +    B NEXT");
        hint_line(206, "HOLD B SAVE");
    }
    year[0] = (char)('0' + (ui->year / 1000) % 10);
    year[1] = (char)('0' + (ui->year / 100) % 10);
    year[2] = (char)('0' + (ui->year / 10) % 10);
    year[3] = (char)('0' + ui->year % 10);
    year[4] = 0;
    put_u2(month, ui->month);
    put_u2(day, ui->day);
    int x = 50;
    x = put_token(x, 70, year, 2, edit_color(ui, 0));
    lcd_fill_rect(x, 70, 2, 14, COL_BG);
    x += 2;
    x = put_token(x, 70, "-", 2, COL_DIM);
    lcd_fill_rect(x, 70, 2, 14, COL_BG);
    x += 2;
    x = put_token(x, 70, month, 2, edit_color(ui, 1));
    lcd_fill_rect(x, 70, 2, 14, COL_BG);
    x += 2;
    x = put_token(x, 70, "-", 2, COL_DIM);
    lcd_fill_rect(x, 70, 2, 14, COL_BG);
    x += 2;
    put_token(x, 70, day, 2, edit_color(ui, 2));

    draw_hhmm(46, 108, 5, ui->hour < 0 ? 0 : ui->hour, ui->minute,
              edit_color(ui, 3), edit_color(ui, 4), COL_BG);
    lcd_fill_rect(90, 160, 60, 16, COL_BG);
    text_center(160, ui->hour12 ? "12H" : "24H", 2, edit_color(ui, 5), COL_BG);
}

void ui_power(const Ui *ui, int full) {
    char sleep_line[16];
    char *p = put_s(sleep_line, "SLEEP ");
    p = put_u2(p, ui->sleep_s);
    put_s(p, "s");
    const char *items[3];
    items[0] = "STOP";
    items[1] = "STANDBY";
    items[2] = sleep_line;
    const char *hints[3] = {
        "RAM STAYS, BOTH BTNS",
        "WAKE ONLY PA0",
        "A CHANGES DELAY"
    };
    if (full) {
        lcd_clear(COL_BG);
        text_center(36, "POWER", 2, COL_DIM, COL_BG);
        hint_line(206, "HOLD B BACK");
    }
    for (int i = 0; i < 3; i++) {
        int y = 78 + i * 32;
        int selected = (i == ui->sel);
        lcd_fill_rect(36, y - 4, 168, 24, selected ? COL_SLOT : COL_BG);
        if (selected) {
            lcd_fill_rect(36, y - 4, 3, 24, COL_ACCENT);
        }
        text_at(56, y, items[i], 2, selected ? COL_FG : COL_DIM,
                selected ? COL_SLOT : COL_BG);
    }
    hint_line(184, hints[ui->sel < 0 || ui->sel > 2 ? 0 : ui->sel]);
}

void ui_ring(const Ui *ui, int full) {
    if (full) {
        lcd_clear(COL_BG);
        text_center(48, "ALARM", 2, COL_ACCENT, COL_BG);
        hint_line(200, "ANY BUTTON STOPS");
    }
    uint16_t color = ui->blink ? COL_ACCENT : COL_FG;
    draw_hhmm(UI_TIME_X, 100, UI_TIME_SCALE, ui->alarm_h, ui->alarm_m, color, color, COL_BG);
}
