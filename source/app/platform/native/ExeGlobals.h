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
* Storage is reserved per requested address, as before. An initializer write may reserve less than
* a later `StaticRef` type asks for (the buffer then grows, keeping its contents), and
* `ReportOverlaps` lists the addresses where several types alias the same executable data.
*
* Sizes are only comparable between requests of the same kind: a typed `StaticRef` request
* describes the native object, while an initializer write describes the executable's bytes. The
* executable's element layout can be narrower than the native one (pointers widen), and a row can
* clear only part of an object, so a size difference between those two is expected and is not
* reported. A size difference between two *typed* requests at one address is a real layout clash
* and is logged.
*/
struct ExeGlobals {
    /*!
    * @brief Get (allocating on first request) the storage for the executable global at `addr`.
    *
    * Requests come from a typed `StaticRef`, so a different typed size at the same address is
    * reported as a layout clash.
    *
    * @param addr  The address of the global in the game executable.
    * @param size  The size in bytes of the requested type.
    * @param align The alignment in bytes of the requested type.
    * @return Zero-initialized storage of at least `size` bytes.
    */
    static void* Get(uintptr_t addr, size_t size, size_t align);

    /*!
    * @brief Make sure at least `size` bytes at `addr` are reserved and return them.
    *
    * Used by the executable's initializer replay, whose byte counts are expressed in the 32-bit
    * layout; a size different from an existing allocation is expected and is not reported.
    */
    static void* Ensure(uintptr_t addr, size_t size);

    /*!
    * @brief Copy `size` bytes of `data` into the executable global at `addr`.
    *
    * Writes that belong to a row whose native element layout differs from the executable's are
    * applied through the typed container instead (`ExeData.manual.cpp`).
    */
    static void Write(uintptr_t addr, const void* data, size_t size);

    /*!
    * @brief Zero `size` bytes at `addr`, same redirecting rules as `Write`.
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
