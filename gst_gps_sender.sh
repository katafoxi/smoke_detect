#!/bin/sh
#SENDERS

HOST=127.0.0.1
PORT=5000
SRTPORT=5005

gst-launch-1.0 \
    # Видеоисточник (например, веб-камера)
    v4l2src device=/dev/video0 ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    queue ! \
    x264enc tune=zerolatency bitrate=500 ! \
    h264parse ! \
    mpegtsmux name=mux ! \
    queue ! \
    udpsink host=<IP_получателя> port=5000 \
    \
    # GPS-источник (NMEA через /dev/ttyACM0)
    filesrc location=/dev/ttyACM0 blocksize=1 ! \
    fdsrc fd=0 ! \
    queue ! \
    text/x-raw,format=utf8 ! \
    subtitleenc ! \
    queue ! \
    mux.