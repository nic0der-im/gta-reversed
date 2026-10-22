#include "StdInc.h"
#include "InteriorGroup_c.h"
#include "Interior_c.h"

#include "Tasks/TaskTypes/Interior/TaskInteriorBeInOffice.h"
#include "Tasks/TaskTypes/Interior/TaskInteriorBeInShop.h"
#include "Tasks/TaskTypes/Interior/TaskInteriorShopKeeper.h"

namespace {

/*!
* @return `(int32)(rand() * (1 / 32768) * scale)` - Retail's `fild`/`fmul`/`ftol` sequence using the `1 / 32768` constant.
* @note The result is truncated towards zero (Meaning it can be negative if `scale` is).
*/
int32 RandomScaled(const float scale) {
    return static_cast<int32>(static_cast<float>(CGeneral::GetRandomNumber()) * (1.0f / 32768.0f) * scale);
}

/*!
* @return A random number in the range [min, max) - Same as retail's `GetRandomNumberInRange_int32` (0x407180).
* @note Unlike `CGeneral::GetRandomNumberInRange` this doesn't assert `max >= min`, as retail allows empty ranges
*       (Eg.: when there are no interior infos of the queried type).
*/
int32 RandomRange(const int32 min, const int32 max) {
    return min + RandomScaled(static_cast<float>(max - min));
}

/*!
* @brief Pick a random (non-null) interior of the given group.
* @return The interior, or null if the group has none.
*/
Interior_c* PickRandomInterior(InteriorGroup_c& grp) {
    const auto rndIdx = RandomScaled(static_cast<float>(grp.m_numInteriors));
    for (auto i = 0, found = 0; i < static_cast<int32>(std::size(grp.m_interiors)); i++) {
        if (!grp.m_interiors[i]) {
            continue;
        }
        if (found++ == rndIdx) {
            return grp.m_interiors[i];
        }
    }
    return nullptr;
}

} // namespace

void InteriorGroup_c::InjectHooks() {
    RH_ScopedClass(InteriorGroup_c);
    RH_ScopedCategory("Interior");

    //RH_ScopedInstall(Constructor, 0x597FE0, { .Reversed = false });
    //RH_ScopedInstall(Destructor, 0x597FF0, { .Reversed = false });

    RH_ScopedInstall(Init, 0x5947E0);
    RH_ScopedInstall(Update, 0x5968E0, { .Reversed = false });
    RH_ScopedInstall(SetupPeds, 0x596890, { .Reversed = false });
    RH_ScopedInstall(UpdatePeds, 0x596830, { .Reversed = false });
    RH_ScopedInstall(SetupHousePeds, 0x5965E0, { .Reversed = false });
    RH_ScopedInstall(SetupPaths, 0x595590, { .Reversed = false });
    RH_ScopedInstall(ArePathsLoaded, 0x595380, { .Reversed = false });
    RH_ScopedInstall(Setup, 0x595320, { .Reversed = false });
    RH_ScopedInstall(Exit, 0x595290, { .Reversed = false });
    RH_ScopedInstall(ContainsInteriorType, 0x595250, { .Reversed = false });
    RH_ScopedInstall(CalcIsVisible, 0x595200);
    RH_ScopedInstall(DereferenceAnims, 0x595160);
    RH_ScopedInstall(ReferenceAnims, 0x5950D0);
    RH_ScopedInstall(UpdateOfficePeds, 0x594E90);
    RH_ScopedInstall(RemovePed, 0x594E30);
    RH_ScopedInstall(SetupShopPeds, 0x594C10);
    RH_ScopedInstall(SetupOfficePeds, 0x594BF0);
    RH_ScopedInstall(GetEntity, 0x594BD0);
    RH_ScopedInstall(GetPed, 0x594B90);
    RH_ScopedInstall(FindClosestInteriorInfo, 0x594A50);
    RH_ScopedInstall(FindInteriorInfo, 0x594970);
    RH_ScopedInstall(GetNumInteriorInfos, 0x594920);
    RH_ScopedInstall(GetRandomInterior, 0x5948C0);
    RH_ScopedInstall(AddInterior, 0x594840);
}

