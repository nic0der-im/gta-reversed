#include "StdInc.h"

#include "TaskComplexProstituteSolicit.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexEnterCarAsPassenger.h"
#include "TaskSimpleStandStill.h"
#include "TaskComplexCarDrive.h"
#include "SeekEntity/TaskComplexSeekEntity.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "CarEnterExit.h"
#include "Pad.h"
#include "General.h"
#include "Stats.h"
#include "Wanted.h"
#include "Audio/entities/AEPedSpeechAudioEntity.h"

// 0x661A60
CTaskComplexProstituteSolicit::CTaskComplexProstituteSolicit(CPed* client) : CTaskComplex() {
    m_nLastSavedTime = 0;
    m_nNextTimeToCheckForSecludedPlace = 0;
    m_nLastPaymentTime = 0;
    m_nCurrentTimer = 0;
    m_pClient = client;
    m_nVehicleMovementTimer = 850;
    b07 = true;
    b08 = true;
    b10 = true;
    m_pClient->RegisterReference(m_pClient);
}

// 0x661AF0
CTaskComplexProstituteSolicit::~CTaskComplexProstituteSolicit() {
    auto player = FindPlayerPed();
    if (!player)
        return;

    CEntity::ClearReference(player->GetPlayerData()->m_pCurrentProstitutePed);
    if (bMoveCameraDown) {
        bMoveCameraDown = false;
    }
}

// 0x661B80
bool CTaskComplexProstituteSolicit::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    bool aborted = m_pSubTask->MakeAbortable(ped, priority, event);
    if (aborted) {
        bMoveCameraDown = false;
    }
    return aborted;
}

// 0x661D30
void CTaskComplexProstituteSolicit::GetRidOfPlayerProstitute() {
    auto* prostitute = FindPlayerPed()->GetPlayerData()->m_pCurrentProstitutePed;
    if (!prostitute)
        return;

    const auto intel = prostitute->GetIntelligence();
    auto* task = intel->FindTaskByType(TASK_COMPLEX_PROSTITUTE_SOLICIT);
    if (!task)
        return;

    auto* t = static_cast<CTaskComplexProstituteSolicit*>(task);
    t->bTaskCanBeFinished = true;
    t->m_nCurrentTimer = 0;
}

// 0x666360
CTask* CTaskComplexProstituteSolicit::CreateSubTask(eTaskType taskType, CPed* prostitute) {
    CVehicle* const veh = m_pClient->m_pVehicle;

    switch (taskType) {
    case TASK_COMPLEX_CAR_DRIVE:
        bSearchingForSecludedPlace = true;
        bSexProcessStarted         = false;
        return new CTaskComplexCarDrive(veh);

    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill(5000, false, false, 8.0f);

    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        return new CTaskComplexEnterCarAsPassenger(veh, 8, false);

    case TASK_COMPLEX_LEAVE_CAR:
        return new CTaskComplexLeaveCar(veh, 0, 0, true, false);

    case TASK_COMPLEX_SEEK_ENTITY: {
        CMatrix inv;
        Invert(*veh->m_matrix, inv);

        const CVector door10 = CCarEnterExit::GetPositionToOpenCarDoor(veh, 10);
        const CVector door8  = CCarEnterExit::GetPositionToOpenCarDoor(veh, 8);

        // Seek the door closest to the prostitute (In vehicle space)
        const CVector& prostitutePos = prostitute->GetPosition();
        const CVector& doorPos       = (prostitutePos - door8).SquaredMagnitude() <= (prostitutePos - door10).SquaredMagnitude()
            ? door8
            : door10;

        return new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
            veh,
            50'000,
            1'000,
            1.0f,
            2.0f,
            2.0f,
            false,
            false,
            CEntitySeekPosCalculatorXYOffset{ inv.TransformPoint(doorPos) }
        };
    }

    case TASK_COMPLEX_TURN_TO_FACE_ENTITY:
        return new CTaskComplexTurnToFaceEntityOrCoord(m_pClient, 0.5f, 0.2f);

    default:
        return nullptr;
    }
}

