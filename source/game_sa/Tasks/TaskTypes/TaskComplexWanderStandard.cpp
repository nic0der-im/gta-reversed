#include "StdInc.h"

#include "TaskComplexWanderStandard.h"

#include "EventAcquaintancePed.h"
#include "EventAcquaintancePedRespect.h"
#include "EventChatPartner.h"
#include "EventSexyVehicle.h"
#include "GameLogic.h"
#include "PedGroups.h"
#include "Streaming.h"
#include "SurfaceInfos_c.h"
#include "Vehicle.h"
#include "World.h"

// 0x48E4F0
CTaskComplexWanderStandard::CTaskComplexWanderStandard(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) :
    CTaskComplexWander(MoveState, Dir, bWanderSensibly),
    m_nMinNextScanTime{ 0 }
{
}

// 0x672600
void CTaskComplexWanderStandard::ScanForStuff(CPed* ped) {
    if (!m_TaskTimer.m_bStarted) {
        m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
        m_TaskTimer.m_nInterval = 50;
        m_TaskTimer.m_bStarted = true;
    }

    if (CTimer::GetTimeInMS() < m_nMinNextScanTime)
        return;

    if (m_TaskTimer.m_bStarted) { // V547 Expression 'm_TaskTimer.m_bStarted' is always true.
        if (m_TaskTimer.m_bStopped) {
            m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_TaskTimer.m_bStopped = false;
        }

        if (CTimer::GetTimeInMS() >= m_TaskTimer.m_nStartTime + m_TaskTimer.m_nInterval) {
            m_TaskTimer.m_nInterval = 50;
            m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_TaskTimer.m_bStarted = true;
            if (!LookForGangMembers(ped) && !LookForChatPartners(ped)) {
                CTaskComplexWanderStandard::LookForSexyCars(ped);
            }
        }
    }
}

// 0x66AED0
bool CTaskComplexWanderStandard::WillChat(const CPed& first, const CPed& second) {
    if (first.m_nPedType == PED_TYPE_CRIMINAL || second.m_nPedType == PED_TYPE_CRIMINAL) {
        return false;
    }
    if (first.m_nPedType == PED_TYPE_COP || second.m_nPedType == PED_TYPE_COP) {
        return false;
    }
    if (first.IsPlayer() || second.IsPlayer()) {
        return false;
    }
    if (first.IsGangster() || second.IsGangster()) {
        return false;
    }
    return CPedIntelligence::AreFriends(first, second);
}

// 0x66AF60
void CTaskComplexWanderStandard::SetNextMinScanTime(CPed* ped) {
    CTask* const active = ped->GetTaskManager().GetActiveTask();
    if (!active || active->GetTaskType() != GetTaskType()) {
        return;
    }

    auto* const activeWander = static_cast<CTaskComplexWander*>(active);
    if (activeWander->GetWanderType() != GetWanderType()) {
        return;
    }

    static_cast<CTaskComplexWanderStandard*>(active)->m_nMinNextScanTime = CTimer::GetTimeInMS() + 100'000;
}

// 0x66AFD0
bool CTaskComplexWanderStandard::LookForSexyCars(CPed* ped) {
    const auto& pedPos = ped->GetPosition();

    for (auto& vehicle : ped->GetIntelligence()->GetVehicleScanner().GetEntities<CVehicle>()) {
        if (&vehicle == ped->m_pVehicle) {
            continue;
        }
        if (vehicle.m_pHandlingData->m_nMonetaryValue <= 40'000) {
            continue;
        }
        if (vehicle.m_fHealth <= 500.0f) {
            continue;
        }

        const auto delta = vehicle.GetPosition() - pedPos;
        if (delta.SquaredMagnitude() >= 25.0f) {
            continue;
        }
        if (delta.Dot(ped->GetForward()) <= 0.0f) {
            continue;
        }
        if (!CWorld::GetIsLineOfSightClear(pedPos, vehicle.GetPosition(), true, false, false, true, false, false, false)) {
            continue;
        }

        CEventSexyVehicle event{ &vehicle };
        ped->GetEventGroup().Add(&event, false);
        SetNextMinScanTime(ped);
        return true;
    }
    return false;
}

