#pragma once

#include <filesystem>

#include <libnetorrent.h>

namespace fs = std::filesystem;

namespace pt
{
namespace Core
{
    class Configuration;
    class Environment;
}
namespace API
{
    class IPlugin
    {
    public:
        virtual ~IPlugin();
        static IPlugin* Load(fs::path const& p, Core::Environment* env, Core::Configuration* cfg);

        virtual void EmitEvent(libnetorrent_event_t event, void* param) = 0;
    };
}
}
