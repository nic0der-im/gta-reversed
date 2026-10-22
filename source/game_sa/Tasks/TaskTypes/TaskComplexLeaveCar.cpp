#include "StdInc.h"

#include "TaskComplexLeaveCar.h"

#include "TaskSimplePause.h"
#include "TaskComplexGetUpAndStandStill.h"
#include "TaskSimpleDie.h"
#include "TaskComplexLeaveBoat.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskSimpleCarCloseDoorFromOutside.h"
#include "TaskSimpleCarWaitToSlowDown.h"
#include "TaskSimpleCarWaitForDoorNotToBeInUse.h"
#include "TaskSimpleCarGetOut.h"
#include "TaskSimpleCarJumpOut.h"
#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimpleCarForcePedOut.h"
#include "TaskComplexCarSlowBeDraggedOut.h"
#include "TaskUtilityLineUpPedWithCar.h"
#include "CarEnterExit.h"
#include "PedGroups.h"
#include "EventGroupEvent.h"
#include "Events/EventLeaderExitedCarAsDriver.h"
#include "Automobile.h"
#include "DamageManager.h"
#include "AnimManager.h"

// 0x646CF0
//! Query the "getting in/out" state of the given door
static void ComputeGettingInOutFlagsForDoor(const CVehicle* vehicle, int32 door, bool& gettingIn, bool& gettingOut) {
    uint8 flag;
    switch (door) {
    case 8:  flag = 4; break;
    case 9:  flag = 8; break;
    case 10: flag = 1; break;
    case 11: flag = 2; break;
    default: return;
    }
    if (vehicle->m_nGettingInFlags & flag) {
        gettingIn = true;
    }
    if (vehicle->m_nGettingOutFlags & flag) {
        gettingOut = true;
    }
}

void CTaskComplexLeaveCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexLeaveCar, 0x86E828, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(CreateSubTask, 0x641530);

    // 0x641100
    RH_ScopedVMTInstall(MakeAbortable, 0x641100);
    // 0x6419F0
    RH_ScopedVMTInstall(CreateNextSubTask, 0x6419F0);
    // 0x641FC0
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x641FC0);
}

// 0x62F1A0
CTaskComplexLeaveCar::CTaskComplexLeaveCar(CVehicle* targetVehicle, int32 nTargetDoor, int32 nDelayTime) : CTaskComplexLeaveCar(targetVehicle, nTargetDoor, nDelayTime, false, true) {
    m_bDie = true;
}

// 0x63B8C0
CTaskComplexLeaveCar::CTaskComplexLeaveCar(CVehicle* targetVehicle, int32 nTargetDoor, int32 nDelayTime, bool bSensibleLeaveCar, bool bForceGetOut) : CTaskComplex() {
    m_nTargetDoor                  = nTargetDoor;
    m_nDelayTime                   = nDelayTime;
    m_bSensibleLeaveCar            = bSensibleLeaveCar;
    m_pTargetVehicle               = targetVehicle;
    m_bForceGetOut                 = bForceGetOut;
    m_bDie                         = false;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    m_nDoorFlagsSet                = 0;
    m_nNumGettingInSet             = 0;
    m_nDieAnimID                   = ANIM_ID_KO_SHOT_FRONT_0;
    m_fDieAnimBlendDelta           = 4.0f;
    m_fDieAnimSpeed                = 1.0f;
    m_bIsInAir                     = false;

    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

// 0x63B970
CTaskComplexLeaveCar::~CTaskComplexLeaveCar() {
    if (m_pTargetVehicle) {
        m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsSet);
        m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInSet;
        m_pTargetVehicle->CleanUpOldReference(reinterpret_cast<CEntity**>(&m_pTargetVehicle));
    }

    delete m_pTaskUtilityLineUpPedWithCar;
}

CTaskComplexLeaveCar::CTaskComplexLeaveCar(const CTaskComplexLeaveCar& o) :
    CTaskComplexLeaveCar{ o.m_pTargetVehicle, o.m_nTargetDoor, o.m_nDelayTime, o.m_bSensibleLeaveCar, o.m_bForceGetOut }
{
}

