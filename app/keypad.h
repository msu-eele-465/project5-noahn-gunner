#ifndef KEYPAD_H
#define KEYPAD_H

#include <msp430.h>
#include <stdbool.h>

// Variables (shared)
extern char current_key;
extern char prev_key;
extern volatile int col_1, col_2, col_3;
extern volatile unsigned int col_4;
extern volatile int current_row;
extern volatile int locked_state;
extern volatile int password_unlock;
extern volatile int pass_inx_char;
extern volatile int LED_speed;

// Functions
int init_keypad_ports(void);
int init_keypad_irqs(void);
int row_cycle(void);
int col_masking(void);
int lock_state(void);
int button_logic(void);
void sendCommandByte(char byte);
void ADC_set_window_size(int n);  // from ADC.c
void display_temp(float temp);
void display_window_size(char win);
void pattern_name(int pattern);


#endif
