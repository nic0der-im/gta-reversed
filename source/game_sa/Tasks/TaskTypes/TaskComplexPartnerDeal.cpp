#include "StdInc.h"

#include "TaskComplexPartnerDeal.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleRunAnim.h"

void CTaskComplexPartnerDeal::InjectHooks()
{
    RH_ScopedVirtualClass(CTaskComplexPartnerDeal, 0x870754, 14);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x684190);
}

CTaskComplexPartnerDeal::CTaskComplexPartnerDeal(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, CVector point) :
    CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, true, 1, point)
{
    m_taskId = TASK_COMPLEX_PARTNER_DEAL;
    strcpy_s(m_animBlockName, "gangs");
}

CTaskComplexPartnerDeal* CTaskComplexPartnerDeal::Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, CVector point)
{
    this->CTaskComplexPartnerDeal::CTaskComplexPartnerDeal(commandName, partner, leadSpeaker, distanceMultiplier, point);
    return this;
}

CTask* CTaskComplexPartnerDeal::CreateFirstSubTask(CPed* ped)
{
    // 0x6823B0 is a 5-byte tail-jump (`jmp 0x681F20`) to the shared base implementation
    return CTaskComplexPartner::CreateFirstSubTask(ped);
}

void CTaskComplexPartnerDeal::StreamRequiredAnims()
{
    CAnimManager::StreamAnimBlock(m_animBlockName, CTaskComplexGangLeader::ShouldLoadGangAnims(), m_requiredAnimsStreamedIn);
}

CTaskComplexSequence* CTaskComplexPartnerDeal::GetPartnerSequence()
{
    // Face the partner, then play the deal animation (the leader is the dealer, so he plays the selling anim)
    auto* const sequence = new CTaskComplexSequence{};
    sequence->AddTask(new CTaskComplexTurnToFaceEntityOrCoord{ m_partner, 0.5f, 0.02f });
    sequence->AddTask(new CTaskSimpleRunAnim{
        ANIM_GROUP_GANGS,
        m_leadSpeaker ? ANIM_ID_DEALER_DEAL : ANIM_ID_DRUGS_BUY,
        4.0f,
        false
    });
    return sequence;
}
