/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "Bike.h"

#include "Buoyancy.h"
#include "VehicleRecording.h"

// 0xC1C804 - Unnamed: The signed square of the bike's raw steer angle, published by `CBike::ProcessControlInputs`
static auto& ms_fSteerAngleSquared = StaticRef<float>(0xC1C804);



void CBike::InjectHooks() {
    RH_ScopedVirtualClass(CBike, 0x871360, 67);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6BF430);
    RH_ScopedInstall(Destructor, 0x6B57A0);
    RH_ScopedInstall(dmgDrawCarCollidingParticles, 0x6B5A00);
    RH_ScopedInstall(DamageKnockOffRider, 0x6B5A10);
    RH_ScopedInstall(KnockOffRider, 0x6B5F40);
    RH_ScopedInstall(SetRemoveAnimFlags, 0x6B5F50);
    RH_ScopedInstall(ReduceHornCounter, 0x6B5F90);
    RH_ScopedVMTInstall(ProcessAI, 0x6BC930);
    RH_ScopedInstall(ProcessBuoyancy, 0x6B5FB0);
    RH_ScopedInstall(ResetSuspension, 0x6B6740);
    RH_ScopedInstall(GetAllWheelsOffGround, 0x6B6790);
    RH_ScopedInstall(DebugCode, 0x6B67A0);
    RH_ScopedInstall(DoSoftGroundResistance, 0x6B6D40);
    RH_ScopedInstall(PlayHornIfNecessary, 0x6B7130);
    RH_ScopedInstall(CalculateLeanMatrix, 0x6B7150);
    RH_ScopedInstall(ProcessRiderAnims, 0x6B7280, { .Reversed = false });
    RH_ScopedInstall(FixHandsToBars, 0x6B7F90, { .Reversed = false });
    RH_ScopedInstall(PlaceOnRoadProperly, 0x6BEEB0);
    RH_ScopedInstall(GetCorrectedWorldDoorPosition, 0x6BF230);
    RH_ScopedVMTInstall(Fix, 0x6B7050);
    RH_ScopedVMTInstall(BlowUpCar, 0x6BEA10);
    RH_ScopedVMTInstall(ProcessDrivingAnims, 0x6BF400);
    RH_ScopedVMTInstall(BurstTyre, 0x6BEB20);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6BE310);
    RH_ScopedVMTInstall(ProcessEntityCollision, 0x6BDEA0);
    RH_ScopedVMTInstall(Render, 0x6BDE20);
    RH_ScopedVMTInstall(PreRender, 0x6BD090, { .Reversed = false });
    RH_ScopedVMTInstall(Teleport, 0x6BCFC0);
    RH_ScopedVMTInstall(ProcessControl, 0x6B9250, { .Reversed = false });
    RH_ScopedVMTInstall(VehicleDamage, 0x6B8EC0);
    RH_ScopedVMTInstall(SetupSuspensionLines, 0x6B89B0);
    RH_ScopedVMTInstall(SetModelIndex, 0x6B8970);
    RH_ScopedVMTInstall(PlayCarHorn, 0x6B7080);
    RH_ScopedVMTInstall(SetupDamageAfterLoad, 0x6B7070);
    RH_ScopedVMTInstall(DoBurstAndSoftGroundRatios, 0x6B6950);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6B67E0);
    RH_ScopedVMTInstall(RemoveRefsToVehicle, 0x6B67B0);
    RH_ScopedVMTInstall(ProcessControlCollisionCheck, 0x6B6620);
    RH_ScopedVMTInstall(GetComponentWorldPosition, 0x6B5990);
    RH_ScopedVMTInstall(ProcessOpenDoor, 0x6B58D0);
}

// 0x6BF430
CBike::CBike(int32 modelIndex, eVehicleCreatedBy createdBy) : CVehicle(createdBy) {
    auto mi = CModelInfo::GetModelInfo(modelIndex)->AsVehicleModelInfoPtr();
    if (mi->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        const auto& animationStyle = CAnimManager::GetAnimBlocks()[mi->GetAnimFileIndex()].GroupId;
        m_RideAnimData.AnimGroup = animationStyle;
        if (animationStyle < ANIM_GROUP_BIKES || animationStyle > ANIM_GROUP_WAYFARER) {
            m_RideAnimData.AnimGroup = ANIM_GROUP_BIKES;
        }
    }

    m_nVehicleSubType = VEHICLE_TYPE_BIKE;
    m_nVehicleType = VEHICLE_TYPE_BIKE;

    m_BlowUpTimer = 0.0f;
    m_nBrakesOn = false;
    nBikeFlags = 0;
    SetModelIndex(modelIndex);

    m_pHandlingData = gHandlingDataMgr.GetVehiclePointer(mi->m_nHandlingId);
    m_BikeHandling = gHandlingDataMgr.GetBikeHandlingPointer(mi->m_nHandlingId);
    m_nHandlingFlagsIntValue = m_pHandlingData->m_nHandlingFlags;
    m_pFlyingHandlingData = gHandlingDataMgr.GetFlyingPointer(static_cast<uint8>(mi->m_nHandlingId));
    m_fBrakeCount = 20.0f;
    mi->ChooseVehicleColour(m_nPrimaryColor, m_nSecondaryColor, m_nTertiaryColor, m_nQuaternaryColor, 1);
    m_fSwingArmLength = 0.0f;
    m_fForkYOffset = 0.0f;
    m_fForkZOffset = 0.0f;
    m_nFixLeftHand = false;
    m_nFixRightHand = false;
    m_fSteerAngleTan = std::tan(DegreesToRadians(mi->m_fBikeSteerAngle));
    m_fMass = m_pHandlingData->m_fMass;
    m_fTurnMass = m_pHandlingData->m_fTurnMass;
    m_vecCentreOfMass = m_pHandlingData->m_vecCentreOfMass;
    m_vecCentreOfMass.z = 0.1f;
    m_fAirResistance = GetDefaultAirResistance();
    m_fElasticity = 0.05f;
    m_fBuoyancyConstant = m_pHandlingData->m_fBuoyancyConstant;
    m_fSteerAngle = 0.0f;
    m_GasPedal = 0.0f;
    m_BrakePedal = 0.0f;
    m_Damager = nullptr;
    m_pWhoInstalledBombOnMe = nullptr;
    m_GasPedalAudioRevs = 0.0f;
    m_fTyreTemp = 1.0f;
    m_fBrakingSlide = 0.0f;
    m_PrevSpeed = 0.0f;

    for (auto i = 0; i < 2; ++i) {
        m_nWheelStatus[i] = 0;
        m_aWheelSkidmarkType[i] = eSkidmarkType::DEFAULT;
        m_bWheelBloody[i] = false;
        m_bMoreSkidMarks[i] = false;
        m_aWheelPitchAngles[i] = 0.0f;
        m_aWheelAngularVelocity[i] = 0.0f;
        m_aWheelSuspensionHeights[i] = 0.0f;
        m_aWheelOrigHeights[i] = 0.0f;
        m_WheelStates[i] = WHEEL_STATE_NORMAL;
    }

    for (auto i = 0; i < 4; ++i) {
        m_aWheelColPoints[i] = {};
        m_aWheelRatios[i] = 1.0f;
        m_aRatioHistory[i] = 0.0f;
        m_WheelCounts[i] = 0.0f;
        m_fSuspensionLength[i] = 0.0f;
        m_fLineLength[i] = 0.0f;
        m_aGroundPhysicalPtrs[i] = nullptr;
        m_aGroundOffsets[i] = CVector{};
    }

    m_nNoOfContactWheels = 0;
    m_NumDriveWheelsOnGround = 0;
    m_NumDriveWheelsOnGroundLastFrame = 0;
    m_fHeightAboveRoad = 0.0f;
    m_fExtraTractionMult = 1.0f;

    if (!mi->m_pColModel->m_pColData->m_pLines) {
        mi->m_pColModel->m_pColData->m_nNumLines = 4;
        mi->m_pColModel->m_pColData->m_pLines = static_cast<CColLine*>(CMemoryMgr::Malloc(4 * sizeof(CColLine)));
        mi->m_pColModel->m_pColData->m_pLines[1].m_vecStart.x = 99'999.99f; // todo: explain this
    }
    mi->m_pColModel->m_pColData->m_pLines[0].m_vecStart.z = 99'999.99f;
    CBike::SetupSuspensionLines();

    m_autoPilot.m_nTempAction = TEMPACT_NONE;
    m_autoPilot.SetCarMission(MISSION_NONE, 0);
    m_autoPilot.carCtrlFlags.bAvoidLevelTransitions = false;

    SetStatus(STATUS_SIMPLE);
    m_nNumPassengers = 0;
    vehicleFlags.bLowVehicle = false;
    vehicleFlags.bIsBig = false;
    vehicleFlags.bIsVan = false;

    m_bLeanMatrixCalculated = false;
    m_mLeanMatrix = *m_matrix;
    m_vecOldSpeedForPlayback = CVector{};
    m_vehicleAudio.Initialise(this);
}

