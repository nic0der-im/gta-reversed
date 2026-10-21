#include "StdInc.h"

#include "extensions/utility.hpp"

#include "TaskComplexArrestPed.h"
#include "TaskComplexFallAndGetUp.h"
#include "TaskSimpleWaitUntilPedIsOutCar.h"
#include "TaskSimpleArrestPed.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskComplexDestroyCar.h"
#include "SeekEntity/TaskComplexSeekEntityStandard.h"
#include "TaskComplexDragPedFromCar.h"
#include "TaskComplexOpenDriverDoor.h"
#include "TaskComplexOpenPassengerDoor.h"

#include "CarEnterExit.h"
#include "eTargetDoor.h"

void CTaskComplexArrestPed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexArrestPed, 0x8709A8, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x68B990);
    RH_ScopedInstall(Destructor, 0x68BA00);
    RH_ScopedVMTInstall(MakeAbortable, 0x68BA60);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x690220);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x6907A0);
    RH_ScopedVMTInstall(ControlSubTask, 0x68D350);
    RH_ScopedInstall(CreateSubTask, 0x68CF80);
}

// 0x68B990
CTaskComplexArrestPed::CTaskComplexArrestPed(CPed* ped) : CTaskComplex() {
    m_PedToArrest = ped;
    m_Vehicle = nullptr;
    CEntity::SafeRegisterRef(m_PedToArrest);
}

// 0x68BA00
CTaskComplexArrestPed::~CTaskComplexArrestPed() {
    CEntity::SafeCleanUpRef(m_PedToArrest);
}

// 0x68BA60


bool CTaskComplexArrestPed::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x690220
CTask* CTaskComplexArrestPed::CreateNextSubTask(CPed* ped) {
    if (!m_PedToArrest) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    // 0x690246
    // Ped is being arrested by someone else - Kill them (or arrest them if we're close enough)
    if (m_PedToArrest->bIsBeingArrested) {
        switch (m_pSubTask->GetTaskType()) {
        case TASK_SIMPLE_ARREST_PED:
            break;
        case TASK_COMPLEX_SEEK_ENTITY:
            if (!static_cast<CTaskComplexSeekEntityStandard*>(m_pSubTask)->HasAchievedSeekEntity()) {
                return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
            }
            return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
        default:
            return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
        }
    }

    // 0x6902CE
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_SEEK_ENTITY: {
        // If the ped is falling over - And we're close enough - Arrest them
        if (const auto fallTask = static_cast<CTaskComplexFallAndGetUp*>(m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_FALL_AND_GET_UP))) {
            if (fallTask->IsFalling() && static_cast<CTaskComplexSeekEntityStandard*>(m_pSubTask)->HasAchievedSeekEntity()) {
                auto dir = m_PedToArrest->GetPosition() - ped->GetPosition();
                if (std::abs(dir.z) <= 2.f) {
                    dir.z = 0.f;
                    if (dir.SquaredMagnitude() <= sq(3.f)) {
                        fallTask->SetDownTime(100'000);
                        return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
                    }
                }
            }
        }
        return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    }
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
        // If the ped was dragged out already (and is falling over) - And we're close enough - Arrest them
        if (const auto fallTask = static_cast<CTaskComplexFallAndGetUp*>(m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_FALL_AND_GET_UP))) {
            if (fallTask->IsFalling() && !static_cast<CTaskComplexEnterCar*>(m_pSubTask)->IsQuitAfterDraggingPedOut()) {
                auto dir = m_PedToArrest->GetPosition() - ped->GetPosition();
                if (std::abs(dir.z) <= 2.f) {
                    dir.z = 0.f;
                    if (dir.SquaredMagnitude() <= sq(3.f)) {
                        fallTask->SetDownTime(100'000);
                        return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
                    }
                }
            }
        }
        return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    }
    case TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR: // 0x6902FE
    case TASK_COMPLEX_CAR_OPEN_PASSENGER_DOOR: { // 0x690392
        const auto enterCarTask = static_cast<CTaskComplexEnterCar*>(m_pSubTask);

        // Remember the vehicle we couldn't unlock the doors of
        if (   enterCarTask->IsQuitAfterOpeningDoor()
            && m_PedToArrest->m_pVehicle
            && !m_PedToArrest->m_pVehicle->CanPedOpenLocks(ped)
        ) {
            m_Vehicle = m_PedToArrest->m_pVehicle;
        }

        if (!m_PedToArrest->IsAlive()) {
            return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
        }

        // The door was already opened, but the ped is still inside
        if (m_PedToArrest->bInVehicle && !enterCarTask->IsQuitAfterOpeningDoor()) {
            if (m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)) {
                return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
            }
            return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
        }
        break;
    }
    case TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR:
        return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    case TASK_COMPLEX_KILL_PED_ON_FOOT: { // 0x690494
        if (m_PedToArrest->m_fHealth <= 0.f) {
            return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
        }

        // Ped is falling over
        if (const auto fallTask = static_cast<CTaskComplexFallAndGetUp*>(m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_FALL_AND_GET_UP))) {
            if (fallTask->IsFalling()) {
                auto dir = m_PedToArrest->GetPosition() - ped->GetPosition();
                if (std::abs(dir.z) > 2.f) {
                    return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
                }
                dir.z = 0.f;
                if (dir.SquaredMagnitude() > sq(3.f)) {
                    return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
                }
                fallTask->SetDownTime(100'000);
                return CreateSubTask(TASK_SIMPLE_ARREST_PED, ped);
            }
        }

        // If the ped is a player that is being pursued by the cops - Stop chasing them (Another cop will take over)
        if (ped->m_nPedType != PED_TYPE_COP && m_PedToArrest->IsPlayer()) {
            if (const auto playerData = m_PedToArrest->GetPlayerData()) {
                if (playerData->m_pWanted->m_NumCopsInPursuit != 0) {
                    return CreateSubTask(TASK_FINISHED, ped);
                }
            }
        }
        return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    }
    case TASK_COMPLEX_DESTROY_CAR: // 0x69046F
    case TASK_SIMPLE_ARREST_PED:   // 0x6904C1
        return CreateSubTask(TASK_FINISHED, ped);
    default:
        return nullptr;
    }

    // 0x6906E3
    // Couldn't get the ped out/close enough - Just kill them
    return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
}

