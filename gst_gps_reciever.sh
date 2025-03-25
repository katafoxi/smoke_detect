gst-launch-1.0 \
    udpsrc port=5000 ! \
    tsparse ! \
    tsdemux name=demux \
    \
    demux. ! \
    queue ! \
    h264parse ! \
    avdec_h264 ! \
    videoconvert ! \
    autovideosink \
    \
    demux. ! \
    queue ! \
    subtitledec ! \
    textoverlay valignment=top halignment=left font-desc="Sans 24" ! \
    videoconvert ! \
    autovideosink