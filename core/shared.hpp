#pragma once
#include <atomic>
#include <cstdint>
#include "core_api.hpp"

struct CORE_API SharedData {
    std::atomic<uint32_t> magic;
    std::atomic<long long> counter;
    std::atomic<bool> child_add10_running;
    std::atomic<bool> child_mul2_running;
};

CORE_API extern SharedData* shared;

CORE_API void init_shared();
CORE_API void lock_shared();
CORE_API void unlock_shared();
