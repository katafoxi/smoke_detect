#include <jansson.h>
#include <string.h>
#include "app_conf_parser.h" // Подключаем файл с определением структуры

#define CONFIG_GET_INT(json, field, target) \
    do { \
        json_t *json_##field = json_object_get(json, #field); \
        if (json_##field && json_is_integer(json_##field)) { \
            target = json_integer_value(json_##field); \
        } \
    } while (0)

#define CONFIG_GET_FLOAT(json, field, target) \
    do { \
        json_t *json_##field = json_object_get(json, #field); \
        if (json_##field && json_is_real(json_##field)) { \
            target = json_real_value(json_##field); \
        } \
    } while (0)

#define CONFIG_GET_BOOL(json, field, target) \
    do { \
        json_t *json_##field = json_object_get(json, #field); \
        if (json_##field && json_is_boolean(json_##field)) { \
            target = json_boolean_value(json_##field); \
        } \
    } while (0)

#define CONFIG_GET_STRING(json, field, target) \
    do { \
        json_t *json_##field = json_object_get(json, #field); \
        if (json_##field && json_is_string(json_##field)) { \
            target = g_strdup(json_string_value(json_##field)); \
        } \
    } while (0)

app_init_context* parse_config_file(const char* config_path) {
    json_t *root;
    json_error_t error;
    
    // 1. Загрузка и парсинг JSON файла
    root = json_load_file(config_path, 0, &error);
    if (!root) {
        g_printerr("ERROR: Failed to parse JSON config (%s) at line %d: %s\n",
                  config_path, error.line, error.text);
        return NULL;
    }

    // 2. Выделение памяти под структуру
    app_init_context *config = g_new0(app_init_context, 1);
    if (!config) {
        g_printerr("ERROR: Failed to allocate memory for config\n");
        json_decref(root);
        return NULL;
    }

    // 3. Парсинг основных параметров
    CONFIG_GET_BOOL(root, is_cuda_device_integrated, config->is_cuda_device_integrated);
    CONFIG_GET_BOOL(root, is_nvinfer_server, config->is_nvinfer_server);
    CONFIG_GET_STRING(root, infer_config_file, config->infer_config_file);
    CONFIG_GET_STRING(root, output_filename, config->output_filename);
    
    // 4. Парсинг параметров muxer
    json_t *muxer = json_object_get(root, "muxer");
    if (muxer) {
        CONFIG_GET_INT(muxer, output_width, config->muxer_output_width);
        CONFIG_GET_INT(muxer, output_height, config->muxer_output_height);
        CONFIG_GET_INT(muxer, batch_timeout_usec, config->muxer_batch_timeout_usec);
    }

    // 5. Парсинг параметров nvsegvisual
    json_t *nvsegvisual = json_object_get(root, "nvsegvisual");
    if (nvsegvisual) {
        CONFIG_GET_INT(nvsegvisual, width, config->nvsegvisual_width);
        CONFIG_GET_INT(nvsegvisual, height, config->nvsegvisual_heidght);
        CONFIG_GET_FLOAT(nvsegvisual, alpha, config->nvsegvisual_alpha);
    }

    // 6. Парсинг параметров nvdosd
    json_t *nvdosd = json_object_get(root, "nvdosd");
    if (nvdosd) {
        CONFIG_GET_INT(nvdosd, clock_font_size, config->nvdosd_clock_font_size);
    }

    // 7. Парсинг параметров valve
    json_t *valve = json_object_get(root, "valve");
    if (valve) {
        CONFIG_GET_INT(valve, video_duration_sec, config->valve_video_duration_sec);
    }

    // 8. Парсинг источников (sources)
    json_t *sources = json_object_get(root, "sources");
    if (sources && json_is_array(sources)) {
        config->num_sources = json_array_size(sources);
        config->sources = g_new0(char*, config->num_sources+1); // +1 для NULL в конце
        
        // Заполняем остальные аргументы
        for (size_t i = 0; i < config->num_sources; i++) {
            json_t *source = json_array_get(sources, i);
            if (json_is_string(source)) {
                config->sources[i] = g_strdup(json_string_value(source));
            }
        }
    }
    else
        g_printerr("ERROR: В конфиге приложения в секции 'sources' ожидается массив строк.");

    json_decref(root);
    return config;
}

void free_app_init_context(app_init_context *config) {
    if (!config) return;
    
    // Освобождаем строки
    g_free(config->infer_config_file);
    g_free(config->output_filename);
    
    // Освобождаем argv
    if (config->sources) {
        for (guint i = 0; i < config->num_sources; i++) {
            g_free(config->sources[i]);
        }
        g_free(config->sources);
    }
    
    g_free(config);
}