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
#include "TaskComplexCarSlowBeDraggedOut.h"

void CTaskComplexLeaveCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexLeaveCar, 0x86E828, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(CreateSubTask, 0x641530);
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
    return plugin::CallMethodAndReturn<bool, 0x641100, CTask*, CPed*, int32, const CEvent*>(this, ped, priority, event);
}

// 0x6419F0
CTask* CTaskComplexLeaveCar::CreateNextSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x6419F0, CTask*, CPed*>(this, ped);
}

// 0x641FC0
CTask* CTaskComplexLeaveCar::CreateFirstSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x641FC0, CTask*, CPed*>(this, ped);
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
    case TASK_SIMPLE_CAR_FORCE_PED_OUT: { // 0x32F
        // NOTE: The class of this task type has not been reversed yet (ctor `0x647710`, vtable `0x86EE94`, size `0x10`,
        //       stores the vehicle at `+0x08` and the door at `+0x0C`), so the exe's ctor is called on a task pool allocation.
        const auto task = static_cast<CTask*>(CTask::operator new(0x10));
        if (!task) {
            return nullptr;
        }
        plugin::CallMethod<0x647710, CTask*, CVehicle*, int32>(task, m_pTargetVehicle, m_nTargetDoor);
        return task;
    }
    case TASK_SIMPLE_CAR_SET_PED_OUT: // 0x330
        return new CTaskSimpleCarSetPedOut(m_pTargetVehicle, static_cast<eTargetDoor>(m_nTargetDoor), m_bSensibleLeaveCar);
    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT: // 0x337
        return new CTaskComplexCarSlowBeDraggedOut(m_pTargetVehicle, static_cast<eTargetDoor>(m_nTargetDoor), true);
    default:
        return nullptr;
    }
}
