#include "init.h"

#define black RGB565(0,0,0)
#define white RGB565(255,255,255)
#define red RGB565(184,15,15)
#define green RGB565(85,255,150)
#define blue RGB565(0,0,255)
#define grey RGB565(237,245,250)


u32 BAT_LVL = 0;
u32 H = 0, M = 0, S = 0;

int main(void)
{
	int k=0;
	 if (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) {
	        PWR_ClearFlag(PWR_FLAG_SB);
	        k=1;
	 }
	SystemInit();
	DWT_Init();
	GPIO_Init_All();
	Init_AT24();
	ADC1_PA4_Init();
	DISP_ON_PORT->ODR |= DISP_ON_PIN; delay_ms(100);
	//PWR_EnterSTANDBYMode();

	LED_PORT -> BSRR = LED_PIN;

	BAT_LVL = ADC_Read_PA4();

	GC9A01_SPI2_Init();
	GC9A01_Init();
	DrawGUI();
	StartRTC();
	//if (!k)
		//PWR_EnterSTANDBYMode();
    while(1)
    {
    	if (SW_PORT->IDR & SW2_PIN)
    	{ k = 0; Enter_Standby();}
    	//BAT_LVL = ADC_Read_PA4();
    }
}

void ConvertTime()
{
	RTC_WaitForLastTask();
	u32 Counter = RTC_GetCounter();
	RTC_WaitForLastTask();

	H = Counter/3600;
	M = (Counter-(H)/3600)/60;
	S = Counter - (H*3600) - (M*60);
}

void Enter_Standby(void) {
	SendCommand(0x28);
	DISP_ON_PORT->ODR &= ~DISP_ON_PIN;
    // Настройка PA0 как источника пробуждения
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_IPD; // Input pull-down
    GPIO_Init(GPIOA, &gpio);

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN;

    gpio.GPIO_Mode = GPIO_Mode_IPD; // Input pull-down
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Pin = GPIO_Pin_All;
    GPIO_Init(GPIOC, &gpio);
    GPIO_Init(GPIOD, &gpio);
    GPIO_Init(GPIOE, &gpio);
    GPIO_Init(GPIOA, &gpio);
    GPIO_Init(GPIOB, &gpio);

    /* Disable GPIOs clock */
    RCC->APB2ENR &= ~(RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN);

    PWR_WakeUpPinCmd(ENABLE); // Активировать Wake-Up Pin
    PWR_EnterSTANDBYMode();
}

void DrawGUI(void)
{
	GC9A01_setXY(120, 120);
	FillScreen(black);

	DrawFillRect(80,30,BAT_LVL/81,18,green);
	DrawFillRect(130,34,4,10,white);
	DrawRect(80,30,50,18,white);
	uint8_t w = 80, h = 70, y = 80, gap = 16, R = 12;

	DrawRoundedRect(TFT_WIDTH/2 - w - gap/2, y, w, h, R, grey);
	DrawRoundedRect(TFT_WIDTH/2 + gap/2, y, w, h, R, grey);

	DrawFillCircle(TFT_WIDTH/2, y+3*h/8, 4, white);
	DrawFillCircle(TFT_WIDTH/2, y+5*h/8, 4, white);
	/*GC9A01_setXY(40, 60);
	GC9A01_PutInt(H, &Font_11x18, white);
	GC9A01_Putc(':', &Font_11x18, white);
	GC9A01_PutInt(M, &Font_11x18, white);
	GC9A01_Putc(':', &Font_11x18, white);
	GC9A01_PutInt(S, &Font_11x18, white);*/
	GC9A01_setXY(120-((strlen("MNSTech")*11)/2),192);
	GC9A01_Puts_Fill("MNSTech", &Font_11x18, 10, white, red);
	GC9A01_setXY(120-((strlen("Версия 2")*6)/2),220);
	GC9A01_Puts("Версия 2", &Font_6x8, white);

	SendCommand(0x29); delay_ms(5);
}

void UpdateGUI(void)
{
	ConvertTime();
	BAT_LVL = (ADC_Read_PA4() > STM_VCC) ? ADC_Read_PA4() - STM_VCC : 0;
	DrawFillRect(40,60,190,20,black);
	GC9A01_setXY(40, 60);
	GC9A01_PutInt(H, &Font_11x18, white);
	GC9A01_Putc(':', &Font_11x18, white);
	GC9A01_PutInt(M, &Font_11x18, white);
	GC9A01_Putc(':', &Font_11x18, white);
	GC9A01_PutInt(S, &Font_11x18, white);
	GC9A01_Putc(' | ', &Font_11x18, white);
	GC9A01_PutInt(BAT_LVL, &Font_11x18, green);
	GC9A01_Puts(" mV", &Font_11x18, green);
	DrawFillRect(80,30,50,18,black);
	DrawRect(80,30,50,18,white);
	DrawFillRect(80,30,BAT_LVL/8,18,green);
	delay_ms(500);
}
