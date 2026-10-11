#include "libnetorrent_impl.hpp"

#include <Windows.h>

#include <vector>

#include "../bittorrent/torrenthandle.hpp"
#include "../bittorrent/torrentstatus.hpp"
#include "../buildinfo.hpp"
#include "../core/configuration.hpp"
#include "../core/environment.hpp"
#include "../core/utils.hpp"
#include "../http/httpclient.hpp"
#include "../ui/mainframe.hpp"
#include "../ui/translator.hpp"

using pt::API::IPlugin;

struct libnetorrent_http_response_t
{
    std::string data;
};

class Plugin : public IPlugin
{
public:
    Plugin(HMODULE hModule, pt::Core::Environment* env, pt::Core::Configuration* cfg)
        : m_hModule(hModule),
        m_env(env),
        m_cfg(cfg)
    {
        libnetorrent_init_t init = reinterpret_cast<libnetorrent_init_t>(
            GetProcAddress(
                m_hModule,
                LIBNETORRENT_TOSTRING(LIBNETORRENT_INIT)));

        init(LIBNETORRENT_API_VERSION,
            reinterpret_cast<libnetorrent_plugin_t*>(this));
    }

    virtual ~Plugin()
    {
        EmitEvent(libnetorrent_event_shutdown, nullptr);
    }

    pt::Core::Configuration* Configuration() { return m_cfg; }
    pt::Core::Environment* Environment() { return m_env; }

    void AddHook(libnetorrent_hook_callback_t callback, libnetorrent_param_t* param)
    {
        m_hooks.push_back({ callback, param });
    }

    void EmitEvent(libnetorrent_event_t event, void* param)
    {
        for (auto const& cb : m_hooks)
        {
            cb.callback(event, reinterpret_cast<libnetorrent_param_t*>(param), cb.param);
        }
    }

private:
    struct Hook
    {
        libnetorrent_hook_callback_t callback;
        libnetorrent_param_t* param;
    };

    pt::Core::Environment* m_env;
    pt::Core::Configuration* m_cfg;
    HMODULE m_hModule;
    std::vector<Hook> m_hooks;
};

IPlugin::~IPlugin()
{
}

IPlugin* IPlugin::Load(fs::path const& p, pt::Core::Environment* env, pt::Core::Configuration* cfg)
{
    return new Plugin(
        LoadLibrary(p.wstring().c_str()),
        env,
        cfg);
}

libnetorrent_result_t libnetorrent_config_get(libnetorrent_plugin_t* plugin, libnetorrent_config_t** config)
{
    auto p = reinterpret_cast<Plugin*>(plugin);
    *config = reinterpret_cast<libnetorrent_config_t*>(p->Configuration());
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_config_bool_get(libnetorrent_config_t* cfg, const char* key, bool* result)
{
    auto config = reinterpret_cast<pt::Core::Configuration*>(cfg);
    *result = config->Get<bool>(key).value();
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_config_string_get(libnetorrent_config_t* cfg, const char* key, char* result, size_t* len)
{
    auto config = reinterpret_cast<pt::Core::Configuration*>(cfg);
    auto res = config->Get<std::string>(key).value_or("");
    strncpy(result, res.c_str(), *len);
    *len = res.size();
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_config_string_set(libnetorrent_config_t* config, const char* key, const char* value, size_t len)
{
    std::string val = len == (size_t)-1
        ? std::string(value)
        : std::string(value, len);

    reinterpret_cast<pt::Core::Configuration*>(config)->Set(key, val);

    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_http_get(const char* url, libnetorrent_http_callback_t callback, libnetorrent_param_t* user)
{
    auto client = std::make_shared<pt::Http::HttpClient>();
    client->Get(url, [client, callback, user](int statusCode, std::string const& data)
        {
            libnetorrent_http_response_t response;
            response.data = data;
            callback(&response, (libnetorrent_http_status_t)statusCode, user);
        });

    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_http_response_body(libnetorrent_http_response_t* response, char* body, size_t len)
{
    strncpy(body, response->data.c_str(), len);
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_http_response_body_len(libnetorrent_http_response_t* response, size_t* len)
{
    *len = response->data.size();
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_i18n(const char* key, wchar_t* target, size_t* len)
{
    wxString translation = pt::UI::Translator::GetInstance().Translate(key);
    size_t ll = len == nullptr
        ? wcslen(target)
        : *len;

    if (len != nullptr)
    {
        *len = translation.size();
    }

    if (translation.size() > ll || target == nullptr)
    {
        return libnetorrent_insufficient_buffer;
    }

    wcsncpy(target, translation.wc_str(), ll);

    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_mainwnd_native_handle(libnetorrent_mainwnd_t* wnd, void** handle)
{
    *handle = reinterpret_cast<pt::UI::MainFrame*>(wnd)->GetHWND();
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_menu_get(libnetorrent_mainwnd_t* wnd, libnetorrent_menu_id_t id, libnetorrent_menu_t** menu)
{
    auto mf = reinterpret_cast<pt::UI::MainFrame*>(wnd);
    auto menuBar = mf->GetMenuBar();

    switch (id)
    {
    case libnetorrent_menu_help:
        *menu = reinterpret_cast<libnetorrent_menu_t*>(menuBar->GetMenu(2));
        return libnetorrent_ok;
    }

    return libnetorrent_err;
}

libnetorrent_result_t libnetorrent_menu_insert_item(libnetorrent_menu_t* menu, uint32_t pos, const wchar_t* label, size_t len, libnetorrent_menuitem_callback_t cb, libnetorrent_param_t* user, libnetorrent_menuitem_t** item)
{
    auto m = reinterpret_cast<wxMenu*>(menu);
    auto itm = m->Insert(pos, 9999, wxString(label, len));
    m->Bind(
        wxEVT_MENU,
        [menu, item, cb, user](wxCommandEvent&)
        {
            cb(nullptr, user);
        },
        9999);

    if (item != nullptr)
    {
        *item = reinterpret_cast<libnetorrent_menuitem_t*>(itm);
    }

    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_menu_insert_separator(libnetorrent_menu_t* menu, uint32_t pos)
{
    auto m = reinterpret_cast<wxMenu*>(menu);
    m->InsertSeparator(pos);
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_register_hook(libnetorrent_plugin_t* plugin, libnetorrent_hook_callback_t cb, libnetorrent_param_t* user)
{
    reinterpret_cast<Plugin*>(plugin)->AddHook(cb, user);
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_string_towide(const char* input, wchar_t* output, size_t len)
{
    auto out = pt::Utils::toStdWString(input);
    wcsncpy(output, out.c_str(), len);
    return libnetorrent_ok;
}

libnetorrent_result_t libnetorrent_torrent_stats_get(libnetorrent_torrent_t* torrent, libnetorrent_torrent_stats_t* stats)
{
    auto t = reinterpret_cast<pt::BitTorrent::TorrentHandle*>(torrent);
    auto s = t->Status();

    stats->download_payload_rate = s.downloadPayloadRate;
    stats->upload_payload_rate = s.uploadPayloadRate;

    return libnetorrent_ok;
}

const char* libnetorrent_version()
{
    return pt::BuildInfo::version();
}
