#pragma once

#include <Base.h>
#include <PluginBase.h>
#include <reversiblehooks/ReversibleHooks.h>

#include "./PedGroupDefaultTaskAllocator.h"
#include "TaskComplexFollowLeaderInFormation.h"
#include "TaskComplexGangFollower.h"
#include "TaskComplexGangLeader.h"

class NOTSA_EXPORT_VTABLE CPedGroupDefaultTaskAllocatorRandom final : public CPedGroupDefaultTaskAllocator {
public:
    /* no virtual destructor */

    // 0x5F6530
    ePedGroupDefaultTaskAllocatorType GetType() const override { return ePedGroupDefaultTaskAllocatorType::RANDOM; };

    // 0x5F6E90
    void AllocateDefaultTasks(CPedGroup* pedGroup, CPed* ped) const override {
        const auto leader = pedGroup->GetMembership().GetLeader();
        auto&      pairs  = pedGroup->GetIntelligence().GetDefaultPedTaskPairs();

        // Followers: each one follows the leader at its standing formation offset
        for (auto i = 0; i < TOTAL_PED_GROUP_FOLLOWERS; i++) {
            auto& tp = pairs[i];
            if (!tp.Ped) {
                continue;
            }
            if (ped && tp.Ped != ped) {
                continue;
            }
            auto* const task = new CTaskComplexGangFollower{
                pedGroup,
                leader,
                static_cast<uint8>(i),
                CVector{CTaskComplexFollowLeaderInFormation::ms_offsets.Offsets[i]},
                10.f
            };
            task->m_FollowLeader = pedGroup->m_bMembersEnterLeadersVehicle; // 0x65EE90
            tp.Task = task;
        }

        // The leader's own slot gets a gang-leader task
        auto& leaderPair = pairs[CPedGroupMembership::LEADER_MEM_ID];
        if (leaderPair.Ped && (!ped || leaderPair.Ped == ped)) {
            leaderPair.Task = new CTaskComplexGangLeader{pedGroup};
        }
    }

public:
    static inline void InjectHooks() {
        RH_ScopedVirtualClass(CPedGroupDefaultTaskAllocatorRandom, 0x86C77C, 2);
        RH_ScopedCategory("Tasks/Allocators/PedGroup");

        RH_ScopedVMTInstall(AllocateDefaultTasks, 0x5F6E90);
        RH_ScopedVMTInstall(GetType, 0x5F6530);
    }
};
VALIDATE_SIZE(CPedGroupDefaultTaskAllocatorRandom, sizeof(void*)); /* vtable only */
