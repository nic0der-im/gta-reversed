#pragma once

#include <string>
#include <string_view>

namespace notsa::native {
/*!
* @brief Translate a (Windows style) game path into a path usable on the host filesystem.
*
* Backslashes are turned into slashes. If the resulting path doesn't exist, each path component
* is resolved case-insensitively against the parent directory listing, because the game data
* ships with hard-coded casing while host filesystems (or the user's copy of the data) may differ.
* When a component can't be resolved, it is kept as-is - the caller will then fail the same way
* it would on Windows.
*
* @param path The path to translate (UTF-8).
* @return The translated path (UTF-8).
*/
std::string NormalizePath(std::string_view path);
} // namespace notsa::native
