#!/bin/bash
gst-launch-1.0 \
udpsrc port=5000 \
    caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" !\
rtph264depay !\
decodebin !\
videoconvert !\
streammux.sink_0 nvstreammux \
	name=streammux \
	batch-size=1 \
	width=1280  \
	height=720 \
    !\
nvinfer \
	name=nvinfer \
	unique-id=1  \
	infer-on-gie-id=1 \
	infer-on-class-ids="0:" \
	batch-size=1  \
	config-file-path="./configs/seg_ac_infer.txt" \
    !\
nvsegvisual \
    batch-size=1 \
    width=512 \
    height=512 \
    alpha=0.7 \
    original-background=1 \
    class-id=0 \
    gpu-on=1 \
    qos=0 \
    operate-on-seg-meta-id=1 \
    !\
nvdsosd \
    display-clock=1 \
    display-mask=1 \
    clock-font="Serif" \
    clock-font-size=12 \
    process-mode=1 \
    qos=1 \
    !\
nvvidconv ! "video/x-raw,format=NV12" !\
nv3dsink async=0 \

<<EXAMPLE
# https://gist.github.com/esrever10/7d39fe2d4163c5b2d7006495c3c911bb
# receive h264 rtp stream:
gst-launch-1.0 -v \
udpsrc port=5000 \
caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" !\
rtph264depay !\
decodebin !\
videoconvert !\
autovideosink


udpsrc port=5000 !\
    application/x-rtp,media=video,encoding-name=H264,payload=96 !\
rtph264depay !\
h264parse !\
avdec_h264 !\
EXAMPLE