// 0x641100
bool CTaskComplexLeaveCar::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (!m_pTargetVehicle) {
        return true;
    }

    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);

        if (ped->bInVehicle) {
            ComputeTargetDoorToExitIfNotSet(ped);

            // NOTE: These throw-away tasks are used for their side effects only
            CTaskSimpleCarCloseDoorFromOutside{ m_pTargetVehicle, (uint32)m_nTargetDoor, nullptr }.MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);
            CTaskSimpleCarSetPedOut{ m_pTargetVehicle, (eTargetDoor)m_nTargetDoor, m_bSensibleLeaveCar }.MakeAbortable(ped);
        }

        ClearGettingInOutFlags();
        return true;
    }

    if (priority != ABORT_PRIORITY_URGENT) {
        return false;
    }

    const auto subTaskType = m_pSubTask->GetTaskType();
    if (subTaskType != TASK_SIMPLE_DIE) {
        if ((subTaskType == TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN || subTaskType == TASK_COMPLEX_GET_UP_AND_STAND_STILL)
            && event
            && event->GetEventType() == EVENT_DAMAGE
            && static_cast<const CEventDamage*>(event)->HasKilledPed()
        ) {
            if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event)) {
                return false;
            }
            ClearGettingInOutFlags();
            return true;
        }

        if (subTaskType == TASK_SIMPLE_CAR_JUMP_OUT && event) {
            const auto eventType = event->GetEventType();
            if (eventType == EVENT_IN_AIR || eventType == EVENT_IN_WATER) {
                if (eventType == EVENT_IN_AIR) {
                    m_bIsInAir = true;
                }
                if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event)) {
                    return false;
                }

                float volume;
                if (ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_Type == WEAPON_PARACHUTE
                    && eventType == EVENT_IN_AIR
                    && m_pTargetVehicle->GetNumContactWheels() == 0
                ) {
                    if (m_pTargetVehicle->m_nVehicleSubType != VEHICLE_TYPE_PLANE
                        || m_pTargetVehicle->m_vecMoveSpeed.Magnitude() <= 0.2f
                    ) {
                        if (!ped->IsPlayer()) {
                            return false;
                        }

                        CColPoint colPoint;
                        CEntity*  entity;
                        if (CWorld::ProcessVerticalLine(ped->GetPosition(), -10.f, colPoint, entity, true, false, false, false, true, false, nullptr)) {
                            return false;
                        }
                    }
                    volume = 8.f;
                } else if (eventType == EVENT_IN_WATER) {
                    if (-0.3f <= ped->m_vecMoveSpeed.z) {
                        return false;
                    }
                    volume = 16.f;
                } else {
                    return false;
                }

                CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_FALL_FRONT, volume);
                return false;
            }
        }

        if (!event || m_pSubTask->GetTaskType() != TASK_SIMPLE_CAR_DRIVE_TIMED) {
            return false;
        }
        return m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event);
    }

    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_DRIVE_TIMED) {
        return false;
    }

    if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, event)) {
        return false;
    }

    ClearGettingInOutFlags();
    return true;
}

