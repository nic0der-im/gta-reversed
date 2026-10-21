#pragma once

#include <cstddef>
#include <cstdint>

namespace notsa::native {
/*!
* @brief Backing storage for the game executable's globals in the native build.
*
* Every `StaticRef` in the native build gets zero-initialized storage from here, mimicking
* the executable's `.data` section before its CRT static initialization runs. The real
* initializers are implemented separately, by `ApplyExeData()`.
*
* Storage is reserved per requested address, as before. An initializer write may reserve more
* than a later `StaticRef` type asks for (the buffer then grows, keeping its contents), and
* `ReportOverlaps` lists the addresses where several types alias the same executable data.
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
    * @brief Make sure at least `size` bytes at `addr` are reserved and return them.
    */
    static void* Ensure(uintptr_t addr, size_t size);

    /*!
    * @brief Copy `size` bytes of `data` into the executable global at `addr`.
    */
    static void Write(uintptr_t addr, const void* data, size_t size);

    /*!
    * @brief Zero `size` bytes at `addr`.
    */
    static void ZeroFill(uintptr_t addr, size_t size);

    /*!
    * @brief Log a warning for every pair of allocated globals whose `[addr, addr + size)` ranges overlap.
    *
    * Overlapping requests mean two unrelated `StaticRef` types alias the same executable data,
    * which has to be resolved by using a single reference to the containing type.
    */
    static void ReportOverlaps();
};
} // namespace notsa::native
