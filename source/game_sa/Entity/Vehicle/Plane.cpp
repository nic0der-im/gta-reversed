#include "StdInc.h"

#include "Plane.h"
#include "CarCtrl.h"
#include "FireManager.h"
#include "Shadows.h"
#include "Events/EventDanger.h"

auto& HARRIER_NOZZLE_ROTATERATE = StaticRef<float>(0x8D33DC);       // 25.0f
auto& PLANE_DAMAGE_WAVE_COUNTER_VAR = StaticRef<float>(0x8D33E0);   // 0.75f
auto& PLANE_DAMAGE_THRESHHOLD = StaticRef<float>(0x8D33E4);         // 500.0f
auto& PLANE_DAMAGE_SCALE_MASS = StaticRef<float>(0x8D33E8);         // 10000.0f
auto& PLANE_DAMAGE_DESTROY_THRESHHOLD = StaticRef<float>(0x8D33EC); // 5000.0f
auto& vecRCBaronGunPos = StaticRef<CVector>(0x8D33F0);            // <0.0f, 0.45f, 0.0f>

namespace {
    // 0x6C4130
    //! Sets the vehicle's rotation to a pure Z-rotation of `heading`, keeping its position.
    //! Not `CPlaceable::SetHeading` (0x43E0C0): this one uses the vehicle matrix' axis convention.
    void SetSpawnHeading(CPlaceable& placeable, float heading) {
        auto& mat = placeable.GetMatrix();

        const float sin = std::sin(heading);
        const float cos = std::cos(heading);

        mat.GetRight()   = CVector{  sin, -cos, 0.0f };
        mat.GetForward() = CVector{  cos,  sin, 0.0f };
        mat.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };
    }
}


void CPlane::InjectHooks() {
    RH_ScopedVirtualClass(CPlane, 0x871948, 71);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6C8E20);
    RH_ScopedInstall(InitPlaneGenerationAndRemoval, 0x6CAD90);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6C9140);
    RH_ScopedVMTInstall(BurstTyre, 0x6C9150);
    RH_ScopedVMTInstall(PreRender, 0x6C94A0);
    RH_ScopedVMTInstall(Render, 0x6CAB70);
    RH_ScopedInstall(IsAlreadyFlying, 0x6CAB90);
    RH_ScopedVMTInstall(Fix, 0x6CABB0);
    RH_ScopedVMTInstall(SetupDamageAfterLoad, 0x6CAC10);
    RH_ScopedInstall(SetGearUp, 0x6CAC20);
    RH_ScopedInstall(SetGearDown, 0x6CAC70);
    RH_ScopedVMTInstall(OpenDoor, 0x6CACB0);
    RH_ScopedVMTInstall(ProcessControl, 0x6C9260);
    RH_ScopedVMTInstall(ProcessControlInputs, 0x6CADD0);
    RH_ScopedVMTInstall(ProcessFlyingCarStuff, 0x6CB7C0);
    RH_ScopedVMTInstall(VehicleDamage, 0x6CC4B0);
    RH_ScopedInstall(CountPlanesAndHelis, 0x6CCA50);
    RH_ScopedInstall(AreWeInNoPlaneZone, 0x6CCAA0);
    RH_ScopedInstall(AreWeInNoBigPlaneZone, 0x6CCBB0);
    RH_ScopedInstall(SwitchAmbientPlanes, 0x6CCC50);
    RH_ScopedVMTInstall(BlowUpCar, 0x6CCCF0);
    RH_ScopedInstall(FindPlaneCreationCoors, 0x6CD090);
    RH_ScopedInstall(DoPlaneGenerationAndRemoval, 0x6CD2F0);
}

// 0x6C8E20
CPlane::CPlane(int32 modelIndex, eVehicleCreatedBy createdBy) : CAutomobile(modelIndex, createdBy, true) {
    m_nVehicleSubType = VEHICLE_TYPE_PLANE;

    m_fLeftRightSkid               = 0.0f;
    m_fSteeringUpDown              = 0.0f;
    m_fSteeringLeftRight           = 0.0f;
    m_fAccelerationBreakStatus     = 0.0f;
    m_fAccelerationBreakStatusPrev = 1.0f;
    m_fPropSpeed                   = 0.0f;
    field_9C8                      = 0.0f;
    m_LandingGearAngle             = 0.0f;
    m_StallCounter                 = 0;
    m_planeCreationHeading         = 0.0f;
    m_planeHeading                 = 0.0f;
    m_planeHeadingPrev             = 0.0f;
    m_maxAltitude                  = 15.0f;
    m_altitude                     = 25.0f;
    m_minAltitude                  = 20.0f;
    m_forwardZ                     = 0;
    m_nStartedFlyingTime           = 0;
    m_fSteeringFactor              = 0.0f;

    if (m_nModelIndex != MODEL_VORTEX)
        physicalFlags.bDontCollideWithFlyers = true;

    m_nExtendedRemovalRange = 255;
    vehicleFlags.bNeverUseSmallerRemovalRange = true;
    vehicleFlags.bIsBig = true;

    auto& leftDoor = m_doors[DOOR_LEFT_FRONT];
    switch (modelIndex) {
    case MODEL_HYDRA:
    case MODEL_RUSTLER:
    case MODEL_CROPDUST:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init((3.0f * PI) / 5.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        break;
    case MODEL_SHAMAL:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init(-((3.0f * PI) / 4.0f), 0.0f, DOOR_AXIS_Z, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_LF]), 0);
        break;
    case MODEL_NEVADA:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init(-TWO_PI / 5.0f, 0.0f, DOOR_AXIS_NEG_Y, DOOR_AXIS_Z, DOOR_EXTRA_BASED);
        break;
    case MODEL_VORTEX:
        if (m_panels[FRONT_LEFT_PANEL].m_nFrameId == (uint16)-1)
            m_panels[FRONT_LEFT_PANEL].SetPanel(PLANE_GEAR_L, 1, -0.25f);
        break;
    case MODEL_STUNT:
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        leftDoor.Init((3.0f * PI) / 5.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_LB]), 0);
        rwObjectSetFlags(GetFirstObject(m_aCarNodes[PLANE_WHEEL_RB]), 0);
        break;
    }

    CVector modelPos, localPos;
    for (auto wheelId = 0; wheelId < 4; wheelId++) {
        GetVehicleModelInfo()->GetWheelPosn(wheelId, modelPos, false);
        GetVehicleModelInfo()->GetWheelPosn(wheelId, localPos, true);
        m_wheelPosition[wheelId] = m_wheelPosition[wheelId] - modelPos.z + localPos.z;
    }

    m_planeDamageWave = 0;
    m_pGunParticles = nullptr;
    m_nFiringMultiplier = 16;
    field_9DC = 0;
    field_9E0 = 0;
    m_apJettrusParticles.fill(nullptr);

    m_pSmokeParticle = nullptr;

    if (m_nModelIndex == MODEL_HYDRA)
        m_wMiscComponentAngle = HARRIER_NOZZLE_ROTATE_LIMIT;

    m_bSmokeEjectorEnabled = false;
}

// 0x6C9160
CPlane::~CPlane() {
    if (m_pGunParticles) {
        for (auto i = 0; i < CVehicle::GetPlaneNumGuns(); i++) {
            if (auto& particle = m_pGunParticles[i]) {
                particle->Kill();
                g_fxMan.DestroyFxSystem(particle);
            }
        }
        delete[] m_pGunParticles;
        m_pGunParticles = nullptr;
    }

    for (auto particle : m_apJettrusParticles) {
        if (particle) {
            FxSystem_c::KillAndClear(particle);
        }
    }

    FxSystem_c::SafeKillAndClear(m_pSmokeParticle);

    m_vehicleAudio.Terminate();
}

// 0x6CAD90
void CPlane::InitPlaneGenerationAndRemoval() {
    GenPlane_Status = 0;
    GenPlane_LastTimeGenerated = 0;
    GenPlane_Active = true;
}

