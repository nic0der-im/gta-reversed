#include "StdInc.h"

#include "PedStats.h"
#include "CarEnterExit.h"
#include <reversiblebugfixes/Bugs.hpp>
#include "TaskSimpleCarSetPedInAsDriver.h"
#include "TaskComplexDriveWander.h"
#include "TaskSimpleCarSetPedInAsPassenger.h"
#include "General.h"
#include "Pad.h"
#include "Timer.h"
#include "PlayerPed.h"
#include "Camera.h"
#include "EventPedEnteredMyVehicle.h"
#include "VehicleAnimGroupData.h"
#include "eTargetDoor.h"

void CCarEnterExit::InjectHooks() {
    RH_ScopedClass(CCarEnterExit);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(AddInCarAnim, 0x64F720);
    RH_ScopedInstall(CarHasDoorToClose, 0x64EE10);
    RH_ScopedInstall(CarHasDoorToOpen, 0x64EDD0);
    RH_ScopedInstall(CarHasOpenableDoor, 0x64EE50);
    RH_ScopedInstall(CarHasPartiallyOpenDoor, 0x64EE70);
    RH_ScopedInstall(ComputeDoorFlag, 0x64E550);
    RH_ScopedInstall(ComputeOppositeDoorFlag, 0x64E610);
    RH_ScopedInstall(ComputePassengerIndexFromCarDoor, 0x64F1E0);
    RH_ScopedInstall(ComputeSlowJackedPed, 0x64F070);
    RH_ScopedInstall(ComputeTargetDoorToEnterAsPassenger, 0x64F190);
    RH_ScopedInstall(ComputeTargetDoorToExit, 0x64F110);
    RH_ScopedInstall(GetNearestCarDoor, 0x6528F0);
    RH_ScopedInstall(GetNearestCarPassengerDoor, 0x650BB0);
    RH_ScopedInstall(GetPositionToOpenCarDoor, 0x64E740);
    RH_ScopedInstall(IsCarDoorInUse, 0x64EC90);
    RH_ScopedInstall(IsCarDoorReady, 0x64ED90);
    RH_ScopedInstall(IsCarQuickJackPossible, 0x64EF00);
    RH_ScopedInstall(IsCarSlowJackRequired, 0x64EF70);
    RH_ScopedInstall(IsClearToDriveAway, 0x6509B0);
    RH_ScopedInstall(IsPathToDoorBlockedByVehicleCollisionModel, 0x651210);
    RH_ScopedInstall(IsPedHealthy, 0x64EEE0);
    RH_ScopedInstall(IsPlayerToQuitCarEnter, 0x64F240);
    RH_ScopedInstall(IsRoomForPedToLeaveCar, 0x6504C0);
    RH_ScopedInstall(IsVehicleHealthy, 0x64EEC0);
    RH_ScopedInstall(IsVehicleStealable, 0x6510D0);
    RH_ScopedInstall(MakeUndraggedDriverPedLeaveCar, 0x64F600);
    RH_ScopedInstall(MakeUndraggedPassengerPedsLeaveCar, 0x64F540);
    RH_ScopedInstall(QuitEnteringCar, 0x650130);
    RH_ScopedInstall(RemoveCarSitAnim, 0x64F680);
    RH_ScopedInstall(RemoveGetInAnims, 0x64F6E0);
    RH_ScopedInstall(SetAnimOffsetForEnterOrExitVehicle, 0x64F860);
    RH_ScopedInstall(SetPedInCarDirect, 0x650280);
}

// 0x64F720
void CCarEnterExit::AddInCarAnim(const CVehicle* vehicle, CPed* ped, bool bAsDriver) {
    const auto [grpId, animId] = [&]() -> std::pair<AssocGroupId, AnimationId> {
        if (bAsDriver) { // Inverted
            if (const auto data = const_cast<CVehicle*>(vehicle)->GetRideAnimData()) {
                return { data->AnimGroup, ANIM_ID_BIKE_RIDE };
            } else if (vehicle->IsBoat()) {
                if (vehicle->m_pHandlingData->m_bSitInBoat) {
                    return { ANIM_GROUP_DEFAULT, ANIM_ID_DRIVE_BOAT };
                }
            } else if (vehicle->vehicleFlags.bLowVehicle) {
                return { ANIM_GROUP_DEFAULT, ANIM_ID_CAR_LSIT };
            }

            return { ANIM_GROUP_DEFAULT, ANIM_ID_CAR_SIT };
        } else {
            if (const auto data = const_cast<CVehicle*>(vehicle)->GetRideAnimData()) {
                return { data->AnimGroup, ANIM_ID_BIKE_RIDE };
            } else if (vehicle->vehicleFlags.bLowVehicle) {
                return { ANIM_GROUP_DEFAULT, ANIM_ID_CAR_SITPLO };
            }

            return { ANIM_GROUP_DEFAULT, ANIM_ID_CAR_SITP };
        }
    }();
    CAnimManager::BlendAnimation(ped->GetRpClump(), grpId, animId, 1000.f);
    ped->StopNonPartialAnims();
}

// 0x64EE10
bool CCarEnterExit::CarHasDoorToClose(const CVehicle* vehicle, int32 doorId) {
    auto& veh = const_cast<CVehicle&>(*vehicle);
    return !veh.IsDoorMissingU32(doorId) && !veh.IsDoorClosedU32(doorId);
}

// 0x64EDD0
bool CCarEnterExit::CarHasDoorToOpen(const CVehicle* vehicle, int32 doorId) {
    auto& veh = const_cast<CVehicle&>(*vehicle);
    return !veh.IsDoorMissingU32((uint32)doorId) && !veh.IsDoorFullyOpenU32((uint32)doorId);
}

// 0x64EE50
bool CCarEnterExit::CarHasOpenableDoor(const CVehicle* vehicle, int32 doorId_UnusedArg, const CPed* ped) {
    return vehicle->CanPedOpenLocks(ped);
}

// 0x64EE70
bool CCarEnterExit::CarHasPartiallyOpenDoor(const CVehicle* vehicle, int32 doorId) {
    auto& veh = const_cast<CVehicle&>(*vehicle); // TODO: Fix
    return !veh.IsDoorMissingU32((uint32)doorId)
        && !veh.IsDoorFullyOpenU32((uint32)doorId)
        && !veh.IsDoorClosedU32((uint32)doorId);
}

