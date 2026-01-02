#include "master.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

static const char* LOCK_FILE = "program.lock";
static int d = -1;

bool ac_master()
{
#ifdef _WIN32
    HANDLE h = CreateFileA(
        LOCK_FILE, GENERIC_WRITE,
        0, nullptr, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    return h != INVALID_HANDLE_VALUE;
#else
    int fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
    return flock(fd, LOCK_EX | LOCK_NB) == 0;
#endif
}