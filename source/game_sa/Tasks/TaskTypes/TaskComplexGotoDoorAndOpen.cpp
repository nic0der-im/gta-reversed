#include "StdInc.h"

#include "TaskComplexGotoDoorAndOpen.h"
#include "TaskSimpleGoToPoint.h"
#include "TaskSimpleRunAnim.h"
#include "EventAreaCodes.h"
#include "Object.h"

void CTaskComplexGotoDoorAndOpen::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGotoDoorAndOpen, 0x86FFC0, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedOverloadedInstall(Constructor, "0", 0x66BB20, CTaskComplexGotoDoorAndOpen*(CTaskComplexGotoDoorAndOpen::*)(CObject *));
    RH_ScopedOverloadedInstall(Constructor, "1", 0x66BBA0, CTaskComplexGotoDoorAndOpen*(CTaskComplexGotoDoorAndOpen::*)(CVector const&, CVector const&));
    RH_ScopedInstall(Destructor, 0x66BC00);
    RH_ScopedInstall(Clone, 0x66BCA0);
    RH_ScopedInstall(CreateNextSubTask, 0x66C0D0);
    RH_ScopedInstall(CreateFirstSubTask, 0x66BD40);
    RH_ScopedInstall(ControlSubTask, 0x66C1F0);
}

// 0x66BB20
CTaskComplexGotoDoorAndOpen::CTaskComplexGotoDoorAndOpen(CObject* door) : CTaskComplex() {
    m_Object = door;
    m_nStartTime = 0;
    m_nOffsetTime = 0;
    byte30 = 0;
    m_bRefreshTime = false;
    m_nFlags = m_nFlags & 0xF0 | 1; // todo: flags
    CEntity::SafeRegisterRef(m_Object);
}

// 0x66BBA0
CTaskComplexGotoDoorAndOpen::CTaskComplexGotoDoorAndOpen(const CVector& start, const CVector& end) : CTaskComplex() {
    m_Object = nullptr;
    m_Start = start;
    m_End = end;
    m_nStartTime = 0;
    m_nOffsetTime = 0;
    byte30 = 0;
    m_bRefreshTime = false;
    m_nFlags &= 244u; // todo: flags
}

// 0x66BC00
CTaskComplexGotoDoorAndOpen::~CTaskComplexGotoDoorAndOpen() {
    CEntity::SafeCleanUpRef(m_Object);
    if ((m_nFlags & 8) != 0) { // transition finished
        CPad::GetPad()->bPlayerOnInteriorTransition = false;
    }
}

// 0x66BCA0
CTask* CTaskComplexGotoDoorAndOpen::Clone() const {
    if ((m_nFlags & 1) != 0) { // todo: flags
        return new CTaskComplexGotoDoorAndOpen(m_Object);
    } else {
        return new CTaskComplexGotoDoorAndOpen(m_Start, m_End);
    }
}

// 0x66BC80
bool CTaskComplexGotoDoorAndOpen::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return priority == ABORT_PRIORITY_IMMEDIATE && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_IMMEDIATE, event);
}

// 0x66C0D0
CTask* CTaskComplexGotoDoorAndOpen::CreateNextSubTask(CPed* ped) {
    if (m_pSubTask->GetTaskType() != TASK_SIMPLE_GO_TO_POINT) {
        return nullptr;
    }

    if ((m_nFlags & 2) != 0) { // Second goto finished => done
        if (ped->IsPlayer()) {
            CPad::GetPad()->bPlayerOnInteriorTransition = true;
        }
        return nullptr;
    }

    auto& taskMgr = ped->GetTaskManager();
    if (!taskMgr.GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
        taskMgr.SetTaskSecondary(new CTaskSimpleRunAnim{ANIM_GROUP_DEFAULT, ANIM_ID_WALK_DOORPARTIAL, 8.f, false}, TASK_SECONDARY_PARTIAL_ANIM);
    }
    m_nFlags |= 2;
    return new CTaskSimpleGoToPoint{PEDMOVE_WALK, m_End, 0.2f, false, false};
}

// 0x66BD40
CTask* CTaskComplexGotoDoorAndOpen::CreateFirstSubTask(CPed* ped) {
    if (ped->IsPlayer()) {
        CPad::GetPad()->bPlayerOnInteriorTransition = true;
        m_nFlags |= 8;
    }

    CEventAreaCodes event{ped};
    GetEventGlobalGroup()->Add(&event, false);

    m_nStartTime  = CTimer::GetTimeInMS();
    m_nOffsetTime = 1000;
    byte30        = 1;

    if (!m_Object) {
        if ((m_nFlags & 1) != 0) {
            return nullptr;
        }
        m_nFlags |= 2;
        return new CTaskSimpleGoToPoint{PEDMOVE_WALK, m_End, 0.5f, false, false};
    }

    const auto& doorPos = m_Object->GetPosition();
    const auto& fwd     = m_Object->GetMatrix().GetForward();
    const auto  base    = doorPos + m_Object->GetMatrix().GetRight() * 0.75f;

    // Pick the side of the door the ped is on
    if ((doorPos - ped->GetPosition()).Dot(fwd) <= 0.f) {
        m_Start = base - fwd * -0.5f;
        m_End   = base - fwd * 2.f;
    } else {
        m_Start = base + fwd * -0.5f;
        m_End   = base + fwd * 2.f;
    }

    if (m_Object->m_nPhysicalFlags & 8) { // bCollidable
        m_nFlags |= 4;
        m_Object->m_nPhysicalFlags &= ~8u; // bCollidable = false
        m_Object->m_nPhysicalFlags &= ~4u; // bDisableCollisionForce = false
    }

    if (RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_SPRINT)) {
        CAnimManager::BlendAnimation(ped->GetRpClump(), ped->m_nAnimGroup, ANIM_ID_WALK, 1000.f);
    }

    return new CTaskSimpleGoToPoint{PEDMOVE_WALK, m_Start, 0.35f, false, false};
}

// 0x66C1F0
CTask* CTaskComplexGotoDoorAndOpen::ControlSubTask(CPed* ped) {
    const auto Abort = [&]() -> CTask* {
        if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            if (ped->IsPlayer()) {
                CPad::GetPad()->bPlayerOnInteriorTransition = true;
            }
            return nullptr;
        }
        return m_pSubTask;
    };

    if (byte30) {
        if (m_bRefreshTime) {
            m_nStartTime   = CTimer::GetTimeInMS();
            m_bRefreshTime = false;
        }
        if (CTimer::GetTimeInMS() >= m_nStartTime + m_nOffsetTime) { // Timed out
            return Abort();
        }
    }

    // NOTE: Original quirk - abort if there's no door object but the task was created from one
    if (!m_Object && (m_nFlags & 1) != 0) {
        return Abort();
    }
    return m_pSubTask;
}
