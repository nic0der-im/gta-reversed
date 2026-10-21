#pragma once

#ifdef NOTSA_NATIVE

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
} // namespace notsa::native

#endif // NOTSA_NATIVE
