#include <msp430.h>

#include "msp430fr2310.h"
#include "src/LED_Patterns.h"


// Timing Variables
//static float base_period = 1.0;         // Default to 1 second
static uint16_t base_period_units = 4;  // 4 units = 1.00 sec, unit = 0.25s
static uint8_t pattern_scale_divisor = 1;     // 1 = full period, 2 = half period

static unsigned int pattern_step1 = 0;
static unsigned int pattern_step2 = 0;
static unsigned int pattern_step3 = 0;
volatile uint8_t i2c_idle_counter = 0;
#define I2C_IDLE_TIMEOUT 5  // 2 seconds


//static unsigned int count = 0;
static led_pattern_t current_pattern = Pattern_Off;
static bool pattern_active = false;
volatile bool update_flag = false;

// -- Correct LED Patterns
static const unsigned char Pattern_0_P1 = (LED5 | LED7);  // P1 LEDs
static const unsigned char Pattern_0_P2 = (LED1 | LED3);         // P2 LEDs

static const unsigned char Pattern_1_P1[2] = {  
    (LED5 | LED7),         // Step 0: 1 0 1 0 1 0 1 0 (Same as Pattern 0)
    (LED4 | LED6 | LED8)   // Step 1: 0 1 0 1 0 1 0 1 (Inverted)
};

static const unsigned char Pattern_1_P2[2] = {  
    (LED1 | LED3),         // Step 0: 1 0 1 0 1 0 1 0 (Same as Pattern 0)
    (LED2)                 // Step 1: 0 1 0 1 0 1 0 1 (Inverted)
};

static const unsigned char Pattern_3_P1[6] = {  
    (LED4 | LED5),      // Step 0: 0 0 0 1 1 0 0 0
    (LED6),      // Step 1: 0 0 1 0 0 1 0 0
    (LED7),      // Step 2: 0 1 0 0 0 0 1 0
    (LED8),      // Step 3: 1 0 0 0 0 0 0 1
    (LED7),      // Step 4: 0 1 0 0 0 0 1 0
    (LED6)       // Step 5: 0 0 1 0 0 1 0 0
};

static const unsigned char Pattern_3_P2[6] = {  
    (0),                // Step 0
    (LED3),                // Step 1
    (LED2),             // Step 2
    (LED1),                // Step 3
    (LED2),             // Step 4
    (LED3)                 // Step 5
};




// Timer_B0 Configuration (Updated)
void setupTimer(void) {
    TB0CTL = TBCLR;  // Clear Timer
    TB0CTL |= TBSSEL__SMCLK;  // Use SMCLK
    TB0CTL |= MC__UP;  // Up mode
    TB0CTL |= ID__8;   // Divide input clock by 8
    TB0EX0 = TBIDEX__7; // Further divide by 7

    TB0CCR0 = 18858;    // Maximum period (~1 sec) (NEEDS REFINING)

    TB0CCTL0 = CCIE;   // Enable interrupt
    TB0CCTL0 &= ~CCIFG;  // Clear interrupt flag

    __enable_interrupt();
}

void updateTimerPeriod(void) {
    uint16_t scaled_units = base_period_units / pattern_scale_divisor;
    TB0CTL |= TBCLR;  // Stop and clear the timer

    // Each unit is 0.25 seconds
    // 0.25s = 4714 ticks with current timer setup
    TB0CCR0 = 4714 * scaled_units;
    TB0CTL |= MC__UP;  // Resume in Up mode
}

void increaseTimerPeriod(void) {
    if (base_period_units < 4) {
        base_period_units += 1;
        updateTimerPeriod();
    }
}

void decreaseTimerPeriod(void) {
    if (base_period_units > 1) {
        base_period_units -= 1;
        updateTimerPeriod();
    }
}


void init_heartbeat(void) {
    // Set P1.0 as output
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    // Timer_B1 CCR0 setup (1 Hz assuming ACLK = 32.768 kHz)
    TB1CCTL0 = CCIE;              // Enable interrupt
    TB1CCR0 = 32768;              // 1 second period
    TB1CTL = TBSSEL__ACLK | MC__UP | TBCLR;  // ACLK, up mode, clear
}


// Initialize LED GPIOs
void array_init(void) {
    PM5CTL0 &= ~LOCKLPM5;

    P1DIR |= (BIT0 | LED4 | LED5 | LED6 | LED7 | LED8);  // Set P1 LEDs as outputs
    P2DIR |= (LED1 | LED2 | LED3);                // Set P2 LEDs as outputs

    // Set P2.6 & P2.7 to GPIO mode
    P2SEL0 &= ~(BIT6 | BIT7);
    P2SEL1 &= ~(BIT6 | BIT7);

    P1OUT = 0x00;  // Turn off P1 LEDs
    P2OUT = 0x00;  // Turn off P2 LEDs
}

void selectPattern(led_pattern_t pattern) {
    if (pattern == current_pattern) {
        switch (current_pattern) {
            case Pattern_1_Toggle:
                pattern_step1 = 0;
                break;
            case Pattern_2_Up:
                pattern_step2 = 0;
                break;
            case Pattern_3_In_Out:
                pattern_step3 = 0;
                break;
        }
    } else {
        current_pattern = pattern;
    }

    // Update timer period based on pattern's spec
    switch (pattern) {
        case Pattern_1_Toggle:
            pattern_scale_divisor = 1;  // Full base period
            break;
        case Pattern_2_Up:
        case Pattern_3_In_Out:
            pattern_scale_divisor = 2;  // Half base period
            break;
        default:
            pattern_scale_divisor = 1;
            break;
    }

    updateTimerPeriod();  // Recalculate based on scale
    pattern_active = true;
}


