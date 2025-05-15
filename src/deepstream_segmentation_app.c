/*
 * SPDX-FileCopyrightText: Copyright (c) 2019-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <cuda_runtime_api.h>

#include "gstnvdsmeta.h"
#include "nvds_yml_parser.h"
#include "gst-nvmessage.h"
#include <time.h> // Добавлено для nanosleep
#include "callback.h"

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

#define CREATE_ELEMENT(var, factory, name)              \
  if (!(var = gst_element_factory_make(factory, name))) \
  {                                                     \
    g_printerr("Failed to create %s\n", name);          \
    return -1;                                          \
  }

// #define MAPPING "/ridgerun"
// #define SERVICE "12345"

// static gboolean PERF_MODE = FALSE;

/* tiler_sink_pad_buffer_probe  will extract metadata received
on segmentation  src pad */
static GstPadProbeReturn
tiler_src_pad_buffer_probe(
    GstPad *pad G_GNUC_UNUSED,
    GstPadProbeInfo *info,
    gpointer u_data G_GNUC_UNUSED)
{
  GstBuffer *buf = (GstBuffer *)info->data;
  NvDsMetaList *l_frame = NULL;
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

  for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
       l_frame = l_frame->next)
  {
    // TODO:
  }
  return GST_PAD_PROBE_OK;
}

static GstElement *
create_source_bin(guint index, gchar *uri)
{
  GstElement *bin = NULL;
  GstElement *uri_decode_bin = NULL;
  gchar bin_name[16] = {0};

  g_snprintf(bin_name, 15, "source-bin-%02d", index);
  /* Create a source GstBin to abstract this bin's content from the rest of the
   * pipeline */
  bin = gst_bin_new(bin_name);
  if (!bin)
  {
    g_printerr("element BIN in source bin could not be created.\n");
    return NULL;
  }

  /* Source element for reading from the uri.
   * We will use decodebin and let it figure out the container format of the
   * stream and the codec and plug the appropriate demux and decode plugins. */
  uri_decode_bin = gst_element_factory_make("uridecodebin", "uri-decode-bin");
  if (!uri_decode_bin)
  {
    g_printerr("uri_decode_bin in source bin could not be created.\n");
    gst_object_unref(bin); // Освобождаем bin перед возвратом
    return NULL;
  }
  g_object_set(G_OBJECT(uri_decode_bin), "uri", uri, NULL);
  g_signal_connect(G_OBJECT(uri_decode_bin), "pad-added", G_CALLBACK(cb_newpad), bin);
  // add uri_decode_bin to common bin
  gst_bin_add(GST_BIN(bin), uri_decode_bin);

  /* We need to create a ghost pad for the source bin which will act as a proxy
   * for the video decoder src pad. The ghost pad will not have a target right
   * now. Once the decode bin creates the video decoder and generates the
   * cb_newpad callback, we will set the ghost pad target to the video decoder
   * src pad. */
  if (!gst_element_add_pad(bin, gst_ghost_pad_new_no_target("src", GST_PAD_SRC)))
  {
    g_printerr("Failed to add ghost pad in source bin\n");
    return NULL;
  }

  return bin;
}

// Динамическое управление valve через 5 секунд
static gboolean
close_valve(gpointer valve)
{
  g_print("Closing valve...\n");
  g_object_set(valve, "drop", TRUE, NULL);
  return G_SOURCE_REMOVE;
}

static void
usage(const char *bin_name)
{
  g_printerr(" \
Something wrong with arguments.\n \
-------------------------------\n \
Usage:\n %s config_file <file1> [file2] ... [fileN]\n\n", bin_name);
  g_printerr(" \
For nvinferserver, Usage:\n \
%s -t inferserver config_file <file1> [file2] ... [fileN]\n", bin_name);
}

