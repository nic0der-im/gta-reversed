#include "StdInc.h"

#include "TaskComplexWanderCriminal.h"

#include "CarEnterExit.h"
#include "EventVehicleToSteal.h"
#include "Vehicle.h"
#include "VehicleScanner.h"

// 0x48E610
CTaskComplexWanderCriminal::CTaskComplexWanderCriminal(eMoveState MoveState, uint8 Dir, bool bWanderSensibly) : CTaskComplexWander(MoveState, Dir, bWanderSensibly) { }

// 0x670350
void CTaskComplexWanderCriminal::ScanForStuff(CPed* ped) {
    if (!m_TaskTimer.m_bStarted) {
        m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
        m_TaskTimer.m_nInterval  = 50;
        m_TaskTimer.m_bStarted   = true;

        // Delays the next search by a random amount of up to 30s.
        // NOTE: Retail computes `now - (int32)(rand * (1/32768) * -30000)`, which is the same as
        //       `now + (int32)(rand * (1/32768) * 30000)` (The subtraction of a negative value)
        m_nMinNextScanTime = CTimer::GetTimeInMS() - (uint32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * -30'000.0f);
    }

    if (m_TaskTimer.m_bStarted) {
        if (m_TaskTimer.m_bStopped) {
            m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_TaskTimer.m_bStopped   = false;
        }

        if (CTimer::GetTimeInMS() >= m_TaskTimer.m_nStartTime + m_TaskTimer.m_nInterval) {
            m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_TaskTimer.m_nInterval  = 50;
            m_TaskTimer.m_bStarted   = true;

            if (CTimer::GetTimeInMS() >= m_nMinNextScanTime) {
                LookForCarsToSteal(ped);
            }
        }
    }
}

// 0x66B4F0
void CTaskComplexWanderCriminal::LookForCarsToSteal(CPed* ped) {
    CVehicle* closestVehicle = nullptr;
    float     closestDist    = std::numeric_limits<float>::max(); // FLT_MAX

    for (auto& vehicle : ped->GetIntelligence()->GetVehicleScanner().GetEntities<CVehicle>()) {
        if (!CCarEnterExit::IsVehicleStealable(&vehicle, ped)) {
            continue;
        }

        const auto dist = (vehicle.GetPosition() - ped->GetPosition()).SquaredMagnitude();
        if (dist < closestDist) {
            closestDist    = dist;
            closestVehicle = &vehicle;
        }
    }

    if (!closestVehicle) {
        return;
    }

    m_nMinNextScanTime = CTimer::GetTimeInMS() + 30'000;

    // Retail only reacts to ~20% of the found vehicles
    if ((float)CGeneral::GetRandomNumber() * (1.0f / 32767.0f) < 0.2f) {
        CEventVehicleToSteal event{ closestVehicle };
        ped->GetEventGroup().Add(&event, false);
    }
}
