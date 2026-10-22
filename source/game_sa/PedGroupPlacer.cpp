#include "StdInc.h"
#include "PedPlacement.h"
#include "Tasks/Allocators/PedGroup/PedGroupDefaultTaskAllocators.h"
#include "TaskComplexWanderGang.h"
#include "TaskComplexBeInGroup.h"
#include "TaskComplexFollowLeaderInFormation.h"

void CPedGroupPlacer::InjectHooks() {
    RH_ScopedClass(CPedGroupPlacer);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(PlaceFormationGroup, 0x5FC9B0);
    RH_ScopedInstall(PlaceChatGroup, 0x5FCE80);
    RH_ScopedInstall(PlaceRandomGroup, 0x5FD330);
    RH_ScopedInstall(PlaceGroup, 0x5FD810);
}

// 0x5FC9B0
bool CPedGroupPlacer::PlaceFormationGroup(ePedType type, uint32 numOfPeds, const CVector& origin, ePedGroupDefaultTaskAllocatorType unused) {
    const auto groupId = CPedGroups::AddGroup();
    if (groupId < 0) {
        return false;
    }

    // Don't create the group in the player's view (Too close to him)
    if (TheCamera.IsSphereVisible(origin, 3.f)) {
        if ((FindPlayerPed()->GetPosition() - origin).Magnitude2D() < CPopulation::PedCreationDistMultiplier() * 42.5f) {
            return false;
        }
    }

    if (!CPedPlacement::IsPositionClearForPed(origin, 3.f, -1, nullptr, true, true, true)) {
        return false;
    }

    bool groundFound{};
    const float groundZ = CWorld::FindGroundZFor3DCoord({ origin.x, origin.y, origin.z + 1.f }, &groundFound);
    if (!groundFound) {
        return false;
    }

    const float z = std::max(groundZ + 1.f, origin.z);

    const auto leaderModelId = CPopulation::ChooseGangOccupation(static_cast<eGangID>(type - PED_TYPE_GANG1));
    if (!CModelInfo::GetPedModelInfo(leaderModelId)->GetRwObject()) { // Model isn't streamed in
        return false;
    }

    CPed* const leader = CPopulation::AddPed(type, leaderModelId, { origin.x, origin.y, z }, false);
    if (!leader) {
        return false;
    }

    constexpr uint32 kMaxPeds = 8; // Number of peds the original function has room for

    CPed*  createdPeds[kMaxPeds]{ leader }; // The first ped is the group's leader
    uint32 numCreated = 1;

    for (uint32 i = 1; i < numOfPeds; i++) {
        const auto modelId = CPopulation::ChooseGangOccupation(static_cast<eGangID>(type - PED_TYPE_GANG1));
        if (!CModelInfo::GetPedModelInfo(modelId)->GetRwObject()) { // Model isn't streamed in
            continue;
        }

        CPed* const ped = CPopulation::AddPed(type, modelId, { origin.x, origin.y, z }, false);
        if (!ped) {
            continue;
        }

        // Offset the new ped from the leader using the precomputed formation ring
        const auto& off = CTaskComplexFollowLeaderInFormation::ms_offsets.Offsets[i];
        ped->SetPosn(leader->GetPosition() + CVector{ off.x, off.y, 0.f });

        // Snap the ped back to the ground
        bool memberGroundFound{};
        const auto& pedPos = ped->GetPosition();
        const float memberZ = std::max(CWorld::FindGroundZFor3DCoord({ pedPos.x, pedPos.y, pedPos.z + 1.f }, &memberGroundFound) + 1.f, pedPos.z);

        // The followers have to be in (roughly) the leader's sight, and on (roughly) the same height
        if (!memberGroundFound
            || std::abs(memberZ - leader->GetPosition().z) >= 1.f
            || !CWorld::GetIsLineOfSightClear(pedPos, leader->GetPosition(), true, false, false, false)
        ) {
            CPopulation::RemovePed(ped);
            continue;
        }

        ped->SetPosn(pedPos.x, pedPos.y, memberZ);

        createdPeds[numCreated++] = ped;
        CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);
    }

    if (numCreated < 1) {
        for (uint32 i = 0; i < numCreated; i++) {
            CPopulation::RemovePed(createdPeds[i]);
        }
        return false;
    }

    auto& group = CPedGroups::GetGroup(groupId);
    group.GetIntelligence().SetDefaultTaskAllocator(CPedGroupDefaultTaskAllocators::Get(ePedGroupDefaultTaskAllocatorType::FOLLOW_ANY_MEANS));
    group.GetMembership().SetLeader(createdPeds[0]);
    group.GetMembership().Process();
    group.GetIntelligence().Process();

    createdPeds[0]->GetTaskManager().SetTask(new CTaskComplexBeInGroup(groupId, true), TASK_PRIMARY_PRIMARY);

    for (uint32 i = 1; i < numCreated; i++) {
        CPed* const ped = createdPeds[i];

        group.GetMembership().AddFollower(ped);
        group.GetMembership().Process();
        group.GetIntelligence().Process();

        ped->GetTaskManager().SetTask(new CTaskComplexBeInGroup(groupId, false), TASK_PRIMARY_PRIMARY);
    }

    return true;
}

