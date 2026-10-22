#include "StdInc.h"

#include <cstdlib>
#ifdef NOTSA_NATIVE
#include <mutex>
#include <unordered_map>
#endif

#ifdef MEMORY_MGR_USE_MEMORY_HEAP
static CRITICAL_SECTION   gstrucMemoryHeapCS;
static CRITICAL_SECTION   gstrucScratchPadCS;
static LPCRITICAL_SECTION gpMemoryHeapCS;
static LPCRITICAL_SECTION gpScratchPadCS;
#endif

#ifdef MEMORY_MGR_USE_MEMORY_HEAP
    #define LOCK_MEMORYHEAP()       EnterCriticalSection(gpMemoryHeapCS)
    #define UNLOCK_MEMORYHEAP()     LeaveCriticalSection(gpMemoryHeapCS)
    #define LOCK_SCRATCHPAD()       EnterCriticalSection(gpScratchPadCS)
    #define UNLOCK_SCRATCHPAD()     LeaveCriticalSection(gpScratchPadCS)
#else
    #define LOCK_MEMORYHEAP()
    #define UNLOCK_MEMORYHEAP()
    #define LOCK_SCRATCHPAD()
    #define UNLOCK_SCRATCHPAD()
#endif

#ifdef NOTSA_NATIVE
namespace {
struct LiveBlock {
    uint32 size{};
    int32  id{};
    bool   scratch{};
};

std::mutex                         g_blockMutex;
std::unordered_map<void*, LiveBlock> g_blocks;
uint32                             g_scratchUsed = 0;
int32                              g_trackingIds = 0;

bool IsScratchRange(const void* memory) {
    const auto* begin = static_cast<const char*>(static_cast<const void*>(PC_Scratch));
    const auto* p     = static_cast<const char*>(memory);
    return memory && p >= begin && p < begin + sizeof(PC_Scratch);
}

void AccountLocked(void* memory, uint32 size, int32 id, bool scratch) {
    g_blocks.insert_or_assign(memory, LiveBlock{ size, id, scratch });
    CMemoryMgr::m_memUsed += static_cast<int32>(size);
    CMemoryMgr::m_blocksUsed += 1;
    if (id >= 0 && id < g_trackingIds && CMemoryMgr::m_pMemUsedArray && CMemoryMgr::m_pBlockArray) {
        CMemoryMgr::m_pMemUsedArray[id] += static_cast<int32>(size);
        CMemoryMgr::m_pBlockArray[id] += 1;
    }
}

void UnaccountLocked(std::unordered_map<void*, LiveBlock>::iterator it) {
    const LiveBlock block = it->second;
    CMemoryMgr::m_memUsed -= static_cast<int32>(block.size);
    CMemoryMgr::m_blocksUsed -= 1;
    if (block.id >= 0 && block.id < g_trackingIds && CMemoryMgr::m_pMemUsedArray && CMemoryMgr::m_pBlockArray) {
        CMemoryMgr::m_pMemUsedArray[block.id] -= static_cast<int32>(block.size);
        CMemoryMgr::m_pBlockArray[block.id] -= 1;
    }
    g_blocks.erase(it);
}

void ResetScratchLocked() {
    for (auto it = g_blocks.begin(); it != g_blocks.end();) {
        if (!it->second.scratch) {
            ++it;
            continue;
        }
        auto dead = it++;
        UnaccountLocked(dead);
    }
    g_scratchUsed = 0;
}

bool OwnsLocked(void* memory) {
    return memory && g_blocks.find(memory) != g_blocks.end();
}
} // namespace
#endif

void CMemoryMgr::InjectHooks() {
    RH_ScopedClass(CMemoryMgr);
    RH_ScopedCategory("Memory");
    RH_ScopedInstall(InitScratchPad, 0x72F480, { .Reversed = false });
    RH_ScopedOverloadedInstall(Malloc, "", 0x72F420, void*(*)(uint32, uint32), { .Reversed = false });
    RH_ScopedInstall(Free, 0x72F430, { .Reversed = false });
    RH_ScopedOverloadedInstall(Realloc, "", 0x72F440, uint8*(*)(void*, uint32, uint32), { .Reversed = false });
    RH_ScopedOverloadedInstall(Calloc, "", 0x72F460, uint8*(*)(uint32, uint32, uint32), { .Reversed = false });
    RH_ScopedInstall(FreeAlign, 0x72F4F0, { .Reversed = false });
    RH_ScopedOverloadedInstall(MallocAlign, "", 0x72F4C0, void*(*)(uint32, uint32), { .Reversed = false });
}

