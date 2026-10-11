#include "StdInc.h"

#include "TaskInteriorUseInfo.h"
#include "Interior/Interior_c.h"
#include "Interior/InteriorInfo_t.h"
#include "Interior/InteriorManager_c.h"
#include "TaskInteriorGoToInfo.h"
#include "TaskInteriorLieInBed.h"
#include "TaskInteriorSitAtDesk.h"
#include "TaskInteriorShopKeeper.h"
#include "TaskComplexSequence.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleRunTimedAnim.h"
#include "TaskSimpleStandStill.h"
#include "Audio/Enums/PedSpeechContexts.h"

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

    if (m_pSubTask->GetTaskType() == TASK_INTERIOR_GOTO_INFO) {
        switch (m_IntInfo->Type) {
        case eInteriorInfoType::UNK_3: // Lie in bed (left)
            return new CTaskInteriorLieInBed{ m_Dur, m_IntInfo, false, m_bDoInstantly };
        case eInteriorInfoType::UNK_4: // Lie in bed (right)
            return new CTaskInteriorLieInBed{ m_Dur, m_IntInfo, true, m_bDoInstantly };
        case eInteriorInfoType::UNK_1: { // Sit on chair
            // NOTE: CTaskInteriorSitOnChair (vtbl 0x870314, size 0x2c) isn't reversed yet, so call its original ctor
            auto* const mem = CTask::operator new(0x2c);
            return plugin::CallMethodAndReturn<CTask*, 0x675C30, void*, int32, InteriorInfo_t*, bool>(mem, m_Dur, m_IntInfo, m_bDoInstantly);
        }
        case eInteriorInfoType::UNK_2: // Stand still (basically forever)
            return new CTaskSimpleStandStill{ 9'999'999, false, false, 8.f };
        case eInteriorInfoType::UNK_5: // Wash up
            if (InteriorManager_c::AreAnimsLoaded(0)) {
                return new CTaskSimpleRunTimedAnim{
                    ANIM_GROUP_INT_HOUSE,
                    ANIM_ID_WASH_UP,
                    m_bDoInstantly ? 1000.f : 4.f,
                    4.f,
                    (uint32)m_Dur,
                    false
                };
            }
            break;
        case eInteriorInfoType::UNK_6: // Sit at desk
            return new CTaskInteriorSitAtDesk{ m_Dur, m_IntInfo, m_bDoInstantly };
        case eInteriorInfoType::UNK_7: // Stand still (office)
            if (InteriorManager_c::AreAnimsLoaded(2)) {
                return new CTaskSimpleStandStill{ 5000, false, false, 8.f };
            }
            break;
        case eInteriorInfoType::UNK_8: // Shop: browse
            if (InteriorManager_c::AreAnimsLoaded(1)) {
                ped->Say(CTX_GLOBAL_SHOP_BROWSE);
                return new CTaskSimpleRunAnim{ ANIM_GROUP_INT_SHOP, ANIM_ID_SHOP_SHELF, 4.f, false };
            }
            break;
        case eInteriorInfoType::UNK_9: // Shop: pay
            if (InteriorManager_c::AreAnimsLoaded(1)) {
                auto* const shopKeeper = m_Int->m_pGroup->GetPed(0)->GetTaskManager().GetTaskPrimary(TASK_PRIMARY_DEFAULT);
                if (shopKeeper->GetTaskType() == TASK_INTERIOR_SHOPKEEPER) {
                    static_cast<CTaskInteriorShopKeeper*>(shopKeeper)->m_bStartServeAnim = true;
                    ped->Say(CTX_GLOBAL_SHOP_BUY);
                    auto* const seq = new CTaskComplexSequence{};
                    seq->AddTask(new CTaskSimpleRunAnim{ ANIM_GROUP_INT_SHOP, ANIM_ID_SHOP_PAY, 4.f, false });
                    return seq;
                }
            }
            break;
        case eInteriorInfoType::UNK_10: // Stand still (looped)
            return new CTaskSimpleStandStill{ 0, true, false, 8.f };
        default:
            break;
        }
    }

    if (m_IntInfo->EntityIgnoredCollision) {
        ped->m_pEntityIgnoredCollision = nullptr;
    }
    return nullptr;
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
