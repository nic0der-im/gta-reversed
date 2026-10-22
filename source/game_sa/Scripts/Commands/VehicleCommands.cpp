#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>
#include <cassert>

#include "PlayerInfo.h"
#include "World.h"
#include "CarGenerator.h"
#include "TheCarGenerators.h"
#include "CommandParser/Parser.hpp"
#include "CarCtrl.h"
#include "Timer.h"
#include "Radar.h"
#include "VehicleRecording.h"
#include "General.h"
#include "PathFind.h"
#include "LoadedCarGroup.h"
#include "Camera.h"
#include "Object.h"
#include "CarEnterExit.h"
#include "VehicleModelInfo.h"
#include "Streaming.h"
#include "Shopping.h"
#include "Plane.h"
#include "Train.h"
#include "Automobile.h"
#include "Population.h"
#include "MenuSystem.h"
#include <TaskTypes/TaskComplexEnterCarAsDriverTimed.h>
#include <TaskTypes/TaskComplexEnterCarAsPassengerTimed.h>
#include <TaskTypes/TaskComplexLeaveCar.h>
#include <TaskTypes/TaskComplexDriveToPoint.h>
#include <TaskTypes/TaskComplexDriveWander.h>
#include <TaskTypes/TaskComplexLeaveAnyCar.h>
#include <TaskTypes/TaskComplexShuffleSeats.h>
#include <TaskTypes/TaskSimpleCarSetTempAction.h>
#include <TaskTypes/TaskComplexCarDriveMission.h>
#include <TaskTypes/TaskSimpleCarSetPedInAsPassenger.h>
#include <TaskTypes/SeekEntity/TaskComplexSeekEntityStandard.h>
#include <TaskTypes/TaskComplexLeaveCarAndFlee.h>
#include <TaskTypes/TaskComplexDestroyCar.h>
#include <TaskTypes/TaskComplexDrivePointRoute.h>
#include "CarAI.h"
#include "ModelIndices.h"
#include "UpsideDownCarCheck.h"
#include "Bike.h"
using namespace notsa::script;

/*!
* Various vehicle commands
*/

namespace {
void ClearHeliOrientation(CHeli& heli) {
    heli.ClearHeliOrientation();
}

void RemoveRCBuggy() {
    FindPlayerInfo().BlowUpRCBuggy(false);
}

void SetCarProofs(CVehicle& veh, bool bullet, bool fire, bool explosion, bool collision, bool melee) {
    auto& flags           = veh.physicalFlags;
    flags.bBulletProof    = bullet;
    flags.bFireProof      = fire;
    flags.bExplosionProof = explosion;
    flags.bCollisionProof = collision;
    flags.bMeleeProof     = melee;
}

void SwitchCarGenerator(int32 generatorId, int32 count) {
    const auto generator = CTheCarGenerators::Get(generatorId);
    if (count) {
        generator->SwitchOn();
        if (count <= 100) {
            generator->m_nGenerateCount = count;
        }
    } else {
        generator->SwitchOff();
    }
}

void SetHasBeenOwnedForCarGenerator(int32 generatorId, bool alreadyOwned) {
    CTheCarGenerators::Get(generatorId)->bPlayerHasAlreadyOwnedCar = alreadyOwned;
}

float GetCarSpeed(CVehicle& veh) {
    return veh.m_vecMoveSpeed.Magnitude() * 50.f;
}

void SetCarDrivingStyle(CVehicle& veh, eCarDrivingStyle style) {
    veh.m_autoPilot.m_nCarDrivingStyle = style;
}

bool IsFirstCarColor(CVehicle& veh, int32 color) {
    return veh.m_nPrimaryColor == color;
}

bool IsSecondCarColor(CVehicle& veh, int32 color) {
    return veh.m_nSecondaryColor == color;
}

MultiRet<uint8, uint8> GetExtraCarColors(CVehicle& veh) {
    return { veh.m_nTertiaryColor, veh.m_nQuaternaryColor };
}

void PopCarBootUsingPhysics(CAutomobile& automobile) {
    automobile.PopBootUsingPhysics();
}

void SkipToNextAllowedStation(CTrain& train) {
    CTrain::SkipToNextAllowedStation(&train);
}

void SetRailTrackResistanceMult(float value) {
    CVehicle::ms_fRailTrackResistance = CVehicle::ms_fRailTrackResistanceDefault * (value > 0.0f ? value : 1.0f);
}

void DisableHeliAudio(CVehicle& vehicle, bool enable) {
    if (enable) {
        vehicle.m_vehicleAudio.EnableHelicoptor();
    } else {
        vehicle.m_vehicleAudio.DisableHelicoptor();
    }
}

bool IsCarInAirProper(CRunningScript& S, CVehicle& vehicle) {
    for (auto* const e : vehicle.GetCollidingEntities()) {
        if (e && (e->GetIsTypeBuilding() || e->GetIsTypeVehicle())) {
            return false;
        }
    }
    return true;
}

bool IsCarStuck(CVehicle& vehicle) {
    return CTheScripts::StuckCars.HasCarBeenStuckForAWhile(GetVehiclePool()->GetRef(&vehicle));
}

void AddStuckCarCheck(CVehicle& vehicle, float stuckRadius, uint32 time) {
    CTheScripts::StuckCars.AddCarToCheck(GetVehiclePool()->GetRef(&vehicle), stuckRadius, time, false, false, false, false, 0);
}

void RemoveStuckCarCheck(CVehicle& vehicle) {
    CTheScripts::StuckCars.RemoveCarFromCheck(GetVehiclePool()->GetRef(&vehicle));
}

void AddStuckCarCheckWithWarp(CVehicle& vehicle, float stuckRadius, uint32 time, bool stuck, bool flipped, bool inWater, int8 numberOfNodesToCheck) {
    CTheScripts::StuckCars.AddCarToCheck(GetVehiclePool()->GetRef(&vehicle), stuckRadius, time, true, stuck, flipped, inWater, numberOfNodesToCheck);
}

void PlaneAttackPlayerUsingDogFight(CPlane& plane, CPlayerPed& player, float altitude) {
    if (plane.m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && plane.m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        plane.m_autoPilot.SetCarMission(eCarMission::MISSION_PLANE_DOG_FIGHT_PLAYER);
    }
    plane.m_minAltitude = altitude;
}

/// SET_CAR_ALWAYS_CREATE_SKIDS(07EE)
void SetCarAlwaysCreateSkids(CVehicle& vehicle, bool enable) {
    vehicle.vehicleFlags.bAlwaysSkidMarks = enable;
}

/// SET_CAR_AS_MISSION_CAR(0763)
void SetCarAsMissionCar(CRunningScript& S, CVehicle& vehicle) {
    if (S.m_UsesMissionCleanup && (vehicle.IsCreatedBy(eVehicleCreatedBy::RANDOM_VEHICLE) || vehicle.IsCreatedBy(eVehicleCreatedBy::PARKED_VEHICLE))) {
        vehicle.SetVehicleCreatedBy(eVehicleCreatedBy::MISSION_VEHICLE);
        CTheScripts::MissionCleanUp.AddEntityToList(vehicle);
    }
}

/// CREATE_CAR(00A5)
CVehicle* CreateCar(CRunningScript& S, notsa::script::Model model, CVector posn) {
    return CCarCtrl::CreateCarForScript(model, posn, S.m_IsExternal);
}

/// DELETE_CAR(00A6)
void DeleteCar(CRunningScript& S, ScriptEntity<CVehicle> vehicle) {
    if (vehicle.e) {
        CWorld::Remove(vehicle.e);
        CWorld::RemoveReferencesToDeletedObject(vehicle.e);
        delete vehicle.e;
    }

    if (S.m_IsExternal) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(vehicle.h, MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
    }
}

/// CAR_SET_IDLE(00A9)
void CarSetIdle(CVehicle& vehicle) {
    // The crash-and-burn missions are never cleared (See 0x463490 as well)
    if (vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        vehicle.m_autoPilot.m_nCarMission = eCarMission::MISSION_NONE;
    }
}

/// GET_CAR_COORDINATES(00AA)
CVector GetCarCoordinates(const CVehicle& vehicle) {
    return vehicle.GetPosition();
}

/// SET_CAR_COORDINATES(00AB)
void SetCarCoordinates(CVehicle& vehicle, CVector posn) {
    CCarCtrl::SetCoordsOfScriptCar(&vehicle, posn.x, posn.y, posn.z, 0, 1);
}

/// SET_CAR_CRUISE_SPEED(00AD)
void SetCarCruiseSpeed(CVehicle& vehicle, float speed) {
    vehicle.m_autoPilot.m_nCruiseSpeed = static_cast<uint8>(speed);
}

/// SET_CAR_MISSION(00AF)
void SetCarMission(CVehicle& vehicle, int32 mission) {
    // 0x463490: the crash-and-burn missions are not overridden
    if (vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        vehicle.m_autoPilot.m_nCarMission = static_cast<eCarMission>(mission & 0xFF);
    }
    vehicle.m_autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS();
    // 0x41BDD0(1): switch the engine on unless it's broken
    vehicle.vehicleFlags.bEngineOn = !vehicle.vehicleFlags.bEngineBroken;
}

/// IS_CAR_IN_AREA_2D(00B0)
bool IsCarInArea2D(CRunningScript& S, CVehicle& vehicle, float x1, float y1, float x2, float y2, bool highlight) {
    if (highlight) {
        S.HighlightImportantArea(CVector2D{x1, y1}, CVector2D{x2, y2});
    }
    return vehicle.IsWithinArea(x1, y1, x2, y2);
}

/// IS_CAR_DEAD(0119)
bool IsCarDead(CVehicle* vehicle) {
    if (!vehicle || vehicle->GetStatus() == STATUS_WRECKED) {
        return true;
    }
    return vehicle->vehicleFlags.bIsDrowning; // Bit 6 of `veh+0x42B`
}

/// IS_CAR_MODEL(0137)
bool IsCarModel(const CVehicle& vehicle, notsa::script::Model model) {
    return vehicle.GetModelIndex() == model.value;
}

/// CREATE_CAR_GENERATOR(014B)
int32 CreateCarGenerator(CVector posn, float angle, int32 modelId, int16 color1, int16 color2, uint8 forceSpawn, uint8 alarmChances, uint8 doorLockChances, uint16 minDelay, uint16 maxDelay) {
    if (posn.z > MAP_Z_LOW_LIMIT) {
        posn.z += 0.015f; // 0x859F00
    }
    return CTheCarGenerators::CreateCarGenerator(posn, angle, modelId, color1, color2, forceSpawn, alarmChances, doorLockChances, minDelay, maxDelay, 0, true);
}

/// GET_CAR_HEADING(0174)
float GetCarHeading(const CVehicle& vehicle) {
    return FixAngleDegrees(RadiansToDegrees(vehicle.GetHeading()));
}

/// SET_CAR_HEADING(0175)
void SetCarHeading(CVehicle& vehicle, float heading) {
    vehicle.SetHeading(DegreesToRadians(FixAngleDegrees(heading)));
    vehicle.UpdateRwMatrix();
}

/// ADD_BLIP_FOR_CAR(0186)
tBlipHandle AddBlipForCar(CVehicle& vehicle) {
    const auto blip = CRadar::SetEntityBlip(BLIP_CAR, GetVehiclePool()->GetRef(&vehicle), 0, BLIP_DISPLAY_BOTH);
    CRadar::ChangeBlipScale(blip, 3);
    return blip;
}

/// LOCATE_CAR_2D(01AD), LOCATE_CAR_3D(01AF), LOCATE_STOPPED_CAR_3D(01B0)
OpcodeResult LocateCar(CRunningScript& S, eScriptCommands command) {
    S.LocateCarCommand((int32)command);
    return OR_CONTINUE;
}

/// IS_CAR_STOPPED(01C1)
bool IsCarStopped(CVehicle& vehicle) {
    return CTheScripts::IsVehicleStopped(&vehicle);
}

/// MARK_CAR_AS_NO_LONGER_NEEDED(01C3)
void MarkCarAsNoLongerNeeded(CRunningScript& S, ScriptEntity<CVehicle> vehicle) {
    CTheScripts::CleanUpThisVehicle(vehicle.e);

    if (S.m_IsExternal) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(vehicle.h, MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
    }
}

/// SET_CAR_DENSITY_MULTIPLIER(01EB)
void SetCarDensityMultiplier(float multiplier) {
    CCarCtrl::CarDensityMultiplier = multiplier;
}

/// IS_CAR_UPSIDEDOWN(01F4)
bool IsCarUpsideDown(CVehicle& vehicle) {
    return vehicle.GetMatrix().GetUp().z < 0.3f; // 0x858C24
}

/// LOCK_CAR_DOORS(020A)
void LockCarDoors(CVehicle& vehicle, int32 lockState) {
    vehicle.m_nDoorLock = static_cast<eCarLock>(lockState);
}

/// IS_CAR_UPRIGHT(020D)
bool IsCarUpright(CVehicle& vehicle) {
    return vehicle.GetMatrix().GetUp().z >= 0.0f;
}

/// SET_CAR_HEALTH(0224)
void SetCarHealth(CVehicle& vehicle, int32 health) {
    vehicle.m_fHealth = static_cast<float>(health);
}

/// GET_CAR_HEALTH(0227)
int32 GetCarHealth(const CVehicle& vehicle) {
    return static_cast<int32>(vehicle.m_fHealth);
}

/// CHANGE_CAR_COLOUR(0229)
void ChangeCarColour(CVehicle& vehicle, uint8 colour1, uint8 colour2) {
    vehicle.m_nPrimaryColor   = colour1;
    vehicle.m_nSecondaryColor = colour2;
}

/// TURN_CAR_TO_FACE_COORD(039F)
void TurnCarToFaceCoord(CVehicle& vehicle, float x, float y) {
    const auto pos = vehicle.GetPosition();
    float heading = CGeneral::GetATanOfXY(pos.x - x, pos.y - y) + HALF_PI;
    if (heading > TWO_PI) {
        heading -= TWO_PI;
    }
    vehicle.SetHeading(heading);
}

/// CLEAR_AREA_OF_CARS(03BA)
void ClearAreaOfCars(float xA, float yA, float zA, float xB, float yB, float zB) {
    CWorld::ClearCarsFromArea(
        std::min(xA, xB), std::min(yA, yB), std::min(zA, zB),
        std::max(xA, xB), std::max(yA, yB), std::max(zA, zB)
    );
}

/// CREATE_RANDOM_CAR_FOR_CAR_PARK(03C5)
CVehicle* CreateRandomCarForCarPark(CVector posn, float heading) {
    if (CCarCtrl::NumRandomCars > 0x2C) {
        return nullptr;
    }

    const auto model = CPopulation::m_AppropriateLoadedCars.PickRandomCar(false, true);
    if (model == MODEL_INVALID) {
        return nullptr;
    }

    auto* const vehicle = CCarCtrl::GetNewVehicleDependingOnCarModel(model, eVehicleCreatedBy::RANDOM_VEHICLE);
    if (!vehicle) {
        return nullptr;
    }

    posn.z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
    vehicle->SetPosn(posn);
    vehicle->SetHeading(DegreesToRadians(heading));
    CTheScripts::ClearSpaceForMissionEntity(posn, vehicle);

    vehicle->SetStatus(STATUS_ABANDONED);
    vehicle->vehicleFlags.bIsLocked          = false;
    vehicle->vehicleFlags.bIsCarParkVehicle  = true;
    CCarCtrl::JoinCarWithRoadSystem(vehicle);

    if (vehicle->m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && vehicle->m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        vehicle->m_autoPilot.m_nCarMission = eCarMission::MISSION_NONE;
    }
    vehicle->m_autoPilot.m_nTempAction      = TEMPACT_NONE;
    vehicle->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    vehicle->m_autoPilot.m_speed            = 9.0f;
    vehicle->m_autoPilot.SetCruiseSpeed(9);
    vehicle->vehicleFlags.bEngineOn = false;

    CWorld::Add(vehicle);
    return vehicle;
}

/// GET_CLOSEST_CAR_NODE_WITH_HEADING(03D3)
MultiRet<CVector, float> GetClosestCarNodeWithHeading(CVector posn) {
    if (posn.z <= MAP_Z_LOW_LIMIT) {
        posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);
    }
    // TODO(re): the retail node search (`0x44F460`, node-type mask `0x497423FE`) isn't reversed,
    // so the plain closest car node is used instead.
    const auto node = ThePaths.FindNodeClosestToCoors(posn, PATH_TYPE_VEH, 999999.88f, 1, 0, 0, 0, 0);
    bool       found{};
    const auto coords = ThePaths.FindNodeCoorsForScript(node, &found);
    if (!found) {
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    }
    return {coords, ThePaths.FindNodeOrientationForCarPlacement(node)};
}