static int
fill_cuda_device_prop(struct cudaDeviceProp *cuda_device_prop)
{
  int current_device_id = -1;
  cudaError_t cuda_err = cudaGetDevice(&current_device_id);

  if (cuda_err != cudaSuccess || current_device_id == -1)
  {
    g_printerr("CUDA error getting device_id: %s\n", cudaGetErrorString(cuda_err));
    return -1;
  }

  cuda_err = cudaGetDeviceProperties(cuda_device_prop, current_device_id);
  if (cuda_err != cudaSuccess)
  {
    g_printerr("CUDA don`t get device prop error: %s\n", cudaGetErrorString(cuda_err));
    return -1;
  }
  g_print("Using CUDA device %d: %s\n", current_device_id, cuda_device_prop->name);
  return 0;
}

static int
check_input_arguments(int argc, char *argv[], gboolean *is_nvinfer_server)
{
  if (argc < 3)
  {
    usage(argv[0]);
    return -1;
  }

  if (argc >= 3 && !strcmp("-t", argv[1]))
  {
    if (!strcmp("inferserver", argv[2]))
    {
      *is_nvinfer_server = TRUE;
    }
    else
    {
      usage(argv[0]);
      return -1;
    }
    g_print("Using nvinferserver as the inference plugin\n");
  }
  return 0;
}

