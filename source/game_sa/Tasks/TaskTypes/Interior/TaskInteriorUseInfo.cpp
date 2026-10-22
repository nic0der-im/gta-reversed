#include "StdInc.h"

#include "TaskInteriorUseInfo.h"
#include "TaskInteriorGoToInfo.h"
#include "TaskInteriorLieInBed.h"
#include "TaskInteriorShopKeeper.h"
#include "TaskInteriorSitAtDesk.h"
#include "TaskInteriorSitInChair.h"
#include "TaskComplexSequence.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleRunTimedAnim.h"
#include "TaskSimpleStandStill.h"

#include "Interior/Interior_c.h"
#include "Interior/InteriorGroup_c.h"
#include "Interior/InteriorInfo_t.h"
#include "Interior/InteriorManager_c.h"

void CTaskInteriorUseInfo::InjectHooks() {
    RH_ScopedVirtualClass(CTaskInteriorUseInfo, 0x8702e8, 11);
    RH_ScopedCategory("Tasks/TaskTypes/Interior");

    RH_ScopedInstall(Constructor, 0x675A50);
    RH_ScopedInstall(Destructor, 0x675A90);

    RH_ScopedVMTInstall(Clone, 0x675AB0);
    RH_ScopedVMTInstall(GetTaskType, 0x675A80);
    RH_ScopedVMTInstall(MakeAbortable, 0x675B30);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x676880);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x675B60);
    RH_ScopedVMTInstall(ControlSubTask, 0x675C00);
}

// 0x675A50
CTaskInteriorUseInfo::CTaskInteriorUseInfo(InteriorInfo_t* interiorInfo, Interior_c* interior, int32 duration, bool bDoInstantly) :
    m_IntInfo{interiorInfo},
    m_Int{interior},
    m_Dur{duration},
    m_bDoInstantly{bDoInstantly}
{
}

// 0x675AB0
CTaskInteriorUseInfo::CTaskInteriorUseInfo(const CTaskInteriorUseInfo& o) :
    CTaskInteriorUseInfo{o.m_IntInfo, o.m_Int, o.m_Dur, o.m_bDoInstantly}
{
}

// 0x675A90
CTaskInteriorUseInfo::~CTaskInteriorUseInfo() {
    m_IntInfo->IsInUse = false;
}

// 0x675B30
bool CTaskInteriorUseInfo::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (m_IntInfo->EntityIgnoredCollision) {
        ped->m_pEntityIgnoredCollision = nullptr;
    }
    return CTaskComplex::MakeAbortable(ped, priority, event);
}

// 0x676880
CTask* CTaskInteriorUseInfo::CreateNextSubTask(CPed* ped) {
    if (!m_IntInfo) {
        return nullptr;
    }

    if (m_pSubTask->GetTaskType() != TASK_INTERIOR_GOTO_INFO) {
        return nullptr;
    }

    switch (m_IntInfo->Type) {
    case eInteriorInfoType::UNK_1:
        return new CTaskInteriorSitInChair{ m_Dur, m_IntInfo, m_bDoInstantly };
    case eInteriorInfoType::UNK_2:
        return new CTaskSimpleStandStill{ 9'999'999, false, false, 8.0f };
    case eInteriorInfoType::UNK_3:
        return new CTaskInteriorLieInBed{ m_Dur, m_IntInfo, false, m_bDoInstantly };
    case eInteriorInfoType::UNK_4:
        return new CTaskInteriorLieInBed{ m_Dur, m_IntInfo, true, m_bDoInstantly };
    case eInteriorInfoType::UNK_5:
        if (!InteriorManager_c::AreAnimsLoaded(0)) { // `int_house`
            return nullptr;
        }
        return new CTaskSimpleRunTimedAnim{
            ANIM_GROUP_INT_HOUSE,
            ANIM_ID_WASH_UP,
            m_bDoInstantly ? 1000.0f : 4.0f,
            4.0f,
            (uint32)m_Dur,
            false
        };
    case eInteriorInfoType::UNK_6:
        return new CTaskInteriorSitAtDesk{ m_Dur, m_IntInfo, m_bDoInstantly };
    case eInteriorInfoType::UNK_7:
        if (!InteriorManager_c::AreAnimsLoaded(2)) { // `int_office`
            return nullptr;
        }
        return new CTaskSimpleStandStill{ 5000, false, false, 8.0f };
    case eInteriorInfoType::UNK_8:
        if (!InteriorManager_c::AreAnimsLoaded(1)) { // `int_shop`
            return nullptr;
        }
        ped->Say((eGlobalSpeechContext)0xBC);
        return new CTaskSimpleRunAnim{ ANIM_GROUP_INT_SHOP, ANIM_ID_SHOP_SHELF, 4.0f, false };
    case eInteriorInfoType::UNK_9: {
        if (!InteriorManager_c::AreAnimsLoaded(1)) { // `int_shop`
            return nullptr;
        }
        // TODO(re): The original also sets `CTaskInteriorShopKeeper::m_bStartServeAnim` (The field is private, a setter is needed)
        if (!m_Int->m_pGroup->GetPed(0)->GetIntelligence()->GetTaskManager().GetActiveTaskAs<CTaskInteriorShopKeeper>()) {
            return nullptr;
        }
        ped->Say((eGlobalSpeechContext)0xBD);
        return new CTaskComplexSequence{
            new CTaskSimpleRunAnim{ ANIM_GROUP_INT_SHOP, ANIM_ID_SHOP_PAY, 4.0f, false }
        };
    }
    case eInteriorInfoType::UNK_10:
        return new CTaskSimpleStandStill{ 0, true, false, 8.0f };
    default:
        if (m_IntInfo->EntityIgnoredCollision) {
            ped->m_pEntityIgnoredCollision = nullptr;
        }
        return nullptr;
    }
}

// 0x675B60
CTask* CTaskInteriorUseInfo::CreateFirstSubTask(CPed* ped) {
    if (!m_IntInfo) {
        return nullptr;
    }
    m_IntInfo->IsInUse = true;
    return new CTaskInteriorGoToInfo{ m_IntInfo, m_Int, m_bDoInstantly };
}

// 0x675C00
CTask* CTaskInteriorUseInfo::ControlSubTask(CPed* ped) {
    if (!m_IntInfo) {
        return nullptr;
    }
    if (m_IntInfo->EntityIgnoredCollision) {
        ped->m_pEntityIgnoredCollision = m_IntInfo->EntityIgnoredCollision;
    }
    return m_pSubTask;
}
