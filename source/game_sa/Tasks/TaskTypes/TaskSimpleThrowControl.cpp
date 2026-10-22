#include "StdInc.h"
#include "TaskSimpleThrowControl.h"
#include "TaskSimpleThrowProjectile.h"

void CTaskSimpleThrowControl::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleThrowControl, 0x86D7B4, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x61F8B0);
    RH_ScopedInstall(Destructor, 0x61F950);

    RH_ScopedVMTInstall(Clone, 0x6230B0);
    RH_ScopedVMTInstall(GetTaskType, 0x61F940);
    RH_ScopedVMTInstall(MakeAbortable, 0x61F9B0);
    RH_ScopedVMTInstall(ProcessPed, 0x61F9F0);
}

// 0x61F8B0
CTaskSimpleThrowControl::CTaskSimpleThrowControl(CEntity* targetEntity, CVector const* pos) :
    m_entity{targetEntity},
    m_pos{pos ? *pos : CVector{}}
{
    assert(m_entity);

    CEntity::SafeRegisterRef(m_entity);
}

// NOTSA
CTaskSimpleThrowControl::CTaskSimpleThrowControl(const CTaskSimpleThrowControl& o) :
    CTaskSimpleThrowControl{o.m_entity, &o.m_pos}
{
}

// 0x61F950
CTaskSimpleThrowControl::~CTaskSimpleThrowControl() {
    CEntity::SafeCleanUpRef(m_entity);
}

// 0x61F9B0
bool CTaskSimpleThrowControl::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (!ped->GetIntelligence()->GetTaskThrow()) {
        return true;
    }
    return ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_ATTACK)->MakeAbortable(ped, priority, event);
}

// 0x61F9F0
bool CTaskSimpleThrowControl::ProcessPed(CPed* ped) {
    if (byte8) {
        return true;
    }

    ped->SetMoveState(PEDMOVE_STILL);

    auto& taskMgr = ped->GetIntelligence()->GetTaskManager();
    if (auto* const attackTask = taskMgr.GetTaskSecondary(TASK_SECONDARY_ATTACK)) {
        if (attackTask->GetTaskType() != TASK_SIMPLE_THROW_PROJECTILE) {
            attackTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
            return false;
        }
    } else {
        if (m_isAttacking) {
            return true;
        }
        taskMgr.SetTaskSecondary(new CTaskSimpleThrowProjectile{ m_entity, m_pos }, TASK_SECONDARY_ATTACK);
        m_isAttacking = true;
    }

    auto* const throwTask = ped->GetIntelligence()->GetTaskThrow();

    const CVector* const targetPos = m_pos != CVector{} ? &m_pos : nullptr;

    if (m_entity) {
        const auto dir = m_entity->GetPosition() - ped->GetPosition();
        ped->m_fAimingRotation = std::atan2(-dir.x, dir.y);
    } else if (targetPos) {
        const auto dir = *targetPos - ped->GetPosition();
        ped->m_fAimingRotation = std::atan2(-dir.x, dir.y);
    }

    if (!m_isAttacking) {
        return false;
    }
    if (throwTask->m_bIsFinished) {
        return true;
    }
    if (targetPos) {
        throwTask->m_TargetPos = *targetPos;
    }
    return false;
}
