# Radio Astronomy
This project is an attempt at an SDR DAS phased-array radio astronomy system.  The intent is to produce a flexible and scalable system that can be easily repurposed with minimal additional cost.

## Digitizer
The digitizer board employs a direct IF to digital conversion toplogy to reduce the 62.5 MHz IF input into I+Q baseband data.

![alt text][digitzer]


## Baseband Data
The SDR DAS modules will produce baseband I+Q samples at 25 MSps with 10 MHz for I, 10 MHz for Q, and a combined bandwidth of 20MHz.  The extra 2.5 MHz of Nyquist margin includes aliasing artifacts and may be utilized if degraded SNR is acceptable.  The 25 MSps I+Q data is formatted as two's complement 16-bit integers for both I and Q which yields a raw data bandwidth of 800 Mbps.  An FT601 USB 3.0 FIFO is used to offload the data.

## Master Clock
A master clock system will be employed to minimize timming drift between the SDR DAS modules.  The master clock provides a BPSK encoded PPS signal on a 25 MHz carrier via coax.  The rising edge of the PPS signal will be used to synchronize sample framing and PLL counters to minimuze phase errors and relative drift.  The master clock source is GPS disciplined to reduce frequency drift and absolute offset errors.  A plot of the GPSDO PLL performance is provided below.  The GPS antenna is currently located indoors.  The PLL Error and RMS Error values are the mean and RMS computed over a trailing 64s window by the on-board microntroller.  The two "Mean" dials on the right show the average values over the full 12 hour span.

![alt text][gpsdo]


[digitzer]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/digitizer_block_diagram.png "Block Diagram"
[gpsdo]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/gpsdo_grafana.png "GPSDO Performance"
