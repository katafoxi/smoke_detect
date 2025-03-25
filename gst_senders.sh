#!/bin/sh
#RECIEVERS

FILENAME="OutputVideo1920p_yuv420p"
VID_FILE="${FILENAME}.mp4"
SRT_FILE="${FILENAME}.srt"

HOST=127.0.0.1
PORT=5000
SRTPORT=5005

VID_RTP_=5010
VID_RTCP=5011
VID_RTCP_RECV=5015

AUD_RTP_=5020
AUD_RTCP=5021
AUD_RTCP_RECV=5025

SRT_RTP_=5030
SRT_RTCP=5031
SRT_RTCP_RECV=5035

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
#OK
# RPT_VID_SRT

gst-launch-1.0 -e --gst-debug=*:2  \
rtpbin name=rtpbin  \
udpsrc address=${HOST} port=${VID_RTP_} caps="${VID_RTP__CAPS}" ! rtpbin.recv_rtp_sink_0 \
udpsrc address=${HOST} port=${VID_RTCP} caps="${VID_RTCP_CAPS}" ! rtpbin.recv_rtcp_sink_0 \
rtpbin.send_rtcp_src_0 ! udpsink host=${HOST}  port=${VID_RTCP_RECV} sync=false async=false \
\
udpsrc address=${HOST} port=${SRT_RTP_} caps="${SRT_RTP__CAPS}" ! rtpbin.recv_rtp_sink_1 \
udpsrc address=${HOST} port=${SRT_RTCP}                         ! rtpbin.recv_rtcp_sink_1 \
rtpbin.send_rtcp_src_1 ! udpsink host=${HOST} port=${SRT_RTCP_RECV} sync=false async=false \
\
rtpbin. ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! textoverlay name=overlay wait-text=false font-desc="Sans, 62" ! autovideosink sync=false \
rtpbin. ! rtpgstdepay  ! overlay.text_sink

#is-live=true
# ! "text/x-raw,format=(string)pango-markup" 

###############################################################################

#OK
#RTP_VID_AUD --> VID_AUD
# gst-launch-1.0 -v \
# rtpbin name=rtpbin \
# udpsrc address=${HOST} port=${VID_RTP_} caps="${VID_RTP__CAPS}" ! rtpbin.recv_rtp_sink_0 \
# udpsrc address=${HOST} port=${VID_RTCP} caps="${VID_RTCP_CAPS}" ! rtpbin.recv_rtcp_sink_0 \
# udpsrc address=${HOST} port=${AUD_RTP_} caps="${AUD_RTP__CAPS}" ! rtpbin.recv_rtp_sink_1 \
# udpsrc address=${HOST} port=${AUD_RTCP} caps="${AUD_RTCP_CAPS}" ! rtpbin.recv_rtcp_sink_1 \
# rtpbin. ! rtph264depay ! decodebin ! videoconvert ! autovideosink sync=true \
# rtpbin. ! rtpamrdepay  ! amrnbdec  ! audioconvert ! autoaudiosink sync=true




###############################################################################
#OK
# fVID

# gst-launch-1.0 -e -v \
# filesrc location=${VID_FILE} !\
# decodebin caps="video/x-raw" !\
# videoscale ! video/x-raw,width=512,height=512 ! \
# videoconvert !\
# autovideosink 


###############################################################################
###############################################################################
###############################################################################
#OK
# testVID + fSRT

# gst-launch-1.0 -e \
# textoverlay name=overlay wait-text=true font-desc="Sans, 62" !\
# autovideosink \
# \
# videotestsrc \
#     do-timestamp=true \
#     pattern=18 \
#     flip=true \
#     animation-mode=1 !\
#     "video/x-raw,framerate=30/1,width=512,height=266" !\
# videorate max-rate=30 !\
# videoconvert !\
# overlay.video_sink \
# \
# filesrc location=${SRT_FILE} !\
# subparse  video-fps=3/1 !\
# overlay.text_sink