// 0x6CCCF0
void CPlane::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    // Ambient planes (and the RC Baron) don't explode right away - they're told to crash and burn
    if (   GetStatus() != STATUS_PLAYER
        && m_autoPilot.m_nCarMission != MISSION_PLANE_CRASH_AND_BURN
        && m_nModelIndex != MODEL_RCBARON
    ) {
        m_autoPilot.SetCarMission(MISSION_PLANE_CRASH_AND_BURN);
        m_fHealth = 0.0f;
        return;
    }

    if (damager == FindPlayerPed(-1) || damager == FindPlayerVehicle(-1, false)) {
        auto& playerInfo = FindPlayerInfo();
        playerInfo.m_nHavocCaused += 20;
        playerInfo.m_fCurrentChaseValue += 10.0f;
        CStats::IncrementStat(STAT_COST_OF_PROPERTY_DAMAGED, (float)CGeneral::GetRandomNumberInRange(4'000, 10'000));
    }

    if (GetStatus() == STATUS_PLAYER) {
        // Hide the occupants (the plane is about to explode)
        if (m_pDriver) {
            m_pDriver->bDontRender = true;
        }
        for (auto* const passenger : m_apPassengers) {
            if (passenger) {
                passenger->bDontRender = true;
            }
        }

        m_nFlags &= 0xFFFFFF7E; // Clears `m_bUsesCollision` and `m_bIsVisible`
        ResetMoveSpeed();
        ResetTurnSpeed();
    }

    SetStatus(STATUS_WRECKED);
    physicalFlags.bRenderScorched = true;
    m_nTimeWhenBlowedUp           = CTimer::GetTimeInMS();

    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), ATOMIC_PIPE_NO_EXTRA_PASSES);
    m_damageManager.FuckCarCompletely(false);

    if (m_nModelIndex != MODEL_RCBARON) {
        SetBumperDamage(FRONT_BUMPER, false);
        SetBumperDamage(REAR_BUMPER, false);
        for (auto door = 0; door < MAX_DOORS; door++) {
            SetDoorDamage(static_cast<eDoors>(door), false);
        }

        SpawnFlyingComponent(static_cast<eCarNodes>(PLANE_WHEEL_LF), 1);

        if (auto* const node = m_aCarNodes[PLANE_WHEEL_LF]) {
            if (auto* const atomic = GetFirstObject(node)) {
                rwObjectSetFlags(atomic, 0);
            }
        }
    }

    m_nBombOnBoard = 0;
    m_fHealth      = 0.0f;
    m_wBombTimer   = 0;

    TheCamera.CamShake(0.4f, GetPosition());
    KillPedsInVehicle();

    const bool wasAmbulanceOnDuty = vehicleFlags.bIsAmbulanceOnDuty;
    const bool wasFireTruckOnDuty = vehicleFlags.bIsFireTruckOnDuty;
    vehicleFlags.bIsAmbulanceOnDuty = false;
    vehicleFlags.bIsFireTruckOnDuty = false;
    vehicleFlags.bEngineOn          = false;
    vehicleFlags.bLightsOn          = false;
    vehicleFlags.bSirenOrAlarm      = false;
    m_nOverrideLights               = NO_CAR_LIGHT_OVERRIDE;
    m_bSmokeEjectorEnabled          = false;
    autoFlags.bTaxiLight            = false;

    if (wasAmbulanceOnDuty) {
        --CCarCtrl::NumAmbulancesOnDuty;
    }
    if (wasFireTruckOnDuty) {
        --CCarCtrl::NumFireTrucksOnDuty;
    }

    ChangeLawEnforcerState(false);
    gFireManager.StartFire(this, damager, 0.8f, true, 7000, 0);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);

    CExplosion::AddExplosion(
        this,
        damager,
        m_nModelIndex == MODEL_RCBARON ? EXPLOSION_RC_VEHICLE : EXPLOSION_AIRCRAFT,
        GetPosition(),
        0,
        1,
        -1.0f,
        0
    );
}


// 0x6CABB0
void CPlane::Fix() {
    m_damageManager.ResetDamageStatus();
    if (m_pHandlingData->m_bNoDoors) {
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_RIGHT_FRONT, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_LEFT_REAR, DAMSTATE_NOTPRESENT);
        m_damageManager.SetDoorStatus(DOOR_RIGHT_REAR, DAMSTATE_NOTPRESENT);
    }
    SetupDamageAfterLoad();
}

// 0x6CACB0
void CPlane::OpenDoor(CPed* ped, int32 componentId, eDoors door, float doorOpenRatio, bool playSound) {
    CAutomobile::OpenDoor(ped, componentId, door, doorOpenRatio, playSound);

    if (m_nModelIndex != MODEL_STUNT)
        return;

    // Unfinished code R*, which removed in Android
    if (false) // byte_C1CAFC
    {
        CMatrix matrix(RwFrameGetMatrix(m_aCarNodes[componentId]), false);
        const auto y = m_doors[door].m_angle - m_doors[door].m_prevAngle + matrix.GetPosition().y;
        matrix.SetTranslate({matrix.GetPosition().x, y, matrix.GetPosition().z});
        matrix.UpdateRW();
    }
}

// 0x6CAC10
void CPlane::SetupDamageAfterLoad() {
    vehicleFlags.bIsDamaged = false;
}

