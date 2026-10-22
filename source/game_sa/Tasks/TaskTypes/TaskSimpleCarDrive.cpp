#include "StdInc.h"
#include "TaskSimpleCarDrive.h"
#include "TaskUtilityLineUpPedWithCar.h"
#include "Ragdoll/IKChainManager.h"

#include "CarEnterExit.h"
#include "DamageManager.h"
#include "EventCarUpsideDown.h"
#include "EventCopCarBeingStolen.h"
#include "EventScriptCommand.h"
#include "GameLogic.h"
#include "PedGroups.h"
#include "TaskComplexDriveWander.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexSequence.h"
#include "TaskSimpleCarSetPedOut.h"
#include "TaskSimpleGangDriveBy.h"
#include "TaskSimpleHoldEntity.h"
#include "UpsideDownCarCheck.h"
#include "VehicleAnimGroupData.h"
#include "Weather.h"

namespace {
//! The car-seat animations `SetPedPosition` looks for while lining the ped up with the car.
constexpr AnimationId s_CarSeatAnims[] = {
    static_cast<AnimationId>(359), static_cast<AnimationId>(360),
    static_cast<AnimationId>(361), static_cast<AnimationId>(362),
    static_cast<AnimationId>(363), static_cast<AnimationId>(367),
    static_cast<AnimationId>(368), static_cast<AnimationId>(369),
    static_cast<AnimationId>(370), static_cast<AnimationId>(371),
    static_cast<AnimationId>(372), static_cast<AnimationId>(386),
};

//! 0x642700 - Plays (and remembers) the animation used when the ped rolls the car door shut.
void StartCloseDoorAnim(CTaskSimpleCarDrive& task, CPed* ped) {
    // TODO(re: The vanilla code derives the animation group with
    // `CVehicleAnimGroup::GetGroup(anim 0x182)` (the id isn't named in `AnimationEnums.h`) and
    // installs the callback at `0x63C840`; neither that callback's behaviour nor the anim's name is
    // established. The animation is played the same way.
    const auto animId = static_cast<AnimationId>(0x182);
    task.m_pAnimCloseDoorRolling = CAnimManager::AddAnimation(
        ped->GetRpClump(),
        CVehicleAnimGroupData::GetGroupForAnim(ANIM_GROUP_STDCARAMIMS, animId),
        animId
    );
    if (task.m_pAnimCloseDoorRolling) {
        task.m_pAnimCloseDoorRolling->SetFinishCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
    }
}
} // namespace

void CTaskSimpleCarDrive::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleCarDrive, 0x86E904, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x63C340);
    RH_ScopedInstall(Destructor, 0x63C460);

    RH_ScopedInstall(TriggerIK, 0x63C500);
    RH_ScopedInstall(UpdateBopping, 0x63C900);
    RH_ScopedInstall(StartBopping, 0x642760);
    RH_ScopedInstall(ProcessHeadBopping, 0x6428C0);
    RH_ScopedInstall(ProcessArmBopping, 0x642AE0);
    RH_ScopedInstall(ProcessBopping, 0x642E70);
    RH_ScopedVMTInstall(Clone, 0x63DC20);
    RH_ScopedVMTInstall(GetTaskType, 0x63C450);
    RH_ScopedVMTInstall(MakeAbortable, 0x63C670);
    RH_ScopedVMTInstall(ProcessPed, 0x644470);
    RH_ScopedVMTInstall(SetPedPosition, 0x63C770);
}

// 0x63C340
CTaskSimpleCarDrive::CTaskSimpleCarDrive(CVehicle* vehicle, CTaskUtilityLineUpPedWithCar* utilityTask, bool updateCurrentVehicle) : CTaskSimple() {
    m_pVehicle = vehicle;
    m_pAnimCloseDoorRolling = nullptr;
    m_pTaskUtilityLineUpPedWithCar = nullptr;
    m_nTimePassedSinceCarUpSideDown = 0;

    m_bUpdateCurrentVehicle = updateCurrentVehicle;
    m_b08 = true;

    CEntity::SafeRegisterRef(m_pVehicle);

    if (utilityTask) {
        m_pTaskUtilityLineUpPedWithCar = new CTaskUtilityLineUpPedWithCar(CVector{}, 0, utilityTask->m_nDoorOpenPosType, utilityTask->m_nDoorIdx);
    }

    m_fHeadBoppingFactor = 0.0f;
    m_fHeadBoppingOrientation = 0.0f;
    m_fRandomHeadBoppingMultiplier = 0.0f;
    m_nBoppingStartTime = -1;
}

