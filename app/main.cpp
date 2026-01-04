#include "shared.hpp"
#include "logger.hpp"
#include "timeutil.hpp"
#include "master.hpp"
#include "chidren.hpp"
#include "cli.hpp"

#include <thread>
#include <atomic>
#include <chrono>
#include <string>

/*
    Обработка завершения по Ctrl+C.

    - корректно остановить все рабочие потоки
    - не рвать shared memory в произвольном состоянии
    - дать дочерним потокам завершиться
*/

#ifdef _WIN32
#include <windows.h>


// Используется в обработчике консольных событий.
static std::atomic<bool>* g_running_ptr = nullptr;


// Вызывается системой при Ctrl+C.
static BOOL WINAPI console_handler(DWORD type)
{
    if (!g_running_ptr) return FALSE;
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT) {
        // Сигнализируем всем потокам о завершении
        g_running_ptr->store(false);
        return TRUE;
    }
    return FALSE;
}
#else // POSIX

#include <signal.h>
static std::atomic<bool>* g_running_ptr = nullptr;

static void sigint_handler(int)
{
    if (g_running_ptr) g_running_ptr->store(false);
}
#endif

int main(int argc, char** argv)
{
    init_shared();

    // Если shared memory не инициализировалась — фатальная ошибка
    if (!shared) {
        log_line(now() + " PID=" + std::to_string(pid()) + " FATAL: shared memory not initialized");
        return 1;
    }

    /*
        Проверяем, не запущены ли мы в режиме дочерней копии.
        - просто выполняем нужное действие и выходим
    */
    if (argc >= 2) {
        std::string mode = argv[1];
        if (mode == "child_add10") return run_child_add10();
        if (mode == "child_mul2")  return run_child_mul2();
    }

    log_line(now() + " PID = " + std::to_string(pid()) + " started");

    /*
        Определяем, является ли текущий процесс мастером.

        Мастер:
        - пишет лог раз в секунду
        - запускает дочерние копии раз в 3 секунды

        Остальные процессы:
        - только увеличивают счётчик
        - принимают CLI-команды
    */
    bool is_master = ac_master();

    // флаг завершения
    std::atomic<bool> running{true};
    g_running_ptr = &running;

    // Регистрируем обработчики Ctrl+C / SIGINT
#ifdef _WIN32
    SetConsoleCtrlHandler(console_handler, TRUE);
#else
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
#endif

    // Поток counter
    std::thread counter_thread([&] {
        while (running.load()) {
            lock_shared();
            shared->counter.fetch_add(1);
            unlock_shared();

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });

    // Поток CLI
    std::thread cli_thread(cli_loop);

    // Потоки log и spawn запускает только мастер
    std::thread log_thread;
    std::thread spawn_thread;

    if (is_master) {
        // поток логирование
        log_thread = std::thread([&] {
            while (running.load()) {
                long long v;
                lock_shared();
                v = shared->counter.load();
                unlock_shared();

                log_line(now() + " PID = " + std::to_string(pid()) +
                         " counter=" + std::to_string(v));

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
        // поток spawn дочеренних процессов
        spawn_thread = std::thread([&] {
            while (running.load()) {
                spawn_children();
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        });
    }

    cli_thread.join();      // закончится когда stdin закроется
    running.store(false);   // если stdin закрыли — останавливаем всё

    counter_thread.join();

    if (is_master) {
        if (spawn_thread.joinable()) spawn_thread.join();
        if (log_thread.joinable()) log_thread.join();
    }

    return 0;
}
