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

// hi-res counter
#define DIV_LSB (800)
// low-res counter
#define DIV_MSB (62500)
// combined counters
#define DIV_ALL    ( 50000000)
#define MOD_ALL_HI ( 24999999)
#define MOD_ALL_LO (-25000000)
// clock cycle length
#define CLK_NS (20)

// hard PPS offset adjustment threshold
// PPS can can be realigned in 16 us steps
// tracking error has 1 ns resolution
#define MAX_PPS_ERROR (20000) // 20 microseconds

// loop tuning
#define MAX_FB (0xfff0u) // 4095 * 16
#define ZERO_FB (0x7ff8u) // 2047.5 * 16

// statistics calculation
#define RING_SIZE (64u)
#define SETTLED_VAR (250) // 250 nanoseconds RMS

// PID control loop
#define ERROR_LIMIT (2048) // 2.048 microseconds
#define PID_RES (1000)
// stable lock
#define PID_P (50)          // 0.050 = 119.24 ppt / 20ns
#define PID_I (4)           // 0.004 = (1.0 ns / 119.24 ppt) / 2048.0 s
#define PID_D (0)           // 0
// initial lock
#define PID_P_FAST (2097)   // 2.097 = (1.0 ns / 119.24 ppt) / 4.0 s
#define PID_I_FAST (66)     // 0.066 = (1.0 ns / 119.24 ppt) / 128.0 s
#define PID_D_FAST (0)      // 0

// ppm scalar (effective ppm per bit with +/- 6.25ppm pull range)
#define PPM_SCALE (119.24e-6f)

volatile uint16_t pllFeedback;

// PID control loop
volatile int32_t prevError;
volatile int32_t integrator;
volatile int32_t cP;
volatile int32_t cI;
volatile int32_t cD;

volatile uint8_t statsIndex;
volatile uint16_t adc_temp[RING_SIZE];
volatile int32_t error[RING_SIZE];
volatile uint8_t realigned[RING_SIZE];

volatile uint8_t pllLocked;
volatile uint8_t pllSettled;
volatile float pllError;
volatile float pllErrorRms;
volatile float pllAdjustment;
volatile float pllTemperature;

#define CAL_SECOND_TEMP (41.0f)
#define CAL_SECOND_OFFSET (2245)
float kelvin_per_adc;

void setPpsOffset(uint16_t offset);
uint8_t readProdByte(const volatile uint8_t *offset);

void initGPSDO() {
    pllFeedback = 0;
    statsIndex = 0;
    pllLocked = 0;
    pllSettled = 0;
    pllError = 0;
    pllErrorRms = 0;
    pllAdjustment = 0;
    pllTemperature = 0;
    integrator = 0;
    prevError = 0;
    cP = 0;
    cI = 0;
    cD = 0;

    // init DAC
    DACB.CTRLC = 0x09u; // AVCC Ref, left-aligned
    DACB.CTRLB = 0x00u; // Enable Channel 0
    DACB.CTRLA = 0x05u; // Enable Channel 0
    while(!(DACB.STATUS & 0x01u)) nop();
    DACB.CH0DATA = ZERO_FB;
    pllFeedback = ZERO_FB;

    // DAC Twiddling
    TCD1.CTRLB = 0x30u;
    TCD1.CTRLD = 0x2du;
    TCD1.PER = 499u;
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
    // ISR is located in "net/dhcp_client.c" to reduce overhead
    TCD0.INTCTRLA = 0x02u;
    TCD0.PER = DIV_MSB - 1u;
    TCD0.CCA = 0u;
    TCD0.CCB = 0u;

    // start timers
    TCC0.CNT = 0;
    TCD0.CNT = 0;
    TCC1.CTRLA = 0x01u;

    // configure ADC for temperature sensor
    // cancel any pending conversions, disable ADC
    ADCA.CTRLA = ADC_FLUSH_bm;
    // set up exactly how Atmel did when they measured the calibration value
    ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc; // unsigned conversion, produces result in range 0-2048
    ADCA.REFCTRL = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm;
    ADCA.PRESCALER = ADC_PRESCALER_DIV512_gc;
    ADCA.CALL = readProdByte(&PRODSIGNATURES_ADCACAL0);
    ADCA.CALH = readProdByte(&PRODSIGNATURES_ADCACAL1);
    ADCA.CTRLA = ADC_ENABLE_bm;
    // configure channel zero for temperature
    ADCA.CH0.CTRL = ADC_CH_INPUTMODE_INTERNAL_gc | ADC_CH_GAIN_1X_gc;
    ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;

    // get 358 K factory calibrated value
    uint16_t ref = readProdByte(&PRODSIGNATURES_TEMPSENSE1);
    ref <<= 8u;
    ref |= readProdByte(&PRODSIGNATURES_TEMPSENSE0);
    kelvin_per_adc = (85.0f - CAL_SECOND_TEMP) / (float) (ref - CAL_SECOND_OFFSET); // reference is ADC reading at 85C

    // prepare first sample
    ADCA.CH0.INTFLAGS = 0x01u;
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while(!(ADCA.CH0.INTFLAGS & 0x01u)) nop();
    adc_temp[0] = ADCA.CH0.RES;
    for(uint8_t i = 1; i < RING_SIZE; i++) {
        adc_temp[i] = adc_temp[0];
    }
    // ready next sample
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
}

