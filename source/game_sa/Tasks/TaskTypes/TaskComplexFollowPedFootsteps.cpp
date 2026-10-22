#include "StdInc.h"

#include "TaskComplexFollowPedFootsteps.h"
#include "TaskComplexSeekEntity.h"
#include "TaskSimpleGoToPoint.h"
#include "TaskSimpleHitHead.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexFollowPedFootsteps::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexFollowPedFootsteps, 0x870CC0, 12);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x694E20);
    RH_ScopedVMTInstall(MakeAbortable, 0x694ED0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x694EE0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x695000);
    RH_ScopedVMTInstall(ControlSubTask, 0x695090);
    RH_ScopedVMTInstall(CreateSubTask, 0x695E40); // This one is a new virtual (slot 0x2C of the class' VMT)
}

// 0x694E20
CTaskComplexFollowPedFootsteps::CTaskComplexFollowPedFootsteps(CPed* ped) : CTaskComplex() {
    m_targetPed               = ped;
    m_updateGoToPoint         = false;
    m_subTaskCreateCheckTimer = CTimer::GetTimeInMS();
    m_lineOfSightCheckTimer   = 0;
    m_pointRoute              = nullptr;
    m_moveState               = PEDMOVE_WALK;

    CEntity::SafeRegisterRef(m_targetPed);

    m_pointRoute = new CPointRoute();
}

CTaskComplexFollowPedFootsteps* CTaskComplexFollowPedFootsteps::Constructor(CPed* ped) {
    this->CTaskComplexFollowPedFootsteps::CTaskComplexFollowPedFootsteps(ped);
    return this;
}

CTaskComplexFollowPedFootsteps::~CTaskComplexFollowPedFootsteps() {
    CEntity::SafeCleanUpRef(m_targetPed);

    delete m_pointRoute;
    m_pointRoute = nullptr;
}

