#!/bin/sh
#SENDERS

HOST=127.0.0.1
PORT=5000
SRTPORT=5005

FILENAME="OutputVideo1920p_yuv420p"
VID_FILE="./streams/${FILENAME}.mp4"
SRT_FILE="./streams/${FILENAME}.srt"

VID_RTP_=5010
VID_RTCP=5011
VID_RTCP_LISTEN=5015

AUD_RTP_=5020
AUD_RTCP=5021
AUD_RTCP_LISTEN=5025

SRT_RTP_=5030
SRT_RTCP=5031
SRT_RTCP_LISTEN=5035

###############################################################################

VID_RTP__CAPS="\
    application/x-rtp, \
    media=(string)video, \
    encoding-name=(string)H264, \
    clock-rate=(int)90000, \
    payload=(int)96"
VID_RTCP_CAPS="application/x-rtcp"

AUD_RTP__CAPS="\
    application/x-rtp,\
    media=(string)audio,\
    clock-rate=(int)8000,\
    encoding-name=(string)AMR,\
    encoding-params=(string)1,\
    octet-align=(string)1"
AUD_RTCP_CAPS="application/x-rtcp"

SRT_RTP__CAPS="\
    application/x-rtp, \
    encoding-name=(string)X-GST, \
    media=(string)application, \
    clock-rate=(int)100"

# SRT_RTP__CAPS="\
#     application/x-rtp, \
#     encoding-name=(string)X-GST, \
#     media=(string)application, \
#     clock-rate=(int)1000"

###############################################################################
###############################################################################
###############################################################################
# ██████╗ ██╗   ██╗██████╗ ██████╗ 
# ╚════██╗██║   ██║██╔══██╗██╔══██╗
#  █████╔╝██║   ██║██║  ██║██████╔╝
# ██╔═══╝ ██║   ██║██║  ██║██╔═══╝ 
# ███████╗╚██████╔╝██████╔╝██║     
# ╚══════╝ ╚═════╝ ╚═════╝ ╚═╝     senders


###############################################################################
###############################################################################
###############################################################################
#OK
#id_UDP_SRT

# gst-launch-1.0 -e -v \
# filesrc location="${SRT_FILE}" ! subparse ! rtpgstpay pt=98  ! queue !\
# udpsink  host=${HOST}  port=${SRTPORT} qos=true 

#  sync=true  async=false

###############################################################################
###############################################################################
###############################################################################
#OK
#SRT_fakesink

# gst-launch-1.0  \
# filesrc do-timestamp=true location=${SRT_FILE} !\
# subparse !\
# rtpgstpay pt=98  !\
# queue !\
# fakesink silent=false -v

###############################################################################
###############################################################################
###############################################################################
#OK
#vUDP

# gst-launch-1.0  -e \
# filesrc location="OutputVideo1920p_yuv420p.mp4" !\
# decodebin !\
# videoconvert !\
# x264enc tune=zerolatency !\
# queue !\
# rtph264pay pt=96 !\
# udpsink host=${HOST}  port=${PORT} 


###############################################################################
###############################################################################
###############################################################################
#OK
#id_2UDP

# gst-launch-1.0  -e \
# \
# filesrc location="OutputVideo1920p_yuv420p.mp4" !\
# decodebin !\
# videoconvert !\
# x264enc tune=zerolatency !\
# queue !\
# rtph264pay pt=96 !\
# udpsink host=${HOST}  port=${PORT} qos=true \
# \
# filesrc do-timestamp=true location="OutputVideo1920p_yuv420p.srt" !\
# subparse !\
# queue !\
# rtpgstpay pt=98 !\
# udpsink host=${HOST} port=${SRTPORT} qos=true


###############################################################################
###############################################################################
###############################################################################

# ██████╗ ████████╗██████╗ ██████╗ ██╗███╗   ██╗
# ██╔══██╗╚══██╔══╝██╔══██╗██╔══██╗██║████╗  ██║
# ██████╔╝   ██║   ██████╔╝██████╔╝██║██╔██╗ ██║
# ██╔══██╗   ██║   ██╔═══╝ ██╔══██╗██║██║╚██╗██║
# ██║  ██║   ██║   ██║     ██████╔╝██║██║ ╚████║
# ╚═╝  ╚═╝   ╚═╝   ╚═╝     ╚═════╝ ╚═╝╚═╝  ╚═══╝senders


#RTP+RTSP
#id_2UDP

#OK
#RTP_VID_AUD
# gst-launch-1.0 -e -v  \
# rtpbin name=rtpbin \
# videotestsrc !  decodebin ! videoconvert ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! rtph264pay pt=96 ! rtpbin.send_rtp_sink_0 \
# audiotestsrc ! audioconvert ! amrnbenc ! rtpamrpay !    rtpbin.send_rtp_sink_1 \
# rtpbin.send_rtp_src_0  ! udpsink host=${HOST} port=${VID_RTP_} sync=true  async=false \
# rtpbin.send_rtcp_src_0 ! udpsink host=${HOST} port=${VID_RTCP} sync=false async=false \
# rtpbin.send_rtp_src_1  ! udpsink host=${HOST} port=${AUD_RTP_} sync=true  async=false \
# rtpbin.send_rtcp_src_1 ! udpsink host=${HOST} port=${AUD_RTCP} sync=false async=false \
# udpsrc port=${VID_RTCP_LISTEN} ! rtpbin.recv_rtcp_sink_0 \
# udpsrc port=${AUD_RTCP_LISTEN} ! rtpbin.recv_rtcp_sink_1 \


