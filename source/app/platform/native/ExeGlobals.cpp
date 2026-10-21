#include "ExeGlobals.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

namespace notsa::native {
namespace {
struct Entry {
    void*  ptr;
    size_t size;
    size_t align;
};

struct Registry {
    std::mutex                           mutex;
    std::unordered_map<uintptr_t, Entry> entries;
};

// Function-local static: avoids depending on static initialization order.
Registry& GetRegistry() {
    static Registry registry;
    return registry;
}

/*!
* @brief Reserve (or grow) the storage for `addr`, preserving whatever is already in it.
* @note  Caller must hold the registry mutex.
*
* Growing replaces the buffer, so a pointer obtained from `Get` before the growth is gone.
* That is acceptable because growth only happens while `ApplyExeData` writes the executable's
* static initializers (before the game asks for any of that storage), and because `StaticRef`
* resolves its storage on every access.
*/
void* Acquire(Registry& registry, uintptr_t addr, size_t size, size_t align) {
    const auto it = registry.entries.find(addr);
    if (it == registry.entries.end()) {
        auto* ptr = ::operator new(size, std::align_val_t{align});
        std::memset(ptr, 0, size);
        registry.entries.emplace(addr, Entry{ptr, size, align});
        return ptr;
    }

    auto& [ptr, sizeInUse, alignInUse] = it->second;
    if (sizeInUse < size) {
        // An initializer wrote part of this object before anything asked for all of it.
        auto* const grown = ::operator new(size, std::align_val_t{align});
        std::memset(grown, 0, size);
        std::memcpy(grown, ptr, sizeInUse);
        ::operator delete(ptr, std::align_val_t{alignInUse});
        ptr = grown;
        sizeInUse = size;
        alignInUse = align;
    } else if (sizeInUse != size) {
        // A larger type already owns the address: two `StaticRef`s alias the same global.
        spdlog::warn("StaticRef 0x{:X} requested with sizes {} and {}", addr, sizeInUse, size);
    }
    return ptr;
}
} // namespace

void* ExeGlobals::Get(uintptr_t addr, size_t size, size_t align) {
    auto& registry = GetRegistry();
    std::lock_guard lock{registry.mutex};
    return Acquire(registry, addr, size, align);
}

void* ExeGlobals::Ensure(uintptr_t addr, size_t size) {
    return Get(addr, size, alignof(std::max_align_t));
}

void ExeGlobals::Write(uintptr_t addr, const void* data, size_t size) {
    std::memcpy(Ensure(addr, size), data, size);
}

void ExeGlobals::ZeroFill(uintptr_t addr, size_t size) {
    std::memset(Ensure(addr, size), 0, size);
}

void ExeGlobals::ReportOverlaps() {
    auto& registry = GetRegistry();
    std::lock_guard lock{registry.mutex};

    std::vector<std::pair<uintptr_t, Entry>> sorted{registry.entries.begin(), registry.entries.end()};
    std::ranges::sort(sorted, {}, &std::pair<uintptr_t, Entry>::first);

    for (size_t i = 0; i < sorted.size(); i++) {
        const auto& [addr, entry] = sorted[i];
        const auto endAddr = addr + entry.size;
        for (size_t j = i + 1; j < sorted.size() && sorted[j].first < endAddr; j++) {
            spdlog::warn(
                "ExeGlobals overlap: 0x{:X} (size {}) overlaps 0x{:X} (size {})",
                addr, entry.size, sorted[j].first, sorted[j].second.size
            );
        }
    }
}
} // namespace notsa::native