// 0x6CC4B0
void CPlane::VehicleDamage(float damageIntensity, eVehicleCollisionComponent component, CEntity* damager, CVector* vecCollisionCoors, CVector* vecCollisionDirection, eWeaponType weapon) {
    float destroyThreshold = PLANE_DAMAGE_DESTROY_THRESHHOLD;
    float collideThreshold = PLANE_DAMAGE_THRESHHOLD;
    float particleScale    = 0.333f;

    if (m_nModelIndex == MODEL_VORTEX) {
        CAutomobile::VehicleDamage(damageIntensity, component, damager, vecCollisionCoors, vecCollisionDirection, weapon);
        return;
    }

    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }
    if (   GetStatus() != STATUS_PLAYER
        && physicalFlags.bInvulnerable
        && m_pDamageEntity != FindPlayerPed(-1)
        && m_pDamageEntity != FindPlayerVehicle(-1, false)
    ) {
        return;
    }

    bool collisionDamage = false;

    if (damageIntensity == 0.0f) {
        // Fire/explosion damage: use the stored intensity and last collision position
        damageIntensity = m_fDamageIntensity;
        particleScale   = 1.0f;

        const float massScale = m_fMass / PLANE_DAMAGE_SCALE_MASS;
        collideThreshold = std::max(massScale, 1.0f) * PLANE_DAMAGE_THRESHHOLD;
        destroyThreshold = massScale * PLANE_DAMAGE_DESTROY_THRESHHOLD;

        vecCollisionCoors = &m_vecLastCollisionPosn;

        if (m_fDamageIntensity < 0.0f) {
            return;
        }
        if (physicalFlags.bCollisionProof) {
            return;
        }
        if (m_pDamageEntity && m_pDamageEntity->GetType() == ENTITY_TYPE_PED) {
            return;
        }
    } else {
        bool damagedByFireExplosionOrBullet = false;
        if (!CanVehicleBeDamaged(damager, weapon, damagedByFireExplosionOrBullet)) {
            return;
        }

        // Only explosives do collisional damage to planes
        if (   weapon != WEAPON_ROCKET
            && weapon != WEAPON_ROCKET_HS
            && weapon != WEAPON_FREEFALL_BOMB
            && weapon != WEAPON_EXPLOSION
        ) {
            return;
        }

        collisionDamage  = true;
        damageIntensity  = 0.0f;
    }

    float damage = damageIntensity * m_pHandlingData->m_fCollisionDamageMultiplier;

    const float healthBefore = m_fHealth;

    if (this == FindPlayerVehicle(-1, false)) {
        damage *= vehicleFlags.bTakeLessDamage ? (1.0f / 6.0f) : 0.5f;
    } else if (vehicleFlags.bTakeLessDamage) {
        damage *= (1.0f / 12.0f);
    } else if (m_pDamageEntity == FindPlayerVehicle(-1, false)) {
        damage *= (2.0f / 3.0f);
    } else {
        damage *= 0.25f;
    }

    m_fHealth -= damage;
    if (m_fHealth < 0.0f && healthBefore > 0.0f) {
        m_fHealth = 1.0f;
    }

    if (destroyThreshold < damageIntensity || (m_fHealth < 1.0f && healthBefore > 0.0f)) {
        BlowUpCar(this, false);
        return;
    }

    if (damageIntensity <= collideThreshold && !collisionDamage) {
        return;
    }
    if (GetStatus() == STATUS_WRECKED) {
        return;
    }
    if (m_fMass <= 1000.0f) {
        return;
    }

    // Impact point in model space
    const CVector impactPos = GetMatrix().InverseTransformVector(*vecCollisionCoors - GetPosition());

    // Find the closest component to the impact
    int32 best        = -1;
    float bestDist    = 1000.0f;
    for (int32 comp = 12; comp < 25; comp++) {
        const auto* const node = m_aCarNodes[comp];
        if (!node) {
            continue;
        }
        const float distSq = (RwFrameGetMatrix(node)->pos - impactPos).SquaredMagnitude();
        if (distSq < bestDist * bestDist) {
            bestDist = std::sqrt(distSq);
            best     = comp;
        }
    }

    if (best < 0) {
        return;
    }

    // The rudder is sometimes hit as one of the elevators instead
    if (best == 16 && (CGeneral::GetRandomNumber() & 1)) {
        if (m_aCarNodes[17] && (CGeneral::GetRandomNumber() & 1)) {
            best = 17;
        } else if (m_aCarNodes[18]) {
            best = 18;
        }
    }

    if (m_damageManager.ProgressAeroplaneDamage(static_cast<uint8>(best))) {
        if (m_damageManager.GetAeroplaneCompStatus(static_cast<uint8>(best)) == DAMSTATE_OPENED && best >= 16 && best <= 22) {
            // Loose components start bouncing
            for (auto& panel : m_panels) {
                if (panel.m_nFrameId == (uint16)-1) {
                    panel.SetPanel(static_cast<int16>(best), 0, -0.02f);
                    break;
                }
                if (panel.m_nFrameId == static_cast<uint16>(best)) {
                    break;
                }
            }
        } else if (m_damageManager.GetAeroplaneCompStatus(static_cast<uint8>(best)) == DAMSTATE_DAMAGED) {
            SetComponentVisibility(m_aCarNodes[best], 2);
        }
    }

    dmgDrawCarCollidingParticles(*vecCollisionCoors, particleScale * damageIntensity, weapon);

    const auto nodePos = RwFrameGetMatrix(m_aCarNodes[best])->pos;

    FxSystem_c::SafeKillAndClear(m_pSmokeParticle);

    if (const auto modellingMatrix = GetModellingMatrix()) {
        m_pSmokeParticle = g_fxMan.CreateFxSystem("fire_med", nodePos, modellingMatrix, false);
        if (m_pSmokeParticle) {
            m_pSmokeParticle->Play();
            m_pSmokeParticle->SetVelAdd(-m_vecMoveSpeed * 5.0f);
            m_pSmokeParticle->SetLocalParticles(true);
            m_nSmokeTimer = CGeneral::GetRandomNumberInRange(2'000, 4'000);
        }
    }
}

// 0x6CAB90
void CPlane::IsAlreadyFlying() {
    m_nStartedFlyingTime = CTimer::GetTimeInMS() - 20000;
}

// 0x6CAC20
void CPlane::SetGearUp() {
    m_LandingGearAngle = 1.0f;
    m_fAirResistance = m_pHandlingData->m_fDragMult / 1000.0f / 2.0f * m_pFlyingHandlingData->m_fGearUpR;
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_LEFT,  WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_LEFT,   WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_RIGHT, WHEEL_STATUS_MISSING);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_RIGHT,  WHEEL_STATUS_MISSING);
}

// 0x6CAC70
void CPlane::SetGearDown() {
    m_LandingGearAngle = 0.0f;
    m_fAirResistance = m_pHandlingData->m_fDragMult / 1000.0f / 2.0f;
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_LEFT,  WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_LEFT,   WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_FRONT_RIGHT, WHEEL_STATUS_OK);
    m_damageManager.SetWheelStatus(CAR_WHEEL_REAR_RIGHT,  WHEEL_STATUS_OK);
}

// 0x6CCA50
uint32 CPlane::CountPlanesAndHelis() {
    uint32 counter = 0;
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (vehicle.IsSubHeli() || vehicle.IsSubPlane()) {
            counter++;
        }
    }
    return counter;
}

// 0x6CCAA0
bool CPlane::AreWeInNoPlaneZone() {
    const auto& camPos = TheCamera.GetPosition();
    constexpr CVector vec1 = { -1073.0f, -675.0f, 50.0f };

    return DistanceBetweenPoints(vec1, camPos) < 200.0f ||
           camPos.x > -2743.0f && camPos.x < -2626.0f && camPos.y > 1300.0f && camPos.y < 2200.0f || // todo: Is point inside
           camPos.x > -1668.0f && camPos.x < -1122.0f && camPos.y > 541.0f && camPos.y < 1118.0f;
}

// 0x6CCBB0
bool CPlane::AreWeInNoBigPlaneZone() {
    // untested
    const auto& camPos = TheCamera.GetPosition();
    return DistanceBetweenPoints2D({ +1522.0f, -1237.0f }, camPos) < 800.0f ||
           DistanceBetweenPoints2D({ -1836.0f, +659.0f }, camPos) < 800.0f;
}

// 0x6CCC50
void CPlane::SwitchAmbientPlanes(bool enable) {
    if (!GenPlane_Active) {
        GenPlane_Active = enable;
        return;
    }

    // Turning the ambient spawner off also removes the planes/helis it created
    if (!enable) {
        auto* const pool = GetVehiclePool();
        for (int32 i = static_cast<int32>(pool->GetSize()) - 1; i >= 0; i--) {
            CVehicle* const vehicle = pool->GetAt(i);
            if (!vehicle) {
                continue;
            }
            if (   vehicle->m_nVehicleType != VEHICLE_TYPE_HELI
                && vehicle->m_nVehicleType != VEHICLE_TYPE_PLANE
            ) {
                continue;
            }
            if (vehicle->m_nCreatedBy != RANDOM_VEHICLE) {
                continue;
            }
            CWorld::Remove(vehicle);
            delete vehicle;
        }
    }

    GenPlane_Active = enable;
}

// 0x6CD090
void CPlane::FindPlaneCreationCoors(CVector* outCoors, CVector* outTargetCoors, float* outPlaneOrientation, float* outFlightHeight, bool isBigPlane) {
    const float radius     = isBigPlane ? 340.0f : 140.0f;
    const float baseHeight = isBigPlane ? 200.0f : 25.0f;

    for (int32 step = 0; step < 500; step += 10) {
        const float angle = (float)(CGeneral::GetRandomNumber() % 360) * 0.017453292f;

        *outCoors        = FindPlayerCoors(-1);
        *outFlightHeight = (float)(CGeneral::GetRandomNumber() & 0xF) + (float)step + baseHeight;

        outCoors->x += std::cos(angle) * radius;
        outCoors->y += std::sin(angle) * radius;
        outCoors->z += *outFlightHeight;

        *outTargetCoors = FindPlayerCoors(-1);
        *outTargetCoors += TheCamera.GetActiveCam().m_vecFront * ((float)(CGeneral::GetRandomNumber() & 0x1F) + 20.0f);
        outTargetCoors->z += *outFlightHeight;

        *outPlaneOrientation = CGeneral::GetATanOfXY(
            outTargetCoors->x - outCoors->x,
            outTargetCoors->y - outCoors->y
        );

        // Probe the line from the spawn point out past the target
        const CVector probeEnd = *outTargetCoors + (*outTargetCoors - *outCoors);

        CWorld::AdvanceCurrentScanCode();

        CColSphere probe;
        probe.Set(15.0f, *outCoors, SURFACE_DEFAULT, 0, tColLighting{0xFF});

        if (!CWorld::GetIsLineOfSightClear(*outCoors, probeEnd, true, false, false, false, false, false, false)) {
            continue;
        }

        // NOTE: The 4th argument (the bounding box of `spA`/`spB`) is passed uninitialized by the
        //       original, and `CCollision::CheckCameraCollisionBuildings` never reads it.
        const CColBox unusedBox{};
        if (!CCollision::CheckCameraCollisionBuildings(
                static_cast<int32>(outCoors->x),
                static_cast<int32>(outCoors->y),
                unusedBox,
                probe,
                probe,
                probe
            )
        ) {
            break;
        }
    }

    *outFlightHeight += 20.0f;
    outTargetCoors->z += 20.0f;
}

