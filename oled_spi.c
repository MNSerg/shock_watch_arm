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
#include "oled_spi.h"

#define RES_1327_On GPIO_ResetBits(GPIOB, GPIO_Pin_12);
#define RES_1327_Off GPIO_SetBits(GPIOB, GPIO_Pin_12);
#define DC_1327_Command GPIO_ResetBits(GPIOA, GPIO_Pin_8);
#define DC_1327_Data GPIO_SetBits(GPIOA, GPIO_Pin_8);
#define CS_1327_On GPIO_ResetBits(GPIOB, GPIO_Pin_3);
#define CS_1327_Off GPIO_SetBits(GPIOB, GPIO_Pin_3);

#define SSD_SPI

/* Write command */
#define SSD1327_WRITECOMMAND(command)      ssd1327_Command((command))
/* Write data */
#define SSD1327_WRITEDATA(data)            ssd1327_Data((data))
/* Absolute value */
#define ABS(x)   ((x) > 0 ? (x) : -(x))

/* SSD1327 data buffer */
static uint8_t SSD1327_Buffer[SSD1327_WIDTH * SSD1327_HEIGHT / 2];

/* Private SSD1327 structure */
typedef struct {
	uint16_t CurrentX;
	uint16_t CurrentY;
	uint8_t Inverted;
	uint8_t Initialized;
} SSD1327_t;

/* Private variable */
static SSD1327_t SSD1327;

void ssd1327_Command(uint8_t Command) {
	DC_1327_Command;
	CS_1327_On;
#ifdef SSD_SPI
	SPI_I2S_SendData(SPI2, Command);
#endif
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)!=SET);
	CS_1327_Off;
}

void SSD1327_Init_SPI2(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;

    /* Configure SPI pins: SCK and MOSI -------------------------------*/
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_15 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    //GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure SPI pins:   CS LMX -------------------------------*/
/*    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
*/
    /* Configure SPI pins: SDO -------------------------------*/
 /*   GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);*/

	SPI_InitTypeDef SPI_InitStructure;

	/* SPI1 Configuration ----------------------------------------------------*/
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;//8
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);

	/* Enable SPI */
	SPI_Cmd(SPI2, ENABLE);
}

void ssd1327_Data(uint8_t Data) {
	DC_1327_Data;
	CS_1327_On;
#ifdef SSD_SPI
	SPI_I2S_SendData(SPI2, Data);
#endif
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)!=SET);
	CS_1327_Off;
}

void ssd1327_SPI_WriteMulti(uint8_t* data, uint16_t count) {
	uint16_t i;
	DC_1327_Data;
	CS_1327_On;
	for (i = 0; i < count; i++) {
#ifdef SSD_SPI
		SPI_I2S_SendData(SPI2, data[i]);
#endif
		while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)!=SET);
	}
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)!=SET);
	CS_1327_Off;
}

/* Private functions */

void ssd1327_SPI_Write(uint8_t reg, uint8_t data) {
	CS_1327_On;
#ifdef SSD_SPI
	SPI_I2S_SendData(SPI2, reg);
	SPI_I2S_SendData(SPI2, data);
#endif
	CS_1327_Off;
}

// end i2c
#define SSD1327_SETCOLUMNADDRESS	0x15
#define SSD1327_SETROWADDRESS		0x75
#define SSD1327_SETCONTRASTCURRENT 	0x81
#define SSD1327_NOP					0x84
#define SSD1327_SEGREMAP			0xA0
#define SSD1327_SETDISPLAYSTARTLINE	0xA1
#define SSD1327_SETDISPLAYOFFSET	0xA2
#define SSD1327_DISPLAYALLON_RESUME 0xA4
#define SSD1327_DISPLAYALLON 		0xA5
#define SSD1327_NORMALDISPLAY 		0xA6
#define SSD1327_INVERTDISPLAY 		0xA7
#define SSD1327_SETMULTIPLEX		0xA8
#define SSD1327_FUNCTIONSELECTIONA	0xAB
#define SSD1327_DISPLAYOFF			0xAE
#define SSD1327_DISPLAYON			0xAF
#define SSD1327_SETPHASELENGTH		0xB1
#define SSD1327_SETFRONTCLOCKDIVIDER_OSCILLATORFREQUENCY 0xB3
#define SSD1327_SETGPIO				0xB5
#define SSD1327_SETSECONDPRECHARGEPERTIOD	0xB6
#define SSD1327_SETGRAYSCALETABLE	0xB8
#define SSD1327_SELECTDEFAULTLINEARGRAYSCALETABLE	0xB9
#define SSD1327_SETPRECHARGEVOLTAGE	0xBC
#define SSD1327_SETSETVCOMVOLTAGE	0xBE
#define SSD1327_FUNCTIONSELECTIONB	0xD5
#define SSD1327_SETCOMMANDLOCK		0xFD

