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

// loop tuning
#define MAX_PPS_DELTA (4) // 64 microseconds
#define MAX_PLL_INTERVAL (64) // 64 PPS events
#define SETTLED_VAR (40000) // 1 microsecond RMS
#define RING_SIZE (64u)
#define RING_DIV (8u)

// hi-res counter
#define DIV_LSB (400)

// low-res counter
#define DIV_MSB    ( 62500)
#define MOD_MSB_HI ( 31249)
#define MOD_MSB_LO (-31250)

// combined counters
#define DIV_ALL    ( 25000000)
#define MOD_ALL_HI ( 12499999)
#define MOD_ALL_LO (-12500000)

volatile uint16_t ppsOffset;
volatile uint16_t pllFeedback;
volatile uint16_t pllInterval;
volatile uint16_t pllDivisor;

volatile uint8_t prevPllUpdate;
volatile uint8_t statsIndex;
volatile int16_t error[RING_SIZE];
volatile uint16_t interval[RING_SIZE];
volatile uint8_t realigned[RING_SIZE];

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
    TCC1.PER = DIV_LSB - 1u;
    // OVF carry
    EVSYS.CH7MUX = 0xc8u;
    EVSYS.CH7CTRL = 0x00u;

    // PPS Generation
    PORTC.DIRSET = 0x03u;
    TCC0.CTRLA = 0x0fu;
    TCC0.CTRLB = 0x33u;
    TCC0.PER = DIV_MSB - 1u;
    setPpsOffset(0);

    // PPS Capture
    TCD0.CTRLA = 0x0fu;
    TCD0.CTRLB = 0x30u;
    TCD0.CTRLD = 0x3du;
    TCD0.INTCTRLB = 0x03u;
    TCD0.PER = DIV_MSB - 1u;
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
    if(offset < 56250) {
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

int16_t getDelta(uint16_t lsbA, uint16_t msbA, uint16_t lsbB, uint16_t msbB) {
    int32_t a = (((uint32_t)msbA) * DIV_LSB) + lsbA;
    int32_t b = (((uint32_t)msbB) * DIV_LSB) + lsbB;

    // modulo difference
    int32_t diff = a - b;
    if(diff > MOD_ALL_HI)
        diff = DIV_ALL - diff;
    if(diff < MOD_ALL_LO)
        diff = DIV_ALL + diff;

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
    int32_t a = (uint32_t) TCD0.CCA;
    int32_t b = (uint32_t) TCD0.CCB;

    // modulo difference
    int32_t diff = a - b;
    if(diff > MOD_MSB_HI)
        diff = DIV_MSB - diff;
    if(diff < MOD_MSB_LO)
        diff = DIV_MSB + diff;

    // force PPS alignment if outside tolerance
    if(diff < 0) diff = -diff;
    if(diff > MAX_PPS_DELTA) {
        setPpsOffset(TCD0.CCA);
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
    statsIndex = (statsIndex + 1u) & (RING_SIZE - 1u);
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
    for(uint8_t i = 0; i < RING_SIZE; i++) {
        acc += error[i];
        unstable |= realigned[i];
    }
    pllError = acc / RING_DIV;

    int16_t mean = (int16_t) (acc / RING_SIZE);
    acc = 0;
    for(uint8_t i = 0; i < RING_SIZE; i++) {
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
            for(uint8_t i = 1; i < RING_SIZE; i++) {
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