// 0x72F3B0
void CMemoryMgr::Init() {
#ifdef NOTSA_NATIVE
    // Idempotent: a second call keeps the tracking arrays and only rewinds the scratch pad.
    if (!m_pMemUsedArray || !m_pBlockArray) {
        InitMemoryTracking(MEMORY_MGR_TRACKING_NUM_IDS);
    }
    InitScratchPad();
    return;
#endif
#ifdef MEMORY_MGR_USE_MEMORY_HEAP
    // init main heap
    g_Heaps[HEAP_PRIMARY].Init(malloc(_MEMORYMGR_HEAP_SIZE), _MEMORYMGR_HEAP_SIZE, false);

    // critial sections initialization
    InitializeCriticalSection(&_gstrucMemoryHeapCS);
    InitializeCriticalSection(&_gstrucScratchPadCS);
    gmemoryHeapCS = &_gstrucMemoryHeapCS;
    gpScratchPadCS = &_gstrucScratchPadCS;

    // init second heap
    g_Heaps[HEAP_SECONDARY].Init(g_Heaps[HEAP_PRIMARY].Malloc(2100 * 1024), 2100 * 1024, true);

    // init other stuff
    CMemoryMgr::InitMemoryTracking(_MEMORYMGR_TRACKING_NUM_IDS);
    CMemoryMgr::InitScratchPad();
#endif
}

// 0x72F8F0
void CMemoryMgr::InitMemoryTracking(int32 numIDs) {
#ifdef NOTSA_NATIVE
    g_trackingIds = numIDs;
#endif
    m_memUsed = 0;
    m_blocksUsed = 0;
    PushMemId(MEM_GAME);
    PushMemId(MEM_INTERNAL_OR_SYSTEM);
    m_pMemUsedArray = (int32*)Malloc(sizeof(int32) * numIDs);
    m_pBlockArray   = (int32*)Malloc(sizeof(int32) * numIDs);
    PopMemId();

    // zero block arrays
    for (int32 i = 0; i < numIDs; ++i) {
        m_pMemUsedArray[i] = 0;
        m_pBlockArray[i] = 0;
    }
}

void CMemoryMgr::Shutdown() {
#ifdef NOTSA_NATIVE
    int32* used = nullptr;
    int32* blocks = nullptr;
    {
        std::lock_guard lock{ g_blockMutex };
        g_scratchUsed = 0;
        g_blocks.clear();
        used = m_pMemUsedArray;
        blocks = m_pBlockArray;
        m_pMemUsedArray = nullptr;
        m_pBlockArray = nullptr;
        g_trackingIds = 0;
        m_memUsed = 0;
        m_blocksUsed = 0;
    }
    // User blocks stay with their owners. Only the tracking tables themselves are released.
    ::free(used);
    ::free(blocks);
    return;
#endif
#ifdef MEMORY_MGR_USE_MEMORY_HEAP
    g_Heaps[HEAP_SECONDARY].PrintMemoryLeaks();
    g_Heaps[HEAP_PRIMARY].Free(g_Heaps[HEAP_SECONDARY]._pFirstBlock);
    g_Heaps[HEAP_PRIMARY].Shutdown();
#endif
}

/**
 * @static
 * debug only shit!
 */
void CMemoryMgr::SetHint(void* memory, const char* hint) {
    if (!memory) {
        return;
    }

    for (auto& heap : g_Heaps) {
        if (!heap.IsHeapPointer(memory))
            continue;

        if (HeapBlockDesc* desc = heap._FindBlockDesc(memory)) {
            desc->m_upDebugInfo = (const uint8*)hint;
#if defined MEMORY_MGR_USE_HEAP_FLAGS
            desc->m_Flags.NoDebugHint     = (hint == nullptr);
            desc->m_Flags.StringDebugInfo = (hint != nullptr);
#endif
            return;
        }
    }
    //_FATAL("Not a heap pointer!");
}

