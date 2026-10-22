/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "CarCtrl.h"
#include "TrafficLights.h"
#include "TheScripts.h"
#include "GangWars.h"
#include "Game.h"
#include "General.h"
#include "GameLogic.h"
#include "CutsceneMgr.h"
#include "TheCarGenerators.h"
#include "eAreaCodes.h"
#include "Curves.h"
#include "Garages.h"
#include "EventPotentialGetRunOver.h"
#include "EventAcquaintancePedHate.h"
#include "TaskComplexWander.h"
#include "TaskComplexDriveWander.h"

#include <reversiblebugfixes/Bugs.hpp>

#include "VehicleRecording.h"
#include "Tasks/TaskTypes/TaskComplexLeaveAnyCar.h"

auto& apCarsToKeep = StaticRef<CVehicle*[2]>(0x969084);
auto& aCarsToKeepTime = StaticRef<std::array<uint32, 2>>(0x96907C);

// Cache used by `CCarCtrl::GenerateCarCreationCoors2` (0x424210)
static auto& s_CarCreationNodeCacheMask = StaticRef<uint32>(0x969108);
static auto& s_CarCreationNodeCacheTime = StaticRef<uint32>(0x9690E8);
static auto& s_CarCreationCachedPosn    = StaticRef<CVector>(0x9690EC);
static auto& s_CarCreationNodeB1        = StaticRef<CNodeAddress>(0x969104);
static auto& s_CarCreationNodeB2        = StaticRef<CNodeAddress>(0x969100);
static auto& s_CarCreationNodeA1        = StaticRef<CNodeAddress>(0x9690FC);
static auto& s_CarCreationNodeA2        = StaticRef<CNodeAddress>(0x9690F8);

void CCarCtrl::InjectHooks()
{
    RH_ScopedClass(CCarCtrl);
    RH_ScopedCategoryGlobal();

    using namespace ReversibleHooks;
    RH_ScopedInstall(Init, 0x4212E0);
    RH_ScopedInstall(ReInit, 0x4213B0);
    RH_ScopedInstall(InitSequence, 0x421740);
    RH_ScopedInstall(ChooseGangCarModel, 0x421A40);
    RH_ScopedInstall(ChoosePoliceCarModel, 0x421980);
    RH_ScopedInstall(CreateCarForScript, 0x431F80);
    RH_ScopedInstall(ChooseBoatModel, 0x421970);
    RH_ScopedInstall(ChooseCarModelToLoad, 0x421900);
    RH_ScopedInstall(GetNewVehicleDependingOnCarModel, 0x421440);
    RH_ScopedInstall(IsAnyoneParking, 0x42C250);
    RH_ScopedInstall(IsThisVehicleInteresting, 0x423EA0);
    RH_ScopedInstall(JoinCarWithRoadAccordingToMission, 0x432CB0);
    RH_ScopedInstall(PossiblyFireHSMissile, 0x429600);
    RH_ScopedInstall(PruneVehiclesOfInterest, 0x423F10);
    RH_ScopedInstall(RemoveCarsIfThePoolGetsFull, 0x4322B0);
    RH_ScopedInstall(RemoveDistantCars, 0x42CD10);
    RH_ScopedInstall(RemoveFromInterestingVehicleList, 0x423ED0);
    RH_ScopedInstall(ScriptGenerateOneEmergencyServicesCar, 0x42FBC0);
    RH_ScopedInstall(SlowCarDownForObject, 0x426220);
    RH_ScopedInstall(SlowCarOnRailsDownForTrafficAndLights, 0x434790);
    RH_ScopedInstall(FindMaxSteerAngle, 0x427FE0);
    RH_ScopedInstall(GenerateRandomCars, 0x4341C0);
    RH_ScopedInstall(ChooseModel, 0x424CE0);
    RH_ScopedInstall(ClearInterestingVehicleList, 0x423F00);
    RH_ScopedInstall(ClitargetOrientationToLink, 0x422760);
    RH_ScopedInstall(CreateConvoy, 0x42C740);
    RH_ScopedInstall(CreatePoliceChase, 0x42C2B0);
    RH_ScopedInstall(DealWithBend_Racing, 0x428040);
    RH_ScopedInstall(DragCarToPoint, 0x42EC90);
    RH_ScopedInstall(FindAngleToWeaveThroughTraffic, 0x4325C0);
    RH_ScopedInstall(FindIntersection2Lines, 0x4226F0);
    RH_ScopedInstall(FindLinksToGoWithTheseNodes, 0x42B470);
    RH_ScopedInstall(FindMaximumSpeedForThisCarInTraffic, 0x434400);
    RH_ScopedInstall(FindNodesThisCarIsNearestTo, 0x42BD20);
    RH_ScopedInstall(FindPathDirection, 0x422090);
    RH_ScopedInstall(FindPercDependingOnDistToLink, 0x422620);
    RH_ScopedInstall(FindSequenceElement, 0x421770);
    RH_ScopedInstall(FindSpeedMultiplier, 0x4224E0);
    RH_ScopedInstall(FindSpeedMultiplierWithSpeedFromNodes, 0x424130);
    RH_ScopedInstall(FindGhostRoadHeight, 0x422370);
    RH_ScopedInstall(FireHeliRocketsAtTarget, 0x42B270);
    RH_ScopedInstall(FlyAIHeliInCertainDirection, 0x429A70);
    RH_ScopedInstall(FlyAIHeliToTarget_FixedOrientation, 0x423940);
    RH_ScopedInstall(FlyAIPlaneInCertainDirection, 0x423000);
    RH_ScopedInstall(GenerateCarCreationCoors2, 0x424210);
    RH_ScopedInstall(GenerateEmergencyServicesCar, 0x42F9C0);
    RH_ScopedInstall(GenerateOneEmergencyServicesCar, 0x42B7D0);
    RH_ScopedInstall(GenerateOneRandomCar, 0x430050);
    RH_ScopedInstall(GetAIHeliToAttackPlayer, 0x42F3C0);
    RH_ScopedInstall(GetAIHeliToFlyInDirection, 0x42A730);
    RH_ScopedInstall(GetAIPlaneToAttackPlayer, 0x429780);
    RH_ScopedInstall(GetAIPlaneToDoDogFight, 0x429890);
    RH_ScopedInstall(GetAIPlaneToDoDogFightAgainstPlayer, 0x42F370);
    RH_ScopedInstall(IsThisAnAppropriateNode, 0x42DAB0);
    RH_ScopedInstall(JoinCarWithRoadSystem, 0x42F5A0);
    RH_ScopedInstall(JoinCarWithRoadSystemGotoCoors, 0x42F870);
    RH_ScopedInstall(PickNextNodeAccordingStrategy, 0x432B10);
    RH_ScopedInstall(PickNextNodeRandomly, 0x42DE80);
    RH_ScopedInstall(PickNextNodeToChaseCar, 0x426EF0);
    RH_ScopedInstall(PickNextNodeToFollowPath, 0x427740);
    RH_ScopedInstall(PossiblyRemoveVehicle, 0x424F80);
    RH_ScopedInstall(ReconsiderRoute, 0x42FC40);
    RH_ScopedInstall(RegisterVehicleOfInterest, 0x423DE0);
    RH_ScopedInstall(ScanForPedDanger, 0x42CE40);
    RH_ScopedInstall(SetCoordsOfScriptCar, 0x4342A0);
    RH_ScopedInstall(SetUpDriverAndPassengersForVehicle, 0x4217C0);
    RH_ScopedInstall(SlowCarDownForCarsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x432420);
    RH_ScopedInstall(SlowCarDownForObjectsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x42D4F0);
    RH_ScopedInstall(SlowCarDownForOtherCar, 0x42D0E0);
    RH_ScopedInstall(SlowCarDownForPedsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x425440);
    RH_ScopedInstall(SteerAIBoatWithPhysicsAttackingPlayer, 0x428DE0);
    RH_ScopedInstall(SteerAIBoatWithPhysicsCirclingPlayer, 0x429090);
    RH_ScopedInstall(SteerAIBoatWithPhysicsHeadingForTarget, 0x428BE0);
    RH_ScopedInstall(SteerAICarBlockingPlayerForwardAndBack, 0x422B20);
    RH_ScopedInstall(SteerAICarParkParallel, 0x433BA0);
    RH_ScopedInstall(SteerAICarParkPerpendicular, 0x433EA0);
    RH_ScopedInstall(SteerAICarTowardsPointInEscort, 0x4336D0);
    RH_ScopedInstall(SteerAICarWithPhysics, 0x437C20);
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPath, 0x434900);
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPath_Racing, 0x435830);
    RH_ScopedInstall(SteerAICarWithPhysicsFollowPreRecordedPath, 0x432DD0);
    RH_ScopedInstall(SteerAICarWithPhysicsHeadingForTarget, 0x433280);
    RH_ScopedInstall(SteerAICarWithPhysicsTryingToBlockTarget, 0x4335E0);
    RH_ScopedInstall(SteerAICarWithPhysicsTryingToBlockTarget_Stop, 0x428990);
    RH_ScopedInstall(SteerAICarWithPhysics_OnlyMission, 0x436A90);
    RH_ScopedInstall(SteerAIHeliAsPoliceHeli, 0x42AAD0);
    RH_ScopedInstall(SteerAIHeliFlyingAwayFromPlayer, 0x42ACB0);
    RH_ScopedInstall(SteerAIHeliToCrashAndBurn, 0x4238E0);
    RH_ScopedInstall(SteerAIHeliToFollowEntity, 0x42A750);
    RH_ScopedInstall(SteerAIHeliToKeepEntityInView, 0x42AEB0);
    RH_ScopedInstall(SteerAIHeliToLand, 0x42AD30);
    RH_ScopedInstall(SteerAIHeliTowardsTargetCoors, 0x42A630);
    RH_ScopedInstall(SteerAIPlaneToCrashAndBurn, 0x423880);
    RH_ScopedInstall(SteerAIPlaneToFollowEntity, 0x4237F0);
    RH_ScopedInstall(SteerAIPlaneTowardsTargetCoors, 0x423790);
    RH_ScopedInstall(StopCarIfNodesAreInvalid, 0x422590);
    RH_ScopedInstall(SwitchBetweenPhysicsAndGhost, 0x4222A0);
    RH_ScopedInstall(SwitchVehicleToRealPhysics, 0x423FC0);
    RH_ScopedInstall(TestCollisionBetween2MovingRects, 0x425B30);
    RH_ScopedInstall(TestCollisionBetween2MovingRects_OnlyFrontBumper, 0x425F70);
    RH_ScopedInstall(TestWhetherToFirePlaneGuns, 0x429520);
    RH_ScopedInstall(ThisVehicleShouldTryNotToTurn, 0x421FE0);
    RH_ScopedInstall(TriggerDogFightMoves, 0x429300);
    RH_ScopedInstall(UpdateCarCount, 0x424000);
    RH_ScopedInstall(UpdateCarOnRails, 0x436540);
    RH_ScopedInstall(WeaveForObject, 0x426BC0);
    RH_ScopedInstall(WeaveForOtherCar, 0x426350);
    RH_ScopedInstall(WeaveForPed, 0x426970);
    RH_ScopedInstall(WeaveThroughCarsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x42D680);
    RH_ScopedInstall(WeaveThroughObjectsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x42D950);
    RH_ScopedInstall(WeaveThroughPedsSectorList<CPtrListDoubleLink<CPhysical*>>, 0x42D7E0);
}

// 0x4212E0
void CCarCtrl::Init() {
    ZoneScoped;

    CarDensityMultiplier = 1.0f;
    NumRandomCars = 0;
    NumLawEnforcerCars = 0;
    NumMissionCars = 0;
    NumParkedCars = 0;
    NumPermanentVehicles = 0;
    NumAmbulancesOnDuty = 0;
    NumFireTrucksOnDuty = 0;

    LastTimeAmbulanceCreated = 0;
    LastTimeFireTruckCreated = 0;
    bAllowEmergencyServicesToBeCreated = true;
    bCarsGeneratedAroundCamera = false;
    CountDownToCarsAtStart = 2;

    TimeNextMadDriverChaseCreated = CGeneral::GetRandomNumberInRange(600.0f, 1200.0f);

    std::ranges::fill(apCarsToKeep, nullptr);
    for (auto& group : CPopulation::m_LoadedGangCars) {
        group.Clear();
    }
    CPopulation::m_AppropriateLoadedCars.Clear();
    CPopulation::m_InAppropriateLoadedCars.Clear();
    CPopulation::m_LoadedBoats.Clear();
}

// 0x4213B0
void CCarCtrl::ReInit() {
    CarDensityMultiplier = 1.0f;
    NumRandomCars = 0;
    NumLawEnforcerCars = 0;
    NumMissionCars = 0;
    NumParkedCars = 0;
    NumPermanentVehicles = 0;
    NumAmbulancesOnDuty = 0;
    NumFireTrucksOnDuty = 0;

    LastTimeLawEnforcerCreated = 0;

    bAllowEmergencyServicesToBeCreated = true;
    CountDownToCarsAtStart = 2;

    std::ranges::fill(apCarsToKeep, nullptr);
    for (auto& group : CPopulation::m_LoadedGangCars) {
        group.Clear();
    }
    CPopulation::m_AppropriateLoadedCars.Clear();
    CPopulation::m_InAppropriateLoadedCars.Clear();
    CPopulation::m_LoadedBoats.Clear();
}

// 0x421970
int32 CCarCtrl::ChooseBoatModel() {
    return CPopulation::m_LoadedBoats.PickLeastUsedModel(1);
}

// 0x421900
int32 CCarCtrl::ChooseCarModelToLoad(int32 groupID) {
    const auto numCarsInGroup = CPopulation::m_nNumCarsInGroup[groupID];
    if (numCarsInGroup > 0) {
        for (auto i = 0; i < 16; i++) { // 16 tries
            const auto model = CPopulation::m_CarGroups[groupID][CGeneral::GetRandomNumberInRange(numCarsInGroup)];
            if (!CStreaming::IsModelLoaded(model)) {
                return model;
            }
        }
    }
    return -1;
}

eModelID CCarCtrl::ChooseGangCarModel(eGangID loadedCarGroupId) {
    return CPopulation::PickGangCar(loadedCarGroupId);
}

// 0x424CE0
int32 CCarCtrl::ChooseModel(int32* modelClass) {
    const int32 totalCars = (int32)(
        CPopCycle::m_NumOther_Cars +
        CPopCycle::m_NumCops_Cars +
        CPopCycle::m_NumGangs_Cars +
        CPopCycle::m_NumDealers_Cars
    );
    if (totalCars <= 0) {
        return -1;
    }

    if (bDebugRareCarCreation) {
        if ((int32)((float)CGeneral::GetRandomNumber() * (1.0f / 65536.0f) * 100.0f) != 0) {
            return -1;
        }
    }

    const float randomRoll  = (float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f);
    const float densityMult = 1.0f / (float)totalCars;

    //! Dealers
    if (CPopCycle::m_NumDealers_Cars * densityMult > randomRoll && !bForceGangCars) {
        *modelClass = 25;
        const auto model = CPopulation::m_CarGroups[POPCYCLE_CARGROUP_DEALERS][0];
        return CStreaming::ms_aInfoForModel[model].IsLoaded() ? model : -1;
    }

    //! Gangs
    if ((CPopCycle::m_NumGangs_Cars + CPopCycle::m_NumDealers_Cars) * densityMult > randomRoll || bForceGangCars) {
        if (CPopulation::m_bDontCreateRandomGangMembers) {
            return -1;
        }

        const CZoneInfo* const zoneInfo = CPopCycle::m_pCurrZoneInfo;

        int32 totalGangStrength = 0;
        for (const auto gangStrength : zoneInfo->GangStrength) {
            totalGangStrength += gangStrength;
        }
        if (totalGangStrength <= 0) {
            return -1;
        }

        int32 strengthLeft = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 65536.0f) * (float)totalGangStrength);

        int32 gang = 0;
        if (zoneInfo->GangStrength[0] < strengthLeft) {
            //! NOTE: Exactly as in the original - This loop isn't bounds checked
            do {
                strengthLeft -= zoneInfo->GangStrength[gang];
                gang++;
            } while (zoneInfo->GangStrength[gang] < strengthLeft);
        }

        if (bForceGangCars) {
            gang = CGeneral::GetRandomNumberInRange(0, TOTAL_GANGS - 1);
        }

        *modelClass = gang + 14;

        if ((int32)CPopulation::PickGangCar((eGangID)gang) < 0) {
            return -1;
        }

        for (auto tries = 0; tries < 10; tries++) {
            const auto index = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 65536.0f) * 23.0f);
            const auto model = CPopulation::m_CarGroups[POPCYCLE_CARGROUP_BALLAS + gang][index];
            if (model == 2000) { //!< Empty slot
                continue;
            }
            if (CStreaming::ms_aInfoForModel[model].IsLoaded()) {
                return model;
            }
        }
        return -1;
    }

    //! Police
    if ((CPopCycle::m_NumCops_Cars + CPopCycle::m_NumGangs_Cars + CPopCycle::m_NumDealers_Cars) * densityMult > randomRoll) {
        if (CGangWars::GangWarFightingGoingOn()) {
            return -1;
        }
        if (CPopulation::m_bDontCreateRandomCops) {
            return -1;
        }
        *modelClass = 13;
        return ChoosePoliceCarModel(0);
    }

    //! Civilians
    *modelClass = 0;
    if (CTheScripts::ForceRandomCarModel != -1) {
        return CTheScripts::ForceRandomCarModel;
    }
    return CPopulation::m_AppropriateLoadedCars.PickRandomCar(true, false);
}

int32 CCarCtrl::ChoosePoliceCarModel(uint32 ignoreCopBike) {
    CWanted* playerWanted = FindPlayerWanted();
    if (playerWanted->AreSwatRequired()
        && CStreaming::IsModelLoaded(MODEL_ENFORCER)
        && CStreaming::IsModelLoaded(MODEL_SWAT)
    ) {
        if (CGeneral::GetRandomNumberInRange(0, 3) == 2)
            return MODEL_ENFORCER;
    }
    else
    {
        if (playerWanted->AreFbiRequired()
            && CStreaming::IsModelLoaded(MODEL_FBIRANCH)
            && CStreaming::IsModelLoaded(MODEL_FBI))
            return MODEL_FBIRANCH;

        if (playerWanted->AreArmyRequired()
            && CStreaming::IsModelLoaded(MODEL_RHINO)
            && CStreaming::IsModelLoaded(MODEL_BARRACKS)
            && CStreaming::IsModelLoaded(MODEL_ARMY))
            return (CGeneral::GetRandomNumber() < 0x3FFF) + MODEL_RHINO;
    }
    return CStreaming::GetDefaultCopCarModel(ignoreCopBike);
}

// 0x423F00
void CCarCtrl::ClearInterestingVehicleList() {
    apCarsToKeep[0] = nullptr;
    apCarsToKeep[1] = nullptr;
}

// 0x422760
void CCarCtrl::ClitargetOrientationToLink(CVehicle* vehicle, CCarPathLinkAddress arg2, int8 arg3, float* arg4, float arg5, float arg6) {
    if (!ThePaths.m_pPathNodes[arg2.m_wAreaId]) {
        return;
    }

    const CCarPathLink& link = ThePaths.GetCarPathLink(arg2);

    const float dirX = (float)link.m_dir.x * (float)arg3;
    const float dirY = (float)link.m_dir.y * (float)arg3;

    const CVector pos = vehicle->GetPosition();

    CGeneral::GetATanOfXY(arg5 - pos.x, arg6 - pos.y); //!< Return value unused

    float laneA, laneB;
    if (link.m_numSameDirLanes == 0) {
        laneA = laneB = (float)link.m_numOppositeDirLanes * 0.5f;
    } else if (link.m_numOppositeDirLanes == 0) {
        laneA = laneB = (float)link.m_numSameDirLanes * 0.5f;
    } else {
        const uint8 laneAIdx = arg3 == 0 ? link.m_numOppositeDirLanes : link.m_numSameDirLanes;
        const uint8 laneBIdx = arg3 == 0 ? link.m_numSameDirLanes : link.m_numOppositeDirLanes;
        laneA = (float)laneAIdx + link.GetNodePathWidth() / 5.4f;
        laneB = (float)laneBIdx;
    }

    const float angle1 = CGeneral::GetATanOfXY(
        (dirY * (laneA - 0.3f) * 5.4f + arg5) - pos.x,
        (arg6 - (laneA - 0.3f) * dirX * 5.4f) - pos.y
    );
    const float angle2 = CGeneral::GetATanOfXY(
        (arg5 - (laneB - 0.3f) * dirY * 5.4f) - pos.x,
        ((laneB - 0.3f) * dirX * 5.4f + arg6) - pos.y
    );

    float d1 = angle1 - *arg4;
    float d2 = angle2 - *arg4;

    while (d1 > 3.1415927f)  d1 -= 6.2831855f;
    while (d1 < -3.1415927f) d1 += 6.2831855f;
    while (d2 > 3.1415927f)  d2 -= 6.2831855f;
    while (d2 < -3.1415927f) d2 += 6.2831855f;

    if (d1 >= 0.0f || d2 >= 0.0f) {
        if (d1 <= 0.0f || d2 <= 0.0f) { //!< Mixed signs (or a zero) - Don't change anything
            goto wrap;
        }
        if (d1 < d2) {
            d2 = d1;
        }
    } else if (d2 < d1) {
        *arg4 += d1;
        goto wrap;
    }
    *arg4 += d2;

wrap:
    while (*arg4 < 0.0f)         *arg4 += 6.2831855f;
    while (*arg4 > 6.2831855f)   *arg4 -= 6.2831855f;
}

// 0x431F80
CVehicle* CCarCtrl::CreateCarForScript(int32 modelid, CVector posn, bool doMissionCleanup) {
    if (CModelInfo::IsBoatModel(modelid))
    {
        auto* boat = new CBoat(modelid, eVehicleCreatedBy::MISSION_VEHICLE);
        if (posn.z <= MAP_Z_LOW_LIMIT)
            posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);

        posn.z += boat->GetDistanceFromCentreOfMassToBaseOfModel();
        boat->SetPosn(posn);

        CTheScripts::ClearSpaceForMissionEntity(posn, boat);
        boat->vehicleFlags.bEngineOn = false;
        boat->vehicleFlags.bIsLocked = true;
        boat->SetStatus(STATUS_ABANDONED);
        JoinCarWithRoadSystem(boat);

        boat->m_autoPilot.SetCarMission(eCarMission::MISSION_NONE);
        boat->m_autoPilot.m_nTempAction = TEMPACT_NONE;
        boat->m_autoPilot.m_speed = 20.0F;
        boat->m_autoPilot.SetCruiseSpeed(20);

        if (doMissionCleanup)
            boat->m_bIsStaticWaitingForCollision = true;

        boat->m_autoPilot.movementFlags.bIsStopped = true;
        CWorld::Add(boat);

        if (doMissionCleanup)
            CTheScripts::MissionCleanUp.AddEntityToList(GetVehiclePool()->GetRef(boat), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);

        return boat;
    }

    auto* vehicle = GetNewVehicleDependingOnCarModel(modelid, eVehicleCreatedBy::MISSION_VEHICLE);
    if (posn.z <= MAP_Z_LOW_LIMIT)
        posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);

    posn.z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
    vehicle->SetPosn(posn);

    if (!doMissionCleanup)
    {
        if (vehicle->IsAutomobile())
            vehicle->AsAutomobile()->PlaceOnRoadProperly();
        else if (vehicle->IsBike())
            vehicle->AsBike()->PlaceOnRoadProperly();
    }

    if (vehicle->IsTrain())
        vehicle->AsTrain()->trainFlags.bNotOnARailRoad = true;

    CTheScripts::ClearSpaceForMissionEntity(posn, vehicle);
    vehicle->vehicleFlags.bIsLocked = true;
    vehicle->SetStatus(STATUS_ABANDONED);
    JoinCarWithRoadSystem(vehicle);
    vehicle->vehicleFlags.bEngineOn = false;
    vehicle->vehicleFlags.bHasBeenOwnedByPlayer = true;

    vehicle->m_autoPilot.SetCarMission(eCarMission::MISSION_NONE);
    vehicle->m_autoPilot.m_nTempAction = TEMPACT_NONE;
    vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    vehicle->m_autoPilot.m_speed = 13.0F;
    vehicle->m_autoPilot.SetCruiseSpeed(13);
    vehicle->m_autoPilot.m_nCurrentLane = 0;
    vehicle->m_autoPilot.m_nNextLane = 0;

    if (doMissionCleanup)
        vehicle->m_bIsStaticWaitingForCollision = true;

    CWorld::Add(vehicle);
    if (doMissionCleanup)
        CTheScripts::MissionCleanUp.AddEntityToList(GetVehiclePool()->GetRef(vehicle), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);

    if (vehicle->IsSubRoadVehicle())
        vehicle->m_autoPilot.movementFlags.bIsStopped = true;

    return vehicle;
}

// 0x42C740
bool CCarCtrl::CreateConvoy(CVehicle* vehicle, int32 arg2) {
    //! 2 followers, or 3 about half of the time
    const int32 numFollowers = ((int32)CGeneral::GetRandomNumberInRange(0.0f, 100.0f) > 50) + 2;

    const float radius = vehicle->GetColModel()->GetBoundRadius();

    //! Make the convoy wander deterministically
    vehicle->m_nForcedRandomRouteSeed = vehicle->m_nRandomSeed;

    CVehicle* prev    = vehicle;
    bool      created = false;

    for (int32 i = 0; i < numFollowers; i++) {
        CVehicle* const car = GetNewVehicleDependingOnCarModel(vehicle->m_nModelIndex, RANDOM_VEHICLE);
        if (!car) {
            continue;
        }

        //! Spawn `i + 1` car-widths behind the leader
        const CVector leaderFwd = vehicle->GetForward();
        const CVector leaderPos = vehicle->GetPosition();
        const float   spacing   = (float)(i + 1) * (radius + radius + 2.5f);

        CVector pos{
            leaderPos.x - leaderFwd.x * spacing,
            leaderPos.y - leaderFwd.y * spacing,
            leaderPos.z - leaderFwd.z * spacing
        };

        //! Drop onto the ground
        float     groundZ = 1e9f;
        CColPoint colPoint{};
        CEntity*  hitEntity{};
        if (CWorld::ProcessVerticalLine(pos, 1000.0f, colPoint, hitEntity, true, false, false, false, true, false, nullptr)) {
            groundZ = colPoint.m_vecPoint.z;
        }
        if (CWorld::ProcessVerticalLine(pos, -1000.0f, colPoint, hitEntity, true, false, false, false, true, false, nullptr)) {
            if (std::abs(colPoint.m_vecPoint.z - pos.z) < std::abs(groundZ - pos.z)) {
                groundZ = colPoint.m_vecPoint.z;
            }
        }
        if (groundZ > 1e9f) {
            delete car;
            continue;
        }
        pos.z = groundZ + car->GetDistanceFromCentreOfMassToBaseOfModel();

        //! Spot must be clear
        int16 numFound = 0;
        CWorld::FindObjectsKindaColliding(pos, radius, true, &numFound, 2, nullptr, false, false, true, true, false);
        if (numFound != 0) {
            delete car;
            continue;
        }

        //! The leader is only set up once, when the first follower was created
        if (!created) {
            vehicle->SetStatus(STATUS_PHYSICS);
            vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
            vehicle->m_vecMoveSpeed = vehicle->GetForward() * ((float)vehicle->m_autoPilot.m_nCruiseSpeed * 0.02f);
        }

        car->GetMatrix() = vehicle->GetMatrix();
        car->SetPosn(pos);
        car->m_vecMoveSpeed = vehicle->m_vecMoveSpeed;
        car->SetStatus(vehicle->GetStatus());
        car->vehicleFlags.bPartOfConvoy = true;
        CWorld::Add(car);
        if (car->m_nVehicleType == VEHICLE_TYPE_BIKE) {
            car->AsBike()->PlaceOnRoadProperly();
        } else if (car->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
            car->AsAutomobile()->PlaceOnRoadProperly();
        }

        SetUpDriverAndPassengersForVehicle(car, arg2, 0, true, false, 99);

        car->m_pDriver->GetIntelligence()->m_TaskMgr.SetTask(
            new CTaskComplexCarDriveWander(car, DRIVING_STYLE_STOP_FOR_CARS, 10.0f),
            TASK_PRIMARY_DEFAULT,
            false
        );

        //! Inherit the leader's driving state
        car->m_autoPilot.m_nCruiseSpeed = vehicle->m_autoPilot.m_nCruiseSpeed;
        car->m_autoPilot.m_speed        = vehicle->m_autoPilot.m_speed - (float)i;
        if (car->m_autoPilot.m_speed < 0.0f) {
            car->m_autoPilot.m_speed = 0.0f;
        }
        car->m_autoPilot.m_nCarDrivingStyle = vehicle->m_autoPilot.m_nCarDrivingStyle;
        car->m_autoPilot.m_nCarMission      = vehicle->m_autoPilot.m_nCarMission;
        car->m_nRandomSeed                  = vehicle->m_nRandomSeed;
        car->m_nForcedRandomRouteSeed       = vehicle->m_nForcedRandomRouteSeed;

        //! Link the convoy backwards
        car->m_autoPilot.m_TargetEntity = prev;
        CEntity::RegisterReference(car->m_autoPilot.m_TargetEntity);

        created = true;
        prev    = car;
    }
    return created;
}

// 0x42C2B0
bool CCarCtrl::CreatePoliceChase(CVehicle* vehicle, int32 arg2, CNodeAddress nodeAddress) {
    if (CPopCycle::m_NumGangs_Cars != 0.0f && (arg2 < 14 || arg2 > 23)) {
        //! 50% chance to not start a chase at all
        if ((int32)CGeneral::GetRandomNumberInRange(0.0f, 4.0f) != 0) {
            return false;
        }
    }

    const int32 modelId = ChoosePoliceCarModel(true);
    if (modelId < 0) {
        return false;
    }
    if (CStreaming::ms_aInfoForModel[modelId].m_LoadState != LOADSTATE_LOADED) {
        return false;
    }

    CVehicle* const cop = GetNewVehicleDependingOnCarModel(modelId, RANDOM_VEHICLE);
    if (!cop) {
        return false;
    }

    //! Spawn at the node, dropped onto the ground
    CVector pos = ThePaths.GetPathNode(nodeAddress)->GetPosition();

    float     groundZ = 1e9f;
    CColPoint colPoint{};
    CEntity*  hitEntity{};
    if (CWorld::ProcessVerticalLine(pos, 1000.0f, colPoint, hitEntity, true, false, false, false, true, false, nullptr)) {
        groundZ = colPoint.m_vecPoint.z;
    }
    if (CWorld::ProcessVerticalLine(pos, -1000.0f, colPoint, hitEntity, true, false, false, false, true, false, nullptr)) {
        if (std::abs(colPoint.m_vecPoint.z - pos.z) < std::abs(groundZ - pos.z)) {
            groundZ = colPoint.m_vecPoint.z;
        }
    }
    if (groundZ > 1e9f) {
        delete cop;
        return false;
    }

    pos.z = groundZ + cop->GetDistanceFromCentreOfMassToBaseOfModel();

    const float radius = cop->GetColModel()->GetBoundRadius();

    //! Don't spawn it in view
    int16 numFound = 0;
    if (!cop->GetIsOnScreen() && TheCamera.IsSphereVisible(pos, radius)) {
        numFound = 1;
        delete cop;
    } else {
        CWorld::FindObjectsKindaColliding(pos, radius, true, &numFound, 2, nullptr, false, false, true, true, false);
    }
    if (numFound != 0) {
        return false;
    }

    //! Set up the fleeing car
    vehicle->SetStatus(STATUS_PHYSICS);
    vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
    vehicle->vehicleFlags.bMadDriver        = true;
    vehicle->m_autoPilot.m_nCruiseSpeed     = (uint8)((float)vehicle->m_autoPilot.m_nCruiseSpeed + 10.0f);
    vehicle->m_vecMoveSpeed = vehicle->GetForward() * ((float)vehicle->m_autoPilot.m_nCruiseSpeed * 0.02f);
    SetUpDriverAndPassengersForVehicle(vehicle, arg2, 2, true, true, 99);

    //! Set up the cop car
    cop->GetMatrix() = vehicle->GetMatrix();
    cop->SetPosn(pos);
    cop->m_vecMoveSpeed = vehicle->m_vecMoveSpeed;
    cop->SetStatus(STATUS_PHYSICS);
    cop->vehicleFlags.bCreatedAsPoliceVehicle = true;
    cop->ChangeLawEnforcerState(true);
    CWorld::Add(cop);
    if (cop->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        cop->AsBike()->PlaceOnRoadProperly();
    } else {
        cop->AsAutomobile()->PlaceOnRoadProperly();
    }
    CCarAI::AddPoliceCarOccupants(cop, true);

    //! The fleeing driver and its passengers must not be removed
    if (vehicle->m_pDriver) {
        vehicle->m_pDriver->m_nPhysicalFlags |= PHYSICAL_b12;
    }
    for (auto i = 0; i < vehicle->m_nMaxPassengers; i++) {
        if (vehicle->m_apPassengers[i]) {
            vehicle->m_apPassengers[i]->m_nPhysicalFlags |= PHYSICAL_b12;
        }
    }

    //! Make the cop chase the fleeing car
    cop->m_pDriver->GetIntelligence()->m_TaskMgr.SetTask(
        new CTaskComplexCarDriveWander(cop, DRIVING_STYLE_STOP_FOR_CARS, 10.0f),
        TASK_PRIMARY_DEFAULT,
        false
    );

    if (CPed* const driver = vehicle->m_pDriver) {
        CEventAcquaintancePedHate event{ driver };
        driver->GetEventGroup().Add(event, false);
    }

    //! Reset the bomb/lights/rope bitfields
    cop->m_nBombOnBoard         = 7;
    cop->m_nOverrideLights      = 3;
    cop->m_ropeType             = 3;
    cop->m_nGunsCycleIndex      = 3;
    cop->m_nOrdnanceCycleIndex  = 3;

    cop->vehicleFlags.bNeverUseSmallerRemovalRange = true;
    return true;
}

// 0x428040
bool CCarCtrl::DealWithBend_Racing(CVehicle* vehicle, CCarPathLinkAddress LinkAddress1, CCarPathLinkAddress LinkAddress2, CCarPathLinkAddress LinkAddress3, CCarPathLinkAddress LinkAddress4, char arg6, char arg7, char arg8, char arg9, float arg10, float* arg11, float* arg12, float* arg13, float* arg14, CVector* pos) {
    const auto Raw = [](const CCarPathLinkAddress& addr) -> uint16 {
        return *reinterpret_cast<const uint16*>(&addr);
    };
    const auto SqDist = [](const CCarPathLink& a, const CCarPathLink& b) {
        const float dx = (a.m_posn.x - b.m_posn.x) * 0.125f;
        const float dy = (a.m_posn.y - b.m_posn.y) * 0.125f;
        return dx * dx + dy * dy;
    };

    if (Raw(LinkAddress1) == 0xFFFF || Raw(LinkAddress2) == 0xFFFF) {
        return false;
    }

    float minAngle, midAngle, maxDist;
    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
        minAngle = 0.34906587f; //!< 20 degrees
        midAngle = 0.69813174f; //!< 40 degrees
        maxDist  = 50.0f;
    } else {
        minAngle = 0.5235988f; //!< 30 degrees
        midAngle = 1.0471976f; //!< 60 degrees
        maxDist  = 40.0f;
    }

    *arg12 = 1.0f; //!< Speed multiplier

    if (!ThePaths.m_pPathNodes[Raw(LinkAddress1) >> 10] || !ThePaths.m_pPathNodes[Raw(LinkAddress2) >> 10]) {
        return false;
    }

    const CCarPathLink* l1 = &ThePaths.m_pNaviNodes[Raw(LinkAddress1) >> 10][Raw(LinkAddress1) & 0x3FF];
    const CCarPathLink* l2 = &ThePaths.m_pNaviNodes[Raw(LinkAddress2) >> 10][Raw(LinkAddress2) & 0x3FF];

    const CVector2D l1Pos  = l1->GetNodeCoors();
    const CVector   vehPos = vehicle->GetPosition();

    const float dist = std::sqrt((l1Pos.x - vehPos.x) * (l1Pos.x - vehPos.x) + (l1Pos.y - vehPos.y) * (l1Pos.y - vehPos.y));
    *arg14 = dist;
    if (dist > 60.0f) {
        return false;
    }

    //! Pick the far end of the bend: prefer `LinkAddress3`'s link, then `LinkAddress4`'s,
    //! each only if it is within 10m of the current second link
    char laneOfs2 = arg7;
    if (Raw(LinkAddress3) != 0xFFFF) {
        const CCarPathLink* l3 = &ThePaths.m_pNaviNodes[Raw(LinkAddress3) >> 10][Raw(LinkAddress3) & 0x3FF];
        if (SqDist(*l2, *l3) < 100.0f) {
            l2       = l3;
            laneOfs2 = arg8;
        }
        if (Raw(LinkAddress4) != 0xFFFF) {
            const CCarPathLink* l4 = &ThePaths.m_pNaviNodes[Raw(LinkAddress4) >> 10][Raw(LinkAddress4) & 0x3FF];
            if (SqDist(*l2, *l4) < 100.0f) {
                l2       = l4;
                laneOfs2 = arg9;
            }
        }
    }

    const float dir1x = (float)l1->m_dir.x * 0.01f * (float)arg6;
    const float dir1y = (float)l1->m_dir.y * 0.01f * (float)arg6;
    const float dir2x = (float)l2->m_dir.x * 0.01f * (float)laneOfs2;
    const float dir2y = (float)l2->m_dir.y * 0.01f * (float)laneOfs2;

    //! How much the vehicle would have to turn to follow the second link
    const CVector vehFwd      = vehicle->GetForward();
    const float   vehHeading  = CGeneral::GetATanOfXY(vehFwd.x, vehFwd.y);
    const float   link2Heading = CGeneral::GetATanOfXY(dir2x, dir2y);
    const float   angleDiff   = CGeneral::LimitRadianAngle(vehHeading - link2Heading);

    if (std::abs(angleDiff) < minAngle) {
        return false;
    }

    const int32 lanes1 = l1->m_numSameDirLanes + l1->m_numOppositeDirLanes;
    const int32 lanes2 = l2->m_numSameDirLanes + l2->m_numOppositeDirLanes;
    if (lanes1 > 3 && lanes2 > 3 && std::abs(angleDiff) < midAngle) {
        return false;
    }

    *arg11 = angleDiff;

    //! Lane center points of both links
    //! @todo(re) The exact pairing of these points (and the 11m clamp) in the original was only partly recovered (0x4285..-0x4287..)
    const CVector2D l2Pos = l2->GetNodeCoors();
    const float     off1  = l1->OneWayLaneOffset();
    const float     off2  = l2->OneWayLaneOffset();

    const auto LanePoint = [](const CVector2D& p, float dirX, float dirY, float k) {
        return CVector2D{p.x + dirX * k, p.y + dirY * k};
    };

    const float k1A = ((float)l1->m_numSameDirLanes - 1.0f + off1) * 5.4f;
    const float k1B = ((float)l1->m_numOppositeDirLanes - 1.0f + off1) * 5.4f;
    const float k2A = ((float)l2->m_numSameDirLanes - 1.0f + off2) * 5.4f;
    const float k2B = ((float)l2->m_numOppositeDirLanes - 1.0f + off2) * 5.4f;

    const CVector2D pA1 = LanePoint(l1Pos, dir1x, dir1y,  k1A);
    const CVector2D pB1 = LanePoint(l1Pos, dir1x, dir1y, -k1B);
    const CVector2D pA2 = LanePoint(l2Pos, dir2x, dir2y,  k2A);
    const CVector2D pB2 = LanePoint(l2Pos, dir2x, dir2y, -k2B);

    const auto SqDist2D = [](const CVector2D& a, const CVector2D& b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    };

    CVector2D near2 = pA2;
    if (SqDist2D(pB1, pA2) < SqDist2D(pA1, pA2)) {
        //! Keep the other end of link 1
        const float dx = pB1.x - pA2.x;
        const float dy = pB1.y - pA2.y;
        const float segmentLen = std::sqrt(dx * dx + dy * dy);
        const float k = segmentLen > 11.0f ? 11.0f / segmentLen : 1.0f;
        near2 = CVector2D{pA2.x + dx * k, pA2.y + dy * k};
    } else {
        const float dx = pA1.x - pA2.x;
        const float dy = pA1.y - pA2.y;
        const float segmentLen = std::sqrt(dx * dx + dy * dy);
        const float k = segmentLen > 11.0f ? 11.0f / segmentLen : 1.0f;
        near2 = CVector2D{pA2.x + dx * k, pA2.y + dy * k};
    }
    if (SqDist2D(pB2, pA1) < SqDist2D(pA2, pA1)) {
        near2 = pB2;
    }
    const CVector2D near1 = SqDist2D(pB1, near2) < SqDist2D(pA1, near2) ? pB1 : pA1;

    FindIntersection2Lines(near1, CVector2D{dir1x, dir1y}, near2, CVector2D{dir2x, dir2y}, &pos->x, &pos->y);

    //! Speed multiplier, scaled by how close the car is to the bend and by the bend's angle
    if (*arg14 < maxDist) {
        const float a = std::min(std::abs(angleDiff) * 0.63661975f, 1.0f); //!< 2/PI
        *arg12 = 1.0f - ((maxDist - *arg14) / maxDist) * a * 0.6f;
    }

    //! Heading: aim at the near point, unless the turn is already severe at this distance
    {
        const float t = std::clamp(arg10 * 0.033333335f, 0.2f, 1.0f);
        const float a = std::min(std::abs(angleDiff) * 0.63661975f, 1.0f);
        if (t * a * 20.0f <= *arg14) {
            *arg13 = CGeneral::GetATanOfXY(near1.x - vehPos.x, near1.y - vehPos.y);
        } else {
            *arg13 = CGeneral::GetATanOfXY(vehPos.x - pos->x, pos->y - vehPos.y);
        }
    }

    return true;
}

// 0x42EC90
void CCarCtrl::DragCarToPoint(CVehicle* vehicle, CVector* pos) {
    const CColModel* const colModel = vehicle->GetColModel();

    //! Chassis width (Typically along the y axis of the collision model)
    const float len = (colModel->m_boundBox.m_vecMax.y - colModel->m_boundBox.m_vecMin.y) * 0.95f;

    float marginBig   = 3.0f;
    float marginSmall = 3.0f;
    if (vehicle->m_autoPilot.field_51 > 16) {
        //! Recovery mode: Accept (almost) any ground
        marginBig = marginSmall = 100.0f;
    }

    const CVector fwd    = vehicle->GetForward();
    const CVector carPos = vehicle->GetPosition();

    //! Rear anchor, half a chassis length behind the centre
    const CVector from{
        carPos.x - fwd.x * len * 0.5f,
        carPos.y - fwd.y * len * 0.5f,
        carPos.z
    };

    //! Where the target sphere meets the car's rear anchor line
    CVector contact;
    {
        const CColLine   line{ from, *pos };
        const CColSphere sphere{ *pos, len };
        CColPoint        colPoint{};
        float            depth = 1.0f;
        if (CCollision::ProcessLineSphere(line, sphere, colPoint, depth)) {
            contact = colPoint.m_vecPoint;
        } else {
            const float dx = from.x - pos->x;
            const float dy = from.y - pos->y;
            const float k  = len / std::sqrt(dx * dx + dy * dy);
            contact = CVector{ dx * k + pos->x, dy * k + pos->y, 0.0f };
        }
    }

    //! Ground height under one end of the car. Returns `false` if nothing was found.
    const auto GroundZUnder = [&](const CVector& point, float zBase, CStoredCollPoly* storedPoly, float& outZ) {
        const float minDist = zBase - marginSmall;

        CColPoint colPoint{};
        CEntity*  hitEntity{};

        if (CCollision::IsStoredPolyStillValidVerticalLine(
                CVector{ point.x, point.y, zBase + marginBig }, minDist, colPoint, storedPoly)
        ) {
            outZ = colPoint.m_vecPoint.z;
            return true;
        }

        if (CWorld::ProcessVerticalLine(
                CVector{ point.x, point.y, zBase + 1.5f }, zBase - 2.0f,
                colPoint, hitEntity, true, false, false, false, false, false, storedPoly)
            || CWorld::ProcessVerticalLine(
                CVector{ point.x, point.y, zBase + marginBig }, minDist,
                colPoint, hitEntity, true, false, false, false, false, false, storedPoly)
        ) {
            //! We're standing on something new - Inherit its tunnel flags
            vehicle->m_pEntityWeAreOn = hitEntity;
            if (hitEntity) {
                vehicle->m_nFlags = (vehicle->m_nFlags & 0x3FFFFFFF) | (hitEntity->m_nFlags & 0xC0000000);
            }
            vehicle->m_autoPilot.field_51 = 0;
            outZ = colPoint.m_vecPoint.z;
            return true;
        }

        return false;
    };

    //! Front end (Under the target point)
    float zFront;
    vehicle->m_autoPilot.field_51++;
    if (!GroundZUnder(*pos, carPos.z + fwd.z * len * 0.5f, &vehicle->m_FrontCollPoly, zFront)) {
        zFront = vehicle->m_fVehicleFrontGroundZ;
    }
    vehicle->m_fVehicleFrontGroundZ = zFront;

    //! Rear end (Under the contact point)
    float zRear;
    if (!GroundZUnder(contact, carPos.z - fwd.z * len * 0.5f, &vehicle->m_RearCollPoly, zRear)) {
        zRear = vehicle->m_fVehicleRearGroundZ;
    }
    vehicle->m_fVehicleRearGroundZ = zRear;

    //! Orient along the ground slope
    CMatrix& m = vehicle->GetMatrix();

    const float invLen = 1.0f / len;
    m.GetRight() = CVector{
        (pos->y - contact.y) * invLen,
        -(pos->x - contact.x) * invLen,
        0.0f
    };

    const float pitch = std::atan2(zFront - zRear, len);
    const float cosP  = std::cos(pitch);
    m.GetForward() = CVector{
        -(cosP * m.GetRight().y),
        cosP * m.GetRight().x,
        std::sin(pitch)
    };
    m.GetUp() = CrossProduct(m.GetRight(), m.GetForward());

    vehicle->UpdateLightingFromStoredPolys();

    vehicle->SetPosn(
        (pos->x + contact.x) * 0.5f,
        (pos->y + contact.y) * 0.5f,
        (zFront + zRear) * 0.5f + vehicle->GetDistanceFromCentreOfMassToBaseOfModel()
    );

    //! Roll (Stored into the steer angle slot)
    const CVector rollVec = CrossProduct(m.GetForward(), fwd);
    const float   roll    = rollVec.z * -10.0f;
    if (roll < 0.5f) {
        vehicle->m_fSteerAngle = roll > -0.5f ? roll : -8.0f;
    } else {
        vehicle->m_fSteerAngle = 0.5f;
    }
}

// 0x4325C0
float CCarCtrl::FindAngleToWeaveThroughTraffic(CVehicle* vehicle, CPhysical* physical, float angleMin, float angleMax, float arg5) {
    //! How far to the sides to look (Scaled by how fast we're going)
    float spread = std::min(2.0f, std::sqrt(vehicle->m_vecMoveSpeed.x * vehicle->m_vecMoveSpeed.x + vehicle->m_vecMoveSpeed.y * vehicle->m_vecMoveSpeed.y) * 2.5f + 1.0f) * arg5 * 12.0f;

    const CVector vehPos = vehicle->GetPosition();
    const float   minX   = vehPos.x - spread;
    const float   maxX   = vehPos.x + spread;
    const float   minY   = vehPos.y - spread;
    const float   maxY   = vehPos.y + spread;

    //! Sector range covered by the scan
    const auto ToSectorIndex = [](float coord) {
        return (int32)std::floor(coord * 0.02f + 60.0f);
    };
    const int32 minXIdx = std::max(0, ToSectorIndex(minX));
    const int32 maxXIdx = std::min(0x77, ToSectorIndex(maxX));
    const int32 minYIdx = std::max(0, ToSectorIndex(minY));
    const int32 maxYIdx = std::min(0x77, ToSectorIndex(maxY));

    CWorld::AdvanceCurrentScanCode();

    float prevMax = -9999.9f;
    float prevMin = -9999.9f;
    do {
        prevMax = angleMax;
        prevMin = angleMin;

        for (int32 y = minYIdx; y <= maxYIdx; y++) {
            for (int32 x = minXIdx; x <= maxXIdx; x++) {
                CRepeatSector& sector = CWorld::ms_aRepeatSectors[y & 0xF][x & 0xF];

                //! The lists are typed per class, but the helpers take a `CPhysical` list - The layouts are identical
                auto& vehicles = reinterpret_cast<CPtrListDoubleLink<CPhysical*>&>(sector.Vehicles);
                auto& peds     = reinterpret_cast<CPtrListDoubleLink<CPhysical*>&>(sector.Peds);
                auto& objects  = reinterpret_cast<CPtrListDoubleLink<CPhysical*>&>(sector.Objects);

                WeaveThroughCarsSectorList(vehicles, vehicle, physical, minX, minY, maxX, maxY, &angleMax, &angleMin);

                if (vehicle->m_autoPilot.m_nCarDrivingStyle != (eCarDrivingStyle)6) {
                    WeaveThroughPedsSectorList(peds, vehicle, physical, minX, minY, maxX, maxY, &angleMax, &angleMin);
                }

                WeaveThroughObjectsSectorList(objects, vehicle, minX, minY, maxX, maxY, &angleMax, &angleMin);

                //! NOTE: Exactly as in the original - The cars are scanned twice
                WeaveThroughCarsSectorList(vehicles, vehicle, physical, minX, minY, maxX, maxY, &angleMax, &angleMin);
            }
        }
    } while (prevMax != angleMax || prevMin != angleMin);

    //! Pick the best of the two angles found
    const float delta = CGeneral::LimitRadianAngle(angleMax - angleMin);
    const float mid   = CGeneral::LimitRadianAngle(angleMin + delta * 0.5f);

    const float a = std::abs(CGeneral::LimitRadianAngle(angleMax - mid));
    const float b = std::abs(CGeneral::LimitRadianAngle(angleMin - mid));

    if (a > HALF_PI && b > HALF_PI) {
        return mid;
    }
    if (std::abs(a - b) >= 0.08f && a < b) {
        return angleMax;
    }
    return angleMin;
}

// 0x4226F0
void CCarCtrl::FindIntersection2Lines(CVector2D point1, CVector2D dir1, CVector2D point2, CVector2D dir2, float* outX, float* outY) {
    const auto det = dir1.Cross(dir2);
    const auto t   = det != 0.0f
        ? (point2 - point1).Cross(dir2) / det
        : 0.0f;
    *outX = point1.x + dir1.x * t;
    *outY = point1.y + dir1.y * t;
}

//! Compare two node addresses the way the original does (As one packed 32bit value)
static bool IsNodeAddressLess(CNodeAddress a, CNodeAddress b) {
    const uint32 packedA = (uint32)a.m_wAreaId | ((uint32)a.m_wNodeId << 16);
    const uint32 packedB = (uint32)b.m_wAreaId | ((uint32)b.m_wNodeId << 16);
    return packedA < packedB;
}

// 0x42B470
void CCarCtrl::FindLinksToGoWithTheseNodes(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    if (vehicle->m_nForcedRandomRouteSeed) {
        srand(vehicle->m_nForcedRandomRouteSeed);
    }

    const CNodeAddress cur     = autoPilot.m_currentAddress;
    const CNodeAddress desired = autoPilot.m_startingRouteNode;

    CPathNode* const node = ThePaths.GetPathNode(cur);

    //! Find the connection leading to the desired node (Bounded, as in the original)
    int16 linkIdx = 0;
    do {
        if (ThePaths.m_pNodeLinks[cur.m_wAreaId][node->m_wBaseLinkId + linkIdx] == desired) {
            break;
        }
    } while (++linkIdx < 12);

    autoPilot.m_nNextPathNodeInfo = ThePaths.m_pNaviLinks[cur.m_wAreaId][node->m_wBaseLinkId + linkIdx];
    autoPilot._smthNext           = IsNodeAddressLess(cur, desired) ? -1 : 1;

    //! Choose the connection to actually take
    int32 idx;
    if (node->m_nNumLinks == 1 || node->m_nNumLinks == 0) {
        idx = 0;
    } else {
        const CVector curNodePos = node->GetPosition();
        const CVector vehPos     = vehicle->GetPosition();

        float best = 999999.9f;
        idx        = -1;
        for (int32 i = 0; i < (int32)node->m_nNumLinks; i++) {
            const CNodeAddress cand = ThePaths.m_pNodeLinks[cur.m_wAreaId][node->m_wBaseLinkId + i];
            if (cand == desired || !ThePaths.m_pPathNodes[cand.m_wAreaId]) {
                continue;
            }
            const CVector candPos = ThePaths.GetPathNode(cand)->GetPosition();
            const float   dist    = CCollision::DistToLine(curNodePos, candPos, vehPos);
            if (dist < best) {
                best = dist;
                idx  = i;
            }
        }
        if (idx < 0) {
            idx = 0;
        }
    }

    const CNodeAddress chosen = ThePaths.m_pNodeLinks[cur.m_wAreaId][node->m_wBaseLinkId + idx];
    autoPilot.m_nCurrentPathNodeInfo = ThePaths.m_pNaviLinks[cur.m_wAreaId][node->m_wBaseLinkId + idx];
    autoPilot._smthCurr              = IsNodeAddressLess(chosen, cur) ? -1 : 1;
}

// 0x42BD20
void CCarCtrl::FindNodesThisCarIsNearestTo(CVehicle* vehicle, CNodeAddress& nodeAddress1, CNodeAddress& nodeAddress2) {
    nodeAddress1 = nodeAddress2 = CNodeAddress{ (uint16)-1, (uint16)-1 };

    float best = 99999.9f;

    const CVector pos = vehicle->GetPosition();

    //! Region window around the car
    int32 x0 = (int32)ThePaths.FindXRegionForCoors(pos.x);
    int32 y0 = (int32)ThePaths.FindYRegionForCoors(pos.y);
    int32 x1 = x0;
    int32 y1 = y0;
    {
        const float fx = pos.x - ThePaths.FindXCoorsForRegion(x0);
        const float fy = pos.y - ThePaths.FindYCoorsForRegion(y0);
        if (fx < 200.0f) x0--;
        if (fy < 200.0f) y0--;
        if (fx > 550.0f) x1++;
        if (fy > 550.0f) y1++;
        x0 = std::max(x0, 0);
        y0 = std::max(y0, 0);
        x1 = std::min(x1, (int32)NUM_PATH_MAP_AREA_X - 1);
        y1 = std::min(y1, (int32)NUM_PATH_MAP_AREA_Y - 1);
    }

    if (x1 < x0) {
        return;
    }

    for (int32 x = x0; x <= x1; x++) {
        for (int32 y = y0, areaId = x + y0 * NUM_PATH_MAP_AREA_X; y <= y1; y++, areaId += NUM_PATH_MAP_AREA_X) {
            const CPathNode* const nodes = ThePaths.m_pPathNodes[areaId];
            if (!nodes || ThePaths.m_anNumVehicleNodes[areaId] == 0) {
                continue;
            }

            for (int32 i = 0; i < (int32)ThePaths.m_anNumVehicleNodes[areaId]; i++) {
                const CPathNode* const node = &nodes[i];

                const CVector nodePos = node->GetPosition();
                { //! Only nodes close by are considered
                    const CVector delta = nodePos - pos;
                    if (delta.Magnitude() >= 150.0f) {
                        continue;
                    }
                }

                //! NOTE: The z axis is exaggerated to favour links on our level
                const CVector lineStart{ nodePos.x, nodePos.y, nodePos.z * 3.0f };
                const CVector point{ pos.x, pos.y, pos.z * 3.0f };

                for (int32 k = 0; k < (int32)node->m_nNumLinks; k++) {
                    const CNodeAddress cand = ThePaths.m_pNodeLinks[areaId][node->m_wBaseLinkId + k];
                    if (!ThePaths.m_pPathNodes[cand.m_wAreaId]) {
                        continue;
                    }

                    const CVector candPos = ThePaths.GetPathNode(cand)->GetPosition();
                    const CVector lineEnd{ candPos.x, candPos.y, candPos.z * 3.0f };

                    const float dist = CCollision::DistToLine(lineStart, lineEnd, point);

                    //! Links aligned with our heading score better
                    CVector dir = lineEnd - lineStart;
                    dir.Normalise();
                    const float score = (1.0f - DotProduct(dir, vehicle->GetForward())) * 5.0f + dist;

                    if (score < best) {
                        best         = score;
                        nodeAddress1 = CNodeAddress{ (uint16)areaId, (uint16)i };
                        nodeAddress2 = cand;
                    }
                }
            }
        }
    }
}

// 0x434400
float CCarCtrl::FindMaximumSpeedForThisCarInTraffic(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    const float baseSpeed = (float)autoPilot.m_nCruiseSpeed * autoPilot.m_SpeedMult;

    //! These styles don't care about the traffic
    switch ((int32)autoPilot.m_nCarDrivingStyle) {
    case 2:
    case 3:
    case 5:
        return baseSpeed;
    default:
        break;
    }

    const CVector vehPos = vehicle->GetPosition();
    const float   minX   = vehPos.x - 14.0f;
    const float   maxX   = vehPos.x + 14.0f;
    const float   minY   = vehPos.y - 14.0f;
    const float   maxY   = vehPos.y + 14.0f;

    //! Sector range covered by the scan
    const auto ToSectorIndex = [](float coord) {
        return (int32)std::floor(coord * 0.02f + 60.0f);
    };
    const int32 minXIdx = std::max(0, ToSectorIndex(minX));
    const int32 maxXIdx = std::min(0x77, ToSectorIndex(maxX));
    const int32 minYIdx = std::max(0, ToSectorIndex(minY));
    const int32 maxYIdx = std::min(0x77, ToSectorIndex(maxY));

    CWorld::AdvanceCurrentScanCode();

    float speed = baseSpeed;
    for (int32 y = minYIdx; y <= maxYIdx; y++) {
        for (int32 x = minXIdx; x <= maxXIdx; x++) {
            CRepeatSector& sector = CWorld::ms_aRepeatSectors[y & 0xF][x & 0xF];

            //! The lists are typed per class, but the helpers take a `CPhysical` list - The layouts are identical
            auto& vehicles = reinterpret_cast<CPtrListDoubleLink<CPhysical*>&>(sector.Vehicles);
            auto& peds     = reinterpret_cast<CPtrListDoubleLink<CPhysical*>&>(sector.Peds);
            auto& objects  = reinterpret_cast<CPtrListDoubleLink<CPhysical*>&>(sector.Objects);

            if (autoPilot.m_nCarDrivingStyle != (eCarDrivingStyle)6) {
                SlowCarDownForCarsSectorList(vehicles, vehicle, minX, minY, maxX, maxY, &speed, baseSpeed);
            }
            SlowCarDownForPedsSectorList(peds, vehicle, minX, minY, maxX, maxY, &speed, baseSpeed);
            SlowCarDownForObjectsSectorList(objects, vehicle, minX, minY, maxX, maxY, &speed, baseSpeed);
        }
    }

    vehicle->vehicleFlags.bWarnedPeds = true;

    if (autoPilot.m_nCarDrivingStyle != (eCarDrivingStyle)0 && autoPilot.m_nCarDrivingStyle != (eCarDrivingStyle)4) {
        return (baseSpeed + speed) * 0.5f;
    }
    return speed;
}

// 0x422090
int8 CCarCtrl::FindPathDirection(CNodeAddress nodeAddress1, CNodeAddress nodeAddress2, CNodeAddress nodeAddress3, bool* arg4) {
    *arg4 = false;

    if (nodeAddress1.m_wAreaId == (uint16)-1
        || nodeAddress2.m_wAreaId == (uint16)-1
        || nodeAddress3.m_wAreaId == (uint16)-1
    ) {
        return 0;
    }

    CPathNode* const area1 = ThePaths.m_pPathNodes[nodeAddress1.m_wAreaId];
    CPathNode* const area2 = ThePaths.m_pPathNodes[nodeAddress2.m_wAreaId];
    CPathNode* const area3 = ThePaths.m_pPathNodes[nodeAddress3.m_wAreaId];
    if (!area1 || !area2 || !area3) {
        return 0;
    }

    const CVector pos1 = area1[nodeAddress1.m_wNodeId].GetPosition();
    const CVector pos2 = area2[nodeAddress2.m_wNodeId].GetPosition();
    const CVector pos3 = area3[nodeAddress3.m_wNodeId].GetPosition();

    CVector2D dir1{pos2.x - pos1.x, pos2.y - pos1.y};
    CVector2D dir2{pos3.x - pos2.x, pos3.y - pos2.y};

    const float len1 = std::sqrt(dir1.SquaredMagnitude());
    if (len1 == 0.0f) {
        return 0;
    }
    dir1.x *= 1.0f / len1;
    dir1.y *= 1.0f / len1;

    const float len2 = std::sqrt(dir2.SquaredMagnitude());
    if (len2 == 0.0f) {
        return 0;
    }
    dir2.x *= 1.0f / len2;
    dir2.y *= 1.0f / len2;

    const float cross = dir1.x * dir2.y - dir2.x * dir1.y;
    const float dot   = dir2.x * dir1.x + dir2.y * dir1.y;

    if (dot > 0.4f) {
        return 1;
    }
    if (dot < -0.3f) {
        *arg4 = true;
    }
    return cross > 0.0f ? 4 : 2;
}

// 0x422620
float CCarCtrl::FindPercDependingOnDistToLink(CVehicle* vehicle, CCarPathLinkAddress linkAddress) {
    return CGeneral::GetPiecewiseLinear({
        { 5.0f,  0.5f },
        { 15.0f, 1.0f },
    }, CVector2D::Dist(ThePaths.GetCarPathLink(linkAddress).GetNodeCoors(), vehicle->GetPosition2D()));
}

// 0x421770
int32 CCarCtrl::FindSequenceElement(int32 element) {
    return bSequenceOtherWay
        ? (element + SequenceRandomOffset) % SequenceElements
        : (SequenceElements - element + SequenceRandomOffset) % SequenceElements;
}

// 0x4224E0
float CCarCtrl::FindSpeedMultiplier(float angleChange, float minAngle, float maxAngle, float minMultiplier) {
    return CGeneral::GetPiecewiseLinear({
        { minAngle, 1.0f },
        { maxAngle, minMultiplier },
    }, std::abs(CGeneral::LimitRadianAngle(angleChange)));
}

// 0x424130
float CCarCtrl::FindSpeedMultiplierWithSpeedFromNodes(int8 speedFromNodes) {
    switch (speedFromNodes) {
    case -1: return 0.5f;
    case 0:  return 0.65f;
    case 2:  return 2.3f;
    default: return 1.0f;
    }
}

// 0x422370
float CCarCtrl::FindGhostRoadHeight(CVehicle* vehicle) {
    const auto curAddress  = vehicle->m_autoPilot.m_currentAddress;
    const auto nextAddress = vehicle->m_autoPilot.m_startingRouteNode;

    if (curAddress.m_wAreaId == (uint16)-1 || nextAddress.m_wAreaId == (uint16)-1) {
        return 0.0f;
    }
    if (!ThePaths.m_pPathNodes[curAddress.m_wAreaId] || !ThePaths.m_pPathNodes[nextAddress.m_wAreaId]) {
        return 0.0f;
    }

    const CVector curPos  = ThePaths.m_pPathNodes[curAddress.m_wAreaId][curAddress.m_wNodeId].GetPosition();
    const CVector nextPos = ThePaths.m_pPathNodes[nextAddress.m_wAreaId][nextAddress.m_wNodeId].GetPosition();
    const CVector vehPos  = vehicle->GetPosition();

    const float curDist  = std::sqrt((curPos.x - vehPos.x) * (curPos.x - vehPos.x) + (curPos.y - vehPos.y) * (curPos.y - vehPos.y));
    const float nextDist = std::sqrt((nextPos.x - vehPos.x) * (nextPos.x - vehPos.x) + (nextPos.y - vehPos.y) * (nextPos.y - vehPos.y));

    return (nextDist * curPos.z + curDist * nextPos.z) / (curDist + nextDist);
}

// 0x42B270
void CCarCtrl::FireHeliRocketsAtTarget(CAutomobile* entityLauncher, CEntity* entity) {
    //! Rockets can only be fired when no other weapon is in use (or the double rockets already are)
    if (entityLauncher->m_nVehicleWeaponInUse != CAR_WEAPON_NOT_USED
        && entityLauncher->m_nVehicleWeaponInUse != CAR_WEAPON_DOUBLE_ROCKET
    ) {
        return;
    }

    //! At most one volley per 250ms window
    if (CTimer::GetTimeInMS() / 250u == CTimer::GetPreviousTimeInMS() / 250u) {
        return;
    }

    const CVector launcherPos = entityLauncher->GetPosition();
    const CVector targetPos   = entity->GetPosition();

    { //! Target must be within 80m
        const CVector delta = targetPos - launcherPos;
        if (delta.Magnitude() >= 80.0f) {
            return;
        }
    }

    const CVector dir = entityLauncher->GetForward();
    { //! ...and roughly in front of us
        const CVector lineEnd = launcherPos + dir;
        if (CCollision::DistToMathematicalLine(&launcherPos, &lineEnd, &targetPos) >= 7.0f) {
            return;
        }
    }

    //! Alternate the launch pylon every 250ms window
    const CVector pylonOffset = (CTimer::GetTimeInMS() / 250u) & 1
        ? dir * -1.5f
        : dir * 1.5f;

    CProjectileInfo::AddProjectile(
        entityLauncher,
        WEAPON_ROCKET,
        launcherPos + dir * 4.0f + pylonOffset,
        1.0f,
        &dir,
        nullptr
    );
}

// 0x429A70
void CCarCtrl::FlyAIHeliInCertainDirection(CHeli* heli, float heading, float dist, bool arg4) {
    //! News heli hover: turn 90 degrees when close & slow
    bool bHalfMinAltitude = false;
    if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_NEWS_BEHAVIOUR
        && dist < (float)heli->m_autoPilot.m_ucHeliTargetDist2
        && heli->m_vecMoveSpeed.Magnitude2D() < 0.01f
    ) {
        bHalfMinAltitude = true;
        heading += PI / 2.0f;
    }

    //! Every 500ms (phase shifted by the entity's random seed)
    if ((CTimer::GetTimeInMS() + heli->m_nRandomSeed) % 500u < (CTimer::GetPreviousTimeInMS() + heli->m_nRandomSeed) % 500u) {
        heli->field_9AC = heli->m_fMaxAltitude;

        const CVector pos = heli->GetPosition();
        const CVector vel = heli->m_vecMoveSpeed;

        //! Look 60m ahead along the requested heading
        const CVector ahead = pos + vel * 50.0f;
        CVector       dir{ std::cos(heading), std::sin(heading), -1.0f };
        dir.Normalise();

        CColPoint colPoint{};
        CEntity*  entity{};
        if (CWorld::ProcessLineOfSight(ahead, ahead + dir * 60.0f, colPoint, entity, true, false, false, false, false, false, false, true)) {
            const float minAlt = bHalfMinAltitude ? heli->m_fMinAltitude * 0.5f : heli->m_fMinAltitude;
            heli->field_9AC   = std::max(minAlt + colPoint.m_vecPoint.z, heli->field_9AC);
        }

        //! Obstacle-avoidance probe along the flight direction
        CVector fwd{ vel.x, vel.y, 0.0f };
        fwd.Normalise();

        float probe = heli->m_vecMoveSpeed.Magnitude2D() * 100.0f;
        probe += (heli->m_autoPilot.m_nCarCtrlFlags & 0x80) ? 5.0f : 30.0f;

        heli->field_9B8 = true;
        if (!CWorld::ProcessLineOfSight(pos, pos + fwd * probe, colPoint, entity, true, false, false, false, false, false, false, true)) {
            heli->field_9B8             = false;
            heli->m_fSteeringLeftRight  = 0.0f;
        } else {
            //! Which side has more room?
            //! @todo(re) The two side probes (0x429D65-0x429EF3) were only partly recovered
            CVector right = heli->GetRight();
            right.z = 0.0f;
            right.Normalise();

            const auto ProbeDistance = [&](const CVector& from, const CVector& to) {
                CColPoint cp{};
                CEntity*  e{};
                if (CWorld::ProcessLineOfSight(from, to, cp, e, true, false, false, false, false, false, false, true)) {
                    return (from - cp.m_vecPoint).Magnitude();
                }
                return (from - to).Magnitude();
            };

            const float freeLeft  = ProbeDistance(pos - right * 10.0f, pos - right * 10.0f + fwd * probe);
            const float freeRight = ProbeDistance(pos + right * 10.0f, pos + right * 10.0f + fwd * probe);

            heli->m_fSteeringLeftRight = freeLeft <= freeRight ? -0.5f : 0.5f;
        }
    }

    if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_LAND_TOUCHING_DOWN) {
        heli->field_9B8             = false;
        heli->m_fSteeringLeftRight  = 0.0f;
    }

    const CVector matrixFwd   = heli->GetForward();
    const float   curHeading  = CGeneral::GetATanOfXY(matrixFwd.x, matrixFwd.y);

    //! Yaw error -> skid
    float yawErr = CGeneral::LimitRadianAngle(heading - curHeading) * -2.0f;
    heli->m_fLeftRightSkid = std::clamp(yawErr, -1.0f, 1.0f);

    //! Vertical control
    const CVector pos = heli->GetPosition();
    const CVector vel = heli->m_vecMoveSpeed;

    float lift = heli->field_9AC - (vel.z * 100.0f + pos.z);
    lift = lift <= 0.0f ? lift * 0.2f : lift * 0.1f;
    lift += ((float)(CGeneral::GetRandomNumber() & 0xF) - 7.0f) * 0.002f;

    const float wasAbove = lift - 0.5f;
    heli->m_fAccelerationBreakStatus = std::clamp(lift, -0.3f, 1.0f);

    //! Vertical steering
    if (dist > 60.0f || !arg4) {
        heli->m_fSteeringUpDown = -0.8f;
    } else {
        const CVector dest = heli->m_autoPilot.m_vecDestinationCoors;
        const float   dx   = (vel.x * 50.0f + pos.x) - dest.x;
        const float   dy   = (vel.y * 50.0f + pos.y) - dest.y;
        const float   d2   = std::sqrt(dx * dx + dy * dy) - (float)heli->m_autoPilot.m_ucHeliTargetDist2;
        if (d2 >= 0.0f) {
            heli->m_fSteeringUpDown = (d2 * -0.8f) / (30.0f - (float)heli->m_autoPilot.m_ucHeliTargetDist2);
        }
    }

    //! Modulate the vertical command by the yaw error
    if (heli->m_fSteeringUpDown < 0.0f) {
        const float a = std::abs(CGeneral::LimitRadianAngle(heading - curHeading));
        float       k = 1.0f - a * 2.1220658f;
        if (k < 0.0f) {
            k = 0.0f;
        }
        heli->m_fSteeringUpDown *= k;
        if (a > PI / 2.0f) {
            const float dotFwd = matrixFwd.x * vel.x + matrixFwd.y * vel.y;
            if (dotFwd > 0.0f) {
                heli->m_fSteeringUpDown = 0.2f;
            }
        }
    }
    if (wasAbove > 0.0f) {
        heli->m_fSteeringUpDown *= 1.0f - std::min(wasAbove, 1.0f);
    }

    //! Blend with the speed-holding term
    bool clampOut = false;
    if (heli->m_fSteeringUpDown >= 0.0f) {
        if (!heli->field_9B8) {
            clampOut = true;
        }
    } else {
        const float dotFwd = matrixFwd.x * vel.x + matrixFwd.y * vel.y;
        float       v      = ((float)heli->m_autoPilot.m_nCruiseSpeed - dotFwd * 60.0f) * 0.1f;
        v                  = -std::clamp(v, 0.0f, 1.0f);
        heli->m_fSteeringUpDown = std::min(heli->m_fSteeringUpDown, v);
        if (heli->m_autoPilot.m_ucHeliSpeedMult != 0 && heli->m_fSteeringUpDown < -0.2f && !heli->field_9B8) {
            const float k = (float)heli->m_autoPilot.m_ucHeliSpeedMult * CTimer::GetTimeStep() * 0.001f;
            heli->m_vecMoveSpeed.x += matrixFwd.x * k;
            heli->m_vecMoveSpeed.y += matrixFwd.y * k;
            clampOut = true;
        }
    }

    if (!clampOut) {
        //! Obstacle ahead: vertical command = 2 * (velocity · forward)
        const float k = matrixFwd.x * vel.x + matrixFwd.y * vel.y;
        heli->m_fSteeringUpDown = k + k;
    }

    heli->m_fSteeringUpDown = std::clamp(heli->m_fSteeringUpDown, -1.0f, 1.0f);
}

// 0x423940
void CCarCtrl::FlyAIHeliToTarget_FixedOrientation(CHeli* heli, float orientation, CVector posn) {
    //! Once every 500ms (Phase shifted by the entity's random seed)
    if ((heli->m_nRandomSeed + CTimer::GetTimeInMS()) % 500u < (CTimer::GetPreviousTimeInMS() + heli->m_nRandomSeed) % 500u) {
        heli->field_9AC = heli->m_fMaxAltitude;

        const CVector pos = heli->GetPosition();
        const CVector src = pos + heli->m_vecMoveSpeed * 50.0f;

        const float cosOrient = std::cos(orientation);
        const float sinOrient = std::sin(orientation);

        const CVector dst{
            src.x + cosOrient * 60.0f,
            src.y + sinOrient * 60.0f,
            src.z - 60.0f
        };

        CColPoint colPoint{};
        CEntity*  entity{};
        if (CWorld::ProcessLineOfSight(src, dst, colPoint, entity, true, false, false, false, false, false, false, true)) {
            float targetAltitude = colPoint.m_vecPoint.z + heli->m_fMinAltitude;
            if (targetAltitude < heli->field_9AC) {
                targetAltitude = heli->field_9AC;
            }
            heli->field_9AC = targetAltitude;
        }
    }

    const CMatrix& matrix     = heli->GetMatrix();
    const CVector  pos        = heli->GetPosition();
    const float    axisHeading = CGeneral::GetATanOfXY(matrix.GetForward().x, matrix.GetForward().y);

    //! Vertical control
    heli->m_fAccelerationBreakStatus = 0.0f;

    float lift = heli->field_9AC - (heli->m_vecMoveSpeed.z * 100.0f + pos.z);
    lift = lift <= 0.0f ? lift * 0.2f : lift * 0.1f;
    heli->m_fAccelerationBreakStatus = lift;

    lift += ((float)(CGeneral::GetRandomNumber() & 0xF) - 7.0f) * 0.002f;
    if (lift > 1.0f) {
        lift = 1.0f;
    } else if (lift < -0.3f) {
        lift = -0.3f;
    }
    heli->m_fAccelerationBreakStatus = lift;

    //! Yaw
    float yaw = orientation - axisHeading;
    while (yaw > 3.1415927f)  yaw -= 6.2831855f;
    while (yaw < -3.1415927f) yaw += 6.2831855f;
    yaw *= -0.5f;
    heli->m_fLeftRightSkid = std::clamp(yaw, -1.0f, 1.0f);

    //! Attitude
    const CVector& right   = matrix.GetRight();
    const CVector& forward = matrix.GetForward();
    const CVector& velocity = heli->m_vecMoveSpeed;

    const float dotRight = right.x * velocity.x + right.y * velocity.y + right.z * velocity.z;
    const float dotFwd   = forward.x * velocity.x + forward.y * velocity.y + forward.z * velocity.z;

    const float rightError = dotRight * 80.0f + (posn.x - pos.x) * right.x + (posn.y - pos.y) * right.y;
    const float fwdError   = dotFwd * 80.0f + (posn.x - pos.x) * forward.x + (posn.y - pos.y) * forward.y;

    heli->m_fSteeringLeftRight = std::abs(rightError) >= 5.0f ? rightError * -0.02f : dotRight;
    heli->m_fSteeringLeftRight = std::clamp(heli->m_fSteeringLeftRight, -0.75f, 0.75f);

    heli->m_fSteeringUpDown = std::abs(fwdError) >= 5.0f ? fwdError * -0.015f : dotFwd;
    heli->m_fSteeringUpDown = std::clamp(heli->m_fSteeringUpDown, -0.5f, 0.5f);
}

// 0x422E10
static bool ProbePlaneClearance(const CPlane* plane, float pitch, float heading, float& outZ) {
    const CVector src = plane->GetPosition() + plane->m_vecMoveSpeed * 50.0f;

    const float cosPitch = std::cos(pitch);
    const CVector dst{
        src.x + std::cos(heading) * cosPitch * 200.0f,
        src.y + std::sin(heading) * cosPitch * 200.0f,
        src.z + std::sin(pitch) * 200.0f,
    };

    CColPoint colPoint{};
    CEntity*  entity{};
    if (CWorld::ProcessLineOfSight(src, dst, colPoint, entity, true, false, false, false, false, false, false, true)) {
        outZ = colPoint.m_vecPoint.z;
        return true;
    }
    return false;
}

// 0x422F80
static float FindPlaneClearance(const CPlane* plane, float heading) {
    static constexpr std::array<float, 6> PITCH_OFFSETS{
        0.10471976f, //!<  +6 degrees
        0.05235988f, //!<  +3 degrees
        0.0f,
        -0.34893f,   //!< -20 degrees
        -0.69787f,   //!< -40 degrees
        -1.0472f,    //!< -60 degrees
    };

    float clearance = 0.0f;
    for (size_t i = 0; i < std::size(PITCH_OFFSETS); i++) {
        float z;
        if (ProbePlaneClearance(plane, PITCH_OFFSETS[i], heading, z)) {
            if (i == 0) { //!< Straight ahead is blocked
                return 100000.0f;
            }
            if (clearance <= z) {
                clearance = z;
            }
        }
    }
    return clearance;
}

// 0x423000
void CCarCtrl::FlyAIPlaneInCertainDirection(CPlane* pPlane) {
    if ((pPlane->m_nRandomSeed + CTimer::GetTimeInMS()) % 1000u < (CTimer::GetPreviousTimeInMS() + pPlane->m_nRandomSeed) % 1000u) {
        CMatrix& matrix = *pPlane->m_matrix;

        const float heading = CGeneral::GetATanOfXY(matrix.GetForward().x, matrix.GetForward().y);

        float baseHeading = pPlane->m_planeHeading;
        switch (pPlane->m_autoPilot.m_nTempAction) {
        case TEMPACT_PLANE_FLY_STRAIGHT:
            baseHeading = heading;
            break;
        case TEMPACT_PLANE_SHARP_LEFT:
            baseHeading = heading - 2.0f;
            break;
        case TEMPACT_PLANE_SHARP_RIGHT:
            baseHeading = heading + 2.0f;
            break;
        }

        pPlane->m_altitude         = 1000.0f;
        pPlane->m_planeHeadingPrev = pPlane->m_planeHeading + 3.1415927f;

        float deltaHeading = heading - baseHeading;
        while (deltaHeading > 3.1415927f)  deltaHeading -= 6.2831855f;
        while (deltaHeading < -3.1415927f) deltaHeading += 6.2831855f;

        const float maxHeadingOffset = std::abs(deltaHeading) >= 0.5235988f ? deltaHeading * 1.5f : 0.0f;

        //! Sweep the candidate offsets (15 degrees apart) - First those within `maxHeadingOffset`, then the rest
        for (int32 pass = 0; pass < 2; pass++) {
            for (int32 i = 1; i < 20; i++) {
                const float offset = (i & 1 ? (float)(i / 2) : -(float)(i / 2)) * 0.2617994f;

                const bool inRange = (offset < 0.0f && maxHeadingOffset < offset) || (offset > 0.0f && offset < maxHeadingOffset);
                if ((pass == 0) != inRange) {
                    continue;
                }

                const float clearance = FindPlaneClearance(pPlane, offset + baseHeading);
                if (clearance < 150.0f) {
                    pPlane->m_planeHeadingPrev = offset * 1.1f + pPlane->m_planeHeading;

                    float targetAltitude = clearance + pPlane->m_minAltitude;
                    if (targetAltitude <= pPlane->m_maxAltitude) {
                        targetAltitude = pPlane->m_maxAltitude;
                    }
                    pPlane->m_altitude = targetAltitude;
                    break;
                }
            }
        }
    }

    CMatrix& matrix = *pPlane->m_matrix;

    //! Reference speed
    const float refSpeed = pPlane->vehicleFlags.bIsRCVehicle ? 7.0f : 32.0f;

    const float axisHeading = CGeneral::GetATanOfXY(matrix.GetForward().x, matrix.GetForward().y);
    const CVector& velocity = pPlane->m_vecMoveSpeed;
    const float speedAlong  = (matrix.GetForward().x * velocity.x + matrix.GetForward().y * velocity.y) * 60.0f;

    //! Damaged (Suspension compressed) => Start the "flying" timer
    if (pPlane->m_fWheelsSuspensionCompression[0] < 1.0f
        || pPlane->m_fWheelsSuspensionCompression[1] < 1.0f
        || pPlane->m_fWheelsSuspensionCompression[2] < 1.0f
        || pPlane->m_fWheelsSuspensionCompression[3] < 1.0f
    ) {
        pPlane->m_nStartedFlyingTime = CTimer::GetTimeInMS();
    }

    const bool levelFlight = CTimer::GetTimeInMS() - pPlane->m_nStartedFlyingTime < 0xFA1u;
    if (levelFlight) {
        pPlane->m_fSteeringUpDown = refSpeed <= speedAlong ? 0.4f : 0.0f;
    } else {
        if (pPlane->m_LandingGearAngle != 1.0f) {
            pPlane->SetGearUp();
        }

        const float posZ        = matrix.GetPosition().z;
        const float newForwardZ = std::asin(matrix.GetForward().z);
        const float oldForwardZ = pPlane->m_forwardZ;
        const float timeStep    = CTimer::GetTimeStep();

        pPlane->m_forwardZ = newForwardZ;

        float climb = (pPlane->m_altitude - (velocity.z * 100.0f + posZ)) * 0.033333335f;
        climb = std::clamp(climb, -0.4f, 0.4f);
        if (speedAlong < refSpeed && climb >= 0.25f) {
            climb = 0.25f;
        }

        pPlane->m_fSteeringUpDown = (climb - ((100.0f / timeStep) * (newForwardZ - oldForwardZ) + newForwardZ)) * 0.5f;
    }

    //! Yaw
    float yawError = pPlane->m_planeHeadingPrev - axisHeading;
    while (yawError < -3.1415927f) yawError += 6.2831855f;
    while (yawError > 3.1415927f)  yawError -= 6.2831855f;

    if (levelFlight) {
        pPlane->m_fLeftRightSkid     = std::clamp(-(pPlane->m_planeCreationHeading - axisHeading) * 10.0f, -1.0f, 1.0f);
        pPlane->m_fSteeringLeftRight = 0.0f;
    } else {
        float steer = std::clamp(-yawError * 1.5f, -0.9f, 0.9f);

        if (pPlane->vehicleFlags.bIsRCVehicle) {
            if (steer > 0.7f) {
                steer = 0.7f;
            } else if (steer < -0.7f) {
                steer = -0.7f;
            }
        }

        const float steerMagnitude = std::abs(steer);

        if (steerMagnitude >= 0.1f) {
            pPlane->m_fLeftRightSkid = steer;
            steer = -steer;
        } else {
            pPlane->m_fLeftRightSkid = steer * 4.0f;
            steer = 0.0f;
        }

        if (speedAlong < refSpeed * 1.2f) {
            float speedMultiplier = 1.0f - (refSpeed * 1.2f - speedAlong) / (refSpeed * 0.5f);
            if (speedMultiplier < 0.0f) {
                speedMultiplier = 0.0f;
            }
            steer = speedMultiplier * steer;
        }

        //! Roll feedback
        float roll = 0.0f;
        if (pPlane->m_matrix) {
            const CVector& right = matrix.GetRight();
            float planar = std::sqrt(right.y * right.y + right.x * right.x);
            if (matrix.GetUp().z < 0.0f) {
                planar = -planar;
            }
            roll = std::atan2(right.z, planar);
        }

        const float timeStepClamped = std::max(CTimer::GetTimeStep(), 1.0f);
        steer = steer - ((roll - pPlane->m_fSteeringFactor) * (30.0f / timeStepClamped) + roll);
        while (steer > 3.1415927f)  steer -= 6.2831855f;
        while (steer < -3.1415927f) steer += 6.2831855f;
        steer *= -1.0f;

        pPlane->m_fSteeringFactor    = roll;
        pPlane->m_fSteeringLeftRight = std::clamp(steer, -1.0f, 1.0f);

        const float prevThrottle = pPlane->m_fSteeringUpDown;
        pPlane->m_fSteeringUpDown += std::abs(0.23f * roll);
        if (prevThrottle < 0.0f) {
            float halfPrevThrottle = prevThrottle * 0.5f;
            if (pPlane->m_fSteeringUpDown < halfPrevThrottle) {
                halfPrevThrottle = pPlane->m_fSteeringUpDown;
            }
            pPlane->m_fSteeringUpDown = halfPrevThrottle;
        }
    }

    pPlane->m_fSteeringUpDown = std::clamp(pPlane->m_fSteeringUpDown, -1.0f, 1.0f);
    pPlane->m_fAccelerationBreakStatus = pPlane->m_fAccelerationBreakStatusPrev;

    if (pPlane->m_autoPilot.m_nTempAction == TEMPACT_PLANE_FLY_UP) {
        pPlane->m_fSteeringUpDown = 1.0f;
        if (speedAlong < 20.0f) {
            pPlane->m_autoPilot.m_nTempAction = TEMPACT_NONE;
        }
    }
}

// 0x424210
bool CCarCtrl::GenerateCarCreationCoors2(CVector posn, float radius, float arg3, float arg4, bool arg5, float arg6, float arg7, CVector* origin, CNodeAddress* nodeAddress1, CNodeAddress* nodeAddress2, float* arg11, bool arg12, bool arg13) {
    float travelled = 0.0f;

    //! Lazy init of the cached node slots
    if (!(s_CarCreationNodeCacheMask & 0x1))  { s_CarCreationNodeCacheMask |= 0x1;  s_CarCreationNodeB1.m_wAreaId = (uint16)-1; }
    if (!(s_CarCreationNodeCacheMask & 0x2))  { s_CarCreationNodeCacheMask |= 0x2;  s_CarCreationNodeB2.m_wAreaId = (uint16)-1; }
    if (!(s_CarCreationNodeCacheMask & 0x4))  { s_CarCreationNodeCacheMask |= 0x4;  s_CarCreationNodeA1.m_wAreaId = (uint16)-1; }
    if (!(s_CarCreationNodeCacheMask & 0x8))  { s_CarCreationNodeCacheMask |= 0x8;  s_CarCreationNodeA2.m_wAreaId = (uint16)-1; }
    if (!(s_CarCreationNodeCacheMask & 0x10)) { s_CarCreationNodeCacheMask |= 0x10; }

    const CVector2D posn2D{posn.x, posn.y};

    //! Refresh the node cache if the point moved more than 10 units, or 5 seconds have passed
    if (CVector2D::Dist(posn2D, CVector2D{s_CarCreationCachedPosn.x, s_CarCreationCachedPosn.y}) > 10.0f
        || s_CarCreationNodeCacheTime < CTimer::GetTimeInMS()
    ) {
        ThePaths.Find2NodesForCarCreation(posn, &s_CarCreationNodeB1, &s_CarCreationNodeB2, true);
        ThePaths.Find2NodesForCarCreation(posn, &s_CarCreationNodeA1, &s_CarCreationNodeA2, false);

        s_CarCreationNodeCacheTime = CTimer::GetTimeInMS() + 5000;
        s_CarCreationCachedPosn    = posn;

        if (s_CarCreationNodeB1.m_wAreaId != (uint16)-1 && ThePaths.m_pPathNodes[s_CarCreationNodeB1.m_wAreaId]) {
            const CPathNode& node = ThePaths.m_pPathNodes[s_CarCreationNodeB1.m_wAreaId][s_CarCreationNodeB1.m_wNodeId];
            const CCarPathLinkAddress linkAddress = ThePaths.m_pNaviLinks[node.m_wAreaId][node.m_wBaseLinkId];

            if (ThePaths.m_pPathNodes[linkAddress.m_wAreaId]) {
                const CCarPathLink& link = ThePaths.GetCarPathLink(linkAddress);
                if (link.m_numSameDirLanes < 2 && link.m_numOppositeDirLanes < 2) {
                    CPopulation::m_bMoreCarsAndFewerPeds = false;
                } else if (CVector2D::Dist(CVector2D(link.m_posn), posn2D) >= 40.0f) {
                    CPopulation::m_bMoreCarsAndFewerPeds = false;
                } else {
                    CPopulation::m_bMoreCarsAndFewerPeds = node.m_nSpeedLimit >= 2; //!< Fast road node
                }
            }
        }
    }

    //! Pick the starting node
    CNodeAddress startAddress;
    if ((CGeneral::GetRandomNumber() & 0xF) == 4 && !arg13) {
        startAddress = ThePaths.FindNodeClosestToCoors(posn, PATH_TYPE_VEH, 50.0f, 0, 0, 0, 1, 1);
        arg6 *= 1.5f;
    } else if ((CGeneral::GetRandomNumber() & 3) == 0) {
        startAddress = arg12 ? s_CarCreationNodeB2 : s_CarCreationNodeA2;
    } else {
        startAddress = arg12 ? s_CarCreationNodeB1 : s_CarCreationNodeA1;
    }

    if (startAddress.m_wAreaId == (uint16)-1 || !ThePaths.m_pPathNodes[startAddress.m_wAreaId]) {
        return false;
    }

    CNodeAddress visited[30]{};
    int32       visitedCount = 1;
    visited[0] = startAddress;

    CNodeAddress curAddress = startAddress;

    while (true) {
        if (travelled >= 230.0f) {
            return false;
        }

        if (!ThePaths.m_pPathNodes[curAddress.m_wAreaId]) {
            return false;
        }

        const CPathNode& curNode = ThePaths.m_pPathNodes[curAddress.m_wAreaId][curAddress.m_wNodeId];

        const int32 numLinks = curNode.m_nNumLinks;
        SequenceElements     = numLinks;
        SequenceRandomOffset = CGeneral::GetRandomNumber() % numLinks; //!< NOTE: Exactly as in the original - Crashes if `numLinks` is 0
        bSequenceOtherWay    = (CGeneral::GetRandomNumber() >> 4) & 1;

        //! Find a node linked to the current one that we haven't visited yet
        CNodeAddress nextAddress;
        bool         linkFound = false;
        for (auto attempt = 0; attempt < numLinks; attempt++) {
            const int32 linkIndex = curNode.m_wBaseLinkId
                + ((bSequenceOtherWay
                        ? SequenceRandomOffset + attempt
                        : SequenceRandomOffset + numLinks - attempt
                   ) % numLinks);

            const CNodeAddress candidate = ThePaths.m_pNodeLinks[curAddress.m_wAreaId][linkIndex];
            if (!ThePaths.m_pPathNodes[candidate.m_wAreaId]
                || !ThePaths.m_pPathNodes[ThePaths.m_pNaviLinks[curAddress.m_wAreaId][linkIndex].m_wAreaId]
            ) {
                continue;
            }

            bool alreadyVisited = false;
            for (int32 i = 0; i < visitedCount; i++) {
                if (visited[i] == candidate) {
                    alreadyVisited = true;
                }
            }
            if (!alreadyVisited) {
                nextAddress = candidate;
                linkFound   = true;
                break;
            }
        }
        if (!linkFound) {
            return false;
        }

        //! Segment geometry
        const CVector curPos  = curNode.GetPosition();
        const CPathNode& nextNode = ThePaths.m_pPathNodes[nextAddress.m_wAreaId][nextAddress.m_wNodeId];
        const CVector nextPos = nextNode.GetPosition();

        const float curDist  = CVector2D::Dist(CVector2D{curPos.x, curPos.y}, posn2D);
        const float nextDist = CVector2D::Dist(CVector2D{nextPos.x, nextPos.y}, posn2D);

        bool advance = false;
        if ((nextNode.m_isSwitchedOff || curNode.m_isSwitchedOff) && arg12) {
            advance = true;
        } else {
            //! Tries to find a point on this segment that lies exactly `threshold` units away from `posn`.
            //! Returns:
            //!   0 - `threshold` doesn't fall between the two nodes
            //!   1 - Found one (Writes `origin` and `arg11`)
            //!  -1 - Found one, but the camera visibility check failed
            const auto tryThreshold = [&](float threshold, bool mustBeVisible) {
                const float curDelta  = curDist - threshold;
                const float nextDelta = nextDist - threshold;
                if (curDelta * nextDelta >= 0.0f) {
                    return 0;
                }

                const float curWeight  = std::abs(curDelta);
                const float nextWeight = std::abs(nextDelta);
                const float invSum     = 1.0f / (curWeight + nextWeight);

                origin->x = (curPos.x * nextWeight + nextPos.x * curWeight) * invSum;
                origin->y = (curPos.y * nextWeight + nextPos.y * curWeight) * invSum;
                origin->z = (curPos.z * nextWeight + nextPos.z * curWeight) * invSum;

                if (TheCamera.IsSphereVisible(*origin, 50.0f) != mustBeVisible) {
                    return -1;
                }

                *arg11 = curWeight * invSum;
                return 1;
            };

            int32 result = tryThreshold(arg6, true);
            if (result != 1) {
                result = tryThreshold(arg7, false);
            }
            if (result != 1) {
                advance = true;
            }
        }

        if (advance) {
            visited[visitedCount++] = nextAddress;

            travelled += std::sqrt(
                (nextPos.x - curPos.x) * (nextPos.x - curPos.x) +
                (nextPos.y - curPos.y) * (nextPos.y - curPos.y)
            );
            curAddress = nextAddress;

            if (visitedCount > 0x1D) {
                return false;
            }
            continue;
        }

        //! Only accept "flat enough" segments
        if (std::abs(curPos.z - nextPos.z) > std::sqrt(
                (nextPos.y - curPos.y) * (nextPos.y - curPos.y) +
                (nextPos.x - curPos.x) * (nextPos.x - curPos.x)
            ) * 0.5f
        ) {
            advance = true;
            continue;
        }

        //! Orient the node pair
        if (CGeneral::GetRandomNumber() & 8) {
            *nodeAddress1 = curAddress;
            *nodeAddress2 = nextAddress;
        } else {
            *nodeAddress1 = nextAddress;
            *nodeAddress2 = curAddress;
            *arg11        = 1.0f - *arg11;
        }

        CPathNode& node1 = ThePaths.m_pPathNodes[nodeAddress1->m_wAreaId][nodeAddress1->m_wNodeId];
        CPathNode& node2 = ThePaths.m_pPathNodes[nodeAddress2->m_wAreaId][nodeAddress2->m_wNodeId];

        if (node2.m_onDeadEnd && ThePaths.ThisNodeWillLeadIntoADeadEnd(&node2, &node1) && arg12) {
            return false;
        }

        //! Direction test
        const float dirX   = origin->x - posn.x;
        const float dirY   = origin->y - posn.y;
        const float invLen = 1.0f / std::sqrt(dirX * dirX + dirY * dirY);
        const float dot    = invLen * dirX * radius + invLen * dirY * arg3;

        if (!arg5) {
            if (dot < arg4) {
                return false;
            }
        } else if (dot <= arg4) {
            return false;
        }

        //! NOTE: Exactly as in the original - The result of this check isn't used
        for (auto i = 0; i < (int32)node1.m_nNumLinks; i++) {
            if (ThePaths.m_pNodeLinks[nodeAddress1->m_wAreaId][node1.m_wBaseLinkId + i] == *nodeAddress2) {
                break;
            }
        }

        sprintf(gString, "tell Obbe it happened again %d/%d %d/%d",
            nodeAddress1->m_wAreaId, nodeAddress1->m_wNodeId,
            nodeAddress2->m_wAreaId, nodeAddress2->m_wNodeId
        );
        return true;
    }
}

// 0x42F9C0
void CCarCtrl::GenerateEmergencyServicesCar() {
    if (!bAllowEmergencyServicesToBeCreated) {
        return;
    }
    if (CGangWars::GangWarFightingGoingOn()) {
        return;
    }
    if (FindPlayerPed(-1)->GetPlayerWanted()->GetWantedLevel() > eWantedLevel::WANTED_LEVEL_3) {
        return;
    }
    if (CGame::currArea != AREA_CODE_NORMAL_WORLD) { //!< No emergency services in interiors
        return;
    }
    if (CTheZones::m_CurrLevel == LEVEL_NAME_COUNTRY_SIDE) { //!< Nor in the countryside
        return;
    }
    if ((int32)(NumAmbulancesOnDuty + NumFireTrucksOnDuty + NumParkedCars + NumMissionCars + NumLawEnforcerCars + NumRandomCars) > (int32)MaxNumberOfCarsInUse) {
        return;
    }

    //! Ambulances
    if (NumAmbulancesOnDuty == 0) {
        CAccidentManager* const accidentMgr = CAccidentManager::GetInstance();
        if (accidentMgr->GetNumberOfFreeAccidents() < 2) {
            CStreaming::StreamAmbulanceAndMedic(false);
        } else {
            CVector playerPos = FindPlayerCoors(-1);

            //! NOTE: The original passes an indeterminate value for `bIgnoreHeadless`
            if (CAccident* const nearest = accidentMgr->GetNearestFreeAccident(playerPos, false);
                nearest
                && CStreaming::StreamAmbulanceAndMedic(true)
                && (uint32)(LastTimeAmbulanceCreated + 30000) < CTimer::GetTimeInMS()
            ) {
                if (GenerateOneEmergencyServicesCar(CStreaming::ms_aDefaultAmbulanceModel[CTheZones::m_CurrLevel], nearest->m_pPed->GetPosition())) {
                    LastTimeAmbulanceCreated = CTimer::GetTimeInMS();
                }
            }
        }
    }

    //! Fire trucks
    if (NumFireTrucksOnDuty == 0) {
        if (gFireManager.GetNumOfNonScriptFires() < 3) {
            CStreaming::StreamFireEngineAndFireman(false);
            return;
        }

        if (CFire* const fire = gFireManager.FindNearestFire(FindPlayerCoors(-1), true, true);
            fire
            && CStreaming::StreamFireEngineAndFireman(true)
            && (uint32)(LastTimeFireTruckCreated + 35000) < CTimer::GetTimeInMS()
        ) {
            if (GenerateOneEmergencyServicesCar(CStreaming::ms_aDefaultFireEngineModel[CTheZones::m_CurrLevel], fire->GetPosition())) {
                LastTimeFireTruckCreated = CTimer::GetTimeInMS();
            }
        }
    }
}

// 0x42B7D0
CAutomobile* CCarCtrl::GenerateOneEmergencyServicesCar(uint32 modelId, CVector posn) {
    //! Find a free spawn spot near the player (5 tries)
    const CVector player = FindPlayerCentreOfWorld(CWorld::PlayerInFocus);

    CVector     origin{};
    CNodeAddress nodeAddress1{};
    CNodeAddress nodeAddress2{};
    float       pct{};

    bool ok = false;
    for (int32 tries = 0; !ok; tries++) {
        if (tries > 4) {
            return nullptr;
        }

        ok = GenerateCarCreationCoors2(
                 player, 0.70710677f, 0.70710677f, -1.0f, true, 160.0f, 160.0f,
                 &origin, &nodeAddress1, &nodeAddress2, &pct, false, false
             )
          && ThePaths.m_pPathNodes[nodeAddress1.m_wAreaId]
          && !ThePaths.GetPathNode(nodeAddress1)->m_bWaterNode;

        if (ok) {
            int16 numFound = 0;
            CWorld::FindObjectsKindaColliding(origin, 10.0f, true, &numFound, 2, nullptr, false, false, true, true, false);
            ok = numFound == 0;
        }
    }

    CAutomobile* const vehicle = new CAutomobile(modelId, RANDOM_VEHICLE, true);
    vehicle->SetPosn(origin);

    //! Face the requested position
    {
        CVector2D fwd{ posn.x - origin.x, posn.y - origin.y };
        const float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y);
        if (len == 0.0f) {
            fwd.x = 1.0f;
        } else {
            fwd.x *= 1.0f / len;
            fwd.y *= 1.0f / len;
        }

        CMatrix& m = vehicle->GetMatrix();
        m.GetForward() = CVector{ fwd.x, fwd.y, 0.0f };
        m.GetRight()   = CVector{ fwd.y, -fwd.x, 0.0f };
        m.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };
    }

    //! Ground height: Pick whichever probe result is closest to the nodes' height
    //! @todo(re) 0x42B7D0 - The original blends a *navi* node's z with a path node's z, but `CCarPathLink` has no z,
    //!                     so both ends are taken from the path nodes here.
    const float zNode = lerp(
        ThePaths.GetPathNode(nodeAddress2)->GetPosition().z,
        ThePaths.GetPathNode(nodeAddress1)->GetPosition().z,
        pct
    );

    float     groundZ = 1e9f;
    CColPoint colPoint{};
    CEntity*  hitEntity{};
    if (CWorld::ProcessVerticalLine(origin, 1000.0f, colPoint, hitEntity, true, false, false, false, true, false, nullptr)) {
        groundZ = colPoint.m_vecPoint.z;
    }
    if (CWorld::ProcessVerticalLine(origin, -1000.0f, colPoint, hitEntity, true, false, false, false, true, false, nullptr)) {
        if (std::abs(colPoint.m_vecPoint.z - zNode) < std::abs(groundZ - zNode)) {
            groundZ = colPoint.m_vecPoint.z;
        }
    }
    if (groundZ == 1e9f) {
        delete vehicle;
        return nullptr;
    }

    vehicle->SetPosn(origin.x, origin.y, groundZ + vehicle->GetDistanceFromCentreOfMassToBaseOfModel());

    vehicle->m_vecMoveSpeed = CVector{};
    vehicle->PlaceOnRoadProperly();
    CWorld::Add(vehicle);

    switch (modelId) {
    case MODEL_FIRETRUK: {
        vehicle->vehicleFlags.bIsFireTruckOnDuty = true;
        NumFireTrucksOnDuty++;
        CCarAI::AddFiretruckOccupants(vehicle);
        break;
    }
    case MODEL_AMBULAN: {
        vehicle->vehicleFlags.bIsAmbulanceOnDuty = true;
        NumAmbulancesOnDuty++;
        CCarAI::AddAmbulanceOccupants(vehicle);
        break;
    }
    case MODEL_ENFORCER:
    case 523: // Police Maverick
    case 596: // Police Car (LSPD)
    case 597: // Police Car (SFPD)
    case 598: // Police Car (LVPD)
    case 599: // Police Ranger
        CCarAI::AddPoliceCarOccupants(vehicle, false);
        break;
    }

    vehicle->vehicleFlags.bTakeLessDamage = true;
    return vehicle;
}

// 0x430050
void CCarCtrl::GenerateOneRandomCar() {
    //! NOTE: These are globals initialized at runtime by other systems (No owning class known)
    static auto& s_SpawnDirX               = StaticRef<float>(0xB6F104);
    static auto& s_SpawnDirY               = StaticRef<float>(0xB6F108);
    static auto& s_SpawnRadiusScale        = StaticRef<float>(0xB6F11C);
    static auto& s_SpawnDirectionThreshold = StaticRef<float>(0xB6F9B4);
    static auto& s_CameraPosition          = StaticRef<CVector>(0xB6F02C);
    static auto& s_CameraEntity            = StaticRef<CEntity*>(0xB6F03C);
    static auto& s_ForceStationarySpawn    = StaticRef<bool>(0x96914F);
    static auto& s_AlwaysGiveUpgrade       = StaticRef<bool>(0x969176);
    static auto& s_UpgradeModelId          = StaticRef<int32>(0x8CD76C);
    static auto& s_BlockBoatModelRequest   = StaticRef<uint8>(0x8E6E68);

    CNodeAddress node1{}, node2{};

    const CVector playerPos   = FindPlayerCentreOfWorld(CWorld::PlayerInFocus);
    const CVector playerSpeed = FindPlayerSpeed(-1); //!< NOTE: Not used here, but the original reads it
    (void)playerSpeed;

    //! Density gate
    float density = CarDensityMultiplier;
    if (CCullZones::FewerCars()) {
        density *= 0.6f;
    }

    const float numCarsInUse = (float)(int32)(NumFireTrucksOnDuty + NumAmbulancesOnDuty + NumMissionCars + NumLawEnforcerCars + NumRandomCars);
    if (CPopulation::FindCarMultiplierMotorway() * (float)MaxNumberOfCarsInUse * density > numCarsInUse) {
        return;
    }
    if ((float)(CPopCycle::m_NumOther_Cars + CPopCycle::m_NumCops_Cars + CPopCycle::m_NumGangs_Cars + CPopCycle::m_NumDealers_Cars) * CPopulation::FindCarMultiplierMotorway() * density > numCarsInUse) {
        return;
    }

    //! Choose the model. Police cars are spawned when the player is wanted (And there's room for them)
    int32 model, modelClass;
    {
        const CWanted* const wanted = FindPlayerWanted(-1);
        const int32          wantedLevel = (int32)wanted->GetWantedLevel();

        if (   wantedLevel < 2
            || (int32)wanted->m_MaxCopCarsInPursuit <= (int32)NumLawEnforcerCars
            || (int32)wanted->m_MaxCopsInPursuit <= (int32)wanted->m_NumCopsInPursuit
            || CGame::currArea != AREA_CODE_NORMAL_WORLD
            || CGangWars::GangWarFightingGoingOn()
            || (   wantedLevel < 4
                && (   wantedLevel < 3
                    || CTimer::GetTimeInMS() <= (uint32)(LastTimeLawEnforcerCreated + 5000))
                && CTimer::GetTimeInMS() <= (uint32)(LastTimeLawEnforcerCreated + 8000))
        ) {
            model = ChooseModel(&modelClass);
            if (model == -1) {
                return;
            }
            if ((modelClass == 13 || modelClass == 24) && wantedLevel > 0) {
                return;
            }
        } else {
            model      = ChoosePoliceCarModel(0);
            modelClass = 13;
        }
    }

    //! Riots: Spawn more police cars
    if (CGameLogic::LaRiotsActiveHere() && !gbLARiots_NoPoliceCars && (CGeneral::GetRandomNumber() & 0x7F) < 0x37) {
        model      = ChoosePoliceCarModel(0);
        modelClass = 13;
    }

    //! Spawn direction
    CVector dir{};
    float   spawnAngle;          //!< `local_11c` in the original
    bool    bAlongRoad;          //!< `local_110` in the original
    bool    bSkipChecks = false; //!< `local_105` in the original

    if (s_SpawnDirectionThreshold > -0.9f) {
        const CVehicle* const playerVeh = FindPlayerVehicle(-1, false);
        float speed = 0.0f;
        if (playerVeh) {
            dir.x = playerVeh->m_vecMoveSpeed.x;
            dir.y = playerVeh->m_vecMoveSpeed.y;
            speed = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        }

        if (playerVeh && speed > 0.1f) {
            dir.x /= speed;
            dir.y /= speed;

            const bool bFast = speed > 0.4f;
            switch (CTimer::GetFrameCounter() & 3) {
            case 0:
                spawnAngle = 0.85f;
                bAlongRoad = true;
                break;
            case 1:
                spawnAngle = bFast ? 0.85f : 0.707f;
                bAlongRoad = true;
                break;
            case 2:
                spawnAngle = 0.707f;
                bAlongRoad = bFast;
                break;
            default: // 3
                spawnAngle = 0.707f;
                bAlongRoad = false;
                break;
            }
        } else {
            //! On foot, or barely moving
            dir.x      = s_SpawnDirX;
            dir.y      = s_SpawnDirY;
            spawnAngle = 0.707f;
            bAlongRoad = !(CTimer::GetFrameCounter() & 1);
        }
    } else {
        bSkipChecks = true;
        dir.x = dir.y = 0.707f;
        spawnAngle    = -1.0f;
        bAlongRoad    = true;
    }

    //! NOTE: The original passes `&dir.y` for `arg11` too (The value gets overwritten by the callee)
    CVector origin;
    if (!GenerateCarCreationCoors2(
            playerPos,
            dir.x,
            dir.y,
            spawnAngle,
            bAlongRoad,
            s_SpawnRadiusScale * 160.0f,
            40.0f,
            &origin,
            &node1,
            &node2,
            &dir.y,
            !(modelClass == 13 && (int32)FindPlayerWanted(-1)->GetWantedLevel() > 0),
            false
        )
    ) {
        return;
    }

    const CPathNode& nodeA    = ThePaths.m_pPathNodes[node1.m_wAreaId][node1.m_wNodeId];
    const CPathNode& nodeB    = ThePaths.m_pPathNodes[node2.m_wAreaId][node2.m_wNodeId];
    const CVector    nodePosA = nodeA.GetPosition();
    const CVector    nodePosB = nodeB.GetPosition();

    //! Spawn probability of the nodes limits the chance of a car spawning here
    if (std::min<uint8>(nodeA.m_nSpawnProbability, nodeB.m_nSpawnProbability) < (CGeneral::GetRandomNumber() & 0xF)) {
        return;
    }

    //! Water nodes spawn boats
    bool  bWaterNode;
    float spawnRadius;
    if (nodeA.m_bWaterNode) {
        bWaterNode = true;
        if (modelClass == 13) {
            model      = 430; //!< `PREDATOR`
            modelClass = 24;
            if (s_BlockBoatModelRequest != 1) {
                CStreaming::RequestModel(430, 8); //!< NOTE: The original uses flag 8 here
                return;
            }
        } else {
            model = (int32)CPopulation::m_LoadedBoats.PickLeastUsedModel(1);
            if (model == -1 || !CStreaming::ms_aInfoForModel[model].IsLoaded()) {
                return;
            }
        }
        spawnRadius = 40.0f;
    } else {
        bWaterNode  = false;
        spawnRadius = 8.0f;
    }

    //! Don't spawn on top of anything
    int16 numColliding = 0;
    CWorld::FindObjectsKindaColliding(origin, spawnRadius, true, &numColliding, 2, nullptr, false, true, true, false, false);
    if (numColliding) {
        return;
    }

    //! Find the navi-link (at `node1`) going towards `node2`
    int16 linkIdx = 0;
    for (; linkIdx < nodeA.m_nNumLinks; linkIdx++) {
        if (ThePaths.m_pNodeLinks[node1.m_wAreaId][nodeA.m_wBaseLinkId + linkIdx] == node2) {
            break;
        }
    }
    const CCarPathLinkAddress naviLinkAddr = ThePaths.m_pNaviLinks[node1.m_wAreaId][nodeA.m_wBaseLinkId + linkIdx];
    const CCarPathLink&      naviLink      = ThePaths.GetCarPathLink(naviLinkAddr);

    const uint8 numLanes = naviLink.m_attachedTo == node2 ? naviLink.m_numSameDirLanes : naviLink.m_numOppositeDirLanes;
    if (numLanes < 2) {
        if (model == 430 || model == 431) { //!< Boats don't belong here
            return;
        }
    } else if (CModelInfo::GetModelInfo(model)->AsVehicleModelInfoPtr()->m_nVehicleType == VEHICLE_TYPE_BMX) {
        return;
    }
    if (numLanes == 0) {
        return;
    }

    //! Some zones only spawn cars of their own type
    bool bRestrictedZone = false;
    if (CPopCycle::m_pCurrZoneInfo) {
        const uint8 zoneType = (uint8)CTheZones::GetZoneInfo(origin, nullptr)->PopType;
        if (zoneType > (uint8)eZonePopulationType::AIRPORT && zoneType <= (uint8)eZonePopulationType::AIRPORT_RUNWAY) {
            if (zoneType != (uint8)CPopCycle::m_nCurrentZoneType) {
                return;
            }
            bRestrictedZone = true;
        }
    }

    //! Create the vehicle
    CVehicle* const veh = GetNewVehicleDependingOnCarModel(model, 1);
    auto&           autoPilot = veh->m_autoPilot;

    autoPilot.m_endingRouteNode.ResetAreaId();
    autoPilot.m_currentAddress    = node1;
    autoPilot.m_startingRouteNode = node2;

    if (modelClass == 13) { //!< Police
        autoPilot.m_nTempAction = TEMPACT_NONE;
        if ((int32)FindPlayerWanted(-1)->GetWantedLevel() == 0) {
            autoPilot.SetCruiseSpeed((uint32)CGeneral::GetRandomNumberInRange(19.0f, 24.0f));
            autoPilot.m_nCarDrivingStyle = eCarDrivingStyle::DRIVING_STYLE_STOP_FOR_CARS;
            autoPilot.m_nCarMission      = MISSION_CRUISE;
        } else {
            autoPilot.SetCruiseSpeed(CCarAI::FindPoliceCarSpeedForWantedLevel(veh));
            autoPilot.m_nCarMission      = veh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE
                ? CCarAI::FindPoliceBikeMissionForWantedLevel()
                : CCarAI::FindPoliceCarMissionForWantedLevel();
            autoPilot.m_nCarDrivingStyle = eCarDrivingStyle::DRIVING_STYLE_AVOID_CARS;
        }
        if (model == 490) { //!< Unmarked police car, no colours
            veh->m_nPrimaryColor   = 0;
            veh->m_nSecondaryColor = 0;
        }
        veh->vehicleFlags.bCreatedAsPoliceVehicle = true;
    } else if (modelClass == 24) { //!< Police boat
        autoPilot.m_nTempAction = TEMPACT_NONE;
        autoPilot.SetCruiseSpeed((uint32)CGeneral::GetRandomNumberInRange(14.0f, 19.0f));
        autoPilot.m_nCarDrivingStyle = eCarDrivingStyle::DRIVING_STYLE_AVOID_CARS;
        autoPilot.m_nCarMission      = CCarAI::FindPoliceBoatMissionForWantedLevel();
        veh->vehicleFlags.bCreatedAsPoliceVehicle = true;
    } else {
        autoPilot.SetCruiseSpeed((uint32)CGeneral::GetRandomNumberInRange(13.0f, 21.0f));
        switch (modelClass) {
        case 3: //!< Heli
            autoPilot.SetCruiseSpeed((uint32)CGeneral::GetRandomNumberInRange(19.0f, 27.0f));
            break;
        case 1: //!< Monster truck
            autoPilot.SetCruiseSpeed((uint32)CGeneral::GetRandomNumberInRange(10.0f, 15.0f));
            break;
        }

        //! Boats and planes are slower than cars
        const CColModel* const colModel = CModelInfo::GetModelInfo(model)->GetColModel();
        if (colModel->GetBoundingBox().m_vecMax.y - colModel->GetBoundingBox().m_vecMin.y > 10.0f || modelClass == 5) {
            autoPilot.SetCruiseSpeed((uint32)autoPilot.m_nCruiseSpeed * 3 / 4);
        }

        if (bWaterNode) {
            if (veh->m_nModelIndex == 446 || veh->m_nModelIndex == 452 || veh->m_nModelIndex == 493) { //!< `0x1BE`, `0x1C4`, `0x1ED`
                autoPilot.SetCruiseSpeed((uint32)CGeneral::GetRandomNumberInRange(25.0f, 35.0f));
            } else {
                autoPilot.SetCruiseSpeed((uint32)CGeneral::GetRandomNumberInRange(15.0f, 24.0f));
            }
        }

        autoPilot.m_nCarMission      = MISSION_CRUISE;
        autoPilot.m_nTempAction      = TEMPACT_NONE;
        autoPilot.m_nCarDrivingStyle = eCarDrivingStyle::DRIVING_STYLE_STOP_FOR_CARS;
    }

    if (veh->m_nModelIndex == 423) { //!< `0x1A7`
        veh->vehicleFlags.bSirenOrAlarm = true;
    }

    autoPilot.m_nNextPathNodeInfo = naviLinkAddr;

    const uint8 lane = (uint8)(CGeneral::GetRandomNumber() % numLanes);
    autoPilot.m_nCurrentLane = lane;
    autoPilot.m_nNextLane    = lane;

    //! Some cars are parked (i.e. they don't move)
    bool bStationary = false;
    {
        const int32 parkedChance = CGameLogic::LaRiotsActiveHere()
            ? 0x50
            : veh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE
                ? 0x32
                : veh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BOAT
                    ? 10
                    : 200;
        if (!bWaterNode && modelClass != 13 && !bRestrictedZone) {
            if (CGeneral::GetRandomNumberInRange(0, parkedChance) == 0 || s_ForceStationarySpawn) {
                bStationary = true;
                dir.y       = 1.0f;
            }
        }
    }

    //! Where along the link should we spawn?
    {
        const float linkDist = std::sqrt(
            (nodePosA.x - nodePosB.x) * (nodePosA.x - nodePosB.x) +
            (nodePosA.y - nodePosB.y) * (nodePosA.y - nodePosB.y)
        );
        const float carLen = (CModelInfo::GetModelInfo(model)->GetColModel()->GetBoundingBox().m_vecMax.y
                              - CModelInfo::GetModelInfo(model)->GetColModel()->GetBoundingBox().m_vecMin.y) * 0.5f + 1.0f;
        if (carLen <= linkDist * 0.5f) {
            const float ratio = carLen / linkDist;
            dir.y             = std::max(dir.y, ratio);
            if (dir.y >= 1.0f - ratio) {
                dir.y = 1.0f - ratio;
            }
        } else {
            dir.y = 0.5f;
        }
    }
    const float lateralPct = dir.y;

    autoPilot._smthNext = IsNodeAddressLess(node1, node2) ? -1 : 1;

    if (nodeA.m_nNumLinks == 1) {
        delete veh;
        return;
    }

    //! Pick a random other link at `node1` as the "current" one
    {
        CCarPathLinkAddress randLink;
        do {
            randLink = ThePaths.m_pNaviLinks[node1.m_wAreaId][nodeA.m_wBaseLinkId + CGeneral::GetRandomNumber() % nodeA.m_nNumLinks];
        } while ((CNodeAddress)randLink == (CNodeAddress)autoPilot.m_nNextPathNodeInfo);

        autoPilot.m_nCurrentPathNodeInfo = randLink;
        if (!ThePaths.m_pPathNodes[randLink.m_wAreaId]) {
            delete veh;
            return;
        }
    }

    autoPilot._smthCurr = IsNodeAddressLess(node1, node2) ? -1 : 1;

    //! Build the spawn matrix along the link
    {
        CVector rawDir = nodePosB - nodePosA;
        CVector fwd    = rawDir;
        fwd.Normalise();

        CVector right{rawDir.y, -rawDir.x, 0.0f};
        right.Normalise();

        CMatrix& m = veh->GetMatrix();
        m.GetForward() = fwd;
        m.GetRight()   = right;
        m.GetUp()      = CVector{0.0f, 0.0f, 1.0f};
    }

    //! Lane offsets
    const CCarPathLink& curLink  = ThePaths.GetCarPathLink(autoPilot.m_nCurrentPathNodeInfo);
    const CCarPathLink& nextLink = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo);

    const float curDirX  = (float)curLink.m_dir.x * 0.01f * (float)autoPilot._smthCurr;
    const float curDirY  = (float)curLink.m_dir.y * 0.01f * (float)autoPilot._smthCurr;
    const float nextDirX = (float)nextLink.m_dir.x * 0.01f * (float)autoPilot._smthNext;
    const float nextDirY = (float)nextLink.m_dir.y * 0.01f * (float)autoPilot._smthNext;

    float curLaneOffset  = (curLink.OneWayLaneOffset() + (float)autoPilot.m_nCurrentLane) * 5.4f;
    float nextLaneOffset = (nextLink.OneWayLaneOffset() + (float)autoPilot.m_nNextLane) * 5.4f;
    if (veh->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
        curLaneOffset  += 1.458f;
        nextLaneOffset += 1.458f;
    }

    const CVector curLanePos {
        (float)curLink.m_posn.x * 0.125f + curLaneOffset * curDirY,
        (float)curLink.m_posn.y * 0.125f - curLaneOffset * curDirX,
        0.0f
    };
    const CVector nextLanePos{
        (float)nextLink.m_posn.x * 0.125f + nextLaneOffset * nextDirY,
        (float)nextLink.m_posn.y * 0.125f - nextLaneOffset * nextDirX,
        0.0f
    };

    const float speedScale = CCurves::CalcSpeedScaleFactor(
        curLanePos, nextLanePos,
        curDirX, curDirY,
        nextDirX, nextDirY
    );

    autoPilot.m_nSpeedScaleFactor = (uint32)(speedScale * 1000.0f / autoPilot.m_speed);
    autoPilot.field_C             = (int32)((float)autoPilot.m_nSpeedScaleFactor - spawnAngle * (float)CTimer::GetTimeInMS());

    const float curveTime = (float)(uint32)(CTimer::GetTimeInMS() - (uint32)autoPilot.field_C) / (float)autoPilot.m_nSpeedScaleFactor;

    CVector curveSpeed;
    CCurves::CalcCurvePoint(
        curLanePos, nextLanePos,
        CVector{curDirX, curDirY, 0.0f}, CVector{nextDirX, nextDirY, 0.0f},
        curveTime,
        (int32)autoPilot.m_nSpeedScaleFactor,
        origin,
        curveSpeed
    );

    //! Place the car a little bit along the link, in the direction of travel
    CVector spawnPos = nodePosB + (nodePosB - nodePosA).Normalized() * 2.0f;
    spawnPos.z       = (1.0f - lateralPct) * nodePosA.z + lateralPct * nodePosB.z;

    //! Snap to the ground/water
    const CVector velocity = curveSpeed * (1.0f / 60.0f) - (origin - playerPos);

    {
        float groundZ = 1e9f;
        if (bWaterNode) {
            if (!CWaterLevel::GetWaterLevel(spawnPos, groundZ, true)) {
                delete veh;
                return;
            }
        } else {
            CColPoint colPoint{};
            CEntity*  colEntity{};
            if (CWorld::ProcessVerticalLine(spawnPos, 1000.0f, colPoint, colEntity, true, false, false, false, true, false, nullptr)) {
                groundZ = colPoint.m_vecPoint.z;
            }
            if (CWorld::ProcessVerticalLine(spawnPos, -1000.0f, colPoint, colEntity, true, false, false, false, true, false, nullptr)) {
                if (std::abs(colPoint.m_vecPoint.z - spawnPos.z) < std::abs(groundZ - spawnPos.z)) {
                    groundZ = colPoint.m_vecPoint.z;
                }
            }
        }

        if (groundZ == 1e9f || std::abs(groundZ - spawnPos.z) > 7.0f) {
            delete veh;
            return;
        }

        if (CModelInfo::IsBoatModel(veh->m_nModelIndex)) {
            spawnPos.z = groundZ;
            veh->m_nExtendedRemovalRange = 0xFF;
        } else {
            spawnPos.z = groundZ + veh->GetDistanceFromCentreOfMassToBaseOfModel();
        }
    }

    veh->SetPosn(spawnPos);

    veh->m_vecMoveSpeed     = CVector{};
    veh->m_autoPilot.m_speed = 0.0f;

    //! Police cars are always "physics"
    if (modelClass == 13) {
        veh->SetStatus(autoPilot.m_nCarMission == MISSION_CRUISE ? STATUS_SIMPLE : STATUS_PHYSICS);
    } else if (modelClass == 24 || bWaterNode) {
        veh->SetStatus(STATUS_PHYSICS);
    } else if (veh->GetStatus() != STATUS_PHYSICS) {
        veh->SetStatus(STATUS_SIMPLE);
    }

    CVisibilityPlugins::SetClumpAlpha(veh->GetRpClump(), 0);

    if (s_AlwaysGiveUpgrade && veh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
        veh->AddVehicleUpgrade(s_UpgradeModelId);
    }

    //! Distance checks around the camera
    const CVector spawnToPlayer = origin - playerPos;
    bool          bSkipCollisionCheck = false;
    {
        const float dist         = spawnToPlayer.Magnitude();
        const float removalRange = std::max((float)veh->m_nExtendedRemovalRange, 170.0f);

        if (veh->GetIsOnScreen()) {
            if (removalRange * s_SpawnRadiusScale < dist || dist < s_SpawnRadiusScale * 150.0f) {
                delete veh;
                return;
            }

            const CVector camPos = s_CameraEntity ? s_CameraEntity->GetPosition() : s_CameraPosition;
            if ((veh->GetPosition() - camPos).Magnitude() < s_SpawnRadiusScale * 120.0f || bSkipChecks) {
                bSkipCollisionCheck = true;
            } else if (veh->m_nModelIndex == 484) { //!< `0x1E4`
                delete veh;
                return;
            }
        } else if (removalRange * (1.0f / 170.0f) * 45.0f < dist && !bSkipChecks) {
            delete veh;
            return;
        }
    }

    //! Don't spawn on top of anything, nor facing towards the player
    if (!bSkipCollisionCheck) {
        const CColModel* const colModel = CModelInfo::GetModelInfo(veh->m_nModelIndex)->GetColModel();
        int16                  colliding = 0;
        CWorld::FindObjectsKindaColliding(veh->GetPosition(), colModel->GetBoundingBox().m_vecMax.z, true, &colliding, 2, nullptr, false, true, true, false, false);
        if (colliding || 0.0f <= velocity.x * spawnToPlayer.x + velocity.y * spawnToPlayer.y) {
            delete veh;
            return;
        }
    }

    veh->GetVehicleModelInfo()->ChooseVehicleColour(veh->m_nPrimaryColor, veh->m_nSecondaryColor, veh->m_nTertiaryColor, veh->m_nQuaternaryColor, 1);
    CWorld::Add(veh);

    if (veh->m_nModelIndex == 531 || veh->m_nModelIndex == 532 || veh->m_nVehicleSubType == VEHICLE_TYPE_BMX) { //!< `0x213`, `0x214`
        autoPilot.m_nCruiseSpeed /= 3;
    }

    if (CGameLogic::LaRiotsActiveHere()) {
        veh->m_fDirtLevel = (float)(CGeneral::GetRandomNumber() % 1000);
    }

    if (modelClass == 13) {
        LastTimeLawEnforcerCreated = CTimer::GetTimeInMS();
    }

    if (veh->m_nModelIndex == 457) { //!< `0x1C9`
        veh->SetStatus(STATUS_PHYSICS);
        autoPilot.m_nCarDrivingStyle = (eCarDrivingStyle)2;
    }

    if (veh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
        switch (modelClass) {
        case 0: case 4: case 5: case 6: case 13:
            if (CGeneral::GetRandomNumberInRange(0, 20) == 0) {
                veh->AsAutomobile()->SetRandomDamage(false);
            }
            break;
        case 1: case 14: case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
            if (CGeneral::GetRandomNumberInRange(0, 8) == 0) {
                veh->AsAutomobile()->SetRandomDamage(true);
            }
            break;
        }
    }

    if (veh->m_nVehicleSubType == VEHICLE_TYPE_BIKE && autoPilot.m_nCarDrivingStyle == eCarDrivingStyle::DRIVING_STYLE_STOP_FOR_CARS) {
        veh->SetStatus(STATUS_PHYSICS);
        autoPilot.m_nCarDrivingStyle = (eCarDrivingStyle)6;
    }

    //! Mad driver chase
    if (!bWaterNode && modelClass != 13 && !bRestrictedZone) {
        if (   FindPlayerPed(-1)->GetPlayerWanted()->GetWantedLevel() == eWantedLevel::WANTED_CLEAN
            && (s_ForceStationarySpawn || TimeNextMadDriverChaseCreated <= 0.0f)
            && CreatePoliceChase(veh, modelClass, node1)
        ) {
            TimeNextMadDriverChaseCreated = CGameLogic::LaRiotsActiveHere()
                ? CGeneral::GetRandomNumberInRange(240.0f, 480.0f)
                : CGeneral::GetRandomNumberInRange(600.0f, 1200.0f);
        }
    }

    //! Occupants
    if (!bStationary) {
        if (modelClass == 13 || modelClass == 24) {
            CCarAI::AddPoliceCarOccupants(veh, false);
        } else {
            bCarIsBeingCreated = true;
            SetUpDriverAndPassengersForVehicle(veh, modelClass, 0, false, false, 99);
            bCarIsBeingCreated = false;
        }
    } else if (   (veh->m_nModelIndex == 431 || veh->m_nModelIndex == 429 || veh->m_nModelIndex == 521 || veh->m_nModelIndex == 522
                || veh->m_nModelIndex == 581 || veh->m_nModelIndex == 586) //!< `0x1AF`, `0x1AD`, `0x209`, `0x20A`, `0x245`, `0x24A`
               && CGame::currArea == AREA_CODE_NORMAL_WORLD
               && CGeneral::GetRandomNumberInRange(0, 7) == 0
               && CreateConvoy(veh, modelClass)
    ) {
        SetUpDriverAndPassengersForVehicle(veh, modelClass, 1, true, false, 99);
    } else {
        SetUpDriverAndPassengersForVehicle(veh, modelClass, 1, true, false, 99);

        autoPilot.m_nCruiseSpeed     = (uint8)CGeneral::GetRandomNumber();
        veh->SetStatus(STATUS_PHYSICS);
        autoPilot.m_nCarDrivingStyle = (eCarDrivingStyle)2;

        veh->m_vecMoveSpeed = veh->GetForward() * ((float)autoPilot.m_nCruiseSpeed * 0.02f);

        // TODO(re): The original also sets a flag (bit `0x800`) on the driver (At `CPed + 0x478`)
        //           when riots are going on - That flag is not identified in the repo yet.
    }

    if (modelClass == 13 || modelClass == 24) {
        veh->ChangeLawEnforcerState(true);
    }

    CStreaming::PossiblyStreamCarOutAfterCreation(veh->m_nModelIndex);

    //! 0x421120 - Increase the "no cars" counter (Used by the population code)
    autoPilot.field_50 = (char)std::min<int32>((int32)autoPilot.field_50 + 1, 0x78);
}



// 0x4341C0
void CCarCtrl::GenerateRandomCars() {
    if (CCutsceneMgr::ms_running) {
        CountDownToCarsAtStart = 2;
        return;
    }
    if (CGangWars::DontCreateCivilians() || !CGame::CanSeeOutSideFromCurrArea()) {
        return;
    }

    if (CGameLogic::LaRiotsActiveHere() && TimeNextMadDriverChaseCreated > 480.0f) {
        TimeNextMadDriverChaseCreated = CGeneral::GetRandomNumberInRange(240.0f, 480.0f);
    }
    TimeNextMadDriverChaseCreated -= (CTimer::GetTimeStep() * 0.02f);

    if (NumRandomCars < 45) {
        if (CountDownToCarsAtStart) {
            if (--CountDownToCarsAtStart == 0) {
                for (auto i = 100; i --> 0;) {
                    GenerateOneRandomCar();
                }
                CTheCarGenerators::GenerateEvenIfPlayerIsCloseCounter = 20;
            }
        } else {
            GenerateOneRandomCar();
            GenerateOneRandomCar();
        }
    }

    CTrain::DoTrainGenerationAndRemoval();
    CPlane::DoPlaneGenerationAndRemoval();

    // 0x434277 - About every 4 seconds
    if (CTimer::GetTimeInMS() / 4096u != CTimer::GetPreviousTimeInMS() / 4096u) {
        GenerateEmergencyServicesCar();
    }
}

// 0x42F3C0
void CCarCtrl::GetAIHeliToAttackPlayer(CAutomobile* automobile) {
    CHeli* const  heli        = automobile->AsHeli();
    const CVector playerCoors = FindPlayerCoors(-1);
    const CVector pos         = heli->GetPosition();

    const float dx     = playerCoors.x - pos.x;
    const float dy     = playerCoors.y - pos.y;
    float       angle  = CGeneral::GetATanOfXY(dx, dy);
    float       dist2D = std::sqrt(dx * dx + dy * dy);

    heli->m_fMaxAltitude = playerCoors.z;
    heli->m_autoPilot.m_vecDestinationCoors = playerCoors;

    if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_ATTACK_PLAYER) {
        //! Close in: break off and strafe
        if (dist2D < 15.0f) {
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_ATTACK_PLAYER_FLY_AWAY;
        }
        dist2D += 50.0f;
    } else if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_ATTACK_PLAYER_FLY_AWAY) {
        //! Far enough: Come back in, from the opposite direction
        if (dist2D > 18.0f) {
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_ATTACK_PLAYER;
        }
        angle += PI;
    }

    FlyAIHeliInCertainDirection(heli, angle, dist2D, false);

    TestWhetherToFirePlaneGuns(automobile, FindPlayerEntity(-1));
    FireHeliRocketsAtTarget(automobile, FindPlayerEntity(-1));
}

// 0x42A730
void CCarCtrl::GetAIHeliToFlyInDirection(CAutomobile* automobile) {
    CHeli* const heli = automobile->AsHeli();
    FlyAIHeliInCertainDirection(heli, heli->field_9B4, 1000.0f, false);
}

// 0x429780
void CCarCtrl::GetAIPlaneToAttackPlayer(CAutomobile* automobile) {
    CPlane* const plane = automobile->AsPlane();

    const CVector playerSpeed = FindPlayerSpeed(-1);
    const CVector playerPos   = FindPlayerCoors(-1);
    const CVector pos         = plane->GetPosition();

    plane->m_planeHeading = CGeneral::GetATanOfXY(
        (playerSpeed.x * 50.0f + playerPos.x) - pos.x,
        (playerSpeed.y * 50.0f + playerPos.y) - pos.y
    );
    plane->m_maxAltitude = playerSpeed.z * 50.0f + playerPos.z;

    FlyAIPlaneInCertainDirection(plane);

    if (CVehicle* const playerVeh = FindPlayerVehicle(-1, false)) {
        if (playerVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE) {
            TriggerDogFightMoves(plane, FindPlayerVehicle(-1, false));
        }
    }

    TestWhetherToFirePlaneGuns(plane, FindPlayerVehicle(-1, false));
    PossiblyFireHSMissile(plane, FindPlayerVehicle(-1, false));
}

// 0x429890
void CCarCtrl::GetAIPlaneToDoDogFight(CAutomobile* automobile) {
    CPlane* const   plane  = automobile->AsPlane();
    CVehicle* const target = plane->m_autoPilot.m_TargetEntity;

    const CVector targetPos   = target->GetPosition();
    const CVector targetSpeed = target->m_vecMoveSpeed;
    const CVector aim         = targetPos + targetSpeed * 50.0f;
    const CVector pos         = plane->GetPosition();

    if (plane->m_autoPilot.m_bPlaneDogfightSomething) {
        //! Fly to the previously chosen dogfight point
        plane->m_planeHeading = CGeneral::GetATanOfXY(plane->m_autoPilot.m_vecDestinationCoors.x - pos.x, plane->m_autoPilot.m_vecDestinationCoors.y - pos.y);
        plane->m_maxAltitude  = plane->m_autoPilot.m_vecDestinationCoors.z;

        const float dx = plane->m_autoPilot.m_vecDestinationCoors.x - pos.x;
        const float dy = plane->m_autoPilot.m_vecDestinationCoors.y - pos.y;
        if (std::sqrt(dx * dx + dy * dy) < 50.0f) {
            plane->m_autoPilot.m_bPlaneDogfightSomething = false;
        }
    } else {
        plane->m_planeHeading = CGeneral::GetATanOfXY(aim.x - pos.x, aim.y - pos.y);
        plane->m_maxAltitude  = aim.z;

        if ((CGeneral::GetRandomNumber() & 0x3FF) == 500) {
            plane->m_autoPilot.m_bPlaneDogfightSomething = true;
            plane->m_autoPilot.m_vecDestinationCoors.x = (float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 600.0f + aim.x - 300.0f;
            plane->m_autoPilot.m_vecDestinationCoors.y = (float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 600.0f + aim.y - 300.0f;
            plane->m_autoPilot.m_vecDestinationCoors.z = aim.z + 50.0f;
        }
    }

    FlyAIPlaneInCertainDirection(plane);
    TestWhetherToFirePlaneGuns(plane, target);
    PossiblyFireHSMissile(plane, target);
}

// 0x42F370
void CCarCtrl::GetAIPlaneToDoDogFightAgainstPlayer(CAutomobile* automobile) {
    if (CVehicle* const playerVeh = FindPlayerVehicle(-1, false)) {
        automobile->m_autoPilot.m_TargetEntity = playerVeh;
    } else {
        //! NOTE: The target may be a ped too, hence the `CVehicle` cast
        automobile->m_autoPilot.m_TargetEntity = reinterpret_cast<CVehicle*>(FindPlayerPed(-1));
    }

    GetAIPlaneToDoDogFight(automobile);
}

// 0x421440
CVehicle* CCarCtrl::GetNewVehicleDependingOnCarModel(int32 modelId, uint8 createdBy) {
    const auto eCreatedBy = static_cast<eVehicleCreatedBy>(createdBy);
    switch (CModelInfo::GetModelInfo(modelId)->AsVehicleModelInfoPtr()->m_nVehicleType) {
    case VEHICLE_TYPE_MTRUCK:
        if (auto* vehicle = new CMonsterTruck(modelId, eCreatedBy)) {
            return vehicle;
        }
        break;
    case VEHICLE_TYPE_QUAD:
        if (auto* vehicle = new CQuadBike(modelId, eCreatedBy)) {
            return vehicle;
        }
        break;
    case VEHICLE_TYPE_HELI:
        if (auto* vehicle = new CHeli(modelId, eCreatedBy)) {
            return vehicle;
        }
        break;
    case VEHICLE_TYPE_PLANE:
        if (auto* vehicle = new CPlane(modelId, eCreatedBy)) {
            return vehicle;
        }
        break;
    case VEHICLE_TYPE_BOAT:
        if (auto* vehicle = new CBoat(modelId, eCreatedBy)) {
            return vehicle;
        }
        break;
    case VEHICLE_TYPE_TRAIN:
        if (auto* vehicle = new CTrain(modelId, eCreatedBy)) {
            return vehicle;
        }
        break;
    case VEHICLE_TYPE_BIKE:
        if (auto* bike = new CBike(modelId, eCreatedBy)) {
            bike->bikeFlags.bOnSideStand = true;
            return bike;
        }
        break;
    case VEHICLE_TYPE_BMX:
        if (auto* bike = new CBmx(modelId, eCreatedBy)) {
            bike->bikeFlags.bOnSideStand = true;
            return bike;
        }
        break;
    case VEHICLE_TYPE_TRAILER:
        if (auto* vehicle = new CTrailer(modelId, eCreatedBy)) {
            return vehicle;
        }
        break;
    default: //!< Includes `VEHICLE_TYPE_AUTOMOBILE`, and anything else (7, 8)
        if (auto* vehicle = new CAutomobile(modelId, eCreatedBy, true)) {
            return vehicle;
        }
        break;
    }
    return nullptr;
}

// 0x42C250
bool CCarCtrl::IsAnyoneParking() {
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        switch (veh.m_autoPilot.m_nCarMission) {
        case eCarMission::MISSION_PARK_PARALLEL:
        case eCarMission::MISSION_PARK_PARALLEL_2:
        case eCarMission::MISSION_PARK_PERPENDICULAR:
        case eCarMission::MISSION_PARK_PERPENDICULAR_2:
            return true;
        }
    }
    return false;
}

// 0x42DAB0
bool CCarCtrl::IsThisAnAppropriateNode(CVehicle* vehicle, CNodeAddress cur, CNodeAddress prev, CNodeAddress next, bool arg5) {
    CPathNode* const nextArea = ThePaths.m_pPathNodes[next.m_wAreaId];
    if (!nextArea || cur == next) {
        return false;
    }

    const CPathNode* const nextNode = &nextArea[next.m_wNodeId];
    const CPathNode* const prevNode = &ThePaths.m_pPathNodes[prev.m_wAreaId][prev.m_wNodeId];

    //! Water <-> land transitions are only allowed for the hovercraft
    if (prevNode->m_bWaterNode != nextNode->m_bWaterNode && vehicle->m_nModelIndex != 539) {
        return false;
    }

    const auto DoesItFit = [&] {
        int16 numFound = 0;
        CWorld::FindObjectsKindaColliding(nextNode->GetPosition(), 5.0f, true, &numFound, 2, nullptr, false, false, true, true, false);
        return numFound == 0;
    };

    const CColModel* const colModel = vehicle->GetColModel();

    switch (nextNode->m_nBehaviourType) {
    case 1: { //!< Roadblock
        if (vehicle->m_nCreatedBy == MISSION_VEHICLE) {
            return false;
        }
        if (vehicle->m_pDriver && vehicle->m_pDriver->GetCreatedBy() == PED_MISSION) {
            return false;
        }
        if (vehicle->GetVehicleModelInfo()->m_nVehicleClass == VEHICLE_CLASS_BIG) {
            return false;
        }
        if (!arg5) {
            //! Reject if the node lies behind us
            const CVector cross = CrossProduct(
                nextNode->GetPosition() - prevNode->GetPosition(),
                nextNode->GetPosition() - vehicle->GetPosition()
            );
            if (cross.z < 0.0f) {
                return false;
            }
        }
        if (vehicle->IsLawEnforcementVehicle()) {
            return false;
        }
        if (vehicle->GetVehicleModelInfo()->m_nVehicleClass == VEHICLE_CLASS_BIG) {
            return false;
        }
        if (vehicle->m_pDriver && vehicle->m_pDriver->m_nPedType == PED_TYPE_CRIMINAL) {
            return false;
        }
        if (IsAnyoneParking()) {
            return false;
        }
        return DoesItFit();
    }
    case 2: { //!< Parking node
        if (vehicle->m_nCreatedBy == MISSION_VEHICLE) {
            return false;
        }
        if (vehicle->m_pDriver && vehicle->m_pDriver->GetCreatedBy() == PED_MISSION) {
            return false;
        }
        if (!arg5) {
            if (DotProduct(vehicle->GetForward(), nextNode->GetPosition() - vehicle->GetPosition()) < 0.0f) {
                return false;
            }
        }
        if (vehicle->IsLawEnforcementVehicle()) {
            return false;
        }
        if (vehicle->GetVehicleModelInfo()->m_nVehicleClass == VEHICLE_CLASS_BIG) {
            return false;
        }
        if (vehicle->m_pDriver && vehicle->m_pDriver->m_nPedType == PED_TYPE_CRIMINAL) {
            return false;
        }
        if (IsAnyoneParking()) {
            return false;
        }
        return DoesItFit();
    }
    case 5:
    case 10:
        if (arg5) {
            return false;
        }
        if (colModel->m_boundBox.m_vecMax.z < 2.0f) {
            return false;
        }
        break;
    case 8:
    case 9:
        if (arg5) {
            return false;
        }
        if (colModel->m_boundBox.m_vecMax.z > 1.5f
            || colModel->m_boundBox.m_vecMax.x > 2.0f
            || colModel->m_boundBox.m_vecMax.y > 4.0f
        ) {
            return false;
        }
        break;
    default:
        break;
    }

    //! A cruising mission vehicle may only take a "don't wander" node if the previous one had it too
    if (vehicle->m_nCreatedBy == MISSION_VEHICLE
        && vehicle->m_autoPilot.m_nCarMission == MISSION_CRUISE
        && nextNode->m_bDontWander
        && !prevNode->m_bDontWander
    ) {
        return false;
    }

    //! Dead-end/switched-off nodes are only taken if we're already on one
    if (nextNode->m_onDeadEnd && !prevNode->m_onDeadEnd) {
        return false;
    }
    if (nextNode->m_isSwitchedOff && !prevNode->m_isSwitchedOff) {
        return false;
    }

    return !arg5;
}

// 0x423EA0
bool CCarCtrl::IsThisVehicleInteresting(CVehicle* vehicle) {
    for (auto& car : apCarsToKeep) {
        if (car == vehicle) {
            return true;
        }
    }
    return false;
}

// 0x432CB0
void CCarCtrl::JoinCarWithRoadAccordingToMission(CVehicle* vehicle) {
    switch (vehicle->m_autoPilot.m_nCarMission) {
    case MISSION_NONE:
    case MISSION_CRUISE:
    case MISSION_WAITFORDELETION:
    case MISSION_EMERGENCYVEHICLE_STOP:
    case MISSION_STOP_FOREVER:
    case MISSION_FOLLOW_RECORDED_PATH:
    case MISSION_PARK_PERPENDICULAR:
    case MISSION_PARK_PARALLEL:
    case MISSION_PARK_PERPENDICULAR_2:
    case MISSION_PARK_PARALLEL_2:
        return JoinCarWithRoadSystem(vehicle);
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_RAMPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_HANDBRAKESTOP:
    case MISSION_BOAT_ATTACKPLAYER:
    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_1:
    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_2:
    case MISSION_BLOCKPLAYER_FORWARDANDBACK:
    case MISSION_APPROACHPLAYER_FARAWAY:
    case MISSION_APPROACHPLAYER_CLOSE:
    case MISSION_BOAT_CIRCLEPLAYER: {
        JoinCarWithRoadSystemGotoCoors(vehicle, FindPlayerCoors(-1), true, vehicle->IsSubBoat());
        break;
    }
    case MISSION_GOTOCOORDINATES:
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE:
    case MISSION_GOTOCOORDINATES_ACCURATE:
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE_ACCURATE:
    case MISSION_GOTOCOORDINATES_ASTHECROWSWIMS:
    case MISSION_GOTOCOORDINATES_RACING: {
        JoinCarWithRoadSystemGotoCoors(vehicle, vehicle->m_autoPilot.m_vecDestinationCoors, true, vehicle->IsSubBoat());
        break;
    }
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_RAMCAR_CLOSE:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_BLOCKCAR_CLOSE:
    case MISSION_BLOCKCAR_HANDBRAKESTOP:
    case MISSION_PROTECTION_REAR:
    case MISSION_PROTECTION_FRONT:
    case MISSION_ESCORT_LEFT:
    case MISSION_ESCORT_RIGHT:
    case MISSION_ESCORT_REAR:
    case MISSION_ESCORT_FRONT:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_FOLLOWCAR_CLOSE:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_KILLPED_CLOSE:
    case MISSION_DO_DRIVEBY_CLOSE:
    case MISSION_DO_DRIVEBY_FARAWAY:
    case MISSION_ESCORT_LEFT_FARAWAY:
    case MISSION_ESCORT_RIGHT_FARAWAY:
    case MISSION_ESCORT_REAR_FARAWAY:
    case MISSION_ESCORT_FRONT_FARAWAY: {
        JoinCarWithRoadSystemGotoCoors(vehicle, vehicle->m_autoPilot.m_TargetEntity->GetPosition(), true, vehicle->IsSubBoat());
        break;
    }
    }
}

// 0x42F5A0
void CCarCtrl::JoinCarWithRoadSystem(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    //! Reset the routing state (The original only resets the area IDs to `-1`)
    autoPilot.m_endingRouteNode.ResetAreaId();
    autoPilot.m_currentAddress.ResetAreaId();
    autoPilot.m_startingRouteNode.ResetAreaId();
    autoPilot.m_nCurrentPathNodeInfo  = CCarPathLinkAddress{};
    autoPilot.m_nNextPathNodeInfo     = CCarPathLinkAddress{};
    autoPilot.m_nPreviousPathNodeInfo = CCarPathLinkAddress{};
    autoPilot.m_nPathFindNodesCount   = 0;

    //! Node closest to us, favouring the direction we're facing
    const CVector      fwd     = vehicle->GetForward();
    const CNodeAddress nearest = ThePaths.FindNodeClosestToCoorsFavourDirection(
        vehicle->GetPosition(),
        PATH_TYPE_VEH,
        CVector2D{ fwd.x, fwd.y }
    );
    if (nearest.m_wAreaId == (uint16)-1 || !ThePaths.m_pPathNodes[nearest.m_wAreaId]) {
        return;
    }

    const CPathNode& node = *ThePaths.GetPathNode(nearest);
    if (!node.m_nNumLinks) {
        return;
    }

    //! Find the link closest to the node we're on
    const CVector nodePos = node.GetPosition();
    CNodeAddress  closest{ (uint16)-1, (uint16)-1 };
    float         closestDist = 999999.9f;
    for (auto i = 0u; i < node.m_nNumLinks; i++) {
        const CNodeAddress link = ThePaths.m_pNodeLinks[nearest.m_wAreaId][node.m_wBaseLinkId + i];
        if (!ThePaths.m_pPathNodes[link.m_wAreaId]) {
            continue;
        }

        const CVector linkPos = ThePaths.GetPathNode(link)->GetPosition();
        const float   dx      = linkPos.x - nodePos.x;
        const float   dy      = linkPos.y - nodePos.y;
        const float   dist    = std::sqrt(dx * dx + dy * dy);
        if (dist < closestDist) {
            closestDist = dist;
            closest     = link;
        }
    }
    if (closest.m_wAreaId == (uint16)-1) {
        return;
    }

    //! If the node we're on is behind the closest link (Relative to our heading), swap them
    CVector heading = fwd;
    if (heading.x == 0.0f && heading.y == 0.0f) {
        heading.x = 1.0f;
    }

    CNodeAddress current = closest;
    CNodeAddress start   = nearest;
    if (const CVector closestPos = ThePaths.GetPathNode(closest)->GetPosition();
        (nodePos.y - closestPos.y) * heading.y + (nodePos.x - closestPos.x) * heading.x < 0.0f
    ) {
        std::swap(current, start);
    }

    autoPilot.m_endingRouteNode.ResetAreaId();
    autoPilot.m_currentAddress    = current;
    autoPilot.m_startingRouteNode = start;
    FindLinksToGoWithTheseNodes(vehicle);
    autoPilot.m_nCurrentLane = 0;
    autoPilot.m_nNextLane    = 0;
}

/*!
* @addr 0x44E4F0
* @brief Drop the first path node when `startPos` lies between it and the one after it.
*
* NOTE: The function itself lives in `PathFind.cpp` (Not reversed yet), this is an inlined copy.
*/
static void DropFirstPathNodeIfStartIsBetween(const CVector& startPos, CNodeAddress* nodes, int16& count) {
    if (count < 2) {
        return;
    }
    if (!ThePaths.m_pPathNodes[nodes[0].m_wAreaId] || !ThePaths.m_pPathNodes[nodes[1].m_wAreaId]) {
        return;
    }

    const CVector pos0 = ThePaths.GetPathNode(nodes[0])->GetPosition();
    const CVector pos1 = ThePaths.GetPathNode(nodes[1])->GetPosition();

    //! We're between the two nodes if the vectors pointing to them from us face opposite directions
    if ((pos0.x - startPos.x) * (pos1.x - startPos.x) + (pos0.y - startPos.y) * (pos1.y - startPos.y) >= 0.0f) {
        return;
    }

    --count;
    for (int16 i = 0; i < count; i++) {
        nodes[i] = nodes[i + 1];
    }
}

// 0x42F870
bool CCarCtrl::JoinCarWithRoadSystemGotoCoors(CVehicle* vehicle, const CVector& posn, bool unused, bool bIsBoat) {
    auto& autoPilot = vehicle->m_autoPilot;

    autoPilot.m_vecDestinationCoors = posn;

    //! 0x8A5F44 - The "no forbidden node" address (Note: The node ID is `0`, not `-1`)
    static const CNodeAddress NoForbiddenNode{ (uint16)-1, 0 };

    ThePaths.DoPathSearch(
        PATH_TYPE_VEH,
        vehicle->GetPosition(),
        NoForbiddenNode,
        posn,
        autoPilot.m_aPathFindNodesInfo.data(),
        (int16&)autoPilot.m_nPathFindNodesCount,
        8,
        nullptr,
        1000000.0f,
        nullptr,
        1000000.0f,
        (autoPilot.m_nCarCtrlFlags >> 6) & 1, //!< bCantGoAgainstTraffic
        NoForbiddenNode,
        vehicle->m_nModelIndex == 0x21B,
        bIsBoat
    );

    DropFirstPathNodeIfStartIsBetween(vehicle->GetPosition(), autoPilot.m_aPathFindNodesInfo.data(), (int16&)autoPilot.m_nPathFindNodesCount);

    if (autoPilot.m_nPathFindNodesCount > 1) {
        autoPilot.m_currentAddress = autoPilot.m_aPathFindNodesInfo[0];
        autoPilot.m_endingRouteNode.ResetAreaId();
        autoPilot.RemoveOnePathNode();
        autoPilot.m_startingRouteNode = autoPilot.m_aPathFindNodesInfo[0];
        autoPilot.RemoveOnePathNode();
        FindLinksToGoWithTheseNodes(vehicle);
        autoPilot.m_nCurrentLane = 0;
        autoPilot.m_nNextLane    = 0;
        return false;
    }

    JoinCarWithRoadSystem(vehicle);
    autoPilot.m_nPathFindNodesCount = 0;
    return true;
}

// 0x432B10
bool CCarCtrl::PickNextNodeAccordingStrategy(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    //! Speed limit of the node we're heading to
    {
        const CPathNode* const node = ThePaths.GetPathNode(autoPilot.m_startingRouteNode);
        autoPilot.field_41 = (uint8)node->m_nSpeedLimit;
        if (const uint8 behaviour = node->m_nBehaviourType; behaviour > 7 && behaviour < 10) {
            autoPilot.field_41 = 0xFF;
        }
    }

    switch (autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY: {
        const CVector playerPos = FindPlayerCoors(-1);
        return PickNextNodeToChaseCar(vehicle, playerPos.x, playerPos.y, playerPos.z);
    }
    case MISSION_GOTOCOORDINATES:
    case MISSION_GOTOCOORDINATES_ACCURATE:
        return PickNextNodeToFollowPath(vehicle);

    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
    case MISSION_ESCORT_LEFT_FARAWAY:
    case MISSION_ESCORT_RIGHT_FARAWAY:
    case MISSION_ESCORT_REAR_FARAWAY:
    case MISSION_ESCORT_FRONT_FARAWAY:
        break;

    default:
        PickNextNodeRandomly(vehicle);
        return false; //!< NOTE: The original masks the callee result's low byte away
    }

    //! Follow the target entity
    const CVector targetPos = autoPilot.m_TargetEntity->GetPosition();
    return PickNextNodeToChaseCar(vehicle, targetPos.x, targetPos.y, targetPos.z);
}

// 0x421740
void CCarCtrl::InitSequence(int32 numSequenceElements) {
    SequenceElements = numSequenceElements;
    SequenceRandomOffset = CGeneral::GetRandomNumber() % numSequenceElements;
    bSequenceOtherWay = (CGeneral::GetRandomNumber() / 4) % 2;
}

// 0x42DE80
void CCarCtrl::PickNextNodeRandomly(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    if (vehicle->m_nForcedRandomRouteSeed) {
        srand(vehicle->m_nForcedRandomRouteSeed);
    }

    const CNodeAddress        cur      = autoPilot.m_currentAddress;
    const CNodeAddress        start    = autoPilot.m_startingRouteNode;
    const CCarPathLinkAddress nextInfo = autoPilot.m_nNextPathNodeInfo;

    if (!ThePaths.m_pPathNodes[cur.m_wAreaId]
        || !ThePaths.m_pPathNodes[start.m_wAreaId]
        || !ThePaths.m_pPathNodes[nextInfo.m_wAreaId]
    ) {
        return;
    }

    const CPathNode* const startNode = ThePaths.GetPathNode(start);
    const int32            numLinks  = startNode->m_nNumLinks;

    //! Outgoing lane count/mask of the link we're currently on
    const CCarPathLink& curLink   = ThePaths.GetCarPathLink(nextInfo);
    const bool          fromStart = curLink.m_attachedTo == start;
    const uint8         laneCount = fromStart ? curLink.m_numOppositeDirLanes : curLink.m_numSameDirLanes;
    const uint8         lanesMask = fromStart
        ? (uint8)(curLink.m_numOppositeDirLanes << 3) //!< `laneByte & 0x38`
        : (uint8)(curLink.m_numSameDirLanes);         //!< `laneByte & 0x07`

    //! Which turn directions are allowed from the lane we're in?
    uint8 allowed = 0;
    if (autoPilot.m_nNextLane == 0) {
        allowed |= 4;
    }
    uint8 tmp = allowed;
    if (autoPilot.m_nNextLane == (int8)((laneCount & 7) - 1)) {
        tmp      = allowed | 2;
        allowed |= 2;
    }
    if ((laneCount & 7) < 3 || tmp == 0) {
        allowed = tmp | 1;
    }

    autoPilot.m_nPreviousPathNodeInfo = autoPilot.m_nCurrentPathNodeInfo;
    autoPilot.m_currentAddress        = start;

    if (ThisVehicleShouldTryNotToTurn(vehicle)) {
        allowed = 1;
    }

    InitSequence(numLinks);

    //! The raw lane byte of a link (`m_numSameDirLanes` in the low 3 bits, `m_numOppositeDirLanes` in the next 3)
    const auto LaneByte = [](const CCarPathLink& link) {
        return (uint8)(link.m_numSameDirLanes | (link.m_numOppositeDirLanes << 3));
    };

    const auto ConnectionOf = [&](int32 i) {
        return ThePaths.m_pNodeLinks[start.m_wAreaId][startNode->m_wBaseLinkId + FindSequenceElement(i)];
    };
    const auto NaviLinkOf = [&](int32 i) {
        return ThePaths.m_pNaviLinks[start.m_wAreaId][startNode->m_wBaseLinkId + FindSequenceElement(i)];
    };

    bool found = false;

    //! Attempt 1: Candidates that pass the full validity check
    for (int32 i = 0; i < numLinks; i++) {
        const CNodeAddress cand = ConnectionOf(i);
        autoPilot.m_startingRouteNode = cand;

        if (!ThePaths.m_pPathNodes[cand.m_wAreaId]) {
            continue;
        }

        bool       flag    = false;
        const int8 pathDir = FindPathDirection(cur, autoPilot.m_currentAddress, cand, &flag);

        const CCarPathLinkAddress candInfo = NaviLinkOf(i);
        if (!ThePaths.m_pPathNodes[candInfo.m_wAreaId]) {
            continue;
        }
        if (vehicle->GetStatus() == STATUS_SIMPLE && flag) {
            continue;
        }

        const CCarPathLink& candLink = ThePaths.GetCarPathLink(candInfo);
        const uint8         candLanesMask = candLink.m_attachedTo == start
            ? (uint8)(candLink.m_numOppositeDirLanes << 3)
            : (uint8)(candLink.m_numSameDirLanes);

        //! @todo(re) 0x42DE80 - The two booleans handed to `IsThisAnAppropriateNode` and tested below are built
        //!                     from the packed lane bytes; their exact derivation isn't recovered, so `candLanesMask`
        //!                     stands in for both.
        if (IsThisAnAppropriateNode(vehicle, cur, autoPilot.m_currentAddress, cand, candLanesMask != 0)
            && (pathDir & allowed) != 0
            && (lanesMask != 0 || candLanesMask == 0)
        ) {
            found = true;
            break;
        }
    }

    //! Attempt 2: Any other connection, as long as it isn't the current node
    if (!found) {
        for (int32 i = 0; i < numLinks; i++) {
            const CNodeAddress cand = ConnectionOf(i);
            autoPilot.m_startingRouteNode = cand;

            if (!ThePaths.m_pPathNodes[cand.m_wAreaId]) {
                continue;
            }

            const CCarPathLinkAddress candInfo = NaviLinkOf(i);
            if (!ThePaths.m_pPathNodes[candInfo.m_wAreaId]) {
                continue;
            }

            if (cand != cur
                && LaneByte(ThePaths.GetCarPathLink(candInfo)) != 0
                && (!ThePaths.GetPathNode(cand)->m_isSwitchedOff || ThePaths.GetPathNode(cur)->m_isSwitchedOff)
            ) {
                found = true;
                break;
            }
        }
    }

    //! Attempt 3: Same as above, dropping the switched-off requirement
    if (!found) {
        for (int32 i = 0; i < numLinks; i++) {
            const CNodeAddress cand = ConnectionOf(i);
            autoPilot.m_startingRouteNode = cand;

            if (!ThePaths.m_pPathNodes[cand.m_wAreaId]) {
                continue;
            }

            const CCarPathLinkAddress candInfo = NaviLinkOf(i);
            if (!ThePaths.m_pPathNodes[candInfo.m_wAreaId]) {
                continue;
            }

            if (cand != cur && LaneByte(ThePaths.GetCarPathLink(candInfo)) != 0) {
                found = true;
                break;
            }
        }
    }

    if (!found) {
        autoPilot.m_startingRouteNode = cur;
    }

    //! Stuck on the same node - Unstick ourselves
    if (cur == autoPilot.m_startingRouteNode && vehicle->GetStatus() != STATUS_PHYSICS) {
        vehicle->SetStatus(STATUS_PHYSICS);
        autoPilot.m_nTempAction                = TEMPACT_NONE;
        autoPilot.m_nTempActionTime            = CTimer::GetTimeInMS() + 2000;
        autoPilot.m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
        //! @todo(re) 0x42DE80 - The original also clears an unknown `CPhysical` byte at offset `+0xB8`.
    }

    //! Apply the chosen node's behaviour
    switch (ThePaths.GetPathNode(autoPilot.m_startingRouteNode)->m_nBehaviourType) {
    case 1:
    case 2: {
        vehicle->SetStatus(STATUS_PHYSICS);
        autoPilot.m_nTempAction                = TEMPACT_NONE;
        autoPilot.m_nTempActionTime            = CTimer::GetTimeInMS() + 2000;
        autoPilot.m_nCarMission                = ThePaths.GetPathNode(autoPilot.m_startingRouteNode)->m_nBehaviourType == 1
            ? MISSION_PARK_PARALLEL          //!< Roadblock: Stop in the middle of the road
            : MISSION_PARK_PERPENDICULAR;    //!< Parking node
        autoPilot.m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
        autoPilot.m_nCarDrivingStyle           = DRIVING_STYLE_STOP_FOR_CARS;
        break;
    }
    case 10:
        autoPilot.m_nTempAction     = TEMPACT_WAIT;
        autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 10000;
        if (vehicle->GetStatus() == STATUS_SIMPLE) {
            autoPilot.ModifySpeed(0.0f);
        }
        break;
    default:
        break;
    }

    if (ThePaths.GetPathNode(autoPilot.m_currentAddress)->m_nBehaviourType == 9
        && vehicle->m_nCreatedBy != MISSION_VEHICLE
    ) {
        autoPilot.m_nTempAction     = TEMPACT_WAIT;
        autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 0x1194;
        if (vehicle->GetStatus() == STATUS_SIMPLE) {
            autoPilot.ModifySpeed(0.0f);
        }
    }

    //! Rotate the path bookkeeping
    const int8 oldCurr = autoPilot._smthCurr;
    autoPilot.m_nPreviousPathNodeInfo = autoPilot.m_nCurrentPathNodeInfo;
    autoPilot.m_nCurrentPathNodeInfo  = autoPilot.m_nNextPathNodeInfo;
    autoPilot._smthCurr               = autoPilot._smthNext;
    autoPilot.field_C                += autoPilot.m_nSpeedScaleFactor;
    autoPilot._smthPrev               = oldCurr;
    autoPilot.m_nCurrentLane          = autoPilot.m_nNextLane;
    autoPilot.m_nNextPathNodeInfo     = nextInfo;

    //! Direction and lane count of the new next-link
    uint8 laneCountNext;
    if (IsNodeAddressLess(start, autoPilot.m_startingRouteNode)) {
        autoPilot._smthNext = -1;
        laneCountNext       = ThePaths.GetCarPathLink(nextInfo).m_numOppositeDirLanes;
    } else {
        autoPilot._smthNext = 1;
        laneCountNext       = ThePaths.GetCarPathLink(nextInfo).m_numSameDirLanes;
    }

    //! Randomly drift between lanes on sharp direction changes
    //! @todo(re) 0x42DE80 - The exact operand order of the 256.0f (`16m`) test wasn't recovered; this mirrors
    //!                     `PickNextNodeToChaseCar`, which uses the same 2D squared distance threshold.
    {
        const CVector curPos  = ThePaths.GetPathNode(cur)->GetPosition();
        const CVector nextPos = ThePaths.GetPathNode(autoPilot.m_startingRouteNode)->GetPosition();
        const float   d2      = (nextPos.x - curPos.x) * (nextPos.x - curPos.x)
                              + (nextPos.y - curPos.y) * (nextPos.y - curPos.y);
        if (d2 > 256.0f) {
            if (--autoPilot.field_50 == 0) {
                autoPilot.field_50 = (CGeneral::GetRandomNumber() & 3) + 4;
                autoPilot.m_nNextLane += (CGeneral::GetRandomNumber() < 0x3FFF) ? 1 : -1;
            }
        }
    }

    //! Clamp the lane, honouring the "stay in lane" flags
    autoPilot.m_nNextLane = (int8)std::min<int32>(autoPilot.m_nNextLane, laneCountNext - 1);
    if (autoPilot.m_nNextLane < 1) {
        autoPilot.m_nNextLane = 0;
    }
    if (autoPilot.m_nCarCtrlFlags & 0x08) { //!< bStayInFastLane
        autoPilot.m_nNextLane = 0;
    } else if (autoPilot.m_nCarCtrlFlags & 0x10) { //!< bStayInSlowLane
        autoPilot.m_nNextLane = (int8)std::max<int32>(0, laneCountNext - 1);
    }

    //! Speed scale factor from the lane geometry (Only for cars in physics state)
    if (vehicle->GetStatus() == STATUS_SIMPLE) {
        const CCarPathLink& curNavi  = ThePaths.GetCarPathLink(autoPilot.m_nCurrentPathNodeInfo);
        const CCarPathLink& nextNavi = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo);

        const float ofsCur  = (curNavi.OneWayLaneOffset()  + (float)autoPilot.m_nCurrentLane) * 5.4f;
        const float ofsNext = (nextNavi.OneWayLaneOffset() + (float)autoPilot.m_nNextLane) * 5.4f;

        const CVector2D curPos  = curNavi.GetNodeCoors();
        const CVector2D nextPos = nextNavi.GetNodeCoors();

        const CVector2D dirCur{ curNavi.m_dir.x * 0.01f, curNavi.m_dir.y * 0.01f };
        const CVector2D dirNext{ nextNavi.m_dir.x * 0.01f, nextNavi.m_dir.y * 0.01f };

        const CVector startCoors{
            curPos.x + ofsCur * dirCur.y,
            curPos.y - ofsCur * dirCur.x,
            0.0f
        };
        const CVector endCoors{
            nextPos.x + ofsNext * dirNext.y,
            nextPos.y - ofsNext * dirNext.x,
            0.0f
        };

        const float sf = CCurves::CalcSpeedScaleFactor(
            startCoors, endCoors,
            dirCur.x * (float)autoPilot._smthCurr,
            dirCur.y * (float)autoPilot._smthCurr,
            dirNext.x * (float)autoPilot._smthNext,
            dirNext.y * (float)autoPilot._smthNext
        );

        autoPilot.m_nSpeedScaleFactor = std::max((uint32)(int32)sf, 10u);
    }
}

// 0x426EF0
bool CCarCtrl::PickNextNodeToChaseCar(CVehicle* vehicle, float destX, float destY, float destZ) {
    auto& autoPilot = vehicle->m_autoPilot;

    CNodeAddress found[2] = { CNodeAddress{(uint16)-1, (uint16)-1}, CNodeAddress{(uint16)-1, (uint16)-1} };
    int16        numFound = -1;
    CNodeAddress next{ (uint16)-1, (uint16)-1 };
    float        maxDist   = 1000000.0f;
    float        foundDist = 0.0f;

    if (vehicle->m_nForcedRandomRouteSeed) {
        srand(vehicle->m_nForcedRandomRouteSeed);
    }

    const CNodeAddress curNode = autoPilot.m_currentAddress;
    const CNodeAddress start   = autoPilot.m_startingRouteNode;

    const bool lowTrafficArea = CWeather::WeatherRegion == WEATHER_REGION_DEFAULT || CWeather::WeatherRegion == WEATHER_REGION_DESERT;
    if (lowTrafficArea) {
        maxDist = 50.0f;
    }

    const CPathNode* node = ThePaths.GetPathNode(start);

    //! 0x8A5F44 - The "no forbidden node" address (Note: The node ID is `0`, not `-1`)
    static const CNodeAddress NoForbiddenNode{ (uint16)-1, 0 };

    ThePaths.DoPathSearch(
        PATH_TYPE_VEH,
        node->GetPosition(),
        start,
        CVector{destX, destY, destZ},
        found,
        numFound,
        2,
        &foundDist,
        maxDist,
        nullptr,
        1000000.0f,
        false,
        NoForbiddenNode,
        vehicle->m_nModelIndex == 0x21B,
        false
    );

    if (lowTrafficArea) {
        if (numFound == 0) {
            return true;
        }
        const CVector pos = vehicle->GetPosition();
        const float   d   = std::sqrt((destX - pos.x) * (destX - pos.x) + (destY - pos.y) * (destY - pos.y));
        if (d * 3.0f < foundDist) {
            return true;
        }
    }

    int32 nextLinkIndex = 0;
    bool  haveNext      = false;

    if (numFound == 1 || numFound == 2) {
        for (int32 k = 0; k < numFound; k++) {
            if (found[k] == start) {
                continue;
            }
            next     = found[k];
            haveNext = true;
            break;
        }
    }

    if (haveNext) {
        int32 i = 0;
        while (ThePaths.m_pNodeLinks[start.m_wAreaId][node->m_wBaseLinkId + i] != next) {
            i++;
        }
        nextLinkIndex = i;
    } else {
        //! No usable path result: pick the link that turns the least away from the destination
        const CVector pos           = vehicle->GetPosition();
        const float   headingToDest = CGeneral::GetATanOfXY(destX - pos.x, destY - pos.y);
        const uint8   numLinks      = node->m_nNumLinks;
        const CVector nodePos       = node->GetPosition();

        float best = 10.0f;
        for (int32 i = 0; i < numLinks; i++) {
            const CNodeAddress link = ThePaths.m_pNodeLinks[start.m_wAreaId][node->m_wBaseLinkId + i];
            if ((link == curNode && numLinks > 1) || !ThePaths.m_pPathNodes[link.m_wAreaId]) {
                continue;
            }

            const CPathNode* const   tgt  = &ThePaths.m_pPathNodes[link.m_wAreaId][link.m_wNodeId];
            const CVector            tgtPos = tgt->GetPosition();
            const float              ang  = std::abs(CGeneral::LimitRadianAngle(
                CGeneral::GetATanOfXY(tgtPos.x - nodePos.x, tgtPos.y - nodePos.y) - headingToDest
            ));
            if (ang < best) {
                best          = ang;
                next          = link;
                nextLinkIndex = i;
            }
        }
    }

    //! Shift the 3-node history and commit the new node
    const CNodeAddress oldCurrent = autoPilot.m_currentAddress;
    autoPilot.m_currentAddress     = autoPilot.m_startingRouteNode;
    autoPilot.m_endingRouteNode    = oldCurrent;
    autoPilot.m_startingRouteNode  = next;
    autoPilot.field_C             += autoPilot.m_nSpeedScaleFactor;
    autoPilot.m_nPreviousPathNodeInfo = autoPilot.m_nCurrentPathNodeInfo;
    autoPilot.m_nCurrentPathNodeInfo  = autoPilot.m_nNextPathNodeInfo;
    autoPilot._smthPrev               = autoPilot._smthCurr;
    autoPilot._smthCurr               = autoPilot._smthNext;
    autoPilot.m_nCurrentLane          = autoPilot.m_nNextLane;

    autoPilot.m_nNextPathNodeInfo = ThePaths.m_pNaviLinks[start.m_wAreaId][node->m_wBaseLinkId + nextLinkIndex];

    if (StopCarIfNodesAreInvalid(vehicle)) {
        return true;
    }

    //! Lane choice
    uint8 laneMax;
    {
        const CNodeAddress a = curNode;
        const CNodeAddress b = autoPilot.m_startingRouteNode;

        const bool goingForward = a.m_wAreaId != b.m_wAreaId
            ? a.m_wAreaId < b.m_wAreaId
            : a.m_wNodeId < b.m_wNodeId;

        const CCarPathLink& nextLink = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo);
        if (goingForward) {
            autoPilot._smthNext = -1;
            laneMax             = nextLink.m_numOppositeDirLanes;
        } else {
            autoPilot._smthNext = 1;
            laneMax             = nextLink.m_numSameDirLanes;
        }
    }

    //! Wander between lanes when the two links are more than 16m apart
    {
        const CCarPathLink& a = ThePaths.GetCarPathLink(autoPilot.m_nCurrentPathNodeInfo);
        const CCarPathLink& b = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo);
        const float d2 = (a.m_posn.x * 0.125f - b.m_posn.x * 0.125f) * (a.m_posn.x * 0.125f - b.m_posn.x * 0.125f)
                       + (a.m_posn.y * 0.125f - b.m_posn.y * 0.125f) * (a.m_posn.y * 0.125f - b.m_posn.y * 0.125f);
        if (d2 > 256.0f) {
            switch (autoPilot.m_nCarMission) {
            case MISSION_RAMPLAYER_FARAWAY:
            case MISSION_BLOCKPLAYER_FARAWAY:
            case MISSION_RAMCAR_FARAWAY:
            case MISSION_FOLLOWCAR_FARAWAY:
            case MISSION_BLOCKCAR_FARAWAY:
            case MISSION_ESCORT_LEFT_FARAWAY:
            case MISSION_KILLPED_FARAWAY:
            case MISSION_DO_DRIVEBY_FARAWAY:
                break;
            default:
                if (--autoPilot.field_50 == 0) {
                    autoPilot.field_50 = (CGeneral::GetRandomNumber() & 3) + 4;
                    autoPilot.m_nNextLane += (CGeneral::GetRandomNumber() < 0x3FFF) ? 1 : -1;
                }
                break;
            }
        }
    }

    //! Clamp the lane
    {
        autoPilot.m_nNextLane = (int8)std::min<int32>(autoPilot.m_nNextLane, laneMax - 1);
        if (autoPilot.m_nNextLane <= 0) {
            autoPilot.m_nNextLane = 0;
        }
        if (!(autoPilot.m_nCarCtrlFlags & 0x08)) { //!< !bStayInFastLane
            if ((autoPilot.m_nCarCtrlFlags & 0x10) || vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
                autoPilot.m_nNextLane = (int8)std::max<int32>(0, laneMax - 1);
            }
        } else {
            autoPilot.m_nNextLane = 0;
        }
    }

    return false;
}

// 0x427740
bool CCarCtrl::PickNextNodeToFollowPath(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    if (vehicle->m_nForcedRandomRouteSeed) {
        srand(vehicle->m_nForcedRandomRouteSeed);
    }

    const CNodeAddress start = autoPilot.m_startingRouteNode;

    if (autoPilot.m_nPathFindNodesCount < 1) {
        //! 0x8A5F44 - The "no forbidden node" address (Note: The node ID is `0`, not `-1`)
        static const CNodeAddress NoForbiddenNode{ (uint16)-1, 0 };

        ThePaths.DoPathSearch(
            PATH_TYPE_VEH,
            vehicle->GetPosition(),
            start,
            autoPilot.m_vecDestinationCoors,
            autoPilot.m_aPathFindNodesInfo.data(),
            (int16&)autoPilot.m_nPathFindNodesCount,
            8,
            nullptr,
            1000000.0f,
            nullptr,
            1000000.0f,
            (autoPilot.m_nCarCtrlFlags >> 6) & 1, //!< bCantGoAgainstTraffic
            NoForbiddenNode,
            vehicle->m_nModelIndex == 0x21B,
            false
        );

        if (autoPilot.m_nPathFindNodesCount < 2) {
            return true;
        }
        if (start == autoPilot.m_aPathFindNodesInfo[0]) {
            autoPilot.RemoveOnePathNode();
        }
        if (start == autoPilot.m_aPathFindNodesInfo[0]) {
            autoPilot.RemoveOnePathNode();
        }
        if (autoPilot.m_nPathFindNodesCount < 1) {
            return true;
        }
    }

    //! Shift the history and consume the first path node
    const CNodeAddress oldStart = autoPilot.m_startingRouteNode;
    autoPilot.m_startingRouteNode = autoPilot.m_aPathFindNodesInfo[0];
    autoPilot.m_endingRouteNode   = autoPilot.m_currentAddress;
    autoPilot.m_currentAddress    = oldStart;
    autoPilot.RemoveOnePathNode();

    autoPilot.m_nPreviousPathNodeInfo = autoPilot.m_nCurrentPathNodeInfo;
    autoPilot.m_nCurrentPathNodeInfo  = autoPilot.m_nNextPathNodeInfo;
    autoPilot._smthPrev               = autoPilot._smthCurr;
    autoPilot.field_C                += autoPilot.m_nSpeedScaleFactor;
    autoPilot.m_nCurrentLane          = autoPilot.m_nNextLane;

    //! The navi-link going from the (new) current address to the (new) starting node
    {
        const CNodeAddress cur  = autoPilot.m_currentAddress;
        const CPathNode*   node = ThePaths.GetPathNode(cur);
        int32              i    = 0;
        while (ThePaths.m_pNodeLinks[cur.m_wAreaId][node->m_wBaseLinkId + i] != autoPilot.m_startingRouteNode) {
            i++;
        }
        autoPilot.m_nNextPathNodeInfo = ThePaths.m_pNaviLinks[cur.m_wAreaId][node->m_wBaseLinkId + i];
    }

    //! Lane choice
    uint8 laneMax;
    {
        const CNodeAddress cur  = autoPilot.m_currentAddress;
        const CNodeAddress dest = autoPilot.m_startingRouteNode;

        const bool goingForward = cur.m_wAreaId != dest.m_wAreaId
            ? cur.m_wAreaId < dest.m_wAreaId
            : cur.m_wNodeId < dest.m_wNodeId;

        const CCarPathLink& nextLink = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo);
        if (goingForward) {
            autoPilot._smthNext = -1;
            laneMax             = nextLink.m_numOppositeDirLanes;
        } else {
            autoPilot._smthNext = 1;
            laneMax             = nextLink.m_numSameDirLanes;
        }
    }

    //! Randomly wander between lanes when the two links are more than 16m apart
    {
        const CCarPathLink& a = ThePaths.GetCarPathLink(autoPilot.m_nCurrentPathNodeInfo);
        const CCarPathLink& b = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo);
        const float d2 = (a.m_posn.x * 0.125f - b.m_posn.x * 0.125f) * (a.m_posn.x * 0.125f - b.m_posn.x * 0.125f)
                       + (a.m_posn.y * 0.125f - b.m_posn.y * 0.125f) * (a.m_posn.y * 0.125f - b.m_posn.y * 0.125f);
        if (d2 > 256.0f && (CGeneral::GetRandomNumber() & 0x600) == 0) {
            autoPilot.m_nNextLane += (CGeneral::GetRandomNumber() < 0x3FFF) ? 1 : -1;
        }
    }

    //! Clamp the lane
    {
        autoPilot.m_nNextLane = (int8)std::min<int32>(autoPilot.m_nNextLane, laneMax - 1);
        if (autoPilot.m_nNextLane <= 0) {
            autoPilot.m_nNextLane = 0;
        }
        if (!(autoPilot.m_nCarCtrlFlags & 0x08)) { //!< !bStayInFastLane
            if (autoPilot.m_nCarCtrlFlags & 0x10) { //!< bStayInSlowLane
                autoPilot.m_nNextLane = (int8)std::max<int32>(0, laneMax - 1);
            }
        } else {
            autoPilot.m_nNextLane = 0;
        }
    }

    //! Speed scale factor from the lane geometry
    //! @todo(re) The original offsets the link positions by `(OneWayLaneOffset() + lane) * 5.4`
    //!           before calling `CalcSpeedScaleFactor` (see 0x427CF3-0x427E..)
    {
        const CCarPathLink& from = ThePaths.GetCarPathLink(autoPilot.m_nCurrentPathNodeInfo);
        const CCarPathLink& to   = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo);

        const float speedScale = CCurves::CalcSpeedScaleFactor(
            CVector{from.m_posn.x * 0.125f, from.m_posn.y * 0.125f, 0.0f},
            CVector{to.m_posn.x * 0.125f, to.m_posn.y * 0.125f, 0.0f},
            (float)from.m_dir.x * 0.01f * (float)autoPilot._smthCurr,
            (float)from.m_dir.y * 0.01f * (float)autoPilot._smthCurr,
            (float)to.m_dir.x * 0.01f * (float)autoPilot._smthNext,
            (float)to.m_dir.y * 0.01f * (float)autoPilot._smthNext
        );
        autoPilot.m_nSpeedScaleFactor = std::max<int32>(10, (int32)speedScale);
    }

    //! Parking node: hand over to real physics and switch mission
    {
        const CNodeAddress n = autoPilot.m_startingRouteNode;
        if (ThePaths.m_pPathNodes[n.m_wAreaId] && (ThePaths.m_pPathNodes[n.m_wAreaId][n.m_wNodeId].m_nBehaviourType & 0xF) == 2) {
            SwitchVehicleToRealPhysics(vehicle);
            autoPilot.m_nCarMission = MISSION_PARK_PERPENDICULAR;
        }
    }

    return false;
}

// 0x429600
void CCarCtrl::PossiblyFireHSMissile(CVehicle* entityLauncher, CEntity* targetEntity) {
    if (!targetEntity)
        return;

    if (CTimer::GetTimeInMS() / 2000u == CTimer::GetPreviousTimeInMS() / 2000u)
        return;

    const CVector launcherPos = entityLauncher->GetPosition();
    const CVector targetPos = targetEntity->GetPosition();
    CVector dir = targetPos - launcherPos;
    const float dist = dir.Magnitude();
    if (dist < 160.0f && dist > 30.0f) {
        CMatrix launcherMat = entityLauncher->GetMatrix();
        CVector dirNormalized = dir;
        dir.Normalise();
        if (DotProduct(launcherMat.GetForward(), dirNormalized) > 0.8f) {
            CProjectileInfo::AddProjectile(
                entityLauncher,
                eWeaponType::WEAPON_ROCKET_HS,
                launcherPos + launcherMat.GetForward() * 4.0f - launcherMat.GetUp() * 3.0f,
                1.0f,
                &entityLauncher->GetMatrix().GetForward(),
                targetEntity
            );
        }
    }
}

// 0x424F80
void CCarCtrl::PossiblyRemoveVehicle(CVehicle* vehicle) {
    if (vehicle->m_nNumGettingIn) {
        return;
    }

    if (vehicle->vehicleFlags.bPartOfConvoy && vehicle->m_autoPilot.m_TargetEntity) {
        return;
    }

    const CVector& playerPos = FindPlayerCentreOfWorld(CWorld::PlayerInFocus);
    const auto&    cam       = TheCamera.m_aCams[TheCamera.m_nActiveCam];

    if (!vehicle->vehicleFlags.bIsLocked && vehicle->CanBeDeleted() && !CCranes::IsThisCarBeingTargettedByAnyCrane(vehicle)) {
        if (vehicle->vehicleFlags.bFadeOut && CVisibilityPlugins::GetClumpAlpha(vehicle->GetRpClump()) == 0) {
            CWorld::Remove(vehicle);
            delete vehicle;
            return;
        }

        const CVector vehiclePos = vehicle->GetPosition();
        const float   dx         = vehiclePos.x - playerPos.x;
        const float   dy         = vehiclePos.y - playerPos.y;
        const float   dist2D     = std::sqrt(dx * dx + dy * dy);

        float removalDist;
        if (!vehicle->GetIsOnScreen()
            && !cam.m_bFixingBeta
            && !cam.m_bTheHeightFixerVehicleIsATrain
            && !cam.m_bCollisionChecksOn
            && TheCamera.GetLookDirection() != 0
            && vehicle->m_nCreatedBy != PARKED_VEHICLE
            && vehicle->m_nModelIndex != MODEL_AMBULAN
            && vehicle->m_nModelIndex != MODEL_FIRETRUK
            && !vehicle->vehicleFlags.bIsLawEnforcer
            && !vehicle->vehicleFlags.bIsCarParkVehicle
            && CTimer::GetTimeInMS() >= vehicle->m_nTimeTillWeNeedThisCar
            && !vehicle->vehicleFlags.bNeverUseSmallerRemovalRange
        ) {
            removalDist = 44.0f;
        } else {
            // 0xB6F11C -> `TheCamera.m_fLODDistMultiplier` (offset 0xF4 in the 32-bit layout)
            removalDist = TheCamera.m_fLODDistMultiplier * 170.0f;
        }

        // 0xB6F9B4 -> `TheCamera.m_mCameraMatrix.GetForward().y` (offset 0x98C in the 32-bit layout)
        if (TheCamera.m_mCameraMatrix.GetForward().y < -0.9f) {
            removalDist = 70.0f;
        }

        const float maxRemovalDist = std::max((float)vehicle->m_nExtendedRemovalRange, 170.0f);
        if (maxRemovalDist * removalDist * (1.0f / 170.0f) > dist2D && vehicle->m_autoPilot.m_nCarMission != MISSION_PLANE_ATTACK_PLAYER_POLICE) {
            if (CGarages::IsPointWithinHideOutGarage(vehicle->GetPosition())) {
                goto check_status;
            }
            if (IsThisVehicleInteresting(vehicle)) {
                vehicle->m_nFakePhysics = 10;
                return;
            }
            if (vehicle->GetIsOnScreen()) {
                vehicle->vehicleFlags.bFadeOut = true;
                return;
            }
            CWorld::Remove(vehicle);
            delete vehicle;
            return;
        }

    check_status:
        switch (vehicle->GetStatus()) {
        case STATUS_SIMPLE:
            if (std::abs(vehicle->GetUpVector().z) < 0.74f) {
                CWorld::Remove(vehicle);
                delete vehicle;
                return;
            }
            break;
        case STATUS_PHYSICS:
        case STATUS_WRECKED:
            if ((vehicle->m_nVehicleSubType == VEHICLE_TYPE_PLANE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_HELI)
                && vehicle->m_bIsStuck
            ) {
                CWorld::Remove(vehicle);
                delete vehicle;
                return;
            }
            break;
        default:
            break;
        }
    }

    //! Abandoned car
    if (vehicle->m_nVehicleSubType != VEHICLE_TYPE_HELI
        && vehicle->m_nVehicleSubType != VEHICLE_TYPE_PLANE
        && vehicle->m_autoPilot.m_nTempAction != TEMPACT_STUCKINTRAFFIC
        && (vehicle->GetStatus() == STATUS_SIMPLE
            || (vehicle->GetStatus() == STATUS_PHYSICS
                && (vehicle->m_autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_STOP_FOR_CARS
                    || vehicle->m_autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_STOP_FOR_CARS_IGNORE_LIGHTS
                )
            )
        )
        && CTimer::GetTimeInMS() - vehicle->m_nLastCollisionTime > 5000
        && vehicle->m_nTimeTillWeNeedThisCar == 0
        && CTimer::GetTimeInMS() != 0
        && !vehicle->GetIsOnScreen()
        && (vehicle->GetPosition() - playerPos).Magnitude() > 22.0f
        && !IsThisVehicleInteresting(vehicle)
        && !vehicle->vehicleFlags.bIsLocked
        && vehicle->CanBeDeleted()
        && !CTrafficLights::ShouldCarStopForLight(vehicle, true)
        && !CTrafficLights::ShouldCarStopForBridge(vehicle)
        && !CGarages::IsPointWithinHideOutGarage(vehicle->GetPosition())
    ) {
        CWorld::Remove(vehicle);
        delete vehicle;
        return;
    }

    //! Wrecked, far away and unseen
    if (vehicle->GetStatus() != STATUS_WRECKED
        || vehicle->m_nTimeWhenBlowedUp == 0
        || CTimer::GetTimeInMS() <= vehicle->m_nTimeWhenBlowedUp + 60000u
        || CTimer::GetTimeInMS() <= vehicle->m_nTimeTillWeNeedThisCar
        || vehicle->GetIsOnScreen()
        || (vehicle->GetPosition() - playerPos).Magnitude() <= 42.25f
        || CGarages::IsPointWithinHideOutGarage(vehicle->GetPosition())
    ) {
        return;
    }

    CWorld::Remove(vehicle);
    delete vehicle;
}

// 0x423F10
void CCarCtrl::PruneVehiclesOfInterest() {
    ZoneScoped;

    if ((CTimer::GetFrameCounter() % 64) == 19 && FindPlayerCoors(-1).z < 950.0f) {
        for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
            if (apCarsToKeep[i]) {
                if (CTimer::GetTimeInMS() > aCarsToKeepTime[i] + 180000) {
                    apCarsToKeep[i] = nullptr;
                }
            }
        }
    }
}

// 0x42FC40
void CCarCtrl::ReconsiderRoute(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    //! At most once every 2 seconds (Offset by the time the car switched to real physics)
    const uint32 timeOffset = autoPilot.m_nTimeSwitchedToRealPhysics & 0xFFFF;
    if ((CTimer::GetTimeInMS() + timeOffset) / 2000u == (CTimer::GetPreviousTimeInMS() + timeOffset) / 2000u) {
        return;
    }

    switch (autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_GOTOCOORDINATES:
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
        break;
    default:
        autoPilot.m_ucCarMissionModeCounter = 0;
        return;
    }

    CNodeAddress node1{}, node2{};
    FindNodesThisCarIsNearestTo(vehicle, node1, node2);
    if (node1.m_wAreaId == (uint16)-1) {
        return;
    }

    //! Don't re-route if we're already on the link we'd route to
    const CNodeAddress& cur   = autoPilot.m_currentAddress;
    const CNodeAddress& start = autoPilot.m_startingRouteNode;
    const CNodeAddress& end   = autoPilot.m_endingRouteNode;
    if (   (cur == node1 && start == node2)
        || (cur == node2 && start == node1)
        || (end == node1 && cur == node2)
        || (end == node2 && cur == node1)
        || (cur == node2)
        || (end == node2)
    ) {
        autoPilot.m_ucCarMissionModeCounter = 0;
        return;
    }

    //! Give the current route a few more chances before considering a new one
    if (++autoPilot.m_ucCarMissionModeCounter <= 4) {
        return;
    }

    //! NOTE: The `default` case reads an uninitialized stack variable in the original (Unreachable,
    //!       as the missions above always match one of the cases below) - We just keep the destination
    CVector dest = autoPilot.m_vecDestinationCoors;
    switch (autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY:
        dest = FindPlayerCoors(-1);
        break;
    case MISSION_GOTOCOORDINATES:
        dest = autoPilot.m_vecDestinationCoors;
        break;
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
        dest = autoPilot.m_TargetEntity->GetPosition();
        break;
    default:
        break;
    }

    //! Check how long the route to the destination would be
    int16 numNodes{};
    float distance{};
    ThePaths.DoPathSearch(
        PATH_TYPE_VEH,
        vehicle->GetPosition(),
        node2,
        dest,
        nullptr,
        numNodes,
        0,
        &distance,
        1000000.0f,
        nullptr,
        1000000.0f,
        (autoPilot.m_nCarCtrlFlags >> 6) & 1, //!< bCantGoAgainstTraffic
        cur,
        vehicle->m_nModelIndex == 0x21B,
        false
    );

    if (distance >= 90000.0f || numNodes <= 1) {
        autoPilot.m_ucCarMissionModeCounter = 0;
        return;
    }

    //! Commit the new route
    autoPilot.m_currentAddress    = node1;
    autoPilot.m_startingRouteNode = node2;
    FindLinksToGoWithTheseNodes(vehicle);

    ThePaths.DoPathSearch(
        PATH_TYPE_VEH,
        vehicle->GetPosition(),
        node2,
        dest,
        autoPilot.m_aPathFindNodesInfo.data(),
        (int16&)autoPilot.m_nPathFindNodesCount,
        8,
        nullptr,
        1000000.0f,
        nullptr,
        1000000.0f,
        (autoPilot.m_nCarCtrlFlags >> 6) & 1, //!< bCantGoAgainstTraffic
        autoPilot.m_currentAddress,
        vehicle->m_nModelIndex == 0x21B,
        false
    );
    autoPilot.RemoveOnePathNode();

    autoPilot.m_ucCarMissionModeCounter = 0;
}

// 0x423DE0
void CCarCtrl::RegisterVehicleOfInterest(CVehicle* vehicle) {
    const uint32 now = CTimer::GetTimeInMS();

    for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
        if (apCarsToKeep[i] == vehicle) { //!< Already registered, refresh the time
            aCarsToKeepTime[i] = now;
            return;
        }
    }
    for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
        if (!apCarsToKeep[i]) { //!< Free slot
            apCarsToKeep[i] = vehicle;
            aCarsToKeepTime[i] = now;
            return;
        }
    }

    //! No free slot, evict the least recently registered
    uint32 oldest = 0xFFFFFFFF;
    size_t slot = 0;
    for (size_t i = 0; i < std::size(apCarsToKeep); i++) {
        if (apCarsToKeep[i] && aCarsToKeepTime[i] < oldest) {
            oldest = aCarsToKeepTime[i];
            slot = i;
        }
    }
    apCarsToKeep[slot] = vehicle;
    aCarsToKeepTime[slot] = now;
}

// 0x4322B0
void CCarCtrl::RemoveCarsIfThePoolGetsFull() {
    ZoneScoped;

    if (CTimer::GetFrameCounter() % 8 != 3)
        return;

    if (GetVehiclePool()->GetNoOfFreeSpaces() >= 8)
        return;

    // Find closest deletable vehicle
    const CVector camPos = TheCamera.GetPosition();
    float fClosestDist = std::numeric_limits<float>::max();
    CVehicle* closestVeh = nullptr;
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        if (IsThisVehicleInteresting(&veh)) {
            continue;
        }
        if (veh.vehicleFlags.bIsLocked) {
            continue;
        }
        if (!veh.CanBeDeleted()) {
            continue;
        }
        if (CCranes::IsThisCarBeingTargettedByAnyCrane(&veh)) {
            continue;
        }

        const float fCamVehDist = (camPos - veh.GetPosition()).Magnitude();
        if (fClosestDist > fCamVehDist) {
            fClosestDist = fCamVehDist;
            closestVeh   = &veh;
        }
    }
    if (closestVeh) {
        CWorld::Remove(closestVeh);
        delete closestVeh;
    }
}

// 0x42CD10
void CCarCtrl::RemoveDistantCars() {
    ZoneScoped;

    // FIXBUGS: First remove vehicles that can be removed
    if (notsa::bugfixes::CCarCtrl_RemoveDistantCars_UseAfterFree) {
        for (auto& veh : GetVehiclePool()->GetAllValid()) {
            PossiblyRemoveVehicle(&veh);
        }
    }

    //... only then process them, this way we don't do use-after-free
    // only other solution would be `PossiblyRemoveVehicle` returning a `bool`
    // to indicate whenever the vehicle was deleted or not.
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        if (!notsa::bugfixes::CCarCtrl_RemoveDistantCars_UseAfterFree) {
            PossiblyRemoveVehicle(&veh); // This may or may not invalidate `veh`
        }
        if (!veh.vehicleFlags.bCreateRoadBlockPeds) {
            continue;
        }
        if (DistanceBetweenPoints(FindPlayerCentreOfWorld(), veh.GetPosition()) >= 54.5f) {
            continue;
        }
        CRoadBlocks::GenerateRoadBlockPedsForCar(
            &veh,
            veh.m_nPedsPositionForRoadBlock,
            veh.IsLawEnforcementVehicle() ? PED_TYPE_COP : PED_TYPE_GANG1
        );
        veh.vehicleFlags.bCreateRoadBlockPeds = false;
    }
}

// 0x423ED0
void CCarCtrl::RemoveFromInterestingVehicleList(CVehicle* vehicle) {
    for (auto& car : apCarsToKeep) {
        if (car == vehicle) {
            car = nullptr;
            break;
        }
    }
}

// 0x42CE40
void CCarCtrl::ScanForPedDanger(CVehicle* vehicle) {
    const float radius = vehicle == FindPlayerVehicle(-1, false) ? 44.0f : 11.0f;

    //! NOTE: The peds sector scan temporarily clears bit 1 - Restore it afterwards
    const bool wasHonkingAtPed = vehicle->m_autoPilot.carCtrlFlags.bHonkAtPed;

    const CVector pos  = vehicle->GetPosition();
    const float   minX = pos.x - radius;
    const float   maxX = pos.x + radius;
    const float   minY = pos.y - radius;
    const float   maxY = pos.y + radius;

    //! Sector range covered by the scan
    const auto ToSectorIndex = [](float coord) {
        return std::clamp((int32)std::floor(coord * 0.02f + 60.0f), 0, (int32)MAX_SECTORS_X - 1);
    };
    const int32 x0 = ToSectorIndex(minX);
    const int32 x1 = ToSectorIndex(maxX);
    const int32 y0 = ToSectorIndex(minY);
    const int32 y1 = ToSectorIndex(maxY);

    //! NOTE: `CWorld::ClearScanCodes` is protected - `AdvanceCurrentScanCode` wraps it the same way
    CWorld::AdvanceCurrentScanCode();

    float speedOut = (float)vehicle->m_autoPilot.m_nCruiseSpeed;
    for (int32 y = y0; y <= y1; y++) {
        for (int32 x = x0; x <= x1; x++) {
            //! The sector lists are typed per class, but the helper takes a `CPhysical` list - The layouts are identical
            SlowCarDownForPedsSectorList(
                reinterpret_cast<CPtrListDoubleLink<CPhysical*>&>(CWorld::ms_aRepeatSectors[y & 0xF][x & 0xF].Peds),
                vehicle,
                minX, minY, maxX, maxY,
                &speedOut,
                (float)vehicle->m_autoPilot.m_nCruiseSpeed
            );
        }
    }

    vehicle->vehicleFlags.bWarnedPeds = true;
    vehicle->m_autoPilot.carCtrlFlags.bHonkAtPed = wasHonkingAtPed;
}

// 0x42FBC0
bool CCarCtrl::ScriptGenerateOneEmergencyServicesCar(uint32 modelId, CVector posn) {
    if (CStreaming::IsModelLoaded(modelId)) {
        if (auto pAuto = GenerateOneEmergencyServicesCar(modelId, posn)) {
            pAuto->m_autoPilot.m_vecDestinationCoors = posn;
            pAuto->m_autoPilot.SetCarMission(JoinCarWithRoadSystemGotoCoors(pAuto, posn, false, false) ? MISSION_GOTOCOORDINATES_STRAIGHTLINE : MISSION_GOTOCOORDINATES);
            return true;
        }
    }
    return false;
}

// 0x4342A0
void CCarCtrl::SetCoordsOfScriptCar(CVehicle* vehicle, float x, float y, float z, uint8 arg5, uint8 arg6) {
    if (z > -100.0f) {
        z = CWorld::FindGroundZForCoord(x, y);
    }
    if (arg6) {
        z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
    }

    //! NOTE: `+0x10` in the entity vtable is `CEntity::SetIsStatic`
    vehicle->SetIsStatic(false);

    CTheScripts::StuckCars.ClearStuckFlagForCar(GetVehiclePool()->GetIndex(vehicle));

    //! NOTE: `+0x38` in the entity vtable is `CEntity::Teleport`
    vehicle->Teleport(CVector{x, y, z}, false);

    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
        CTheScripts::ClearSpaceForMissionEntity(CVector{x, y, z}, vehicle);
        vehicle->m_autoPilot.m_nTempAction = TEMPACT_NONE;
        return;
    }

    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        vehicle->AsBike()->PlaceOnRoadProperly();
    } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE || vehicle->m_nVehicleType == VEHICLE_TYPE_TRAILER) {
        vehicle->AsAutomobile()->PlaceOnRoadProperly();
    }

    CTheScripts::ClearSpaceForMissionEntity(CVector{x, y, z}, vehicle);
    JoinCarWithRoadAccordingToMission(vehicle);
    vehicle->m_autoPilot.m_nTempAction = TEMPACT_NONE;
}

// 0x4217C0
void CCarCtrl::SetUpDriverAndPassengersForVehicle(CVehicle* vehicle, int32 arg2, int32 arg3, bool arg4, bool arg5, int32 passengersNum) {
    vehicle->SetUpDriver(arg2, arg4, arg5);

    //! Gang members (in the `ePedType` range 14..23) get a smoke to hold ~50% of the time
    if (arg2 > 0xD && arg2 < 0x18 && CGeneral::GetRandomNumber() < 0x3FFF) {
        vehicle->m_pDriver->GiveObjectToPedToHold(ModelIndices::MI_GANG_SMOKE, 1);
    }

    if ((int32)vehicle->m_nMaxPassengers < passengersNum) {
        passengersNum = vehicle->m_nMaxPassengers;
    }

    if (arg3 < passengersNum) {
        for (auto numLeft = passengersNum - arg3; numLeft; numLeft--) {
            arg3 += (float)CGeneral::GetRandomNumber() * 3.051851e-05f < 0.125f;
        }
        if (arg3 < passengersNum) {
            goto havePassengerCount;
        }
    }
    arg3 = passengersNum;

havePassengerCount:
    const auto modelIndex = vehicle->m_nModelIndex;
    if (CModelInfo::IsCarModel(modelIndex)) {
        //! Vans take the driver only
        if (vehicle->GetVehicleModelInfo()->GetAnimFileIndex() == CAnimManager::GetAnimationBlockIndex("van") && arg3 > 0) {
            arg3 = 1;
        }
    }

    for (auto seat = 0; seat < arg3; seat++) {
        if (CPed* ped = vehicle->SetupPassenger(seat, arg2, arg4, arg5)) {
            ped->UpdateStatEnteringVehicle();
            if (arg2 > 0xD && arg2 < 0x18 && CGeneral::GetRandomNumber() < 0x3FFF) {
                ped->GiveObjectToPedToHold(ModelIndices::MI_GANG_SMOKE, 1);
            }
        }
    }
}

// 0x432420
template<typename PtrListType>
void CCarCtrl::SlowCarDownForCarsSectorList(PtrListType& ptrList, CVehicle* vehicle, float minX, float minY, float maxX, float maxY, float* speed, float arg8) {
    for (CPhysical* const phys : ptrList) {
        if (phys == vehicle) {
            continue;
        }
        if (phys->IsScanCodeCurrent()) {
            continue;
        }
        if (!phys->m_bUsesCollision) {
            continue;
        }
        phys->SetCurrentScanCode();

        const CVector boundCentre = phys->GetBoundCentre();
        if (!(minX < boundCentre.x && boundCentre.x < maxX && minY < boundCentre.y && boundCentre.y < maxY)) {
            continue;
        }

        const CVector vehPos = vehicle->GetPosition();
        if (std::abs(boundCentre.z - vehPos.z) >= 10.0f) {
            continue;
        }

        //! The car must lie on the road plane we're driving on
        const CVector vehFwd = vehicle->GetForward();
        const float   along  = CCollision::DistAlongLine2D(vehPos.x, vehPos.y, vehFwd.x, vehFwd.y, boundCentre.x, boundCentre.y);
        if (std::abs(boundCentre.z - (along * vehFwd.z + vehPos.z)) < 3.0f) {
            SlowCarDownForOtherCar(phys, vehicle, speed, arg8);
        }
    }
}

// 0x426220
void CCarCtrl::SlowCarDownForObject(CEntity* entity, CVehicle* vehicle, float* arg3, float arg4) {
    const CVector entityDir = entity->GetPosition() - vehicle->GetPosition();
    const float entityHeading = DotProduct(entityDir, vehicle->GetMatrix().GetForward());
    if (entityHeading > 0.0f && entityHeading < 20.0f) {
        if (entity->GetColModel()->GetBoundRadius() + vehicle->GetColModel()->GetBoundingBox().m_vecMax.x > fabs(DotProduct(entityDir, vehicle->GetMatrix().GetRight()))) {
            if (entityHeading >= 7.0f) {
                *arg3 = std::min(*arg3, (1.0f - (entityHeading - 7.0f) / 13.0f)) * arg4; // Original code multiplies by 0.07692308, which is the recp. of 13
            } else {
                *arg3 = 0.0f;
            }
        }
    }
}

// 0x42D4F0
template<typename PtrListType>
void CCarCtrl::SlowCarDownForObjectsSectorList(PtrListType& ptrList, CVehicle* vehicle, float arg3, float arg4, float arg5, float arg6, float* arg7, float arg8) {
    for (CPhysical* const phys : ptrList) {
        if (phys->IsScanCodeCurrent()) {
            continue;
        }
        phys->SetCurrentScanCode();

        //! Only road-block props are considered
        const eModelID model = (eModelID)phys->m_nModelIndex;
        if (model != ModelIndices::MI_ROADWORKBARRIER1
            && model != ModelIndices::MI_ROADBLOCKFUCKEDCAR1
            && model != ModelIndices::MI_ROADBLOCKFUCKEDCAR2
        ) {
            continue;
        }

        const CVector boundCentre = phys->GetBoundCentre();
        if (!(arg3 < boundCentre.x && boundCentre.x < arg5 && arg4 < boundCentre.y && boundCentre.y < arg6)) {
            continue;
        }

        const CVector vehPos = vehicle->GetPosition();
        if (std::abs(boundCentre.z - vehPos.z) >= 10.0f) {
            continue;
        }

        //! The object must lie on the road plane we're driving on
        const CVector vehFwd = vehicle->GetForward();
        const float   along  = CCollision::DistAlongLine2D(vehPos.x, vehPos.y, vehFwd.x, vehFwd.y, boundCentre.x, boundCentre.y);
        if (std::abs(boundCentre.z - (along * vehFwd.z + vehPos.z)) < 3.0f) {
            SlowCarDownForObject(phys, vehicle, arg7, arg8);
        }
    }
}

// 0x42D0E0
void CCarCtrl::SlowCarDownForOtherCar(CEntity* car1, CVehicle* car2, float* arg3, float arg4) {
    CVehicle* const other = car1->AsVehicle();

    //! Normalize `car2`'s forward (XY only)
    CVector fwd2 = car2->GetForward();
    {
        const float len = std::sqrt(fwd2.x * fwd2.x + fwd2.y * fwd2.y);
        if (len == 0.0f) {
            fwd2.x = 1.0f;
        } else {
            fwd2.x *= 1.0f / len;
            fwd2.y *= 1.0f / len;
        }
    }

    { //! Nothing to do if the other car is behind us
        const CVector otherPos = other->GetPosition();
        const CVector ourPos   = car2->GetPosition();
        if ((otherPos.x - ourPos.x) * fwd2.x + (otherPos.y - ourPos.y) * fwd2.y < 0.0f) {
            return;
        }
    }

    const CVector otherSpeed = other->m_vecMoveSpeed;
    float         vx         = otherSpeed.x * 60.0f - fwd2.x * arg4;
    float         vy         = otherSpeed.y * 60.0f - fwd2.y * arg4;

    if (!other->m_matrix) {
        other->AllocateMatrix();
        other->m_placement.UpdateMatrix(other->m_matrix);
    }

    //! Normalize `car1`'s forward (XY only)
    CVector fwd1 = other->GetForward();
    {
        const float len = std::sqrt(fwd1.x * fwd1.x + fwd1.y * fwd1.y);
        if (len == 0.0f) {
            fwd1.x = 1.0f;
            fwd1.y = 0.0f;
        } else {
            fwd1.x *= 1.0f / len;
            fwd1.y *= 1.0f / len;
        }
        fwd1.z = 0.0f;
    }

    const float d1   = TestCollisionBetween2MovingRects_OnlyFrontBumper(other, car2, vx, vy, &fwd2, &fwd1);
    const float d2   = TestCollisionBetween2MovingRects(car2, other, -vx, -vy, &fwd1, &fwd2);
    const float dist = std::min(d1, d2);

    if (dist < 0.0f) {
        return;
    }

    if (dist < 1.5f) {
        car2->m_autoPilot.carCtrlFlags.bHonkAtCar = true;
        car2->m_autoPilot.m_ObstructingEntity = other;
        CEntity::RegisterReference(car2->m_autoPilot.m_ObstructingEntity);

        //! Scale the speed down the closer we get
        if (dist < 1.0f / arg4) {
            *arg3 = 0.0f;
        } else if (dist >= 3.0f / arg4) {
            float d = (dist - 0.2f) * 0.7692308f; //!< 10/13
            if (d < 0.0f) {
                d = 0.0f;
            }
            const float limit = d * arg4;
            if (*arg3 < limit) {
                *arg3 = limit;
            }
        } else if (*arg3 >= 1.0f) {
            *arg3 = 1.0f;
        }
    }

    //! Both cars are heading straight at each other and haven't been switched to real physics recently
    if (dist >= 0.0f && dist < 0.5f
        && other->GetType() == ENTITY_TYPE_VEHICLE
        && (uint32)(CTimer::GetTimeInMS() - car2->m_autoPilot.m_nTimeSwitchedToRealPhysics) > 15000
        && (uint32)(CTimer::GetTimeInMS() - other->m_autoPilot.m_nTimeSwitchedToRealPhysics) > 15000
    ) {
        const CVector fwdOther = other->GetForward();
        const CVector fwdCar2  = car2->GetForward();
        const float   dot      = fwdOther.x * fwdCar2.x + fwdOther.y * fwdCar2.y;

        //! NOTE: `car2 < car1` is a raw pointer comparison - Only one of the two cars reacts to a given pair
        if (other != FindPlayerVehicle(-1, false) && dot < -0.5f && car2 < other) {
            arg4 *= 0.2f;
            if (arg4 < *arg3) {
                arg4 = *arg3;
            }
            *arg3 = arg4;

            if (car2->GetStatus() == STATUS_SIMPLE) {
                SwitchVehicleToRealPhysics(car2);
            }
            car2->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
            car2->m_autoPilot.m_nTempActionTime  = CTimer::GetTimeInMS() + 1000;
        }
    }
}

// 0x425440
template<typename PtrListType>
void CCarCtrl::SlowCarDownForPedsSectorList(PtrListType& ptrList, CVehicle* vehicle, float minX, float minY, float maxX, float maxY, float* outSpeedMult, float arg8) {
    const CColModel* vehCol = CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel();
    float            vehLen = vehCol->m_boundBox.m_vecMax.x;
    const float      vehWide = vehCol->m_boundBox.m_vecMax.y;

    const CVector vehFwd           = vehicle->GetForward();
    const float   vehSpeedAlongFwd = vehFwd.x * vehicle->m_vecMoveSpeed.x + vehFwd.y * vehicle->m_vecMoveSpeed.y + vehFwd.z * vehicle->m_vecMoveSpeed.z;

    const bool bSlowDownAlways =
           FindPlayerVehicle(-1, false) == vehicle
        || vehicle->m_nVehicleType == VEHICLE_TYPE_TRAIN
        || vehicle->m_nVehicleType == VEHICLE_TYPE_PLANE
        || vehicle->m_nVehicleType == VEHICLE_TYPE_HELI
        || (vehicle->m_autoPilot.m_nCarDrivingStyle != DRIVING_STYLE_STOP_FOR_CARS
            && vehicle->m_autoPilot.m_nCarDrivingStyle != DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS)
        || vehicle->GetStatus() == STATUS_PHYSICS;

    for (CPhysical* const phys : ptrList) {
        if (phys->IsScanCodeCurrent()) {
            continue;
        }
        if (!phys->m_bUsesCollision) {
            continue;
        }
        phys->SetCurrentScanCode();

        const CVector entPos = phys->GetPosition();
        if (!(minX < entPos.x && entPos.x < maxX && minY < entPos.y && entPos.y < maxY)) {
            continue;
        }

        const CVector vehPos = vehicle->GetPosition();
        if (std::abs(entPos.z - vehPos.z) >= 6.0f) {
            continue;
        }

        //! Project the entity onto the vehicle's forward line (and check the expected height there)
        const float along = CCollision::DistAlongLine2D(vehPos.x, vehPos.y, vehFwd.x, vehFwd.y, entPos.x, entPos.y);
        if (std::abs(entPos.z - (along * vehFwd.z + vehPos.z)) >= 3.0f) {
            continue;
        }

        const float relX = entPos.x - vehPos.x;
        const float relY = entPos.y - vehPos.y;
        const float relZ = entPos.z - vehPos.z;

        const float relFwd = relX * vehFwd.x + relY * vehFwd.y + relZ * vehFwd.z;

        //! Brake (and honk) for the ped in front of us
        if (vehicle->m_autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_STOP_FOR_CARS
            || vehicle->m_autoPilot.m_nCarDrivingStyle == DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS
        ) {
            if (!(phys == FindPlayerPed(-1) && CWorld::Players[CWorld::PlayerInFocus].m_pLastTargetVehicle == vehicle)
                && vehWide < relFwd
                && (relFwd - vehWide) < vehSpeedAlongFwd * 200.0f
            ) {
                if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
                    vehLen *= 1.6f;
                }
                if (std::abs(relFwd) < vehLen + 0.5f && (relFwd - vehWide) < 13.0f) {
                    const float t = std::max((relFwd - vehWide) - 1.0f, 0.0f);
                    const float m = t * (1.0f / 13.0f) * arg8;
                    *outSpeedMult = std::min(std::min(m, 1.0f), *outSpeedMult);

                    vehicle->m_autoPilot.m_nCarCtrlFlags |= 0x02; //!< bHonkAtPed
                    if (relFwd - vehWide < 4.0f) {
                        vehicle->m_autoPilot.m_nTempAction     = TEMPACT_WAIT;
                        vehicle->m_autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 4000;
                    }
                    if (relFwd - vehWide < 2.5f) {
                        vehicle->m_autoPilot.m_nTempAction     = TEMPACT_BRAKE;
                        vehicle->m_autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 4000;
                    }
                }
            }
        }

        if (phys->GetType() == ENTITY_TYPE_PED) {
            //! Tell the ped we're honking at him
            if (vehicle == FindPlayerVehicle(-1, false) && vehicle->m_HornCounter != 0) {
                if ((phys->GetPosition() - vehicle->GetPosition()).Magnitude() < 7.0f) {
                    CEventPotentialGetRunOver event{ vehicle };
                    phys->AsPed()->GetEventGroup().Add(&event, false);
                }
            }

            //! We're heading right at the ped, and he's in our way
            if (bSlowDownAlways && vehSpeedAlongFwd != 0.0f) {
                const bool sameDirection = (relFwd >= 0.0f) == (vehSpeedAlongFwd >= 0.0f);
                // NOTE: The `[esp+0x48]` comparison in the original reads a stack slot that is never
                // written to - It is used here as `Abs(vehSpeedAlongFwd) * 50.0f` (its intended value)
                if (sameDirection
                    && vehWide < std::abs(relFwd)
                    && std::abs(vehSpeedAlongFwd) > 0.05f
                    && (std::abs(relFwd) - vehWide) < std::abs(vehSpeedAlongFwd) * 50.0f
                    && std::abs(relFwd) < vehLen + 0.35f
                ) {
                    CEventPotentialGetRunOver event{ vehicle };
                    phys->AsPed()->GetEventGroup().Add(&event, false);
                    if (vehicle->m_pDriver && vehicle->m_pDriver->IsPlayer()) {
                        vehicle->m_pDriver->GetIntelligence()->IncrementAngerAtPlayer(2);
                    }
                }
            }
        }
    }
}

// 0x434790
void CCarCtrl::SlowCarOnRailsDownForTrafficAndLights(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    if ((((int8)CTimer::GetFrameCounter() + (int8)(vehicle->m_nRandomSeed)) & 3) == 0) {
        if (CTrafficLights::ShouldCarStopForLight(vehicle, false) || CTrafficLights::ShouldCarStopForBridge(vehicle)) {
            CCarAI::CarHasReasonToStop(vehicle);
            autoPilot.m_fMaxTrafficSpeed = 0.0f;
        } else {
            autoPilot.m_fMaxTrafficSpeed = FindMaximumSpeedForThisCarInTraffic(vehicle);
        }
    }

    if (autoPilot.m_fMaxTrafficSpeed >= autoPilot.m_speed) {
        autoPilot.ModifySpeed(std::min(autoPilot.m_fMaxTrafficSpeed, CTimer::GetTimeStep() * 0.05f + autoPilot.m_speed));
    } else if (autoPilot.m_speed >= 0.1f) {
        autoPilot.ModifySpeed(std::max(autoPilot.m_fMaxTrafficSpeed, autoPilot.m_speed - CTimer::GetTimeStep() * 0.7f));
    } else if (autoPilot.m_speed != 0.0f) {
        autoPilot.ModifySpeed(0.0f);
    }
}

// 0x428DE0
void CCarCtrl::SteerAIBoatWithPhysicsAttackingPlayer(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    const CVector pos       = vehicle->GetPosition();
    const CVector playerPos = FindPlayerCoors(-1);
    const float   dist      = (playerPos - pos).Magnitude();

    float leadTime = dist * 0.05f;
    if (leadTime >= 2.0f) {
        leadTime = 2.0f;
    }

    //! Own heading (normalized 2D forward vector)
    CVector fwd = vehicle->GetForward();
    {
        const float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y);
        if (len == 0.0f) {
            fwd.x = 1.0f;
        } else {
            fwd.x /= len;
            fwd.y /= len;
        }
    }

    //! Aim at where the player will be
    const CVector playerVel  = FindPlayerSpeed(-1);
    const CVector playerPos2 = FindPlayerCoors(-1);
    const float   targetX    = playerPos2.x + leadTime * playerVel.x * 60.0f;
    const float   targetY    = playerPos2.y + leadTime * playerVel.y * 60.0f;

    float angle = CGeneral::GetATanOfXY(targetX - pos.x, targetY - pos.y) - CGeneral::GetATanOfXY(fwd.x, fwd.y);
    while (angle < -PI) angle += TWO_PI;
    while (angle >  PI) angle -= TWO_PI;

    const float speed = std::sqrt(vehicle->m_vecMoveSpeed.x * vehicle->m_vecMoveSpeed.x + vehicle->m_vecMoveSpeed.y * vehicle->m_vecMoveSpeed.y) * 60.0f;
    float       delta = (float)vehicle->m_autoPilot.m_nCruiseSpeed - speed;
    if (delta <= 0.0f) {
        delta /= (float)vehicle->m_autoPilot.m_nCruiseSpeed;
        *arg3 = (delta <= 0.25f) ? 1.0f - (0.25f - delta) * 4.0f : 1.0f;
    } else {
        *arg3 = (delta < -5.0f) ? -0.2f : -0.1f;
    }

    *arg4 = 0.0f;
    *arg2 = angle;
    *arg5 = false;

    //! Close range gun run
    if (vehicle->m_nModelIndex == MODEL_PREDATOR && dist < 40.0f && angle < 0.15f) {
        vehicle->FireFixedMachineGuns();
    }
}

// 0x429090
void CCarCtrl::SteerAIBoatWithPhysicsCirclingPlayer(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    const CVector pos       = vehicle->GetPosition();
    const CVector playerPos = FindPlayerCoors(-1);

    CVector2D toPlayer{ playerPos.x - pos.x, playerPos.y - pos.y };
    toPlayer.Normalise(); //!< Unused by the original, kept for fidelity

    //! Offset the aim point perpendicular to the player direction
    float offX, offY;
    if ((vehicle->m_nRandomSeed & 1) == 0) {
        offX =  (playerPos.y - pos.y) * 26.0f;
        offY = -(playerPos.x - pos.x) * 26.0f;
    } else {
        offX =  (playerPos.y - pos.y) * -12.0f;
        offY = -(playerPos.x - pos.x) * -12.0f;
    }

    const CVector playerPos2 = FindPlayerCoors(-1);

    CVector fwd = vehicle->GetForward();
    {
        const float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y);
        if (len == 0.0f) {
            fwd.x = 1.0f;
        } else {
            fwd.x /= len;
            fwd.y /= len;
        }
    }

    const CVector pos2 = vehicle->GetPosition();
    float angle = CGeneral::GetATanOfXY((offX + playerPos2.x) - pos2.x, (offY + playerPos2.y) - pos2.y)
                - CGeneral::GetATanOfXY(fwd.x, fwd.y);
    while (angle < -PI) angle += TWO_PI;
    while (angle >  PI) angle -= TWO_PI;

    const float speed = std::sqrt(vehicle->m_vecMoveSpeed.x * vehicle->m_vecMoveSpeed.x + vehicle->m_vecMoveSpeed.y * vehicle->m_vecMoveSpeed.y) * 60.0f;
    float       delta = (float)vehicle->m_autoPilot.m_nCruiseSpeed - speed;
    if (delta <= 0.0f) {
        delta /= (float)vehicle->m_autoPilot.m_nCruiseSpeed;
        *arg3 = (delta <= 0.25f) ? 1.0f - (0.25f - delta) * 4.0f : 1.0f;
    } else {
        *arg3 = (delta < -5.0f) ? -0.2f : -0.1f;
    }

    *arg4 = 0.0f;
    *arg2 = angle;
    *arg5 = false;
}

// 0x428BE0
void CCarCtrl::SteerAIBoatWithPhysicsHeadingForTarget(CVehicle* vehicle, float targetX, float targetY, float* arg4, float* arg5, float* arg6) {
    //! Boat heading (normalized 2D forward vector)
    CVector fwd = vehicle->GetForward();
    {
        const float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y);
        if (len == 0.0f) {
            fwd.x = 1.0f;
        } else {
            fwd.x /= len;
            fwd.y /= len;
        }
    }

    const CVector pos = vehicle->GetPosition();
    float angle = CGeneral::GetATanOfXY(targetX - pos.x, targetY - pos.y) - CGeneral::GetATanOfXY(fwd.x, fwd.y);
    while (angle < -PI) angle += TWO_PI;
    while (angle >  PI) angle -= TWO_PI;
    angle = std::clamp(angle, -0.5f, 0.5f);

    const float speed = std::sqrt(vehicle->m_vecMoveSpeed.x * vehicle->m_vecMoveSpeed.x + vehicle->m_vecMoveSpeed.y * vehicle->m_vecMoveSpeed.y) * 60.0f;
    float       delta = (float)vehicle->m_autoPilot.m_nCruiseSpeed - speed;
    if (delta <= 0.0f) {
        delta /= (float)vehicle->m_autoPilot.m_nCruiseSpeed;
        *arg5 = (delta <= 0.25f) ? 1.0f - (0.25f - delta) * 4.0f : 1.0f;
        *arg6 = 0.0f;
        *arg4 = angle;
        return;
    }

    *arg5 = -0.1f;
    if (delta < -5.0f) {
        *arg5 = -0.2f;
    }
    *arg6 = 0.0f;
    *arg4 = -angle;
}

// 0x422B20
void CCarCtrl::SteerAICarBlockingPlayerForwardAndBack(CVehicle* vehicle, float* arg2, float* arg3, float* arg4, bool* arg5) {
    *arg2 = 0.0f;
    *arg5 = false;

    const CVector playerSpeed = FindPlayerSpeed(-1);

    CEntity* const player = FindPlayerEntity(-1);
    if (!player->m_matrix) {
        player->AllocateMatrix();
        player->m_placement.UpdateMatrix(player->m_matrix);
    }

    const CMatrix& vehMat = vehicle->GetMatrix();

    CVector right = vehMat.GetRight();
    right.z = 0.0f;
    right.Normalise();

    CVector forward = vehMat.GetForward();
    forward.z = 0.0f;
    forward.Normalise();

    const CVector speedDir{
        playerSpeed.x + player->m_matrix->GetForward().x * 0.1f,
        playerSpeed.y + player->m_matrix->GetForward().y * 0.1f,
        0.0f
    };

    const CVector vehPos    = vehicle->GetPosition();
    const CVector playerPos = FindPlayerCoors(-1);

    float denom = right.x * speedDir.x + right.y * speedDir.y;
    if (denom == 0.0f) {
        denom = 0.01f;
    }

    const float t = -(((playerPos.x - vehPos.x) * right.x + right.y * (playerPos.y - vehPos.y)) / denom);
    if (t < 0.0f) { //!< Player is behind the blocking car
        *arg3 = 0.0f;
        *arg4 = 0.0f;
        return;
    }

    const float forwardSpeed = forward.x * vehicle->m_vecMoveSpeed.x + forward.y * vehicle->m_vecMoveSpeed.y;

    float v = (playerPos.x - vehPos.x) * forward.x
            + forward.y * (playerPos.y - vehPos.y)
            + (forward.x * speedDir.x + forward.y * speedDir.y) * t
            - forwardSpeed * t;

    if (v > 0.0f) { //!< Player is ahead of us in our own frame
        v *= 0.1f;
        if (v > 1.0f) {
            v = 1.0f;
        }
        *arg3 = v;
        *arg4 = 0.0f;
        return;
    }

    if (forwardSpeed <= 0.0f) { //!< We're not moving forward
        v *= 0.1f;
        if (v < -1.0f) {
            v = -1.0f;
        }
        *arg3 = v;
        *arg4 = 0.0f;
    } else { //!< We're moving forward, so reverse
        v *= -0.1f;
        *arg3 = 0.0f;
        if (v > 1.0f) {
            v = 1.0f;
        }
        *arg4 = v;
        if (v > 0.95f) {
            *arg5 = true;
        }
    }
}

// 0x433BA0
void CCarCtrl::SteerAICarParkParallel(CVehicle* vehicle, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    auto& autoPilot = vehicle->m_autoPilot;

    if (!ThePaths.m_pPathNodes[autoPilot.m_startingRouteNode.m_wAreaId]
        || !ThePaths.m_pPathNodes[autoPilot.m_currentAddress.m_wAreaId]
    ) {
        autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
        *outSteer               = 0.0f;
        *outThrottle            = 0.0f;
        *outBrake               = 0.0f;
        return;
    }

    const CPathNode& curNode = ThePaths.m_pPathNodes[autoPilot.m_currentAddress.m_wAreaId][autoPilot.m_currentAddress.m_wNodeId];

    CVector target = curNode.GetPosition();
    if (autoPilot.m_nCarMission == MISSION_PARK_PARALLEL) {
        //! Aim one unit past the node we're on
        const CPathNode& nextNode = ThePaths.m_pPathNodes[autoPilot.m_startingRouteNode.m_wAreaId][autoPilot.m_startingRouteNode.m_wNodeId];
        CVector          dir      = nextNode.GetPosition() - curNode.GetPosition();
        dir.Normalise();
        target += dir;
    }

    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, outSteer, outThrottle, outBrake, outHandBrake);

    autoPilot.m_nCruiseSpeed = std::min<uint8>(autoPilot.m_nCruiseSpeed, 8);

    const CVector vehPos = vehicle->GetPosition();
    const float   dist   = std::sqrt((target.x - vehPos.x) * (target.x - vehPos.x) + (target.y - vehPos.y) * (target.y - vehPos.y));

    if (autoPilot.m_nCarMission == MISSION_PARK_PARALLEL) {
        if (dist < 4.0f) {
            autoPilot.m_nCarMission = MISSION_PARK_PARALLEL_2;
        }
        return;
    }

    if (dist < 2.0f) {
        autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
        vehicle->vehicleFlags.bEngineOn = false;

        if (vehicle->m_pDriver) {
            vehicle->m_pDriver->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar(0, true, false), (ePrimaryTasks)3);
        }
        for (CPed* const passenger : vehicle->m_apPassengers) {
            if (passenger) {
                passenger->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar(0, true, false), (ePrimaryTasks)3);
            }
        }
    }
}

// 0x433EA0
void CCarCtrl::SteerAICarParkPerpendicular(CVehicle* vehicle, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    auto& autoPilot = vehicle->m_autoPilot;

    if (!ThePaths.m_pPathNodes[autoPilot.m_startingRouteNode.m_wAreaId]
        || !ThePaths.m_pPathNodes[autoPilot.m_currentAddress.m_wAreaId]
    ) {
        autoPilot.m_nCarMission = MISSION_STOP_FOREVER;
        *outSteer               = 0.0f;
        *outThrottle            = 0.0f;
        *outBrake               = 0.0f;
        return;
    }

    const CPathNode& curNode  = ThePaths.m_pPathNodes[autoPilot.m_currentAddress.m_wAreaId][autoPilot.m_currentAddress.m_wNodeId];
    const CPathNode& nextNode = ThePaths.m_pPathNodes[autoPilot.m_startingRouteNode.m_wAreaId][autoPilot.m_startingRouteNode.m_wNodeId];

    const CVector curPos  = curNode.GetPosition();
    const CVector nextPos = nextNode.GetPosition();
    const CVector carPos  = vehicle->GetPosition();

    //! Close enough to the parking spot's axis
    if (autoPilot.m_nCarMission == MISSION_PARK_PERPENDICULAR) {
        if (CCollision::DistToMathematicalLine(&curPos, &nextPos, &carPos) < 6.0f) {
            autoPilot.m_nCarMission = MISSION_PARK_PERPENDICULAR_2;
        }
    }

    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, curPos.x, curPos.y, outSteer, outThrottle, outBrake, outHandBrake);

    autoPilot.m_nCruiseSpeed = std::min<uint8>(autoPilot.m_nCruiseSpeed, 8);

    if (std::sqrt((curPos.x - carPos.x) * (curPos.x - carPos.x) + (curPos.y - carPos.y) * (curPos.y - carPos.y)) < 2.0f) {
        autoPilot.m_nCarMission            = MISSION_STOP_FOREVER;
        autoPilot.movementFlags.bIsParked  = true;
        vehicle->vehicleFlags.bEngineOn    = false;

        if (vehicle->m_pDriver) {
            vehicle->m_pDriver->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar(0, true, false), (ePrimaryTasks)3);
        }
        for (CPed* const passenger : vehicle->m_apPassengers) {
            if (passenger) {
                passenger->GetTaskManager().SetTask(new CTaskComplexLeaveAnyCar(0, true, false), (ePrimaryTasks)3);
            }
        }
    }
}

// 0x4336D0
void CCarCtrl::SteerAICarTowardsPointInEscort(CVehicle* vehicle, CVehicle* escortTarget, float offsetX, float offsetY, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    auto& autoPilot = vehicle->m_autoPilot;

    //! The point to drive to (Offset from the escort, in its local space)
    const CVector offset = escortTarget->GetMatrix().TransformPoint(CVector{offsetX, offsetY, 0.0f});
    const CVector escortFwd = escortTarget->GetForward();

    const CVector point{
        offset.x + escortTarget->m_vecMoveSpeed.x + escortFwd.x * 3.0f,
        offset.y + escortTarget->m_vecMoveSpeed.y + escortFwd.y * 3.0f,
        0.0f
    };

    *outHandBrake = false;

    const CVector carPos     = vehicle->GetPosition();
    const CVector carFront   = vehicle->GetForward();
    const float   carHeading = CGeneral::GetATanOfXY(carFront.x, carFront.y);

    float steer = FindAngleToWeaveThroughTraffic(
        vehicle, nullptr,
        CGeneral::GetATanOfXY(point.x - carPos.x, point.y - carPos.y),
        carHeading,
        1.0f
    ) - carHeading;
    steer = CGeneral::LimitRadianAngle(steer);
    steer = std::clamp(steer, -FindMaxSteerAngle(vehicle), FindMaxSteerAngle(vehicle));

    //! NOTE: The original computes this heading and then discards it
    (void)CGeneral::GetATanOfXY(point.x - carPos.x, point.y - carPos.y);

    const float dx    = point.x - carPos.x;
    const float dy    = point.y - carPos.y;
    const float along = dx * carFront.x + dy * carFront.y;
    const float dist  = std::sqrt(dx * dx + dy * dy);

    float targetSpeed;
    if (along <= 0.5f) {
        if (dist < 15.0f) {
            *outSteer    = 0.0f;
            *outThrottle = 0.0f;
            *outBrake    = along < -3.0f ? 1.0f : 0.1f;
            return;
        }
        targetSpeed = 8.0f;
    } else {
        const float escortSpeed = escortTarget->m_vecMoveSpeed.Magnitude() * 60.0f;
        if (dist >= 15.0f) {
            targetSpeed = 300.0f;
        } else {
            targetSpeed = std::max(dist * 3.5f, escortSpeed + std::min(4.0f, along - 0.6f));
        }
        targetSpeed = std::min(targetSpeed, escortSpeed + 10.0f);
    }

    const float currentSpeed = vehicle->m_vecMoveSpeed.Magnitude() * 60.0f;
    const float diff         = targetSpeed - currentSpeed;

    *outBrake = 0.0f;
    if (diff > 0.0f) {
        *outThrottle = currentSpeed < 25.0f ? std::min(1.0f, diff * 0.1f) : 1.0f;
    } else {
        *outThrottle = 0.0f;
        *outBrake    = std::min(0.5f, -diff * 0.05f);
    }

    *outSteer = steer;
}

// 0x437C20
void CCarCtrl::SteerAICarWithPhysics(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    float steer     = 0.0f;
    float throttle  = 0.0f;
    float brake     = 0.0f;
    bool  handBrake = false;

    //! Cars on a recording only steer by themselves when their AI is enabled
    if (autoPilot.m_vehicleRecordingId >= 0 && !CVehicleRecording::bUseCarAI[autoPilot.m_vehicleRecordingId]) {
        return;
    }

    SwitchBetweenPhysicsAndGhost(vehicle);
    autoPilot.m_ObstructingEntity = nullptr;

    //! Missions that require a target
    switch (autoPilot.m_nCarMission) {
    case MISSION_RAMCAR_FARAWAY: case MISSION_RAMCAR_CLOSE:
    case MISSION_BLOCKCAR_FARAWAY: case MISSION_BLOCKCAR_CLOSE: case MISSION_BLOCKCAR_HANDBRAKESTOP:
    case MISSION_PROTECTION_REAR: case MISSION_PROTECTION_FRONT:
    case MISSION_ESCORT_LEFT: case MISSION_ESCORT_RIGHT: case MISSION_ESCORT_REAR: case MISSION_ESCORT_FRONT:
    case MISSION_FOLLOW_RECORDED_PATH:
    case MISSION_FOLLOWCAR_FARAWAY: case MISSION_FOLLOWCAR_CLOSE:
    case MISSION_KILLPED_FARAWAY: case MISSION_KILLPED_CLOSE:
        if (!autoPilot.m_TargetEntity) {
            autoPilot.m_nCarMission = MISSION_NONE;
        }
        break;
    default:
        break;
    }
    if ((int32)autoPilot.m_nCarMission == 0x27 && !autoPilot.m_TargetEntity) {
        autoPilot.m_nCarMission = (eCarMission)0x2A;
    }

    if (autoPilot.movementFlags.bIsStopped) {
        brake = 0.2f;

        const CVector pos = vehicle->GetPosition();
        if (ThePaths.AreNodesLoadedForArea(pos.x - 270.0f, pos.x + 270.0f, pos.y - 270.0f, pos.y + 270.0f)) {
            autoPilot.movementFlags.bIsStopped = false;
            JoinCarWithRoadAccordingToMission(vehicle);
        }
    } else {
        //! Parked cars that need to move again first have to back out
        if (autoPilot.movementFlags.bIsParked && autoPilot.m_nCarMission != MISSION_STOP_FOREVER && autoPilot.m_nCarMission != MISSION_NONE) {
            autoPilot.m_nTempAction     = (eAutoPilotTempAction)0x16;
            autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 2000;
            autoPilot.movementFlags.bIsParked = false;

            const CNodeAddress oldCurrent = autoPilot.m_currentAddress;
            autoPilot.m_currentAddress    = autoPilot.m_startingRouteNode;
            autoPilot.m_startingRouteNode = oldCurrent;
            autoPilot.m_endingRouteNode   = oldCurrent;
        }

        const uint32 now = CTimer::GetTimeInMS();
        switch (autoPilot.m_nTempAction) {
        case TEMPACT_WAIT:
            brake = 0.2f;
            break;

        case TEMPACT_REVERSE: {
            SteerAICarWithPhysics_OnlyMission(vehicle, &steer, &throttle, &brake, &handBrake);
            steer = -steer;
            if (DotProduct(vehicle->m_vecMoveSpeed, vehicle->GetForward()) <= 0.04f) {
                brake    = 0.0f;
                throttle = -0.5f;
            } else {
                brake    = 0.5f;
                throttle = 0.0f;
            }
            if (autoPilot.m_nTempActionTime < now) {
                autoPilot.m_nTempAction           = TEMPACT_NONE;
                autoPilot.m_nTimeToStartMission   = now;
            }
            break;
        }

        case TEMPACT_HANDBRAKETURNLEFT:
            handBrake = true;
            steer     = 1.0f;
            throttle  = 0.0f;
            brake     = 0.0f;
            if (autoPilot.m_nTempActionTime < now) {
                autoPilot.m_nTempAction         = TEMPACT_NONE;
                autoPilot.m_nTimeToStartMission = now;
            }
            break;

        case TEMPACT_HANDBRAKETURNRIGHT:
            handBrake = true;
            steer     = -1.0f;
            throttle  = 0.0f;
            brake     = 0.0f;
            if (autoPilot.m_nTempActionTime < now) {
                autoPilot.m_nTempAction         = TEMPACT_NONE;
                autoPilot.m_nTimeToStartMission = now;
            }
            break;

        case TEMPACT_HANDBRAKESTRAIGHT:
            handBrake = true;
            steer     = 0.0f;
            throttle  = 0.0f;
            brake     = 0.0f;
            if (autoPilot.m_nTempActionTime < now) {
                autoPilot.m_nTempAction         = TEMPACT_NONE;
                autoPilot.m_nTimeToStartMission = now;
            }
            break;

        case TEMPACT_TURNLEFT:
            steer    = 1.0f;
            throttle = 1.0f;
            break;

        case TEMPACT_TURNRIGHT:
            steer    = -1.0f;
            throttle = 1.0f;
            break;

        case TEMPACT_GOFORWARD:
            throttle = 0.5f;
            break;

        case TEMPACT_SWIRVELEFT:
        case TEMPACT_SWIRVERIGHT:
        case TEMPACT_SWIRVELEFT_STOP:
        case TEMPACT_SWIRVERIGHT_STOP: {
            const bool left = autoPilot.m_nTempAction == TEMPACT_SWIRVELEFT || autoPilot.m_nTempAction == TEMPACT_SWIRVELEFT_STOP;
            steer    = left ? 0.25f : -0.25f;
            throttle = 0.0f;
            brake    = 0.001f;
            if (autoPilot.m_nTempActionTime - 1250 < now) {
                steer = -steer;
            }
            if (autoPilot.m_nTempActionTime < now) {
                if (autoPilot.m_nTempAction == TEMPACT_SWIRVELEFT_STOP || autoPilot.m_nTempAction == TEMPACT_SWIRVERIGHT_STOP) {
                    autoPilot.m_nTempAction     = TEMPACT_WAIT;
                    autoPilot.m_nTempActionTime = now + 4000;
                } else {
                    autoPilot.m_nTempAction = TEMPACT_NONE;
                }
            }
            break;
        }

        case TEMPACT_REVERSE_LEFT:
        case TEMPACT_REVERSE_RIGHT:
            brake    = -0.75f;
            throttle = 0.0f;
            steer    = FindMaxSteerAngle(vehicle) * (autoPilot.m_nTempAction == TEMPACT_REVERSE_LEFT ? 1.0f : -1.0f);
            break;

        case TEMPACT_PLANE_FLY_UP:
        case TEMPACT_PLANE_FLY_STRAIGHT:
        case TEMPACT_PLANE_SHARP_LEFT:
        case TEMPACT_PLANE_SHARP_RIGHT:
            if (autoPilot.m_nTempActionTime < now) {
                autoPilot.m_nTempAction = TEMPACT_NONE;
            }
            SteerAICarWithPhysics_OnlyMission(vehicle, &steer, &throttle, &brake, &handBrake);
            break;

        case TEMPACT_HEADON_COLLISION:
            steer = std::clamp(
                vehicle->m_fSteerAngle + (vehicle->m_fSteerAngle > 0.0f ? 1.0f : -1.0f) * CTimer::GetTimeStep() * 0.05f,
                -0.5f, 0.5f
            );
            throttle = 0.0f;
            brake    = autoPilot.m_nTempActionTime < now ? 0.0f : 1.0f;
            break;

        case TEMPACT_REVERSE_STRAIGHT: //!< 0x16 - Back out
            throttle = 0.0f;
            if (DotProduct(vehicle->m_vecMoveSpeed, vehicle->GetForward()) <= 0.1f) {
                throttle = -0.5f;
                brake    = 0.0f;
            } else {
                brake = 0.5f;
            }
            break;

        case TEMPACT_BOOST_USE_STEERING_ANGLE:
            steer    = vehicle->m_fSteerAngle;
            throttle = 1.0f;
            brake    = 0.0f;
            vehicle->m_vecMoveSpeed += vehicle->GetForward() * (CTimer::GetTimeStep() * 0.012f);
            break;

        case TEMPACT_BRAKE: //!< 0x18
            throttle = 0.5f;
            brake    = 0.0f;
            steer    = 0.0f;
            autoPilot.m_nTempAction              = TEMPACT_NONE;
            autoPilot.m_nTimeToStartMission      = now;
            autoPilot.m_nTimeSwitchedToRealPhysics = now;
            break;

        default:
            SteerAICarWithPhysics_OnlyMission(vehicle, &steer, &throttle, &brake, &handBrake);
            break;
        }
    }

    vehicle->m_BrakePedal = brake;
    vehicle->m_fSteerAngle = steer;
    vehicle->m_GasPedal    = throttle;

    vehicle->vehicleFlags.bIsHandbrakeOn = handBrake;
}

// 0x434900
void CCarCtrl::SteerAICarWithPhysicsFollowPath(CVehicle* vehicle, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    auto& autoPilot = vehicle->m_autoPilot;

    if (StopCarIfNodesAreInvalid(vehicle)) {
        return;
    }

    //! The car's heading as a normalised 2D front vector
    float headingX = vehicle->GetForward().x;
    float headingY = vehicle->GetForward().y;
    if (const float len = std::sqrt(headingX * headingX + headingY * headingY); len == 0.0f) {
        headingX = 1.0f;
    } else {
        headingX /= len;
        headingY /= len;
    }

    const auto Stop = [&] {
        *outBrake     = 1.0f;
        *outSteer     = 0.0f;
        *outThrottle  = 0.0f;
        *outHandBrake = false;
    };

    //! The raw (Packed) value of a path link address
    const auto RawAddr = [](const CCarPathLinkAddress& addr) {
        return *reinterpret_cast<const uint16*>(&addr);
    };

    CCarPathLinkAddress curAddr  = autoPilot.m_nCurrentPathNodeInfo;
    CCarPathLinkAddress nextAddr = autoPilot.m_nNextPathNodeInfo;

    if (!ThePaths.m_pPathNodes[curAddr.m_wAreaId] || !ThePaths.m_pPathNodes[nextAddr.m_wAreaId]) {
        Stop();
        return;
    }

    //! Link directions, taking the direction we're driving in into account
    float dirCurrX = (float)ThePaths.GetCarPathLink(curAddr).m_dir.x * 0.01f * (float)autoPilot._smthCurr;
    float dirCurrY = (float)ThePaths.GetCarPathLink(curAddr).m_dir.y * 0.01f * (float)autoPilot._smthCurr;
    float dirNextX = (float)ThePaths.GetCarPathLink(nextAddr).m_dir.x * 0.01f * (float)autoPilot._smthNext;
    float dirNextY = (float)ThePaths.GetCarPathLink(nextAddr).m_dir.y * 0.01f * (float)autoPilot._smthNext;

    //! How far to the side of the link's centre line we should drive
    float laneCurr = (ThePaths.GetCarPathLink(curAddr).OneWayLaneOffset() + (float)autoPilot.m_nCurrentLane) * 5.4f;
    float laneNext = (ThePaths.GetCarPathLink(nextAddr).OneWayLaneOffset() + (float)autoPilot.m_nNextLane) * 5.4f;
    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
        laneCurr += 1.4580001f;
        laneNext += 1.4580001f;
    }

    const CVector2D carPos      = vehicle->GetPosition2D();
    const CVector2D currLinkPos = ThePaths.GetCarPathLink(curAddr).GetNodeCoors();
    const CVector2D nextLinkPos = ThePaths.GetCarPathLink(nextAddr).GetNodeCoors();

    //! Lane corrected positions of both links
    float pointCurrX = currLinkPos.x + laneCurr * dirCurrY;
    float pointCurrY = currLinkPos.y - laneCurr * dirCurrX;
    float pointNextX = nextLinkPos.x + laneNext * dirNextY;
    float pointNextY = nextLinkPos.y - laneNext * dirNextX;

    float dist = std::sqrt(
        (carPos.x - pointCurrX) * (carPos.x - pointCurrX)
        + (carPos.y - pointCurrY) * (carPos.y - pointCurrY)
    );

    //! Are we already heading along the current lane, so that there's no need to pick a new node?
    bool followCurrentLane = false;
    if (dist > 5.0f) {
        const float linkX   = pointNextX - pointCurrX;
        const float linkY   = pointNextY - pointCurrY;
        const float dot     = linkX * (carPos.x - pointCurrX) + linkY * (carPos.y - pointCurrY);

        if (dot <= 0.0f || dist > 8.0f) {
            if (dot / (std::sqrt(linkX * linkX + linkY * linkY) * dist) <= 0.7f && RawAddr(curAddr) != RawAddr(nextAddr)) {
                followCurrentLane = true;
            }
        }
    }

    if (!followCurrentLane) {
        if (PickNextNodeAccordingStrategy(vehicle)) {
            //! The node was picked - Some missions don't need to follow the lane any more
            switch (autoPilot.m_nCarMission) {
            case MISSION_RAMPLAYER_FARAWAY:
                autoPilot.m_nCarMission = MISSION_RAMPLAYER_CLOSE;
                SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, FindPlayerCoors(-1).x, FindPlayerCoors(-1).y, outSteer, outThrottle, outBrake, outHandBrake);
                return;
            case MISSION_BLOCKPLAYER_FARAWAY:
                autoPilot.m_nCarMission = MISSION_BLOCKPLAYER_CLOSE;
                SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, FindPlayerCoors(-1).x, FindPlayerCoors(-1).y, outSteer, outThrottle, outBrake, outHandBrake);
                return;
            case MISSION_GOTOCOORDINATES:
                autoPilot.m_nCarMission = MISSION_GOTOCOORDINATES_STRAIGHTLINE;
                SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, autoPilot.m_vecDestinationCoors.x, autoPilot.m_vecDestinationCoors.y, outSteer, outThrottle, outBrake, outHandBrake);
                return;
            case MISSION_GOTOCOORDINATES_ACCURATE:
                autoPilot.m_nCarMission = MISSION_GOTOCOORDINATES_STRAIGHTLINE_ACCURATE;
                SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, autoPilot.m_vecDestinationCoors.x, autoPilot.m_vecDestinationCoors.y, outSteer, outThrottle, outBrake, outHandBrake);
                return;

            //! These all turn into their close range counterpart
            case MISSION_RAMCAR_FARAWAY:
            case MISSION_BLOCKCAR_FARAWAY:
            case MISSION_ESCORT_LEFT_FARAWAY:
            case MISSION_ESCORT_RIGHT_FARAWAY:
            case MISSION_ESCORT_REAR_FARAWAY:
            case MISSION_ESCORT_FRONT_FARAWAY: {
                switch (autoPilot.m_nCarMission) {
                case MISSION_RAMCAR_FARAWAY:        autoPilot.m_nCarMission = MISSION_RAMCAR_CLOSE; break;
                case MISSION_BLOCKCAR_FARAWAY:      autoPilot.m_nCarMission = MISSION_BLOCKCAR_CLOSE; break;
                case MISSION_ESCORT_LEFT_FARAWAY:   autoPilot.m_nCarMission = MISSION_ESCORT_LEFT; break;
                case MISSION_ESCORT_RIGHT_FARAWAY:  autoPilot.m_nCarMission = MISSION_ESCORT_RIGHT; break;
                case MISSION_ESCORT_REAR_FARAWAY:   autoPilot.m_nCarMission = MISSION_ESCORT_REAR; break;
                case MISSION_ESCORT_FRONT_FARAWAY:  autoPilot.m_nCarMission = MISSION_ESCORT_FRONT; break;
                default:                            NOTSA_UNREACHABLE();
                }
                const CVector targetPos = autoPilot.m_TargetEntity->GetPosition();
                SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, targetPos.x, targetPos.y, outSteer, outThrottle, outBrake, outHandBrake);
                return;
            }
            case MISSION_APPROACHPLAYER_FARAWAY:
                autoPilot.m_nCarMission = MISSION_APPROACHPLAYER_CLOSE;
                SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, FindPlayerCoors(-1).x, FindPlayerCoors(-1).y, outSteer, outThrottle, outBrake, outHandBrake);
                return;
            default:
                break;
            }
        }

        //! The path data changed - Recompute everything for the new links
        curAddr  = autoPilot.m_nCurrentPathNodeInfo;
        nextAddr = autoPilot.m_nNextPathNodeInfo;

        if (!ThePaths.m_pPathNodes[curAddr.m_wAreaId] || !ThePaths.m_pPathNodes[nextAddr.m_wAreaId]) {
            Stop();
            return;
        }

        const CCarPathLink& curLink  = ThePaths.GetCarPathLink(curAddr);
        const CCarPathLink& nextLink = ThePaths.GetCarPathLink(nextAddr);

        laneCurr = (curLink.OneWayLaneOffset() + (float)autoPilot.m_nCurrentLane) * 5.4f;

        const CVector2D newCurrPos = curLink.GetNodeCoors();
        dist = std::sqrt(
            ((laneCurr * dirCurrY + newCurrPos.x) - carPos.x) * ((laneCurr * dirCurrY + newCurrPos.x) - carPos.x)
            + ((newCurrPos.y - laneCurr * dirCurrX) - carPos.y) * ((newCurrPos.y - laneCurr * dirCurrX) - carPos.y)
        );

        dirCurrX = (float)curLink.m_dir.x * 0.01f * (float)autoPilot._smthCurr;
        dirCurrY = (float)curLink.m_dir.y * 0.01f * (float)autoPilot._smthCurr;
        dirNextX = (float)nextLink.m_dir.x * 0.01f * (float)autoPilot._smthNext;
        dirNextY = (float)nextLink.m_dir.y * 0.01f * (float)autoPilot._smthNext;

        pointCurrX = newCurrPos.x + laneCurr * dirCurrY;
        pointCurrY = newCurrPos.y - laneCurr * dirCurrX;
    }

    //! Steer towards the middle of the lane, a bit behind the link's point
    CVector2D aimPoint{ pointCurrX, pointCurrY };
    if (dist <= 40.0f) {
        aimPoint.x -= dist * dirCurrX * 0.35f;
        aimPoint.y -= dist * dirCurrY * 0.35f;
    }

    const float aimAngle   = CGeneral::GetATanOfXY(aimPoint.x - carPos.x, aimPoint.y - carPos.y);
    const float carHeading = CGeneral::GetATanOfXY(headingX, headingY);

    float steerAngle = aimAngle;
    switch ((int32)autoPilot.m_nCarDrivingStyle) {
    case 2:
    case 5:
    case 6:
        steerAngle = FindAngleToWeaveThroughTraffic(vehicle, nullptr, aimAngle, carHeading, 1.0f);
        break;
    default:
        break;
    }

    steerAngle = CGeneral::LimitRadianAngle(steerAngle - carHeading);
    {
        const float maxSteerAngle = FindMaxSteerAngle(vehicle);
        steerAngle = std::clamp(steerAngle, -maxSteerAngle, maxSteerAngle);
    }

    //! How fast we're going along the front of the car
    const float forwardSpeed = DotProduct(vehicle->m_vecMoveSpeed, vehicle->GetForward()) * 60.0f;

    //! How much the traffic in front of us allows us to go
    float trafficSpeedMult;
    switch ((int32)autoPilot.m_nCarDrivingStyle) {
    case 0:
    case 1:
    case 4:
    case 6:
        trafficSpeedMult = FindMaximumSpeedForThisCarInTraffic(vehicle) / (float)autoPilot.m_nCruiseSpeed;
        break;
    default:
        trafficSpeedMult = 1.0f;
        break;
    }

    //! Traffic lights (And bridges) might also stop us
    switch ((int32)autoPilot.m_nCarDrivingStyle) {
    case 0:
    case 1:
    case 5:
    case 6:
        if (CTrafficLights::ShouldCarStopForLight(vehicle, false)) {
            CCarAI::CarHasReasonToStop(vehicle);
            trafficSpeedMult = 0.0f;
        }
        break;
    default:
        break;
    }
    if (CTrafficLights::ShouldCarStopForBridge(vehicle)) {
        CCarAI::CarHasReasonToStop(vehicle);
        trafficSpeedMult = 0.0f;
    }

    //! How much the current corner allows us to go
    const float cornerSpeedMult = FindSpeedMultiplier(
        CGeneral::GetATanOfXY(dirCurrX, dirCurrY) - CGeneral::GetATanOfXY(dirNextX, dirNextY),
        0.1f, 1.2f, 0.4f
    );

    //! How much of a turn is left until we're on the lane
    float laneSpeedMult = FindSpeedMultiplier(
        CGeneral::GetATanOfXY(
            (ThePaths.GetCarPathLink(curAddr).OneWayLaneOffset() + (float)autoPilot.m_nCurrentLane) * 5.4f * dirCurrY + ThePaths.GetCarPathLink(curAddr).GetNodeCoors().x - carPos.x,
            ThePaths.GetCarPathLink(curAddr).GetNodeCoors().y - (ThePaths.GetCarPathLink(curAddr).OneWayLaneOffset() + (float)autoPilot.m_nCurrentLane) * 5.4f * dirCurrX - carPos.y
        ) - carHeading,
        0.4f, 1.2f, 0.4f
    );

    float cornerFactor = 1.0f;
    if (dist <= 40.0f && autoPilot.m_nCruiseSpeed >= 12) {
        cornerFactor = 1.0f - (1.0f - cornerSpeedMult) * (1.0f - dist * 0.025f);
    }

    laneSpeedMult = std::min({ laneSpeedMult, cornerFactor, trafficSpeedMult });

    const float targetSpeed = (float)autoPilot.m_nCruiseSpeed * laneSpeedMult;
    const float diff        = targetSpeed - forwardSpeed;

    *outBrake = 0.0f;
    if (targetSpeed > 0.05f || diff > 0.03f) {
        if (diff <= 0.0f) {
            *outThrottle = 0.0f;
            *outBrake    = std::min(0.5f, -diff * 0.083333336f);
        } else {
            *outThrottle = std::min(1.0f, diff * (forwardSpeed > 2.0f ? 0.125f : 0.25f));
            // TODO(re) 0x434900: A BMX going more than 3 units over the target speed also touches the
            //                    (unidentified) float at `veh + 0x818` - See `SteerAICarWithPhysicsHeadingForTarget`
        }
    } else {
        *outBrake    = 1.0f;
        *outThrottle = 0.0f;
    }

    *outSteer     = steerAngle;
    *outHandBrake = false;

    //! Closer to the destination than we're allowed to be - Back out
    if ((autoPilot.m_nCarMission == MISSION_GOTOCOORDINATES || autoPilot.m_nCarMission == MISSION_GOTOCOORDINATES_ACCURATE)
        && autoPilot.m_nTempAction == TEMPACT_NONE
    ) {
        const CVector carPos3D = vehicle->GetPosition();
        const float   distToDest = (carPos3D - autoPilot.m_vecDestinationCoors).Magnitude();
        if (distToDest <= 8.0f) {
            //! Note: The original really does dot the position with the front vector here (Not the direction to the destination)
            const float dot = DotProduct(carPos3D, vehicle->GetForward());
            if (std::abs(dot) / distToDest <= 0.2f) {
                autoPilot.m_nTempAction     = TEMPACT_REVERSE;
                autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 2000;
            }
        }
    }
}

// 0x435830
void CCarCtrl::SteerAICarWithPhysicsFollowPath_Racing(CVehicle* vehicle, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    auto& autoPilot = vehicle->m_autoPilot;

    if (StopCarIfNodesAreInvalid(vehicle)) {
        return;
    }

    const auto Stop = [&] {
        *outBrake     = 1.0f;
        *outThrottle  = 0.0f;
        *outSteer     = 0.0f;
        *outHandBrake = false;
    };

    //! Speed along the front of the car
    const float speedAlong = DotProduct(vehicle->m_vecMoveSpeed, vehicle->GetForward()) * 60.0f;

    //! Normalised 2D heading
    float headingX = vehicle->GetForward().x;
    float headingY = vehicle->GetForward().y;
    if (const float len = std::sqrt(headingX * headingX + headingY * headingY); len == 0.0f) {
        headingX = 1.0f;
    } else {
        headingX /= len;
        headingY /= len;
    }

    const auto RawAddr = [](const CCarPathLinkAddress& addr) {
        return *reinterpret_cast<const uint16*>(&addr);
    };

    if (!ThePaths.m_pPathNodes[autoPilot.m_nCurrentPathNodeInfo.m_wAreaId]
        || !ThePaths.m_pPathNodes[autoPilot.m_nNextPathNodeInfo.m_wAreaId]
    ) {
        Stop();
        return;
    }

    //! Would the current lane take us to the next link?
    const CVector2D carPos      = vehicle->GetPosition2D();
    const CVector2D currLinkPos = ThePaths.GetCarPathLink(autoPilot.m_nCurrentPathNodeInfo).GetNodeCoors();
    const CVector2D nextLinkPos = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo).GetNodeCoors();

    const float dist = std::sqrt(
        (carPos.x - currLinkPos.x) * (carPos.x - currLinkPos.x)
        + (carPos.y - currLinkPos.y) * (carPos.y - currLinkPos.y)
    );

    bool followCurrentLane = false;
    if (dist > 8.0f) {
        const float linkX = nextLinkPos.x - currLinkPos.x;
        const float linkY = nextLinkPos.y - currLinkPos.y;
        const float dot   = linkX * (carPos.x - currLinkPos.x) + linkY * (carPos.y - currLinkPos.y);

        if (dot <= 0.0f || dist > 12.0f) {
            if (dot / (std::sqrt(linkX * linkX + linkY * linkY) * dist) <= 0.7f
                && RawAddr(autoPilot.m_nCurrentPathNodeInfo) != RawAddr(autoPilot.m_nNextPathNodeInfo)
            ) {
                followCurrentLane = true;
            }
        }
    }

    if (!followCurrentLane) {
        //! Not enough path nodes left - Request a new path
        if (autoPilot.m_nPathFindNodesCount < 4) {
            //! 0x8A5F44 - The "no forbidden node" address (Note: The node ID is `0`, not `-1`)
            static const CNodeAddress NoForbiddenNode{ (uint16)-1, 0 };

            ThePaths.DoPathSearch(
                PATH_TYPE_VEH,
                vehicle->GetPosition(),
                autoPilot.m_startingRouteNode,
                autoPilot.m_vecDestinationCoors,
                autoPilot.m_aPathFindNodesInfo.data(),
                (int16&)autoPilot.m_nPathFindNodesCount,
                8,
                nullptr,
                1000000.0f,
                nullptr,
                1000000.0f,
                (autoPilot.m_nCarCtrlFlags >> 6) & 1, //!< bCantGoAgainstTraffic
                NoForbiddenNode,
                vehicle->m_nModelIndex == 0x21B,
                false
            );

            if (autoPilot.m_nPathFindNodesCount > 0) {
                autoPilot.RemoveOnePathNode();
                if (autoPilot.m_nPathFindNodesCount > 0 && autoPilot.m_startingRouteNode == autoPilot.m_aPathFindNodesInfo[0]) {
                    autoPilot.RemoveOnePathNode();
                }
            }

            //! Nothing found - Drive straight towards the destination instead
            if (autoPilot.m_nPathFindNodesCount < 1) {
                autoPilot.m_nCarMission = MISSION_GOTOCOORDINATES_STRAIGHTLINE;
                SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, autoPilot.m_vecDestinationCoors.x, autoPilot.m_vecDestinationCoors.y, outSteer, outThrottle, outBrake, outHandBrake);
                return;
            }
        }

        //! Shift the path bookkeeping and consume the first path node
        autoPilot.m_endingRouteNode   = autoPilot.m_currentAddress;
        autoPilot.m_currentAddress    = autoPilot.m_startingRouteNode;
        autoPilot.m_startingRouteNode = autoPilot.m_aPathFindNodesInfo[0];
        autoPilot.RemoveOnePathNode();

        autoPilot.m_nPreviousPathNodeInfo = autoPilot.m_nCurrentPathNodeInfo;
        autoPilot.m_nCurrentPathNodeInfo  = autoPilot.m_nNextPathNodeInfo;
        autoPilot._smthPrev               = autoPilot._smthCurr;
        autoPilot.m_nCurrentLane          = autoPilot.m_nNextLane;
        autoPilot._smthCurr               = autoPilot._smthNext;

        //! The navi link going from the (new) current address to the (new) starting node
        {
            const CNodeAddress cur  = autoPilot.m_currentAddress;
            const CPathNode*   node = ThePaths.GetPathNode(cur);
            int32              i    = 0;
            while (ThePaths.m_pNodeLinks[cur.m_wAreaId][node->m_wBaseLinkId + i] != autoPilot.m_startingRouteNode) {
                i++;
            }
            autoPilot.m_nNextPathNodeInfo = ThePaths.m_pNaviLinks[cur.m_wAreaId][node->m_wBaseLinkId + i];
        }

        autoPilot._smthNext = IsNodeAddressLess(autoPilot.m_currentAddress, autoPilot.m_startingRouteNode) ? -1 : 1;
    }

    //! The links of the next few path nodes, and the nodes they connect
    CCarPathLinkAddress linkAddrs[8]{}; //!< Default constructed = `0xFFFF` ("Invalid")
    CNodeAddress        nodes[8]{};
    int8                smth[8]{};      //!< `_smthCurr`/`_smthNext` for each of the links above

    linkAddrs[0] = autoPilot.m_nCurrentPathNodeInfo;
    linkAddrs[1] = autoPilot.m_nNextPathNodeInfo;
    smth[0]      = autoPilot._smthCurr;
    smth[1]      = autoPilot._smthNext;
    nodes[0]     = autoPilot.m_currentAddress;
    nodes[1]     = autoPilot.m_startingRouteNode;

    for (int32 i = 0; i < 4 && i < (int16)autoPilot.m_nPathFindNodesCount; i++) {
        if (autoPilot.m_aPathFindNodesInfo[i].m_wAreaId == (uint16)-1) {
            break;
        }

        nodes[i + 2]     = autoPilot.m_aPathFindNodesInfo[i];
        linkAddrs[i + 2] = ThePaths.FindLinkBetweenNodes(nodes[i + 1], nodes[i + 2]);
        smth[i + 2]      = IsNodeAddressLess(nodes[i + 1], nodes[i + 2]) ? -1 : 1;
    }

    //! Look for a bend ahead and, if there is one, aim at it
    float bendAngle = 0.0f, bendSpeedMult = 1.0f, bendHeading = 0.0f, bendDist = 0.0f;
    CVector bendPos{};

    uint16 chosenLink = 0xFFFF;
    for (int32 i = 0; i < 4; i++) {
        if (DealWithBend_Racing(
                vehicle,
                linkAddrs[i], linkAddrs[i + 1], linkAddrs[i + 2], linkAddrs[i + 3],
                smth[i], smth[i + 1], smth[i + 2], smth[i + 3],
                speedAlong,
                &bendAngle, &bendSpeedMult, &bendHeading, &bendDist, &bendPos
            )
        ) {
            chosenLink = RawAddr(linkAddrs[i]);
            break;
        }
    }

    const float carHeading = CGeneral::GetATanOfXY(headingX, headingY);
    float       targetAngle = carHeading;

    //! Interpolate between the current and the next link's position, and aim there
    float pointX, pointY;
    if (chosenLink == 0xFFFF) {
        //! No bend - Adjust the heading to the links ahead (In reverse order)
        for (int32 i = 5; i >= 2; i--) {
            if (RawAddr(linkAddrs[i]) != 0xFFFF) {
                const CVector2D pos = ThePaths.GetCarPathLink(linkAddrs[i]).GetNodeCoors();
                ClitargetOrientationToLink(vehicle, linkAddrs[i], smth[i], &targetAngle, pos.x, pos.y);
            }
        }

        const CVector2D nextPos = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo).GetNodeCoors();
        ClitargetOrientationToLink(vehicle, autoPilot.m_nNextPathNodeInfo, autoPilot._smthNext, &targetAngle, nextPos.x, nextPos.y);

        const float perc = FindPercDependingOnDistToLink(vehicle, autoPilot.m_nCurrentPathNodeInfo);
        pointX = (1.0f - perc) * nextLinkPos.x + perc * currLinkPos.x;
        pointY = (1.0f - perc) * nextLinkPos.y + perc * currLinkPos.y;
    } else {
        if (RawAddr(autoPilot.m_nCurrentPathNodeInfo) != chosenLink) {
            const CVector2D nextPos = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo).GetNodeCoors();
            ClitargetOrientationToLink(vehicle, autoPilot.m_nNextPathNodeInfo, autoPilot._smthNext, &targetAngle, nextPos.x, nextPos.y);
        }

        //! Bias the interpolation towards the current link
        const float perc = (FindPercDependingOnDistToLink(vehicle, autoPilot.m_nCurrentPathNodeInfo) + 2.0f) * 0.33333334f;
        pointX = (1.0f - perc) * nextLinkPos.x + perc * currLinkPos.x;
        pointY = (1.0f - perc) * nextLinkPos.y + perc * currLinkPos.y;
    }

    ClitargetOrientationToLink(vehicle, autoPilot.m_nCurrentPathNodeInfo, autoPilot._smthCurr, &targetAngle, pointX, pointY);

    //! How much we'd have to turn to follow the lane
    float steerAngle = FindAngleToWeaveThroughTraffic(vehicle, nullptr, targetAngle, carHeading, 2.0f) - carHeading;
    steerAngle = CGeneral::LimitRadianAngle(steerAngle);

    //! The higher the speed, the less we're allowed to steer
    const float speed = vehicle->m_vecMoveSpeed.Magnitude();
    float       maxSteerAngle;
    if (speed > 0.7f) {
        maxSteerAngle = 0.2f;
    } else {
        maxSteerAngle = std::min(0.9f - speed, 0.7f);
    }

    bool steeringClamped = false;
    if (steerAngle < -maxSteerAngle) {
        steerAngle     = -maxSteerAngle;
        steeringClamped = true;
    }
    if (steerAngle >= maxSteerAngle) {
        steerAngle      = maxSteerAngle;
        steeringClamped = true;
    }

    //! Traffic ahead (And the bend) limit our speed
    float speedMult = 1.0f;
    if (CTrafficLights::ShouldCarStopForBridge(vehicle)) {
        CCarAI::CarHasReasonToStop(vehicle);
        speedMult = 0.0f;
    }
    speedMult = std::min(speedMult, bendSpeedMult);

    const float targetSpeed = (float)autoPilot.m_nCruiseSpeed * speedMult;
    const float diff        = targetSpeed - speedAlong;

    *outBrake = 0.0f;
    if (targetSpeed > 0.05f || diff > 0.03f) {
        if (diff <= 0.0f) {
            *outThrottle = 0.0f;
            *outBrake    = vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX
                ? std::min(1.0f, -diff * 0.06666667f)
                : std::min(0.5f, -diff * 0.05f);
        } else if ((!steeringClamped) || speedAlong <= 7.0f) {
            *outThrottle = std::min(1.0f, diff * (speedAlong > 2.0f ? 0.125f : 0.25f));
            // TODO(re) 0x435830: A BMX going more than 3 units over the target speed also touches the
            //                    (unidentified) float at `veh + 0x818` - See `SteerAICarWithPhysicsHeadingForTarget`
        } else {
            *outThrottle = 0.0f;
        }
    } else {
        *outBrake    = 1.0f;
        *outThrottle = 0.0f;
    }

    *outSteer     = steerAngle;
    *outHandBrake = false;
}

// 0x432DD0
void CCarCtrl::SteerAICarWithPhysicsFollowPreRecordedPath(CVehicle* vehicle, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    auto& autoPilot = vehicle->m_autoPilot;

    const int32 id = autoPilot.m_vehicleRecordingId;
    const auto  Abort = [&] {
        *outSteer     = 0.0f;
        *outThrottle  = 0.0f;
        *outBrake     = 0.0f;
        *outHandBrake = false;
        autoPilot.m_nCarMission = (eCarMission)0xB; //!< `MISSION_STOP_FOREVER`
    };

    if (id < 0 || !CVehicleRecording::pPlaybackBuffer[id]) {
        Abort();
        return;
    }

    if (CVehicleRecording::bPlaybackPaused[id]) {
        *outSteer     = 0.0f;
        *outThrottle  = 0.0f;
        *outBrake     = 0.5f;
        *outHandBrake = false;
        return;
    }

    //! `PlaybackIndex` is a byte offset into the frame buffer
    const auto* const bufferBase = (const uint8*)CVehicleRecording::pPlaybackBuffer[id];
    const auto frameAt = [&](int32 byteOffset) {
        return (const CVehicleStateEachFrame*)(bufferBase + byteOffset);
    };

    const CVector carPos = vehicle->GetPosition();

    //! Skip frames we've already passed (Or that are too close)
    while (true) {
        const auto* const cur  = frameAt(CVehicleRecording::PlaybackIndex[id]);
        const auto* const next = frameAt(CVehicleRecording::PlaybackIndex[id] + 0x20);

        const float curDist  = (cur->m_vecPosn - carPos).Magnitude();
        const float nextDist = (next->m_vecPosn - carPos).Magnitude();

        if (curDist >= 10.0f && curDist <= nextDist) {
            break;
        }

        const int32 oldIndex = CVehicleRecording::PlaybackIndex[id];
        CVehicleRecording::PlaybackIndex[id] += 0x20;
        if (CVehicleRecording::PlaybackBufferSize[id] - 0x20 <= oldIndex + 0x20) {
            CVehicleRecording::StopPlaybackWithIndex(id);
            autoPilot.m_vehicleRecordingId = -1;
            Abort();
            return;
        }
    }

    //! Steer towards the next frame
    const auto* const curFrame  = frameAt(CVehicleRecording::PlaybackIndex[id]);
    const auto* const nextFrame = frameAt(CVehicleRecording::PlaybackIndex[id] + 0x20);

    const float currentHeading = CGeneral::GetATanOfXY(vehicle->GetForward().x, vehicle->GetForward().y);
    const float desiredHeading = CGeneral::GetATanOfXY(nextFrame->m_vecPosn.x - carPos.x, nextFrame->m_vecPosn.y - carPos.y);

    float steer = FindAngleToWeaveThroughTraffic(vehicle, nullptr, desiredHeading, currentHeading, 2.0f) - currentHeading;
    steer       = CGeneral::LimitRadianAngle(steer);

    steer = std::clamp(steer, -FindMaxSteerAngle(vehicle), FindMaxSteerAngle(vehicle));

    //! Desired speed
    float targetSpeed = std::sqrt(
        (float)curFrame->m_sVelocity.y / 16383.5f * ((float)curFrame->m_sVelocity.y / 16383.5f) +
        (float)curFrame->m_sVelocity.x / 16383.5f * ((float)curFrame->m_sVelocity.x / 16383.5f)
    ) * CVehicleRecording::PlaybackSpeed[id] * 60.0f;

    if (CVehicleRecording::PlaybackIndex[id] < 0x321 && targetSpeed <= 5.0f) {
        targetSpeed = 5.0f;
    }

    const float currentSpeed = vehicle->m_vecMoveSpeed.Magnitude() * 60.0f;
    const float diff         = targetSpeed - currentSpeed;

    *outBrake    = 0.0f;
    *outHandBrake = false;

    if (diff <= 0.0f) {
        *outThrottle = 0.0f;
        *outBrake    = std::min(0.5f, -diff * 0.05f);
    } else if (currentSpeed >= 2.0f) {
        *outThrottle = std::min(1.0f, diff * 0.125f);
    } else {
        *outThrottle = std::min(1.0f, diff * 0.25f);
    }

    *outSteer = steer;
}

// 0x433280
void CCarCtrl::SteerAICarWithPhysicsHeadingForTarget(CVehicle* vehicle, CPhysical* target, float targetX, float targetY, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    auto& autoPilot = vehicle->m_autoPilot;

    *outHandBrake = false;

    const CVector carPos     = vehicle->GetPosition();
    const CVector carFront   = vehicle->GetForward();
    const float   carHeading = CGeneral::GetATanOfXY(carFront.x, carFront.y);

    float desired = CGeneral::GetATanOfXY(targetX - carPos.x, targetY - carPos.y);

    //! Some driving styles weave through traffic
    switch ((int32)autoPilot.m_nCarDrivingStyle) {
    case 2:
    case 5:
    case 6:
        desired = FindAngleToWeaveThroughTraffic(vehicle, target, desired, carHeading, 1.0f);
        break;
    default:
        break;
    }

    float delta = CGeneral::LimitRadianAngle(desired - carHeading);

    //! Too much of a turn while moving fast -> Pull the handbrake
    if (vehicle->m_vecMoveSpeed.Magnitude() > 0.3f && std::abs(delta) > 0.7f) {
        *outHandBrake = true;
    }

    delta = std::clamp(delta, -FindMaxSteerAngle(vehicle), FindMaxSteerAngle(vehicle));

    //! Speed along the turn
    const float speedMult    = FindSpeedMultiplier(
        CGeneral::GetATanOfXY(targetX - carPos.x, targetY - carPos.y) - carHeading,
        0.4f, 1.2f, 0.4f
    );
    const float currentSpeed = vehicle->m_vecMoveSpeed.Magnitude() * 60.0f;
    const float diff         = speedMult * (float)autoPilot.m_nCruiseSpeed - currentSpeed;

    *outBrake = 0.0f;
    if (diff <= 0.0f) {
        *outThrottle = 0.0f;
        *outBrake    = std::min(0.5f, -diff * 0.05f);
        *outSteer    = delta;
        return;
    }

    *outThrottle = currentSpeed >= 25.0f ? 1.0f : std::min(1.0f, diff * 0.1f);

    // TODO(re): The original also has a BMX-only special case (`m_nVehicleSubType == 10`) that checks/sets
    //           the float at `veh + 0x818` (Not identified in the repo).

    *outSteer = delta;
}

// 0x4335E0
void CCarCtrl::SteerAICarWithPhysicsTryingToBlockTarget(CVehicle* vehicle, CEntity* Unusued, float targetX, float targetY, float dirX, float dirY, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    //! Clamp the direction's length
    if (const float len = std::sqrt(dirX * dirX + dirY * dirY); len > 0.13f) {
        const float scale = 0.13f / len;
        dirX *= scale;
        dirY *= scale;
    }

    //! Aim a bit ahead of the target
    targetX += dirX * 60.0f;
    targetY += dirY * 60.0f;

    vehicle->m_autoPilot.m_nCarDrivingStyle = (eCarDrivingStyle)2;

    SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, targetX, targetY, outSteer, outThrottle, outBrake, outHandBrake);

    //! If we've reached the target, start blocking
    const CVector carPos = vehicle->GetPosition();
    if ((targetX - carPos.x) * (targetX - carPos.x) + (targetY - carPos.y) * (targetY - carPos.y) < 25.0f) {
        vehicle->m_autoPilot.m_nCarMission = vehicle->m_autoPilot.m_nCarMission == MISSION_BLOCKCAR_CLOSE
            ? MISSION_BLOCKPLAYER_HANDBRAKESTOP
            : MISSION_BLOCKCAR_HANDBRAKESTOP;
    }
}

// 0x428990
void CCarCtrl::SteerAICarWithPhysicsTryingToBlockTarget_Stop(CVehicle* vehicle, float x, float y, float arg4, float arg5, float* arg6, float* arg7, float* arg8, bool* arg9) {
    *arg6 = 0.0f;
    *arg7 = 0.0f;
    *arg8 = 1.0f;
    *arg9 = true;

    const CVector pos = vehicle->GetPosition();
    const float   dist2 = (pos.x - x) * (pos.x - x) + (pos.y - y) * (pos.y - y);

    if (dist2 > 100.0f) {
        //! Too far away, go back to blocking
        vehicle->m_autoPilot.m_nCarMission = vehicle->m_autoPilot.m_nCarMission != MISSION_BLOCKCAR_HANDBRAKESTOP
            ? MISSION_BLOCKPLAYER_CLOSE
            : MISSION_BLOCKCAR_CLOSE;
        return;
    }

    if (vehicle->m_autoPilot.m_nCarMission == MISSION_BLOCKCAR_HANDBRAKESTOP) {
        if (vehicle->m_pDriver && vehicle->m_pDriver->GetTaskManager().GetActiveTask()) {
            if (vehicle->m_pDriver->GetTaskManager().GetActiveTask()->GetTaskType() == TASK_COMPLEX_KILL_CRIMINAL) {
                return;
            }
        }
        if (vehicle->m_vecMoveSpeed.SquaredMagnitude() >= 0.0001f) {
            return;
        }
        if (arg4 * arg4 + arg5 * arg5 >= 0.0004f) {
            return;
        }
        if (!vehicle->vehicleFlags.bIsLawEnforcer) {
            return;
        }
    } else {
        CVehicle* playerVeh = FindPlayerVehicle(-1, false);
        if (playerVeh && playerVeh->m_vecMoveSpeed.Magnitude() < 0.05f) {
            vehicle->m_nCopsInCarTimer += (int16)(CTimer::GetTimeStep() * 16.6667f);
        } else {
            vehicle->m_nCopsInCarTimer = 0;
        }

        playerVeh = FindPlayerVehicle(-1, false);
        if (playerVeh) {
            if (!playerVeh->IsUpsideDown()) {
                playerVeh = FindPlayerVehicle(-1, false);
                if (playerVeh->m_vecMoveSpeed.Magnitude() >= 0.05f) {
                    return; //!< The player is still driving
                }
                if (vehicle->m_nCopsInCarTimer <= (int16)TIME_COPS_WAIT_TO_EXIT_AFTER_STOPPING) {
                    return;
                }
            }
        }
        if (!vehicle->vehicleFlags.bIsLawEnforcer) {
            return;
        }
        if (dist2 >= 100.0f) {
            return;
        }
    }

    CCarAI::TellOccupantsToLeaveCar(vehicle);
    vehicle->m_autoPilot.m_nCruiseSpeed = 0;
    vehicle->m_autoPilot.m_nCarMission  = MISSION_NONE;
}

// 0x436A90
void CCarCtrl::SteerAICarWithPhysics_OnlyMission(CVehicle* vehicle, float* outSteer, float* outThrottle, float* outBrake, bool* outHandBrake) {
    auto& autoPilot = vehicle->m_autoPilot;

    //! Every "stand still" mission ends up here
    const auto Brake = [&] {
        *outSteer     = 0.0f;
        *outThrottle  = 0.0f;
        *outHandBrake = true;
        *outBrake     = 0.5f;
    };

    switch (autoPilot.m_nCarMission) {
    case MISSION_NONE:
        Brake();
        return;

    //! The far away missions all follow the road system
    case MISSION_CRUISE:
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_GOTOCOORDINATES:
    case MISSION_GOTOCOORDINATES_ACCURATE:
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_BLOCKCAR_FARAWAY:
    case MISSION_APPROACHPLAYER_FARAWAY:
    case MISSION_FOLLOWCAR_FARAWAY:
    case MISSION_KILLPED_FARAWAY:
    case MISSION_DO_DRIVEBY_FARAWAY:
    case MISSION_ESCORT_LEFT_FARAWAY:
    case MISSION_ESCORT_RIGHT_FARAWAY:
    case MISSION_ESCORT_REAR_FARAWAY:
    case MISSION_ESCORT_FRONT_FARAWAY:
        SteerAICarWithPhysicsFollowPath(vehicle, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_RAMPLAYER_CLOSE: {
        CVector target = FindPlayerCoors(-1);

        if (const CVehicle* const playerVeh = FindPlayerVehicle(-1, false)) {
            const float dot = DotProduct(vehicle->GetForward(), playerVeh->GetForward());
            const uint8 rnd = vehicle->m_nRandomSeedUpperByte;

            if (!(rnd & 1) || dot <= 0.5f) {
                //! Random sideways offset, so that not every car rams the same spot
                const float side = (float)((int32)rnd - 0x80);
                target.x += side * playerVeh->GetRight().x * 0.00625f;
                target.y += side * playerVeh->GetRight().y * 0.00625f;
            } else {
                //! Facing the same way as the player - Aim alongside their car
                const float len = (vehicle->GetColModel()->m_boundBox.m_vecMax.x + playerVeh->GetColModel()->m_boundBox.m_vecMax.x) - 0.2f;
                const float side = (rnd & 2) ? 1.0f : -1.0f;
                target.x += side * len * playerVeh->GetRight().x;
                target.y += side * len * playerVeh->GetRight().y;

                //! Close enough - Turn into them
                const float dist = (vehicle->GetPosition() - target).Magnitude();
                if (dist < (playerVeh->m_vecMoveSpeed - vehicle->m_vecMoveSpeed).Magnitude() * 12.0f + 2.0f
                    && autoPilot.m_nTempAction == TEMPACT_NONE
                ) {
                    autoPilot.m_nTempAction     = (eAutoPilotTempAction)(TEMPACT_TURNRIGHT - ((rnd & 2) != 0));
                    autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + 250;
                }
            }

            //! Coming towards us - Lead the target using their speed
            if (dot < 0.0f) {
                target += FindPlayerSpeed(-1) * (dot * -0.02f);
            }

            //! Cops might shout at the player while ramming them
            if (vehicle->vehicleFlags.bIsLawEnforcer && std::sqrt(sq(playerVeh->m_vecMoveSpeed.x) + sq(playerVeh->m_vecMoveSpeed.y)) > 0.4f) {
                if (CPed* const ped = FindPlayerVehicle(-1, false)->PickRandomPassenger()) {
                    ped->Say(CTX_GLOBAL_CAR_POLICE_PURSUIT, 0, 1.0f, false, false, false);
                }
            }
        }

        SteerAICarWithPhysicsHeadingForTarget(vehicle, FindPlayerVehicle(-1, false), target.x, target.y, outSteer, outThrottle, outBrake, outHandBrake);
        return;
    }

    case MISSION_BLOCKPLAYER_CLOSE:
        SteerAICarWithPhysicsTryingToBlockTarget(
            vehicle,
            FindPlayerEntity(-1),
            FindPlayerCoors(-1).x,
            FindPlayerCoors(-1).y,
            FindPlayerSpeed(-1).x,
            FindPlayerSpeed(-1).y,
            outSteer, outThrottle, outBrake, outHandBrake
        );
        return;

    case MISSION_BLOCKPLAYER_HANDBRAKESTOP:
        SteerAICarWithPhysicsTryingToBlockTarget_Stop(
            vehicle,
            FindPlayerCoors(-1).x,
            FindPlayerCoors(-1).y,
            FindPlayerSpeed(-1).x,
            FindPlayerSpeed(-1).y,
            outSteer, outThrottle, outBrake, outHandBrake
        );
        return;

    case MISSION_WAITFORDELETION:
    case MISSION_PROTECTION_REAR:
    case MISSION_PROTECTION_FRONT:
        return; //!< Keep whatever the caller had

    case MISSION_GOTOCOORDINATES_STRAIGHTLINE:
    case MISSION_GOTOCOORDINATES_STRAIGHTLINE_ACCURATE:
        SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, autoPilot.m_vecDestinationCoors.x, autoPilot.m_vecDestinationCoors.y, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_EMERGENCYVEHICLE_STOP:
    case MISSION_STOP_FOREVER:
        Brake();
        return;

    case MISSION_GOTOCOORDINATES_ASTHECROWSWIMS:
        SteerAIBoatWithPhysicsHeadingForTarget(vehicle, autoPilot.m_vecDestinationCoors.x, autoPilot.m_vecDestinationCoors.y, outSteer, outThrottle, outBrake);
        *outHandBrake = false;
        return;

    case MISSION_RAMCAR_CLOSE:
    case MISSION_KILLPED_CLOSE:
        SteerAICarWithPhysicsHeadingForTarget(vehicle, autoPilot.m_TargetEntity, autoPilot.m_TargetEntity->GetPosition().x, autoPilot.m_TargetEntity->GetPosition().y, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_BLOCKCAR_CLOSE:
        SteerAICarWithPhysicsTryingToBlockTarget(
            vehicle,
            autoPilot.m_TargetEntity,
            autoPilot.m_TargetEntity->GetPosition().x,
            autoPilot.m_TargetEntity->GetPosition().y,
            autoPilot.m_TargetEntity->m_vecMoveSpeed.x,
            autoPilot.m_TargetEntity->m_vecMoveSpeed.y,
            outSteer, outThrottle, outBrake, outHandBrake
        );
        return;

    case MISSION_BLOCKCAR_HANDBRAKESTOP:
        SteerAICarWithPhysicsTryingToBlockTarget_Stop(
            vehicle,
            autoPilot.m_TargetEntity->GetPosition().x,
            autoPilot.m_TargetEntity->GetPosition().y,
            autoPilot.m_TargetEntity->m_vecMoveSpeed.x,
            autoPilot.m_TargetEntity->m_vecMoveSpeed.y,
            outSteer, outThrottle, outBrake, outHandBrake
        );
        return;

    case MISSION_HELI_FLYTOCOORS:
        SteerAIHeliTowardsTargetCoors(vehicle->AsAutomobile());
        return;

    case MISSION_BOAT_ATTACKPLAYER:
        SteerAIBoatWithPhysicsAttackingPlayer(vehicle, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_PLANE_FLYTOCOORS:
        SteerAIPlaneTowardsTargetCoors(vehicle->AsAutomobile());
        return;

    case MISSION_HELI_ATTACK_PLAYER:
    case MISSION_HELI_ATTACK_PLAYER_FLY_AWAY:
        GetAIHeliToAttackPlayer(vehicle->AsAutomobile());
        return;

    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_1:
        SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, autoPilot.m_vecDestinationCoors.x, autoPilot.m_vecDestinationCoors.y, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_2:
        SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, FindPlayerCoors(-1).x, FindPlayerCoors(-1).y, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_BLOCKPLAYER_FORWARDANDBACK:
        SteerAICarBlockingPlayerForwardAndBack(vehicle, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    //! Escort missions: Drive next to/in front of/behind the target
    case MISSION_ESCORT_LEFT:
    case MISSION_ESCORT_RIGHT:
    case MISSION_ESCORT_REAR:
    case MISSION_ESCORT_FRONT: {
        CVehicle* const target = autoPilot.m_TargetEntity;

        const float targetWidth = target->GetColModel()->m_boundBox.m_vecMax.x;
        const float ownWidth    = vehicle->GetColModel()->m_boundBox.m_vecMax.x;
        const float targetLen   = target->GetColModel()->m_boundBox.m_vecMax.y;
        const float ownLen      = vehicle->GetColModel()->m_boundBox.m_vecMax.y;

        float offsetX = 0.0f;
        float offsetY = 0.0f;
        switch (autoPilot.m_nCarMission) {
        case MISSION_ESCORT_LEFT:
            offsetX = -(targetWidth + ownWidth + 2.0f);
            break;
        case MISSION_ESCORT_RIGHT:
            offsetX = targetWidth + ownWidth + 2.0f;
            break;
        case MISSION_ESCORT_REAR:
            offsetY = -(targetLen + ownLen + 7.0f);
            break;
        case MISSION_ESCORT_FRONT:
            offsetY = targetLen + ownLen + 7.0f;
            break;
        default:
            NOTSA_UNREACHABLE();
        }

        SteerAICarTowardsPointInEscort(vehicle, target, offsetX, offsetY, outSteer, outThrottle, outBrake, outHandBrake);
        return;
    }

    case MISSION_GOTOCOORDINATES_RACING:
        SteerAICarWithPhysicsFollowPath_Racing(vehicle, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_FOLLOW_RECORDED_PATH:
        SteerAICarWithPhysicsFollowPreRecordedPath(vehicle, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_PLANE_ATTACK_PLAYER:
    case MISSION_PLANE_ATTACK_PLAYER_POLICE:
        GetAIPlaneToAttackPlayer(vehicle->AsAutomobile());
        return;

    case MISSION_PLANE_FLYINDIRECTION:
        FlyAIPlaneInCertainDirection(vehicle->AsPlane());
        return;

    case MISSION_PLANE_FOLLOW_ENTITY:
        SteerAIPlaneToFollowEntity(vehicle->AsAutomobile());
        return;

    case MISSION_HELI_FLYINDIRECTION:
        GetAIHeliToFlyInDirection(vehicle->AsAutomobile());
        return;

    case MISSION_HELI_FOLLOW_ENTITY:
    case MISSION_HELI_NEWS_BEHAVIOUR:
        SteerAIHeliToFollowEntity(vehicle->AsAutomobile());
        return;

    case MISSION_HELI_POLICE_BEHAVIOUR:
        SteerAIHeliAsPoliceHeli(vehicle->AsAutomobile());
        return;

    case MISSION_HELI_FLY_AWAY_FROM_PLAYER:
        SteerAIHeliFlyingAwayFromPlayer(vehicle->AsAutomobile());
        return;

    case MISSION_APPROACHPLAYER_CLOSE: {
        //! Too close - Just brake
        if ((FindPlayerCoors(-1) - vehicle->GetPosition()).Magnitude() <= 10.0f) {
            *outSteer     = 0.0f;
            *outThrottle  = 0.0f;
            *outBrake     = 1.0f;
            *outHandBrake = false;
            return;
        }

        CVector target = FindPlayerCoors(-1);
        if (CVehicle* const playerVeh = FindPlayerVehicle(-1, false)) {
            //! Note: The components are intentionally taken with mixed signs - The result is perpendicular
            //!       to the direction to the player, that is, a point to the side of the player's car.
            const CVector carPos   = vehicle->GetPosition();
            const CVector toTarget = FindPlayerCoors(-1) - carPos;
            if ((playerVeh->GetForward().y + playerVeh->GetForward().x) * toTarget.x > 0.0f) {
                const CMatrix& playerMat = playerVeh->GetMatrix();

                const CVector pointRight = playerMat.TransformPoint(CVector{ 8.0f, 0.0f, 0.0f });
                const CVector pointLeft  = playerMat.TransformPoint(CVector{-4.0f, 0.0f, 0.0f });

                const float distRight = (carPos - pointRight).Magnitude();
                const float distLeft  = (carPos - pointLeft).Magnitude();

                target = distLeft > distRight ? pointRight : pointLeft;
            }
        }

        SteerAICarWithPhysicsHeadingForTarget(vehicle, FindPlayerVehicle(-1, false), target.x, target.y, outSteer, outThrottle, outBrake, outHandBrake);
        return;
    }

    case MISSION_PARK_PERPENDICULAR:
    case MISSION_PARK_PERPENDICULAR_2:
        SteerAICarParkPerpendicular(vehicle, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_PARK_PARALLEL:
    case MISSION_PARK_PARALLEL_2:
        SteerAICarParkParallel(vehicle, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    case MISSION_HELI_LAND:
    case MISSION_HELI_LAND_TOUCHING_DOWN:
        SteerAIHeliToLand(vehicle->AsAutomobile());
        return;

    case MISSION_HELI_KEEP_ENTITY_IN_VIEW:
        SteerAIHeliToKeepEntityInView(vehicle->AsAutomobile());
        return;

    case MISSION_FOLLOWCAR_CLOSE: {
        CVehicle* const target = autoPilot.m_TargetEntity;
        const CVector   targetPos = target->GetPosition();

        SteerAICarWithPhysicsHeadingForTarget(vehicle, target, targetPos.x, targetPos.y, outSteer, outThrottle, outBrake, outHandBrake);

        //! Keep the follow distance
        const float dist = (vehicle->GetPosition() - targetPos).Magnitude();
        if (dist <= (float)autoPilot.m_ucCarFollowDist + 10.0f) {
            const float err = dist - (float)autoPilot.m_ucCarFollowDist;
            float diff = err >= 0.0f ? err * 2.0f : err * 5.0f;
            diff += (std::sqrt(sq(target->m_vecMoveSpeed.x) + sq(target->m_vecMoveSpeed.y))
                   - std::sqrt(sq(vehicle->m_vecMoveSpeed.x) + sq(vehicle->m_vecMoveSpeed.y))) * 60.0f;

            if (diff < 0.0f) {
                *outThrottle = 0.0f;
                *outBrake    = std::min(1.0f, -diff * 0.1f);
            } else {
                *outThrottle = std::min(1.0f, diff * 0.05f);
                *outBrake    = 0.0f;
            }
        }
        return;
    }

    case MISSION_PLANE_CRASH_AND_BURN:
        SteerAIPlaneToCrashAndBurn(vehicle->AsAutomobile());
        return;

    case MISSION_HELI_CRASH_AND_BURN:
        SteerAIHeliToCrashAndBurn(vehicle->AsAutomobile());
        return;

    case MISSION_DO_DRIVEBY_CLOSE: {
        CVehicle* const target = autoPilot.m_TargetEntity;
        if (!target) {
            return;
        }

        const CVector targetPos = target->GetPosition();
        const CVector carPos    = vehicle->GetPosition();

        //! Perpendicular to the direction to the target (The mixed signs are from the original)
        CVector side{ carPos.x - targetPos.x, targetPos.y - carPos.y, 0.0f };
        side.Normalise();
        side *= 10.0f;
        if (vehicle->m_nRandomSeedUpperByte & 1) {
            side = -side;
        }

        SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, targetPos.x + side.x, targetPos.y + side.y, outSteer, outThrottle, outBrake, outHandBrake);
        return;
    }

    case MISSION_PLANE_DOG_FIGHT_ENTITY:
        GetAIPlaneToDoDogFight(vehicle->AsAutomobile());
        return;

    case MISSION_PLANE_DOG_FIGHT_PLAYER:
        GetAIPlaneToDoDogFightAgainstPlayer(vehicle->AsAutomobile());
        return;

    case MISSION_BOAT_CIRCLEPLAYER:
        SteerAIBoatWithPhysicsCirclingPlayer(vehicle, outSteer, outThrottle, outBrake, outHandBrake);
        return;

    default:
        return;
    }
}

// 0x42AAD0
void CCarCtrl::SteerAIHeliAsPoliceHeli(CAutomobile* automobile) {
    CHeli* const heli = automobile->AsHeli();

    const CVehicle* const target = heli->m_autoPilot.m_TargetEntity;
    const CVector  targetPos = target->GetPosition();
    const CVector  pos       = heli->GetPosition();

    const float dx     = targetPos.x - pos.x;
    const float dy     = targetPos.y - pos.y;
    const float angle  = CGeneral::GetATanOfXY(dx, dy);
    const float dist2D = std::sqrt(dx * dx + dy * dy);

    //! Hover at the target's height, but never below 6m (25m when far away)
    const float altitude = std::max(targetPos.z, 6.0f);
    heli->m_fMaxAltitude = altitude;
    if (dist2D > 50.0f) {
        heli->m_fMaxAltitude = std::max(altitude, 25.0f);
    }

    heli->m_autoPilot.m_vecDestinationCoors = targetPos;

    FlyAIHeliInCertainDirection(heli, angle, dist2D, true);

    //! Bail out (fly away) when badly damaged
    if (heli->m_fHealth < 230.0f) {
        heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
    }
}

// 0x42ACB0
void CCarCtrl::SteerAIHeliFlyingAwayFromPlayer(CAutomobile* automobile) {
    CHeli* const  heli        = automobile->AsHeli();
    const CVector playerCoors = FindPlayerCoors(-1);
    const CVector pos         = heli->GetPosition();

    const float angle = CGeneral::GetATanOfXY(playerCoors.x - pos.x, playerCoors.y - pos.y) + PI;

    FlyAIHeliInCertainDirection(heli, angle, 1000.0f, false);
}

// 0x4238E0
void CCarCtrl::SteerAIHeliToCrashAndBurn(CAutomobile* automobile) {
    CHeli* const heli = automobile->AsHeli();
    const bool   sign = heli->m_nRandomSeed & 1;

    heli->m_fSteeringUpDown          = -0.3f;
    heli->m_fLeftRightSkid           = sign ? heli->field_A14 : -heli->field_A14;
    heli->m_fAccelerationBreakStatus = -0.5f;
    heli->m_fSteeringLeftRight       = sign ? 1.0f : -1.0f;
}

// 0x42A750
void CCarCtrl::SteerAIHeliToFollowEntity(CAutomobile* automobile) {
    CHeli* const heli = automobile->AsHeli();
    //! NOTE: The target may be a ped too, hence the `CEntity` cast
    CEntity* const target = reinterpret_cast<CEntity*>(heli->m_autoPilot.m_TargetEntity);

    heli->m_autoPilot.m_vecDestinationCoors = target->GetPosition();

    //! Aim ahead of the target by `field_4A` meters along its forward axis
    if (heli->m_autoPilot.field_4A != 0) {
        const float   k   = (float)heli->m_autoPilot.field_4A;
        const CVector fwd = target->GetForward();
        heli->m_autoPilot.m_vecDestinationCoors.x += fwd.x * k;
        heli->m_autoPilot.m_vecDestinationCoors.y += fwd.y * k;
    }

    const CVector pos     = heli->GetPosition();
    const CVector dest    = heli->m_autoPilot.m_vecDestinationCoors;
    const float   heading = CGeneral::GetATanOfXY(dest.x - pos.x, dest.y - pos.y);
    const float   dist    = std::sqrt((dest.x - pos.x) * (dest.x - pos.x) + (dest.y - pos.y) * (dest.y - pos.y));

    //! Altitude to keep: The target's height, but never below 6m (and 25m when far away)
    const float targetZ = target->GetPosition().z;
    heli->m_fMaxAltitude = std::max(targetZ, 6.0f);
    if (dist >= 50.0f) {
        heli->m_fMaxAltitude = std::max(heli->m_fMaxAltitude, 25.0f);
    }

    if (heli->m_fForcedOrientation < 0.0f) {
        FlyAIHeliInCertainDirection(heli, heading, dist, true);
    } else {
        FlyAIHeliToTarget_FixedOrientation(heli, heli->m_fForcedOrientation, target->GetPosition());
    }

    //! "Reach the target" special handling
    if (heli->m_autoPilot.m_nCarCtrlFlags & 0x20) {
        const CVector heliPos   = heli->GetPosition();
        const CVector targetPos = target->GetPosition();
        const float   dx        = heliPos.x - targetPos.x;
        const float   dy        = heliPos.y - targetPos.y;
        if (std::sqrt(dx * dx + dy * dy) < 25.0f) {
            switch (target->GetType()) {
            case ENTITY_TYPE_VEHICLE: {
                if (target->GetStatus() == STATUS_SIMPLE || target->GetStatus() == STATUS_PHYSICS) {
                    auto* const targetVeh = target->AsVehicle();
                    targetVeh->m_autoPilot.SetCarMission(MISSION_CRUISE);
                    targetVeh->m_autoPilot.SetCruiseSpeed(100);
                    targetVeh->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_AVOID_CARS;
                    targetVeh->SetStatus(STATUS_PHYSICS);
                }
                break;
            }
            case ENTITY_TYPE_PED: {
                if (CTask* const task = target->AsPed()->GetTaskManager().GetActiveTask()) {
                    if (task->GetTaskType() == TASK_COMPLEX_WANDER) {
                        static_cast<CTaskComplexWander*>(task)->m_nMoveState = PEDMOVE_SPRINT;
                    }
                }
                break;
            }
            default:
                break;
            }
            heli->m_autoPilot.m_nCarCtrlFlags &= ~0x20;
        }
    }

    //! Give up following after 50 seconds
    if (heli->m_autoPilot.m_nCarCtrlFlags & 0x80) {
        if (CTimer::GetTimeInMS() > heli->m_nCreationTime + 50000) {
            heli->m_autoPilot.m_nCarCtrlFlags &= ~0x80;
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
        }
    }

    //! A news heli that is nearly destroyed breaks off
    if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_NEWS_BEHAVIOUR && heli->m_fHealth < 300.0f) {
        heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
    }
}

// 0x42AEB0
void CCarCtrl::SteerAIHeliToKeepEntityInView(CAutomobile* automobile) {
    CHeli* const heli = automobile->AsHeli();

    const CVehicle* const target = heli->m_autoPilot.m_TargetEntity;
    const CVector  targetPos = target->GetPosition();
    const CVector  pos       = heli->GetPosition();

    const float dx            = targetPos.x - pos.x;
    const float dy            = targetPos.y - pos.y;
    const float angleToTarget = CGeneral::GetATanOfXY(dx, dy);
    const float dist2D        = std::sqrt(dx * dx + dy * dy);

    //! Too far away - Fall back to plain following
    if (dist2D > 2.0f * (float)heli->m_autoPilot.m_ucHeliTargetDist) {
        SteerAIHeliToFollowEntity(automobile);
        return;
    }

    //! Keep the target at our 3 o'clock (90 degrees to the right of our heading)
    const CVector fwd = heli->GetForward();
    float         rel = (angleToTarget + PI / 2.0f) - CGeneral::GetATanOfXY(fwd.x, fwd.y);
    while (rel > PI)  rel -= TWO_PI;
    while (rel < -PI) rel += TWO_PI;
    heli->m_fLeftRightSkid = std::clamp(-rel, -1.0f, 1.0f);

    heli->m_fMaxAltitude = targetPos.z + 15.0f;
    heli->field_9AC      = heli->m_fMaxAltitude;
    heli->m_autoPilot.m_vecDestinationCoors = targetPos;

    //! Vertical control
    {
        const float dz   = heli->m_fMaxAltitude - (pos.z + heli->m_vecMoveSpeed.z * 100.0f);
        float       lift = dz * (dz <= 0.0f ? 0.2f : 0.1f) + 0.3f;
        lift += ((float)(CGeneral::GetRandomNumber() & 0xF) - 7.0f) * 0.002f;
        heli->m_fAccelerationBreakStatus = std::clamp(lift, 0.0f, 1.0f);
    }

    //! Lateral control
    const float targetDist = (float)heli->m_autoPilot.m_ucHeliTargetDist;
    if (targetDist * 0.5f > dist2D) {
        heli->m_fSteeringLeftRight = 0.5f;
    } else if (dist2D <= targetDist) {
        heli->m_fSteeringLeftRight = DotProduct(heli->m_vecMoveSpeed, heli->GetRight());
    } else {
        heli->m_fSteeringLeftRight = -0.5f;
    }

    //! Longitudinal control
    heli->m_fSteeringUpDown = 0.0f;
    if (dist2D < targetDist * 1.5f) {
        heli->m_fSteeringUpDown = DotProduct(heli->m_vecMoveSpeed, heli->GetForward());
    }
}

// 0x42AD30
void CCarCtrl::SteerAIHeliToLand(CAutomobile* automobile) {
    CHeli* const  heli = automobile->AsHeli();
    const CVector pos  = heli->GetPosition();

    const float dx = heli->m_autoPilot.m_vecDestinationCoors.x - pos.x;
    const float dy = heli->m_autoPilot.m_vecDestinationCoors.y - pos.y;

    const float angle  = CGeneral::GetATanOfXY(dx, dy);
    const float dist2D = std::sqrt(dx * dx + dy * dy);

    FlyAIHeliInCertainDirection(heli, angle, dist2D, true);

    //! Touch down: Kill all lift once we're above the spot and slow enough
    if (dist2D < 10.0f && heli->m_vecMoveSpeed.Magnitude2D() < 0.05f) {
        heli->m_fMinAltitude = 0.0f;
        heli->m_fMaxAltitude = 0.0f;
        if (   heli->m_fWheelsSuspensionCompression[0] < 1.0f
            || heli->m_fWheelsSuspensionCompression[1] < 1.0f
            || heli->m_fWheelsSuspensionCompression[2] < 1.0f
            || heli->m_fWheelsSuspensionCompression[3] < 1.0f
        ) {
            heli->m_fAccelerationBreakStatus = 0.0f;
            heli->m_fLeftRightSkid           = 0.0f;
            heli->m_fSteeringUpDown          = 0.0f;
            heli->m_fSteeringLeftRight       = 0.0f;
        }
    }
}

// 0x42A630
void CCarCtrl::SteerAIHeliTowardsTargetCoors(CAutomobile* automobile) {
    CHeli* const  heli = automobile->AsHeli();
    const CVector pos  = heli->GetPosition();
    const CVector dest = heli->m_autoPilot.m_vecDestinationCoors;

    if (heli->m_fForcedOrientation < 0.0f) {
        const float heading = CGeneral::GetATanOfXY(dest.x - pos.x, dest.y - pos.y);
        const float dist    = std::sqrt((dest.x - pos.x) * (dest.x - pos.x) + (dest.y - pos.y) * (dest.y - pos.y));
        FlyAIHeliInCertainDirection(heli, heading, dist, true);
    } else {
        FlyAIHeliToTarget_FixedOrientation(heli, heli->m_fForcedOrientation, dest);
    }
}

// 0x423880
void CCarCtrl::SteerAIPlaneToCrashAndBurn(CAutomobile* automobile) {
    CPlane* const plane = automobile->AsPlane();
    const bool    sign  = plane->m_nRandomSeed & 1;

    plane->m_fSteeringUpDown          = -0.3f;
    plane->m_fLeftRightSkid           = sign ? 1.0f : -1.0f;
    plane->m_fAccelerationBreakStatus = 0.0f;
    plane->m_fSteeringLeftRight       = sign ? 1.0f : -1.0f;
}

// 0x4237F0
void CCarCtrl::SteerAIPlaneToFollowEntity(CAutomobile* automobile) {
    CPlane* const plane = automobile->AsPlane();

    CEntity* const target = plane->m_autoPilot.m_TargetEntity;

    const CVector targetPos = target->GetPosition();
    const CVector pos       = plane->GetPosition();

    plane->m_planeHeading = CGeneral::GetATanOfXY(targetPos.x - pos.x, targetPos.y - pos.y);
    plane->m_maxAltitude  = targetPos.z;
    FlyAIPlaneInCertainDirection(plane);
}

// 0x423790
void CCarCtrl::SteerAIPlaneTowardsTargetCoors(CAutomobile* automobile) {
    CPlane* const plane = automobile->AsPlane();
    const CVector pos   = plane->GetPosition();
    const CVector targetCoors = plane->m_autoPilot.m_vecDestinationCoors;

    plane->m_planeHeading = CGeneral::GetATanOfXY(targetCoors.x - pos.x, targetCoors.y - pos.y);
    FlyAIPlaneInCertainDirection(plane);
}

// 0x422590
bool CCarCtrl::StopCarIfNodesAreInvalid(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    if (autoPilot.m_nCurrentPathNodeInfo.IsValid()
        && ThePaths.m_pPathNodes[autoPilot.m_nCurrentPathNodeInfo.m_wAreaId]
        && autoPilot.m_nNextPathNodeInfo.IsValid()
        && ThePaths.m_pPathNodes[autoPilot.m_nNextPathNodeInfo.m_wAreaId]
        && autoPilot.m_currentAddress.IsAreaValid()
        && ThePaths.m_pPathNodes[autoPilot.m_currentAddress.m_wAreaId]
        && autoPilot.m_startingRouteNode.IsAreaValid()
        && ThePaths.m_pPathNodes[autoPilot.m_startingRouteNode.m_wAreaId]
    ) {
        return false;
    }

    autoPilot.movementFlags.bIsStopped = true;
    return true;
}

// 0x4222A0
void CCarCtrl::SwitchBetweenPhysicsAndGhost(CVehicle* vehicle) {
    if (!(vehicle->m_nPhysicalFlags & PHYSICAL_b15)) {
        return;
    }
    if (vehicle->m_nCreatedBy != MISSION_VEHICLE) {
        return;
    }
    switch (vehicle->m_nVehicleSubType) {
    case VEHICLE_TYPE_HELI:
    case VEHICLE_TYPE_PLANE:
    case VEHICLE_TYPE_BOAT:
        return;
    }

    switch (vehicle->GetStatus()) {
    case STATUS_PHYSICS:
        if (!CColStore::HasCollisionLoaded(vehicle->GetPosition(), AREA_CODE_NORMAL_WORLD)) {
            vehicle->SetStatus(STATUS_GHOST);
            if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
                for (auto wheel = 0; wheel < (int32)eCarWheel::MAX_CARWHEELS; wheel++) {
                    vehicle->AsAutomobile()->GetDamageManager().SetWheelStatus((eCarWheel)wheel, WHEEL_STATUS_OK);
                }
            }
        }
        break;
    case STATUS_GHOST:
        if (CColStore::HasCollisionLoaded(vehicle->GetPosition(), AREA_CODE_NORMAL_WORLD)) {
            vehicle->SetStatus(STATUS_PHYSICS);
            if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
                vehicle->AsAutomobile()->PlaceOnRoadProperly();
            } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
                vehicle->AsBike()->PlaceOnRoadProperly();
            }
        }
        break;
    default:
        break;
    }
}

// 0x423FC0
void CCarCtrl::SwitchVehicleToRealPhysics(CVehicle* vehicle) {
    vehicle->SetStatus(STATUS_GHOST);
    vehicle->m_autoPilot.m_nTempAction                  = TEMPACT_NONE;
    vehicle->m_autoPilot.m_nTimeToStartMission          = CTimer::GetTimeInMS() + 2000;
    vehicle->m_autoPilot.m_nTimeSwitchedToRealPhysics   = CTimer::GetTimeInMS();
    vehicle->m_nFakePhysics                             = 0;
}

// 0x425B30
float CCarCtrl::TestCollisionBetween2MovingRects(CVehicle* vehicle1, CVehicle* vehicle2, float arg3, float arg4, CVector* axis, CVector* orient) {
    const CVector p1 = vehicle1->GetPosition();
    const CVector p2 = vehicle2->GetPosition();

    const CColModel* col2 = CModelInfo::GetModelInfo(vehicle2->m_nModelIndex)->GetColModel();
    const float v2HalfLen = col2->m_boundBox.m_vecMax.x; //!< Along `axis`
    const float v2HalfWid = col2->m_boundBox.m_vecMax.y; //!< Across `axis` (positive side)
    const float v2Front   = -col2->m_boundBox.m_vecMin.y; //!< Across `axis` (negative side)

    const CColModel* col1 = CModelInfo::GetModelInfo(vehicle1->m_nModelIndex)->GetColModel();
    const float v1HalfLen = col1->m_boundBox.m_vecMax.x;
    const float v1HalfWid = col1->m_boundBox.m_vecMax.y;

    const float dx = p1.x - p2.x;
    const float dy = p1.y - p2.y;

    const float relPerpSpeed  = arg3 * axis->y - arg4 * axis->x; //!< Cross product
    const float relAlongSpeed = arg3 * axis->x + arg4 * axis->y; //!< Dot product

    float tMin = 1.0f;

    for (int32 i = 0; i < 2; i++) {
        //! The two corners of vehicle 1's box (front/back)
        float lx, ly;
        if (i == 0) {
            lx = v1HalfWid * orient->x + v1HalfLen * orient->y + dx;
            ly = v1HalfWid * orient->y + dy - v1HalfLen * orient->x;
        } else {
            lx = v1HalfWid * orient->x + dx - v1HalfLen * orient->y;
            ly = v1HalfWid * orient->y + v1HalfLen * orient->x + dy;
        }

        //! Axis 1 (perpendicular to `axis`), symmetric extents
        float entry1, exit1;
        {
            const float sep = lx * axis->y - ly * axis->x;
            if (sep > v2HalfLen) {
                if (relPerpSpeed > 0.0f) {
                    entry1 = 1.0f;
                    exit1  = 1.0f;
                } else {
                    entry1 = -(sep - v2HalfLen) / relPerpSpeed;
                    if (entry1 > 1.0f) {
                        entry1 = 1.0f;
                        exit1  = 1.0f;
                    } else {
                        exit1 = std::min(entry1 - (v2HalfLen + v2HalfLen) / relPerpSpeed, 1.0f);
                    }
                }
            } else if (sep < -v2HalfLen) {
                if (relPerpSpeed < 0.0f) {
                    entry1 = 1.0f;
                    exit1  = 1.0f;
                } else {
                    entry1 = -(sep + v2HalfLen) / relPerpSpeed;
                    if (entry1 > 1.0f) {
                        entry1 = 1.0f;
                        exit1  = 1.0f;
                    } else {
                        exit1 = std::min(entry1 + (v2HalfLen + v2HalfLen) / relPerpSpeed, 1.0f);
                    }
                }
            } else if (relPerpSpeed > 0.0f) {
                entry1 = 0.0f;
                exit1  = (v2HalfLen - sep) / relPerpSpeed;
            } else if (relPerpSpeed < 0.0f) {
                entry1 = 0.0f;
                exit1  = -(sep + v2HalfLen) / relPerpSpeed;
                if (exit1 > 1.0f) {
                    exit1 = 1.0f;
                }
            } else {
                entry1 = 0.0f;
                exit1  = 1.0f;
            }
        }

        //! Axis 2 (along `axis`), asymmetric extents (`[-v2Front, v2HalfWid]`)
        float entry2, exit2;
        {
            const float sep = lx * axis->x + ly * axis->y;
            if (sep > v2HalfWid) {
                if (relAlongSpeed > 0.0f) {
                    entry2 = 1.0f;
                    exit2  = 1.0f;
                } else {
                    entry2 = -(sep - v2HalfWid) / relAlongSpeed;
                    if (entry2 > 1.0f) {
                        entry2 = 1.0f;
                        exit2  = 1.0f;
                    } else {
                        exit2 = std::min(entry2 - (v2HalfWid + v2Front) / relAlongSpeed, 1.0f);
                    }
                }
            } else if (sep < -v2Front) {
                if (relAlongSpeed < 0.0f) {
                    entry2 = 1.0f;
                    exit2  = 1.0f;
                } else {
                    entry2 = -(sep + v2Front) / relAlongSpeed;
                    if (entry2 > 1.0f) {
                        entry2 = 1.0f;
                        exit2  = 1.0f;
                    } else {
                        exit2 = std::min(entry2 + (v2HalfWid + v2Front) / relAlongSpeed, 1.0f);
                    }
                }
            } else if (relAlongSpeed > 0.0f) {
                entry2 = 0.0f;
                exit2  = (v2HalfWid - sep) / relAlongSpeed;
            } else if (relAlongSpeed < 0.0f) {
                entry2 = 0.0f;
                exit2  = -(sep + v2Front) / relAlongSpeed;
                if (exit2 > 1.0f) {
                    exit2 = 1.0f;
                }
            } else {
                entry2 = 0.0f;
                exit2  = 1.0f;
            }
        }

        const float entry = std::max(entry1, entry2);
        if (entry < exit1 && entry < exit2 && entry <= tMin) {
            tMin = entry;
        }
    }

    return tMin;
}

// 0x425F70
float CCarCtrl::TestCollisionBetween2MovingRects_OnlyFrontBumper(CVehicle* vehicle1, CVehicle* vehicle2, float arg3, float arg4, CVector* axis, CVector* orient) {
    const CVector p1 = vehicle1->GetPosition();
    const CVector p2 = vehicle2->GetPosition();

    const CColModel* col2 = CModelInfo::GetModelInfo(vehicle2->m_nModelIndex)->GetColModel();
    const float v2HalfWid = col2->m_boundBox.m_vecMax.y;
    const float v2HalfLen = col2->m_boundBox.m_vecMax.x;

    const CColModel* col1 = CModelInfo::GetModelInfo(vehicle1->m_nModelIndex)->GetColModel();
    const float v1HalfWid = col1->m_boundBox.m_vecMax.y;
    const float v1HalfLen = col1->m_boundBox.m_vecMax.x;
    const float v1Front   = -col1->m_boundBox.m_vecMin.y;

    //! Vehicle 2's box origin, offset by its extents along `orient`
    const float oX = v2HalfLen * orient->y + v2HalfWid * orient->x + p2.x;
    const float oY = v2HalfLen * orient->x + v2HalfWid * orient->y + p2.y;

    float tMin = 1.0f;

    for (int32 i = 0; i < 4; i++) {
        //! The 4 corners of vehicle 1's front bumper
        float cx, cy;
        switch (i) {
        case 0:
            cx = v1HalfWid * orient->x + v1HalfLen * orient->y + p1.x;
            cy = v1HalfWid * orient->y + p1.y - v1HalfLen * orient->x;
            break;
        case 1:
            cx = v1HalfWid * orient->x + p1.x - v1HalfLen * orient->y;
            cy = v1HalfLen * orient->x + v1HalfWid * orient->y + p1.y;
            break;
        case 2:
            cx = v1HalfLen * orient->y + (p1.x - v1Front * orient->x);
            cy = (p1.y - v1Front * orient->y) - v1HalfLen * orient->x;
            break;
        default: // case 3
            cx = (p1.x - v1Front * orient->x) - v1HalfLen * orient->y;
            cy = v1HalfLen * orient->x + (p1.y - v1Front * orient->y);
            break;
        }

        const float d0 = (cx - oX) * axis->x + (cy - oY) * axis->y;
        const float d1 = ((cx + arg3) - oX) * axis->x + ((cy + arg4) - oY) * axis->y;

        if (d0 > 0.0f && d1 < 0.0f) {
            const float s0 = (oX - cx) * arg4 - (oY - cy) * arg3;
            const float s1 = (((v2HalfLen * orient->y + p2.x) - v2HalfWid * orient->x) - cx) * arg4
                           - (((v2HalfLen * orient->x + v2HalfWid * orient->y + p2.y)) - cy) * arg3;
            if (s0 * s1 < 0.0f) {
                const float t = d0 / (d0 - d1);
                if (t <= tMin) {
                    tMin = t;
                }
            }
        }
    }

    return tMin;
}

// 0x429520
void CCarCtrl::TestWhetherToFirePlaneGuns(CVehicle* vehicle, CEntity* target) {
    vehicle->vehicleFlags.bFireGun = false;

    if (vehicle->m_nVehicleWeaponInUse != CAR_WEAPON_NOT_USED && vehicle->m_nVehicleWeaponInUse != CAR_WEAPON_HEAVY_GUN) {
        return;
    }
    if (!target) {
        return;
    }

    const CVector d = target->GetPosition() - vehicle->GetPosition();
    if (d.Magnitude() >= 150.0f) {
        return;
    }

    const CVector dir = d.Normalized();
    const CVector fwd = vehicle->GetForward();
    if (fwd.x * dir.x + fwd.y * dir.y + fwd.z * dir.z > 0.8f) {
        vehicle->vehicleFlags.bFireGun = true;
    }
}

// 0x421FE0
bool CCarCtrl::ThisVehicleShouldTryNotToTurn(CVehicle* vehicle) {
    switch (vehicle->m_nModelIndex) {
    case MODEL_LINERUN:
    case MODEL_DUMPER:
    case MODEL_BUS:
    case MODEL_COACH:
    case MODEL_PACKER:
    case MODEL_FLATBED:
    case MODEL_PETRO:
    case MODEL_RDTRAIN:
    case MODEL_CEMENT:
        return true;
    default:
        return false;
    }
}

// 0x429300
void CCarCtrl::TriggerDogFightMoves(CVehicle* vehicle1, CVehicle* vehicle2) {
    if (vehicle1->m_autoPilot.m_nTempAction != TEMPACT_NONE) {
        return;
    }

    const CVector pos1 = vehicle1->GetPosition();
    const CVector pos2 = vehicle2->GetPosition();
    const CVector d    = pos1 - pos2;

    if (d.Magnitude() > 70.0f) {
        return;
    }

    const CVector dir   = d.Normalized();
    const CVector fwd2  = vehicle2->GetForward();
    const float   along = dir.x * fwd2.x + dir.y * fwd2.y + dir.z * fwd2.z;

    if (std::abs(pos1.z - pos2.z) > 15.0f) {
        return;
    }

    switch (CGeneral::GetRandomNumber() & 0xFF) {
    case 0x0C:
        if (along > 0.0f) {
            vehicle1->m_autoPilot.m_nTempAction     = TEMPACT_PLANE_FLY_STRAIGHT;
            vehicle1->m_autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumber() & 0x3FF) + 1500;
        }
        break;
    case 0x0D:
        if (along > 0.0f) {
            vehicle1->m_autoPilot.m_nTempAction     = TEMPACT_PLANE_SHARP_LEFT;
            vehicle1->m_autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumber() & 0x1FF) + 700;
        }
        break;
    case 0x0E:
        if (along > 0.0f) {
            vehicle1->m_autoPilot.m_nTempAction     = TEMPACT_PLANE_SHARP_RIGHT;
            vehicle1->m_autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumber() & 0x1FF) + 700;
        }
        break;
    case 0x0F:
        if (along > 0.7f) {
            vehicle1->m_autoPilot.m_nTempAction     = TEMPACT_PLANE_FLY_UP;
            vehicle1->m_autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + (CGeneral::GetRandomNumber() & 0x7FF) + 3000;
        }
        break;
    }
}

// 0x424000
void CCarCtrl::UpdateCarCount(CVehicle* vehicle, uint8 bDecrease) {
    if (!bDecrease) {
        switch (vehicle->m_nCreatedBy) {
        case RANDOM_VEHICLE:
            if (vehicle->vehicleFlags.bIsLawEnforcer) {
                ++NumLawEnforcerCars;
            }
            ++NumRandomCars;
            return;
        case MISSION_VEHICLE:
            if (vehicle->vehicleFlags.bIsLawEnforcer) { //!< Mission cars no longer count as law enforcers
                vehicle->vehicleFlags.bIsLawEnforcer = false;
                --NumLawEnforcerCars;
            }
            ++NumMissionCars;
            return;
        case PARKED_VEHICLE:
            ++NumParkedCars;
            return;
        case PERMANENT_VEHICLE:
            ++NumPermanentVehicles;
            return;
        default:
            return;
        }
    } else {
        switch (vehicle->m_nCreatedBy) {
        case RANDOM_VEHICLE:
            if (vehicle->vehicleFlags.bIsLawEnforcer) {
                --NumLawEnforcerCars;
                if ((int32)NumLawEnforcerCars < 0) {
                    NumLawEnforcerCars = 0;
                }
            }
            --NumRandomCars;
            if (NumRandomCars < 0) {
                NumRandomCars = 0;
            }
            return;
        case MISSION_VEHICLE:
            --NumMissionCars;
            if (NumMissionCars < 0) {
                NumMissionCars = 0;
            }
            return;
        case PARKED_VEHICLE:
            --NumParkedCars;
            if ((int32)NumParkedCars < 0) {
                NumParkedCars = 0;
            }
            return;
        case PERMANENT_VEHICLE:
            --NumPermanentVehicles;
            if (NumPermanentVehicles < 0) {
                NumPermanentVehicles = 0;
            }
            return;
        default:
            return;
        }
    }
}

// 0x436540
void CCarCtrl::UpdateCarOnRails(CVehicle* vehicle) {
    auto& autoPilot = vehicle->m_autoPilot;

    StopCarIfNodesAreInvalid(vehicle); //!< Return value intentionally ignored

    if (autoPilot.movementFlags.bIsStopped) {
        return;
    }

    switch (autoPilot.m_nTempAction) {
    case TEMPACT_WAIT:
    case TEMPACT_BRAKE: {
        vehicle->m_vecMoveSpeed = CVector{0.0f, 0.0f, 0.0f};
        autoPilot.ModifySpeed(0.0f);

        //! Stop doing whatever we were doing once we've waited long enough
        if (CTimer::GetTimeInMS() > autoPilot.m_nTempActionTime) {
            autoPilot.m_nTempAction                = TEMPACT_NONE;
            autoPilot.m_nTimeToStartMission        = CTimer::GetTimeInMS();
            autoPilot.m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
        }
        return;
    }
    case TEMPACT_STUCKINTRAFFIC:
        vehicle->m_vecMoveSpeed = CVector{0.0f, 0.0f, 0.0f};
        autoPilot.ModifySpeed(0.0f);
        return;
    default:
        break;
    }

    //! Normal rail update
    SlowCarOnRailsDownForTrafficAndLights(vehicle);

    if (CTimer::GetTimeInMS() >= autoPilot.field_C + (int32)autoPilot.m_nSpeedScaleFactor) {
        PickNextNodeAccordingStrategy(vehicle);
    }

    //! Already on real physics - The rail code isn't in charge
    if (vehicle->GetStatus() == STATUS_PHYSICS) {
        return;
    }

    //! How far along the current link we are
    const uint32 elapsedMs = CTimer::GetTimeInMS() - (uint32)autoPilot.field_C;
    const float  progress  = (float)elapsedMs / (float)autoPilot.m_nSpeedScaleFactor;

    const CCarPathLink& curLink  = ThePaths.GetCarPathLink(autoPilot.m_nCurrentPathNodeInfo);
    const CCarPathLink& nextLink = ThePaths.GetCarPathLink(autoPilot.m_nNextPathNodeInfo);

    //! Directions of both links
    const float dirCurrX = (float)curLink.m_dir.x * 0.01f * (float)autoPilot._smthCurr;
    const float dirCurrY = (float)curLink.m_dir.y * 0.01f * (float)autoPilot._smthCurr;
    const float dirNextX = (float)nextLink.m_dir.x * 0.01f * (float)autoPilot._smthNext;
    const float dirNextY = (float)nextLink.m_dir.y * 0.01f * (float)autoPilot._smthNext;

    //! How far to the side of the links' centre lines we should drive
    float laneCurr = (curLink.OneWayLaneOffset() + (float)autoPilot.m_nCurrentLane) * 5.4f;
    float laneNext = (nextLink.OneWayLaneOffset() + (float)autoPilot.m_nNextLane) * 5.4f;
    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
        laneCurr += 1.4580001f;
        laneNext += 1.4580001f;
    }

    //! Pseudo random lane jitter, taken from the low bits of the path link addresses (And the car's seed)
    const auto RawAddr = [](const CCarPathLinkAddress& addr) {
        return *reinterpret_cast<const uint16*>(&addr);
    };
    const auto Jitter = [&](uint16 addr, int32 bitOffset) {
        return (float)(int32)(((uint32)addr + vehicle->m_nRandomSeed) >> bitOffset & 7) - 3.0f;
    };
    CVector dirCurr{ dirCurrX + Jitter(RawAddr(autoPilot.m_nCurrentPathNodeInfo), 0) * 0.009f, dirCurrY + Jitter(RawAddr(autoPilot.m_nCurrentPathNodeInfo), 3) * 0.009f, 0.0f };
    CVector dirNext{ dirNextX + Jitter(RawAddr(autoPilot.m_nNextPathNodeInfo),    0) * 0.009f, dirNextY + Jitter(RawAddr(autoPilot.m_nNextPathNodeInfo),    3) * 0.009f, 0.0f };
    dirCurr.Normalise();
    dirNext.Normalise();

    //! Lane corrected positions of both links
    const CVector pointCurr{ curLink.GetNodeCoors().x + laneCurr * dirCurrY, curLink.GetNodeCoors().y - laneCurr * dirCurrX, 0.0f };
    const CVector pointNext{ nextLink.GetNodeCoors().x + laneNext * dirNextY, nextLink.GetNodeCoors().y - laneNext * dirNextX, 0.0f };

    //! Interpolate along the curve connecting both links
    CVector point, speed;
    CCurves::CalcCurvePoint(
        pointCurr,
        pointNext,
        dirCurr,
        dirNext,
        progress,
        (int32)autoPilot.m_nSpeedScaleFactor,
        point,
        speed
    );

    point.z = 15.0f;
    DragCarToPoint(vehicle, &point);

    if (autoPilot.m_nCurrentPathNodeInfo.m_wAreaId == autoPilot.m_nNextPathNodeInfo.m_wAreaId
        && autoPilot.m_nCurrentPathNodeInfo.m_wCarPathLinkId == autoPilot.m_nNextPathNodeInfo.m_wCarPathLinkId
    ) {
        return;
    }

    //! The speed is in units of 1/60th of a second
    vehicle->m_vecMoveSpeed = speed * 0.016666668f;
}

// 0x426BC0
void CCarCtrl::WeaveForObject(CEntity* entity, CVehicle* vehicle, float* arg3, float* arg4) {
    float offRight, offFwd;

    const eModelID modelIndex = (eModelID)entity->m_nModelIndex;
    if (modelIndex == ModelIndices::MI_TRAFFICLIGHTS) {
        offRight = 2.957f;
        offFwd   = 0.147f;
    } else if (modelIndex == ModelIndices::MI_SINGLESTREETLIGHTS1) {
        offRight = 0.744f;
        offFwd   = 0.0f;
    } else if (modelIndex == ModelIndices::MI_SINGLESTREETLIGHTS2) {
        offRight = 0.043f;
        offFwd   = 0.0f;
    } else if (modelIndex == ModelIndices::MI_SINGLESTREETLIGHTS3) {
        offRight = 1.143f;
        offFwd   = 0.145f;
    } else if (modelIndex == ModelIndices::MI_DOUBLESTREETLIGHTS) {
        offRight = 0.0f;
        offFwd   = -0.048f;
    } else {
        if (!CModelInfo::GetModelInfo(entity->m_nModelIndex)->SwaysInWind()) {
            return;
        }
        offRight = 0.0f;
        offFwd   = 0.0f;
    }

    //! The object's origin, offset along its own right/forward axes
    const CVector objPos = entity->GetPosition();
    const CVector right  = entity->GetRight();
    const CVector fwd    = entity->GetForward();
    const CVector objPosOfs{
        objPos.x + right.x * offRight + fwd.x * offFwd,
        objPos.y + right.y * offRight + fwd.y * offFwd,
        objPos.z
    };

    const CVector vehPos = vehicle->GetPosition();

    const float dx      = objPosOfs.x - vehPos.x;
    const float dy      = objPosOfs.y - vehPos.y;
    const float heading = CGeneral::GetATanOfXY(dx, dy);

    //! Half the vehicle length (plus a bit), converted to an angle at the current distance
    const CColModel* vehCol = CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel();
    const float      dist   = std::sqrt(dx * dx + dy * dy);
    const float      delta  = ((vehCol->m_boundBox.m_vecMax.x * 2.4f + 0.3f) / dist) * 0.5f;

    if (std::abs(CGeneral::LimitRadianAngle(heading - *arg3)) < delta) {
        *arg3 = heading - delta;
        if (*arg3 < -PI) {
            *arg3 += TWO_PI;
        }
    }
    if (std::abs(CGeneral::LimitRadianAngle(heading - *arg4)) < delta) {
        *arg4 = heading + delta;
        if (*arg4 > PI) {
            *arg4 -= TWO_PI;
        }
    }
}

/*!
 * @addr 0x421A50
 * @brief Test if `vehicle1`'s box (given by its 4 corners) would collide with `vehicle2`'s box
 *        (given by its 4 corners) when it is moved along the `angle` direction at the given speed.
 *
 * @note The original builds a swept rectangle from `angle` and tests 12 line-segment pairs using
 *       `CCollision::Test2DLineAgainst2DLine`. The displacement (`(speed - speedMagnitude * dir(angle)) * 100`)
 *       and the line-pair test could be recovered, but not the exact 12 pairs, so this reimplementation
 *       tests every edge of the moved box against every edge of the other box.
 *
 * @todo(re) 0x421A50 - Recover the exact set of line pairs tested by the original.
 */
static bool TestCollisionRectsForAngle(float angle, const CVector2D (&rectA)[4], const CVector2D (&rectB)[4], float speedX, float speedY, float speedMagnitude, bool swap) {
    const CVector2D *a = rectA, *b = rectB;
    if (swap) {
        a = rectB;
        b = rectA;
    }

    const float dispX = (speedX - std::cos(angle) * speedMagnitude) * 100.0f;
    const float dispY = (speedY - std::sin(angle) * speedMagnitude) * 100.0f;

    for (auto i = 0u; i < 4u; i++) {
        const CVector2D& a0 = a[i] + CVector2D{dispX, dispY};
        const CVector2D& a1 = a[(i + 1) % 4] + CVector2D{dispX, dispY};
        for (auto j = 0u; j < 4u; j++) {
            const CVector2D& b0 = b[j];
            const CVector2D& b1 = b[(j + 1) % 4];
            if (CCollision::Test2DLineAgainst2DLine(a0.x, a0.y, a1.x, a1.y, b0.x, b0.y, b1.x, b1.y)) {
                return true;
            }
        }
    }
    return false;
}

// 0x426350
void CCarCtrl::WeaveForOtherCar(CEntity* entity, CVehicle* vehicle, float* arg3, float* arg4) {
    const eCarMission vehMission = vehicle->m_autoPilot.m_nCarMission;
    CEntity* const target = reinterpret_cast<CEntity*>(vehicle->m_autoPilot.m_TargetEntity);

    //! These pairs have a dedicated behaviour elsewhere
    if ((vehMission == MISSION_RAMPLAYER_CLOSE && entity == FindPlayerVehicle(-1, false))
        || (vehMission == MISSION_RAMCAR_CLOSE && entity == target)
        || (vehMission == MISSION_FOLLOWCAR_CLOSE && (entity == target
                || (entity->GetType() == ENTITY_TYPE_VEHICLE && entity->AsVehicle()->vehicleFlags.bPartOfConvoy)))
        || (vehMission == MISSION_KILLPED_CLOSE
            && entity->GetType() == ENTITY_TYPE_PED
            && entity->AsPed()->bInVehicle
            && entity->AsPed()->m_pVehicle == target)
    ) {
        return;
    }

    //! The other entity is escorting us
    if (entity->GetType() == ENTITY_TYPE_VEHICLE) {
        const auto* const otherVeh = entity->AsVehicle();
        if (otherVeh->m_autoPilot.m_nCarMission >= MISSION_PROTECTION_REAR
            && otherVeh->m_autoPilot.m_nCarMission <= MISSION_ESCORT_FRONT
            && otherVeh->m_autoPilot.m_TargetEntity == vehicle
        ) {
            return;
        }
    }

    const CVector entPos = entity->GetPosition();
    const CVector vehPos = vehicle->GetPosition();
    const CVector vehFwd = vehicle->GetForward();

    const float relX  = entPos.x - vehPos.x;
    const float relY  = entPos.y - vehPos.y;
    const float along = relX * vehFwd.x + relY * vehFwd.y;

    if (along < 0.0f) {
        return;
    }

    //! The 4 corners of both boxes (expanded by 0.2m)
    const CColModel* vehCol = CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel();
    const CColModel* entCol = CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel();
    const CVector    vehRight = vehicle->GetRight();
    const CVector    entFwd   = entity->GetForward();
    const CVector    entRight = entity->GetRight();

    const float vehLen = vehCol->m_boundBox.m_vecMax.x + 0.2f;
    const float vehWid = vehCol->m_boundBox.m_vecMax.y + 0.2f;
    const float entLen = entCol->m_boundBox.m_vecMax.x + 0.2f;
    const float entWid = entCol->m_boundBox.m_vecMax.y + 0.2f;

    CVector2D vehCorners[4], entCorners[4];
    for (auto i = 0u; i < 4u; i++) {
        const float sx = (i & 1) ? -1.0f : 1.0f;
        const float sy = (i & 2) ? -1.0f : 1.0f;
        vehCorners[i] = CVector2D{
            vehPos.x + vehRight.x * vehLen * sx + vehFwd.x * vehWid * sy,
            vehPos.y + vehRight.y * vehLen * sx + vehFwd.y * vehWid * sy
        };
        entCorners[i] = CVector2D{
            entPos.x + entRight.x * entLen * sx + entFwd.x * entWid * sy,
            entPos.y + entRight.y * entLen * sx + entFwd.y * entWid * sy
        };
    }

    const float vehSpeed2D = std::sqrt(vehicle->m_vecMoveSpeed.x * vehicle->m_vecMoveSpeed.x + vehicle->m_vecMoveSpeed.y * vehicle->m_vecMoveSpeed.y);

    //! Narrow the allowed steering arc, 6 degrees at a time
    for (auto steps = 0; TestCollisionRectsForAngle(*arg3, vehCorners, entCorners, vehicle->m_vecMoveSpeed.x, vehicle->m_vecMoveSpeed.y, vehSpeed2D, false); ) {
        if (++steps >= 8) {
            break;
        }
        *arg3 -= 0.10471976f; //!< 6 degrees
        if (*arg3 < 0.0f) {
            *arg3 += TWO_PI;
        }
    }

    for (auto steps = 0; TestCollisionRectsForAngle(*arg4, vehCorners, entCorners, vehicle->m_vecMoveSpeed.x, vehicle->m_vecMoveSpeed.y, vehSpeed2D, true); ) {
        if (++steps >= 8) {
            return;
        }
        *arg4 += 0.10471976f; //!< 6 degrees
        if (*arg4 > TWO_PI) {
            *arg4 -= TWO_PI;
        }
    }
}

// 0x426970
void CCarCtrl::WeaveForPed(CPed* ped, CVehicle* vehicle, float* arg3, float* arg4) {
    const eCarMission mission = vehicle->m_autoPilot.m_nCarMission;

    //! These pairs have a dedicated behaviour elsewhere
    if ((mission == MISSION_RAMPLAYER_CLOSE && ped == FindPlayerPed(-1))
        || (mission == MISSION_KILLPED_CLOSE && ped == reinterpret_cast<CEntity*>(vehicle->m_autoPilot.m_TargetEntity))
    ) {
        return;
    }

    const CVector pedPos = ped->GetPosition();
    const CVector vehPos = vehicle->GetPosition();

    const float dx      = pedPos.x - vehPos.x;
    const float dy      = pedPos.y - vehPos.y;
    const float heading = CGeneral::GetATanOfXY(dx, dy);
    const float dist    = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.0f) {
        return;
    }

    //! Half the vehicle's width (plus a bit), converted to an angle at the current distance
    const float delta = ((vehicle->GetColModel()->m_boundBox.m_vecMax.x * 2.4f + 0.8f) / dist) * 0.5f;

    { //! Widen the left bound
        float rel = heading - *arg3;
        while (rel < -PI) rel += TWO_PI;
        while (rel > PI)  rel -= TWO_PI;
        if (std::abs(rel) < delta) {
            *arg3 = heading - delta;
            if (*arg3 < -PI) {
                *arg3 += TWO_PI;
            }
        }
    }
    { //! Widen the right bound
        float rel = heading - *arg4;
        while (rel < -PI) rel += TWO_PI;
        while (rel > PI)  rel -= TWO_PI;
        if (std::abs(rel) < delta) {
            *arg4 = heading + delta;
            if (*arg4 > PI) {
                *arg4 -= TWO_PI;
            }
        }
    }
}

// 0x42D680
template<typename PtrListType>
void CCarCtrl::WeaveThroughCarsSectorList(PtrListType& ptrList, CVehicle* vehicle, CPhysical* physical, float arg4, float arg5, float arg6, float arg7, float* arg8, float* arg9) {
    for (CPhysical* const phys : ptrList) {
        if (phys->IsScanCodeCurrent()) {
            continue;
        }
        if (!phys->m_bUsesCollision) {
            continue;
        }
        if (phys == physical) {
            continue;
        }
        phys->SetCurrentScanCode();

        const CVector boundCentre = phys->GetBoundCentre();
        if (!(arg4 < boundCentre.x && boundCentre.x < arg6 && arg5 < boundCentre.y && boundCentre.y < arg7)) {
            continue;
        }

        if (std::abs(phys->GetPosition().z - vehicle->GetPosition().z) >= 8.0f) {
            continue;
        }
        if (phys == vehicle) {
            continue;
        }
        //! Convoy cars only weave for non-convoy traffic
        if (vehicle->vehicleFlags.bPartOfConvoy && phys->AsVehicle()->vehicleFlags.bPartOfConvoy) {
            continue;
        }

        WeaveForOtherCar(phys, vehicle, arg8, arg9);
    }
}

// 0x42D950
template<typename PtrListType>
void CCarCtrl::WeaveThroughObjectsSectorList(PtrListType& ptrList, CVehicle* vehicle, float arg3, float arg4, float arg5, float arg6, float* arg7, float* arg8) {
    for (CPhysical* const phys : ptrList) {
        if (phys->IsScanCodeCurrent()) {
            continue;
        }
        if (!phys->m_bUsesCollision) {
            continue;
        }
        phys->SetCurrentScanCode();

        const CVector pos = phys->GetPosition();
        if (!(arg3 < pos.x && pos.x < arg5 && arg4 < pos.y && pos.y < arg6)) {
            continue;
        }

        if (std::abs(pos.z - vehicle->GetPosition().z) >= 8.0f) {
            continue;
        }
        //! Only weave around objects while (nearly) upright
        if (vehicle->GetMatrix().GetUp().z <= 0.9f) {
            continue;
        }

        WeaveForObject(phys, vehicle, arg7, arg8);
    }
}

// 0x42D7E0
template<typename PtrListType>
void CCarCtrl::WeaveThroughPedsSectorList(PtrListType& ptrList, CVehicle* vehicle, CPhysical* physical, float arg4, float arg5, float arg6, float arg7, float* arg8, float* arg9) {
    for (CPhysical* const phys : ptrList) {
        if (phys->IsScanCodeCurrent()) {
            continue;
        }
        if (!phys->m_bUsesCollision) {
            continue;
        }
        if (phys == physical) {
            continue;
        }
        phys->SetCurrentScanCode();

        const CVector pos = phys->GetPosition();
        if (!(arg4 < pos.x && pos.x < arg6 && arg5 < pos.y && pos.y < arg7)) {
            continue;
        }

        if (std::abs(pos.z - vehicle->GetPosition().z) >= 4.0f) {
            continue;
        }

        //! @todo(re) 0x42D911/0x42D919 - The exe reads two `CPed` fields (offsets `+0x584` / `+0x0FC`) that hold
        //!            vehicle references; only `m_pVehicle`/`m_pContactEntity` match those slots in the repo.
        CPed* const ped = reinterpret_cast<CPed*>(phys);
        if (ped->m_pVehicle == vehicle || ped->m_pContactEntity == vehicle) {
            continue;
        }

        WeaveForPed(ped, vehicle, arg8, arg9);
    }
}

// 0x427FE0
float CCarCtrl::FindMaxSteerAngle(CVehicle* veh) {
    return std::clamp(0.9f - veh->GetMoveSpeed().Magnitude(), 0.2f, 0.7f);
}
