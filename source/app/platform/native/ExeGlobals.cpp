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
} // namespace

void* ExeGlobals::Get(uintptr_t addr, size_t size, size_t align) {
    auto& registry = GetRegistry();
    std::lock_guard lock{registry.mutex};

    if (const auto it = registry.entries.find(addr); it != registry.entries.end()) {
        const auto& [ptr, sizeInUse] = it->second;
        if (sizeInUse < size) {
            spdlog::critical("StaticRef 0x{:X} requested with sizes {} and {}", addr, sizeInUse, size);
            spdlog::shutdown();
            std::abort();
        }
        if (sizeInUse != size) {
            spdlog::warn("StaticRef 0x{:X} requested with sizes {} and {}", addr, sizeInUse, size);
        }
        return ptr;
    }

    auto* ptr = ::operator new(size, std::align_val_t{align});
    std::memset(ptr, 0, size);
    registry.entries.emplace(addr, Entry{ptr, size});
    return ptr;
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
