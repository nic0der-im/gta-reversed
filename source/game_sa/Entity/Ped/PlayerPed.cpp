/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "PlayerPed.h"
#include "TagManager.h"
#include "PedClothesDesc.h"
#include "PedStats.h"
#include "TaskSimpleUseGun.h"
#include "EntryExitManager.h"
#include "MBlur.h"
#include "TaskSimpleFight.h"
#include "TaskComplexSmartFleeEntity.h"
#include "TaskComplexBeInGroup.h"
#include "TaskComplexFacial.h"
#include "TaskSimplePlayerOnFoot.h"
#include "EventDontJoinPlayerGroup.h"
#include "EventNewGangMember.h"

bool CPlayerPed::bDebugPlayerInvincible;
bool CPlayerPed::bDebugTargeting;
bool CPlayerPed::bDebugTapToTarget;

void CPlayerPed::InjectHooks() {
    RH_ScopedVirtualClass(CPlayerPed, 0x86D168, 26);
    RH_ScopedCategory("Entity/Ped");

    RH_ScopedInstall(ResetSprintEnergy, 0x60A530);
    RH_ScopedInstall(ResetPlayerBreath, 0x60A8A0);
    RH_ScopedInstall(RemovePlayerPed, 0x6094A0);
    RH_ScopedInstall(Busted, 0x609EF0);
    RH_ScopedInstall(GetWantedLevel, 0x41BE60);
    RH_ScopedInstall(SetWantedLevel, 0x609F10);
    RH_ScopedInstall(SetWantedLevelNoDrop, 0x609F30);
    RH_ScopedInstall(CheatWantedLevel, 0x609F50);
    RH_ScopedInstall(DoStuffToGoOnFire, 0x60A020);
    RH_ScopedVMTInstall(Load, 0x5D46E0);
    RH_ScopedVMTInstall(Save, 0x5D57E0);
    RH_ScopedInstall(DeactivatePlayerPed, 0x609520);
    RH_ScopedInstall(ReactivatePlayerPed, 0x609540);
    RH_ScopedInstall(GetPadFromPlayer, 0x609560);
    RH_ScopedInstall(CanPlayerStartMission, 0x609590);
    RH_ScopedInstall(IsHidden, 0x609620);
    RH_ScopedInstall(ReApplyMoveAnims, 0x609650);
    RH_ScopedInstall(DoesPlayerWantNewWeapon, 0x609710);
    RH_ScopedInstall(ProcessPlayerWeapon, 0x6097F0);
    RH_ScopedInstall(PickWeaponAllowedFor2Player, 0x609800);
    RH_ScopedInstall(UpdateCameraWeaponModes, 0x609830);
    RH_ScopedInstall(ClearWeaponTarget, 0x609c80);
    RH_ScopedInstall(GetWeaponRadiusOnScreen, 0x609CD0);
    RH_ScopedInstall(PedCanBeTargettedVehicleWise, 0x609D90);
    RH_ScopedInstall(FindTargetPriority, 0x609DE0);
    RH_ScopedInstall(Clear3rdPersonMouseTarget, 0x609ED0);
    RH_ScopedInstall(CanIKReachThisTarget, 0x609F80);
    RH_ScopedInstall(GetPlayerInfoForThisPlayerPed, 0x609FF0);
    RH_ScopedInstall(AnnoyPlayerPed, 0x60A040);
    RH_ScopedInstall(ClearAdrenaline, 0x60A070);
    RH_ScopedInstall(DisbandPlayerGroup, 0x60A0A0);
    RH_ScopedInstall(MakeGroupRespondToPlayerTakingDamage, 0x60A110);
    RH_ScopedInstall(TellGroupToStartFollowingPlayer, 0x60A1D0);
    RH_ScopedInstall(MakePlayerGroupDisappear, 0x60A440);
    RH_ScopedInstall(MakePlayerGroupReappear, 0x60A4B0);
    RH_ScopedInstall(HandleSprintEnergy, 0x60A550);
    RH_ScopedInstall(GetButtonSprintResults, 0x60A820);
    RH_ScopedInstall(HandlePlayerBreath, 0x60A8D0);
    RH_ScopedOverloadedInstall(MakeChangesForNewWeapon, "", 0x60B460, void(CPlayerPed::*)(eWeaponType));
    RH_ScopedGlobalInstall(LOSBlockedBetweenPeds, 0x60B550);
    RH_ScopedInstall(DoesTargetHaveToBeBroken, 0x60C0C0);
    RH_ScopedInstall(SetPlayerMoveBlendRatio, 0x60C520);
    RH_ScopedInstall(FindPedToAttack, 0x60C5F0);
    RH_ScopedInstall(ForceGroupToAlwaysFollow, 0x60C7C0);
    RH_ScopedInstall(ForceGroupToNeverFollow, 0x60C800);
    RH_ScopedOverloadedInstall(MakeChangesForNewWeapon, "BySlot", 0x60D000, void(CPlayerPed::*)(uint32));
    RH_ScopedInstall(EvaluateTarget, 0x60D020);
    RH_ScopedInstall(PlayerHasJustAttackedSomeone, 0x60D5A0);
    RH_ScopedInstall(SetupPlayerPed, 0x60D790);

    RH_ScopedVMTInstall(ProcessControl, 0x60EA90);
    RH_ScopedVMTInstall(SetMoveAnim, 0x609490);
    RH_ScopedInstall(ProcessAnimGroups, 0x6098F0);
    RH_ScopedInstall(ControlButtonSprint, 0x60A610);
    RH_ScopedInstall(SetRealMoveAnim, 0x60A9C0);
    RH_ScopedInstall(Compute3rdPersonMouseTarget, 0x60B650);
    RH_ScopedInstall(DrawTriangleForMouseRecruitPed, 0x60BA80);
    RH_ScopedInstall(KeepAreaAroundPlayerClear, 0x60C1E0);
    RH_ScopedInstall(MakeThisPedJoinOurGroup, 0x60C840);
    RH_ScopedInstall(PlayerWantsToAttack, 0x60CC50);
    RH_ScopedInstall(SetInitialState, 0x60CD20);
    RH_ScopedInstall(EvaluateNeighbouringTarget, 0x60D1C0);
    RH_ScopedInstall(ProcessGroupBehaviour, 0x60D350);
    RH_ScopedInstall(ProcessWeaponSwitch, 0x60D850);
    RH_ScopedInstall(FindWeaponLockOnTarget, 0x60DC50);
    RH_ScopedInstall(FindNextWeaponLockOnTarget, 0x60E530);
}

// TODO: To class and create 2 function
struct CPlayerPedDataSaveStructure {
    uint32          ChaosLevel{};
    eWantedLevel    WantedLevel{};
    CPedClothesDesc ClothesDesc{};
    uint32          ChosenWeapon{};
    // float          Multiplier{}; // Mobile
};

VALIDATE_SIZE(CPlayerPedDataSaveStructure, 0x84 /* + 0x4 */);

// 0x5D46E0
bool CPlayerPed::Load() {
    CPed::Load();

    CGenericGameStorage::LoadDataFromWorkBuffer<uint32>(); // Discard structure size
    auto sd = CGenericGameStorage::LoadDataFromWorkBuffer<CPlayerPedDataSaveStructure>();

    CWanted* wanted = GetPlayerWanted();
    wanted->m_ChaosLevel = sd.ChaosLevel;
    wanted->m_WantedLevel= sd.WantedLevel;

    *GetPlayerData()->m_pPedClothesDesc = sd.ClothesDesc;
    GetPlayerData()->m_nChosenWeapon   = sd.ChosenWeapon;

    return true;
}

// 0x5D57E0
bool CPlayerPed::Save() {
    CPlayerPedDataSaveStructure saveData{};

    CWanted* wanted = GetPlayerWanted();
    saveData.ChaosLevel = wanted->m_ChaosLevel;
    saveData.WantedLevel = wanted->m_WantedLevel;
    saveData.ChosenWeapon = GetPlayerData()->m_nChosenWeapon;
    saveData.ClothesDesc  = *GetPlayerData()->m_pPedClothesDesc;

    CPed::Save();
    CGenericGameStorage::SaveDataToWorkBuffer(sizeof(CPlayerPedDataSaveStructure));
    CGenericGameStorage::SaveDataToWorkBuffer(saveData);

    return true;
}

// 0x60D5B0
CPlayerPed::CPlayerPed(int32 playerId, bool bGroupCreated) : CPed(PED_TYPE_PLAYER1) {
    m_pPlayerData = &CWorld::Players[playerId].m_PlayerData;
    GetPlayerData()->AllocateData();

    CPed::SetModelIndex(MODEL_PLAYER);

    CPlayerPed::SetInitialState(bGroupCreated);

    CEntity::ClearReference(m_pTargetedObject);

    SetPedState(PEDSTATE_IDLE);

    gPlayIdlesAnimBlockIndex = CAnimManager::GetAnimationBlockIndex("playidles");

    if (!bGroupCreated) {
        GetPlayerData()->m_nPlayerGroup = CPedGroups::AddGroup();

        auto& group = CPedGroups::GetGroup(GetPlayerData()->m_nPlayerGroup);
        group.GetIntelligence().SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);
        group.m_bIsMissionGroup = true;
        group.m_groupMembership.SetLeader(this);
        group.Process();

        GetPlayerData()->m_bGroupStuffDisabled = false;
        GetPlayerData()->m_bGroupAlwaysFollow  = false;
        GetPlayerData()->m_bGroupNeverFollow   = false;
    }

    m_fMaxHealth = CStats::GetFatAndMuscleModifier(STAT_MOD_MAX_HEALTH);
    m_fHealth    = m_fMaxHealth;

    m_nFightingStyle      = STYLE_GRAB_KICK;
    m_nAllowedAttackMoves = 15;

    m_p3rdPersonMouseTarget = nullptr;
    field_7A0 = 0;
    m_pedSpeech.Initialise(this);
    GetIntelligence()->m_fDmRadius = 30.0f;
    GetIntelligence()->m_nDmNumPedsToScan = 2;

    bUsedForReplay = bGroupCreated;
}

// 0x6094A0
void CPlayerPed::RemovePlayerPed(int32 playerId) {
    CPed* player = FindPlayerPed(playerId);
    CPlayerInfo* playerInfo = &FindPlayerInfo(playerId);
    if (player)
    {
        CVehicle* playerVehicle = player->m_pVehicle;
        if (playerVehicle && playerVehicle->m_pDriver == player)
        {
            playerVehicle->SetStatus(STATUS_PHYSICS);
            playerVehicle->m_GasPedal = 0.0f;
            playerVehicle->m_BrakePedal = 0.1f;
        }
        CWorld::Remove(static_cast<CEntity*>(player));
        delete player;
        playerInfo->m_pPed = nullptr;
    }
}

// 0x609520
void CPlayerPed::DeactivatePlayerPed(int32 playerId) {
    assert(playerId >= 0);
    CWorld::Remove(FindPlayerPed(playerId));
}

// 0x609540
void CPlayerPed::ReactivatePlayerPed(int32 playerId) {
    assert(playerId >= 0);
    CWorld::Add(FindPlayerPed(playerId));
}

// 0x609560
CPad* CPlayerPed::GetPadFromPlayer() const {
    switch (m_nPedType) {
    case PED_TYPE_PLAYER1:
        return CPad::GetPad(0);

    case PED_TYPE_PLAYER2:
        return CPad::GetPad(1);
    }
    assert(0); // shouldn't happen
    return nullptr;
}

// 0x609590
bool CPlayerPed::CanPlayerStartMission() {
    if (CGameLogic::GameState != GAMELOGIC_STATE_PLAYING || CGameLogic::IsCoopGameGoingOn())
        return false;

    if (!IsPedInControl() && !IsStateDriving())
        return false;

    auto& taskMgr = GetTaskManager();
    if (taskMgr.GetTaskPrimary(TASK_PRIMARY_PHYSICAL_RESPONSE))
        return false;

    if (taskMgr.GetTaskPrimary(TASK_PRIMARY_EVENT_RESPONSE_NONTEMP))
        return false;

    auto primaryTask = taskMgr.GetTaskPrimary(TASK_PRIMARY_PRIMARY);
    if (primaryTask != nullptr && primaryTask->GetTaskType() != TASK_SIMPLE_CAR_DRIVE)
        return false;

    if (taskMgr.GetTaskSecondary(TASK_SECONDARY_ATTACK))
        return false;

    if (!IsAlive())
        return false;

    return !GetEventGroup().GetEventOfType(EVENT_SCRIPT_COMMAND);
}

// 0x609620
bool CPlayerPed::IsHidden() {
    return !bInVehicle && GetLightingTotal() <= 0.05f;
}

// 0x609650
void CPlayerPed::ReApplyMoveAnims() {
    constexpr AnimationId anims[]{
        ANIM_ID_WALK,
        ANIM_ID_RUN,
        ANIM_ID_SPRINT,
        ANIM_ID_IDLE,
        ANIM_ID_WALK_START
    };
    for (const AnimationId& id : anims) {
        if (CAnimBlendAssociation* anim = RpAnimBlendClumpGetAssociation(GetRpClump(), id)) {
            if (anim->GetHashKey() != CAnimManager::GetAnimAssociation(m_nAnimGroup, id)->GetHashKey()) {
                CAnimBlendAssociation* addedAnim = CAnimManager::AddAnimation(GetRpClump(), m_nAnimGroup, id);
                addedAnim->m_BlendDelta = anim->m_BlendDelta;
                addedAnim->m_BlendAmount = anim->m_BlendAmount;

                anim->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                anim->m_BlendDelta = -1000.0f;
            }
        }
    }
}