// 0x661BB0
bool CTaskComplexProstituteSolicit::IsTaskValid(CPed* prostitute, CPed* ped) {
    if (FindPlayerPed() != ped)
        return false;

    if (!ped)
        return false;

    if (!ped->IsInVehicle())
        return false;

    if (ped->bIsBeingArrested)
        return false;

    if (ped->GetPlayerData()->m_pCurrentProstitutePed && ped->GetPlayerData()->m_pCurrentProstitutePed != prostitute)
        return false;

    if (ped->m_pVehicle->GetVehicleAppearance() != VEHICLE_APPEARANCE_AUTOMOBILE)
        return false;

    if (ped->m_pVehicle->IsUpsideDown())
        return false;

    if (ped->m_pVehicle->IsOnItsSide())
        return false;

    auto task = ped->GetTaskManager().GetSimplestActiveTask();
    if (task->GetTaskType() != TASK_SIMPLE_CAR_DRIVE)
        return false;

    if (ped->m_pVehicle->m_pDriver != ped)
        return false;

    if (prostitute->m_pVehicle) {
        if (prostitute->m_pVehicle != ped->m_pVehicle || prostitute->m_pVehicle->m_nNumPassengers != 1)
            return false;
    } else if (ped->m_pVehicle->m_nNumPassengers) {
        return false;
    }

    if (!ped->m_pVehicle->m_nMaxPassengers || ped->m_pVehicle->m_pHandlingData->m_bTandemSeats)
        return false;

    CVector out = ped->GetPosition() - prostitute->GetPosition();
    if (out.SquaredMagnitude() > 100.0f || CTheScripts::IsPlayerOnAMission() || CGameLogic::IsCoopGameGoingOn()) {
        return false;
    }
    return true;
}

// 0x6666A0
CTask* CTaskComplexProstituteSolicit::CreateFirstSubTask(CPed* ped) {
    if (!IsTaskValid(ped, m_pClient)) {
        bTaskCanBeFinished = true;
        return nullptr;
    }

    m_vecVehiclePosn = m_pClient->m_pVehicle->GetPosition();

    auto player = FindPlayerPed();
    m_pClient->GetPlayerData()->m_pCurrentProstitutePed = ped;
    player->GetPlayerData()->m_pCurrentProstitutePed->RegisterReference(player->GetPlayerData()->m_pCurrentProstitutePed);

    if (m_pClient->GetPlayerData()->m_pLastProstituteShagged != ped) {
        CEntity::SafeCleanUpRef(m_pClient->GetPlayerData()->m_pLastProstituteShagged);
        m_pClient->GetPlayerData()->m_pLastProstituteShagged = ped;
        m_pClient->GetPlayerData()->m_pLastProstituteShagged->RegisterReference(m_pClient->GetPlayerData()->m_pLastProstituteShagged);
    }

    return CreateSubTask(TASK_COMPLEX_SEEK_ENTITY, ped);
}

