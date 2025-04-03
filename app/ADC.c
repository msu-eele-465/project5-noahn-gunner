#include "../src/ADC.h"
#include <msp430.h>
#include <math.h>

void ADC_init(void) {
    P1SEL0 |= BIT3;
    P1SEL1 |= BIT3;

    ADCCTL0 = ADCSHT_2 | ADCON;         // Sample-and-hold time, ADC ON
    ADCCTL1 = ADCSHP;                   // Use sampling timer
    ADCCTL2 = ADCRES_2;                 // 12-bit resolution
    ADCMCTL0 = ADCINCH_3 | ADCSREF_0;   // A3 input, Vref+ = AVCC
}



uint16_t ADC_sample(void) {
    ADCCTL0 |= ADCENC | ADCSC;
    while (ADCCTL1 & ADCBUSY);
    volatile uint16_t result = ADCMEM0 & 0x0FFF;  // Optional, ensures 12-bit range
    __no_operation();  // ← BREAKPOINT HERE
    return result;
}


float ADC_to_celsius(uint16_t adc_val) {
    volatile float voltage = (adc_val * 3.3f) / 4095.0f;
    float inner = (1.8639f - voltage) / 3.88e-6f;
    float temp = -1481.96f + sqrtf(2196200.0f + inner);
    return temp;
}

// New helper function for test mode
void ADC_sample_and_flash(void) {
    volatile uint16_t adc_val = ADC_sample();  // Get value directly from function
    volatile float voltage = (adc_val * 3.3f) / 4095.0f;
    volatile float temp = -1481.96f + sqrtf(2196200.0f + (1.8639f - voltage) / 3.88e-6f);

    P1OUT |= BIT0;
    __delay_cycles(50000);
    P1OUT &= ~BIT0;

    __no_operation();  // Set a breakpoint and inspect adc_val, voltage, temp
}

