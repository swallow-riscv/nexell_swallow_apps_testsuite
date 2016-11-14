#!/bin/sh

GADGET_DIR=/sys/kernel/config/usb_gadget/g1

# Set otg host and power up
echo 165 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio165/direction
echo 1 > /sys/class/gpio/gpio165/value
echo 165 > /sys/class/gpio/unexport

echo 52 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio52/direction
echo 1 > /sys/class/gpio/gpio52/value
echo 52 > /sys/class/gpio/unexport

#Mount ConfigFS and create Gadget
if ! [ -d "${GADGET_DIR}" ]; then
        mkdir ${GADGET_DIR}
fi

echo 0x18d1 > /sys/kernel/config/usb_gadget/g1/idVendor
echo 0x0002 > /sys/kernel/config/usb_gadget/g1/idProduct
mkdir /sys/kernel/config/usb_gadget/g1/strings/0x409
mkdir /sys/kernel/config/usb_gadget/g1/configs/c.1
mkdir /sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409
echo 0xc0 > /sys/kernel/config/usb_gadget/g1/configs/c.1/bmAttributes
echo 0 > /sys/kernel/config/usb_gadget/g1/configs/c.1/MaxPower
echo "Conf 1" > /sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409/configuration
echo "Apple" > /sys/kernel/config/usb_gadget/g1/strings/0x409/manufacturer
echo "Carplay" > /sys/kernel/config/usb_gadget/g1/strings/0x409/product
echo "0123456789ABCDEF" > /sys/kernel/config/usb_gadget/g1/strings/0x409/serialnumber
echo "iAP Interface" > /sys/kernel/config/usb_gadget/g1/strings/0x409/interface
echo "CDC NCM Comm Interface" > /sys/kernel/config/usb_gadget/g1/strings/0x409/interface2
echo "CDC NCM Data Interface" > /sys/kernel/config/usb_gadget/g1/strings/0x409/interface3
echo "9A3BFC66E845" > /sys/kernel/config/usb_gadget/g1/strings/0x409/macaddress


mkdir /sys/kernel/config/usb_gadget/g1/functions/carplay.usb0
ln -s /sys/kernel/config/usb_gadget/g1/functions/carplay.usb0 /sys/kernel/config/usb_gadget/g1/configs/c.1/carplay.usb0

echo c0040000.dwc2otg > /sys/kernel/config/usb_gadget/g1/UDC

#Start iap2 application
/usr/bin/ipod_service&

