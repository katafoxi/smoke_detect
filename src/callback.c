#include "callback.h"
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


// Улучшенная функция задержки
static void msleep(unsigned int ms) {
  struct timespec ts = {
      .tv_sec = ms / 1000,
      .tv_nsec = (ms % 1000) * 1000000
  };
  nanosleep(&ts, NULL);
}

gboolean
bus_call(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *)data;
  switch (GST_MESSAGE_TYPE(msg))
  {
  case GST_MESSAGE_EOS:
    g_print("End of stream. Exiting\n");
    // Add the delay to show the result
    msleep(2000);
    g_main_loop_quit(loop);
    break;
  case GST_MESSAGE_WARNING:
  {
    gchar *debug;
    GError *error;
    gst_message_parse_warning(msg, &error, &debug);
    g_printerr("WARNING from element %s: %s\n", GST_OBJECT_NAME(msg->src), error->message);
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
        g_print("Got EOS from stream %d. Stopping.\n", stream_id);
        msleep(2000);
        g_main_loop_quit(loop);
      }
    }
    break;
  }
  default:
    break;
  }
  return TRUE;
}


void
cb_newpad(
    GstElement *decodebin G_GNUC_UNUSED,
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

  if (!strncmp(name, "video", 5))
  {
    if (gst_caps_features_contains(features, GST_CAPS_FEATURES_NVMM))
    {
      GstPad *bin_ghost_pad = gst_element_get_static_pad(source_bin, "src");
      if (!bin_ghost_pad)
      {
        g_printerr("Failed to get ghost pad\n");
        gst_caps_unref(caps);
        return;
      }

      if (!gst_ghost_pad_set_target(GST_GHOST_PAD(bin_ghost_pad), decoder_src_pad))
      {
        g_printerr("Failed to link decoder src pad\n");
      }
      gst_object_unref(bin_ghost_pad);
    }
    else
    {
      g_printerr("Error: NVMM feature not found\n");
    }
  }
  gst_caps_unref(caps); // Устранение утечки
}
