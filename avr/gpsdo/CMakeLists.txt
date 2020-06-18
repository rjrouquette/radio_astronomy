cmake_minimum_required(VERSION 3.0)

set(CMAKE_FIND_ROOT_PATH /usr/lib/avr/include)

# Use avr-gcc and avr-g++
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)

# don't test as they won't work
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project("gpsdo")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -Wall -mmcu=atxmega16a4u -D__AVR_ATxmega16A4U__ -DF_CPU=50000000 -DARCH_XMEGA")

add_executable(
        gpsdo

        main.c
        gpsdo.c
        gpsdo.h
        leds.h
        nop.h

        net/dhcp_client.c
        net/dhcp_client.h
        net/dnslkup.c
        net/dnslkup.h
        net/enc28j60.c
        net/enc28j60.h
        net/ip_arp_udp_tcp.c
        net/ip_arp_udp_tcp.h
        net/net.h
        net/ip_config.h
)