// 0x72F480
void CMemoryMgr::InitScratchPad() {
#ifdef NOTSA_NATIVE
    std::lock_guard lock{ g_blockMutex };
    ResetScratchLocked();
#else
    return plugin::Call<0x72F480>();
#endif
}

void* CMemoryMgr::MallocFromScratchPad(uint32 size) {
#ifdef NOTSA_NATIVE
    // Bump allocator over PC_Scratch (0xC8E0C8, 16384 bytes). Lock/Release/Init rewind it.
    // Absolute 16-byte alignment matches the retail heap, even when the buffer itself is not.
    const uintptr_t base = reinterpret_cast<uintptr_t>(static_cast<void*>(PC_Scratch));
    const uintptr_t limit = base + sizeof(PC_Scratch);
    // A zero-byte request still consumes a slot so two calls don't alias one record.
    const uint32 bytes = size ? ((size + 15u) & ~15u) : 16u;
    void* memory = nullptr;
    uint32 used = 0;
    {
        std::lock_guard lock{ g_blockMutex };
        const uintptr_t start = (base + g_scratchUsed + 15u) & ~uintptr_t{ 15 };
        if (start >= limit || bytes > limit - start) {
            used = g_scratchUsed;
        } else {
            g_scratchUsed = static_cast<uint32>((start + bytes) - base);
            memory = reinterpret_cast<void*>(start);
            AccountLocked(memory, size, m_id, true);
        }
    }
    if (!memory) {
        NOTSA_LOG_WARN("CMemoryMgr scratch pad exhausted ({} requested, {} used of {})", size, used, sizeof(PC_Scratch));
        return nullptr;
    }
    return memory;
#else
    void* memory = g_Heaps[HEAP_SCRATCH].Malloc(size);
    RegisterMalloc(memory);
    return memory;
#endif
}

void* CMemoryMgr::MallocAlignFromScratchPad(uint32 size, uint32 align) {
    if (align < sizeof(void*)) {
        align = sizeof(void*);
    }
    void* memory = MallocFromScratchPad(size + align);
    if (!memory) {
        return nullptr;
    }

    void* result = reinterpret_cast<void*>((uintptr_t(memory) + align) & ~(uintptr_t(align) - 1));
    *(reinterpret_cast<void**>(result) - 1) = memory;
    return result;
}

void CMemoryMgr::LockScratchPad() {
#ifdef NOTSA_NATIVE
    std::lock_guard lock{ g_blockMutex };
    ResetScratchLocked();
#else
    LOCK_SCRATCHPAD();
#endif
}

void CMemoryMgr::ReleaseScratchPad() {
#ifdef NOTSA_NATIVE
    std::lock_guard lock{ g_blockMutex };
    ResetScratchLocked();
#else
    UNLOCK_SCRATCHPAD();
#endif
}

// 0x72F770
void CMemoryMgr::RegisterMalloc(void* memory) {
    int16 memoryId = GET_HEAP_DESC(memory)->m_nMemId;
    int32 size     = GET_HEAP_DESC(memory)->m_nSize + sizeof(HeapBlockDesc);

    // update global info
    m_memUsed    += size;
    m_blocksUsed += 1;

    // update tracking info
    if (memoryId != -1) {
        m_pMemUsedArray[memoryId] += size;
        m_pBlockArray[memoryId]   += 1;
    }
}

// 0x72F7C0
void CMemoryMgr::RegisterFree(void* memory) {
    if (!memory) {
        return;
    }

    int16 memoryId = GET_HEAP_DESC(memory)->m_nMemId;
    int32 size     = GET_HEAP_DESC(memory)->m_nSize + sizeof(HeapBlockDesc);

    // update global info
    m_memUsed    -= size;
    m_blocksUsed -= 1;

    // update tracking info
    if (memoryId != -1) {
        m_pMemUsedArray[memoryId] -= size;
        m_pBlockArray[memoryId]   -= 1;
    }
}

