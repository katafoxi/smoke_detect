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

#define MAPPING "/ridgerun"
#define SERVICE "12345"

static gboolean PERF_MODE = FALSE;

/* tiler_sink_pad_buffer_probe  will extract metadata received on
   segmentation  src pad */
static GstPadProbeReturn
tiler_src_pad_buffer_probe(
    GstPad *pad,
    GstPadProbeInfo *info,
    gpointer u_data)
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

static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *)data;
  switch (GST_MESSAGE_TYPE(msg))
  {
  case GST_MESSAGE_EOS:
    g_print("End of stream\n");
    // Add the delay to show the result
    usleep(2000000);
    g_main_loop_quit(loop);
    break;
  case GST_MESSAGE_WARNING:
  {
    gchar *debug;
    GError *error;
    gst_message_parse_warning(msg, &error, &debug);
    g_printerr("WARNING from element %s: %s\n",
               GST_OBJECT_NAME(msg->src), error->message);
    g_free(debug);
    g_printerr("Warning: %s\n", error->message);
    g_error_free(error);
    break;
  }
  case GST_MESSAGE_ERROR:
  {
    gchar *debug;
    GError *error;
    gst_message_parse_error(msg, &error, &debug);
    g_printerr("ERROR from element %s: %s\n",
               GST_OBJECT_NAME(msg->src), error->message);
    if (debug)
      g_printerr("Error details: %s\n", debug);
    g_free(debug);
    g_error_free(error);
    g_main_loop_quit(loop);
    break;
  }
  case GST_MESSAGE_ELEMENT:
  {
    if (gst_nvmessage_is_stream_eos(msg))
    {
      guint stream_id;
      if (gst_nvmessage_parse_stream_eos(msg, &stream_id))
      {
        g_print("Got EOS from stream %d\n", stream_id);
      }
    }
    break;
  }
  default:
    break;
  }
  return TRUE;
}

static void
cb_newpad(
    GstElement *decodebin,
    GstPad *decoder_src_pad,
    gpointer data)
{

  GstCaps *caps = gst_pad_get_current_caps(decoder_src_pad);
  if (!caps)
  {
    caps = gst_pad_query_caps(decoder_src_pad, NULL);
  }
  const GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *name = gst_structure_get_name(str);
  GstElement *source_bin = (GstElement *)data;
  GstCapsFeatures *features = gst_caps_get_features(caps, 0);

  /* Need to check if the pad created by the decodebin is for video and not
   * audio. */
  if (!strncmp(name, "video", 5))
  {
    /* Link the decodebin pad only if decodebin has picked nvidia
     * decoder plugin nvdec_*. We do this by checking if the pad caps contain
     * NVMM memory features. */
    if (gst_caps_features_contains(features, GST_CAPS_FEATURES_NVMM))
    {
      /* Get the source bin ghost pad */
      GstPad *bin_ghost_pad = gst_element_get_static_pad(source_bin, "src");
      if (!gst_ghost_pad_set_target(GST_GHOST_PAD(bin_ghost_pad),
                                    decoder_src_pad))
      {
        g_printerr("Failed to link decoder src pad to source bin ghost pad\n");
      }
      gst_object_unref(bin_ghost_pad);
    }
    else
    {
      g_printerr("Error: Decodebin did not pick nvidia decoder plugin.\n");
    }
  }
}

