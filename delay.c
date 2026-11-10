#include "delay.h"

uint8_t init_sysclk(void)
{
	ErrorStatus HSEStartUpStatus;
	uint8_t pr=0;
    RCC_DeInit();

    RCC_HSEConfig(RCC_HSE_ON);
    HSEStartUpStatus = RCC_WaitForHSEStartUp();

    if (HSEStartUpStatus == SUCCESS) //Коды ошибок тактирования
    {								// 2 - Timeout
    	__IO int StartUpCounter = 0;//3 - Установлен неверный источник тактирования
    	RCC_PLLConfig(RCC_PLLSource_HSE_Div1,RCC_PLLMul_9); //0 - всё чики-пуки //9
    	RCC->CR |= RCC_CR_PLLON;					//4 - HSE не готов
    	for(StartUpCounter=0; ; StartUpCounter++)
    	  {
    	    if(RCC->CR & RCC_CR_PLLRDY && RCC->CR & RCC_CR_HSERDY)
    	    { pr = 0; break;}

    	    if(StartUpCounter > 1000)
    	    {
    	      RCC->CR &= ~RCC_CR_HSEON;
    	      RCC->CR &= ~RCC_CR_PLLON;
    	      pr = 2;
    	    }
    	  }
    	//NVIC_EnableIRQ(SysTick_IRQn);
    	//NVIC_SetPriority(SysTick_IRQn, 5);
        RCC_HCLKConfig( RCC_SYSCLK_Div1);
        RCC_PCLK2Config( RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div1);

        *((uint32_t*)0x40021004) |= 2;
       RCC->CFGR |= (1<<1);

        if (RCC_GetSYSCLKSource() != 0x08)
        {
        	pr = 3;
        }
    }
    else
    {
        return 4;
    }
    return pr;
}

void DWT_Init(void)
{
    SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // разрешаем использовать счётчик
    DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk;   // запускаем счётчик
}

void delay_us(u32 us)
{
    u32 us_count_tic =  (us * (SystemCoreClock / 1000000)); // получаем кол-во тактов за 1 мкс и умножаем на наше значение
    //us_count_tic /= 2;

    us_count_tic = (us_count_tic >= k) ? us_count_tic-k : 1;

    DWT->CYCCNT = 0U; // обнуляем счётчик
    while(DWT->CYCCNT < us_count_tic);
}

void delay_ms(u32 us)
{
    u32 us_count_tic =  (us * (SystemCoreClock / 1000)); // получаем кол-во тактов за 1 мкс и умножаем на наше значение
    //us_count_tic /= 2;

    us_count_tic = (us_count_tic >= k) ? us_count_tic-k : 1;

    DWT->CYCCNT = 0U; // обнуляем счётчик
    while(DWT->CYCCNT < us_count_tic);
}

void delay_ns(u32 us)
{
   // u32 us_count_tic =  (us * (SystemCoreClock / 72000000)); // получаем кол-во тактов за 1 мкс и умножаем на наше значение
    //us_count_tic /= 2;

    //us_count_tic = (us_count_tic >= k) ? us_count_tic-k : 1;

    DWT->CYCCNT = 0U; // обнуляем счётчик
    while(DWT->CYCCNT < us);
}

volatile void delay(u32 n) {
	while (n--) {}
}
