#include "StdInc.h"

#include "TaskComplexKillPedOnFoot.h"
#include "TaskSimpleLeaveGroup.h"
#include "TaskSimplePause.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleCarDriveTimed.h"
#include "TaskComplexDragPedFromCar.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexKillPedOnFootMelee.h"
#include "TaskComplexKillPedOnFootArmed.h"
#include "TaskComplexDestroyCar.h"
#include "TaskComplexSignalAtPed.h"

#include "CarEnterExit.h"
#include "CopPed.h"
#include "CullZones.h"
#include "EventAreaCodes.h"
#include "Wanted.h"
#include "PlayerPedData.h"

void CTaskComplexKillPedOnFoot::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexKillPedOnFoot, 0x86D894, 11);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x620E30);
    RH_ScopedInstall(CreateSubTask, 0x625E70);

    RH_ScopedVMTInstall(MakeAbortable, 0x625E40);
    RH_ScopedVMTInstall(ControlSubTask, 0x626260);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x62B150);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x62B490);
}

CTaskComplexKillPedOnFoot::CTaskComplexKillPedOnFoot(
    CPed* target,
    int32 time,
    int32 pedFlags,
    int32 delay,
    int32 chance,
    uint8 nCompetence,
    bool bWaitForPlayerToBeSafe,
    bool bWaitingForPlayerToBeSafe
) :
    m_bWaitForPlayerToBeSafe{ bWaitForPlayerToBeSafe },
    m_bWaitingForPlayerToBeSafe{ bWaitingForPlayerToBeSafe },
    m_target{ target },
    m_pedFlags{ pedFlags },
    m_actionDelay{ delay },
    m_actionChance{ chance },
    m_nCompetence{ nCompetence },
    m_time{ time },
    m_startTime{ CTimer::GetTimeInMS() }
{
    CEntity::SafeRegisterRef(m_target);
}

CTaskComplexKillPedOnFoot::~CTaskComplexKillPedOnFoot() {
    CEntity::SafeCleanUpRef(m_target);
}

CTaskComplexKillPedOnFoot* CTaskComplexKillPedOnFoot::Constructor(CPed* target, int32 time, int32 pedFlags, int32 delay, int32 chance, int8 a7) {
    this->CTaskComplexKillPedOnFoot::CTaskComplexKillPedOnFoot(target, time, pedFlags, delay, chance, a7);
    return this;
}

/*!
* @brief Members of a gang (or the player) are taunted by gang members hunting them
* @addr 0x65E9A0
*/
static void SayGangAttackTaunt(CPed* ped, CPed* target) {
    if (!ped || !target) {
        return;
    }
    if (!IsPedTypeGang(ped->m_nPedType)) {
        return;
    }
    if (!IsPedTypeGang(target->m_nPedType) && target != FindPlayerPed()) {
        return;
    }
    switch (target->m_nPedType) {
    case PED_TYPE_GANG1:
        ped->Say(CTX_GLOBAL_ATTACK_GANG_BALLAS);
        break;
    case PED_TYPE_GANG3:
        ped->Say(CTX_GLOBAL_ATTACK_GANG_LSV);
        break;
    case PED_TYPE_GANG7:
        ped->Say(CTX_GLOBAL_ATTACK_GANG_VLA);
        break;
    }
}

// 0x625E40
bool CTaskComplexKillPedOnFoot::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    ped->bDontAcceptIKLookAts = false;

    if (!m_pSubTask) {
        return true;
    }
    return m_pSubTask->MakeAbortable(ped, priority, event);
}