/// GET_CAR_COLOURS(03F3)
MultiRet<uint8, uint8> GetCarColours(const CVehicle& vehicle) {
    return { vehicle.m_nPrimaryColor, vehicle.m_nSecondaryColor };
}

/// SET_ALL_CARS_CAN_BE_DAMAGED(03F4)
void SetAllCarsCanBeDamaged(bool canBeDamaged) {
    CWorld::SetAllCarsCanBeDamaged(canBeDamaged);
    if (!canBeDamaged) {
        CWorld::ExtinguishAllCarFiresInArea(FindPlayerCoors(), 4000.0f);
    }
}

/// GET_OFFSET_FROM_CAR_IN_WORLD_COORDS(0407)
CVector GetOffsetFromCarInWorldCoords(const CVehicle& vehicle, CVector offset) {
    return vehicle.TransformFromObjectSpace(offset);
}

/// IS_CAR_PASSENGER_SEAT_FREE(0431)
bool IsCarPassengerSeatFree(CVehicle& vehicle, int32 seat) {
    return seat < vehicle.m_nMaxPassengers && !vehicle.HasPassengerAtSeat(seat);
}

/// GET_CAR_MODEL(0441)
eModelID GetCarModel(const CVehicle& vehicle) {
    return static_cast<eModelID>(vehicle.m_nModelIndex);
}

/// GET_DRIVER_OF_CAR(046C)
CPed* GetDriverOfCar(const CVehicle& vehicle) {
    return vehicle.m_pDriver;
}

/// IS_CAR_ON_FIRE(0495)
bool IsCarOnFire(const CVehicle& vehicle) {
    // TODO(re): retail also tests the engine status (`CDamageManager`) and the pointer at `veh+0x590`,
    // neither of which is accessible from `CVehicle` here.
    return vehicle.m_pFire != nullptr || vehicle.m_fHealth < 250.0f; // 0x859F80
}

/// GET_NTH_CLOSEST_CAR_NODE(04D3)
MultiRet<CVector, CompareFlagUpdate> GetNthClosestCarNode(CVector posn, int32 n) {
    if (posn.z <= MAP_Z_LOW_LIMIT) {
        posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);
    }
    // TODO(re): the retail node search (`0x44F8C0`, node-type mask `0x497423FE`) isn't reversed
    CNodeAddress nthNode{};
    const auto   node = ThePaths.FindNthNodeClosestToCoors(posn, PATH_TYPE_VEH, 999999.88f, false, false, n - 1, false, false, &nthNode);
    bool         found{};
    const auto   coords = ThePaths.FindNodeCoorsForScript(node, &found);
    return MultiRet<CVector, CompareFlagUpdate>{found ? coords : CVector{0.0f, 0.0f, 0.0f}, CompareFlagUpdate{found}};
}

/// SET_CAR_STRAIGHT_LINE_DISTANCE(04E0)
void SetCarStraightLineDistance(CVehicle& vehicle, int32 distance) {
    vehicle.m_autoPilot.m_nStraightLineDistance = static_cast<uint8>(distance);
}

/// SET_CAR_ONLY_DAMAGED_BY_PLAYER(02AA)
void SetCarOnlyDamagedByPlayer(CVehicle& vehicle, bool onlyPlayer) {
    vehicle.physicalFlags.bInvulnerable = onlyPlayer;
}

/// IS_CAR_IN_WATER(02BF)
bool IsCarInWater(const CVehicle& vehicle) {
    return vehicle.physicalFlags.bSubmergedInWater;
    // TODO(re): retail also treats the Skimmer as being in water when `veh+0x7D4 < 1.0f` and the
    // contact surface (`veh+0x747`) is shallow water; neither field is identified in this repo.
}

