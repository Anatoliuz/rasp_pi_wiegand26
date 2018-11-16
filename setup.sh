#!/bin/bash

sudo systemctl enable pigpiod.service
sudo sudo cp wiegand.service /etc/systemd/system/wiegand.service
systemctl start wiegand.service
systemctl enable wiegand.service