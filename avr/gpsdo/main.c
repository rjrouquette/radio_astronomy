#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "gpsdo.h"
#include "leds.h"
#include "nop.h"
#include "net/enc28j60.h"
#include "net/ip_arp_udp_tcp.h"
#include "net/dhcp_client.h"

#define LEDON ledOn(LED1)
#define LEDOFF ledOff(LED1)

// interface MAC address
uint8_t macAddr[6] = {0x54, 0x55, 0x58, 0x10, 0x00, 0x29};
//uint8_t macAddr[6] = { 0xa0, 0x36, 0x9f, 0x00, 0x00, 0x00 };
// My own IP (DHCP will provide a value for it):
static uint8_t myip[4]={0,0,0,0};
// Default gateway (DHCP will provide a value for it):
static uint8_t gwip[4]={0,0,0,0};
#define TRANS_NUM_GWMAC 1
static uint8_t gwmac[6]={0,0,0,0,0,0};
// Netmask (DHCP will provide a value for it):
static uint8_t netmask[4]={0,0,0,0};

// packet buffer
#define BUFFER_SIZE 650
static uint8_t buf[BUFFER_SIZE+1];

// gps NEMA buffer, record zero is rx buffer
static volatile uint8_t dateTimeReady = 0;
static volatile uint8_t rx_cnt = 0;
char gpsMsgs[4][83];

void initSysClock(void);
void sendGpsConfig(const char *pstr);
uint32_t appendSimpleHash(uint8_t byte, uint32_t hash);
void initMacAddress();
void arpresolver_result_callback(uint8_t *ip __attribute__((unused)),uint8_t reference_number,uint8_t *mac);

uint16_t http200ok(void) {
    return fill_tcp_data_p(
            buf,
            0,
            PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nPragma: no-cache\r\n\r\n")
    );
}

// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf) {
    // snapshot pll status
    uint8_t locked = isPllLocked();
    float error = getPllError();
    float errRms = getPllErrorRms();
    float fdbk = getPllFeedback();
    float temperature = getPllTemperature();

    char temp[16];
    uint16_t plen;
    plen = http200ok();

    plen = fill_tcp_data_p(buf, plen, PSTR("Temperature: "));
    sprintf(temp, "%.1f", temperature);
    plen = fill_tcp_data(buf, plen, temp);

    plen = fill_tcp_data_p(buf, plen, PSTR(" C\nPLL Stable: "));
    plen = fill_tcp_data_p(buf, plen, locked ? PSTR("yes") : PSTR("no"));

    plen = fill_tcp_data_p(buf, plen, PSTR("\nPLL Error: "));
    sprintf(temp, "%.1f", error);
    plen = fill_tcp_data(buf, plen, temp);

    plen = fill_tcp_data_p(buf, plen, PSTR(" ns\nPLL RMSE: "));
    sprintf(temp, "%.1f", errRms);
    plen = fill_tcp_data(buf, plen, temp);

    plen = fill_tcp_data_p(buf, plen, PSTR(" ns\nPLL Offset: "));
    sprintf(temp, "%.4f", fdbk);
    plen = fill_tcp_data(buf, plen, temp);

    plen = fill_tcp_data_p(buf, plen, PSTR(" ppm\nGPS NEMA:\n"));
    plen = fill_tcp_data(buf, plen, gpsMsgs[1]);
    plen = fill_tcp_data(buf, plen, gpsMsgs[2]);
    plen = fill_tcp_data(buf, plen, gpsMsgs[3]);
    plen = fill_tcp_data(buf, plen, "\n");
    return plen;
}

