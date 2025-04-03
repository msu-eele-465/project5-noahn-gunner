#include <stdio.h>
#include <msp430.h>
#include <stdbool.h>
#include "keypad.h"

// initialize vars
char current_key = 'N';
char prev_key = 'N';
volatile int col_1 = BIT4;  // P2.4
volatile int col_2 = BIT5;  // P2.5
volatile int col_3 = BIT6;  // P2.6
volatile unsigned int col_4 = BIT7; // P2.7
volatile current_row = 0;
// 0 = locked, 1 = unlocking, 2 = unlocked
volatile int locked_state = 0;
volatile int password_unlock = false;
// current index of password
volatile int pass_inx_char = 0;
// 0 = stay same, 1 = faster, 2 = slower
volatile int LED_speed = 0;

volatile bool window_size_entry_state = false;
volatile bool pattern_number_entry_state = false;



// initialize ports
int init_keypad_ports(void) {
    // rows (outputs for row_cycle) → P4.4 - P4.7
    P4DIR |= BIT4 | BIT5 | BIT6 | BIT7;
    P4OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);

    // columns (inputs for polling) → P2.4 - P2.7
    P2DIR &= ~(BIT4 | BIT5 | BIT6 | BIT7);
    P2REN |= BIT4 | BIT5 | BIT6 | BIT7;
    P2OUT |= BIT4 | BIT5 | BIT6 | BIT7;
    P2IES |= BIT4 | BIT5 | BIT6 | BIT7;

    return 0;
}

// init irqs
int init_keypad_irqs(void) {
    P2IFG &= ~(BIT4 | BIT5 | BIT6 | BIT7);
    P2IE  |= (BIT4 | BIT5 | BIT6 | BIT7);
    return 0;
}

// cycle through rows turn on/off
int row_cycle(void) {
    current_row = 1;
    P4OUT |= BIT4;
    P4OUT &= ~BIT4;
    __delay_cycles(10000);

    current_row = 2;
    P4OUT |= BIT5;
    P4OUT &= ~BIT5;
    __delay_cycles(10000);

    current_row = 3;
    P4OUT |= BIT6;
    P4OUT &= ~BIT6;
    __delay_cycles(10000);

    current_row = 4;
    P4OUT |= BIT7;
    P4OUT &= ~BIT7;
    __delay_cycles(10000);

    return 0;
}

// mask columns for easier reading
int col_masking(void) {
    col_1 = P2IN & BIT4;
    col_2 = P2IN & BIT5;
    col_3 = P2IN & BIT6;
    col_4 = P2IN & BIT7;
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
    // --- If in "window size entry" state ---
    if (window_size_entry_state) {
        if (current_key >= '1' && current_key <= '9' && current_key != prev_key) {
            int new_size = current_key - '0';
            ADC_set_window_size(new_size);
            window_size_entry_state = false;
            // (Optional) show "Window size set to X"
        }
        return 0;
    }

    // --- If in "pattern number entry" state ---
    if (pattern_number_entry_state) {
        if (current_key >= '0' && current_key <= '7' && current_key != prev_key) {
            int pattern_num = current_key - '0';
            sendCommandByte(pattern_num);
            pattern_number_entry_state = false;
            // (Optional) show "Pattern X selected"
        }
        return 0;
    }

    // --- Entry State Activation ---
    if ((current_key == 'C') && (prev_key != 'C')) {
        window_size_entry_state = true;
        pattern_number_entry_state = false;
        // (Optional) display "Set window size"
        return 0;
    }

    if ((current_key == '#') && (prev_key != '#')) {
        pattern_number_entry_state = true;
        window_size_entry_state = false;
        // (Optional) display "Set pattern"
        return 0;
    }

    // --- Regular Pattern/Speed Keys (outside entry states) ---
    if ((current_key == '0') && (prev_key != '0')) {
        locked_state = 9;
    }
    else if ((current_key == '1') && (prev_key != '1')) {
        locked_state = 2;
    }
    else if ((current_key == '2') && (prev_key != '2')) {
        locked_state = 3;
    }
    else if ((current_key == '3') && (prev_key != '3')) {
        locked_state = 4;
    }
    else if ((current_key == '4') && (prev_key != '4')) {
        locked_state = 5;
    }
    else if ((current_key == '5') && (prev_key != '5')) {
        locked_state = 6;
    }
    else if ((current_key == '6') && (prev_key != '6')) {
        locked_state = 7;
    }
    else if ((current_key == '7') && (prev_key != '7')) {
        locked_state = 8;
    }
    else if ((current_key == 'A') && (prev_key != 'A')) {
        sendCommandByte(0x41);  // Decrease speed
    }
    else if ((current_key == 'B') && (prev_key != 'B')) {
        sendCommandByte(0x42);  // Increase speed
    }

    return 0;
}
