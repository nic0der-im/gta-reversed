/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "MissionCleanup.h"

#include "CarCtrl.h"
#include "Garages.h"
#include "CullZones.h"
#include "VehicleRecording.h"
#include "SpecialFX.h"
#include "PostEffects.h"
#include "EntryExitManager.h"
#include "Tasks/TaskTypes/TaskComplexUseMobilePhone.h"
#include "IplStore.h"
#include "DecisionMakers/DecisionMakerTypesFileLoader.h"
#include "Scripts/Scripted2dEffects.h"
#include "Events/EventGunShot.h"

extern float& gCurDistForCam; // Defined in `Camera.cpp`

// TODO(re): Unnamed globals - only their addresses and the written values are known.
static auto& g_bCriminalWanted     = StaticRef<bool>(0xB72994);  // Checked by `CTaskComplexKillCriminal`
static auto& s_Unknown0xB728E0     = StaticRef<int32>(0xB728E0);
static auto& s_Unknown0xB6EC2C     = StaticRef<uint8>(0xB6EC2C); // Next to `gLastTime2PlayerCameraWasCollided` (Camera.cpp)
static auto& s_Unknown0xB6FD18     = StaticRef<uint8>(0xB6FD18); // Near the tail of `CCamera`
static auto& s_Unknown0xBA18D9     = StaticRef<uint8>(0xBA18D9);
static auto& s_Unknown0xBA677C     = StaticRef<uint8>(0xBA677C);

CMissionCleanup::CMissionCleanup() {
    Init();
}

/* Initializes data
 * @addr 0x4637A0
 */
void CMissionCleanup::Init() {
    m_Count = 0;
    m_Objects.fill(tMissionCleanupEntity());
}

/* Performs a clean-up
 * @addr 0x468560
 */
