#include "StdInc.h"

#include "TaskComplexKillPedOnFoot.h"
#include "TaskSimpleLeaveGroup.h"
#include "TaskSimplePause.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskComplexDragPedFromCar.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexKillPedOnFootMelee.h"
#include "TaskComplexKillPedOnFootArmed.h"
#include "TaskComplexDestroyCar.h"
#include "TaskComplexSignalAtPed.h"

void CTaskComplexKillPedOnFoot::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexKillPedOnFoot, 0x86D894, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x620E30);
    RH_ScopedInstall(CreateSubTask, 0x625E70);
}

CTaskComplexKillPedOnFoot::CTaskComplexKillPedOnFoot(
    CPed* target,
    int32 time,
    int32 pedFlags,
    int32 delay,
    int32 chance,
    uint8 nCompetence,
    bool bWaitForPlayerToBeSafe,
    bool bWaitingForPlayerToBeSafe
) :
    m_bWaitForPlayerToBeSafe{ bWaitForPlayerToBeSafe },
    m_bWaitingForPlayerToBeSafe{ bWaitingForPlayerToBeSafe },
    m_target{ target },
    m_pedFlags{ pedFlags },
    m_actionDelay{ delay },
    m_actionChance{ chance },
    m_nCompetence{ nCompetence },
    m_time{ time },
    m_startTime{ CTimer::GetTimeInMS() }
{
    CEntity::SafeRegisterRef(m_target);
}

CTaskComplexKillPedOnFoot::~CTaskComplexKillPedOnFoot() {
    CEntity::SafeCleanUpRef(m_target);
}

CTaskComplexKillPedOnFoot* CTaskComplexKillPedOnFoot::Constructor(CPed* target, int32 time, int32 pedFlags, int32 delay, int32 chance, int8 a7) {
    this->CTaskComplexKillPedOnFoot::CTaskComplexKillPedOnFoot(target, time, pedFlags, delay, chance, a7);
    return this;
}

bool CTaskComplexKillPedOnFoot::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return plugin::CallMethodAndReturn<bool, 0x625E40, CTaskComplexKillPedOnFoot*, CPed*, eAbortPriority, const CEvent*>(this, ped, priority, event);
}

CTask* CTaskComplexKillPedOnFoot::CreateNextSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x62B150, CTaskComplexKillPedOnFoot*, CPed*>(this, ped);
}

CTask* CTaskComplexKillPedOnFoot::CreateFirstSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x62B490, CTaskComplexKillPedOnFoot*, CPed*>(this, ped);
}

CTask* CTaskComplexKillPedOnFoot::ControlSubTask(CPed* ped) {
    return plugin::CallMethodAndReturn<CTask*, 0x626260, CTaskComplexKillPedOnFoot*, CPed*>(this, ped);
}

// 0x625E70
CTask* CTaskComplexKillPedOnFoot::CreateSubTask(int32 taskId, CPed* ped) {
    switch (taskId) {
    case TASK_NONE:
        return new CTaskSimpleLeaveGroup{};
    case TASK_SIMPLE_PAUSE: {
        CTaskSimpleStandStill{}.ProcessPed(ped);
        return m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe
            ? new CTaskSimplePause{10'000}
            : new CTaskSimplePause{2'000};
    }
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{0, false, false, 8.f};
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return new CTaskSimpleCarDriveTimed{ped->m_pVehicle, 2'000};
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
        auto* task = new CTaskComplexDragPedFromCar{m_target, 0};
        m_timer.Start(2'000);
        return task;
    }
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar{ped->m_pVehicle, 0, 0, true, true};
    case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE: {
        auto* task = new CTaskComplexKillPedOnFootMelee{m_target};
        m_timer.Start(2'000);
        return task;
    }
    case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED: {
        auto* task = new CTaskComplexKillPedOnFootArmed{m_target, (uint32)m_pedFlags, (uint32)m_actionDelay, (uint32)m_actionChance, (int8)m_nCompetence};
#ifdef FIX_BUGS
        if (task) {
            task->m_aimImmediate = m_bAimImmediate;
        }
#else
        task->m_aimImmediate = m_bAimImmediate;
#endif
        m_bAimImmediate = false;
        m_timer.Start(2'000);
        return task;
    }
    case TASK_COMPLEX_DESTROY_CAR:
        return new CTaskComplexDestroyCar{m_target->m_pVehicle, 0, 0, 0};
    case TASK_COMPLEX_SIGNAL_AT_PED: {
        if (FindPlayerPed(0) == m_target) {
            ped->Say(CTX_GLOBAL_PLAYER_WASTED);
        } else if (!CPedGroups::GetPedsGroup(m_target) || FindPlayerPed(0) != CPedGroups::GetPedsGroup(m_target)->GetMembership().GetLeader()) {
            ped->Say(CTX_GLOBAL_ENEMY_GANG_WASTED);
        }
        return new CTaskComplexSignalAtPed{m_target, CGeneral::GetRandomNumberInRange(0, 1500), true};
    }
    case TASK_FINISHED:
        ped->bDontAcceptIKLookAts = false;
        return nullptr;
    default:
        return nullptr;
    }
}
