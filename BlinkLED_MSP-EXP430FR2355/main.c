#include <msp430fr2355.h>
#include <stdint.h>
#include "lcd.h"
#include "intrinsics.h"

int main(void)
{
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    define_ports();

    // Disable low-power mode / GPIO high-impedance
    PM5CTL0 &= ~LOCKLPM5;

    while (true) {

        start_up();
        start_up();

        while (true) {
            pattern_name(0);
            display_window_size('9');
            display_temp(30.4);
            pattern_name(1);
            display_window_size('8');
            display_temp(20.1);
            pattern_name(2);
            display_window_size('7');
            display_temp(24.6);
            pattern_name(3);
            display_window_size('6');
            display_temp(25.6);
            pattern_name(4);
            display_window_size('5');
            display_temp(35.3);
            pattern_name(5);
            display_window_size('4');
            display_temp(19.9);
            pattern_name(6);
            display_window_size('3');
            display_temp(99.9);
            pattern_name(7);
            display_window_size('2');
            display_temp(11.1);
            pattern_name(8);
            display_window_size('1');
            display_temp(34.2);
            pattern_name(9);
            display_window_size('0');
            display_temp(56.1);
            pattern_name(10);
        }
    }
}

