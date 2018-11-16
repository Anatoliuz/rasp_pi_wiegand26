#!/bin/bash

sudo mkdir /home/pi/bin
sudo systemctl disable wiegand.service
gcc -Wall -pthread -ggdb3 -lpigpiod_if2 -lyuarel source.c mongoose.* encoding_* -o ITPRFIDWiegandController
sudo yes | cp -rf ITPRFIDWiegandController /home/pi/bin
sudo systemctl enable pigpiod.service
sudo sudo cp wiegand.service /etc/systemd/system/wiegand.service
sudo systemctl enable wiegand.service
sudo systemctl start wiegand.service