// 0x6419F0
CTask* CTaskComplexLeaveCar::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN: {
        if (!m_pTargetVehicle->CanPedStepOutCar(false)) {
            ComputeAndSetGettingOutFlags(ped);
            ped->SetPedState(PEDSTATE_NONE);
            return CreateSubTask(TASK_SIMPLE_CAR_JUMP_OUT, ped);
        }

        if (!CCarEnterExit::IsRoomForPedToLeaveCar(m_pTargetVehicle, m_nTargetDoor)) {
            // The current door is blocked - Try the one on the opposite side
            int32 altDoor;
            switch (m_nTargetDoor) {
            case 8: // Driver's door
                if (m_pTargetVehicle->m_pDriver || (m_pTargetVehicle->m_nGettingInFlags & 1)) {
                    goto forcePedOut;
                }
                altDoor = 10;
                break;
            case 9:
                if (m_pTargetVehicle->m_apPassengers[1] || (m_pTargetVehicle->m_nGettingInFlags & 2)) {
                    goto forcePedOut;
                }
                altDoor = 11;
                break;
            case 10:
                if (m_pTargetVehicle->m_nVehicleType != VEHICLE_TYPE_BIKE
                    && !m_pTargetVehicle->m_pHandlingData->m_bTandemSeats
                    && (m_pTargetVehicle->m_apPassengers[0] || (m_pTargetVehicle->m_nGettingInFlags & 4))
                ) {
                    goto forcePedOut;
                }
                altDoor = 8;
                break;
            case 11:
                if (m_pTargetVehicle->m_nVehicleType != VEHICLE_TYPE_BIKE
                    && !m_pTargetVehicle->m_pHandlingData->m_bTandemSeats
                    && (m_pTargetVehicle->m_apPassengers[2] || (m_pTargetVehicle->m_nGettingInFlags & 8))
                ) {
                    goto forcePedOut;
                }
                altDoor = 9;
                break;
            default:
                goto forcePedOut;
            }

            if (!CCarEnterExit::IsRoomForPedToLeaveCar(m_pTargetVehicle, altDoor)
                || (m_pTargetVehicle->m_pHandlingData->m_bForceDoorCheck
                    && m_pTargetVehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE
                    && !m_pTargetVehicle->AsAutomobile()->m_aCarNodes[altDoor])
            ) {
                goto forcePedOut;
            }

            m_nTargetDoor = altDoor;
        }

        ComputeAndSetGettingOutFlags(ped);
        ped->SetPedState(PEDSTATE_NONE);

        if (m_bDie) {
            return CreateSubTask(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT, ped);
        }
        CreateUtilityLineUpPedWithCar();
        return CreateSubTask(TASK_SIMPLE_CAR_GET_OUT, ped);
    }

    forcePedOut:
        ComputeAndSetGettingOutFlags(ped);
        ped->SetPedState(PEDSTATE_NONE);
        CreateUtilityLineUpPedWithCar();
        return CreateSubTask(TASK_SIMPLE_CAR_FORCE_PED_OUT, ped);

    case TASK_COMPLEX_LEAVE_BOAT:
        if (m_bDie) {
            return CreateSubTask(TASK_SIMPLE_DIE, ped);
        }
        break;

    case TASK_SIMPLE_PAUSE:
        break;

    case TASK_COMPLEX_GET_UP_AND_STAND_STILL:
        if (ped->bInVehicle) {
            return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
        }
        break;

    case TASK_SIMPLE_DIE:
        break;

    case TASK_SIMPLE_CAR_DRIVE_TIMED: {
        if (!ped->bInVehicle) {
            break;
        }
        if (!m_pTargetVehicle->IsPassenger(ped) && !m_pTargetVehicle->IsDriver(ped)) {
            ped->bInVehicle = false;
            break;
        }

        ComputeTargetDoorToExitIfNotSet(ped);

        bool gettingIn{}, gettingOut{};
        ComputeGettingInOutFlagsForDoor(m_pTargetVehicle, m_nTargetDoor, gettingIn, gettingOut);

        if (m_bForceGetOut) {
            return CreateSubTask(TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN, ped);
        }
        if (gettingOut) {
            return CreateSubTask(TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE, ped);
        }
        if (gettingIn) {
            ped->bInVehicle = false;
            break;
        }
        return CreateSubTask(TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN, ped);
    }

    case TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE:
        m_nDieAnimID = ANIM_ID_KO_SHOT_FRONT_0; // Anim id `0xF`
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);

    case TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE:
        return CreateSubTask(TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN, ped);

    case TASK_SIMPLE_CAR_GET_OUT: {
        m_pTaskUtilityLineUpPedWithCar->m_nDoorOpenPosType = 2;

        if ((!m_bForceGetOut || m_pTargetVehicle->m_nDoorLock != CARLOCK_UNLOCKED)
            && CCarEnterExit::CarHasDoorToClose(m_pTargetVehicle, m_nTargetDoor)
        ) {
            return CreateSubTask(TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE, ped);
        }

        if (m_pTargetVehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE
            && CCarEnterExit::CarHasDoorToClose(m_pTargetVehicle, m_nTargetDoor)
        ) {
            auto& damageManager = m_pTargetVehicle->AsAutomobile()->GetDamageManager();
            const auto doorStatus = damageManager.GetDoorStatus_Component((tComponent)m_nTargetDoor);
            if (doorStatus == DAMSTATE_OK || doorStatus == DAMSTATE_DAMAGED) {
                damageManager.SetDoorStatus_Component((tComponent)m_nTargetDoor, (eDoorStatus)(doorStatus + 1));
            }
        }

        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
    }

    case TASK_SIMPLE_CAR_FORCE_PED_OUT:
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);

    case TASK_SIMPLE_CAR_JUMP_OUT: {
        if (m_bDie) {
            m_nDieAnimID         = ANIM_ID_KO_SHOT_FRONT_0; // Anim id `0xF`
            m_fDieAnimBlendDelta = 1000.f;
            m_fDieAnimSpeed      = 0.5f;
            return CreateSubTask(TASK_SIMPLE_DIE, ped);
        }

        if (!m_pTargetVehicle || m_pTargetVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE || m_pTargetVehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
            break;
        }

        if (m_bIsInAir) {
            return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
        }
        if (ped->bIsDrowning) {
            return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
        }
        ClearGettingInOutFlags();
        return CreateSubTask(TASK_COMPLEX_GET_UP_AND_STAND_STILL, ped);
    }

    case TASK_SIMPLE_CAR_SET_PED_OUT:
        if (m_bDie) {
            return CreateSubTask(TASK_SIMPLE_DIE, ped);
        }
        break;

    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT:
        m_nDieAnimID         = 0x24; // TODO(re): Anim id `0x24`
        m_fDieAnimBlendDelta = 1000.f;
        m_fDieAnimSpeed      = 0.5f;
        if (m_pTargetVehicle && m_pTargetVehicle->m_nVehicleSubType == VEHICLE_TYPE_BOAT) {
            m_pTargetVehicle->m_nVehicleSubType = VEHICLE_TYPE_MTRUCK;
        }
        return CreateSubTask(TASK_SIMPLE_DIE, ped);

    default:
        return nullptr;
    }

    return CreateSubTask(TASK_FINISHED, ped);
}

