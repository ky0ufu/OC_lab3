#include "timeutil.hpp"
#include <chrono>
#include <ctime>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

std::string now()
{
    using namespace std::chrono;
    auto tp = system_clock::now();
    auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
    std::time_t t = system_clock::to_time_t(tp);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    char buf[64];
    std::snprintf(buf, sizeof(buf),
        "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec,
        (long long)ms.count());

    return buf;
}

uint64_t pid()
{
#ifdef _WIN32
    return (uint64_t)GetCurrentProcessId();
#else
    return (uint64_t)getpid();
#endif
}

std::string self_path()
{
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return "program";
    return std::string(buf, buf + n);
#else
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return "./program";
    buf[n] = '\0';
    return std::string(buf);
#endif
}