static GstElement *
create_source_bin(guint index, gchar *uri)
{
  GstElement
      *bin = NULL,
      *uri_decode_bin = NULL;
  gchar bin_name[16] = {};

  int current_device = -1;
  cudaGetDevice(&current_device);
  struct cudaDeviceProp prop;
  cudaGetDeviceProperties(&prop, current_device);

  g_snprintf(bin_name, 15, "source-bin-%02d", index);
  /* Create a source GstBin to abstract this bin's content from the rest of the
   * pipeline */
  bin = gst_bin_new(bin_name);

  /* Source element for reading from the uri.
   * We will use decodebin and let it figure out the container format of the
   * stream and the codec and plug the appropriate demux and decode plugins. */
  uri_decode_bin = gst_element_factory_make("uridecodebin", "uri-decode-bin");
  if (!bin || !uri_decode_bin)
  {
    g_printerr("One element in source bin could not be created.\n");
    return NULL;
  }
  g_object_set(G_OBJECT(uri_decode_bin),
               "uri", uri, NULL);
  g_signal_connect(
      G_OBJECT(uri_decode_bin),
      "pad-added",
      G_CALLBACK(cb_newpad),
      bin);
  // add uri_decode_bin to common bin
  gst_bin_add(GST_BIN(bin),
              uri_decode_bin);

  /* We need to create a ghost pad for the source bin which will act as a proxy
   * for the video decoder src pad. The ghost pad will not have a target right
   * now. Once the decode bin creates the video decoder and generates the
   * cb_newpad callback, we will set the ghost pad target to the video decoder
   * src pad. */
  if (!gst_element_add_pad(bin, gst_ghost_pad_new_no_target("src",
                                                            GST_PAD_SRC)))
  {
    g_printerr("Failed to add ghost pad in source bin\n");
    return NULL;
  }

  return bin;
}