// 0x5947E0
void InteriorGroup_c::Init(CEntity* entity, int32 id) {
    rng::fill(m_interiors, nullptr);
    rng::fill(m_peds, nullptr);
    rng::fill(m_pedsToRemove, nullptr);
    m_numInteriors        = 0;
    m_pathSetupComplete   = 0;
    m_updatePeds          = 0;
    m_isVisible           = false;
    m_lastIsVisible       = false;
    m_animBlockReferenced = false;
    m_pEntity             = entity;
    m_groupId             = static_cast<uint8>(id);
    // NOTE: Retail doesn't touch `m_id`, `m_groupType` or `m_numPeds` here (They're set by the caller before/after this call)
}

// 0x5968E0
void InteriorGroup_c::Update() {
    plugin::CallMethod<0x5968E0, InteriorGroup_c*>(this);
}

// 0x594840
int32 InteriorGroup_c::AddInterior(Interior_c* interior) {
    for (size_t i = 0; i < std::size(m_interiors); ++i) {
        if (m_interiors[i]) {
            continue;
        }
        m_interiors[i] = interior;
        m_numInteriors++;
        return static_cast<int32>(i);
    }
    return static_cast<int32>(std::size(m_interiors));
}

// 0x596890
void InteriorGroup_c::SetupPeds() {
    plugin::CallMethod<0x596890, InteriorGroup_c*>(this);
}

// 0x596830
void InteriorGroup_c::UpdatePeds() {
    plugin::CallMethod<0x596830, InteriorGroup_c*>(this);
}

// 0x5965E0
int32 InteriorGroup_c::SetupHousePeds() {
    return plugin::CallMethodAndReturn<int32, 0x5965E0, InteriorGroup_c*>(this);
}

// 0x595590
int8 InteriorGroup_c::SetupPaths() {
    return plugin::CallMethodAndReturn<int8, 0x595590, InteriorGroup_c*>(this);
}

// 0x595380
int8 InteriorGroup_c::ArePathsLoaded() {
    return plugin::CallMethodAndReturn<int8, 0x595380, InteriorGroup_c*>(this);
}

// 0x595320
void InteriorGroup_c::Setup() {
    plugin::CallMethod<0x595320, InteriorGroup_c*>(this);
}

// 0x595290
int8 InteriorGroup_c::Exit() {
    return plugin::CallMethodAndReturn<int8, 0x595290, InteriorGroup_c*>(this);
}

// 0x595250
int8 InteriorGroup_c::ContainsInteriorType(int32 a2) {
    return plugin::CallMethodAndReturn<int8, 0x595250, InteriorGroup_c*, int32>(this, a2);
}

// 0x595200
int8 InteriorGroup_c::CalcIsVisible() {
    m_lastIsVisible = m_isVisible;
    m_isVisible     = false;
    for (auto i = 0; i < static_cast<int32>(m_numInteriors); i++) {
        // NOTE: Retail doesn't check for `nullptr` here (The array has holes, as `AddInterior`/`RemoveInterior` don't compact it)
        if (m_interiors[i] && m_interiors[i]->IsVisible()) {
            m_isVisible = true;
            break;
        }
    }
    // NOTE: Retail doesn't set `eax` (The value is left over), and no caller uses the return value.
    return static_cast<int8>(m_isVisible);
}

// 0x595160
void InteriorGroup_c::DereferenceAnims() {
    if (!m_animBlockReferenced) {
        return;
    }
    CAnimManager::AddAnimBlockRef(CAnimManager::GetAnimationBlockIndex(GetAnimBlockName()));
    m_animBlockReferenced = false;
}

// 0x5950D0
void InteriorGroup_c::ReferenceAnims() {
    if (m_animBlockReferenced) {
        return;
    }
    const auto animBlkIdx = CAnimManager::GetAnimationBlockIndex(GetAnimBlockName());
    if (CStreaming::IsModelLoaded(IFPToModelId(animBlkIdx))) {
        CAnimManager::AddAnimBlockRef(animBlkIdx);
        m_animBlockReferenced = true;
    } else {
        CStreaming::RequestModel(IFPToModelId(animBlkIdx), STREAMING_KEEP_IN_MEMORY);
    }
}

