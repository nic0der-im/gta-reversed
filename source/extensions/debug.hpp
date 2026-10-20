#pragma once

#include <Base.h>
#include <format>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>

#ifdef NOTSA_NATIVE
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/proc.h>
    #include <unistd.h>
#else
    #include <windows.h>
    #include <debugapi.h>
#endif

namespace notsa {
namespace debug {
#ifdef NOTSA_NATIVE
inline bool IsDebuggerPresent() {
    int               mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
    struct kinfo_proc info{};
    auto              size = sizeof(info);
    return sysctl(mib, std::size(mib), &info, &size, nullptr, 0) == 0 && (info.kp_proc.p_flag & P_TRACED);
}

inline void WaitForDebugger() {
    while (!IsDebuggerPresent()) {
        NOTSA_LOG_INFO("[debug] Waiting for debugger\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

//! Native builds have no console to attach to - Output already goes to the terminal.
inline void DisplayConsole() {}

/*!
 * @return function information (module, function name, line number) at the given address.
 * @note Native builds have no DbgHelp symbol engine, so only the address is returned.
 */
inline std::string GetFunctionInfoAtAddress(uintptr_t address, bool compact = false) {
    return std::format("0x{:X}", address);
}

inline void LoadSymbols() {}
inline void UnloadSymbols() {}
#else
inline bool IsDebuggerPresent() {
    return ::IsDebuggerPresent();
}

inline void WaitForDebugger() {
    while (!IsDebuggerPresent()) {
        NOTSA_LOG_INFO("[debug] Waiting for debugger\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

inline void DisplayConsole() {
    // Support UTF-8 IO for Windows Terminal. (or CMD if a supported font is used)
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

#ifdef NOTSA_STANDALONE
    // In standalone mode try using the parent process's console if it has one
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        VERIFY(AllocConsole());
    }
#else
    // In asi mode always allocate a new console
    VERIFY(AllocConsole());
#endif

    FILESTREAM fs{};
    VERIFY(freopen_s(&fs, "CONIN$", "r", stdin) == NOERROR);
    VERIFY(freopen_s(&fs, "CONOUT$", "w", stdout) == NOERROR);
    VERIFY(freopen_s(&fs, "CONOUT$", "w", stderr) == NOERROR);
}

/*!
 * @return function information (module, function name, line number) at the given address.
 */
std::string GetFunctionInfoAtAddress(uintptr_t address, bool compact = false, HANDLE hProcess = GetCurrentProcess());

/*!
 * @brief Initialize debugging symbols
 * @note Call after the CommandLine has initialized, as it uses the exe path to initialize symbols.
 */
void LoadSymbols();

/*!
 * @brief Unload symbols
 */
void UnloadSymbols();
#endif // NOTSA_NATIVE
};
};