// 0x64E550
int32 CCarEnterExit::ComputeDoorFlag(const CVehicle* vehicle, int32 doorId, bool bSettingFlags) {
    if (bSettingFlags && (vehicle->IsBike() || vehicle->m_pHandlingData->m_bTandemSeats)) {
        switch (doorId) {
        case 8:
        case 10:
        case 18: return 5;
        case 9:
        case 11: return 10;
        default: NOTSA_UNREACHABLE(); // Originally `return 0`
        }
    } else {
        switch (doorId) {
        case 8:  return 4;
        case 9:  return 8;
        case 10:
        case 18: return 1;
        case 11: return 2;
        default: NOTSA_UNREACHABLE(); // Originally `return 0`
        }
    }
}

// 0x64E610
int32 CCarEnterExit::ComputeOppositeDoorFlag(const CVehicle* vehicle, int32 doorId, bool bCheckVehicleType) {
    if (bCheckVehicleType && (vehicle->IsBike() || vehicle->m_pHandlingData->m_bTandemSeats)) {
        switch (doorId) {
        case 8:
        case 10:
        case 18: return 5;
        case 9:
        case 11: return 10;
        default: NOTSA_UNREACHABLE(); // Originally `return 0`
        }
    } else {
        switch (doorId) {
        case 8: return 1;
        case 9: return 2;
        case 10:
        case 18: return 4;
        case 11: return 8;
        default: NOTSA_UNREACHABLE(); // Originally `return 0`
        }
    }
}

// 0x64F1E0
int32 CCarEnterExit::ComputePassengerIndexFromCarDoor(const CVehicle* vehicle, int32 doorId) {
    if (vehicle->IsBike() || vehicle->m_pHandlingData->m_bTandemSeats) {
        switch (doorId) {
        case 9:
        case 11:
            return 0;
        default:
            return -1;
        }
    }

    switch (doorId) {
    case 8:
        return 0;
    case 9:
        return 2;
    case 11:
        return 1;
    default:
        return -1;
    }
}

// NOTSA - The passenger in the seat that's entered through the given door
CPed* CCarEnterExit::GetPassengerAtCarDoor(const CVehicle* vehicle, int32 doorId) {
    const auto psgrIdx = ComputePassengerIndexFromCarDoor(vehicle, doorId);
    if (psgrIdx == -1) {
        // BUG: The original indexes `m_apPassengers` with the result unchecked. If the door has no passenger seat (e.g. door 8 on a bike,
        //      or seat 0 ("any seat") for `CTaskComplexEnterCarAsPassengerTimed`) that reads `m_apPassengers[-1]`, which is `m_pDriver`,
        //      the member right before it. At least for bikes that's what R* intended anyway (See `ComputeSlowJackedPed`).
        return notsa::bugfixes::CCarEnterExit_GetPassengerAtCarDoor_NoPassengerSeat
            ? vehicle->m_pDriver
            : vehicle->m_apPassengers.data()[psgrIdx];
    }
    return vehicle->m_apPassengers[psgrIdx];
}

// 0x64F070
CPed* CCarEnterExit::ComputeSlowJackedPed(const CVehicle* vehicle, int32 doorId) {
    if (vehicle->IsBike() || vehicle->m_pHandlingData->m_bTandemSeats) {
        switch (doorId) {
        case 8:
        case 10:
        case 18:
            return vehicle->m_pDriver;
        case 9:
        case 11:
            return vehicle->m_apPassengers[0];
        default:
            return nullptr;
        }
    }

    switch (doorId) {
    case 8:
        return vehicle->m_apPassengers[0];
    case 9:
        return vehicle->m_apPassengers[2];
    case 10:
        return vehicle->m_pDriver;
    case 11:
        return vehicle->m_apPassengers[1];
    default:
        return nullptr;
    }
}

// 0x64F190
int32 CCarEnterExit::ComputeTargetDoorToEnterAsPassenger(const CVehicle* vehicle, int32 psgrIdx) {
    if (vehicle->vehicleFlags.bIsBus) {
        return 8;
    }

    switch (psgrIdx) {
    case 0:
        return (vehicle->IsBike() || vehicle->m_pHandlingData->m_bTandemSeats) ? 11 : 8; // Inverted condition
    case 1:
        return 11;
    case 2:
        return 9;
    default:
        return -1;
    }
}

// 0x64F110
int32 CCarEnterExit::ComputeTargetDoorToExit(const CVehicle* vehicle, const CPed* ped) {
    if (vehicle->m_pDriver == ped) {
        return 10;
    }

    // Theoritically the rest here is the same as `ComputeTargetDoorToEnterAsPassenger`
    // but I'm not quite sure, as in that function they just check `bIsBus`, while here they check the anim groups
    // So, using the below switch I make sure the theory is right.
    switch (vehicle->GetAnimGroupId()) {
    case ANIM_GROUP_COACHCARANIMS:
    case ANIM_GROUP_BUSCARANIMS:
        assert(vehicle->vehicleFlags.bIsBus);
    }

    if (const auto optIndex = vehicle->GetPassengerIndex(ped)) {
        return ComputeTargetDoorToEnterAsPassenger(vehicle, *optIndex);
    }

    return -1;
}