uint32 CMemoryMgr::GetSizeOfHoles() {
#ifdef NOTSA_NATIVE
    return 0;
#else
    return g_Heaps[HEAP_PRIMARY].GetSizeOfHoles() + g_Heaps[HEAP_SECONDARY].GetSizeOfHoles();
#endif
}

uint32 CMemoryMgr::GetMemoryUsed(eMemoryId id) {
#ifdef NOTSA_NATIVE
    if (!m_pMemUsedArray || id < 0 || id >= g_trackingIds) {
        return 0;
    }
    return static_cast<uint32>(m_pMemUsedArray[id]);
#else
    assert(m_pMemUsedArray);
    return m_pMemUsedArray[id];
#endif
}

uint32 CMemoryMgr::GetBlocksUsed(eMemoryId id) {
#ifdef NOTSA_NATIVE
    if (!m_pBlockArray || id < 0 || id >= g_trackingIds) {
        return 0;
    }
    return static_cast<uint32>(m_pBlockArray[id]);
#else
    assert(m_pBlockArray);
    return m_pBlockArray[id];
#endif
}

uint32 CMemoryMgr::GetLargestFreeBlock() {
#ifdef NOTSA_NATIVE
    // libc has no retail heap. Report the PC heap budget the game was built against.
    return MEMORY_MGR_PC_HEAP_SIZE;
#else
    return g_Heaps[HEAP_PRIMARY].GetLargestFreeBlock();
#endif
}

void* CMemoryMgr::InternalMalloc(uint32 size) {
    if (size <= 2048) {
        if (void* memory = g_Heaps[HEAP_SECONDARY].Malloc(size)) {
            return memory;
        }
    }
    return g_Heaps[HEAP_PRIMARY].Malloc(size);
}

void* CMemoryMgr::InternalRealloc(void* memory, uint32 size) {
    if (g_Heaps[HEAP_SCRATCH].IsHeapPointer(memory)) {
        return g_Heaps[HEAP_SCRATCH].Realloc(memory, size);
    }
    if (g_Heaps[HEAP_SECONDARY].IsHeapPointer(memory)) {
        if (size <= 2048)
            return g_Heaps[HEAP_SECONDARY].Realloc(memory, size);
    } else {
        if (size > 2048)
            return g_Heaps[HEAP_PRIMARY].Realloc(memory, size);
    }
    void* newMemory = InternalMalloc(size);
    if (newMemory) {
        uint32 nMinSize = (uint32)std::min(float(GET_HEAP_DESC(memory)->m_nSize), float(size));
        memcpy(newMemory, memory, nMinSize);
    }
    return newMemory;
}

// 0x72F420
void* CMemoryMgr::Malloc(uint32 size, uint32 nHint) {
#ifdef NOTSA_NATIVE
    UNUSED(nHint);
    // macOS malloc is 16-byte aligned, which is the retail heap alignment.
    void* memory = ::malloc(size);
    if (!memory) {
        return nullptr;
    }
    std::lock_guard lock{ g_blockMutex };
    AccountLocked(memory, size, m_id, false);
    return memory;
#else
#ifdef MEMORY_MGR_USE_MEMORY_HEAP
    _LOCK_MEMORYHEAP();
    void* memory = nullptr;
    int32 iAttempts = 0;
    while (true) {
        memory = InternalMalloc(size);
        if (memory) {
            break;
        }
        if (_gfnMemCompact) {
            _UNLOCK_MEMORYHEAP();
            // sub_2644FC(_nSize, ++i);
            _gfnMemCompact(size, ++iAttempts);
            _LOCK_MEMORYHEAP();
        } else {
            _FATAL("Failed to allocate %d bytes!", size);
        }
    }
    RegisterMalloc(memory);
    CMemoryHeap::SetDebugInfo(memory, nHint);
    _UNLOCK_MEMORYHEAP();
    return memory;
#else
    return plugin::CallAndReturn<void*, 0x72F420, size_t>(size);
#endif
#endif
}

