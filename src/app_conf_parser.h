#pragma once
#ifndef STRUCTS_H // Защита от повторного включения
#define STRUCTS_H
#include <gst/gst.h>
#include <glib.h>

typedef struct
{
    gboolean is_cuda_device_integrated;
    gboolean is_nvinfer_server;
    gchar *infer_config_file;
    guint num_sources;
    char **sources;
    gchar *output_filename;
    int muxer_output_width;
    int muxer_output_height;
    int muxer_batch_timeout_usec;
    int nvsegvisual_width;
    int nvsegvisual_height;
    float nvsegvisual_alpha;
    int nvdosd_clock_font_size;
    int valve_video_duration_sec;
} app_init_context;

/**
 * @brief Парсит JSON конфиг и заполняет структуру конфигурации
 * @param config_path Путь к JSON файлу конфигурации
 * @return Указатель на заполненную структуру конфигурации или NULL при ошибке
 */
app_init_context* parse_config_file(const char* config_path);

#endif