/// IS_CAR_ON_SCREEN(02CA)
bool IsCarOnScreen(const CVehicle& vehicle) {
    const auto radius = vehicle.GetModelInfo()->GetColModel()->GetBoundRadius();
    return TheCamera.IsSphereVisible(vehicle.GetBoundCentre(), radius);
}

/// SET_CAR_MODEL_COMPONENTS(0506)
void SetCarModelComponents(int32 /*unused*/, int32 component1, int32 component2) {
    CVehicleModelInfo::ms_compsToUse[0] = static_cast<int8>(component1);
    CVehicleModelInfo::ms_compsToUse[1] = static_cast<int8>(component2);
}

/// SORT_OUT_OBJECT_COLLISION_WITH_CAR(050E)
void SortOutObjectCollisionWithCar(CObject& object, CVehicle& vehicle) {
    // TODO(re): retail writes the vehicle pointer to `obj+0x128`; the closest named member is the ignored collision entity
    object.m_pEntityIgnoredCollision = &vehicle;
}

/// FREEZE_CAR_POSITION(0519)
void FreezeCarPosition(CVehicle& vehicle, bool freeze) {
    if (freeze) {
        vehicle.physicalFlags.bDisableCollisionForce = true;  // 0x200C
        vehicle.physicalFlags.bCollidable            = false;
        vehicle.physicalFlags.bDontApplySpeed        = true;
        vehicle.SkipPhysics();
        vehicle.m_vecMoveSpeed = CVector{}; // 0x441130(0, 0, 0)
        vehicle.m_vecTurnSpeed = CVector{}; // 0x45AFB0(0, 0, 0)
    } else {
        vehicle.physicalFlags.bDisableCollisionForce = false;
        vehicle.physicalFlags.bCollidable            = true;
        vehicle.physicalFlags.bDontApplySpeed        = false;
    }
}

/// HAS_CAR_BEEN_DAMAGED_BY_CAR(051D)
bool HasCarBeenDamagedByCar(const CVehicle& vehicle, int32 otherVehicleHandle) {
    const auto lastDamager = vehicle.m_pLastDamageEntity;
    if (!lastDamager) {
        return false;
    }
    if (otherVehicleHandle == -1) {
        return lastDamager->GetIsTypeVehicle();
    }
    return lastDamager == GetVehiclePool()->GetAtRef(otherVehicleHandle);
}

/// GET_RANDOM_CAR_OF_TYPE_IN_AREA_NO_SAVE(053E)
int32 GetRandomCarOfTypeInAreaNoSave(CRect area, int32 model) {
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        const auto appearance = veh.GetVehicleAppearance();
        if (appearance != eVehicleAppearance::VEHICLE_APPEARANCE_AUTOMOBILE && appearance != eVehicleAppearance::VEHICLE_APPEARANCE_BIKE) {
            continue;
        }
        if (model >= 0 && static_cast<int32>(veh.GetModelIndex()) != model) {
            continue;
        }
        if (!veh.CanBeDeleted() || !veh.IsWithinArea(area.left, area.top, area.right, area.bottom)) {
            continue;
        }
        veh.vehicleFlags.bHasBeenOwnedByPlayer = true;
        return GetVehiclePool()->GetRef(&veh);
    }
    return -1;
}

/// CLEAR_CAR_LAST_DAMAGE_ENTITY(054F)
void ClearCarLastDamageEntity(CVehicle& vehicle) {
    vehicle.m_pLastDamageEntity = nullptr;
}

/// DOES_VEHICLE_EXIST(056E)
bool DoesVehicleExist(CVehicle* vehicle) {
    return vehicle != nullptr;
}

/// SET_LOAD_COLLISION_FOR_CAR_FLAG(0587)
void SetLoadCollisionForCarFlag(CRunningScript& S, CVehicle& vehicle, bool loadCollision) {
    if (loadCollision) {
        vehicle.physicalFlags.bDontLoadCollision = false;
        if (S.m_IsExternal) {
            CWorld::Remove(&vehicle);
            vehicle.m_bIsStaticWaitingForCollision = true;
            CWorld::Add(&vehicle);
        }
    } else {
        vehicle.physicalFlags.bDontLoadCollision = true;
        if (vehicle.m_bIsStaticWaitingForCollision) {
            vehicle.m_bIsStaticWaitingForCollision = false;
            if (!vehicle.GetIsStatic()) {
                vehicle.AddToMovingList();
            }
        }
    }
}

/// TASK_ENTER_CAR_AS_PASSENGER(05CA)
void TaskEnterCarAsPassenger(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle& vehicle, int32 time, int32 seat) {
    const auto door = seat == -1 ? 0 : CCarEnterExit::ComputeTargetDoorToEnterAsPassenger(&vehicle, seat);
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexEnterCarAsPassengerTimed{ &vehicle, (uint32)door, (uint32)time, true },
        (int32)command
    );
}

/// TASK_ENTER_CAR_AS_DRIVER(05CB)
void TaskEnterCarAsDriver(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle& vehicle, int32 time) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexEnterCarAsDriverTimed{ &vehicle, time },
        (int32)command
    );
}

/// TASK_LEAVE_CAR(05CD)
void TaskLeaveCar(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle& vehicle) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexLeaveCar{ &vehicle, 0, 0, true, false },
        (int32)command
    );
}

/// TASK_CAR_DRIVE_TO_COORD(05D1)
void TaskCarDriveToCoord(CRunningScript& S, eScriptCommands command, int32 pedHandle, int32 vehicleHandle, CVector posn, float speed, int32 drivingStyle, int32 carModel, int32 flag) {
    if (posn.z <= MAP_Z_LOW_LIMIT) {
        posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);
    }
    auto* const vehicle = vehicleHandle < 0 ? nullptr : GetVehiclePool()->GetAtRef(vehicleHandle);
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexDriveToPoint{
            vehicle,
            posn,
            speed,
            drivingStyle,
            carModel == 0 ? MODEL_INVALID : static_cast<eModelID>(carModel == 1 ? 415 : carModel), // retail quirk
            -1.0f,
            (eCarDrivingStyle)flag
        },
        (int32)command
    );
}

/// TASK_CAR_DRIVE_WANDER(05D2)
void TaskCarDriveWander(CRunningScript& S, eScriptCommands command, int32 pedHandle, int32 vehicleHandle, float speed, int32 drivingStyle) {
    auto* const vehicle = vehicleHandle < 0 ? nullptr : GetVehiclePool()->GetAtRef(vehicleHandle);
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexCarDriveWander{ vehicle, (eCarDrivingStyle)drivingStyle, speed },
        (int32)command
    );
}

/// START_PLAYBACK_RECORDED_CAR(05EB)
void StartPlaybackRecordedCar(CVehicle& vehicle, int32 fileNumber) {
    CVehicleRecording::StartPlaybackRecordedCar(&vehicle, fileNumber, false, false);
}

/// IS_PLAYBACK_GOING_ON_FOR_CAR(060E)
bool IsPlaybackGoingOnForCar(CVehicle& vehicle) {
    return CVehicleRecording::IsPlaybackGoingOnForCar(&vehicle);
}

/// TASK_LEAVE_CAR_IMMEDIATELY(0622)
void TaskLeaveCarImmediately(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle& vehicle) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexLeaveCar{ &vehicle, 0, 0, false, false },
        (int32)command
    );
}

/// TASK_LEAVE_ANY_CAR(0633)
void TaskLeaveAnyCar(CRunningScript& S, eScriptCommands command, int32 pedHandle) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexLeaveAnyCar{ 0, true, false },
        (int32)command
    );
}

/// OPEN_CAR_DOOR(0657)
void OpenCarDoor(CVehicle& vehicle, eDoors door) {
    const auto node = CDamageManager::GetCarNodeIndexFromDoor(door);
    // TODO(re): retail only opens the door when it isn't already open and its node isn't damaged
    vehicle.OpenDoor(nullptr, node, door, 1.0f, true);
}

/// CUSTOM_PLATE_FOR_NEXT_CAR(0674)
void CustomPlateForNextCar(notsa::script::Model model, const char* plate) {
    char plateText[9]{};
    for (uint8 i = 0; i < 8; i++) {
        plateText[i] = plate[i] == '_' || plate[i] == '\0' ? ' ' : plate[i];
    }

    const auto mi = CModelInfo::GetModelInfo(model);
    if (mi && mi->GetModelType() == MODEL_INFO_VEHICLE && mi->GetColModel()) {
        static_cast<CVehicleModelInfo*>(mi)->SetCustomCarPlateText(plateText);
    }
}

/// TASK_SHUFFLE_TO_NEXT_CAR_SEAT(0676)
void TaskShuffleToNextCarSeat(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle& vehicle) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexShuffleSeats{ &vehicle },
        (int32)command
    );
}

/// ATTACH_OBJECT_TO_CAR(0681)
void AttachObjectToCar(CObject& object, CVehicle& vehicle, CVector offset, CVector rotationDeg) {
    object.AttachEntityToEntity(
        &vehicle,
        offset,
        CVector{
            DegreesToRadians(rotationDeg.x),
            DegreesToRadians(rotationDeg.y),
            DegreesToRadians(rotationDeg.z)
        }
    );
}

/// DETACH_CAR(0684)
void DetachCar(CVehicle& vehicle, float x, float y, float z, bool applyTurnForce) {
    if (!vehicle.m_pAttachedTo) {
        return;
    }
    // TODO(re): retail also scales `y` by an unidentified constant before converting it to radians
    vehicle.DettachEntityFromEntity(DegreesToRadians(x), DegreesToRadians(y), z, applyTurnForce);
}

/// IS_VEHICLE_ATTACHED(0686)
bool IsVehicleAttached(CVehicle& vehicle) {
    return vehicle.m_pAttachedTo != nullptr;
}

/// TASK_CAR_TEMP_ACTION(06C7)
void TaskCarTempAction(CRunningScript& S, eScriptCommands command, int32 pedHandle, int32 vehicleHandle, int32 tempAction, int32 time) {
    auto* const vehicle = vehicleHandle == -1 ? nullptr : GetVehiclePool()->GetAtRef(vehicleHandle);
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskSimpleCarSetTempAction{ vehicle, (eAutoPilotTempAction)tempAction, (uint32)time },
        (int32)command
    );
}