// 0x72F430
void CMemoryMgr::Free(void* memory) {
#ifdef NOTSA_NATIVE
    if (!memory) {
        return;
    }
    bool release = true;
    {
        std::lock_guard lock{ g_blockMutex };
        if (const auto it = g_blocks.find(memory); it != g_blocks.end()) {
            release = !it->second.scratch;
            UnaccountLocked(it);
        } else if (IsScratchRange(memory)) {
            return;
        }
    }
    if (release) {
        ::free(memory);
    }
    return;
#else
#ifdef MEMORY_MGR_USE_MEMORY_HEAP
    LOCK_MEMORYHEAP();
    RegisterFree(memory);
    if (g_Heaps[HEAP_SCRATCH]._IsHeapPointer(memory)) {
        g_Heaps[HEAP_SCRATCH].Free(memory);
    } else if (g_Heaps[HEAP_SECONDARY]._IsHeapPointer(memory)) {
        g_Heaps[HEAP_SECONDARY].Free(memory);
    } else {
        g_Heaps[HEAP_PRIMARY].Free(memory);
    }
    UNLOCK_MEMORYHEAP();
#else
    return plugin::Call<0x72F430, void*>(memory);
#endif
#endif
}

// 0x72F440
uint8* CMemoryMgr::Realloc(void* memory, uint32 size, uint32 nHint) {
#ifdef NOTSA_NATIVE
    UNUSED(nHint);
    if (!memory) {
        return static_cast<uint8*>(Malloc(size, nHint));
    }
    if (IsScratchRange(memory)) {
        uint32 oldSize = 0;
        int32 oldId = m_id;
        bool had = false;
        {
            std::lock_guard lock{ g_blockMutex };
            if (const auto it = g_blocks.find(memory); it != g_blocks.end() && it->second.scratch) {
                oldSize = it->second.size;
                oldId = it->second.id;
                had = true;
                UnaccountLocked(it);
            }
        }
        void* next = MallocFromScratchPad(size);
        if (!next) {
            if (had) {
                std::lock_guard lock{ g_blockMutex };
                AccountLocked(memory, oldSize, oldId, true);
            }
            return nullptr;
        }
        if (oldSize) {
            std::memcpy(next, memory, std::min(oldSize, size));
        }
        return static_cast<uint8*>(next);
    }

    int32 id = m_id;
    uint32 oldSize = 0;
    bool tracked = false;
    {
        std::lock_guard lock{ g_blockMutex };
        if (const auto it = g_blocks.find(memory); it != g_blocks.end()) {
            tracked = true;
            id = it->second.id;
            oldSize = it->second.size;
            UnaccountLocked(it);
        }
    }
    void* next = ::realloc(memory, size);
    if (!next) {
        if (size != 0 && tracked) {
            std::lock_guard lock{ g_blockMutex };
            AccountLocked(memory, oldSize, id, false);
        }
        return nullptr;
    }
    {
        std::lock_guard lock{ g_blockMutex };
        AccountLocked(next, size, id, false);
    }
    return static_cast<uint8*>(next);
#else
#ifdef MEMORY_MGR_USE_MEMORY_HEAP
    if (!memory) {
        return Malloc(size, nHint);
    }
    LOCK_MEMORYHEAP();
    RegisterFree(memory);
    PushMemId(GET_HEAP_DESC(memory)->m_nMemId);
    int32 iAttempts = 0;
    void* newMemory = nullptr;
    while (true) {
        newMemory = InternalRealloc(memory, size);
        if (newMemory) {
            break;
        }
        if (_gfnMemCompact) {
            // sub_2644FC(_nSize, ++i);
            _gfnMemCompact(size, ++iAttempts);
        } else {
            _FATAL("Failed to allocate %d bytes!", size);
        }
    }
    RegisterMalloc(newMemory);
    PopMemId();
    CMemoryHeap::SetDebugInfo(newMemory, nHint);
    UNLOCK_MEMORYHEAP();
    return newMemory;
#else
    return plugin::CallAndReturn<uint8*, 0x72F440, void*, size_t>(memory, size);
#endif
#endif
}