// 0x626260
CTask* CTaskComplexKillPedOnFoot::ControlSubTask(CPed* ped) {
    ped->bDontAcceptIKLookAts = true;

    eTaskType newType = TASK_NONE;
    CTask* result = m_pSubTask;

    // The sub-task to switch to after checking the current one (Only modifies `newType` on a match)
    const auto DecideNewSubTask = [&]() {
        eTaskType decidedType = TASK_NONE;
        switch (m_pSubTask->GetTaskType()) {
        case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED: { // 0x3EA
            static_cast<CTaskComplexKillPedOnFootArmed*>(m_pSubTask)->m_bShotFiredByPlayer = m_bShotFiredByPlayerFlag;

            if (!ped->GetActiveWeapon().IsTypeMelee() || ped->IsPlayer()) {
                // Unarmed? Switch to the first available weapon, or to melee if there's none
                if (ped->GetActiveWeapon().m_TotalAmmo == 0 && !ped->IsPlayer()) {
                    int32 slot = 0;
                    for (; slot < 13; ++slot) {
                        if (ped->m_aWeapons[slot].m_TotalAmmo > 0) {
                            ped->SetCurrentWeapon(slot);
                            break;
                        }
                    }
                    if (slot == 13) {
                        ped->SetCurrentWeapon(0);
                        decidedType = TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE;
                    }
                }
            } else {
                decidedType = TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE;
            }

            const auto veh = m_target->m_pVehicle;
            if (m_target->bInVehicle && veh && (veh->IsDriver(m_target) || veh->IsPassenger(m_target))) {
                if (veh->m_nVehicleSubType == VEHICLE_TYPE_HELI || veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE) {
                    decidedType = TASK_COMPLEX_DESTROY_CAR;
                } else if (veh->m_nVehicleType != VEHICLE_TYPE_BIKE
                    && veh->m_nVehicleType != VEHICLE_TYPE_BOAT
                    && !ped->bStayInSamePlace
                    && veh->GetMoveSpeed().Magnitude2D() != 0.1f // NaN-safe `!=`
                    && veh->CanPedOpenLocks(ped)
                ) {
                    m_bRoomToDragPedOutOfCar = CCarEnterExit::IsRoomForPedToLeaveCar(
                        veh,
                        CCarEnterExit::ComputeTargetDoorToExit(veh, m_target)
                    );
                    decidedType = m_bRoomToDragPedOutOfCar
                        ? TASK_COMPLEX_DRAG_PED_FROM_CAR
                        : TASK_COMPLEX_DESTROY_CAR;
                }
            }
            break;
        }
        case TASK_COMPLEX_DRAG_PED_FROM_CAR: { // 0x2BF
            const auto veh = m_target->m_pVehicle;
            if (veh && m_target->bInVehicle && (veh->IsDriver(m_target) || veh->IsPassenger(m_target))) {
                if (veh->GetMoveSpeed().Magnitude2D() > 0.1f) {
                    decidedType = TASK_COMPLEX_DESTROY_CAR;
                } else if (m_timer.IsOutOfTime() && (veh->IsPassenger(m_target) || veh->IsDriver(m_target))) {
                    m_bRoomToDragPedOutOfCar = CCarEnterExit::IsRoomForPedToLeaveCar(
                        veh,
                        CCarEnterExit::ComputeTargetDoorToExit(veh, m_target)
                    );
                    if (!m_bRoomToDragPedOutOfCar) {
                        decidedType = TASK_COMPLEX_DESTROY_CAR;
                    }
                    m_timer.Start(2'000);
                }
            }
            break;
        }
        case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE: { // 0x3E9
            static_cast<CTaskComplexKillPedOnFootMelee*>(m_pSubTask)->m_bShotFiredByPlayerFlag = m_bShotFiredByPlayerFlag;

            if (!ped->GetActiveWeapon().IsTypeMelee()) {
                decidedType = TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
            } else if (const auto veh = m_target->m_pVehicle; m_target->bInVehicle && veh) {
                if (veh->m_nVehicleSubType == VEHICLE_TYPE_HELI || veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE) {
                    decidedType = TASK_COMPLEX_DESTROY_CAR;
                } else if (veh->m_nVehicleType != VEHICLE_TYPE_BOAT
                    && !ped->bStayInSamePlace
                    && veh->CanPedOpenLocks(ped)
                ) {
                    m_bRoomToDragPedOutOfCar = CCarEnterExit::IsRoomForPedToLeaveCar(
                        veh,
                        CCarEnterExit::ComputeTargetDoorToExit(veh, m_target)
                    );
                    if (m_bRoomToDragPedOutOfCar) {
                        decidedType = TASK_COMPLEX_DRAG_PED_FROM_CAR;
                    }
                }
            }
            break;
        }
        case TASK_COMPLEX_DESTROY_CAR: { // 0x3EB
            if (!m_target->bInVehicle) {
                decidedType = ped->GetActiveWeapon().IsTypeMelee()
                    ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE
                    : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
                break;
            }

            const auto veh = m_target->m_pVehicle;
            if (!veh || CCarEnterExit::IsVehicleHealthy(veh)) {
                if (!m_target->bDontDragMeOutCar
                    && veh->m_nVehicleSubType != VEHICLE_TYPE_PLANE
                    && veh->m_nVehicleSubType != VEHICLE_TYPE_HELI
                    && veh->m_nVehicleType != VEHICLE_TYPE_BIKE
                    && veh->m_nVehicleType != VEHICLE_TYPE_BOAT
                    && !ped->bStayInSamePlace
                    && veh->GetMoveSpeed().Magnitude2D() != 0.1f // NaN-safe `!=`
                    && veh->CanPedOpenLocks(ped)
                ) {
                    if (m_bRoomToDragPedOutOfCar) {
                        decidedType = TASK_COMPLEX_DRAG_PED_FROM_CAR;
                    } else if (m_timer.IsOutOfTime()) {
                        m_bRoomToDragPedOutOfCar = CCarEnterExit::IsRoomForPedToLeaveCar(
                            veh,
                            CCarEnterExit::ComputeTargetDoorToExit(veh, m_target)
                        );
                        if (m_bRoomToDragPedOutOfCar) {
                            decidedType = TASK_COMPLEX_DRAG_PED_FROM_CAR;
                        }
                        m_timer.Start(2'000);
                    }
                }
            } else {
                decidedType = TASK_COMPLEX_SIGNAL_AT_PED;
            }
            break;
        }
        default:
            break;
        }
        return decidedType;
    };

    if (!m_target) {
        newType = TASK_FINISHED;
    } else if (m_pSubTask->GetTaskType() == TASK_SIMPLE_PAUSE) {
        if (m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe) {
            if (!(FindPlayerWanted()->m_Flags & 4)) { // `m_bEverybodyBackOff`
                m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
            }
        }
        return result;
    } else if (m_time < 1 || CTimer::GetTimeInMS() <= (uint32)(m_startTime + m_time)) {
        if (m_bNewTarget) {
            if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
                return CreateFirstSubTask(ped);
            }
        } else {
            if (!m_bTargetKilled) {
                if (m_target->m_fHealth <= 0.0f) {
                    if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
                        m_bTargetKilled = true;
                        if (m_pSubTask->GetTaskType() != TASK_COMPLEX_SIGNAL_AT_PED) {
                            if (ped->IsCop() && !ped->IsPlayer()) {
                                ped->Say(CTX_GLOBAL_ARREST_CRIM);
                            }
                            if (!ped->bKeepTasksAfterCleanUp) {
                                return new CTaskSimpleLeaveGroup{};
                            }
                            return CreateSubTask(TASK_COMPLEX_SIGNAL_AT_PED, ped);
                        }
                    }
                    newType = DecideNewSubTask();
                } else if (m_target->IsPlayer()) {
                    const auto targetWanted      = m_target->GetPlayerData() ? m_target->GetPlayerData()->m_pWanted : nullptr;
                    const bool everybodyBackOff  = targetWanted && (targetWanted->m_Flags & 4);

                    if ((!everybodyBackOff
                            && (ped->m_nPedType != PED_TYPE_COP
                                || !ped->AsCop()->m_bDontPursuit
                                || FindPlayerWanted()->m_ChanceOnRoadBlock != 0
                            )
                        )
                        || m_pSubTask->GetTaskType() == TASK_SIMPLE_PAUSE
                    ) {
                        newType = DecideNewSubTask();
                    } else {
                        newType = TASK_SIMPLE_PAUSE;
                        if (m_bWaitForPlayerToBeSafe && everybodyBackOff) {
                            m_bWaitingForPlayerToBeSafe = true;
                        }
                    }
                } else {
                    newType = DecideNewSubTask();
                }
            } else {
                newType = DecideNewSubTask();
            }
            m_bShotFiredByPlayerFlag = false;
        }
    } else {
        // The delay to act has expired
        if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return new CTaskSimpleStandStill{ 0, false, false, 8.0f };
        }
    }

    if (m_target && m_target->m_pContactEntity && ped->m_pContactEntity
        && m_target->m_pContactEntity->GetAreaCode() != ped->m_pContactEntity->GetAreaCode()
    ) {
        CEventAreaCodes event{ m_target };
        ped->GetEventGroup().Add(&event, false);
    }

    if (newType != TASK_NONE && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        result = CreateSubTask(newType, ped);
    }

    if (m_target) {
        SayGangAttackTaunt(ped, m_target);
    }

    return result;
}

