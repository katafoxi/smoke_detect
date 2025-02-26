gst-launch-1.0 -v -e --gst-debug=*:1 \
rtpmux name=mux !\
queue2 !\
udpsink host=127.0.0.1 port=5000 \
filesrc location=OutputVideo1920p_yuv420p.mp4 !\
    qtdemux name=video_demux video_demux.video_0 ! \
    queue ! \
    decodebin !\
    nvvidconv !\
    queue2 !\
    x264enc  speed-preset=superfast  ! \
    queue2 !\
    rtph264pay pt=96 ! \
    mux.sink_0 \
filesrc location=OutputVideo1920p_yuv420p.srt !\
    subparse !\
    queue2 !\
    rtpgstpay pt=98 ! \
    mux.sink_1 





# Mixing video+audio
# https://gist.github.com/hum4n0id/cda96fb07a34300cdb2c0e314c14df0a#mixing-videoaudio
# sender(mp4 file)
# SRC=./OutputVideo1920p_yuv420p.mp4 
# gst-launch-1.0 -v \
# mpegtsmux \
#     name=mux \
#     alignment=7 !\
# rtpmp2tpay !\
# udpsink port=5000 host=$HOST sync=false \
# \
# compositor name=videomix !\
#     omxh264enc bitrate=16000000 !\
#     queue2 !                                                mux. \
# audiomixer name=audiomix !\
#     audioconvert !\
#     audioresample !\
#     avenc_ac3 !\
#     queue2 !                                                mux. \
# filesrc location=$SRC !\
#     qtdemux name=demux \
#     demux.video_0 !\
#         queue2 !\
#         h264parse !\
#         omxh264dec !\
#         videoconvert !                                      videomix. \
#     demux.audio_0 !\
#         queue2 !\
#         decodebin !\
#         audioconvert !\
#         audioresample !                                     audiomix. \
# videotestsrc pattern=ball ! videoscale ! video/x-raw,width=100,height=100 !\        videomix. \
# audiotestsrc freq=600 volume=0.1 !                                                  audiomix.



# play `mp4` file with both audio and video
# gst-launch-1.0 filesrc location=/Users/siyao/Movies/why_I_left_China_for_Good.mp4 \
#   ! qtdemux name=demux  demux.audio_0 \
#   ! queue ! decodebin ! audioconvert ! audioresample ! autoaudiosink  demux.video_0 \
#   ! queue ! decodebin ! videoconvert ! videoscale ! autovideosink



# Audio + Video RTP Streaming
# https://gist.github.com/hum4n0id/cda96fb07a34300cdb2c0e314c14df0a#audio--video-rtp-streaming
# sender
# gst-launch-1.0 -v videotestsrc \
#   ! 'video/x-raw,format=I420,width=1920,height=1080,framerate=60/1' \
#   ! omxh264enc insert-sps-pps=true bitrate=16000000 \
#   ! h264parse \
#   ! rtph264pay pt=96 \
#   ! udpsink port=5000 host=$HOST audiotestsrc \
#   ! audioconvert \
#   ! audioresample \
#   ! rtpL24pay \
#   ! udpsink host=$HOST port=5001


# demux and split video and audio(not best)
# https://gist.github.com/hum4n0id/cda96fb07a34300cdb2c0e314c14df0a#demux-and-split-video-and-audionot-best
# sender `mp4` file
gst-launch-1.0 -v \
filesrc location=OutputVideo1920p_yuv420p.mp4 !\
qtdemux name=demux demux.video_0 !\
queue !\
    h264parse !\
    decodebin !\
    x264enc !\
    rtph264pay pt=96 !\
udpsink host=127.0.0.1 port=5000 demux.audio_0 !\
queue !\
    decodebin !\
    audioconvert !\
    audioresample !\
    avenc_ac3 !\
    rtpac3pay !\
udpsink host=127.0.0.1 port=5001

