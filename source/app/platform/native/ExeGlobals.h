#pragma once

#include <cstddef>
#include <cstdint>

namespace notsa::native {
/*!
* @brief Backing storage for the game executable's globals in the native build.
*
* Every `StaticRef` in the native build gets zero-initialized storage from here, mimicking
* the executable's `.bss` before its CRT static initialization runs. The real initializers
* are implemented separately (`RunExeStaticInitializers`).
*/
struct ExeGlobals {
    /*!
    * @brief Get (allocating on first request) the storage for the executable global at `addr`.
    * @param addr  The address of the global in the game executable.
    * @param size  The size in bytes of the requested type.
    * @param align The alignment in bytes of the requested type.
    * @return Zero-initialized storage of at least `size` bytes.
    */
    static void* Get(uintptr_t addr, size_t size, size_t align);

    /*!
    * @brief Log a warning for every pair of allocated globals whose `[addr, addr + size)` ranges overlap.
    *
    * Overlapping requests mean two unrelated `StaticRef` types alias the same executable data,
    * which has to be resolved by using a single reference to the containing type.
    */
    static void ReportOverlaps();
};
} // namespace notsa::native