// 0x63C460
CTaskSimpleCarDrive::~CTaskSimpleCarDrive() {
    CEntity::SafeCleanUpRef(m_pVehicle);

    if (m_pTaskUtilityLineUpPedWithCar) {
        delete m_pTaskUtilityLineUpPedWithCar;
        m_pTaskUtilityLineUpPedWithCar = nullptr;
    }

    if (m_b20 && m_pAnimCloseDoorRolling) {
        // TODO: FIX ME: Keeps triggering, annoying as fuck
        // Seemingly happens when getting of a motorbike (like cops getting off)
        //assert(m_pAnimCloseDoorRolling);
        m_pAnimCloseDoorRolling->SetFinishCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
        if (m_pVehicle) {
            m_pVehicle->ClearGettingOutFlags(1);
        }
    }
}

// 0x63C500
void CTaskSimpleCarDrive::TriggerIK(CPed* ped) const {
    if (!m_pVehicle) {
        return;
    }

    // Made an early-out of this, as this same code is used in both possible cases.
    if (g_ikChainMan.IsLooking(ped) || CGeneral::GetRandomNumberInRange(0, 100) >= 5) {
        return;
    }

    switch (m_pVehicle->m_autoPilot.m_nCarMission) {
    case MISSION_RAMPLAYER_FARAWAY:
    case MISSION_RAMPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_FARAWAY:
    case MISSION_BLOCKPLAYER_CLOSE:
    case MISSION_BLOCKPLAYER_HANDBRAKESTOP: { // Make ped look at player ped
        g_ikChainMan.LookAt("DriveCar", ped, FindPlayerPed(0), 3000, BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
        break;
    }
    case MISSION_RAMCAR_FARAWAY:
    case MISSION_RAMCAR_CLOSE: {
        if (const auto vehTargetCar = m_pVehicle->m_autoPilot.m_TargetEntity) {
            if (vehTargetCar->GetIsTypeVehicle()) {
                if (const auto driver = vehTargetCar->m_pDriver) { // Make ped look at target car or it's driver (if any)
                    g_ikChainMan.LookAt("DriveCar", ped, driver, 3000, BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
                } else {
                    g_ikChainMan.LookAt("DriveCar", ped, vehTargetCar, 3000, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
                }
            }
        }
        break;
    }
    }
}

// 0x63C900
void CTaskSimpleCarDrive::UpdateBopping() {
    const auto timeDelta = (int32)CTimer::GetTimeInMS() - m_nBoppingStartTime;
    m_fBoppingProgress = (float)(timeDelta % m_nBoppingEndTime) / (float)m_nBoppingEndTime;
    m_nBoppingCompletedTimes = timeDelta / m_nBoppingEndTime % 2;
}

// 0x642760
void CTaskSimpleCarDrive::StartBopping(CPed* ped) {
    const auto vehicle = ped->m_pVehicle;

    // Try to adopt an already running bopping schedule (So that all occupants bop in sync)
    int32 startTime = -1;
    int32 interval  = 0;

    if (const auto driver = vehicle->m_pDriver; driver && driver != ped) {
        if (const auto other = static_cast<CTaskSimpleCarDrive*>(driver->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_DRIVE))) {
            startTime = other->m_nBoppingStartTime;
            interval  = other->m_nBoppingEndTime;
        }
    }
    if (startTime == -1) {
        for (const auto passenger : vehicle->m_apPassengers) {
            if (!passenger || passenger == ped) {
                continue;
            }
            if (const auto other = static_cast<CTaskSimpleCarDrive*>(passenger->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_CAR_DRIVE))) {
                startTime = other->m_nBoppingStartTime;
                interval  = other->m_nBoppingEndTime;
            }
            if (startTime != -1) {
                break;
            }
        }
    }

    if (startTime == -1) { // Nobody else is bopping, so start a new schedule
        m_nBoppingStartTime = CTimer::GetTimeInMS();
        // TODO(re: The vanilla code computes `m_nBoppingEndTime` right after a `_rand()` call through
        // two calls to the compiler's float->int helper (`0x821B40`), so the random range it uses
        // cannot be recovered from the decompilation. `4000` is a placeholder, not a value read from
        // the exe (it must stay non-zero - it's a divisor).
        m_nBoppingEndTime   = 4000;
        m_fBoppingProgress  = 0.f;
        return;
    }

    m_nBoppingStartTime = startTime;
    m_nBoppingEndTime   = interval;

    const auto timeDelta = (int32)CTimer::GetTimeInMS() - startTime;
    m_fBoppingProgress       = (float)(timeDelta % interval) / (float)interval;
    m_nBoppingCompletedTimes = timeDelta / interval % 2;
}

// 0x6428C0
void CTaskSimpleCarDrive::ProcessHeadBopping(CPed* ped, bool wantsToBop, float distSq) {
    float targetOrientation = 0.f;

    if (wantsToBop) {
        if (!m_b01) {
            // Start bopping every now and then, as long as we aren't already fading out
            if (m_fHeadBoppingFactor == 0.f && CGeneral::GetRandomNumberInRange(0, 1000) > 995) {
                if (m_nBoppingStartTime == -1) {
                    StartBopping(ped);
                    m_nHeadBoppingStartTime = m_nBoppingStartTime;
                } else {
                    m_nHeadBoppingStartTime = CTimer::GetTimeInMS();
                }
                m_nHeadBoppingDirection        = CGeneral::GetRandomNumberInRange<int32>(1, 3);
                m_fRandomHeadBoppingMultiplier = CGeneral::GetRandomNumberInRange<float>(3.f, 8.f);
                m_fHeadBoppingFactor           = 0.f;
                m_fHeadBoppingOrientation      = 0.f;
                goto UpdateFlag;
            }
        } else {
            targetOrientation = std::sin(m_fBoppingProgress * PI);

            // Keep bopping unless the animation has been running for a while (And the dice say stop)
            if (CTimer::GetTimeInMS() - m_nHeadBoppingStartTime < 5000
                || CGeneral::GetRandomNumberInRange(0, 1000) < 996
            ) {
                goto UpdateFlag;
            }
        }
        wantsToBop = false;
    }

UpdateFlag:
    m_b01 = wantsToBop;

    // Move the head towards the target orientation and blend the factor in/out
    {
        const auto factor = (float)m_b01;
        if (!wantsToBop) {
            m_fHeadBoppingFactor = std::max(m_fHeadBoppingFactor - 0.05f, 0.f);
        } else {
            m_fHeadBoppingFactor = std::min(m_fHeadBoppingFactor + 0.05f, 1.f);
        }
        m_fHeadBoppingOrientation += (targetOrientation - m_fHeadBoppingOrientation) * (wantsToBop ? m_fHeadBoppingFactor : (1.f - m_fHeadBoppingFactor));
        (void)factor;
    }

    if (distSq < 64.f && m_fHeadBoppingOrientation > 0.f) {
        const auto amount = m_fRandomHeadBoppingMultiplier * m_fHeadBoppingOrientation;
        auto&      quat   = ped->m_apBones[PED_NODE_HEAD]->KeyFrame->q;

        if (m_nHeadBoppingDirection > 0) {
            static const CVector s_AxisX{ 1.f, 0.f, 0.f };
            RtQuatRotate(&quat, &s_AxisX, m_nBoppingCompletedTimes != 0 ? -amount : amount, rwCOMBINEREPLACE);
        }

        static const CVector s_AxisZ{ 0.f, 0.f, 1.f };
        RtQuatRotate(&quat, &s_AxisZ, m_nHeadBoppingDirection != 2 ? -amount : amount, rwCOMBINEREPLACE);

        ped->bUpdateMatricesRequired = true;
    }
}

// 0x642AE0
void CTaskSimpleCarDrive::ProcessArmBopping(CPed* ped, bool wantsToBop, float distSq) {
    if (ped->IsPlayer()) {
        return;
    }

    const auto vehicle = ped->m_pVehicle;

    // The window flag belonging to the seat the ped is sitting in
    uint8 seatWindowFlag;
    bool  pedIsSeated = true;
    if (vehicle->m_pDriver == ped) {
        seatWindowFlag = 10;
    } else if (vehicle->m_apPassengers[0] == ped) {
        seatWindowFlag = 8;
    } else if (vehicle->m_apPassengers[1] == ped) {
        seatWindowFlag = 11;
    } else if (vehicle->m_apPassengers[2] == ped) {
        seatWindowFlag = 9;
    } else {
        pedIsSeated = false;
    }

    if (pedIsSeated) {
        if (CWeather::Rain <= 0.f) {
            vehicle->SetWindowOpenFlag(seatWindowFlag);
        } else { // Don't lean out when it's raining
            vehicle->ClearWindowOpenFlag(10);
            vehicle->ClearWindowOpenFlag(8);
            vehicle->ClearWindowOpenFlag(11);
            vehicle->ClearWindowOpenFlag(9);
            wantsToBop = false;
        }
    } else if (CWeather::Rain > 0.f) {
        vehicle->ClearWindowOpenFlag(10);
        vehicle->ClearWindowOpenFlag(8);
        vehicle->ClearWindowOpenFlag(11);
        vehicle->ClearWindowOpenFlag(9);
        wantsToBop = false;
    }

    // Rear seats use a different animation
    const bool isRearSeat = vehicle->m_apPassengers[0] == ped || vehicle->m_apPassengers[2] == ped;

    // TODO(re: The vanilla code uses the *numeric* animation ids `0xA2` (front) and `0xA3` (rear),
    // which aren't named in `AnimationEnums.h`.
    const auto animId = static_cast<AnimationId>(isRearSeat ? 0xA3 : 0xA2);
    auto* const assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), animId);

    if (!wantsToBop) {
        if (assoc) { // Rewind the animation so it stops
            assoc->m_CurrentTime = 0.f;
            assoc->m_Speed        = -4.f;
        }
        m_b02 = false;
        return;
    }

    if (!m_b02) {
        if (CGeneral::GetRandomNumberInRange(0, 1000) > 995) {
            bool canLeanOut = true;

            if (vehicle->m_nVehicleSubType == 0) { // (Not a "special" vehicle - see the sub-type hack)
                const auto door = static_cast<eTargetDoor>(CCarEnterExit::ComputeTargetDoorToExit(vehicle, ped));
                if (static_cast<CAutomobile*>(vehicle)->GetDamageManager().GetDoorStatus_Component(static_cast<tComponent>(door)) != DAMSTATE_OK
                    || CCarEnterExit::CarHasDoorToClose(vehicle, door)
                    || !vehicle->CanPedLeanOut(ped)
                ) {
                    canLeanOut = false;
                }

                // TODO(re: The vanilla code additionally gates this on the vehicle model's dummy
                // positions (`CVehicleModelInfo::m_pVehicleStruct`: the `z` of two dummies, one of
                // them at `+0x78`), on `vehicleFlags.bLowVehicle` and on a per-anim-group flag read
                // from `0xC1CDC8` (stride `0x94`, bit 3). Which dummy lives at `+0x38` isn't
                // established, so that part of the gate is not reproduced.
            }

            if (canLeanOut) {
                CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, animId, 4.f);

                if (m_nBoppingStartTime == -1) {
                    StartBopping(ped);
                    m_nArmBoppingStartTime = m_nBoppingStartTime;
                    m_b02 = true;
                    return;
                }
                m_nArmBoppingStartTime = CTimer::GetTimeInMS();
                m_b02 = true;
                return;
            }
        }
        return;
    }

    // Already leaning out
    if (distSq < 64.f) {
        auto amount = 0.f;
        if (m_fBoppingProgress >= 0.75f) {
            amount = (m_fBoppingProgress - 0.75f) * 45.f;
        }
        auto& quat = isRearSeat
            ? ped->m_apBones[PED_NODE_RIGHT_HAND]->KeyFrame->q
            : ped->m_apBones[PED_NODE_LEFT_HAND]->KeyFrame->q;

        static const CVector s_AxisZ{ 0.f, 0.f, 1.f };
        RtQuatRotate(&quat, &s_AxisZ, -amount, rwCOMBINEREPLACE);

        ped->bUpdateMatricesRequired = true;
    }

    if (CTimer::GetTimeInMS() - m_nArmBoppingStartTime > 5000
        && CGeneral::GetRandomNumberInRange(0, 1000) > 995
        && assoc
    ) {
        assoc->m_CurrentTime = 0.f;
        assoc->m_Speed        = -4.f;
        m_b02 = false;
    }
}