// 0x6B57A0
CBike::~CBike() {
    m_vehicleAudio.Terminate();
}

// 0x6B5A00
void CBike::dmgDrawCarCollidingParticles(const CVector& position, float power, eWeaponType weaponType) {
    // NOP
}

// 0x6B5A10
bool CBike::DamageKnockOffRider(CVehicle* vehicle, float damageIntensity, uint16 pieceType, CEntity* damager, const CVector& collisionPos, const CVector& collisionImpactVelocity) {
    const auto driver    = vehicle->m_pDriver;
    const auto passenger = vehicle->m_apPassengers[0];

    // Impact force relative to the bike's mass
    auto force = damageIntensity / vehicle->m_fMass * 800.0f;

    // A skilled rider resists being knocked off (unless flagged to always come off)
    if (vehicle->GetStatus() != STATUS_PLAYER) {
        if (driver && driver->CantBeKnockedOffBike != CANT_BE_KNOCKED_OFF_ALWAYS_NORMAL) {
            force *= 1.0f - driver->GetBikeRidingSkill() * 0.6f;
        }
    } else {
        force *= 0.75f;
        if (driver) {
            force *= 1.0f - driver->GetBikeRidingSkill() * 0.5f;
        }
    }

    // Only an actual driver gets knocked off
    if (!driver || !driver->IsStateDriving() || force <= 10.0f) {
        return false;
    }

    // A ped already reacting to a hit isn't also knocked off (cops are exempt)
    if (const auto task = driver->GetIntelligence()->GetTaskManager().GetActiveTask()) {
        if (task->GetTaskType() == TASK_SIMPLE_BE_HIT && !driver->IsCop()) {
            return false;
        }
    }

    const auto impactFwdMag   = vehicle->GetForward().Dot(collisionImpactVelocity);
    const auto impactUpMag    = vehicle->GetUp().Dot(collisionImpactVelocity);
    const auto impactRightMag = vehicle->GetRight().Dot(collisionImpactVelocity);

    // Per-axis weighting of the impact
    auto fwdWeight = 0.6f;
    if (std::abs(impactFwdMag) > 0.85f) {
        const auto vertical = collisionImpactVelocity.z < 0.85f ? 0.0f : collisionImpactVelocity.z;
        fwdWeight = 7.0f * sq(vertical) + 0.6f;
    }
    if (vehicle->GetUp().z < 0.0f) { // bike lying on its side / upside down
        fwdWeight = 5.0f;
    }

    auto backWeight = 1.5f;
    auto upWeight   = 0.05f;
    if (vehicle->m_nModelIndex == MODEL_SANCHEZ) {
        fwdWeight *= 0.65f;
        upWeight  *= 0.75f;
    } else if (vehicle->IsSubQuad()) {
        backWeight = 3.0f;
        fwdWeight *= 0.65f;
        upWeight  *= 0.75f;
    }

    if (impactFwdMag > 0.0f) {
        fwdWeight *= 1.0f - driver->GetBikeRidingSkill() * 0.6f;
    }

    force *= std::abs(impactFwdMag) * fwdWeight
           + std::max(impactUpMag, 0.0f) * upWeight
           + std::abs(impactRightMag) * 0.45f
           - std::min(impactUpMag, 0.0f) * backWeight;

    // Don't knock the player off while they're on stairs
    if (driver->IsPlayer() && CCullZones::CamStairsForPlayer() && CCullZones::FindZoneWithStairsAttributeForPlayer()) {
        force = 0.0f;
    }

    // ALWAYS_HARD peds come off at a much lower force threshold
    if (force <= (driver->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_ALWAYS_HARD ? 20.0f : 75.0f)) {
        return false;
    }

    // NEVER peds are never knocked off
    if (driver->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_NEVER) {
        return false;
    }
    if (passenger && passenger->CantBeKnockedOffBike == CANT_BE_KNOCKED_OFF_NEVER) {
        return false;
    }

    // The driver (guaranteed present here) is thrown off, and so is the passenger, both reacting with the driver's facing
    const auto knockOffDir = (uint8)driver->GetLocalDirection(-CVector2D{ collisionImpactVelocity });

    driver->GetEventGroup().Add(CEventKnockOffBike{
        vehicle, vehicle->m_vecMoveSpeed, collisionImpactVelocity, damageIntensity, 0.05f * force, KNOCK_OFF_TYPE_SKIDBACKFRONT, knockOffDir, 0, nullptr, true, false
    });
    if (passenger) {
        passenger->GetEventGroup().Add(CEventKnockOffBike{
            vehicle, vehicle->m_vecMoveSpeed, collisionImpactVelocity, damageIntensity, 0.05f * force, KNOCK_OFF_TYPE_SKIDBACKFRONT, knockOffDir, 0, nullptr, false, false
        });
    }
    return true;
}

// dummy function
// 0x6B5F40
CPed* CBike::KnockOffRider(eWeaponType arg0, uint8 arg1, CPed* ped, bool arg3) {
    return ped;
}

// 0x6B5F50
void CBike::SetRemoveAnimFlags(CPed* ped) {
    if (ped->GetType() != ENTITY_TYPE_PED) {
        return;
    }

    for (auto* assoc = RpAnimBlendClumpGetFirstAssociation(ped->GetRpClump(), ANIMATION_SECONDARY_TASK_ANIM);
         assoc;
         assoc = RpAnimBlendGetNextAssociation(assoc, ANIMATION_SECONDARY_TASK_ANIM))
    {
        assoc->m_Flags |= ANIMATION_IS_BLEND_AUTO_REMOVE;
    }
}

// 0x6B5F90
void CBike::ReduceHornCounter() {
    if (m_HornCounter)
        m_HornCounter -= 1;
}

// 0x6B5FB0
void CBike::ProcessBuoyancy() {
    CVector vecBuoyancyTurnPoint;
    CVector vecBuoyancyForce;
    if (!mod_Buoyancy.ProcessBuoyancy(this, m_fBuoyancyConstant, &vecBuoyancyTurnPoint, &vecBuoyancyForce)) {
        vehicleFlags.bIsDrowning = false;
        physicalFlags.bSubmergedInWater = false;
        physicalFlags.bTouchingWater = false;
        return;
    }

    physicalFlags.bTouchingWater = true;
    ApplyMoveForce(vecBuoyancyForce);
    ApplyTurnForce(vecBuoyancyForce, vecBuoyancyTurnPoint);

    auto fTimeStep = std::max(0.01F, CTimer::GetTimeStep());
    auto fUsedMass = m_fMass / 125.0F;
    auto fBuoyancyForceZ = vecBuoyancyForce.z / (fTimeStep * fUsedMass);

    if (fUsedMass > m_fBuoyancyConstant)
        fBuoyancyForceZ *= 1.05F * fUsedMass / m_fBuoyancyConstant;

    if (physicalFlags.bMakeMassTwiceAsBig)
        fBuoyancyForceZ *= 1.5F;

    auto fBuoyancyForceMult = std::max(0.5F, 1.0F - fBuoyancyForceZ / 20.0F);
    auto fSpeedMult = std::pow(fBuoyancyForceMult, CTimer::GetTimeStep());
    m_vecMoveSpeed *= fSpeedMult;
    m_vecTurnSpeed *= fSpeedMult;

    // 0x6B6443
    if (fBuoyancyForceZ > 0.8F || (fBuoyancyForceZ > 0.4F && IsAnyWheelNotMakingContactWithGround())) {
        vehicleFlags.bIsDrowning = true;
        physicalFlags.bSubmergedInWater = true;

        m_vecMoveSpeed.z = std::max(-0.1F, m_vecMoveSpeed.z);

        if (m_pDriver) {
            ProcessPedInVehicleBuoyancy(m_pDriver->AsPed(), true);
        }
        else {
            vehicleFlags.bEngineOn = false;
        }

        for (const auto passenger : GetPassengers()) {
            ProcessPedInVehicleBuoyancy(passenger, false);
        }
    }
    else {
        vehicleFlags.bIsDrowning = false;
        physicalFlags.bSubmergedInWater = false;
    }
}

