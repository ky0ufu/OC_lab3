#include "logger.hpp"
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

static const char* LOG_FILE = "program.log";

#ifdef _WIN32
static HANDLE g_log_mutex = nullptr;

static void ensure_log_mutex()
{
    if (!g_log_mutex) {
        // Local\\ вместо Global\\
        g_log_mutex = CreateMutexA(nullptr, FALSE, "Local\\program_log_mutex");
    }
}
#endif

void log_line(const std::string& msg)
{
#ifdef _WIN32
    ensure_log_mutex();
    if (g_log_mutex) WaitForSingleObject(g_log_mutex, INFINITE);

    std::ofstream f(LOG_FILE, std::ios::app);
    f << msg << "\n";
    f.flush();

    if (g_log_mutex) ReleaseMutex(g_log_mutex);
#else
    int fd = open(LOG_FILE, O_CREAT | O_WRONLY | O_APPEND, 0666);
    if (fd < 0) return;

    flock(fd, LOCK_EX);

    std::ofstream f(LOG_FILE, std::ios::app);
    f << msg << "\n";
    f.flush();

    flock(fd, LOCK_UN);
    close(fd);
#endif
}