// 0x642E70
void CTaskSimpleCarDrive::ProcessBopping(CPed* ped, bool a3) {
    if (ped->m_pVehicle->m_pDriver == FindPlayerPed(0)
        || ped->m_nPedType == PED_TYPE_COP
        || ped->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT_AND_STAND_UP)
    ) {
        return;
    }

    auto* vehicle = ped->m_pVehicle;
    if (vehicle->IsAutomobile() && !vehicle->IsSubQuad() && !ped->IsCreatedByMission()) {
        if (m_nBoppingStartTime != -1) { // IsBopping
            UpdateBopping();
        }

        const auto dist = DistanceBetweenPointsSquared(TheCamera.GetPosition(), ped->GetPosition());
        ProcessHeadBopping(ped, a3, dist);
        ProcessArmBopping(ped, a3, dist);
        if (m_nBoppingStartTime != -1 && !m_b01 && !m_b02) {
            m_nBoppingStartTime = -1;
        }
    }
}

// 0x63DC20
CTask* CTaskSimpleCarDrive::Clone() const {
    auto task = new CTaskSimpleCarDrive(m_pVehicle);
    task->m_bUpdateCurrentVehicle = m_bUpdateCurrentVehicle;
    return task;
}

// 0x63C670
bool CTaskSimpleCarDrive::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority != ABORT_PRIORITY_IMMEDIATE) {
        // Flag the ped to be put back in the vehicle (See `SetPedPosition`)
        m_b08 = true;
        return !m_b20;
    }

    // Immediate: Yank the ped out of the car
    if (ped->bInVehicle) {
        if (const auto door = CCarEnterExit::ComputeTargetDoorToExit(ped->m_pVehicle, ped)) {
            CTaskSimpleCarSetPedOut setPedOut{ ped->m_pVehicle, static_cast<eTargetDoor>(door), false };
            setPedOut.ProcessPed(ped);
        }
    }

    if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 250);
    }
    return true;
}

