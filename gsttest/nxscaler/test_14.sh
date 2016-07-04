#!/bin/bash

set -e

echo "==> Test Start : [No Scaling] input = crop = display size : 640*480, buffer-type : 1 with videosink"
timeout -t 5 gst-launch-1.0 -eq camerasrc camera-id=0 camera-crop-x=0 camera-crop-y=0 camera-crop-width=640 camera-crop-height=480 buffer-type=1 format=I420 ! nxscaler scaler-crop-x=0 scaler-crop-y=0 scaler-crop-width=640 scaler-crop-height=480 scaler-dst-width=640 scaler-dst-height=480 buffer-type=1 ! nxvideosink 