// 0x609710
bool CPlayerPed::DoesPlayerWantNewWeapon(eWeaponType weaponType, bool arg1) {
    // GetPadFromPlayer(); // Called, but not used

    auto weaponSlot = GetWeaponSlot(weaponType);
    auto weaponInSlotType = GetWeaponInSlot(weaponSlot).m_Type;
    if (weaponInSlotType == weaponType)
        return true;

    if (weaponInSlotType == eWeaponType::WEAPON_UNARMED)
        return true;

    if (arg1)
        return false;

    if (GetIntelligence()->GetTaskJetPack())
        return false;

    /* !See comment!
    if (m_nActiveWeaponSlot == weaponSlot
        && CWeaponInfo::GetWeaponInfo(weaponInSlotType, GetWeaponSkill(weaponInSlotType))->flags.bAimWithArm   \ One of these two is always false, so
        && !CWeaponInfo::GetWeaponInfo(weaponInSlotType, GetWeaponSkill(weaponInSlotType))->flags.bAimWithArm) / the whole expression is always false
        return false;
    */

    if (m_nActiveWeaponSlot == weaponSlot) {
        switch (m_nPedState) {
        case PEDSTATE_ATTACK:
        case PEDSTATE_AIMGUN:
            return false;
        }
    }

    return true;
}

// 0x6097F0
void CPlayerPed::ProcessPlayerWeapon(CPad* pad) {
    /* empty */
}

// 0x609800
void CPlayerPed::PickWeaponAllowedFor2Player() {
    if (!GetWeaponInSlot(GetPlayerData()->m_nChosenWeapon).CanBeUsedFor2Player()) {
        GetPlayerData()->m_nChosenWeapon = eWeaponType::WEAPON_UNARMED;
    }
}

// 0x609830
void CPlayerPed::UpdateCameraWeaponModes(CPad* pad) {
    switch (GetActiveWeapon().m_Type) {
    case eWeaponType::WEAPON_M4:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_M16_1STPERSON, 0, 0);
        break;

    case eWeaponType::WEAPON_SNIPERRIFLE:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_SNIPER, 0, 0);
        break;

    case eWeaponType::WEAPON_RLAUNCHER:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_ROCKETLAUNCHER, 0, 0);
        break;

    case eWeaponType::WEAPON_RLAUNCHER_HS:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_ROCKETLAUNCHER_HS, 0, 0);
        break;

    case eWeaponType::WEAPON_CAMERA:
        TheCamera.SetNewPlayerWeaponMode(eCamMode::MODE_CAMERA, 0, 0);
        break;

    default:
        TheCamera.ClearPlayerWeaponMode();
        break;
    }
}

// 0x6098F0
void CPlayerPed::ProcessAnimGroups() {
    CPlayerPedData* const playerData = GetPlayerData();

    // Anim groups are resolved relative to the "generic" player group (The body type, and thus the
    // group, may change at any moment, so only offsets from `ANIM_GROUP_PLAYER` are ever stored)
    const auto ClothesOffsetGroup = [](int32 group) {
        return group - +ANIM_GROUP_PLAYER + +CClothes::GetDefaultPlayerMotionGroup();
    };

    // The 2nd player uses the anim group of it's model (If it has one)
    const auto DefaultPlayerGroup = [this]() -> int32 {
        if (m_nPedType == PED_TYPE_PLAYER2) {
            const auto modelGroup = static_cast<CPedModelInfo*>(GetModelInfo())->m_nAnimType;
            if (modelGroup != ANIM_GROUP_PLAYER) {
                return +modelGroup;
            }
        }
        return +CClothes::GetDefaultPlayerMotionGroup();
    };

    int32 newGroup;
    // When aiming far enough to the side while using the mouse camera the strafe group is kept,
    // otherwise it's re-evaluated (based on the currently held weapon)
    if (   const float aimHeading = playerData->m_fFPSMoveHeading;
           (aimHeading > -0.87266463f && aimHeading < 0.87266463f) // +-50 degrees
        || !TheCamera.GetActiveCam().Using3rdPersonMouseCam()
        || !CanStrafeOrMouseControl()
    ) {
        eWeaponType weaponType = WEAPON_UNARMED;
        if (m_pWeaponObject) {
            if (CClumpModelInfo* const clumpMI = CVisibilityPlugins::GetClumpModelInfo(m_pWeaponObject);
                clumpMI && clumpMI->GetModelType() == MODEL_INFO_WEAPON
            ) {
                weaponType = static_cast<CWeaponModelInfo*>(clumpMI)->GetWeaponInfo();
            }
        }

        switch (weaponType) {
        case WEAPON_RLAUNCHER:
        case WEAPON_RLAUNCHER_HS:
            newGroup = ClothesOffsetGroup(ANIM_GROUP_PLAYERROCKET);
            break;
        case WEAPON_BASEBALLBAT:
        case WEAPON_SHOVEL:
        case WEAPON_POOL_CUE:
            newGroup = ClothesOffsetGroup(ANIM_GROUP_PLAYERBBBAT);
            break;
        case WEAPON_CHAINSAW:
        case WEAPON_FLAMETHROWER:
        case WEAPON_MINIGUN:
            newGroup = ClothesOffsetGroup(ANIM_GROUP_PLAYERCSAW);
            break;
        case WEAPON_SHOTGUN:
        case WEAPON_SPAS12_SHOTGUN:
        case WEAPON_AK47:
        case WEAPON_M4:
        case WEAPON_COUNTRYRIFLE:
        case WEAPON_SNIPERRIFLE:
            newGroup = ClothesOffsetGroup(ANIM_GROUP_PLAYER2ARMED);
            break;
        default:
            if (GetIntelligence()->GetTaskJetPack()) {
                newGroup = ANIM_GROUP_PLAYERJETPACK;
            } else if (GetClothesDesc()->GetIsWearingBalaclava()) {
                newGroup = ANIM_GROUP_PLAYERSNEAK;
            } else {
                newGroup = DefaultPlayerGroup();
            }
            break;
        }
    } else if (m_nAnimGroup == ANIM_GROUP_PLAYER) {
        newGroup = DefaultPlayerGroup();
    } else {
        newGroup = ClothesOffsetGroup(m_nAnimGroup);
    }

    if (m_nAnimGroup != newGroup) {
        m_nAnimGroup = static_cast<AssocGroupId>(newGroup);
        ReApplyMoveAnims();
    }

    // Stream the melee (and secondary melee) animation blocks needed by the held weapon
    const CWeaponInfo* const weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());

    const auto GetMeleeGroup = [](uint8 baseCombo) -> AssocGroupId {
        return CTaskSimpleFight::m_aComboData[baseCombo - MELEE_COMBO_UNARMED_1].m_nAnimGroup;
    };
    // The anim block of the given group (and it's index), resolving it if the assoc group doesn't have one
    const auto GetBlock = [](AssocGroupId group) {
        CAnimBlock* block = CAnimManager::GetAnimationBlock(group);
        if (!block) {
            block = CAnimManager::GetAnimationBlock(CAnimManager::GetAnimBlockName(group));
        }
        return std::make_pair(block, CAnimManager::GetAnimationBlockIndex(block));
    };

    // Makes sure the group's block is loaded (that is, streams it in if it isn't),
    // and references it if `ref` is true. Returns whether the block is loaded.
    const auto UseGroup = [&](AssocGroupId group, bool ref) {
        const auto [block, blockIdx] = GetBlock(group);
        if (!block->IsLoaded) {
            CStreaming::RequestModel(RESOURCE_ID_IFP + blockIdx, STREAMING_KEEP_IN_MEMORY);
            return false;
        }
        if (ref) {
            CAnimManager::AddAnimBlockRef(blockIdx);
        }
        return true;
    };

    bool unloadPrimary = false;
    bool unloadSecondary = false;

    if (weaponInfo->m_nWeaponFire == WEAPON_FIRE_MELEE && !bInVehicle) {
        // Primary melee group (Based on the weapon's base combo)
        if (weaponInfo->m_nBaseCombo == STYLE_STANDARD) {
            unloadPrimary = playerData->m_nMeleeWeaponAnimReferenced != 0;
        } else if (const auto group = GetMeleeGroup(weaponInfo->m_nBaseCombo);
                   group != playerData->m_nMeleeWeaponAnimReferenced
        ) {
            unloadPrimary = playerData->m_nMeleeWeaponAnimReferenced != 0;
            if (!playerData->m_nMeleeWeaponAnimReferenced && UseGroup(group, true)) {
                playerData->m_nMeleeWeaponAnimReferenced = group;
            } else {
                UseGroup(group, false);
            }
        }

        // Secondary melee group (Based on the ped's fight style)
        if (m_nFightingStyle == STYLE_STANDARD) {
            unloadSecondary = playerData->m_nMeleeWeaponAnimReferencedExtra != 0;
        } else if (const auto group = GetMeleeGroup(m_nFightingStyle);
                   group != playerData->m_nMeleeWeaponAnimReferencedExtra
        ) {
            unloadSecondary = playerData->m_nMeleeWeaponAnimReferencedExtra != 0;
            if (!playerData->m_nMeleeWeaponAnimReferencedExtra && UseGroup(group, true)) {
                playerData->m_nMeleeWeaponAnimReferencedExtra = group;
            } else {
                UseGroup(group, false);
            }
        }
    } else {
        unloadSecondary = true;
    }

    if (unloadPrimary && playerData->m_nMeleeWeaponAnimReferenced) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(static_cast<AssocGroupId>(playerData->m_nMeleeWeaponAnimReferenced)));
        playerData->m_nMeleeWeaponAnimReferenced = 0;
    }
    if (unloadSecondary && playerData->m_nMeleeWeaponAnimReferencedExtra) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(static_cast<AssocGroupId>(playerData->m_nMeleeWeaponAnimReferencedExtra)));
        playerData->m_nMeleeWeaponAnimReferencedExtra = 0;
    }
}

// 0x609C80
void CPlayerPed::ClearWeaponTarget() {
    if (IsPlayer()) {
        CEntity::ClearReference(m_pTargetedObject);
        TheCamera.ClearPlayerWeaponMode();
        CWeaponEffects::ClearCrossHair(m_nPedType);
    }
}

// 0x609CD0
float CPlayerPed::GetWeaponRadiusOnScreen() {
    CWeapon& wep = GetActiveWeapon();
    CWeaponInfo& wepInfo = wep.GetWeaponInfo(this);

    if (wep.IsTypeMelee())
        return 0.0f;

    const float accuracyProg = 0.5f / wepInfo.m_fAccuracy;
    switch (wep.m_Type) {
    case eWeaponType::WEAPON_SHOTGUN:
    case eWeaponType::WEAPON_SPAS12_SHOTGUN:
    case eWeaponType::WEAPON_SAWNOFF_SHOTGUN:
        return std::max(0.2f, accuracyProg); // here they multiply *accuracyProg * 1.0f* :thinking

    default: {
        const float rangeProg = std::min(1.0f, 15.0f / wepInfo.m_fWeaponRange);
        const float radius = (GetPlayerData()->m_fAttackButtonCounter * 0.5f + 1.0f) * rangeProg * accuracyProg;
        if (bIsDucking)
            return std::max(0.2f, radius / 2.0f);
        return std::max(0.2f, radius);
    }
    }
}

// 0x609D90
bool CPlayerPed::PedCanBeTargettedVehicleWise(CPed* ped) {
    if (ped->bInVehicle) {
        CVehicle* veh = ped->m_pVehicle;
        return veh && (veh->IsBike() || veh->vehicleFlags.bVehicleCanBeTargetted);
    }
    return true;
}

// 0x609DE0
float CPlayerPed::FindTargetPriority(CEntity* entity) {
    switch (entity->GetType()) {
    case ENTITY_TYPE_VEHICLE:
        return 0.1f;

    case ENTITY_TYPE_PED: {
        auto ped = entity->AsPed();

        if (ped->bThisPedIsATargetPriority)
            return 1.0f;

        if (ped->GetTaskManager().HasAnyOf<TASK_COMPLEX_KILL_PED_ON_FOOT, TASK_COMPLEX_ARREST_PED>()) {
            return 0.8f;
        }

        if (CPedGroups::AreInSameGroup(this, ped))
            return 0.05f;

        if (ped->m_nPedType == PED_TYPE_GANG2)
            return 0.06f;

        if (ped->IsCreatedByMission())
            return 0.25f;

        return 0.1f;
    }

    case ENTITY_TYPE_OBJECT: {
        switch (entity->AsObject()->m_nObjectType) {
        case eObjectType::OBJECT_MISSION:
        case eObjectType::OBJECT_MISSION2:
            return 0.1f;

        default:
            return 0.0f;
        }
    }
    default: {
        return 0.1f;
    }
    }
}

// 0x609ED0
void CPlayerPed::Clear3rdPersonMouseTarget() {
    CEntity::ClearReference(m_p3rdPersonMouseTarget);
}

// 0x609EF0
void CPlayerPed::Busted() {
    CWanted* wanted = GetWanted();
    if (wanted) {
        wanted->m_ChaosLevel = 0;
    }
}

// 0x41BE60
eWantedLevel CPlayerPed::GetWantedLevel() const {
    if (const auto* wanted = GetWanted()) {
        return wanted->GetWantedLevel();
    }

    return eWantedLevel::WANTED_CLEAN;
}

// 0x609F10
void CPlayerPed::SetWantedLevel(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->SetWantedLevel(level);
}

// 0x609F30
void CPlayerPed::SetWantedLevelNoDrop(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->SetWantedLevelNoDrop(level);
}

// 0x609F50
void CPlayerPed::CheatWantedLevel(eWantedLevel level) {
    CWanted* wanted = GetWanted();
    wanted->CheatWantedLevel(level);
}

// 0x609F80
bool CPlayerPed::CanIKReachThisTarget(CVector posn, CWeapon* weapon, bool arg2) {
    if (!weapon->GetWeaponInfo(this).flags.bAimWithArm) {
        const CVector thisPos = GetPosition();
        return (posn - thisPos).Magnitude2D() >= thisPos.z - posn.z;
    }
    return true;
}

// 0x609FF0
CPlayerInfo* CPlayerPed::GetPlayerInfoForThisPlayerPed() {
    // TODO: Use range for here
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (FindPlayerPed(i) == this)
            return &FindPlayerInfo(i);
    }
    return nullptr;
}

// 0x60A020
void CPlayerPed::DoStuffToGoOnFire() {
    if (m_nPedState == PEDSTATE_SNIPER_MODE)
        TheCamera.ClearPlayerWeaponMode();
}

// 0x60A040
void CPlayerPed::AnnoyPlayerPed(bool arg0) {
    auto& temper = m_pStats->m_nTemper;

    if (temper < 52) {
        temper++;
    } else if (arg0) {
        if (temper < 55)
            temper++;
        else
            temper = 46;
    }
}