// NOTSA
void MakeSurePedHasWeaponInHand(CPed* ped) {
    // Make sure ped has an actual weapon in their hand
    if (!ped->GetActiveWeapon().IsTypeMelee())
        return;

    if (ped->DoWeHaveWeaponAvailable(WEAPON_SHOTGUN)) { // Use shotgun (if available)
        ped->SetCurrentWeapon(WEAPON_SHOTGUN);
        return;
    }

    // Otherwise a pistol
    if (!ped->DoWeHaveWeaponAvailable(WEAPON_PISTOL)) { // Make sure they have one
        ped->GiveWeapon(WEAPON_PISTOL, 10, false);
    }
    ped->SetCurrentWeapon(WEAPON_PISTOL);
}

// 0x6907A0


CTask* CTaskComplexArrestPed::CreateFirstSubTask(CPed* ped) {
    if (!m_PedToArrest) {
        return nullptr;
    }

    m_bSubTaskNeedsToBeCreated = false;

    if (!m_PedToArrest->bInVehicle) {
        return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT, ped);
    }

    if (m_PedToArrest->m_pVehicle->IsBike() || m_PedToArrest->m_pVehicle->IsSubQuad()) { // Just drag ped from a bike/quad
        return CreateSubTask(TASK_COMPLEX_DRAG_PED_FROM_CAR, ped);
    }

    if (m_PedToArrest->m_pVehicle->IsSubBoat()) { // If they're in a boat, just destroy it
        MakeSurePedHasWeaponInHand(ped);
        return CreateSubTask(TASK_COMPLEX_DESTROY_CAR, ped);
    } else {
        if (m_PedToArrest->m_pVehicle->IsUpsideDown() || m_PedToArrest->m_pVehicle->IsOnItsSide()) {
            return CreateSubTask(TASK_COMPLEX_DESTROY_CAR, ped);
        }
        return CreateSubTask(TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR, ped);
    }
}

