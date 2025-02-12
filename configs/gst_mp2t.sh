#!/bin/bash

# SDP file is required for dynamic payload. You may use RTP/MP2T that uses a 
# static payload (33) and can be used with H264 or H265.
# https://forums.developer.nvidia.com/t/vlc-cant-play-video-stream-from-orin-agx/230761/7
# Open source CPU based:
gst-launch-1.0  \
uridecodebin \
    uri=file:///ssd/wdir/smoke_detect/streams/OutputVideo1920p_yuv420p.mp4 \
    name=dec \
    ! \
m.sink_0 nvstreammux \
	name=m \
	batch-size=1 \
	width=1280  \
	height=720 \
    ! \
nvinfer \
	name=nvinfer \
	unique-id=1  \
	infer-on-gie-id=1 \
	infer-on-class-ids="0:"  \
	batch-size=1  \
	config-file-path="/ssd/wdir/smoke_detect/configs/seg_ac_infer.txt" \
    ! \
nvsegvisual \
    batch-size=1 \
    width=512 \
    height=512 \
    alpha=0.7 \
    original-background=1 \
    class-id=1 \
    gpu-on=1 \
    qos=0 \
    operate-on-seg-meta-id=1 \
    ! \
nvdsosd \
    display-clock=1 \
    display-mask=1 \
    clock-font="Serif" \
    clock-font-size=12 \
    process-mode=1 \
    qos=1 \
    ! \
nvvidconv ! 'video/x-raw, format=NV12' ! \
x264enc \
    speed-preset=superfast  \
    key-int-max=30 \
    insert-vui=1 \
    tune=zerolatency \
    ! \
h264parse ! \
mpegtsmux ! \
rtpmp2tpay ! \
udpsink  \
    host=127.0.0.1 \
    port=5000 
    # \
    # auto-multicast=0 \
    # ttl-mc=3
    # sync=false \
    # auto-multicast=0

<< NVIDIA_encoder
#SENDER
gst-launch-1.0 videotestsrc ! \
video/x-raw,width=320,height=240,framerate=30/1 ! \
nvvidconv ! 'video/x-raw(memory:NVMM),format=NV12' ! \
nvv4l2h264enc insert-sps-pps=1 insert-vui=1 !\
h264parse ! \
mpegtsmux ! \
rtpmp2tpay ! \
udpsink port=5004
NVIDIA_encoder

<< RECEIVER
## On Jetson, using HW decoder:
gst-launch-1.0 -v \
udpsrc port=5004 \
! application/x-rtp,media=video,encoding-name=MP2T,clock-rate=90000, payload=33 \
! rtpjitterbuffer latency=300 \
! rtpmp2tdepay \
! tsdemux \
! h264parse \
! nvv4l2decoder \
! nvvidconv \
! xvimagesink

# gstreamer using open-source CPU based decoder:
gst-launch-1.0 -v \
udpsrc port=5004 \
! application/x-rtp,media=video,encoding-name=MP2T,clock-rate=90000, payload=33 \
! rtpjitterbuffer latency=300 \
! rtpmp2tdepay \
! tsdemux \
! h264parse \
! avdec_h264 \
! videoconvert \
! xvimagesink
RECEIVER