#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "NativePath.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

namespace {
std::string ToLower(std::string str) {
    for (auto& c : str) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return str;
}

//! @return The name of the entry in `dir` that matches `component` case-insensitively, or an empty string.
std::string ResolveComponent(const fs::path& dir, std::string_view component) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        return {};
    }

    const auto lower = ToLower(std::string{component});
    for (fs::directory_iterator it{dir, ec}, end; !ec && it != end; it.increment(ec)) {
        const auto name = it->path().filename().string();
        if (ToLower(name) == lower) {
            return name;
        }
    }

    return {};
}
} // namespace

std::string notsa::native::NormalizePath(std::string_view path) {
    if (path.empty()) {
        return {};
    }

    std::string normalized{path};
    // The game's line readers turn CR/LF/commas into spaces and only trim the front,
    // so paths routinely arrive with trailing blanks. Windows' file APIs ignore them;
    // POSIX does not.
    while (!normalized.empty() && (normalized.back() == ' ' || normalized.back() == '\t')) {
        normalized.pop_back();
    }
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    std::error_code ec;
    if (fs::exists(normalized, ec) && !ec) {
        return normalized;
    }

    // Path doesn't exist as-is: resolve each component against its parent's listing.
    fs::path result;
    size_t   pos = 0;
    if (normalized.front() == '/') {
        result = "/";
        pos    = 1;
    }

    while (pos <= normalized.size()) {
        const auto slash      = normalized.find('/', pos);
        const auto component  = normalized.substr(pos, slash == std::string::npos ? std::string::npos : slash - pos);
        if (!component.empty() && component != ".") {
            if (component == "..") {
                result /= "..";
            } else {
                const auto actual = ResolveComponent(result.empty() ? fs::path{"."} : result, component);
                result /= actual.empty() ? component : actual;
            }
        }

        if (slash == std::string::npos) {
            break;
        }
        pos = slash + 1;
    }

    return result.generic_string();
}

#endif // NOTSA_NATIVE