// 0x66FDA0
bool CTaskComplexWanderStandard::LookForChatPartners(CPed* ped) {
    if (!g_surfaceInfos.IsPavement((SurfaceId)ped->m_nContactSurface)) {
        return false;
    }
    if (m_nMoveState >= PEDMOVE_JOG) { // Only walking peds chat
        return false;
    }

    if (const auto* const playerPed = CWorld::Players[CWorld::PlayerInFocus].m_pPed) {
        if (playerPed->bInVehicle) {
            if (const auto* const playerVeh = playerPed->m_pVehicle) {
                if (playerVeh->GetMoveSpeed().SquaredMagnitude() > 0.04f) {
                    return false;
                }
            }
        }
    }

    if (CStreaming::IsVeryBusy() || CGameLogic::LaRiotsActiveHere()) {
        return false;
    }

    const auto& pedPos = ped->GetPosition();
    for (auto& candidate : ped->GetIntelligence()->GetPedScanner().GetEntities<CPed>()) {
        if (!g_surfaceInfos.IsPavement((SurfaceId)candidate.m_nContactSurface)) {
            continue;
        }

        const CTask* const active = candidate.GetTaskManager().GetActiveTask();
        if (!active || active->GetTaskType() != GetTaskType()) {
            continue;
        }

        // Neither ped may already be chatting or in a couple
        if (candidate.GetIntelligence()->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT)
            || ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT)
        ) {
            continue;
        }
        if (candidate.GetEventGroup().GetEventOfType(EVENT_CHAT_PARTNER)
            || ped->GetEventGroup().GetEventOfType(EVENT_CHAT_PARTNER)
        ) {
            continue;
        }
        if (candidate.GetIntelligence()->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE)
            || ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE)
        ) {
            continue;
        }

        if (!WillChat(*ped, candidate)) {
            continue;
        }

        const auto delta = candidate.GetPosition() - pedPos;
        if (delta.SquaredMagnitude() >= 100.0f) {
            continue;
        }
        if (delta.Dot(ped->GetForward()) <= 0.0f) {
            continue;
        }
        if (delta.Dot(candidate.GetForward()) >= 0.0f) {
            continue;
        }
        if (!CWorld::GetIsLineOfSightClear(pedPos, candidate.GetPosition(), true, false, false, true, false, false, false)) {
            continue;
        }

        CEventChatPartner pedEvent{ true, &candidate };
        ped->GetEventGroup().Add(&pedEvent, false);
        CEventChatPartner candidateEvent{ false, ped };
        candidate.GetEventGroup().Add(&candidateEvent, false);

        SetNextMinScanTime(ped);
        SetNextMinScanTime(&candidate);
        return true;
    }
    return false;
}

// 0x670100
bool CTaskComplexWanderStandard::LookForGangMembers(CPed* ped) {
    if (CPedGroups::GetPedsGroup(ped)) {
        return false;
    }

    CVector targetPos;
    ComputeTargetPos(ped, targetPos, m_NextNode);

    const auto& pedPos = ped->GetPosition();
    for (auto& candidate : ped->GetIntelligence()->GetPedScanner().GetEntities<CPed>()) {
        const auto delta = candidate.GetPosition() - targetPos;
        if (delta.Dot(pedPos - targetPos) <= 0.0f) {
            continue;
        }
        if (delta.SquaredMagnitude() >= 25.0f) {
            continue;
        }

        CPedGroup* const group = CPedGroups::GetPedsGroup(&candidate);
        if (!group || group->GetMembership().CountMembersExcludingLeader() <= 0) {
            continue;
        }

        // The exe constructs the (abstract in the repo) base event here; `Respect` is its
        // nearest concrete type (the base's `GetEventType` was not reconstructed).
        CEventAcquaintancePedRespect event{ &candidate };
        ped->GetEventGroup().Add(&event, false);
        SetNextMinScanTime(ped);
        return true;
    }
    return false;
}