uint8_t SSD1327_Init(void) {
	
	RES_1327_On;
	/* A little delay */
	uint32_t p = 2500;
	while(p>0)
		p--;
	RES_1327_Off;
	
	/* Init LCD */
#ifdef SSD1327_OLED
	ssd1327_Command(SSD1327_DISPLAYOFF);  // Display Off

	ssd1327_Command(SSD1327_SETMULTIPLEX);
	ssd1327_Command(0x7F);

	ssd1327_Command(SSD1327_SETDISPLAYSTARTLINE);
	ssd1327_Command(0x00);

	ssd1327_Command(SSD1327_SETDISPLAYOFFSET);
	ssd1327_Command(0x00);	//0x20

	ssd1327_Command(SSD1327_SEGREMAP);	//here do mirror row and column and rotate display
	ssd1327_Command(0b01010111);	//0x51

	ssd1327_Command(SSD1327_SETCONTRASTCURRENT);	// Set Contrast Control
	ssd1327_Command(0xFF);

	ssd1327_Command(SSD1327_SETPHASELENGTH);
	ssd1327_Command(0x22);

	ssd1327_Command(SSD1327_SETFRONTCLOCKDIVIDER_OSCILLATORFREQUENCY);
	ssd1327_Command(0x50);

	ssd1327_Command(SSD1327_SELECTDEFAULTLINEARGRAYSCALETABLE);

	ssd1327_Command(SSD1327_SETPRECHARGEVOLTAGE);
	ssd1327_Command(0x10);

	ssd1327_Command(SSD1327_SETSETVCOMVOLTAGE);
	ssd1327_Command(0x05);

	ssd1327_Command(SSD1327_SETSECONDPRECHARGEPERTIOD);
	ssd1327_Command(0x0a);

	ssd1327_Command(SSD1327_FUNCTIONSELECTIONB);
	ssd1327_Command(0x62);

	ssd1327_Command(SSD1327_SETCOLUMNADDRESS);
	ssd1327_Command(0x00);
	ssd1327_Command(0x3F);

	ssd1327_Command(SSD1327_SETROWADDRESS);
	ssd1327_Command(0x00);
	ssd1327_Command(0x7F);

	ssd1327_Command(SSD1327_NORMALDISPLAY);  // Set Normal Display

	ssd1327_Command(SSD1327_DISPLAYALLON_RESUME);  // Entire Display ON
	
	ssd1327_Command(SSD1327_DISPLAYON);  // Display ON
#elif defined SSD1306_OLED
	ssd1327_Command(0xAE); //display off
	ssd1327_Command(0x20); //Set Memory Addressing Mode
	ssd1327_Command(0x00); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	ssd1327_Command(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
	ssd1327_Command(0xC0); //Set COM Output Scan Direction
	ssd1327_Command(0x00); //---set low column address
	ssd1327_Command(0x10); //---set high column address
	ssd1327_Command(0x40); //--set start line address
	ssd1327_Command(0x81); //--set contrast control register
	ssd1327_Command(0xFF);
	ssd1327_Command(0xA0); //--set segment re-map 0 to 127
	ssd1327_Command(0xA6); //--set normal display
	ssd1327_Command(0xA8); //--set multiplex ratio(1 to 64)
	ssd1327_Command(0x7F); // Default => 0x3F (1/64 Duty)	0x1F(1/32 Duty)
	ssd1327_Command(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	ssd1327_Command(0xD3); //-set display offset
	ssd1327_Command(0x00); //-not offset
	ssd1327_Command(0xD5); //--set display clock divide ratio/oscillator frequency
	ssd1327_Command(0xF0); //--set divide ratio
	ssd1327_Command(0xD9); //--set pre-charge period
	ssd1327_Command(0x22); //
	ssd1327_Command(0xDA); //--set com pins hardware configuration
	ssd1327_Command(0x12);
	ssd1327_Command(0xDB); //--set vcomh
	ssd1327_Command(0x34); //0x34,0.77xVcc
//	ssd1327_Command(0x8D); //--set DC-DC enable
//	ssd1327_Command(0x14); //
	ssd1327_Command(0xAF); //--turn on SSD1306 panel
#endif
	/* Clear screen */
	SSD1327_Fill(SSD1327_COLOR_BLACK);															///////////////////
	
	/* Update screen */
	SSD1327_UpdateScreen();
	
	/* Set default values */
	SSD1327.CurrentX = 0;
	SSD1327.CurrentY = 0;
	
	/* Initialized OK */
	SSD1327.Initialized = 1;
	
	/* Return OK */
	return 1;
}

void SSD1327_UpdateScreen(void) {

#ifdef SSD1327
	ssd1327_Command(SSD1327_SETCOLUMNADDRESS);
	ssd1327_Command(0x00);//00
	ssd1327_Command(0x3F);

	ssd1327_Command(SSD1327_SETROWADDRESS);
	ssd1327_Command(0x00);
	ssd1327_Command(0x7F);

		/* Write multi data */
	ssd1327_SPI_WriteMulti(&SSD1327_Buffer[0], SSD1327_WIDTH * SSD1327_HEIGHT/2);
#else

		for(u8 i = 0; i < SSD1327_WIDTH/8; i++) {
			ssd1327_Command(0xB0 + i); // Set the current RAM page address.
			ssd1327_Command(0x00 + 0x0f);
			ssd1327_Command(0x10 + 0x07);
			ssd1327_SPI_WriteMulti(&SSD1327_Buffer[SSD1327_WIDTH*i],SSD1327_WIDTH);
		}
#endif
}

void SSD1327_ToggleInvert(void) {
	uint16_t i;
	
	/* Toggle invert */
	SSD1327.Inverted = !SSD1327.Inverted;
	
	/* Do memory toggle */
	for (i = 0; i < sizeof(SSD1327_Buffer); i++) {
		SSD1327_Buffer[i] = ~SSD1327_Buffer[i];
	}
}

void SSD1327_Fill(SSD1327_COLOR_t color) {
	/* Set memory */
	memset(SSD1327_Buffer, (color == SSD1327_COLOR_BLACK) ? 0x00 : 0xFF, sizeof(SSD1327_Buffer));
}

void SSD1327_DrawPixel(uint16_t x, uint16_t y, SSD1327_COLOR_t color) {
	if (
		x >= SSD1327_WIDTH ||
		y >= SSD1327_HEIGHT
	) {
		/* Error */
		return;
	}
	
	/* Check if pixels are inverted */
	if (SSD1327.Inverted) {
		color = (SSD1327_COLOR_t)!color;
	}
	
	/* Set color */
#ifdef SSD1327
	if (color == SSD1327_COLOR_WHITE) {
		SSD1327_Buffer[y + (x / 2) * SSD1327_WIDTH] |= 0x0F << ((x % 2)*4);
	} else {
		SSD1327_Buffer[y + (x / 2) * SSD1327_WIDTH] &= ~(0x0F << ((x % 2)*4));
	}
#else

	if (color == SSD1327_COLOR_WHITE) {
		SSD1327_Buffer[x + (y / 8) * SSD1327_WIDTH] |= 1 << (y % 8);
	} else {
		SSD1327_Buffer[x + (y / 8) * SSD1327_WIDTH] &= ~(1 << (y % 8));
	}
#endif
}

void SSD1327_GotoXY(uint16_t x, uint16_t y) {
	/* Set write pointers */
	SSD1327.CurrentX = x;
	SSD1327.CurrentY = y;
}

char SSD1327_Putc(char ch, FontDef_t* Font, SSD1327_COLOR_t color) {
	uint32_t i, b, j;
	
	/* Check available space in LCD */
	if (
		SSD1327_WIDTH <= (SSD1327.CurrentX + Font->FontWidth) ||
		SSD1327_HEIGHT <= (SSD1327.CurrentY + Font->FontHeight)
	) {
		/* Error */
		return 0;
	}
	
	/* Go through font */
	for (i = 0; i < Font->FontHeight; i++) {
		b = Font->data[(ch - 32) * Font->FontHeight + i];
		for (j = 0; j < Font->FontWidth; j++) {
			if ((b << j) & 0x8000) {
				SSD1327_DrawPixel(SSD1327.CurrentX + j, (SSD1327.CurrentY + i), (SSD1327_COLOR_t) color);
			} else {
				SSD1327_DrawPixel(SSD1327.CurrentX + j, (SSD1327.CurrentY + i), (SSD1327_COLOR_t)!color);
			}
		}
	}
	
	/* Increase pointer */
	SSD1327.CurrentX += Font->FontWidth;
	
	/* Return character written */
	return ch;
}

char SSD1327_Puts(char* str, FontDef_t* Font, SSD1327_COLOR_t color) {
	/* Write characters */
	while (*str) {
		/* Write character by character */
		if (SSD1327_Putc(*str, Font, color) != *str) {
			/* Return error */
			return *str;
		}
		
		/* Increase string pointer */
		str++;
	}
	
	/* Everything OK, zero should be returned */
	return *str;
}
 
void SSD1327_PutInt(sc32 var, FontDef_t* Font, SSD1327_COLOR_t color)
{
	s32 Temp, len;
	if (var<0)
	{
		SSD1327_Putc(45, Font, color);
	}
	Temp=var;
	len=1;
	while (Temp)
	{
		Temp/=10;
		len*=10;
	}
	if (var<0)
		Temp=-var;
	else
		Temp=var;
	if (Temp!=0) len/=10;

	while (len)
	{
		SSD1327_Putc(Temp/len+48, Font, color);
		Temp%=len;
		len/=10;
	}
}

void SSD1327_PutFloat(double var, FontDef_t* Font, SSD1327_COLOR_t color)
{
	if (var<0){
		SSD1327_Putc(*"-", Font, color);
		var=-var;
	}
	SSD1327_PutInt(var/1, Font, color);
	if ((var-=(long)(var/1))>0)	SSD1327_Putc(*".", Font, color);
//	var+=0.00000001;
//	var=((long)(var*10000))/10000.0;	//отбрасываем ошибку точности

	while ((var-=(int)(var/1))>0)
	{
		var+=0.000001;
		var=((long)(var*10000))/10000.0;	//отбрасываем ошибку точности

		var*=10;
		SSD1327_PutInt(var/1, Font, color);
	}
}

//вывод дробного с фиксированной точностью
void SSD1327_PutFloat_fix(double var, u8 z, FontDef_t* Font, SSD1327_COLOR_t color)
{
	SSD1327_PutInt(var/1, Font, color);
	if ((var-=(long)(var/1))>0)	SSD1327_Putc(*".", Font, color);

	var+=0.00001;
	while ((z--)>0)
	{
		var*=10;
		SSD1327_PutInt(var/1, Font, color);
		var-=(int)(var/1);
	}
}

void SSD1327_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1327_COLOR_t c) {
	int16_t dx, dy, sx, sy, err, e2, i, tmp; 
	
	/* Check for overflow */
	if (x0 >= SSD1327_WIDTH) {
		x0 = SSD1327_WIDTH - 1;
	}
	if (x1 >= SSD1327_WIDTH) {
		x1 = SSD1327_WIDTH - 1;
	}
	if (y0 >= SSD1327_HEIGHT) {
		y0 = SSD1327_HEIGHT - 1;
	}
	if (y1 >= SSD1327_HEIGHT) {
		y1 = SSD1327_HEIGHT - 1;
	}
	
	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1); 
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1); 
	sx = (x0 < x1) ? 1 : -1; 
	sy = (y0 < y1) ? 1 : -1; 
	err = ((dx > dy) ? dx : -dy) / 2; 

	if (dx == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		/* Vertical line */
		for (i = y0; i <= y1; i++) {
			SSD1327_DrawPixel(x0, i, c);
		}
		
		/* Return from function */
		return;
	}
	
	if (dy == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		/* Horizontal line */
		for (i = x0; i <= x1; i++) {
			SSD1327_DrawPixel(i, y0, c);
		}
		
		/* Return from function */
		return;
	}
	
	while (1) {
		SSD1327_DrawPixel(x0, y0, c);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err; 
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		} 
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		} 
	}
}

