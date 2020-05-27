# EAGLE Board Layout Files
The board layouts for the various sub modules are loacted here.

## Antenna
The antenna board consists of a log-periodic antenna coupled with an LNA and bandpass filters.  The LNA is powered via DC injection.

The log-periodic antenna was designed using the calculators from [changpuak.ch](https://www.changpuak.ch/electronics/lpda.php) and [hamwaves.com](https://hamwaves.com/lpda/en/index.html) by adjusting the frequency used in the calculators to account for the FR4 dielectric constant.  The dielectric constant used was 4.5 per the [OSH Park](https://www.oshpark.com/) website.  The feed end of the anntenna is left open and the load end is coupled to the LNA with a quarter wave transimisson line to transform the differential impedance into a single-ended impedance.  The taper of 0.910, relative spacing of 0.170, and characteristic impedance of 75 Ohms yield a bandwidth of 1.9 % and a directionality of 8.41 dBi.

The LNA utilizes the BFU790F transistor at its core and provides a 75 Ohm input impedance and a 50 Ohm output impedance.  Bandpass filters are applied at both the input and output of the LNA.  Power for the LNA is extractd from the 50 Ohm SMA output.

![alt text][antenna]

[antenna]: https://github.com/rjrouquette/radio_astronomy/raw/master/images/antenna_layout.png "Antenna Layout"
