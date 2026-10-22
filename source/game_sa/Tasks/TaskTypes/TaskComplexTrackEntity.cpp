#include "StdInc.h"
#include "TaskComplexTrackEntity.h"
#include "TaskSimpleGoToPointFine.h"
#include "TaskSimpleStandStill.h"
#include "TaskComplexFollowNodeRoute.h"

void CTaskComplexTrackEntity::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexTrackEntity, 0x86F998, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65F3B0);
    RH_ScopedInstall(Destructor, 0x65F460);

    RH_ScopedInstall(SetOffsetPos, 0x65F760, {.State = HS::RedirectToGTA, .Locked = true});
    RH_ScopedInstall(CalcTargetPos, 0x65F780);
    RH_ScopedInstall(CalcMoveRatio, 0x65F930);

    RH_ScopedVMTInstall(Clone, 0x65F4E0);
    RH_ScopedVMTInstall(GetTaskType, 0x65F450);
    RH_ScopedVMTInstall(MakeAbortable, 0x65F4C0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x65F590);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x65F700);
    RH_ScopedVMTInstall(ControlSubTask, 0x663640, { .Locked = true }); // Locked because it fucks up the stack and crashes 
}

// 0x65F3B0
CTaskComplexTrackEntity::CTaskComplexTrackEntity(CEntity* entity, CVector offsetPos, uint8 a6, int32 a7, float rangeMin, float rangeMax, uint8 a10) :
    m_rangeMax{rangeMax},
    m_rangeMin{rangeMin},
    m_toTrack{entity},
    m_offsetPosn{offsetPos},
    a{a6},
    b{a7},
    f{a10}
{
    CEntity::RegisterReference(m_toTrack);
}

// NOTSA
CTaskComplexTrackEntity::CTaskComplexTrackEntity(const CTaskComplexTrackEntity& o) :
    CTaskComplexTrackEntity{o.m_toTrack, o.m_offsetPosn, o.a, o.b, o.m_rangeMin, o.m_rangeMax, o.f}
{
    m_fMoveRatio = o.m_fMoveRatio;
}

// 0x65F460
CTaskComplexTrackEntity::~CTaskComplexTrackEntity() {
    CEntity::SafeCleanUpRef(m_toTrack);
}

// 0x65F760
void CTaskComplexTrackEntity::SetOffsetPos(CVector posn) {
    m_offsetPosn = posn;
}

// 0x65F780
void CTaskComplexTrackEntity::CalcTargetPos(CPed* ped) {
    m_goToPos = m_toTrack->GetPosition();

    if (f) {
        // The offset is relative to the entity's matrix (X along `right`, Y along `up`)
        if (!m_toTrack->m_matrix) {
            m_toTrack->AllocateMatrix();
            m_toTrack->m_placement.UpdateMatrix(m_toTrack->m_matrix);
        }

        const CMatrix& mat = *m_toTrack->m_matrix;
        m_goToPos += mat.GetRight() * m_offsetPosn.x;
        m_goToPos += mat.GetUp()    * m_offsetPosn.y;
    } else {
        m_goToPos.x += m_offsetPosn.x;
        m_goToPos.y += m_offsetPosn.y;
    }

    // Lead the target by its velocity so we account for the frame's movement
    if (m_toTrack->GetType() == ENTITY_TYPE_VEHICLE || m_toTrack->GetType() == ENTITY_TYPE_PED) {
        m_goToPos += m_toTrack->AsPhysical()->m_vecMoveSpeed * CTimer::GetTimeStep();
    }

    const auto pedPos = ped->GetPosition();
    m_distToTargetSq = sq(m_goToPos.x - pedPos.x) + sq(m_goToPos.y - pedPos.y);
}