/// TASK_CAR_MISSION(06E1)
void TaskCarMission(CRunningScript& S, eScriptCommands command, int32 pedHandle, int32 vehicleHandle, int32 targetVehicleHandle, int32 mission, float speed, int32 drivingStyle) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexCarDriveMission{
            vehicleHandle < 0 ? nullptr : GetVehiclePool()->GetAtRef(vehicleHandle),
            targetVehicleHandle < 0 ? nullptr : GetVehiclePool()->GetAtRef(targetVehicleHandle),
            (eCarMission)mission,
            (eCarDrivingStyle)drivingStyle,
            speed
        },
        (int32)command
    );
}

/// GET_AVAILABLE_VEHICLE_MOD(06E5)
int32 GetAvailableVehicleMod(const CVehicle& vehicle, int32 slot) {
    return vehicle.GetVehicleModelInfo()->m_anUpgrades[slot];
}

/// GET_VEHICLE_MOD_TYPE(06E6)
int32 GetVehicleModType(notsa::script::Model model) {
    // TODO(re): retail decodes the packed model flag word at `mi+0x12` (bits 10-14 = type, bit 8 = type group)
    // into a mod-slot index; that word has no named counterpart in this repo.
    return 0;
}

/// ADD_VEHICLE_MOD(06E7)
void AddVehicleMod(CVehicle& vehicle, notsa::script::Model model) {
    vehicle.AddVehicleUpgrade(model);
}

/// REQUEST_VEHICLE_MOD(06E9)
void RequestVehicleMod(notsa::script::Model model) {
    CStreaming::RequestVehicleUpgrade(model, 0xC);
}

/// HAS_VEHICLE_MOD_LOADED(06EA)
bool HasVehicleModLoaded(notsa::script::Model model) {
    return CStreaming::HasVehicleUpgradeLoaded(model);
}

/// MARK_VEHICLE_MOD_AS_NO_LONGER_NEEDED(06EB)
void MarkVehicleModAsNoLongerNeeded(notsa::script::Model model) {
    CStreaming::SetMissionDoesntRequireModel(model);
    if (const auto other = CVehicleModelInfo::ms_linkedUpgrades.FindOtherUpgrade(model); other != -1) {
        CStreaming::SetMissionDoesntRequireModel(other);
    }
}

/// GIVE_VEHICLE_PAINTJOB(06ED)
void GiveVehiclePaintjob(CVehicle& vehicle, int32 paintjob) {
    vehicle.SetRemap(paintjob);
}

/// DOES_CAR_HAVE_STUCK_CAR_CHECK(06FC)
bool DoesCarHaveStuckCarCheck(int32 carHandle) {
    return CTheScripts::StuckCars.IsCarInStuckCarArray(carHandle);
}

/// CAR_GOTO_COORDINATES_RACING(0704)
void CarGotoCoordinatesRacing(CVehicle& vehicle, CVector posn) {
    if (posn.z <= MAP_Z_LOW_LIMIT) {
        posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);
    }
    posn.z += vehicle.GetDistanceFromCentreOfMassToBaseOfModel();

    const auto joinedRoad = CCarCtrl::JoinCarWithRoadSystemGotoCoors(&vehicle, posn, false, false);
    if (vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        vehicle.m_autoPilot.m_nCarMission = joinedRoad
            ? eCarMission::MISSION_GOTOCOORDINATES_STRAIGHTLINE
            : eCarMission::MISSION_GOTOCOORDINATES_RACING;
    }
    vehicle.SetStatus(STATUS_PHYSICS);
    vehicle.vehicleFlags.bEngineOn = !vehicle.vehicleFlags.bEngineBroken; // 0x41BDD0(1)
    if (vehicle.m_autoPilot.m_nCruiseSpeed < 2) {
        vehicle.m_autoPilot.m_nCruiseSpeed = 1;
    }
    vehicle.m_autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS();
}

/// SKIP_IN_PLAYBACK_RECORDED_CAR(0706)
void SkipInPlaybackRecordedCar(CVehicle& vehicle, float distance) {
    CVehicleRecording::SkipForwardInRecording(&vehicle, distance);
}

/// TASK_WARP_CHAR_INTO_CAR_AS_PASSENGER(072B)
void TaskWarpCharIntoCarAsPassenger(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle& vehicle, int32 seat) {
    const auto door = seat < 0 ? 0 : CCarEnterExit::ComputeTargetDoorToEnterAsPassenger(&vehicle, seat);
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskSimpleCarSetPedInAsPassenger{ &vehicle, (eTargetDoor)door, true },
        (int32)command
    );
}

/// SET_CAR_CAN_GO_AGAINST_TRAFFIC(073B)
void SetCarCanGoAgainstTraffic(CVehicle& vehicle, bool canGoAgainstTraffic) {
    vehicle.m_autoPilot.carCtrlFlags.bCantGoAgainstTraffic = !canGoAgainstTraffic;
}

/// GET_RANDOM_CAR_IN_SPHERE_NO_SAVE(073E)
int32 GetRandomCarInSphereNoSave(CVector posn, float radius, int32 model) {
    int32 result = -1;
    float closestDist = 9999.9f;
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        const auto appearance = veh.GetVehicleAppearance();
        if (appearance != eVehicleAppearance::VEHICLE_APPEARANCE_AUTOMOBILE && appearance != eVehicleAppearance::VEHICLE_APPEARANCE_BIKE) {
            continue;
        }
        if (veh.vehicleFlags.bIsLawEnforcer) {
            continue;
        }
        if (model >= 0 && static_cast<int32>(veh.GetModelIndex()) != model) {
            continue;
        }
        if (!veh.CanBeDeleted()) {
            continue;
        }
        const auto dist = (veh.GetPosition() - posn).Magnitude();
        if (dist < radius && dist < closestDist) {
            result      = GetVehiclePool()->GetRef(&veh);
            closestDist = dist;
        }
    }
    return result;
}

/// TASK_GOTO_CAR(0772)
void TaskGotoCar(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle& vehicle, float radius, float timeout) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexSeekEntityStandard{
            &vehicle,
            static_cast<int32>(radius < 0.0f ? 1000.0f : radius),
            1000,
            timeout,
            // TODO(re): retail passes the global floats at `0x859E30`/`0x859E34` here
            0.0f,
            0.0f,
            true,
            true
        },
        (int32)command
    );
}

/// REQUEST_CAR_RECORDING(07C0)
void RequestCarRecording(int32 fileNumber) {
    CVehicleRecording::RequestRecordingFile(fileNumber);
}

/// HAS_CAR_RECORDING_BEEN_LOADED(07C1)
bool HasCarRecordingBeenLoaded(int32 fileNumber) {
    return CVehicleRecording::HasRecordingFileBeenLoaded(fileNumber);
}

/// APPLY_FORCE_TO_CAR(07D5)
void ApplyForceToCar(CVehicle& vehicle, CVector force, CVector offset) {
    // TODO(re): retail transforms the offset from the car's object space and normalises both vectors first
    vehicle.ApplyForce(force, vehicle.TransformFromObjectSpace(offset), true);
}

/// CONTROL_CAR_HYDRAULICS(07F5)
void ControlCarHydraulics(CVehicle& vehicle, float normalUpper, float normalLower, float extendedUpper, float extendedLower) {
    if (vehicle.m_vehicleSpecialColIndex < 0) {
        vehicle.GetSpecialColModel();
    }
    if (vehicle.m_vehicleSpecialColIndex < 0) {
        return;
    }
    auto& data = CVehicle::m_aSpecialHydraulicData[vehicle.m_vehicleSpecialColIndex];
    data.m_fSuspensionNormalUpperLimit   = normalUpper;
    data.m_fSuspensionNormalLowerLimit   = normalLower;
    data.m_fSuspensionExtendedUpperLimit = extendedUpper;
    data.m_fSuspensionExtendedLowerLimit = extendedLower;
}

/// DOES_CAR_HAVE_HYDRAULICS(0803)
bool DoesCarHaveHydraulics(const CVehicle& vehicle) {
    // TODO(re): retail additionally requires the pointer at `veh+0x594` to be null
    return vehicle.handlingFlags.bHydraulicGeom;
}

/// SET_CAR_ENGINE_BROKEN(081D)
void SetCarEngineBroken(CVehicle& vehicle, bool broken) {
    vehicle.vehicleFlags.bEngineBroken = broken;
    if (broken) {
        vehicle.vehicleFlags.bEngineOn = false;
    }
}

/// SET_VEHICLE_AREA_VISIBLE(0840)
void SetVehicleAreaVisible(CVehicle& vehicle, bool visible) {
    // TODO(re): retail stores the byte at `veh+0x2F`, whose member isn't identified in this repo
    vehicle.m_bIsVisible = visible;
}

/// REMOVE_CAR_RECORDING(0873)
void RemoveCarRecording(int32 fileNumber) {
    CVehicleRecording::RemoveRecordingFile(fileNumber);
}

/// SET_VEHICLE_DIRT_LEVEL(0878)
void SetVehicleDirtLevel(CVehicle& vehicle, float dirtLevel) {
    vehicle.m_fDirtLevel = dirtLevel;
}

/// SET_CHAR_CAN_BE_KNOCKED_OFF_BIKE(08C6)
void SetCharCanBeKnockedOffBike(CPed& ped, int32 cantBeKnockedOffBike) {
    ped.CantBeKnockedOffBike = static_cast<uint8>(cantBeKnockedOffBike & 3);
}

/// SET_PLANE_UNDERCARRIAGE_UP(08E6)
void SetPlaneUndercarriageUp(CPlane& plane, bool up) {
    if (up) {
        plane.SetGearUp();
    } else {
        plane.SetGearDown();
    }
}

/// GET_VEHICLE_CLASS(08EC)
int32 GetVehicleClass(const CVehicle& vehicle) {
    return vehicle.GetVehicleModelInfo()->m_nVehicleClass;
}

/// IS_CHAR_STUCK_UNDER_CAR(095D)
bool IsCharStuckUnderCar(const CPed& ped) {
    return ped.bStuckUnderCar;
}

/// IS_BIG_VEHICLE(0969)
bool IsBigVehicle(const CVehicle& vehicle) {
    return vehicle.vehicleFlags.bIsBig;
}

/// STORE_CAR_MOD_STATE(096B)
void StoreCarModState() {
    CShopping::StoreVehicleMods();
}

/// RESTORE_CAR_MOD_STATE(096C)
void RestoreCarModState() {
    CShopping::RestoreVehicleMods();
}