int main(int argc, char *argv[])
{
  GMainLoop *loop = NULL;
  GstElement
      *pipeline = NULL,
      *streammux = NULL,
      *pgie = NULL,
      *nvvidconv = NULL,
      *nvsegvisual = NULL,
      *nvdsosd = NULL,
      *textoverlay = NULL,
      *text_src = NULL,
      *tee = NULL,
      *queue1 = NULL,
      *queue2 = NULL,
      *valve = NULL,
      *x264enc = NULL,
      *filesink = NULL,
      *sink = NULL;

  GstBus *bus = NULL;
  guint bus_watch_id;
  GstPad *tiler_src_pad = NULL;
  guint i, num_sources = 0;
  // guint tiler_rows, tiler_columns;
  guint pgie_batch_size;
  gboolean is_nvinfer_server = FALSE;
  gchar *infer_config_file = NULL;
  struct cudaDeviceProp cuda_device_prop;

  
  if (check_input_arguments(argc, argv, &is_nvinfer_server)!= 0){
    return -1;
  }
  
  fill_cuda_device_prop(&cuda_device_prop);

  if (is_nvinfer_server)
  {
    num_sources = argc - 4;
    infer_config_file = argv[3];
  }
  else
  {
    num_sources = argc - 2;
    infer_config_file = argv[1];
  }

  /* Standard GStreamer initialization */
  gst_init(&argc, &argv);
  loop = g_main_loop_new(NULL, FALSE);

  /* Create gstreamer elements */
  /* Create Pipeline element that will form a connection of other elements */
  pipeline = gst_pipeline_new("smoke-segmentation-pipeline");

  /* Create nvstreammux instance to form batches from one or more sources. */
  CREATE_ELEMENT(streammux, "nvstreammux", "stream-muxer");
  g_object_set(G_OBJECT(streammux),
               "batch-size", num_sources,
               "width", MUXER_OUTPUT_WIDTH,
               "height", MUXER_OUTPUT_HEIGHT,
               "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);

  gst_bin_add(GST_BIN(pipeline), streammux);

  for (i = 0; i < num_sources; i++)
  {
    GstPad *sinkpad, *srcpad;
    gchar pad_name[16] = {0};
    GstElement *source_bin = NULL;

    if (is_nvinfer_server)
    {
      source_bin = create_source_bin(i, argv[i + 4]);
    }
    else
    {
      source_bin = create_source_bin(i, argv[i + 2]);
    }
    if (!source_bin)
    {
      g_printerr("Failed to create source bin. Exiting.\n");
      return -1;
    }

    gst_bin_add(GST_BIN(pipeline), source_bin);

    g_snprintf(pad_name, 15, "sink_%u", i);
    sinkpad = gst_element_request_pad_simple(streammux, pad_name);
    if (!sinkpad)
    {
      g_printerr("Streammux request sink pad failed. Exiting.\n");
      return -1;
    }

    srcpad = gst_element_get_static_pad(source_bin, "src");
    if (!srcpad)
    {
      g_printerr("Failed to get src pad of source bin. Exiting.\n");
      return -1;
    }

    if (gst_pad_link(srcpad, sinkpad) != GST_PAD_LINK_OK)
    {
      g_printerr("Failed to link source bin to stream muxer. Exiting.\n");
      return -1;
    }

    gst_object_unref(srcpad);
    gst_object_unref(sinkpad);
  }

  //==========
  // PGIE
  //==========

  /* Use nvinfer to infer on batched frame. */
  CREATE_ELEMENT(pgie, is_nvinfer_server ? NVINFERSERVER_PLUGIN : NVINFER_PLUGIN, "primary-nvinference-engine");

  /* Configure the nvinfer element using the nvinfer config file. */
  g_object_set(G_OBJECT(pgie),
               "config-file-path", infer_config_file,
               "unique-id", 1,
               "infer-on-gie-id", 1,
               "infer-on-class-ids", "0:",
               NULL);

  /* Override the batch-size set in the config file with the number of sources. */
  g_object_get(G_OBJECT(pgie), "batch-size", &pgie_batch_size, NULL);

  if (pgie_batch_size != num_sources && !is_nvinfer_server)
  {
    g_printerr("WARNING: Overriding infer-config batch-size (%d) with number of sources (%d)\n",
               pgie_batch_size, num_sources);
    g_object_set(G_OBJECT(pgie),
                 "batch-size", num_sources, NULL);
  }

  CREATE_ELEMENT(nvsegvisual, "nvsegvisual", "nvsegvisual");
  // https://forums.developer.nvidia.com/t/how-to-draw-masks-with-python/308209/9
  g_object_set(G_OBJECT(nvsegvisual),
               "batch-size", num_sources,
               "width", 512,
               "height", 512,
               "alpha", 0.7,                // [float 0-1] Значение альфа для смешивания по пикселям.
               "class-id", 0,               // [uint](0) Идентификатор класса фона, должен быть установлен, если original-background установлен в TRUE
               "gpu-on", 1,                 // [bool](1) Переключение между памятью устройства и хоста
               "operate-on-seg-meta-id", 1, // [int](-1) Визуализация сегментации на seg-metadata с этим уникальным идентификатором. Установите значение -1 для визуализации всех метаданных.
               "original-background", 1,    // [bool](0) вместо маскированного фона показывать оригинальный фон.
               "qos", 0,                    // [bool](0) обработка событий качества обслуживания
               NULL);

  CREATE_ELEMENT(nvdsosd, "nvdsosd", "onscreendisplay");
  g_object_set(G_OBJECT(nvdsosd),
               "display-clock", 1,
               "display-mask", 1,
               "clock-font", "Serif",
               "clock-font-size", 12,
               //"x-clock-offset", 400,
               //"y-clock-offset", 420,
               "process-mode", 1,
               "qos", 1,
               NULL);

  CREATE_ELEMENT(nvvidconv, "nvvideoconvert", "nvvideo-converter");
  CREATE_ELEMENT(textoverlay, "textoverlay", "textoverlay");
  g_object_set(textoverlay,
               "valignment", 1,        // 1 = top alignment
               "halignment", 1,        // 1 = left alignment
               "font-desc", "Sans 20", // Font description
               NULL);

  CREATE_ELEMENT(tee, "tee", "tee");
  CREATE_ELEMENT(queue1, "queue", "queue1");
  CREATE_ELEMENT(valve, "valve", "valve");
  CREATE_ELEMENT(queue2, "queue", "queue2");
  CREATE_ELEMENT(text_src, "mysrc", "text-source");
  CREATE_ELEMENT(x264enc, "x264enc", "x264enc");
  g_object_set(x264enc, "tune", 4, NULL);

  CREATE_ELEMENT(filesink, "filesink", "sink2");
  // Настройка файлового вывода
  g_object_set(filesink, "location", "output.mp4", NULL);

  //==========
  // SINK
  //==========

  if (cuda_device_prop.integrated)
  {
    CREATE_ELEMENT(sink, "nv3dsink", "nvvideo-renderer");
  }
  else
  {
#ifdef __aarch64__
    CREATE_ELEMENT(sink, "nv3dsink", "nvvideo-renderer");
#else
    CREATE_ELEMENT(sink, "nveglglessink", "nvvideo-renderer");
#endif
  }

  g_object_set(G_OBJECT(sink), "async", FALSE, NULL);

  //-------------------------------------------

  // sink = gst_element_factory_make("udpsink", "sink");

  // if ( !sink) {
  //   g_printerr ("SINK element could not be created. Exiting.\n");
  //   return -1;
  // }

  // // Настройка UDP-сервера
  // g_object_set(G_OBJECT(sink),
  //   "host", "127.0.0.1",  // // Адрес получателя
  //   "port", 5000,  //  // Порт получателя
  //   NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  /* Set up the pipeline */
  /* Add all elements into the pipeline */
  gst_bin_add_many(GST_BIN(pipeline),
                   pgie,
                   nvsegvisual,
                   nvdsosd,
                   nvvidconv,
                   text_src,
                   textoverlay,
                   tee,
                   queue1,
                   valve,
                   queue2,
                   sink,
                   x264enc,
                   filesink, NULL);

#define LINK_ELEMENTS(a, b)                                                                      \
  if (!gst_element_link(a, b))                                                                   \
  {                                                                                              \
    g_printerr("Failed to link %s -> %s. Exiting.\n", GST_ELEMENT_NAME(a), GST_ELEMENT_NAME(b)); \
    return -1;                                                                                   \
  }

  /* Link the elements together with error checking */
  LINK_ELEMENTS(streammux, pgie);
  LINK_ELEMENTS(pgie, nvsegvisual);
  LINK_ELEMENTS(nvsegvisual, nvdsosd);
  LINK_ELEMENTS(nvdsosd, nvvidconv);
  LINK_ELEMENTS(nvvidconv, textoverlay);
  LINK_ELEMENTS(textoverlay, tee);

  LINK_ELEMENTS(tee, queue1);
  LINK_ELEMENTS(queue1, sink);

  LINK_ELEMENTS(tee, queue2);
  LINK_ELEMENTS(queue2, valve);
  LINK_ELEMENTS(valve, x264enc);
  LINK_ELEMENTS(x264enc, filesink);

#undef LINK_ELEMENTS

  // Ручное соединение text_src → textoverlay (текст-вход)
  GstPad *text_src_pad = gst_element_get_static_pad(text_src, "src");
  GstPad *text_sink_pad = gst_element_get_static_pad(textoverlay, "text_sink");

  if (!text_src_pad || !text_sink_pad)
  {
    g_printerr("Не найдены pad'ы для текстового соединения!\n");
    return -1;
  }
  if (gst_pad_link(text_src_pad, text_sink_pad) != GST_PAD_LINK_OK)
  {
    g_printerr("Не удалось соединить текст-источник с textoverlay!\n");
    return -1;
  }
  g_print("Link text_src_pad to textoverlay\n");

  gst_object_unref(text_src_pad);
  gst_object_unref(text_sink_pad);

  /* Lets add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the osd element, since by that time, the buffer would have
   * had got all the metadata. */
  tiler_src_pad = gst_element_get_static_pad(pgie, "src");
  if (!tiler_src_pad)
    g_print("Unable to get src pad\n");
  else
    gst_pad_add_probe(
        tiler_src_pad, GST_PAD_PROBE_TYPE_BUFFER,
        tiler_src_pad_buffer_probe, NULL, NULL);
  gst_object_unref(tiler_src_pad);

  /* Set the pipeline to "playing" state */
  g_print("Now playing...\n");
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  // Исправление передачи аргумента в close_valve
  g_timeout_add_seconds(5, (GSourceFunc)close_valve, valve);

  /* Wait till pipeline encounters an error or EOS */
  g_print("Main loop running...\n");
  g_main_loop_run(loop);

  /* Out of the main loop, clean up nicely */
  g_print("Returned, stopping playback\n");
  gst_element_set_state(pipeline, GST_STATE_NULL);
  g_print("Deleting pipeline\n");
  gst_object_unref(GST_OBJECT(pipeline));
  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);
  return 0;
}
