# --gst-debug=*:1 --gst-debug=GST_CAPS:4 \
gst-launch-1.0  -e \
queue name=rtpqueue!\
rtpmux name=mux !\
udpsink  host=127.0.0.1 port=5000 \
\
filesrc location=OutputVideo1920p_yuv420p.mp4 !\
    decodebin !\
    nvvidconv !\
    queue !\
    x264enc  speed-preset=superfast ! \
    queue !\
    rtph264pay  pt=96 ! \
    mux.sink_0 \
\
filesrc location=OutputVideo1920p_yuv420p.srt !\
    subparse !\
    queue !\
    rtpgstpay pt=98 !\
    mux.sink_1

# qtdemux name=video_demux video_demux.video_0  \
#  application/x-rtp,payload=96,rate=90000