#include "StdInc.h"
#include "InteriorGroup_c.h"
#include "Interior_c.h"
#include "InteriorManager_c.h"

#include "Tasks/TaskTypes/Interior/TaskInteriorBeInOffice.h"
#include "Tasks/TaskTypes/Interior/TaskInteriorBeInHouse.h"
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
    RH_ScopedInstall(Update, 0x5968E0);
    RH_ScopedInstall(SetupPeds, 0x596890);
    RH_ScopedInstall(UpdatePeds, 0x596830);
    RH_ScopedInstall(SetupHousePeds, 0x5965E0);
    RH_ScopedInstall(SetupPaths, 0x595590);
    RH_ScopedInstall(ArePathsLoaded, 0x595380);
    RH_ScopedInstall(Setup, 0x595320);
    RH_ScopedInstall(Exit, 0x595290);
    RH_ScopedInstall(ContainsInteriorType, 0x595250);
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
    CalcIsVisible();

    if (!m_pathSetupComplete) {
        SetupPaths();
    }

    // NOTE: The paths must be set up before the peds are (The peds are spawned on the goto points)
    if (m_pathSetupComplete && !m_updatePeds) {
        SetupPeds();
    }

    if (m_updatePeds) {
        UpdatePeds();
    }

    ReferenceAnims();
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
    if (!m_EnEx || !g_interiorMan.ArePedsEnabled()) {
        return;
    }

    switch (static_cast<eInteriorGroupType>(m_groupType)) {
    case eInteriorGroupType::HOUSE:
        SetupHousePeds();
        break;
    case eInteriorGroupType::SHOP:
        SetupShopPeds();
        break;
    case eInteriorGroupType::OFFICE:
        SetupOfficePeds();
        break;
    default:
        break;
    }

    m_updatePeds = 1;
}

// 0x596830
void InteriorGroup_c::UpdatePeds() {
    if (!m_EnEx || !g_interiorMan.ArePedsEnabled()) {
        return;
    }

    for (auto& ped : m_pedsToRemove) {
        if (!ped) {
            continue;
        }
        if (ped->IsPointerValid()) {
            RemovePed(ped);
        }
        ped = nullptr;
    }

    if (m_groupType == static_cast<uint8>(eInteriorGroupType::OFFICE)) {
        UpdateOfficePeds();
    }
}

// 0x5965E0
int32 InteriorGroup_c::SetupHousePeds() {
    CStreaming::StreamPedsForInterior(static_cast<int32>(eInteriorGroupType::HOUSE));
    CStreaming::LoadAllRequestedModels(false);

    m_numPeds = 0;

    // The peds are created at the group's entity position
    if (!m_pEntity->m_matrix) {
        m_pEntity->AllocateMatrix();
        m_pEntity->m_placement.UpdateMatrix(m_pEntity->m_matrix);
    }
    const auto pos = m_pEntity->m_matrix->TransformPoint(CVector{});

    // 50% chance of a second ped
    auto numPeds = 1 + (RandomScaled(100.0f) <= 50);

    // Usually the peds are members of the gang dominating the current zone (If there's one)
    bool     useGangPeds = false;
    ePedType gangPedType = static_cast<ePedType>(0);
    if (CPopCycle::m_pCurrZoneInfo) {
        gangPedType = CPopCycle::PickGangToCreateMembersOf();
        if (gangPedType != 0 && CPopulation::ChooseGangOccupation(static_cast<eGangID>(gangPedType - PED_TYPE_GANG1)) >= 0) {
            useGangPeds = true;
            if (RandomScaled(100.0f) <= 15) { // 15% chance of a gang
                numPeds = 3 + (CGeneral::GetRandomNumber() >= 0x3FFF);
            } else {
                useGangPeds = false;
            }
        }
    }

    for (auto i = 0; i < numPeds; i++) {
        // NOTE: `m_numPeds` is also the index of the next free ped slot
        const auto slot = static_cast<int32>(m_numPeds);

        eModelID modelId{};
        if (useGangPeds) {
            modelId = CPopulation::ChooseGangOccupation(static_cast<eGangID>(gangPedType - PED_TYPE_GANG1));
        } else {
            // The first ped always uses slot 0, while the others use their loop index
            const auto randFactor = numPeds == 1
                ? static_cast<int32>(CGeneral::GetRandomNumber() >= 0x3FFF)
                : i;
            modelId = static_cast<eModelID>(CStreaming::FindMIPedSlotForInterior(randFactor));
        }

        auto* const  pedModelInfo = static_cast<CPedModelInfo*>(CModelInfo::GetModelInfo(modelId));
        auto* const  ped          = CPopulation::AddPed(pedModelInfo->m_nPedType, modelId, pos, false);
        m_peds[slot] = ped;
        if (ped) {
            m_numPeds++;
            CTheScripts::ScriptsForBrains.StartOrRequestNewStreamedScriptBrainWithThisName("house", ped, 3);
            ped->SetCharCreatedBy(PED_MISSION);
            ped->GetIntelligence()->SetPedDecisionMakerType(7);
            ped->GetTaskManager().SetTask(new CTaskInteriorBeInHouse{ this }, TASK_PRIMARY_DEFAULT);
        }

        // The interior manager decides whether the ped may stay alive
        if (!g_interiorMan.GetInteriorPedAliveState(slot)) {
            RemovePed(m_peds[slot]);
        }
    }

    return numPeds;
}

