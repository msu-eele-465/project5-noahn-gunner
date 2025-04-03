#include <stdio.h>
#include <msp430.h>
#include <stdbool.h>

// initialize vars
char current_key = 'N';
char prev_key = 'N';
volatile int col_1 = BIT2;
volatile int col_2 = BIT3;
volatile int col_3 = BIT6;
volatile unsigned int col_4 = BIT7;
volatile current_row = 0;
// 0 = locked, 1 = unlocking, 2 = unlocked
volatile int locked_state = 0;
volatile int password_unlock = false;
// current index of password
volatile int pass_inx_char = 0;
// 0 = stay same, 1 = faster, 2 = slower
volatile int LED_speed = 0;

// initialize ports
int init_keypad_ports(void) {
    // rows (outputs for row_cycle)
    P5DIR |= BIT1; // set 5.1 as output
    P5DIR |= BIT3; // set 5.3 as output 
    P5DIR |= BIT2; // set 5.2 as output
    P5DIR |= BIT0; // set 5.0 as output 
     
    P5OUT &= ~BIT1; // turn 5.1 off
    P5OUT &= ~BIT3; // turn 5.3 off
    P5OUT &= ~BIT2; // turn 5.2 off
    P5OUT &= ~BIT0; // turn 5.0 off

    // columns (inputs for polling)
    P1DIR &= ~BIT2; // set 1.2 as output
    P1REN |= BIT2;  // enable pull up/down resistor
    P1OUT |= BIT2;  // pull up resistor
    P1IES |= BIT2;  // IRQ Sensitivity H to L

    P1DIR &= ~BIT3; // set 1.3 as output
    P1REN |= BIT3;  // enable pull up/down resistor
    P1OUT |= BIT3;  // pull up resistor

    P1DIR &= ~BIT6; // set 1.6 as output
    P1REN |= BIT6;  // enable pull up/down resistor
    P1OUT |= BIT6;  // pull up resistor

    P1DIR &= ~BIT7; // set 1.7 as output
    P1REN |= BIT7;  // enable pull up/down resistor
    P1OUT |= BIT7;  // pull up resistor

    return 0;
}

// init irqs
int init_keypad_irqs(void) {
    P1IFG &= ~BIT2; // clear port 1.2 IRQ flag
    P1IE |= BIT2;   // enable 1.2 IRQ 
    P1IFG &= ~BIT3; // clear port 1.3 IRQ flag
    P1IE |= BIT3;   // enable 1.3 IRQ 
    P1IFG &= ~BIT6; // clear port 1.6 IRQ flag
    P1IE |= BIT6;   // enable 1.6 IRQ 
    P1IFG &= ~BIT7; // clear port 1.7 IRQ flag
    P1IE |= BIT7;   // enable 1.7 IRQ 
    return 0;
}

// cycle through rows turn on/off
int row_cycle(void) {
    current_row = 1;
    P5OUT |= BIT1; // turn 5.1 on
    P5OUT &= ~BIT1; // turn 5.1 off
    int i;
    for(i=0; i<10000; i++) {}
    current_row = 2;
    P5OUT |= BIT3; // turn 5.3 on
    P5OUT &= ~BIT3; // turn 5.3 off
    for(i=0; i<10000; i++) {}
    current_row = 3;
    P5OUT |= BIT2; // turn 5.2 on
    P5OUT &= ~BIT2; // turn 5.2 off
    for(i=0; i<10000; i++) {}
    current_row = 4;
    P5OUT |= BIT0; // turn 5.0 on
    P5OUT &= ~BIT0; // turn 5.0 off
    for(i=0; i<10000; i++) {}

    return 0;
}

// mask columns for easier reading
int col_masking(void) {
    col_1 =  P1IN;
    col_1 &= BIT2;
    col_2 =  P1IN;
    col_2 &= BIT3;
    col_3 =  P1IN;
    col_3 &= BIT6;
    col_4 =  P1IN;
    col_4 &= BIT7;

    return 0;
}

// determine lock state
int lock_state(void) {
    if ((current_key == 'D') && (prev_key != 'D')) {
        // lock system if D is pressed
        locked_state = 0;
        password_unlock = false;
        pass_inx_char = 0;
    } else if (!password_unlock) {
        // logic for inputting password (hard coded in lol)
        if ((current_key == '1') && (prev_key != '1') && (pass_inx_char == 0)) {
            pass_inx_char = 1;
            locked_state = 1;
        } else if ((current_key == '1') && (prev_key != '1') && (pass_inx_char != 0)) {
            locked_state = 0;
            pass_inx_char = 0;
        } else if ((current_key == '2') && (prev_key != '2') && (pass_inx_char == 1)) {
            pass_inx_char = 2;
        } else if ((current_key == '2') && (prev_key != '2') && (pass_inx_char != 1)) {
            locked_state = 0;
            pass_inx_char = 0;
        } else if ((current_key == '3') && (prev_key != '3') && (pass_inx_char == 2)) {
            pass_inx_char = 3;
        } else if ((current_key == '3') && (prev_key != '3') && (pass_inx_char != 2)) {
            locked_state = 0;
            pass_inx_char = 0;
        } else if ((current_key == '4') && (prev_key != '4') && (pass_inx_char == 3)) {
            locked_state = 2;
            password_unlock = true;
        } else if ((current_key == '4') && (prev_key != '4') && (pass_inx_char != 3)) {
            locked_state = 0;
            pass_inx_char = 0;
        } 
    } else {
        // actual functionality logic goes here
        // or maybe in another function idk yet lmao
        button_logic();
    }
    
    return 0;
}

// unlocked funtionality
int button_logic() {
    if ((current_key == '1') && (prev_key != '1')) {
        locked_state = 2;
    } else if ((current_key == '2') && (prev_key != '2')) {
        locked_state = 3;
    } else if ((current_key == '3') && (prev_key != '3')) {
        locked_state = 4;
    } else if ((current_key == '4') && (prev_key != '4')) {
        locked_state = 5;
    } else if ((current_key == '5') && (prev_key != '5')) {
        locked_state = 6;
    } else if ((current_key == '6') && (prev_key != '6')) {
        locked_state = 7;
    } else if ((current_key == '7') && (prev_key != '7')) {
        locked_state = 8;
    } else if ((current_key == '0') && (prev_key != '0')) {
        locked_state = 9;
    } else if ((current_key == 'A') && (prev_key != 'A')) {
        LED_speed = 1;
    } else if ((current_key == 'B') && (prev_key != 'B')) {
        LED_speed = 2;
        
    }

    return 0;
}
