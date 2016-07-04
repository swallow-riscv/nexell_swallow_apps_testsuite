#!/bin/bash

set -e

echo "==> Test Start : [Up Scaling] input = crop size: 320*240,display size: 1280*720, buffer-type : 0 with videoenc"
timeout -t 5 gst-launch-1.0 -eq camerasrc camera-id=0 camera-crop-x=0 camera-crop-y=0 camera-crop-width=320 camera-crop-height=240 buffer-type=1 format=I420 ! nxscaler scaler-crop-x=0 scaler-crop-y=0 scaler-crop-width=320 scaler-crop-height=240 scaler-dst-width=1280 scaler-dst-height=720 buffer-type=0 ! nxvideoenc ! avimux name=mux ! filesink location=test44.avi 