// 0x72F460
uint8* CMemoryMgr::Calloc(uint32 numObj, uint32 sizeObj, uint32 hint) {
#ifdef NOTSA_NATIVE
    UNUSED(hint);
    void* memory = ::calloc(numObj, sizeObj);
    if (!memory) {
        return nullptr;
    }
    const uint64 bytes = uint64{ numObj } * uint64{ sizeObj };
    std::lock_guard lock{ g_blockMutex };
    AccountLocked(memory, bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32>(bytes), m_id, false);
    return static_cast<uint8*>(memory);
#else
    return plugin::CallAndReturn<uint8*, 0x72F460, uint32, uint32>(numObj, sizeObj);
#endif
}

// 0x72F4C0
uint8* CMemoryMgr::MallocAlign(uint32 size, uint32 align, uint32 nHint) {
#ifdef NOTSA_NATIVE
    if (align < sizeof(void*)) {
        align = sizeof(void*);
    }
    auto* memory = static_cast<uint8*>(Malloc(size + align, nHint));
    if (!memory) {
        return nullptr;
    }
    const auto result = (uintptr_t(memory) + align) & ~(uintptr_t(align) - 1);
    *reinterpret_cast<void**>(result - sizeof(void*)) = memory;
    return reinterpret_cast<uint8*>(result);
#else
    auto* memory = Malloc(size + align, nHint);

    auto* result = (void*)(uintptr_t(uintptr_t((uint8*)memory + align)) & ~(uintptr_t(align) - 1));
    *((void**)result - 1) = memory;
    return static_cast<uint8*>(result);
#endif
}

void CMemoryMgr::FreeAlign(void* memory) {
#ifdef NOTSA_NATIVE
    if (!memory) {
        return;
    }
    void* raw = nullptr;
    {
        std::lock_guard lock{ g_blockMutex };
        void* back = *(static_cast<void**>(memory) - 1);
        if (OwnsLocked(back)) {
            raw = back;
        } else if (void* user = *static_cast<void**>(memory); user && user != memory) {
            // Some call sites pass the address of the pointer variable rather than the block.
            void* back2 = *(static_cast<void**>(user) - 1);
            if (OwnsLocked(back2)) {
                raw = back2;
            } else if (OwnsLocked(user)) {
                raw = user;
            }
        }
    }
    if (raw) {
        Free(raw);
    }
#else
    return plugin::Call<0x72F4F0, void*>(memory);
#endif
}

void CMemoryMgr::SetRestrictMemoryMove(bool restrict) {
    g_Heaps[HEAP_PRIMARY].m_bMovingRestricted   = restrict;
    g_Heaps[HEAP_SECONDARY].m_bMovingRestricted = restrict;
}

void CMemoryMgr::PushMemId(eMemoryId id) {
    m_idStack.push(m_id);
    m_id = id;
    g_Heaps[HEAP_PRIMARY].m_nMemId   = m_id;
    g_Heaps[HEAP_SECONDARY].m_nMemId = m_id;
}

void CMemoryMgr::PopMemId() {
    m_id = m_idStack.pop();
    g_Heaps[HEAP_PRIMARY].m_nMemId   = m_id;
    g_Heaps[HEAP_SECONDARY].m_nMemId = m_id;
}

void* CMemoryMgr::MoveMemory(void* memory) {
#ifdef NOTSA_NATIVE
    // The libc heap does not compact, so the block stays put.
    return memory;
#else
    LOCK_MEMORYHEAP();
    RegisterFree(memory);
    void* movedMem = nullptr;
    if (g_Heaps[HEAP_SECONDARY].IsHeapPointer(memory)) {
        movedMem = g_Heaps[HEAP_SECONDARY].MoveMemory(memory);
    } else {
        movedMem = g_Heaps[HEAP_PRIMARY].MoveMemory(memory);
    }
    RegisterMalloc(movedMem);
    UNLOCK_MEMORYHEAP();
    return movedMem;
#endif
}