// 0x6CD2F0
void CPlane::DoPlaneGenerationAndRemoval() {
    if ((CTimer::GetFrameCounter() & 0x1F) != 0x1E) {
        return;
    }
    if (!GenPlane_Active) {
        return;
    }

    const uint32 numPlanesAndHelis = CountPlanesAndHelis();

    // Model still being streamed in
    if (GenPlane_Status != 0) {
        if (GenPlane_Status != 1) {
            return;
        }

        if (!CStreaming::IsModelLoaded(GenPlane_ModelIndex)) {
            CStreaming::RequestModel(GenPlane_ModelIndex, STREAMING_KEEP_IN_MEMORY);
            return;
        }

        CVector coors, targetCoors;
        float   orientation, flightHeight;

        if (GenPlane_ModelIndex == MODEL_POLMAV) {
            // Police Maverick
            FindPlaneCreationCoors(&coors, &targetCoors, &orientation, &flightHeight, false);

            auto* const heli = new CHeli(GenPlane_ModelIndex, RANDOM_VEHICLE);

            heli->SetPosn(coors);
            SetSpawnHeading(*heli, orientation);
            CCarCtrl::JoinCarWithRoadSystem(heli);
            heli->SetStatus(STATUS_PHYSICS);
            heli->m_fHeliRotorSpeed = 0.22f;
            CWorld::Add(heli);

            heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLYINDIRECTION;
            heli->m_fMaxAltitude             = flightHeight;
            heli->m_fMinAltitude             = flightHeight;
            heli->field_9B4                  = orientation;
            heli->SetVelocity({ std::cos(orientation) * 0.6666667f, std::sin(orientation) * 0.6666667f, 0.0f });
            heli->vehicleFlags.bNeverUseSmallerRemovalRange = true;
            heli->m_nExtendedRemovalRange    = 255;
            heli->m_autoPilot.m_nCruiseSpeed = 50;
            heli->vehicleFlags.bEngineOn     = !heli->vehicleFlags.bEngineBroken;

            const CVector heliPos = heli->GetPosition();
            if ((CGeneral::GetRandomNumber() & 1) == 0) {
                // Chase a car
                if (auto* const car = CWorld::FindUnsuspectingTargetCar(heliPos, FindPlayerCoors(-1))) {
                    heli->m_autoPilot.m_nCarMission   = MISSION_HELI_FOLLOW_ENTITY;
                    heli->m_autoPilot.m_TargetEntity = car;
                    car->RegisterReference(reinterpret_cast<CEntity**>(&heli->m_autoPilot.m_TargetEntity));
                    heli->m_nHeliFlags |= 2;
                    heli->m_autoPilot.carCtrlFlags.bDoTargetCatchupCheck = true;
                    heli->m_autoPilot.carCtrlFlags.bHeliFollowTarget    = true;
                }
            } else if (auto* const ped = CWorld::FindUnsuspectingTargetPed(heliPos, FindPlayerCoors(-1))) {
                // Chase a ped
                heli->m_autoPilot.m_nCarMission   = MISSION_HELI_FOLLOW_ENTITY;
                heli->m_autoPilot.m_TargetEntity = reinterpret_cast<CVehicle*>(ped);
                ped->RegisterReference(reinterpret_cast<CEntity**>(&heli->m_autoPilot.m_TargetEntity));
                heli->m_nHeliFlags |= 2;
                heli->m_autoPilot.carCtrlFlags.bDoTargetCatchupCheck = true;
                heli->m_autoPilot.carCtrlFlags.bHeliFollowTarget    = true;

                CPedGroup* const group = CPedGroups::GetPedsGroup(ped);
                if (group && !group->m_bIsMissionGroup) {
                    CEventDanger event(heli, 200.0f);
                    group->GetIntelligence().AddEvent(&event);
                } else {
                    // Don't target the player, mission peds or cops
                    if (ped->IsPlayer() || ped->IsCreatedBy(PED_MISSION) || ped->m_nPedType == PED_TYPE_COP) {
                        GenPlane_LastTimeGenerated = CTimer::GetTimeInMS();
                        GenPlane_Status            = 0;
                        return;
                    }

                    CEventDanger event(heli, 200.0f);
                    ped->GetEventGroup().Add(&event, false);
                }
            }
        } else {
            // Plane
            const bool isBigPlane = GenPlane_ModelIndex == MODEL_AT400 || GenPlane_ModelIndex == MODEL_ANDROM;

            FindPlaneCreationCoors(&coors, &targetCoors, &orientation, &flightHeight, isBigPlane);

            auto* const plane = new CPlane(GenPlane_ModelIndex, RANDOM_VEHICLE);

            plane->SetPosn(coors);
            plane->m_planeCreationHeading = orientation;
            SetSpawnHeading(*plane, orientation);
            CCarCtrl::JoinCarWithRoadSystem(plane);
            plane->SetStatus(STATUS_PHYSICS);
            CWorld::Add(plane);

            plane->m_planeHeading     = orientation;
            plane->m_planeHeadingPrev = orientation;
            plane->m_autoPilot.m_nCarMission = MISSION_PLANE_FLYINDIRECTION;
            plane->m_maxAltitude             = flightHeight;
            plane->m_minAltitude             = flightHeight;
            plane->SetVelocity({ std::cos(orientation) * 0.6666667f, std::sin(orientation) * 0.6666667f, 0.0f });
            plane->vehicleFlags.bNeverUseSmallerRemovalRange = true;

            if (!isBigPlane && GenPlane_ModelIndex != MODEL_HYDRA) {
                plane->m_nExtendedRemovalRange = 150;
            } else {
                plane->m_nExtendedRemovalRange = 360;
                if (isBigPlane) {
                    plane->m_bUsesCollision = false;
                }
            }

            if (GenPlane_ModelIndex == MODEL_HYDRA) {
                plane->m_autoPilot.m_nCarMission = MISSION_PLANE_ATTACK_PLAYER_POLICE;
                plane->m_wMiscComponentAngle     = 0;
            }

            plane->SetGearUp();

            plane->vehicleFlags.bEngineOn = !plane->vehicleFlags.bEngineBroken;

            CStreaming::SetModelIsDeletable(GenPlane_ModelIndex);
            CStreaming::SetModelTxdIsDeletable(GenPlane_ModelIndex);
        }

        GenPlane_LastTimeGenerated = CTimer::GetTimeInMS();
        GenPlane_Status            = 0;
        return;
    }

    // No more planes when outside the city, too many already, or in a no-plane zone / cutscene
    if (   CGame::currArea != 0
        || numPlanesAndHelis > 4
        || AreWeInNoPlaneZone()
        || CCutsceneMgr::ms_cutsceneProcessing
    ) {
        GenPlane_LastTimeGenerated = CTimer::GetTimeInMS();
        return;
    }

    // Police Hydra when the player is wanted and flying
    if (static_cast<int32>(FindPlayerPed(-1)->GetWantedLevel()) >= 4) {
        if (auto* const playerVeh = FindPlayerVehicle(-1, false)) {
            if (   (playerVeh->m_nVehicleSubType == VEHICLE_TYPE_PLANE || playerVeh->m_nVehicleSubType == VEHICLE_TYPE_HELI)
                && playerVeh->m_nModelIndex != MODEL_AT400
                && playerVeh->m_nModelIndex != MODEL_ANDROM
                && CTimer::GetTimeInMS() > GenPlane_LastTimeGenerated + 60'000
            ) {
                uint32 hydras = 0;
                for (const auto& veh : GetVehiclePool()->GetAllValid()) {
                    if (veh.m_autoPilot.m_nCarMission == MISSION_PLANE_ATTACK_PLAYER_POLICE) {
                        hydras++;
                    }
                }
                if (hydras < 1 || (hydras < 2 && static_cast<int32>(FindPlayerPed(-1)->GetWantedLevel()) > 4)) {
                    GenPlane_ModelIndex = MODEL_HYDRA;
                    GenPlane_Status     = 1;
                }
            }
        }
    }

    if (numPlanesAndHelis > 0) {
        return;
    }
    if (GenPlane_Status != 0) {
        return;
    }

    // Small plane
    if (   (CWeather::WeatherRegion == WEATHER_REGION_DEFAULT || CWeather::WeatherRegion == WEATHER_REGION_DESERT)
        && CWeather::OldWeatherType != WEATHER_SANDSTORM_DESERT
        && CWeather::NewWeatherType != WEATHER_SANDSTORM_DESERT
    ) {
        const auto* const playerVeh = FindPlayerVehicle(-1, false);
        if (!playerVeh || (playerVeh->GetVehicleAppearance() != VEHICLE_APPEARANCE_PLANE && playerVeh->GetVehicleAppearance() != VEHICLE_APPEARANCE_HELI)) {
            if (CTimer::GetTimeInMS() > GenPlane_LastTimeGenerated + 120'000) {
                switch (CGeneral::GetRandomNumber() & 7) {
                case 0: case 1: case 2: case 7:
                    GenPlane_ModelIndex = MODEL_RUSTLER;
                    break;
                case 3: case 4:
                    GenPlane_ModelIndex = MODEL_CROPDUST;
                    break;
                case 5: case 6:
                    GenPlane_ModelIndex = MODEL_BEAGLE;
                    break;
                }
                GenPlane_Status = 1;
                return;
            }
        }
    }

    // Scripted / riot police Maverick
    if (CPopCycle::m_bCurrentZoneIsGangArea) {
        if (!CTheScripts::IsPlayerOnAMission() && CTimer::GetTimeInMS() > GenPlane_LastTimeGenerated + 350'000) {
            GenPlane_Status     = 1;
            GenPlane_ModelIndex = MODEL_POLMAV;
            return;
        }
        if (GenPlane_Status != 0) {
            return;
        }
    }

    if (CGameLogic::LaRiotsActiveHere()) {
        GenPlane_Status     = 1;
        GenPlane_ModelIndex = MODEL_POLMAV;
        return;
    }

    // Big plane
    if (   GenPlane_Status == 0
        && CWeather::WeatherRegion != WEATHER_REGION_DEFAULT
        && CWeather::WeatherRegion != WEATHER_REGION_DESERT
        && !CPopCycle::m_bCurrentZoneIsGangArea
        && CWeather::OldWeatherType != WEATHER_SANDSTORM_DESERT
        && CWeather::NewWeatherType != WEATHER_SANDSTORM_DESERT
        && CTimer::GetTimeInMS() > GenPlane_LastTimeGenerated + 200'000
        && !AreWeInNoBigPlaneZone()
    ) {
        const auto* const playerVeh = FindPlayerVehicle(-1, false);
        if (   !playerVeh
            || (playerVeh->GetVehicleAppearance() != VEHICLE_APPEARANCE_PLANE && playerVeh->GetVehicleAppearance() != VEHICLE_APPEARANCE_HELI)
        ) {
            GenPlane_ModelIndex = (CGeneral::GetRandomNumber() & 1) == 0 ? MODEL_AT400 : MODEL_ANDROM;
            GenPlane_Status     = 1;
        }
    }
}