// 0x65F930
void CTaskComplexTrackEntity::CalcMoveRatio(CPed*) { // NOTE: `ped` is unused
    // The distance->move-ratio curve is built from these fixed thresholds
    // TODO(re): the original caches the squared thresholds/reciprocals in statics at 0xC18CF4..0xC18D08
    constexpr struct {
        float min = 0.2f; // Below this distance the ratio is zero
        float mid = 1.0f; // Ratio is 0.5 here
        float max = 5.0f; // Above this distance the ratio is one
    } kThresholds;

    const float dist = std::sqrt(m_distToTargetSq);

    if (m_distToTargetSq <= sq(kThresholds.min)) {
        float40 = 0.0f;
    } else if (m_distToTargetSq >= sq(kThresholds.max)) {
        float40 = 1.0f;
    } else if (m_distToTargetSq > sq(kThresholds.mid)) {
        float40 = (dist - kThresholds.min) / (kThresholds.mid - kThresholds.min) * 0.5f;
    } else {
        float40 = (dist - kThresholds.mid) / (kThresholds.max - kThresholds.mid) * 0.5f + 0.5f;
    }

    float40 = std::sqrt(float40) * 3.0f;
    if (!a && float40 >= 2.0f) {
        float40 = 2.0f;
    }

    if (float40 - m_fMoveRatio < 0.2f) {
        m_fMoveRatio = float40;
    } else {
        m_fMoveRatio += 0.2f;
    }
}

// 0x65F4C0
bool CTaskComplexTrackEntity::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x65F590
CTask* CTaskComplexTrackEntity::CreateNextSubTask(CPed* ped) {
    if (!m_toTrack) {
        return nullptr;
    }

    if (!m_pSubTask || m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
        return new CTaskSimpleStandStill{ };
    }

    if (m_distToTargetSq < sq(m_rangeMin)) {
        return new CTaskSimpleGoToPointFine{ m_fMoveRatio, m_goToPos, 0.25f, nullptr };
    }

    return new CTaskComplexFollowNodeRoute{
        PEDMOVE_RUN,
        m_toTrack->GetPosition(),
        0.5f,
        0.2f,
        2.0f,
        false,
        -1,
        true
    };
}

// 0x65F700
CTask* CTaskComplexTrackEntity::CreateFirstSubTask(CPed* ped) {
    if (m_fMoveRatio < 0.0) {
        m_fMoveRatio = [ped] {
            switch (ped->m_nMoveState) {
            case 1: return 0.0f;
            case 4: return 1.0f;
            case 6: return 2.0f;
            default: return 3.0f;
            }
        }();
    }
    return CreateNextSubTask(ped);
}

// 0x663640
CTask* CTaskComplexTrackEntity::ControlSubTask(CPed* ped) {
    const auto TryAbort = [this, ped] {
        return MakeAbortable(ped);
    };

    const auto TryAbortGetTask = [&, this] {
        return TryAbort() ? nullptr : m_pSubTask;
    };

    if (!m_toTrack) {
        return TryAbortGetTask();
    }

    assert(!gap2 && !gap3); // NOTE: Both seem to be always be false, let's see if that's the case.

    if (gap2) {
        if (gap3) {
            m_someStartTimeMs = CTimer::GetTimeInMS();
            gap3 = false;
        }
        if (CTimer::GetTimeInMS() >= m_someStartTimeMs + m_someStartTimeMs) {
            return TryAbortGetTask();
        }
    }

    CalcTargetPos(ped);

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_FOLLOW_POINT_ROUTE: {
        // If we're totally out of range...
        if (m_distToTargetSq > sq(m_rangeMax)) {
            return nullptr;
        }

        // If we're now in range be a little more precise and create `TASK_SIMPLE_GO_TO_POINT_FINE`
        if (m_distToTargetSq < sq(m_rangeMin) && TryAbort()) {
            return CreateNextSubTask(ped);
        }

        break;
    }
    case TASK_SIMPLE_GO_TO_POINT_FINE: {
        // Check if we're still in range, if not, abort and create `TASK_COMPLEX_FOLLOW_POINT_ROUTE`
        if (m_distToTargetSq >= sq(m_rangeMin) && TryAbort()) {
            return CreateNextSubTask(ped);
        }

        CalcMoveRatio(ped);

        const auto gotoTask = static_cast<CTaskSimpleGoToPointFine*>(m_pSubTask);
        gotoTask->SetTargetPos(m_goToPos);
        gotoTask->SetMoveRatio(m_fMoveRatio);

        break;
    }
    }

    return m_pSubTask;
}
