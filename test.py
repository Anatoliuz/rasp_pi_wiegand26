#!/usr/bin/python3

import requests as req


for x in range(1000000):
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=112233")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=6&data1=5&code=222222")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=8&data1=7&code=FFFFFF")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=7&data1=8&code=222222")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=000000")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=7&data1=8&code=222222")
	print(resp.content)




