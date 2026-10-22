#include "StdInc.h"

#include "TaskSimpleCarSetPedOut.h"

#include "Garages.h"
#include "CarEnterExit.h"
#include "Bike.h"
#include "TaskComplexWander.h"
#include "TaskSimplePlayerOnFoot.h"
#include "TaskSimpleStandStill.h"

// 0x6478B0
CTaskSimpleCarSetPedOut::CTaskSimpleCarSetPedOut(CVehicle* targetVehicle, eTargetDoor nTargetDoor, bool bSwitchOffEngine, bool warpingOutOfCar) :
    m_nTargetDoor{ nTargetDoor },
    m_pTargetVehicle{ targetVehicle },
    m_bSwitchOffEngine{ bSwitchOffEngine },
    m_bWarpingOutOfCar{ warpingOutOfCar }
{
    CEntity::SafeRegisterRef(m_pTargetVehicle);
}

// 0x647950
CTaskSimpleCarSetPedOut::~CTaskSimpleCarSetPedOut() {
    CEntity::SafeCleanUpRef(m_pTargetVehicle);
}

// 0x6479B0
void CTaskSimpleCarSetPedOut::PositionPedOutOfCollision(CPed* ped, CVehicle* veh, int32 door) {
    const auto vehicle = veh ? veh : ped->m_pVehicle;
    if (!vehicle) {
        return;
    }

    // Face the same way as the vehicle (i.e. the opposite of the vehicle's "right" axis)
    const auto heading = vehicle->m_matrix
        ? CGeneral::GetATanOfXY(-vehicle->m_matrix->GetRight().y, vehicle->m_matrix->GetRight().x)
        : vehicle->m_placement.m_fHeading;

    ped->m_fCurrentRotation = heading;
    ped->m_fAimingRotation  = heading;
    if (ped->m_matrix) {
        ped->m_matrix->SetRotateZOnly(heading);
    } else {
        ped->m_placement.m_fHeading = heading;
    }

    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
        // Boats bob up and down, so lift the ped halfway up the boat's "up" axis first
        if (vehicle->m_pHandlingData->m_nModelFlags & VEHICLE_HANDLING_MODEL_SIT_IN_BOAT) {
            ped->GetPosition() = ped->GetPosition() + vehicle->GetMatrix().GetUp() * 0.5f;
        }

        if (ped->TestCollision(false)) {
            if (vehicle->vehicleFlags.bLowVehicle) {
                ped->GetPosition().z -= 0.3f;
                if (!ped->TestCollision(false)) {
                    return;
                }
                ped->GetPosition().z += 0.3f;
                ped->PositionPedOutOfCollision(door, vehicle, true);
                return;
            }
            ped->GetPosition() = ped->GetPosition() - ped->GetMatrix().GetForward() * 0.3f;
            if (!ped->TestCollision(false)) {
                return;
            }
            ped->GetPosition() = ped->GetPosition() + ped->GetMatrix().GetForward() * 0.3f;
            ped->PositionPedOutOfCollision(door, vehicle, true);
            return;
        }

        ped->m_vecMoveSpeed = vehicle->m_vecMoveSpeed * 0.9f;
        ped->m_vecMoveSpeed.z -= 0.1f;
        ped->bIsStanding = true;
        if (!ped->m_standingOnEntity) {
            ped->m_standingOnEntity = vehicle;
            CEntity::SafeRegisterRef(ped->m_standingOnEntity);
        }
        return;
    }

    const auto pedPos = ped->GetPosition();

    const auto HitsSomethingNotAttachedToVehicle = [&](float zOffset) {
        const auto hit = CWorld::TestSphereAgainstWorld(
            { pedPos.x, pedPos.y, pedPos.z + zOffset },
            0.4f,
            vehicle,
            true, true, false, false, false, false
        );
        return hit && hit != vehicle->m_pAttachedTo;
    };

    const auto collides = HitsSomethingNotAttachedToVehicle(-0.2f) || HitsSomethingNotAttachedToVehicle(0.2f);

    if (collides || !CWorld::GetIsLineOfSightClear(vehicle->GetPosition(), pedPos, true, false, false, true, false, false, false)) {
        ped->PositionPedOutOfCollision(door, vehicle, true);
    }
}

