#include "pipeline.h"
#include <string.h>
#include "callback.h"

#define CREATE_ELEMENT(var, factory, name)                    \
    do                                                        \
    {                                                         \
        var = gst_element_factory_make(factory, name);        \
        if (!var)                                             \
        {                                                     \
            g_printerr("ERROR: Failed to create %s\n", name); \
            return NULL;                                      \
        }                                                     \
    } while (0)

// Макросы для упрощения проверки ошибок
#define RETURN_IF_ERROR(cond, msg, retval)  \
    do                                      \
    {                                       \
        if (cond)                           \
        {                                   \
            g_printerr("ERROR: %s\n", msg); \
            return retval;                  \
        }                                   \
    } while (0)

// ================= Реализация =================
PipelineComponents *build_pipeline(app_init_context *app_conf)
{
    RETURN_IF_ERROR(!app_conf, "ERROR: app_conf is NULL\n", NULL);

    guint pgie_batch_size;
    guint i;

    // 0. Выделить память под структуру
    PipelineComponents *comp = g_new0(PipelineComponents, 1);
    RETURN_IF_ERROR(!comp, "Failed to allocate PipelineComponents\n", NULL);

    // 1. Создаём основной pipeline
    comp->pipeline = gst_pipeline_new("smoke-segmentation-pipeline");
    RETURN_IF_ERROR(!comp->pipeline, "Failed to create pipeline", NULL);

    // 2. Создаём streammux (объединяет несколько потоков)
    CREATE_ELEMENT(comp->streammux, "nvstreammux", "stream-muxer");
    g_object_set(G_OBJECT(comp->streammux),
                 "batch-size", app_conf->num_sources,
                 "width", MUXER_OUTPUT_WIDTH,
                 "height", MUXER_OUTPUT_HEIGHT,
                 "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC,
                 NULL);

    gst_bin_add(GST_BIN(comp->pipeline), comp->streammux);

    for (i = 0; i < app_conf->num_sources; i++)
    {
        GstPad *sinkpad, *srcpad;
        gchar pad_name[16] = {0};
        GstElement *source_bin = NULL;

        if (app_conf->is_nvinfer_server)
        {
            source_bin = create_source_bin(i, app_conf->argv[i + 4]);
        }
        else
        {
            source_bin = create_source_bin(i, app_conf->argv[i + 2]);
        }
        if (!source_bin)
        {
            g_printerr("Failed to create source bin. Exiting.\n");
            return NULL;
        }

        gst_bin_add(GST_BIN(comp->pipeline), source_bin);

        g_snprintf(pad_name, 15, "sink_%u", i);
        sinkpad = gst_element_request_pad_simple(comp->streammux, pad_name);
        if (!sinkpad)
        {
            g_printerr("Streammux request sink pad failed. Exiting.\n");
            return NULL;
        }

        srcpad = gst_element_get_static_pad(source_bin, "src");
        if (!srcpad)
        {
            g_printerr("Failed to get src pad of source bin. Exiting.\n");
            return NULL;
        }

        if (gst_pad_link(srcpad, sinkpad) != GST_PAD_LINK_OK)
        {
            g_printerr("Failed to link source bin to stream muxer. Exiting.\n");
            return NULL;
        }

        gst_object_unref(srcpad);
        gst_object_unref(sinkpad);
    }

    // 3. Создаём элемент для инференса (nvinfer или nvinferserver)
    const gchar *plugin_name = app_conf->is_nvinfer_server ? "nvinferserver" : "nvinfer";
    CREATE_ELEMENT(comp->pgie, plugin_name, "primary-nvinference-engine");
    g_object_set(G_OBJECT(comp->pgie),
                 "config-file-path", app_conf->infer_config_file,
                 "unique-id", 1,
                 "infer-on-gie-id", 1,
                 "infer-on-class-ids", "0:",
                 NULL);

    /* Override the batch-size set in the config file with the number of sources. */
    g_object_get(G_OBJECT(comp->pgie), "batch-size", &pgie_batch_size, NULL);

    if (pgie_batch_size != app_conf->num_sources && !app_conf->is_nvinfer_server)
    {
        g_printerr("WARNING: Overriding infer-config batch-size (%d) with number of sources (%d)\n",
                   pgie_batch_size, app_conf->num_sources);
        g_object_set(G_OBJECT(comp->pgie),
                     "batch-size", app_conf->num_sources, NULL);
    }

    // 4. Визуализация сегментации https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvsegvisual.html
    // https://forums.developer.nvidia.com/t/how-to-draw-masks-with-python/308209/9
    CREATE_ELEMENT(comp->nvsegvisual, "nvsegvisual", "nvsegvisual");
    g_object_set(G_OBJECT(comp->nvsegvisual),
                 "batch-size", app_conf->num_sources,
                 "width", 512,
                 "height", 512,
                 "alpha", 0.7,                // [float 0-1] Значение альфа для смешивания по пикселям.
                 "class-id", 0,               // [uint](0) Идентификатор класса фона, должен быть установлен, если original-background установлен в TRUE
                 "gpu-on", 1,                 // [bool](1) Переключение между памятью устройства и хоста
                 "operate-on-seg-meta-id", 1, // [int](-1) Визуализация сегментации на seg-metadata с этим уникальным идентификатором. Установите значение -1 для визуализации всех метаданных.
                 "original-background", 1,    // [bool](0) вместо маскированного фона показывать оригинальный фон.
                 "qos", 0,                    // [bool](0) обработка событий качества обслуживания
                 NULL);

    // 5. Наложение метаданных (bounding boxes, текст) https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvdsosd.html
    CREATE_ELEMENT(comp->nvdsosd, "nvdsosd", "nvdsosd");
    g_object_set(G_OBJECT(comp->nvdsosd),
                 "display-clock", 1,
                 "display-mask", 1,
                 "clock-font", "Serif",
                 "clock-font-size", 12,
                 //"x-clock-offset", 400,
                 //"y-clock-offset", 420,
                 "process-mode", 1,
                 "qos", 1,
                 NULL);

    CREATE_ELEMENT(comp->nvvidconv, "nvvideoconvert", "nvvideo-converter");
    CREATE_ELEMENT(comp->textoverlay, "textoverlay", "textoverlay");
    g_object_set(G_OBJECT(comp->textoverlay),
                 "valignment", 1,        // 1 = top alignment
                 "halignment", 1,        // 1 = left alignment
                 "font-desc", "Sans 20", // Font description
                 NULL);

    CREATE_ELEMENT(comp->tee, "tee", "tee");
    CREATE_ELEMENT(comp->queue1, "queue", "queue1");
    CREATE_ELEMENT(comp->valve, "valve", "valve");
    CREATE_ELEMENT(comp->queue2, "queue", "queue2");
    CREATE_ELEMENT(comp->text_src, "mysrc", "text-source");
    CREATE_ELEMENT(comp->x264enc, "x264enc", "x264enc");
    g_object_set(G_OBJECT(comp->x264enc), "tune", 4, NULL);

    //==========
    // SINK
    //==========
    CREATE_ELEMENT(comp->filesink, "filesink", "sink2");
    g_object_set(comp->filesink, "location", app_conf->output_filename, NULL);

    if (app_conf->is_cuda_device_integrated)
    {
        CREATE_ELEMENT(comp->sink, "nv3dsink", "nvvideo-renderer");
    }
    else
    {
#ifdef __aarch64__
        CREATE_ELEMENT(comp->sink, "nv3dsink", "nvvideo-renderer");
#else
        CREATE_ELEMENT(comp->sink, "nveglglessink", "nvvideo-renderer");
#endif
    }

    g_object_set(G_OBJECT(comp->sink), "async", FALSE, NULL);

    // 7. Добавляем все элементы в pipeline
    /* Set up the pipeline */
    /* Add all elements into the pipeline */
    gst_bin_add_many(GST_BIN(comp->pipeline),
                     comp->pgie,
                     comp->nvsegvisual,
                     comp->nvdsosd,
                     comp->nvvidconv,
                     comp->text_src,
                     comp->textoverlay,
                     comp->tee,
                     comp->queue1,
                     comp->valve,
                     comp->queue2,
                     comp->sink,
                     comp->x264enc,
                     comp->filesink, NULL);

    // 8. Связываем элементы

#define LINK_ELEMENTS(src, dest)                                                                          \
    if (!gst_element_link(src, dest))                                                                     \
    {                                                                                                     \
        g_printerr("Failed to link %s -> %s. Exiting.\n", GST_ELEMENT_NAME(src), GST_ELEMENT_NAME(dest)); \
        free_pipeline(comp);                                                                              \
        return NULL;                                                                                      \
    }

    /* Link the elements together with error checking */
    LINK_ELEMENTS(comp->streammux, comp->pgie);
    LINK_ELEMENTS(comp->pgie, comp->nvsegvisual);
    LINK_ELEMENTS(comp->nvsegvisual, comp->nvdsosd);
    LINK_ELEMENTS(comp->nvdsosd, comp->nvvidconv);
    LINK_ELEMENTS(comp->nvvidconv, comp->textoverlay);
    LINK_ELEMENTS(comp->textoverlay, comp->tee);

    LINK_ELEMENTS(comp->tee, comp->queue1);
    LINK_ELEMENTS(comp->queue1, comp->sink);

    LINK_ELEMENTS(comp->tee, comp->queue2);
    LINK_ELEMENTS(comp->queue2, comp->valve);
    LINK_ELEMENTS(comp->valve, comp->x264enc);
    LINK_ELEMENTS(comp->x264enc, comp->filesink);

#undef LINK_ELEMENTS

    // Ручное соединение text_src → textoverlay (текст-вход)
    GstPad *text_src_pad = gst_element_get_static_pad(comp->text_src, "src");
    GstPad *text_sink_pad = gst_element_get_static_pad(comp->textoverlay, "text_sink");

    if (!text_src_pad || !text_sink_pad)
    {
        g_printerr("Не найдены pad'ы для текстового соединения!\n");
        return NULL;
    }
    if (gst_pad_link(text_src_pad, text_sink_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("Не удалось соединить текст-источник с textoverlay!\n");
        return NULL;
    }
    g_print("Link text_src_pad to textoverlay\n");

    gst_object_unref(text_src_pad);
    gst_object_unref(text_sink_pad);

    return comp;
}

// ================= Вспомогательные функции =================

void free_pipeline(PipelineComponents *comp)
{
    if (!comp)
        return;

    if (comp->pipeline)
    {
        /* Out of the main loop, clean up nicely */
        g_print("Returned, stopping playback\n");
        gst_element_set_state(comp->pipeline, GST_STATE_NULL);
        g_print("Deleting pipeline\n");
        gst_object_unref(comp->pipeline);
    }

    g_free(comp);
}

GstElement *create_source_bin(guint index, const gchar *uri)
{
    GstElement *bin = NULL;
    GstElement *uri_decode_bin = NULL;
    gchar bin_name[16] = {0};

    g_snprintf(bin_name, 15, "source-bin-%02d", index);
    /* Create a source GstBin to abstract this bin's content from the rest of the
     * pipeline */
    bin = gst_bin_new(bin_name);
    RETURN_IF_ERROR(!bin, "element BIN in source bin could not be created.\n", NULL);

    /* Source element for reading from the uri.
     * We will use decodebin and let it figure out the container format of the
     * stream and the codec and plug the appropriate demux and decode plugins. */
    uri_decode_bin = gst_element_factory_make("uridecodebin", "uri-decode-bin");
    CREATE_ELEMENT(uri_decode_bin, "uridecodebin", "uri-decode-bin");
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
        gst_object_unref(bin);
        return NULL;
    }

    return bin;
}