// 0x641FC0
CTask* CTaskComplexLeaveCar::CreateFirstSubTask(CPed* ped) {
    if ((ped->m_nPedState == PEDSTATE_DEAD || ped->bIsBeingArrested) && ped->IsPlayer()) {
        return new CTaskSimplePause(-1);
    }

    if (!ped->bInVehicle) {
        printf("CTaskComplexLeaveCar - ped not in car");
        return new CTaskSimplePause(-1);
    }

    if (m_pTargetVehicle->m_pDriver == ped) {
        if (CPedGroup* const group = CPedGroups::GetPedsGroup(ped); group && group->GetMembership().IsLeader(ped)) {
            group->GetIntelligence().AddEvent(CEventGroupEvent{ ped, new CEventLeaderExitedCarAsDriver{} });
        }

        if (!ped->IsPlayer()) {
            ped->SetRadioStation();
        } else if (ped->m_pVehicle) {
            ped->m_pVehicle->m_vehicleAudio.PlayerAboutToExitVehicleAsDriver();
        }
    }

    if (m_pTargetVehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
        return new CTaskComplexLeaveBoat(m_pTargetVehicle, 0);
    }
    return new CTaskSimpleCarDriveTimed(m_pTargetVehicle, m_nDelayTime);
}

// 0x6421B0
CTask* CTaskComplexLeaveCar::ControlSubTask(CPed* ped) {
    if (!m_pTargetVehicle) {
        return nullptr;
    }

    const auto subTaskType = m_pSubTask->GetTaskType();
    if (!ped->bInVehicle
        && subTaskType != TASK_SIMPLE_CAR_SET_PED_OUT
        && subTaskType != TASK_SIMPLE_CAR_JUMP_OUT
        && subTaskType != TASK_COMPLEX_GET_UP_AND_STAND_STILL
        && subTaskType != TASK_SIMPLE_DIE
    ) {
        return CreateSubTask(TASK_SIMPLE_CAR_SET_PED_OUT, ped);
    }

    if (!m_bSensibleLeaveCar && subTaskType == TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN) {
        m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
    }

    return m_pSubTask;
}

