#include "time_civil.h"

/* Дни считаются алгоритмом Ховарда Хиннанта.
   Эпоха прошивки — 2000-01-01 00:00:00, это unix-день 10957.
   Счётчик RTC хранит секунды от этой даты и помещается в uint32 до 2136 года. */

static int32_t days_from_civil(int year, int month, int day) {
    year -= (month <= 2);
    const int32_t era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);
    /* Март становится месяцем 0, чтобы високосный день оказался в конце года. */
    const int madj = month + (month > 2 ? -3 : 9);
    const unsigned doy = (unsigned)((153 * madj + 2) / 5 + day - 1);
    const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

static void civil_from_days(int32_t z, int *year, int *month, int *day) {
    z += 719468;
    const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    const int32_t y = (int32_t)yoe + era * 400;
    const unsigned doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    const unsigned mp = (5u * doy + 2u) / 153u;
    const unsigned d = doy - (153u * mp + 2u) / 5u + 1u;
    const unsigned m = (mp < 10u) ? (mp + 3u) : (mp - 9u);
    *year = (int)(y + (m <= 2u));
    *month = (int)m;
    *day = (int)d;
}

int days_in_month(int year, int month) {
    static const uint8_t md[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
        return 30;
    }
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        return 29;
    }
    return md[month - 1];
}

uint32_t epoch_from_civil(int year, int month, int day, int hour, int minute, int second) {
    int32_t days = days_from_civil(year, month, day) - 10957;
    if (days < 0) {
        days = 0;
    }
    return (uint32_t)days * 86400u
        + (uint32_t)hour * 3600u
        + (uint32_t)minute * 60u
        + (uint32_t)second;
}

void civil_from_epoch(uint32_t epoch, Civil *out) {
    uint32_t sod = epoch % 86400u;
    int32_t days = (int32_t)(epoch / 86400u);
    civil_from_days(days + 10957, &out->year, &out->month, &out->day);
    out->hour = (int)(sod / 3600u);
    out->minute = (int)((sod % 3600u) / 60u);
    out->second = (int)(sod % 60u);
    /* 2000-01-01 — суббота, при воскресенье = 0 это 6. */
    out->wday = (int)((days + 6) % 7);
}