// 0x666780
CTask* CTaskComplexProstituteSolicit::CreateNextSubTask(CPed* ped) {
    if (!m_pClient) {
        return nullptr;
    }

    if (!IsTaskValid(ped, m_pClient)) {
        bTaskCanBeFinished = true;
    }

    switch (const auto subTaskType = m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_STAND_STILL:
        if (!bPlayerHasAcceptedSexProposition) {
            return CreateSubTask(TASK_FINISHED, ped);
        }
        if (!CCheat::IsActive(CHEAT_PROSTITUTES_PAY_YOU)) {
            auto player = FindPlayerPed();
            if (player->GetPlayerInfoForThisPlayerPed()->m_nMoney < 20) {
                CMessages::ClearMessages(false);
                CMessages::AddMessageQ(TheText.Get("PROS_06"), 2000, 1, true); // You've got money right?
                CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true); // Stop wasting my time!
                return CreateSubTask(TASK_FINISHED, ped);
            }
        }
        return CreateSubTask(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER, ped);

    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        ped->Say(CTX_GLOBAL_SOLICIT_THANKS);
        m_vecVehiclePosn = m_pClient->m_pVehicle->GetPosition();
        return CreateSubTask(TASK_COMPLEX_CAR_DRIVE, ped);

    case TASK_COMPLEX_LEAVE_CAR:
        g_ikChainMan.LookAt("TaskProzzy", ped, m_pClient, 2500, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
        return CreateSubTask(TASK_FINISHED, ped);

    case TASK_COMPLEX_SEEK_ENTITY:
        g_ikChainMan.LookAt("TaskProzzy", ped, m_pClient, 5000, BONE_UNKNOWN, nullptr, false, 0.25f, 500, 3, false);
        return CreateSubTask(TASK_COMPLEX_TURN_TO_FACE_ENTITY, ped);

    case TASK_COMPLEX_TURN_TO_FACE_ENTITY:
        ped->Say(CTX_GLOBAL_SOLICIT);
        CMessages::AddMessageQ(TheText.Get("PROS_04"), 5000, 1, true); // You want a good time, honey?
        return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);

    case TASK_COMPLEX_CAR_DRIVE:
        return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);

    default:
        return nullptr;
    }
}

