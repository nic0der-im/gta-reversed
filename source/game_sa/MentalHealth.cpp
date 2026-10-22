#include "StdInc.h"
#include "MentalHealth.h"

#include "Events/EventHealthReallyLow.h"
#include "Events/EventHealthLow.h"
#include "Events/EventLowAngerAtPlayer.h"
#include "Events/EventHighAngerAtPlayer.h"

// 0x6008A0
void CMentalState::Process(CPed* ped) {
    m_pedHealth = (uint8)(int32)ped->m_fHealth;

    if (ped->bInVehicle && ped->m_pVehicle) {
        m_vehicleHealth = (uint8)(int32)ped->m_pVehicle->m_fHealth;
    }

    // Health events (the thresholds are on *bytes*, so health above 255 wraps)
    eEventType healthEvent = EVENT_NONE;
    if (ped->bInVehicle) {
        // NOTE: The shipped code zero-extends the byte (`movzbl`) before comparing it against these
        //       thresholds, so `m_oldVehicleHealth >= 600` is never true (a byte can't reach 600).
        if ((int32)m_oldVehicleHealth >= 600) {
            if ((int32)m_vehicleHealth < 300) {
                healthEvent = EVENT_HEALTH_REALLY_LOW;
            } else if ((int32)m_vehicleHealth < 600) {
                healthEvent = EVENT_HEALTH_LOW;
            }
        } else if ((int32)m_oldVehicleHealth >= 300 && (int32)m_vehicleHealth < 300) {
            healthEvent = EVENT_HEALTH_REALLY_LOW;
        }
    } else {
        if (m_oldPedHealth >= 50) {
            if (m_pedHealth < 10) {
                healthEvent = EVENT_HEALTH_REALLY_LOW;
            } else if (m_pedHealth < 50) {
                healthEvent = EVENT_HEALTH_LOW;
            }
        } else if (m_oldPedHealth >= 10 && m_pedHealth < 10) {
            healthEvent = EVENT_HEALTH_REALLY_LOW;
        }
    }

    auto& eventGroup = ped->GetEventGroup();
    switch (healthEvent) {
    case EVENT_HEALTH_REALLY_LOW: {
        CEventHealthReallyLow event{};
        eventGroup.Add(&event, false);
        break;
    }
    case EVENT_HEALTH_LOW: {
        CEventHealthLow event{};
        eventGroup.Add(&event, false);
        break;
    }
    default:
        break;
    }

    // Anger at player events
    if (m_LastAngerAtPlayer <= 3) {
        if (m_AngerAtPlayer <= 6) {
            if (m_AngerAtPlayer > 3) {
                CEventLowAngerAtPlayer event{};
                eventGroup.Add(&event, false);
            }
        } else {
            CEventHighAngerAtPlayer event{};
            eventGroup.Add(&event, false);
        }
    } else if (m_LastAngerAtPlayer <= 6 && m_AngerAtPlayer > 6) {
        CEventHighAngerAtPlayer event{};
        eventGroup.Add(&event, false);
    }

    m_LastAngerAtPlayer = m_AngerAtPlayer;
    m_oldPedHealth      = m_pedHealth;
    m_oldVehicleHealth  = m_vehicleHealth;
}
