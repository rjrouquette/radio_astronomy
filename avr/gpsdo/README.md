# GPSDO Firmware
Atmel XMega16A4U source code for PPS generation, GPS PLL, and UDP status engine.

The uC generates its own PPS signal for comparison with the GPS PPS signal by cascading two of the 16-bit counters to fully divide the 25 MHz clock into one second intervals and an external XOR gate to combine the timer comparison outputs.  The phase alignment between the two PPS signals is performed using an external D-type flip-flop that captures the GPS PPS level on the rising edge of the uC PPS signal.

The uC closes the PPS PLL by using the external phase comparison to increment/decrement the DAC output level.  Additional loop stability improvements such as PPS edge stepping and a dynamic update interval are implemented in software to improve settling time and reduce oscillations.

Tracking statistics are also collected using an additional 16-bit timer to capture the relative timing of the two PPS sigmals.  This information is accessible via the UDP network interface.
