#include "master.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

static const char* LOCK_FILE = "program.lock";

#ifdef _WIN32
static HANDLE g_lock_handle = INVALID_HANDLE_VALUE;
#else
static int g_lock_fd = -1;
#endif

bool ac_master()
{
#ifdef _WIN32
    if (g_lock_handle != INVALID_HANDLE_VALUE) return true;

    g_lock_handle = CreateFileA(
        LOCK_FILE,
        GENERIC_WRITE,
        0,              // no sharing
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    return g_lock_handle != INVALID_HANDLE_VALUE;
#else
    if (g_lock_fd != -1) return true;

    g_lock_fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
    if (g_lock_fd < 0) return false;

    return flock(g_lock_fd, LOCK_EX | LOCK_NB) == 0;
#endif
}
