#include "ADC.h"
//#include "lcd.h"
#include <msp430.h>
#include <math.h>

#define MAX_WINDOW 10  // You can increase if needed

static float temp_buffer[MAX_WINDOW];
volatile int window_size = 3;   // Default n = 3
static int temp_index = 0;
static int temp_count = 0;
int counter = 0;

void ADC_init(void) {
    P1SEL0 |= BIT4;
    P1SEL1 |= BIT4;


    ADCCTL0 = ADCSHT_2 | ADCON;         // Sample-and-hold time, ADC ON
    ADCCTL1 = ADCSHP;                   // Use sampling timer
    ADCCTL2 = ADCRES_2;                 // 12-bit resolution
    ADCMCTL0 = ADCINCH_4 | ADCSREF_0;   // A3 input, Vref+ = AVCC
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


void ADC_set_window_size(int n) {
    int i;
    counter = 0;
    if (n > 0 && n <= MAX_WINDOW) {
        window_size = n;
        temp_index = 0;
        temp_count = 0;
        for (i = 0; i < MAX_WINDOW; i++) {
            temp_buffer[i] = 0.0f;
        }
    }
}

float ADC_add_sample(float temp) {
    int i;
    temp_buffer[temp_index] = temp;
    temp_index = (temp_index + 1) % window_size;
    if (temp_count < window_size) temp_count++;

    float sum = 0.0f;
    for (i = 0; i < temp_count; i++) {
        sum += temp_buffer[i];
    }

    return sum / temp_count;
}

void ADC_sample_and_flash(void) {
    uint16_t adc_val = ADC_sample();
    volatile float voltage = (adc_val * 3.3f) / 4095.0f;
    volatile float temp = -1481.96f + sqrtf(2196200.0f + (1.8639f - voltage) / 3.88e-6f);

    volatile float avg_temp = ADC_add_sample(temp);  // ← Moving average!
    counter++;

    if (counter > window_size) {
        int temp_tens = ((int)avg_temp / 10);
        int temp_ones = ((int)avg_temp % 10);
        int temp_tenths = (((int)(10 * avg_temp) % 100) % 10);
        location_by_coords(2, 1);
        write_by_ascii('T');
        write_by_ascii('=');
        write_by_ascii(temp_tens + 48);
        write_by_ascii(temp_ones + 48);
        write_by_ascii('.');
        write_by_ascii(temp_tenths + 48);
    }

    // Toggle LED as confirmation
    P1OUT |= BIT0;
    __delay_cycles(50000);
    P1OUT &= ~BIT0;

    __no_operation();  // Set breakpoint and inspect avg_temp
}
