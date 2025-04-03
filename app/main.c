#include <msp430.h>
#include <stdbool.h>
#include "../src/ADC.h"
#include "../src/keypad.c"
#include "../src/keypad.h"
#include "msp430fr2355.h"

volatile bool sample_flag = false;
int data_Cnt = 0;
volatile int prev_state = 0;
bool already_sent = false;
char packet[] = {0x03};

void init_I2C() {
    //-- 1. Put eUSCI_B0 into software reset
	UCB0CTLW0 |= UCSWRST;                   // UCSWRST=1 for eUSCI_B0 in SW reset

	//-- 2. Configure eUSCI_B0
	UCB0CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK=SMCLK=1MHz
	UCB0BRW = 10;                           // Divide BRCLK by 10 for SCL=100kHz

	UCB0CTLW0 |= UCMODE_3;                  // Put into I2C mode
	UCB0CTLW0 |= UCMST;                     // Put into master mode
	UCB0CTLW0 |= UCTR;
	UCB0I2CSA = 0x0042;                     // Slave address = 0x42 (LED Bar)

	UCB0CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached
	UCB0TBCNT = sizeof(packet);             // # of bytes in packet

	//-- 3. Configure Ports
	P1SEL1 &= ~BIT3;                        // P1.3 = SCL
	P1SEL0 |= BIT3;

	P1SEL1 &= ~BIT2;                        // P1.2 = SDA
	P1SEL0 |= BIT2;

	PM5CTL0 &= ~LOCKLPM5;                   // Disable Low Power Mode

	//-- 4. Take eUSCI_B0 out of SW reset
	UCB0CTLW0 &= ~UCSWRST;                  // UCSWRST = 1 for eUSCI_B0 in SW reset

	//-- 5. Enable Interrupts
	UCB0IE |= UCTXIE0;                      // Enable I2C Tx0 IRQ
	__enable_interrupt();
}

void send_I2C_packet() {
    UCB0IFG &= ~UCTXIFG0;                   // Clear flags
    UCB0TXBUF = packet[0];                  // Load byte
    UCB0CTLW0 |= UCTXSTT;                   // Start
    while (UCB0CTLW0 & UCTXSTT);            // Wait for start to clear
    while (!(UCB0IFG & UCSTPIFG));          // Wait for stop
    UCB0IFG &= ~UCSTPIFG;
}

void sendCommandByte(char byte) {
    packet[0] = byte;
    UCB0TBCNT = sizeof(packet);      // Set packet length again
    UCB0IFG &= ~UCTXIFG0;            // Clear TX flag
    UCB0TXBUF = packet[0];           // Load byte
    UCB0CTLW0 |= UCTXSTT;            // Send START
    while (UCB0CTLW0 & UCTXSTT);     // Wait for START to clear
    while (!(UCB0IFG & UCSTPIFG));   // Wait for STOP
    UCB0IFG &= ~UCSTPIFG;
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    P1DIR |= BIT0;      // LED
    P1OUT &= ~BIT0;

    P6DIR |= BIT6;  // Unlock status LED
    P6OUT &= ~BIT6;

    init_keypad_ports();
    init_I2C();

    ADC_init();

    PM5CTL0 &= ~LOCKLPM5;

    // Timer for 0.5 s sampling
    TB0CCTL0 = CCIE;          // Enable CCR0 interrupt
    TB0CCR0 = 16384;          // 0.5 seconds
    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;  // ACLK, up mode, clear

    init_keypad_irqs();
    __enable_interrupt();

    int i;
    // Enable low-power mode with global interrupts
    //_BIS_SR((LPM0_bits | GIE));

    while (true) {
        if (sample_flag) {
            sample_flag = false;
            ADC_sample_and_flash();
        }

        col_masking();

        for(i=0; i<10000; i++) {}
        
        lock_state();

        // === UNLOCK DETECTED ===
        if (locked_state == 2 && password_unlock) {


            if (!already_sent) {
                send_I2C_packet();
                already_sent = true;
            }

        } else {
            already_sent = false;  // Reset if it gets re-locked
        }

        if (locked_state == 2) {
            P6OUT |= BIT6;
        } else if (locked_state == 0) {
            P6OUT &= ~BIT6;
        }

        for(i=0; i<10000; i++) {}
        
        prev_key = current_key;
        
        for(i=0; i<10000; i++) {}
        
        row_cycle();
        
        for(i=0; i<10000; i++) {}
    }

}

//----Interrupt Service Routines
#pragma vector = PORT2_VECTOR
__interrupt void ISR_Port2_Column(void) {
    col_masking();
    if (col_1 == 0) {
        if (current_row == 1) current_key = '1';
        else if (current_row == 2) current_key = '4';
        else if (current_row == 3) current_key = '7';
        else if (current_row == 4) current_key = '*';
    } else if (col_2 == 0) {
        if (current_row == 1) current_key = '2';
        else if (current_row == 2) current_key = '5';
        else if (current_row == 3) current_key = '8';
        else if (current_row == 4) current_key = '0';
    } else if (col_3 == 0) {
        if (current_row == 1) current_key = '3';
        else if (current_row == 2) current_key = '6';
        else if (current_row == 3) current_key = '9';
        else if (current_row == 4) current_key = '#';
    } else if (col_4 == 0) {
        if (current_row == 1) current_key = 'A';
        else if (current_row == 2) current_key = 'B';
        else if (current_row == 3) current_key = 'C';
        else if (current_row == 4) current_key = 'D';
    } else {
        current_key = 'N';
    }

    P2IFG &= ~(BIT4 | BIT5 | BIT6 | BIT7);
    P1OUT ^= BIT0;
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void) {
    sample_flag = true;
}

#pragma vector = TIMER0_B1_VECTOR
__interrupt void ISR_TB0_Overflow(void) {
    sample_flag = true;
    P1OUT ^= BIT0;
    TB0CTL &= ~TBIFG;
}

#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void){
    UCB0TXBUF = packet[data_Cnt];          // Send the byte
    data_Cnt++;

    if (data_Cnt >= sizeof(packet)) {
        data_Cnt = 0;                      // Reset for next transmission
    }
}
