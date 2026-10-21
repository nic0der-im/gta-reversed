//! @file ExeData.generated.h
//! The executable's CRT static-initializer effects, ready to replay into `ExeGlobals`.
//!
//! GENERATED FILE - DO NOT EDIT. Regenerate with `python3 tools/native/gen_exedata.py`.
//! Source: the `__xi_a..__xi_z` table (`0x8A4000..0x8A5A10`, 1667 entries) of `gta_sa.exe`.

#pragma once

#ifdef NOTSA_NATIVE

#include <cstdint>
#include <span>

namespace notsa::native {

//! What one row of the generated table does. The meaning of the fields depends on the kind; see
//! the comment on each enumerator.
enum class ExeDataKind : std::uint8_t {
    StoreBytes,   //!< Write the low `Size` (1..8) bytes of `Value` (little endian) at `Addr`.
    StoreFloat,   //!< Write the 32-bit float `Value` at `Addr` (result of an x87 constant chain).
    ZeroFill,     //!< Write `Size` zero bytes at `Addr`.
    FillPattern,  //!< Write the low `Size` bytes of `Value` at `Addr + i * Aux`, for `i` < `Count`.
    VectorCtor,   //!< `_eh_vector_constructor_iterator`: `Count` elements of `Size` bytes at
                  //!< `Addr` constructed by `Aux` (a constructor function in gta_sa.exe).
    ObjectCtor,   //!< Construct the object at `Addr` with `Aux` (a constructor function in
                  //!< gta_sa.exe). The pushed arguments are recorded in the table comment.
    Skip,         //!< No effect in the native build (`_atexit`-only, or the writes are already
                  //!< zero). `Addr` is the initializer's entry point, for traceability.
    Unhandled,    //!< Not applied. `Addr` is the initializer's entry point. Either
                  //!< `ApplyManualExeDataEntry` handles it, or it is a known gap listed in
                  //!< `docs/NativePort.md` (pointer stores, CRT internals, non-constant maths).
};

//! One effect of the executable's static initialization.
struct ExeDataEntry {
    ExeDataKind Kind;     //!< What the row does.
    std::uintptr_t Addr;  //!< Destination address in the executable's `.data`.
    std::uintptr_t Aux;   //!< `FillPattern` stride / `VectorCtor`+`ObjectCtor` constructor.
    std::uint32_t Size;   //!< Payload size, element size (`VectorCtor`) or byte count (`ZeroFill`).
    std::uint32_t Count;  //!< Element repetitions (`FillPattern`, `VectorCtor`).
    std::uint64_t Value;  //!< Payload bytes, little endian.
};

//! Every row, in the order the executable's `_cinit` would have run the initializers.
std::span<const ExeDataEntry> GetExeDataEntries();

//! Replay the table into `ExeGlobals`, in table order. Called once from `main`.
void ApplyExeData();

} // namespace notsa::native

#endif // NOTSA_NATIVE