// 0x68D350
CTask* CTaskComplexArrestPed::ControlSubTask(CPed* ped) {
    // Automatically make ped say something on function return
    const notsa::ScopeGuard Have_A_Nice_Day_Sir{
        [this, ped] {
            if (m_PedToArrest && m_PedToArrest->IsPlayer()) {
                if (FindPlayerWanted()->m_NumCopsInPursuit == 1) {
                    ped->Say(CTX_GLOBAL_SOLO);
                }
            }
        }
    };

    // Tries to abort current sub-task and replace it with `taskType`.
    const auto TryReplaceSubTask = [this, ped](auto taskType) {
        // Inverted `if` and got rid of `taskType == TASK_NONE` (in which case `m_pSubTask` was returned always)
        if (m_pSubTask->MakeAbortable(ped)) {
            return CreateSubTask(taskType, ped);
        } else {
            return m_pSubTask;
        }
    };

    // 0x68D7CD
    // Make sure the ped has a ranged weapon (So they can take out the tires or whatever)
    const auto DoDestroyCarTask = [&]() -> CTask* {
        if (ped->GetActiveWeapon().IsTypeMelee()) {
            ped->SetCurrentWeapon(ped->DoWeHaveWeaponAvailable(WEAPON_SHOTGUN) ? WEAPON_SHOTGUN : WEAPON_PISTOL);
        }
        return TryReplaceSubTask(TASK_COMPLEX_DESTROY_CAR);
    };

    // 0x68D370, 0x68D37F
    if (!m_PedToArrest || m_PedToArrest->m_fHealth <= 0.f) {
        return TryReplaceSubTask(TASK_FINISHED);
    }

    // 0x68D39F
    if (m_bSubTaskNeedsToBeCreated) {
        return m_pSubTask->MakeAbortable(ped)
            ? CreateFirstSubTask(ped)
            : m_pSubTask;
    }

    // 0x68D3CB
    if (m_PedToArrest->bIsBeingArrested) {
        switch (m_pSubTask->GetTaskType()) {
        case TASK_SIMPLE_ARREST_PED:
        case TASK_COMPLEX_SEEK_ENTITY:
            break;
        default:
            return TryReplaceSubTask(TASK_COMPLEX_SEEK_ENTITY);
        }
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: // 0x68D49D
    case TASK_COMPLEX_DESTROY_CAR: { // 0x68D5F6
        if (!m_PedToArrest->bInVehicle) { // If not in vehicle anymore, try to kill them
            return TryReplaceSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT);
        }
        break;
    }
    case TASK_COMPLEX_KILL_PED_ON_FOOT: { // 0x68D626
        // See if ped is falling, and is close enough
        if (const auto task = static_cast<CTaskComplexFallAndGetUp*>(m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_FALL_AND_GET_UP))) {
            if (task->IsFalling()) {
                auto dir = m_PedToArrest->GetPosition() - ped->GetPosition();
                if (std::abs(dir.z) <= 2.f) {
                    dir.z = 0.f; // Check 2D distance only
                    if (dir.SquaredMagnitude() <= sq(3.f)) {
                        task->SetDownTime(100'000);
                        return TryReplaceSubTask(TASK_SIMPLE_ARREST_PED);
                    }
                }
                return TryReplaceSubTask(TASK_COMPLEX_SEEK_ENTITY); // Too far away
            }
        }

        if (!m_PedToArrest->bInVehicle || !m_PedToArrest->m_pVehicle) {
            break;
        }

        const auto vehicle = m_PedToArrest->m_pVehicle;

        // Ped has gotten into a vehicle we can't get them out of, destroy it
        if (   vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT
            || vehicle->m_nVehicleSubType == VEHICLE_TYPE_PLANE
            || vehicle->m_nVehicleSubType == VEHICLE_TYPE_HELI
        ) {
            return DoDestroyCarTask();
        }

        if (!ped->GetActiveWeapon().IsTypeMelee() && !FindPlayerWanted()->IsClosestCop(ped, 2)) {
            return TryReplaceSubTask(TASK_COMPLEX_DESTROY_CAR);
        }

        if (vehicle == m_Vehicle || m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)) {
            break;
        }

        if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
            return TryReplaceSubTask(TASK_COMPLEX_DRAG_PED_FROM_CAR);
        }

        if (vehicle->IsUpsideDown() || vehicle->IsOnItsSide()) {
            return TryReplaceSubTask(TASK_COMPLEX_DESTROY_CAR);
        }

        return TryReplaceSubTask(TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR);
    }
    case TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR: { // 0x68D424
        const auto vehicle = m_PedToArrest->m_pVehicle;

        // Maybe wait until ped gets out of the car..
        if (   m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)
            && m_PedToArrest->bInVehicle
            && CVector::DistSqr(m_PedToArrest->GetPosition(), ped->GetPosition()) < 25.f
        ) {
            return TryReplaceSubTask(TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR);
        }

        // Ped not in vehicle any more, just kill 'em
        if (!m_PedToArrest->bInVehicle) {
            return TryReplaceSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT);
        }

        // Ped can't open driver door, but we can open front right door?
        if (   !CCarEnterExit::IsRoomForPedToLeaveCar(vehicle, TARGET_DOOR_DRIVER, nullptr)
            && CCarEnterExit::IsRoomForPedToLeaveCar(vehicle, TARGET_DOOR_FRONT_RIGHT, nullptr)
        ) {
            return TryReplaceSubTask(TASK_COMPLEX_CAR_OPEN_PASSENGER_DOOR);
        }

        break; // No free door, keep at it
    }
    case TASK_COMPLEX_CAR_OPEN_PASSENGER_DOOR: { // 0x68D510
        // Pretty much the copy of the above, with minor changes (See change 1,2)

        const auto vehicle = m_PedToArrest->m_pVehicle;

        // Maybe wait until ped gets out of the car..
        if (   m_PedToArrest->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)
            && m_PedToArrest->bInVehicle
            && CVector::DistSqr(m_PedToArrest->GetPosition(), ped->GetPosition()) < 25.f
        ) {
            return TryReplaceSubTask(TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR);
        }

        // Ped not in vehicle any more, just kill 'em
        if (!m_PedToArrest->bInVehicle) {
            return TryReplaceSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT);
        }

        // Ped can't open passenger door, but driver door can be opened by us? - Change 1
        if (   !CCarEnterExit::IsRoomForPedToLeaveCar(vehicle, TARGET_DOOR_FRONT_RIGHT, nullptr)
            && CCarEnterExit::IsRoomForPedToLeaveCar(vehicle, TARGET_DOOR_DRIVER, nullptr)
        ) {
            return TryReplaceSubTask(TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR);
        }

        // Change 2
        return DoDestroyCarTask();
    }
    }

    // Just continue with current sub-task
    return m_pSubTask;
}

