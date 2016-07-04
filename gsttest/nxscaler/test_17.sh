#!/bin/bash

set -e

echo "==> Test Start : [No Scaling] input = crop = display size : 1280*720, buffer-type : 0 with videosink"
timeout -t 5 gst-launch-1.0 -eq camerasrc camera-id=0 camera-crop-x=0 camera-crop-y=0 camera-crop-width=1280 camera-crop-height=720 buffer-type=1 format=I420 ! nxscaler scaler-crop-x=0 scaler-crop-y=0 scaler-crop-width=1280 scaler-crop-height=720 scaler-dst-width=1280 scaler-dst-height=720 buffer-type=0 ! nxvideosink 
