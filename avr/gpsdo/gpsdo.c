//
// Created by robert on 5/27/20.
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include "gpsdo.h"
#include "nop.h"
#include "leds.h"
#include "net/dhcp_client.h"

// loop tuning
#define MAX_FB (4095u << 4u)
#define ZERO_FB (2173u << 4u) // 0 ppm
#define MAX_PPS_DELTA (1) // 16 microseconds
#define SETTLED_VAR (250) // 250 nanoseconds RMS
#define RING_SIZE (64u)
#define RES_NS (40)

// ppm scalar (effective ppm per bit with +/- 170ppm pull range)
#define PPM_SCALE (0.0052506f) // 0.08401f

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

volatile uint8_t dhcpSec;
volatile uint16_t pllFeedback;

volatile int16_t prevPllError;
volatile uint8_t statsIndex;
volatile uint16_t adjustment[RING_SIZE];
volatile int16_t error[RING_SIZE];
volatile uint8_t realigned[RING_SIZE];

uint8_t pllLocked;
uint8_t pllSettled;
float pllError;
float pllErrorRms;
float pllAdjustment;

void setPpsOffset(uint16_t offset);

void initGPSDO() {
    dhcpSec = 0;
    pllFeedback = 0;
    statsIndex = 0;
    pllLocked = 0;
    pllSettled = 0;
    pllError = 0;
    pllErrorRms = 0;
    pllAdjustment = 0;
    prevPllError = 0;

    // init DAC
    DACB.CTRLC = 0x09u; // AVCC Ref, left-aligned
    DACB.CTRLB = 0x00u; // Enable Channel 0
    DACB.CTRLA = 0x05u; // Enable Channel 0
    while(!(DACB.STATUS & 0x01u)) nop();
    DACB.CH0DATA = ZERO_FB;
    pllFeedback = ZERO_FB;
    for(uint8_t i = 0; i < RING_SIZE; i++) {
        adjustment[i] = pllFeedback;
    }

    // DAC Twiddling
    TCD1.CTRLB = 0x30u;
    TCD1.CTRLD = 0x2du;
    TCD1.PER = 99u; // 250 kHz
    // high priority overflow interrupt
    TCD1.INTCTRLA = 0x03u;
    TCD1.CTRLA = 0x01u;

    // PPS Capture
    PORTA.DIRCLR = 0xc0u; // pin 6 + 7
    PORTA.PIN6CTRL = 0x01u; // rising edge
    PORTA.PIN7CTRL = 0x01u; // rising edge
    EVSYS.CH5MUX = 0x56u; // pin PA6 (gPPS)
    EVSYS.CH5CTRL = 0x00u;
    EVSYS.CH6MUX = 0x57u; // pin PA7 (uPPS)
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
    // mid priority capture interrupt
    TCD0.INTCTRLB = 0x02u;
    // mid priority overflow interrupt
    TCD0.INTCTRLA = 0x02u;
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

float getPllError() {
    return pllError;
}

float getPllErrorRms() {
    return pllErrorRms;
}

float getPllFeedback() {
    return pllAdjustment;
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

inline void decFeedback(uint8_t step) {
    if(pllFeedback > (step - 1u)) {
        pllFeedback -= step;
    }
}

inline void incFeedback(uint8_t step) {
    if(pllFeedback < MAX_FB - step) {
        pllFeedback += step;
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
        prevPllError = 0;
    } else {
        realigned[statsIndex] = 0;
    }
}

inline void onRisingPPS() {
    // longer led pulse when locked
    if(pllLocked) ledOn(LED0);

    // check if PPS was realigned
    alignPPS();

    // compute current tracking error
    int16_t currError = getDelta(
            TCC1.CCA, TCD0.CCA,
            TCC1.CCB, TCD0.CCB
    );
    int16_t deltaError = currError - prevPllError;
    int16_t step = currError;
    if(step < 1) step = 1 - step;
    if(step > 16) step = 16;

    // update PLL feedback
    if(PORTB.IN & 1u) {
        if(deltaError < 1) {
            incFeedback(step);
        }
    } else {
        if(deltaError > -1) {
            decFeedback(step);
        }
    }

    // update status ring
    prevPllError = currError;
    error[statsIndex] = currError;
    adjustment[statsIndex] = pllFeedback;
    statsIndex = (statsIndex + 1u) & (RING_SIZE - 1u);

    // update statistics
    ledOn(LED0);
    pllLocked = 1;
    int64_t acc = 0;
    for(uint8_t i = 0; i < RING_SIZE; i++) {
        acc += adjustment[i];
        pllLocked &= (realigned[i] ^ 1u);
    }
    pllAdjustment = (float) acc;
    pllAdjustment /= RING_SIZE;
    pllAdjustment -= ZERO_FB;
    pllAdjustment *= PPM_SCALE;

    acc = 0;
    for(uint8_t i = 0; i < RING_SIZE; i++) {
        acc += error[i];
    }
    pllError = (float) acc;
    pllError /= RING_SIZE;
    pllError *= RES_NS;

    acc = 0;
    for(uint8_t i = 0; i < RING_SIZE; i++) {
        int32_t diff = error[i];
        acc += diff * diff;
    }
    pllErrorRms = (float) ((acc < 0) ? -acc : acc);
    pllErrorRms /= RING_SIZE;
    pllErrorRms = sqrtf(pllErrorRms);
    pllErrorRms *= RES_NS;

    // determine if loop has settled
    pllSettled = (pllLocked && (pllErrorRms <= SETTLED_VAR));
    if (!pllSettled) ledOff(LED0);
}

// DAC output twiddling
static uint8_t twiddle = 0;
ISR(TCD1_OVF_vect, ISR_BLOCK) {
    DACB.CH0DATA = pllFeedback + twiddle;
    twiddle = (twiddle + 1u) & 0xfu;
}

// PPS leading edge
ISR(TCD0_CCA_vect, ISR_NOBLOCK) {
    onRisingPPS();
}

// one second interval
ISR(TCD0_OVF_vect, ISR_NOBLOCK) {
    // increment dhcp counter
    if(++dhcpSec > 5) {
        dhcpSec = 0;
        dhcp_6sec_tick();
    }
}
