#include "StdInc.h"

#include "PedShelterAttractor.h"

#include "Tasks/TaskTypes/TaskComplexGoToAttractor.h"

#include <numbers>

namespace {
constexpr float SHELTER_TWO_PI = std::numbers::pi_v<float> * 2.0f; // 0x858CBC
}

// 0x5EF420
CVector* CPedShelterAttractor::GetDisplacement(int32 pedId) {
    if (ms_displacements.empty()) {
        for (auto i = 0; i < 5; i++) {
            CVector candidate;
            bool    tooClose;
            do { // Rejection sampling: retry until not within 1 unit of an existing displacement
                const auto angle  = (float)rand() * (1.0f / 32767.0f) * SHELTER_TWO_PI; // 0x858C7C, 0x858CBC
                const auto radius = (float)rand() * (1.0f / 32767.0f) * 2.0f;
                candidate = { std::cos(angle) * radius, std::sin(angle) * radius, 0.0f };
                tooClose  = rng::any_of(ms_displacements, [&](const CVector& d) { return (d - candidate).MagnitudeSquared() < 1.0f; }); // 0x858624
            } while (tooClose);
            ms_displacements.emplace_back(candidate); // 0x5EE370
        }
    }
    return ms_displacements.begin() + pedId; // NOTE: `operator[]` returns by value, so go through `begin()`
}

// 0x5EFC40
void CPedShelterAttractor::ComputeAttractPos(int32 pedId, CVector& outPos) {
    if (m_Fx) {
        outPos = *GetDisplacement(pedId) + m_Pos;
    }
}

// 0x5E9690
void CPedShelterAttractor::ComputeAttractHeading(int32 bQueue, float& heading) {
    heading = CGeneral::GetRandomNumberInRange(0.0f, SHELTER_TWO_PI);
}

// 0x5EF570
bool CPedShelterAttractor::BroadcastDeparture(CPed* ped) {
    const auto it = rng::find(m_ArrivedPeds, ped);
    if (it == m_ArrivedPeds.end()) {
        return false;
    }
    const auto pair = rng::find(m_PedTaskPairs, ped, &CPedTaskPair::Ped);
    if (pair != m_PedTaskPairs.end()) {
        m_PedTaskPairs.erase(pair);
    }
    m_ArrivedPeds.erase(it);
    for (auto* const attractedPed : m_AttractPeds) {
        const auto n = (int32)(m_ArrivedPeds.size());
        SetTaskForPed(attractedPed, new CTaskComplexGoToAttractor{
            this,
            ComputeAttractPos(n),
            ComputeAttractHeading(n),
            m_AchieveQueueTime,
            n,
            PEDMOVE_WALK
        });
    }
    return true;
}

void CPedShelterAttractor::InjectHooks() {
    RH_ScopedVirtualClass(CPedShelterAttractor, 0x86C5B4, 6);
    RH_ScopedCategory("Attractors");

    RH_ScopedInstall(GetDisplacement, 0x5EF420, { .Reversed = false }); // not installed: CPedShelterAttractor::InjectHooks() is never called
    RH_ScopedVMTInstall(ComputeAttractPos, 0x5EFC40, { .Reversed = false }); // not installed: CPedShelterAttractor::InjectHooks() is never called
    RH_ScopedVMTInstall(ComputeAttractHeading, 0x5E9690, { .Reversed = false }); // not installed: CPedShelterAttractor::InjectHooks() is never called
    RH_ScopedVMTInstall(BroadcastDeparture, 0x5EF570, { .Reversed = false }); // not installed: CPedShelterAttractor::InjectHooks() is never called
}
