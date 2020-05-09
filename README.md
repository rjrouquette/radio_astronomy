# Radio Astronomy
This project is an attempt at an SDR DAS phased-array radio astronomy system.  The intent is to produce a flexible and scalable system that can be easily repurposed with minimal additional cost.

## Digitizer
The digitizer board employs a direct IF to digital conversion toplogy to reduce the 50 MHz IF input into I+Q baseband data.

![alt text][digitzer]


## Baseband Data
The SDR DAS modules will produce baseband I+Q samples at 25 MSps with 10 MHz for I, 10 MHz for Q, and a combined bandwidth of 20MHz.  The extra 2.5 MHz of Nyquist margin includes aliasing artifacts and may be utilized if degraded SNR is acceptable.  The 25 MSps I+Q data is formatted as two's complement 16-bit integers for both I and Q which yields a raw data bandwidth of 800 Mbps.  Gigabit ethernet is used to offload the sample data.

## Network Data Transport
Baseband sample data from the SDR DAS modules will transported over ethernet via UDP.  To ensure reliable transmission in the face of packet loss, the UDP packets will be grouped into frames of 57 packets and employ a Hamming(57,50) erasure coding scheme.  This will allow for full frame recovery with, at most, 3 dropped packets per frame.  Each packet will contain an 8 byte header and 250 samples.  This yields a packet payload of 1008 bytes.  The header structure is shown below.

```
------------------------------------------------------------------------
| 4 bytes                                      | 4 bytes               |
|----------------------------------------------|-----------------------|
| 15-bit epoch | 11-bit frame | 6-bit subframe | digitizer status bits |
| monotonic    | mod 2000     | mod 56         | PPS, PLL, OVR, etc.   |
------------------------------------------------------------------------
```

The total protocol overhead (EC + UDP + IP + Ethernet) comes to 22.5% (979.5 Mbps)

## Master Clock
A master clock system will be employed to minimize timming drift between the SDR DAS modules.  The master clock will transmit a BPSK encoded PPS on a 25 Mhz carrier via coax.  The rising edge of the PPS signal will be used to synchronize sample framing and PLL counters to minimuze phase errors.  The master clock source will be GPS disciplined to reduce frequency drift.


[digitzer]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/digitizer_block_diagram.png "Block Diagram"