inline void CBike::ProcessPedInVehicleBuoyancy(CPed* ped, bool bIsDriver) {
    if (!ped)
        return;

    ped->physicalFlags.bTouchingWater = true;
    if (!ped->IsPlayer() && bikeFlags.bWaterTight)
        return;

    if (ped->IsPlayer())
        ped->AsPlayer()->HandlePlayerBreath(true, 1.0F);

    if (IsAnyWheelMakingContactWithGround()) {
        if (!ped->IsPlayer()) {
            auto pedDamageResponseCalc = CPedDamageResponseCalculator(this, CTimer::GetTimeStep(), eWeaponType::WEAPON_DROWNING, PED_PIECE_TORSO, false);
            auto damageEvent = CEventDamage(this, CTimer::GetTimeInMS(), eWeaponType::WEAPON_DROWNING, PED_PIECE_TORSO, 0, false, true);
            if (damageEvent.AffectsPed(ped))
                pedDamageResponseCalc.ComputeDamageResponse(ped, damageEvent.m_damageResponse, true);
            else
                damageEvent.m_damageResponse.m_bDamageCalculated = true;

            ped->GetEventGroup().Add(&damageEvent, false);
        }
    } else {
        auto knockOffBikeEvent = CEventKnockOffBike(this, m_vecMoveSpeed, m_vecLastCollisionImpactVelocity, m_fDamageIntensity, 0.0F, KNOCK_OFF_TYPE_FALL, 0, 0, nullptr, bIsDriver, false);
        ped->GetEventGroup().Add(&knockOffBikeEvent);
        if (bIsDriver) {
            vehicleFlags.bEngineOn = false;
        }
    }
}

// 0x6BC930
bool CBike::ProcessAI(uint32& extraHandlingFlags) {
    const auto& mi = *GetVehicleModelInfo();

    m_autoPilot.m_nMovementFlags &= ~(0x1 | 0x2); // bIsStopped | bIsParked

    if (m_autoPilot.m_vehicleRecordingId >= 0 && !CVehicleRecording::bUseCarAI[m_autoPilot.m_vehicleRecordingId]) {
        extraHandlingFlags += 2;
        return false;
    }

    switch (GetStatus()) {
    case STATUS_PLAYER: {
        extraHandlingFlags += 2;
        bikeFlags.bGettingPickedUp = false;

        if (const auto playerPed = FindPlayerPed();
            playerPed->m_nPedState != PEDSTATE_DRAGGED_FROM_CAR && playerPed->m_nPedState != PEDSTATE_EXIT_CAR
        ) {
            if (m_pDriver) {
                if (m_pDriver == CWorld::Players[0].m_pPed) {
                    ProcessControlInputs(0);
                } else if (m_pDriver == CWorld::Players[1].m_pPed) {
                    ProcessControlInputs(1);
                }
            }

            //! Steering turn force - Its direction/magnitude depends on which way the rider is leaning
            const auto ApplyLeanForce = [&, this](float leanForce, float pedal) {
                const auto force = leanForce
                                 * m_fTurnMass
                                 * m_RideAnimData.LeanFwd
                                 * (pedal + std::max(std::min(0.1f, GetMoveSpeed().Magnitude()) / 0.1f, pedal))
                                 * 0.5f
                                 * CStats::GetFatAndMuscleModifier(STAT_MOD_11);

                ApplyTurnForce(GetUp() * -(CTimer::GetTimeStep() * force), m_vecCentreOfMass + GetForward());
            };

            if (m_RideAnimData.LeanFwd <= 0.0f) {
                m_vecCentreOfMass.y = m_BikeHandling->m_fLeanBakCOM * m_RideAnimData.LeanFwd + m_pHandlingData->m_fTractionMultiplier;

                const bool brakeOrHandbrake = m_BrakePedal != 0.0f || vehicleFlags.bIsHandbrakeOn;
                if (!brakeOrHandbrake || m_nNoOfContactWheels == 0) {
                    ApplyLeanForce(m_BikeHandling->m_fLeanBakForce, m_GasPedal);
                }
            } else {
                m_vecCentreOfMass.y = m_BikeHandling->m_fLeanFwdCOM * m_RideAnimData.LeanFwd + m_pHandlingData->m_fTractionMultiplier;

                if (m_BrakePedal < 0.0f || m_nNoOfContactWheels == 0) {
                    ApplyLeanForce(m_BikeHandling->m_fLeanFwdForce, m_BrakePedal);
                }
            }
        }

        PruneReferences();
        if (GetStatus() == STATUS_PLAYER) {
            DoDriveByShootings();
        }
        DoSoftGroundResistance(extraHandlingFlags);

        if (CPad::GetPad(0)->CarGunJustDown()) {
            ActivateBomb();
        }
        return true;
    }
    case STATUS_PLAYER_PLAYBACK_FROM_BUFFER: {
        extraHandlingFlags += 2;
        return true;
    }
    case STATUS_SIMPLE: {
        CCarAI::UpdateCarAI(this);
        CPhysical::ProcessControl();
        CCarCtrl::UpdateCarOnRails(this);

        m_NumDriveWheelsOnGroundLastFrame = m_NumDriveWheelsOnGround;
        m_NumDriveWheelsOnGround           = 2;
        m_nNoOfContactWheels               = 2;

        m_pHandlingData->GetTransmission().CalculateGearForSimpleCar(m_autoPilot.m_speed * 0.02f, m_nCurrentGear);

        m_aWheelPitchAngles[0] += CVehicle::ProcessWheelRotation(WHEEL_STATE_NORMAL, GetForward(), GetMoveSpeed(), mi.m_fWheelSizeFront * 0.5f) * CTimer::GetTimeStep();
        m_aWheelPitchAngles[1] += CVehicle::ProcessWheelRotation(WHEEL_STATE_NORMAL, GetForward(), GetMoveSpeed(), mi.m_fWheelSizeRear * 0.5f) * CTimer::GetTimeStep();

        if (m_autoPilot.movementFlags.bIsStopped || m_autoPilot.movementFlags.bIsParked) {
            PlayCarHorn();
        }

        if (m_nVehicleWeaponInUse) {
            m_nVehicleWeaponInUse = (eCarWeapon)((int32)m_nVehicleWeaponInUse - 1);
        }

        nBikeFlags &= ~0x80; // bWheelieForCamera
        vehicleFlags.bVehicleColProcessed = false;
        vehicleFlags.bAudioChangingGear   = false;
        return true;
    }
    case STATUS_PHYSICS:
    case STATUS_GHOST: { // 0xC (Not a real status, but whatever - The game handles it here)
        CCarAI::UpdateCarAI(this);
        CCarCtrl::SteerAICarWithPhysics(this);

        if (m_autoPilot.movementFlags.bIsStopped || m_autoPilot.movementFlags.bIsParked) {
            PlayCarHorn();
        }

        extraHandlingFlags += 2;

        nBikeFlags &= ~0x80; // bWheelieForCamera
        if (vehicleFlags.bIsBeingCarJacked) {
            vehicleFlags.bIsHandbrakeOn = true;
            m_GasPedal   = 0.0f;
            m_BrakePedal = 1.0f;
            return true;
        }
        bikeFlags.bGettingPickedUp = false;
        return true;
    }
    case STATUS_ABANDONED: {
        m_BrakePedal = 0.0f;

        const auto isStoppedOrLeaning = GetMoveSpeed().SquaredMagnitude() < 0.01f || bikeFlags.bOnSideStand;
        vehicleFlags.bIsHandbrakeOn = isStoppedOrLeaning;

        m_GasPedal             = 0.0f;
        m_nVehicleWeaponInUse  = CAR_WEAPON_NOT_USED;

        if ((m_pDriver || m_apPassengers[0] || vehicleFlags.bIsBeingCarJacked) && !bikeFlags.bOnSideStand) {
            extraHandlingFlags += 2;
        }

        nBikeFlags &= ~0x80; // bWheelieForCamera
        m_RideAnimData.AnimLeanLeft = 0.0f;
        m_RideAnimData.AnimLeanFwd  = 0.0f;

        if (vehicleFlags.bIsBeingCarJacked) {
            vehicleFlags.bIsHandbrakeOn = true;
            m_GasPedal   = 0.0f;
            m_BrakePedal = 1.0f;
        }
        return true;
    }
    case STATUS_WRECKED: {
        m_BrakePedal                = 0.05f;
        vehicleFlags.bIsHandbrakeOn = true;
        m_fSteerAngle               = 0.0f;
        m_GasPedal                  = 0.0f;
        m_nVehicleWeaponInUse       = CAR_WEAPON_NOT_USED;
        nBikeFlags &= ~0x80; // bWheelieForCamera
        m_RideAnimData.AnimLeanLeft = 0.0f;
        m_RideAnimData.AnimLeanFwd  = 0.0f;
        return true;
    }
    case STATUS_FORCED_STOP: {
        if (GetMoveSpeed().SquaredMagnitude() < 0.01f) {
            vehicleFlags.bIsHandbrakeOn = true;
            m_BrakePedal                = 1.0f;
        } else {
            vehicleFlags.bIsHandbrakeOn = false;
            m_BrakePedal                = 0.0f;
        }
        m_fSteerAngle         = 0.0f;
        m_GasPedal            = 0.0f;
        m_nVehicleWeaponInUse = CAR_WEAPON_NOT_USED;
        extraHandlingFlags += 2;
        nBikeFlags &= ~0x80; // bWheelieForCamera
        return true;
    }
    }
    return true;
}