int main(void) {
    cli();
    initLEDs();
    initSysClock();
    initGPSDO();

    // init USART for 9600 baud (bsel = -4, bscale = 2588)
    USARTC1.BAUDCTRLA = 0x1cu;
    USARTC1.BAUDCTRLB = 0xcau;
    USARTC1.CTRLA = 0x10u;
    USARTC1.CTRLC = 0x03u;
    USARTC1.CTRLB = 0x10u;
    // set PPS clock frequency to 64MHz
    sendGpsConfig(PSTR("$PSTMSETPAR,1197,64*1\r\n"));
    // disable low power mode
    sendGpsConfig(PSTR("$PSTMSETPAR,1200,0x80000000,2*46\r\n"))

    // startup complete
    PMIC.CTRL = 0x07u; // enable all interrupts
    sei();

    uint16_t dat_p,plen;
    uint8_t rval;

    // init ethernet controller
    initMacAddress();
    enc28j60Init(macAddr);

    /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
    // LEDB=green LEDA=yellow
    //
    // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
    // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
    enc28j60PhyWrite(PHLCON,0x476);

    LEDON;
    // DHCP handling. Get the initial IP
    rval=0;
    init_mac(macAddr);
    while(rval==0){
        plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
        buf[BUFFER_SIZE]=0;
        rval=packetloop_dhcp_initial_ip_assignment(buf,plen,macAddr[0]);
    }
    // we have an IP:
    dhcp_get_my_ip(myip,netmask,gwip);
    client_ifconfig(myip,netmask);
    LEDOFF;

    // pause for a bit
    _delay_ms(100);

    LEDON;
    // we have a gateway.
    // find the mac address of the gateway (e.g your dsl router).
    get_mac_with_arp(gwip,TRANS_NUM_GWMAC,&arpresolver_result_callback);
    while(get_mac_with_arp_wait()){
        // to process the ARP reply we must call the packetloop
        plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
        packetloop_arp_icmp_tcp(buf,plen);
    }
    LEDOFF;

    // infinite loop
    for(;;) {
        plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
        buf[BUFFER_SIZE]='\0'; // http is an ascii protocol. Make sure we have a string terminator.

        // DHCP renew IP:
        plen=packetloop_dhcp_renewhandler(buf,plen); // for this to work you have to call dhcp_6sec_tick() every 6 sec
        dat_p=packetloop_arp_icmp_tcp(buf,plen);

        // dat_p will be unequal to zero if there is a valid  http get
        if(dat_p==0){
            // no http request
            if (enc28j60linkup()) {
                LEDON;
            } else {
                LEDOFF;
            }
            continue;
        }

        // tcp port 80 begin
        LEDOFF;
        if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0) {
            // head, post and other methods:
            dat_p=http200ok();
            dat_p=fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
            www_server_reply(buf,dat_p);
        }
        // just one web page in the "root directory" of the web server
        else if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0) {
            dat_p=print_webpage(buf);
            www_server_reply(buf,dat_p);
        }
        else {
            dat_p=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
            www_server_reply(buf,dat_p);
        }
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

uint32_t appendSimpleHash(uint8_t byte, uint32_t hash) {
    return ((hash) << 5u) + (hash) + byte;
}

uint8_t readProdByte(const volatile uint8_t *offset) {
    NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
    uint8_t temp = pgm_read_byte(offset);
    NVM.CMD = NVM_CMD_NO_OPERATION_gc;
    return temp;
}

void initMacAddress() {
    // create hash of serial number
    uint32_t hash = 5381;
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_LOTNUM0), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_LOTNUM1), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_LOTNUM2), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_LOTNUM3), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_LOTNUM4), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_LOTNUM5), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_WAFNUM), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_COORDX0), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_COORDX1), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_COORDY0), hash);
    hash = appendSimpleHash(readProdByte(&PRODSIGNATURES_COORDY1), hash);

    // use lower 24-bits as last 24-bits of MAC address
    macAddr[5] = (hash >> 0u) & 0xffu;
    macAddr[4] = (hash >> 8u) & 0xffu;
    macAddr[3] = (hash >> 16u) & 0xffu;
}

// the __attribute__((unused)) is a gcc compiler directive to avoid warnings about unsed variables.
void arpresolver_result_callback(uint8_t *ip __attribute__((unused)),uint8_t reference_number,uint8_t *mac){
    uint8_t i=0;
    if (reference_number==TRANS_NUM_GWMAC){
        // copy mac address over:
        while(i<6){gwmac[i]=mac[i];i++;}
    }
}

ISR(USARTC1_RXC_vect, ISR_NOBLOCK) {
    uint8_t byte = USARTC1.DATA;
    if(byte == '$') {
        rx_cnt = 0;
    }
    else if(rx_cnt >= 82) {
        return;
    }

    // save byte
    gpsMsgs[0][rx_cnt++] = byte;

    // end-of-message
    if(byte == '\n') {
        gpsMsgs[0][rx_cnt] = 0;

        if(strncmp(gpsMsgs[0], "$GPGGA,", 7) == 0) {
            strcpy(gpsMsgs[1], gpsMsgs[0]);
        }
        else if(strncmp(gpsMsgs[0], "$GNGSA,", 7) == 0) {
            strcpy(gpsMsgs[2], gpsMsgs[0]);
        }
        else if(strncmp(gpsMsgs[0], "$GPRMC,", 7) == 0) {
            strcpy(gpsMsgs[3], gpsMsgs[0]);
            dateTimeReady = 1u;
        }
    }
}

void sendGpsConfig(const char *pstr) {
    for(;;) {
        char byte = pgm_read_byte(pstr);
        if(byte == 0) break;
        while(!(USARTC1.STATUS & 0x20u)) nop();
        USARTC1.DATA = byte;
    }
}
