#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>

void lcd_io_init(void);
void lcd_panel_init(void);
void lcd_sleep(void);
void lcd_fill_rect(int x, int y, int w, int h, uint16_t color);
void lcd_fill_circle(int cx, int cy, int radius, uint16_t color);
void lcd_clear(uint16_t color);
void lcd_stream_begin(int x, int y, int w, int h);
void lcd_stream_push(uint16_t color);
void lcd_stream_end(void);

#endif