// 0x6BF400
void CBike::ProcessDrivingAnims(CPed* driver, bool blend) {
    if (m_bOffscreen && GetStatus() == STATUS_PLAYER)
        return;

    ProcessRiderAnims(driver, this, &m_RideAnimData, m_BikeHandling, 0);
}

// 0x6B7280
void CBike::ProcessRiderAnims(CPed* rider, CVehicle* vehicle, CRideAnimData* rideData, tBikeHandlingData* handling, int16 a5) {
    plugin::Call<0x6B7280, CPed*, CVehicle*, CRideAnimData*, tBikeHandlingData*, int16>(rider, vehicle, rideData, handling, a5);
}

// 0x6BEB20
bool CBike::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    if (vehicleFlags.bTyresDontBurst || physicalFlags.bRenderScorched) {
        return false;
    }

    switch (tyreComponentId) {
    case CAR_PIECE_WHEEL_LF: tyreComponentId = 0; break; // Front wheel of a 2-wheeled bike
    case CAR_PIECE_WHEEL_RL: tyreComponentId = 1; break; // Rear wheel of a 2-wheeled bike
    }

    if (m_nWheelStatus[tyreComponentId] != 0) {
        return false; // Already burst
    }
    m_nWheelStatus[tyreComponentId] = 1;

    m_vehicleAudio.AddAudioEvent(AE_TYRE_BURST, 0.0f);
    if (GetStatus() == STATUS_SIMPLE) {
        CCarCtrl::SwitchVehicleToRealPhysics(this);
    }

    if (bPhysicalEffect) {
        constexpr auto force = 0.02f;
        ApplyMoveForce(GetRight() * CGeneral::GetRandomNumberInRange(-force, force) * m_fMass);
        ApplyTurnForce(
            GetForward() * CGeneral::GetRandomNumberInRange(-force, force) * m_fTurnMass,
            GetForward()
        );
    }

    // NOTE: The original then had a rider knock-off block gated on `tyreComponentId == 0x0D`/`0x0E`,
    //       which is unreachable, because `tyreComponentId` was already remapped to 0/1 above.
    return true;
}

// 0x6BE310
void CBike::ProcessControlInputs(uint8 playerNum) {
    const auto pad      = CPad::GetPad(playerNum);
    const auto fwdSpeed = DotProduct(GetMoveSpeed(), GetForward());

    //> Handbrake
    if (pad->GetExitVehicle()) {
        vehicleFlags.bIsHandbrakeOn = true;
    } else {
        vehicleFlags.bIsHandbrakeOn = pad->GetHandBrake() != 0;
    }

    const auto keyboardSteering = [&, this] {
        const auto k = CTimer::GetTimeStep() * 0.2f;
        m_fRawSteerAngle += (-(float)pad->GetSteeringLeftRight() * 0.0078125f - m_fRawSteerAngle) * k;
        m_RideAnimData.LeanFwd += (-(float)pad->GetSteeringUpDown() * 0.0078125f - m_RideAnimData.LeanFwd) * k;
    };

    //> Update the steer angle
    bool doMouseSteering{};
    if (!CCamera::m_bUseMouse3rdPerson || !m_bEnableMouseSteering) {
        keyboardSteering();
    } else if (CPad::NewMouseControllerState.m_AmountMoved.x != 0.0f || CPad::NewMouseControllerState.m_AmountMoved.y != 0.0f) {
        doMouseSteering = true;
    } else if (   m_nLastControlInput == eControllerType::MOUSE
               && std::abs(m_fRawSteerAngle) > 0.0f
               && pad->GetSteeringLeftRight() == 0
               && pad->GetSteeringUpDown() == 0
    ) {
        doMouseSteering = true;
    } else if (   pad->GetSteeringLeftRight() != 0
               || pad->GetSteeringUpDown() != 0
               || m_nLastControlInput != eControllerType::MOUSE
    ) {
        m_nLastControlInput = eControllerType::KEYBOARD;
        keyboardSteering();
    }
    // (Else: Simply keep the current steer values as-is)

    if (doMouseSteering) {
        m_nLastControlInput = eControllerType::MOUSE;

        if (pad->NewState.m_bVehicleMouseLook == 0) {
            m_fRawSteerAngle      += CPad::NewMouseControllerState.m_AmountMoved.x * -0.0035f;
            m_RideAnimData.LeanFwd += CPad::NewMouseControllerState.m_AmountMoved.y * -0.0035f;
        }

        // Slowly steer back to 0
        const auto decay = std::pow(0.975f, CTimer::GetTimeStep());
        if (std::abs(m_fRawSteerAngle) < 0.35f || pad->NewState.m_bVehicleMouseLook != 0) {
            m_fRawSteerAngle *= decay;
        }
        if (std::abs(m_RideAnimData.LeanFwd) < 0.35f || pad->NewState.m_bVehicleMouseLook != 0) {
            m_RideAnimData.LeanFwd *= decay;
        }
    }

    m_fRawSteerAngle       = std::clamp(m_fRawSteerAngle, -1.0f, 1.0f);
    m_RideAnimData.LeanFwd = std::clamp(m_RideAnimData.LeanFwd, -1.0f, 1.0f);

    //> Pedals
    const auto pedalInput = float(int32(pad->GetAccelerate()) - int32(pad->GetBrake())) * 0.003921569f; // 1/255
    bool pedalsApplied = false;
    if (std::abs(fwdSpeed) >= 0.01f) {
        if (fwdSpeed < 0.0f) {
            if (pedalInput >= 0.0f) {
                m_BrakePedal = pedalInput;
                m_GasPedal   = 0.0f;
                pedalsApplied = true;
            } else {
                m_GasPedal = pedalInput;
            }
        } else {
            if (pedalInput < 0.0f) {
                m_GasPedal   = 0.0f;
                m_BrakePedal = -pedalInput;
                pedalsApplied = true;
            } else {
                m_GasPedal = pedalInput;
            }
        }
    } else {
        // Standing still - Both pedals pressed at once starts a burnout
        if (pad->GetAccelerate() > 150 && pad->GetBrake() > 150 && m_nVehicleSubType != VEHICLE_TYPE_BMX) {
            m_GasPedal   = (float)pad->GetAccelerate() * 0.003921569f;
            m_nBrakesOn  = 1;
            m_BrakePedal = (float)pad->GetBrake() * 0.003921569f;
            pedalsApplied = true;
        } else {
            m_GasPedal = pedalInput;
        }
    }
    if (!pedalsApplied) {
        m_BrakePedal = 0.0f;
    }

    //> Publish the (signed) squared steer angle (Used by the lean code elsewhere)
    const auto steerSq = std::copysignf(sq(m_fRawSteerAngle), m_fRawSteerAngle);
    ms_fSteerAngleSquared = steerSq;

    if (m_autoPilot.m_vehicleRecordingId < 0 || CVehicleRecording::bUseCarAI[m_autoPilot.m_vehicleRecordingId]) {
        m_fSteerAngle = m_pHandlingData->m_fSteeringLock * DegreesToRadians(1.0f) * steerSq;
    }

    //> Comedy controls (The bike wiggles around)
    if (vehicleFlags.bComedyControls) {
        if ((CTimer::GetTimeInMS() & 0x3C00) < 0x3000) {
            m_GasPedal = 1.0f;
        }
        if ((((CTimer::GetTimeInMS() >> 10) + 6) & 0xF) < 0xC) {
            m_BrakePedal = 0.0f;
        }
        vehicleFlags.bIsHandbrakeOn = false;
        m_fSteerAngle += (CTimer::GetTimeInMS() & 0x800) ? 0.08f : -0.03f;
    }

    //> Slow the bike down if the player can't control it
    if (const auto pad0 = CPad::GetPad(0); pad0->DisablePlayerControls && CGameLogic::SkipState != 2) {
        m_BrakePedal                = 1.0f;
        vehicleFlags.bIsHandbrakeOn = true;
        m_GasPedal                  = 0.0f;

        FindPlayerPed()->KeepAreaAroundPlayerClear();

        const auto speed = GetMoveSpeed().Magnitude();
        if (speed > 0.28f) {
            GetMoveSpeed() *= 0.28f / speed;
        }
    }
}

