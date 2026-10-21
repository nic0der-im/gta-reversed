#include "StdInc.h"
#include "TaskComplexEnterCarAsPassengerWait.h"

#include "TaskComplexEnterCarAsPassengerTimed.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleWaitUntilPedIsInCar.h"

#include <reversiblebugfixes/Bugs.hpp>

void CTaskComplexEnterCarAsPassengerWait::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexEnterCarAsPassengerWait, 0x86E778, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x63B320);
    RH_ScopedInstall(Destructor, 0x63B3C0);

    RH_ScopedInstall(CreateSubTask, 0x6408D0);
    RH_ScopedVMTInstall(Clone, 0x63D850);
    RH_ScopedVMTInstall(GetTaskType, 0x63B3B0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x643E10);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x643F00);
    RH_ScopedVMTInstall(ControlSubTask, 0x640730);
}

// 0x63B320
CTaskComplexEnterCarAsPassengerWait::CTaskComplexEnterCarAsPassengerWait(CVehicle* target, CPed* waitFor, bool forceFrontSeat, eMoveState ms) :
    m_Car{target},
    m_WaitForPed{waitFor},
    m_bForceFrontSeat{forceFrontSeat},
    m_MoveState{ms}
{
    CEntity::SafeRegisterRef(m_Car);
    CEntity::SafeRegisterRef(m_WaitForPed);
}
    
// NOTSA (for 0x63D850)
CTaskComplexEnterCarAsPassengerWait::CTaskComplexEnterCarAsPassengerWait(const CTaskComplexEnterCarAsPassengerWait& o) :
    CTaskComplexEnterCarAsPassengerWait{
        o.m_Car,
        o.m_WaitForPed,
        o.m_bForceFrontSeat,
        o.m_MoveState
    }
{
}

// 0x63B3C0
CTaskComplexEnterCarAsPassengerWait::~CTaskComplexEnterCarAsPassengerWait() {
    CEntity::SafeCleanUpRef(m_Car);
    CEntity::SafeCleanUpRef(m_WaitForPed);
}

// 0x6408D0
CTask* CTaskComplexEnterCarAsPassengerWait::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_WAIT_UNTIL_PED_IN_CAR:
        return new CTaskSimpleWaitUntilPedIsInCar{ m_WaitForPed };
    case TASK_COMPLEX_SEQUENCE: {
        const auto sequence = new CTaskComplexSequence{};
        sequence->AddTask(new CTaskComplexTurnToFaceEntityOrCoord{ m_Car, 0.5f, 0.2f });
        sequence->AddTask(new CTaskSimpleStandStill{ 1000, false, false, 8.0f });
        return sequence;
    }
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER_TIMED: {
        const auto targetSeat = m_bForceFrontSeat
            ? static_cast<uint32>(CCarEnterExit::ComputeTargetDoorToEnterAsPassenger(m_Car, 0))
            : 0u;
        const auto task = static_cast<CTaskComplexEnterCarAsPassengerTimed*>(CTask::operator new(sizeof(CTaskComplexEnterCarAsPassengerTimed)));
        // NOTE: Placement new is used because a plain `new` would throw on task pool exhaustion instead of returning null
        if (task) {
            ::new (task) CTaskComplexEnterCarAsPassengerTimed{ m_Car, targetSeat, 0xFFFFFFFFu, true };
        } else if (notsa::bugfixes::GenericCrashing) {
            return nullptr;
        }
        task->SetMoveState(m_MoveState); // Retail writes through the null task here (Only if the task pool is exhausted)
        return task;
    }
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return new CTaskSimpleCarDriveTimed{ m_Car, 0 };
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: {
        CVector point;
        if (CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_Car, point)) {
            return new CTaskComplexGoToPointAndStandStill{ m_MoveState, point, 1.5f, 2.0f };
        }
        const auto radius = CModelInfo::GetModelInfo(m_Car->m_nModelIndex)->GetColModel()->GetBoundRadius() + 1.5f;
        return new CTaskComplexGoToPointAndStandStill{ m_MoveState, m_Car->GetPosition(), radius, 2.0f };
    }
    default:
        return nullptr;
    }
}

// 0x643E10
CTask* CTaskComplexEnterCarAsPassengerWait::CreateNextSubTask(CPed* ped) {
    if (!m_Car) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER_TIMED:
        if (!ped->bInVehicle && ++m_EnterCarFails <= 15 && m_Car) {
            return CreateSubTask(TASK_COMPLEX_SEQUENCE, ped);
        }
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_COMPLEX_SEQUENCE:
    case TASK_SIMPLE_WAIT_UNTIL_PED_IN_CAR:
        return CreateSubTask(m_Car ? TASK_COMPLEX_ENTER_CAR_AS_PASSENGER_TIMED : TASK_FINISHED, ped);
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
        return CreateSubTask(m_WaitForPed ? TASK_SIMPLE_WAIT_UNTIL_PED_IN_CAR : TASK_FINISHED, ped);
    default:
        return nullptr;
    }
}

// 0x643F00
CTask* CTaskComplexEnterCarAsPassengerWait::CreateFirstSubTask(CPed* ped) {
    if (ped->bInVehicle && ped->m_pVehicle && ped->m_pVehicle == m_Car && m_Car->IsPassenger(ped)) {
        return CreateSubTask(TASK_SIMPLE_CAR_DRIVE_TIMED, ped);
    }

    if (!m_WaitForPed) {
        return CreateSubTask(m_Car ? TASK_COMPLEX_ENTER_CAR_AS_PASSENGER_TIMED : TASK_FINISHED, ped);
    }

    return CreateSubTask(m_Car ? TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL : TASK_FINISHED, ped);
}

// 0x640730
CTask* CTaskComplexEnterCarAsPassengerWait::ControlSubTask(CPed* ped) {
    if (!m_bPlayedSample) {
        if (m_WaitForPed) {
            if (m_WaitForPed->IsPlayer() && ped->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_GET_IN)) {
                m_bPlayedSample = true;
                ped->Say(CTX_GLOBAL_CAR_GET_IN);
            }
        } else if (const auto group = CPedGroups::GetPedsGroup(ped)) {
            if (const auto leader = group->GetMembership().GetLeader(); leader && leader->IsPlayer() && ped->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_GET_IN)) {
                m_bPlayedSample = true;
                ped->Say(CTX_GLOBAL_CAR_WAIT_FOR_ME);
            }
        }
    }

    if (m_Car && m_pSubTask->GetTaskType() == TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) {
        const auto goToTask = notsa::cast<CTaskComplexGoToPointAndStandStill>(m_pSubTask);

        CVector point;
        const auto hasPoint = CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_Car, point);

        // Retail accumulates this in x87 extended precision
        const auto& target = goToTask->m_vecTargetPoint;
        const auto  dx     = static_cast<double>(point.x) - target.x;
        const auto  dy     = static_cast<double>(point.y) - target.y;
        const auto  dz     = static_cast<double>(point.z) - target.z;
        if (dx * dx + dy * dy + dz * dz > 0.025f) {
            if (hasPoint) {
                goToTask->GoToPoint(point, 1.5f, 2.0f, false);
            } else {
                const auto radius = CModelInfo::GetModelInfo(m_Car->m_nModelIndex)->GetColModel()->GetBoundRadius() + 1.5f;
                goToTask->GoToPoint(m_Car->GetPosition(), radius, 2.0f, false);
            }
        }
    }

    return m_pSubTask;
}