// 0x60A070
void CPlayerPed::ClearAdrenaline() {
    if (GetPlayerData()->m_bAdrenaline && GetPlayerData()->m_nAdrenalineEndTime != 0) {
        GetPlayerData()->m_nAdrenalineEndTime = 0;
        CTimer::ResetTimeScale();
    }
}

// 0x60A0A0
void CPlayerPed::DisbandPlayerGroup() {
    auto& ms = GetPlayerGroup().GetMembership();
    if (const auto numMembers = ms.CountMembersExcludingLeader()) {
        Say(numMembers > 1 ? CTX_GLOBAL_ORDER_DISBAND_MANY : CTX_GLOBAL_ORDER_ATTACK_SINGLE);
    } else {
        ms.RemoveAllFollowers(true);
    }
}

// 0x60A110
void CPlayerPed::MakeGroupRespondToPlayerTakingDamage(CEventDamage& damageEvent) {
    auto& group = GetPlayerGroup();
    if (!damageEvent.m_pSourceEntity)
        return;
    if (group.GetMembership().CountMembersExcludingLeader() < 1)
        return;
    if (!group.m_bMembersEnterLeadersVehicle)
        return;

    CEventGroupEvent groupEvent(this, damageEvent.Clone());
    group.GetIntelligence().AddEvent(&groupEvent);
}

// 0x60A1D0
void CPlayerPed::TellGroupToStartFollowingPlayer(bool arg0, bool arg1, bool arg2) {
    if (GetPlayerData()->m_bGroupAlwaysFollow && !arg0)
        return;
    if (GetPlayerData()->m_bGroupNeverFollow && arg0)
        return;

    CPedGroup& group = GetPlayerGroup();
    CPedGroupIntelligence& groupIntel = group.GetIntelligence();
    CPedGroupMembership& membership = group.GetMembership();
    if (!arg2 && !membership.CountMembersExcludingLeader())
        return;

    group.m_bMembersEnterLeadersVehicle = arg0;
    groupIntel.SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);
    if (arg0) {
        CEventPlayerCommandToGroup playerCmdEvent;
        playerCmdEvent.ComputeResponseTaskType(&group);
        if (playerCmdEvent.WillRespond()) {
            auto gatherCmdEvent = new CEventPlayerCommandToGroup(ePlayerGroupCommand::PLAYER_GROUP_COMMAND_GATHER);
            gatherCmdEvent->m_TaskId = playerCmdEvent.m_TaskId;

            CEventGroupEvent groupEvent(this, gatherCmdEvent);
            groupIntel.AddEvent(&groupEvent);
        }
    }

    if (arg1) {
        const uint32 nMembers = membership.CountMembersExcludingLeader();
        if (!nMembers)
            return;

        if (arg0) {
            const float distToFurthest = group.FindDistanceToFurthestMember();
            if (nMembers > 1) {
                if (distToFurthest >= 3.0f)
                    Say(distToFurthest >= 10.0f ? CTX_GLOBAL_ORDER_FOLLOW_FAR_MANY : CTX_GLOBAL_ORDER_FOLLOW_NEAR_MANY);
                else
                    Say(CTX_GLOBAL_ORDER_FOLLOW_VNEAR_MANY);
            } else {
                if (distToFurthest >= 3.0f)
                    Say(distToFurthest >= 10.0f ? CTX_GLOBAL_ORDER_FOLLOW_FAR_ONE : CTX_GLOBAL_ORDER_FOLLOW_NEAR_ONE);
                else
                    Say(CTX_GLOBAL_ORDER_FOLLOW_VNEAR_ONE);
            }
        } else if (nMembers > 1) {
            Say(CTX_GLOBAL_ORDER_WAIT_MANY);
        } else {
            Say(CTX_GLOBAL_ORDER_WAIT_ONE);
        }
    }
}

// 0x60A440
void CPlayerPed::MakePlayerGroupDisappear() {
    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; i++) {
        if (CPed* member = membership.GetMember(i)) {
            if (!member->IsCreatedByMission()) {
                member->SetCollisionProcessed(false);
                member->SetIsVisible(false);
                abTempNeverLeavesGroup[i] = member->bNeverLeavesGroup;
                member->bNeverLeavesGroup = true;
            }
        }
    }
}

// 0x60A4B0
void CPlayerPed::MakePlayerGroupReappear() {
    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; i++) {
        if (CPed* member = membership.GetMember(i)) {
            if (!member->IsCreatedByMission()) {
                member->SetIsVisible(true);
                if (!member->bInVehicle)
                    member->SetUsesCollision(true);
                member->bNeverLeavesGroup = abTempNeverLeavesGroup[i];
            }
        }
    }
}

// 0x60A530
void CPlayerPed::ResetSprintEnergy()
{
    GetPlayerData()->m_fTimeCanRun = CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN);
}

// 0x60A550
bool CPlayerPed::HandleSprintEnergy(bool sprint, float adrenalineConsumedPerTimeStep) {
    float& timeCanRun = GetPlayerData()->m_fTimeCanRun;
    if (sprint) {
        if (FindPlayerInfo().m_bDoesNotGetTired)
            return true;
        if (GetPlayerData()->m_bAdrenaline || adrenalineConsumedPerTimeStep == 0.0f)
            return true;

        if (timeCanRun > -150.0f) { // TODO: Find out what this magic number is
            timeCanRun = std::max(-150.0f, timeCanRun - CTimer::GetTimeStep() * adrenalineConsumedPerTimeStep);
            return true;
        }
    } else {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN) > timeCanRun) {
            timeCanRun += CTimer::GetTimeStep() * adrenalineConsumedPerTimeStep / 2.0f;
        }
    }
    return false;
}

constexpr auto PLAYER_SPRINT_THRESHOLD{ 5.0f }; // 0x8D2458
constexpr struct tPlayerSprintSet { // From 0x8D2460
    float field_0;
    float field_4;
    float field_8;
    float field_C;
    float field_10;
    float field_14;
    float field_18;
    float field_1C;
} PLAYER_SPRINT_SET[] = {
    // 0x0, 0x4,  0x8,  0xC,  0x10,  0x14, 0x18, 0x1C
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 1.0f, 0.5f, 0.3f }, // GROUND
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 0.0f, 0.4f, 1.0f }, // BMX
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 1.0f, 0.3f, 0.3f }, // WATER
    { 4.0f, 0.7f, 0.2f, 5.0f, 10.0f, 0.0f, 0.0f, 1.0f }  // UNDERWATER
};

// 0x60A610
float CPlayerPed::ControlButtonSprint(eSprintType sprintType) {
    CPlayerPedData* const playerData = GetPlayerData();
    if (!playerData) {
        return 0.0f;
    }

    CPad* const pad = GetPadFromPlayer();
    if (!pad) {
        return 0.0f;
    }

    const auto& sprintSet = PLAYER_SPRINT_SET[sprintType];

    const bool wantsToSprint = !playerData->m_bPlayerSprintDisabled
        && (playerData->m_fMoveSpeed > 0.0f || playerData->m_fTimeCanRun > 0.0f);

    float& speed = playerData->m_fMoveSpeed;
    if (pad->SprintJustDown() && wantsToSprint) {
        speed = std::min(sprintSet.field_0 + speed, sprintSet.field_10);
    } else if (pad->GetSprint() && wantsToSprint) {
        speed = std::max(speed - CTimer::GetTimeStep() * sprintSet.field_4, 1.0f);
    } else if (speed > 0.0f) {
        speed = std::max(speed - CTimer::GetTimeStep() * sprintSet.field_8, 0.0f);
    }

    float progress;
    float energyCost;
    if (speed <= sprintSet.field_C) {
        if (speed <= 0.0f || !wantsToSprint) {
            return 0.0f;
        }
        progress   = 1.0f;
        energyCost = sprintSet.field_14;
    } else {
        progress   = speed / sprintSet.field_C;
        energyCost = sprintSet.field_18;
        if (progress <= 0.0f) {
            return 0.0f;
        }
    }

    if (!HandleSprintEnergy(true, energyCost)) {
        speed = 0.0f;
        return 0.0f;
    }

    return std::max(progress - 1.0f, 0.0f) * sprintSet.field_1C + 1.0f;
}

// 0x60A820
float CPlayerPed::GetButtonSprintResults(eSprintType sprintType) {
#ifndef NOTSA_NATIVE
    // Forces the compiler to preserve the value of `edx`.
    // Otherwise it's value is lost when called from 0x60B44C.
    // which causes a crash (as it is used to store a pointer to an anim blend assoc)
    __asm { and edx, edx };
#endif

    if (GetPlayerData()->m_fMoveSpeed <= PLAYER_SPRINT_THRESHOLD) {
        return GetPlayerData()->m_fMoveSpeed <= 0.0f ? 0.0f : 1.0f;
    } else {
        const float progress = std::max(0.0f, GetPlayerData()->m_fMoveSpeed / PLAYER_SPRINT_THRESHOLD - 1.0f);
        return PLAYER_SPRINT_SET[sprintType].field_1C * progress + 1.0f;
    }
}

// 0x60A8A0
void CPlayerPed::ResetPlayerBreath() {
    GetPlayerData()->m_fBreath = CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG);
    GetPlayerData()->m_bRequireHandleBreath = false;
}

// 0x60A8D0
void CPlayerPed::HandlePlayerBreath(bool bDecreaseAir, float fMultiplier) {
    float& breath = GetPlayerData()->m_fBreath;
    float  decreaseAmount = CTimer::GetTimeStep() * fMultiplier;
    if (!bDecreaseAir || CCheat::IsActive(CHEAT_INFINITE_OXYGEN)) {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG) > breath)
            breath += decreaseAmount * 2.0f;
    } else {
        if (breath > 0.0f && bDrownsInWater)
            breath = std::max(0.0f, breath - decreaseAmount);
        else
            CWeapon::GenerateDamageEvent(this, this, eWeaponType::WEAPON_DROWNING, (int32)(decreaseAmount * 3.0f), PED_PIECE_TORSO, 0);
    }
    GetPlayerData()->m_bRequireHandleBreath = false;
}

// TODO(re): Unnamed global (0x969177) - While set, the adrenaline (slow-motion) effect doesn't expire
static auto& bKeepAdrenalineEffect = StaticRef<bool>(0x969177);