void SSD1327_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1327_COLOR_t c) {
	/* Check input parameters */
	if (
		x >= SSD1327_WIDTH ||
		y >= SSD1327_HEIGHT
	) {
		/* Return error */
		return;
	}
	
	/* Check width and height */
	if ((x + w) >= SSD1327_WIDTH) {
		w = SSD1327_WIDTH - x;
	}
	if ((y + h) >= SSD1327_HEIGHT) {
		h = SSD1327_HEIGHT - y;
	}
	
	/* Draw 4 lines */
	SSD1327_DrawLine(x, y, x + w, y, c);         /* Top line */
	SSD1327_DrawLine(x, y + h, x + w, y + h, c); /* Bottom line */
	SSD1327_DrawLine(x, y, x, y + h, c);         /* Left line */
	SSD1327_DrawLine(x + w, y, x + w, y + h, c); /* Right line */
}

void SSD1327_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1327_COLOR_t c) {
	uint8_t i;
	
	/* Check input parameters */
	if (
		x >= SSD1327_WIDTH ||
		y >= SSD1327_HEIGHT
	) {
		/* Return error */
		return;
	}
	
	/* Check width and height */
	if ((x + w) >= SSD1327_WIDTH) {
		w = SSD1327_WIDTH - x;
	}
	if ((y + h) >= SSD1327_HEIGHT) {
		h = SSD1327_HEIGHT - y;
	}
	
	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		SSD1327_DrawLine(x, y + i, x + w, y + i, c);
	}
}

