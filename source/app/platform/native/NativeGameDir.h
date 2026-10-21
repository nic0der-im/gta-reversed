#pragma once

#include <string>

namespace notsa::native {

/*!
* @brief Locates the game data directory (`models/gta3.img`, `data/`, `audio/`, ...).
*
* `Resolve` is the only entry point used by `NativeMain`. The directory is taken, in order, from:
*
* 1. `--game-dir <path>` on the command line.
* 2. The `GTASA_DIR` environment variable.
* 3. The path picked on an earlier run, stored in `gamedir.txt` inside
*    `SDL_GetPrefPath("gta-reversed", "gtasa")`.
* 4. A native folder picker (`SDL_ShowOpenFolderDialog`). A successful choice is written back to
*    `gamedir.txt`, so the next launch starts without asking.
*
* A candidate is accepted only when `<dir>/models/gta3.img` exists. Components are resolved
* case-insensitively (see `NormalizePath`), because the game data ships with Windows casing while
* macOS filesystems are case-insensitive but case-preserving.
*/
struct NativeGameDir {
    /*!
    * @brief Resolve the game directory, prompting the user (with an error box) when needed.
    *
    * @return The normalized, validated game directory, or an empty string when no usable
    *         directory could be determined. The caller should exit with code 1 in that case.
    */
    static std::string Resolve(int argc, char** argv);
};

} // namespace notsa::native
