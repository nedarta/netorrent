#ifndef LIBNETORRENT_H
#define LIBNETORRENT_H

#include <stdint.h>

#define LIBNETORRENT_API_VERSION 1
#define LIBNETORRENT_INIT libnetorrent_init
#define LIBNETORRENT_STRINGIFY(x) #x
#define LIBNETORRENT_TOSTRING(x) LIBNETORRENT_STRINGIFY(x)

#define LIBNETORRENT_DEFINE_PLUGIN(name,version,init_func) \
    extern "C" __declspec(dllexport) libnetorrent_result_t LIBNETORRENT_INIT(int v, libnetorrent_plugin_t* a) \
    { \
        return init_func(v,a); \
    }

#if defined(LIBNETORRENT_API_EXPORT)
#define LIBNETORRENT_API_FUNCTION \
    extern "C" __declspec(dllexport)
#else
#define LIBNETORRENT_API_FUNCTION \
    extern "C" __declspec(dllimport)
#endif

typedef struct libnetorrent_config_t libnetorrent_config_t;
typedef struct libnetorrent_http_response_t libnetorrent_http_response_t;
typedef struct libnetorrent_mainwnd_t libnetorrent_mainwnd_t;
typedef struct libnetorrent_menu_t libnetorrent_menu_t;
typedef struct libnetorrent_menuitem_t libnetorrent_menuitem_t;
typedef struct libnetorrent_param_t libnetorrent_param_t;
typedef struct libnetorrent_plugin_t libnetorrent_plugin_t;
typedef struct libnetorrent_torrent_t libnetorrent_torrent_t;

enum libnetorrent_event_t
{
    libnetorrent_event_mainwnd_created,
    libnetorrent_event_shutdown
};

enum libnetorrent_http_status_t
{
    libnetorrent_http_ok = 200,
    libnetorrent_http_not_found = 404,
    libnetorrent_http_internal_server_error = 500
};

enum libnetorrent_menu_id_t
{
    libnetorrent_menu_file,
    libnetorrent_menu_view,
    libnetorrent_menu_help
};

enum libnetorrent_result_t
{
    libnetorrent_ok,
    libnetorrent_err,
    libnetorrent_version_mismatch,
    libnetorrent_insufficient_buffer
};

typedef struct libnetorrent_torrent_stats_t
{
    int32_t download_payload_rate;
    int32_t upload_payload_rate;
} libnetorrent_torrent_stats_t;

typedef libnetorrent_result_t(*libnetorrent_http_callback_t)(libnetorrent_http_response_t*, libnetorrent_http_status_t, libnetorrent_param_t*);
typedef libnetorrent_result_t(*libnetorrent_init_t)(int, libnetorrent_plugin_t*);
typedef libnetorrent_result_t(*libnetorrent_hook_callback_t)(libnetorrent_event_t, libnetorrent_param_t*, libnetorrent_param_t*);
typedef libnetorrent_result_t(*libnetorrent_menuitem_callback_t)(libnetorrent_menuitem_t*, libnetorrent_param_t*);

/*
Config.
*/
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_config_get(libnetorrent_plugin_t* plugin, libnetorrent_config_t** config);
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_config_bool_get(libnetorrent_config_t* config, const char* key, bool* result);
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_config_string_get(libnetorrent_config_t* config, const char* key, char* result, size_t* len);
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_config_string_set(libnetorrent_config_t* config, const char* key, const char* value, size_t len);

/*
HTTP.
*/
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_http_get(const char* url, libnetorrent_http_callback_t cb, libnetorrent_param_t * user);
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_http_response_body(libnetorrent_http_response_t* response, char* body, size_t len);
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_http_response_body_len(libnetorrent_http_response_t* response, size_t* len);

LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_i18n(const char* key, wchar_t* target, size_t* len);

LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_mainwnd_native_handle(libnetorrent_mainwnd_t* wnd, void** handle);

/*
Menu.
*/
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_menu_get(libnetorrent_mainwnd_t* wnd, libnetorrent_menu_id_t id, libnetorrent_menu_t** menu);
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_menu_insert_item(libnetorrent_menu_t* menu, uint32_t pos, const wchar_t* label, size_t len, libnetorrent_menuitem_callback_t cb, libnetorrent_param_t* param, libnetorrent_menuitem_t** item);
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_menu_insert_separator(libnetorrent_menu_t* menu, uint32_t pos);

/*
String functions
*/
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_string_towide(const char* input, wchar_t* output, size_t len);

LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_register_hook(libnetorrent_plugin_t* app, libnetorrent_hook_callback_t cb, libnetorrent_param_t* user);
LIBNETORRENT_API_FUNCTION libnetorrent_result_t libnetorrent_torrent_stats_get(libnetorrent_torrent_t* torrent, libnetorrent_torrent_stats_t* stats);

LIBNETORRENT_API_FUNCTION const char* libnetorrent_version();
#endif