static void
usage(const char *bin)
{
  g_printerr("Usage: %s config_file <file1> [file2] ... [fileN]\n", bin);
  g_printerr("For nvinferserver, Usage: %s -t inferserver config_file <file1> [file2] ... [fileN]\n", bin);
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
      *filesink = NULL,
      *sink = NULL;

  GstBus *bus = NULL;
  guint bus_watch_id;
  GstPad *tiler_src_pad = NULL;
  guint i, num_sources = 0;
  guint tiler_rows, tiler_columns;
  guint pgie_batch_size;
  gboolean is_nvinfer_server = FALSE;
  gchar *infer_config_file = NULL;

  int current_device = -1;
  cudaGetDevice(&current_device);
  struct cudaDeviceProp prop;
  cudaGetDeviceProperties(&prop, current_device);

  //===========================================================================
  /* Check input arguments */
  //===========================================================================
  if (argc < 3)
  {
    usage(argv[0]);
    return -1;
  }

  if (argc >= 3 && !strcmp("-t", argv[1]))
  {
    if (!strcmp("inferserver", argv[2]))
    {
      is_nvinfer_server = TRUE;
    }
    else
    {
      usage(argv[0]);
      return -1;
    }
    g_print("Using nvinferserver as the inference plugin\n");
  }

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

  //==========
  // Streammux
  //==========
  /* Create nvstreammux instance to form batches from one or more sources. */
  streammux = gst_element_factory_make("nvstreammux", "stream-muxer");

  if (!pipeline || !streammux)
  {
    g_printerr("Streammux element could not be created. Exiting.\n");
    return -1;
  }

  g_object_set(G_OBJECT(streammux),
               "batch-size", num_sources, NULL);
  g_object_set(G_OBJECT(streammux),
               "width", MUXER_OUTPUT_WIDTH,
               "height", MUXER_OUTPUT_HEIGHT,
               "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);

  gst_bin_add(GST_BIN(pipeline), streammux);

  for (i = 0; i < num_sources; i++)
  {
    GstPad *sinkpad, *srcpad;
    gchar pad_name[16] = {};
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
  pgie = gst_element_factory_make(
      is_nvinfer_server ? NVINFERSERVER_PLUGIN : NVINFER_PLUGIN,
      "primary-nvinference-engine");

  /* Configure the nvinfer element using the nvinfer config file. */
  g_object_set(G_OBJECT(pgie),
               "config-file-path", infer_config_file,
               "unique-id", 1,
               "infer-on-gie-id", 1,
               "infer-on-class-ids", "0:",
               NULL);

  if (!pgie)
  {
    g_printerr("PGIE could not be created. Exiting.\n");
    return -1;
  }

  /* Override the batch-size set in the config file with the number of sources. */
  g_object_get(G_OBJECT(pgie),
               "batch-size", &pgie_batch_size, NULL);

  if (pgie_batch_size != num_sources && !is_nvinfer_server)
  {
    g_printerr("WARNING: Overriding infer-config batch-size (%d) with number of sources (%d)\n",
               pgie_batch_size, num_sources);
    g_object_set(G_OBJECT(pgie),
                 "batch-size", num_sources, NULL);
  }

  //==========
  // NVSEGVISUAL
  //==========
  nvsegvisual = gst_element_factory_make("nvsegvisual", "nvsegvisual");
  if (!nvsegvisual)
  {
    g_printerr("NVSEGVISUAL element could not be created. Exiting.\n");
    return -1;
  }

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

  //==========
  // NVDSOSD
  //==========

  nvdsosd = gst_element_factory_make("nvdsosd", "onscreendisplay");
  if (!nvdsosd)
  {
    g_printerr("NVDSOSD element could not be created. Exiting.\n");
    return -1;
  }
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

  //==========
  // NVVIDCONV
  //==========
  /* Use convertor to convert to appropriate format */
  nvvidconv = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");
  // g_object_set(G_OBJECT(nvvidconv),
  // "video/x-raw,format=NV12",
  //   NULL);

  if (!nvvidconv)
  {
    g_printerr("One element could not be created. Exiting.\n");
    return -1;
  }

  //==========
  // TEXTOVERLAY
  //==========

  textoverlay = gst_element_factory_make("textoverlay", "textoverlay");
  if (!textoverlay)
  {
    g_printerr("TEXTOVERLAY element could not be created. Exiting.\n");
    return -1;
  }
  g_object_set(textoverlay,
               "valignment", 1,        // 1 = top alignment
               "halignment", 1,        // 1 = left alignment
               "font-desc", "Sans 20", // Font description
               NULL);

  //==========
  // TEE
  //==========
  tee = gst_element_factory_make("tee", "tee");
  if (!tee)
  {
    g_printerr("TEE element could not be created. Exiting.\n");
    return -1;
  }

  //==========
  // QUEUE
  //==========
  queue1 = gst_element_factory_make("queue", "queue1");
  if (!queue1)
  {
    g_printerr("queue1 element could not be created. Exiting.\n");
    return -1;
  }

  //==========
  // VALVE
  //==========
  valve = gst_element_factory_make("valve", "valve");
  if (!valve)
  {
    g_printerr("valve element could not be created. Exiting. \n");
    return -1;
  }

  //==========
  // SRTSRC mysrc plugin
  //==========

  text_src = gst_element_factory_make("mysrc", "text-source");
  if (!text_src)
  {
    g_printerr("text_src element could not be created. Exiting.\n");
    return -1;
  }

  //==========
  // SINK
  //==========

  if (prop.integrated)
  {
    sink = gst_element_factory_make("nv3dsink", "nvvideo-renderer");
  }
  else
  {
#ifdef __aarch64__
    sink = gst_element_factory_make("nv3dsink", "nvvideo-renderer");
#else
    sink = gst_element_factory_make("nveglglessink", "nvvideo-renderer");
#endif
  }

  g_object_set(G_OBJECT(sink),
               "async", FALSE,
               NULL);

  //==========
  // FILESINK
  //==========

  filesink = gst_element_factory_make("filesink", "sink2");
  if (!filesink)
  {
    g_printerr("FILESINK element could not be created. Exiting.\n");
    return -1;
  }
  // Настройка файлового вывода
  g_object_set(filesink, "location", "output.mp4", NULL);

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
                   queue1,
                   sink, NULL);

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
  LINK_ELEMENTS(textoverlay, queue1);
  LINK_ELEMENTS(queue1, sink);

#undef LINK_ELEMENTS

  // Ручное соединение text_src → textoverlay (текст-вход)
  GstPad *text_src_pad = gst_element_get_static_pad(text_src, "src");
  GstPad *text_sink_pad = gst_element_get_static_pad(textoverlay, "text_sink");

  if (!text_src_pad || !text_sink_pad)
  {
    g_printerr("Не найдены pad'ы для текстового соединения!\n");
    gst_object_unref(pipeline);
    return -1;
  }
  if (gst_pad_link(text_src_pad, text_sink_pad) != GST_PAD_LINK_OK)
  {
    g_printerr("Не удалось соединить текст-источник с textoverlay!\n");
    gst_object_unref(pipeline);
    return -1;
  }

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

  /* Wait till pipeline encounters an error or EOS */
  g_print("Running...\n");
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
