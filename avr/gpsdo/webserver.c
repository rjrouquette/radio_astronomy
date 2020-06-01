//
// Created by robert on 5/31/20.
//

#include <avr/io.h>
#include <avr/interrupt.h>

#include "leds.h"
#include "webserver.h"
#include "net/enc28j60.h"
#include "net/ip_arp_udp_tcp.h"
#include "net/dhcp_client.h"

#define LEDON ledOn(LED1)
#define LEDOFF ledOff(LED1)

// interface MAC address
uint8_t macAddr[6] = {0x54, 0x55, 0x58, 0x10, 0x00, 0x29};
// My own IP (DHCP will provide a value for it):
static uint8_t myip[4]={0,0,0,0};
// Default gateway (DHCP will provide a value for it):
static uint8_t gwip[4]={0,0,0,0};
#define TRANS_NUM_GWMAC 1
static uint8_t gwmac[6];
// Netmask (DHCP will provide a value for it):
static uint8_t netmask[4];

// packet buffer
#define BUFFER_SIZE 650
static uint8_t buf[BUFFER_SIZE+1];
static uint8_t start_web_client=0;
static uint8_t web_client_sendok=0;
static int8_t processing_state=0;

static volatile uint8_t sec=0; // counts up to 6 and goes back to zero
static volatile uint8_t gsec=0; // counts up beyond 6 sec

void appendSimpleHash(uint8_t byte, uint32_t *hash) {
    (*hash) = ((*hash) << 5u) + (*hash) + byte;
}

void initMacAddress() {
    // create hash of serial number
    uint32_t hash = 5381;
    appendSimpleHash(PRODSIGNATURES_LOTNUM0, &hash);
    appendSimpleHash(PRODSIGNATURES_LOTNUM1, &hash);
    appendSimpleHash(PRODSIGNATURES_LOTNUM2, &hash);
    appendSimpleHash(PRODSIGNATURES_LOTNUM3, &hash);
    appendSimpleHash(PRODSIGNATURES_LOTNUM4, &hash);
    appendSimpleHash(PRODSIGNATURES_LOTNUM5, &hash);
    appendSimpleHash(PRODSIGNATURES_WAFNUM, &hash);
    appendSimpleHash(PRODSIGNATURES_COORDX0, &hash);
    appendSimpleHash(PRODSIGNATURES_COORDX1, &hash);
    appendSimpleHash(PRODSIGNATURES_COORDY0, &hash);
    appendSimpleHash(PRODSIGNATURES_COORDY1, &hash);

    // use lower 24-bits as lower 24-bits of MAC address
    uint8_t *bytes = (uint8_t *) &hash;
    macAddr[0] = bytes[0];
    macAddr[1] = bytes[1];
    macAddr[2] = bytes[2];
}

void arpresolver_result_callback(uint8_t *ip __attribute__((unused)),uint8_t reference_number,uint8_t *mac);

void initWebserver() {
    uint16_t dat_p,plen;
    char str[20];
    uint8_t rval;
    uint8_t i;

    // init ethernet controller
    initMacAddress();
    enc28j60Init(macAddr);

    /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
    // LEDB=green LEDA=yellow
    //
    // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
    // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
    enc28j60PhyWrite(PHLCON,0x476);
/*
    LEDON;
    // DHCP handling. Get the initial IP
    rval=0;
    init_mac(macAddr);
    while(rval==0){
        plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
        buf[BUFFER_SIZE]='\0';
        rval=packetloop_dhcp_initial_ip_assignment(buf,plen,macAddr[0]);
    }
    // we have an IP:
    dhcp_get_my_ip(myip,netmask,gwip);
    client_ifconfig(myip,netmask);
    LEDOFF;

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
*/
}

void updateWebserver() {

}

// the __attribute__((unused)) is a gcc compiler directive to avoid warnings about unsed variables.
void arpresolver_result_callback(uint8_t *ip __attribute__((unused)),uint8_t reference_number, uint8_t *mac) {
    uint8_t i=0;
    if (reference_number==TRANS_NUM_GWMAC){
        // copy mac address over:
        while(i<6){gwmac[i]=mac[i];i++;}
    }
}

// one-second interval
ISR(TCC0_OVF_vect, ISR_NOBLOCK) {
    sec++;
    gsec++;
    if (sec > 5){
        sec = 0;
        dhcp_6sec_tick();
    }
    LED_PORT.OUTTGL = LED1;
}
