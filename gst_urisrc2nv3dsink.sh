#!/bin/bash
gst-launch-1.0 \
uridecodebin \
    uri="file:///wdir/smoke_detect/streams/OutputVideo1920p_yuv420p.mp4" \
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
	infer-on-class-ids="0:" \
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
    display-text=1 \
    clock-font="Serif" \
    clock-font-size=12 \
    process-mode=1 \
    qos=1 !\
nvvidconv ! "video/x-raw,format=NV12" !\
textoverlay \
    text="Hello Smoke!" \
    font-desc="Sans, 72" \
    shaded-background=yes !\
nv3dsink async=0 
