#!/bin/sh

HOST=127.0.0.1
PORT=5000

# --gst-debug=*:1 --gst-debug=GST_CAPS:4 \
gst-launch-1.0  -e \
mqueue.src_0 ! queue2 !\
rtpmux name=mux !\
udpsink  host=${HOST}  port=${PORT} \
\
mqueue.src_1 ! queue2 ! mux. \
\
filesrc location=OutputVideo1920p_yuv420p.mp4 !\
    decodebin !\
    nvvidconv !\
    x264enc !\
    queue2 !\
    rtph264pay pt=96 !\
    multiqueue name=mqueue \
\
filesrc location=OutputVideo1920p_yuv420p.srt !\
    subparse !\
    queue2 !\
    rtpgstpay pt=98 !\
    mqueue. 


# qtdemux name=video_demux video_demux.video_0  \
#  application/x-rtp,payload=96,rate=90000