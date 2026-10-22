#include "StdInc.h"
#include "TaskComplexSmartFleePoint.h"

#include "TaskComplexWanderFlee.h"
#include "TaskComplexWander.h"
#include "TaskComplexLeaveAnyCar.h"
#include "TaskComplexSequence.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleTired.h"
#include "Ragdoll/IKChainManager.h"

namespace {
//! 0x669D30 - `CTaskComplexWander::SetDir` (Not hooked)
void WanderSetDir(CTaskComplexWander* wander, uint8 dir) {
    wander->m_nDir = dir;
    wander->m_bNewDir = true;
}

//! 0x669D50 - `CTaskComplexWander::SetPathNodes` (Not hooked)
void WanderSetPathNodes(CTaskComplexWander* dst, const CTaskComplexWander* src) {
    if (dst->m_LastNode != src->m_LastNode || dst->m_NextNode != src->m_NextNode) {
        dst->m_LastNode = src->m_LastNode;
        dst->m_NextNode = src->m_NextNode;
        dst->m_nDir     = src->m_nDir;
        dst->m_bNewNodes = true;
    }
}
}

void CTaskComplexSmartFleePoint::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexSmartFleePoint, 0x86f744, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65BD20);
    RH_ScopedInstall(Destructor, 0x65BDB0);

    RH_ScopedInstall(SetDefaultTaskWanderDir, 0x65BE00);
    RH_ScopedInstall(ComputeFleeDir, 0x65BE40);
    RH_ScopedInstall(CreateSubTask, 0x65BE80);
    RH_ScopedInstall(SetFleePosition, 0x65C3C0);

    RH_ScopedVMTInstall(Clone, 0x65CED0);
    RH_ScopedVMTInstall(GetTaskType, 0x65BDA0);
    RH_ScopedVMTInstall(MakeAbortable, 0x65BDC0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x65C0C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x65C140);
    RH_ScopedVMTInstall(ControlSubTask, 0x65C1E0);
}

// 0x65BD20
CTaskComplexSmartFleePoint::CTaskComplexSmartFleePoint(CVector const& fleePos, bool doScream, float safeDist, int32 fleeTimeMs) :
    m_fleePoint{fleePos},
    m_doScream{doScream},
    m_safeDist{safeDist},
    m_fleeTimeMs{fleeTimeMs}
{
    m_timer.Start(m_fleeTimeMs);
}

CTaskComplexSmartFleePoint::CTaskComplexSmartFleePoint(const CTaskComplexSmartFleePoint& o)
    : CTaskComplexSmartFleePoint{o.m_fleePoint, o.m_doScream, o.m_safeDist, o.m_fleeTimeMs}
{
}

// 0x65BE00
int8 CTaskComplexSmartFleePoint::SetDefaultTaskWanderDir(CPed* ped) {
    auto* const activeTask = ped->GetTaskManager().m_aPrimaryTasks[TASK_PRIMARY_DEFAULT];
    if (activeTask && activeTask->GetTaskType() == TASK_COMPLEX_WANDER) {
        WanderSetPathNodes(
            static_cast<CTaskComplexWander*>(activeTask),
            static_cast<const CTaskComplexWander*>(m_pSubTask)
        );
    }
    // TODO(re): The original never writes AL; the header's `int8` return value is unverified.
    return 0;
}

// 0x65BE40
uint32 CTaskComplexSmartFleePoint::ComputeFleeDir(CPed* ped) {
    const auto& pedPos = ped->GetPosition();
    return CGeneral::GetNodeHeadingFromVector(pedPos.x - m_fleePoint.x, pedPos.y - m_fleePoint.y);
}

// 0x65BE80
CTask* CTaskComplexSmartFleePoint::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_COMPLEX_LEAVE_ANY_CAR:
        return new CTaskComplexLeaveAnyCar{0, false, true};
    case TASK_SIMPLE_STAND_STILL:
        SetDefaultTaskWanderDir(ped);
        return new CTaskSimpleStandStill{0, false, false, 8.f};
    case TASK_COMPLEX_SEQUENCE:
        if (m_moveState == PEDMOVE_RUN) {
            auto* const sequence = new CTaskComplexSequence{};
            sequence->AddTask(new CTaskSimpleRunAnim{ANIM_GROUP_DEFAULT, ANIM_ID_FLEE_LKAROUND_01});
            sequence->AddTask(new CTaskSimpleTired{2000});
            return sequence;
        }
        break;
    case TASK_COMPLEX_WANDER:
        return new CTaskComplexWanderFlee{m_moveState, m_fleeDir};
    default:
        break;
    }
    return nullptr;
}