// 0x5FCE80
bool CPedGroupPlacer::PlaceChatGroup(ePedType type, uint32 numOfPeds, const CVector& origin, ePedGroupDefaultTaskAllocatorType unused) {
    if (numOfPeds < 2) {
        return false;
    }

    const auto groupId = CPedGroups::AddGroup();
    if (groupId < 0) {
        return false;
    }

    constexpr uint32 kMaxPeds  = 8;             // Number of peds the original function has room for
    constexpr float  kRandNorm = 1.f / 32768.f; // `GetRandomNumber` returns a number in [0, 32767]

    // Arrange the peds on a ring around `origin`
    const auto stepAngle = TWO_PI / (float)numOfPeds;
    const auto radius    = std::sqrt(0.5f / (1.f - std::cos(stepAngle)));

    // Don't create the group in the player's view (Too close to him)
    if (TheCamera.IsSphereVisible(origin, radius)) {
        if ((FindPlayerPed()->GetPosition() - origin).Magnitude2D() < CPopulation::PedCreationDistMultiplier() * 42.5f) {
            return false;
        }
    }

    if (!CPedPlacement::IsPositionClearForPed(origin, radius, -1, nullptr, true, true, true)) {
        return false;
    }

    CPed*   createdPeds[kMaxPeds]{};
    uint32  numCreated{};
    CVector leaderPos{};
    for (uint32 i = 0; i < numOfPeds; i++) {
        // Position on the ring, with a bit of randomness so the group doesn't look too artificial
        const float angle = i * stepAngle + (CGeneral::GetRandomNumber() * kRandNorm * 0.4f - 0.2f) * stepAngle;
        const float dist  = radius * (CGeneral::GetRandomNumber() * kRandNorm * 0.4f - 0.2f) + radius;
        const float x     = std::cos(angle) * dist + origin.x;
        const float y     = std::sin(angle) * dist + origin.y;

        bool groundFound{};
        const float groundZ = CWorld::FindGroundZFor3DCoord({ x, y, origin.z + 1.f }, &groundFound);
        if (!groundFound) {
            continue;
        }

        const CVector pos{ x, y, std::max(groundZ + 1.f, origin.z) };
        if (i == 0) { // The first ped is the group's leader
            leaderPos = pos;
        }

        const auto modelId   = CPopulation::ChooseGangOccupation(static_cast<eGangID>(type - PED_TYPE_GANG1));
        const auto modelInfo = CModelInfo::GetPedModelInfo(modelId);
        if (!modelInfo->GetRwObject()) { // Model isn't streamed in
            continue;
        }

        // Reject the position if there's anything in the way (Except for the peds placed so far)
        CEntity* hitEntities[9]{};
        CPedPlacement::IsPositionClearForPed(pos, modelInfo->GetColModel()->GetBoundRadius(), 9, hitEntities, true, true, true);

        bool blockedByOtherEntity{};
        for (const auto hitEntity : hitEntities) {
            if (!hitEntity) {
                continue;
            }
            if (std::find(createdPeds, createdPeds + numCreated, hitEntity) == createdPeds + numCreated) {
                blockedByOtherEntity = true;
                break;
            }
        }

        // The followers have to be in the leader's sight, and on (roughly) the same height
        const bool hasLineOfSightToLeader = numCreated == 0 || CWorld::GetIsLineOfSightClear(pos, leaderPos, true, false, false, false);
        const bool isNearLeaderZ          = std::abs(pos.z - leaderPos.z) < 1.f;

        if (blockedByOtherEntity || !hasLineOfSightToLeader || !isNearLeaderZ) {
            continue;
        }

        CPed* const ped = CPopulation::AddPed(type, modelId, pos, false);
        if (!ped) {
            CPopulation::RemovePed(nullptr); // Original passes `nullptr` here
            continue;
        }

        createdPeds[numCreated++] = ped;

        const auto heading = CGeneral::GetRadianAngleBetweenPoints(origin.x, origin.y, pos.x, pos.y);
        ped->m_fCurrentRotation = heading;
        ped->m_fAimingRotation  = heading;
        CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);
    }

    if (numCreated < 1) {
        for (uint32 i = 0; i < numCreated; i++) {
            CPopulation::RemovePed(createdPeds[i]);
        }
        return false;
    }

    auto& group = CPedGroups::GetGroup(groupId);
    group.GetIntelligence().SetDefaultTaskAllocator(CPedGroupDefaultTaskAllocators::Get(ePedGroupDefaultTaskAllocatorType::STAND_STILL));
    group.GetMembership().SetLeader(createdPeds[0]);
    group.GetMembership().Process();
    group.GetIntelligence().Process();

    createdPeds[0]->GetTaskManager().SetTask(new CTaskComplexBeInGroup(groupId, true), TASK_PRIMARY_PRIMARY);

    for (uint32 i = 1; i < numCreated; i++) {
        CPed* const ped = createdPeds[i];

        group.GetMembership().AddFollower(ped);
        group.GetMembership().Process();
        group.GetIntelligence().Process();

        ped->GetTaskManager().SetTask(new CTaskComplexBeInGroup(groupId, false), TASK_PRIMARY_PRIMARY);
    }

    return true;
}