// 0x6528F0
bool CCarEnterExit::GetNearestCarDoor(const CPed* ped, const CVehicle* vehicle, CVector& outPos, int32& doorId) {
    auto driverDraggedOutOffset = vehicle->m_pDriver ? &ms_vecPedQuickDraggedOutCarAnimOffset : nullptr;
    auto psgrDraggedOutOffset   = vehicle->HasPassengerAtSeat(0) ? &ms_vecPedQuickDraggedOutCarAnimOffset : nullptr;

    if ((vehicle->IsBike() && !vehicle->IsSubBMX()) || vehicle->IsSubQuad()) {
        driverDraggedOutOffset = nullptr;
        psgrDraggedOutOffset = nullptr;

        if (ped->GetTaskManager().GetActiveTask()->GetTaskType() != TASK_COMPLEX_ENTER_CAR_AS_PASSENGER) {
            if (std::abs(vehicle->GetRight().z) < 0.1f) { // Isn't on it's side
                // Check if ped is 30 degrees to the left of the vehicle
                // Original code used atan and whatnot, but this achieves the same result
                if (DotProduct2D(vehicle->GetRight(), ped->GetForward()) > 0 // On the left
                 && DotProduct2D(vehicle->GetForward(), ped->GetForward()) > std::cos(PI / 6.f)
                ) {
                    if ((ped->IsPlayer() && ped->GetPlayerData()->m_fMoveBlendRatio > 1.5f && doorId == 0) 
                    || (!ped->IsPlayer() && ped->m_nPedType != PED_TYPE_COP && ped->m_nMoveState == PEDMOVE_RUN && ped->m_pStats->m_nTemper > 65 && doorId == 0)
                    ) {
                        // 18 here is probably either from eBikeNodes or eQuadNodes, not sure?
                        if (IsRoomForPedToLeaveCar(vehicle, 18)) {
                            doorId = 18;
                            outPos = GetPositionToOpenCarDoor(vehicle, 18);
                            return true;
                        }
                    }
                }
            }
        }
    } else if (vehicle->vehicleFlags.bIsBus || vehicle->vehicleFlags.bLowVehicle) {
        driverDraggedOutOffset = nullptr;
        psgrDraggedOutOffset = nullptr;
    }


    const auto posDoorFLeft = GetPositionToOpenCarDoor(vehicle, CAR_DOOR_LF);
    const auto posDoorFRight = GetPositionToOpenCarDoor(vehicle, CAR_DOOR_RF);

    CVector2D pedPos2D = ped->GetPosition();
    CVector2D dir2DToDoorFLeft = posDoorFLeft - pedPos2D, dir2DToDoorFRight = posDoorFRight - pedPos2D;

    if (vehicle->m_pVehicleBeingTowed) {
        if (dir2DToDoorFLeft.SquaredMagnitude() < dir2DToDoorFRight.SquaredMagnitude()) {
            if (IsPathToDoorBlockedByVehicleCollisionModel(ped, vehicle, posDoorFRight)) {
                dir2DToDoorFRight = { 999.90002f, 999.90002f };
            } else if (IsPathToDoorBlockedByVehicleCollisionModel(ped, vehicle, posDoorFLeft)) {
                dir2DToDoorFLeft = { 999.90002f, 999.90002f };
            }
        }
    }

    if (vehicle->m_pHandlingData->m_bForceDoorCheck && vehicle->IsAutomobile()) {
        const auto aut = static_cast<const CAutomobile*>(vehicle);

        if (aut->m_aCarNodes[CAR_DOOR_RF]) { // Inverted
            if (!aut->m_aCarNodes[CAR_DOOR_LF]) {
                if (IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_RF, driverDraggedOutOffset)) {
                    doorId = CAR_DOOR_RF;
                    outPos = posDoorFRight;
                    return true;
                }
            }
        } else {
            if (IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_LF, driverDraggedOutOffset)) {
                doorId = CAR_DOOR_LF;
                outPos = posDoorFLeft;
                return true;
            }
        }

        return false;
    }

    if (doorId != CAR_NODE_NONE && IsRoomForPedToLeaveCar(vehicle, doorId, driverDraggedOutOffset)) {
        switch (doorId) {
        case CAR_DOOR_LF: {
            doorId = CAR_DOOR_LF;
            outPos = posDoorFLeft;
            return true;
        }
        case CAR_DOOR_RF: {
            doorId = CAR_DOOR_RF;
            outPos = posDoorFRight;
            return true;
        }
        default:
            return false;
        }
    }

    if (!vehicle->m_pDriver
    || (!CPedGroups::AreInSameGroup(ped, vehicle->m_pDriver) && !vehicle->m_pDriver->bDontDragMeOutCar)
    ) {
        if (vehicle->vehicleFlags.bIsBus
         || dir2DToDoorFRight.SquaredMagnitude() > dir2DToDoorFLeft.SquaredMagnitude()
        ) {
            if (IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_LF, driverDraggedOutOffset)) {
                doorId = CAR_DOOR_LF;
                outPos = posDoorFLeft;
                return true;
            }
            if (IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_RF, driverDraggedOutOffset)) {
                doorId = CAR_DOOR_RF;
                outPos = posDoorFRight;
                return true;
            }
        } else {
            if (IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_RF, driverDraggedOutOffset)) {
                if ((
                        vehicle->HasPassengerAtSeat(0)
                        && !vehicle->IsBike()
                        && !vehicle->m_pHandlingData->m_bTandemSeats
                        && (CPedGroups::AreInSameGroup(vehicle->m_apPassengers[0], ped) || vehicle->m_apPassengers[0]->bDontDragMeOutCar || vehicle->IsMissionVehicle())
                        && IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_LF, driverDraggedOutOffset)
                    ) || (
                        vehicle->m_nGettingInFlags & 4
                        && IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_LF, driverDraggedOutOffset)
                    )
                ) {
                    doorId = CAR_DOOR_LF;
                    outPos = posDoorFLeft;
                    return true;
                } else {
                    doorId = CAR_DOOR_RF;
                    outPos = posDoorFRight;
                    return true;
                }
            }

            if (IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_LF, driverDraggedOutOffset)) {
                doorId = CAR_DOOR_LF;
                outPos = posDoorFLeft;
                return true;
            }
        }
    }
    return false;
}

