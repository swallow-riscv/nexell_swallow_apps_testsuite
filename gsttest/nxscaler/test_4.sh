#!/bin/bash

set -e

echo "==> Test Start : [Scaling Size Error] crop size > input size : input = 320*240, crop = 320*240(x=10,y=10) with Renderer"
timeout -t 5 gst-launch-1.0 -eq camerasrc camera-id=0 camera-crop-x=0 camera-crop-y=0 camera-crop-width=320 camera-crop-height=240 buffer-type=1 format=I420 ! nxscaler scaler-crop-x=10 scaler-crop-y=10 scaler-crop-width=320 scaler-crop-height=240 scaler-dst-width=320 scaler-dst-height=240 buffer-type=1 ! nxrenderer 