// 0x6BDEA0
int32 CBike::ProcessEntityCollision(CEntity* entity, CColPoint* outColPoints) {
    if (GetStatus() != STATUS_SIMPLE) {
        vehicleFlags.bVehicleColProcessed = true;
    }

    const auto tcd = GetColData(),
               ocd = entity->GetColData();

#ifdef FIX_BUGS // Text search for `FIX_BUGS@CAutomobile::ProcessEntityCollision:1`
    if (!tcd || !ocd) {
        return 0;
    }
#endif

    if (physicalFlags.bSkipLineCol || physicalFlags.bProcessingShift || entity->GetIsTypePed()) {
        tcd->m_nNumLines = 0; // Later reset back to original value
    }

    const auto ogWheelRatios = m_aWheelRatios;

    auto numColPts = CCollision::ProcessColModels(
        GetMatrix(), *GetColModel(),
        entity->GetMatrix(), *entity->GetColModel(),
        *(std::array<CColPoint, 32>*)(outColPoints),
        m_aWheelColPoints.data(),
        m_aWheelRatios.data(),
        false
    );

    // Possibly add driver & entity collisions to `outColPoints`
    if (m_pDriver && m_nTestPedCollision) {
        const auto pcd = m_pDriver->GetColData();
        if (!pcd->m_nNumLines) {
            std::array<CColPoint, 32> pedCPs{};

            CMatrix driverMat = GetMatrix();
            driverMat.GetPosition() += GetDriverSeatDummyPositionWS();

            std::array<CColPoint, 32> pedEntityColPts{};
            const auto numPedEntityColPts = CCollision::ProcessColModels(
                driverMat, *m_pDriver->GetColModel(),
                entity->GetMatrix(), *entity->GetColModel(),
                pedEntityColPts,
                nullptr,
                nullptr,
                false
            );

            if (numPedEntityColPts) {
                if (m_nTestPedCollision == 1) {
                    m_nTestPedCollision = 0;
                } else {
                    for (auto i = 0; i < numPedEntityColPts && numColPts < 32; i++) {
                        const auto& pedEntityCP = pedCPs[i];
                        if (pedEntityCP.m_nPieceTypeA == PED_COL_SPHERE_LEG) {
                            continue;
                        }
                        outColPoints[numColPts++] = pedEntityCP;
                    }
                }
            }
        }
    }
    
    size_t numProcessedLines{};
    if (tcd->m_nNumLines) {
        // Process the real wheels
        for (auto i = 0; i < NUM_SUSP_LINES; i++) {
            const auto& cp = m_aWheelColPoints[i];

            const auto wheelColPtsTouchDist = m_aWheelRatios[i];
            if (wheelColPtsTouchDist >= 1.f || wheelColPtsTouchDist >= ogWheelRatios[i]) {
                continue;
            }

            numProcessedLines++;

            m_anCollisionLighting[i] = cp.m_nLightingB;
            m_nContactSurface = cp.m_nSurfaceTypeB;

            switch (entity->GetType()) {
            case ENTITY_TYPE_VEHICLE:
            case ENTITY_TYPE_OBJECT: {
                CEntity::ChangeEntityReference(m_aGroundPhysicalPtrs[i], entity->AsPhysical());

                m_aGroundOffsets[i] = cp.m_vecPoint - entity->GetPosition();
                if (entity->GetIsTypeVehicle()) {
                    m_anCollisionLighting[i] = entity->AsVehicle()->m_anCollisionLighting[i];
                }
                break;
            }
            case ENTITY_TYPE_BUILDING: {
                m_pEntityWeAreOn = entity;
                m_bTunnel = entity->m_bTunnel;
                m_bTunnelTransition = entity->m_bTunnelTransition;
                break;
            }
            }
        }
    } else {
        tcd->m_nNumLines = NUM_SUSP_LINES;
    }

    if (numColPts > 0 || numProcessedLines > 0) {
        AddCollisionRecord(entity);
        if (!entity->GetIsTypeBuilding()) {
            entity->AsPhysical()->AddCollisionRecord(this);
        }
        if (numColPts > 0) {
            if (   entity->GetIsTypeBuilding()
                || (entity->GetIsTypeObject() && entity->AsPhysical()->physicalFlags.bDisableCollisionForce)
            ) {
                SetHasHitWall(true);
            }
        }
    }

    return numColPts;
}

// 0x6B9250
void CBike::ProcessControl() {
    plugin::CallMethod<0x6B9250, CBike*>(this);
}

// 0x6B6740
void CBike::ResetSuspension() {
    for (auto i = 0u; i < 2; i++) {
        m_aWheelPitchAngles[i] = 0.0f;
        m_WheelStates[i]       = WHEEL_STATE_NORMAL;
    }
    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        m_aWheelRatios[i] = 1.0f;
        m_WheelCounts[i]  = 0.0f;
    }
}

// 0x6B6790
bool CBike::GetAllWheelsOffGround() const {
    return m_nNoOfContactWheels == 0;
}

// 0x6B67A0
void CBike::DebugCode() {
    // NOP
}

// 0x6B6D40
void CBike::DoSoftGroundResistance(uint32& arg0) {
    // A wheel is "digging in" if it's compressed and standing on the given kind of surface
    const auto anyWheelOn = [this](auto&& surfaceTest) {
        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            if (m_aWheelRatios[i] < 1.0f && surfaceTest(m_aWheelColPoints[i].m_nSurfaceTypeB)) {
                return true;
            }
        }
        return false;
    };

    const bool isOnSand  = anyWheelOn([](eSurfaceType s) { return g_surfaceInfos.GetAdhesionGroup(s) == ADHESION_GROUP_SAND; });
    const bool isOnRails = !isOnSand && anyWheelOn([](eSurfaceType s) { return s == SURFACE_RAILTRACK; });
    if (!isOnSand && !isOnRails) {
        return;
    }

    // Remove the velocity component along the up axis
    const auto& up   = GetUp();
    auto&       move = GetMoveSpeed();
    auto        vel  = move - up * DotProduct(move, up);

    if (isOnSand) {
        if (m_GasPedal > 0.3f) {
            if (vel.SquaredMagnitude() < 0.09f) {
                arg0 += 4; // NOTE: `+=` (Not `|=`) as in the original
            }
            vel -= GetForward() * DotProduct(vel, GetForward());
        }
        ApplyMoveForce(vel * -(CTimer::GetTimeStep() * m_fMass * 0.02f));
    } else {
        ApplyMoveForce(vel * -(CTimer::GetTimeStep() * m_fMass * ms_fRailTrackResistance));
    }
}

// 0x6B7130
void CBike::PlayHornIfNecessary() {
    if (m_autoPilot.carCtrlFlags.bHonkAtCar || m_autoPilot.carCtrlFlags.bHonkAtPed)
        PlayCarHorn();
}