// 0x60A9C0
void CPlayerPed::SetRealMoveAnim() {
    CPlayerPedData* const playerData = GetPlayerData();
    RpClump* const        clump      = GetRpClump();

    const auto GetAssoc = [clump](AnimationId animId) {
        return RpAnimBlendClumpGetAssociation(clump, animId);
    };

    CAnimBlendAssociation* aWalk       = GetAssoc(ANIM_ID_WALK);       // Idle/walk
    CAnimBlendAssociation* aRun        = GetAssoc(ANIM_ID_RUN);        // Run
    CAnimBlendAssociation* aSprint     = GetAssoc(ANIM_ID_SPRINT);     // Sprint
    CAnimBlendAssociation* aWalkStart  = GetAssoc(ANIM_ID_WALK_START); // Walk start
    CAnimBlendAssociation* aIdle       = GetAssoc(ANIM_ID_IDLE);       // Idle
    CAnimBlendAssociation* aRunStop    = GetAssoc(ANIM_ID_RUN_STOP);   // Run stop
    CAnimBlendAssociation* aRunStopAlt = GetAssoc(ANIM_ID_RUN_STOPR);  // Run stop (alt.)
    CAnimBlendAssociation* aTurnLeft   = GetAssoc(ANIM_ID_TURN_L);     // Turn left
    CAnimBlendAssociation* aTurnRight  = GetAssoc(ANIM_ID_TURN_R);     // Turn right
    CAnimBlendAssociation* aIdleTired  = GetAssoc(ANIM_ID_IDLE_TIRED); // Tired idle

    // Everything up to the multi-purpose tail (`tail:`) - Uses a lambda so `goto tail` becomes `return`
    [&] {
        if (bResetWalkAnims) {
            if (aWalk)   aWalk->SetCurrentTime(0.0f);
            if (aRun)    aRun->SetCurrentTime(0.0f);
            if (aSprint) aSprint->SetCurrentTime(0.0f);
            bResetWalkAnims = false;
        }

        // Run-stop animations take precedence while playing
        if ((aRunStop && aRunStop->IsPlaying()) || (aRunStopAlt && aRunStopAlt->IsPlaying())) {
            m_nMoveState = PEDMOVE_RUN;
            if (aRunStop && !aRunStop->IsPlaying() && aRunStop->m_CurrentTime < aRunStop->GetHier()->m_fTotalTime) {
                aRunStop->m_Flags |= ANIMATION_IS_PLAYING;
            }
            return;
        }

        // Stop the run-stop animation(s) and go back to idle
        if ((aRunStop && aRunStop->m_BlendDelta >= 0.0f) || (aRunStopAlt && aRunStopAlt->m_BlendDelta >= 0.0f)) {
            if (CAnimBlendAssociation* const toStop = aRunStop ? aRunStop : aRunStopAlt) {
                toStop->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                toStop->m_BlendAmount = 1.0f;
                toStop->m_BlendDelta  = -8.0f;
            }
            RestoreHeadingRate();
            if (!aIdle) {
                aIdle = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_IDLE, 8.0f);
            }
            aIdle->m_BlendAmount = 0.0f;
            aIdle->m_BlendDelta  = 8.0f;
            return;
        }

        // Aiming / running
        if (playerData->m_fMoveBlendRatio != 0.0f || aSprint) {
            if (aIdle) {
                if (!aWalkStart) {
                    aWalkStart = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_WALK_START);
                } else {
                    aWalkStart->m_BlendAmount = 1.0f;
                    aWalkStart->m_BlendDelta  = 0.0f;
                }
                if (aWalk) aWalk->SetCurrentTime(0.0f);
                if (aRun)  aRun->SetCurrentTime(0.0f);
                delete aIdle;
                aIdle = nullptr;
                if (aIdleTired) {
                    aIdleTired->m_BlendDelta = -4.0f;
                }
                if (aSprint) {
                    delete aSprint;
                    aSprint = nullptr;
                }
                m_nMoveState = PEDMOVE_WALK;
            }
            if (aRunStop) {
                delete aRunStop;
                aRunStop = nullptr;
                RestoreHeadingRate();
            }
            if (aRunStopAlt) {
                delete aRunStopAlt;
                aRunStopAlt = nullptr;
                RestoreHeadingRate();
            }
            if (aTurnLeft) {
                delete aTurnLeft;
                aTurnLeft = nullptr;
            }
            if (aTurnRight) {
                delete aTurnRight;
                aTurnRight = nullptr;
            }
            if (!aWalk) {
                aWalk = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_WALK);
                aWalk->m_BlendAmount = 0.0f;
            }
            if (!aRun) {
                aRun = CAnimManager::AddAnimation(clump, m_nAnimGroup, ANIM_ID_RUN);
                aRun->m_BlendAmount = 0.0f;
            }
            if (aWalkStart && (!aWalkStart->IsPlaying() || aWalkStart->GetHier()->m_fTotalTime > aWalkStart->m_CurrentTime + aWalkStart->m_TimeStep)) {
                delete aWalkStart;
                aWalkStart = nullptr;
                aWalk->m_Flags |= ANIMATION_IS_PLAYING;
                aRun->m_Flags  |= ANIMATION_IS_PLAYING;
            }
            if (m_nMoveState == PEDMOVE_SPRINT && aWalkStart) {
                m_nMoveState = PEDMOVE_STILL;
            }

            if (!aSprint || (m_nMoveState == PEDMOVE_SPRINT && playerData->m_fMoveBlendRatio >= 0.4f)) {
                if (aWalkStart) {
                    aWalk->m_Flags &= ~ANIMATION_IS_PLAYING;
                    aRun->m_Flags  &= ~ANIMATION_IS_PLAYING;
                    aWalk->m_BlendAmount = 0.0f;
                    aRun->m_BlendAmount  = 0.0f;
                    return;
                }
                if (m_nMoveState == PEDMOVE_SPRINT) {
                    if (!aSprint) {
                        if (aRun->m_BlendAmount < 1.0f) {
                            if (aWalk->m_BlendAmount == 0.0f && aRun->m_BlendAmount == 0.0f) {
                                aWalk->m_BlendAmount = 1.0f;
                            }
                            if (aRun->m_BlendDelta > 0.0f) {
                                aRun = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_RUN, 4.0f);
                            }
                            playerData->m_fMoveBlendRatio = aRun->m_BlendDelta + 1.0f;
                            return;
                        }
                        aSprint = CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_SPRINT, 2.0f);
                    } else if (aSprint->m_BlendDelta < 0.0f) {
                        aSprint->m_BlendDelta = 2.0f;
                        aRun->m_BlendDelta    = -2.0f;
                    }
                    if (aSprint) {
                        CStats::UpdateStatsWhenSprinting();
                    }
                    return;
                }
                if (playerData->m_fMoveBlendRatio >= 1.0f) {
                    if (playerData->m_fMoveBlendRatio >= 2.0f) {
                        aWalk->m_BlendAmount = 0.0f;
                        aRun->m_BlendAmount  = 1.0f;
                        aWalk->m_BlendDelta  = 0.0f;
                        aRun->m_BlendDelta   = 0.0f;
                        m_nMoveState = PEDMOVE_RUN;
                        CStats::UpdateStatsWhenRunning();
                    } else {
                        aWalk->m_BlendAmount = 2.0f - playerData->m_fMoveBlendRatio;
                        aRun->m_BlendAmount  = playerData->m_fMoveBlendRatio - 1.0f;
                        aWalk->m_BlendDelta  = 0.0f;
                        aRun->m_BlendDelta   = 0.0f;
                        m_nMoveState = PEDMOVE_RUN;
                    }
                    return;
                }
                aWalk->m_BlendAmount = 1.0f;
                aRun->m_BlendAmount  = 0.0f;
                aWalk->m_BlendDelta  = 0.0f;
                aRun->m_BlendDelta   = 0.0f;
            } else {
                if (aSprint->m_BlendAmount == 0.0f) {
                    aSprint->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                    aSprint->m_BlendDelta = -1000.0f;
                } else if (aSprint->m_BlendDelta >= 0.0f || aSprint->m_BlendAmount >= 0.8f) {
                    if (playerData->m_fMoveBlendRatio >= 0.4f) {
                        if (aSprint->m_BlendDelta >= 0.0f) {
                            aSprint->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                            aSprint->m_BlendDelta = -1.0f;
                            aRun->m_BlendDelta    = 1.0f;
                        }
                    } else {
                        CAnimBlendAssociation* runStop;
                        if (aSprint->m_CurrentTime / aSprint->GetHier()->m_fTotalTime >= 0.5f) {
                            runStop = CAnimManager::AddAnimation(clump, ANIM_GROUP_DEFAULT, ANIM_ID_RUN_STOPR);
                        } else {
                            runStop = CAnimManager::AddAnimation(clump, ANIM_GROUP_DEFAULT, ANIM_ID_RUN_STOP);
                        }
                        runStop->m_BlendAmount = 1.0f;
                        runStop->SetDeleteCallback(CPed::RestoreHeadingRateCB, this);
                        m_fHeadingChangeRate  = 0.0f;
                        aSprint->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
                        aSprint->m_BlendDelta = -1000.0f;
                        aWalk->m_Flags &= ~ANIMATION_IS_PLAYING;
                        aRun->m_Flags  &= ~ANIMATION_IS_PLAYING;
                        aWalk->m_BlendAmount = 0.0f;
                        aRun->m_BlendAmount  = 0.0f;
                        aWalk->m_BlendDelta  = 0.0f;
                        aRun->m_BlendDelta   = 0.0f;
                    }
                } else if (playerData->m_fMoveBlendRatio < 1.0f) {
                    aSprint->m_BlendDelta = -8.0f;
                    aRun->m_BlendDelta    = 8.0f;
                }
                if (playerData->m_fMoveBlendRatio > 1.0f) {
                    m_nMoveState = PEDMOVE_RUN;
                    return;
                }
            }
            m_nMoveState = PEDMOVE_WALK;
            return;
        }

        // Not moving
        CPad* const pad = GetPadFromPlayer();
        bool blendIdle  = true;
        if (pad && pad->GetForceCameraBehindPlayer()) {
            int16 aimLR = pad->AimWeaponLeftRight(this);
            if (aimLR != 0 && TheCamera.GetActiveCam().m_nMode == MODE_FOLLOWPED) {
                blendIdle = false;
                if (aimLR >= 0) {
                    if (!aTurnRight || aTurnRight->m_BlendDelta < 0.0f || (aTurnRight->m_BlendAmount < 1.0f && aTurnRight->m_BlendDelta > 0.0f)) {
                        aTurnRight = CAnimManager::BlendAnimation(clump, ANIM_GROUP_DEFAULT, ANIM_ID_TURN_R, 16.0f);
                    }
                    aimLR = pad->AimWeaponLeftRight(this);
                } else {
                    if (!aTurnLeft || aTurnLeft->m_BlendDelta < 0.0f || (aTurnLeft->m_BlendAmount < 1.0f && aTurnLeft->m_BlendDelta > 0.0f)) {
                        aTurnLeft = CAnimManager::BlendAnimation(clump, ANIM_GROUP_DEFAULT, ANIM_ID_TURN_L, 16.0f);
                    }
                    aTurnRight = aTurnLeft;
                    aimLR = pad->AimWeaponLeftRight(this);
                }
                aTurnRight->m_Speed = std::fabs(static_cast<float>(aimLR)) * 0.0078125f; // / 128
                if (aIdle && aIdle->m_BlendAmount >= 0.01f) {
                    delete aIdle;
                    aIdle = nullptr;
                }
            }
        }
        if (blendIdle) {
            if (!aIdle) {
                CAnimManager::BlendAnimation(clump, m_nAnimGroup, ANIM_ID_IDLE, 4.0f);
            }
        }

        // Tired idle (When the player has ran out of breath/stamina)
        const bool stopIdleTired =
               playerData->m_fTimeCanRun >= 0.0f
            || GetIntelligence()->GetTaskFighting()
            || GetIntelligence()->GetTaskUseGun()
            || GetIntelligence()->GetTaskDuck(true)
            || GetIntelligence()->GetTaskThrow()
            || GetIntelligence()->GetTaskJetPack()
            || CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f, nullptr, true, false, false, false, false, false);

        if (stopIdleTired) {
            if (aIdleTired && aIdleTired->m_BlendAmount > 0.0f && aIdleTired->m_BlendDelta >= 0.0f) {
                aIdleTired->m_Flags &= ~ANIMATION_IS_PLAYING;
                aIdleTired->m_BlendDelta = -2.0f;
            }
        } else if (!aIdleTired) {
            aIdleTired = CAnimManager::BlendAnimation(
                clump,
                CClothes::GetDefaultPlayerMotionGroup() == ANIM_GROUP_FAT ? ANIM_GROUP_FAT_TIRED : ANIM_GROUP_DEFAULT,
                ANIM_ID_IDLE_TIRED,
                4.0f
            );
            aIdleTired->m_Flags |= ANIMATION_IS_PLAYING;
            m_nMoveState = PEDMOVE_STILL;
            return;
        }
        m_nMoveState = PEDMOVE_STILL;
    }();

    // Slow-motion (adrenaline)
    if (playerData->m_bAdrenaline) {
        float animSpeed;
        if (playerData->m_nAdrenalineEndTime < CTimer::GetTimeInMS() && !bKeepAdrenalineEffect) {
            playerData->m_bAdrenaline = false;
            CTimer::SetTimeScale(1.0f);
            animSpeed = 1.0f;
        } else {
            CTimer::SetTimeScale(0.33333334f);
            animSpeed = 2.0f;
        }
        if (aWalkStart) aWalkStart->m_Speed = animSpeed;
        if (aWalk)      aWalk->m_Speed = animSpeed;
        if (aRun)       aRun->m_Speed = animSpeed;
        if (!aSprint) {
            return;
        }
        aSprint->m_Speed = animSpeed;
    }

    if (aSprint) {
        if (TheCamera.GetActiveCam().m_nMode == MODE_FIXED) {
            aSprint->m_Speed = 0.7f;
            return;
        }
        aSprint->m_Speed = std::max(1.0f, GetButtonSprintResults(SPRINT_GROUND));
    }
}

// 0x60B460
void CPlayerPed::MakeChangesForNewWeapon(eWeaponType weaponType) {
    GetActiveWeapon().StopWeaponEffect();
    if (m_nPedState == PEDSTATE_SNIPER_MODE)
        TheCamera.ClearPlayerWeaponMode();

    SetCurrentWeapon(weaponType);

    GetPlayerData()->m_nChosenWeapon = m_nActiveWeaponSlot;
    GetPlayerData()->m_fAttackButtonCounter= 0.0f;

    CWeapon& wep = GetActiveWeapon();
    CWeaponInfo& wepInfo = wep.GetWeaponInfo(this);

    wep.m_AmmoInClip = std::min<uint32>(wep.m_TotalAmmo, (uint32)wepInfo.m_nAmmoClip);

    if (!wepInfo.flags.bCanAim)
        ClearWeaponTarget();

    if (!wepInfo.flags.bOnlyFreeAim)
        GetPlayerData()->m_bFreeAiming = false;


    if (auto anim = RpAnimBlendClumpGetAssociation(GetRpClump(), ANIM_ID_FIRE))
        anim->m_Flags |= ANIMATION_IS_PLAYING & ANIMATION_IS_FINISH_AUTO_REMOVE;

    TheCamera.ClearPlayerWeaponMode();
}

// 0x60B550
bool LOSBlockedBetweenPeds(CEntity* entity1, CEntity* entity2) {
    CVector origin{};
    if (entity1->GetIsTypePed()) {
        origin = entity1->AsPed()->GetBonePosition(eBoneTag::BONE_NECK, false);
        if (entity1->AsPed()->bIsDucking)
            origin.z += 0.35f;
    } else {
        origin = entity1->GetPosition();
    }

    const auto target = entity2->GetIsTypePed()
        ? entity1->AsPed()->GetBonePosition(eBoneTag::BONE_NECK, false)
        : entity1->GetPosition();

    CColPoint colPoint;
    CEntity* hitEntity;
    if (CWorld::ProcessLineOfSight(origin, target, colPoint, hitEntity, true, false, false, true, false, false, false, true))
        return hitEntity != entity2;
    return false;
}

