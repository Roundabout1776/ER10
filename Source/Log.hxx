#pragma once

#include <cstdio>
#include <iostream>

enum class ELogLevel
{
    Critical,
    Info,
    Debug,
    Verbose
};

namespace Log
{
#ifdef EQUINOX_REACH_DEVELOPMENT
    static constexpr ELogLevel LogLevel = ELogLevel::Debug;
#else
    static constexpr ELogLevel LogLevel = ELogLevel::Info;
#endif

    template <ELogLevel ThisLogLevel, typename... Ps>
    static constexpr void LogInternal(const char* Prefix, const char* Fmt, Ps... Args)
    {
        if constexpr (ThisLogLevel <= LogLevel)
        {
            char Entry[1024]{};
            char Msg[960]{};
            snprintf(Msg, 960, Fmt, Args...);
            snprintf(Entry, 1024, "[%s] %s", Prefix, Msg);
            std::cout << Entry << std::endl;
        }
    }

#define LOG_CATEGORY(Name)                                  \
    template <ELogLevel ThisLogLevel, typename... Ps>       \
    static constexpr void Name(const char* Fmt, Ps... Args) \
    {                                                       \
        LogInternal<ThisLogLevel>(#Name, Fmt, Args...);     \
    }

    LOG_CATEGORY(Audio)
    LOG_CATEGORY(Memory)
    LOG_CATEGORY(Platform)
    LOG_CATEGORY(Draw)

#undef LOG_CATEGORY
}