// 0x5FD330
bool CPedGroupPlacer::PlaceRandomGroup(ePedType type, uint32 numOfPeds, const CVector& origin, ePedGroupDefaultTaskAllocatorType unused) {
    if (numOfPeds < 2) {
        return false;
    }

    const auto groupId = CPedGroups::AddGroup();
    if (groupId < 0) {
        return false;
    }

    constexpr uint32 kMaxPeds  = 8;             // Number of peds the original function has room for
    constexpr float  kRandNorm = 1.f / 32768.f; // `GetRandomNumber` returns a number in [0, 32767]

    // Arrange the peds on a ring around `origin`
    const auto stepAngle = TWO_PI / (float)numOfPeds;
    const auto radius    = std::sqrt(0.5f / (1.f - std::cos(stepAngle)));

    // Don't create the group in the player's view
    if (TheCamera.IsSphereVisible(origin, radius)) {
        if ((FindPlayerPed()->GetPosition() - origin).Magnitude2D() < CPopulation::PedCreationDistMultiplier() * 42.5f) {
            return false;
        }
    }

    if (!CPedPlacement::IsPositionClearForPed(origin, radius, -1, nullptr, true, true, true)) {
        return false;
    }

    CPed*   createdPeds[kMaxPeds]{};
    uint32  numCreated{};
    CVector leaderPos{};
    for (uint32 i = 0; i < numOfPeds; i++) {
        // Position on the ring, with a bit of randomness so the group doesn't look too artificial
        const float angle = i * stepAngle + (CGeneral::GetRandomNumber() * kRandNorm * 0.4f - 0.2f) * stepAngle;
        const float dist  = radius * (CGeneral::GetRandomNumber() * kRandNorm * 0.4f - 0.2f) + radius;
        const float x     = std::cos(angle) * dist + origin.x;
        const float y     = std::sin(angle) * dist + origin.y;

        bool groundFound{};
        const float groundZ = CWorld::FindGroundZFor3DCoord({ x, y, origin.z + 1.f }, &groundFound);
        if (!groundFound) {
            continue;
        }

        const CVector pos{ x, y, std::max(groundZ + 1.f, origin.z) };

        if (i == 0) { // The first ped is the group's leader
            leaderPos = pos;
        }

        const auto modelId   = CPopulation::ChooseGangOccupation(static_cast<eGangID>(type - PED_TYPE_GANG1));
        const auto modelInfo = CModelInfo::GetPedModelInfo(modelId);
        if (!modelInfo->GetRwObject()) { // Model isn't streamed in
            continue;
        }

        // Reject the position if there's anything in the way (Except for the peds placed so far)
        CEntity* hitEntities[9]{};
        CPedPlacement::IsPositionClearForPed(pos, modelInfo->GetColModel()->GetBoundRadius(), 9, hitEntities, true, true, true);

        bool blockedByOtherEntity{};
        for (const auto hitEntity : hitEntities) {
            if (!hitEntity) {
                continue;
            }
            if (std::find(createdPeds, createdPeds + numCreated, hitEntity) == createdPeds + numCreated) {
                blockedByOtherEntity = true;
                break;
            }
        }

        // The followers have to be in the leader's sight, and on (roughly) the same height
        const bool hasLineOfSightToLeader = i == 0 || CWorld::GetIsLineOfSightClear(pos, leaderPos, true, false, false, false);
        const bool isNearLeaderZ          = i == 0 || std::abs(pos.z - leaderPos.z) < 1.f;

        if (blockedByOtherEntity || !hasLineOfSightToLeader || !isNearLeaderZ) {
            continue;
        }

        CPed* const ped = CPopulation::AddPed(type, modelId, pos, false);
        if (!ped) {
            CPopulation::RemovePed(nullptr); // Original passes `nullptr` here
            continue;
        }

        createdPeds[numCreated++] = ped;

        const auto heading = CGeneral::GetRadianAngleBetweenPoints(origin.x, origin.y, pos.x, pos.y);
        ped->m_fCurrentRotation = heading;
        ped->m_fAimingRotation  = heading;
        CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);
    }

    if (numCreated == 0) {
        return false;
    }

    auto& group = CPedGroups::GetGroup(groupId);
    group.GetIntelligence().SetDefaultTaskAllocator(CPedGroupDefaultTaskAllocators::Get(ePedGroupDefaultTaskAllocatorType::RANDOM));

    for (uint32 i = 0; i < numCreated; i++) {
        CPed* const ped = createdPeds[i];

        if (i == 0) {
            group.GetMembership().SetLeader(ped);
        } else {
            group.GetMembership().AddFollower(ped);
        }
        group.GetMembership().Process();
        group.GetIntelligence().Process();

        ped->GetTaskManager().SetTask(
            new CTaskComplexWanderGang(PEDMOVE_WALK, (uint8)(CGeneral::GetRandomNumber() * (8.f * kRandNorm)), 5'000, true, 0.5f),
            TASK_PRIMARY_DEFAULT
        );
        ped->GetTaskManager().SetTask(
            new CTaskComplexBeInGroup(groupId, false),
            TASK_PRIMARY_PRIMARY
        );
    }

    return true;
}

// 0x5FD810
bool CPedGroupPlacer::PlaceGroup(ePedType type, uint32 numOfPeds, const CVector& origin, ePedGroupDefaultTaskAllocatorType allocType) {
    using enum ePedGroupDefaultTaskAllocatorType;
    switch (allocType) {
    case FOLLOW_ANY_MEANS:
    case FOLLOW_LIMITED:   return PlaceFormationGroup(type, numOfPeds, origin, allocType);
    case STAND_STILL:
    case CHAT:             return PlaceChatGroup(type, numOfPeds, origin, allocType);
    case RANDOM:           return PlaceRandomGroup(type, numOfPeds, origin, allocType);
    default:               NOTSA_UNREACHABLE();
    }
}