// 0x68CF80
CTask* CTaskComplexArrestPed::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_ARREST_PED:
        if (m_PedToArrest->m_pVehicle) {
            if (m_PedToArrest->m_pVehicle->IsDriver(m_PedToArrest)) {
                m_PedToArrest->m_pVehicle->vehicleFlags.bIsHandbrakeOn = true;
                m_PedToArrest->m_pVehicle->SetStatus(STATUS_FORCED_STOP);
            }
        }
        return new CTaskSimpleArrestPed(m_PedToArrest);

    case TASK_COMPLEX_KILL_PED_ON_FOOT:
        return new CTaskComplexKillPedOnFoot(m_PedToArrest, -1, 0, 0, 0, 1);

    case TASK_COMPLEX_DESTROY_CAR:
        return new CTaskComplexDestroyCar(m_PedToArrest->m_pVehicle, 0, 0, 0);

    case TASK_COMPLEX_SEEK_ENTITY: {
        const auto radius = m_PedToArrest->bIsBeingArrested ? 4.0f : 3.0f;
        return new CTaskComplexSeekEntityStandard(m_PedToArrest, 50'000, 1000, radius, 2.0f, 2.0f, true, true);
    }
    case TASK_COMPLEX_DRAG_PED_FROM_CAR:
        return new CTaskComplexDragPedFromCar(m_PedToArrest, 100'000);

    case TASK_COMPLEX_CAR_OPEN_DRIVER_DOOR:
        return new CTaskComplexOpenDriverDoor(m_PedToArrest->m_pVehicle);

    case TASK_COMPLEX_CAR_OPEN_PASSENGER_DOOR:
        return new CTaskComplexOpenPassengerDoor(m_PedToArrest->m_pVehicle, 8); // todo: magic number

    case TASK_SIMPLE_WAIT_UNTIL_PED_OUT_CAR: {
        return new CTaskSimpleWaitUntilPedIsOutCar{m_PedToArrest, m_PedToArrest->GetPosition() - ped->GetPosition()};
    }
    default:
        return nullptr;
    }
}