// 0x62B150
CTask* CTaskComplexKillPedOnFoot::CreateNextSubTask(CPed* ped) {
    if (!m_target) {
        return CreateSubTask(TASK_FINISHED, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_STAND_STILL: // 0xCB
    case TASK_NONE:               // 200
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_SIMPLE_PAUSE: { // 0xCA
        if (ped->IsCop() && ped->AsCop()->m_bDontPursuit) {
            if (FindPlayerWanted()->m_ChanceOnRoadBlock > 0) {
                return CreateSubTask(TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED, ped);
            }
            return CreateSubTask(TASK_SIMPLE_PAUSE, ped);
        }
        if (m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe) {
            if (!(FindPlayerWanted()->m_Flags & 4)) { // `m_bEverybodyBackOff`
                m_bWaitingForPlayerToBeSafe = false;
                return CreateFirstSubTask(ped);
            }
        }
        return CreateSubTask(TASK_FINISHED, ped);
    }
    case TASK_COMPLEX_LEAVE_CAR: { // 0x2C0
        if (ped->bInVehicle) {
            return CreateSubTask(TASK_SIMPLE_CAR_DRIVE_TIMED, ped);
        }

        // If the target is a player (that we've just made leave a car), give this ped a pistol
        const bool targetIsPlayer = m_target->IsPlayer();
        if (targetIsPlayer) {
            const auto dist = (m_target->GetPosition() - ped->GetPosition()).Magnitude();
            if ((CCullZones::NoPolice()
                    || (m_target->m_standingOnEntity
                        && m_target->m_standingOnEntity != ped->m_standingOnEntity
                        && dist < 5.0f
                    )
                )
                && (m_target->GetPlayerData()->m_nPlayerFlags & 1) // `m_bStoppedMoving`
                && (IsPedTypeGang(ped->m_nPedType) || ped->m_nPedType == PED_TYPE_COP)
                && ped->GetCreatedBy() != PED_MISSION
            ) {
                ped->GiveWeapon(WEAPON_PISTOL, 1'000, true);
                ped->SetCurrentWeapon(WEAPON_PISTOL);
                ped->SetMoveState(PEDMOVE_STILL);
            }
        }

        return CreateSubTask(ped->GetActiveWeapon().IsTypeMelee()
            ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE
            : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED, ped);
    }
    case TASK_SIMPLE_CAR_DRIVE_TIMED: // 0x2CF
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: // 0x2BF
        if (static_cast<CTaskComplexDragPedFromCar*>(m_pSubTask)->IsQuitAfterDraggingPedOut()) {
            return CreateSubTask(TASK_COMPLEX_DESTROY_CAR, ped);
        }
        return CreateSubTask(ped->GetActiveWeapon().IsTypeMelee()
            ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE
            : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED, ped);
    case TASK_COMPLEX_DESTROY_CAR: // 0x3EB
        return CreateSubTask(TASK_FINISHED, ped);
    case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE: // 0x3E9
    case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED: // 0x3EA
        return CreateSubTask(TASK_FINISHED, ped);
    default:
        return nullptr;
    }
}

// 0x62B490
CTask* CTaskComplexKillPedOnFoot::CreateFirstSubTask(CPed* ped) {
    m_bNewTarget = false;
    m_bRoomToDragPedOutOfCar = true;

    if (!m_target) {
        ped->bDontAcceptIKLookAts = false;
        return nullptr;
    }

    if (m_target->m_fHealth <= 0.0f) {
        m_bTargetKilled = true;
    }

    if (ped->m_pVehicle && ped->bInVehicle) {
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
    }

    // Give the ped a pistol when hunting a player that is nearby (and not in a vehicle)
    if (m_target->IsPlayer()) {
        const auto dist = (m_target->GetPosition() - ped->GetPosition()).Magnitude();
        if ((CCullZones::NoPolice()
                || (m_target->m_standingOnEntity
                    && m_target->m_standingOnEntity != ped->m_standingOnEntity
                    && dist < 5.0f
                )
            )
            && (m_target->GetPlayerData()->m_nPlayerFlags & 1) // `m_bStoppedMoving`
            && (IsPedTypeGang(ped->m_nPedType) || ped->m_nPedType == PED_TYPE_COP)
            && ped->GetCreatedBy() != PED_MISSION
        ) {
            ped->GiveWeapon(WEAPON_PISTOL, 1'000, true);
            ped->SetCurrentWeapon(WEAPON_PISTOL);
            ped->SetMoveState(PEDMOVE_STILL);
        }
    }

    eTaskType newType;
    if (const auto veh = m_target->m_pVehicle; m_target->bInVehicle && veh
        && (veh->IsDriver(m_target) || veh->IsPassenger(m_target))
    ) {
        m_bRoomToDragPedOutOfCar = true;

        if (veh->m_nVehicleSubType == VEHICLE_TYPE_HELI || veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE) {
            newType = TASK_COMPLEX_DESTROY_CAR;
        } else if (veh->m_nVehicleSubType != VEHICLE_TYPE_PLANE
            && veh->m_nVehicleSubType != VEHICLE_TYPE_HELI
            && !ped->bStayInSamePlace
            && veh->GetMoveSpeed().Magnitude2D() <= 0.1f
            && veh->CanPedOpenLocks(ped)
            && (veh->m_nVehicleType != VEHICLE_TYPE_BIKE || ped->GetActiveWeapon().IsTypeMelee())
        ) {
            if (veh->m_nVehicleType != VEHICLE_TYPE_BOAT) {
                m_bRoomToDragPedOutOfCar = CCarEnterExit::IsRoomForPedToLeaveCar(
                    veh,
                    CCarEnterExit::ComputeTargetDoorToExit(veh, m_target)
                );
                if (m_bRoomToDragPedOutOfCar) {
                    newType = m_target->bDontDragMeOutCar
                        ? TASK_COMPLEX_DESTROY_CAR
                        : TASK_COMPLEX_DRAG_PED_FROM_CAR;
                    const auto task = CreateSubTask(newType, ped);
                    ped->DropEntityThatThisPedIsHolding(true);
                    return task;
                }
            }
            newType = TASK_COMPLEX_DESTROY_CAR;
        } else {
            newType = TASK_COMPLEX_DESTROY_CAR;
        }
    } else {
        newType = ped->GetActiveWeapon().IsTypeMelee()
            ? TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE
            : TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED;
    }

    const auto task = CreateSubTask(newType, ped);
    ped->DropEntityThatThisPedIsHolding(true);
    return task;
}

// 0x625E70
CTask* CTaskComplexKillPedOnFoot::CreateSubTask(int32 taskId, CPed* ped) {
    switch (taskId) {
    case TASK_NONE:
        return new CTaskSimpleLeaveGroup{};
    case TASK_SIMPLE_PAUSE: {
        CTaskSimpleStandStill{}.ProcessPed(ped);
        return m_bWaitForPlayerToBeSafe && m_bWaitingForPlayerToBeSafe
            ? new CTaskSimplePause{10'000}
            : new CTaskSimplePause{2'000};
    }
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{0, false, false, 8.f};
    case TASK_SIMPLE_CAR_DRIVE_TIMED:
        return new CTaskSimpleCarDriveTimed{ped->m_pVehicle, 2'000};
    case TASK_COMPLEX_DRAG_PED_FROM_CAR: {
        auto* task = new CTaskComplexDragPedFromCar{m_target, 0};
        m_timer.Start(2'000);
        return task;
    }
    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar{ped->m_pVehicle, 0, 0, true, true};
    case TASK_COMPLEX_KILL_PED_ON_FOOT_MELEE: {
        auto* task = new CTaskComplexKillPedOnFootMelee{m_target};
        m_timer.Start(2'000);
        return task;
    }
    case TASK_COMPLEX_KILL_PED_ON_FOOT_ARMED: {
        auto* task = new CTaskComplexKillPedOnFootArmed{m_target, (uint32)m_pedFlags, (uint32)m_actionDelay, (uint32)m_actionChance, (int8)m_nCompetence};
#ifdef FIX_BUGS
        if (task) {
            task->m_aimImmediate = m_bAimImmediate;
        }
#else
        task->m_aimImmediate = m_bAimImmediate;
#endif
        m_bAimImmediate = false;
        m_timer.Start(2'000);
        return task;
    }
    case TASK_COMPLEX_DESTROY_CAR:
        return new CTaskComplexDestroyCar{m_target->m_pVehicle, 0, 0, 0};
    case TASK_COMPLEX_SIGNAL_AT_PED: {
        if (FindPlayerPed(0) == m_target) {
            ped->Say(CTX_GLOBAL_PLAYER_WASTED);
        } else if (!CPedGroups::GetPedsGroup(m_target) || FindPlayerPed(0) != CPedGroups::GetPedsGroup(m_target)->GetMembership().GetLeader()) {
            ped->Say(CTX_GLOBAL_ENEMY_GANG_WASTED);
        }
        return new CTaskComplexSignalAtPed{m_target, CGeneral::GetRandomNumberInRange(0, 1500), true};
    }
    case TASK_FINISHED:
        ped->bDontAcceptIKLookAts = false;
        return nullptr;
    default:
        return nullptr;
    }
}
