#include "StdInc.h"

#include "TaskComplexFollowLeaderInFormation.h"

#include "PedGroup.h"
#include "PedGroups.h"
#include "TaskComplexSeekEntity.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskSimpleCarDrive.h"
#include "TaskSimplePause.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskComplexLeaveCar.h"
#include "ModelInfo.h"

void CTaskComplexFollowLeaderInFormation::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexFollowLeaderInFormation, 0x870c3c, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6949A0);
    RH_ScopedInstall(Destructor, 0x694A40);
    RH_ScopedInstall(CreateSubTask, 0x6962A0);

    RH_ScopedVMTInstall(Clone, 0x695740, { .Reversed = false });
    RH_ScopedVMTInstall(GetTaskType, 0x694A30, { .Reversed = false });
    RH_ScopedVMTInstall(CreateNextSubTask, 0x696820);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6968E0);
    RH_ScopedVMTInstall(ControlSubTask, 0x696940);
}

// 0x6949A0
CTaskComplexFollowLeaderInFormation::CTaskComplexFollowLeaderInFormation(CPedGroup* pedGroup, CPed* ped, const CVector& posn, float a5) : CTaskComplex() {
    m_Group  = pedGroup;
    m_Leader = ped;
    m_Pos    = posn;
    m_Int    = 4;
    m_Dist   = a5;
    if (m_Leader) {
        CEntity::RegisterReference(m_Leader);
    }
}

// For 0x695740
CTaskComplexFollowLeaderInFormation::CTaskComplexFollowLeaderInFormation(const CTaskComplexFollowLeaderInFormation& o) :
    CTaskComplexFollowLeaderInFormation{
        o.m_Group,
        o.m_Leader,
        o.m_Pos,
        o.m_Dist
    }
{
}

// 0x694A40
CTaskComplexFollowLeaderInFormation::~CTaskComplexFollowLeaderInFormation() {
    CEntity::SafeCleanUpRef(m_Leader);
}

// 0x6962A0
CTask* CTaskComplexFollowLeaderInFormation::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_ACHIEVE_HEADING: { // 0x386
        auto& membership = m_Group->GetMembership();

        CPed* member = membership.GetMember(CGeneral::GetRandomNumberInRange(0, (int32)membership.CountMembers()));
        if (member == ped) {
            member = membership.GetLeader();
        }

        if (!member || CGeneral::GetRandomNumberInRange(0, 50) != 20) {
            return new CTaskSimpleAchieveHeading{ ped->m_fCurrentRotation, 0.5f, 0.2f };
        }

        CVector dir = member->GetPosition() - ped->GetPosition();
        dir.Normalise();
        return new CTaskSimpleAchieveHeading{
            CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(dir.x, dir.y, 0.0f, 0.0f)),
            0.5f,
            0.2f
        };
    }
    case TASK_COMPLEX_SEEK_ENTITY: { // 0x38B
        auto* task = new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
            m_Leader,
            50'000,
            1'000,
            1.0f,
            2.0f,
            2.0f,
            false,
            true,
            CEntitySeekPosCalculatorXYOffset{m_Pos}
        };
        task->SetMoveState(PEDMOVE_SPRINT);
        return task;
    }
    case TASK_SIMPLE_CAR_DRIVE: // 0x2C5
        return new CTaskSimpleCarDrive{ ped->m_pVehicle, nullptr, false };
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER: // 0x2BC
        return new CTaskComplexEnterCarAsPassenger{ m_Leader->m_pVehicle, 0, false };
    case TASK_SIMPLE_PAUSE: // 0xCA
        return new CTaskSimplePause{ 2'000 };
    case TASK_SIMPLE_STAND_STILL: // 0xCB
        return new CTaskSimpleStandStill{ 500, false, false, 8.0f };
    case TASK_COMPLEX_LEAVE_CAR: // 0x2C0
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false };
    default:
        return nullptr;
    }
}

// 0x696820
CTask* CTaskComplexFollowLeaderInFormation::CreateNextSubTask(CPed* ped) {
    const auto leaderMoveState = m_Leader->m_nMoveState;

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_LEAVE_CAR:      // 0x2C0
    case TASK_SIMPLE_STAND_STILL:     // 0xCB
    case TASK_SIMPLE_CAR_DRIVE:       // 0x2C5
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    case TASK_SIMPLE_PAUSE: // 0xCA
        return CreateSubTask(TASK_SIMPLE_PAUSE, ped);
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER: // 0x2BC
        return CreateFirstSubTask(ped);
    case TASK_COMPLEX_SEEK_ENTITY: // 0x38B
        if (leaderMoveState == PEDMOVE_NONE || leaderMoveState == PEDMOVE_STILL) {
            return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
        }
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    default:
        return nullptr;
    }
}

// 0x6968E0
CTask* CTaskComplexFollowLeaderInFormation::CreateFirstSubTask(CPed* ped) {
    if (!ped->bInVehicle || !ped->m_pVehicle) {
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    }
    if (m_Leader->bInVehicle && m_Leader->m_pVehicle == ped->m_pVehicle) {
        return CreateSubTask(TASK_SIMPLE_CAR_DRIVE, ped);
    }
    return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
}

// 0x696940
CTask* CTaskComplexFollowLeaderInFormation::ControlSubTask(CPed* ped) {
    CTask* result = m_pSubTask;
    if (!m_Leader) {
        return nullptr;
    }

    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_CAR_DRIVE) {
        return result;
    }

    const auto dist2 = (m_Leader->GetPosition() - ped->GetPosition()).SquaredMagnitude();

    if (m_Leader->bInVehicle && m_Leader->m_pVehicle) {
        // Leader is in a vehicle
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SEEK_ENTITY) {
            const auto radius = m_Leader->m_pVehicle->GetModelInfo()->GetColModel()->GetBoundRadius();
            static_cast<CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>*>(m_pSubTask)->SetMaxEntityDistance(radius);

            if (m_Dist <= 0.0f) {
                return result;
            }
            if (dist2 <= sq(m_Dist)) {
                return result;
            }
            if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr)) {
                return result;
            }
            CTaskSimpleStandStill{ 0, false, false, 8.0f }.ProcessPed(ped);
            return new CTaskSimplePause{ 2'000 };
        }

        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
            return result;
        }

        if (CPedGroups::GetPedsGroup(ped)) {
            return result;
        }

        const auto veh      = m_Leader->m_pVehicle;
        const auto radius   = veh->GetModelInfo()->GetColModel()->GetBoundRadius();
        if (dist2 < sq(radius)) {
            if (veh->m_nMaxPassengers <= veh->m_nNumPassengers) {
                return result;
            }
            if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr)) {
                return result;
            }
            return new CTaskComplexEnterCarAsPassenger{ veh, 0, false };
        }
        if (sq(m_Dist) <= dist2) {
            return result;
        }
        if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr)) {
            return result;
        }
    } else {
        // Leader is on foot
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER) {
            if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr)) {
                return result;
            }
            return CreateFirstSubTask(ped);
        }
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_PAUSE
            && m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL
        ) {
            return result;
        }
        if (sq(m_Dist) <= dist2) {
            return result;
        }
    }

    return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
}
