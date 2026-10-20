/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "app/app_debug.h"
#include <rw/rwplcore.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include <ranges>
namespace rng = std::ranges;
namespace rngv = std::views;

#include <filesystem>
namespace fs = std::filesystem;

#define PLUGIN_API

#ifdef NOTSA_NATIVE
//! Native builds are 64 bit, so the original 32 bit layouts don't hold anymore.
//! Use `VALIDATE_SIZE_ALWAYS` for structures that must keep their PC layout in every
//! build (i.e. the ones written verbatim into save games).
    #define VALIDATE_SIZE(struc, size)
    #define VALIDATE_OFFSET(struc, member, offset)
#else
    #define VALIDATE_SIZE(struc, size) static_assert(sizeof(struc) == size, "Invalid structure size of " #struc)
    #define VALIDATE_OFFSET(struc, member, offset) \
        static_assert(offsetof(struc, member) == offset, "The offset of " #member " in " #struc " is not " #offset "...")
#endif
#define VALIDATE_SIZE_ALWAYS(struc, size) static_assert(sizeof(struc) == size, "Invalid structure size of " #struc)

VALIDATE_SIZE(bool, 1);
VALIDATE_SIZE(char, 1);
VALIDATE_SIZE(short, 2);
VALIDATE_SIZE(int, 4);
VALIDATE_SIZE(float, 4);
VALIDATE_SIZE(long long, 8);

// Use this to add const that wasn't there in the original code
#define Const const

// Basic types for structures describing
typedef int8_t    int8;
typedef int16_t   int16;
typedef int32_t   int32;
typedef int64_t   int64;
typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint64_t  uint64;
typedef intptr_t  intptr;
typedef uintptr_t uintptr;

typedef uint8     bool8;
typedef uint16    bool16;
typedef uint32    bool32;

#if (defined(__GNUC__) || defined(__GNUG__) || defined(__clang__))
#define UNREACHABLE_INTRINSIC(...) __builtin_unreachable()
#elif (defined(_MSC_VER))
#define UNREACHABLE_INTRINSIC(...) __assume(false)
#else
#define UNREACHABLE_INTRINSIC(...) assert(false)
#endif

namespace notsa {
/*!
* @return Return source code's base path
*/
fs::path GetSourceCodeBasePath();
};

//! Cause a debug break. Native builds have no `__debugbreak`, but clang has a builtin.
#ifdef NOTSA_NATIVE
    #define NOTSA_DEBUG_BREAK_INTRINSIC() __builtin_debugtrap()
#else
    #define NOTSA_DEBUG_BREAK_INTRINSIC() __debugbreak()
#endif

// Use the `NOTSA_UNREACHABLE` macro for unreachable code paths.
// In debug mode it will do a DebugBreak() and print a message to the console,
// while in release code it'll be optimized away (by using special compiler directives)
// Also serves as a way to supress compiler warnings (for example, when you don't have need a `default` case in a `switch`)
#if _DEBUG
#include <format>
#ifndef NOTSA_NATIVE
    #include <winuser.h>
#endif

namespace notsa {
template<typename... Ts>
[[noreturn]] static void unreachable(std::string_view method, std::string_view file, unsigned line, std::string userDetails = "<None provided>") {
    const auto mbMsg = std::format(
        "File: {}\nIn: {}:{}\n\nDetails:\n{}",
        fs::relative(file, GetSourceCodeBasePath()).string(),
        method,
        line,
        userDetails
    );

    spdlog::error(mbMsg);
    spdlog::dump_backtrace();
    spdlog::apply_all([](std::shared_ptr<spdlog::logger> l) { // Flush all sinks immidiately
        l->flush();
    });

#ifdef NOTSA_NATIVE
    // Native platforms have no message box here (`Base.h` must stay free of SDL):
    // The message is already in the log, so just break/abort.
    NOTSA_DEBUG_BREAK_INTRINSIC();
    std::abort();
#else
    const auto result = MessageBox(
        NULL,
        mbMsg.c_str(),
        "Unreachable code reached! SEND HALP IMMEDIATELY!",
        MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND
    );

    switch (result) {
    case IDRETRY: 
        NOTSA_DEBUG_BREAK_INTRINSIC();  // Cause a debug break 
        [[fallthrough]]; // If it's "Continue"'d fall to exit anyways, because function is no-return
    case IDIGNORE:
    case IDABORT:
    default:
        // This is here because:
        // - The compiler will optimize unreachable code away in release builds
        // - No warnings will be generated for missing `return`s
        // - If this function is ever called we're supposed to abort
        exit(3);
    }
#endif
}
};
// TODO/NOTE: We might need to manually suppress warnings here?
// Since all the code here is perfectly valid, so the compiler might
// still complain that, for example, the function doesn't return on all code paths, etc
#define IMPL_NOTSA_UNREACHABLE_FMT_ARGS(...) std::format(__VA_ARGS__)
#define NOTSA_UNREACHABLE(...) do { ::notsa::unreachable(__FUNCTION__, __FILE__, __LINE__ __VA_OPT__(,IMPL_NOTSA_UNREACHABLE_FMT_ARGS(__VA_ARGS__))); } while (false)
#else 
#define NOTSA_UNREACHABLE(...) UNREACHABLE_INTRINSIC()
#endif
#define NOTSA_UNUSED_FUNCTION() NOTSA_UNREACHABLE("Unused Function")
#define NOTSA_UNREACHABLE_CASE(val) NOTSA_UNREACHABLE("Unreachable switch case with value: {}", val)

#ifdef _DEBUG
#define NOTSA_DEBUG_BREAK() NOTSA_DEBUG_BREAK_INTRINSIC()
#else
#define NOTSA_DEBUG_BREAK()
#endif

// In order to be able to get the vtable address using GetProcAddress
// the whole class must be exported. (Along which the vtable is exported as well)
// See `ReversibleHooks::detail::GetClassVTableAddress`
// This should be added to every and all class with a vtable
// (Only meaningful for the Windows `.asi` build - Native builds don't export anything)
#ifdef NOTSA_NATIVE
    #define NOTSA_EXPORT_VTABLE
#else
    #define NOTSA_EXPORT_VTABLE __declspec(dllexport)
#endif

// Macro for unused function arguments - Use it to avoid compiler warnings of unused arguments
#define UNUSED(x) (void)(x);

// Macro for unused function return values.
// Eventually could instead verify the returned value? In case of `sscanf` etc...
#define RET_IGNORED(x) (void)(x);

//! Cause a debug break
#define NOTSA_DEBUGBREAK() NOTSA_DEBUG_BREAK_INTRINSIC()

//! switch case fallthru
#define NOTSA_SWCFALLTHRU [[fallthrough]]

//! Macro for passing a string var to *scanf_s function.
//! The secure variant takes the buffer's size right after the buffer - Plain `sscanf`
//! (`crt_compat.h`) doesn't.
#ifdef NOTSA_NATIVE
    #define SCANF_S_STR(s) s
#else
    #define SCANF_S_STR(s) s, std::size(s)
#endif

#ifdef NOTSA_NATIVE
    #define NOTSA_FORCEINLINE inline __attribute__((always_inline))
#else
    #define NOTSA_FORCEINLINE __forceinline
#endif

#ifdef NOTSA_NATIVE
#include "app/platform/native/ExeGlobals.h"
#endif

/*!
* @brief Used for static variable references
*
* @tparam T    The type of the variable
* @param Addr  The address of it
*/
template<typename T>
T& StaticRef(uintptr addr) {
#ifdef NOTSA_STANDALONE_DUMP_HOOKS_ONLY
    // NOTE/BUG:
    // In NOTSA_STANDALONE_DUMP_HOOKS_ONLY, StaticRef() returns a single per-type static buffer for all addresses.
    // That aliases unrelated globals of the same type (e.g., many StaticRef<int32>(...)), so writes intended for one address will overwrite the dummy storage for another.
    // This can corrupt state during hook registration and make dump output unreliable/non-deterministic.
    // It can be easily fixed by putting the address in the template too, but we're not doing that yet because I guess it would impact compile times + it'd be a big diff in terms of code for now
    alignas(alignof(T)) static uint8 buf[sizeof(T)]{};
    return *reinterpret_cast<T*>(buf);
#elif defined(NOTSA_NATIVE)
    // The executable's globals are emulated by zero-initialized storage (mimicking `.bss` before the exe's CRT init)
    return *static_cast<T*>(notsa::native::ExeGlobals::Get(addr, sizeof(T), alignof(T)));
#else
    return *reinterpret_cast<T*>(addr);
#endif
}

/*!
 * @brief Use for scoped static variables (That is, static variables that are initialized in functions)
 * @brief See `CAEGlobalWeaponAudioEntity::ServiceAmbientGunFire` for examples)
 * @tparam T The type of the var
 * @param varAddr 
 * @param flagsAddr 
 * @param flagsMask 
 * @param initVal 
 * @return 
 */
template<typename T>
T& ScopedStaticRef(uintptr varAddr, uintptr flagsAddr, uint32 flagsMask, T&& initVal) {
    auto& var   = StaticRef<T>(varAddr);
    auto& flags = StaticRef<uint32>(flagsAddr);
    if (!(flags & flagsMask)) {
        flags |= flagsMask;
        var    = initVal;
    }
    return var;
}

template<typename T>
void SAFE_RELEASE(T*& ptr) { // DirectX stuff `Release()`
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

// std::format support for enums
// either using `EnumToString`, or using the enum name and value as a fallback
template<typename Enum>
    requires std::is_enum_v<Enum>
struct std::formatter<Enum> : std::formatter<std::string> {
    auto format(Enum e, format_context& ctx) const {
        if constexpr (requires { EnumToString(e); }) {
            if (const auto name = EnumToString(e)) {
                return formatter<string>::format(*name, ctx);
            }
        }
        return formatter<string>::format(
            std::format("{} ({})", typeid(Enum).name(), static_cast<std::underlying_type_t<Enum>>(e)),
            ctx
        );
    }
};

#define _IGNORED_
#define _CAN_BE_NULL_

// TODO: Use premake/cmake for this instead of relaying on `_DEBUG`
#ifdef _DEBUG
#define NOTSA_DEBUG 1
#endif

#if (defined(__GNUC__) || defined(__GNUG__) || defined(__clang__))
#define PLUGIN_SOURCE_FILE // No `init_seg` equivalent - See `init_priority` on the variables that need it
#define PLUGIN_VARIABLE
#define _NOINLINE_ __attribute__((noinline))
#elif (defined(_MSC_VER))
#define PLUGIN_SOURCE_FILE  __pragma(init_seg(lib))
#define PLUGIN_VARIABLE
#define _NOINLINE_ __declspec(noinline)
#else
#define PLUGIN_SOURCE_FILE
#define PLUGIN_VARIABLE
#define _NOINLINE_
#endif

// III/VC char > wchar_t string conversion
#define _SWSTRING_INIT(str, id) std::wstring my_ws##id; for (size_t i = 0; i < strlen(str); i++) my_ws##id += str[i]
#define _SWSTRING(id) const_cast<wchar_t *>(my_ws##id.c_str())
#define _SWSTRING_STATIC_INIT(id) static wchar_t my_ws##id[512] ; my_ws##id[0] = 0
#define _SWSTRING_STATIC(id) my_ws##id
#define _SWSTRING_STATIC_FROM(id, src) for (size_t i = 0; i < strlen(src); i++) my_ws##id[i] = src[i]
#define _SWSTRING_STATIC_TO(id, dst) for (size_t i = 0; i < wcslen(my_ws##id); i++) dst[i] = static_cast<char>(my_ws##id[i])

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RwRGBAReal, red, blue, green, alpha);
