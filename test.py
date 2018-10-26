#!/usr/bin/python3

import requests as req


for x in range(1000000):
	resp = req.request(method='GET', url="http://192.168.15.116:8000/wiegand26?data0=5&data1=6&code=FFFFFF")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.116:8000/")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.116:8000/wieg")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.116:8000/wiegand26?data0=5&data1=6&code=FFFFF")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.116:8000/wiegand26?data0=5&data1=6&code=QFFFFF")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.116:8000/wiegand26?data0=-1&data1=6&code=FFFFFF")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.116:8000/wiegand26?data2=5&data1=6&code=FFFFFF")
	print(resp.content)

