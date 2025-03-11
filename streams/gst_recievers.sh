#!/bin/sh
#RECIEVERS

HOST=127.0.0.1
PORT=5000
SRTPORT=5005
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
# nvvidconv !\
# overlay.video_sink \
# \
# filesrc location=OutputVideo1920p_yuv420p.srt !\
# subparse  video-fps=3/1 !\
# overlay.text_sink

###############################################################################
###############################################################################
###############################################################################
#OK
# fsrcVID + fSRT

# gst-launch-1.0 -e \
# filesrc location=OutputVideo1920p_yuv420p.mp4 !\
# decodebin !\
# nvvidconv ! video/x-raw,framerate=30/1,width=512,height=266 !\
# textoverlay name=overlay wait-text=true font-desc="Sans, 62" !\
# autovideosink \
# \
# filesrc location=OutputVideo1920p_yuv420p.srt do-timestamp=true !\
# subparse video-fps=30/1 !\
# overlay.text_sink

###############################################################################
###############################################################################
###############################################################################
# ОК
# UDP_SRT -> fileVID + uSRT
#CURRENT

# gst-launch-1.0 -e \
# filesrc location=OutputVideo1920p_yuv420p.mp4 !\
# decodebin !\
# nvvidconv ! video/x-raw,framerate=30/1,width=512,height=266 !\
# textoverlay name=overlay font-desc="Sans, 62" !\
# queue !\
# autovideosink \
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
# rtpgstdepay name=gstdepay  !\
# overlay.text_sink
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
# nvvidconv ! video/x-raw,framerate=30/1,width=512,height=266 !\
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
# nvvidconv ! video/x-raw,framerate=30/1,width=512,height=266 !\
# overlay.video_sink \
# \
# filesrc location=OutputVideo1920p_yuv420p.srt !\
# subparse !\
# overlay.text_sink

###############################################################################
###############################################################################
###############################################################################
#YES VERY BAD
# 2UDP-> uVID + uSRT

# gst-launch-1.0 -e \
# textoverlay name=overlay font-desc="Sans, 62" !\
# autovideosink sync=true \
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
# nvvidconv ! video/x-raw,framerate=30/1,width=512,height=266 !\
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
#NO
# 2UDP-> uVID + uSRT

gst-launch-1.0  \
textoverlay name=overlay font-desc="Sans, 62" !\
autovideosink sync=false \
\
udpsrc name=VID_udp_stream port=${PORT} \
    caps="application/x-rtp, \
        encoding-name=H264, \
        media=video, \
        clock-rate=(int)90000, \
        payload=(int)96 " !\
rtpjitterbuffer !\
rtph264depay !\
queue !\
h264parse !\
decodebin !\
nvvidconv ! video/x-raw,framerate=30/1,width=512,height=266 !\
overlay.video_sink \
\
udpsrc name=SRT_udp_stream \
    address=${HOST} port=${SRTPORT} \
    do-timestamp=true \
    caps="application/x-rtp, \
        encoding-name=X-GST, \
        media=application, \
        payload=(int)98 " !\
rtpgstdepay name=gstdepay !\
queue !\
overlay.text_sink
###############################################################################
###############################################################################
###############################################################################

# gst-launch-1.0 -e \
# textoverlay name=overlay !\
# autovideosink sync=false \
# udpsrc name=udpVID port=5000 caps="\
# application/x-rtp, \
# encoding-name=H264, \
# media=video, \
# clock-rate=(int)90000, \
# payload=(int)96 " !\
#     rtph264depay  !\
#     h264parse !\
#     decodebin !\
#     nvvidconv !\
#     overlay.video_sink \
# udpsrc name=udpSRT port=5001 caps=" \
# application/x-rtp, \
# encoding-name=X-GST, \
# media=application, \
# clock-rate=(int)90000, \
# payload=(int)98 " !\
#     rtpgstdepay name=gstdepay  ! subparse ! overlay.text_sink

# filesrc location=OutputVideo1920p_yuv420p.srt !     subparse !     overlay.text_sink      


###############################################################################
###############################################################################
###############################################################################

    # text="Hello Smoke!" \
    # font-desc="Sans, 72" \
    # shaded-background=yes !\
    
# gst-launch-1.0 -v -e --gst-debug=*:2 \
# textoverlay name=overlay \
# autovideosink sync=false \
#     udpsrc name=mux port=5000 caps="application/x-rtp" ! rtpptdemux name=demux \
#         demux.src_96 !\
#         "application/x-rtp,encoding-name=H264, media=video, clock-rate=90000, payload=96" !\
#             rtph264depay !\
#             decodebin !\
#             nvvidconv ! "video/x-raw,format=NV12" !\
#             queue !\
#             overlay.video_sink \
#             \
#         demux.src_98 !\
#         "application/x-rtp,encoding-name=X-GST, media=application, clock-rate=90000, payload=98" !\
#             rtpgstdepay !\
#             subparse !\
#             queue !\
#             overlay.text_sink 


# gst-launch-1.0 textoverlay name=overlay ! videoconvert ! videoscale ! autovideosink \
# filesrc location=movie.avi ! decodebin3 !  videoconvert ! overlay.video_sink \
# filesrc location=movie.srt ! subparse ! overlay.text_sink

###############################################################################
###############################################################################
###############################################################################
#TEST 

# gst-launch-1.0 -e \
# rtpbin name=rtpbin \
# \
# udpsrc port=${PORT} \
#     caps="application/x-rtp, \
#         media=(string)video, \
#         encoding-name=(string)H264, \
#         payload=(int)96" !\
# rtpbin.recv_rtp_sink_0 \
# \
# rtpbin.recv_rtp_src_0 !\
# rtph264depay !\
# decodebin !\
# autovideosink \
# \
# udpsrc port=$((${PORT} + 1)) ! rtpbin.recv_rtcp_sink_0 \
# \
# \
# \
# udpsrc port=${SRTPORT} \
#     caps="application/x-rtp, \
#         media=(string)text, \false
#         encoding-name=(string)X-GST, \
#         payload=(int)98" !\
# rtpbin.recv_rtp_sink_1 \
# \
# rtpbin.recv_rtp_src_1 !\
# rtpgstdepay !\
# subparse !\
# textoverlay name=overlay !\
# autovideosink \
# \
# udpsrc port=$((${SRTPORT} + 1)) ! rtpbin.recv_rtcp_sink_1
