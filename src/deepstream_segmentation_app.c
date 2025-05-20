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

#include "structs.h"
#include "callback.h"
#include "pipeline.h"

// #define MAPPING "/ridgerun"
// #define SERVICE "12345"

// static gboolean PERF_MODE = FALSE;

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
Usage:\n %s config_file <file1> [file2] ... [fileN]\n\n",
             bin_name);
  g_printerr(" \
For nvinferserver, Usage:\n \
%s -t inferserver config_file <file1> [file2] ... [fileN]\n",
             bin_name);
}

static gboolean
fill_cuda_device_prop()
{
  struct cudaDeviceProp cuda_device_prop;
  int current_device_id = -1;
  cudaError_t cuda_err = cudaGetDevice(&current_device_id);

  if (cuda_err != cudaSuccess || current_device_id == -1)
  {
    g_printerr("CUDA error getting device_id: %s\n", cudaGetErrorString(cuda_err));
    return -1;
  }

  cuda_err = cudaGetDeviceProperties(&cuda_device_prop, current_device_id);
  if (cuda_err != cudaSuccess)
  {
    g_printerr("CUDA don`t get device prop error: %s\n", cudaGetErrorString(cuda_err));
    return -1;
  }
  g_print("Using CUDA device %d: %s\n", current_device_id, cuda_device_prop.name);
  return cuda_device_prop.integrated;
}

static int
check_input_arguments(app_init_context *conf)
{
  if (conf->argc < 3)
  {
    usage(conf->argv[0]);
    return -1;
  }

  if (conf->argc >= 3 && !strcmp("-t", conf->argv[1]))
  {
    if (!strcmp("inferserver", conf->argv[2]))
    {
      conf->is_nvinfer_server = TRUE;
      conf->num_sources = conf->argc - 4;
      conf->infer_config_file = conf->argv[3];
    }
    else
    {
      usage((*conf).argv[0]);
      return -1;
    }
    g_print("Using nvinferserver as the inference plugin\n");
  }

  conf->num_sources = conf->argc - 2;
  conf->infer_config_file = conf->argv[1];

  return 0;
}

int main(int argc, char *argv[])
{
  GMainLoop *loop = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id;

  app_init_context app_conf = {
      .argc = argc,
      .argv = argv,
      .is_cuda_device_integrated = fill_cuda_device_prop(),
      .is_nvinfer_server = FALSE,
      .infer_config_file = NULL,
      .num_sources = 0,
      .output_filename = "output.mp4"};

  if (check_input_arguments(&app_conf) != 0)
  {
    return -1;
  }

  /* Standard GStreamer initialization */
  gst_init(&argc, &argv);
  loop = g_main_loop_new(NULL, FALSE);

  PipelineComponents *pipeline_context = build_pipeline(&app_conf);

  if (!pipeline_context)
  {
    g_printerr("Failed to create pipeline\n");
    return -1;
  }

  /* we add a message handler */
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_context->pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  // Запуск pipeline
  /* Set the pipeline to "playing" state */
  g_print("Now playing...\n");
  gst_element_set_state(pipeline_context->pipeline, GST_STATE_PLAYING);

  // Исправление передачи аргумента в close_valve
  g_timeout_add_seconds(5, (GSourceFunc)close_valve, pipeline_context->valve);

  /* Wait till pipeline encounters an error or EOS */
  g_print("Main loop running...\n");
  g_main_loop_run(loop);

  // Очистка
  free_pipeline(pipeline_context);

  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);
  return 0;
}

/* tiler_sink_pad_buffer_probe  will extract metadata received
on segmentation  src pad */

// static GstPadProbeReturn
// tiler_src_pad_buffer_probe(
//     GstPad *pad G_GNUC_UNUSED,
//     GstPadProbeInfo *info,
//     gpointer u_data G_GNUC_UNUSED)
// {
//   GstBuffer *buf = (GstBuffer *)info->data;
//   NvDsMetaList *l_frame = NULL;
//   NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

//   for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
//        l_frame = l_frame->next)
//   {
//     // TODO:
//   }
//   return GST_PAD_PROBE_OK;
// }

//-------------------------------------------

// GstPad *tiler_src_pad = NULL;
/* Lets add probe to get informed of the meta data generated, we add probe to
 * the sink pad of the osd element, since by that time, the buffer would have
 * had got all the metadata. */
// tiler_src_pad = gst_element_get_static_pad(pgie, "src");
// if (!tiler_src_pad)
// g_print("Unable to get src pad\n");
// else
// gst_pad_add_probe(
//   tiler_src_pad, GST_PAD_PROBE_TYPE_BUFFER,
//   tiler_src_pad_buffer_probe, NULL, NULL);
//   gst_object_unref(tiler_src_pad);

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
