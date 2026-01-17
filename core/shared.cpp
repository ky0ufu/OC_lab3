#include "shared.hpp"
#include "logger.hpp"
#include "timeutil.hpp"

#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else // POSIX
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#endif

// Проверка на shared memory уже инициализирована ранее.
static const uint32_t SHM_MAGIC = 0xC0FFEE11;

SharedData* shared = nullptr;

#ifdef _WIN32

static const char* SHM_NAME = "Local\\global_counter_shm";
static const char* MUTEX_NAME = "Local\\global_counter_mutex";
static const char* INIT_MUTEX_NAME = "Local\\global_counter_init_mutex";

static HANDLE g_map = nullptr;  // handle на shared memory
static HANDLE g_counter_mutex = nullptr; // mutex для защиты счетчика
static HANDLE g_init_mutex = nullptr; // mutex для init

// вывод ошибок в win
static std::string win_err()
{
    DWORD e = GetLastError();
    char* msg = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, 
        e, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msg,
         0, 
         nullptr
    );
    std::string s = msg ? msg : "";
    if (msg) LocalFree(msg);
    return "winerr=" + std::to_string((unsigned long)e) + " " + s;
}
#else // POSIX
static const char* SHM_NAME = "/global_counter_shm";
static pthread_mutex_t* g_counter_mutex = nullptr; // указатель на mutex в shared memory
#endif


/*
    Инициализация shared memory.

    Вызывается каждым процессом при старте.
    Нужен, чтобы несколько процессов
    не инициализировали shared memory одновременно.
*/
void init_shared()
{
#ifdef _WIN32
    if (!g_init_mutex) g_init_mutex = CreateMutexA(nullptr, FALSE, INIT_MUTEX_NAME);
    if (!g_init_mutex) {
        log_line(now() + " PID=" + std::to_string(pid()) + " init_shared: CreateMutex(init) failed: " + win_err());
        return;
    }

    WaitForSingleObject(g_init_mutex, INFINITE);

    /*
        Создаём или открываем shared memory.
        Если объект уже существует — просто подключаемся к нему.
    */
    g_map = CreateFileMappingA(
        INVALID_HANDLE_VALUE, 
        nullptr, 
        PAGE_READWRITE, 
        0,
        (DWORD)sizeof(SharedData), 
        SHM_NAME);
    if (!g_map) {
        log_line(now() + " PID=" + std::to_string(pid()) + " init_shared: CreateFileMapping failed: " + win_err());
        ReleaseMutex(g_init_mutex);
        return;
    }

    // Отображаем shared memory в адресное пространство процесса.

    void* p = MapViewOfFile(g_map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (!p) {
        log_line(now() + " PID=" + std::to_string(pid()) + " init_shared: MapViewOfFile failed: " + win_err());
        ReleaseMutex(g_init_mutex);
        return;
    }

    shared = static_cast<SharedData*>(p);

    // Mutex для защиты операций со счётчиком.

    if (!g_counter_mutex) g_counter_mutex = CreateMutexA(nullptr, FALSE, MUTEX_NAME);
    if (!g_counter_mutex) {
        log_line(now() + " PID=" + std::to_string(pid()) + " init_shared: CreateMutex(counter) failed: " + win_err());
        ReleaseMutex(g_init_mutex);
        return;
    }


    // Если magic не совпадает — значит: - это первый процесс

    uint32_t m = shared->magic.load(std::memory_order_acquire);
    if (m != SHM_MAGIC) {
        shared->counter.store(0, std::memory_order_relaxed);
        shared->child_add10_running.store(false, std::memory_order_relaxed);
        shared->child_mul2_running.store(false, std::memory_order_relaxed);
        shared->magic.store(SHM_MAGIC, std::memory_order_release);
    }

    ReleaseMutex(g_init_mutex);

#else // POSIX

    // Пытаемся создать shared memory эксклюзивно.
    // Если удалось — первый процесс.

    bool created = false;

    int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);

    if (fd >= 0) {
        created = true;
        ftruncate(fd, sizeof(SharedData) + sizeof(pthread_mutex_t));
    } 
    else {
        // Объект уже существует — просто открываем
        fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
        ftruncate(fd, sizeof(SharedData) + sizeof(pthread_mutex_t));
    }

    // Отображаем shared memory в адресное пространство.
    void* mem = mmap(nullptr, sizeof(SharedData) + sizeof(pthread_mutex_t),
                     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    shared = static_cast<SharedData*>(mem);

    // Mutex лежит сразу после структуры SharedData в shared memory.
    g_counter_mutex = reinterpret_cast<pthread_mutex_t*>((char*)mem + sizeof(SharedData));

    // Только первый процесс инициализирует структуру
    // и mutex с атрибутом PTHREAD_PROCESS_SHARED.

    if (created) {
        shared->magic.store(SHM_MAGIC, std::memory_order_relaxed);
        shared->counter.store(0, std::memory_order_relaxed);
        shared->child_add10_running.store(false, std::memory_order_relaxed);
        shared->child_mul2_running.store(false, std::memory_order_relaxed);

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(g_counter_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }
#endif
}

//Блокировка shared counter для межпроцессной синхронизации.
void lock_shared()
{
#ifdef _WIN32
    if (!g_counter_mutex) return;
    WaitForSingleObject(g_counter_mutex, INFINITE);
#else
    pthread_mutex_lock(g_counter_mutex);
#endif
}

//Разблокировка shared counter.
void unlock_shared()
{
#ifdef _WIN32
    if (!g_counter_mutex) return;
    ReleaseMutex(g_counter_mutex);
#else
    pthread_mutex_unlock(g_counter_mutex);
#endif
}
