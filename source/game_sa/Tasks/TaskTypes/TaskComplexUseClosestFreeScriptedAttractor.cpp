#include "StdInc.h"
#include "TaskComplexUseClosestFreeScriptedAttractor.h"

#include "TaskComplexUseEffect.h"
#include "TaskComplexUseEffectRunning.h"
#include "TaskComplexUseEffectSprinting.h"
#include "InterestingEvents.h"
#include "PedAttractorManager.h"
#include "Scripted2dEffects.h"

void CTaskComplexUseClosestFreeScriptedAttractor::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexUseClosestFreeScriptedAttractor, 0x86e428, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6346F0);
    RH_ScopedInstall(Destructor, 0x634720);

    RH_ScopedGlobalInstall(ComputeClosestFreeScriptedEffect, 0x634740);

    RH_ScopedVMTInstall(Clone, 0x636F70);
    RH_ScopedVMTInstall(GetTaskType, 0x634710);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x634730);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x639530);
    RH_ScopedVMTInstall(ControlSubTask, 0x634890);
}

CTaskComplexUseClosestFreeScriptedAttractor::CTaskComplexUseClosestFreeScriptedAttractor(eMoveState ms) :
    m_MoveState{ms}
{
}

CTaskComplexUseClosestFreeScriptedAttractor::CTaskComplexUseClosestFreeScriptedAttractor(const CTaskComplexUseClosestFreeScriptedAttractor&) :
    CTaskComplexUseClosestFreeScriptedAttractor{}
{
}

// 0x634740
C2dEffect* CTaskComplexUseClosestFreeScriptedAttractor::ComputeClosestFreeScriptedEffect(CPed const& ped) {
    C2dEffect* closestEffect = nullptr;
    float      closestDistSq = FLT_MAX;

    for (int32 i = 0; i < NUM_SCRIPTED_2D_EFFECTS; i++) {
        if (!CScripted2dEffects::ms_activated[i]) {
            continue;
        }

        // Check if the ped is allowed to use this scripted effect
        const auto& userList = CScripted2dEffects::ms_userLists[i];
        if (userList.m_bUseList) {
            bool isAllowed = false;
            for (const auto modelId : userList.m_UserTypes) {
                if (modelId == static_cast<int32>(ped.GetModelIndex())) {
                    isAllowed = true;
                    break;
                }
            }
            if (!isAllowed) {
                for (size_t j = 0; j < userList.m_UserTypes.size(); j++) {
                    if (userList.m_UserTypes[j] == -2 && userList.m_UserTypesByPedType[j] == static_cast<int32>(ped.m_nPedType)) {
                        isAllowed = true;
                        break;
                    }
                }
            }
            if (!isAllowed) {
                continue;
            }
        }

        const auto effect = &CScripted2dEffects::ms_effects[i];

        const float distSq = (effect->m_Pos - ped.GetPosition()).SquaredMagnitude();
        if (distSq >= closestDistSq) {
            continue;
        }

        // TODO(re): The original also constructs (and destroys) a temporary `CMatrix` (SetScale(1.0f)) here, unused by this routine.

        if (!GetPedAttractorManager()->HasEmptySlot(reinterpret_cast<const C2dEffectPedAttractor*>(effect), nullptr)) {
            continue;
        }

        closestDistSq = distSq;
        closestEffect = effect;
    }

    return closestEffect;
}

// 0x639530
CTask* CTaskComplexUseClosestFreeScriptedAttractor::CreateFirstSubTask(CPed* ped) {
    const auto effect = ComputeClosestFreeScriptedEffect(*ped);
    if (!effect) {
        return nullptr;
    }

    g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_3, ped);

    const auto fx = reinterpret_cast<C2dEffectPedAttractor*>(effect);

    switch (m_MoveState) {
    case PEDMOVE_RUN:
        return new CTaskComplexUseEffectRunning{fx, nullptr};
    case PEDMOVE_SPRINT:
        return new CTaskComplexUseEffectSprinting{fx, nullptr};
    case PEDMOVE_WALK:
    default:
        return new CTaskComplexUseEffect{fx, nullptr};
    }
}