void updatePattern(void) {
    if (!pattern_active) return;

    switch (current_pattern) {
        case Pattern_0_Static:
            LED_PORT1 = (LED_PORT1 & ~LED_MASK_P1) | (Pattern_0_P1 & LED_MASK_P1);
            LED_PORT2 = Pattern_0_P2;
            break;
        
        case Pattern_1_Toggle:
            //LED_PORT1 = Pattern_1_P1[pattern_step1];
            LED_PORT1 = (LED_PORT1 & ~LED_MASK_P1) | (Pattern_1_P1[pattern_step1] & LED_MASK_P1);
            LED_PORT2 = Pattern_1_P2[pattern_step1];
            pattern_step1 = (pattern_step1 + 1) % 2;  // Toggle between 2 states
            break;

        case Pattern_2_Up: {
            uint8_t count = pattern_step2;

            // Debug: Show count value
            //P1OUT ^= (count & 1) ? BIT4 : 0;  // Blink P1.4 when LSB is 1

            // Map bits to Port 2
            LED_PORT2 = 0;
            if (count & (1 << 0)) LED_PORT2 |= LED1;
            if (count & (1 << 1)) LED_PORT2 |= LED2;
            if (count & (1 << 2)) LED_PORT2 |= LED3;

            // Map bits to Port 1
            uint8_t new_leds = 0;
            if (count & (1 << 3)) new_leds |= LED4;
            if (count & (1 << 4)) new_leds |= LED5;
            if (count & (1 << 5)) new_leds |= LED6;
            if (count & (1 << 6)) new_leds |= LED7;
            if (count & (1 << 7)) new_leds |= LED8;

            LED_PORT1 = (LED_PORT1 & ~LED_MASK_P1) | (new_leds & LED_MASK_P1);

            pattern_step2 = (pattern_step2 + 1) & 0xFF;  // Wrap after 255
            break;
        }

        case Pattern_3_In_Out:
            LED_PORT1 = (LED_PORT1 & ~LED_MASK_P1) | (Pattern_3_P1[pattern_step3] & LED_MASK_P1);
            LED_PORT2 = Pattern_3_P2[pattern_step3];
            pattern_step3 = (pattern_step3 + 1) % 6;  // Cycle through 6 steps
            break;

        default:
            LED_PORT1 &= ~LED_MASK_P1;
            LED_PORT2 = 0x00;
            break;    
    }
}

void i2c_init_slave(void) {
    // Set P1.2 (SDA) and P1.3 (SCL) for I2C functionality
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    P1REN |= BIT2 | BIT3;  // Enable resistor on P1.2 (SDA) and P1.3 (SCL)
    P1OUT |= BIT2 | BIT3;  // Set as pull-up


    // Put eUSCI_B0 into reset
    UCB0CTLW0 = UCSWRST;

    // I2C mode, synchronous
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;

    // 7-bit slave address
    UCB0I2COA0 = SLAVE_ADDR | UCOAEN;

    // Clear reset to start operation
    UCB0CTLW0 &= ~UCSWRST;

    // Enable I2C interrupts
    UCB0IE |= UCRXIE0 | UCSTPIE | UCNACKIE;
}




int main(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;
    array_init();              // Initialize LEDs and timer
    setupTimer();              // Start Timer_B0 for LED patterns
    init_heartbeat();         // Start the heartbeat LED
    i2c_init_slave();

    __enable_interrupt();
    __bis_SR_register(GIE);        // Enable global interrupts
    //selectPattern(Pattern_1_Toggle);  // Start with a default pattern

    while (1) {
        if (update_flag) {
            update_flag = false;
            updatePattern();
        }
    }
}




// Timer_B0 ISR (Triggers Pattern Updates)
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void) {
    update_flag = true;
    
    if (i2c_idle_counter < 255)
        i2c_idle_counter++;

    if (i2c_idle_counter >= I2C_IDLE_TIMEOUT) {
        P1OUT &= ~BIT0; // Turn off LED if timeout reached
    }
    TB0CCTL0 &= ~CCIFG;  
}

// I2C
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void) {
    switch (__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG)) {
        case USCI_I2C_UCRXIFG0: {
            uint8_t received = UCB0RXBUF;

            // Reset inactivity timer + turn on status LED
            i2c_idle_counter = 0;
            P1OUT |= BIT0;

            // Use recieved byte to select pattern
            if (received <= Pattern_3_In_Out) {
                selectPattern((led_pattern_t)received);
            }
            // Handle speed control
            else if (received == 'A') {
                decreaseTimerPeriod();
            }
            else if (received == 'B') {
                increaseTimerPeriod();
            }

            break;
        }
        
        case USCI_I2C_UCSTPIFG: {
            // Stop condition recieved
            UCB0IFG &= ~UCSTPIFG;
            break;
        }

        case USCI_I2C_UCNACKIFG: {
            // NACK handling (just in case)
            UCB0CTLW0 |= UCTXNACK;
            break;
        }

        default:
            break;
    }
}

// Timer_B1 CCR0 ISR
#pragma vector = TIMER1_B0_VECTOR
__interrupt void Timer_B1_ISR(void) {
    P1OUT ^= BIT0;   // Toggle heartbeat LED
}
