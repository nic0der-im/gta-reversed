#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "app/platform/native/NativeCrash.h"

#include <csignal>
#include <cstdio>
#include <cstring>
#include <iterator>
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

void OnFatalSignal(int sig) {
    // Only async-signal-safe calls here: no allocation and no logging.
    char msg[64];
    if (std::snprintf(msg, sizeof(msg), "Native: fatal signal %d\n", sig) > 0) {
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
    action.sa_handler = &OnFatalSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    for (const auto sig : { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT }) {
        sigaction(sig, &action, nullptr);
    }
}

} // namespace notsa::native

#endif // NOTSA_NATIVE