// 0x6C9140
bool CPlane::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6C9150
bool CPlane::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6C94A0
void CPlane::PreRender() {
    CVehicle::PreRender();

    // TODO(re): The original first does the Vortex-only extra virtual step (`(*(*this + 0x114))()`) and
    //           calls the render-flag helper `0x6AA290` for components `4`, `7`, `2` (and `5` with `3`
    //           for the Vortex); neither helper is reversed yet.

    // TODO(re): The landing gear compression animation (wheels' height above ground, including the
    //           Andromeda's gear angles from an `acos`-like helper) is not reconstructed: the x87
    //           register flow through the 4-wheel loop isn't recoverable from the decompilation.

    // Propellers
    field_9C8 += CTimer::GetTimeStep() * m_fPropSpeed;
    while (field_9C8 > TWO_PI) {
        field_9C8 -= TWO_PI;
    }

    // Animate the props/rotors: each pair spins in the opposite direction
    for (auto i = 0; i < 2; i++) {
        const auto first  = static_cast<ePlaneNodes>(PLANE_STATIC_PROP  + i * 2); // 12, 14
        const auto second = static_cast<ePlaneNodes>(PLANE_MOVING_PROP  + i * 2); // 13, 15
        const float sign  = i == 0 ? 1.0f : -1.0f;

        if (auto* const node = m_aCarNodes[first]) {
            SetComponentRotation(node, AXIS_Y, sign * (field_9C8 + field_9C8), true);
            if (auto* const atomic = reinterpret_cast<RpAtomic*>(GetFirstObject(node))) {
                SetComponentAtomicAlpha(atomic, 255);
            }
        }
        if (auto* const node = m_aCarNodes[second]) {
            SetComponentRotation(node, AXIS_Y, -(sign * field_9C8), true);
            if (auto* const atomic = reinterpret_cast<RpAtomic*>(GetFirstObject(node))) {
                SetComponentAtomicAlpha(atomic, 0);
            }
        }
    }

    // Control surfaces
    if (GetStatus() <= STATUS_ABANDONED) {
        const CVector axis = m_vecMoveSpeed; // TODO(re): the original uses `modelInfo + 0x5C` minus the velocity

        const auto RotateSurface = [&](RwFrame* node, float angle) {
            if (!node) {
                return;
            }
            CQuaternion q;
            RwV3d quatAxis{ axis.x, axis.y, axis.z };
            q.Set(&quatAxis, angle);
            q.Get(RwFrameGetMatrix(node));
            RwMatrixUpdate(RwFrameGetMatrix(node));
        };

        RotateSurface(m_aCarNodes[PLANE_RUDDER],      m_fLeftRightSkid * 0.6981317f);        // 40°
        RotateSurface(m_aCarNodes[PLANE_ELEVATOR_L], -(m_fSteeringUpDown * 0.43633232f));    // 25°
        RotateSurface(m_aCarNodes[PLANE_ELEVATOR_R], -(m_fSteeringUpDown * 0.43633232f));
        RotateSurface(m_aCarNodes[PLANE_AILERON_L],  m_fSteeringLeftRight * 0.5235988f);     // 30°
        RotateSurface(m_aCarNodes[PLANE_AILERON_R], -(m_fSteeringLeftRight * 0.5235988f));

        // Gear/misc components follow the landing gear angle
        const float gearAngle = std::fabs(m_LandingGearAngle);
        SetComponentRotation(m_aCarNodes[PLANE_GEAR_L], AXIS_X, gearAngle, true);
        SetComponentRotation(m_aCarNodes[PLANE_GEAR_R], AXIS_X, gearAngle, true);

        switch (m_nModelIndex) {
        case MODEL_ANDROM:
            SetComponentRotation(m_aCarNodes[PLANE_MISC_B], AXIS_X, static_cast<float>(m_wMiscComponentAngle) * ANDROM_COL_ANGLE_MULT, true);
            break;
        case MODEL_HYDRA: {
            const float nozzleAngle = (static_cast<float>(m_wMiscComponentAngle) * 1.5707964f) / static_cast<float>(HARRIER_NOZZLE_ROTATE_LIMIT);
            SetComponentRotation(m_aCarNodes[PLANE_MISC_A], AXIS_X, nozzleAngle, true);
            SetComponentRotation(m_aCarNodes[PLANE_MISC_B], AXIS_X, nozzleAngle, true);
            break;
        }
        default:
            break;
        }
        // TODO(re): The per-model angle table applied to nodes `21..24` (`0x69C..0x6A8`), the Vortex'
        //           velocity-based aileron smoothing and the Harrier's jet-thruster FX are not
        //           reconstructed (they need the unnamed tweak constants and the `FxSystem` matrix helpers).
    }

    // Bouncing panels
    for (auto& panel : m_panels) {
        if (panel.m_nFrameId >= PLANE_NUM_NODES || panel.m_nFrameId == (uint16)-1) {
            continue;
        }
        if (auto* const node = m_aCarNodes[panel.m_nFrameId]) {
            panel.ProcessPanel(this, node, m_moveForce, m_turnForce, 0.03f, 0.98f);
        }
    }
    m_moveForce = m_vecMoveSpeed + m_vecFrictionMoveSpeed;
    m_turnForce = m_vecTurnSpeed + m_vecFrictionTurnSpeed;

    CShadows::StoreShadowForVehicle(this, VEH_SHD_PLANE);

    // Motion blur streaks
    if (!vehicleFlags.bIsDrowning && m_nModelIndex != MODEL_VORTEX && m_nModelIndex != MODEL_RCBARON) {
        // TODO(re): The two streak points come from a model-space point (`mi + 0x5C` -> `+0x90..0x98`)
        //           transformed by the plane's matrix; the model offsets aren't recovered.
    }

    // Smoke ejector
    if (m_bSmokeEjectorEnabled) {
        if (m_nModelIndex == MODEL_CROPDUST) {
            const auto particleData = FxPrtMult_c(1.0f, 1.0f, 1.0f, 0.4f, 1.0f, 1.0f, 0.2f);
            const CVector pos  = GetMatrix().TransformPoint({ 0.0f, -0.5f, -0.5f });
            const CVector vel  = m_vecMoveSpeed * 10.0f;
            g_fx.m_SmokeHuge->AddParticle(pos, vel, 0.0f, particleData);
        } else if (m_nModelIndex == MODEL_STUNT) {
            const auto particleData = FxPrtMult_c(1.0f, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.3f);
            const CVector pos  = GetMatrix().TransformPoint({ 0.0f, -5.0f, 0.0f });
            const CVector vel  = m_vecMoveSpeed * 10.0f;
            g_fx.m_SmokeHuge->AddParticle(pos, vel, 0.0f, particleData);
        }
    }

    if (m_nModelIndex == MODEL_SKIMMER && physicalFlags.bSubmergedInWater) {
        DoBoatSplashes(m_fDoomHorizontalRotation);
    }
}