// 0x60B650
void CPlayerPed::Compute3rdPersonMouseTarget(bool checkCollision) {
    if (!CCamera::m_bUseMouse3rdPerson) {
        // Nothing to do except expiring the target (See the end of the function)
    } else {
        const CVector pedPos = GetPosition();
        const float   range  = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill())->m_fTargetRange;

        CVector rayStart, rayEnd;
        if (!checkCollision) {
            // Ray from the camera's source along it's forward vector
            rayStart = TheCamera.GetActiveCam().m_vecSource;
            const CVector& camFront = TheCamera.GetActiveCam().m_vecFront;
            if (const float behindDist = DotProduct(camFront, rayStart - pedPos); behindDist < 0.0f) {
                rayStart -= camFront * behindDist; // Push the source forward until it's in front of the ped
            }
            rayEnd = rayStart + camFront * range;
        } else {
            TheCamera.Find3rdPersonCamTargetVector(range, pedPos, rayStart, rayEnd);
        }

        CColPoint colPoint;
        CEntity*  hitEntity = nullptr;

        CWorld::pIgnoreEntity  = this;
        CWorld::bIncludeBikers = true;
        const bool hasHit = CWorld::ProcessLineOfSight(rayStart, rayEnd, colPoint, hitEntity, false, false, true, false, false, false, false, false);
        CWorld::ResetLineTestOptions();

        // NOTE: Only peds are checked for, so no need to verify the type of `hitEntity`
        CPed* const target = hasHit && hitEntity != this && IsAlive() ? hitEntity->AsPed() : nullptr;
        if (target) {
            if (m_p3rdPersonMouseTarget != target) {
                CEntity::ClearReference(m_p3rdPersonMouseTarget);
                m_p3rdPersonMouseTarget = target;
                target->RegisterReference(reinterpret_cast<CEntity**>(&m_p3rdPersonMouseTarget));
            }
            field_7A0 = CTimer::GetTimeInMS() + 1000; // The target is valid for a second

            if (CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nWeaponFire == WEAPON_FIRE_MELEE) {
                return;
            }
            if (!CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill())->flags.bCanAim) {
                return;
            }
            if (!target->GetIntelligence()->IsInSeeingRange(pedPos)) {
                return;
            }

            // Don't notify the target if it's already reacting to us aiming at it
            CPedIntelligence* const targetIntel = target->GetIntelligence();
            CTask* task = targetIntel->m_TaskMgr.m_aPrimaryTasks[0];
            if (!task) task = targetIntel->m_TaskMgr.m_aPrimaryTasks[1];
            if (!task) task = targetIntel->m_TaskMgr.m_aPrimaryTasks[2];
            if (task && task->GetTaskType() == TASK_COMPLEX_REACT_TO_GUN_AIMED_AT) {
                return;
            }

            if (GetActiveWeapon().m_Type != WEAPON_PISTOL_SILENCED) {
                Say(CTX_GLOBAL_PULL_GUN);
            }

            if (CPedGroup* const targetGroup = CPedGroups::GetPedsGroup(target)) {
                if (CPedGroups::AreInSameGroup(target, this)) {
                    return;
                }
                CEventGroupEvent groupEvent(target, new CEventGunAimedAt(this));
                targetGroup->GetIntelligence().AddEvent(&groupEvent);
            } else {
                CEventGunAimedAt eventGunAimedAt(this);
                target->GetEventGroup().Add(&eventGunAimedAt, false);
            }
            return;
        }
    }

    // The target expired (NOTE: The reference isn't cleaned up, matching the original code)
    if (m_p3rdPersonMouseTarget && field_7A0 < CTimer::GetTimeInMS()) {
        m_p3rdPersonMouseTarget = nullptr;
    }
}

// 0x60BA80
void CPlayerPed::DrawTriangleForMouseRecruitPed() {
    CPed* const target = m_p3rdPersonMouseTarget;
    if (!target) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, RWRSTATE(rwALPHATESTFUNCTIONALWAYS));

    // The marker is colored after the target's health (red = almost dead, green = healthy)
    const float healthRatio = std::min(target->m_fHealth / target->m_fMaxHealth, 1.0f);
    const CRGBA color = healthRatio > 0.0f
        ? CRGBA(static_cast<uint8>((1.0f - healthRatio) * 255.0f), static_cast<uint8>(healthRatio * 255.0f), 0, 255)
        : CRGBA(0, 0, 0, 255);

    // The marker gets smaller the closer the target is (And is capped at 60m)
    const float size = std::clamp((target->GetPosition() - GetPosition()).Magnitude() - 10.0f, 0.0f, 50.0f) * 0.02f * 0.825f + 0.175f;

    // Billboard basis: The camera's right vector (Or it's heading if it has no matrix)
    const CVector right = TheCamera.m_matrix
        ? TheCamera.m_matrix->GetRight()
        : CVector{ std::cos(TheCamera.m_placement.m_fHeading), std::sin(TheCamera.m_placement.m_fHeading), 0.0f };

    const CVector base = target->GetPosition() + CVector{ 0.0f, 0.0f, 1.0f };
    const CVector apex = base + CVector{ 0.0f, 0.0f, size };

    // NOTE: The order of the corners is flipped for gang members, which flips the triangle's winding
    CVector verts[3];
    if (target->m_nPedType == PED_TYPE_GANG1) {
        verts[0] = apex;
        verts[1] = base - right * size;
        verts[2] = base + right * size;
    } else {
        verts[0] = apex;
        verts[1] = base + right * size;
        verts[2] = base - right * size;
    }

    // Offset each vertex towards the camera so the marker isn't hidden inside the target
    const CVector camPos = TheCamera.GetPosition();
    for (CVector& v : verts) {
        CVector towardsCam = camPos - v;
        towardsCam.Normalise();
        v += towardsCam;
    }

    const RwRGBA rwColor = color.ToRwRGBA();
    for (size_t i = 0; i < std::size(verts); i++) {
        RwIm3DVertex* const vert = &TempBufferVertices.m_3d[i];
        RxObjSpace3DVertexSetPreLitColor(vert, &rwColor);
        RxObjSpace3DVertexSetPos(vert, &verts[i]);
    }
    aTempBufferIndices[0] = 0;
    aTempBufferIndices[1] = 1;
    aTempBufferIndices[2] = 2;

    if (RwIm3DTransform(TempBufferVertices.m_3d, 3, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, 3);
        RwIm3DEnd();
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,      RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,       RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,        RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,           RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,          RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,  RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,  RWRSTATE(rwALPHATESTFUNCTIONGREATER));
}

// 0x60C0C0
bool CPlayerPed::DoesTargetHaveToBeBroken(CEntity* target, CWeapon* weapon) {
    if (!target->GetIsVisible()) {
        return true;
    }

    if (weapon->GetWeaponRange(this, target) < (target->GetPosition() - GetPosition()).Magnitude()) {
        return true;
    }

    if (weapon->m_Type == eWeaponType::WEAPON_SPRAYCAN) {
        if (target->GetIsTypeBuilding()) {
            if (CTagManager::IsTag(*target)) {
                if (CTagManager::GetAlpha(*target) == 255) { // they probably used -1
                    return true;
                }
            }
        }
    }

    return !CanIKReachThisTarget(target->GetPosition(), weapon, false);
}

// 0x60C1E0
void CPlayerPed::KeepAreaAroundPlayerClear() {
    // Make the (non-group) peds around the player flee
    CPedGroupMembership& groupMembership = GetPlayerGroup().GetMembership();
    for (CPed& ped : GetIntelligence()->m_pedScanner.GetEntities<CPed>()) {
        if (!ped.IsCreatedBy(PED_GAME)) {
            continue;
        }
        if (ped.bInVehicle || !ped.IsAlive() || groupMembership.IsMember(&ped)) {
            continue;
        }

        if (!ped.GetIsOnScreen() || ped.bKeepTasksAfterCleanUp) {
            ped.FlagToDestroyWhenNextProcessed();
            continue;
        }

        // Don't add the task again if the ped is already fleeing from us
        if (CTask* const task = ped.GetIntelligence()->FindTaskByType(TASK_COMPLEX_SMART_FLEE_ENTITY)) {
            if (static_cast<CTaskComplexSmartFleeEntity*>(task)->m_fleeFrom == this) {
                continue;
            }
        }
        if (CEvent* const event = ped.GetEventGroup().GetEventOfType(EVENT_SCRIPT_COMMAND)) {
            if (CTask* const task = static_cast<CEventScriptCommand*>(event)->m_task; task && task->GetTaskType() == TASK_COMPLEX_SMART_FLEE_ENTITY) {
                continue;
            }
        }

        auto* const fleeTask = new CTaskComplexSmartFleeEntity(this, false, 1000.0f, 100'000, 1000, 0.0f);
        fleeTask->m_moveState = PEDMOVE_WALK;
        CEventScriptCommand event(3, fleeTask);
        ped.GetEventGroup().Add(&event, false);
    }

    // Tell the vehicles around the player to get away from him
    const CVector refPos = bInVehicle && m_pVehicle ? m_pVehicle->GetPosition() : GetPosition();

    int16    numVehicles = 0;
    CEntity* vehicles[6];
    CWorld::FindObjectsInRange(GetPosition(), 20.0f, true, &numVehicles, (int16)std::size(vehicles), vehicles, false, true, false, false, false);
    for (int16 i = 0; i < numVehicles; i++) {
        CVehicle* const veh = vehicles[i]->AsVehicle();
        if (veh->m_nCreatedBy == MISSION_VEHICLE) {
            continue;
        }
        if (veh->GetStatus() == STATUS_PLAYER || veh->GetStatus() == STATUS_FORCED_STOP) {
            continue;
        }

        const CVector vehPos = veh->GetPosition();
        if ((vehPos - refPos).SquaredMagnitude() <= 25.0f) { // Within 5m
            const CVector vehForward = veh->m_matrix ? veh->m_matrix->GetForward() : CVector{};
            if (DotProduct2D(refPos - vehPos, vehForward) <= 0.0f) {
                veh->m_autoPilot.SetTempAction(TEMPACT_GOFORWARD, 2000);
            } else {
                veh->m_autoPilot.SetTempAction(TEMPACT_REVERSE, 2000);
            }
        } else {
            veh->m_autoPilot.SetTempAction(TEMPACT_WAIT, 5000);
        }
        CCarCtrl::PossiblyRemoveVehicle(veh);
    }
}

// 0x60C520
void CPlayerPed::SetPlayerMoveBlendRatio(CVector* point) {
    float& moveBlendRatio = GetPlayerData()->m_fMoveBlendRatio;
    if (point) {
        moveBlendRatio = std::min(2.0f, (*point - GetPosition()).Magnitude2D() * 2.0f);
    } else {
        switch (m_nMoveState)
        {
        case PEDMOVE_WALK:
            moveBlendRatio = 1.0f;
            break;

        case PEDMOVE_RUN:
            moveBlendRatio = 1.8f;
            break;

        case PEDMOVE_SPRINT:
            moveBlendRatio = 2.5f;
            break;

        default:
            moveBlendRatio = 0.0f;
            break;
        }
    }
    SetRealMoveAnim();
}

// 0x60C5F0
CPed* CPlayerPed::FindPedToAttack() {
    CVector origin = FindPlayerCoors();
    origin.z = 0.0f;

    CVector end = origin + TheCamera.GetForward() * 100.0f;
    end.z = 0.0f;

    CPed* closestPed{};
    float closestDistance = std::numeric_limits<float>::max();

    CPedGroupMembership& membership = GetPlayerGroup().GetMembership();
    for (int i = 0; GetPedPool()->GetSize(); i++) {
        CPed* ped = GetPedPool()->GetAt(i);
        if (!ped)
            continue;
        if (ped->IsPlayer())
            continue;
        if (!ped->IsAlive())
            continue;
        if (membership.IsMember(ped))
            continue;
        if (ped->m_nPedType == PED_TYPE_GANG2)
            continue;

        CVector point = ped->GetPosition();
        point.z = 0.0f;

        float dist = CCollision::DistToLine(origin, end, point);
        float pointDist = (point - origin).Magnitude2D();
        if (pointDist > 20.0f)
            dist += (pointDist - 20.0f) / 5.0f;

        if (IsPedTypeGang(ped->m_nPedType))
            dist = std::max(0.0f, dist / 2.0f - 2.0f);

        if (dist < closestDistance) {
            closestDistance = dist;
            closestPed = ped;
        }
    }
    return closestPed;
}

// 0x60C7C0
void CPlayerPed::ForceGroupToAlwaysFollow(bool enable) {
    GetPlayerData()->m_bGroupAlwaysFollow = enable;
    if (enable)
        TellGroupToStartFollowingPlayer(true, false, true);
}

// 0x60C800
void CPlayerPed::ForceGroupToNeverFollow(bool enable) {
    GetPlayerData()->m_bGroupNeverFollow = enable;
    if (enable)
        TellGroupToStartFollowingPlayer(false, false, true);
}

// TODO(re): Unnamed global (0xB6F858) - Compared against various weapon ids, it blocks weapon cycling
static auto& nPadWeaponId = StaticRef<int32>(0xB6F858);

// TODO(re): Unnamed flags (0x96917C, 0x96917D, 0x96917E) - Read by 0x609380 only.
//           They are set (By cheats/missions) to allow recruiting peds that aren't part of the player's gang.
static auto& bRecruitAnyoneFlag1 = StaticRef<bool>(0x96917C);
static auto& bRecruitAnyoneFlag2 = StaticRef<bool>(0x96917D);
static auto& bRecruitAnyoneFlag3 = StaticRef<bool>(0x96917E);

// 0x609380
static bool CanPlayerRecruitAnyone() {
    return bRecruitAnyoneFlag1 || bRecruitAnyoneFlag2 || bRecruitAnyoneFlag3;
}

// 0x60C840
void CPlayerPed::MakeThisPedJoinOurGroup(CPed* ped) {
    if (ped->bDruggedUp) {
        ped->Say(CTX_GLOBAL_DRUGGED_IGNORE);
        return;
    }

    CPlayerPedData* const playerData = GetPlayerData();
    CPedGroupMembership&  membership = GetPlayerGroup().GetMembership();

    if (ped->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_KILL_PED_ON_FOOT)) {
        return;
    }
    if (ped->m_nPedType != PED_TYPE_GANG2 && !CanPlayerRecruitAnyone()) {
        return;
    }
    if (membership.IsMember(ped)) {
        return;
    }

    CAEPedSpeechAudioEntity::SetCJMood(MOOD_UNK, 10'000, 1, -1, -1);
    ped->Say(CTX_GLOBAL_JOIN_ME_ASK, 0, 1.0f, true);

    // The group's size is limited by the player's stats, and a script-set limit
    int32 maxMembers = std::min<int32>(CStats::FindMaxNumberOfGroupMembers(), playerData->m_nScriptLimitToGangSize);
    if (const float cityUnlocked = CStats::GetStatValue(STAT_CITY_UNLOCKED); cityUnlocked == 1.0f || cityUnlocked == 2.0f) {
        maxMembers = 0;
    }

    const int32 numMembers = membership.CountMembersExcludingLeader();
    if ((!CanPlayerRecruitAnyone() || numMembers > 6) && numMembers >= maxMembers) {
        // The group is full (or recruiting isn't allowed at all)
        ped->Say(CTX_GLOBAL_JOIN_GANG_NO, 2000, 1.0f, true);
        CEventDontJoinPlayerGroup event(this);
        ped->GetEventGroup().Add(&event, false);
        return;
    }

    // Remove the ped from it's current group (if any)
    if (CPedGroup* const pedGroup = CPedGroups::GetPedsGroup(ped)) {
        pedGroup->GetMembership().RemoveMember(ped);
    }

    // Make the ped join us
    {
        CEventScriptCommand event(3, new CTaskComplexBeInGroup(playerData->m_nPlayerGroup));
        ped->GetEventGroup().Add(&event, false);
    }
    membership.AddFollower(ped);
    GetPlayerGroup().Process();
    ped->GiveWeaponWhenJoiningGang();

    // Tell the rest of the group about the new member
    {
        CEventGroupEvent groupEvent(this, new CEventNewGangMember(ped));
        GetPlayerGroup().GetIntelligence().AddEvent(&groupEvent);
    }

    ped->bDrownsInWater = false; // `m_nPedFlags` bit 0x100000
    CStats::IncrementStat(STAT_GANG_MEMBERS_RECRUITED, 1.0f);
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_INCREASE, STAT_GANG_STRENGTH, 1.0f);

    // Give the new member a radar blip so the player can find him
    constexpr eBlipColour BLIP_COLOUR_RECRUITED_GANG_MEMBER = (eBlipColour)0x46C800FF; // RGBA(70, 200, 0, 255)
    const tBlipHandle blip = CRadar::SetEntityBlip(BLIP_CHAR, CPools::GetPedRef(ped), BLIP_COLOUR_RECRUITED_GANG_MEMBER, BLIP_DISPLAY_BLIPONLY);
    CRadar::ChangeBlipScale(blip, 2);
    CRadar::ChangeBlipColour(blip, BLIP_COLOUR_RECRUITED_GANG_MEMBER);
    CRadar::SetBlipFriendly(blip, true);

    ped->bClearRadarBlipOnDeath = true; // `m_nPedFlags` bit 0x2000 of the 3rd word

    // TODO(re): `CPedGroupMembership::m_separationRange` (0xC0994C) is private and has no setter,
    //           so it can't be set to 120.0f here (Which is what the original does)

    ped->Say(CTX_GLOBAL_JOIN_GANG_YES, 2000, 1.0f, true);
}