// 0x63C770
bool CTaskSimpleCarDrive::SetPedPosition(CPed* ped) {
    if (m_b08) {
        // Take over the utility task of whichever car-drive task is now in the primary slot (`this`
        // is being replaced by it - e.g. when re-entering the vehicle)
        if (const auto other = static_cast<CTaskSimpleCarDrive*>(ped->GetTaskManager().FindTaskByType(TASK_PRIMARY_DEFAULT, TASK_SIMPLE_CAR_DRIVE))) {
            if (other != this && other->m_pTaskUtilityLineUpPedWithCar) {
                m_pTaskUtilityLineUpPedWithCar       = other->m_pTaskUtilityLineUpPedWithCar;
                other->m_pTaskUtilityLineUpPedWithCar = nullptr;
            }
        }
        m_b08 = false;
    }

    if (!m_pTaskUtilityLineUpPedWithCar) {
        if (m_pVehicle && ped->m_pVehicle == m_pVehicle && ped->bInVehicle) {
            ped->SetPedPositionInCar();
        }
        return true;
    }

    // Line the ped up with the first car-seat animation that is playing
    CAnimBlendAssociation* assoc = nullptr;
    for (const auto animId : s_CarSeatAnims) {
        if ((assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), animId))) {
            break;
        }
    }

    m_pTaskUtilityLineUpPedWithCar->ProcessPed(ped, m_pVehicle, assoc);

    delete m_pTaskUtilityLineUpPedWithCar;
    m_pTaskUtilityLineUpPedWithCar = nullptr;

    return true;
}

