#ifndef ADC_H
#define ADC_H

#include <stdint.h>
extern volatile int window_size;

void ADC_init(void);
uint16_t ADC_sample(void);
float ADC_to_celsius(uint16_t adc_val);
void ADC_sample_and_flash(void);  // For test mode (flashes LED and samples temp)


#endif