/// GET_CURRENT_CAR_MOD(096D)
int32 GetCurrentCarMod(CVehicle& vehicle, int32 slot) {
    switch (slot) {
    case 0:  return vehicle.GetUpgrade(0);
    case 1: {
        const auto upgrade = vehicle.GetUpgrade(1);
        return upgrade == -1 ? vehicle.GetUpgrade(2) : upgrade;
    }
    case 2:  return vehicle.GetUpgrade(6);
    case 3: {
        const auto upgrade = vehicle.GetUpgrade(8);
        return upgrade == -1 ? vehicle.GetUpgrade(9) : upgrade;
    }
    case 4:  return vehicle.GetUpgrade(10);
    case 5:  return vehicle.GetUpgrade(0xB);
    case 6:  return vehicle.GetUpgrade(0xC);
    case 7:  return vehicle.GetUpgrade(0xE);
    case 8:  return vehicle.GetUpgrade(0xF);
    case 9:  return vehicle.GetUpgrade(0x10);
    case 10: return vehicle.GetUpgrade(0x11);
    case 12: return vehicle.GetReplacementUpgrade(2);
    case 13: return vehicle.GetReplacementUpgrade(0x13);
    case 14: return vehicle.GetReplacementUpgrade(0xC);
    case 15: return vehicle.GetReplacementUpgrade(0xD);
    case 16: return vehicle.GetReplacementUpgrade(0x14);
    default: return -1;
    }
}

/// IS_CAR_LOW_RIDER(096E)
bool IsCarLowRider(const CVehicle& vehicle) {
    return vehicle.handlingFlags.bLowRider;
}

/// IS_CAR_STREET_RACER(096F)
bool IsCarStreetRacer(const CVehicle& vehicle) {
    return vehicle.handlingFlags.bStreetRacer;
}

/// IS_EMERGENCY_SERVICES_VEHICLE(0975)
bool IsEmergencyServicesVehicle(CVehicle& vehicle) {
    if (vehicle.IsLawEnforcementVehicle()) {
        return true;
    }
    return vehicle.GetModelId() == MODEL_FIRETRUK
        || vehicle.GetModelId() == MODEL_AMBULAN
        || vehicle.GetModelId() == MODEL_FIRELA;
}

/// GET_NUM_CAR_COLOURS(097D)
int32 GetNumCarColours(const CVehicle& vehicle) {
    // TODO(re): retail reads the byte at `mi+0x2D0`, which has no named counterpart in this repo
    return 0;
}

/// HAS_TRAIN_DERAILED(0981)
bool HasTrainDerailed(CVehicle& vehicle) {
    return vehicle.IsTrain() && vehicle.AsTrain()->trainFlags.bNotOnARailRoad;
}

/// GET_CAR_BLOCKING_CAR(0987)
CVehicle* GetCarBlockingCar(const CVehicle& vehicle) {
    // TODO(re): retail reads the entity at `veh+0x420` (last obstructing entity); the closest named member is the ignored collision entity
    auto* const blocking = vehicle.m_pEntityIgnoredCollision;
    return blocking && blocking->GetIsTypeVehicle() ? static_cast<CVehicle*>(blocking) : nullptr;
}

/// GET_CURRENT_VEHICLE_PAINTJOB(0988)
int32 GetCurrentVehiclePaintjob(CVehicle& vehicle) {
    return vehicle.GetRemapIndex();
}

/// IS_CHAR_IN_ANY_TRAIN(09AE)
bool IsCharInAnyTrain(CPed& ped) {
    return ped.bInVehicle && ped.m_pVehicle && ped.m_pVehicle->IsTrain();
}

/// GET_CAR_DOOR_LOCK_STATUS(09B3)
int32 GetCarDoorLockStatus(const CVehicle& vehicle) {
    return static_cast<int32>(vehicle.m_nDoorLock);
}

/// IS_COP_VEHICLE_IN_AREA_3D_NO_SAVE(09C3)
bool IsCopVehicleInArea3DNoSave(CVector cornerA, CVector cornerB) {
    const auto xMin = std::min(cornerA.x, cornerB.x), xMax = std::max(cornerA.x, cornerB.x);
    const auto yMin = std::min(cornerA.y, cornerB.y), yMax = std::max(cornerA.y, cornerB.y);
    const auto zMin = std::min(cornerA.z, cornerB.z), zMax = std::max(cornerA.z, cornerB.z);
    for (auto& veh : GetVehiclePool()->GetAllValid()) {
        if (!veh.IsLawEnforcementVehicle() || veh.GetModelIndex() == MODEL_PREDATOR) {
            continue;
        }
        const auto pos = veh.GetPosition();
        if (pos.x >= xMin && pos.x < xMax
         && pos.y >= yMin && pos.y < yMax
         && pos.z >= zMin && pos.z < zMax
        ) {
            return true;
        }
    }
    return false;
}

/// GET_CAR_MODEL_VALUE(09E1)
float GetCarModelValue(notsa::script::Model model) {
    // TODO(re): retail reads the float at `0xC2BAB4 + handlingId * 0xE0`, whose table isn't identified in this repo
    return 0.0f;
}

/// CREATE_CAR_GENERATOR_WITH_PLATE(09E2)
int32 CreateCarGeneratorWithPlate(CVector posn, float angle, int32 modelId, int16 color1, int16 color2, uint8 forceSpawn, uint8 alarmChances, uint8 doorLockChances, uint16 minDelay, uint16 maxDelay, const char* plate) {
    if (posn.z > MAP_Z_LOW_LIMIT) {
        posn.z += 0.015f; // 0x859F00
    }
    const auto index = CTheCarGenerators::CreateCarGenerator(posn, angle, modelId, color1, color2, forceSpawn, alarmChances, doorLockChances, minDelay, maxDelay, 0, true);

    char plateText[9]{};
    for (uint8 i = 0; i < 8; i++) {
        plateText[i] = plate[i] == '_' || plate[i] == '\0' ? ' ' : plate[i];
    }
    CTheCarGenerators::m_SpecialPlateHandler.Add(index, plateText);
    return index;
}

/// RESET_VEHICLE_HYDRAULICS(09FE)
void ResetVehicleHydraulics(CVehicle& vehicle) {
    // TODO(re): retail additionally requires the pointer at `veh+0x590` to be null
    if (vehicle.handlingFlags.bHydraulicGeom && vehicle.IsAutomobile()) {
        vehicle.AsAutomobile()->m_wMiscComponentAngle = 0;
    }
}

/// CHANGE_CAR_COLOUR_FROM_MENU(0A22)
void ChangeCarColourFromMenu(int32 menuId, CVehicle& vehicle, int32 primary, int32 colourId) {
    const auto colour = CMenuSystem::GetCarColourFromGrid(static_cast<MenuId>(menuId), static_cast<uint8>(colourId));
    if (primary == 1) {
        vehicle.m_nPrimaryColor = colour;
    } else {
        vehicle.m_nSecondaryColor = colour;
    }
}

/// FIX_CAR(0A30)
void FixCar(CVehicle& vehicle) {
    vehicle.Fix();
    vehicle.m_fHealth = 1000.0f;
}

//
// Car AI / coordinates
//

/// CAR_GOTO_COORDINATES(00A7), CAR_GOTO_COORDINATES_ACCURATE(02C2)
void SetCarGotoCoordinates(CVehicle& vehicle, CVector posn, eCarMission missionOnRoad, eCarMission missionStraightLine) {
    if (posn.z <= MAP_Z_LOW_LIMIT) {
        posn.z = CWorld::FindGroundZForCoord(posn.x, posn.y);
    }
    posn.z += vehicle.GetDistanceFromCentreOfMassToBaseOfModel();

    const auto joinedRoad = CCarCtrl::JoinCarWithRoadSystemGotoCoors(&vehicle, posn, false, false);
    if (vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        vehicle.m_autoPilot.m_nCarMission = joinedRoad ? missionStraightLine : missionOnRoad;
    }
    vehicle.SetStatus(STATUS_PHYSICS); // `veh+0x36`: status bits := STATUS_PHYSICS (3)
    vehicle.vehicleFlags.bEngineOn = !vehicle.vehicleFlags.bEngineBroken; // 0x41BDD0(1)
    if (vehicle.m_autoPilot.m_nCruiseSpeed < 2) {
        vehicle.m_autoPilot.m_nCruiseSpeed = 1;
    }
    vehicle.m_autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS();
}

/// CAR_GOTO_COORDINATES(00A7)
void CarGotoCoordinates(CVehicle& vehicle, CVector posn) {
    SetCarGotoCoordinates(vehicle, posn, eCarMission::MISSION_GOTOCOORDINATES, eCarMission::MISSION_GOTOCOORDINATES_STRAIGHTLINE);
}

/// CAR_GOTO_COORDINATES_ACCURATE(02C2)
void CarGotoCoordinatesAccurate(CVehicle& vehicle, CVector posn) {
    SetCarGotoCoordinates(vehicle, posn, eCarMission::MISSION_GOTOCOORDINATES_ACCURATE, eCarMission::MISSION_GOTOCOORDINATES_STRAIGHTLINE_ACCURATE);
}

/// CAR_WANDER_RANDOMLY(00A8)
void CarWanderRandomly(CVehicle& vehicle) {
    CCarCtrl::JoinCarWithRoadSystem(&vehicle);
    if (vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_PLANE_CRASH_AND_BURN && vehicle.m_autoPilot.m_nCarMission != eCarMission::MISSION_HELI_CRASH_AND_BURN) {
        vehicle.m_autoPilot.m_nCarMission = eCarMission::MISSION_CRUISE;
    }
    vehicle.vehicleFlags.bEngineOn = !vehicle.vehicleFlags.bEngineBroken; // 0x41BDD0(1)
    if (vehicle.m_autoPilot.m_nCruiseSpeed < 2) {
        vehicle.m_autoPilot.m_nCruiseSpeed = 1;
    }
    vehicle.m_autoPilot.m_nTimeToStartMission = CTimer::GetTimeInMS();
}

/// IS_CAR_IN_AREA_3D(00B1), IS_CAR_STOPPED_IN_AREA_2D(01AB), IS_CAR_STOPPED_IN_AREA_3D(01AC)
OpcodeResult IsCarInArea(CRunningScript& S, eScriptCommands command) {
    S.CarInAreaCheckCommand((int32)command);
    return OR_CONTINUE;
}

