#!/usr/bin/python3
# GPSDO InfluxDB Scraper

import requests
import time
import json

gpsdoHost = '192.168.3.155'
gpsdoUrl = 'http://'+gpsdoHost+'/'
influxUrl = 'http://192.168.3.200:8086/write?db=radio_astronomy'

while True:
    try:
        r = requests.get(gpsdoUrl, timeout=1)
        lines = r.text.split('\n')
        temp = float(lines[0].split(': ')[1].strip().split()[0])
        locked = lines[1].split(': ')[1].strip() == 'yes'
        error = float(lines[2].split(': ')[1].strip().split()[0])
        rmse = float(lines[3].split(': ')[1].strip().split()[0])
        ppm = float(lines[4].split(': ')[1].strip().split()[0])
        body = 'gpsdo,host=%s temperature=%.1f,locked=%d,pll_error=%.1f,pll_rmse=%.1f,pll_ppm=%1.3f %d\n' % (gpsdoHost, temp, locked, error, rmse, ppm, int(time.time() * 1000000000))

        gpsFix = lines[6].strip().split(',')
        fixType = int(gpsFix[6])
        fixSats = int(gpsFix[7])
        fixDop = float(gpsFix[8])

        body += 'gpsdo,host=%s gps_fix=%d,gps_sats=%d,gps_dop=%.2f %d\n' % (gpsdoHost, fixType, fixSats, fixDop, int(time.time() * 1000000000))
        requests.post(influxUrl, data=body, timeout=1)
    except:
        print('failed to poll or record stats')

    # 10 second update interval
    time.sleep(10)
