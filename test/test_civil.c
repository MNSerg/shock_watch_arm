#include <stdio.h>
#include "../src/time_civil.c"

static int fail;

static void expect_eq(const char *name, int got, int want) {
    if (got != want) {
        printf("%s: got %d want %d\n", name, got, want);
        fail = 1;
    }
}

int main(void) {
    Civil now;
    expect_eq("epoch0", (int)epoch_from_civil(2000, 1, 1, 0, 0, 0), 0);
    civil_from_epoch(0u, &now);
    expect_eq("y", now.year, 2000);
    expect_eq("m", now.month, 1);
    expect_eq("d", now.day, 1);
    expect_eq("wd", now.wday, 6);

    uint32_t epoch = epoch_from_civil(2026, 11, 10, 15, 4, 5);
    civil_from_epoch(epoch, &now);
    expect_eq("y26", now.year, 2026);
    expect_eq("m11", now.month, 11);
    expect_eq("d10", now.day, 10);
    expect_eq("h", now.hour, 15);
    expect_eq("mi", now.minute, 4);
    expect_eq("s", now.second, 5);
    expect_eq("tue", now.wday, 2);

    epoch = epoch_from_civil(2000, 2, 29, 0, 0, 0);
    civil_from_epoch(epoch, &now);
    expect_eq("leap d", now.day, 29);
    expect_eq("leap m", now.month, 2);
    expect_eq("dim", days_in_month(2000, 2), 29);
    expect_eq("dim2100", days_in_month(2100, 2), 28);
    expect_eq("dim2026", days_in_month(2026, 11), 30);

    for (int year = 2000; year <= 2099; year++) {
        for (int month = 1; month <= 12; month++) {
            int dim = days_in_month(year, month);
            for (int day = 1; day <= dim; day++) {
                uint32_t value = epoch_from_civil(year, month, day, 3, 4, 5);
                civil_from_epoch(value, &now);
                if (now.year != year || now.month != month || now.day != day
                    || now.hour != 3 || now.minute != 4 || now.second != 5) {
                    printf("roundtrip %d-%d-%d\n", year, month, day);
                    return 1;
                }
            }
        }
    }
    if (fail) {
        return 1;
    }
    puts("civil ok");
    return 0;
}