/// IS_CAR_HEALTH_GREATER(0185)
bool IsCarHealthGreater(const CVehicle& vehicle, int32 health) {
    return (float)health < vehicle.m_fHealth; // `veh+0x4C0`
}

/// IS_CAR_STUCK_ON_ROOF(018F)
bool IsCarStuckOnRoof(int32 carHandle) {
    return CUpsideDownCarCheck{}.HasCarBeenUpsideDownForAWhile(carHandle);
}

/// SET_CAR_HEAVY(01EC)
void SetCarHeavy(CVehicle& vehicle, bool heavy) {
    vehicle.physicalFlags.bMakeMassTwiceAsBig = heavy;
    if (heavy) {
        // 0x858B3C == 3.0f, 0x858C80 == 5.0f
        vehicle.m_fMass            = vehicle.m_pHandlingData->m_fMass * 3.0f;
        vehicle.m_fTurnMass        = vehicle.m_pHandlingData->m_fTurnMass * 5.0f;
        vehicle.m_fBuoyancyConstant = vehicle.m_pHandlingData->m_fBuoyancyConstant * 2.0f;
    } else {
        vehicle.m_fMass             = vehicle.m_pHandlingData->m_fMass;
        vehicle.m_fTurnMass         = vehicle.m_pHandlingData->m_fTurnMass;
        vehicle.m_fBuoyancyConstant = vehicle.m_pHandlingData->m_fBuoyancyConstant;
    }
}

/// GET_CAR_FORWARD_X(02F8), GET_CAR_FORWARD_Y(02F9)
CVector GetCarForward2DNormalised(const CVehicle& vehicle) {
    const auto fwd = vehicle.GetForward(); // `veh+0x14`->matrix.up (RW `Up`)
    const auto len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y);
    return { fwd.x / len, fwd.y / len, fwd.z };
}

/// GET_CAR_FORWARD_X(02F8)
float GetCarForwardX(const CVehicle& vehicle) {
    return GetCarForward2DNormalised(vehicle).x;
}

/// GET_CAR_FORWARD_Y(02F9)
float GetCarForwardY(const CVehicle& vehicle) {
    return GetCarForward2DNormalised(vehicle).y;
}

/// SET_CAR_VISIBLE(0338)
void SetCarVisible(CVehicle& vehicle, bool visible) {
    vehicle.SetIsVisible(visible); // `veh+0x1C` bit 7
}

/// SET_CAR_WATERTIGHT(039C)
void SetCarWatertight(CVehicle& vehicle, bool watertight) {
    if (vehicle.m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
        vehicle.AsAutomobile()->autoFlags.bWaterTight = watertight; // `veh+0x868` bit 2
    } else if (vehicle.m_nVehicleType == VEHICLE_TYPE_BIKE) {
        vehicle.AsBike()->bikeFlags.bWaterTight = watertight; // `veh+0x614` bit 2
    }
}

/// SET_CAR_STATUS(03A2)
void SetCarStatus(CVehicle& vehicle, eEntityStatus status) {
    if (vehicle.GetStatus() == STATUS_SIMPLE && status != STATUS_SIMPLE) {
        CCarCtrl::SwitchVehicleToRealPhysics(&vehicle); // 0x423FC0
    }
    vehicle.SetStatus(status); // `veh+0x36`: status bits := status
}

/// SET_CAR_STRONG(03AB)
void SetCarStrong(CVehicle& vehicle, bool strong) {
    vehicle.vehicleFlags.bTakeLessDamage = strong; // `veh+0x429` bit 7
}

/// IS_CAR_VISIBLY_DAMAGED(03C9)
bool IsCarVisiblyDamaged(const CVehicle& vehicle) {
    return vehicle.vehicleFlags.bIsDamaged; // `veh+0x42A` bit 0
}

/// SET_CAR_CAN_BE_DAMAGED(03F5)
void SetCarCanBeDamaged(CVehicle& vehicle, bool canBeDamaged) {
    vehicle.vehicleFlags.bCanBeDamaged = canBeDamaged; // `veh+0x42A` bit 5
    if (!canBeDamaged) {
        vehicle.ExtinguishCarFire();
    }
}

/// SET_CAR_TRACTION(0423)
void SetCarTraction(CVehicle& vehicle, float traction) {
    if (vehicle.m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
        vehicle.AsAutomobile()->m_fCarTraction = traction; // `veh+0x8A0`
    } else {
        vehicle.AsBike()->m_fExtraTractionMult = traction; // `veh+0x794`
    }
}

/// SET_CAR_AVOID_LEVEL_TRANSITIONS(0428)
void SetCarAvoidLevelTransitions(CVehicle& vehicle, bool avoid) {
    vehicle.m_autoPilot.carCtrlFlags.bAvoidLevelTransitions = avoid;
}

/// SET_CAR_STAY_IN_FAST_LANE(0466)
void SetCarStayInFastLane(CVehicle& vehicle, bool stay) {
    vehicle.m_autoPilot.carCtrlFlags.bStayInFastLane = stay;
}

/// SET_CAR_STAY_IN_SLOW_LANE(0714)
void SetCarStayInSlowLane(CVehicle& vehicle, bool stay) {
    vehicle.m_autoPilot.carCtrlFlags.bStayInSlowLane = stay;
}

/// SET_CAR_TEMP_ACTION(0477)
void SetCarTempAction(CVehicle& vehicle, eAutoPilotTempAction action, int32 time) {
    vehicle.m_autoPilot.m_nTempAction     = action;                      // `veh+0x3BB`
    vehicle.m_autoPilot.m_nTempActionTime = CTimer::GetTimeInMS() + time; // `veh+0x3BC`
}

/// SET_CAR_RANDOM_ROUTE_SEED(048B)
void SetCarRandomRouteSeed(CVehicle& vehicle, int32 seed) {
    vehicle.m_nForcedRandomRouteSeed = static_cast<int16>(seed); // `veh+0x45E`
}

/// IS_CAR_TYRE_BURST(0496)
bool IsCarTireBurst(CVehicle& vehicle, int32 wheel) {
    // 4 == all wheels
    if (vehicle.m_nVehicleType == VEHICLE_TYPE_BIKE) {
        auto& bike = *vehicle.AsBike(); // `veh+0x65C` = `m_nWheelStatus[2]`
        if (wheel == 4) {
            return bike.m_nWheelStatus[0] == WHEEL_STATUS_BURST || bike.m_nWheelStatus[1] == WHEEL_STATUS_BURST;
        }
        if (wheel == 2) {
            wheel = 0;
        } else if (wheel == 3) {
            wheel = 1;
        }
        return bike.m_nWheelStatus[wheel] == WHEEL_STATUS_BURST;
    }
    // TODO(re): retail blindly reads the damage manager at `veh+0x5A0`, which only exists on
    // CAutomobile-derived classes; boats/trains/bike-derived vehicles are rejected here instead.
    switch (vehicle.m_nVehicleType) {
    case VEHICLE_TYPE_AUTOMOBILE:
    case VEHICLE_TYPE_MTRUCK:
    case VEHICLE_TYPE_QUAD:
    case VEHICLE_TYPE_HELI:
    case VEHICLE_TYPE_PLANE:
    case VEHICLE_TYPE_FHELI:
    case VEHICLE_TYPE_FPLANE:
    case VEHICLE_TYPE_TRAILER:
        break;
    default:
        return false;
    }
    auto& damageManager = vehicle.AsAutomobile()->GetDamageManager();
    if (wheel == 4) {
        for (auto w = 0; w < MAX_CARWHEELS; w++) {
            if (damageManager.GetWheelStatus((eCarWheel)w) == WHEEL_STATUS_BURST) {
                return true;
            }
        }
        return false;
    }
    return damageManager.GetWheelStatus((eCarWheel)wheel) == WHEEL_STATUS_BURST;
}

/// IS_CAR_WAITING_FOR_WORLD_COLLISION(04F1)
bool IsCarWaitingForWorldCollision(const CVehicle& vehicle) {
    return vehicle.m_bIsStaticWaitingForCollision; // `veh+0x1C` bit 18
}

/// GET_CAR_SPEED_VECTOR(06A2)
CVector GetCarSpeedVector(const CVehicle& vehicle) {
    return vehicle.m_vecMoveSpeed * 50.0f; // 0x858B40
}

/// GET_CAR_ROLL(06BE)
float GetCarRoll(CVehicle& vehicle) {
    return vehicle.AsAutomobile()->GetCarRoll();
}

/// GET_CAR_PITCH(077D)
float GetCarPitch(CVehicle& vehicle) {
    // `CAutomobile::GetCarPitch()` result converted to degrees and normalised into [0, 360)
    auto pitch = RadiansToDegrees(vehicle.AsAutomobile()->GetCarPitch());
    if (pitch < 0.0f) { // 0x858B50
        pitch += 360.0f; // 0x859E2C
    }
    if (pitch > 360.0f) {
        pitch -= 360.0f;
    }
    return pitch;
}

/// SET_CAR_ROTATION_VELOCITY(07DB)
void SetCarRotationVelocity(CVehicle& vehicle, CVector rotationVelocity) {
    // The velocity is scaled by the frame time (0xB7CB5C) and then transformed into world space
    const CVector turnSpeed = vehicle.GetMatrix().TransformVector(rotationVelocity * CTimer::GetTimeStep());
    if (vehicle.m_bIsStatic) { // `veh+0x1C` bit 2
        // TODO(re): retail also invokes the entity's 3rd vtable method with `false` here; its semantics
        // (0x472652) aren't established, only the following `CPhysical::AddToMovingList` call is.
        vehicle.AddToMovingList();
    }
    vehicle.m_vecTurnSpeed = turnSpeed * 0.02f; // 1/50, 0x858B38 (`CPhysical::SetTurnSpeed`, 0x45AFB0)
}

/// SET_CAR_FOLLOW_CAR(07F8)
void SetCarFollowCar(CVehicle& vehicle, CVehicle* target, float radius) {
    CCarAI::TellCarToFollowOtherCar(&vehicle, target, radius);
}