// 0x644470
bool CTaskSimpleCarDrive::ProcessPed(CPed* ped) {
    const bool radioRetuneJustStarted = AudioEngine.HasRadioRetuneJustStarted();

    // Keep `m_pVehicle` pointing at the vehicle the ped is currently in
    if (m_bUpdateCurrentVehicle && ped->bInVehicle && ped->m_pVehicle) {
        if (ped->m_pVehicle != m_pVehicle) {
            if (m_pVehicle) {
                CEntity::SafeCleanUpRef(m_pVehicle);
            }
            m_pVehicle = ped->m_pVehicle;
            CEntity::SafeRegisterRef(m_pVehicle);
        }
    }

    if (!m_pVehicle || !ped->bInVehicle) {
        ped->bInVehicle = false;
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }
        return true;
    }

    const auto group = CPedGroups::GetPedsGroup(ped);

    // Upside down car
    if (!CUpsideDownCarCheck{}.IsCarUpsideDown(m_pVehicle)) {
        m_nTimePassedSinceCarUpSideDown = 0;
    } else {
        if (const auto player = FindPlayerPed(); ped != player && player->bInVehicle && player->m_pVehicle == m_pVehicle) {
            ped->Say((eGlobalSpeechContext)22, 0, 1.f);
        }
        if (!group || group->GetMembership().IsLeader(ped)) {
            m_nTimePassedSinceCarUpSideDown += (int32)CTimer::GetTimeStepInMS();
            if (m_nTimePassedSinceCarUpSideDown > 2000) {
                ped->GetEventGroup().Add(new CEventCarUpsideDown(m_pVehicle), false);
            }
        }
    }

    // Player stealing a cop car
    if (ped->IsPlayer() && m_pVehicle->IsLawEnforcementVehicle()) {
        if (!m_copCarStolenTimer.IsStarted() || m_copCarStolenTimer.IsOutOfTime()) {
            m_copCarStolenTimer.Start(2000);
            GetEventGlobalGroup()->Add(new CEventCopCarBeingStolen(ped, m_pVehicle), false);
        }
    }

    // Peds on bikes/quads can be shot while driving
    if (m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BIKE
        || m_pVehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD
        || m_pVehicle->m_nModelIndex == 0x213 // TODO(re: Unnamed model id (0x213))
    ) {
        ped->bTestForShotInVehicle = true;
    }

    const bool isDriver = ped->IsPlayer() && m_pVehicle->m_pDriver == ped;
    if (!isDriver) {
        if (m_pVehicle->m_pDriver != ped && m_pVehicle->IsPassenger(ped)) {
            // Passenger: blend the "sitting in a car" animation (unless the ped is leaning out)
            if (!m_b08 && !m_b10) {
                if (const auto rideAnimData = m_pVehicle->GetRideAnimData()) {
                    CAnimManager::BlendAnimation(ped->GetRpClump(), rideAnimData->AnimGroup, static_cast<AnimationId>(0xCD), 8.f);
                }
                m_b10 = true;
            }

            ProcessBopping(ped, true);

            // Co-op: let the player lean out and drive-by
            if (ped->IsPlayer() && CGameLogic::IsCoopGameGoingOn()
                && ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_TotalAmmo > 0
                && m_pVehicle->CanPedLeanOut(ped)
            ) {
                const bool isRearSeat = m_pVehicle->m_apPassengers[1] != ped;
                auto* task = new CTaskSimpleGangDriveBy(nullptr, nullptr, 100.f, 100, eDrivebyStyle::AI_ALL_DIRN, isRearSeat);
                ped->GetEventGroup().Add(new CEventScriptCommand(TASK_PRIMARY_PRIMARY, task, false), false);
            }
        }
    } else {
        // Driver: let the player lean out and drive-by (co-op)
        if (ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_TotalAmmo > 0 && m_pVehicle->CanPedLeanOut(ped)) {
            auto* task = new CTaskSimpleGangDriveBy(nullptr, nullptr, 100.f, 100, eDrivebyStyle::AI_ALL_DIRN, false);
            ped->GetEventGroup().Add(new CEventScriptCommand(TASK_PRIMARY_PRIMARY, task, false), false);
        }

        // Get-out animation when the door is open and the player isn't trying to drive
        if (m_pVehicle->m_nModelIndex != 0x23A // TODO(re: Unnamed model id (0x23A))
            && m_pVehicle->m_nVehicleType == VEHICLE_TYPE_TRAIN
            && !m_pVehicle->IsOpenTopCar() /* TODO(re: `IsClosed()` in the vanilla code */
        ) {
            if (const auto pad = ped->AsPlayer()->GetPadFromPlayer()) {
                if (pad->GetAccelerate() == 0 && pad->GetSteeringLeftRight() == 0 && pad->GetBrake() == 0) {
                    const auto groupId = CVehicleAnimGroupData::GetGroupForAnim(
                        static_cast<AssocGroupId>(m_pVehicle->m_pHandlingData->GetAnimGroupId()),
                        static_cast<AnimationId>(0x182)
                    );
                    m_pVehicle->ProcessOpenDoor(ped, 10, groupId, 0x182, 1.f);
                }
            }
            goto AfterDrivingAnims;
        }

        // Driver's door is damaged: get out (through it)
        if (m_pVehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE
            && static_cast<CAutomobile*>(m_pVehicle)->GetDamageManager().GetDoorStatus(DOOR_LEFT_FRONT) == DAMSTATE_OPENED
            && !m_pVehicle->IsDoorMissing(DOOR_LEFT_FRONT)
        ) {
            const auto assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), static_cast<AnimationId>(0x182));
            const bool doorFlagsClear = !(m_pVehicle->m_nGettingOutFlags & 1) && !(m_pVehicle->m_nGettingInFlags & 1);
            if (!doorFlagsClear) {
                if (!assoc) {
                    const auto pad = ped->AsPlayer()->GetPadFromPlayer();
                    if (!pad || (pad->GetAccelerate() == 0 && pad->GetSteeringLeftRight() == 0 && pad->GetBrake() == 0)) {
                        m_pVehicle->SetGettingOutFlags(1);
                        m_b20 = true;
                        StartCloseDoorAnim(*this, ped);
                        return false;
                    }
                } else {
                    const auto groupId = CVehicleAnimGroupData::GetGroupForAnim(
                        static_cast<AssocGroupId>(m_pVehicle->m_pHandlingData->GetAnimGroupId()),
                        static_cast<AnimationId>(assoc->GetAnimId())
                    );
                    m_pVehicle->ProcessOpenDoor(ped, 10, groupId, static_cast<uint32>(assoc->GetAnimId()), assoc->m_CurrentTime);
                }
            }
            ProcessBopping(ped, false);
            goto AfterDrivingAnims;
        }

        m_pVehicle->ProcessDrivingAnims(ped, radioRetuneJustStarted);
        ProcessBopping(ped, true);
    }