// 0x595590
int8 InteriorGroup_c::SetupPaths() {
    if (!ArePathsLoaded()) {
        return 0;
    }

    ThePaths.StartNewInterior(m_id);

    constexpr auto NUM_INTERIORS = sizeof(InteriorGroup_c::m_interiors) / sizeof(InteriorGroup_c::m_interiors[0]);

    // 1. Create a path node for every goto point of every interior.
    //    `gotoPtBase[i]` is the index of the first node belonging to `m_interiors[i]`
    int32 gotoPtBase[NUM_INTERIORS]{};
    int32 numGotoPoints{}; //!< Number of goto points processed so far (Also the node index the next interior starts at)
    int32 numNodes{};      //!< Number of nodes created so far
    for (auto i = 0; i < static_cast<int32>(std::size(m_interiors)); i++) {
        const auto interior = m_interiors[i];
        if (!interior) {
            continue;
        }
        gotoPtBase[i] = numGotoPoints;

        const auto numPts = static_cast<int32>(interior->m_numGotoPoints);
        if (numPts > 2) { // Retail only creates nodes if there are more than 2 goto points
            const auto* const gotoPts = interior->m_gotoPoints;
            ThePaths.AddNodeToNewInterior(gotoPts[0].m_pos.x, gotoPts[0].m_pos.y, gotoPts[0].m_pos.z, false, -1, -1, -1, -1, -1, -1);
            ThePaths.AddNodeToNewInterior(gotoPts[1].m_pos.x, gotoPts[1].m_pos.y, gotoPts[1].m_pos.z, false, -1, -1, -1, -1, -1, -1);
            ThePaths.AddInteriorLink(numNodes + 1, numGotoPoints);
            for (auto k = 2; k < numPts; k++) {
                ThePaths.AddNodeToNewInterior(gotoPts[k].m_pos.x, gotoPts[k].m_pos.y, gotoPts[k].m_pos.z, false, -1, -1, -1, -1, -1, -1);
                ThePaths.AddInteriorLink(numNodes + k, numGotoPoints + k - 2);
                if (k & 1) {
                    ThePaths.AddInteriorLink(numNodes + k, numGotoPoints + k - 1);
                }
            }
            numNodes += numPts;
        }
        numGotoPoints += numPts;
    }

    // 2. Create the 2 nodes of every exit door and link them to the interior's goto points
    // At most 4 doors can be pending per interior
    constexpr auto NUM_MAX_PENDING = NUM_INTERIORS * 4;
    CVector pendingPos[NUM_MAX_PENDING]{};
    int32   pendingNodeIdx[NUM_MAX_PENDING]{};
    bool    pendingLinked[NUM_MAX_PENDING]{};
    auto    numPending = 0;

    int32 nodeIdx = numGotoPoints; //!< Index of the node to create next (Retail starts at the goto point counter, not at `numNodes`)

    for (auto i = 0; i < static_cast<int32>(std::size(m_interiors)); i++) {
        const auto interior = m_interiors[i];
        if (!interior) {
            continue;
        }

        auto* const doors = interior->m_doorPathNodes;

        int32 doorNodeIdx[4]{ -1, -1, -1, -1 }; //!< The index of each door's "inside" node
        bool  bPosSet{};                        //!< If `m_position`/`m_nodeAddress` have been set already (By the first door that exists)

        //! @brief Creates both nodes of a door and links them to the interior's goto points
        const auto ProcessDoor = [&](int32 doorIdx, const InteriorDoorPathNode& door, bool bDontWander) {
            const auto insideNodeIdx  = nodeIdx;
            const auto insideNodeAddr = ThePaths.AddNodeToNewInterior(door.m_posIn.x, door.m_posIn.y, door.m_posIn.z, false, -1, -1, -1, -1, -1, -1);
            if (door.m_gotoPointA != -1) {
                ThePaths.AddInteriorLink(insideNodeIdx, gotoPtBase[i] + door.m_gotoPointA);
            }
            if (door.m_gotoPointB != -1) {
                ThePaths.AddInteriorLink(insideNodeIdx, gotoPtBase[i] + door.m_gotoPointB);
            }
            doorNodeIdx[doorIdx] = insideNodeIdx;
            if (!bPosSet) {
                interior->m_position    = door.m_posIn;
                interior->m_nodeAddress = insideNodeAddr;
                bPosSet                 = true;
            }

            ThePaths.AddNodeToNewInterior(door.m_posOut.x, door.m_posOut.y, door.m_posOut.z, bDontWander, -1, -1, -1, -1, -1, -1);
            ThePaths.AddInteriorLink(insideNodeIdx + 1, insideNodeIdx);

            const auto exteriorAddr = ThePaths.FindNearestExteriorNodeToInteriorNode(insideNodeIdx + 1);
            if (!exteriorAddr.IsAreaValid()) {
                // No exterior node nearby - The links are resolved once all the nodes are created (See below)
                pendingPos[numPending]     = door.m_posOut;
                pendingNodeIdx[numPending] = insideNodeIdx + 1;
                pendingLinked[numPending]  = false;
                numPending++;
            } else {
                interior->m_exteriorNodeAddress = exteriorAddr;

                const auto exteriorPos        = ThePaths.m_pPathNodes[exteriorAddr.m_wAreaId][exteriorAddr.m_wNodeId].GetPosition();
                interior->m_exteriorNodePos = exteriorPos;

                // NOTE: Retail only links the node if the distance is within 3 units (Comparing the *squared* distance against 9)
                if (m_groupType == static_cast<uint8>(eInteriorGroupType::HOUSE) && CVector::DistSqr(door.m_posOut, exteriorPos) <= 9.0f) {
                    ThePaths.AddInteriorLinkToExternalNode(insideNodeIdx + 1, exteriorAddr);
                }
            }

            nodeIdx += 2;
        };

        const auto& box = *interior->m_box;
        if (box.m_door >= 0) {
            // NOTE: Retail passes `m_groupType != 0` only for this door's node
            ProcessDoor(0, doors[0], m_groupType != static_cast<uint8>(eInteriorGroupType::HOUSE));
        }
        if (box.m_lDoorStart >= 0) {
            ProcessDoor(1, doors[1], false);
        }
        if (box.m_tDoorStart >= 0) {
            ProcessDoor(2, doors[2], false);
        }
        if (box.m_rDoorStart >= 0) {
            ProcessDoor(3, doors[3], false);
        }

        // If the interior has no goto points its doors are linked to each other
        if (interior->m_numGotoPoints == 0) {
            if (box.m_door >= 0) {
                if (box.m_lDoorStart >= 0) ThePaths.AddInteriorLink(doorNodeIdx[0], doorNodeIdx[1]);
                if (box.m_tDoorStart >= 0) ThePaths.AddInteriorLink(doorNodeIdx[0], doorNodeIdx[2]);
                if (box.m_rDoorStart >= 0) ThePaths.AddInteriorLink(doorNodeIdx[0], doorNodeIdx[3]);
            }
            if (box.m_tDoorStart >= 0) {
                if (box.m_lDoorStart >= 0) ThePaths.AddInteriorLink(doorNodeIdx[2], doorNodeIdx[1]);
                if (box.m_rDoorStart >= 0) ThePaths.AddInteriorLink(doorNodeIdx[2], doorNodeIdx[3]);
            }
        }
    }

    // 3. Link each pending door node to its closest pending neighbor (Or remove its links if there's no close one)
    for (auto i = 0; i < numPending; i++) {
        if (pendingLinked[i]) {
            continue;
        }

        auto bestDistSq = 1e9f; // Same initial value as retail
        auto bestIdx    = -1;
        for (auto j = i + 1; j < numPending; j++) {
            if (pendingLinked[j]) {
                continue;
            }
            const auto distSq = CVector::DistSqr(pendingPos[i], pendingPos[j]);
            if (distSq <= bestDistSq) {
                bestDistSq = distSq;
                bestIdx    = j;
            }
        }

        // NOTE: Retail compares the *squared* distance against 3.0 (Not `sqrt(3)`)
        if (bestDistSq <= 3.0f) {
            pendingLinked[i]       = true;
            pendingLinked[bestIdx] = true;
            ThePaths.AddInteriorLink(pendingNodeIdx[i], pendingNodeIdx[bestIdx]);
        } else {
            // The node isn't close to any other pending one - Undo its links
            ThePaths.RemoveInteriorLinks((uint32)pendingNodeIdx[i]);
        }
    }

    ThePaths.CompleteNewInterior(nullptr);
    m_pathSetupComplete = 1;

    // NOTE: Retail's return value is whatever was left in `eax` (No caller uses it)
    return 0;
}