// 0x6CAB70
void CPlane::Render() {
    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3000;
    CVehicle::Render();
}

// 0x6C9260
void CPlane::ProcessControl() {
    // Cropduster/Stuntplane: the right shock button toggles the smoke ejector
    if (GetStatus() == STATUS_PLAYER && (m_nModelIndex == MODEL_CROPDUST || m_nModelIndex == MODEL_STUNT)) {
        if (CPad::GetPad(m_pDriver ? m_pDriver->GetPadNumber() : 0)->IsRightShockPressed()) {
            m_bSmokeEjectorEnabled = !m_bSmokeEjectorEnabled;
        }
    }

    if (m_bSmokeEjectorEnabled && (!vehicleFlags.bEngineOn || vehicleFlags.bIsDrowning || !m_pDriver)) {
        m_bSmokeEjectorEnabled = false;
    }

    if (m_nModelIndex == MODEL_SKIMMER) {
        m_damageManager.SetAllWheelsState(WHEEL_STATUS_MISSING);
    }

    CAutomobile::ProcessControl();

    // Hand the stall counter over to the audio entity (it's reset for the next frame)
    if (m_StallCounter > 0) {
        m_vehicleAudio.m_DoCountStalls = static_cast<int16>(m_StallCounter);
        m_StallCounter                 = 0;
    } else {
        m_vehicleAudio.m_DoCountStalls = 0;
    }

    CVehicle::ProcessWeapons();

    if (m_nModelIndex == MODEL_VORTEX) {
        m_WheelStates.fill(WHEEL_STATE_NORMAL);
    }

    if (m_pSmokeParticle) {
        RwMatrix out;
        m_nSmokeTimer += static_cast<uint32>(static_cast<int32>(-CTimer::GetTimeStepInMS()));
        m_pSmokeParticle->GetCompositeMatrix(&out);

        const CVector velocity = -m_vecMoveSpeed * 5.0f;
        const auto particleData = FxPrtMult_c(0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0.1f);
        g_fx.m_SmokeHuge->AddParticle(out.pos, velocity, 0.00f, particleData);
        g_fx.m_SmokeHuge->AddParticle(out.pos, velocity, 0.20f, particleData);

        if (static_cast<int32>(m_nSmokeTimer) <= 0 || vehicleFlags.bIsDrowning) {
            m_pSmokeParticle->Kill();
            m_pSmokeParticle = nullptr;
        }
    }
}

