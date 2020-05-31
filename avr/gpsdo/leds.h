//
// Created by robert on 5/31/20.
//

#ifndef GPSDO_LEDS_H
#define GPSDO_LEDS_H

#include <avr/io.h>

#define LED_PORT (PORTD)
#define LED0 (1u)
#define LED1 (2u)

inline void ledOn(uint8_t mask) {
    LED_PORT.OUTSET = mask;
}

inline void ledOff(uint8_t mask) {
    LED_PORT.OUTCLR = mask;
}

inline void initLEDs() {
    // init LEDs
    LED_PORT.DIRSET = LED0 | LED1;
    ledOff(LED0);
    ledOff(LED1);
}

#endif //GPSDO_LEDS_H