// 0x650BB0
bool CCarEnterExit::GetNearestCarPassengerDoor(const CPed* ped, const CVehicle* vehicle, CVector* outVec, int32* doorId, bool CheckIfOccupiedTandemSeat, bool CheckIfDoorIsEnterable, bool CheckIfRoomToGetIn) {
    // Coach/bus: The only door available is the front one
    switch (vehicle->GetAnimGroupId()) {
    case ANIM_GROUP_COACHCARANIMS:
    case ANIM_GROUP_BUSCARANIMS:
        if ((CheckIfDoorIsEnterable && (vehicle->m_nGettingInFlags & 4))
         || (CheckIfRoomToGetIn && !IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_RF, nullptr))
        ) {
            return false;
        }
        *outVec = GetPositionToOpenCarDoor(vehicle, CAR_DOOR_RF);
        *doorId = CAR_DOOR_RF;
        return true;
    }

    const CVector pedPos = ped->GetPosition();

    // Candidates: A door is only valid if its position was computed (The `999` distances are used to
    // reject doors - The nearest one is then chosen by comparing the squared 2D distances)
    CVector pos{};   // Also the default output (The original reuses the same stack slot)
    CVector posRF{}, posLR{}, posRR{};
    float   distLRX = 999.f, distLRY = 999.f;
    float   distRFX = 999.f, distRFY = 999.f;
    float   distRRX = 999.f, distRRY = 999.f;
    bool    result  = false;

    if (vehicle->IsBike() || vehicle->m_pHandlingData->m_bTandemSeats) {
        if (!CheckIfOccupiedTandemSeat || !vehicle->m_apPassengers[0]) {
            if ((!CheckIfDoorIsEnterable || !(vehicle->m_nGettingInFlags & 2))
             && (!CheckIfRoomToGetIn || IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_LR, nullptr))
            ) {
                posLR   = pos = GetPositionToOpenCarDoor(vehicle, CAR_DOOR_LR);
                distLRX = posLR.x - pedPos.x;
                distLRY = posLR.y - pedPos.y;
                result  = true;
            }
            if ((!CheckIfDoorIsEnterable || !(vehicle->m_nGettingInFlags & 8))
             && (!CheckIfRoomToGetIn || IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_RR, nullptr))
            ) {
                posRR   = pos = GetPositionToOpenCarDoor(vehicle, CAR_DOOR_RR);
                distRRX = posRR.x - pedPos.x;
                distRRY = posRR.y - pedPos.y;
                result  = true;
            }
        }
    } else {
        if (!(CheckIfOccupiedTandemSeat && vehicle->m_apPassengers[0])
         && !(CheckIfDoorIsEnterable && (vehicle->m_nGettingInFlags & 4))
         && !(CheckIfRoomToGetIn && !IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_RF, nullptr))
        ) {
            posRF   = pos = GetPositionToOpenCarDoor(vehicle, CAR_DOOR_RF);
            distRFX = posRF.x - pedPos.x;
            distRFY = posRF.y - pedPos.y;
        }
        result = true; // NOTE: Set even if no candidate was produced
    }

    // Rear doors are only available on vehicles with more than 2 doors
    if (vehicle->GetVehicleModelInfo()->m_nNumDoors > 2) {
        const auto CanUseRearDoor = [vehicle](int32 doorNode) {
            return !vehicle->m_pHandlingData->m_bForceDoorCheck
                || (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE
                    && static_cast<const CAutomobile*>(vehicle)->m_aCarNodes[doorNode]
                );
        };

        if (CanUseRearDoor(CAR_DOOR_LR)
         && (!CheckIfOccupiedTandemSeat || !vehicle->m_apPassengers[1])
         && (!CheckIfDoorIsEnterable || !(vehicle->m_nGettingInFlags & 2))
         && (!CheckIfRoomToGetIn || IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_LR, nullptr))
        ) {
            posLR   = GetPositionToOpenCarDoor(vehicle, CAR_DOOR_LR);
            distLRX = posLR.x - pedPos.x;
            distLRY = posLR.y - pedPos.y;
            result  = true;
        }
        if (CanUseRearDoor(CAR_DOOR_RR)
         && (!CheckIfOccupiedTandemSeat || !vehicle->m_apPassengers[2])
         && (!CheckIfDoorIsEnterable || !(vehicle->m_nGettingInFlags & 8))
         && (!CheckIfRoomToGetIn || IsRoomForPedToLeaveCar(vehicle, CAR_DOOR_RR, nullptr))
        ) {
            posRR   = GetPositionToOpenCarDoor(vehicle, CAR_DOOR_RR);
            distRRX = posRR.x - pedPos.x;
            distRRY = posRR.y - pedPos.y;
            result  = true;
        }
    }

    *outVec = pos;
    *doorId = CAR_DOOR_RF;
    if (distLRX * distLRX + distLRY * distLRY < distRFX * distRFX + distRFY * distRFY) {
        *doorId = CAR_DOOR_LR;
        *outVec = posLR;
        distRFX = distLRX;
        distRFY = distLRY;
    }
    if (distRRX * distRRX + distRRY * distRRY < distRFX * distRFX + distRFY * distRFY) {
        *doorId = CAR_DOOR_RR;
        *outVec = posRR;
    }
    return result;
}

// 0x64E740
// Originally RVO'd
CVector CCarEnterExit::GetPositionToOpenCarDoor(const CVehicle* vehicle, int32 doorId) {
    const auto  modelInfo = vehicle->GetVehicleModelInfo();
    const auto& handling  = *vehicle->m_pHandlingData;
    const auto& mat       = *vehicle->m_matrix;
    const auto& dummyPos  = modelInfo->m_pVehicleStruct->m_avDummyPos;

    // Boats have their front door at the front light dummy
    const auto& dummyFront = dummyPos[modelInfo->IsBoat() ? DUMMY_LIGHT_FRONT_MAIN : DUMMY_SEAT_FRONT];
    const auto& dummyRear  = dummyPos[DUMMY_SEAT_REAR];

    if (vehicle->IsBike() || handling.m_bTandemSeats) {
        if (doorId == TARGET_DOOR_UNK) {
            const auto offs = vehicle->GetAnimGroup().ComputeAnimDoorOffsets(ENTER_BIKE_FRONT);
            return mat.TransformPoint(dummyFront - CVector{ offs.x, -offs.y, offs.z });
        }

        CVector       offs  = vehicle->GetAnimGroup().ComputeAnimDoorOffsets(ENTER_FRONT);
        const CVector dummy = doorId == CAR_DOOR_LR || doorId == CAR_DOOR_RR ? dummyRear : dummyFront;

        if (vehicle->IsBike()) {
            if (doorId == CAR_DOOR_RR || doorId == CAR_DOOR_RF) {
                offs.x = -offs.x;
            }
            CVector correctedPos;
            const_cast<CVehicle*>(vehicle)->AsBike()->GetCorrectedWorldDoorPosition(correctedPos, offs, dummy);
            return correctedPos;
        }

        // Tandem seats (But not bikes)
        CVector pos = dummy;
        if (doorId == CAR_DOOR_RR || doorId == CAR_DOOR_RF) {
            offs.x = -offs.x;
            pos.x += handling.m_fSeatOffsetDistance;
        } else {
            pos.x -= handling.m_fSeatOffsetDistance;
        }
        return mat.TransformPoint(pos - offs);
    }

    // NOTE: `m_nAnimGroup` can't be `101` with valid handling data (There are only 30 anim groups),
    //       but the original still checks for it.
    const float seatOffset = (handling.m_nAnimGroup == 101 && (doorId == CAR_DOOR_LR || doorId == CAR_DOOR_RR))
        ? 0.f
        : handling.m_fSeatOffsetDistance;

    eVehAnimDoorOffset offsetIdx;
    switch (doorId) {
    case CAR_DOOR_LF:
    case CAR_DOOR_RF:
        offsetIdx = ENTER_FRONT;
        break;
    case CAR_DOOR_LR:
    case CAR_DOOR_RR:
        offsetIdx = ENTER_REAR;
        break;
    case TARGET_DOOR_UNK:
        offsetIdx = ENTER_BIKE_FRONT;
        break;
    default:
        // NOTE: The original uses the raw bits of the seat offset here (MSVC stack-slot reuse)
        offsetIdx = static_cast<eVehAnimDoorOffset>(static_cast<uint8>(std::bit_cast<uint32>(seatOffset)));
        break;
    }

    CVector       offs = vehicle->GetAnimGroup().ComputeAnimDoorOffsets(offsetIdx);
    CVector       pos;
    switch (doorId) {
    case CAR_DOOR_RF:
        pos   = dummyFront;
        pos.x += seatOffset;
        offs.x = -offs.x;
        break;
    case CAR_DOOR_RR:
        pos   = dummyRear;
        pos.x += seatOffset;
        offs.x = -offs.x;
        break;
    case CAR_DOOR_LF:
        pos   = dummyFront;
        pos.x = -(pos.x + seatOffset);
        break;
    case CAR_DOOR_LR:
        pos   = dummyRear;
        pos.x = -(pos.x + seatOffset);
        break;
    default:
        pos  = dummyFront;
        offs = CVector{};
        break;
    }

    CVector v = pos - offs;

    // Raised/hydraulic vehicles have the door at a fixed height above the ground
    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_MTRUCK
     || (handling.m_bIsBig && vehicle->m_nVehicleSubType != VEHICLE_TYPE_PLANE)
    ) {
        v.z = 0.95f - const_cast<CVehicle*>(vehicle)->GetHeightAboveRoad();
    }

    CVector out = mat.TransformVector(v);
    out += vehicle->GetPosition(); // Handles `m_matrix == nullptr`
    return out;
}

