#!/usr/bin/python3
# GPSDO InfluxDB Scraper

import requests
import time
import json

host = '192.168.3.155'

while True:
    try:
        r = requests.get('http://'+host+'/', timeout=1)
        lines = r.text.split('\n')
        locked = lines[0].split(': ')[1].strip() == 'yes'
        error = float(lines[1].split(': ')[1].strip().split()[0])
        rmse = float(lines[2].split(': ')[1].strip().split()[0])

        body = 'gpsdo,host=%s locked=%d,pll_error=%.1f,pll_rmse=%.1f %d' % (host, locked, error, rmse, int(time.time() * 1000000000))
        requests.post('http://192.168.3.200:8086/write?db=radio_astronomy', data=body, timeout=1)
    except:
        print('failed to poll or record stats');

    # 10 second update interval
    time.sleep(10)
