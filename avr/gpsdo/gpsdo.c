//
// Created by robert on 5/27/20.
//

#include "gpsdo.h"
#include <avr/io.h>

#define LED_PORT (PORTD)
#define LED0 (2u)
#define LED1 (4u)

inline void ledOn(uint8_t mask) {
    LED_PORT.OUTSET = mask;
}

inline void ledOff(uint8_t mask) {
    LED_PORT.OUTCLR = mask;
}

void initGPSDO() {
    // init LEDs
    LED_PORT.OUTSET = LED0 | LED1;

    ledOn(LED0);
}
