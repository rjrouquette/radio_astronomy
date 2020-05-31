//
// Created by robert on 5/31/20.
//

#include <avr/io.h>

#include "leds.h"
#include "webserver.h"
#include "net/enc28j60.h"

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

void initWebserver() {
    initMacAddress();

    // init ethernet controller
    enc28j60Init(macAddr);

}

void updateWebserver() {

}
