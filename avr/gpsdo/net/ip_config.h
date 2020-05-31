//
// Created by robert on 5/30/20.
//

#ifndef GPSDO_IP_CONFIG_H
#define GPSDO_IP_CONFIG_H

//------------- functions in ip_arp_udp_tcp.c --------------
// an NTP client (ntp clock):
#undef NTP_client
// a spontanious sending UDP client (needed as well for DNS and DHCP)
#define UDP_client
// a server answering to UDP messages
#undef UDP_server

// define this if you want to use enc28j60EnableBroadcast/enc28j60DisableBroadcast
// the dhcp_client.c needs this.
#define ENC28J60_BROADCAST

// a web server
#define WWW_server

// to send out a ping:
#undef PING_client
#define PINGPATTERN 0x42

// a UDP wake on lan sender:
//#undef WOL_client

// function to send a gratuitous arp
#undef GRATARP

// a "web browser". This can be use to upload data
// to a web server on the internet by encoding the data
// into the url (like a Form action of type GET):
#undef WWW_client
// if you do not need a browser and just a server:
//#undef WWW_client
//
//------------- functions in websrv_help_functions.c --------------
//
// functions to decode cgi-form data:
#undef FROMDECODE_websrv_help

// function to encode a URL (mostly needed for a web client)
#undef URLENCODE_websrv_help

#endif //GPSDO_IP_CONFIG_H
