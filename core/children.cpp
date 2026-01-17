#include "chidren.hpp"
#include "shared.hpp"
#include "logger.hpp"
#include "timeutil.hpp"

#include <thread>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

/*
    Пытается атомарно пометить копию как "running".

    compare_exchange:
    expected = false
    если flag == false - становится true и возвращает true
    если flag == true - ничего не меняется и возвращает false
*/
static bool try_mark_running(std::atomic<bool>& flag)
{
    bool expected = false;
    return flag.compare_exchange_strong(expected, true, std::memory_order_acq_rel);
}

// Сбрасывает флаг "running".
static void clear_running(std::atomic<bool>& flag)
{
    flag.store(false, std::memory_order_release);
}

/*
    Логика:
    - пишет время старта
    - увеличивает счётчик на 10
    - пишет время выхода
    - завершает процесс
*/

int run_child_add10()
{
    shared->child_add10_running.store(true, std::memory_order_release);

    log_line(now() + " PID = " + std::to_string(pid()) + " child_add10 start");

    lock_shared();
    shared->counter.fetch_add(10);
    unlock_shared();

    log_line(now() + " PID = " + std::to_string(pid()) + " child_add10 exit");

    // Сообщаем мастеру, что копия завершилась
    shared->child_add10_running.store(false, std::memory_order_release);
    return 0;
}

/*
    Логика:
    - пишет время старта
    - умножает текущее значение счётчика на 2
    - через 2 секунды делит текущее значение счётчика на 2
    - пишет время выхода
    - завершает процесс
*/
int run_child_mul2()
{
    shared->child_mul2_running.store(true, std::memory_order_release);

    log_line(now() + " PID = " + std::to_string(pid()) + " child_mul2 start");

    lock_shared();
    long long v = shared->counter.load();
    shared->counter.store(v * 2);
    unlock_shared();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    lock_shared();
    long long v2 = shared->counter.load();
    shared->counter.store(v2 / 2);
    unlock_shared();

    log_line(now() + " PID = " + std::to_string(pid()) + " child_mul2 exit");

    // Сообщаем мастеру, что копия завершилась
    shared->child_mul2_running.store(false, std::memory_order_release);
    return 0;
}


// Проверяет, запущена ли сейчас хотя бы одна копия.
bool is_child_running()
{
    return shared->child_add10_running.load(std::memory_order_acquire) ||
           shared->child_mul2_running.load(std::memory_order_acquire);
}


// Запуск одной дочерней копии как нового процесса.

static bool spawn_one(const std::string& arg)
{
#ifdef _WIN32
    /*
        Используем CreateProcessA.
        Передаём аргумент:
            program.exe child_add10
            program.exe child_mul2
    */
    std::string cmd = "\"" + self_path() + "\" " + arg;

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    // CreateProcess требует изменяемый buffer
    std::string mutable_cmd = cmd;
    BOOL ok = CreateProcessA(
        nullptr, 
        mutable_cmd.data(),                      
        nullptr, nullptr, 
        FALSE,
        0, 
        nullptr, nullptr, 
        &si, &pi);

    if (ok) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
    log_line(now() + " PID=" + std::to_string(pid()) + " failed to spawn " + arg);
    return false;
#else // POSIX
    // используем posix_spawn.
    std::string exe = self_path();
    char* argv_spawn[] = {
        const_cast<char*>(exe.c_str()),
        const_cast<char*>(arg.c_str()),
        nullptr
    };

    pid_t child_pid{};
    int rc = posix_spawn(&child_pid, exe.c_str(), nullptr, nullptr, argv_spawn, environ);
    if (rc == 0) return true;

    log_line(now() + " PID=" + std::to_string(pid()) + " failed to spawn " + arg);
    return false;
#endif
}

void spawn_children()
{
    // add10
    if (!try_mark_running(shared->child_add10_running)) {
        log_line(now() + " PID=" + std::to_string(pid()) + " child_add10 still running; skip spawn");
    } else {
        if (!spawn_one("child_add10")) {
            clear_running(shared->child_add10_running);
        }
    }

    // mul2
    if (!try_mark_running(shared->child_mul2_running)) {
        log_line(now() + " PID=" + std::to_string(pid()) + " child_mul2 still running; skip spawn");
    } else {
        if (!spawn_one("child_mul2")) {
            clear_running(shared->child_mul2_running);
        }
    }
}
