#pragma once

#include <string>

namespace pt::RSS
{
    struct Feed
    {
        int id = -1;
        std::string url;
        std::string name;
        bool enabled = true;
        bool isRegex = false;
        std::string includeFilter;
        std::string excludeFilter;
    };
}
