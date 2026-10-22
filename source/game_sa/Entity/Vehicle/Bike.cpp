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
#include "Automobile.h"
#include "Shadows.h"

// 0xC1C804 - Unnamed: The signed square of the bike's raw steer angle, published by `CBike::ProcessControlInputs`
static auto& ms_fSteerAngleSquared = StaticRef<float>(0xC1C804);

// 0x8D3274 - Unnamed: Object space offset added to the right hand bone while fixing the hands to the bars
static constexpr CVector ms_vecHandOffset = { -0.075f, -0.03f, 0.0f };

// 0xC1C81C - Unnamed: The divisor of the forward speed used by the exhaust particles check (1 / 180)
static auto& ms_fExhaustSpeedDivisor = StaticRef<float>(0xC1C81C);

// 0xC1C26C - Unnamed: The front wheel's state (`tWheelState`) handed to/from `CVehicle::ProcessBikeWheel`
static auto& ms_FrontWheelState = StaticRef<tWheelState>(0xC1C26C);

// 0xC1C270 - Unnamed: The rear wheel's state (`tWheelState`) handed to/from `CVehicle::ProcessBikeWheel`
static auto& ms_RearWheelState = StaticRef<tWheelState>(0xC1C270);

// 0xC1C27C - Unnamed: The drive thrust handed to `CVehicle::ProcessBikeWheel`
static auto& ms_fWheelThrust = StaticRef<float>(0xC1C27C);

// 0xC1C818 - Unnamed: Scale applied to the bike's traction multiplier (0.004)
static auto& ms_fBikeTractionScale = StaticRef<float>(0xC1C818);

