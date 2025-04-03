#include <msp430.h>
#include <stdbool.h>
#include "../src/ADC.h"

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    P1DIR |= BIT0;      // LED
    P1OUT &= ~BIT0;

    P4DIR &= ~BIT1;     // Button input
    P4REN |= BIT1;
    P4OUT |= BIT1;



    ADC_init();

    PM5CTL0 &= ~LOCKLPM5;

    // Optional heartbeat
    TB0CTL |= TBCLR | TBSSEL__ACLK | MC__CONTINUOUS;
    TB0CTL &= ~TBIFG;
    TB0CTL |= TBIE;
    __enable_interrupt();

    while (true) {
        if ((P4IN & BIT1) == 0) { // Button pressed
            ADC_sample_and_flash();  // Handles everything inside ADC.c
        }
    }
}

#pragma vector = TIMER0_B1_VECTOR
__interrupt void ISR_TB0_Overflow(void) {
    P1OUT ^= BIT0;
    TB0CTL &= ~TBIFG;
}