void CMissionCleanup::Process() {
    ZoneScoped;

    if (CTheScripts::bScriptHasFadedOut) {
        CTheScripts::bScriptHasFadedOut = false;
        if (CWorld::Players[0].m_nPlayerState != PLAYERSTATE_HAS_DIED
         && CWorld::Players[0].m_nPlayerState != PLAYERSTATE_HAS_BEEN_ARRESTED
        ) {
            TheCamera.Fade(0.5f, eFadeFlag::FADE_OUT);
            CPad::GetPad(0)->bPlayerSafe = false;
        }
    }

    CTrain::DisableRandomTrains(false);
    CTrain::ReleaseMissionTrains();
    CPlane::SwitchAmbientPlanes(true);

    CPopulation::PedDensityMultiplier = 1.0f;
    CCarCtrl::CarDensityMultiplier    = 1.0f;

    if (CGangWars::bTrainingMission) {
        CGangWars::bTrainingMission = false;
        CTheZones::FillZonesWithGangColours(!CGangWars::bGangWarsActive);
    }

    CPopulation::m_AllRandomPedsThisType         = -1;
    CGangWars::bCanTriggerGangWarWhenOnAMission  = false;
    CGangWars::ClearSpecificZonesToTriggerGangWar();
    CPopulation::m_bDontCreateRandomGangMembers  = false;
    CPopulation::m_bOnlyCreateRandomGangMembers  = false;
    CPopulation::m_bDontCreateRandomCops         = false;
    CGarages::NoResprays                         = false;
    CGarages::AllRespraysCloseOrOpen(true);
    CCullZones::bMilitaryZonesDisabled            = false;

    FindPlayerWanted()->m_Multiplier = 1.0f;
    CPickups::RemoveMissionPickUps();
    CRoadBlocks::ClearScriptRoadBlocks();
    CStreaming::DisableCopBikes(false);
    g_bCriminalWanted                       = true;
    CObject::bArea51SamSiteDisabled         = false;
    CObject::bAircraftCarrierSamSiteDisabled = true;

    ThePaths.ReleaseRequestedNodes();
    ThePaths.UnMarkAllRoadNodesAsDontWander();
    ThePaths.TidyUpNodeSwitchesAfterMission();
    CVehicleRecording::RemoveAllRecordingsThatArentUsed();
    TheCamera.SetWideScreenOff();

    //! Two player camera modes
    TheCamera.m_nModeForTwoPlayersSeparateCars                = (eCamMode)0x32;
    TheCamera.m_nModeForTwoPlayersSameCarShootingAllowed      = (eCamMode)0x31;
    TheCamera.m_nModeForTwoPlayersSameCarShootingNotAllowed   = (eCamMode)0x03;
    TheCamera.m_nModeForTwoPlayersNotBothInCar                = (eCamMode)0x30;
    TheCamera.m_bDisableFirstPersonInCar                      = false;
    s_Unknown0xB6FD18                                         = 0;
    TheCamera.InitialiseScriptableComponents();
    TheCamera.ResetDuckingSystem(nullptr);
    gCurDistForCam = 1.0f;
    s_Unknown0xB6EC2C = 0;

    CGameLogic::bScriptCoopGameGoingOn = false;
    CTheScripts::bDrawCrossHair        = eCrossHairType::NONE;
    CSpecialFX::bVideoCam               = false;
    CSpecialFX::bLiftCam                = false;
    CPostEffects::ScriptResetForEffects();
    CEntryExitManager::ms_bDisabled    = 0;
    if (CGame::currArea == AREA_CODE_NORMAL_WORLD) {
        CTimeCycle::StopExtraColour(false);
    }

    for (uint16 i = 0; i < 4; i++) {
        AudioEngine.ClearMissionAudio((uint8)i);
    }

    CWeather::ReleaseWeather();
    s_Unknown0xB728E0 = 99999;
    for (int32 i = 0; i < 10; i++) {
        CStreaming::SetMissionDoesntRequireSpecialChar(i);
    }
    CTheScripts::ClearAllSuppressedCarModels();

    CTheScripts::ForceRandomCarModel      = -1;
    CStreaming::ms_disableStreaming       = false;
    CHud::m_ItemToFlash                   = (eHudItem)-1;
    CHud::bScriptDontDisplayRadar         = false;
    CHud::bScriptDontDisplayVehicleName   = false;
    CHud::bScriptDontDisplayAreaName      = false;
    CHud::bScriptForceDisplayWithCounters = false;
    s_Unknown0xBA677C                     = 0;
    CTheScripts::RadarZoomValue           = 0;
    CTheScripts::RadarShowBlipOnAllLevels = false;
    CTheScripts::HideAllFrontEndMapBlips  = false;
    C3dMarkers::ForceRender(false);
    CTheScripts::bDisplayHud                             = true;
    CTheScripts::fCameraHeadingWhenPlayerIsAttached      = 0.0f;
    CTheScripts::fCameraHeadingStepWhenPlayerIsAttached  = 0.0f;
    CTheScripts::bEnableCraneRaise                       = true;
    CTheScripts::bEnableCraneLower                       = true;
    CTheScripts::bEnableCraneRelease                     = true;
    s_Unknown0xBA18D9                                    = 0;
    CTheScripts::bUseMessageFormatting                   = false;
    CTheScripts::MessageCentre                           = 0;
    CTheScripts::MessageWidth                            = 0;
    CTheScripts::bDrawOddJobTitleBeforeFade              = true;
    CTheScripts::bDrawSubtitlesBeforeFade                = true;

    //> Player group
    {
        auto* const playerPed  = CWorld::Players[0].m_pPed;
        auto* const playerData = playerPed->GetPlayerData();
        auto&       group      = CPedGroups::GetGroup(playerData->m_nPlayerGroup);

        // TODO(re): `CPedGroupMembership::m_separationRange` (0xC0994C) is private and has no setter,
        //           so it can't be set to 120.0f here (Nothing else in the repo can set it either)

        group.GetIntelligence().SetDefaultTaskAllocatorType(ePedGroupDefaultTaskAllocatorType::FOLLOW_LIMITED);
        group.GetIntelligence().SetGroupDecisionMakerType(eDecisionMakerType::UNKNOWN);

        playerData->m_bGroupStuffDisabled = false;

        playerPed->ForceGroupToAlwaysFollow(false);
        playerPed->ForceGroupToNeverFollow(false);
        playerPed->MakePlayerGroupReappear();

        playerData->m_nScriptLimitToGangSize = 99;
        playerPed->field_728                 = 0x3F800000; // TODO(re): The original stores `1.0f` into `CPed::field_728`, which is typed `int32`

        CWorld::Players[0].m_PlayerData.m_nFadeDrunkenness = 1;
        CWorld::Players[0].m_PlayerData.m_nDrugLevel       = 0;
        playerPed->EnablePedSpeech();
        playerPed->EnablePedSpeechForScriptSpeech();
    }

    //> Pad
    {
        auto& pad = *CPad::GetPad(0);
        pad.SetDrunkInputDelay(0);
        pad.bApplyBrakes                     = false;
        pad.bDisablePlayerEnterCar           = false;
        pad.bDisablePlayerDuck               = false;
        pad.bDisablePlayerFireWeapon         = false;
        pad.bDisablePlayerFireWeaponWithL1   = false;
        pad.bDisablePlayerCycleWeapon        = false;
        pad.bDisablePlayerJump               = false;
        pad.bDisablePlayerDisplayVitalStats  = false;
    }

    //> Mobile phone task (if the player is using one, abort it)
    if (auto* const task = CWorld::Players[0].m_pPed->GetIntelligence()->GetTaskManager().FindTaskByType(
            TASK_PRIMARY_PRIMARY,
            TASK_COMPLEX_USE_MOBILE_PHONE
        )
    ) {
        auto* const phoneTask = static_cast<CTaskComplexUseMobilePhone*>(task);
        if (!phoneTask->m_bQuit) {
            phoneTask->m_bQuit = true;
            phoneTask->MakeAbortable(CWorld::Players[0].m_pPed, ABORT_PRIORITY_LEISURE, nullptr);
        }
    }

    CVehicle::bDisableRemoteDetonation          = false;
    CVehicle::bDisableRemoteDetonationOnContact = false;
    CGameLogic::ClearSkip(true);

    if (CGameLogic::GameState != GAMELOGIC_STATE_WASTED
     && CGameLogic::GameState != GAMELOGIC_STATE_BUSTED
    ) {
        if (const auto state = FindPlayerPed()->m_nPedState;
            state != PEDSTATE_DEAD && state != PEDSTATE_DIE
        ) {
            CRestart::ClearRespawnPointForDurationOfMission();
        }
    }

    CTheScripts::RiotIntensity = 0;
    gFireManager.ClearAllScriptFireFlags();
    CTheScripts::StoreVehicleIndex    = -1;
    CTheScripts::StoreVehicleWasRandom = true;
    CTheScripts::UpsideDownCars.Init();
    CTheScripts::StuckCars.Init();
    CStats::bShowUpdateStats                    = true;
    CEventGunShot::ms_fGunShotSenseRangeForRiot2 = -1.0f;
    CHud::m_fHelpMessageBoxWidth                = 200.0f;
    CVehicle::ms_forceVehicleLightsOff          = false;

    if (!CTheScripts::bMiniGameInProgress) {
        if (!CWorld::Players[CWorld::PlayerInFocus].m_pRemoteVehicle) {
            TheCamera.Restore();
        }

        auto* const wanted = CWorld::Players[0].m_pPed->GetPlayerWanted();
        wanted->m_bPoliceBackOff   = false;
        wanted->m_bEverybodyBackOff = false;

        CWorld::Players[0].MakePlayerSafe(false, 10'000.0f);
        CHud::SetHelpMessage(nullptr, true, false, false);
    }

    //> Clean up all entities known to the script
    for (auto& entity : m_Objects) {
        switch ((MissionCleanUpEntityType)entity.type) {
        case MISSION_CLEANUP_ENTITY_TYPE_EMPTY:
            continue;
        case MISSION_CLEANUP_ENTITY_TYPE_VEHICLE: {
            if (auto* const veh = GetVehiclePool()->GetAtRef(entity.handle)) {
                CTheScripts::CleanUpThisVehicle(veh);
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_PED: {
            if (auto* const ped = GetPedPool()->GetAtRef(entity.handle)) {
                CTheScripts::CleanUpThisPed(ped);
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_OBJECT: {
            if (auto* const obj = GetObjectPool()->GetAtRef(entity.handle)) {
                CTheScripts::CleanUpThisObject(obj);
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_PARTICLE: {
            if (const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_EFFECT_SYSTEM); idx >= 0) {
                if (auto* const fx = CTheScripts::ScriptEffectSystemArray[idx].m_pFxSystem) {
                    fx->Kill();
                    CTheScripts::RemoveScriptEffectSystem(entity.handle);
                }
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_GROUP: {
            if (const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_PED_GROUP); idx >= 0) {
                CPedGroups::RemoveGroup(idx);
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_PED_QUEUE: {
            if (const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_2D_EFFECT); idx >= 0) {
                CScripted2dEffects::ms_activated[idx] = false;
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_TASK_SEQUENCE: {
            if (const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_SEQUENCE_TASK); idx >= 0) {
                auto& sequence = CTaskSequences::ms_taskSequence[idx];
                if (sequence.m_RefCnt == 0) {
                    sequence.m_bFlushTasks = false;
                    sequence.Flush();
                } else {
                    sequence.m_bFlushTasks = true;
                }
                CTaskSequences::ms_bIsOpened[idx]         = false;
                CTheScripts::ScriptSequenceTaskArray[idx].m_bUsed = false;
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_DECISION_MAKER: {
            if (const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_DECISION_MAKER); idx >= 0) {
                CDecisionMakerTypesFileLoader::UnloadDecisionMaker((eDecisionTypes)idx);
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_SEARCHLIGHT: {
            if (const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_SEARCH_LIGHT); idx >= 0) {
                CTheScripts::RemoveScriptSearchLight(entity.handle);
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_CHECKPOINT: {
            if (const auto idx = CTheScripts::GetActualScriptThingIndex(entity.handle, SCRIPT_THING_CHECKPOINT); idx >= 0) {
                CTheScripts::RemoveScriptCheckpoint(entity.handle);
            }
            break;
        }
        case MISSION_CLEANUP_ENTITY_TYPE_TXD: {
            CTheScripts::RemoveScriptTextureDictionary();
            break;
        }
        default:
            break;
        }

        RemoveEntityFromList(entity.handle, (MissionCleanUpEntityType)entity.type);
    }

    //> Wipe the ped-type acquaintance matrix
    for (auto pedType = 0; pedType < 0x18; pedType++) {
        for (auto otherPedType = 0x18; otherPedType <= 0x1F; otherPedType++) {
            const auto flag = CPedType::GetPedFlag((ePedType)otherPedType);
            for (auto id = 0; id < 5; id++) {
                CPedType::ClearPedTypeAsAcquaintance((AcquaintanceId)id, (ePedType)pedType, flag);
            }
        }
    }
    for (auto pedType = 0x18; pedType <= 0x1F; pedType++) {
        for (auto id = 0; id < 5; id++) {
            CPedType::ClearPedTypeAcquaintances((AcquaintanceId)id, (ePedType)pedType);
        }
    }
}

/* Finds a free entity, returns NULL if no free entity can be found.
 * @addr 0x4637C0
 */
tMissionCleanupEntity* CMissionCleanup::FindFree() {
    for (auto& entity : m_Objects) {
        if (entity.type == MISSION_CLEANUP_ENTITY_TYPE_EMPTY) {
            return &entity;
        }
    }
    return nullptr;
}

/* Adds entity to list
 * @addr 0x4637E0
 */
void CMissionCleanup::AddEntityToList(int32 handle, MissionCleanUpEntityType type) {
    for (auto& entity : m_Objects) {
        if (entity.type == MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_EMPTY) {
            entity.handle = handle;
            entity.type   = type;
            ++m_Count;
            return;
        }
    }
}

/* Remotes entity from list
 * @addr 0x4654B0
 */
void CMissionCleanup::RemoveEntityFromList(int32 handle, MissionCleanUpEntityType type) {
    for (auto& entity : m_Objects) {
        if (entity.type != type || entity.handle != handle) {
            continue;
        }

        switch (entity.type) {
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_VEHICLE: {
            auto* veh = GetVehiclePool()->GetAtRef(entity.handle);

            if (veh && veh->m_bIsStaticWaitingForCollision) {
                veh->m_bIsStaticWaitingForCollision = false;
                if (!veh->GetIsStatic()) {
                    veh->AddToMovingList();
                }
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_PED: {
            auto* ped = GetPedPool()->GetAtRef(entity.handle);

            if (ped && ped->m_bIsStaticWaitingForCollision) {
                ped->m_bIsStaticWaitingForCollision = false;
                if (!ped->GetIsStatic()) {
                    ped->AddToMovingList();
                }
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_OBJECT: {
            auto* obj = GetObjectPool()->GetAtRef(entity.handle);

            if (obj && obj->m_bIsStaticWaitingForCollision) {
                obj->m_bIsStaticWaitingForCollision = false;
                if (!obj->GetIsStatic()) {
                    obj->AddToMovingList();
                }
            }
            break;
        }
        default:
            break;
        }

        entity = tMissionCleanupEntity();
        --m_Count;
    }
}

/* Checks if collision has loaded for mission objects
 * @addr 0x4652D0
 */
void CMissionCleanup::CheckIfCollisionHasLoadedForMissionObjects() {
    ZoneScoped;

    for (auto& entity : m_Objects) {
        CEntity* ent{};
        switch (entity.type) {
        case MISSION_CLEANUP_ENTITY_TYPE_VEHICLE:
            ent = GetVehiclePool()->GetAtRef(entity.handle);
            break;
        case MISSION_CLEANUP_ENTITY_TYPE_PED:
            ent = GetPedPool()->GetAtRef(entity.handle);
            break;
        case MISSION_CLEANUP_ENTITY_TYPE_OBJECT:
            ent = GetObjectPool()->GetAtRef(entity.handle);
            break;
        default:
            continue;
        }

        if (!ent || !ent->m_bIsStaticWaitingForCollision) {
            continue;
        }

        const auto& pos      = ent->GetPosition();
        const auto  areaCode = ent->GetAreaCode();

        if (!CColStore::HasCollisionLoaded(pos, areaCode)) {
            continue;
        }
        if (!CIplStore::HaveIplsLoaded(pos, areaCode)) {
            continue;
        }

        ent->m_bIsStaticWaitingForCollision = false;

        if (!ent->GetIsStatic()) {
            ent->AsPhysical()->AddToMovingList();
        }

        if (ent->GetIsTypeVehicle()) {
            auto* const veh = ent->AsVehicle();
            switch (veh->m_nVehicleType) {
            case VEHICLE_TYPE_AUTOMOBILE:
            case VEHICLE_TYPE_TRAILER:
                static_cast<CAutomobile*>(veh)->PlaceOnRoadProperly();
                break;
            case VEHICLE_TYPE_BIKE:
                static_cast<CBike*>(veh)->PlaceOnRoadProperly();
                break;
            default:
                break;
            }
        }
    }
}

// NOTSA
void CMissionCleanup::AddEntityToList(CObject& obj) {
    return AddEntityToList(GetObjectPool()->GetRef(&obj), MISSION_CLEANUP_ENTITY_TYPE_OBJECT);
}

// NOTSA
void CMissionCleanup::AddEntityToList(CPed& ped) {
    return AddEntityToList(GetPedPool()->GetRef(&ped), MISSION_CLEANUP_ENTITY_TYPE_PED);
}

// NOTSA
void CMissionCleanup::AddEntityToList(CVehicle& veh) {
    return AddEntityToList(GetVehiclePool()->GetRef(&veh), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
}

// NOTSA
void CMissionCleanup::RemoveEntityFromList(CObject& obj) {
    return RemoveEntityFromList(GetObjectPool()->GetRef(&obj), MISSION_CLEANUP_ENTITY_TYPE_OBJECT);
}

void CMissionCleanup::RemoveEntityFromList(CPed& ped) {
    return RemoveEntityFromList(GetPedPool()->GetRef(&ped), MISSION_CLEANUP_ENTITY_TYPE_PED);
}

void CMissionCleanup::RemoveEntityFromList(CVehicle& veh) {
    return RemoveEntityFromList(GetVehiclePool()->GetRef(&veh), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
}
