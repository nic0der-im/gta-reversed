#include "StdInc.h"

#include "Remote.h"
#include "CarCtrl.h"

//! Index of the player that's currently controlling a remote-controlled vehicle.
//! @note The original only ever writes `0` to it (In `CGame::Init2`), but all of the RC code indexes the player array by it.
static inline auto& g_nRemoteSlot = StaticRef<uint8>(0xB7CD74);

// 0x45AE80
void CRemote::TakeRemoteControlledCarFromPlayer(bool bCreateRemoteVehicleExplosion)
{
    auto&           playerInfo = CWorld::Players[g_nRemoteSlot];
    CVehicle* const veh        = playerInfo.m_pRemoteVehicle;

    if (veh->m_nCreatedBy == MISSION_VEHICLE) {
        veh->SetVehicleCreatedBy(RANDOM_VEHICLE);

        // NOTE: The original recomputes the SCM handle by dividing the pointer difference by a hard-coded `0xA18`
        //       (Which is only correct for automobile-sized vehicles) - `GetRef` is the equivalent.
        CTheScripts::MissionCleanUp.RemoveEntityFromList(
            GetVehiclePool()->GetRef(veh),
            MISSION_CLEANUP_ENTITY_TYPE_VEHICLE
        );
    }

    veh->vehicleFlags.bIsLocked = false;

    // Arm the "remote vehicle explosion" state of the player
    playerInfo.m_nTimeOfRemoteVehicleExplosion    = CTimer::GetTimeInMS();
    playerInfo.m_bAfterRemoteVehicleExplosion     = true;
    playerInfo.m_bCreateRemoteVehicleExplosion    = bCreateRemoteVehicleExplosion;
    playerInfo.m_bFadeAfterRemoteVehicleExplosion = true;
}

// 0x45AB10
void CRemote::GivePlayerRemoteControlledCar(CVector pos, float rotation, int16 modelId)
{
    CVehicle* veh{};
    if (CModelInfo::IsHeliModel(modelId)) {
        veh = new CHeli(modelId, MISSION_VEHICLE);
    } else if (CModelInfo::IsPlaneModel(modelId)) {
        veh = new CPlane(modelId, MISSION_VEHICLE);
    } else {
        veh = new CAutomobile(modelId, MISSION_VEHICLE, true);
    }

    const auto groundZ    = CWorld::FindGroundZFor3DCoord({ pos.x, pos.y, pos.z + 2.0f });
    const auto baseOffset = veh->GetDistanceFromCentreOfMassToBaseOfModel();

    veh->GetMatrix().SetRotateZOnly(rotation);
    veh->SetPosn(pos.x, pos.y, groundZ + baseOffset);

    veh->SetStatus(STATUS_REMOTE_CONTROLLED);
    veh->vehicleFlags.bIsLocked = true;

    CCarCtrl::JoinCarWithRoadSystem(veh);

    // Must come after `JoinCarWithRoadSystem`, as that one sets some of these too
    veh->m_autoPilot.m_nCurrentLane     = 0;
    veh->m_autoPilot.m_nNextLane        = 0;
    veh->m_autoPilot.m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    veh->m_autoPilot.m_nCarMission      = MISSION_NONE;
    veh->m_autoPilot.m_nTempAction      = TEMPACT_NONE;
    veh->m_autoPilot.m_speed            = 9.0f;
    veh->m_autoPilot.m_nCruiseSpeed     = 9;

    veh->vehicleFlags.bEngineOn = !veh->vehicleFlags.bHasBeenOwnedByPlayer;

    CWorld::Add(veh);

    // The player's current vehicle is forced to stop (The RC one is now the one being driven)
    if (auto* const playerVehicle = FindPlayerVehicle(-1, false)) {
        playerVehicle->SetStatus(STATUS_FORCED_STOP);
    }

    // Remember the vehicle as the player's remote-controlled one
    CEntity::SetEntityReference(CWorld::Players[g_nRemoteSlot].m_pRemoteVehicle, veh);

    TheCamera.TakeControl(veh, MODE_CAM_ON_A_STRING, eSwitchType::INTERPOLATION, 1);
    TheCamera.SetZoomValueCamStringScript(1);
}
