#include "StdInc.h"
#include "FurnitureSubGroup_c.h"
#include "Furniture_c.h"

// 0xBAE1F8 - The global pool that all furniture is allocated from (256 entries)
static inline auto& g_furniturePool = StaticRef<std::array<Furniture_c, 256>>(0xBAE1F8);

// 0xBAB378 - The index of the next free entry in `g_furniturePool` (Only the lower word is ever written by the exe,
// but it's declared as a dword elsewhere - Same as `FurnitureManager_c`'s `g_currFurnitureId`)
static inline auto& g_furniturePoolSize = StaticRef<uint32>(0xBAB378);

void FurnitureSubGroup_c::InjectHooks() {
    RH_ScopedClass(FurnitureSubGroup_c);
    RH_ScopedCategory("Interior");

    RH_ScopedInstall(GetFurniture, 0x590EE0);
    RH_ScopedInstall(GetRandomId, 0x590FD0);
    RH_ScopedInstall(AddFurniture, 0x5C00C0);
}

//! Random index in range [0, count - 1], truncated towards zero.
//! The original computes `(int32)(count * ((uint16)_rand() / 32768.0f))` using the x87 (`__ftol`),
//! `CGeneral::GetRandomNumber()` already yields `rand()` clamped to the MSVC generator's 15 bits.
static int32 GetRandomIndex(int32 count) {
    return (int32)(count * (CGeneral::GetRandomNumber() / 32768.0f));
}

//! The original compares the wealth as an unsigned byte (`JC`/`JA`), so don't compare it as signed.
static bool IsWealthInRange(const Furniture_c& furniture, uint8 wealth) {
    return wealth >= (uint8)furniture.m_nWealthMin
        && wealth <= (uint8)furniture.m_nWealthMax;
}

//! 0x5C0190-0x5C01E5 - `(uint8)((int32)v + ((v - (float)(int32)v) >= 0.02f))`
static uint8 ToWidthByte(float v) {
    const auto i = (int32)v;
    return (uint8)(i + ((v - (float)i) >= 0.02f));
}

// 0x5C00C0
bool FurnitureSubGroup_c::AddFurniture(uint16 modelId, int16 id, uint8 wealthMin, uint8 wealthMax, uint8 maxAng) {
    if (g_furniturePoolSize >= g_furniturePool.size()) {
        return false;
    }

    auto& f = g_furniturePool[g_furniturePoolSize++];
    f.m_nModelId = static_cast<int16>(modelId);
    f.m_nId      = id;

    const auto colModel = CModelInfo::GetModelInfo(modelId)->m_pColModel;
    float widthX, widthY;
    if (m_bCanSteal) { // Use the full extents of the model
        widthX = colModel->m_boundBox.m_vecMax.x - colModel->m_boundBox.m_vecMin.x;
        widthY = colModel->m_boundBox.m_vecMax.y - colModel->m_boundBox.m_vecMin.y;
    } else { // Use the maximum extents, with a bit of space around them
        widthX = colModel->m_boundBox.m_vecMax.x + 0.5f;
        widthY = colModel->m_boundBox.m_vecMax.y + 0.5f;
    }

    f.m_nWidthX                  = static_cast<int8>(ToWidthByte(widthX));
    f.m_nWidthY                  = static_cast<int8>(ToWidthByte(widthY));
    f.m_nWealthMax               = static_cast<int8>(wealthMax);
    f.m_nWealthMin               = static_cast<int8>(wealthMin);
    f.m_nMaxAng                  = static_cast<int8>(maxAng);
    f.m_bCanPlaceInFrontOfWindow = m_bCanPlaceInFrontOfWindow;
    f.m_bIsTall                  = m_bIsTall;
    f.m_bCanSteal                = m_bCanSteal;

    m_Furnitures.AddItem(&f);

    return true;
}

// 0x590EE0
Furniture_c* FurnitureSubGroup_c::GetFurniture(int16 id, uint8 wealth) {
    if (id >= 0) { // Look the furniture up by its id
        for (auto& f : m_Furnitures) {
            if (f.m_nId == id) {
                return &f;
            }
        }
        return nullptr;
    }

    if (wealth == 0xFF) { // Any furniture will do
        return m_Furnitures.GetItemOffset(true, GetRandomIndex(static_cast<int32>(m_Furnitures.GetNumItems())));
    }

    // Count the furniture matching the wealth
    int32 numMatching = 0;
    for (auto& f : m_Furnitures) {
        if (IsWealthInRange(f, wealth)) {
            ++numMatching;
        }
    }

    // And return the randomly chosen one of them
    const auto idx = GetRandomIndex(numMatching);
    int32 i = 0;
    for (auto& f : m_Furnitures) {
        if (!IsWealthInRange(f, wealth)) {
            continue;
        }
        if (i++ == idx) {
            return &f;
        }
    }

    return nullptr;
}

// 0x590FD0
int32 FurnitureSubGroup_c::GetRandomId(uint8 wealth) {
    if (wealth == 0xFF) { // Any furniture will do
        const auto f = m_Furnitures.GetItemOffset(true, GetRandomIndex(static_cast<int32>(m_Furnitures.GetNumItems())));
#ifdef FIX_BUGS
        if (!f) { // The original reads from `nullptr` if the list is empty
            return -1;
        }
#endif
        return f->m_nId;
    }

    // Count the furniture matching the wealth
    int32 numMatching = 0;
    for (auto& f : m_Furnitures) {
        if (IsWealthInRange(f, wealth)) {
            ++numMatching;
        }
    }

    // And return the id of the randomly chosen one of them
    const auto idx = GetRandomIndex(numMatching);
    int32 i = 0;
    for (auto& f : m_Furnitures) {
        if (!IsWealthInRange(f, wealth)) {
            continue;
        }
        if (i++ == idx) {
            return f.m_nId;
        }
    }

    return -1;
}

void FurnitureSubGroup_c::Exit() {
    m_Furnitures.RemoveAll();
}
