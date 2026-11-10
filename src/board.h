/* Распиновка и константы платы.
   Подтверждено старыми исходниками: SPI2 дисплея, I2C2 EEPROM,
   АЦП батареи на PA4, пробуждение из Standby на PA0.
   Второй кнопки в репозитории не было (порт SW2 потерян вместе с init.h).
   PA1 — рабочее назначение: у PA0 и PB0 общий EXTI0, вместе их не повесить.
   Сверьте оба входа с платой до прошивки.
   Неиспользуемые выводы остаются аналоговыми входами и никуда не дёргаются.
   Отдельного канала электрошока в прошивке нет. */
#ifndef BOARD_H
#define BOARD_H

#include "stm32f103.h"

#define HSE_HZ 8000000u
#define PLL_MUL 9u          /* 8 МГц * 9 = 72 МГц. Для кварца 12 МГц поставьте 6. */

#define LCD_W 240
#define LCD_H 240
/* MADCTL, бит 3 = BGR. Если картинка перевёрнута: 0x48, 0x88, 0xC8.
   Если чёрный и белый поменяны местами, поставьте LCD_INVERT в 0. */
#define LCD_MADCTL 0x08
#define LCD_INVERT 1
/* Делитель SPI2 от PCLK1: 0 = /2. На 72 МГц это 18 МГц.
   Если панель рябит, поставьте 1 (/4, 9 МГц). */
#define LCD_SPI_BR 0u

#define BOARD_BTN1_PORT GPIOA
#define BOARD_BTN1_PIN  0u    /* WKUP, активный высокий уровень, нужен внешний подтяг к земле */
#define BOARD_BTN2_PORT GPIOA
#define BOARD_BTN2_PIN  1u

/* Пины питания подсветки и светодиода в старом коде не были заданы.
   0 — прошивка их не трогает. Не включайте, пока не сверите схему:
   на плате есть силовой транзистор, и чужой выход его откроет. */
#define BOARD_HAS_DISP_EN 0
#define BOARD_DISP_EN_PORT GPIOB
#define BOARD_DISP_EN_PIN  1u
#define BOARD_HAS_LED 0
#define BOARD_LED_PORT GPIOC
#define BOARD_LED_PIN  13u
#define BOARD_LED_ACTIVE_LOW 1

/* Делитель батареи к PA4: верхний резистор к плюсу, нижний к земле.
   По умолчанию равные плечи, опорное 3.30 В. Напряжение на выводе не выше 3.6 В. */
#define BOARD_VREF_MV 3300u
#define BOARD_BAT_RTOP 100u
#define BOARD_BAT_RBOT 100u
#define BOARD_BAT_EMPTY_MV 3400
#define BOARD_BAT_FULL_MV  4100

/* AT24C02, A2=A1=A0=0. В регистр DR пишется уже 8-битный адрес, без сдвига. */
#define EE_ADDR_WRITE 0xA0u

#define COL_BG     ((uint16_t)0x0000u)
#define COL_FG     ((uint16_t)0xEF5Du) /* 236,237,239 */
#define COL_DIM    ((uint16_t)0x7BC6u) /* 120,126,134 */
#define COL_ACCENT ((uint16_t)0xED96u) /* 232,176,92 */
#define COL_WARN   ((uint16_t)0xD4A9u) /* 214,84,72 */
#define COL_TICK   ((uint16_t)0x44AAu) /* 70,76,84 */
#define COL_SLOT   ((uint16_t)0x18E4u) /* 28,30,36 — подложка выбранного пункта */

#define BTN_NONE     0u
#define BTN1_SHORT   1u
#define BTN1_LONG    2u
#define BTN2_SHORT   3u
#define BTN2_LONG    4u

#define SCR_FACE   0
#define SCR_STATUS 1
#define SCR_MENU   2
#define SCR_ALARM  3
#define SCR_TIMER  4
#define SCR_CLOCK  5
#define SCR_POWER  6
#define SCR_RING   7

#endif
