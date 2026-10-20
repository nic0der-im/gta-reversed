#include "StdInc.h"

#include "oswrapper/oswrapper.h"

#ifdef NOTSA_NATIVE

#include "app/platform/native/NativePath.h"

#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

namespace {
std::string g_BasePath;

//! Prefix relative paths with `OS_SetFilePathOffset`'s path and normalize them.
std::string BuildPath(const char* file) {
    std::string path;
    if (!g_BasePath.empty()) {
        path = g_BasePath;
        if (path.back() != '/') {
            path += '/';
        }
    }
    path += file;
    return notsa::native::NormalizePath(path);
}

struct OSThread {
    std::thread             thread;
    std::mutex              mutex;
    std::condition_variable cv;
    bool                    started{};
    bool                    suspended{};
};

struct OSMutex {
    std::mutex mutex;
};

struct OSSemaphore {
    std::mutex              mutex;
    std::condition_variable cv;
    int32                   count{};
    int32                   maxCount{1};
};
} // namespace

// 0x5617A0
//! Monotonic time in 100ns ticks, matching the Windows' `QueryPerformanceCounter` scale.
uint64 GetOSWPerformanceTime() {
    using namespace std::chrono;
    static const auto s_Start = steady_clock::now();
    return static_cast<uint64>(duration_cast<nanoseconds>(steady_clock::now() - s_Start).count() / 100);
}

//! Matches the Windows' `QueryPerformanceFrequency` on most systems (10MHz).
int64 GetOSWPerformanceFrequency() {
    return 10'000'000;
}

void OS_DebugOut(const char* msg) {
    spdlog::debug("{}", msg);
}

const char* OS_FileGetArchiveName(int32 archive) {
    UNUSED(archive);
    return nullptr;
}

int32 OS_FileSize(void* file) {
    auto* stream = static_cast<FILE*>(file);
    if (!stream) {
        return 0;
    }

    const auto curPos = std::ftell(stream);
    std::fseek(stream, 0, SEEK_END);
    const auto size = std::ftell(stream);
    std::fseek(stream, curPos, SEEK_SET);
    return static_cast<int32>(size);
}

int32 OS_FileDelete(const char* file) {
    return std::remove(BuildPath(file).c_str()) == 0 ? 1 : 0;
}

int32 OS_FileOpen(OSFileDataArea dataArea, void** pfile, const char* file, OSFileAccessType access) {
    UNUSED(dataArea);

    const char* mode;
    switch (access) {
    case FILE_ACCESS_READ:     mode = "rb";  break;
    case FILE_ACCESS_WRITE:    mode = "wb";  break;
    case FILE_ACCESS_EXISTING: mode = "rb+"; break;
    default:                   mode = "rb";  break;
    }

    *pfile = std::fopen(BuildPath(file).c_str(), mode);
    return *pfile ? 0 : (errno ? errno : ENOENT);
}

int32 OS_FileGetPosition(void* file) {
    auto* stream = static_cast<FILE*>(file);
    return stream ? static_cast<int32>(std::ftell(stream)) : 0;
}

void OS_FileSetPosition(void* file, int32 pos) {
    if (auto* stream = static_cast<FILE*>(file)) {
        std::fseek(stream, pos, SEEK_SET);
    }
}

int32 OS_FileRead(void* file, void* dest, int32 destSize) {
    auto* stream = static_cast<FILE*>(file);
    if (!stream) {
        return 3;
    }
    const auto read = static_cast<int32>(std::fread(dest, 1, destSize, stream));
    return read == destSize ? 0 : 3;
}

int32 OS_FileWrite(void* file, const void* src, int32 srcSize) {
    auto* stream = static_cast<FILE*>(file);
    if (!stream) {
        return 3;
    }
    const auto written = static_cast<int32>(std::fwrite(src, 1, srcSize, stream));
    return written == srcSize ? 0 : 3;
}

int32 OS_FileClose(void* file) {
    auto* stream = static_cast<FILE*>(file);
    return stream ? std::fclose(stream) : 0;
}

void OS_DebugBreak() {
    __builtin_debugtrap();
}

void OS_ThreadSleep(uint32 nMSec) {
    std::this_thread::sleep_for(std::chrono::milliseconds{nMSec});
}

void* OS_SemaphoreCreate(int32 iMaximumCount, const char* name) {
    UNUSED(name);

    auto* semaphore = new OSSemaphore;
    semaphore->maxCount = iMaximumCount > 0 ? iMaximumCount : 1;
    return semaphore;
}

bool OS_SemaphorePost(void* semaphore) {
    auto* sem = static_cast<OSSemaphore*>(semaphore);
    if (!sem) {
        return false;
    }

    {
        std::lock_guard lock{sem->mutex};
        if (sem->count >= sem->maxCount) {
            return false;
        }
        ++sem->count;
    }
    sem->cv.notify_one();
    return true;
}

void OS_SemaphoreWait(void* semaphore) {
    auto* sem = static_cast<OSSemaphore*>(semaphore);
    if (!sem) {
        return;
    }

    std::unique_lock lock{sem->mutex};
    sem->cv.wait(lock, [sem] { return sem->count > 0; });
    --sem->count;
}

void OS_SemaphoreDelete(void* semaphore) {
    delete static_cast<OSSemaphore*>(semaphore);
}

void* OS_MutexCreate(const char* name) {
    UNUSED(name);
    return new OSMutex;
}

void OS_MutexRelease(void* mutex) {
    if (auto* m = static_cast<OSMutex*>(mutex)) {
        m->mutex.unlock();
    }
}

void OS_MutexObtain(void* mutex) {
    if (auto* m = static_cast<OSMutex*>(mutex)) {
        m->mutex.lock();
    }
}

void OS_MutexDelete(void* mutex) {
    delete static_cast<OSMutex*>(mutex);
}

void OS_SetFilePathOffset(const char* path) {
    g_BasePath = path ? path : "";
}

//! @note Thread priorities can't be portably set, so they're ignored.
void* OS_ThreadLaunch(OS_ThreadRoutine pfnStart, void* param, uint32 nFlags, const char* name, void* unk, OSThreadPriority priority) {
    UNUSED(unk, priority);

    auto* thread = new OSThread;
    if (nFlags & OS_CREATE_SUSPENDED) {
        thread->suspended = true;
        thread->thread    = std::thread{[thread, pfnStart, param] {
            {
                std::unique_lock lock{thread->mutex};
                thread->cv.wait(lock, [thread] { return thread->started; });
            }
            pfnStart(param);
        }};
    } else {
        thread->started = true;
        thread->thread  = std::thread{[pfnStart, param] { pfnStart(param); }};
    }

    NOTSA_LOG_DEBUG("OS: Thread {:p} ({}) created", static_cast<void*>(thread), name ? name : "<unnamed>");
    return thread;
}

void OS_ThreadClose(void* thread) {
    delete static_cast<OSThread*>(thread);
}

void OS_ThreadWait(void* thread) {
    auto* t = static_cast<OSThread*>(thread);
    if (t && t->thread.joinable()) {
        t->thread.join();
    }
}

void OS_ThreadResume(void* thread) {
    auto* t = static_cast<OSThread*>(thread);
    if (!t) {
        return;
    }

    {
        std::lock_guard lock{t->mutex};
        t->suspended = false;
        t->started   = true;
    }
    t->cv.notify_all();
}

double OS_TimeAccurate() {
    return static_cast<double>(GetOSWPerformanceTime()) / 1000.0;
}

uint32 OS_TimeMS() {
    return static_cast<uint32>(GetOSWPerformanceTime() / GetOSWPerformanceFrequency());
}

#endif // NOTSA_NATIVE