// 0x60CC50
bool CPlayerPed::PlayerWantsToAttack() {
    CPedGroup& group = GetPlayerGroup();
    if (group.GetMembership().CountMembersExcludingLeader() < 1) {
        return false;
    }
    if (!group.m_bMembersEnterLeadersVehicle) {
        return false;
    }

    group.GetIntelligence().ReportAllBarScriptTasksFinished();

    CEntity* target = m_pTargetedObject;
    if (CCamera::m_bUseMouse3rdPerson && !target) {
        target = m_p3rdPersonMouseTarget;
    }

    if (target) {
        if (target->GetType() != ENTITY_TYPE_PED) {
            if (CTagManager::IsTag(*target)) {
                return false;
            }
            if (target->GetType() == ENTITY_TYPE_OBJECT && target->AsObject()->CanBeTargetted()) {
                return false;
            }
            target = FindPedToAttack();
        }
    } else {
        target = FindPedToAttack();
    }

    if (!target) {
        return false;
    }

    group.PlayerGaveCommand_Attack(this, target->AsPed());
    return true; // NOTE: The original doesn't return anything meaningful
}

// 0x60CD20
void CPlayerPed::SetInitialState(bool bGroupCreated) {
    CMBlur::ClearDrunkBlur();
    CTimer::SetTimeScale(1.0f);

    m_bUsesCollision        = true;
    physicalFlags.bApplyGravity = true;

    ClearAimFlag();
    ClearLookFlag();
    bRenderPedInCar = true; // `m_nPedFlags` bit 0x2000

    if (m_pFire) {
        m_pFire->Extinguish();
    }

    SetPedState(PEDSTATE_IDLE);
    SetMoveState(PEDMOVE_STILL);

    bIsDucking       = false; // `m_nPedFlags` bit 0x4000000
    bDontRender      = false; // `m_nPedFlags` bit 0x2
    bIsBeingArrested = false; // `m_nPedFlags` bit 0x40
    bCanExitCar      = true;  // `m_nPedFlags` bit 0x4000000 of the 3rd word

    GetIntelligence()->FlushIntelligence();
    RpAnimBlendClumpRemoveAllAssociations(GetRpClump());

    GetTaskManager().SetTask(new CTaskSimplePlayerOnFoot(), TASK_PRIMARY_DEFAULT);

    m_nAnimGroup = ANIM_GROUP_PLAYER;
    bIsPedDieAnimPlaying = false; // `m_nPedFlags` bit 0x100000

    if (CPlayerPedData* const playerData = GetPlayerData()) {
        playerData->m_bAdrenaline = false;
    }

    SetRealMoveAnim();

    m_pStats->m_nTemper = 50;

    if (m_pAttachedTo && !m_bUsesCollision) {
        m_bUsesCollision = true;
    }
    m_pAttachedTo = nullptr;
    m_nTurretAmmo = 0;

    GetTaskManager().SetTaskSecondary(new CTaskComplexFacial(), TASK_SECONDARY_FACIAL_COMPLEX);

    if (!bGroupCreated && !(GetPlayerData()->m_nPlayerFlags & 0x400)) {
        CPedGroup& group = GetPlayerGroup();
        group.m_bMembersEnterLeadersVehicle = true;
        group.GetIntelligence().SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::RANDOM);

        CEventPlayerCommandToGroup attackEvent(ePlayerGroupCommand::PLAYER_GROUP_COMMAND_ATTACK);
        attackEvent.ComputeResponseTaskType(&group);
        if (attackEvent.WillRespond()) {
            auto* const gatherEvent = new CEventPlayerCommandToGroup(ePlayerGroupCommand::PLAYER_GROUP_COMMAND_GATHER);
            gatherEvent->m_TaskId = attackEvent.m_TaskId;

            CEventGroupEvent groupEvent(this, gatherEvent);
            group.GetIntelligence().AddEvent(&groupEvent);
        }
    }

    if (CPlayerPedData* const playerData = GetPlayerData()) {
        playerData->SetInitialState();
    }
}

// 0x60D000
void CPlayerPed::MakeChangesForNewWeapon(uint32 weaponSlot) {
    if (weaponSlot != -1)
        MakeChangesForNewWeapon(GetWeaponInSlot(weaponSlot).m_Type);
}

static auto& PLAYER_MAX_TARGET_VIEW_ANGLE = StaticRef<float>(0x8D243C); // 140.0f

// 0x60D020
void CPlayerPed::EvaluateTarget(CEntity* target, CEntity *& outTarget, float & outTargetPriority, float maxDistance, float compensationRotRad, bool arg5) {
    const CVector dir = target->GetPosition() - GetPosition();
    const float dist = dir.Magnitude();

    if (dist > maxDistance)
        return;

    if (DoesTargetHaveToBeBroken(target, &GetActiveWeapon()))
        return;

    const float targetAngleDeg = std::fabs(RadiansToDegrees(CGeneral::LimitRadianAngle(CGeneral::GetATanOf(dir) - compensationRotRad)));

    float viewAngleMultiplier = 1.0f - targetAngleDeg / PLAYER_MAX_TARGET_VIEW_ANGLE;
    if (dist > 1.0f)
        viewAngleMultiplier /= sqrt(sqrt(dist)); // Take quad root of dist

    const float targetPriority = FindTargetPriority(target) * viewAngleMultiplier;
    if (targetPriority > outTargetPriority && !LOSBlockedBetweenPeds(this, target)) {
        outTarget = target;
        outTargetPriority = targetPriority;
    }
}

// 0x60D1C0
void CPlayerPed::EvaluateNeighbouringTarget(CEntity* target, CEntity** outTarget, float* outTargetPriority, float maxDistance, float arg4, bool arg5) {
    if ((target->GetPosition() - GetPosition()).Magnitude() >= maxDistance) {
        return;
    }
    if (DoesTargetHaveToBeBroken(target, &GetActiveWeapon())) {
        return;
    }

    // Angle of the target relative to the camera (and the given heading)
    const CVector camPos = TheCamera.GetPosition();
    float angle = CGeneral::GetATanOfXY(target->GetPosition().x - camPos.x, target->GetPosition().y - camPos.y) - arg4;
    angle = CGeneral::LimitRadianAngle(angle);

    if (std::fabs(angle) >= 0.87266463f) { // 50 degrees
        return;
    }

    // Only consider targets on the requested side (Note: The score is maximized, so it's the negated angle)
    float score;
    if (!arg5) {
        if (angle >= 0.0f) {
            score = -100'000.0f;
        } else {
            score = -std::fabs(angle);
        }
    } else {
        if (angle >= 0.0f) {
            score = -std::fabs(angle);
        } else {
            score = -100'000.0f;
        }
    }

    if (score <= *outTargetPriority) {
        return;
    }

    *outTarget         = target;
    *outTargetPriority = score;
}

// 0x60D350
void CPlayerPed::ProcessGroupBehaviour(CPad* pad) {
    CEntity* target = m_pTargetedObject;
    if (CCamera::m_bUseMouse3rdPerson && !target) {
        target = m_p3rdPersonMouseTarget;
    }

    CPlayerPedData* const playerData = GetPlayerData();

    // Recruit the ped the player is looking at (Or tell the group to follow if there's none)
    if (!FindPlayerVehicle()) {
        if (pad->GetGroupControlForward()) {
            playerData->m_nPadUpPressedInMilliseconds += CTimer::GetTimeStepInMS();
            if (playerData->m_nPadUpPressedInMilliseconds == 1200 && !(playerData->m_nPlayerFlags & 0x100)) {
                DisbandPlayerGroup();
            }
        } else {
            if (playerData->m_nPadUpPressedInMilliseconds != 0 && playerData->m_nPadUpPressedInMilliseconds < 1200) {
                if (!target || target->GetType() != ENTITY_TYPE_PED) {
                    TellGroupToStartFollowingPlayer(true, true, false);
                } else if (target->AsPed()->m_nPedType == PED_TYPE_GANG2 || CanPlayerRecruitAnyone()) {
                    if (!(playerData->m_nPlayerFlags & 0x100)) {
                        MakeThisPedJoinOurGroup(target->AsPed());
                    }
                } else {
                    TellGroupToStartFollowingPlayer(true, true, false);
                }
            }
            playerData->m_nPadUpPressedInMilliseconds = 0;
        }
    }

    if (playerData->m_nPlayerFlags & 0x100) {
        return;
    }

    // Follow (Or tell a gang member to join the group)
    if (!FindPlayerVehicle()) {
        if (!pad->GetGroupControlBack()) {
            if (playerData->m_nPadDownPressedInMilliseconds != 0 && playerData->m_nPadDownPressedInMilliseconds < 1200) {
                if (!target || target->GetType() != ENTITY_TYPE_PED || target->AsPed()->m_nPedType != PED_TYPE_GANG2) {
                    TellGroupToStartFollowingPlayer(false, true, false);
                } else {
                    MakeThisPedJoinOurGroup(target->AsPed());
                }
            }
            playerData->m_nPadDownPressedInMilliseconds = 0;
        } else {
            playerData->m_nPadDownPressedInMilliseconds += CTimer::GetTimeStepInMS();
            if (playerData->m_nPadDownPressedInMilliseconds > 1199) {
                DisbandPlayerGroup();
            }
        }
    }

    // Keep the player's hatred of cops in sync with his wanted level (Every ~32 frames)
    if ((CTimer::GetFrameCounter() & 0x1F) == 6) {
        const CWanted* const wanted      = GetPlayerWanted();
        const bool           isWanted    = wanted && wanted->m_WantedLevel != eWantedLevel::WANTED_CLEAN;
        const uint32         copPedFlag = CPedType::GetPedFlag(PED_TYPE_COP);

        for (CPed& gangMember : GetPedPool()->GetAllValid()) {
            if (gangMember.m_nPedType != PED_TYPE_GANG2) {
                continue;
            }
            if (isWanted) {
                m_acquaintance.SetAsAcquaintance(ACQUAINTANCE_HATE, copPedFlag);
            } else {
                m_acquaintance.ClearAsAcquaintance(ACQUAINTANCE_HATE, copPedFlag);
            }
        }
    }
}

// 0x60D5A0
bool CPlayerPed::PlayerHasJustAttackedSomeone() {
    return PlayerWantsToAttack();
}

// 0x60D790
void CPlayerPed::SetupPlayerPed(int32 playerId) {
    auto ped = new CPlayerPed(playerId, false);
    auto& playerInfo = FindPlayerInfo(playerId);
    playerInfo.m_pPed = ped;

    if (playerId == 1)
        ped->m_nPedType = PED_TYPE_PLAYER2;

    ped->SetOrientation(0.0f, 0.0f, 0.0f);
    CWorld::Add(ped);
    ped->m_nWeaponAccuracy = 100;
    playerInfo.m_nPlayerState = ePlayerState::PLAYERSTATE_PLAYING;
}

