#include "StdInc.h"
#include "TaskComplexUseClosestFreeScriptedAttractor.h"

#include "TaskComplexUseEffect.h"
#include "TaskComplexUseEffectRunning.h"
#include "TaskComplexUseEffectSprinting.h"
#include "PedAttractorManager.h"
#include "InterestingEvents.h"
#include "Scripts/Scripted2dEffects.h"

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
C2dEffect* __stdcall CTaskComplexUseClosestFreeScriptedAttractor::ComputeClosestFreeScriptedEffect(CPed const& ped) {
    const auto pedPos = ped.GetPosition();

    C2dEffect* closest{};
    float      closestDistSq = std::numeric_limits<float>::max();
    for (auto i = 0; i < NUM_SCRIPTED_2D_EFFECTS; i++) {
        if (!CScripted2dEffects::ms_activated[i]) {
            continue;
        }
        const auto& users = CScripted2dEffects::ms_userLists[i];
        if (users.m_bUseList) { // Restricted to certain users only?
            const auto isUser =
                rng::contains(users.m_UserTypes, (int32)(int16)ped.m_nModelIndex) // 0x634795
             || rng::any_of(rngv::iota(0, 4), [&](int32 u) { // 0x6347ab
                    return users.m_UserTypes[u] == -2 && users.m_UserTypesByPedType[u] == (int32)ped.m_nPedType;
                });
            if (!isUser) {
                continue;
            }
        }
        auto* const fx     = CScripted2dEffects::GetEffect(i);
        const auto  distSq = (pedPos - fx->m_Pos).SquaredMagnitude(); // NOTE: effect position is at offset 0 of the 0x40-byte entry
        if (distSq < closestDistSq && GetPedAttractorManager()->HasEmptySlot(fx, nullptr)) {
            closestDistSq = distSq;
            closest       = reinterpret_cast<C2dEffect*>(fx); // binary compatible
        }
    }
    return closest;
}

// 0x639530
CTask* CTaskComplexUseClosestFreeScriptedAttractor::CreateFirstSubTask(CPed* ped) {
    auto* const fx = reinterpret_cast<C2dEffectPedAttractor*>(ComputeClosestFreeScriptedEffect(*ped));
    if (!fx) {
        return nullptr;
    }
    g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_3, ped);
    switch (m_MoveState) {
    case PEDMOVE_RUN:    return new CTaskComplexUseEffectRunning{ fx, nullptr };
    case PEDMOVE_SPRINT: return new CTaskComplexUseEffectSprinting{ fx, nullptr };
    default:             return new CTaskComplexUseEffect{ fx, nullptr };
    }
}