// 0x64EC90
bool CCarEnterExit::IsCarDoorInUse(const CVehicle* vehicle, int32 firstDoorId, int32 secondDoorId) {
    const auto CheckIsDoorInUse = [vehicle](int32 door) {
        const auto CheckInOutFlags = [vehicle](uint32 n) {
            const auto flag = 1 << n;
            return (flag & vehicle->m_nGettingInFlags) || (flag & vehicle->m_nGettingOutFlags);
        };
        switch (door) {
        case 8: return CheckInOutFlags(2);
        case 9: return CheckInOutFlags(3);
        case 10:
        case 18: return CheckInOutFlags(0);
        case 11: return CheckInOutFlags(1);
        default: return false;
        }
    };
    return CheckIsDoorInUse(firstDoorId) || CheckIsDoorInUse(secondDoorId);
}

// 0x64ED90
bool CCarEnterExit::IsCarDoorReady(const CVehicle* vehicle, int32 doorId) {
    // TODO: Make IsDoorReadyU32 a const member function to avoid const_cast
    auto& veh = const_cast<CVehicle&>(*vehicle); // TODO: Fix
    return veh.IsDoorReadyU32((uint32)doorId)
        || veh.IsDoorFullyOpenU32((uint32)doorId);
}

// 0x64EF00
bool CCarEnterExit::IsCarQuickJackPossible(CVehicle* vehicle, int32 doorId, const CPed* ped) {
    // I think doorId 10 is the driver's door
    //if (doorId == 10 && vehicle->IsAutomobile() && !vehicle->IsDoorMissingU32(doorId) && vehicle->IsDoorClosedU32(doorId)) {
    //    // This does *nothing* - I tried `return vehicle->CanPedOpenLocks(ped);` but that just breaks everything.
    //    // Basically, returning anything but `false` from here breaks the code (in `CTaskComplexEnterCar`)
    //    vehicle->CanPedOpenLocks(ped); 
    //}
    return false;
}

// 0x64EF70
bool CCarEnterExit::IsCarSlowJackRequired(const CVehicle* vehicle, int32 doorId) {
    if (vehicle->IsBike() || (vehicle->m_pHandlingData->m_bTandemSeats)) {
        switch (doorId) {
        case 8:
        case 10:
        case 18:
            return vehicle->HasDriver();
        case 9:
        case 11:
            return vehicle->HasPassengerAtSeat(0);
        default:
            return false;
        }
    }

    int group = vehicle->GetAnimGroupId();
    if (group == ANIM_GROUP_COACHCARANIMS || group == ANIM_GROUP_BUSCARANIMS) {
        switch (doorId) {
        case 8:
            return false;
        case 10:
            return vehicle->HasDriver();
        }
    } else {
        switch (doorId) {
        case 8:
            return vehicle->HasPassengerAtSeat(0);
        case 9:
            return vehicle->HasPassengerAtSeat(2);
        case 10:
            return vehicle->HasDriver();
        case 11:
            return vehicle->HasPassengerAtSeat(1);
        default:
            return false;
        }
    }
    return false;
}

// 0x6509B0
bool CCarEnterExit::IsClearToDriveAway(const CVehicle* vehicle) {
    const auto& vehPos = vehicle->GetPosition();
    const auto  bbSizeY = vehicle->GetColModel()->GetBoundingBox().GetSize().y;
    CEntity* hitEntity{};
    CColPoint hitCP{};
    return !CWorld::ProcessLineOfSight(vehPos + vehicle->GetForward() * bbSizeY, vehPos, hitCP, hitEntity, true, true, false, false, false, true, true, false) || hitEntity == vehicle;
}

// 0x651210
bool CCarEnterExit::IsPathToDoorBlockedByVehicleCollisionModel(const CPed* ped, const CVehicle* vehicle, const CVector& pos) {
    if (vehicle->GetModelIndex() == eModelID::MODEL_AT400) {
        return false;
    }

    const auto vehMatInv = Invert(*vehicle->m_matrix);
    const CColLine line{
        vehMatInv.TransformPoint(ped->GetPosition()),
        vehMatInv.TransformPoint(pos)
    };

    for (const auto& sp : vehicle->GetColModel()->GetData()->GetSpheres()) {
        if (CCollision::TestLineSphere(line, sp)) {
            return false;
        }
    }

    return true;
}