// 0x6669D0
CTask* CTaskComplexProstituteSolicit::ControlSubTask(CPed* ped) {
    // TODO(re): Unknown global (0 in the static image) - When 1 the whole "sex" process is skipped
    static auto& s_SexProcessSkipped = StaticRef<int32>(0xB6F0D8);

    // Falls back onto the state of the "sex" process (See below)
    bMoveCameraDown = bSexProcessStarted;

    if (!IsTaskValid(ped, m_pClient)) {
        bMoveCameraDown    = false;
        bTaskCanBeFinished = true;
    }

    const auto delta = static_cast<int16>(CTimer::GetTimeStepInMS());

    if (bTaskCanBeFinished) {
        if (!m_nCurrentTimer) {
            if (m_pSubTask->GetTaskType() != TASK_COMPLEX_LEAVE_CAR
                && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)
            ) {
                return CreateSubTask(TASK_COMPLEX_LEAVE_CAR, ped);
            }
        } else if ((m_nCurrentTimer -= delta) < 1) {
            CAEPedSpeechAudioEntity::SetCJMood(MOOD_WR, 120'000);
            m_pClient->Say(CTX_GLOBAL_AFTER_SEX, 0, 1.f, false, false, false);
            m_nCurrentTimer = 0;
        }
        return m_pSubTask;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_STAND_STILL: {
        if (CPad::GetPad()->ConversationYesJustDown()) {
            bPlayerHasAcceptedSexProposition = true;
            if (m_pClient) {
                ped->Say(CTX_GLOBAL_SOLICIT_PRO_YES, 0, 1.f, false, false, false);
            }
            return CreateFirstSubTask(ped);
        }
        if (CPad::GetPad()->ConversationNoJustDown()) {
            if (m_pClient) {
                ped->Say(CTX_GLOBAL_SOLICIT_PRO_NO, 0, 1.f, false, false, false);
            }
            bTaskCanBeFinished = true;
        }
        return m_pSubTask;
    }
    case TASK_COMPLEX_SEEK_ENTITY:
    case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
        // The vehicle (And with that, the ped) has moved away - Can't do it anymore
        if ((m_pClient->m_pVehicle->GetPosition() - m_vecVehiclePosn).SquaredMagnitude() > 16.0f) {
            bTaskCanBeFinished = true;
        }
        return m_pSubTask;
    default:
        break;
    }

    if (m_pSubTask->GetTaskType() != TASK_COMPLEX_CAR_DRIVE) {
        return m_pSubTask;
    }

    // ---------------- TASK_COMPLEX_CAR_DRIVE ----------------
    const auto  now = CTimer::GetTimeInMS();
    CVehicle* const veh = m_pClient->m_pVehicle;

    if (s_SexProcessSkipped == 1) {
        m_nLastSavedTime = now;
        m_nCurrentTimer  = 8000;
        bMoveCameraDown  = false;
        b08              = true;
        bVehicleShifted  = true;
        return m_pSubTask;
    }

    // Isn't the vehicle moving?
    const bool vehicleStill = ((veh->m_vecMoveSpeed * 50.0f).SquaredMagnitude() < 0.5625f); // `0.75f * 0.75f`
    if (!vehicleStill || b08) {
        b08              = false;
        m_nLastSavedTime = now;
    }

    CWanted* const wanted = FindPlayerWanted(-1);

    if (m_nNextTimeToCheckForSecludedPlace < now) {
        bPedsCanPotentiallySeeThis = false;
        bPedsCanSeeThis            = false;
        bCopsCanSeeThis            = false;

        m_nNextTimeToCheckForSecludedPlace = now + 1000;

        CEntity** const entities = ped->GetIntelligence()->GetPedEntities();
        for (auto i = 0u; i < 16u; i++) {
            const auto p = static_cast<CPed*>(entities[i]);
            if (!p || p == m_pClient || p->m_nPedType == PED_TYPE_PROSTITUTE) {
                continue;
            }

            const float dist2 = (p->GetPosition() - ped->GetPosition()).SquaredMagnitude();
            if (dist2 < 56.25f) { // `7.5f * 7.5f`
                bPedsCanSeeThis = true;
            }
            if (dist2 < 400.f) { // `20.f * 20.f`
                bPedsCanPotentiallySeeThis = true;
            }

            if (p->m_nPedType == PED_TYPE_COP && bSexProcessStarted && wanted && wanted->m_ChaosLevel < 1) {
                // Temporarily make the vehicle "not use collision" so the LOS check ignores it
                const bool usesCollision = m_pClient->m_pVehicle->m_bUsesCollision;
                m_pClient->m_pVehicle->m_bUsesCollision = false;
                const bool clear = CWorld::GetIsLineOfSightClear(
                    p->GetPosition(),
                    m_pClient->GetPosition(),
                    true, true, false, true, false, true, false
                );
                m_pClient->m_pVehicle->m_bUsesCollision = usesCollision;

                if (clear) {
                    bCopsCanSeeThis = true;
                }
            }
        }
    }

    if (bSearchingForSecludedPlace) {
        // Wait until the vehicle has been standing still for a while
        if (!vehicleStill || now - m_nLastSavedTime <= 4000) {
            return m_pSubTask;
        }
        if (bPedsCanPotentiallySeeThis) {
            if (!bSecludedPlaceMessageShown) {
                CMessages::AddMessageQ(TheText.Get("PROS_01"), 3000, 1, true);
                bSecludedPlaceMessageShown = true;
            }
            return m_pSubTask;
        }

        // Found a spot - Get to it!
        bSearchingForSecludedPlace = false;
        bSexProcessStarted         = true;
        m_nLastPaymentTime         = now;
        CMessages::AddMessageQ(TheText.Get("PROS_02"), 2000, 1, true);
        return m_pSubTask;
    }

    if (!bSexProcessStarted) {
        return m_pSubTask;
    }

    if (b07) {
        b07             = false;
        m_nCurrentTimer = 15000;
        CStats::IncrementStat(STAT_NUMBER_OF_PROSTITUTES_VISITED, 1.f);
    }

    const bool padActive = CPad::GetPad()->GetAccelerate() || CPad::GetPad()->GetBrake();
    const auto rnd       = static_cast<uint32>(rand());

    bool witnessedByCop = false;
    if (bCopsCanSeeThis && wanted && wanted->m_ChaosLevel < 1) {
        FindPlayerWanted(-1)->SetWantedLevel(eWantedLevel::WANTED_LEVEL_1);
        witnessedByCop = true;
    }

    if (padActive || bPedsCanSeeThis || witnessedByCop) {
        // The player stopped (Or someone's watching) - Look for a more secluded place
        m_nLastSavedTime = now;
        bSearchingForSecludedPlace = true;
        bSexProcessStarted         = false;
        bMoveCameraDown            = true;

        if (padActive) {
            if (rnd >= 0x1FFFFFFF && m_nCurrentTimer >= 3000) {
                m_nCurrentTimer = 8000;
                return m_pSubTask;
            }
            bTaskCanBeFinished = true;
            m_nCurrentTimer    = 0;
            CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true);
            return m_pSubTask;
        }

        if (m_nCurrentTimer < 3000) {
            bTaskCanBeFinished = true;
            m_nCurrentTimer    = 0;
            return m_pSubTask;
        }
        CMessages::AddMessageQ(TheText.Get("PROS_01"), 3000, 1, true);
        m_nCurrentTimer = 8000;
        return m_pSubTask;
    }

    // Keep going - Simulate the vehicle's movement
    if ((m_nVehicleMovementTimer -= delta) < 1) {
        float speed = CGeneral::GetRandomNumberInRange(-0.9f, -0.5f);

        if (m_nCurrentTimer > 10000) {
            m_nVehicleMovementTimer = 850;
        } else if (m_nCurrentTimer > 5000) {
            m_nVehicleMovementTimer = 450;
        } else if (m_nCurrentTimer > 1000) {
            m_nVehicleMovementTimer = 120;
        } else {
            speed *= 0.5f;
            m_nVehicleMovementTimer = 850;
            CPad::GetPad()->StartShake(1000, 0x78, 0);
        }

        const float mass = std::max(std::min(150.f, veh->m_fMass / 15.f), m_pClient->m_fMass);
        const auto  vehPos = veh->GetPosition();
        const auto& pos    = (rnd & 1) ? m_pClient->GetPosition() : ped->GetPosition();
        veh->ApplyTurnForce(
            { 0.f, 0.f, mass * speed },
            { pos.x - vehPos.x, pos.y - vehPos.y, 0.f }
        );
        veh->m_vehicleAudio.AddAudioEvent(AE_SUSPENSION_BOUNCE, 0.f);

        if (b10 && rnd > 0xFFFFFFF) {
            ped->Say((rnd & 0xFFFF) < 0xFF ? CTX_GLOBAL_GIVING_HEAD : CTX_GLOBAL_HAVING_SEX, 0, 0.5f);
        }
    }

    if ((m_nCurrentTimer -= delta) < 1) {
        m_nCurrentTimer    = 3000;
        bSexProcessStarted = false;
        bTaskCanBeFinished = true;
    }

    if (now - m_nLastPaymentTime > 1000) {
        m_nLastPaymentTime = now;

        CPlayerInfo* const playerInfo = m_pClient->AsPlayer()->GetPlayerInfoForThisPlayerPed();
        if (CCheat::IsActive(CHEAT_PROSTITUTES_PAY_YOU)) {
            playerInfo->m_nMoney += 2;
        } else if (playerInfo->m_nMoney >= 2) {
            playerInfo->m_nMoney -= 2;
            CStats::IncrementStat(STAT_PROSTITUTE_BUDGET, 2.f);
            ped->m_nMoneyCount++;
        } else {
            playerInfo->m_nMoney = 0;
            m_nCurrentTimer      = 0;
            bSexProcessStarted   = false;
            bTaskCanBeFinished   = true;
            CMessages::ClearMessages(false);
            CMessages::AddMessageQ(TheText.Get("PROS_06"), 2000, 1, true); // You've got money right?
            CMessages::AddMessageQ(TheText.Get("PROS_09"), 3000, 1, true); // Stop wasting my time!
        }

        if (!bPedsCanPotentiallySeeThis) {
            playerInfo->AddHealth(2);
        }
    }

    return m_pSubTask;
}