uint8_t isPllLocked() {
    return pllSettled;
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

float getPllTemperature() {
    return pllTemperature;
}

void setPpsOffset(uint16_t offset) {
    if(offset < 56250u) {
        TCC0.CCA = offset;
        TCC0.CCB = offset + 6250u;
        PORTC.PIN0CTRL = 0x00u;
    } else {
        TCC0.CCA = offset;
        TCC0.CCB = offset - 56249u;
        PORTC.PIN0CTRL = 0x40u;
    }
}

// A = gPPS, B = uPPS
int32_t getDelta(uint16_t lsbA, uint16_t msbA, uint16_t lsbB, uint16_t msbB) {
    int32_t a = msbA; a *= DIV_LSB; a += lsbA;
    int32_t b = msbB; b *= DIV_LSB; b += lsbB;

    // modulo difference
    b -= a;
    if(b > MOD_ALL_HI)
        b = DIV_ALL - b;
    if(b < MOD_ALL_LO)
        b = DIV_ALL + b;

    if(b < 1) {
        b *= CLK_NS;
        b -= CLK_NS / 2;
    } else {
        b -= 1;
        b *= CLK_NS;
        b += CLK_NS / 2;
    }

    return b;
}

// update PID control loop
void updatePID(int32_t currError) {
    // restrict error range
    if(currError > ERROR_LIMIT)
        currError = ERROR_LIMIT;
    if(currError < -ERROR_LIMIT)
        currError = -ERROR_LIMIT;

    // update integrator
    integrator += currError * cI;

    // compute delta error
    int32_t deltaError = currError - prevError;
    prevError = currError;

    // compute feedback
    int32_t fb = integrator;
    fb += currError * cP;
    fb += deltaError * cD;
    fb /= PID_RES;
    fb += ZERO_FB;

    // limit feedback range
    if(fb > MAX_FB) fb = MAX_FB;
    if(fb < 0) fb = 0;

    // apply feedback
    pllFeedback = fb;
}

void onRisingPPS() {
    ledOn(LED0);

    // measure tracking error
    int32_t currError = getDelta(
            TCC1.CCA, TCD0.CCA,
            TCC1.CCB, TCD0.CCB
    );

    // update PID loop
    updatePID(currError);

    // force PPS realignment if necessary
    if(
        currError >  MAX_PPS_ERROR ||
        currError < -MAX_PPS_ERROR
    ) {
        setPpsOffset(TCD0.CCA);
        realigned[statsIndex] = 1;
    } else {
        realigned[statsIndex] = 0;
    }

    // update status ring
    error[statsIndex] = currError;
    adc_temp[statsIndex] = ADCA.CH0.RES;
    statsIndex = (statsIndex + 1u) & (RING_SIZE - 1u);

    // update statistics
    pllAdjustment = (float) pllFeedback;
    pllAdjustment -= ZERO_FB;
    pllAdjustment *= PPM_SCALE;

    pllLocked = 1;
    int64_t acc = 0;
    for(uint8_t i = 0; i < RING_SIZE; i++) {
        acc += error[i];
        pllLocked &= (realigned[i] ^ 1u);
    }
    pllError = (float) acc;
    pllError /= RING_SIZE;

    acc = 0;
    for(uint8_t i = 0; i < RING_SIZE; i++) {
        int32_t diff = error[i];
        acc += diff * diff;
    }
    pllErrorRms = (float) ((acc < 0) ? -acc : acc);
    pllErrorRms /= RING_SIZE;
    pllErrorRms = sqrtf(pllErrorRms);

    // determine if loop has settled
    pllSettled = (pllLocked && (pllErrorRms <= SETTLED_VAR));
    if (pllSettled) {
        cP = PID_P;
        cI = PID_I;
        cD = PID_D;
    } else {
        cP = PID_P_FAST;
        cI = PID_I_FAST;
        cD = PID_D_FAST;
        ledOff(LED0);
    }

    // compute temperature
    acc = 0;
    for(uint8_t i = 0; i < RING_SIZE; i++) {
        acc += adc_temp[i];
    }
    pllTemperature = (float) acc;
    pllTemperature /= RING_SIZE;
    pllTemperature -= CAL_SECOND_OFFSET;
    pllTemperature *= kelvin_per_adc;
    pllTemperature += CAL_SECOND_TEMP;

    // prepare next temperature sample
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
}

// DAC output twiddling
static volatile uint8_t twiddle = 0;
ISR(TCD1_OVF_vect, ISR_BLOCK) {
    uint8_t t = twiddle;
    DACB.CH0DATA = pllFeedback + t;
    t += 0x01u;
    t &= 0x0fu;
    twiddle = t;
}

// PPS leading edge
ISR(TCD0_CCA_vect, ISR_NOBLOCK) {
    onRisingPPS();
}