// 0x6B7150
void CBike::CalculateLeanMatrix() {
    if (m_bLeanMatrixCalculated)
        return;

    CMatrix mat;
    mat.SetRotateX(fabs(m_RideAnimData.LeanAngle) * -0.05f);
    mat.RotateY(m_RideAnimData.LeanAngle);
    m_mLeanMatrix = GetMatrix();
    m_mLeanMatrix = m_mLeanMatrix * mat;
    // place wheel back on ground
    m_mLeanMatrix.GetPosition() += GetUp() * (1.0f - cos(m_RideAnimData.LeanAngle)) * GetColModel()->GetBoundingBox().m_vecMin.z;
    m_bLeanMatrixCalculated = true;
}

// 0x6B7F90
void CBike::FixHandsToBars(CPed* rider) {
    plugin::CallMethod<0x6B7F90, CBike*, CPed*>(this, rider);
}

// 0x6BEEB0
void CBike::PlaceOnRoadProperly() {
    const auto& colModel = *GetColModel();

    const auto pos        = GetPosition();
    const auto fwd        = GetForward();
    const auto halfLengthF = -colModel.m_boundBox.m_vecMin.y; // Front of the vehicle
    const auto halfLengthR =  colModel.m_boundBox.m_vecMax.y; // Rear of the vehicle

    const auto rearPt  = CVector{ pos.x - fwd.x * halfLengthF, pos.y - fwd.y * halfLengthF, pos.z };
    const auto frontPt = CVector{ pos.x + fwd.x * halfLengthR, pos.y + fwd.y * halfLengthR, pos.z };

    const auto ProbeDownwards = [&](const CVector& pt, CStoredCollPoly& collPoly) {
        CColPoint colPoint{};
        CEntity*  colEntity{};
        if (!CWorld::ProcessVerticalLine(CVector{ pt.x, pt.y, pos.z + 5.0f }, pos.z - 5.0f, colPoint, colEntity, true)) {
            return pos.z;
        }

        collPoly.ligthing    = colPoint.m_nLightingB;
        m_pEntityWeAreOn     = colEntity;
        m_bTunnel            = colEntity->m_bTunnel;
        m_bTunnelTransition  = colEntity->m_bTunnelTransition;
        return colPoint.m_vecPoint.z;
    };

    const auto frontZ = ProbeDownwards(frontPt, m_FrontCollPoly);
    const auto rearZ  = ProbeDownwards(rearPt,  m_RearCollPoly);

    const auto length    = halfLengthF + halfLengthR;
    const auto pitch     = std::atan2((frontZ - rearZ) / length, 1.0f);
    const auto cosPitch  = std::cos(pitch);
    const auto sinPitch  = std::sin(pitch);

    GetRight() = CVector{
        (frontPt.y - rearPt.y) / length,
        -((frontPt.x - rearPt.x) / length),
        0.0f
    };
    GetForward() = CVector{
        -cosPitch * GetRight().y,
        cosPitch * GetRight().x,
        sinPitch
    };
    GetUp() = CrossProduct(GetRight(), GetForward());

    SetPosn(CVector{
        (frontPt.x + rearPt.x) * 0.5f,
        (frontPt.y + rearPt.y) * 0.5f,
        (frontZ + rearZ) * 0.5f + GetHeightAboveRoad()
    });
}

// 0x6BF230
void CBike::GetCorrectedWorldDoorPosition(CVector& out, CVector arg1, CVector arg2) {
    const auto side  = CrossProduct(GetForward(), CVector{0.0f, 0.0f, 1.0f});
    const auto right = CrossProduct(side, GetForward());

    const auto& box = GetColModel()->m_boundBox;
    const auto  bodyWidth = box.m_vecMax.x < box.m_vecMax.z ? box.m_vecMax.z - box.m_vecMax.x : 0.0f;

    out = CVector{};
    out += GetForward() * (arg2.z - arg1.z);
    out += side * (bodyWidth * DotProduct(side, GetUp()) + (arg2.y - arg1.y));
    out += right * (arg2.x - arg1.x);
    out += GetPosition();
}

// 0x6BEA10
void CBike::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    GetMoveSpeed().z += 0.13f;
    SetStatus(STATUS_WRECKED);
    physicalFlags.bRenderScorched = true;

    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), ATOMIC_PIPE_NO_EXTRA_PASSES);

    m_fHealth    = 0.0f;
    m_wBombTimer = 0;

    TheCamera.CamShake(0.4f, GetPosition());
    KillPedsInVehicle();

    m_nOverrideLights      = NO_CAR_LIGHT_OVERRIDE;
    vehicleFlags.bEngineOn = false;
    vehicleFlags.bLightsOn = false;

    ChangeLawEnforcerState(false);

    CExplosion::AddExplosion(this, damager, EXPLOSION_CAR, GetPosition(), 0, true, -1.0f, bHideExplosion);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);
}

// 0x6B7050
void CBike::Fix() {
    vehicleFlags.bIsDamaged = false;
    bikeFlags.bEngineOnFire = false;
    m_nWheelStatus[0] = 0;
    m_nWheelStatus[1] = 0;
}

// 0x6BD090
void CBike::PreRender() {
    plugin::CallMethod<0x6BD090, CBike*>(this);
}

// 0x6BDE20
void CBike::Render() {
    auto savedRef = 0;
    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &savedRef);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(1));

    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3000;
    CVehicle::Render();

    if (m_renderLights.m_bRightFront) {
        CalculateLeanMatrix();
        CVehicle::DoHeadLightBeam(eVehicleLightId::MAIN, m_mLeanMatrix, true);
    }

    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(savedRef));
}

// 0x6BCFC0
void CBike::Teleport(CVector destination, bool resetRotation) {
    CWorld::Remove(this);

    GetPosition() = destination;
    if (resetRotation)
        SetOrientation(0.0f, 0.0f, 0.0f);

    ResetMoveSpeed();
    ResetTurnSpeed();
    ResetSuspension();

    CWorld::Add(this);
}

// 0x6B8EC0
void CBike::VehicleDamage(float damageIntensity, eVehicleCollisionComponent component, CEntity* damager, CVector* vecCollisionCoors, CVector* vecCollisionDirection, eWeaponType weapon) {
    // NOTE: The actual damage taken is `m_fDamageIntensity` (This is only called with `damageIntensity <= 0` after collisions)
    if (damageIntensity > 0.0f) {
        return;
    }
    if (m_fDamageIntensity < 1.0f || !vehicleFlags.bCanBeDamaged) {
        return;
    }

    auto damage = m_fDamageIntensity;
    if (GetStatus() == STATUS_PLAYER && CStats::GetPercentageProgress() >= 100.0f) {
        damage *= 0.5f;
    }

    if (bikeFlags.bOnSideStand && damage > 20.0f) {
        bikeFlags.bOnSideStand = false;
    }

    DamageKnockOffRider(this, m_fDamageIntensity, m_nPieceType, m_pDamageEntity, m_vecLastCollisionPosn, m_vecLastCollisionImpactVelocity);

    if (m_pDamageEntity && m_pDamageEntity->GetType() == ENTITY_TYPE_VEHICLE) {
        m_nLastWeaponDamageType = WEAPON_RAMMEDBYCAR;
        m_pLastDamageEntity     = m_pDamageEntity;
        CEntity::RegisterReference(&m_pLastDamageEntity);
    }

    // A ped colliding with our upper side doesn't damage us
    if (   physicalFlags.bCollisionProof
        || (   m_pDamageEntity
            && m_pDamageEntity->GetType() == ENTITY_TYPE_PED
            && DotProduct(m_vecLastCollisionImpactVelocity, GetUp()) > 0.6f)
    ) {
        return;
    }

    if (damage > 25.0f && GetStatus() != STATUS_WRECKED) {
        // If we're a cop bike, and the damager is the player's vehicle, give the player a wanted level
        if (vehicleFlags.bIsLawEnforcer) {
            const auto playerVeh = FindPlayerVehicle();
            if (playerVeh && m_pDamageEntity == playerVeh && GetStatus() != STATUS_ABANDONED) {
                const auto theirSpeed = playerVeh->GetMoveSpeed().Magnitude();
                if (theirSpeed >= GetMoveSpeed().Magnitude() && theirSpeed > 0.1f) {
                    FindPlayerPed()->SetWantedLevelNoDrop(eWantedLevel::WANTED_LEVEL_1);
                }
            }
        }

        damage = (damage - 25.0f) * m_pHandlingData->m_fCollisionDamageMultiplier;
        if (damage > 0.0f) {
            // Rider complains (unless it's the player's vehicle hitting somebody else's mission vehicle)
            if (damage > 5.0f && m_pDriver && m_pDamageEntity && m_pDamageEntity->GetType() == ENTITY_TYPE_VEHICLE) {
                if (FindPlayerVehicle() != this || m_pDamageEntity->AsVehicle()->m_nCreatedBy != MISSION_VEHICLE) {
                    if (m_pDamageEntity->AsVehicle()->m_pDriver) {
                        m_pDriver->Say(CTX_GLOBAL_CRASH_BIKE, 0, 1.0f);
                    }
                }
            }

            // Scale the damage based on who owns the bike, and who hit it
            if (this == FindPlayerVehicle()) {
                damage *= vehicleFlags.bTakeLessDamage ? 1.0f / 6.0f : 0.5f;
            } else if (!vehicleFlags.bTakeLessDamage) {
                damage *= m_pDamageEntity && FindPlayerVehicle() == m_pDamageEntity ? 2.0f / 3.0f : 0.25f;
            } else {
                damage *= 1.0f / 12.0f;
            }

            const auto oldHealth = m_fHealth;
            m_fHealth -= damage;
            if (m_fHealth < 1.0f && oldHealth > 1.0f) {
                m_fHealth = 1.0f; // Don't let it die from a single hit
            }
        }
    }

    if (m_fHealth < 250.0f && !bikeFlags.bEngineOnFire) {
        bikeFlags.bEngineOnFire = true;
        m_BlowUpTimer           = 0;
        m_Damager               = m_pDamageEntity;
        if (m_Damager) {
            CEntity::RegisterReference(&m_Damager);
        }
    }
}

