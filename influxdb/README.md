# InfluxDB Data Collectors
Collection of scripts and systemd wrappers to collect record statistics to influxdb.

## GPSDO
Requests and records data from the micro webserver on the master clock board.  The python script requires python3.

The files should be installed as follows:
* gpsdo.py -> /usr/local/bin/gpsdo.py
* gpsdo.service -> /etc/systemd/system/ (can also go to one of the alternate systemd directories)
