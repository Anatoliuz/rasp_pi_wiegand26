#!/usr/bin/python3

import requests as req


for x in range(1000000):
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=00FFAA")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=BBCC00")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=000001")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=FDFDFD")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=00FFAA")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=00FFAA")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=00FFAA")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=00FFAA")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=00FFAA")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=00FFAA")
	print(resp.content)
	resp = req.request(method='GET', url="http://192.168.15.117/wiegand26?data0=5&data1=6&code=00FFAA")
	print(resp.content)