/// SET_CAR_HYDRAULICS(07FF)
void SetCarHydraulics(CVehicle& vehicle, bool state) {
    if (state) {
        vehicle.AddVehicleUpgrade(ModelIndices::MI_HYDRAULICS); // 0x6E3290
    } else {
        vehicle.RemoveVehicleUpgrade(ModelIndices::MI_HYDRAULICS); // 0x6DF930
    }
}

/// GET_CAR_UPRIGHT_VALUE(083F)
float GetCarUprightValue(const CVehicle& vehicle) {
    return vehicle.GetUp().z; // `veh+0x14`->matrix.at (RW `At`) .z
}

/// SET_CAR_CAN_BE_VISIBLY_DAMAGED(0852)
void SetCarCanBeVisiblyDamaged(CVehicle& vehicle, bool canBeDamaged) {
    // TODO(re): retail writes `veh+0x868` (the automobile flag) regardless of the vehicle type;
    // the write is only meaningful for automobile-derived vehicles, so it's guarded here.
    if (vehicle.IsAutomobile()) {
        vehicle.AsAutomobile()->autoFlags.bCanBeVisiblyDamaged = canBeDamaged;
    }
}

/// SET_CAR_COORDINATES_NO_OFFSET(088C)
void SetCarCoordinatesNoOffset(CVehicle& vehicle, CVector posn) {
    CCarCtrl::SetCoordsOfScriptCar(&vehicle, posn.x, posn.y, posn.z, 0, 0);
}

/// SET_CAR_ENGINE_ON(0918)
void SetCarEngineOn(CVehicle& vehicle, bool on) {
    // 0x41BDD0: a broken engine can never be turned on
    if (vehicle.vehicleFlags.bEngineBroken) {
        vehicle.vehicleFlags.bEngineOn = false;
    } else {
        vehicle.vehicleFlags.bEngineOn = on;
    }
}

/// SET_CAR_LIGHTS_ON(0919)
void SetCarLightsOn(CVehicle& vehicle, bool on) {
    vehicle.vehicleFlags.bLightsOn = on; // `veh+0x428` bit 6
}

/// GET_CAR_MOVING_COMPONENT_OFFSET(098D)
float GetCarMovingComponentOffset(CVehicle& vehicle) {
    return vehicle.AsAutomobile()->GetMovingCollisionOffset();
}

/// SET_CAR_COLLISION(099A)
void SetCarCollision(CVehicle& vehicle, bool collision) {
    vehicle.m_bUsesCollision          = collision; // `veh+0x1C` bit 0
    vehicle.physicalFlags.bApplyGravity = collision; // `veh+0x40` bit 1
}

/// IS_CAR_DOOR_DAMAGED(09BB)
bool IsCarDoorDamaged(CVehicle& vehicle, eDoors door) {
    return vehicle.AsAutomobile()->GetDamageManager().GetDoorStatus(door) != 0;
}

//
// Vehicle tasks
//

/// TASK_LEAVE_CAR_AND_FLEE(05CF)
void TaskLeaveCarAndFlee(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle* vehicle, CVector fleePoint) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexLeaveCarAndFlee{ vehicle, fleePoint, TARGET_DOOR_FRONT_LEFT, 0, false },
        (int32)command
    );
}

/// TASK_DESTROY_CAR(0672)
void TaskDestroyCar(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle* vehicle) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexDestroyCar{ vehicle, 0, 0, 0 },
        (int32)command
    );
}

/// TASK_EVERYONE_LEAVE_CAR(068B)
void TaskEveryoneLeaveCar(CRunningScript& S, eScriptCommands command, CVehicle& vehicle) {
    if (vehicle.m_pDriver) {
        S.GivePedScriptedTask(
            GetPedPool()->GetRef(vehicle.m_pDriver),
            new CTaskComplexLeaveAnyCar{ 0, true, false },
            (int32)command
        );
    }
    for (auto i = 0; i < (int32)vehicle.m_nMaxPassengers; i++) {
        auto* const passenger = vehicle.m_apPassengers[i];
        if (!passenger) {
            continue;
        }
        // Retail staggers the passengers: `random(-250, 250) + 500 + i * 500` ms
        const auto delay = CGeneral::GetRandomNumberInRange<int32>(-250, 250) + 500 + i * 500;
        S.GivePedScriptedTask(
            GetPedPool()->GetRef(passenger),
            new CTaskComplexLeaveAnyCar{ delay, true, false },
            (int32)command
        );
    }
}

/// TASK_DRIVE_POINT_ROUTE(06BB)
void TaskDrivePointRoute(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle* vehicle, float speed) {
    // TODO(re): retail uses the script route held in the global at `0xC18D50` (filled by EXTEND_ROUTE/FLUSH_ROUTE,
    // which live in GameCommands.cpp as a file-static `s_ScriptRoute`), so this empty route is a placeholder.
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexDrivePointRoute{ vehicle, CPointRoute{}, speed, 2, MODEL_INVALID, -1.0f, DRIVING_STYLE_STOP_FOR_CARS },
        (int32)command
    );
}

/// TASK_DRIVE_POINT_ROUTE_ADVANCED(07E7)
void TaskDrivePointRouteAdvanced(CRunningScript& S, eScriptCommands command, int32 pedHandle, CVehicle* vehicle, float speed, int32 mode, int32 carModel, int32 drivingStyle) {
    // TODO(re): see TASK_DRIVE_POINT_ROUTE(06BB) - the shared script route (`0xC18D50`) isn't reachable from here.
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexDrivePointRoute{
            vehicle,
            CPointRoute{},
            speed,
            (uint32)mode,
            carModel == 0 ? MODEL_INVALID : static_cast<eModelID>(carModel == 1 ? 415 : carModel), // retail quirk
            -1.0f,
            (eCarDrivingStyle)drivingStyle
        },
        (int32)command
    );
}

}

