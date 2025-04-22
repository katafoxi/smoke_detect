#!/bin/bash

HOST=127.0.0.1
PORT=5000
SRTPORT=5005

VID_RTP_=5010
VID_RTCP=5011
VID_RTCP_LISTEN=5015

AUD_RTP_=5020
AUD_RTCP=5021
AUD_RTCP_LISTEN=5025

SRT_RTP_=5030
SRT_RTCP=5031
SRT_RTCP_LISTEN=5035

FILENAME="0402_1"
VID_FORMAT="MP4"
VID_FILE="${FILENAME}.${VID_FORMAT}"
SRT_FILE="./streams/${FILENAME}.srt"

gst-launch-1.0 \
uridecodebin \
    uri=file:///wdir/smoke_detect/streams/${FILENAME}.${VID_FORMAT} \
    name=dec !\
m.sink_0 nvstreammux \
	name=m \
	batch-size=1 \
	width=1280  \
	height=720 !\
nvinfer \
	name=nvinfer \
	unique-id=1  \
	infer-on-gie-id=1 \
	infer-on-class-ids="0:"  \
	batch-size=1  \
	config-file-path="./configs/segm_aa_infer_conf.txt" !\
nvsegvisual \
    batch-size=1 \
    width=512 \
    height=512 \
    alpha=0.7 \
    original-background=1 \
    class-id=0 \
    gpu-on=1 \
    qos=0 \
    operate-on-seg-meta-id=1 !\
nvdsosd \
    display-clock=1 \
    display-mask=1 \
    clock-font="Serif" \
    clock-font-size=12 \
    process-mode=1 \
    qos=1 !\
nvvidconv ! video/x-raw,format=NV12 ! \
x264enc  speed-preset=superfast  ! \
mux. mp4mux name=mux ! \
filesink location=segm_${FILENAME}.${VID_FORMAT} 
