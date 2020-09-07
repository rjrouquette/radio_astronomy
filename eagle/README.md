# EAGLE Board Layout Files
The board layouts for the various sub modules are loacted here.

## Antenna
The antenna boards are dual-polarized patch antennas with diversity outputs.  The patch antennas are matched to 50 Ohms using a microstrip line and inductor.

![alt text][antenna]

## Master Clock (GPSDO)
The master clock board is based around the STMicro TESEO-LIV3R GPS module and a SiTime SIT3807 VCXO.  The board injects 3.3 V into the 50 Ohm GPS SMA connector to support active antennas.  The reference clock BPSK output signals are available through four 50 Ohm SMA connectors.  Power is supllied to the board either through a micro USB connector or wire holes.  An ATXMega16A4U and discrete logic form the basis of the GPSDO PLL and BPSK transmitter.  The XMega DAC output passes through a 6.3 second RC low-pass filter before entring the control pin on the VCXO.

![alt text][clk_master]

## Slave Clock (GPSDO)
The slave clock board is based around the SiTime SIT3808 VCXO and provides both clock and PPS recovery.

![alt text][clk_slave]


[antenna]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/antenna_layout.png "Antenna Layout"
[clk_master]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/master_clock_layout.png "Master Clock Layout"
[clk_slave]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/slave_clock_layout.png "Slave Clock Layout"
