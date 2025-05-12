#pragma once

#include <gst/gst.h>
#define GST_CAPS_FEATURES_NVMM "memory:NVMM"

// Обработчик сообщений от bus
gboolean bus_call(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, gpointer data);

// Обработчик нового pad в decodebin
void cb_newpad(GstElement *decodebin G_GNUC_UNUSED, GstPad *pad, gpointer data);