#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

void buttons_init(void);
void buttons_suppress_until_release(void);
uint8_t buttons_get(void);
uint32_t millis(void);
void systick_start(void);

#endif