// 0x60D850
void CPlayerPed::ProcessWeaponSwitch(CPad* pad) {
    CPlayerPedData* const playerData = GetPlayerData();

    if (CDarkel::FrenzyOnGoing() || m_pAttachedTo || GetIntelligence()->GetTaskJetPack()) {
        return;
    }

    // Cycle through the weapons (Only when not aiming at anything)
    if (!m_pTargetedObject
        && !(playerData->m_nPlayerFlags & 0x8)
        && !playerData->m_bDontAllowWeaponChange
        && !(playerData->m_nPlayerFlags & 0x800)
    ) {
        const auto IsUsableSlot = [this](int32 slot) {
            CWeapon& weapon = m_aWeapons[slot];
            return weapon.m_Type != WEAPON_UNARMED
                && weapon.HasWeaponAmmoToBeUsed()
                && (!CGameLogic::IsCoopGameGoingOn() || weapon.CanBeUsedFor2Player());
        };
        const auto IsCyclingBlocked = [] {
            switch (nPadWeaponId) {
            case WEAPON_SNIPERRIFLE:
            case WEAPON_EXTINGUISHER:
            case WEAPON_POOL_CUE:
            case WEAPON_REMOTE_SATCHEL_CHARGE:
            case WEAPON_KATANA:
            case WEAPON_DETONATOR:
            case WEAPON_EXPLOSION:
            case WEAPON_UZI_DRIVEBY:
            case WEAPON_PARACHUTE:
                return true;
            }
            return false;
        };

        if (pad->CycleWeaponRightJustDown()) {
            if (!IsCyclingBlocked()) {
                int32 slot = m_nActiveWeaponSlot + 1;
                while (slot < (int32)NUM_WEAPON_SLOTS && !IsUsableSlot(slot)) {
                    slot++;
                }
                playerData->m_nChosenWeapon = slot >= (int32)NUM_WEAPON_SLOTS ? 0 : slot;
            }
        } else if (pad->CycleWeaponLeftJustDown()) {
            switch (nPadWeaponId) {
            case WEAPON_SNIPERRIFLE:
            case WEAPON_POOL_CUE:
            case WEAPON_KATANA:
            case WEAPON_EXPLOSION:
            case WEAPON_PARACHUTE:
                break;
            default: {
                int32 slot = m_nActiveWeaponSlot - 1;
                while (true) {
                    if (slot < 0) {
                        slot = (int32)NUM_WEAPON_SLOTS - 1;
                    }
                    if (slot == 0 || IsUsableSlot(slot)) {
                        break;
                    }
                    slot--;
                }
                playerData->m_nChosenWeapon = slot;
                break;
            }
            }
        }
    }

    // Select another weapon if the current one ran out of ammo
    const CWeaponInfo* const weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD);
    if (weaponInfo->m_nWeaponFire != WEAPON_FIRE_MELEE && !(pad->GetWeapon(this) && GetActiveWeapon().m_Type == WEAPON_MINIGUN)) {
        if (GetActiveWeapon().m_TotalAmmo < 1) {
            switch (nPadWeaponId) {
            case WEAPON_SNIPERRIFLE:
            case WEAPON_POOL_CUE:
            case WEAPON_KATANA:
            case WEAPON_EXPLOSION:
                return;
            }

            if (GetActiveWeapon().m_Type == WEAPON_DETONATOR && m_aWeapons[8].m_Type == WEAPON_REMOTE_SATCHEL_CHARGE) {
                playerData->m_nChosenWeapon = 8;
            } else {
                playerData->m_nChosenWeapon = m_nActiveWeaponSlot - 1;
            }

            int32 slot = playerData->m_nChosenWeapon;
            while (slot >= 0) {
                if ((slot == 5 && m_aWeapons[4].m_Type == WEAPON_BASEBALLBAT)
                    || (m_aWeapons[slot].m_TotalAmmo > 0
                        && m_aWeapons[slot].m_Type != WEAPON_MOLOTOV
                        && m_aWeapons[slot].m_Type != WEAPON_TEARGAS
                        && m_aWeapons[slot].m_Type != WEAPON_GRENADE)
                ) {
                    break;
                }
                slot--;
            }
            playerData->m_nChosenWeapon = std::max(slot, 0);
        }
    }

    if (playerData->m_nChosenWeapon == m_nActiveWeaponSlot) {
        return;
    }

    if (CTaskSimpleUseGun* const useGun = GetIntelligence()->GetTaskUseGun()) {
        switch (useGun->m_NextCmd) {
        case eGunCommand::FIRE:
        case eGunCommand::FIREBURST:
            return;
        case eGunCommand::RELOAD:
            if (useGun->m_Anim) {
                return;
            }
            break;
        }
    }

    RemoveWeaponAnims(m_nActiveWeaponSlot, -1000.0f);
    MakeChangesForNewWeapon(m_aWeapons[playerData->m_nChosenWeapon].m_Type);
}

// 0x60DC50
bool CPlayerPed::FindWeaponLockOnTarget() {
    const CWeaponInfo* const weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());
    const float              range      = weaponInfo->m_fTargetRange;

    // Already have a target? Just make sure it's still in (2D) range
    if (m_pTargetedObject) {
        const float maxDist = range * CWeapon::TargetWeaponRangeMultiplier(m_pTargetedObject, this);
        if ((m_pTargetedObject->GetPosition() - GetPosition()).Magnitude2D() > maxDist) {
            CEntity::ClearReference(m_pTargetedObject);
            return false;
        }
        return true;
    }

    constexpr float HALF_DEGREE_IN_RAD = 0.008726646f; // PI / 360

    CEntity* best      = nullptr;
    float    bestScore = -10'000.0f;

    // Which direction to search in (The player's forward, or the pad's stick if it's pushed far enough)
    CVector forward     = GetForwardVector();
    forward.Normalise();
    const float playerHeading = CGeneral::GetATanOfXY(forward.x, forward.y);
    float       scanHeading   = playerHeading;
    if (CPad* const pad = GetPadFromPlayer()) {
        const float leftRight = static_cast<float>(pad->GetPedWalkLeftRight());
        const float upDown    = static_cast<float>(pad->GetPedWalkUpDown());
        if (std::fabs(leftRight) > 60.0f || std::fabs(upDown) > 60.0f) {
            scanHeading = CGeneral::LimitRadianAngle(
                CGeneral::GetRadianAngleBetweenPoints(-leftRight, upDown, 0.0f, 0.0f)
                - TheCamera.GetHeading()
                + 1.5707964f
            );
        }
    }

    // The spray can locks onto the closest tag in front of the player
    if (GetActiveWeapon().m_Type == WEAPON_SPRAYCAN) {
        const float heading = m_matrix ? playerHeading : m_placement.m_fHeading;

        int16    numObjects = 0;
        CEntity* objects[0xF];
        CWorld::FindObjectsInRange(GetPosition() + forward * 8.0f, 8.0f, false, &numObjects, (int16)std::size(objects), objects, true, false, false, false, false);

        CEntity* closestTag   = nullptr;
        float    closestAngle = 3.1415927f;
        for (int16 i = 0; i < numObjects; i++) {
            CEntity* const object = objects[i];
            if (!CTagManager::IsTag(*object) || CTagManager::GetAlpha(*object) == 255) {
                continue;
            }
            const CVector delta = object->GetPosition() - GetPosition();
            if (delta.SquaredMagnitude() >= range * range) {
                continue;
            }
            const float angle = CGeneral::LimitRadianAngle(CGeneral::GetATanOfXY(-delta.x, delta.y) - heading);
            if (!closestTag || angle < closestAngle) {
                closestAngle = angle;
                closestTag   = object;
            }
        }

        if (closestTag && !LOSBlockedBetweenPeds(this, closestTag)) {
            CEntity::ChangeEntityReference(m_pTargetedObject, closestTag);
            GetPlayerData()->m_bDontAllowWeaponChange = true;
            return true;
        }
    }

    // Peds
    const CVector playerPos = GetPosition();
    for (CPed& ped : GetPedPool()->GetAllValid()) {
        if (&ped == this) {
            continue;
        }
        if (ped.m_nPedState == PEDSTATE_DIE || ped.m_nPedState == PEDSTATE_DEAD) {
            continue;
        }
        if (ped.bInVehicle
            && (!ped.m_pVehicle
                || (ped.m_pVehicle->m_nVehicleType != VEHICLE_TYPE_BIKE && !ped.m_pVehicle->vehicleFlags.bVehicleCanBeTargetted))
        ) {
            continue;
        }
        if (ped.bNeverEverTargetThisPed) {
            continue;
        }
        if (IsPedTypePlayer(ped.m_nPedType) && CGameLogic::bPlayersCannotTargetEachOther) {
            continue;
        }
        if (CPedGroups::AreInSameGroup(&ped, this)) {
            continue;
        }

        const CVector pedPos = ped.GetPosition();
        if (std::fabs(CGeneral::LimitRadianAngle(CGeneral::GetATanOfXY(pedPos.x - playerPos.x, pedPos.y - playerPos.y) - scanHeading)) >= PLAYER_MAX_TARGET_VIEW_ANGLE * HALF_DEGREE_IN_RAD) {
            continue;
        }

        // The target also has to be in front of the player (Checked from a point slightly in front of him)
        const CVector eye = playerPos + forward * 3.0f;
        if (std::fabs(CGeneral::LimitRadianAngle(CGeneral::GetATanOfXY(pedPos.x - eye.x, pedPos.y - eye.y) - scanHeading)) >= 90.0f * HALF_DEGREE_IN_RAD) {
            continue;
        }

        const float maxDist = range * CWeapon::TargetWeaponRangeMultiplier(&ped, this);
        if ((pedPos - playerPos).Magnitude() >= maxDist) {
            continue;
        }

        EvaluateTarget(&ped, best, bestScore, maxDist, scanHeading, false);
    }

    // Objects
    for (CObject& object : GetObjectPool()->GetAllValid()) {
        if (!object.CanBeTargetted() || object.objectFlags.bIsExploded || !object.GetRwObject()) {
            continue;
        }
        if (!CanIKReachThisTarget(object.GetPosition(), &GetActiveWeapon(), true)) {
            continue;
        }
        EvaluateTarget(&object, best, bestScore, range, scanHeading, true);
    }

    // Vehicles (Only in co-op)
    if (CGameLogic::IsCoopGameGoingOn()) {
        for (CVehicle& vehicle : GetVehiclePool()->GetAllValid()) {
            if (vehicle.m_nPhysicalFlags & PHYSICAL_DESTROYED) {
                continue;
            }
            if (vehicle.m_nVehicleSubType == VEHICLE_TYPE_BMX) {
                continue;
            }
            if (!CanIKReachThisTarget(vehicle.GetPosition(), &GetActiveWeapon(), true)) {
                continue;
            }
            EvaluateTarget(&vehicle, best, bestScore, range, scanHeading, true);
        }
    }

    if (!best) {
        return false;
    }

    CEntity::ChangeEntityReference(m_pTargetedObject, best);
    GetPlayerData()->m_bDontAllowWeaponChange = true;
    return true;
}

// 0x60E530
bool CPlayerPed::FindNextWeaponLockOnTarget(CEntity* currentTarget, bool rightSide) {
    const CWeaponInfo* const weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());
    const float              range      = weaponInfo->m_fTargetRange;

    CEntity* best      = nullptr;
    float    bestScore = -10'000.0f;

    // Search around the camera-to-currentTarget axis (Or the camera's forward vector if there's no target)
    const CVector camPos  = TheCamera.GetPosition();
    const CVector scanDir = currentTarget
        ? currentTarget->GetPosition() - camPos
        : TheCamera.GetActiveCam().m_vecFront; // NOTE: 0xB6F9AC in the original
    const float scanHeading = CGeneral::GetATanOfXY(scanDir.x, scanDir.y);

    const auto EvaluateCandidate = [&](CEntity* candidate) {
        EvaluateNeighbouringTarget(candidate, &best, &bestScore, range * CWeapon::TargetWeaponRangeMultiplier(candidate, this), scanHeading, rightSide);
    };

    // Peds
    for (CPed& ped : GetPedPool()->GetAllValid()) {
        if (&ped == this || &ped == currentTarget) {
            continue;
        }
        if (ped.m_nPedState == PEDSTATE_DIE || ped.m_nPedState == PEDSTATE_DEAD) {
            continue;
        }
        if (ped.bInVehicle
            && (!ped.m_pVehicle
                || (ped.m_pVehicle->m_nVehicleType != VEHICLE_TYPE_BIKE && !ped.m_pVehicle->vehicleFlags.bVehicleCanBeTargetted))
        ) {
            continue;
        }
        if (ped.bNeverEverTargetThisPed) {
            continue;
        }
        if (CPedGroups::AreInSameGroup(&ped, this)) {
            continue;
        }
        if (IsPedTypePlayer(ped.m_nPedType) && CGameLogic::bPlayersCannotTargetEachOther) {
            continue;
        }
        if (LOSBlockedBetweenPeds(this, &ped)) {
            continue;
        }
        if (!CanIKReachThisTarget(ped.GetPosition(), &GetActiveWeapon(), true)) {
            continue;
        }
        EvaluateCandidate(&ped);
    }

    // Objects
    for (CObject& object : GetObjectPool()->GetAllValid()) {
        if (!object.CanBeTargetted() || object.objectFlags.bIsExploded || !object.GetRwObject()) {
            continue;
        }
        if (!CanIKReachThisTarget(object.GetPosition(), &GetActiveWeapon(), true)) {
            continue;
        }
        EvaluateCandidate(&object);
    }

    // Vehicles (Only in co-op)
    if (CGameLogic::IsCoopGameGoingOn()) {
        for (CVehicle& vehicle : GetVehiclePool()->GetAllValid()) {
            if (vehicle.m_nPhysicalFlags & PHYSICAL_DESTROYED) {
                continue;
            }
            if (vehicle.m_nVehicleSubType == VEHICLE_TYPE_BMX) {
                continue;
            }
            if (!CanIKReachThisTarget(vehicle.GetPosition(), &GetActiveWeapon(), true)) {
                continue;
            }
            EvaluateCandidate(&vehicle);
        }
    }

    if (!best) {
        return false;
    }

    // Let the newly targeted ped know we're aiming at him
    if (best->GetType() == ENTITY_TYPE_PED
        && CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD)->m_nWeaponFire != WEAPON_FIRE_MELEE
    ) {
        CPed* const targetPed = best->AsPed();
        if (CPedGroup* const targetGroup = CPedGroups::GetPedsGroup(targetPed)) {
            if (!CPedGroups::AreInSameGroup(targetPed, this)) {
                CEventGroupEvent groupEvent(targetPed, new CEventGunAimedAt(this));
                targetGroup->GetIntelligence().AddEvent(&groupEvent);
            }
        } else {
            CEventGunAimedAt eventGunAimedAt(this);
            targetPed->GetEventGroup().Add(&eventGunAimedAt, false);
        }
    }

    CEntity::ChangeEntityReference(m_pTargetedObject, best);
    GetPlayerData()->m_bDontAllowWeaponChange = true;
    return true;
}

