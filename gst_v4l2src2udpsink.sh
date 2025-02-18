#!/bin/sh
gst-launch-1.0 \
v4l2src device=/dev/video0 !\
    image/jpeg,format=MJPG,width=1280,height=720,framerate=30/1 !\
nvv4l2decoder mjpeg=1 !\
nvvidconv ! video/x-raw, format=NV12 !\
x264enc speed-preset=superfast !\
rtph264pay !\
udpsink port=5000 host=127.0.0.1

<<PLAY_COMMANDS
gst-launch-1.0 \
udpsrc port=5000 !\
    application/x-rtp,media=video,encoding-name=H264 !\
rtph264depay !\
avdec_h264 !\
autovideosink
PLAY_COMMANDS