// 0x595380
int8 InteriorGroup_c::ArePathsLoaded() {
    const auto* const colModel = CModelInfo::GetModelInfo(m_pEntity->m_nModelIndex)->GetColModel();
    const auto&       bbox     = colModel->m_boundBox;

    // Transform the 8 corners of the entity's bounding box (By its matrix, as it may be rotated) and compute their min/max
    const std::array corners{
        CVector{ bbox.m_vecMin.x, bbox.m_vecMin.y, bbox.m_vecMin.z },
        CVector{ bbox.m_vecMin.x, bbox.m_vecMax.y, bbox.m_vecMin.z },
        CVector{ bbox.m_vecMax.x, bbox.m_vecMax.y, bbox.m_vecMin.z },
        CVector{ bbox.m_vecMax.x, bbox.m_vecMin.y, bbox.m_vecMin.z },
        CVector{ bbox.m_vecMin.x, bbox.m_vecMin.y, bbox.m_vecMax.z },
        CVector{ bbox.m_vecMin.x, bbox.m_vecMax.y, bbox.m_vecMax.z },
        CVector{ bbox.m_vecMax.x, bbox.m_vecMax.y, bbox.m_vecMax.z },
        CVector{ bbox.m_vecMax.x, bbox.m_vecMin.y, bbox.m_vecMax.z },
    };

    CVector min{ 999999.0f, 999999.0f, 999999.0f };
    CVector max{ -999999.0f, -999999.0f, -999999.0f };
    for (const auto& corner : corners) {
        // NOTE: Retail allocates the matrix inside this loop (Re-checked on every iteration)
        if (!m_pEntity->m_matrix) {
            m_pEntity->AllocateMatrix();
            m_pEntity->m_placement.UpdateMatrix(m_pEntity->m_matrix);
        }
        const auto pt = m_pEntity->m_matrix->TransformPoint(corner);
        if (pt.x < min.x) min.x = pt.x;
        if (pt.y < min.y) min.y = pt.y;
        if (pt.z < min.z) min.z = pt.z;
        if (pt.x > max.x) max.x = pt.x;
        if (pt.y > max.y) max.y = pt.y;
        if (pt.z > max.z) max.z = pt.z;
    }

    return ThePaths.AreNodesLoadedForArea(min.x, max.x, min.y, max.y);
}

