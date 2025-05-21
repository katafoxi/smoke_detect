#!/bin/bash

GPS_TO_SRT_PLUGIN_PATH="/wdir/gps_parse_plugin/builddir/plugin/"
DEEPSTREAM_PLUGIN_PATH="/opt/nvidia/deepstream/deepstream-7.0/lib/gst-plugins/"
export GST_PLUGIN_SYSTEM_PATH_1_0="/lib/aarch64-linux-gnu/gstreamer-1.0:${GPS_TO_SRT_PLUGIN_PATH}:${DEEPSTREAM_PLUGIN_PATH}"
export GST_PLUGIN_PATH="/lib/aarch64-linux-gnu/gstreamer-1.0:${GPS_TO_SRT_PLUGIN_PATH}:${DEEPSTREAM_PLUGIN_PATH}"

echo "$GST_PLUGIN_SYSTEM_PATH_1_0"

# gst-launch-1.0 \
#   filesrc location=test.mp4 ! \
#     decodebin name=dec \
#     dec. ! queue ! nvvidconv ! textoverlay name=overlay ! autovideosink \
#   mysrc ! overlay.

 /wdir/smoke_detect/build/segmsmoke_app "app_config.json"


