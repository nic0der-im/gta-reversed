#include "StdInc.h"

#include "TaskSimpleChoking.h"

// 0x6202C0
CTaskSimpleChoking::CTaskSimpleChoking(CPed* attacker, bool bIsTeargas) :
    m_pAttacker{attacker},
    m_bIsTeargas{bIsTeargas}
{
    CEntity::SafeRegisterRef(m_pAttacker);
    m_pAnim          = nullptr;
    m_bIsFinished    = false;
    m_nTimeRemaining = CGeneral::GetRandomNumberInRange(0u, 1000u);
    m_nTimeStarted   = CTimer::GetTimeInMS();
}

// 0x623220
CTaskSimpleChoking::CTaskSimpleChoking(const CTaskSimpleChoking& o) :
    CTaskSimpleChoking{o.m_pAttacker, o.m_bIsTeargas}
{
}

namespace {
// 0x407180
int32 RandomIntOriginal(int32 min, int32 max) {
    return min + static_cast<int32>((static_cast<int64>(CGeneral::GetRandomNumber() & 0xFFFF) * (max - min)) / 32768);
}
}

// 0x620480
static void ChokingFinishCB(CAnimBlendAssociation*, void* data) {
    auto* const self = static_cast<CTaskSimpleChoking*>(data);
    self->m_bIsFinished = true;
    self->m_pAnim       = nullptr;
}

// 0x6203F0
bool CTaskSimpleChoking::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority == ABORT_PRIORITY_URGENT || priority == ABORT_PRIORITY_IMMEDIATE) {
        // 0x39 == first event priority that may abort us (vtable slot 2)
        if (event && event->GetEventPriority() < 0x39) {
            return false;
        }
        if (m_pAnim) {
            m_pAnim->SetBlendDelta(-4.f);
            m_pAnim->SetDefaultFinishCallback();
            m_pAnim = nullptr;
        }
        m_bIsFinished = true;
    } else if (m_pAnim) {
        m_pAnim->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
        m_pAnim->SetBlendDelta(-4.f);
        m_pAnim->SetDefaultFinishCallback();
        m_pAnim = nullptr;
    }
    return true;
}

// 0x620490
bool CTaskSimpleChoking::ProcessPed(CPed* ped) {
    if (m_bIsFinished) {
        return true;
    }

    if (m_pAttacker && !ped->IsPlayer() && !m_bIsTeargas) {
        const auto dir = m_pAttacker->GetPosition() - ped->GetPosition();
        if (dir.Dot(ped->GetForward()) > 0.f) {
            ped->m_fAimingRotation = std::atan2(-dir.x, dir.y);
        }
    }

    const auto dt = (uint32)CTimer::GetTimeStepInMS();
    m_nTimeRemaining = m_nTimeRemaining > dt ? m_nTimeRemaining - dt : 0;

    if (!m_pAnim) {
        m_pAnim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_GAS_CWR, 4.f);
        m_pAnim->SetFinishCallback(ChokingFinishCB, this);
        m_pAnim->SetSpeed(CGeneral::GetRandomNumberInRange(0.75f, 1.f));
    } else if (m_nTimeRemaining == 0) {
        if (m_pAnim->m_AnimId == ANIM_ID_GAS_CWR) {
            m_pAnim->SetDefaultFinishCallback();
            m_pAnim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_IDLE_TIRED, 4.f);
            m_pAnim->SetFinishCallback(ChokingFinishCB, this);
            m_nTimeRemaining = CTimer::GetTimeInMS() - m_nTimeStarted;
            if (m_nTimeRemaining >= (uint32)RandomIntOriginal(8000, 12000)) {
                m_nTimeRemaining = RandomIntOriginal(8000, 12000);
            }
        } else {
            m_pAnim->SetBlendDelta(-4.f);
            m_pAnim->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
        }
    }

    ped->Say(CTX_GLOBAL_PAIN_COUGH);
    return false;
}

// 0x620660
void CTaskSimpleChoking::UpdateChoke(CPed* victim, CPed* attacker, bool bIsTeargas) {
    m_bIsTeargas = bIsTeargas;
    if (m_pAttacker != attacker) {
        CEntity::ChangeEntityReference(m_pAttacker, attacker);
    }
    m_bIsFinished = false;
    if (m_pAnim && m_pAnim->m_AnimId != ANIM_ID_GAS_CWR) {
        m_pAnim->SetDefaultFinishCallback();
        m_pAnim = CAnimManager::BlendAnimation(victim->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_GAS_CWR, 4.f);
        m_pAnim->SetFinishCallback([](CAnimBlendAssociation* a, void* data) {
            const auto self = static_cast<CTaskSimpleChoking*>(data);
            self->m_bIsFinished = true;
            self->m_pAnim = nullptr;
        });
        m_pAnim->SetSpeed(CGeneral::GetRandomNumberInRange(0.8f, 1.1f));
        m_nTimeStarted = CTimer::GetTimeInMS() - m_nTimeRemaining;
    }
    m_nTimeRemaining = CGeneral::GetRandomNumberInRange<uint32>(1'000, 2'000);
}

void CTaskSimpleChoking::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleChoking, 0x86d820, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6202C0);
    RH_ScopedInstall(Destructor, 0x620370);

    RH_ScopedInstall(UpdateChoke, 0x620660);

    RH_ScopedVMTInstall(Clone, 0x623220);
    RH_ScopedVMTInstall(GetTaskType, 0x620360);
    RH_ScopedVMTInstall(MakeAbortable, 0x6203F0);
    RH_ScopedVMTInstall(ProcessPed, 0x620490);
}