// 0x641530
CTask* CTaskComplexLeaveCar::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_PAUSE: // 0xCA
        return new CTaskSimplePause(-1);
    case TASK_COMPLEX_GET_UP_AND_STAND_STILL: // 0xCE
        return new CTaskComplexGetUpAndStandStill();
    case TASK_SIMPLE_DIE: // 0xD4
        return new CTaskSimpleDie(ANIM_GROUP_DEFAULT, static_cast<AnimationId>(m_nDieAnimID), m_fDieAnimBlendDelta, m_fDieAnimSpeed);
    case TASK_COMPLEX_LEAVE_BOAT: // 0x2CC
        return new CTaskComplexLeaveBoat(m_pTargetVehicle, 0);
    case TASK_SIMPLE_CAR_DRIVE_TIMED: // 0x2CF
        return new CTaskSimpleCarDriveTimed(m_pTargetVehicle, m_nDelayTime);
    case TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE: // 0x326
        return new CTaskSimpleCarCloseDoorFromOutside(m_pTargetVehicle, m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar);
    case TASK_SIMPLE_CAR_WAIT_TO_SLOW_DOWN: { // 0x329
        if (m_bDie || !m_bSensibleLeaveCar) {
            return new CTaskSimpleCarWaitToSlowDown(m_pTargetVehicle, CTaskSimpleCarWaitToSlowDown::SlowDownType::DONT_WAIT);
        }
        return new CTaskSimpleCarWaitToSlowDown(
            m_pTargetVehicle,
            ped->IsPlayer()
                ? CTaskSimpleCarWaitToSlowDown::SlowDownType::SLOW_ENOUGH_TO_STEP_OR_JUMP
                : CTaskSimpleCarWaitToSlowDown::SlowDownType::SLOW_ENOUGH_TO_STEP
        );
    }
    case TASK_SIMPLE_CAR_WAIT_FOR_DOOR_NOT_TO_BE_IN_USE: // 0x32A
        return new CTaskSimpleCarWaitForDoorNotToBeInUse(m_pTargetVehicle, m_nTargetDoor, 0);
    case TASK_SIMPLE_CAR_GET_OUT: // 0x32D
        return new CTaskSimpleCarGetOut(m_pTargetVehicle, m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar);
    case TASK_SIMPLE_CAR_JUMP_OUT: // 0x32E
        return new CTaskSimpleCarJumpOut(m_pTargetVehicle, m_nTargetDoor, m_pTaskUtilityLineUpPedWithCar);
    case TASK_SIMPLE_CAR_FORCE_PED_OUT: // 0x32F
        return new CTaskSimpleCarForcePedOut(m_pTargetVehicle, m_nTargetDoor);
    case TASK_SIMPLE_CAR_SET_PED_OUT: // 0x330
        return new CTaskSimpleCarSetPedOut(m_pTargetVehicle, static_cast<eTargetDoor>(m_nTargetDoor), m_bSensibleLeaveCar);
    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT: // 0x337
        return new CTaskComplexCarSlowBeDraggedOut(m_pTargetVehicle, static_cast<eTargetDoor>(m_nTargetDoor), true);
    default:
        return nullptr;
    }
}

// 0x63BA00
void CTaskComplexLeaveCar::ComputeAndSetGettingOutFlags(CPed* ped) {
    m_nDoorFlagsSet = (uint8)CCarEnterExit::ComputeDoorFlag(m_pTargetVehicle, m_nTargetDoor, true);
    m_pTargetVehicle->SetGettingOutFlags(m_nDoorFlagsSet);
    m_nNumGettingInSet = 1;
    m_pTargetVehicle->m_nNumGettingIn++;

    if (m_pTargetVehicle->m_pDriver && !m_pTargetVehicle->m_pDriver->IsPlayer() && ped == m_pTargetVehicle->m_pDriver && m_bSensibleLeaveCar) {
        m_pTargetVehicle->m_autoPilot.m_nCruiseSpeed = 0;
        m_pTargetVehicle->m_autoPilot.m_nCarMission  = MISSION_NONE;
    }

    if (ped->IsPlayer() && ped == m_pTargetVehicle->m_pDriver) {
        m_pTargetVehicle->SetStatus(STATUS_FORCED_STOP);
    }
}

// 0x63BAB0
void CTaskComplexLeaveCar::ComputeTargetDoorToExitIfNotSet(CPed* ped) {
    if (!m_nTargetDoor) {
        m_nTargetDoor = CCarEnterExit::ComputeTargetDoorToExit(m_pTargetVehicle, ped);
    }
}

// 0x63BAE0
void CTaskComplexLeaveCar::CreateUtilityLineUpPedWithCar() {
    m_pTaskUtilityLineUpPedWithCar = new CTaskUtilityLineUpPedWithCar(CVector{}, 0, 0, m_nTargetDoor);
}

void CTaskComplexLeaveCar::ClearGettingInOutFlags() {
    m_pTargetVehicle->ClearGettingOutFlags(m_nDoorFlagsSet);
    m_nDoorFlagsSet = 0;
    m_pTargetVehicle->m_nNumGettingIn -= m_nNumGettingInSet;
    m_nNumGettingInSet = 0;
}
