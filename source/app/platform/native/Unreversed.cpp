#include "Unreversed.h"

#include <cstdlib>

#include <spdlog/spdlog.h>

namespace notsa::native {
void UnreversedCall(uintptr_t addr, const char* kind) {
    spdlog::critical("Unreversed exe {} 0x{:X} called", kind, addr);
    spdlog::shutdown();
    std::abort();
}
} // namespace notsa::native