// 0x594E90
void InteriorGroup_c::UpdateOfficePeds() {
    if (!m_isVisible) {
        if (!m_lastIsVisible) {
            return;
        }
        // Not visible anymore - Remove all peds
        for (auto& ped : m_peds) {
            if (!ped) {
                continue;
            }
            if (ped->IsPointerValid()) {
                CPopulation::RemovePed(ped);
            }
            ped = nullptr;
        }
        m_numPeds = 0;
        return;
    }
    if (m_lastIsVisible) {
        return;
    }

    // Just became visible - Create some peds around the group's entity
    if (!m_pEntity->m_matrix) {
        m_pEntity->AllocateMatrix();
        m_pEntity->m_placement.UpdateMatrix(m_pEntity->m_matrix);
    }
    const auto pos = m_pEntity->m_matrix->TransformPoint(CVector{});

    // Create more peds during the day, and less (or none) during the night
    const auto numInfos = GetNumInteriorInfos(static_cast<int32>(eInteriorInfoType::UNK_6));
    const auto numPeds  = [&]() -> int32 {
        if (CClock::GetIsTimeInRange(9, 18)) {
            return RandomRange(numInfos / 2, numInfos);
        }
        if (CClock::GetIsTimeInRange(18, 22) || CClock::GetIsTimeInRange(6, 9)) {
            return RandomRange(0, numInfos / 2);
        }
        return 0;
    }();
    const auto numToCreate = std::min<int32>(numPeds, 16);
    for (auto i = 0; i < numToCreate; i++) {
        const auto  modelId      = CStreaming::FindMIPedSlotForInterior(RandomScaled(8.0f));
        auto* const pedModelInfo = static_cast<CPedModelInfo*>(CModelInfo::GetModelInfo(modelId));

        const auto ped = CPopulation::AddPed(pedModelInfo->m_nPedType, static_cast<eModelID>(modelId), pos, false);
        m_peds[m_numPeds] = ped;
        if (!ped) {
            continue;
        }
        m_numPeds++;
        ped->SetCharCreatedBy(PED_MISSION);
        ped->GetIntelligence()->SetPedDecisionMakerType(7);
        ped->GetTaskManager().SetTask(new CTaskInteriorBeInOffice{ this }, TASK_PRIMARY_DEFAULT);
    }
}

// 0x594E30
int8 InteriorGroup_c::RemovePed(CPed* ped) {
    // NOTE: Retail's return value is whatever was left in `eax` - No caller uses it.
    for (auto i = 0; i < static_cast<int32>(std::size(m_peds)); i++) {
        if (!m_peds[i] || m_peds[i] != ped) {
            continue;
        }
        CPopulation::RemovePed(ped);
        m_peds[i] = nullptr;
        m_numPeds--;
        return 1;
    }
    return 0;
}

// 0x594C10
int32 InteriorGroup_c::SetupShopPeds() {
    CStreaming::StreamPedsForInterior(static_cast<int32>(eInteriorGroupType::SHOP));
    CStreaming::LoadAllRequestedModels(false);
    m_numPeds = 0;

    // `2 - rand * (1 / 32768) * -3` is `2..4`
    const auto numPeds = (2 - RandomScaled(-3.0f)) * m_numInteriors + 1;
    for (auto i = 0; i < numPeds; i++) {
        // The first ped uses ped slot 0, the rest use a random one from 1 to 7
        const auto randFactor = i == 0 ? 0 : 1 - RandomScaled(-7.0f);

        const auto  modelId      = CStreaming::FindMIPedSlotForInterior(randFactor);
        auto* const pedModelInfo = static_cast<CPedModelInfo*>(CModelInfo::GetModelInfo(modelId));

        auto* const interior = PickRandomInterior(*this); // Same as `GetRandomInterior`, but returns a pointer usable on 64-bit
        int32       tileX{}, tileY{};
        CVector     pos{};
        interior->GetRandomTile(3, &tileX, &tileY);
        interior->GetTileCentre(static_cast<float>(tileX), static_cast<float>(tileY), &pos);
        pos.z += 1.0f;

        const auto ped = CPopulation::AddPed(pedModelInfo->m_nPedType, static_cast<eModelID>(modelId), pos, false);
        m_peds[m_numPeds] = ped;
        if (!ped) {
            continue;
        }
        m_numPeds++;
        ped->SetCharCreatedBy(PED_MISSION);
        ped->GetIntelligence()->SetPedDecisionMakerType(7);

        // The first ped is the shop keeper, the rest are customers
        if (i == 0) {
            ped->GetTaskManager().SetTask(new CTaskInteriorShopKeeper{ this, false }, TASK_PRIMARY_DEFAULT);
        } else {
            ped->GetTaskManager().SetTask(new CTaskInteriorBeInShop{ this }, TASK_PRIMARY_DEFAULT);
        }
    }
    return numPeds;
}

