#include <msp430.h>
#include <stdbool.h>
#include "../src/ADC.h"

volatile bool sample_flag = false;


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

    // Timer for 0.5 s sampling
    TB0CCTL0 = CCIE;          // Enable CCR0 interrupt
    TB0CCR0 = 16384;          // 0.5 seconds
    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;  // ACLK, up mode, clear

    __enable_interrupt();

    while (true) {
        if (sample_flag) {
            sample_flag = false;
            ADC_sample_and_flash();
        }
    }

}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void) {
    sample_flag = true;
}

#pragma vector = TIMER0_B1_VECTOR
__interrupt void ISR_TB0_Overflow(void) {
    P1OUT ^= BIT0;
    TB0CTL &= ~TBIFG;
}

