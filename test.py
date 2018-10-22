#!/usr/bin/python3

import requests as req


for x in range(100):
	resp = req.request(method='GET', url="http://192.168.15.116:8000/wiegand26?data0=5&data1=6&code=FFFFFF")
	print(resp.content)
