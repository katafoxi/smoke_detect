#!/bin/bash
gst-launch-1.0 \
v4l2src device=/dev/video0 io-mode=2 !\
    "image/jpeg,width=1280,height=720,framerate=30/1,format=MJPG" !\
jpegdec !\
nvvideoconvert ! "video/x-raw(memory:NVMM),format=NV12" !\
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
	config-file-path="./configs/seg_ac_infer.txt" !\
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
nvvidconv ! "video/x-raw,format=NV12" !\
nv3dsink async=0 

<<EXAMPLE
## работает как udpsrc

gst-launch-1.0 \
v4l2src device=/dev/video0 !\
    image/jpeg,format=MJPG,width=1280,height=720,framerate=30/1 !\
nvv4l2decoder mjpeg=1 !\
nvvidconv ! video/x-raw, format=NV12 !\
x264enc speed-preset=superfast !\
rtph264pay !\
udpsink port=5000 host=127.0.0.1


#base
v4l2src device=/dev/video0 !\
    image/jpeg,format=MJPG,width=1280,height=720,framerate=30/1 !\
decodebin !\
videoconvert !\


# https://forums.developer.nvidia.com/t/csi-camera-input-deepstream-python-application/217411/11
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-h264,width=1280,height=720,framerate=30/1 ! fakesink


## https://docs.nvidia.com/jetson/archives/r34.1/DeveloperGuide/text/SD/Multimedia/AcceleratedGstreamer.html#progressive-capture-using-nvv4l2camerasrc

gst-launch-1.0 \
nvv4l2camerasrc device=/dev/video3 ! \
    'video/x-raw(memory:NVMM), format=(string)UYVY, \
    width=(int)1280, height=(int)720, \
    interlace-mode= progressive, \
    framerate=(fraction)30/1' ! \
nvvidconv ! \
    'video/x-raw(memory:NVMM), format=(string)NV12' ! \
nv3dsink -e

#работает
gst-launch-1.0 v4l2src device="/dev/video0" ! \
     "video/x-raw, width=640, height=480, format=(string)YUY2" ! \
     xvimagesink -e

## 
## https://forums.developer.nvidia.com/t/deepstream-pipeline-for-mjpg-usb-camera/284150/4
gst-launch-1.0 \
v4l2src device=/dev/video0 io-mode=2 !\
    'image/jpeg,width=1280,height=720,framerate=30/1,format=MJPG' !\
jpegdec !\
nvvideoconvert ! 'video/x-raw(memory:NVMM),format=NV12' !\
mux.sink_0 nvstreammux live-source=1 name=mux batch-size=1 width=1280 height=720 !\
nvinfer config-file-path=/workspace/ds-test/config_pgie_yolo_v4.txt batch-size=1 !\
nvmultistreamtiler rows=1 columns=1 width=1920 height=1080 !\
nvvideoconvert !\
nvegltransform !\
nveglglessink
EXAMPLE
