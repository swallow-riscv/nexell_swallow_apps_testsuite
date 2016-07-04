#!/bin/bash

set -e

echo "==> Test Start : [Down Scaling] input = crop size: 1920*1080,display size: 320*240, buffer-type : 0 with videoenc"
timeout -t 5 gst-launch-1.0 -eq camerasrc camera-id=0 camera-crop-x=0 camera-crop-y=0 camera-crop-width=1920 camera-crop-height=1080 buffer-type=1 format=I420 ! nxscaler scaler-crop-x=0 scaler-crop-y=0 scaler-crop-width=1920 scaler-crop-height=1080 scaler-dst-width=320 scaler-dst-height=240 buffer-type=0 ! nxvideoenc ! avimux name=mux ! filesink location=test76.avi 
