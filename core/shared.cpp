#include "shared.hpp"
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

static const char* SHM_NAME = "global_counter";
SharedData* shared = nullptr;

void init_shared()
{
#ifdef _WIN32
#else
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedData));

    shared = static_cast<SharedData*>(
        mmap(nullptr, sizeof(SharedData),
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
#endif

    static bool init = false;
    if (!init) {
        shared->counter.store(0);
        shared->child_add10_running.store(false);
        shared->child_mul2_running.store(false);
#ifdef _WIN32
        shared->mutex = CreateMutexA(nullptr, FALSE, "counter_mutex");
#else
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&shared->mutex, &attr);
#endif
        init = true;
    }
}

void lock_shared()
{
#ifdef _WIN32
    WaitForSingleObject(shared->mutex, INFINITE);
#else
    pthread_mutex_lock(&shared->mutex);
#endif
}

void unlock_shared()
{
#ifdef _WIN32
    ReleaseMutex(shared->mutex);
#else
    pthread_mutex_unlock(&shared->mutex);
#endif
}