// 0x96914C - Unnamed: Handling cheat (see `cTransmission::CalculateDriveAcceleration`) state for bikes
static auto& ms_nHandlingCheat = StaticRef<uint8>(0x96914C);



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
    RH_ScopedInstall(ProcessRiderAnims, 0x6B7280);
    RH_ScopedInstall(FixHandsToBars, 0x6B7F90);
    RH_ScopedInstall(PlaceOnRoadProperly, 0x6BEEB0);
    RH_ScopedInstall(GetCorrectedWorldDoorPosition, 0x6BF230);
    RH_ScopedVMTInstall(Fix, 0x6B7050);
    RH_ScopedVMTInstall(BlowUpCar, 0x6BEA10);
    RH_ScopedVMTInstall(ProcessDrivingAnims, 0x6BF400);
    RH_ScopedVMTInstall(BurstTyre, 0x6BEB20);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6BE310);
    RH_ScopedVMTInstall(ProcessEntityCollision, 0x6BDEA0);
    RH_ScopedVMTInstall(Render, 0x6BDE20);
    RH_ScopedVMTInstall(PreRender, 0x6BD090);
    RH_ScopedVMTInstall(Teleport, 0x6BCFC0);
    RH_ScopedVMTInstall(ProcessControl, 0x6B9250);
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
    (void)a5; // Not used by the original either

    const auto isPlayer = rider->IsPlayer();

    // The vehicle is a bike (Or a BMX), or a quad (Which derives from `CAutomobile`)
    CBike*       bike{};
    CAutomobile* quad{};
    int32        numContactWheels{};
    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        bike             = static_cast<CBike*>(vehicle);
        numContactWheels = bike->m_nNoOfContactWheels;
    } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
        quad             = static_cast<CAutomobile*>(vehicle);
        numContactWheels = quad->m_nNumContactWheels;
    }

    const auto clump     = rider->GetRpClump();
    const auto animGroup = rideData->AnimGroup;
    const auto GetAnim   = [clump](uint32 animId) { return RpAnimBlendClumpGetAssociation(clump, animId); };

    auto* leftAnim    = GetAnim(ANIM_ID_BIKE_LEFT);
    auto* rightAnim   = GetAnim(ANIM_ID_BIKE_RIGHT);
    auto* stillAnim   = GetAnim(ANIM_ID_BIKE_STILL);
    auto* fwdAnim     = isPlayer ? GetAnim(ANIM_ID_BIKE_FWD) : nullptr;
    auto* backAnim    = isPlayer ? GetAnim(ANIM_ID_BIKE_BACK) : nullptr;
    auto* pushesAnim  = GetAnim(ANIM_ID_BIKE_PUSHES);
    auto* driveByAnim = GetAnim(ANIM_ID_BIKE_DRIVEBYLHS);
    if (!driveByAnim) {
        driveByAnim = GetAnim(ANIM_ID_BIKE_DRIVEBYRHS);
    }
    if (!driveByAnim) {
        driveByAnim = GetAnim(ANIM_ID_BIKE_DRIVEBYFT);
    }

    const auto  fwdSpeed = DotProduct(vehicle->m_vecMoveSpeed, vehicle->GetForward());
    const auto& handlingData = *vehicle->m_pHandlingData;

    //> A BMX rider releases the right hand off the handlebar while doing tricks/drive-bys
    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BMX && (pushesAnim || driveByAnim)) {
        if (bike) {
            bike->m_nFixRightHand = false;
        }
    }

    const auto AddKnockOffEvent = [&](CPed* ped, bool isVictimDriver) {
        ped->GetEventGroup().Add(CEventKnockOffBike{
            vehicle, vehicle->m_vecMoveSpeed, CVector{ 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f,
            KNOCK_OFF_TYPE_SKIDBACK_FALLR, 2, 0, nullptr, isVictimDriver, false
        });
    };

    //> Knocked off if spinning too fast
    {
        const float maxTurnSpeed = (rider->GetBikeRidingSkill() + 1.0f) * 0.3f;
        if (maxTurnSpeed * maxTurnSpeed < vehicle->m_vecTurnSpeed.SquaredMagnitude()) {
            AddKnockOffEvent(rider, true);
            if (vehicle->m_apPassengers[0] && vehicle->m_apPassengers[0] != rider) {
                AddKnockOffEvent(vehicle->m_apPassengers[0], false);
            }
        }
    }

    float leanAnimMult  = 1.0f; // Multiplier applied to the blending of the lean animations
    float leanFwdTarget = 0.0f; // Set to `-1` when there's no target (Meaning "keep the current value")

    //> Lean forward/backward, and knock-off while reversing
    if (driveByAnim || std::abs(fwdSpeed) >= 0.02f) {
        bool clampAnimDeltas = false;
        if (fwdSpeed >= 0.0f) {
            clampAnimDeltas = true;
        } else {
            float maxReverseSpeed = (rider->GetBikeRidingSkill() + 1.0f) * 3.5f;
            if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
                maxReverseSpeed += maxReverseSpeed;
            }
            const auto reverseSpeedLimit = maxReverseSpeed * handlingData.m_transmissionData.m_MaxReverseVelocity;
            if (reverseSpeedLimit > fwdSpeed) {
                if (   numContactWheels > 0
                    || (   vehicle->GetUp().z < -0.5f
                        && CTimer::GetTimeInMS() - vehicle->m_nLastCollisionTime < 100
                    )
                ) {
                    AddKnockOffEvent(rider, true);
                    if (vehicle->m_apPassengers[0] && vehicle->m_apPassengers[0] != rider) {
                        AddKnockOffEvent(vehicle->m_apPassengers[0], false);
                    }
                } else if (   vehicle->m_GasPedal >= 0.0f
                           || fwdSpeed >= handlingData.m_transmissionData.m_MaxReverseVelocity * 1.5f
                ) {
                    if (isPlayer && fwdSpeed > handlingData.m_transmissionData.m_MaxReverseVelocity * 1.5f) {
                        leanFwdTarget = -1.0f;
                    }
                    clampAnimDeltas = true;
                } else if (   !pushesAnim
                           || (   pushesAnim->m_BlendAmount < 1.0f
                               && pushesAnim->m_BlendDelta <= 0.0f
                              )
                ) {
                    pushesAnim = CAnimManager::BlendAnimation(clump, animGroup, ANIM_ID_BIKE_PUSHES, 4.0f);
                }
            } else {
                clampAnimDeltas = true;
            }
        }

        if (clampAnimDeltas) {
            //> Don't let the still/pushes animations play forward again
            for (auto* anim : { stillAnim, pushesAnim }) {
                if (anim && anim->m_BlendDelta >= 0.0f) {
                    anim->m_BlendDelta = -4.0f;
                }
            }
        }
    } else if (   !stillAnim
               || (   stillAnim->m_BlendAmount < 1.0f
                   && stillAnim->m_BlendDelta <= 0.0f
                  )
    ) {
        stillAnim = CAnimManager::BlendAnimation(clump, animGroup, ANIM_ID_BIKE_STILL, 2.0f);
    }

    //> How much of the lean animations is "visible" (Inverse of the blend of the still/pushes/drive-by animations)
    if (stillAnim) {
        const auto blend = std::min(1.0f, CTimer::GetTimeStepNonClipped() * 0.02f * stillAnim->m_BlendDelta + stillAnim->m_BlendAmount);
        leanAnimMult = 1.0f - blend;
    }
    for (auto* anim : { driveByAnim, pushesAnim }) {
        if (anim) {
            const auto blend = CTimer::GetTimeStepNonClipped() * 0.02f * anim->m_BlendDelta + anim->m_BlendAmount;
            leanAnimMult -= std::min(blend, leanAnimMult);
        }
    }

    //> The target lean angle (Derived from the lean of the bike)
    auto leanAngle = rideData->LeanAngle / handling->m_fFullAnimLean;
    if (leanFwdTarget == -1.0f) {
        leanAngle = 0.0f;
    } else {
        leanAngle = std::clamp(leanAngle, -1.0f, 1.0f);
    }
    {
        const auto damp = std::pow(0.86f, CTimer::GetTimeStep());
        rideData->AnimLeanLeft = damp * rideData->AnimLeanLeft + (1.0f - damp) * leanAngle;
    }

    //> Lean forward when braking/accelerating
    const auto ApplyLeanFwdPedals = [&] {
        if (vehicle->m_BrakePedal > 0.5f && fwdSpeed > 0.01f) {
            if (leanFwdTarget <= 0.1f) {
                leanFwdTarget = 0.1f;
            }
        } else if (vehicle->m_GasPedal > 0.5f) {
            if (   leanFwdTarget < 0.0f
                && fwdSpeed < handlingData.m_transmissionData.m_MaxFlatVelocity * 0.3f
                && leanFwdTarget >= -0.3f
            ) {
                leanFwdTarget = -0.3f;
            }
        }
    };
    const auto ApplyWheelieAnimTail = [&](float wheelieAngleDiff) {
        if (wheelieAngleDiff < 0.15f) {
            if (leanFwdTarget <= 0.25f) {
                leanFwdTarget = 0.25f;
            }
        } else {
            ApplyLeanFwdPedals();
        }
    };

    if (!isPlayer || vehicle->m_apPassengers[0]) {
        leanFwdTarget = 0.0f;
        if (bike) {
            bike->nBikeFlags &= ~0x80; // bWheelieForCamera
        }
    } else if (leanFwdTarget > -1.0f) {
        leanFwdTarget = rideData->LeanFwd;

        if (bike) {
            //> Wheelie while the front wheel is off the ground, and the rear is on it
            const auto fwdZ = vehicle->GetForward().z;
            bike->nBikeFlags &= ~0x80; // bWheelieForCamera
            if (   bike->m_WheelCounts[0] > 0.0f
                || bike->m_WheelCounts[1] > 0.0f
                || fwdZ <= 0.0f
                || (bike->m_WheelCounts[2] <= 0.0f && bike->m_WheelCounts[3] <= 0.0f)
            ) {
                //> Stoppie while both rear wheels are off the ground, and the bike is nosing down
                if (   bike->m_WheelCounts[2] <= 0.0f
                    && bike->m_WheelCounts[3] <= 0.0f
                    && fwdZ < 0.0f
                    && (bike->m_WheelCounts[0] > 0.0f || bike->m_WheelCounts[1] > 0.0f)
                    && handling->m_fStoppieAng * 0.6f < handling->m_fStoppieAng - fwdZ
                ) {
                    bike->nBikeFlags |= 0x80; // bWheelieForCamera
                }
                ApplyLeanFwdPedals(); // No wheelie, but apply the "press gas/brake" part
            } else {
                ApplyWheelieAnimTail(handling->m_fWheelieAng - fwdZ);
            }
        } else if (quad) {
            const auto fwdZ = vehicle->GetForward().z;
            if (   quad->m_WheelCounts[0] <= 0.0f
                && quad->m_WheelCounts[1] <= 0.0f
                && fwdZ > 0.0f
                && (quad->m_WheelCounts[2] > 0.0f || quad->m_WheelCounts[3] > 0.0f)
            ) {
                ApplyWheelieAnimTail(handling->m_fWheelieAng - fwdZ);
            } else {
                ApplyLeanFwdPedals(); // No wheelie, but apply the "press gas/brake" part
            }
        }

        if (std::abs(leanAngle) > 0.3f) {
            leanFwdTarget *= std::max(0.0f, 1.0f - (std::abs(leanAngle) - 0.3f) / 0.26f);
        }
    }

    if (isPlayer) {
        const auto damp = std::pow(0.89f, CTimer::GetTimeStep());
        rideData->AnimLeanFwd = damp * rideData->AnimLeanFwd + (1.0f - damp) * leanFwdTarget;
    } else {
        rideData->AnimLeanFwd = 0.0f;
    }

    //> Distribute the lean between the left/right and forward/backward animations
    CVector leanWeights{}; // x = forward/backward, y = left/right
    if (std::abs(rideData->AnimLeanLeft) > 0.56f || !isPlayer) {
        leanWeights = CVector{ 0.0f, 1.0f, 0.0f };
    } else if (std::abs(rideData->AnimLeanFwd) <= 0.56f) {
        leanWeights = CVector{ rideData->AnimLeanFwd, rideData->AnimLeanLeft, 0.0f };
        const auto mag = leanWeights.Magnitude2D();
        if (mag <= 0.01f) {
            leanWeights.x = std::abs(leanWeights.x);
            leanWeights.y = std::abs(leanWeights.y);
        } else {
            leanWeights.x = std::abs(leanWeights.x / mag);
            leanWeights.y = std::abs(leanWeights.y / mag);
        }
    } else {
        leanWeights = CVector{ 1.0f, 0.0f, 0.0f };
    }

    if (isPlayer) {
        if (!fwdAnim) {
            fwdAnim = CAnimManager::AddAnimation(clump, animGroup, ANIM_ID_BIKE_FWD);
        }
        if (!backAnim) {
            backAnim = CAnimManager::AddAnimation(clump, animGroup, ANIM_ID_BIKE_BACK);
        }
        auto* const anim = rideData->AnimLeanFwd < 0.0f ? backAnim : fwdAnim;
        auto* const otherAnim = rideData->AnimLeanFwd < 0.0f ? fwdAnim : backAnim;
        anim->m_BlendAmount = leanWeights.x * leanAnimMult;
        anim->SetCurrentTime((rideData->AnimLeanFwd < 0.0f ? -1.0f : 1.0f) * anim->GetHier()->GetTotalTime() * rideData->AnimLeanFwd);
        anim->SetFlag(ANIMATION_IS_PLAYING, false);
        otherAnim->m_BlendAmount = 0.0f;
    }

    if (!leftAnim) {
        leftAnim = CAnimManager::AddAnimation(clump, animGroup, ANIM_ID_BIKE_LEFT);
    }
    if (!rightAnim) {
        rightAnim = CAnimManager::AddAnimation(clump, animGroup, ANIM_ID_BIKE_RIGHT);
    }
    {
        auto* const anim      = rideData->AnimLeanLeft < 0.0f ? leftAnim : rightAnim;
        auto* const otherAnim = rideData->AnimLeanLeft < 0.0f ? rightAnim : leftAnim;
        anim->m_BlendAmount = leanWeights.y * leanAnimMult;
        anim->SetCurrentTime((rideData->AnimLeanLeft < 0.0f ? -1.0f : 1.0f) * anim->GetHier()->GetTotalTime() * rideData->AnimLeanLeft);
        anim->SetFlag(ANIMATION_IS_PLAYING, false);
        otherAnim->m_BlendAmount = 0.0f;
    }

    //> Wobble the head around a bit at higher speeds
    if (fwdSpeed > 0.3f) {
        const auto  spread = fwdSpeed * 6.0f;
        const auto  RandomAngle = [spread] { return (spread - -spread) * (float)rand() * 3.051851e-05f + -spread; };
        static const CVector xAxis = { 1.0f, 0.0f, 0.0f };
        static const CVector yAxis = { 0.0f, 1.0f, 0.0f };
        auto* const headFrame = rider->m_apBones[PED_NODE_HEAD]->KeyFrame;
        RtQuatRotate(&headFrame->q, &xAxis, RandomAngle(), rwCOMBINEPOSTCONCAT);
        RtQuatRotate(&headFrame->q, &yAxis, RandomAngle(), rwCOMBINEPOSTCONCAT);
        rider->bUpdateAnimHeading = true;
    }
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
    const auto& mat     = GetMatrix();
    const auto  colData = GetColModel()->m_pColData;
    const auto& mi      = *GetVehicleModelInfo();
    const auto  dt      = CTimer::GetTimeStep();

    uint32 extraHandlingFlags = 0;

    m_vehicleAudio.Service();
    vehicleFlags.bWarnedPeds        = false;
    m_bLeanMatrixCalculated         = false;
    m_nBrakesOn                     = 0;
    nBikeFlags                     &= ~0x20; // bPlayerBoost
    vehicleFlags.bRestingOnPhysical = false;

    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    ProcessCarAlarm();
    ActivateBombWhenEntered();
    UpdateClumpAlpha();

    //> Keep track of whether a player is riding this bike (0 -> 1 -> 2)
    if (!m_pDriver || (!m_pDriver->IsPlayer() && (!m_apPassengers[0] || !m_apPassengers[0]->IsPlayer()))) {
        m_nTestPedCollision = 0;
    } else if (m_nTestPedCollision == 1) {
        m_nTestPedCollision = 2;
    } else if (m_nTestPedCollision == 0) {
        m_nTestPedCollision = 1;
    }

    ProcessAI(extraHandlingFlags);

    const auto status = GetStatus();
    if (status == STATUS_SIMPLE) {
        return;
    }

    if ((nBikeFlags & 0x10) && (std::abs(GetRight().z) > 0.35f || std::abs(GetUp().z) > 0.5f)) {
        nBikeFlags &= ~0x10; // bOnSideStand
    }

    //> Center of mass (and lean damping)
    if (!(extraHandlingFlags & EXTRA_HANDLING_NITROS) && !(nBikeFlags & 0x18)) {
        m_vecCentreOfMass = CVector{ m_pHandlingData->m_vecCentreOfMass.x, m_pHandlingData->m_vecCentreOfMass.y, m_BikeHandling->m_fNoPlayerCOMz };
    } else {
        float leanDamp   = 0.9995f;
        float turnDamp   = 0.9f;
        float dampScaleX = 1.0f;

        const auto angVelObj = mat.InverseTransformVector(m_vecTurnSpeed); // Angular velocity in object space

        if (status == STATUS_PLAYER) {
            if (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f) {
                if (m_WheelCounts[2] <= 0.0f && m_WheelCounts[3] <= 0.0f) {
                    dampScaleX = 100.0f;
                    const auto mu1 = CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.075f;
                    const auto mu2 = CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.25f;
                    if (GetUp().z < 0.0f) {
                        leanDamp *= std::min(std::abs(m_BikeHandling->m_fStoppieAng - GetUp().z) * mu2, mu1) + 0.9f;
                    }
                }
            } else {
                dampScaleX = 100.0f;
                if (m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f) {
                    if (GetUp().z > 0.0f) {
                        leanDamp -= std::min(std::abs(m_BikeHandling->m_fWheelieAng - GetUp().z) * (CStats::GetFatAndMuscleModifier(STAT_MOD_13) * 0.2f), 0.05f);
                    } else {
                        leanDamp = 0.98f;
                    }
                } else {
                    leanDamp = 0.98f;
                }
            }
        }

        leanDamp = std::pow(leanDamp / (sq(angVelObj.x) * dampScaleX + 1.0f), dt);
        turnDamp = std::pow(turnDamp / (sq(angVelObj.y) * 1000.0f + 1.0f), dt);

        const auto deltaX  = angVelObj.x * leanDamp - angVelObj.x;
        const auto deltaY  = angVelObj.y * turnDamp - angVelObj.y;
        const auto comRot  = mat.TransformVector(m_vecCentreOfMass);
        ApplyTurnForce(GetUp() * (-deltaY * m_fTurnMass), comRot + GetRight());
        ApplyTurnForce(GetUp() * (deltaX * m_fTurnMass),  comRot + GetForward());

        if (status != STATUS_PLAYER) {
            m_vecCentreOfMass = m_pHandlingData->m_vecCentreOfMass;
        }
    }

    bool bSkipPhysics = false;

    //> Settle wrecked/abandoned bikes that have come to rest
    if (!m_bIsStuck && (status == STATUS_ABANDONED || status == STATUS_WRECKED) && !(nBikeFlags & 8)) {
        const bool bIsAtRest = !vehicleFlags.bVehicleColProcessed
                            && m_vecMoveSpeed.x == 0.0f && m_vecMoveSpeed.y == 0.0f && m_vecMoveSpeed.z == 0.0f
                            && m_WheelCounts[3] != 1.0f;

        float maxForce, maxTorque, maxMovingSpeed;
        if (status == STATUS_WRECKED) {
            maxForce       = 0.006f;
            maxTorque      = 0.0015f;
            maxMovingSpeed = 0.015f;
        } else {
            maxForce       = 0.003f;
            maxTorque      = 0.0009f;
            maxMovingSpeed = 0.005f;
        }

        m_vecForce  = (m_vecForce + m_vecMoveSpeed) * 0.5f;
        m_vecTorque = (m_vecTorque + m_vecTurnSpeed) * 0.5f;

        if ((   m_vecForce.SquaredMagnitude()  <= sq(maxForce  * dt)
             && m_vecTorque.SquaredMagnitude() <= sq(maxTorque * dt)
             && m_fMovingSpeed < maxMovingSpeed
            ) || bIsAtRest
        ) {
            ++m_nFakePhysics;
            if (m_nFakePhysics > 10 || bIsAtRest) {
                if (!bIsAtRest || m_nFakePhysics > 10) {
                    m_nFakePhysics = 10;
                }
                m_vecMoveSpeed = CVector{};
                m_vecTurnSpeed = CVector{};
                bSkipPhysics   = true;
            }
        } else {
            m_nFakePhysics = 0;
        }
    }

    //> Anything we're resting on?
    for (const auto& ground : m_aGroundPhysicalPtrs) {
        if (ground) {
            vehicleFlags.bRestingOnPhysical = true;
            if (CWorld::bForceProcessControl && (ground->m_nFlags & 0x20)) { // m_bIsInSafePosition
                m_bWasPostponed = true;
                return;
            }
        }
    }

    if (vehicleFlags.bRestingOnPhysical) {
        bSkipPhysics   = false;
        m_nFakePhysics = 0;
    }

    VehicleDamage(0.0f, eVehicleCollisionComponent::DEFAULT, nullptr, nullptr, nullptr, WEAPON_RAMMEDBYCAR);

    const bool bSlidingSideways = (   m_fDamageIntensity > 0.0f
                                   && std::abs(DotProduct(m_vecLastCollisionImpactVelocity, GetRight())) > 0.5f
                                   && m_vecMoveSpeed.SquaredMagnitude() < 0.1f
                                  )
                               || (nBikeFlags & 8);

    //> Contact with the ground
    if (!bSkipPhysics) {
        if (!vehicleFlags.bVehicleColProcessed) {
            ProcessControlCollisionCheck(true);
        }

        if (!(extraHandlingFlags & EXTRA_HANDLING_NITROS) && !(nBikeFlags & 0x18)) {
            if (GetRight().z >= 0.0f) {
                if (m_fSteerAngle < 0.43633232f) {
                    m_fSteerAngle += dt * 0.008726646f;
                }
            } else if (m_fSteerAngle > -0.43633232f) {
                m_fSteerAngle -= dt * 0.008726646f;
            }
        }

        const auto savedAirResistance = m_fAirResistance;

        //> Rider leaning forward -> less air resistance (and a small boost while accelerating)
        if (status == STATUS_PLAYER && m_pDriver) {
            if (const auto anim = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_FWD)) {
                if (anim->m_BlendAmount > 0.5f && anim->m_CurrentTime > 0.06f && anim->m_CurrentTime < 0.14f) {
                    m_fAirResistance *= CCullZones::DoExtraAirResistanceForPlayer() ? 0.85f : 0.6f;
                    if (m_GasPedal > 0.5f && DotProduct(m_vecMoveSpeed, GetForward()) > 0.25f) {
                        ApplyMoveForce(GetForward() * (dt * m_fMass * 0.0016f));
                        nBikeFlags |= 0x20; // bPlayerBoost
                    }
                }
            }
        }

        const bool wasSubmerged = physicalFlags.bSubmergedInWater;

        CPhysical::ProcessControl();

        m_fAirResistance = savedAirResistance;

        ProcessBuoyancy();

        if (!wasSubmerged && physicalFlags.bSubmergedInWater) {
            if (!m_pDriver || !m_pDriver->IsPlayer()) {
                for (auto i = 0u; i < m_nMaxPassengers; i++) {
                    if (m_apPassengers[i] && m_apPassengers[i]->IsPlayer()) {
                        m_apPassengers[i]->AsPlayer()->ResetPlayerBreath();
                    }
                }
            } else {
                m_pDriver->AsPlayer()->ResetPlayerBreath();
            }
        }

        //> Suspension compression
        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            const auto offset = 1.0f - m_fSuspensionLength[i] / m_fLineLength[i];
            m_aWheelRatios[i] = (m_aWheelRatios[i] - offset) / (1.0f - offset);
        }

        CVector wheelPos[NUM_SUSP_LINES]{};  // Relative to the bike's position
        CVector wheelNorm[NUM_SUSP_LINES]{}; // Suspension line direction (world space)
        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                wheelPos[i]  = m_aWheelColPoints[i].m_vecPoint - GetPosition();
                const auto& line = colData->m_pLines[i];
                wheelNorm[i] = mat.TransformVector(line.m_vecEnd - line.m_vecStart).Normalized();
            }
        }

        float springForceDampingLimit[NUM_SUSP_LINES]{};
        m_aWheelSkidmarkType = { eSkidmarkType::DEFAULT, eSkidmarkType::DEFAULT };
        m_bMoreSkidMarks     = { false, false };

        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            auto& colPoint = m_aWheelColPoints[i];
            if (m_aWheelRatios[i] >= 1.0f) {
                //! Not touching the ground, so just put the wheel at the end of its suspension line
                wheelPos[i] = mat.TransformVector(colData->m_pLines[i].m_vecEnd);
            } else {
                auto bias = m_pHandlingData->m_fSuspensionBiasBetweenFrontAndRear;
                if (i >= 2) { // Rear wheels
                    bias = 1.0f - bias;
                }

                if (colPoint.m_vecNormal.z <= 0.35f) {
                    ApplySpringCollision(m_pHandlingData->m_fSuspensionForceLevel, wheelNorm[i], wheelPos[i], m_aWheelRatios[i], bias, springForceDampingLimit[i]);
                } else {
                    ApplySpringCollisionAlt(m_pHandlingData->m_fSuspensionForceLevel, wheelNorm[i], wheelPos[i], m_aWheelRatios[i], bias, colPoint.m_vecNormal, springForceDampingLimit[i]);
                }

                const auto skidmarkType = g_surfaceInfos.GetSkidmarkType(colPoint.m_nSurfaceTypeB);
                const auto skidmarkIdx  = i >= 2 ? 1 : 0;
                m_aWheelSkidmarkType[skidmarkIdx] = (eSkidmarkType)skidmarkType;
                if (skidmarkType == 2) {
                    m_bMoreSkidMarks[skidmarkIdx] = true;
                }
            }
        }

        //> Speed of the wheel contact points
        CVector wheelContactSpeed[NUM_SUSP_LINES];
        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            wheelContactSpeed[i] = GetSpeed(wheelPos[i]);
            if (m_aGroundPhysicalPtrs[i]) {
                wheelContactSpeed[i] -= GetSpeed(m_aGroundOffsets[i]);
            }
        }

        //> Use the wheel collision normals as suspension directions when available
        if (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f) {
            const auto& frontColPoint = m_aWheelColPoints[m_aWheelRatios[0] >= 1.0f ? 1 : 0];
            if (frontColPoint.m_vecNormal.z > 0.35f) {
                wheelNorm[0] = -frontColPoint.m_vecNormal;
            }

            const auto& frontColPoint2 = m_aWheelColPoints[m_aWheelRatios[1] >= 1.0f ? 0 : 1];
            if (frontColPoint2.m_vecNormal.z > 0.35f) {
                wheelNorm[1] = -frontColPoint2.m_vecNormal;
            }
        }
        if (m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f) {
            const auto& rearColPoint = m_aWheelColPoints[m_aWheelRatios[2] >= 1.0f ? 3 : 2];
            if (rearColPoint.m_vecNormal.z > 0.35f) {
                wheelNorm[2] = -rearColPoint.m_vecNormal;
            }

            const auto& rearColPoint2 = m_aWheelColPoints[m_aWheelRatios[3] >= 1.0f ? 2 : 3];
            if (rearColPoint2.m_vecNormal.z > 0.35f) {
                wheelNorm[3] = -rearColPoint2.m_vecNormal;
            }
        }

        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            if (m_aWheelRatios[i] < 1.0f) {
                ApplySpringDampening(m_pHandlingData->m_fSuspensionDampingLevel, springForceDampingLimit[i], wheelNorm[i], wheelPos[i], wheelContactSpeed[i]);
            }
        }

        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            wheelContactSpeed[i] = GetSpeed(wheelPos[i]);
            if (m_aGroundPhysicalPtrs[i]) {
                wheelContactSpeed[i] -= GetSpeed(m_aGroundOffsets[i]);
            }
        }

        const auto forwardSpeed = DotProduct(m_vecMoveSpeed, GetForward());

        float velocity = forwardSpeed;
        const auto driveAcceleration = m_pHandlingData->GetTransmission().CalculateDriveAcceleration(
            m_GasPedal,
            m_nCurrentGear,
            m_fGearChangeCount,
            velocity,
            nullptr,
            nullptr,
            m_NumDriveWheelsOnGround,
            ms_nHandlingCheat
        );
        const auto thrust = driveAcceleration / m_fVelocityFrequency;

        auto brakeThrust = m_pHandlingData->m_fBrakeDeceleration * m_BrakePedal * dt;

        float brakeBias, brakeBias2, tractionBias, tractionBias2;
        if (   status == STATUS_PLAYER
            || status == STATUS_REMOTE_CONTROLLED
            || !(m_pHandlingData->m_nHandlingFlags & VEHICLE_HANDLING_NPC_NEUTRAL_HANDL)
        ) {
            brakeBias    = m_pHandlingData->m_fBrakeBias * 2.0f;
            brakeBias2   = (1.0f - m_pHandlingData->m_fBrakeBias) * 2.0f;
            tractionBias = m_pHandlingData->m_fTractionBias * 2.0f;
            tractionBias2 = 2.0f - tractionBias;
        } else {
            brakeBias     = 1.0f;
            brakeBias2    = 1.0f;
            tractionBias  = 1.0f;
            tractionBias2 = 1.0f;
        }
        //! Note: `brakeBias`/`tractionBias` are the front values (`brakeBias2`/`tractionBias2` the rear ones)

        m_NumDriveWheelsOnGroundLastFrame = m_NumDriveWheelsOnGround;
        m_nNoOfContactWheels               = 0;
        m_NumDriveWheelsOnGround           = 0;

        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            bool bInContact;
            if (m_aWheelRatios[i] >= 1.0f) {
                m_WheelCounts[i] = std::max(m_WheelCounts[i] - dt, 0.0f);
                bInContact       = m_WheelCounts[i] > 0.0f;
            } else {
                m_WheelCounts[i] = 4.0f;
                bInContact       = true;
            }

            if (bInContact) {
                ++m_nNoOfContactWheels;
                if (i >= 2) { // Rear wheels
                    m_NumDriveWheelsOnGround = 1;
                }
                if (m_nNoOfContactWheels == 1) {
                    m_vecAveGroundNormal = m_aWheelColPoints[i].m_vecNormal;
                } else {
                    m_vecAveGroundNormal += m_aWheelColPoints[i].m_vecNormal;
                }
            }
        }

        if (m_nNoOfContactWheels == 0) {
            m_vecAveGroundNormal = CVector{ 0.0f, 0.0f, 1.0f };
        } else {
            m_vecAveGroundNormal *= 1.0f / (float)m_nNoOfContactWheels;
            if (DotProduct(GetUp(), m_vecAveGroundNormal) < -0.5f) {
                m_vecAveGroundNormal = -m_vecAveGroundNormal;
            }
        }

        //> Pick the wheels that are actually on the ground
        const auto frontWheel = m_aWheelRatios[0] > m_aWheelRatios[1] ? 1 : 0;
        const auto rearWheel  = m_aWheelRatios[2] > m_aWheelRatios[3] ? 3 : 2;

        auto frontContactPoint = mat.TransformVector({
            0.0f,
            colData->m_pLines[0].m_vecStart.y,
            colData->m_pLines[0].m_vecStart.z - m_aWheelRatios[frontWheel] * m_fSuspensionLength[0] - mi.m_fWheelSizeFront * 0.5f
        });
        auto rearContactPoint = mat.TransformVector({
            0.0f,
            colData->m_pLines[3].m_vecStart.y,
            colData->m_pLines[2].m_vecStart.z - m_aWheelRatios[rearWheel] * m_fSuspensionLength[2] - mi.m_fWheelSizeRear * 0.5f
        });

        //> Traction multiplier (in handling units)
        const auto tractionMult = m_pHandlingData->m_fTractionMultiplier * m_fExtraTractionMult * ms_fBikeTractionScale * 0.25f;

        //> Wheel steering (bar) angle
        if (status == STATUS_PLAYER || !(nBikeFlags & 0x10) || (nBikeFlags & 8)) {
            if (std::abs(m_vecMoveSpeed.x) < 0.01f && std::abs(m_vecMoveSpeed.y) < 0.01f && m_fSteerAngle == 0.0f) {
                m_RideAnimData.BarSteerAngle = std::pow(0.96f, dt) * m_RideAnimData.BarSteerAngle;
            } else {
                float steerScale;
                if (forwardSpeed <= 0.01f || (m_WheelCounts[0] <= 0.0f && m_WheelCounts[1] <= 0.0f) || status != STATUS_PLAYER) {
                    steerScale = 1.0f;
                } else {
                    CColPoint tmpColPoint{};
                    tmpColPoint.m_nSurfaceTypeA = SURFACE_WHEELBASE;
                    tmpColPoint.m_nSurfaceTypeB = SURFACE_TARMAC;

                    auto adhesive = g_surfaceInfos.GetAdhesiveLimit(&tmpColPoint) * m_BikeHandling->m_fSpeedSteer * tractionMult * 4.0f;

                    const auto adhesionGroup = g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[rearWheel].m_nSurfaceTypeB);
                    if (adhesionGroup == 3 || adhesionGroup == 4) {
                        adhesive *= m_BikeHandling->m_fSlipSteer;
                    }

                    steerScale = std::min(adhesive / sq(forwardSpeed), 1.0f);
                    steerScale = std::asin(steerScale) / (m_pHandlingData->m_fSteeringLock * 0.017453292f);
                    if (   (m_fSteerAngle < 0.0f && m_RideAnimData.LeanAngle < 0.0f)
                        || (m_fSteerAngle > 0.0f && m_RideAnimData.LeanAngle > 0.0f)
                    ) {
                        steerScale *= 2.0f;
                    }
                    if (steerScale > 1.0f) {
                        steerScale = 1.0f;
                    }
                }
                if (status != STATUS_PLAYER) {
                    steerScale = 1.0f;
                }
                m_RideAnimData.BarSteerAngle = steerScale * m_fSteerAngle;
            }
        } else if (m_RideAnimData.BarSteerAngle < 0.34906587f) {
            m_RideAnimData.BarSteerAngle += dt * 0.02617994f;
        }

        //> Wheel FWD/RIGHT directions (rotated by the bar steer angle), orthogonalized against the ground
        const auto ComputeWheelDirs = [&](uint32 wheel, CVector& outFwd, CVector& outRight) {
            const auto dir = mat.TransformVector({ -std::sin(m_RideAnimData.BarSteerAngle), std::cos(m_RideAnimData.BarSteerAngle), 0.0f });
            const auto& normal = m_aWheelColPoints[wheel].m_vecNormal;

            outFwd   = (dir - normal * DotProduct(dir, normal)).Normalized();
            outRight = CrossProduct(outFwd, normal).Normalized();
            if (bSlidingSideways) {
                outRight.z = 0.0f;
            }
        };

        const auto ProcessFrontWheel = [&, this] {
            if (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) {
                CVector wheelFwd, wheelRight;
                ComputeWheelDirs(frontWheel, wheelFwd, wheelRight);

                ms_fWheelThrust = 0.0f;
                m_aWheelColPoints[frontWheel].m_nSurfaceTypeA = SURFACE_WHEELBASE;

                auto adhesion = g_surfaceInfos.GetAdhesiveLimit(&m_aWheelColPoints[frontWheel]) * tractionMult;

                auto destabTraction = 1.0f;
                if (m_fBrakingSlide > 0.0f) {
                    switch (g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[frontWheel].m_nSurfaceTypeB)) {
                    case ADHESION_GROUP_WET:
                    case ADHESION_GROUP_LOOSE:
                        destabTraction = 0.9f;
                        break;
                    case ADHESION_GROUP_SAND:
                        destabTraction = 0.7f;
                        break;
                    }
                }

                if (status == STATUS_PLAYER) {
                    adhesion *= g_surfaceInfos.GetWetMultiplier(m_aWheelColPoints[frontWheel].m_nSurfaceTypeB);
                }
                if (m_nWheelStatus[0] == WHEEL_STATUS_BURST) {
                    adhesion *= 0.4f;
                }

                ms_FrontWheelState = m_WheelStates[0];

                auto contactSpeed = GetSpeed(frontContactPoint);
                if (m_aGroundPhysicalPtrs[frontWheel]) {
                    contactSpeed -= GetSpeed(m_aGroundOffsets[frontWheel]);
                }

                ProcessBikeWheel(
                    wheelFwd, wheelRight, contactSpeed, frontContactPoint,
                    2,
                    ms_fWheelThrust,
                    brakeBias * brakeThrust,
                    adhesion * tractionBias,
                    destabTraction,
                    0,
                    &m_aWheelAngularVelocity[0],
                    &ms_FrontWheelState,
                    BIKE_WHEEL_F_STD,
                    m_nWheelStatus[0]
                );

                if ((extraHandlingFlags & 4) && (ms_FrontWheelState == WHEEL_STATE_SPINNING || ms_FrontWheelState == WHEEL_STATE_SKIDDING)) {
                    ms_FrontWheelState = WHEEL_STATE_NORMAL;
                }
            } else {
                const auto angularVelocity = m_aWheelAngularVelocity[0] * 0.95f;
                m_aWheelAngularVelocity[0] = angularVelocity;
                m_aWheelPitchAngles[0]    += angularVelocity * dt;
            }
        };

        const auto ProcessRearWheel = [&, this] {
            if (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f) {
                CVector wheelFwd, wheelRight;
                ComputeWheelDirs(rearWheel, wheelFwd, wheelRight);

                auto rearBrake      = brakeThrust;
                auto destabTraction = tractionMult;

                if (!vehicleFlags.bIsHandbrakeOn) {
                    if (m_nBrakesOn == 0) {
                        if (m_fTyreTemp >= 1.0f && m_GasPedal > 0.75f) {
                            destabTraction = tractionMult * m_fTyreTemp;

                            const auto f = (1.0f - m_fTyreTemp) * m_fSteerAngle * m_fTurnMass * -0.0007f;
                            ApplyTurnForce(GetRight() * (f * dt), wheelPos[2]);
                        }
                    } else {
                        rearBrake      = 0.0f;
                        destabTraction = 0.0f;

                        const auto f = m_fSteerAngle * m_fTurnMass * -0.0007f;
                        ApplyTurnForce(GetRight() * (f * dt), wheelPos[2]);
                    }
                } else {
                    rearBrake    = 20000.0f;
                    m_fTyreTemp  = 1.0f;
                }

                if (ms_fWheelThrust > 0.0f && rearBrake > 0.0f) {
                    rearBrake = 0.0f;
                }
                ms_fWheelThrust = thrust;

                m_aWheelColPoints[rearWheel].m_nSurfaceTypeA = SURFACE_WHEELBASE;

                auto adhesion = g_surfaceInfos.GetAdhesiveLimit(&m_aWheelColPoints[rearWheel]) * destabTraction;

                destabTraction = 1.0f;
                if (m_fBrakingSlide > 0.0f) {
                    switch (g_surfaceInfos.GetAdhesionGroup(m_aWheelColPoints[rearWheel].m_nSurfaceTypeB)) {
                    case ADHESION_GROUP_WET:
                    case ADHESION_GROUP_LOOSE:
                        destabTraction = 0.9f;
                        break;
                    case ADHESION_GROUP_SAND:
                        destabTraction = 0.7f;
                        break;
                    }
                }

                if (status == STATUS_PLAYER) {
                    adhesion *= g_surfaceInfos.GetWetMultiplier(m_aWheelColPoints[rearWheel].m_nSurfaceTypeB);
                }
                if (m_nWheelStatus[1] == WHEEL_STATUS_BURST) {
                    adhesion *= 0.4f;
                }

                ms_RearWheelState = m_WheelStates[1];

                auto contactSpeed = GetSpeed(rearContactPoint);
                if (m_aGroundPhysicalPtrs[rearWheel]) {
                    contactSpeed -= GetSpeed(m_aGroundOffsets[rearWheel]);
                }

                ProcessBikeWheel(
                    wheelFwd, wheelRight, contactSpeed, rearContactPoint,
                    2,
                    ms_fWheelThrust,
                    rearBrake * brakeBias2,
                    adhesion * tractionBias2,
                    destabTraction,
                    1,
                    &m_aWheelAngularVelocity[1],
                    &ms_RearWheelState,
                    BIKE_WHEEL_R_STD,
                    m_nWheelStatus[1]
                );

                if ((extraHandlingFlags & 4) && (ms_RearWheelState == WHEEL_STATE_SPINNING || ms_RearWheelState == WHEEL_STATE_SKIDDING)) {
                    ms_RearWheelState = WHEEL_STATE_NORMAL;
                }
            } else {
                if (!vehicleFlags.bIsHandbrakeOn) {
                    if (thrust != 0.0f) {
                        if (thrust <= 0.0f) {
                            if (m_aWheelAngularVelocity[1] > -1.0f) {
                                m_aWheelAngularVelocity[1] += 0.05f;
                            }
                        } else if (m_aWheelAngularVelocity[1] < 1.0f) {
                            m_aWheelAngularVelocity[1] -= 0.1f;
                        }
                    }
                } else {
                    m_aWheelAngularVelocity[1] = 0.0f;
                }
                m_aWheelPitchAngles[1] += dt * m_aWheelAngularVelocity[1];
            }
        };

        const bool bProcessRearFirst = m_pHandlingData->m_nHandlingFlags & VEHICLE_HANDLING_PROC_REARWHEEL_1ST;
        if (!bProcessRearFirst) {
            ProcessFrontWheel();
        }
        ProcessRearWheel();
        if (bProcessRearFirst) {
            ProcessFrontWheel();
        }

        if (m_nBrakesOn == 0 || m_WheelStates[1] != WHEEL_STATE_SPINNING) {
            if (m_fTyreTemp < 1.0f) {
                m_fTyreTemp += dt * 0.005f;
            }
        } else {
            m_fTyreTemp = std::max(m_fTyreTemp - dt * 0.002f, 0.0f);
        }

        for (auto& ground : m_aGroundPhysicalPtrs) {
            ground = nullptr;
        }

        //> Average ground normal (when sliding sideways, align it with the bike)
        float tireLean = 0.0f;
        if (m_pDriver) {
            if (const auto anim = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_STILL)) {
                tireLean = anim->m_BlendAmount * 0.17453294f;
            }
        }

        if (bSlidingSideways) {
            m_vecAveGroundNormal = CVector{ 0.0f, 0.0f, 1.0f };
            const auto tmp = GetForward().Cross(m_vecAveGroundNormal).Normalized();
            m_vecAveGroundNormal = tmp.Cross(GetForward()).Normalized();
        }

        //> Desired lean angle
        if (!(extraHandlingFlags & EXTRA_HANDLING_NITROS) && !(nBikeFlags & 8)) {
            if (!(nBikeFlags & 0x10)) { // Not on the side stand
                m_RideAnimData.DesiredLeanAngle *= std::pow(0.95f, dt);
            } else {
                const auto k = std::pow(0.97f, dt);
                m_RideAnimData.DesiredLeanAngle = k * m_RideAnimData.DesiredLeanAngle
                                                - (1.0f - k) * (std::asin(std::clamp(GetRight().z, -1.0f, 1.0f)) + tireLean + 0.2617994f);
            }
        } else {
            m_vecGroundRight = GetForward().Cross(m_vecAveGroundNormal).Normalized();

            //> How much we're sliding sideways (converted to a rate)
            float leanTarget;
            if (m_pAttachedTo) {
                leanTarget = 0.0f;
            } else if (m_nNoOfContactWheels == 0) {
                leanTarget = (m_fSteerAngle / (m_pHandlingData->m_fSteeringLock * 0.017453292f)) * dt * -0.004f;
            } else {
                leanTarget = DotProduct(m_vecMoveSpeed - m_vecOldSpeedForPlayback, m_vecGroundRight);
                if (physicalFlags.bDisableCollisionForce) {
                    m_vecOldSpeedForPlayback = m_vecMoveSpeed;
                }
            }

            leanTarget /= std::max(dt, 0.01f) * 0.008f;

            const auto maxLean = m_nWheelStatus[0] == WHEEL_STATUS_BURST
                ? m_BikeHandling->m_fMaxLean * 0.4f
                : m_BikeHandling->m_fMaxLean;
            leanTarget = std::clamp(leanTarget, -maxLean, maxLean);

            const auto k = std::pow(m_BikeHandling->m_fDesLean, dt);
            m_RideAnimData.DesiredLeanAngle = k * m_RideAnimData.DesiredLeanAngle
                                            + (1.0f - k) * (std::asin(leanTarget) - tireLean);
        }
        m_RideAnimData.LeanAngle = m_RideAnimData.DesiredLeanAngle;

        m_WheelStates[0] = ms_FrontWheelState;
        m_WheelStates[1] = ms_RearWheelState;
        if (m_GasPedal < 0.0f && m_WheelStates[1] == WHEEL_STATE_SPINNING) {
            m_WheelStates[1] = WHEEL_STATE_NORMAL;
        }

        if (GetStatus() == STATUS_PLAYER) {
            ProcessSirenAndHorn(true);
        } else if (m_HornCounter) {
            --m_HornCounter;
        }
    } else {
        //> No physics this frame - just keep the bike on its side stand
        SkipPhysics();
        vehicleFlags.bVehicleColProcessed = false;
        vehicleFlags.bAudioChangingGear   = false;

        if (nBikeFlags & 0x10) { // bOnSideStand
            if (m_RideAnimData.BarSteerAngle < 0.34906587f) {
                m_RideAnimData.BarSteerAngle += dt * 0.017453292f;
            }

            const auto k = std::pow(0.97f, dt);
            m_RideAnimData.DesiredLeanAngle = k * m_RideAnimData.DesiredLeanAngle
                                            - (1.0f - k) * (std::asin(std::clamp(GetRight().z, -1.0f, 1.0f)) + 0.2617994f);
            m_RideAnimData.LeanAngle = m_RideAnimData.DesiredLeanAngle;
        }
    }

    //> Fire particle
    if (m_fHealth >= 250.0f || GetStatus() == STATUS_WRECKED) {
        m_BlowUpTimer = 0.0f;
        if (m_pFireParticle) {
            m_pFireParticle->Kill();
            m_pFireParticle = nullptr;
        }
    } else {
        if (m_nVehicleSubType != VEHICLE_TYPE_BMX) {
            if (!m_pFireParticle) {
                if (const auto modellingMat = GetModellingMatrix()) {
                    m_pFireParticle = g_fxMan.CreateFxSystem("fire_bike", mi.GetModelDummyPosition(DUMMY_ENGINE), modellingMat);
                    if (m_pFireParticle) {
                        m_pFireParticle->Play();
                        GetEventGlobalGroup()->Add(CEventVehicleOnFire{ this });
                    }
                }
            }
        }

        m_BlowUpTimer += (float)(uint32)(dt * 20.0f); // 0.02f * 1000.0f
        if (m_BlowUpTimer > 5000.0f) {
            BlowUpCar(m_Damager, false);
        }
    }

    ProcessDelayedExplosion();

    //> Landing shake
    float maxDeflection = 0.0f;
    float roughness     = 0.0f;
    {
        const auto speedSq = m_vecMoveSpeed.SquaredMagnitude();
        for (auto i = 0u; i < NUM_SUSP_LINES; i++) {
            const auto deflection = m_aRatioHistory[i] - m_aWheelRatios[i];
            if (   deflection > 0.3f
                && (i == 0 || i == 2)
                && speedSq > 0.04f
                && (GetStatus() == STATUS_PLAYER || GetStatus() == STATUS_PHYSICS)
                && deflection > maxDeflection
            ) {
                maxDeflection = deflection;
            }

            if (m_aWheelRatios[i] < 1.0f && GetStatus() == STATUS_PLAYER) {
                const auto surfaceRoughness = (float)(int)g_surfaceInfos.GetRoughness(m_aWheelColPoints[i].m_nSurfaceTypeB) * 0.1f;
                roughness = std::max(roughness, surfaceRoughness);
            }

            m_aRatioHistory[i] = m_aWheelRatios[i];
            m_aWheelRatios[i]  = 1.0f;
        }

        if ((CTimer::GetTimeInMS() & 0x7FF) > 800) {
            if (roughness >= 0.29f) {
                maxDeflection = 0.0f;
            }
            roughness = 0.0f;
        }

        if ((maxDeflection > 0.0f || roughness > 0.0f) && GetStatus() == STATUS_PLAYER && m_vecMoveSpeed.SquaredMagnitude() > 0.01f) {
            const auto speed = m_vecMoveSpeed.Magnitude();

            float strength;
            float durationScale;
            float strengthMax;
            if (maxDeflection > 0.0f) {
                strength       = (speed / m_fMass) * maxDeflection * 400000.0f + 100.0f;
                strengthMax    = 250.0f;
                durationScale  = 20000.0f;
            } else {
                strength       = (speed / m_fMass) * roughness * 400000.0f + 40.0f;
                strengthMax    = 150.0f;
                durationScale  = 5000.0f;
            }
            strength = std::min(strength, strengthMax);

            const auto duration = dt * durationScale / strength;
            CPad::GetPad(0)->StartShake((int16)duration, (uint8)(int)strength, 0);
        }
    }

    vehicleFlags.bVehicleColProcessed = false;
    vehicleFlags.bAudioChangingGear   = false;

    if (!vehicleFlags.bWarnedPeds) {
        CCarCtrl::ScanForPedDanger(this);
    }

    //> Stop the bike from sliding around when it should be standing still
    if (!physicalFlags.bDisableCollisionForce || !physicalFlags.bCollidable) {
        if (   !bSkipPhysics
            && (m_GasPedal == 0.0f || GetStatus() == STATUS_WRECKED)
            && std::abs(m_vecMoveSpeed.x) < 0.005f
            && std::abs(m_vecMoveSpeed.y) < 0.005f
            && std::abs(m_vecMoveSpeed.z) < 0.005f
            && (m_fDamageIntensity <= 0.0f || m_pDamageEntity != FindPlayerPed(-1))
        ) {
            m_vecTurnSpeed.z = 0.0f;
            m_vecMoveSpeed   = CVector{};
        }
    } else {
        m_vecMoveSpeed         = CVector{};
        m_vecTurnSpeed         = CVector{};
        m_vecFrictionMoveSpeed = CVector{};
        m_vecFrictionTurnSpeed = CVector{};
    }

    //> Extra lean forces (nitros/side stand)
    if (!(extraHandlingFlags & EXTRA_HANDLING_NITROS) && !(nBikeFlags & 0x18)) {
        return;
    }

    const auto leanAmount = std::clamp(DotProduct(GetRight(), m_vecAveGroundNormal), -1.0f, 1.0f);
    const auto comRot     = mat.TransformVector(m_vecCentreOfMass);
    const auto point      = comRot + GetRight();

    if (!(extraHandlingFlags & EXTRA_HANDLING_NITROS)) {
        ApplyTurnForce(GetUp() * (leanAmount * m_fTurnMass * -0.1f * dt), point);
    } else {
        ApplyTurnForce(GetUp() * (leanAmount * m_fTurnMass * -0.07f * dt), point);
        nBikeFlags &= ~0x10; // bOnSideStand
    }

    if (GetStatus() != STATUS_PLAYER) {
        return;
    }

    //> Wheelie
    if (   m_WheelCounts[0] <= 0.0f && m_WheelCounts[1] <= 0.0f
        && GetUp().z > 0.0f
        && (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f)
    ) {
        float pitch = m_BikeHandling->m_fWheelieAng - GetUp().z;
        if (pitch > 0.15f) {
            pitch = std::max(0.3f - pitch, 0.0f);
        } else if (pitch < -0.08f) {
            pitch = std::max(-0.14f - pitch, 0.0f);
        }

        const auto speed = std::min(m_vecMoveSpeed.Magnitude(), 0.1f);

        //> Stabilize the wheelie
        const auto stabilize = CStats::GetFatAndMuscleModifier(STAT_MOD_12) * (pitch * m_BikeHandling->m_fWheelieStabMult * speed) * m_fTurnMass * dt * 0.5f;
        ApplyTurnForce(GetUp() * stabilize, comRot + GetForward());

        //> Steer the wheelie
        const auto steer = m_BikeHandling->m_fWheelieSteer * m_RideAnimData.BarSteerAngle * m_fTurnMass * dt * 0.5f;
        ApplyTurnForce(GetRight() * steer, comRot + GetForward());

        //> Push the bike sideways
        const auto lateral = m_vecMoveSpeed.Magnitude() * m_fMass * m_BikeHandling->m_fWheelieSteer * m_RideAnimData.BarSteerAngle * dt * 0.01f;
        ApplyMoveForce(GetRight() * (lateral * m_vecMoveSpeed.Magnitude()));

        m_RideAnimData.LeanAngle += dt * m_RideAnimData.BarSteerAngle * -0.1f;
    } else {
        //> Stoppie
        if (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f || GetUp().z >= 0.0f || (m_WheelCounts[0] <= 0.0f && m_WheelCounts[1] <= 0.0f)) {
            return;
        }

        float pitch = m_BikeHandling->m_fStoppieAng - GetUp().z;
        if (pitch > 0.15f) {
            pitch = std::max(0.3f - pitch, 0.0f);
        } else if (pitch < -0.15f) {
            pitch = std::max(-0.3f - pitch, 0.0f);
        }

        const auto speed = std::min(m_vecMoveSpeed.Magnitude(), 0.1f);

        //> Stabilize the stoppie
        const auto stabilize = CStats::GetFatAndMuscleModifier(STAT_MOD_12) * (pitch * m_BikeHandling->m_fStoppieStabMult * speed) * m_fTurnMass * dt * 0.5f;
        ApplyTurnForce(GetUp() * stabilize, comRot + GetForward());

        //> Counter the lateral velocity
        const auto lateral = DotProduct(GetRight(), m_vecMoveSpeed) * m_fTurnMass * dt * 0.05f;
        const auto axis    = CrossProduct(CVector{ 0.0f, 0.0f, 1.0f }, GetRight()).Normalized();
        ApplyTurnForce(GetRight() * -lateral, -axis);

        m_RideAnimData.LeanAngle += dt * m_RideAnimData.BarSteerAngle * -0.1f;
    }
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
    if (!m_nFixRightHand && !m_nFixLeftHand) {
        return;
    }

    if (!m_aBikeNodes[BIKE_CHASSIS]) {
        return;
    }

    //> The world matrix of the chassis frame (`bikeMatrix * chassisLTM`)
    CMatrix chassisLTM;
    chassisLTM.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_CHASSIS]), false);

    CMatrix chassisWorld = GetMatrix();
    chassisWorld *= chassisLTM;

    const auto hier = GetAnimHierarchyFromSkinClump(rider->GetRpClump());

    //> The handlebar (hand rest) position, in the bike's object space
    CVector handRest = GetVehicleModelInfo()->m_pVehicleStruct->m_avDummyPos[DUMMY_HAND_REST];
    if (handRest.x == 0.0f && handRest.y == 0.0f && handRest.z == 0.0f) {
        // Some bike models have no `DUMMY_HAND_REST` - use their hardcoded position instead
        switch (m_nModelIndex) {
        case MODEL_BIKE:
            handRest = CVector{ 0.25f, 0.21f, 0.69f };
            break;
        case MODEL_MTBIKE:
        default:
            handRest = CVector{ 0.25f, 0.29f, 0.525f };
            break;
        }
    } else {
        // Transform the hand bone position into the chassis' space, and add it to the hand rest position
        const auto& handBoneMat = RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_R_HAND)];
        CMatrix     handMat{ const_cast<RwMatrix*>(&handBoneMat), false };
        handRest += chassisWorld.InverseTransformVector(handMat.TransformVector(ms_vecHandOffset));
    }

    //> Move the bones of an arm so the hand ends up at `target`, distributing the movement along the arm
    const auto FixArm = [&](const CVector& target, eBoneTag hand, eBoneTag finger, eBoneTag fingerTip, eBoneTag foreArm, eBoneTag upperArm, eBoneTag breast, float foreArmScale, float upperArmScale) {
        auto& handPos = RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, hand)].pos;
        const CVector delta = target - CVector{ handPos.x, handPos.y, handPos.z };

        const auto AddOffset = [&](eBoneTag bone, const CVector& offset) {
            auto& pos = RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, bone)].pos;
            pos.x += offset.x;
            pos.y += offset.y;
            pos.z += offset.z;
        };
        AddOffset(hand,       delta);
        AddOffset(finger,     delta);
        AddOffset(fingerTip,  delta);
        AddOffset(foreArm,    delta * foreArmScale);
        AddOffset(upperArm,   delta * upperArmScale);
        if (rider->IsPlayer()) {
            AddOffset(breast, delta * upperArmScale);
        }
    };

    if (m_nFixRightHand) {
        FixArm(chassisWorld.TransformPoint(handRest), BONE_R_HAND, BONE_R_FINGER, BONE_R_FINGER_01, BONE_R_FORE_ARM, BONE_R_UPPER_ARM, BONE_R_BREAST, 0.667f, 0.333f);
    }

    if (m_nFixLeftHand) {
        CVector handRestLeft = handRest;
        handRestLeft.x *= -1.0f; // The dummy is authored for the right hand
        FixArm(chassisWorld.TransformPoint(handRestLeft), BONE_L_HAND, BONE_L_FINGER, BONE_L_FINGER_01, BONE_L_FORE_ARM, BONE_L_UPPER_ARM, BONE_L_BREAST, 0.75f, 0.4f);
    }

    m_nFixLeftHand  = false;
    m_nFixRightHand = false;
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
    CVehicle::PreRender();

    const auto colModel = GetColModel();
    const auto colData  = colModel->m_pColData;

    if (vehicleFlags.bVehicleColProcessed) {
        DoBurstAndSoftGroundRatios();

        // Front suspension
        {
            const auto frac   = 1.0f - m_fSuspensionLength[0] / m_fLineLength[0];
            const auto ratio  = (std::min(m_aWheelRatios[0], m_aWheelRatios[1]) - frac) / (1.0f - frac);
            auto       radius = colData->m_pLines[0].m_vecStart.z;
            if (ratio > 0.0f) {
                radius -= ratio * m_fSuspensionLength[0];
            }
            m_aWheelSuspensionHeights[0] += (radius - m_aWheelSuspensionHeights[0]) * 0.75f;
        }

        // Rear suspension
        {
            const auto frac   = 1.0f - m_fSuspensionLength[2] / m_fLineLength[2];
            const auto ratio  = (std::min(m_aWheelRatios[2], m_aWheelRatios[3]) - frac) / (1.0f - frac);
            auto       radius = colData->m_pLines[2].m_vecStart.z;
            if (ratio > 0.0f) {
                radius -= ratio * m_fSuspensionLength[2];
            }
            m_aWheelSuspensionHeights[1] += (radius - m_aWheelSuspensionHeights[1]) * 0.75f;
        }
    }

    if (GetStatus() <= STATUS_PHYSICS) {
        const auto speedMagnitude = m_vecMoveSpeed.Magnitude();
        for (auto i = 0; i < 2; i++) {
            // Which of the two suspension lines of this wheel is on the ground
            const auto colPointIdx = i == 0
                ? (m_aRatioHistory[0] >= 1.0f && m_aRatioHistory[1] < 1.0f ? 1 : 0)
                : (m_aRatioHistory[3] >= 1.0f && m_aRatioHistory[2] < 1.0f ? 2 : 3);

            auto flags = (i == 0 && m_WheelStates[1] != WHEEL_STATE_FIXED) ? 4u : 0u;
            if (m_bWheelBloody[i]) {
                flags += 1;
            }
            if (m_bMoreSkidMarks[i]) {
                flags += 2;
            }

            // Lean the wheels sideways so they still line up with the (leaning) bike
            auto wheelPos = m_aWheelColPoints[colPointIdx].m_vecPoint
                          + m_matrix->GetRight() * (std::sin(m_RideAnimData.LeanAngle) * colModel->m_boundBox.m_vecMin.z * 0.8f);

            AddSingleWheelParticles(
                m_WheelStates[i],
                m_nWheelStatus[i],
                m_aRatioHistory[colPointIdx],
                speedMagnitude,
                &m_aWheelColPoints[colPointIdx],
                &wheelPos,
                m_RideAnimData.LeanAngle <= 0.0f ? 1.0f : -1.0f,
                i,
                (uint32)m_aWheelSkidmarkType[i],
                &m_bWheelBloody[i],
                flags
            );
        }
    }

    m_bLeanMatrixCalculated = false;
    CalculateLeanMatrix();

    //> Exhaust particles
    if (   vehicleFlags.bEngineOn
        && !m_pHandlingData->m_bNoExhaust
        && DotProduct(GetForward(), m_vecMoveSpeed) / ms_fExhaustSpeedDivisor < 130.0f
        && !vehicleFlags.bIsDrowning
    ) {
        AddExhaustParticles();
    }

    AddDamagedVehicleParticles();

    //> Police bike siren lights
    // TODO(re): The original flashes the two siren coronas (`(uint32)this + 0x15`/`0x16`, at
    //           `{±0.28f, 0.6f, 0.3f}`), alternating on `CTimer::m_snTimeInMilliseconds & 0x1ff < 0x100`,
    //           and adds a `CPointLights::AddLight` 1m in front and 0.5m above the bike, with an intensity
    //           derived from `*(float*)0xB7C4E4` (which isn't identified yet). Not established.

    DoVehicleLights(m_mLeanMatrix, (eVehicleLightsFlags)4);
    CShadows::StoreShadowForVehicle(this, VEH_SHD_BIKE);

    const auto mi = GetVehicleModelInfo();

    //> Wheel spin
    if (m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f) {
        const auto steerAngle = m_fSteerAngle;
        const auto wheelDir   = GetMatrix().TransformVector(CVector{ -std::sin(steerAngle), std::cos(steerAngle), 0.0f });
        const auto ratio      = std::min(m_aWheelRatios[0], m_aWheelRatios[1]);
        const auto wheelPos   = CVector{
            0.0f,
            (colData->m_pLines[0].m_vecStart.y + colData->m_pLines[1].m_vecStart.y) * 0.5f,
            colData->m_pLines[0].m_vecStart.z - ratio * m_fSuspensionLength[0] - mi->m_fWheelSizeFront * 0.5f
        };
        m_aWheelAngularVelocity[0] = ProcessWheelRotation(WHEEL_STATE_NORMAL, wheelDir, GetSpeed(wheelPos), mi->m_fWheelSizeFront * 0.5f);
        m_aWheelPitchAngles[0] += m_aWheelAngularVelocity[0] * CTimer::GetTimeStep();
    }
    if (m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f) {
        const auto ratio    = std::min(m_aWheelRatios[2], m_aWheelRatios[3]);
        const auto wheelPos = CVector{
            0.0f,
            (colData->m_pLines[2].m_vecStart.y + colData->m_pLines[3].m_vecStart.y) * 0.5f,
            colData->m_pLines[2].m_vecStart.z - ratio * m_fSuspensionLength[2] - mi->m_fWheelSizeRear * 0.5f
        };
        m_aWheelAngularVelocity[1] = ProcessWheelRotation(m_WheelStates[1], GetForward(), GetSpeed(wheelPos), mi->m_fWheelSizeRear * 0.5f);
        m_aWheelPitchAngles[1] += m_aWheelAngularVelocity[1] * CTimer::GetTimeStep();
    }

    CMatrix mat;

    // Front forks - Rotated by the steering
    if (m_aBikeNodes[BIKE_FORKS_FRONT]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_FORKS_FRONT]), false);
        const auto pos = mat.GetPosition();

        // Build the steering quaternion's matrix
        RwMatrix quatMat{};
        CMatrix  quatMtx{&quatMat, false};
        quatMtx.SetUnity();
        quatMtx.UpdateRW();

        const auto steerAngle = DegreesToRadians(mi->m_fBikeSteerAngle);
        CVector    steerAxis{ 0.0f, std::sin(steerAngle), -std::cos(steerAngle) };
        steerAxis.Normalise();

        CQuaternion quat;
        quat.Set(&steerAxis, -m_RideAnimData.BarSteerAngle);
        quat.Get(&quatMat);
        quatMtx.Update();

        mat.SetUnity();
        mat *= quatMtx;
        mat.GetPosition() += pos;
        mat.UpdateRW();

        // Handlebars - Follow the forks
        if (m_aBikeNodes[BIKE_HANDLEBARS]) {
            CMatrix matHandlebars;
            matHandlebars.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_HANDLEBARS]), false);
            const auto posHandlebars = matHandlebars.GetPosition();
            if (GetStatus() == STATUS_ABANDONED || GetStatus() == STATUS_WRECKED) {
                matHandlebars.SetUnity();
                matHandlebars *= quatMtx;
                matHandlebars.GetPosition() += posHandlebars;
            } else {
                matHandlebars.SetTranslate(posHandlebars);
            }
            matHandlebars.UpdateRW();
        }
    }

    // Rear forks - Rotated by the suspension
    if (m_aBikeNodes[BIKE_FORKS_REAR]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_FORKS_REAR]), false);
        const auto pos = mat.GetPosition();
        mat.SetRotate(-std::asin((m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1]) / m_fSwingArmLength), 0.0f, 0.0f);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // Front wheel
    if (m_aBikeNodes[BIKE_WHEEL_FRONT]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_WHEEL_FRONT]), false);
        auto pos = mat.GetPosition();
        pos.z = m_aWheelSuspensionHeights[0] - m_fForkZOffset;
        pos.y = (colData->m_pLines[0].m_vecStart.y + colData->m_pLines[1].m_vecStart.y) * 0.5f - m_fForkYOffset
              - (m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0]) * m_fSteerAngleTan;
        if (m_nWheelStatus[0] == 1) {
            const auto pitch = m_aWheelPitchAngles[0];
            mat.SetRotate(pitch, 0.0f, std::sin(pitch) * 0.05f);
        } else {
            mat.SetRotateX(m_aWheelPitchAngles[0]);
        }
        mat.GetPosition() += pos;
        mat.UpdateRW();

        // Mudguard - Follows the front wheel
        if (m_aBikeNodes[BIKE_MUDGUARD]) {
            CMatrix matMudguard;
            matMudguard.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_MUDGUARD]), false);
            matMudguard.GetPosition() = pos;
            matMudguard.UpdateRW();
        }
    }

    // Rear wheel
    if (m_aBikeNodes[BIKE_WHEEL_REAR]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_WHEEL_REAR]), false);
        auto pos = mat.GetPosition();
        if (m_nWheelStatus[1] == 1) {
            const auto pitch = m_aWheelPitchAngles[1];
            mat.SetRotate(pitch, 0.0f, std::sin(pitch) * 0.07f);
        } else {
            mat.SetRotateX(m_aWheelPitchAngles[1]);
        }
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // Chassis
    if (m_aBikeNodes[BIKE_CHASSIS]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BIKE_CHASSIS]), false);
        auto pos = mat.GetPosition();
        pos.z = (1.0f - std::cos(m_RideAnimData.LeanAngle)) * colModel->m_boundBox.m_vecMin.z * 0.9f;
        mat.SetRotateX(std::abs(m_RideAnimData.LeanAngle) * -0.05f);
        mat.RotateY(m_RideAnimData.LeanAngle);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }
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

