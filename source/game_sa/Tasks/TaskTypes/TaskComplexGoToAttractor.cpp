#include "StdInc.h"

#include "TaskComplexGoToAttractor.h"
#include "PedAtmAttractor.h"
#include "PedPlacement.h"
#include "TaskComplexSequence.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleSlideToCoord.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexGoToAttractor::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGoToAttractor, 0x86FF3C, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x66B640);
    RH_ScopedInstall(Destructor, 0x66B6A0);

    RH_ScopedVMTInstall(Clone, 0x66D130);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x66B6C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x670420);
}

// 0x66B640
CTaskComplexGoToAttractor::CTaskComplexGoToAttractor(CPedAttractor* attractor, const CVector& pos, float heading, float attrTime, int32 queueNumber, eMoveState ms) : CTaskComplex() {
    m_Attractor = attractor;
    m_vecAttrPosn = pos;
    m_fAttrHeading = heading;
    m_MoveState = ms;
    m_fAttrTime = attrTime;
    m_nQueueNumber = queueNumber;
}

// 0x66D130
// NOTE: Retail doesn't call the class ctor (0x66B640) here - It only runs the `CTaskComplex` base ctor
// (which sets the vtable and zeroes `m_pSubTask`), writes the derived vtable, then copies all fields
// by hand. The result is the same object that the ctor below produces.
CTask* CTaskComplexGoToAttractor::Clone() const {
    return new CTaskComplexGoToAttractor(m_Attractor, m_vecAttrPosn, m_fAttrHeading, m_fAttrTime, m_nQueueNumber, m_MoveState);
}

// 0x66B6B0
bool CTaskComplexGoToAttractor::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x66B6C0
CTask* CTaskComplexGoToAttractor::CreateNextSubTask(CPed* ped) {
    GetPedAttractorManager()->BroadcastArrival(ped, m_Attractor);
    if (ped->bUseAttractorInstantly && m_Attractor->GetHeadOfQueue() != ped) {
        ped->bUseAttractorInstantly = false;
    }
    return nullptr;
}

// 0x670420
CTask* CTaskComplexGoToAttractor::CreateFirstSubTask(CPed* ped) {
    auto moveState = m_MoveState;
    if (m_Attractor->GetType() == PED_ATTRACTOR_SHELTER) { // Peds run towards shelters
        moveState = PEDMOVE_RUN;
    }

    if (ped->bUseAttractorInstantly) {
        // Retail modifies `z` in place and ignores the result
        m_vecAttrPosn = std::get<CVector>(CPedPlacement::FindZCoorForPed(m_vecAttrPosn));

        ped->SetPosn(m_vecAttrPosn);
        ped->m_fCurrentRotation = m_fAttrHeading;
        ped->m_fAimingRotation  = m_fAttrHeading;

        return new CTaskSimpleStandStill(0, false, false, 8.0f);
    }

    const auto seq = new CTaskComplexSequence{};
    seq->AddTask(new CTaskComplexGoToPointAndStandStill(moveState, m_vecAttrPosn, 0.5f, 2.0f, false, false));
    seq->AddTask(new CTaskSimpleSlideToCoord(m_vecAttrPosn, m_fAttrHeading, 0.5f));
    return seq;
}

// 0x66B710
CTask* CTaskComplexGoToAttractor::ControlSubTask(CPed* ped) {
    return m_pSubTask;
}