void* CMemoryMgr::MoveMemory(void* memory, void** ppUnk, int32 iUnk) {
#ifdef NOTSA_NATIVE
    UNUSED(ppUnk);
    UNUSED(iUnk);
    return memory;
#else
    LOCK_MEMORYHEAP();
    RegisterFree(memory);
    void* movedMem = nullptr;
    if (g_Heaps[HEAP_SECONDARY].IsHeapPointer(memory)) {
        movedMem = g_Heaps[HEAP_SECONDARY].MoveMemory(memory, ppUnk, iUnk);
    } else {
        movedMem = g_Heaps[HEAP_PRIMARY].MoveMemory(memory, ppUnk, iUnk);
    }
    RegisterMalloc(movedMem);
    UNLOCK_MEMORYHEAP();
    return movedMem;
#endif
}

void* CMemoryMgr::Malloc(uint32 size) {
    void* memory = Malloc(size, 0);
#if defined MEMORY_MGR_USE_HEAP_FLAGS
    GET_HEAP_DESC(memory)->m_Flags.NoDebugHint = true;
#endif
    return memory;
}

void* CMemoryMgr::MallocAlign(uint32 size, uint32 align) {
#ifdef NOTSA_NATIVE
    return MallocAlign(size, align, 0);
#else
    return plugin::CallAndReturn<void*, 0x72F4C0, uint32, uint32>(size, align);

    void* memory = MallocAlign(size, align, 0);
#if defined MEMORY_MGR_USE_HEAP_FLAGS
    GET_HEAP_DESC(memory)->m_Flags.NoDebugHint = true;
#endif
    return memory;
#endif
}

void* CMemoryMgr::Calloc(uint32 hint, uint32 size) {
    void* memory = Calloc(hint, size, 0);
#if defined MEMORY_MGR_USE_HEAP_FLAGS
    GET_HEAP_DESC(memory)->m_Flags.NoDebugHint = true;
#endif
    return memory;
}

uint8* CMemoryMgr::Realloc(void* memory, uint32 size) {
    memory = Realloc(memory, size, 0);
#if defined MEMORY_MGR_USE_HEAP_FLAGS
    GET_HEAP_DESC(memory)->m_Flags.NoDebugHint = true;
#endif
    return static_cast<uint8*>(memory);
}

#ifdef MEMORY_MGR_OVERLOAD_NEW_DELETE

void* operator new[](uint32 size) {
    void* memory = CMemoryMgr::Malloc(size);
#if defined MEMORY_MGR_USE_HEAP_FLAGS
    GET_HEAP_DESC(memory)->m_Flags.AllocatedUsingNew = true;
    GET_HEAP_DESC(memory)->m_Flags.IsArray = true;
#endif
    return memory;
}

void operator delete[](void* memory) {
    if (!memory) {
        return;
    }
#if defined MEMORY_MGR_USE_HEAP_FLAGS
    assert(GET_HEAP_DESC(memory)->m_Flags.AllocatedUsingNew);
    assert(GET_HEAP_DESC(memory)->m_Flags.IsArray);
#endif
    CMemoryMgr::Free(memory);
}

void* operator new(uint32 size) {
    void* memory = CMemoryMgr::Malloc(size);
#if defined MEMORY_MGR_USE_HEAP_FLAGS
    GET_HEAP_DESC(memory)->m_Flags.AllocatedUsingNew = true;
#endif
    return memory;
}

void operator delete(void* memory) {
    if (!memory) {
        return;
    }
#if defined MEMORY_MGR_USE_HEAP_FLAGS
    assert(GET_HEAP_DESC(memory)->m_Flags.AllocatedUsingNew);
    assert(!GET_HEAP_DESC(memory)->m_Flags.IsArray);
#endif
    CMemoryMgr::Free(memory);
}

#endif

#ifndef NOTSA_STANDALONE
void* operator new(size_t size) {
    return plugin::CallAndReturn<void*, 0x82119A, size_t>(size);
}

void* operator new[](size_t size) {
    return plugin::CallAndReturn<void*, 0x821195, size_t>(size);
}

void operator delete(void* p) {
    plugin::Call<0x8214BD, void*>(p);
}

void operator delete[](void* p) {
    plugin::Call<0x8213AE, void*>(p);
}
#endif
