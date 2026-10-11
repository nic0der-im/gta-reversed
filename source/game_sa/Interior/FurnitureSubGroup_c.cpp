#include "StdInc.h"
#include "FurnitureSubGroup_c.h"
#include "Furniture_c.h"
#include "General.h"

void FurnitureSubGroup_c::InjectHooks() {
    RH_ScopedClass(FurnitureSubGroup_c);
    RH_ScopedCategory("Interior");

    RH_ScopedInstall(GetFurniture, 0x590EE0);
    RH_ScopedInstall(GetRandomId, 0x590FD0);
    RH_ScopedInstall(AddFurniture, 0x5C00C0);
}

static auto& g_currFurnitureId = StaticRef<uint32>(0xBAB378);
static auto& g_furnitureStore  = StaticRef<std::array<Furniture_c, 256>>(0xBAE1F8);

// Truncates `v`, then rounds up if the fractional part is >= 0.02 [0x5C0190 etc.]
static uint8 FurnitureWidth(float v) {
    const auto i = (int32)v;
    return (uint8)(i + (v - (float)i >= 0.02f ? 1 : 0));
}

// 0x5C00C0
bool FurnitureSubGroup_c::AddFurniture(uint16 modelId, int16 id, uint8 wealthMin, uint8 wealthMax, uint8 maxAng) {
    if (g_currFurnitureId >= g_furnitureStore.size()) {
        return false;
    }

    auto& f = g_furnitureStore[g_currFurnitureId++];
    f.m_nModelId = modelId;
    f.m_nId      = id;

    const auto& bb = CModelInfo::GetModelInfo(modelId)->GetColModel()->m_boundBox;
    float x, y;
    if (m_bCanSteal) {
        x = bb.m_vecMax.x - bb.m_vecMin.x;
        y = bb.m_vecMax.y - bb.m_vecMin.y;
    } else {
        x = bb.m_vecMax.x + 0.5f;
        y = bb.m_vecMax.y + 0.5f;
    }
    f.m_nWidthX = FurnitureWidth(x);
    f.m_nWidthY = FurnitureWidth(y);

    f.m_nWealthMin = wealthMin;
    f.m_nWealthMax = wealthMax;
    f.m_nMaxAng    = maxAng;
    f.m_bCanPlaceInFrontOfWindow = m_bCanPlaceInFrontOfWindow;
    f.m_bIsTall                  = m_bIsTall;
    f.m_bCanSteal                = m_bCanSteal;

    m_Furnitures.AddItem(&f);
    return true;
}

// Picks a random item (among those whose wealth range contains `wealth`)
// NOTE: The original had an additional `wealth == 0xFFFFFFFF` branch (random item of the whole list), but `wealth` is a zero-extended byte so it was dead code.
static Furniture_c* PickRandomFurniture(TList_c<Furniture_c>& list, uint8 wealth) {
    const auto Matches = [&](const Furniture_c& f) {
        return (uint8)f.m_nWealthMin <= wealth && wealth <= (uint8)f.m_nWealthMax;
    };

    int32 numMatching = 0;
    for (auto& f : list) {
        numMatching += Matches(f);
    }

    // NOTE: Original constant is 1/32768 (0x858B14), not RAND_MAX_FLOAT_RECIPROCAL
    const auto target = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * (float)numMatching);

    int32 n = 0;
    for (auto& f : list) {
        if (Matches(f)) {
            if (n == target) {
                return &f;
            }
            n++;
        }
    }
    return nullptr;
}

// 0x590EE0
Furniture_c* FurnitureSubGroup_c::GetFurniture(int16 id, uint8 wealth) {
    if (id >= 0) {
        for (auto& f : m_Furnitures) {
            if (f.m_nId == id) {
                return &f;
            }
        }
        return nullptr;
    }
    return PickRandomFurniture(m_Furnitures, wealth);
}

// 0x590FD0
int32 FurnitureSubGroup_c::GetRandomId(uint8 wealth) {
    const auto f = PickRandomFurniture(m_Furnitures, wealth);
    return f ? f->m_nId : -1;
}

void FurnitureSubGroup_c::Exit() {
    m_Furnitures.RemoveAll();
}
