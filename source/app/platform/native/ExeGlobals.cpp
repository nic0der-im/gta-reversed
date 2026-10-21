#include "ExeGlobals.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

#ifdef NOTSA_NATIVE
// This file is in the source list of every platform (the native directory is never filtered out),
// and only the native build replays the executable's initializers.
#include "app/platform/native/ExeData.manual.h"
#endif

namespace notsa::native {
namespace {
struct Entry {
    void*  ptr;
    size_t size;
    size_t align;
    //! Whether the allocation was created by a typed `StaticRef` request (as opposed to an
    //! initializer write, whose size describes the executable's layout of the same global).
    bool   typed;
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
* @param typed Whether the request comes from a typed `StaticRef` rather than an initializer write.
*
* Growing replaces the buffer, so a pointer obtained from `Get` before the growth is gone.
* That is acceptable because growth only happens while `ApplyExeData` writes the executable's
* static initializers (before the game asks for any of that storage), and because `StaticRef`
* resolves its storage on every access.
*/
void* Acquire(Registry& registry, uintptr_t addr, size_t size, size_t align, bool typed) {
    const auto it = registry.entries.find(addr);
    if (it == registry.entries.end()) {
        auto* ptr = ::operator new(size, std::align_val_t{align});
        std::memset(ptr, 0, size);
        registry.entries.emplace(addr, Entry{ptr, size, align, typed});
        return ptr;
    }

    auto& entry = it->second;
    if (entry.size < size) {
        // An initializer wrote part of this object before anything asked for all of it.
        auto* const grown = ::operator new(size, std::align_val_t{align});
        std::memset(grown, 0, size);
        std::memcpy(grown, entry.ptr, entry.size);
        ::operator delete(entry.ptr, std::align_val_t{entry.align});
        entry.ptr = grown;
        entry.size = size;
        entry.align = align;
    } else if (entry.size != size && typed && entry.typed) {
        // Two typed `StaticRef`s claim the same address with different sizes: a real layout clash.
        // A typed request next to an initializer write is not one - their sizes describe the same
        // global in two different layouts.
        spdlog::warn("StaticRef 0x{:X} requested with sizes {} and {}", addr, entry.size, size);
    }
    entry.typed |= typed;
    return entry.ptr;
}
} // namespace

void* ExeGlobals::Get(uintptr_t addr, size_t size, size_t align) {
    auto& registry = GetRegistry();
    std::lock_guard lock{registry.mutex};
    return Acquire(registry, addr, size, align, /*typed=*/true);
}

void* ExeGlobals::Ensure(uintptr_t addr, size_t size) {
    auto& registry = GetRegistry();
    std::lock_guard lock{registry.mutex};
    return Acquire(registry, addr, size, alignof(std::max_align_t), /*typed=*/false);
}

void ExeGlobals::Write(uintptr_t addr, const void* data, size_t size) {
#ifdef NOTSA_NATIVE
    // Rows whose native element is wider than the executable's stride cannot be replayed byte for
    // byte: the executable's offsets do not address the native members. Those rows are applied
    // through their typed container instead (outside the registry lock).
    if (ApplyManualExeDataPattern(addr, data, size)) {
        return;
    }
#endif
    std::memcpy(Ensure(addr, size), data, size);
}

void ExeGlobals::ZeroFill(uintptr_t addr, size_t size) {
#ifdef NOTSA_NATIVE
    if (ApplyManualExeDataZeroFill(addr, size)) {
        return;
    }
#endif
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
