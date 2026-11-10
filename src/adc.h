#ifndef ADC_H
#define ADC_H

void adc_init(void);
int adc_battery_mv(void);
int adc_battery_pct(int millivolts);

#endif