// 0x647D10
bool CTaskSimpleCarSetPedOut::ProcessPed(CPed* ped) {
    ped->bInVehicle = false;
    ped->m_bUsesCollision = true;
    ped->UpdateStatLeavingVehicle();

    if (!m_bKnockedOffBike) {
        PositionPedOutOfCollision(ped, nullptr, m_nTargetDoor);
    }
    CCarEnterExit::RemoveCarSitAnim(ped);
    ped->RestartNonPartialAnims();

    if (!m_bKnockedOffBike && !m_bFallingOutOfCar && m_pTargetVehicle->m_nVehicleType != VEHICLE_TYPE_BOAT) {
        ped->m_vecMoveSpeed.Reset();
    }

    if (const auto veh = ped->m_pVehicle) {
        if (m_nDoorFlagsToClear) {
            veh->ClearGettingOutFlags(m_nDoorFlagsToClear);
        }
        if (m_nNumGettingInToClear) {
            veh->m_nNumGettingIn -= m_nNumGettingInToClear;
        }
        if (veh->m_pDriver == ped) {
            veh->RemoveDriver(!m_bSwitchOffEngine);
            veh->SetStatus(STATUS_PHYSICS);
            if (veh->m_nDoorLock == CARLOCK_COP_CAR) {
                veh->m_nDoorLock = CARLOCK_UNLOCKED;
            }
            if (ped->m_nPedType == PED_TYPE_COP && veh->IsLawEnforcementVehicle()) {
                veh->ChangeLawEnforcerState(false);
            }
        } else {
            veh->RemovePassenger(ped);
        }

        if (CGarages::IsPointWithinAnyGarage(ped->GetPosition())) {
            veh->m_nOverrideLights = NO_CAR_LIGHT_OVERRIDE;
            veh->vehicleFlags.bLightsOn = false;
        }
    }

    if (!m_bFallingOutOfCar
        && !m_bKnockedOffBike
        && ped->m_pVehicle
        && ped->m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE
        && std::abs(ped->m_vecMoveSpeed.x) < 0.1f
        && std::abs(ped->m_vecMoveSpeed.y) < 0.1f
    ) {
        static_cast<CBike*>(ped->m_pVehicle)->bikeFlags.bOnSideStand = true;
    }

    if (ped->IsPlayer()) {
        ped->GetTaskManager().SetTask(new CTaskSimplePlayerOnFoot(), TASK_PRIMARY_DEFAULT);
    } else if (ped->IsCreatedByMission()) {
        ped->GetTaskManager().SetTask(new CTaskSimpleStandStill(999'999, true, false, 8.f), TASK_PRIMARY_DEFAULT);
    } else {
        ped->GetTaskManager().SetTask(CTaskComplexWander::GetWanderTaskByPedType(ped), TASK_PRIMARY_DEFAULT);
    }

    ped->ReplaceWeaponWhenExitingVehicle();
    ped->bHasJustLeftCar = true;
    ped->m_nPedState = PEDSTATE_IDLE;

    if (m_pTargetVehicle && m_pTargetVehicle->physicalFlags.bTouchingWater) {
        ped->physicalFlags.bTouchingWater = true;
    }

    return true;
}

// 0x649F50
CTask* CTaskSimpleCarSetPedOut::Clone() const {
    auto* task = new CTaskSimpleCarSetPedOut(m_pTargetVehicle, m_nTargetDoor, m_bSwitchOffEngine);
    task->m_bWarpingOutOfCar     = m_bWarpingOutOfCar;
    task->m_bFallingOutOfCar     = m_bFallingOutOfCar;
    task->m_bKnockedOffBike      = m_bKnockedOffBike;
    task->m_nDoorFlagsToClear    = m_nDoorFlagsToClear;
    task->m_nNumGettingInToClear = m_nNumGettingInToClear;
    return task;
}