// 0x6B89B0
void CBike::SetupSuspensionLines() {
    const auto& mi       = *GetVehicleModelInfo();
    auto&       colModel = *GetColModel();
    auto&       colData  = *colModel.m_pColData;
    const auto& handling = *m_pHandlingData;

    // `CBike::CBike` puts this sentinel into the freshly allocated lines
    const bool hasPredefinedLines = colData.m_pLines[0].m_vecStart.z != 99999.99f;

    //! Get the node's position relative to the clump's root frame (I.e. in vehicle space)
    const auto GetNodePos = [this](RwFrame* node) {
        RwMatrix   matrix     = *RwFrameGetMatrix(node);
        const auto clumpFrame = RpClumpGetFrame(GetRpClump());
        for (auto parent = RwFrameGetParent(node); parent && parent != clumpFrame; parent = RwFrameGetParent(parent)) {
            RwMatrixTransform(&matrix, RwFrameGetMatrix(parent), rwCOMBINEPOSTCONCAT);
        }
        return CVector{matrix.pos.x, matrix.pos.y, matrix.pos.z};
    };

    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        auto& line = colData.m_pLines[i];

        CVector pos{};
        auto    yOffset = 0.0f;
        if (hasPredefinedLines) {
            pos = CVector{line.m_vecStart.x, line.m_vecStart.y, m_aWheelOrigHeights[i < 2 ? 0 : 1]};
        } else {
            switch (i) {
            case 0:
                pos     = GetNodePos(m_aBikeNodes[BIKE_WHEEL_FRONT]);
                yOffset = mi.m_fWheelSizeFront * 0.25f;
                break;
            case 1:
                pos     = GetNodePos(m_aBikeNodes[BIKE_WHEEL_FRONT]);
                yOffset = mi.m_fWheelSizeFront * -0.25f;
                break;
            case 2:
                pos     = GetNodePos(m_aBikeNodes[BIKE_WHEEL_REAR]);
                yOffset = mi.m_fWheelSizeRear * 0.25f;
                break;
            case 3:
                pos     = GetNodePos(m_aBikeNodes[BIKE_WHEEL_REAR]);
                yOffset = mi.m_fWheelSizeRear * -0.25f;
                break;
            default:
                NOTSA_UNREACHABLE();
            }

            if (i == 0) {
                m_aWheelOrigHeights[0] = pos.z;
            } else if (i == 2) {
                m_aWheelOrigHeights[1] = pos.z;

                // Swing arm length (Distance between the rear wheel and the rear fork)
                if (const auto forksRear = m_aBikeNodes[BIKE_FORKS_REAR]) {
                    const auto forkPos = GetNodePos(forksRear);
                    const auto d = pos - forkPos;
                    m_fSwingArmLength = std::sqrt(sq(d.y) + sq(d.z));
                } else {
                    m_fSwingArmLength = 0.0f;
                }
            }
        }

        const auto wheelSize = i < 2 ? mi.m_fWheelSizeFront : mi.m_fWheelSizeRear;
        pos.y += yOffset;
        pos.z += handling.m_fSuspensionUpperLimit;

        line.m_vecStart = pos;
        line.m_vecEnd   = CVector{
            pos.x,
            pos.y,
            (handling.m_fSuspensionLowerLimit - handling.m_fSuspensionUpperLimit) - wheelSize * 0.5f + pos.z
        };

        m_fSuspensionLength[i] = handling.m_fSuspensionUpperLimit - handling.m_fSuspensionLowerLimit;
        m_fLineLength[i]       = line.m_vecStart.z - line.m_vecEnd.z;
    }

    if (!hasPredefinedLines) {
        const auto forkPos = GetNodePos(m_aBikeNodes[BIKE_FORKS_FRONT]);
        m_fForkYOffset = forkPos.y;
        m_fForkZOffset = forkPos.z;
    }

    m_fHeightAboveRoad =
        (mi.m_fWheelSizeFront * 0.5f - colData.m_pLines[0].m_vecStart.z)
        + (1.0f - 1.0f / (handling.m_fSuspensionForceLevel * 4.0f)) * m_fSuspensionLength[0];

    for (auto i = 0u; i < 2; i++) {
        m_aWheelSuspensionHeights[i] = (i == 0 ? mi.m_fWheelSizeFront : mi.m_fWheelSizeRear) * 0.5f - m_fHeightAboveRoad;
    }

    if (colData.m_pLines[0].m_vecEnd.z < colModel.m_boundBox.m_vecMin.z) {
        colModel.m_boundBox.m_vecMin.z = colData.m_pLines[0].m_vecEnd.z;
    }

    colModel.m_boundSphere.m_fRadius = std::max({
        colModel.m_boundSphere.m_fRadius,
        colModel.m_boundBox.m_vecMin.Magnitude(),
        colModel.m_boundBox.m_vecMax.Magnitude()
    });

    // "Force ground clearance": lower the collision spheres' bottoms to the ground clearance height
    if (handling.m_bForceGroundClearance && !hasPredefinedLines) {
        const auto groundClearance = 0.25f - m_fHeightAboveRoad;
        for (auto&& sphere : colData.GetSpheres()) {
            if (sphere.m_vecCenter.z - sphere.m_fRadius < groundClearance) {
                if (sphere.m_fRadius > 0.4f) {
                    sphere.m_fRadius = std::max(sphere.m_vecCenter.z - groundClearance, 0.4f);
                }
                sphere.m_vecCenter.z = groundClearance + sphere.m_fRadius;
            }
        }
    }
}

// 0x6B8970
void CBike::SetModelIndex(uint32 index) {
    CVehicle::SetModelIndex(index);
    SetupModelNodes();
}

// 0x6B5960
void CBike::SetupModelNodes() {
    std::ranges::fill(m_aBikeNodes, nullptr);
    CClumpModelInfo::FillFrameArray(GetRpClump(), m_aBikeNodes.data());
}

// 0x6B7080 - Same as `CAutomobile::PlayCarHorn`
void CBike::PlayCarHorn() {
    if (IsAlarmActivated() && GetStatus() != STATUS_WRECKED || m_HornCounter) {
        return;
    }

    if (m_nCarHornTimer) {
        m_nCarHornTimer -= 1;
        return;
    }

    m_nCarHornTimer = CGeneral::GetRandomNumber() % 128 - 106; // TODO: GetRandomNumberInRange
    if (const auto r = m_nCarHornTimer % 8; r < 4) {
        if (r >= 2) {
            if (m_pDriver && m_autoPilot.carCtrlFlags.bHonkAtCar) {
                m_pDriver->Say(CTX_GLOBAL_BLOCKED);
            }
        }
        m_HornCounter = 45;
    } else {
        if (m_pDriver) {
            m_pDriver->Say(CTX_GLOBAL_BLOCKED);
        }
    }
}

