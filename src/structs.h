#pragma once
#ifndef STRUCTS_H  // Защита от повторного включения
#define STRUCTS_H
#include <gst/gst.h>
#include <glib.h>


typedef struct
{
    int argc;
    char **argv;
    gboolean is_cuda_device_integrated;
    gboolean is_nvinfer_server;
    gchar *infer_config_file;
    guint num_sources;
    gchar *output_filename;
} app_init_context;

#endif