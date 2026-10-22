#include "StdInc.h"

#include "TaskComplexWanderProstitute.h"
#include "TaskComplexProstituteSolicit.h"
#include "EventSexyPed.h"
#include "General.h"

// 0x672690
CTaskComplexWanderProstitute::CTaskComplexWanderProstitute(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) :
    CTaskComplexWanderStandard(MoveState, Dir, bWanderSensibly),
    m_nStartTimeInMs{ 0 }
{
}

// 0x672700
void CTaskComplexWanderProstitute::ScanForStuff(CPed* ped) {
    CTaskComplexWanderStandard::ScanForStuff(ped);

    if (CTimer::GetTimeInMS() <= m_nStartTimeInMs) {
        return;
    }
    m_nStartTimeInMs = CTimer::GetTimeInMS() + 2'000;

    const auto entities = ped->GetIntelligence()->GetPedEntities();

    // There's already another prostitute around - Don't solicite
    for (auto i = 0u; i < MAX_NUM_ENTITIES; i++) {
        const auto candidate = static_cast<CPed*>(entities[i]);
        if (candidate && candidate->m_nPedType == PED_TYPE_PROSTITUTE) {
            return;
        }
    }

    for (auto i = 0u; i < MAX_NUM_ENTITIES; i++) {
        const auto candidate = static_cast<CPed*>(entities[i]);
        if (!candidate) {
            continue;
        }
        if (!candidate->GetPlayerData()) {
            continue;
        }
        if (!candidate->m_pVehicle) {
            continue;
        }
        // Already shagged this one
        if (candidate->GetPlayerData()->m_pLastProstituteShagged == ped) {
            continue;
        }

        // The pimpier the car, the more likely a client is chosen
        // (`CGeneral::GetRandomNumber()` is scaled by `1 / 32768` and `50'000` (0x858B14, 0x870184))
        if (candidate->m_pVehicle->m_pHandlingData->m_nMonetaryValue <= (int32)(CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 50'000.0f)) {
            continue;
        }

        // The car has to be moving (at least ~2/50 units per second)
        if ((candidate->m_pVehicle->GetMoveSpeed() * 50.0f).SquaredMagnitude() < 4.0f) {
            continue;
        }

        if (!CTaskComplexProstituteSolicit::IsTaskValid(ped, candidate)) {
            continue;
        }

        CEventSexyPed event{ candidate, TASK_COMPLEX_PROSTITUTE_SOLICIT };
        ped->GetEventGroup().Add(&event, false);
    }
}