// 0x60EA90
void CPlayerPed::ProcessControl() {
    if (GetPlayerData()->m_nCarDangerCounter)
        GetPlayerData()->m_nCarDangerCounter--;
    if (!GetPlayerData()->m_nCarDangerCounter)
        GetPlayerData()->m_pDangerCar = 0;
    if (GetPlayerData()->m_nFadeDrunkenness) {
        if (GetPlayerData()->m_nDrunkenness - 1 > 0) {
            --GetPlayerData()->m_nDrunkenness;
        } else {
            GetPlayerData()->m_nDrunkenness = 0;
            CMBlur::ClearDrunkBlur();
            GetPlayerData()->m_nFadeDrunkenness = 0;
        }
    }
    if (GetPlayerData()->m_nDrunkenness) 
        CMBlur::SetDrunkBlur(GetPlayerData()->m_nDrunkenness / 255.0f);
    if (GetPlayerData()->m_bRequireHandleBreath) {
        if (CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG) > GetPlayerData()->m_fBreath)
            GetPlayerData()->m_fBreath += CTimer::GetTimeStep() + CTimer::GetTimeStep();
        GetPlayerData()->m_bRequireHandleBreath = false;
    }
    GetPlayerData()->m_bRequireHandleBreath = true;
    CPed::ProcessControl();
    bCheckColAboveHead = true;
    float markColor = 1.0f;
    bool limitMarkColor = true;
    CVector effectPos;
    CPad* pad = CPad::GetPad(m_nPedType);
    if (!bCanPointGunAtTarget) {
        GetPlayerWanted()->Update();
        PruneReferences();
        if (GetActiveWeapon().m_Type == WEAPON_MINIGUN) {
            auto weaponInfo = CWeaponInfo::GetWeaponInfo(WEAPON_MINIGUN, eWeaponSkill::STD);
            if (GetIntelligence()->GetTaskUseGun()) {
                auto animAssoc = GetIntelligence()->GetTaskUseGun()->m_Anim;
                if (animAssoc && animAssoc->m_CurrentTime - animAssoc->m_TimeStep < weaponInfo->m_fAnimLoopEnd) {
                    if (GetPlayerData()->m_fGunSpinSpeed < 0.45f) {
                        GetPlayerData()->m_fGunSpinSpeed += CTimer::GetTimeStep() * 0.025f;
                        GetPlayerData()->m_fGunSpinSpeed = std::min(GetPlayerData()->m_fGunSpinSpeed, 0.45f);
                    }
                    if (pad->GetWeapon(this) && GetActiveWeapon().m_TotalAmmo > 0 && animAssoc->m_CurrentTime >= weaponInfo->m_fAnimLoopStart) 
                        m_weaponAudio.AddAudioEvent(AE_WEAPON_FIRE_MINIGUN_AMMO);
                    else 
                        m_weaponAudio.AddAudioEvent(AE_WEAPON_FIRE_MINIGUN_NO_AMMO);
                }
            } else {
                if (GetPlayerData()->m_fGunSpinSpeed > 0.0f) {
                    GetPlayerData()->m_fGunSpinSpeed -= CTimer::GetTimeStep() * 0.003f;
                    GetPlayerData()->m_fGunSpinSpeed = std::max(GetPlayerData()->m_fGunSpinSpeed, 0.0f);
                }
            }
        }
        if (GetActiveWeapon().m_Type == WEAPON_CHAINSAW && m_nPedState != PEDSTATE_ATTACK && !bInVehicle) {
            GetIntelligence()->GetTaskSwim(); // hmmm?
        }
        if (m_pTargetedObject) {
            ClearReference(m_p3rdPersonMouseTarget);
            if (m_pTargetedObject->GetIsTypePed()) {
                CPed* targetPed = m_pTargetedObject->AsPed();
                auto weaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, GetWeaponSkill());
                float targetHeadRange = weaponInfo->GetTargetHeadRange();
                markColor = targetPed->m_fHealth / targetPed->m_fMaxHealth;
                bool instantFireHit = false;
                if (targetPed->IsAlive()) {
                    auto stdWeaponInfo = CWeaponInfo::GetWeaponInfo(GetActiveWeapon().m_Type, eWeaponSkill::STD);
                    if (stdWeaponInfo->m_nWeaponFire == WEAPON_FIRE_INSTANT_HIT) {
                        instantFireHit = true;
                        CVector distance = targetPed->GetPosition() - GetPosition();
                        if (targetHeadRange * targetHeadRange > distance.SquaredMagnitude()) {
                            GetPlayerData()->m_nTargetBone = BONE_HEAD;
                            GetPlayerData()->m_vecTargetBoneOffset.x = 0.05f;
                        }
                    }
                }
                if (!instantFireHit) {
                    GetPlayerData()->m_nTargetBone = BONE_SPINE1;
                    GetPlayerData()->m_vecTargetBoneOffset.x = 0.2f;
                }
                effectPos = GetPlayerData()->m_vecTargetBoneOffset;
                targetPed->GetTransformedBonePosition(effectPos, static_cast<eBoneTag>(GetPlayerData()->m_nTargetBone), false);
                bool targetIsInVehicle = false;
                if (markColor > 0.0f) {
                    if (!targetPed->bInVehicle && targetPed->m_nMoveState != PEDMOVE_STILL) {
                        effectPos += targetPed->m_vecMoveSpeed * CTimer::GetTimeStep();
                    }
                }
                if (targetPed->bInVehicle) {
                    auto targetVeh = targetPed->m_pVehicle;
                    if (targetVeh)
                        effectPos += (targetVeh->m_vecMoveSpeed + targetVeh->m_vecTurnSpeed) * CTimer::GetTimeStep();
                }
            } else if (m_pTargetedObject->GetIsTypeVehicle()) {
                CVehicle* targetVeh = m_pTargetedObject->AsVehicle();
                effectPos = (targetVeh->m_vecMoveSpeed + targetVeh->m_vecTurnSpeed) * CTimer::GetTimeStep();
                effectPos += targetVeh->GetPosition();
            } else if (m_pTargetedObject->GetIsTypeObject()) {
                CObject* targetObj = m_pTargetedObject->AsObject();
                effectPos = targetObj->m_vecMoveSpeed * CTimer::GetTimeStep();
                effectPos += targetObj->GetPosition();
                markColor = targetObj->m_fHealth * 0.001f;
            } else {
                effectPos = m_pTargetedObject->GetPosition();
                limitMarkColor = false; 
            }
        }
    }
    if (m_pTargetedObject) {
        uint8 r = 0, g = 0, b = 0;
        bool setRGB = true;
        if (limitMarkColor) {
            if (markColor > 0.0f)
                markColor = std::min(markColor, 1.0f);
            else
                setRGB = false;
        }
        if (setRGB) {
            r = static_cast<uint8>((1.0f - markColor) * 255.0f);
            g = static_cast<uint8>(markColor * 255.0f);
            b = static_cast<uint8>(0.0f);
        }
        CVector distance = effectPos - GetPosition();
        float size = 1.0f - distance.Magnitude() * 0.02f;
        CWeaponEffects::MarkTarget(m_nPedType, effectPos, r, g, b, 255u, size, false);
    }
    if (m_nMoveState != PEDMOVE_NONE) {
        if (m_nMoveState != PEDMOVE_RUN) {
            if (m_nMoveState != PEDMOVE_SPRINT)
                HandleSprintEnergy(false, 1.0f);
        } else if (CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN) > GetPlayerData()->m_fTimeCanRun)
            GetPlayerData()->m_fTimeCanRun += CTimer::GetTimeStep() * 0.15f;
    } else if (bInVehicle) {
        if (m_pVehicle && !m_pVehicle->IsSubBMX())
            HandleSprintEnergy(false, 1.0f);
    }
    GetActiveWeapon().Update(this);
    if (m_nPedState == PEDSTATE_DEAD || m_nPedState == PEDSTATE_DIE) {
        ClearWeaponTarget();
        return;
    }
    if (pad) {
        if (pad->WeaponJustDown(this)) {
            auto& activeWeapon = GetActiveWeapon();
            auto weaponType = activeWeapon.m_Type;
            if (!TheCamera.Using1stPersonWeaponMode() || activeWeapon.m_State == WEAPONSTATE_OUT_OF_AMMO) {
                if (!GetIntelligence()->GetTaskSwim()) {
                    if (weaponType == WEAPON_SNIPERRIFLE) {
                        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_FIRE_FAIL_SNIPERRIFFLE, 0.0f, 1.0f);
                    } else if (weaponType == WEAPON_RLAUNCHER || weaponType == WEAPON_RLAUNCHER_HS) {
                        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_FIRE_FAIL_ROCKET, 0.0f, 1.0f);
                    }
                }
            }
        }
        if (IsPedShootable() && this->m_nPedState != PEDSTATE_ANSWER_MOBILE) {
            int32 slot = CWorld::FindPlayerSlotWithPedPointer(this);
            if (!CWorld::Players[slot].m_pRemoteVehicle)
                ProcessWeaponSwitch(pad);
        }
    }
    ProcessAnimGroups();
    if (pad && TheCamera.GetActiveCamera().m_nMode == MODE_FOLLOWPED && !TheCamera.GetActiveCamera().m_nDirectionWasLooking) {
        auto& activeCam = TheCamera.GetActiveCamera();
        m_nLookTime = 0;
        float lookDir = CGeneral::LimitRadianAngle(atan2(-activeCam.m_vecFront.x, activeCam.m_vecFront.y));
        float angle = fabs(lookDir - m_fCurrentRotation);
        if (m_nPedState != PEDSTATE_ATTACK && angle > DegreesToRadians(30.0f) && angle < DegreesToRadians(330.0f)) {
            if (angle > DegreesToRadians(150.0f) && angle < DegreesToRadians(210.0f)) {
                float dir1 = CGeneral::LimitRadianAngle(m_fCurrentRotation - DegreesToRadians(150.0f));
                float dir2 = CGeneral::LimitRadianAngle(m_fCurrentRotation + DegreesToRadians(150.0f));
                lookDir = dir1;
                if (m_fLookDirection != 999'999.f && !bIsDucking) {
                    if (fabs(dir2 - m_fLookDirection) <= fabs(dir1 - m_fLookDirection))
                        lookDir = dir2;
                }
            }
            SetLookFlag(lookDir, true, false);
            SetLookTimer(static_cast<uint32>(CTimer::GetTimeStepInMS() * 5.0f));
        } else {
            ClearLookFlag();
        }
    }
    if (m_nMoveState == PEDMOVE_SPRINT && bIsLooking) {
        ClearLookFlag();
        SetLookTimer(250);
    }
    if (m_vecMoveSpeed.Magnitude() >= 0.1f) {
        GetPlayerData()->m_nStandStillTimer = 0;
        GetPlayerData()->m_bStoppedMoving = false;
    } else if (!GetPlayerData()->m_nStandStillTimer) {
        GetPlayerData()->m_nStandStillTimer = CTimer::GetTimeInMS() + 500;
    } else if (CTimer::GetTimeInMS() > GetPlayerData()->m_nStandStillTimer) {
        GetPlayerData()->m_bStoppedMoving = true;
    }
    if (GetPlayerData()->m_bDontAllowWeaponChange) {
        if (IsPlayer()) {
            if (!CPad::GetPad(0)->GetTarget())
                GetPlayerData()->m_bDontAllowWeaponChange = false;
        }
    }
    if (m_nPedState != PEDSTATE_SNIPER_MODE && GetActiveWeapon().m_State == WEAPONSTATE_FIRING)
        GetPlayerData()->m_nLastTimeFiring = CTimer::GetTimeInMS();
    ProcessGroupBehaviour(pad);
    if (bInVehicle)
        CCarCtrl::RegisterVehicleOfInterest(m_pVehicle);
    if (!GetIsVisible())
        UpdateRpHAnim();
    if (bInVehicle) {
        CPad* pad = CPad::GetPad(0);
        if (!pad->IsDPadDownPressed()) {
            if (pad->IsDPadUpPressed())
                GetPlayerData()->m_bPlayersGangActive = true;
        } else {
            GetPlayerData()->m_bPlayersGangActive = false;
        }
    }
    if (physicalFlags.bSubmergedInWater) {
        CVector pos = GetPosition();
        pos.z += 1.5f;
        if (CWaterLevel::GetWaterLevel(pos.x, pos.y, pos.z, GetPlayerData()->m_fWaterHeight, true, nullptr)) {
            auto& box = CEntity::GetColModel()->GetBoundingBox();
            float playerMinZ = pos.z + box.m_vecMin.z;
            float playerMaxZ = pos.z + box.m_vecMax.z;
            if (GetPlayerData()->m_fWaterHeight < playerMaxZ) {
                if (GetPlayerData()->m_fWaterHeight > playerMinZ)
                    GetPlayerData()->m_nWaterCoverPerc = static_cast<uint8>((GetPlayerData()->m_fWaterHeight - playerMinZ) / (playerMaxZ - playerMinZ) * 100.0f);
                else
                    GetPlayerData()->m_nWaterCoverPerc = 0;
            } else {
                GetPlayerData()->m_nWaterCoverPerc = 100;
            }
        } else {
            physicalFlags.bSubmergedInWater = false;
        }
    } else {
        GetPlayerData()->m_nWaterCoverPerc = 0;
    }
    if ((CTimer::GetFrameCounter() & 0x7F) == 0 && !FindPlayerVehicle()) {
        auto& group = CPedGroups::GetGroup(GetPlayerData()->m_nPlayerGroup);
        if (group.m_bMembersEnterLeadersVehicle) {
            int32 memberCount = group.m_groupMembership.CountMembersExcludingLeader();
            if (memberCount > 0) {
                float distance = group.FindDistanceToNearestMember(nullptr);
                if (distance > 20.0f && distance < 100.0f && CGame::currArea == AREA_CODE_NORMAL_WORLD) {
                    if (memberCount == 1)
                        Say(CTX_GLOBAL_ORDER_KEEP_UP_ONE);
                    else
                        Say(CTX_GLOBAL_ORDER_KEEP_UP_MANY);
                    for (int32 i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; ++i) {
                        CPed* member = group.m_groupMembership.GetMember(i);
                        if (member && CGeneral::GetRandomNumberInRange(0.0f, 1.0f) < 0.5f) {
                            int32 offset = CGeneral::GetRandomNumberInRange(3000, 4500);
                            member->Say(CTX_GLOBAL_FOLLOW_REPLY, offset);
                        }
                    }
                }
            }
        }
    }
    if (!bInVehicle && GetLightingTotal() <= 0.05f && !CEntryExitManager::WeAreInInteriorTransition())
        Say(CTX_GLOBAL_BREATHING);
}

// 0x609490
void CPlayerPed::SetMoveAnim() {
    //nop
}
