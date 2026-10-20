#pragma once

/*
 * Replacements for the MSVC CRT extensions (`*_s`, `_snprintf`, `_stricmp`, ...) that
 * the code base uses. Included by `nativeincl.h`, so it's only ever seen by native
 * builds - The MSVC implementations are left untouched for the Windows `.asi`.
 *
 * The semantics mirror the MSVC ones closely enough for the call sites in this code
 * base: on error the destination is empty/untouched and a non-zero `errno_t` is
 * returned, never a buffer overflow.
 */

#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <unistd.h>

using errno_t = int;

//! Case insensitive comparisons
#define _stricmp  strcasecmp
#define _strnicmp strncasecmp

//! `_snprintf` is MSVC's spelling of `snprintf` (both take the buffer size)
#define _snprintf std::snprintf

//! `sscanf_s` is plain `sscanf` - MSVC's size arguments for `%s` are dropped with
//! `SCANF_S_STR` (See `Base.h`)
#define sscanf_s sscanf

//! `_chdir`/`_getcwd` are the POSIX ones with an underscore prefix
#define _chdir  chdir
#define _getcwd getcwd

namespace notsa::native::crt {

inline size_t StrNLen(const char* str, size_t maxLen) {
    size_t len = 0;
    while (len < maxLen && str[len]) {
        len++;
    }
    return len;
}

inline errno_t Copy(char* dst, size_t dstSize, const char* src) {
    if (!dst || !dstSize) {
        return EINVAL;
    }
    if (!src) {
        dst[0] = '\0';
        return EINVAL;
    }
    const auto srcLen = std::strlen(src);
    if (srcLen >= dstSize) {
        dst[0] = '\0';
        return ERANGE;
    }
    std::memcpy(dst, src, srcLen + 1);
    return 0;
}

inline errno_t CopyN(char* dst, size_t dstSize, const char* src, size_t count) {
    if (!dst || !dstSize) {
        return EINVAL;
    }
    if (!src) {
        dst[0] = '\0';
        return EINVAL;
    }
    const auto srcLen = StrNLen(src, count);
    if (srcLen >= dstSize) {
        dst[0] = '\0';
        return ERANGE;
    }
    std::memcpy(dst, src, srcLen);
    dst[srcLen] = '\0';
    return 0;
}

inline errno_t Cat(char* dst, size_t dstSize, const char* src) {
    if (!dst || !dstSize) {
        return EINVAL;
    }
    if (!src) {
        return EINVAL;
    }
    const auto dstLen = StrNLen(dst, dstSize);
    if (dstLen == dstSize) { // Not null terminated
        return EINVAL;
    }
    return Copy(dst + dstLen, dstSize - dstLen, src);
}

inline errno_t Open(FILE** file, const char* path, const char* mode) {
    if (!file) {
        return EINVAL;
    }
    *file = std::fopen(path, mode);
    return *file ? 0 : errno;
}

inline int Print(char* dst, size_t dstSize, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const auto result = std::vsnprintf(dst, dstSize, fmt, args);
    va_end(args);
    return result < 0 ? -1 : result;
}

} // namespace notsa::native::crt

template<size_t N>
inline errno_t strcpy_s(char (&dst)[N], const char* src) {
    return notsa::native::crt::Copy(dst, N, src);
}

inline errno_t strcpy_s(char* dst, size_t dstSize, const char* src) {
    return notsa::native::crt::Copy(dst, dstSize, src);
}

template<size_t N>
inline errno_t strcat_s(char (&dst)[N], const char* src) {
    return notsa::native::crt::Cat(dst, N, src);
}

inline errno_t strcat_s(char* dst, size_t dstSize, const char* src) {
    return notsa::native::crt::Cat(dst, dstSize, src);
}

template<size_t N>
inline errno_t strncpy_s(char (&dst)[N], const char* src, size_t count) {
    return notsa::native::crt::CopyN(dst, N, src, count);
}

inline errno_t strncpy_s(char* dst, size_t dstSize, const char* src, size_t count) {
    return notsa::native::crt::CopyN(dst, dstSize, src, count);
}

template<size_t N, typename... Ts>
inline int sprintf_s(char (&dst)[N], const char* fmt, Ts... args) {
    return std::snprintf(dst, N, fmt, args...);
}

inline int sprintf_s(char* dst, size_t dstSize, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const auto result = std::vsnprintf(dst, dstSize, fmt, args);
    va_end(args);
    return result < 0 ? -1 : result;
}

inline errno_t fopen_s(FILE** file, const char* path, const char* mode) {
    return notsa::native::crt::Open(file, path, mode);
}