// 0x6CADD0
void CPlane::ProcessControlInputs(uint8 playerNum) {
    auto* const pad = CPad::GetPad(playerNum);

    const float verticalSpeed = DotProduct(m_vecMoveSpeed, GetUp());
    const float dt            = CTimer::GetTimeStep();

    const auto SmoothFromSticks = [&, this] {
        m_fSteeringLeftRight += (pad->GetSteeringLeftRight() / 128.0f - m_fSteeringLeftRight) * dt * 0.2f;
        m_fSteeringUpDown    += (pad->GetSteeringUpDown()    / 128.0f - m_fSteeringUpDown)    * dt * 0.2f;
    };
    const auto DecaySticks = [&, this] {
        const float decay = std::pow(0.99f, dt);
        if (std::fabs(m_fSteeringLeftRight) < 0.15f) {
            m_fSteeringLeftRight *= decay;
        }
        if (std::fabs(m_fSteeringUpDown) < 0.15f) {
            m_fSteeringUpDown *= decay;
        }
    };

    // Steering: mouse flying (when allowed) or the pad sticks
    if (!CCamera::m_bUseMouse3rdPerson || !CVehicle::m_bEnableMouseFlying) {
        CVehicle::m_nLastControlInput = eControllerType::KEYBOARD;
        SmoothFromSticks();
    } else {
        const auto& mouseDelta = CPad::NewMouseControllerState.m_AmountMoved;
        if (mouseDelta.x == 0.0f && mouseDelta.y == 0.0f) {
            const bool sticksIdle = pad->GetSteeringLeftRight() == 0 && pad->GetSteeringUpDown() == 0;
            if (sticksIdle && CVehicle::m_nLastControlInput == eControllerType::MOUSE) {
                DecaySticks();
            } else {
                CVehicle::m_nLastControlInput = eControllerType::KEYBOARD;
                SmoothFromSticks();
            }
        } else {
            CVehicle::m_nLastControlInput = eControllerType::MOUSE;
            if (!pad->NewState.m_bVehicleMouseLook) {
                m_fSteeringLeftRight += mouseDelta.x * 0.0015f;
                m_fSteeringUpDown    += mouseDelta.y * 0.0015f;
            }
            DecaySticks();
        }
    }

    m_fSteeringLeftRight = std::clamp(m_fSteeringLeftRight, -1.0f, 1.0f);
    m_fSteeringUpDown    = std::clamp(m_fSteeringUpDown,    -1.0f, 1.0f);

    // Yaw (rudder)
    if (m_nModelIndex == MODEL_VORTEX) {
        m_fLeftRightSkid += (pad->GetSteeringLeftRight() / 128.0f - m_fLeftRightSkid) * dt * 0.2f;
    } else {
        const float yawInput = pad->GetLookLeft()
            ? -1.0f
            : (pad->GetLookRight() ? 1.0f : 0.0f);
        m_fLeftRightSkid += (yawInput - m_fLeftRightSkid) * dt * 0.2f;
    }
    m_fLeftRightSkid = std::clamp(m_fLeftRightSkid, -1.0f, 1.0f);

    // Steering angle, reduced the more vertically we're moving
    m_fSteerAngle = -(
        m_pHandlingData->m_fSteeringLock
        * 0.017453292f // DEG_TO_RAD
        * m_fLeftRightSkid
        * std::max(1.0f - verticalSpeed, 0.1f)
    );

    if (m_nModelIndex == MODEL_VORTEX && pad->GetHandBrake()) {
        m_fSteerAngle    *= 1.5f;
        m_fLeftRightSkid *= 1.5f;
    }

    m_fAccelerationBreakStatus = (float)(pad->GetAccelerate() - pad->GetBrake()) * (1.0f / 255.0f);

    // Landing gear. The button (right shock) toggles between gear up and gear down.
    const bool canToggleGear =
           pad->NewState.ShockButtonR != 0
        && pad->OldState.ShockButtonR == 0
        && m_fWheelsSuspensionCompression[0] == 1.0f
        && m_fWheelsSuspensionCompression[1] == 1.0f
        && m_fWheelsSuspensionCompression[2] == 1.0f
        && m_fWheelsSuspensionCompression[3] == 1.0f
        && m_pFlyingHandlingData->m_fGearUpR < 1.0f
        && m_nModelIndex != MODEL_RCBARON;

    if (canToggleGear) {
        if (m_LandingGearAngle == 0.0f) {
            m_damageManager.SetAllWheelsState(WHEEL_STATUS_MISSING);
            m_LandingGearAngle += dt * 0.02f;
        } else if (m_LandingGearAngle == 1.0f) {
            m_LandingGearAngle = dt * 0.02f - 1.0f;
        } else {
            m_LandingGearAngle = -m_LandingGearAngle;
        }
    } else if (m_LandingGearAngle >= 0.0f) {
        // Raising: keep going until fully up
        if (m_LandingGearAngle > 0.0f && m_LandingGearAngle < 1.0f) {
            m_LandingGearAngle += dt * 0.02f;
            if (m_LandingGearAngle >= 1.0f) {
                SetGearUp();
            }
        }
    } else {
        // Lowering: go back to 0, then the gear is down again
        m_LandingGearAngle += dt * 0.02f;
        if (m_LandingGearAngle >= 0.0f) {
            m_LandingGearAngle = 0.0f;
            m_damageManager.SetAllWheelsState(WHEEL_STATUS_OK);
            m_fAirResistance = m_pHandlingData->m_fDragMult / 1000.0f * 0.5f;
        }
    }

    // Harrier: the gun up/down axis rotates the nozzles
    if (m_nModelIndex == MODEL_HYDRA) {
        const int16 gunUpDown = pad->GetCarGunUpDown();
        if (std::fabs((float)gunUpDown) > 10.0f) {
            m_wMiscComponentAnglePrev = m_wMiscComponentAngle;

            const auto delta = static_cast<int32>(
                (float)gunUpDown * dt * HARRIER_NOZZLE_ROTATERATE * 0.0078125f
            );
            const auto angle = static_cast<int32>(m_wMiscComponentAngle) + delta;
            m_wMiscComponentAngle = angle < 0 ? 0 : static_cast<uint16>(angle);
            if (static_cast<int32>(m_wMiscComponentAngle) > static_cast<int32>(HARRIER_NOZZLE_ROTATE_LIMIT)) {
                m_wMiscComponentAngle = HARRIER_NOZZLE_ROTATE_LIMIT;
            }
        }
    }

    vehicleFlags.bIsHandbrakeOn = false;
    m_GasPedal                  = 0.0f;

    // Brake pedal
    if (verticalSpeed > 0.0f && pad->GetBrake() != 0) {
        m_BrakePedal = (float)pad->GetBrake() * (verticalSpeed <= 0.35f ? (1.0f / 255.0f) : (1.0f / 510.0f));
    } else if (m_vecMoveSpeed.Magnitude() < 0.1f && pad->GetBrake() < 10 && pad->GetAccelerate() < 10) {
        m_BrakePedal = 0.5f;
    } else {
        m_BrakePedal = 0.0f;
    }

    if (pad->DisablePlayerControls) {
        m_BrakePedal                = 1.0f;
        vehicleFlags.bIsHandbrakeOn = true;
        m_GasPedal                  = 0.0f;

        FindPlayerPed(-1)->KeepAreaAroundPlayerClear();

        const float speed = m_vecMoveSpeed.Magnitude();
        if (speed > 0.28f) {
            m_vecMoveSpeed *= 0.28f / speed;
        }
    }
}

