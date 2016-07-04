#!/bin/bash

set -e

echo "==> Test Start : [Format] Unsupported format : YUY2 "
timeout -t 5 gst-launch-1.0 -eq camerasrc camera-id=0 camera-crop-x=0 camera-crop-y=0 camera-crop-width=640 camera-crop-height=480 buffer-type=1 format=YUY2 ! nxscaler scaler-crop-x=0 scaler-crop-y=0 scaler-crop-width=640 scaler-crop-height=480 scaler-dst-width=640 scaler-dst-height=480 buffer-type=1 ! nxrenderer
