#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "NativeGameDir.h"
#include "NativePath.h"

#include <SDL3/SDL.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace fs = std::filesystem;

namespace {

constexpr auto APP_ORG              = "gta-reversed";
constexpr auto APP_NAME             = "gtasa";
constexpr auto GAME_DIR_FILE        = "gamedir.txt";
constexpr auto GAME_IMAGE           = "/models/gta3.img";
constexpr auto PICKER_TIMEOUT       = std::chrono::minutes{5};
constexpr auto PICKER_PUMP_INTERVAL = std::chrono::milliseconds{16};

void ShowError(std::string message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GTA: San Andreas", message.c_str(), nullptr);
}

//! @return Where the folder picker's answer is persisted. Empty when the host has no pref directory.
fs::path SavedGameDirFile() {
    char* prefPath = SDL_GetPrefPath(APP_ORG, APP_NAME);
    if (!prefPath) {
        return {};
    }
    const fs::path dir{prefPath};
    SDL_free(prefPath); // `SDL_GetPrefPath` hands over a copy the caller owns
    return dir / GAME_DIR_FILE;
}

//! @return The directory saved by an earlier run, or an empty string when there's none.
std::string LoadSavedGameDir() {
    const auto file = SavedGameDirFile();
    if (file.empty()) {
        return {};
    }

    std::ifstream in{file, std::ios::binary};
    if (!in) {
        return {};
    }

    std::string line;
    std::getline(in, line);

    // Tolerate stray blanks / CRLF from hand-edited files
    const auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = line.find_last_not_of(" \t\r\n");
    return line.substr(first, last - first + 1);
}

void SaveGameDir(const std::string& dir) {
    const auto file = SavedGameDirFile();
    if (file.empty()) {
        return;
    }

    std::ofstream out{file, std::ios::binary | std::ios::trunc};
    if (!out) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to save the game directory to `%s`", file.string().c_str());
        return;
    }
    out << dir << '\n';
}

//! @return The normalized directory when it contains `models/gta3.img`, otherwise nothing.
std::optional<std::string> ValidateGameDir(std::string_view dir) {
    if (dir.empty()) {
        return std::nullopt;
    }

    auto normalized = notsa::native::NormalizePath(dir);
    std::error_code ec;
    if (!fs::is_regular_file(notsa::native::NormalizePath(normalized + GAME_IMAGE), ec) || ec) {
        return std::nullopt;
    }
    return normalized;
}

struct FolderPickerResult {
    std::mutex  mutex;
    std::string path;
    bool        done = false;
};

void SDLCALL OnFolderPicked(void* userdata, const char* const* filelist, int /*filter*/) {
    auto& result = *static_cast<FolderPickerResult*>(userdata);

    std::lock_guard lock{result.mutex};
    // `filelist` is NULL on error, and points at a NULL entry when the user cancelled.
    if (filelist && filelist[0]) {
        result.path = filelist[0];
    } else if (!filelist) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "The folder picker failed: %s", SDL_GetError());
    }
    result.done = true;
}

/*!
* @brief Run the native folder picker and wait for its answer.
*
* The dialog is asynchronous (and may call back on another thread), so the answer is polled while
* pumping events. On macOS the dialog is modal and already answered once
* `SDL_ShowOpenFolderDialog` returns, so the loop exits on its first iteration.
*
* Requires the video subsystem to be initialized.
*/
std::string PickGameDir() {
    FolderPickerResult result;
    SDL_ShowOpenFolderDialog(OnFolderPicked, &result, nullptr, nullptr, false);

    const auto deadline = std::chrono::steady_clock::now() + PICKER_TIMEOUT;
    while (true) {
        {
            std::lock_guard lock{result.mutex};
            if (result.done) {
                return result.path;
            }
        }

        // Some backends (the Linux XDG portal) only run the dialog from an event loop.
        SDL_PumpEvents();
        if (std::chrono::steady_clock::now() >= deadline) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "The folder picker did not answer within 5 minutes");
            return {};
        }
        std::this_thread::sleep_for(PICKER_PUMP_INTERVAL);
    }
}

} // namespace

std::string notsa::native::NativeGameDir::Resolve(int argc, char** argv) {
    // 1. `--game-dir <path>`, then 2. `GTASA_DIR`. Both are explicit, so a wrong value is reported
    //    instead of silently falling through to the picker.
    std::string explicitDir;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--game-dir") == 0 && i + 1 < argc) {
            explicitDir = argv[++i];
        }
    }
    if (explicitDir.empty()) {
        if (const char* envDir = std::getenv("GTASA_DIR")) {
            explicitDir = envDir;
        }
    }
    if (!explicitDir.empty()) {
        if (auto dir = ValidateGameDir(explicitDir)) {
            return *dir;
        }
        ShowError(std::format("`{}/models/gta3.img` not found", notsa::native::NormalizePath(explicitDir)));
        return {};
    }

    // 3. The directory chosen on a previous run.
    if (const auto savedDir = LoadSavedGameDir(); !savedDir.empty()) {
        if (auto dir = ValidateGameDir(savedDir)) {
            return *dir;
        }
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "The saved game directory `%s` doesn't contain `models/gta3.img`; asking again", savedDir.c_str());
    }

    // 4. Ask the user. The picker needs the video subsystem, which `main` would initialize only
    //    later, so bring it up here.
    if (!SDL_WasInit(SDL_INIT_VIDEO) && !SDL_Init(SDL_INIT_VIDEO)) {
        ShowError(std::format("Failed to initialize SDL video for the folder picker: {}", SDL_GetError()));
        return {};
    }

    const auto picked = PickGameDir();
    if (picked.empty()) {
        ShowError("Pass --game-dir or set GTASA_DIR");
        return {};
    }

    auto dir = ValidateGameDir(picked);
    if (!dir) {
        ShowError(std::format("`{}/models/gta3.img` not found", notsa::native::NormalizePath(picked)));
        return {};
    }
    SaveGameDir(*dir);
    return *dir;
}

#endif // NOTSA_NATIVE
