#include "StdInc.h"

#include "TaskComplexCarSlowBeDraggedOutAndStandUp.h"
#include "TaskComplexCarSlowBeDraggedOut.h"
#include "TaskComplexGetUpAndStandStill.h"
#include "TaskComplexLeaveCar.h"
#include "TaskSimpleCarSetPedOut.h"
#include "Events/EventDeath.h"
#include "Vehicle.h"

void CTaskComplexCarSlowBeDraggedOutAndStandUp::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexCarSlowBeDraggedOutAndStandUp, 0x86EF80, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x648620);
    RH_ScopedInstall(Destructor, 0x648690);
    RH_ScopedInstall(CreateSubTask, 0x648710);
    RH_ScopedInstall(Clone, 0x64A190);
    RH_ScopedInstall(GetTaskType, 0x648680);
    RH_ScopedInstall(MakeAbortable, 0x6486F0);
    RH_ScopedInstall(CreateNextSubTask, 0x6488F0);
    RH_ScopedInstall(CreateFirstSubTask, 0x648A10);
    RH_ScopedInstall(ControlSubTask, 0x648B80);
}

// 0x648620
CTaskComplexCarSlowBeDraggedOutAndStandUp::CTaskComplexCarSlowBeDraggedOutAndStandUp(CVehicle* vehicle, int32 a3) : CTaskComplex() {
    m_Vehicle = vehicle;
    m_Door = a3;
    CEntity::SafeRegisterRef(m_Vehicle);
}

// 0x648690
CTaskComplexCarSlowBeDraggedOutAndStandUp::~CTaskComplexCarSlowBeDraggedOutAndStandUp() {
    CEntity::SafeCleanUpRef(m_Vehicle);
}

// 0x648710
CTask* CTaskComplexCarSlowBeDraggedOutAndStandUp::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_COMPLEX_LEAVE_CAR: { // 0x2c0
        eTargetDoor door;
        switch (m_Vehicle->GetAnimGroupId()) {
        case ANIM_GROUP_COACHCARANIMS:
            door = (eTargetDoor)8;
            break;
        case ANIM_GROUP_TANKCARANIMS:
            door = (eTargetDoor)10;
            break;
        case ANIM_GROUP_BUSCARANIMS:
            if (m_Door != 8) {
                return nullptr;
            }
            door = (eTargetDoor)8;
            break;
        default:
            return nullptr;
        }
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, (int32)door, 0, false, true };
    }
    case TASK_COMPLEX_GET_UP_AND_STAND_STILL: // 0xce
        return new CTaskComplexGetUpAndStandStill{};
    case TASK_SIMPLE_CAR_SET_PED_OUT: // 0x330
        return new CTaskSimpleCarSetPedOut{ ped->m_pVehicle, (eTargetDoor)m_Door, true };
    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT: // 0x337
        return new CTaskComplexCarSlowBeDraggedOut{ m_Vehicle, (eTargetDoor)m_Door, false };
    default:
        return nullptr;
    }
}

// 0x6486F0
bool CTaskComplexCarSlowBeDraggedOutAndStandUp::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        return m_pSubTask->MakeAbortable(ped, priority, event);
    }
    return false;
}

// 0x6488F0
CTask* CTaskComplexCarSlowBeDraggedOutAndStandUp::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT: // 0x337
        if (ped->IsAlive() && ped->m_fHealth > 0.0f) {
            return new CTaskComplexGetUpAndStandStill{};
        }
        ped->GetEventGroup().Add(CEventDeath{ false }, false);
        return nullptr;
    case TASK_COMPLEX_GET_UP_AND_STAND_STILL: // 0xce
        // NOTE: original calls `ped->IsPlayer()` here and discards the result
        return nullptr;
    default: // TASK_SIMPLE_CAR_SET_PED_OUT, TASK_COMPLEX_LEAVE_CAR, ...
        return nullptr;
    }
}

// 0x648A10
CTask* CTaskComplexCarSlowBeDraggedOutAndStandUp::CreateFirstSubTask(CPed* ped) {
    auto* const veh = ped->m_pVehicle;
    assert(veh); // original derefs it unconditionally further down

    if (ped == FindPlayerPed(0)) {
        if (veh) {
            veh->m_vehicleAudio.PlayerAboutToExitVehicleAsDriver();
        }
    } else if (veh->m_pDriver == ped) {
        ped->SetRadioStation();
    }

    const auto group = veh->GetAnimGroupId();
    if (group == ANIM_GROUP_COACHCARANIMS || group == ANIM_GROUP_TANKCARANIMS || (m_Door == 8 && group == ANIM_GROUP_BUSCARANIMS)) {
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
    }

    std::optional<eAudioEvents> audioEvent;
    switch (group) {
    case ANIM_GROUP_STDCARAMIMS:
    case ANIM_GROUP_VANCARANIMS:
    case ANIM_GROUP_RUSTPLANEANIMS:
    case ANIM_GROUP_KARTCARANIMS:
    case ANIM_GROUP_CONVCARANIMS:
    case ANIM_GROUP_STDTALLCARAMIMS:
    case ANIM_GROUP_HOVERCARANIMS:
    case ANIM_GROUP_STDCARUPRIGHT:
        if (m_Door == 10) {
            audioEvent = AE_PED_JACKED_CAR_PUNCH;
        } else if (m_Door == 8) {
            audioEvent = AE_PED_JACKED_CAR_HEAD_BANG;
        }
        break;
    case ANIM_GROUP_LOWCARAMIMS:
        audioEvent = AE_PED_JACKED_CAR_KICK;
        break;
    case ANIM_GROUP_DOZERCARANIMS:
        audioEvent = AE_PED_JACKED_DOZER;
        break;
    }
    if (audioEvent) {
        ped->m_pedAudio.AddAudioEvent(*audioEvent, 0.0f, 1.0f, nullptr, SURFACE_DEFAULT, 0, 0);
    }

    return CreateSubTask(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT, ped);
}

// 0x648B80
CTask* CTaskComplexCarSlowBeDraggedOutAndStandUp::ControlSubTask(CPed* ped) {
    return m_pSubTask;
}