// 0x594BF0
void InteriorGroup_c::SetupOfficePeds() {
    CStreaming::StreamPedsForInterior(static_cast<int32>(eInteriorGroupType::OFFICE));
    CStreaming::LoadAllRequestedModels(false);
    m_numPeds = 0;
}

// 0x594BD0
CEntity* InteriorGroup_c::GetEntity() {
    return m_pEntity;
}

// 0x594B90
CPed* InteriorGroup_c::GetPed(int32 idx) {
    return m_peds[idx];
}

// 0x594A50
bool InteriorGroup_c::FindClosestInteriorInfo(int32 infoType, CVector point, float radius, InteriorInfo_t** interiorInfo, Interior_c** interior, float* pSome) {
    const auto radiusSq = radius * radius;

    InteriorInfo_t* closestInfo{};
    Interior_c*     closestInterior{};
    auto            closestDistSq = 999999.f;

    for (auto* const grpInterior : m_interiors) {
        if (!grpInterior || !grpInterior->IsPtInside(point) || grpInterior->m_interiorInfosCount <= 0) {
            continue;
        }
        for (auto& info : grpInterior->m_interiorInfos | rng::views::take(grpInterior->m_interiorInfosCount)) {
            if ((infoType != -1 && info.Type != static_cast<eInteriorInfoType>(infoType)) || info.IsInUse) {
                continue;
            }
            const auto distSq = CVector::DistSqr(point, info.Pos);
            if (distSq < radiusSq && distSq < closestDistSq) {
                closestDistSq   = distSq;
                closestInfo     = &info;
                closestInterior = grpInterior;
            }
        }
    }

    if (!closestInfo) {
        *interiorInfo = nullptr;
        *interior     = nullptr;
        return false;
    }
    *interiorInfo = closestInfo;
    *interior     = closestInterior;
    *pSome        = closestDistSq; //< Squared distance
    return true;
}

// 0x594970
bool InteriorGroup_c::FindInteriorInfo(eInteriorInfoType infoType, InteriorInfo_t** interiorInfo, Interior_c** interior) {
    // Gather all the (unused) infos of the given type
    std::vector<std::pair<InteriorInfo_t*, Interior_c*>> infos;
    for (auto* const grpInterior : m_interiors) {
        if (!grpInterior || grpInterior->m_interiorInfosCount <= 0) {
            continue;
        }
        for (auto& info : grpInterior->m_interiorInfos | rng::views::take(grpInterior->m_interiorInfosCount)) {
            if (info.Type == infoType && !info.IsInUse) {
                infos.emplace_back(&info, grpInterior);
            }
        }
    }
    if (infos.empty()) {
        *interiorInfo = nullptr;
        *interior     = nullptr;
        return false;
    }
    const auto& [foundInfo, foundInterior] = infos[RandomScaled(static_cast<float>(infos.size()))];
    *interiorInfo = foundInfo;
    *interior     = foundInterior;
    return true;
}

// 0x594920
int32 InteriorGroup_c::GetNumInteriorInfos(int32 infoType) {
    int32 numInfos{};
    for (const auto* interior : m_interiors) {
        if (!interior || interior->m_interiorInfosCount <= 0) {
            continue;
        }
        for (const auto& info : interior->m_interiorInfos | rng::views::take(interior->m_interiorInfosCount)) {
            numInfos += info.Type.get_underlying() == infoType;
        }
    }
    return numInfos;
}

// 0x5948C0
int32 InteriorGroup_c::GetRandomInterior() {
    // TODO(re): Retail returns the `Interior_c*` itself (See `PickRandomInterior`), but the return type is `int32`,
    //           so the pointer would be truncated on 64-bit - This is why `SetupShopPeds` uses `PickRandomInterior` directly.
    return static_cast<int32>(reinterpret_cast<intptr_t>(PickRandomInterior(*this)));
}

//! @notsa
const char* InteriorGroup_c::GetAnimBlockName() {
    switch ((eInteriorGroupType)m_groupType) {
    case eInteriorGroupType::HOUSE:  return "int_house";
    case eInteriorGroupType::SHOP:   return "int_shop";
    case eInteriorGroupType::OFFICE: return "int_office";
    default:                         NOTSA_UNREACHABLE();
    }
}
