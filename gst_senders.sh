#!/bin/sh
#SENDERS

HOST=127.0.0.1
PORT=5000
SRTPORT=5005
###############################################################################
###############################################################################
###############################################################################
#OK
#id_UDP_SRT

# --gst-debug=*:1 --gst-debug=GST_CAPS:4 \

# gst-launch-1.0  \
# filesrc location=OutputVideo1920p_yuv420p.srt !\
# subparse !\
# rtpgstpay pt=98  !\
# queue !\
# udpsink  host=${HOST}  port=${SRTPORT}

###############################################################################
###############################################################################
###############################################################################
#OK
#SRT_fakesink

# --gst-debug=*:1 --gst-debug=GST_CAPS:4 \

# gst-launch-1.0  \
# filesrc do-timestamp=true location=OutputVideo1920p_yuv420p.srt !\
# subparse !\
# rtpgstpay pt=98  !\
# queue !\
# fakesink silent=false -v


###############################################################################
###############################################################################
###############################################################################
#OK
#id_2UDP

# --gst-debug=*:1 --gst-debug=GST_CAPS:4 \

# gst-launch-1.0  -e \
# filesrc location=OutputVideo1920p_yuv420p.mp4 !\
# decodebin !\
# nvvidconv !\
# x264enc !\
# queue !\
# rtph264pay pt=96 !\
# udpsink host=${HOST}  port=${PORT} \
# \
# filesrc do-timestamp=true location=OutputVideo1920p_yuv420p.srt !\
# subparse !\
# queue !\
# rtpgstpay pt=98 !\
# udpsink host=${HOST} port=${SRTPORT} qos=true

###############################################################################
###############################################################################
###############################################################################
#NO
#id_2UDP

# --gst-debug=*:1 --gst-debug=GST_CAPS:4 \

gst-launch-1.0 -e \
filesrc location=OutputVideo1920p_yuv420p.mp4 !\
decodebin !\
nvvidconv !\
x264enc !\
queue !\
rtph264pay pt=96 !\
udpsink host=${HOST}  port=${PORT} \
\
filesrc do-timestamp=true location=OutputVideo1920p_yuv420p.srt !\
subparse !\
queue !\
rtpgstpay pt=98 !\
udpsink host=${HOST} port=${SRTPORT} qos=true


###############################################################################
###############################################################################
###############################################################################

# --gst-debug=*:1 --gst-debug=GST_CAPS:4 \

# gst-launch-1.0  -e \
# mqueue.src_0 ! queue2 !\
# rtpmux name=mux !\
# udpsink  host=${HOST}  port=${PORT} \
# \
# mqueue.src_1 ! queue2 ! mux. \
# \
# filesrc location=OutputVideo1920p_yuv420p.mp4 !\
#     decodebin !\
#     nvvidconv !\
#     x264enc !\
#     queue2 !\
#     rtph264pay pt=96 !\
#     multiqueue name=mqueue \
# \
# filesrc location=OutputVideo1920p_yuv420p.srt !\
#     subparse !\
#     queue2 !\
#     rtpgstpay pt=98 !\
#     mqueue. 

# qtdemux name=video_demux video_demux.video_0  \
#  application/x-rtp,payload=96,rate=90000

###############################################################################
###############################################################################
###############################################################################
#TEST 


# gst-launch-1.0 -e \
# rtpbin name=rtpbin \
# \
# filesrc location=OutputVideo1920p_yuv420p.mp4 !\
# decodebin !\
# nvvidconv !\
# x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream !\
# rtph264pay pt=96 !\
# rtpbin.send_rtp_sink_0 \
# rtpbin.send_rtp_src_0 ! udpsink host=${HOST} port=${PORT} \
# rtpbin.send_rtcp_src_0 ! udpsink host=${HOST} port=$((${PORT} + 1)) sync=false async=false \
# \
# filesrc do-timestamp=true location=OutputVideo1920p_yuv420p.srt !\
# subparse !\
# rtpgstpay pt=98 !\
# rtpbin.send_rtp_sink_1 \
# rtpbin.send_rtp_src_1 ! udpsink host=${HOST} port=${SRTPORT} \
# rtpbin.send_rtcp_src_1 ! udpsink host=${HOST} port=$((${SRTPORT} + 1)) sync=false async=false