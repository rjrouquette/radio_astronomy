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

#define MAX_PPS_DELTA (4) // 64 microseconds
#define MAX_PLL_INTERVAL (64) // 64 PPS events
#define SETTLED_VAR (40000) // 25 microseconds RMS

volatile uint16_t ppsOffset;
volatile uint16_t pllFeedback;
volatile uint16_t pllInterval;
volatile uint16_t pllDivisor;

volatile uint8_t prevPllUpdate;
volatile uint8_t statsIndex;
volatile int16_t error[64];
volatile uint16_t interval[64];
volatile uint8_t realigned[64];

uint8_t pllLocked;
int32_t pllError;
int32_t pllErrorVar;


void setPpsOffset(uint16_t offset);

inline void ledOn(uint8_t mask) {
    LED_PORT.OUTSET = mask;
}

inline void ledOff(uint8_t mask) {
    LED_PORT.OUTCLR = mask;
}

void initGPSDO() {
    ppsOffset = 0;
    pllFeedback = 0;
    pllInterval = 1;
    pllDivisor = 1;
    statsIndex = 0;
    prevPllUpdate = 0;
    pllLocked = 0;
    pllError = 0;
    pllErrorVar = 0;

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
    TCC1.CTRLB = 0x30u;
    TCC1.CTRLD = 0x2du;
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
    TCD0.CTRLD = 0x3du;
    TCD0.INTCTRLB = 0x03u;
    TCD0.PER = 62499u;
    TCD0.CCA = 0u;
    TCD0.CCB = 0u;

    // start timers
    TCC0.CNT = 0;
    TCD0.CNT = 0;
    TCC1.CTRLA = 0x01u;
}

uint8_t isPllLocked() {
    return pllLocked;
}

uint16_t getPllInterval() {
    return pllInterval;
}

int32_t getPllError() {
    return pllError;
}

int32_t getPllErrorVar() {
    return pllErrorVar;
}

void setPpsOffset(uint16_t offset) {
    if(offset < 56249) {
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

int16_t getDelta(uint16_t a0, uint16_t a1, uint16_t b0, uint16_t b1) {
    int32_t a = (a1 * 400) * a0;
    int32_t b = (b1 * 400) * b0;
    // modulo difference
    int32_t diff = a - b;
    if(diff > 12499999)
        diff = 25000000 - diff;
    if(diff < -125000000)
        diff = 25000000 + diff;

    if(diff > 32767)
        return 32767;
    if(diff < -32768)
        return -32768;
    return (int16_t) diff;
}

inline void decFeedback() {
    if(pllFeedback > 0u) {
        DACB.CH0DATA = --pllFeedback;
    }
}

inline void incFeedback() {
    if(pllFeedback < 4095u) {
        DACB.CH0DATA = ++pllFeedback;
    }
}

inline void alignPPS() {
    // faster alignment
    uint16_t a = TCD0.CCA;
    uint16_t b = TCD0.CCB;
    uint16_t delta;
    if(a > b)
        delta = a - b;
    else
        delta = b - a;
    if(delta > 31249)
        delta = 62500 - delta;

    if(delta > MAX_PPS_DELTA) {
        setPpsOffset(a);
        realigned[statsIndex] = 1;
    } else {
        realigned[statsIndex] = 0;
    }
}

inline uint8_t checkPllDivisor() {
    if(--pllDivisor > 0) {
        return 0;
    }
    pllDivisor = pllInterval;
    return 1;
}

inline void onRisingPPS() {
    // check if PPS was realigned
    alignPPS();

    // check if it is time to update the PLL
    if(!checkPllDivisor())
        return;

    // update PLL feedback
    if(PORTB.IN & 1u) {
        incFeedback();
    } else {
        decFeedback();
    }

    error[statsIndex] = getDelta(
            TCC1.CCA, TCD0.CCA,
            TCC1.CCB, TCD0.CCB
    );
    interval[statsIndex] = pllInterval;
    statsIndex = (statsIndex + 1u) & 63u;
}

// PPS leading edge
ISR(TCD0_CCA_vect, ISR_BLOCK) {
    onRisingPPS();
}

void updatePLL() {
    if(statsIndex == prevPllUpdate)
        return;
    prevPllUpdate = statsIndex;

    ledOn(LED0);
    if(pllLocked)
        ledOn(LED1);

    uint8_t unstable = 0;
    int32_t acc = 0;
    for(uint8_t i = 0; i < 64; i++) {
        acc += error[i];
        unstable |= realigned[i];
    }
    pllError = acc / 8;

    int16_t mean = (int16_t) (acc / 64);
    acc = 0;
    for(uint8_t i = 0; i < 64; i++) {
        int32_t diff = error[i] - mean;
        acc += diff * diff;
    }
    pllErrorVar = acc;

    if(unstable) {
        // PLL lock has been broken, start over
        pllLocked = 0;
        pllInterval = 1;
        pllDivisor = 1;
    } else {
        // gradually reduce update interval to improve stability
        pllLocked = 1;
        if(pllInterval < MAX_PLL_INTERVAL) {
            uint16_t match = interval[0];
            for(uint8_t i = 1; i < 64; i++) {
                if(match != interval[i]) {
                    match = 0;
                    break;
                }
            }
            if(match) {
                pllInterval = pllInterval << 1u;
            }
        }
    }

    ledOff(LED0);
    if((!pllLocked) || (pllErrorVar > SETTLED_VAR))
        ledOff(LED1);
}