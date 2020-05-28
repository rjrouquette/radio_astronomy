#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "gpsdo.h"
#include "nop.h"

void initSysClock(void);

int main(void) {
    cli();
    initSysClock();
    initGPSDO();

    // startup complete
    PMIC.CTRL = 0x04u; // enable high-level interrupts
    sei();

    // infinite loop
    for(;;) {
        updatePLL();
    }

    return 0;
}

void initSysClock(void) {
    // drop down to 2MHz clock before changing PLL settings
    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_RC2M_gc; // Select 2MHz RC OSC
    nop4();

    OSC.XOSCCTRL = OSC_XOSCSEL_EXTCLK_gc;
    OSC.CTRL |= OSC_XOSCEN_bm;
    while(!(OSC.STATUS & OSC_XOSCRDY_bm)); // wait for external clock ready

    CCP = CCP_IOREG_gc;
    OSC.XOSCFAIL = 0x01u; // enable interrupt on clock failure
    nop4();

    CCP = CCP_IOREG_gc;
    CLK.PSCTRL = 0x00u; // no prescaling
    nop4();

    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_XOSC_gc; // Select external clock
    nop4();
}

ISR(OSC_OSCF_vect) {
    // reset xmega
    cli();
    CCP = 0xD8;
    RST.CTRL = RST_SWRST_bm;
}