void notsa::script::commands::vehicle::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Vehicle");

    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_HELI_ORIENTATION, ClearHeliOrientation);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_RC_BUGGY, RemoveRCBuggy);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_PROOFS, SetCarProofs);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_CAR_GENERATOR, SwitchCarGenerator);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_HAS_BEEN_OWNED_FOR_CAR_GENERATOR, SetHasBeenOwnedForCarGenerator);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_SPEED, GetCarSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_DRIVING_STYLE, SetCarDrivingStyle);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_FIRST_CAR_COLOUR, IsFirstCarColor);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_SECOND_CAR_COLOUR, IsSecondCarColor);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_EXTRA_CAR_COLOURS, GetExtraCarColors);
    REGISTER_COMMAND_HANDLER(COMMAND_POP_CAR_BOOT_USING_PHYSICS, PopCarBootUsingPhysics);
    REGISTER_COMMAND_HANDLER(COMMAND_SKIP_TO_NEXT_ALLOWED_STATION, SkipToNextAllowedStation);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_RAILTRACK_RESISTANCE_MULT, SetRailTrackResistanceMult);
    REGISTER_COMMAND_HANDLER(COMMAND_DISABLE_HELI_AUDIO, DisableHeliAudio);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_IN_AIR_PROPER, IsCarInAirProper);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_STUCK, IsCarStuck);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_STUCK_CAR_CHECK, AddStuckCarCheck);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_STUCK_CAR_CHECK, RemoveStuckCarCheck);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_STUCK_CAR_CHECK_WITH_WARP, AddStuckCarCheckWithWarp);
    REGISTER_COMMAND_HANDLER(COMMAND_PLANE_ATTACK_PLAYER_USING_DOG_FIGHT, PlaneAttackPlayerUsingDogFight);

    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_ALWAYS_CREATE_SKIDS, SetCarAlwaysCreateSkids);

    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_CAR, CreateCar);
    REGISTER_COMMAND_HANDLER(COMMAND_DELETE_CAR, DeleteCar);
    REGISTER_COMMAND_HANDLER(COMMAND_CAR_SET_IDLE, CarSetIdle);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_COORDINATES, GetCarCoordinates);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_COORDINATES, SetCarCoordinates);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_CRUISE_SPEED, SetCarCruiseSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_MISSION, SetCarMission);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_IN_AREA_2D, IsCarInArea2D);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_DEAD, IsCarDead);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_MODEL, IsCarModel);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_CAR_GENERATOR, CreateCarGenerator);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_HEADING, GetCarHeading);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_HEADING, SetCarHeading);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_BLIP_FOR_CAR, AddBlipForCar);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCATE_CAR_2D, LocateCar);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCATE_CAR_3D, LocateCar);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCATE_STOPPED_CAR_3D, LocateCar);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_STOPPED, IsCarStopped);
    REGISTER_COMMAND_HANDLER(COMMAND_MARK_CAR_AS_NO_LONGER_NEEDED, MarkCarAsNoLongerNeeded);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_DENSITY_MULTIPLIER, SetCarDensityMultiplier);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_UPSIDEDOWN, IsCarUpsideDown);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCK_CAR_DOORS, LockCarDoors);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_UPRIGHT, IsCarUpright);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_HEALTH, SetCarHealth);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_HEALTH, GetCarHealth);
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_CAR_COLOUR, ChangeCarColour);
    REGISTER_COMMAND_HANDLER(COMMAND_TURN_CAR_TO_FACE_COORD, TurnCarToFaceCoord);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_AREA_OF_CARS, ClearAreaOfCars);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_RANDOM_CAR_FOR_CAR_PARK, CreateRandomCarForCarPark);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CLOSEST_CAR_NODE_WITH_HEADING, GetClosestCarNodeWithHeading);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_COLOURS, GetCarColours);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_ALL_CARS_CAN_BE_DAMAGED, SetAllCarsCanBeDamaged);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OFFSET_FROM_CAR_IN_WORLD_COORDS, GetOffsetFromCarInWorldCoords);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_PASSENGER_SEAT_FREE, IsCarPassengerSeatFree);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_MODEL, GetCarModel);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_DRIVER_OF_CAR, GetDriverOfCar);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_ON_FIRE, IsCarOnFire);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_NTH_CLOSEST_CAR_NODE, GetNthClosestCarNode);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_STRAIGHT_LINE_DISTANCE, SetCarStraightLineDistance);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_ONLY_DAMAGED_BY_PLAYER, SetCarOnlyDamagedByPlayer);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_IN_WATER, IsCarInWater);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_ON_SCREEN, IsCarOnScreen);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_MODEL_COMPONENTS, SetCarModelComponents);
    REGISTER_COMMAND_HANDLER(COMMAND_SORT_OUT_OBJECT_COLLISION_WITH_CAR, SortOutObjectCollisionWithCar);
    REGISTER_COMMAND_HANDLER(COMMAND_FREEZE_CAR_POSITION, FreezeCarPosition);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_CAR_BEEN_DAMAGED_BY_CAR, HasCarBeenDamagedByCar);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_RANDOM_CAR_OF_TYPE_IN_AREA_NO_SAVE, GetRandomCarOfTypeInAreaNoSave);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_CAR_LAST_DAMAGE_ENTITY, ClearCarLastDamageEntity);
    REGISTER_COMMAND_HANDLER(COMMAND_DOES_VEHICLE_EXIST, DoesVehicleExist);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_LOAD_COLLISION_FOR_CAR_FLAG, SetLoadCollisionForCarFlag);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_ENTER_CAR_AS_PASSENGER, TaskEnterCarAsPassenger);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_ENTER_CAR_AS_DRIVER, TaskEnterCarAsDriver);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_LEAVE_CAR, TaskLeaveCar);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_CAR_DRIVE_TO_COORD, TaskCarDriveToCoord);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_CAR_DRIVE_WANDER, TaskCarDriveWander);
    REGISTER_COMMAND_HANDLER(COMMAND_START_PLAYBACK_RECORDED_CAR, StartPlaybackRecordedCar);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_PLAYBACK_GOING_ON_FOR_CAR, IsPlaybackGoingOnForCar);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_LEAVE_CAR_IMMEDIATELY, TaskLeaveCarImmediately);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_LEAVE_ANY_CAR, TaskLeaveAnyCar);
    REGISTER_COMMAND_HANDLER(COMMAND_OPEN_CAR_DOOR, OpenCarDoor);
    REGISTER_COMMAND_HANDLER(COMMAND_CUSTOM_PLATE_FOR_NEXT_CAR, CustomPlateForNextCar);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_SHUFFLE_TO_NEXT_CAR_SEAT, TaskShuffleToNextCarSeat);
    REGISTER_COMMAND_HANDLER(COMMAND_ATTACH_OBJECT_TO_CAR, AttachObjectToCar);
    REGISTER_COMMAND_HANDLER(COMMAND_DETACH_CAR, DetachCar);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_VEHICLE_ATTACHED, IsVehicleAttached);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_CAR_TEMP_ACTION, TaskCarTempAction);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_CAR_MISSION, TaskCarMission);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_AVAILABLE_VEHICLE_MOD, GetAvailableVehicleMod);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_VEHICLE_MOD_TYPE, GetVehicleModType);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_VEHICLE_MOD, AddVehicleMod);
    REGISTER_COMMAND_HANDLER(COMMAND_REQUEST_VEHICLE_MOD, RequestVehicleMod);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_VEHICLE_MOD_LOADED, HasVehicleModLoaded);
    REGISTER_COMMAND_HANDLER(COMMAND_MARK_VEHICLE_MOD_AS_NO_LONGER_NEEDED, MarkVehicleModAsNoLongerNeeded);
    REGISTER_COMMAND_HANDLER(COMMAND_GIVE_VEHICLE_PAINTJOB, GiveVehiclePaintjob);
    REGISTER_COMMAND_HANDLER(COMMAND_DOES_CAR_HAVE_STUCK_CAR_CHECK, DoesCarHaveStuckCarCheck);
    REGISTER_COMMAND_HANDLER(COMMAND_CAR_GOTO_COORDINATES_RACING, CarGotoCoordinatesRacing);
    REGISTER_COMMAND_HANDLER(COMMAND_SKIP_IN_PLAYBACK_RECORDED_CAR, SkipInPlaybackRecordedCar);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_WARP_CHAR_INTO_CAR_AS_PASSENGER, TaskWarpCharIntoCarAsPassenger);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_CAN_GO_AGAINST_TRAFFIC, SetCarCanGoAgainstTraffic);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_RANDOM_CAR_IN_SPHERE_NO_SAVE, GetRandomCarInSphereNoSave);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_GOTO_CAR, TaskGotoCar);
    REGISTER_COMMAND_HANDLER(COMMAND_REQUEST_CAR_RECORDING, RequestCarRecording);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_CAR_RECORDING_BEEN_LOADED, HasCarRecordingBeenLoaded);
    REGISTER_COMMAND_HANDLER(COMMAND_APPLY_FORCE_TO_CAR, ApplyForceToCar);
    REGISTER_COMMAND_HANDLER(COMMAND_CONTROL_CAR_HYDRAULICS, ControlCarHydraulics);
    REGISTER_COMMAND_HANDLER(COMMAND_DOES_CAR_HAVE_HYDRAULICS, DoesCarHaveHydraulics);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_ENGINE_BROKEN, SetCarEngineBroken);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_VEHICLE_AREA_VISIBLE, SetVehicleAreaVisible);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_CAR_RECORDING, RemoveCarRecording);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_VEHICLE_DIRT_LEVEL, SetVehicleDirtLevel);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CHAR_CAN_BE_KNOCKED_OFF_BIKE, SetCharCanBeKnockedOffBike);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_PLANE_UNDERCARRIAGE_UP, SetPlaneUndercarriageUp);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_VEHICLE_CLASS, GetVehicleClass);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CHAR_STUCK_UNDER_CAR, IsCharStuckUnderCar);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_BIG_VEHICLE, IsBigVehicle);
    REGISTER_COMMAND_HANDLER(COMMAND_STORE_CAR_MOD_STATE, StoreCarModState);
    REGISTER_COMMAND_HANDLER(COMMAND_RESTORE_CAR_MOD_STATE, RestoreCarModState);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CURRENT_CAR_MOD, GetCurrentCarMod);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_LOW_RIDER, IsCarLowRider);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_STREET_RACER, IsCarStreetRacer);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_EMERGENCY_SERVICES_VEHICLE, IsEmergencyServicesVehicle);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_NUM_CAR_COLOURS, GetNumCarColours);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_TRAIN_DERAILED, HasTrainDerailed);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_BLOCKING_CAR, GetCarBlockingCar);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CURRENT_VEHICLE_PAINTJOB, GetCurrentVehiclePaintjob);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CHAR_IN_ANY_TRAIN, IsCharInAnyTrain);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_DOOR_LOCK_STATUS, GetCarDoorLockStatus);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_COP_VEHICLE_IN_AREA_3D_NO_SAVE, IsCopVehicleInArea3DNoSave);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_MODEL_VALUE, GetCarModelValue);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_CAR_GENERATOR_WITH_PLATE, CreateCarGeneratorWithPlate);
    REGISTER_COMMAND_HANDLER(COMMAND_RESET_VEHICLE_HYDRAULICS, ResetVehicleHydraulics);
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_CAR_COLOUR_FROM_MENU, ChangeCarColourFromMenu);
    REGISTER_COMMAND_HANDLER(COMMAND_FIX_CAR, FixCar);

    REGISTER_COMMAND_HANDLER(COMMAND_CAR_GOTO_COORDINATES, CarGotoCoordinates);
    REGISTER_COMMAND_HANDLER(COMMAND_CAR_GOTO_COORDINATES_ACCURATE, CarGotoCoordinatesAccurate);
    REGISTER_COMMAND_HANDLER(COMMAND_CAR_WANDER_RANDOMLY, CarWanderRandomly);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_IN_AREA_3D, IsCarInArea);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_STOPPED_IN_AREA_2D, IsCarInArea);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_STOPPED_IN_AREA_3D, IsCarInArea);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_HEALTH_GREATER, IsCarHealthGreater);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_STUCK_ON_ROOF, IsCarStuckOnRoof);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_HEAVY, SetCarHeavy);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_FORWARD_X, GetCarForwardX);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_FORWARD_Y, GetCarForwardY);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_VISIBLE, SetCarVisible);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_WATERTIGHT, SetCarWatertight);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_STATUS, SetCarStatus);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_STRONG, SetCarStrong);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_VISIBLY_DAMAGED, IsCarVisiblyDamaged);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_CAN_BE_DAMAGED, SetCarCanBeDamaged);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_TRACTION, SetCarTraction);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_AVOID_LEVEL_TRANSITIONS, SetCarAvoidLevelTransitions);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_STAY_IN_FAST_LANE, SetCarStayInFastLane);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_STAY_IN_SLOW_LANE, SetCarStayInSlowLane);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_TEMP_ACTION, SetCarTempAction);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_RANDOM_ROUTE_SEED, SetCarRandomRouteSeed);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_TYRE_BURST, IsCarTireBurst);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_WAITING_FOR_WORLD_COLLISION, IsCarWaitingForWorldCollision);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_SPEED_VECTOR, GetCarSpeedVector);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_ROLL, GetCarRoll);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_PITCH, GetCarPitch);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_ROTATION_VELOCITY, SetCarRotationVelocity);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_FOLLOW_CAR, SetCarFollowCar);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_HYDRAULICS, SetCarHydraulics);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_UPRIGHT_VALUE, GetCarUprightValue);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_CAN_BE_VISIBLY_DAMAGED, SetCarCanBeVisiblyDamaged);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_COORDINATES_NO_OFFSET, SetCarCoordinatesNoOffset);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_ENGINE_ON, SetCarEngineOn);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_LIGHTS_ON, SetCarLightsOn);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CAR_MOVING_COMPONENT_OFFSET, GetCarMovingComponentOffset);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_COLLISION, SetCarCollision);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CAR_DOOR_DAMAGED, IsCarDoorDamaged);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_AS_MISSION_CAR, SetCarAsMissionCar);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_LEAVE_CAR_AND_FLEE, TaskLeaveCarAndFlee);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_DESTROY_CAR, TaskDestroyCar);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_EVERYONE_LEAVE_CAR, TaskEveryoneLeaveCar);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_DRIVE_POINT_ROUTE, TaskDrivePointRoute);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_DRIVE_POINT_ROUTE_ADVANCED, TaskDrivePointRouteAdvanced);


    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_TAXI);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_TAXI_TIMER);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_BOAT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_NUMBER_OF_CARS_COLLECTED_BY_GARAGE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_CAR_BEEN_TAKEN_TO_GARAGE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_RANDOM_CAR_OF_TYPE_IN_ZONE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_RESPRAY_HAPPENED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_RAM_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_BLOCK_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_FUNNY_SUSPENSION);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_BIG_WHEELS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_CAR_RADIO);
}