// 0x694ED0
bool CTaskComplexFollowPedFootsteps::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x694EE0
CTask* CTaskComplexFollowPedFootsteps::CreateNextSubTask(CPed* ped) {
    if (!m_targetPed) { // 0x694EEB
        return CreateSubTask(TASK_FINISHED, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_HIT_HEAD:
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
    case TASK_SIMPLE_STAND_STILL:
    case TASK_SIMPLE_GO_TO_POINT:
        if (m_pointRoute && !m_pointRoute->IsEmpty()) {
            return CreateSubTask(TASK_SIMPLE_GO_TO_POINT, ped);
        }
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
    case TASK_COMPLEX_SEEK_ENTITY:
        // NOTE: Uses the raw 1.4f constant (Not squared!) - matches the game's behaviour
        if ((m_targetPed->GetPosition() - ped->GetPosition()).SquaredMagnitude() > 1.4f) {
            return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
        }
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
    default: // Includes `TASK_NONE`
        return CreateSubTask(TASK_FINISHED, ped);
    }
}

// 0x695000
CTask* CTaskComplexFollowPedFootsteps::CreateFirstSubTask(CPed* ped) {
    if (!m_targetPed) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    // NOTE: Uses the raw 1.4f constant (Not squared!) - matches the game's behaviour
    if ((m_targetPed->GetPosition() - ped->GetPosition()).SquaredMagnitude() > 1.4f) {
        return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
    }

    return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
}

// 0x695090
CTask* CTaskComplexFollowPedFootsteps::ControlSubTask(CPed* ped) {
    // Recreate the sub-task (Iff the current one can be aborted - Otherwise it's kept as-is)
    const auto CreateNewSubTask = [&](eTaskType type) -> CTask* {
        return m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)
            ? CreateSubTask(type, ped)
            : m_pSubTask;
    };

    if (!m_targetPed || !m_pointRoute || !m_targetPed->IsAlive()) {
        return CreateNewSubTask(TASK_FINISHED);
    }

    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_SEEK_ENTITY) {
        return m_pSubTask; // Seeking is left alone
    }

    if (m_targetPed->bInVehicle) {
        return CreateNewSubTask(TASK_COMPLEX_SEEK_ENTITY);
    }

    if (ped->GetIntelligence()->m_AnotherStaticCounter > 30) {
        return CreateNewSubTask(TASK_SIMPLE_HIT_HEAD);
    }

    // Refresh the line of sight check every 500ms
    if (CTimer::GetTimeInMS() - m_lineOfSightCheckTimer > 500) {
        m_lineOfSightCheckTimer = CTimer::GetTimeInMS();
        m_updateGoToPoint       = CWorld::GetIsLineOfSightClear(
            ped->GetPosition(),
            m_targetPed->GetPosition(),
            true,  // buildings
            false, // vehicles
            false, // peds
            true,  // objects
            false, // dummies
            false, // doSeeThroughCheck
            false  // doCameraIgnoreCheck
        );
    }

    const CVector toTarget = m_targetPed->GetPosition() - ped->GetPosition();
    const float   dist2    = toTarget.SquaredMagnitude();

    // Close enough (But not standing still yet) - Just stand still, no need to walk towards the target
    if (dist2 < sq(1.4f) && m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
        if (!m_updateGoToPoint) {
            return m_pSubTask;
        }
        m_pointRoute->Clear();
        return CreateNewSubTask(TASK_SIMPLE_STAND_STILL);
    }

    // Too far away (Or too close, but too high up/down) - Seek the target instead
    if (dist2 > sq(8.f) || (std::sqrtf(sq(toTarget.x) + sq(toTarget.y)) < 1.f && std::fabsf(toTarget.z) > 2.f)) {
        return CreateNewSubTask(TASK_COMPLEX_SEEK_ENTITY);
    }

    if (m_updateGoToPoint) {
        m_pointRoute->Clear();
        m_pointRoute->AddUnlessFull(m_targetPed->GetPosition());
        m_subTaskCreateCheckTimer = CTimer::GetTimeInMS() - 332; // Force the route to be rebuild during this call
        if (CTask* const goToTask = ped->GetIntelligence()->m_TaskMgr.GetSimplestActiveTask(); goToTask && CTask::IsGoToTask(goToTask)) {
            static_cast<CTaskSimpleGoToPoint*>(goToTask)->UpdatePoint(m_targetPed->GetPosition(), 0.5f, false);
        }
    }

    // Don't rebuild the route too often (Or we'll never reach the target)
    if (CTimer::GetTimeInMS() - m_subTaskCreateCheckTimer < 166 || m_pointRoute->GetSize() > 7) {
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
            return m_pSubTask;
        }
        if (m_pointRoute->GetSize() < 2) {
            return m_pSubTask;
        }
        return CreateNewSubTask(TASK_SIMPLE_GO_TO_POINT);
    }
    m_subTaskCreateCheckTimer = CTimer::GetTimeInMS();

    // Add the target's position to the route (Unless it's basically the same as the last one)
    const CVector targetPos    = m_targetPed->GetPosition();
    bool          addTargetPos = true;
    if (!m_pointRoute->IsEmpty()) {
        const CVector lastPoint = (*m_pointRoute)[m_pointRoute->GetSize() - 1];
        CVector       deltaToLast = targetPos - lastPoint;
        if (deltaToLast.SquaredMagnitude() <= sq(0.35f)) { // The same point as the last one, no need to add it again
            addTargetPos = false;
        } else if (m_pointRoute->GetSize() > 1) { // Remove the last point if it's in the same direction as the new one
            CVector prevDelta = lastPoint - (*m_pointRoute)[m_pointRoute->GetSize() - 2];
            prevDelta.Normalise();
            deltaToLast.Normalise();
            if (prevDelta.Dot(deltaToLast) >= 0.95f) {
                m_pointRoute->PopBack();
            }
        }
    }
    if (addTargetPos) {
        m_pointRoute->AddUnlessFull(targetPos);
    }

    // Walk or run depending on how long the route (plus the distance to the target) is
    float routeSum = 0.f;
    for (int32 i = 1; i < (int32)m_pointRoute->GetSize(); i++) {
        routeSum += ((*m_pointRoute)[i] - (*m_pointRoute)[0]).SquaredMagnitude();
    }
    const float pedDist2 = (ped->GetPosition() - m_targetPed->GetPosition()).SquaredMagnitude();
    m_moveState = (int16)((pedDist2 + routeSum <= sq(3.5f)) ? PEDMOVE_WALK : PEDMOVE_RUN);
    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_GO_TO_POINT) {
        static_cast<CTaskSimpleGoTo*>(m_pSubTask)->m_moveState = (eMoveState)m_moveState;
    }

    return m_pSubTask;
}

// 0x695E40
CTask* CTaskComplexFollowPedFootsteps::CreateSubTask(eTaskType taskType, CPed*) {
    if (!m_targetPed || !m_pointRoute) {
        return nullptr;
    }

    switch (taskType) {
    case TASK_SIMPLE_GO_TO_POINT: {
        // NOTE: The first route point is popped off - It's consumed by the task
        const CVector goToPoint = m_pointRoute->PopFront();
        return new CTaskSimpleGoToPoint{ (eMoveState)m_moveState, goToPoint, 0.01f, false, false };
    }
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 10'000, false, false, 8.0f };
    case TASK_SIMPLE_HIT_HEAD:
        return new CTaskSimpleHitHead{};
    case TASK_COMPLEX_SEEK_ENTITY: {
        const float maxEntityDist2D = m_targetPed->bInVehicle ? 1.f * 4.f : 1.f;
        return new CTaskComplexSeekEntity<CEntitySeekPosCalculatorStandard>{
            m_targetPed,
            50'000, // seekInterval
            1'000,  // scanInterval
            maxEntityDist2D,
            2.f,    // moveStateRadius
            2.f,    // followNodeThresholdHeightChange
            true,   // bPlayTiredAnim
            true    // bFaceEntityWhenDone
        };
    }
    default: // Includes `TASK_FINISHED`
        return nullptr;
    }
}
