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

// 0x6203F0
bool CTaskSimpleChoking::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    switch (priority) {
    case ABORT_PRIORITY_URGENT:
    case ABORT_PRIORITY_IMMEDIATE: {
        if (event && event->GetEventPriority() < 57) {
            return false;
        }
        m_bIsFinished = true;
        break;
    }
    case ABORT_PRIORITY_LEISURE: {
        if (m_pAnim) {
            m_pAnim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
        }
        break;
    }
    default: {
        NOTSA_UNREACHABLE_CASE(priority);
    }
    }

    if (auto* const anim = std::exchange(m_pAnim, nullptr)) {
        anim->SetBlendDelta(-4.f);
        anim->SetDefaultFinishCallback();
    }

    return true;
}

// 0x620490
bool CTaskSimpleChoking::ProcessPed(CPed* ped) {
    if (m_bIsFinished) {
        return true;
    }

    if (m_pAttacker && !ped->IsPlayer() && !m_bIsTeargas) {
        const auto pedToAttacker = m_pAttacker->GetPosition() - ped->GetPosition();
        if (pedToAttacker.Dot(ped->GetForward()) > 0.f) {
            ped->m_fAimingRotation = pedToAttacker.Heading();
        }
    }

    const auto timeStepMS = static_cast<uint32>(CTimer::GetTimeStep() * 0.02f * 1000.f);
    if (m_nTimeRemaining > timeStepMS) {
        m_nTimeRemaining -= timeStepMS;
    } else {
        m_nTimeRemaining = 0;
    }

    constexpr auto finishAnimCB = [](CAnimBlendAssociation*, void* data) { // 0x620480
        const auto self     = static_cast<CTaskSimpleChoking*>(data);
        self->m_bIsFinished = true;
        self->m_pAnim       = nullptr;
    };

    if (!m_pAnim) {
        m_pAnim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_GAS_CWR, 4.f);
        m_pAnim->SetFinishCallback(finishAnimCB, this);
        m_pAnim->SetSpeed(CGeneral::GetRandomNumberInRange(0.75f, 1.f));
    } else if (m_nTimeRemaining == 0) {
        if (m_pAnim->m_AnimId == ANIM_ID_GAS_CWR) {
            m_pAnim->SetDefaultFinishCallback();
            m_pAnim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_IDLE_TIRED, 4.f);
            m_pAnim->SetFinishCallback(finishAnimCB, this);
            m_nTimeRemaining = CTimer::GetTimeInMS() - m_nTimeStarted;
            if (m_nTimeRemaining >= static_cast<uint32>(CGeneral::GetRandomNumberInRange(8000, 12000))) {
                m_nTimeRemaining = CGeneral::GetRandomNumberInRange(8000, 12000);
            }
        } else {
            m_pAnim->SetBlendDelta(-4.f);
            m_pAnim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
        }
    }

    ped->Say(CTX_GLOBAL_PAIN_COUGH, 0, 1.f, false, false, false);
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
