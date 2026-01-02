#include "chidren.hpp"
#include "shared.hpp"
#include "logger.hpp"
#include "timeutil.hpp"
#include <iostream>
#include <thread>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


static void child_add10()
{


    shared->child_add10_running.store(true);

    log_line(now() + " PID = " + std::to_string(pid()) + " child_add10 start");

    lock_shared();
    shared->counter.fetch_add(10);
    unlock_shared();

    log_line(now() + " PID = " + std::to_string(pid()) + " child_add10 exit");
    
    shared->child_add10_running.store(false);

}

static void child_mul2()
{
    shared->child_mul2_running = true;

    log_line(now() + " PID = " + std::to_string(pid()) + " child_mul2 start");

    lock_shared();
    long long base = shared->counter.load();

    shared->counter.store(shared->counter.load() * 2);
    unlock_shared();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    lock_shared();
    shared->counter.store(shared->counter.load()/2);
    unlock_shared();

    log_line(now() + " PID = " + std::to_string(pid()) + " child_mul2 exit");

    shared->child_mul2_running = false;
}


bool is_child_running() {
    return shared->child_add10_running ||
        shared->child_mul2_running;
}
void spawn_children()
{

    if (is_child_running()) {
        log_line("PID=" + std::to_string(pid()) + " child already running skip spawn");
        return;
    }
#ifdef _WIN32
    STARTUPINFOA si{sizeof(si)};
    PROCESS_INFORMATION pi{};

    CreateProcessA(nullptr, (LPSTR) "program child_add10",
                   nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

    CreateProcessA(nullptr, (LPSTR) "program child_mul2",
                   nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
#else
    if (fork() == 0) {child_add10(); _exit(0);}
    if (fork() == 0) {child_mul2(); _exit(0);}
#endif
}