// 0x64EEE0
bool CCarEnterExit::IsPedHealthy(CPed* ped) {
    return ped->m_fHealth > 0.f;
}

// 0x64F240
bool CCarEnterExit::IsPlayerToQuitCarEnter(const CPed* ped, const CVehicle* vehicle, int32 startTime, CTask* task) {
    const auto pad = static_cast<const CPlayerPed*>(ped)->GetPadFromPlayer();

    float heading = ped->m_fCurrentRotation;

    // Determine the heading the ped should move towards to get away from the car
    {
        bool quitByMeleeButton = false;
        bool computeHeading    = false;
        if (task) {
            switch (task->GetTaskType()) {
            case TASK_SIMPLE_CAR_ALIGN:
            case TASK_SIMPLE_STAND_STILL:
            case TASK_COMPLEX_FALL_AND_GET_UP:
            case TASK_COMPLEX_LEAVE_CAR:
            case TASK_SIMPLE_CAR_OPEN_DOOR_FROM_OUTSIDE:
            case TASK_SIMPLE_CAR_OPEN_LOCKED_DOOR_FROM_OUTSIDE:
            case TASK_SIMPLE_BIKE_PICK_UP:
            case TASK_SIMPLE_CAR_QUICK_DRAG_PED_OUT:
            case TASK_SIMPLE_CAR_SLOW_DRAG_PED_OUT:
                quitByMeleeButton = true;
                computeHeading    = true;
                break;
            case TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_INSIDE:
            case TASK_SIMPLE_CAR_GET_IN:
            case TASK_SIMPLE_CAR_SHUFFLE:
            case TASK_SIMPLE_CAR_SET_PED_IN_AS_DRIVER:
            case TASK_SIMPLE_CAR_SET_PED_OUT:
            case TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR:
                computeHeading = true;
                break;
            default:
                break;
            }
        }

        if (computeHeading) {
            const auto& vehMat = *vehicle->m_matrix;

            // The door is on the side of the vehicle the ped is on
            const bool pedIsLeftOfVehicle = DotProduct(ped->GetPosition() - vehicle->GetPosition(), vehMat.GetRight()) < 0.f;

            heading = std::atan2(-vehMat.GetForward().x, vehMat.GetForward().y) + (pedIsLeftOfVehicle ? -PI / 2.f : PI / 2.f);
            if (vehMat.GetUp().z < 0.f) { // Vehicle is upside down
                heading += PI;
                if (heading >= PI) {
                    heading -= 2.f * PI;
                }
            }
            if (heading >= PI) {
                heading -= 2.f * PI;
            }
            if (heading < -PI) {
                heading += 2.f * PI;
            }
        }

        if (vehicle->m_pFire) {
            return true;
        }

        if (pad->DisablePlayerControls) {
            return false;
        }

        if (quitByMeleeButton) {
            if (pad->MeleeAttackJustDown(false)) {
                return true;
            }
        } else {
            const auto elapsedMs = static_cast<float>(static_cast<uint32>(CTimer::GetTimeInMS() - startTime));
            if (elapsedMs < 500.f) {
                return false;
            }
        }
    }

    // The movement stick has to point away from the door for the ped to quit
    const float upDown    = static_cast<float>(pad->GetPedWalkUpDown());
    const float leftRight = static_cast<float>(pad->GetPedWalkLeftRight());

    float angle = CGeneral::GetRadianAngleBetweenPoints(0.f, 0.f, -leftRight, upDown) - TheCamera.m_fOrientation;
    if (angle >= heading + PI) {
        angle -= 2.f * PI;
    } else if (angle < heading - PI) {
        angle += 2.f * PI;
    }

    return std::sqrt(leftRight * leftRight + upDown * upDown) * (1.f / 128.f) > 0.75f
        && std::abs(angle - heading) > PI / 4.f;
}

// 0x6504C0
bool CCarEnterExit::IsRoomForPedToLeaveCar(const CVehicle* vehicle, int32 doorId, const CVector* pos) {
    const auto modelInfo = vehicle->GetVehicleModelInfo();
    const bool isBike    = vehicle->IsBike();

    // The dummy the door is placed at (And, for the left doors, the sign of its X axis)
    CVector dummy;
    if (isBike || vehicle->m_pHandlingData->m_bTandemSeats) {
        dummy = doorId == CAR_DOOR_LR || doorId == CAR_DOOR_RR
            ? modelInfo->GetBackSeatPosn()
            : modelInfo->GetFrontSeatPosn();
        if (doorId == CAR_DOOR_LF || doorId == CAR_DOOR_LR) {
            dummy.x = -dummy.x;
        }
    } else {
        switch (doorId) {
        case CAR_DOOR_RF:
            dummy = modelInfo->GetFrontSeatPosn();
            break;
        case CAR_DOOR_RR:
            dummy = modelInfo->GetBackSeatPosn();
            break;
        case CAR_DOOR_LF:
            dummy   = modelInfo->GetFrontSeatPosn();
            dummy.x = -dummy.x;
            break;
        case CAR_DOOR_LR:
            dummy   = modelInfo->GetBackSeatPosn();
            dummy.x = -dummy.x;
            break;
        default:
            return false;
        }
    }

    const auto& mat        = *vehicle->m_matrix;
    CVector     dummyWorld = mat.TransformPoint(dummy);
    CVector     doorPos    = GetPositionToOpenCarDoor(vehicle, doorId);

    if (pos) {
        CVector offset = *pos;
        if (doorId == CAR_DOOR_RF || doorId == CAR_DOOR_RR) {
            offset.x = -offset.x;
        }
        doorPos += mat.TransformVector(offset);
    }

    if (mat.GetUp().z < 0.f) { // Vehicle is upside down
        dummyWorld.z += 0.5f;
        doorPos.z    += 0.5f;
    }

    // There must be a clear line between the seat and the door
    {
        const CVector from = dummyWorld;
        CVector       to;
        if (isBike) {
            to = CVector{ doorPos.x, doorPos.y, doorPos.z + 0.2f };
        } else {
            const float dx    = doorPos.x - dummyWorld.x;
            const float dy    = doorPos.y - dummyWorld.y;
            const float len   = std::sqrt(dx * dx + dy * dy);
            const float scale = len / (len + 0.35f);
            to = CVector{ dummyWorld.x + dx * scale, dummyWorld.y + dy * scale, doorPos.z };
        }
        if (!CWorld::GetIsLineOfSightClear(from, to, true, false, false, true, false, false, false)) {
            return false;
        }
    }

    // And nothing may be in the way of the door itself
    {
        auto* const entity = CWorld::TestSphereAgainstWorld(
            doorPos,
            0.35f,
            const_cast<CVehicle*>(vehicle),
            vehicle->m_nVehicleType != VEHICLE_TYPE_TRAIN,
            true,
            false,
            true,
            false,
            false
        );
        if (entity
            && !(entity->m_nModelIndex == MODEL_TUGSTAIR && vehicle->m_nModelIndex == MODEL_AT400)
            && entity != vehicle->m_pAttachedTo
        ) {
            return false;
        }
    }

    // The door mustn't be right at (Or slightly under) the ground level
    CColPoint colPoint{};
    CEntity*  colEntity{};
    if (CWorld::ProcessVerticalLine(doorPos, 1000.f, colPoint, colEntity, true, false, false, true, false, false, nullptr)) {
        if (colPoint.m_vecPoint.z >= doorPos.z && colPoint.m_vecPoint.z <= doorPos.z + 0.6f) {
            return false;
        }
    }
    const float traceZ = colPoint.m_vecPoint.z;

    // Boats (And amphibious vehicles/aircraft) are allowed to be below the water level
    float limitZ;
    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT
     || vehicle->m_nModelIndex == MODEL_SKIMMER
     || vehicle->m_nModelIndex == MODEL_VORTEX
     || vehicle->m_nModelIndex == MODEL_SEASPAR
     || vehicle->m_nModelIndex == MODEL_LEVIATHN
    ) {
        limitZ = traceZ - 1.f;
    } else {
        if (!CWorld::ProcessVerticalLine(doorPos, -1000.f, colPoint, colEntity, true, false, false, true, false, false, nullptr)) {
            return false;
        }
        limitZ = colPoint.m_vecPoint.z;
    }

    return !(traceZ != 0.f && traceZ < limitZ);
}