###############################################################################
###############################################################################
###############################################################################
#OK
# fVID + fSRT

# gst-launch-1.0 -e -v \
# textoverlay name=overlay \
#     wait-text=true \
#     font-desc="Sans, 62" ! \
# "video/x-raw,width=512,height=512" !\
# autovideosink \
# \
# filesrc location=${VID_FILE} ! decodebin ! videoscale ! videoconvert     ! overlay.video_sink \
# filesrc location=${SRT_FILE} do-timestamp=true ! subparse video-fps=30/1 ! overlay.text_sink



###############################################################################
###############################################################################
###############################################################################
#OKbad
# UDP_SRT --> testVID + uSRT

# gst-launch-1.0 -e \
# textoverlay name=overlay wait-text=false font-desc="Sans, 62" !\
# autovideosink \
# \
# videotestsrc \
#     do-timestamp=true \
#     pattern=18 \
#     flip=true \
#     animation-mode=1 !\
#     "video/x-raw,framerate=30/1,width=512,height=266" !\
# videorate max-rate=30 !\
# videoconvert !\
# queue !\
# overlay.video_sink \
# \
# udpsrc name=SRT_udp_stream \
#     address=${HOST} port=${SRTPORT} \
#     do-timestamp=true \
#     caps="application/x-rtp, \
#         encoding-name=X-GST, \
#         media=application, \
#         payload=(int)98 " !\
# rtpjitterbuffer latency=500 !\
# rtpgstdepay name=gstdepay  !\
# queue !\
# overlay.text_sink



###############################################################################
###############################################################################
###############################################################################
# ОК
# UDP_SRT -> fileVID + uSRT

#OK
# gst-launch-1.0 -e \
# textoverlay name=overlay wait-text=false font-desc="Sans, 62" !\
# autovideosink \
# \
# filesrc location=${VID_FILE} !\
# decodebin !\
# videoscale ! "video/x-raw,width=512,height=512" !\
# videoconvert !\
# queue !\
# overlay.video_sink \
# \
# udpsrc name=SRT_udp_stream \
#     address=${HOST} port=${SRTPORT} \
#     buffer-size=1000000 \
#     do-timestamp=true \
#     caps="application/x-rtp, \
#         encoding-name=X-GST, \
#         media=application, \
#         clock-rate=(int)90000, \
#         payload=(int)98 " !\
# rtpjitterbuffer latency=500 !\
# rtpgstdepay name=gstdepay  !\
# queue !\
# overlay.text_sink


###############################################################################
###############################################################################
###############################################################################
#OK
#vUDP -> uVID

# gst-launch-1.0 -e \
# udpsrc name=VID_udp_stream port=${PORT} \
#     caps="application/x-rtp, \
#         encoding-name=H264, \
#         media=video, \
#         clock-rate=(int)90000, \
#         payload=(int)96" !\
# rtph264depay !\
# h264parse !\
# decodebin !\
# videoconvert  !\
# autovideosink sync=false 

###############################################################################
###############################################################################
###############################################################################
#OK
#2UDP -> uVID + staticOverlay

# gst-launch-1.0 -e \
# textoverlay name=overlay \
#     text="Hello Smoke!" \
#     font-desc="Sans, 72" \
#     shaded-background=yes !\
# autovideosink sync=false \
# \
# udpsrc name=VID_udp_stream port=${PORT} \
#     caps="application/x-rtp, \
#         encoding-name=H264, \
#         media=video, \
#         clock-rate=(int)90000, \
#         payload=(int)96" !\
# rtph264depay !\
# h264parse !\
# decodebin !\
# videoconvert !\
# overlay.video_sink

###############################################################################
###############################################################################
###############################################################################
#OK
#2UDP-> uVID + fileSRT_Overlay

