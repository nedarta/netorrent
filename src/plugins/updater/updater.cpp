#include <libnetorrent.h>

#include <string>
#include <Windows.h>
#include <CommCtrl.h>
#include <shellapi.h>

#pragma warning(push)
#pragma warning(disable: 4200)
#pragma warning(disable: 4996)
#include "sajson.h"
#include "semver.hpp"
#pragma warning(pop)

#define DEFAULT_I18N_BUFFER_SIZE 256

struct updater_request_data_t
{
    bool force;
    libnetorrent_config_t* config;
    libnetorrent_mainwnd_t* wnd;
};

void show_available_update(libnetorrent_mainwnd_t* wnd, libnetorrent_config_t* config, const char* version, const char* url)
{
    HWND hWnd = nullptr;
    libnetorrent_mainwnd_native_handle(wnd, reinterpret_cast<void**>(&hWnd));

    // get wide version
    wchar_t versionw[100];
    libnetorrent_string_towide(version, versionw, 100);

    wchar_t content[DEFAULT_I18N_BUFFER_SIZE];
    size_t content_len = DEFAULT_I18N_BUFFER_SIZE;
    libnetorrent_i18n("new_version_available", content, &content_len);

    wchar_t main[DEFAULT_I18N_BUFFER_SIZE];
    wchar_t main_format[DEFAULT_I18N_BUFFER_SIZE];
    size_t main_len = DEFAULT_I18N_BUFFER_SIZE;
    libnetorrent_i18n("netorrent_v_available", main, &main_len);
    swprintf(main_format, DEFAULT_I18N_BUFFER_SIZE, main, versionw);

    wchar_t verification[DEFAULT_I18N_BUFFER_SIZE];
    size_t verification_len = DEFAULT_I18N_BUFFER_SIZE;
    libnetorrent_i18n("ignore_update", verification, &verification_len);

    wchar_t show[DEFAULT_I18N_BUFFER_SIZE];
    size_t show_len = DEFAULT_I18N_BUFFER_SIZE;
    libnetorrent_i18n("show_on_github", show, &show_len);

    const TASKDIALOG_BUTTON pButtons[] =
    {
        { 1000, show },
    };

    TASKDIALOGCONFIG tdf = { sizeof(TASKDIALOGCONFIG) };
    tdf.cButtons = ARRAYSIZE(pButtons);
    tdf.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    tdf.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_USE_COMMAND_LINKS;
    tdf.hwndParent = hWnd;
    tdf.pButtons = pButtons;
    tdf.pszMainIcon = TD_INFORMATION_ICON;
    tdf.pszMainInstruction = main_format;
    tdf.pszVerificationText = verification;
    tdf.pszWindowTitle = TEXT("NeTorrent");

    int pnButton = -1;
    int pnRadioButton = -1;
    BOOL pfVerificationFlagChecked = FALSE;

    TaskDialogIndirect(&tdf, &pnButton, &pnRadioButton, &pfVerificationFlagChecked);

    if (pnButton == 1000)
    {
        wchar_t urlw[255];
        libnetorrent_string_towide(url, urlw, 255);

        SHELLEXECUTEINFO sei;
        ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

        sei.cbSize = sizeof(SHELLEXECUTEINFO);
        sei.lpFile = urlw;
        sei.lpVerb = TEXT("open");
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_FLAG_NO_UI;

        ::ShellExecuteEx(&sei);
    }

    if (pfVerificationFlagChecked)
    {
        libnetorrent_config_string_set(
            config,
            "update_checks.ignored_version",
            version,
            (size_t)-1);
    }
}

void show_no_update(libnetorrent_mainwnd_t* wnd)
{
    HWND hWnd = nullptr;
    libnetorrent_mainwnd_native_handle(wnd, reinterpret_cast<void**>(&hWnd));

    wchar_t main[DEFAULT_I18N_BUFFER_SIZE];
    size_t main_len = DEFAULT_I18N_BUFFER_SIZE;
    libnetorrent_i18n("no_update_available", main, &main_len);

    TASKDIALOGCONFIG tdf = { sizeof(TASKDIALOGCONFIG) };
    tdf.dwCommonButtons = TDCBF_OK_BUTTON;
    tdf.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
    tdf.hwndParent = hWnd;
    tdf.pszMainIcon = TD_INFORMATION_ICON;
    tdf.pszMainInstruction = main;
    tdf.pszWindowTitle = L"NeTorrent";

    TaskDialogIndirect(&tdf, nullptr, nullptr, nullptr);
}