// 0x64EEC0
bool CCarEnterExit::IsVehicleHealthy(const CVehicle* vehicle) {
    return vehicle->GetStatus() != STATUS_WRECKED;
}

// 0x6510D0
bool CCarEnterExit::IsVehicleStealable(const CVehicle* vehicle, const CPed* ped) {
    switch (vehicle->m_nVehicleSubType) {
    case VEHICLE_TYPE_PLANE:
    case VEHICLE_TYPE_HELI:
        return false;
    }

    switch (vehicle->m_nVehicleType) {
    case VEHICLE_TYPE_AUTOMOBILE:
    case VEHICLE_TYPE_BIKE:
        break;
    default:
        return false;
    }

    if (ped->m_pVehicle != vehicle) {
        switch (vehicle->GetCreatedBy()) {
        case RANDOM_VEHICLE:
        case PARKED_VEHICLE:
            break;
        default:
            return false;
        }
    }

    if (CUpsideDownCarCheck{}.IsCarUpsideDown(vehicle)) {
        return false;
    }

    if (!vehicle->CanBeDriven()) {
        return false;
    }

    if (vehicle->IsLawEnforcementVehicle()) {
        return false;
    }

    if (const auto drvr = vehicle->m_pDriver) {
        if (   drvr->IsCreatedByMission()
            || drvr->IsPlayer()
            || ped->GetIntelligence()->IsFriendlyWith(*drvr)
            || CPedGroups::AreInSameGroup(ped, drvr)
        ) {
            return false;
        }
    }

    if (const auto grp = ped->GetGroup()) {
        if (grp->IsAnyoneUsingCar(vehicle)) {
            return false;
        }
    }

    if (vehicle->m_pFire) {
        return false;
    }

    if (vehicle->m_fHealth <= 600.f) {
        return false;
    }

    if (vehicle->IsUpsideDown() || vehicle->IsOnItsSide()) {
        return false;
    }

    if (!IsClearToDriveAway(vehicle)) {
        return false;
    }

    return true;
}

// 0x64F600
void CCarEnterExit::MakeUndraggedDriverPedLeaveCar(const CVehicle* vehicle, const CPed* pedGettingIn) {
    auto& veh = const_cast<CVehicle&>(*vehicle); // TODO: Fix
    auto& ped = const_cast<CPed&>(*pedGettingIn); // TODO: Fix
    veh.m_pDriver->GetEventGroup().Add(CEventDraggedOutCar{ &veh, &ped, true });
}

// 0x64F540
void CCarEnterExit::MakeUndraggedPassengerPedsLeaveCar(const CVehicle* targetVehicle, const CPed* draggedPed, const CPed* ped) {
    auto& veh = const_cast<CVehicle&>(*targetVehicle); // TODO: Fix
    for (const auto passenger : veh.GetPassengers()) {
        if (!passenger || passenger == draggedPed || passenger->bStayInCarOnJack) {
            continue;
        }
        const auto door = ComputeTargetDoorToExit(&veh, passenger);
        passenger->GetEventGroup().Add(CEventPedEnteredMyVehicle{ const_cast<CPed*>(ped), &veh, static_cast<eTargetDoor>(door) });
    }
}

// unused
// 0x650130
void CCarEnterExit::QuitEnteringCar(CPed* ped, CVehicle* vehicle, int32 doorId, bool bCarWasBeingJacked) {
    RemoveGetInAnims(ped);
    ped->RestartNonPartialAnims();
    if (!RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_IDLE)) {
        CAnimManager::BlendAnimation(ped->GetRpClump(), ped->m_nAnimGroup, ANIM_ID_IDLE, 1000.0f);
    }

    if (bCarWasBeingJacked) {
        vehicle->vehicleFlags.bIsBeingCarJacked = true;
    }
    vehicle->m_nNumGettingIn--;

    if (vehicle->IsBike() || vehicle->m_pHandlingData->m_bTandemSeats) {
        switch (doorId) {
        case 8:
        case 10:
            vehicle->SetGettingInFlags(5);
            break;
        case 9:
        case 11:
            vehicle->SetGettingInFlags(10);
            break;
        }
        vehicle->vehicleFlags.bIsBig = false;
    } else {
        switch (doorId) {
        case 8:
            vehicle->SetGettingInFlags(4);
            break;
        case 9:
            vehicle->SetGettingInFlags(8);
            break;
        case 10:
            vehicle->SetGettingInFlags(vehicle->m_nMaxPassengers ? 1 : 3);
            break;
        case 11:
            vehicle->SetGettingInFlags(vehicle->m_nMaxPassengers ? 2 : 3);
            break;
        }
    }
    ped->SetUsesCollision(false);
}

