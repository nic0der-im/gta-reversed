#include "StdInc.h"

#include "TaskAllocatorKillThreatsBasic.h"
#include <InterestingEvents.h>
#include <TaskComplexKillPedGroupOnFoot.h>
#include <TaskComplexSequence.h>
#include <TaskSimpleLookAbout.h>

// 0x69C710
CTaskAllocatorKillThreatsBasic::CTaskAllocatorKillThreatsBasic(CPed* threat) :
    m_Threat{threat}
{
}

// 0x69D170
void CTaskAllocatorKillThreatsBasic::AllocateTasks(CPedGroupIntelligence* intel) {
    intel->FlushTasks(intel->GetPedTaskPairs(), nullptr);
    intel->FlushTasks(intel->GetSecondaryPedTaskPairs(), nullptr);

    if (!m_Threat) {
        return;
    }

    auto* const group = &intel->GetPedGroup();
    if (auto* const threatsGroup = m_Threat->GetGroup()) {
        if (threatsGroup == group) {
            NOTSA_LOG_DEBUG("ComputeKillThreatsBasicResponse() - threat ped already in group"); // vanilla
        } else {
            CPed* closest[TOTAL_PED_GROUP_MEMBERS]{};
            ComputeClosestPeds(*group, *threatsGroup, closest);
            for (int32 i = 0; i < TOTAL_PED_GROUP_MEMBERS; i++) {
                auto* mem = group->GetMembership().GetMember(i);
                if (!mem || mem->IsPlayer()) {
                    continue;
                }
                intel->SetEventResponseTask(
                    mem,
                    CTaskComplexKillPedGroupOnFoot{ CPedGroups::GetGroupId(threatsGroup), closest[i] }
                );
            }
            g_InterestingEvents.Add(CInterestingEvents::GANG_FIGHT, group->GetMembership().GetLeader()); // 0x69D436
        }
    } else { // 0x69D2EB
        for (auto* const mem : group->GetMembership().GetMembers()) {
            if (mem->IsPlayer()) {
                continue;
            }
            intel->SetEventResponseTask(
                mem,
                CTaskComplexSequence{
                    new CTaskComplexKillPedOnFoot{ m_Threat },
                    new CTaskSimpleLookAbout{ CGeneral::GetRandomNumberInRange(1'000u, 2'000u) },
                }
            );
        }
        g_InterestingEvents.Add(CInterestingEvents::GANG_ATTACKING_PED, group->GetMembership().GetLeader()); // 0x69D436
    }
}

// 0x69C7E0
CTaskAllocator* CTaskAllocatorKillThreatsBasic::ProcessGroup(CPedGroupIntelligence* intel) {
    m_Timer.StartIfNotAlready(0);
    if (m_Timer.IsOutOfTime()) {
        m_Timer.Start(5'000);
        AllocateTasks(intel);
    }
    return this;
}

// 0x69C850
void CTaskAllocatorKillThreatsBasic::ComputeClosestPeds(CPedGroup& group1, CPedGroup& group2, CPed** peds) {
    constexpr auto N = TOTAL_PED_GROUP_MEMBERS;

    auto& mem1 = group1.GetMembership();
    auto& mem2 = group2.GetMembership();

    std::fill_n(peds, N, nullptr);

    // distSq[i][j] = squared distance between group1's member i, and group2's member j
    float distSq[N][N];
    for (auto& row : distSq) {
        std::fill(std::begin(row), std::end(row), FLT_MAX);
    }

    for (int32 i = 0; i < N; i++) {
        auto* const m1 = mem1.GetMember(i);
        if (!m1 || !m1->IsAlive() || m1->IsPlayer()) {
            continue;
        }
        for (int32 j = 0; j < N; j++) {
            auto* const m2 = mem2.GetMember(j);
            if (!m2 || !m2->IsAlive()) {
                continue;
            }
            distSq[i][j] = (m1->GetPosition() - m2->GetPosition()).MagnitudeSquared();
        }
    }

    // Greedily pair up the closest members (each member of group1 and group2 is used at most once)
    for (int32 n = 0; n < N; n++) {
        int32 bestI = -1, bestJ = -1;
        float best  = FLT_MAX;
        for (int32 i = 0; i < N; i++) {
            for (int32 j = 0; j < N; j++) {
                if (distSq[i][j] < best) {
                    best  = distSq[i][j];
                    bestI = i;
                    bestJ = j;
                }
            }
        }
        if (bestI < 0 || bestJ < 0) {
            continue;
        }
        for (int32 k = 0; k < N; k++) {
            distSq[bestI][k] = FLT_MAX;
            distSq[k][bestJ] = FLT_MAX;
        }
        peds[bestI] = mem2.GetMember(bestJ);
    }

    // Peds that didn't get a target are given the leader (or if there's none/he's dead, the first alive member)
    CPed* fallback = mem1.GetLeader();
    if (!fallback || !fallback->IsAlive()) {
        fallback = nullptr;
        // NOTE: Original code only checks the first 7 members (0x69CAAD)
        for (int32 i = 0; i < N - 1; i++) {
            if (auto* const m = mem1.GetMember(i); m && m->IsAlive()) {
                fallback = m;
                break;
            }
        }
        if (!fallback) {
            return;
        }
    }
    for (int32 i = 0; i < N; i++) {
        if (mem1.GetMember(i) && !peds[i]) {
            peds[i] = fallback;
        }
    }
}

void CTaskAllocatorKillThreatsBasic::InjectHooks() {
    RH_ScopedVirtualClass(CTaskAllocatorKillThreatsBasic, 0x870e90, 6);
    RH_ScopedCategory("Tasks/Allocators");

    RH_ScopedInstall(Constructor, 0x69C710);
    RH_ScopedInstall(Destructor, 0x69C780);

    RH_ScopedGlobalInstall(ComputeClosestPeds, 0x69C850);
    RH_ScopedVMTInstall(GetType, 0x69C770);
    RH_ScopedVMTInstall(AllocateTasks, 0x69D170);
    RH_ScopedVMTInstall(ProcessGroup, 0x69C7E0);
}
