#ifndef TIME_CIVIL_H
#define TIME_CIVIL_H

#include <stdint.h>

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int wday; /* 0 = воскресенье */
} Civil;

uint32_t epoch_from_civil(int year, int month, int day, int hour, int minute, int second);
void civil_from_epoch(uint32_t epoch, Civil *out);
int days_in_month(int year, int month);

#endif