// 0x65C3C0
int8 CTaskComplexSmartFleePoint::SetFleePosition(CVector const& a2, float a3, bool a4) {
    if (m_fleePoint.x == a2.x && m_fleePoint.y == a2.y && m_fleePoint.z == a2.z && m_safeDist == a3) {
        m_doScream = a4;
        // TODO(re): The original never writes AL; the header's `int8` return value is unverified.
        return 0;
    }

    m_fleePoint        = a2;
    m_safeDist         = a3;
    m_hasFleePointChanged = true;
    m_doScream         = a4;
    // TODO(re): The original never writes AL; the header's `int8` return value is unverified.
    return 0;
}

// 0x65BDC0
bool CTaskComplexSmartFleePoint::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority == ABORT_PRIORITY_LEISURE) {
        m_fleeTimeMs            = -1;
        m_timer.m_nStartTime    = CTimer::GetTimeInMS();
        m_timer.m_nInterval     = -1;
        m_timer.m_bStarted      = true;
    }
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x65C0C0
CTask* CTaskComplexSmartFleePoint::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_STAND_STILL:
        return CreateSubTask(TASK_COMPLEX_SEQUENCE, ped);
    case TASK_COMPLEX_SEQUENCE:
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_COMPLEX_LEAVE_ANY_CAR:
        m_fleeDir = ComputeFleeDir(ped);
        return CreateSubTask(TASK_COMPLEX_WANDER, ped);
    default:
        return nullptr;
    }
}

// 0x65C140
CTask* CTaskComplexSmartFleePoint::CreateFirstSubTask(CPed* ped) {
    m_initalPos = ped->GetPosition();

    if (ped->IsInVehicle()) {
        return CreateSubTask(TASK_COMPLEX_LEAVE_ANY_CAR, ped);
    }

    m_fleeDir = ComputeFleeDir(ped);
    return CreateSubTask(TASK_COMPLEX_WANDER, ped);
}

// 0x65C1E0
CTask* CTaskComplexSmartFleePoint::ControlSubTask(CPed* ped) {
    CTask* subTask = m_pSubTask;

    if (subTask->GetTaskType() != TASK_COMPLEX_WANDER) {
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }
        return subTask;
    }

    if (m_doScream) {
        ped->Say(CTX_GLOBAL_PAIN_PANIC, 0, 0.1f);
    }

    auto* const wander = static_cast<CTaskComplexWander*>(subTask);
    wander->m_nMoveState = m_moveState;

    const auto& pedPos = ped->GetPosition();
    if (m_hasFleePointChanged) {
        m_hasFleePointChanged = false;

        if (m_timer.m_bStarted) {
            m_timer.m_nStartTime = CTimer::GetTimeInMS();
            m_timer.m_nInterval  = m_fleeTimeMs;
            m_timer.m_bStarted   = true;
        }

        const auto fleeDir = ComputeFleeDir(ped);
        if (m_fleeDir != fleeDir) {
            m_fleeDir = fleeDir;
            WanderSetDir(wander, m_fleeDir);
        }
    } else {
        if (m_timer.m_bStarted && m_timer.IsOutOfTime()) {
            subTask = CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
        } else {
            const float safeDistSq = sq(m_safeDist);
            if (   safeDistSq < (m_fleePoint - pedPos).SquaredMagnitude()
                && safeDistSq < (m_initalPos - pedPos).SquaredMagnitude()
            ) {
                subTask = CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
            }
        }
    }

    if (m_moveState == PEDMOVE_RUN && !g_ikChainMan.IsLooking(ped)) {
        if (CGeneral::GetRandomNumberInRange(0, 100) < 6) {
            g_ikChainMan.LookAt(
                "TaskSmartFleePoint",
                ped,
                nullptr,
                2000,
                BONE_UNKNOWN,
                &m_fleePoint,
                false,
                0.25f,
                500,
                3,
                false
            );
        }
    }

    return subTask;
}
