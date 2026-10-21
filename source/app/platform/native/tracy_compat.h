#pragma once

/*
 * Tracy is a retail-exe-only dependency (see `conanfile.py` and `source/CMakeLists.txt`):
 * the native port has no profiler, so every Tracy macro used by the shared game code is
 * defined away here. `StdInc.h` includes this instead of `<tracy/Tracy.hpp>` when
 * `NOTSA_NATIVE` is defined.
 *
 * Only add a macro here when the shared code actually uses it - keeping the list short
 * makes it obvious what would have to be re-enabled if the native port ever grows a
 * profiler.
 */

#define ZoneScoped
#define ZoneScopedN(name)
#define ZoneText(txt, size)
#define FrameMark
