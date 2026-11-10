/**
 * original author:  Tilen Majerle<tilen@majerle.eu>
 * modification for STM32f10x: Alexander Lutsai<s.lyra@ya.ru>

   ----------------------------------------------------------------------
   	Copyright (C) Alexander Lutsai, 2016
    Copyright (C) Tilen Majerle, 2015
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.
     
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
 */
#ifndef OLED_SPI_H
#define OLED_SPI_H 100

//****Driver OLED****
#define SSD1306_OLED
//#define SSD1327_OLED
//*******************

#include "fonts_oled_spi.h"
#include "stm32f10x.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
//#include "stdlib.h"
//#include "string.h"

void ssd1327_SPI_Init();

/**
 * @brief  Writes single byte to slave
 * @param  *SPIx: SPI used
 * @param  address: 7 bit slave address, left aligned, bits 7:1 are used, LSB bit is not used
 * @param  reg: register to write to
 * @param  data: data to be written
 * @retval None
 */
void ssd1327_SPI_Write(uint8_t reg, uint8_t data);

/**
 * @brief  Writes multi bytes to slave
 * @param  *SPIx: SPI used
 * @param  address: 7 bit slave address, left aligned, bits 7:1 are used, LSB bit is not used
 * @param  reg: register to write to
 * @param  *data: pointer to data array to write it to slave
 * @param  count: how many bytes will be written
 * @retval None
 */
void ssd1327_SPI_WriteMulti(uint8_t *data, uint16_t count);

/**
 * @brief  Writes to slave
 * @param  *SPIx: SPI used
 * @param  data: data to be sent
 * @retval None
 * @note   For private use
 */
void ssd1327_SPI_WriteData(uint8_t data);

// end SPI

/* SPI settings */
#ifndef SSD1327_SPI
#define SSD1327_SPI              SPI2
#endif

/* SSD1327 settings */
/* SSD1327 width in pixels */
#ifndef SSD1327_WIDTH
#define SSD1327_WIDTH            128
#endif
/* SSD1327 LCD height in pixels */
#ifndef SSD1327_HEIGHT
#define SSD1327_HEIGHT           128
#endif

/**
 * @brief  SSD1327 color enumeration
 */
typedef enum {
	SSD1327_COLOR_BLACK = 0x00, /*!< Black color, no pixel */
	SSD1327_COLOR_WHITE = 0x01  /*!< Pixel is set. Color depends on LCD */
} SSD1327_COLOR_t;



/**
 * @brief  Initializes SSD1327 LCD
 * @param  None
 * @retval Initialization status:
 *           - 0: LCD was not detected on SPI port
 *           - > 0: LCD initialized OK and ready to use
 */
uint8_t SSD1327_Init(void);

void SSD1327_Init_SPI2(void);

/** 
 * @brief  Updates buffer from internal RAM to LCD
 * @note   This function must be called each time you do some changes to LCD, to update buffer from RAM to LCD
 * @param  None
 * @retval None
 */
void SSD1327_UpdateScreen(void);

/**
 * @brief  Toggles pixels invertion inside internal RAM
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  None
 * @retval None
 */
void SSD1327_ToggleInvert(void);

/** 
 * @brief  Fills entire LCD with desired color
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  Color: Color to be used for screen fill. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval None
 */
void SSD1327_Fill(SSD1327_COLOR_t Color);

/**
 * @brief  Draws pixel at desired location
 * @note   @ref SSD1327_UpdateScreen() must called after that in order to see updated LCD screen
 * @param  x: X location. This parameter can be a value between 0 and SSD1327_WIDTH - 1
 * @param  y: Y location. This parameter can be a value between 0 and SSD1327_HEIGHT - 1
 * @param  color: Color to be used for screen fill. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval None
 */
void SSD1327_DrawPixel(uint16_t x, uint16_t y, SSD1327_COLOR_t color);

/**
 * @brief  Sets cursor pointer to desired location for strings
 * @param  x: X location. This parameter can be a value between 0 and SSD1327_WIDTH - 1
 * @param  y: Y location. This parameter can be a value between 0 and SSD1327_HEIGHT - 1
 * @retval None
 */
void SSD1327_GotoXY(uint16_t x, uint16_t y);

/**
 * @brief  Puts character to internal RAM
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  ch: Character to be written
 * @param  *Font: Pointer to @ref FontDef_t structure with used font
 * @param  color: Color used for drawing. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval Character written
 */
char SSD1327_Putc(char ch, FontDef_t* Font, SSD1327_COLOR_t color);

/**
 * @brief  Puts string to internal RAM
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  *str: String to be written
 * @param  *Font: Pointer to @ref FontDef_t structure with used font
 * @param  color: Color used for drawing. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval Zero on success or character value when function failed
 */
char SSD1327_Puts(char* str, FontDef_t* Font, SSD1327_COLOR_t color);

/**
 * @brief  Draws line on LCD
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x0: Line X start point. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y0: Line Y start point. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  x1: Line X end point. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y1: Line Y end point. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval None
 */
void SSD1327_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1327_COLOR_t c);

/**
 * @brief  Draws rectangle on LCD
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: Top left X start point. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y: Top left Y start point. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  w: Rectangle width in units of pixels
 * @param  h: Rectangle height in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval None
 */
void SSD1327_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1327_COLOR_t c);

/**
 * @brief  Draws filled rectangle on LCD
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: Top left X start point. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y: Top left Y start point. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  w: Rectangle width in units of pixels
 * @param  h: Rectangle height in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval None
 */
void SSD1327_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1327_COLOR_t c);

/**
 * @brief  Draws triangle on LCD
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x1: First coordinate X location. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y1: First coordinate Y location. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  x2: Second coordinate X location. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y2: Second coordinate Y location. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  x3: Third coordinate X location. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y3: Third coordinate Y location. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval None
 */
void SSD1327_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1327_COLOR_t color);

/**
 * @brief  Draws circle to STM buffer
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: X location for center of circle. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y: Y location for center of circle. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  r: Circle radius in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval None
 */
void SSD1327_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1327_COLOR_t c);

/**
 * @brief  Draws filled circle to STM buffer
 * @note   @ref SSD1327_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: X location for center of circle. Valid input is 0 to SSD1327_WIDTH - 1
 * @param  y: Y location for center of circle. Valid input is 0 to SSD1327_HEIGHT - 1
 * @param  r: Circle radius in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSD1327_COLOR_t enumeration
 * @retval None
 */
void SSD1327_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1327_COLOR_t c);

void SSD1327_PutInt(sc32 var, FontDef_t* Font, SSD1327_COLOR_t color);

void SSD1327_PutFloat(double var, FontDef_t* Font, SSD1327_COLOR_t color);

void SSD1327_PutFloat_fix(double var, u8 z, FontDef_t* Font, SSD1327_COLOR_t color);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
