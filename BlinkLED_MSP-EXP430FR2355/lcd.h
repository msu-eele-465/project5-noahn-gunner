#include <stdbool.h>

bool cursor_on = true;
bool cursor_blink = true;
int window_size = 3;

void define_ports(void) {
    // data nibble DB7-BD4 (P5.0-5.3)
    P5DIR |= BIT0;
    P5OUT &= ~BIT0;
    P5DIR |= BIT1;
    P5OUT &= ~BIT1;
    P5DIR |= BIT2;
    P5OUT &= ~BIT2;
    P5DIR |= BIT3;
    P5OUT &= ~BIT3;

    // RS (P5.4)
    P5DIR |= BIT4;
    P5OUT &= ~BIT4;
    
    // RW (P3.0)
    P3DIR |= BIT0;
    P3OUT &= ~BIT0;

    // Enable (P3.1)
    P3DIR |= BIT1;
    P3OUT &= ~BIT1;
}

// turns enable on
void enable_high(void) {
    P3OUT |= BIT1;
}

// turns enable off
void enable_low(void) {
    P3OUT &= ~BIT1;
}

void enable_pulse() {
    enable_high();
    __delay_cycles(50000);
    enable_low();
}

void rs_high(void) {
    P5OUT |= BIT4;
}

void rs_low(void) {
    P5OUT &= ~BIT4;
}

void rw_high(void) {
    P3OUT |= BIT0;
}

void rw_low(void) {
    P3OUT &= ~BIT0;
}

void set_nibble(int nibbleInt) {
    int nib0 = nibbleInt;
    if ((nib0 &= BIT0) != 0) {
        P5OUT |= BIT0;
    } else {
        P5OUT &= ~BIT0;
    }
    
    int nib1 = nibbleInt;
    if ((nib1 &= BIT1) != 0) {
        P5OUT |= BIT1;
    } else {
        P5OUT &= ~BIT1;
    }
    
    int nib2 = nibbleInt;
    if ((nib2 &= BIT2) != 0) {
        P5OUT |= BIT2;
    } else {
        P5OUT &= ~BIT2;
    }
    
    int nib3 = nibbleInt;
    if ((nib3 &= BIT3) != 0) {
        P5OUT |= BIT3;
    } else {
        P5OUT &= ~BIT3;
    }
}

void start_up() {
    // set to 4 bit mode
    set_nibble(0b0010);
    enable_pulse();
    __delay_cycles(100000);
    set_nibble(0b0010);
    enable_pulse();
    __delay_cycles(100000);

    // 2 line mode, display on
    set_nibble(0b1100);
    enable_pulse();
    __delay_cycles(100000);

    // display on, cursor off, blink on
    set_nibble(0b0000);
    enable_pulse();
    __delay_cycles(100000);
    set_nibble(0b1101);
    enable_pulse();
    __delay_cycles(100000);

    // clear display
    set_nibble(0b0000);
    enable_pulse();
    __delay_cycles(100000);
    set_nibble(0b0001);
    enable_pulse();
    __delay_cycles(100000);

    // entry mode set, increment mode, shift off
    set_nibble(0b0000);
    enable_pulse();
    __delay_cycles(100000);
    set_nibble(0b0110);
    enable_pulse();
    __delay_cycles(100000);
}

void clear_display() {
    set_nibble(0b0000);
    enable_pulse();
    __delay_cycles(5000);
    set_nibble(0b0001);
    enable_pulse();
    __delay_cycles(5000);
}

void set_cursor_location(int upperNibble, int lowerNibble) {
    // expecting full address given to be 1 concatenated with 7 bit address, split into 2 nibbles
    // e.g. for first char in first line, upperNibble = 1000, lowerNibble = 0000
    set_nibble(upperNibble);
    enable_pulse();
    __delay_cycles(5000);
    set_nibble(lowerNibble);
    enable_pulse();
    __delay_cycles(5000);
}

void write_character(int upperNibble, int lowerNibble) {
    // expecting full address split into 2 nibbles
    // e.g. for 0, upperNibble = 0011, lowerNibble = 0000
    rs_high();
    set_nibble(upperNibble);
    enable_pulse();
    __delay_cycles(500);
    set_nibble(lowerNibble);
    enable_pulse();
    __delay_cycles(500);
    rs_low();
}

void update_cursor_status() {
    const int upper = 0b0000;
    int lower = 0b1100;
    if (cursor_on) {
        if (cursor_blink) {
            lower = 0b1111;
        } else {
            lower = 0b1110;
        }
    } else {
            lower = 0b1100;
    }
    set_nibble(upper);
    enable_pulse();
    __delay_cycles(5000);
    set_nibble(lower);
    enable_pulse();
    __delay_cycles(5000);
}

void write_by_ascii(char ch) {
    int ascii_ch = ch;
    int ascii_ch_upper = ch >> 4;
    write_character(ascii_ch_upper, ascii_ch);
}

void location_by_coords(int row, int column) {
    int upper = 0b1000;
    int lower = column - 1;
    if (row == 1) {
        upper = 0b1000;
    } else if (row == 2) {
        upper = 0b1100;
    } else {
        upper = 0b1000;
    }
    set_cursor_location(upper, lower);
} 

void pressed_char(char ch) {
    int ascii_ch = ch;
    location_by_coords(2, 16);
    write_by_ascii(ascii_ch);
}

void write_word(char *word) {
    location_by_coords(1, 1);
    int i;
    for (i = 0; word[i] != '\0'; i++) {
        write_by_ascii(word[i]);
    }
}

void char_test() {
    location_by_coords(1, 1);
    int upper;
    int lower;
    int current_col = 0;
    for (upper = 0; upper < 32; upper++) {
        for (lower = 0; lower < 32; lower++) {
            if (current_col != 16) {
                write_character(upper, lower);
                current_col++;
            } else {
                location_by_coords(1, 1);
                write_character(upper, lower);
                current_col = 1;
            }
        }
    }
}

void display_window_size(char win) {
    location_by_coords(2, 14);
    write_by_ascii('N');
    write_by_ascii('=');
    write_by_ascii(win);
}

void display_temp(float temp) {
    int temp_tens = ((int)temp / 10);
    int temp_ones = ((int)temp % 10);
    int temp_tenths = (((int)(10 * temp) % 100) % 10);
    location_by_coords(2, 1);
    write_by_ascii('T');
    write_by_ascii('=');
    write_by_ascii(temp_tens + 48);
    write_by_ascii(temp_ones + 48);
    write_by_ascii('.');
    write_by_ascii(temp_tenths + 48);
}

void pattern_name(int pattern) {
    location_by_coords(1, 1);
    switch (pattern) {
                case 0:
                    write_word("static        ");
                    break;
                case 1:
                    write_word("toggle        ");
                    break;
                case 2:
                    write_word("up counter    ");
                    break;
                case 3:
                    write_word("in and out    ");
                    break;
                case 4:
                    write_word("down counter  ");
                    break;
                case 5:
                    write_word("rotate 1 left ");
                    break;
                case 6:
                    write_word("rotate 7 right");
                    break;
                case 7:
                    write_word("fill left     ");
                    break;
                case 8:
                    write_word("              ");
                    break;
                case 9: 
                    write_word("set window size");
                    break;
                case 10: 
                    write_word("set pattern");
                    break;
            }
}
