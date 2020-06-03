# EAGLE Board Layout Files
The board layouts for the various sub modules are loacted here.

## Antenna
The antenna board consists of a log-periodic antenna coupled with an LNA and bandpass filters.  The LNA is powered via DC injection.

The log-periodic antenna was designed using the calculators from [changpuak.ch](https://www.changpuak.ch/electronics/lpda.php) and [hamwaves.com](https://hamwaves.com/lpda/en/index.html) by adjusting the frequency used in the calculators to account for the FR4 dielectric constant.  The dielectric constant used was 4.5 per the [OSH Park](https://www.oshpark.com/) website which yields a scale factor of 2.12.  The feed end of the anntenna is left open and the load end is coupled to the LNA with a quarter wave transimisson line to transform the differential impedance into a single-ended impedance.  The taper of 0.910, relative spacing of 0.170, and characteristic impedance of 75 Ohms yield a bandwidth of 1.9 % and a directionality of 8.41 dBi.

The LNA utilizes the BFU730F transistor at its core and provides a 75 Ohm input impedance and a 50 Ohm output impedance.  A bandpass filter is applied at the output of the LNA.  Power for the LNA is extracted from the 50 Ohm SMA output.

![alt text][antenna]

## Master Clock (GPSDO)
The master clock board is based around the STMicro TESEO-LIV3R GPS module and a SiTime SIT3807 VCXO.  The board injects 3.3 V into the 50 Ohm GPS SMA connector to support active antennas.  The reference clock BPSK output signals are provided through four 50 Ohm SMA connectors.  Power is supllied to the board via a micro USB connector.  Wire holes are also provided as a power option.  A combination of an ATXMega16A4U and discrete logic form the basis of the GPSOD PLL and BPSK tranmitter.

![alt text][clk_master]


[antenna]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/antenna_layout.png "Antenna Layout"
[clk_master]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/master_clock_layout.png "Master Clock Layout"
