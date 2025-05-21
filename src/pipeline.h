#ifndef GST_PIPELINE_H
#define GST_PIPELINE_H

#include <gst/gst.h>
#include <glib.h>
#include "app_conf_parser.h"

/* The muxer output resolution must be set if the input streams will be of
 * different resolution. The muxer will scale all the input frames to this
 * resolution. */
#define MUXER_OUTPUT_WIDTH 1280
#define MUXER_OUTPUT_HEIGHT 720

/* Muxer batch formation timeout, for e.g. 40 millisec. Should ideally be set
 * based on the fastest source's framerate. */
#define MUXER_BATCH_TIMEOUT_USEC 40000

#define TILED_OUTPUT_WIDTH 512
#define TILED_OUTPUT_HEIGHT 512

#define NVINFER_PLUGIN "nvinfer"
#define NVINFERSERVER_PLUGIN "nvinferserver"
/* NVIDIA Decoder source pad memory feature. This feature signifies that source
 * pads having this capability will push GstBuffers containing cuda buffers. */
#define GST_CAPS_FEATURES_NVMM "memory:NVMM"



// Структура для хранения компонентов pipeline
typedef struct {
    GstElement *pipeline;     // Весь pipeline
    GstElement *streammux;    // nvstreammux
    GstElement *pgie;         // nvinfer или nvinferserver
    GstElement *nvvidconv;
    GstElement *nvsegvisual;  // nvsegvisual
    GstElement *nvdsosd;      // nvdsosd
    GstElement *textoverlay;
    GstElement *text_src;
    GstElement *tee;
    GstElement *queue1;
    GstElement *queue2;
    GstElement *valve;
    GstElement *x264enc;
    GstElement *filesink;
    GstElement *sink;         // Вывод (nveglglessink, filesink и т. д.)
} PipelineComponents;


// Функции для работы с pipeline

/**
 * Создаёт и настраивает GStreamer-пайплайн.
 * 
 * @param app_conf  Контекст для приложения

 * 
 * @return Указатель на PipelineComponents или NULL при ошибке
 */
PipelineComponents* build_pipeline(app_init_context *app_conf);

/**
 * Освобождает ресурсы pipeline.
 */
void free_pipeline(PipelineComponents *pipeline);

/**
 * Создаёт source bin (uridecodebin + видеоисточник).
 */
GstElement* create_source_bin(guint index, const gchar *uri);

#endif // GST_PIPELINE_H