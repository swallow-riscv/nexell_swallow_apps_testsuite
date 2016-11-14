#!/bin/sh

echo 165 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio165/direction
echo 0 > /sys/class/gpio/gpio165/value
echo 165 > /sys/class/gpio/unexport

PID_IPOD=`pidof ipod_service`
kill $PID_IPOD

echo "" > /sys/kernel/config/usb_gadget/g1/UDC
rm /sys/kernel/config/usb_gadget/g1/configs/c.1/carplay.usb0
rmdir /sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409
rmdir /sys/kernel/config/usb_gadget/g1/configs/c.1
rmdir /sys/kernel/config/usb_gadget/g1/functions/carplay.usb0
rmdir /sys/kernel/config/usb_gadget/g1/strings/0x409

echo 52 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio52/direction
echo 0 > /sys/class/gpio/gpio52/value
echo 52 > /sys/class/gpio/unexport

echo 165 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio165/direction
echo 1 > /sys/class/gpio/gpio165/value
echo 165 > /sys/class/gpio/unexport