libnetorrent_result_t parse_response(
    libnetorrent_http_response_t* response,
    libnetorrent_http_status_t status,
    libnetorrent_param_t* user)
{
    updater_request_data_t* data = reinterpret_cast<updater_request_data_t*>(user);

    switch (status)
    {
    case libnetorrent_http_ok:
    {
        size_t len;
        libnetorrent_http_response_body_len(response, &len);

        if (len > 0)
        {
            std::string body(len, '\0');
            libnetorrent_http_response_body(response, body.data(), body.size());

            const sajson::document& doc = sajson::parse(
                sajson::dynamic_allocation(),
                sajson::mutable_string_view(body.size(), body.data()));

            if (!doc.is_valid())
            {
                // TODO: log
                break;
            }

            const sajson::value& root = doc.get_root();

            std::string version = root.get_value_of_key(sajson::literal("version")).as_string();

            char ignoredVersion[100];
            size_t ignoredVersionLen = 100;

            libnetorrent_config_string_get(
                data->config,
                "update_checks.ignored_version",
                ignoredVersion,
                &ignoredVersionLen);

            if (version == std::string(ignoredVersion, ignoredVersionLen) && !data->force)
            {
                break;
            }

            semver::version parsedVersion(version);
            semver::version currentVersion(libnetorrent_version());

            if (parsedVersion > currentVersion)
            {
                std::string url = root.get_value_of_key(sajson::literal("url")).as_string();

                show_available_update(
                    data->wnd,
                    data->config,
                    version.c_str(),
                    url.c_str());
            }
            else if (data->force)
            {
                show_no_update(data->wnd);
            }
        }
        break;
    }
    }

    return libnetorrent_ok;
}

void make_request(libnetorrent_config_t* config, libnetorrent_mainwnd_t* wnd, bool force)
{
    updater_request_data_t* data = new updater_request_data_t();
    data->config = config;
    data->force = force;
    data->wnd = wnd;

    char url[255];
    size_t url_len = 255;

    libnetorrent_config_string_get(config, "update_checks.url", url, &url_len);
    libnetorrent_http_get(url, parse_response, reinterpret_cast<libnetorrent_param_t*>(data));
}

libnetorrent_result_t check(libnetorrent_menuitem_t* item, libnetorrent_param_t* param)
{
    updater_request_data_t* data = reinterpret_cast<updater_request_data_t*>(param);
    make_request(data->config, data->wnd, data->force);
    return libnetorrent_ok;
}

libnetorrent_result_t on_events(
    libnetorrent_event_t event,
    libnetorrent_param_t* param,
    libnetorrent_param_t* user)
{
    libnetorrent_plugin_t* plugin = reinterpret_cast<libnetorrent_plugin_t*>(user);

    switch (event)
    {
    case libnetorrent_event_mainwnd_created:
    {
        libnetorrent_config_t* config = nullptr;
        libnetorrent_config_get(plugin, &config);

        bool enabled = false;
        libnetorrent_config_bool_get(config, "update_checks.enabled", &enabled);

        if (enabled)
        {
            make_request(
                config,
                reinterpret_cast<libnetorrent_mainwnd_t*>(param),
                false);
        }

        // Insert item in about menu to force a check for update
        libnetorrent_menu_t* help;
        libnetorrent_menu_get(reinterpret_cast<libnetorrent_mainwnd_t*>(param), libnetorrent_menu_help, &help);

        wchar_t check_for_update[DEFAULT_I18N_BUFFER_SIZE];
        size_t check_for_update_len = DEFAULT_I18N_BUFFER_SIZE;
        libnetorrent_i18n("amp_check_for_update", check_for_update, &check_for_update_len);

        updater_request_data_t* data = new updater_request_data_t();
        data->config = config;
        data->force = true;
        data->wnd = reinterpret_cast<libnetorrent_mainwnd_t*>(param);

        libnetorrent_menu_insert_item(
            help,
            0,
            check_for_update,
            check_for_update_len,
            check,
            reinterpret_cast<libnetorrent_param_t*>(data),
            nullptr);

        libnetorrent_menu_insert_separator(
            help,
            1);

        break;
    }
    }

    return libnetorrent_ok;
}

libnetorrent_result_t init_updater(int version, libnetorrent_plugin_t* plugin)
{
    if (version != LIBNETORRENT_API_VERSION)
    {
        return libnetorrent_version_mismatch;
    }

    return libnetorrent_register_hook(plugin, on_events, reinterpret_cast<libnetorrent_param_t*>(plugin));
}

LIBNETORRENT_DEFINE_PLUGIN(
    "updater",
    "1.0",
    init_updater);
