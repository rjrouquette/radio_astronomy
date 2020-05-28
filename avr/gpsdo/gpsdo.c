//
// Created by robert on 5/27/20.
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include "gpsdo.h"
#include "nop.h"

#define LED_PORT (PORTD)
#define LED0 (1u)
#define LED1 (2u)

volatile uint16_t ppsOffset;
volatile uint16_t pllFeedback;

void setPpsOffset(uint16_t offset);

inline void ledOn(uint8_t mask) {
    LED_PORT.OUTSET = mask;
}

inline void ledOff(uint8_t mask) {
    LED_PORT.OUTCLR = mask;
}

void initGPSDO() {
    ppsOffset = 0;

    // init LEDs
    LED_PORT.DIRSET = LED0 | LED1;
    ledOff(LED0);
    ledOff(LED1);

    // init DAC
    DACB.CTRLC = 0x08u; // AVCC Ref
    DACB.CTRLB = 0x00u; // Enable Channel 0
    DACB.CTRLA = 0x05u; // Enable Channel 0
    while(!(DACB.STATUS & 0x01u)) nop();
    DACB.CH0DATA = 2048u; // start at mid-scale
    pllFeedback = 2048u;

    // PPS Capture
    PORTA.DIRCLR = 0xc0u; // pin 6 + 7
    PORTA.PIN6CTRL = 0x01u; // rising edge
    PORTA.PIN7CTRL = 0x01u; // rising edge
    EVSYS.CH5MUX = 0x56u; // pin PA6
    EVSYS.CH5CTRL = 0x00u;
    EVSYS.CH6MUX = 0x57u; // pin PA7
    EVSYS.CH6CTRL = 0x00u;

    // PPS Prescaling
    TCC1.PER = 399u;
    // OVF carry
    EVSYS.CH7MUX = 0xc8u;
    EVSYS.CH7CTRL = 0x00u;

    // PPS Generation
    PORTC.DIRSET = 0x03u;
    TCC0.CTRLA = 0x0fu;
    TCC0.CTRLB = 0x33u;
    TCC0.PER = 62499u;
    setPpsOffset(0);

    // PPS Capture
    TCD0.CTRLA = 0x0fu;
    TCD0.CTRLB = 0x30u;
    TCD0.CTRLD = 0x2du;
    TCD0.INTCTRLB = 0x03u;
    TCD0.PER = 62499u;
    TCD0.CCA = 0u;
    TCD0.CCB = 0u;

    // start timers
    TCC0.CNT = 0;
    TCD0.CNT = 0;
    TCC1.CTRLA = 0x01u;
}

void setPpsOffset(uint16_t offset) {
    if(offset <= 56249) {
        TCC0.CCA = offset;
        TCC0.CCB = offset + 6250;
        PORTC.PIN0CTRL = 0x00u;
    } else {
        TCC0.CCA = offset;
        TCC0.CCB = offset - 56249;
        PORTC.PIN0CTRL = 0x40u;
    }
    ppsOffset = offset;
}

void decFeedback() {
    if(pllFeedback > 0) {
        DACB.CH0DATA = --pllFeedback;
    }
}

void incFeedback() {
    if(pllFeedback < 4095) {
        DACB.CH0DATA = ++pllFeedback;
    }
}

// PPS leading edge
ISR(TCD0_CCA_vect, ISR_BLOCK) {
    if(PORTB.IN & 1u) {
        incFeedback();
        ledOn(LED1);
    } else {
        decFeedback();
        ledOff(LED1);
    }

    // faster alignment
    uint16_t a = TCD0.CCA;
    uint16_t b = TCD0.CCB;
    uint16_t delta;
    if(a > b)
        delta = a - b;
    else
        delta = b - a;
    if(delta > 31250)
        delta = 62500 - delta;
    if(delta > 62) {
        setPpsOffset(a);
    }
}