// 0x64F680
void CCarEnterExit::RemoveCarSitAnim(const CPed* ped) {
    for (auto anim = RpAnimBlendClumpGetFirstAssociation(ped->GetRpClump(), ANIMATION_SECONDARY_TASK_ANIM); anim; anim = RpAnimBlendGetNextAssociation(anim, ANIMATION_SECONDARY_TASK_ANIM)) {
        anim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
        anim->m_BlendDelta = -1000.f;
    }
    CAnimManager::BlendAnimation(ped->GetRpClump(), ped->m_nAnimGroup, ANIM_ID_IDLE, 1000.0);
}

// 0x64F6E0
void CCarEnterExit::RemoveGetInAnims(const CPed* ped) {
    for (auto anim = RpAnimBlendClumpGetFirstAssociation(ped->GetRpClump(), ANIMATION_IS_PARTIAL); anim; anim = RpAnimBlendGetNextAssociation(anim, ANIMATION_IS_PARTIAL)) {
        anim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
        anim->m_BlendDelta = -1000.f;
    }
}

// 0x64F860
void CCarEnterExit::SetAnimOffsetForEnterOrExitVehicle() {
    if (ms_bPedOffsetsCalculated) {
        return;
    }

    const auto animBlockIdxs = {
        CAnimManager::GetAnimationBlockIndex("int_house"),
        CAnimManager::GetAnimationBlockIndex("int_office")
    };

    for (const auto idx : animBlockIdxs) {
        CStreaming::RequestModel(IFPToModelId(idx), STREAMING_KEEP_IN_MEMORY);
    }
    CStreaming::LoadAllRequestedModels(false);

    for (const auto idx : animBlockIdxs) {
        CAnimManager::AddAnimBlockRef(idx);
    }

    {
        const auto anim = CAnimManager::GetAnimAssociation(ANIM_GROUP_DEFAULT, ANIM_ID_GETUP_0);
        CAnimManager::UncompressAnimation(anim->m_BlendHier);
        const auto& seq = anim->m_BlendHier->GetSequences()[0];
        ms_vecPedGetUpAnimOffset = seq.m_FramesNum ? seq.GetUKeyFrame(0)->Trans : CVector{};
    }

    ms_vecPedQuickDraggedOutCarAnimOffset = CVector{ -1.841797f, -0.3261719f, -0.01269531f };

    const std::tuple<AssocGroupId, AnimationId, CVector*> toProcess[]{
        {ANIM_GROUP_INT_HOUSE,  ANIM_ID_BED_IN_L,   &ms_vecPedBedLAnimOffset  },
        {ANIM_GROUP_INT_HOUSE,  ANIM_ID_BED_IN_R,   &ms_vecPedBedRAnimOffset  },
        {ANIM_GROUP_INT_OFFICE, ANIM_ID_OFF_SIT_IN, &ms_vecPedDeskAnimOffset  },
        {ANIM_GROUP_INT_HOUSE,  ANIM_ID_LOU_IN,     &ms_vecPedChairAnimOffset },
    };
    for (const auto [grpId, animId, out] : toProcess) {
        // Calculate translation delta between first and last sequence frames
        *out = [grpId, animId] {
            const auto anim = CAnimManager::GetAnimAssociation(grpId, animId);
            CAnimManager::UncompressAnimation(anim->m_BlendHier);
            const auto& seq = anim->m_BlendHier->GetSequences()[0];
            if (seq.m_FramesNum > 0) {
                return seq.GetUKeyFrame(seq.m_FramesNum - 1)->Trans - seq.GetUKeyFrame(0)->Trans;
            }
            return CVector{};
        }();
    }

    for (const auto idx : animBlockIdxs) {
        CAnimManager::RemoveAnimBlockRef(idx);
    }

    ms_bPedOffsetsCalculated = true;
}

// 0x650280
bool CCarEnterExit::SetPedInCarDirect(CPed* ped, CVehicle* vehicle, int32 doorId, bool bAsDriver) {
    if (bAsDriver) {
        // Warp ped into vehicle
        CTaskSimpleCarSetPedInAsDriver task{ vehicle };
        task.m_bWarpingInToCar = true;
        task.ProcessPed(ped);

        // And make them drive
        ped->GetTaskManager().SetTask(new CTaskComplexCarDriveWander{ vehicle, vehicle->m_autoPilot.m_nCarDrivingStyle, (float)vehicle->m_autoPilot.m_nCruiseSpeed }, TASK_PRIMARY_PRIMARY);

        return true;
    }

    // Warp ped into vehicle
    {
        CTaskSimpleCarSetPedInAsPassenger task{ vehicle, (eTargetDoor)doorId };
        task.m_bWarpingInToCar = true;
        task.ProcessPed(ped);
    }

    if (vehicle->IsBike()) {
        ped->GetTaskManager().SetTask(new CTaskComplexCarDrive{ vehicle, false }, TASK_PRIMARY_PRIMARY);
    }

    // Set mutal acquaintance respect between the ped and all other occupants up to the ped's seat
    // I assume the function is only ever called with `bAsDriver` if there are no passengers
    // So that's why this code-path is only reachable if `bAsDriver` is false

    const auto SetMutalAcquaintanceWith = [ped](CPed* other) {
        if (other) {
            const auto SetWith = [](CPed* of, CPed* with) {
                if (!of->IsCreatedByMission()) {
                    of->GetAcquaintance().SetAsAcquaintance(ACQUAINTANCE_RESPECT, CPedType::GetPedFlag(with->m_nPedType));
                }
            };
            SetWith(ped, other);
            SetWith(other, ped);
        }
    };

    SetMutalAcquaintanceWith(vehicle->m_pDriver);

    const auto psgrIdx = ComputePassengerIndexFromCarDoor(vehicle, doorId);
    assert(psgrIdx != -1); // I really doubt this can happen, if it does, an `if` has to be added
    rng::for_each(vehicle->GetPassengers() | rng::views::take((size_t)psgrIdx), SetMutalAcquaintanceWith); // Set with all other passengers up to the ped's seat

    return true;
}
