#pragma once

#ifdef NOTSA_NATIVE

#include <cstddef>
#include <cstdint>

#include "app/platform/native/ExeData.generated.h"

namespace notsa::native {
/*!
* @brief Apply one `ObjectCtor`/`VectorCtor`/`Unhandled` row of the exe's initializer table by hand.
*
* The generated table cannot call into the executable, so constructing a static object of a type
* that is already reversed has to go through the typed `StaticRef` in `ExeData.manual.cpp`.
*
* @return `true` when the row was applied (or is known to be a no-op in the native build).
*/
bool ApplyManualExeDataEntry(const ExeDataEntry& entry);

/*!
* @brief Apply one `FillPattern` write of the exe's initializer table.
*
* A pattern row stores a value at `Addr + i * stride` in the executable's element layout. When the
* native element is wider than that stride (pointers widen), replaying the bytes would land on a
* different member - on the pointer fields of the neighbouring element, at worst - so the row is
* applied here, element by element, through the typed container.
*
* Rows whose native element has the same size as the executable's stride are still claimed; the
* typed member write is equivalent and keeps the rule independent of which layout happens to match.
*
* @param addr  Executable address of the written bytes (element `i`, offset inside the element).
* @param data  The bytes the executable writes there.
* @param size  The number of bytes written.
* @return `true` when a row owns `addr` and applied the write through its native type; the caller
*         must then skip the plain write.
*/
bool ApplyManualExeDataPattern(std::uintptr_t addr, const void* data, std::size_t size);

/*!
* @brief Apply one `ZeroFill` write of the exe's initializer table through the typed container.
*
* Clearing rows are applied as a whole object/array clear: the executable's zero byte count is
* expressed in its own element layout and cannot address the native members.
*
* @return `true` when a row owns `addr` and cleared its native type; the caller must then skip the
*         plain zero fill.
*/
bool ApplyManualExeDataZeroFill(std::uintptr_t addr, std::size_t size);
} // namespace notsa::native

#endif // NOTSA_NATIVE
