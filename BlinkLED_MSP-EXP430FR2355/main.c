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
            display_window_size('3');
            display_temp(30.4);
            display_window_size('4');
            display_temp(15.2);
            display_window_size('9');
        }
    }
}