AfterDrivingAnims:
    if (const auto heldEntity = ped->GetEntityThatThisPedIsHolding()) {
        if (heldEntity->m_nModelIndex == ModelIndices::MI_GANG_SMOKE
            && !g_ikChainMan.IsLooking(ped)
            && CGeneral::GetRandomNumberInRange(0, 60) == 15
            && ped->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_HOLD_ENTITY)
        ) {
            if (const auto holdTask = static_cast<CTaskSimpleHoldEntity*>(ped->GetTaskManager().FindActiveTaskByType(TASK_SIMPLE_HOLD_ENTITY))) {
                holdTask->PlayAnim(static_cast<AnimationId>(m_pVehicle->IsPassenger(ped) ? 0xAA : 0xA9), ANIM_GROUP_DEFAULT);
            }
        }
    }

    if (!RpAnimBlendClumpGetAssociation(ped->GetRpClump(), static_cast<AnimationId>(0xA9))) {
        TriggerIK(ped);
    } else {
        if (g_ikChainMan.IsLooking(ped)) {
            g_ikChainMan.AbortLookAt(ped, 250);
        }
        if (m_nBoppingStartTime != -1) {
            m_nBoppingStartTime = -1;
        }
    }

    // Front passenger waiting for the driver when there's none
    if (ped->IsCreatedBy(PED_GAME) && !CPedGroups::GetPedsGroup(ped)) {
        if (!m_pVehicle->m_pDriver && m_pVehicle->m_apPassengers[0] == ped) {
            m_TaskTimer.StartIfNotAlready(4000);
            if (m_TaskTimer.IsOutOfTime()) {
                auto* sequence = new CTaskComplexSequence();
                sequence->AddTask(new CTaskComplexLeaveCar(m_pVehicle, 0, 0, true, false));
                sequence->AddTask(new CTaskComplexCarDriveWander(m_pVehicle, DRIVING_STYLE_STOP_FOR_CARS, 10.f));
                ped->GetEventGroup().Add(new CEventScriptCommand(TASK_PRIMARY_PRIMARY, sequence, false), false);
            }
        }
    }

    // Every now and then the player says something about the driver's driving
    if (((uint16)CTimer::GetFrameCounter() & 0x3FFF) == 0 && m_pVehicle->m_pDriver && m_pVehicle->m_pDriver != ped) {
        if (ped->IsPlayer()) {
            const auto& moveSpeed = m_pVehicle->m_vecMoveSpeed;
            if (moveSpeed.Magnitude2D() > 0.7f) {
                ped->Say((eGlobalSpeechContext)0x20, 0, 1.f);
            }
            if (moveSpeed.Magnitude2D() < 0.1f) {
                ped->Say((eGlobalSpeechContext)0x27, 0, 1.f);
            }
            ped->Say((eGlobalSpeechContext)0x29, 0, 1.f);
        }
    }

    return false;
}