void SSD1327_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1327_COLOR_t color) {
	/* Draw lines */
	SSD1327_DrawLine(x1, y1, x2, y2, color);
	SSD1327_DrawLine(x2, y2, x3, y3, color);
	SSD1327_DrawLine(x3, y3, x1, y1, color);
}


void SSD1327_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1327_COLOR_t color) {
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0, 
	yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0, 
	curpixel = 0;
	
	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	} else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	} else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay){
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		SSD1327_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

void SSD1327_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1327_COLOR_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    SSD1327_DrawPixel(x0, y0 + r, c);
    SSD1327_DrawPixel(x0, y0 - r, c);
    SSD1327_DrawPixel(x0 + r, y0, c);
    SSD1327_DrawPixel(x0 - r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1327_DrawPixel(x0 + x, y0 + y, c);
        SSD1327_DrawPixel(x0 - x, y0 + y, c);
        SSD1327_DrawPixel(x0 + x, y0 - y, c);
        SSD1327_DrawPixel(x0 - x, y0 - y, c);

        SSD1327_DrawPixel(x0 + y, y0 + x, c);
        SSD1327_DrawPixel(x0 - y, y0 + x, c);
        SSD1327_DrawPixel(x0 + y, y0 - x, c);
        SSD1327_DrawPixel(x0 - y, y0 - x, c);
    }
}

void SSD1327_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1327_COLOR_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    SSD1327_DrawPixel(x0, y0 + r, c);
    SSD1327_DrawPixel(x0, y0 - r, c);
    SSD1327_DrawPixel(x0 + r, y0, c);
    SSD1327_DrawPixel(x0 - r, y0, c);
    SSD1327_DrawLine(x0 - r, y0, x0 + r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1327_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
        SSD1327_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);

        SSD1327_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
        SSD1327_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
    }
}
 
void SSD1327_ON(void) {
	SSD1327_WRITECOMMAND(0x8D);
	SSD1327_WRITECOMMAND(0x14);
	SSD1327_WRITECOMMAND(0xAF);
}
void SSD1327_OFF(void) {
	SSD1327_WRITECOMMAND(0x8D);
	SSD1327_WRITECOMMAND(0x10);
	SSD1327_WRITECOMMAND(0xAE);
}