# gst-launch-1.0 -e \
# textoverlay name=overlay font-desc="Sans, 62" !\
# autovideosink sync=false \
# \
# udpsrc name=VID_udp_stream port=${PORT} \
#     caps="application/x-rtp, \
#         encoding-name=H264, \
#         media=video, \
#         clock-rate=(int)90000, \
#         payload=(int)96 " !\
# rtph264depay !\
# h264parse !\
# decodebin !\
# videoconvert !\
# overlay.video_sink \
# \
# filesrc location=${SRT_FILE} !\
# subparse !\
# overlay.text_sink

###############################################################################
###############################################################################
###############################################################################
#OK VERY BAD
# 2UDP-> uVID + uSRT

# autovideosink sync=true \


# gst-launch-1.0 -e \
# textoverlay name=overlay  wait-text=false font-desc="Sans, 62" !\
# autovideosink \
# \
# udpsrc name=VID_udp_stream port=${PORT} \
#     caps="application/x-rtp, \
#         encoding-name=H264, \
#         media=video, \
#         clock-rate=(int)90000, \
#         payload=(int)96 " !\
# rtpjitterbuffer !\
# rtph264depay !\
# h264parse !\
# decodebin !\
# videoconvert !\
# overlay.video_sink \
# \
# udpsrc name=SRT_udp_stream \
#     address=${HOST} port=${SRTPORT} \
#     do-timestamp=true \
#     caps="application/x-rtp, \
#         encoding-name=X-GST, \
#         media=application, \
#         payload=(int)98 " !\
# rtpjitterbuffer !\
# queue !\
# rtpgstdepay name=gstdepay !\
# overlay.text_sink


###############################################################################
###############################################################################
###############################################################################
# ██╗   ██╗██████╗ ██████╗ ███╗   ███╗██╗   ██╗██╗  ██╗
# ██║   ██║██╔══██╗██╔══██╗████╗ ████║██║   ██║╚██╗██╔╝
# ██║   ██║██║  ██║██████╔╝██╔████╔██║██║   ██║ ╚███╔╝ 
# ██║   ██║██║  ██║██╔═══╝ ██║╚██╔╝██║██║   ██║ ██╔██╗ 
# ╚██████╔╝██████╔╝██║     ██║ ╚═╝ ██║╚██████╔╝██╔╝ ██╗
#  ╚═════╝ ╚═════╝ ╚═╝     ╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═╝recievers


# gst-launch-1.0 -v -e --gst-debug=*:2 \
# udpsrc name=VID_udp_stream port=${PORT} \
#     caps="application/x-rtp, \
#         encoding-name=H264, \
#         media=video, \
#         clock-rate=(int)90000, \
#         payload=(int)96 " !\
# rtpjitterbuffer !\
# rtph264depay !\
# h264parse !\
# decodebin !\
# videoconvert !\
# autovideosink

# gst-launch-1.0 -e  --gst-debug=*:2 \
# udpsrc name=mux port=5000 ! "application/x-rtp,media=video,encoding-name=H264" !\
# rtpptdemux name=demux !\
# rtph264depay !\
# h264parse !\
# decodebin !\
# videoconvert  !\
# fakesink 

###############################################################################
    
# gst-launch-1.0 -v -e --gst-debug=*:2 \

# gst-launch-1.0 -v -e --gst-debug=*:2 \
# textoverlay name=overlay !\
# autovideosink sync=false \
# \
# udpsrc name=mux port=5000 caps="application/x-rtp,media=application" !\
# rtpptdemux name=demux \
# \
# demux.src_96 !\
# rtpjitterbuffer ! rtph264depay ! h264parse ! decodebin ! videoconvert ! queue ! overlay.video_sink \
# \
# demux.src_98 ! \
# rtpjitterbuffer ! rtpgstdepay ! queue ! overlay.text_sink 

#  "application/x-rtp,encoding-name=H264, media=video, clock-rate=90000, payload=96" 
# "application/x-rtp,encoding-name=X-GST, media=application, payload=98" 

