#include "StdInc.h"

#include "TaskSimpleCarForcePedOut.h"

#include "CarEnterExit.h"
#include "Vehicle.h"
#include "Ped.h"

// 0x647710
CTaskSimpleCarForcePedOut::CTaskSimpleCarForcePedOut(CVehicle* vehicle, int32 door) :
    m_vehicle{ vehicle },
    m_door{ door }
{
    CEntity::SafeRegisterRef(m_vehicle);
}

// 0x647790
CTaskSimpleCarForcePedOut::~CTaskSimpleCarForcePedOut() {
    CEntity::SafeCleanUpRef(m_vehicle);
}

// 0x6477F0
bool CTaskSimpleCarForcePedOut::ProcessPed(CPed* ped) {
    if (!m_vehicle) {
        return true;
    }

    if (m_vehicle->IsDriver(ped)) {
        ped->PositionPedOutOfCollision(m_door, m_vehicle, true);
        return true;
    }

    // Only allowed to get out if the door was set up to be used to get out of
    if (CCarEnterExit::ComputeOppositeDoorFlag(m_vehicle, m_door, false) & m_vehicle->m_nGettingOutFlags) {
        if (m_vehicle->m_pDriver) {
            return false;
        }

        // No other passenger may be in the way of the door
        // NOTE: The original code uses the seat index returned here as the amount of seats to check
        const auto numSeatsToCheck = CCarEnterExit::ComputePassengerIndexFromCarDoor(m_vehicle, m_door);
        for (auto i = 0; i < numSeatsToCheck; i++) {
            if (m_vehicle->m_apPassengers[i]) {
                return false;
            }
        }
    }

    ped->PositionPedOutOfCollision(m_door, m_vehicle, true);
    return true;
}
