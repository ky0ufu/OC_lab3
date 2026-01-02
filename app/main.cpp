#include "shared.hpp"
#include "logger.hpp"
#include "timeutil.hpp"
#include "master.hpp"
#include "chidren.hpp"
#include "cli.hpp"

#include <thread>
#include <atomic>
#include <chrono>

int main()
{
    init_shared();

    log_line(now() + " PID = " + std::to_string(pid()) + " started");

    bool is_master = ac_master();
    std::atomic<bool>running{true};

    std::thread counter_thread([&] {
        while (running) {
            shared->counter.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });

    std::thread cli_thread(cli_loop);

    if (is_master) {
        std::thread log_thread([&] {
        while (running) {
            log_line(now() + " PID = " + std::to_string(pid()) + 
            " counter=" + std::to_string(shared->counter.load()));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
        std::thread spawn_thread([&] {
            while (running) {
                spawn_children();
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        });
    
        log_thread.join();
        spawn_thread.join();
    }
    counter_thread.join();
    cli_thread.join();
    return 0;
}