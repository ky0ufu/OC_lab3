#pragma once
#include <atomic>
#include "core_api.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif


struct CORE_API SharedData {
    std::atomic<long long> counter;
    std::atomic<bool> child_add10_running;
    std::atomic<bool> child_mul2_running;

#ifdef _WIN32
    HANDLE mutex;
#else
    pthread_mutex_t mutex;
#endif
};

CORE_API extern SharedData* shared;

CORE_API void init_shared();
CORE_API void lock_shared();
CORE_API void unlock_shared();

