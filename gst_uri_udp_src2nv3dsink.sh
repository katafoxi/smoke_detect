#!/bin/bash
gst-launch-1.0 \
uridecodebin \
    uri="udp://127.0.0.1:5000" \
    caps= "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)RAW, sampling=(string)YCbCr-4:2:0, depth=(string)8, width=(string)1280, height=(string)720" \
    name=dec !\
rtph264depay !\
decodebin !\
videoconvert !\
m.sink_0 nvstreammux \
	name=m \
	batch-size=1 \
	width=1280  \
	height=720 !\
nvinfer \
	name=nvinfer \
	unique-id=1  \
	infer-on-gie-id=1 \
	infer-on-class-ids="0:"  \
	batch-size=1  \
	config-file-path="./configs/seg_ac_infer.txt" !\
nvsegvisual \
    batch-size=1 \
    width=512 \
    height=512 \
    alpha=0.7 \
    original-background=1 \
    class-id=0 \
    gpu-on=1 \
    qos=0 \
    operate-on-seg-meta-id=1 !\
nvdsosd \
    display-clock=1 \
    display-mask=1 \
    clock-font="Serif" \
    clock-font-size=12 \
    process-mode=1 \
    qos=1 !\
nvvidconv ! "video/x-raw,format=NV12" !\
nv3dsink async=0 \