// 0x6CB7C0
void CPlane::ProcessFlyingCarStuff() {
    const float dt = CTimer::GetTimeStep();
    if (dt < 0.0f) {
        return;
    }

    // Advance the damage wave by a random multiple of the frame's length
    m_planeDamageWave += static_cast<int32>(
        (0.25f + CGeneral::GetRandomNumberInRange(0.0f, 1.0f) * 1.5f) * (float)static_cast<int32>(CTimer::GetTimeStepInMS())
    );

    const float speedFactor = std::min(m_vecMoveSpeed.Magnitude() * 3.0f, 1.0f);

    float skid      = m_fLeftRightSkid;
    float upDown    = m_fSteeringUpDown;
    float leftRight = m_fSteeringLeftRight;

    // A player plane that's about to fall apart loses its aerodynamic surfaces
    if (m_fHealth < 250.0f && GetStatus() == STATUS_PLAYER) {
        m_damageManager.SetAeroplaneCompStatus(12, DAMSTATE_DAMAGED);
        m_damageManager.SetAeroplaneCompStatus(13, DAMSTATE_DAMAGED);

        skid += 0.5f;
        const float roll = std::fabs(GetRoll());
        if (roll < 2.3561945f) {
            leftRight += 0.75f;
        }
        if (roll > 1.5707964f) {
            upDown += 0.5f;
        }
    }

    // Damaged components flop around, which in turn jitters the controls and spawns debris particles
    if (m_nModelIndex != MODEL_RCBARON) {
        for (int32 comp = 12; comp < 25; comp++) {
            const uint8 compStatus = m_damageManager.GetAeroplaneCompStatus(comp);
            if (!m_aCarNodes[comp] || compStatus <= 0) {
                continue;
            }

            // Find the bouncing panel attached to this component
            // TODO(re): The original scans 4 panel slots (`0x6AC + i * 0x20`, `i < 4`), but only 3 fit
            //           before `m_swingingChassis` (`0x70C`), so the 4th read overlaps that door.
            int32 panelIdx = -1;
            for (auto i = 0u; i < m_panels.size(); i++) {
                if (m_panels[i].m_nFrameId == comp) {
                    panelIdx = static_cast<int32>(i);
                    break;
                }
            }

            float damageFactor{};
            switch (comp) {
            case 12: case 13: case 14: case 15: {
                if (m_fAccelerationBreakStatus > 0.0f) {
                    const float wave = std::sin((float)m_planeDamageWave * TWO_PI / 2500.0f);
                    m_fAccelerationBreakStatus +=
                        (0.2f + CGeneral::GetRandomNumberInRange(0.0f, 1.0f) * 1.6f) // [0.2, 1.8]
                        * (wave - 1.0f)
                        * (float)compStatus * (float)compStatus
                        * dt
                        * 0.2f;
                }
                damageFactor = (float)compStatus * 0.5f;
                break;
            }
            case 16: { // Rudder
                m_fLeftRightSkid = (1.0f - (float)compStatus * 0.2f) * m_fLeftRightSkid;
                skid = m_fLeftRightSkid
                     + CGeneral::GetRandomNumberInRange(-0.05f, 0.05f) * (float)(compStatus * compStatus) * dt * speedFactor;
                if (panelIdx >= 0) {
                    auto& panel = m_panels[panelIdx];
                    panel.m_vecPos.y += CGeneral::GetRandomNumberInRange(-0.002f, 0.002f) * (float)(compStatus * compStatus) * dt * speedFactor;
                    skid -= 10.0f * panel.m_vecRotation.y;
                }
                damageFactor = (float)compStatus * 0.2f;
                break;
            }
            case 17: case 18: { // Elevators
                m_fSteeringUpDown = (1.0f - (float)compStatus * 0.2f) * m_fSteeringUpDown;
                upDown = m_fSteeringUpDown
                       + CGeneral::GetRandomNumberInRange(-0.05f, 0.05f) * (float)(compStatus * compStatus) * dt * speedFactor;
                if (panelIdx >= 0) {
                    auto& panel = m_panels[panelIdx];
                    panel.m_vecPos.y += CGeneral::GetRandomNumberInRange(-0.002f, 0.002f) * (float)(compStatus * compStatus) * dt * speedFactor;
                    upDown -= 10.0f * panel.m_vecRotation.y;
                }
                damageFactor = (float)compStatus * 0.15f;
                break;
            }
            case 19: case 20: { // Ailerons
                m_fSteeringLeftRight = (1.0f - (float)compStatus * 0.2f) * m_fSteeringLeftRight;
                leftRight = m_fSteeringLeftRight
                          + CGeneral::GetRandomNumberInRange(-0.05f, 0.05f) * (float)(compStatus * compStatus) * dt * speedFactor;
                if (panelIdx >= 0) {
                    auto& panel = m_panels[panelIdx];
                    panel.m_vecPos.y += CGeneral::GetRandomNumberInRange(-0.002f, 0.002f) * (float)(compStatus * compStatus) * dt * speedFactor;
                    const float gain = 10.0f * panel.m_vecRotation.y;
                    if (comp == 19) {
                        leftRight += gain;
                    } else {
                        leftRight -= gain;
                    }
                }
                damageFactor = (float)compStatus * 0.25f;
                break;
            }
            default:
                continue;
            }

            // Debris particles (only when moving fast enough, mostly near the camera)
            if (damageFactor <= 0.0f || vehicleFlags.bIsDrowning) {
                continue;
            }
            if (speedFactor <= 0.3f && (CGeneral::GetRandomNumber() & 7) != 0) {
                continue;
            }
            if (   DistanceBetweenPointsSquared(GetPosition(), TheCamera.GetPosition()) >= 6400.0f
                && GetStatus() != STATUS_PLAYER
            ) {
                continue;
            }
            if (auto* const node = m_aCarNodes[comp]) {
                const CVector nodePos = GetMatrix().TransformPoint(RwFrameGetMatrix(node)->pos);

                const auto particleData = FxPrtMult_c(0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, damageFactor);

                CVector velocity = m_vecMoveSpeed * 0.25f * 50.0f;
                velocity.x *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);
                velocity.y *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);
                velocity.z *= CGeneral::GetRandomNumberInRange(0.9f, 1.1f);

                const CVector pos = nodePos + CVector{
                    CGeneral::GetRandomNumberInRange(-damageFactor, damageFactor),
                    CGeneral::GetRandomNumberInRange(-damageFactor, damageFactor),
                    CGeneral::GetRandomNumberInRange(-damageFactor, damageFactor),
                };

                g_fx.m_SmokeHuge->AddParticle(pos, velocity, 0.0f, particleData);
            }
        }
    }

    const auto status = GetStatus();

    // Not flying: wind the propeller down and give up
    if (status != STATUS_PLAYER && status != STATUS_FORCED_STOP && status != STATUS_PHYSICS) {
        const float decay = dt * 0.001f;
        if (m_fPropSpeed <= decay) {
            m_fPropSpeed                = 0.0f;
            vehicleFlags.bEngineOn      = false;
            return;
        }
        if (m_fPropSpeed > PLANE_STD_PROP_SPEED) {
            m_fPropSpeed -= dt * 0.003f;
        } else {
            m_fPropSpeed -= decay;
        }
        m_nFakePhysics = 0;
        return;
    }

    // Target propeller speed depends on the throttle
    float targetPropSpeed = PLANE_STD_PROP_SPEED;
    if (m_fAccelerationBreakStatus > 0.0f) {
        targetPropSpeed += (PLANE_MAX_PROP_SPEED - PLANE_STD_PROP_SPEED) * m_fAccelerationBreakStatus;
    } else if (m_fAccelerationBreakStatus < 0.0f) {
        targetPropSpeed += (PLANE_STD_PROP_SPEED - PLANE_MIN_PROP_SPEED) * m_fAccelerationBreakStatus;
    }

    if (status == STATUS_PLAYER || status == STATUS_FORCED_STOP) {
        const eFlightModel flightModel = m_nModelIndex == MODEL_RCBARON ? FLIGHTMODEL_RCPLANE : FLIGHTMODEL_PLANE;

        const float height = HeightAboveCeiling(GetPosition().z, flightModel);
        if (height > 0.0f) {
            targetPropSpeed *= std::max(height * 0.02f, 0.0f);
            m_fAccelerationBreakStatus = std::max(m_fAccelerationBreakStatus - height * 0.04f, -1.0f);
        }
    }

    const bool isDrowning = vehicleFlags.bIsDrowning;
    m_fPropSpeed += (targetPropSpeed - m_fPropSpeed) * dt * PLANE_ROC_PROP_SPEED;

    if (isDrowning) {
        m_fPropSpeed                = 0.0f;
        m_fAccelerationBreakStatus  = 0.0f;
        vehicleFlags.bEngineOn      = false;
    }

    if (m_nModelIndex == MODEL_RCBARON) {
        if (!isDrowning && vehicleFlags.bEngineOn) {
            CVehicle::FlyingControl(FLIGHTMODEL_RCPLANE, skid, upDown, leftRight, m_fAccelerationBreakStatus);
        }
        return;
    }

    if (!vehicleFlags.bEngineOn) {
        return;
    }

    // The engines are running, and either the propeller pushes air or we're already moving
    if (   m_fPropSpeed <= PLANE_MIN_PROP_SPEED
        && m_vecMoveSpeed.SquaredMagnitude() <= 0.05f
    ) {
        return;
    }

    // Hydra: while the nozzles are fully tilted down it flies like a helicopter
    if (   m_nModelIndex == MODEL_HYDRA
        && GetStatus() == STATUS_PLAYER
        && static_cast<int32>(m_wMiscComponentAngle) >= static_cast<int32>(HARRIER_NOZZLE_SWITCH_LIMIT)
    ) {
        auto* const savedFlyingHandling = m_pFlyingHandlingData;
        m_pFlyingHandlingData = gHandlingDataMgr.GetFlyingPointer(
            static_cast<uint8>(CModelInfo::GetModelInfo(MODEL_HUNTER)->AsVehicleModelInfoPtr()->m_nHandlingId)
        );

        if (m_fAccelerationBreakStatus > 0.0f || (m_nNumContactWheels < 4 && !physicalFlags.bTouchingWater)) {
            CVehicle::FlyingControl(
                FLIGHTMODEL_HELI,
                skid,
                upDown,
                -leftRight,
                0.25f * m_fAccelerationBreakStatus
            );
        }

        m_pFlyingHandlingData = savedFlyingHandling;
        return;
    }

    CVehicle::FlyingControl(FLIGHTMODEL_PLANE, skid, upDown, leftRight, m_fAccelerationBreakStatus);
}
