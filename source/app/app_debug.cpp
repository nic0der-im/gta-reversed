#include "StdInc.h"

#include "app_debug.h"
#ifndef NOTSA_NATIVE
#include <windows.h>
#endif
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define FINAL 0

#ifndef FINAL
RtCharset* debugCharset;
bool g_bDebugRenderGroups;
#endif

#if !defined(FINAL)
static bool charsetOpen;
void OpenCharsetSafe() {
    if (!charsetOpen)
        RtCharsetOpen();
    charsetOpen = true;
}
#endif

// 0x734610
void CreateDebugFont() {
#ifndef FINAL
    RwRGBA color = { 255, 255, 128, 255 };
    RwRGBA colorbg = { 0, 0, 0, 0 };
    OpenCharsetSafe();
    debugCharset = RtCharsetCreate(&color, &colorbg);
#endif
}

// 0x734620
void DestroyDebugFont() {
#ifndef FINAL
    RtCharsetDestroy(debugCharset);
    RtCharsetClose();
    charsetOpen = false;
#endif
}

// unused
// 0x734630
void ObrsPrintfString(const char* str, int16 x, int16 y) {
#ifndef FINAL
    RtCharsetPrintBuffered(debugCharset, str, x * 8, y * 16, true);
#endif
}

// 0x734640
void FlushObrsPrintfs() {
#ifndef FINAL
    RtCharsetBufferFlush();
#endif
}

notsa::Logging::Logging() {
    using namespace std::chrono_literals;
#if 0
    while (!IsDebuggerPresent()) {
        Sleep(1);
    }
#endif
    // See https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
    spdlog::set_pattern("%^[%l][%H:%M:%S.%e][%s:%#]: %v%$");
    spdlog::enable_backtrace(128);
    spdlog::set_level(spdlog::level::debug);

#ifdef NOTSA_NATIVE
    // Native builds run from a terminal - the working directory is the game directory
    m_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.log"));
#else
    // Next to the exe, not in the working directory (that's `scripts/` while the ASI loader loads us)
    const auto exeDir = [] {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, std::size(path));
        return std::filesystem::path{ path }.parent_path();
    }();
    std::error_code ec; // The sink throws if the directory is missing (e.g. on a fresh install)
    fs::create_directory(exeDir / "logs", ec);
    m_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>((exeDir / "logs" / "log.log").string()));
#endif
    m_sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    spdlog::set_default_logger(Create("default"));
#ifdef NOTSA_NATIVE
    spdlog::flush_every(100ms);
#else
    // Wine deadlocks if DllMain spawns threads, so don't use flush_every (thread) there: flush each message synchronously instead.
    if (GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version")) {
        spdlog::flush_on(spdlog::level::trace);
    } else {
        spdlog::flush_every(100ms);
    }
#endif
}

notsa::Logging::~Logging() {
    spdlog::shutdown();
}

auto notsa::Logging::Create(std::string name, std::optional<spdlog::level::level_enum> level) -> notsa::log_ptr {
    auto logger = std::make_shared<spdlog::logger>(name, m_sinks.begin(), m_sinks.end());
    spdlog::initialize_logger(logger);
    if (level.has_value()) {
        logger->set_level(*level);
    }
    return logger;
}
