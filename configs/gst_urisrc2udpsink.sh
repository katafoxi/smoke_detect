#!/bin/bash
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
    class-id=0 \
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
nvvidconv ! video/x-raw,format=NV12 ! \
x264enc  speed-preset=superfast  ! \
rtph264pay  ! \
udpsink  \
    host=127.0.0.1 \
    port=5000 \
    auto-multicast=true \
    ttl-mc=3
    # sync=false \
    # auto-multicast=0



#######
<<PLAY_COMMANDS
udpsin + sdp_file 
https://gist.github.com/esrever10/7d39fe2d4163c5b2d7006495c3c911bb?permalink_comment_id=3301904#gistcomment-3301904

# test playing
gst-launch-1.0 \
udpsrc port=5000 \
! application/x-rtp,media=video,encoding-name=H264 \
! rtph264depay \
! avdec_h264 \
! autovideosink

gst-launch-1.0 \
udpsrc port=5000 \
! application/x-rtp,media=video,encoding-name=H264 \
! rtpmp2tpay \
! avdec_h264 \
! autovideosink

gst-launch-1.0 \
-v \
udpsrc multicast-group=224.100.10.10 port=3001 \
! application/x-rtp,media=video,encoding-name=MP2T,clock-rate=90000, payload=33 \
! rtpjitterbuffer latency=300 
! rtpmp2tdepay \
! decodebin \
! autovideosink

PLAY_COMMANDS


    

# gst-launch-1.0 v4l2src device=/dev/video0 ! image/jpeg,format=MJPG,width=1280,height=720,framerate=30/1 ! nvv4l2decoder mjpeg=1 ! nvvidconv ! video/x-raw, format=NV12 ! x264enc speed-preset=superfast ! rtph264pay ! udpsink port=5000 host=127.0.0.1


# https://forums.developer.nvidia.com/t/vlc-cant-play-video-stream-from-orin-agx/230761/7
# out_pipeline_str << "appsrc ! video/x-raw, format=(string)BGR ! "
# "videoconvert ! video/x-raw, format=(string)BGRx ! 
# nvvidconv ! "
# "video/x-raw(memory:NVMM), format=(string)NV12 ! 
# nvv4l2h264enc insert-sps-pps=true insert-vui=true idrinterval=15 ! "
# "h264parse ! 
# mpegtsmux ! 
# rtpmp2tpay !
# udpsink host=HOST_IP port=8001 auto-multicast=0";