###############################################################################

#OK
#RPT_VID_SRT
# https://www.onvif.org/specs/stream/ONVIF-Streaming-Spec.pdf

# gst-launch-1.0 rtpbin name=rtpbin \
# \
# filesrc location=${VID_FILE} ! decodebin ! videoconvert ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! rtph264pay pt=96 ! rtpbin.send_rtp_sink_0 \
# rtpbin.send_rtp_src_0  ! udpsink host=${HOST} port=${VID_RTP_} sync=true  async=false \
# rtpbin.send_rtcp_src_0 ! udpsink host=${HOST} port=${VID_RTCP} sync=false async=false \
# udpsrc port=${VID_RTCP_LISTEN} ! rtpbin.recv_rtcp_sink_0 \
# \
# filesrc location=${SRT_FILE} ! subparse ! rtpgstpay pt=98 ! rtpbin.send_rtp_sink_1 \
# rtpbin.send_rtp_src_1  ! udpsink host=${HOST} port=${SRT_RTP_} sync=true async=false \
# rtpbin.send_rtcp_src_1 ! udpsink host=${HOST} port=${SRT_RTCP} sync=false async=false \
# udpsrc port=${SRT_RTCP_LISTEN} ! rtpbin.recv_rtcp_sink_1

###############################################################################
#NO

#RPT_VID_GPS_SRT


# gst-launch-1.0 rtpbin name=rtpbin \
# \
# filesrc location=${VID_FILE} ! decodebin ! videoconvert ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! rtph264pay pt=96 ! rtpbin.send_rtp_sink_0 \
# rtpbin.send_rtp_src_0  ! udpsink host=${HOST} port=${VID_RTP_} sync=true  async=false \
# rtpbin.send_rtcp_src_0 ! udpsink host=${HOST} port=${VID_RTCP} sync=false async=false \
# udpsrc port=${VID_RTCP_LISTEN} ! rtpbin.recv_rtcp_sink_0 \
# \
# filesrc location=/dev/ttyACM0 blocksize=1 ! fdsrc fd=0 ! queue !\
# text/x-raw,format=utf8 ! subtitleenc ! \! rtpgstpay pt=98 ! rtpbin.send_rtp_sink_1 \
# \
# rtpbin.send_rtp_src_1  ! udpsink host=${HOST} port=${SRT_RTP_} sync=true async=false \
# rtpbin.send_rtcp_src_1 ! udpsink host=${HOST} port=${SRT_RTCP} sync=false async=false \
# udpsrc port=${SRT_RTCP_LISTEN} ! rtpbin.recv_rtcp_sink_1



###############################################################################
###############################################################################
###############################################################################
# ██╗   ██╗██████╗ ██████╗ ███╗   ███╗██╗   ██╗██╗  ██╗
# ██║   ██║██╔══██╗██╔══██╗████╗ ████║██║   ██║╚██╗██╔╝
# ██║   ██║██║  ██║██████╔╝██╔████╔██║██║   ██║ ╚███╔╝ 
# ██║   ██║██║  ██║██╔═══╝ ██║╚██╔╝██║██║   ██║ ██╔██╗ 
# ╚██████╔╝██████╔╝██║     ██║ ╚═╝ ██║╚██████╔╝██╔╝ ██╗
#  ╚═════╝ ╚═════╝ ╚═╝     ╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═╝

# --gst-debug=*:1 --gst-debug=GST_CAPS:4 \

#NO
# gst-launch-1.0  -e \
# mqueue.src_0 !\
# rtpmux name=mux !\
# udpsink  host=${HOST}  port=${PORT} \
# \
# mqueue.src_1 ! queue ! mux. \
# \
# filesrc location=OutputVideo1920p_yuv420p.mp4 !\
# decodebin !\
# videoconvert !\
# x264enc !\
# rtph264pay pt=96 !\
# multiqueue name=mqueue \
# \
# filesrc location=OutputVideo1920p_yuv420p.srt !\
# subparse !\
# queue !\
# rtpgstpay pt=98 !\
# mqueue.

# qtdemux name=video_demux video_demux.video_0  \
#  application/x-rtp,payload=96,rate=90000
###############################################################################
###############################################################################


# gst-launch-1.0  -e \
# rtpmux name=mux !\
# udpsink  host=${HOST}  port=${PORT} \
# \
# filesrc location="OutputVideo1920p_yuv420p.mp4" !\
# decodebin !\
# videoconvert !\
# x264enc tune=zerolatency !\
# queue !\
# rtph264pay pt=96 !\
# mux.sink_0 
# \
# \
# filesrc do-timestamp=true location="OutputVideo1920p_yuv420p.srt" !\
# subparse !\
# queue !\
# rtpgstpay pt=98 !\
# mux.sink_1 \

# gst-launch-1.0  -e \
# filesrc location="OutputVideo1920p_yuv420p.mp4" !\
# decodebin !\
# videoconvert !\
# x264enc tune=zerolatency !\
# queue !\
# rtph264pay pt=96 !\
# rtpmux name=mux !\
# udpsink  host=${HOST}  port=${PORT} 