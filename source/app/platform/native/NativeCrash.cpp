#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "app/platform/native/NativeCrash.h"

#include <csignal>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <sys/ucontext.h>
#include <unistd.h>

#include <execinfo.h>

namespace {

//! Writes the whole string to stderr, ignoring errors (we're about to die anyway).
void WriteToStderr(const char* str) {
    const auto len = std::strlen(str);
    for (size_t numWritten = 0; numWritten < len; ) {
        const auto n = ::write(STDERR_FILENO, str + numWritten, len - numWritten);
        if (n <= 0) {
            return;
        }
        numWritten += static_cast<size_t>(n);
    }
}

//! The interrupted `pc`, which the backtrace can't be trusted to report (there's no DWARF).
void* GetSignalPC(void* context) {
#if defined(__APPLE__) && defined(__aarch64__)
    return context ? reinterpret_cast<void*>(static_cast<ucontext_t*>(context)->uc_mcontext->__ss.__pc) : nullptr;
#elif defined(__APPLE__) && defined(__x86_64__)
    return context ? reinterpret_cast<void*>(static_cast<ucontext_t*>(context)->uc_mcontext->__ss.__rip) : nullptr;
#else
    (void)context;
    return nullptr;
#endif
}

void OnFatalSignal(int sig, siginfo_t* info, void* context) {
    // Only async-signal-safe calls here: no allocation and no logging.
    char msg[128];
    if (std::snprintf(
            msg,
            sizeof(msg),
            "Native: fatal signal %d at %p (pc %p)\n",
            sig,
            sig == SIGSEGV || sig == SIGBUS || sig == SIGILL || sig == SIGFPE ? info->si_addr : nullptr,
            GetSignalPC(context)
        ) > 0
    ) {
        WriteToStderr(msg);
    }

    void* frames[64];
    const auto numFrames = ::backtrace(frames, static_cast<int>(std::size(frames)));
    ::backtrace_symbols_fd(frames, numFrames, STDERR_FILENO);

    // Restore the default disposition and re-raise, so the process still dies from this signal.
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

} // namespace

namespace notsa::native {

void InstallCrashHandler() {
    struct sigaction action = {};
    action.sa_sigaction = &OnFatalSignal;
    action.sa_flags     = SA_RESTART | SA_SIGINFO;
    sigemptyset(&action.sa_mask);

    for (const auto sig : { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP }) {
        sigaction(sig, &action, nullptr);
    }
}

} // namespace notsa::native

#endif // NOTSA_NATIVE
