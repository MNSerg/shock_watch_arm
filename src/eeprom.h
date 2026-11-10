#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>

typedef struct {
    uint8_t hour12;
    uint8_t alarm_on;
    uint8_t alarm_h;
    uint8_t alarm_m;
    uint8_t sleep_s;
} Settings;

void settings_default(Settings *s);
void eeprom_init(void);
int eeprom_load(Settings *s);
int eeprom_save(const Settings *s);
int eeprom_load_epoch(uint32_t *epoch);
int eeprom_save_epoch(uint32_t epoch);

#endif