// 0x595320
void InteriorGroup_c::Setup() {
    // NOTE: These are the `m_box->m_type` values found in retail - They don't map 1:1 to `eInteriorGroupType`
    if (ContainsInteriorType(2)) {
        m_groupType = static_cast<uint8>(eInteriorGroupType::HOUSE);
    } else if (ContainsInteriorType(0) || ContainsInteriorType(6)) {
        m_groupType = static_cast<uint8>(eInteriorGroupType::SHOP);
    } else if (ContainsInteriorType(1)) {
        m_groupType = static_cast<uint8>(eInteriorGroupType::OFFICE);
    } else {
        m_groupType = 0xFF; // None of the above - Not a valid `eInteriorGroupType`
    }

    ReferenceAnims();
}

// 0x595290
int8 InteriorGroup_c::Exit() {
    for (auto* const interior : m_interiors) {
        if (!interior) {
            continue;
        }
        interior->Exit();
        g_interiorMan.ReturnInteriorToPool(interior);
    }

    for (auto i = 0; i < static_cast<int32>(std::size(m_peds)); i++) {
        auto* const ped = m_peds[i];
        if (!ped) {
            continue;
        }
        if (ped->IsPointerValid()) {
            if (!ped->IsAlive()) {
                g_interiorMan.GetInteriorPedAliveState(i) = false;
            }
            CPopulation::RemovePed(ped);
        } else {
            g_interiorMan.GetInteriorPedAliveState(i) = false;
        }
        m_peds[i] = nullptr;
    }

    m_numPeds = 0;
    ThePaths.RemoveInterior(m_id);
    ReferenceAnims();

    // NOTE: Retail's return value is whatever was left in `eax` (No caller uses it)
    return 0;
}

// 0x595250
int8 InteriorGroup_c::ContainsInteriorType(int32 type) {
    for (const auto* const interior : m_interiors) {
        // NOTE: Retail doesn't check `m_box` for being null
        if (interior && interior->m_box->m_type == type) {
            return 1;
        }
    }
    return 0;
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