// 0x6B7070
void CBike::SetupDamageAfterLoad() {
    // NOP
}

// 0x6B6950
void CBike::DoBurstAndSoftGroundRatios() {
    const auto& mi   = *GetVehicleModelInfo();
    const auto speed = std::abs(DotProduct(GetMoveSpeed(), GetForward()));

    // Wheels that haven't been handled by the pair-loop below (And are thus still on "soft" ground)
    std::array<bool, NUM_SUSP_LINES> onSoftGround{ true, true, true, true };

    // Each "wheel pair" (front: 0/1, rear: 2/3) is processed together
    for (auto pair = 0u; pair < 2; pair++) {
        const auto wheelA = pair * 2;     // First wheel of the pair
        const auto wheelB = pair * 2 + 1; // Second wheel of the pair
        const auto wheelSize = pair == 0 ? mi.m_fWheelSizeFront : mi.m_fWheelSizeRear;

        switch (m_nWheelStatus[pair]) {
        case 2: // Burst
            m_aWheelRatios[wheelA] = 1.0f;
            m_aWheelRatios[wheelB] = 1.0f;
            break;
        case 1: { // Soft/Punctured
            // The chance of the tyre slowly inflating back decreases with speed
            const auto rnd = static_cast<int32>(
                static_cast<float>(CGeneral::GetRandomNumber()) * (1.0f / 32768.0f)
                * static_cast<float>(static_cast<int32>(40.0f * speed) + 98)
            );
            if (rnd < 100) {
                const auto ratioIncrease = (m_fLineLength[wheelA] - m_fSuspensionLength[wheelA]) / m_fLineLength[wheelA] * 0.2f;
                m_aWheelRatios[wheelA] = std::min(m_aWheelRatios[wheelA] + ratioIncrease, 1.0f);
                m_aWheelRatios[wheelB] = std::min(m_aWheelRatios[wheelB] + ratioIncrease, 1.0f);
            }
            break;
        }
        default: { // Not damaged
            // Only do the (rail-track specific) "slipping" check if a wheel is standing on a rail
            if (!(   (m_aWheelRatios[wheelA] < 1.0f && m_aWheelColPoints[wheelA].m_nSurfaceTypeB == SURFACE_RAILTRACK)
                  || (m_aWheelRatios[wheelB] < 1.0f && m_aWheelColPoints[wheelB].m_nSurfaceTypeB == SURFACE_RAILTRACK))) {
                continue;
            }

            auto k = 1.5f / (wheelSize * 0.5f);
            if (speed > 0.3f) {
                k *= speed / 0.3f;
            }
            k = 1.0f / k;

            const auto angle1 = m_aWheelPitchAngles[pair] * k;
            const auto frac1  = angle1 - std::trunc(angle1);

            const auto angle2 = (CTimer::GetTimeStep() * m_aWheelAngularVelocity[pair] + m_aWheelPitchAngles[pair]) * k;
            const auto frac2  = angle2 - std::trunc(angle2);

            // The wheel "crossed" an integer angle (A full revolution of the wheel), so it lost some grip
            const auto angVel = m_aWheelAngularVelocity[pair];
            if ((angVel > 0.0f && frac2 < frac1) || frac2 > frac1) {
                const auto ratioDecrease = (m_fLineLength[wheelA] - m_fSuspensionLength[wheelA]) / m_fLineLength[wheelA] * 0.3f;
                m_aWheelRatios[wheelA] = std::max(m_aWheelRatios[wheelA] - ratioDecrease, 0.2f);
                m_aWheelRatios[wheelB] = std::max(m_aWheelRatios[wheelB] - ratioDecrease, 0.2f);
            }
            break;
        }
        }

        onSoftGround[wheelA] = false;
        onSoftGround[wheelB] = false;
    }

    // Wheels still on soft ground slowly get more grip (The ratio goes up to 1)
    for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
        if (!onSoftGround[i] || m_aWheelRatios[i] >= 1.0f) {
            continue;
        }
        if (g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[i].m_nSurfaceTypeB) != ADHESION_GROUP_SAND) {
            continue;
        }
        if (m_nModelIndex == MODEL_RHINO) {
            continue;
        }

        auto scale = 0.25f;
        if (m_nHandlingFlagsIntValue & 0x200000) {        // handlingFlags.bOffroadAbility2
            scale = 0.1f;
        } else if (m_nHandlingFlagsIntValue & 0x100000) { // handlingFlags.bOffroadAbility
            scale = 0.15f;
        }

        auto weatherFactor = (1.0f - (speed / 0.3f) * 0.7f) - CWeather::WetRoads * 0.7f;
        if (weatherFactor < 0.4f) {
            weatherFactor = 0.4f;
        }

        m_aWheelRatios[i] = std::min(
            m_aWheelRatios[i] + (m_fLineLength[i] - m_fSuspensionLength[i]) / m_fLineLength[i] * weatherFactor * scale,
            1.0f
        );
    }
}

// 0x6B67E0
bool CBike::SetUpWheelColModel(CColModel* wheelCol) {
    const auto mi = GetVehicleModelInfo();

    wheelCol->m_boundBox    = GetColModel()->m_boundBox;
    wheelCol->m_boundSphere = GetColModel()->m_boundSphere;

    // The wheel's position relative to the chassis (Not the whole vehicle - The hierarchy is walked until the chassis frame)
    const auto GetWheelPosInChassisSpace = [this](RwFrame* wheelFrame) {
        RwMatrix matrix = *RwFrameGetMatrix(wheelFrame);
        for (auto parent = RwFrameGetParent(wheelFrame); parent && parent != m_aBikeNodes[BIKE_CHASSIS]; parent = RwFrameGetParent(parent)) {
            RwMatrixTransform(&matrix, RwFrameGetMatrix(parent), rwCOMBINEPOSTCONCAT);
        }
        return CVector{matrix.pos.x, matrix.pos.y, matrix.pos.z};
    };

    wheelCol->m_pColData->m_pSpheres[0].Set(mi->m_fWheelSizeFront / 2.0f, GetWheelPosInChassisSpace(m_aBikeNodes[BIKE_WHEEL_FRONT]), SURFACE_RUBBER, CAR_PIECE_WHEEL_LF);
    wheelCol->m_pColData->m_pSpheres[1].Set(mi->m_fWheelSizeRear  / 2.0f, GetWheelPosInChassisSpace(m_aBikeNodes[BIKE_WHEEL_REAR]),  SURFACE_RUBBER, CAR_PIECE_WHEEL_RL);

    wheelCol->m_pColData->m_nNumSpheres = 2;
    return true;
}

// 0x6B67B0
void CBike::RemoveRefsToVehicle(CEntity* entityToRemove) {
    for (auto& entity: m_aGroundPhysicalPtrs) {
        if (entity == entityToRemove)
            entity = nullptr;
    }
}

// 0x6B6620
void CBike::ProcessControlCollisionCheck(bool applySpeed) {
    const CMatrix oldMat = GetMatrix();
    SetIsStuck(false);
    SkipPhysics();
    physicalFlags.bSkipLineCol     = false;
    physicalFlags.bProcessingShift = false;
    m_fMovingSpeed                 = 0.0f;
    rng::fill(m_aWheelRatios, 1.0f);

    if (applySpeed) {
        ApplyMoveSpeed();
        ApplyTurnSpeed();

        for (auto i = 0; CheckCollision() && i < 5; i++) {
            GetMatrix() = oldMat;
            ApplyMoveSpeed();
            ApplyTurnSpeed();
        }
    } else {
        const auto usesCollision = GetUsesCollision();
        SetUsesCollision(false);
        CheckCollision();
        SetUsesCollision(usesCollision);
    }

    SetIsStuck(false);
    SetIsInSafePosition(true);
}

// 0x6B5990
void CBike::GetComponentWorldPosition(int32 componentId, CVector& outPos) {
    if (IsComponentPresent(componentId))
        outPos = RwFrameGetLTM(m_aBikeNodes[componentId])->pos;
    else
        NOTSA_LOG_DEBUG("BikeNode missing: model={}, nodeIdx={}", m_nModelIndex, componentId);
}

// 0x6B58D0
void CBike::ProcessOpenDoor(CPed* ped, uint32 doorComponentId, uint32 animGroup, uint32 animId, float fTime) {
    // NOP
}

