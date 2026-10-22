/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "Shadows.h"
#include "Ropes.h"
#include "WindModifiers.h"
#include "InterestingEvents.h"
#include "CustomBuildingDNPipeline.h"
#include "TaskComplexUseSwatRope.h"
#include "TaskComplexWanderCop.h"
#include "TaskComplexSequence.h"

namespace {
    // TODO(re): `CControllerState` declares no field at `+0x2C`; the game reads it there as an `int16`
    //           (see `CHeli::ProcessControlInputs` 0x6C48A2 and `CPlane::ProcessControlInputs` 0x6CADD0).
    //           It gates the mouse-driven steering (`0` = apply the mouse delta).
    int16& GetPadNewStateField2C(CPad& pad) {
        return *reinterpret_cast<int16*>(reinterpret_cast<uint8*>(&pad.NewState) + 0x2C);
    }

    // TODO(re): `CTimer::m_snPreviousTimeInMilliseconds` (0xB7CB78) is private; the game reads it in
    //           `CHeli::ProcessControl` (0x6C75CF) to fire once per shout interval.
    auto& ms_previousTimeInMilliseconds = StaticRef<uint32>(0xB7CB78);
}

void CHeli::InjectHooks() {
    RH_ScopedVirtualClass(CHeli, 0x871680, 71);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(InitHelis, 0x6C4560);
    RH_ScopedInstall(AddHeliSearchLight, 0x6C45B0);
    RH_ScopedInstall(Pre_SearchLightCone, 0x6C4650);
    RH_ScopedInstall(Post_SearchLightCone, 0x6C46E0);
    RH_ScopedInstall(SwitchPoliceHelis, 0x6C4800);
    RH_ScopedInstall(RenderAllHeliSearchLights, 0x6C7C50);
    RH_ScopedInstall(TestSniperCollision, 0x6C6890);
    RH_ScopedVMTInstall(Render, 0x6C4400);
    RH_ScopedVMTInstall(Fix, 0x6C4530);
    RH_ScopedVMTInstall(BurstTyre, 0x6C4330);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6C4320);
}

// 0x6C4190
CHeli::CHeli(int32 modelIndex, eVehicleCreatedBy createdBy) : CAutomobile(modelIndex, createdBy, true) {
    m_nVehicleSubType = VEHICLE_TYPE_HELI;

    m_fLeftRightSkid           = 0.0f;
    m_fSteeringUpDown          = 0.0f;
    m_fSteeringLeftRight       = 0.0f;
    m_fAccelerationBreakStatus = 0.0f;

    field_99C = 0;
    m_fRotorZ = 0;
    m_fSecondRotorZ = 0;

    m_fMinAltitude = 10.0f;
    m_fMaxAltitude = 10.0f;

    field_9AC = 10.0f;
    field_9B4 = 0;

    m_nHeliFlags = m_nHeliFlags & 0xFC;
    m_fSearchLightIntensity = 0.0f;
    physicalFlags.bDontCollideWithFlyers = true;

    if (modelIndex == MODEL_HUNTER) {
        m_damageManager.SetDoorStatus(DOOR_LEFT_FRONT, DAMSTATE_OK);
        m_doors[DOOR_LEFT_FRONT].Init((3.0f * PI) / 10.0f, 0.0f, DOOR_AXIS_NEG_X, DOOR_AXIS_Y, DOOR_EXTRA_BASED);
    }

    m_nNumSwatOccupants = 4;
    m_aSwatState.fill(0);

    m_nSearchLightTimer = CTimer::GetTimeInMS();

    m_aSearchLightHistoryX.fill(0.0f);
    m_aSearchLightHistoryY.fill(0.0f);

    m_nShootTimer = 0;
    m_nPoliceShoutTimer = CTimer::GetTimeInMS();

    vehicleFlags.bNeverUseSmallerRemovalRange = true; // 0x6C42BD
    m_autoPilot.m_ucHeliTargetDist2 = 10;

    m_ppGunflashFx = nullptr;
    m_nFiringMultiplier = 16;

    field_9B8 = 0;
    m_bSearchLightEnabled = false;
    field_A14 = CGeneral::GetRandomNumberInRange(2.f, 8.f);
}

// 0x6C4340
CHeli::~CHeli() {
    if (m_ppGunflashFx) {
        for (auto i = 0; i < CVehicle::GetPlaneNumGuns(); i++) {
            if (auto& fx = m_ppGunflashFx[i]) {
                fx->Kill();
                g_fxMan.DestroyFxSystem(fx);
            }
        }
        delete[] m_ppGunflashFx;
        m_ppGunflashFx = nullptr;
    }

    m_vehicleAudio.Terminate();
}

// 0x6C4560
void CHeli::InitHelis() {
    std::ranges::fill(pHelis, nullptr);
    for (auto& light : HeliSearchLights) {
        light.Init();
    }
    NumberOfSearchLights = 0;
    bPoliceHelisAllowed = true;
}

// 0x6C45B0
void CHeli::AddHeliSearchLight(const CVector& origin, const CVector& target, float targetRadius, float power, uint32 coronaIndex, uint8 unknownFlag, uint8 drawShadow) {
    auto& light = HeliSearchLights[NumberOfSearchLights];

    light.m_vecOrigin     = origin;
    light.m_vecTarget     = target;
    light.m_fTargetRadius = targetRadius;
    light.m_fPower        = power;
    light.m_nCoronaIndex  = coronaIndex;
    light.field_24        = unknownFlag;
    light.m_bDrawShadow   = drawShadow;

    NumberOfSearchLights += 1;
}

// 0x6C4640
void CHeli::PreRenderAlways() {
    // NOP
}

// 0x6C4650
void CHeli::Pre_SearchLightCone() {
    ZoneScoped;

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,             RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,            RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,        RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,            RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,    RWRSTATE(rwALPHATESTFUNCTIONGREATEREQUAL));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(0));
}

// 0x6C46E0
void CHeli::Post_SearchLightCone() {
    ZoneScoped;

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,             RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,            RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATECULLMODE,             RWRSTATE(rwCULLMODECULLBACK));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,    RWRSTATE(rwALPHATESTFUNCTIONGREATER));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(2u));
}

// 0x6C4750
void CHeli::SpecialHeliPreRender() {
    // NOP
}

// 0x6C4760
CVector CHeli::FindSwatPositionRelativeToHeli(int32 swatNumber) {
    switch ( swatNumber ) {
    case 0:
        return { -1.2f, -1.0f, -0.5f };
    case 1:
        return { 1.2f,  -1.0f, -0.5f };
    case 2:
        return { -1.2f, 1.0f,  -0.5f };
    case 3:
        return { 1.2f,  1.0f,  -0.5f };
    default:
        return { 0.0f,  0.0f,  0.0f  };
    }
}

// 0x6C4800
void CHeli::SwitchPoliceHelis(bool enable) {
    bPoliceHelisAllowed = enable;
}

// 0x6C58E0
void CHeli::SearchLightCone(int32 coronaIndex,
                            CVector origin,
                            CVector target,
                            float targetRadius,
                            float power,
                            uint8 unknownFlag,
                            uint8 drawShadow,
                            CVector& useless0,
                            CVector& useless1,
                            CVector& useless2,
                            bool a11,
                            float baseRadius,
                            float a13,
                            float a14,
                            float a15
) {
    const auto dir = Normalized(target - origin);

    // Clip the beam against the world
    {
        const auto beamEnd = target + dir * 3.0f;

        CColPoint colPoint{};
        CEntity*  hitEntity{};
        if (CWorld::ProcessLineOfSight(origin, beamEnd, colPoint, hitEntity, true, false, false, false, false, false, false, false)) {
            target = colPoint.m_vecPoint;
        }
    }

    const auto farPoint = origin + dir * 100.0f;

    // Corona at the light's source, brightest when looked at down the beam
    {
        const auto  toCam = Normalized(TheCamera.GetPosition() - origin);
        const float f     = std::max(DotProduct(toCam, dir), 0.0f);
        const float f6    = f * f * f * f * f * f;

        // TODO(re): only the numeric part of the argument list is recoverable (0x6C59xx); the remaining
        //           enum/flag arguments below are the closest match to the observed values.
        CCoronas::RegisterCorona(
            coronaIndex,
            nullptr,
            200, 200, 255,
            (uint8)(f6 * 20.0f),
            origin,
            100.0f,
            100.0f,
            eCoronaType::CORONATYPE_HEADLIGHT,
            eCoronaFlareType::FLARETYPE_NONE,
            eCoronaReflType::CORREFL_NONE,
            eCoronaLOSCheck::LOSCHECK_OFF,
            eCoronaTrail::TRAIL_OFF,
            0.0f,
            false,
            1.5f,
            false,
            15.0f,
            false,
            false
        );
    }

    // Build the cone out of 41 quads around the beam axis
    const CVector zAxis{ 0.0f, 0.0f, 1.0f };
    const auto    axisA = Normalized(CrossProduct(zAxis, dir));
    const auto    axisB = Normalized(CrossProduct(axisA, dir));

    auto* const  verts   = &TempBufferVertices.m_3d[uiTempBufferVerticesStored];
    auto* const  indices = &aTempBufferIndices[uiTempBufferIndicesStored];
    int32        numVerts{};
    int32        numIndices{};

    // TODO(re): The original fills the per-vertex colour from `rand() & 0xFF` pairs (0x6C7Dxx) and takes the
    //           alpha from `CustomBuildingDNPipeline::m_fDNBalanceParam * 0.15f + 0.1f`; only the latter is used here.
    const float alpha = CCustomBuildingDNPipeline::m_fDNBalanceParam * 0.15f + 0.1f;

    CVector ringSamples[4]; // Every 10th ring vertex, in order (k = 0, 10, 20, 30)
    auto    ringSampleCount{0};

    for (auto k = 0; k < 0x29; k++) {
        const float angle  = (float)k * 0.15707964f; // pi / 20
        const auto  radial = axisA * std::sin(angle) + axisB * std::cos(angle);

        const auto vNear = origin + radial * baseRadius;
        const auto vFar  = farPoint + radial * targetRadius;

        // Project the far vertex onto the target's plane, along the beam
        const float t    = (vNear.z - target.z) / (vNear.z - vFar.z);
        const auto  proj = vNear + (vFar - vNear) * t;

        if ((k % 10) == 0 && ringSampleCount < (int32)std::size(ringSamples)) {
            ringSamples[ringSampleCount++] = proj;
        }

        // Clamp the projected point to 100m from the light's origin
        auto far = proj;
        const auto offset = proj - vNear;
        if (offset.Magnitude() > 100.0f) {
            far = vNear + Normalized(offset) * 100.0f;
        }

        const auto StoreVertex = [&](const CVector& pos) {
            auto& v = verts[numVerts++];
            RwIm3DVertexSetPos(&v, pos.x, pos.y, pos.z);
            RwIm3DVertexSetRGBA(&v, 255, 255, 255, (uint8)(alpha * 255.0f));
        };

        StoreVertex(vNear);
        StoreVertex(far);

        if (k > 0) {
            const auto s = (uint16)(numVerts - 4);
            indices[numIndices++] = s;
            indices[numIndices++] = s + 3;
            indices[numIndices++] = s + 1;
            if (targetRadius > 0.0f) {
                indices[numIndices++] = s;
                indices[numIndices++] = s + 2;
                indices[numIndices++] = s + 3;
            }
        }
    }

    if (numIndices > 0 && RwIm3DTransform(verts, numVerts, nullptr, rwIM3D_VERTEXUV)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, indices, numIndices);
        RwIm3DEnd();
    }

    uiTempBufferVerticesStored += numVerts;
    uiTempBufferIndicesStored  += numIndices;

    // Hand the cone's tip and its two last ring points back to the caller
    const auto lastRing  = (ringSampleCount > 1 ? ringSamples[ringSampleCount - 1] : target) - target;
    const auto firstRing = (ringSampleCount > 0 ? ringSamples[0] : target) - target;

    useless0 = target;
    useless1.Set(lastRing.x, lastRing.y, lastRing.z);
    useless2.Set(firstRing.x, firstRing.y, firstRing.z);

    if (a11) {
        const auto shadowPos = target + CVector{ 0.0f, 0.0f, 5.0f };
        const auto topX      = useless1.x * 1.2f;
        const auto topY      = useless1.y * 1.2f;
        const auto rightX    = useless2.x * 1.2f;
        const auto rightY    = useless2.y * 1.2f;

        if (std::sqrt(sq(topY) + sq(topX)) < 100.0f && std::sqrt(sq(rightY) + sq(rightX)) < 100.0f) {
            if (DistanceBetweenPoints2D(origin, TheCamera.GetPosition()) <= 25.0f) {
                // TODO(re): the intensity/z-distance/scale arguments of this call are not recoverable (0x6C7Cxx).
                CShadows::StoreShadowToBeRendered(
                    static_cast<uint8>(eShadowType::SHADOW_ADDITIVE),
                    gpShadowExplosionTex,
                    shadowPos,
                    topX, topY,
                    rightX, rightY,
                    255,
                    255, 255, 255,
                    0.0f,
                    false,
                    0.0f,
                    nullptr,
                    false
                );
            }
        }
    }
}

// 0x6C6520
CHeli* CHeli::GenerateHeli(CPed* target, bool newsHeli) {
    auto* const heli = newsHeli
        ? new CHeli(MODEL_VCNMAV, PERMANENT_VEHICLE)
        : new CHeli(MODEL_POLMAV, PERMANENT_VEHICLE);

    // Spawn on a random spot of a 250m ring around the target, 50m above it
    const auto targetPos = target->GetPosition();

    float angle = (float)(CGeneral::GetRandomNumber() & 0xFF) * (TWO_PI / 256.0f);
    CVector pos{
        targetPos.x + std::cos(angle) * 250.0f,
        targetPos.y + std::sin(angle) * 250.0f,
        targetPos.z + 50.0f
    };

    // Retry on the opposite side when outside the map
    if (pos.x < -3000.0f || pos.x > 3000.0f || pos.y < -3000.0f || pos.y > 3000.0f) {
        angle += PI;
        pos.Set(
            targetPos.x + std::cos(angle) * 250.0f,
            targetPos.y + std::sin(angle) * 250.0f,
            targetPos.z + 50.0f
        );
    }

    // Keep away from the no-fly zone around the airport
    if (DistanceBetweenPoints2D(pos, CVector2D{ -2322.0f, -1653.0f }) < 350.0f) {
        pos.Set(
            targetPos.x,
            targetPos.y,
            std::max(targetPos.z + 200.0f, 560.0f)
        );
    }

    pos.z = std::max(pos.z, CWorld::FindGroundZForCoord(pos.x, pos.y) + 20.0f);

    heli->GetMatrix().SetTranslate(pos);
    heli->vehicleFlags.bIsLocked = true;
    heli->SetStatus(STATUS_PHYSICS);

    heli->m_autoPilot.m_nCarMission  = newsHeli ? MISSION_HELI_NEWS_BEHAVIOUR : MISSION_HELI_POLICE_BEHAVIOUR;
    heli->m_autoPilot.m_nCruiseSpeed = newsHeli ? 35 : 70;
    // NOTE: `m_autoPilot.m_TargetEntity` is typed `CVehicle*`, but the game stores any entity (here, a ped) in it.
    heli->m_autoPilot.m_TargetEntity = reinterpret_cast<CVehicle*>(target);

    heli->m_fMaxAltitude = newsHeli ? 30.0f : 20.0f;
    heli->m_fMinAltitude = newsHeli ? 27.0f : 12.0f;

    heli->m_fHeliRotorSpeed = 0.165f;

    // Face away from the target
    const float heading = angle + PI;
    auto&       heliMat = heli->GetMatrix();
    heliMat.GetRight().Set(std::sin(heading), -std::cos(heading), 0.0f);
    heliMat.GetForward().Set(std::cos(heading), std::sin(heading), 0.0f);
    heliMat.GetUp().Set(0.0f, 0.0f, 1.0f);

    CWorld::Add(heli);
    heli->SetUpDriver(-1, false, false);

    // TODO(re): The original searches `pHelis` for a free slot here (0x6C6788), but never stores the result.
    return heli;
}

// 0x6C6890
void CHeli::TestSniperCollision(CVector* origin, CVector* target) {
    CVector point = *target - *origin;

    if (point.z >= point.Magnitude() / 2.0f)
        return;

    for (auto& heli : pHelis) {
        if (!heli || heli->physicalFlags.bBulletProof)
            continue;

        const auto mat = (CMatrix*)heli->m_matrix;
        if (CCollision::DistToLine(*origin, *target, mat->TransformPoint({ -0.43f, 1.49f, 1.5f })) < 0.8f) {
            heli->m_fRotationBalance = (float)(CGeneral::GetRandomNumber() < pow(2, 14) - 1) * 0.1f - 0.05f; // 2^14 - 1 = 16383 [-0.05, 0.05]
            heli->BlowUpCar(FindPlayerPed(), false);
            heli->m_nNumSwatOccupants = 0;
        };
    }
}

// 0x6C69C0
bool CHeli::SendDownSwat() {
    auto* const target = reinterpret_cast<CEntity*>(m_autoPilot.m_TargetEntity);
    const auto  targetPos = target->GetPosition();

    const auto swat = m_nNumSwatOccupants;
    if (!swat) {
        return false;
    }
    if (physicalFlags.bSubmergedInWater) {
        return false;
    }
    // The SWAT ped model must be streamed in before it can be dropped (`0x8E6314` is
    // `ms_aInfoForModel[MODEL_SWAT].m_LoadState`, compared against `LOADSTATE_LOADED`)
    if (!CStreaming::IsModelLoaded(MODEL_SWAT)) {
        return false;
    }
    if (CGeneral::GetRandomNumber() & 0x7F) {
        return false;
    }
    if (DistanceBetweenPoints(GetPosition(), targetPos) >= 50.0f) {
        return false;
    }
    if (m_vecMoveSpeed.Magnitude() > 0.1f) {
        return false;
    }

    // The swat's drop position, in world space
    const CMatrix heliMat{ *m_matrix };
    auto          pos = heliMat.TransformVector(FindSwatPositionRelativeToHeli(swat - 1)) + GetPosition();

    // Make sure there's a gap between the swat and the ground/water below it
    const auto groundZ = CWorld::FindGroundZFor3DCoord(pos, nullptr, nullptr);
    if (std::fabs(pos.z - groundZ) < 2.5f) {
        return false;
    }

    float waterZ{};
    if (CWaterLevel::GetWaterLevelNoWaves(pos, &waterZ) && waterZ > groundZ) {
        return false;
    }

    const auto ropeId = GetRopeId();
    if (!CRopes::RegisterRope(ropeId, 8, pos, false, 0, 0, nullptr, 20'000)) {
        return false;
    }

    // NOTE: `CCopPed` accepts either an `eCopType` or a model id in that slot; the game passes `COP_TYPE_SWAT2` (3).
    auto* const swatPed = CPopulation::AddPed(PED_TYPE_COP, static_cast<eModelID>(COP_TYPE_SWAT2), pos, true);

    {
        auto* const seq = new CTaskComplexSequence();
        seq->AddTask(new CTaskComplexUseSwatRope(ropeId, this));
        seq->AddTask(new CTaskComplexWanderCop(PEDMOVE_WALK, (uint8)CGeneral::GetRandomNumberInRange(0, 8)));
        swatPed->GetIntelligence()->m_TaskMgr.SetTask(seq, static_cast<ePrimaryTasks>(3), false);
    }

    swatPed->m_nFlags &= ~1; // No collision

    m_nNumSwatOccupants--;
    m_aSwatState[swat - 1] = 0xAA;

    // Play the "slide down the rope" animation
    CAnimManager::BlendAnimation(swatPed->GetRpClump(), static_cast<AssocGroupId>(0), static_cast<AnimationId>(0x98), 4.0f);

    return true;
}

// 0x6C79A0
void CHeli::UpdateHelis() {
    NumberOfSearchLights = 0;

    int32 required = FindPlayerWanted(-1)->NumOfHelisRequired();

    // Count the helis in use and check whether they're all police helis
    int32 count{};
    bool  newsHeli{};
    for (auto* const heli : pHelis) {
        if (!heli) {
            continue;
        }
        count++;
        if (heli->m_nModelIndex == MODEL_POLMAV && !heli->physicalFlags.bRenderScorched && !heli->vehicleFlags.bIsDrowning) {
            newsHeli = true;
        }
    }

    if (CCullZones::PlayerNoRain() || CGame::currArea) {
        required = 0;
    }
    if (!bPoliceHelisAllowed) {
        required = 0;
    }
    if (CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT) {
        required = 0;
    }

    // Script helis take over the slots
    for (auto* const heli : pHelis) {
        if (heli && heli->m_nModelIndex == MODEL_VCNMAV) {
            newsHeli = false;
        }
    }

    // The heli's model has to be streamed in before it can be spawned. The retail reads at `0x8E73A4`/`0x8E72F0`
    // are `ms_aInfoForModel[MODEL_POLMAV/MODEL_VCNMAV].m_LoadState` (both compared against `LOADSTATE_LOADED`).
    bool canSpawn;
    if (!CWanted::UseNewsHeliInAdditionToPolice) {
        newsHeli = false;
        canSpawn = CStreaming::IsModelLoaded(MODEL_POLMAV);
    } else if (newsHeli) {
        canSpawn = CStreaming::IsModelLoaded(MODEL_VCNMAV);
    } else {
        canSpawn = CStreaming::IsModelLoaded(MODEL_POLMAV);
    }

    if (canSpawn && TestForNewRandomHelisTimer < CTimer::GetTimeInMS()) {
        TestForNewRandomHelisTimer = CTimer::GetTimeInMS() + 15'000;

        if (count < required) {
            auto* const heli = GenerateHeli(FindPlayerPed(-1), newsHeli);

            auto* const slot = pHelis[0] ? (pHelis[1] ? nullptr : &pHelis[1]) : &pHelis[0];
            if (slot) {
                *slot = heli;
                heli->RegisterReference(reinterpret_cast<CEntity**>(slot));
            }
        }
    }

    for (auto& slot : pHelis) {
        auto* const heli = slot;
        if (!heli) {
            continue;
        }

        if (heli->physicalFlags.bRenderScorched || heli->vehicleFlags.bIsDrowning) {
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
            slot = nullptr;
            continue;
        }

        if (heli->m_autoPilot.m_nCarMission == MISSION_HELI_FLY_AWAY_FROM_PLAYER
            && DistanceBetweenPoints(heli->GetPosition(), FindPlayerCoors()) > 170.0f
        ) {
            CWorld::Remove(heli);
            delete heli;
            slot = nullptr;
        }
    }

    // Retire the helis that aren't needed anymore
    for (auto* const heli : pHelis) {
        if (!heli || heli->m_autoPilot.m_nCarMission == MISSION_HELI_FLY_AWAY_FROM_PLAYER) {
            continue;
        }
        if (required < 1) {
            heli->m_autoPilot.m_nCarMission = MISSION_HELI_FLY_AWAY_FROM_PLAYER;
            heli->m_fMaxAltitude = 100.0f;
            heli->m_fMinAltitude = 100.0f;
        } else {
            required--;
        }
    }
}

// 0x6C7C50
void CHeli::RenderAllHeliSearchLights() {
    ZoneScoped;

    for (auto& light : HeliSearchLights) {
        SearchLightCone(
            light.m_nCoronaIndex,
            light.m_vecOrigin,
            light.m_vecTarget,
            light.m_fTargetRadius,
            light.m_fPower,
            light.field_24,
            light.m_bDrawShadow,
            light.m_vecUseless[0],
            light.m_vecUseless[1],
            light.m_vecUseless[2],
            false,
            0.05f,
            0.0f,
            0.0f,
            1.0f
        );
    }
}

// 0x6C6D30
void CHeli::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    if (!vehicleFlags.bCanBeDamaged) {
        return;
    }

    // Ambient helis and the RC ones that aren't the player's just crash instead of exploding
    if (GetStatus() != STATUS_PLAYER
        && m_autoPilot.m_nCarMission != MISSION_HELI_CRASH_AND_BURN
        && m_nModelIndex != MODEL_RCRAIDER
        && m_nModelIndex != MODEL_RCGOBLIN
    ) {
        m_autoPilot.m_nCarMission = MISSION_HELI_CRASH_AND_BURN;
        m_fHealth = 0.0f;
        return;
    }

    if (damager == FindPlayerPed(-1) || damager == FindPlayerVehicle(-1, false)) {
        auto& playerInfo = FindPlayerInfo();
        playerInfo.m_nHavocCaused += 20;
        playerInfo.m_fCurrentChaseValue += 10.0f;
        CStats::IncrementStat(STAT_COST_OF_PROPERTY_DAMAGED, (float)CGeneral::GetRandomNumberInRange(4'000, 10'000));
    }

    if (m_nModelIndex == MODEL_VCNMAV) {
        CWanted::UseNewsHeliInAdditionToPolice = false;
    }

    if (GetStatus() == STATUS_PLAYER) {
        m_nFlags &= 0xFFFFFF7Eu;
        ResetMoveSpeed();
        ResetTurnSpeed();
    }

    SetStatus(STATUS_WRECKED);
    physicalFlags.bRenderScorched = true;
    m_nTimeWhenBlowedUp           = CTimer::GetTimeInMS();

    CVisibilityPlugins::SetClumpForAllAtomicsFlag(GetRpClump(), eAtomicComponentFlag::ATOMIC_PIPE_NO_EXTRA_PASSES_LOD);
    m_damageManager.FuckCarCompletely(false);

    if (m_nModelIndex != MODEL_RCRAIDER && m_nModelIndex != MODEL_RCGOBLIN) {
        SetBumperDamage(FRONT_BUMPER, false);
        SetBumperDamage(REAR_BUMPER, false);
        for (auto door = 0; door < 6; door++) {
            SetDoorDamage(static_cast<eDoors>(door), false);
        }

        SpawnFlyingComponent(CAR_WHEEL_LF, 1);

        if (auto* const node = m_aCarNodes[CAR_WHEEL_LF]) {
            RpAtomic* atomic{};
            RwFrameForAllObjects(node, GetCurrentAtomicObjectCB, &atomic);
            if (atomic) {
                rwObjectSetFlags(atomic, 0);
            }
        }
    }

    m_nBombOnBoard    = 0;
    m_nOverrideLights = 0;
    m_fHealth         = 0.0f;
    m_wBombTimer      = 0;

    TheCamera.CamShake(0.4f, GetPosition());
    KillPedsInVehicle();

    vehicleFlags.bSirenOrAlarm = false;
    autoFlags.bTaxiLight       = false;
    if (vehicleFlags.bIsAmbulanceOnDuty) {
        vehicleFlags.bIsAmbulanceOnDuty = false;
        --CCarCtrl::NumAmbulancesOnDuty;
    }
    if (vehicleFlags.bIsFireTruckOnDuty) {
        vehicleFlags.bIsFireTruckOnDuty = false;
        --CCarCtrl::NumFireTrucksOnDuty;
    }

    ChangeLawEnforcerState(false);
    gFireManager.StartFire(this, damager, 0.8f, 1, 7000, 0);
    CDarkel::RegisterCarBlownUpByPlayer(*this, 0);

    CExplosion::AddExplosion(
        this,
        damager,
        (m_nModelIndex == MODEL_RCRAIDER || m_nModelIndex == MODEL_RCGOBLIN) ? EXPLOSION_SMALL : EXPLOSION_AIRCRAFT,
        GetPosition(),
        0,
        1,
        -1.0f,
        0
    );
}

// 0x6C4530
void CHeli::Fix() {
    m_damageManager.ResetDamageStatus();
    SetupDamageAfterLoad();
}

// 0x6C4330
bool CHeli::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6C4320
bool CHeli::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6C4830
void CHeli::ProcessControlInputs(uint8 playerNum) {
    auto* const pad = CPad::GetPad(playerNum);

    m_fAccelerationBreakStatus = (float)(pad->GetAccelerate() - pad->GetBrake()) / 255.0f;

    // Reads the sticks into `m_fSteeringUpDown`/`m_fSteeringLeftRight`
    const auto ReadSticks = [&, this] {
        CVehicle::m_nLastControlInput = eControllerType::KEYBOARD;
        m_fSteeringUpDown    = (float)pad->GetSteeringUpDown() / 128.0f;
        m_fSteeringLeftRight = -(float)pad->GetSteeringLeftRight() / 128.0f;
    };

    if (!CCamera::m_bUseMouse3rdPerson || !CVehicle::m_bEnableMouseFlying) {
        ReadSticks();
    } else {
        const auto& mouseDelta = CPad::NewMouseControllerState.m_AmountMoved;
        const bool  sticksIdle = pad->GetSteeringUpDown() == 0 && pad->GetSteeringLeftRight() == 0;
        const bool  wasMouse   = CVehicle::m_nLastControlInput == eControllerType::MOUSE;
        const bool  mouseActive =
            mouseDelta.x != 0.0f ||
            mouseDelta.y != 0.0f ||
            (wasMouse && sticksIdle && (m_fSteeringLeftRight != 0.0f || m_fSteeringUpDown != 0.0f));

        if (mouseActive) {
            CVehicle::m_nLastControlInput = eControllerType::MOUSE;

            if (!GetPadNewStateField2C(*pad)) {
                m_fSteeringLeftRight -= mouseDelta.x * 0.0025f;
                m_fSteeringUpDown   += mouseDelta.y * 0.0025f;
            }

            const float decay = std::pow(0.98f, CTimer::GetTimeStep());
            if (std::fabs(m_fSteeringLeftRight) < 0.5f) {
                m_fSteeringLeftRight *= decay;
            }
            if (std::fabs(m_fSteeringUpDown) < 0.5f) {
                m_fSteeringUpDown *= decay;
            }
        } else if (!(wasMouse && sticksIdle)) {
            ReadSticks();
        }
    }

    m_fSteeringUpDown    = std::clamp(m_fSteeringUpDown,    -1.0f, 1.0f);
    m_fSteeringLeftRight = std::clamp(m_fSteeringLeftRight, -1.0f, 1.0f);

    // Yaw/rudder comes from the look buttons
    m_fLeftRightSkid = (float)(pad->GetLookRight() ? 1 : 0);
    if (pad->GetLookLeft()) {
        m_fLeftRightSkid = -1.0f;
    }

    // While the horn is held and the heli is upright, translate the velocity into cyclic pitch/roll
    if (pad->GetHorn() && GetUpVector().z > 0.0f) {
        const CVector worldUp{ 0.0f, 0.0f, 1.0f };

        m_fLeftRightSkid = 0.0f;

        const auto axis1 = Normalized(CrossProduct(worldUp, GetRightVector()));
        m_fSteeringUpDown = std::clamp(
            DotProduct(axis1, m_vecMoveSpeed) * m_pFlyingHandlingData->m_fPitchStab,
            -2.0f, 2.0f
        );

        const auto axis2 = Normalized(CrossProduct(worldUp, GetForwardVector()));
        m_fSteeringLeftRight = std::clamp(
            DotProduct(axis2, m_vecMoveSpeed) * m_pFlyingHandlingData->m_fRollStab,
            -2.0f, 2.0f
        );
    }

    // Helis don't use the wheel controls
    m_f2ndSteerAngle  = 0.0f;
    m_GasPedal        = 0.0f;
    m_BrakePedal      = 1.0f;
    vehicleFlags.bIsHandbrakeOn = false;

    if (pad->DisablePlayerControls) {
        FindPlayerPed(-1)->KeepAreaAroundPlayerClear();

        // Clamp the horizontal speed while the controls are disabled
        const float speed = m_vecMoveSpeed.Magnitude();
        if (speed > 0.28f) {
            m_vecMoveSpeed *= 0.28f / speed;
        }
    }

    if (m_fHealth < 250.0f) {
        m_fAccelerationBreakStatus = -0.1f;
        m_fLeftRightSkid += 0.5f;
    }
}

// 0x6C4400
void CHeli::Render() {
    auto* mi = GetVehicleModelInfo();
    m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 3000;
    mi->SetVehicleColour(m_nPrimaryColor, m_nSecondaryColor, m_nTertiaryColor, m_nQuaternaryColor);

    auto staticRotor = m_aCarNodes[HELI_STATIC_ROTOR];
    RpAtomic* data = nullptr;
    if (staticRotor) {
        RwFrameForAllObjects(staticRotor, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 255);
    }

    auto staticRotor2 = m_aCarNodes[HELI_STATIC_ROTOR2];
    data = nullptr;
    if (staticRotor2) {
        RwFrameForAllObjects(staticRotor2, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 255);
    }

    auto movingRotor = m_aCarNodes[HELI_MOVING_ROTOR];
    data = nullptr;
    if (movingRotor) {
        RwFrameForAllObjects(movingRotor, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 0);
    }

    auto movingRotor2 = m_aCarNodes[HELI_MOVING_ROTOR2];
    data = nullptr;
    if (movingRotor2) {
        RwFrameForAllObjects(movingRotor2, GetCurrentAtomicObjectCB, &data);
        if (data)
            CVehicle::SetComponentAtomicAlpha(data, 0);
    }

    CEntity::Render(); // exactly CEntity
}

// 0x6C4550
void CHeli::SetupDamageAfterLoad() {
    vehicleFlags.bIsDamaged = false;
}

// 0x6C4E60
void CHeli::ProcessFlyingCarStuff() {
    const auto status = GetStatus();

    // Not flying (parked/abandoned etc.): wind the rotor down and switch the engine off
    if (status != STATUS_PLAYER && status != STATUS_FORCED_STOP && status != STATUS_PHYSICS) {
        if (!(m_pHandlingData->m_nHandlingFlags & VEHICLE_HANDLING_LOW_RIDER)) {
            return;
        }

        vehicleFlags.bEngineOn = false;

        const auto rotorDelta = CTimer::GetTimeStep() * 0.00055f;
        if (m_fHeliRotorSpeed > rotorDelta) {
            m_fHeliRotorSpeed -= rotorDelta;
            m_nFakePhysics = 0;
        } else {
            m_fHeliRotorSpeed = 0.0f;
        }
        return;
    }

    // Spin the rotor up until it can lift the heli
    if (m_fHeliRotorSpeed < 0.22f && !physicalFlags.bSubmergedInWater) {
        m_fHeliRotorSpeed += (m_nModelIndex == MODEL_RCRAIDER || m_nModelIndex == MODEL_RCGOBLIN) ? 0.003f : 0.001f;
    }

    if (m_fHeliRotorSpeed >= 0.15f) {
        const bool isFlyingModel = physicalFlags.bTouchingWater
                                && (m_nModelIndex == MODEL_SEASPAR || m_nModelIndex == MODEL_LEVIATHN);

        if (vehicleFlags.bIsRCVehicle) {
            CVehicle::FlyingControl(
                FLIGHTMODEL_RCHELI,
                m_fLeftRightSkid,
                m_fSteeringUpDown,
                m_fSteeringLeftRight,
                m_fAccelerationBreakStatus
            );
        } else {
            // Don't apply the flight controls while resting on all wheels without any input
            const bool landedAndIdle =
                   (m_nNumContactWheels >= 4 || isFlyingModel)
                && !(m_fAccelerationBreakStatus > 0.0f)
                && std::fabs(m_vecMoveSpeed.x) <= 0.02f
                && std::fabs(m_vecMoveSpeed.y) <= 0.02f
                && std::fabs(m_vecMoveSpeed.z) <= 0.02f;

            if (!landedAndIdle) {
                CVehicle::FlyingControl(
                    FLIGHTMODEL_HELI,
                    m_fLeftRightSkid,
                    m_fSteeringUpDown,
                    m_fSteeringLeftRight,
                    m_fAccelerationBreakStatus
                );
            }
        }
    }

    // Blade collision with the ground
    if (m_fHeliRotorSpeed > 0.015f) {
        if (auto* const rotorNode = m_aCarNodes[HELI_MOVING_ROTOR]) {
            CMatrix rotorMat{};
            rotorMat.Attach(RwFrameGetMatrix(rotorNode), false);

            RpAtomic* bladeAtomic{};
            RwFrameForAllObjects(rotorNode, GetCurrentAtomicObjectCB, &bladeAtomic);

            if (bladeAtomic) {
                if (RpAtomicGetFlags(bladeAtomic) & rpATOMICRENDER) {
                    _rpAtomicResyncInterpolatedSphere(bladeAtomic);
                }

                const float bladeRadius = RpAtomicGetBoundingSphere(bladeAtomic)->radius;
                if (bladeRadius > 0.1f) {
                    const float damageMult =
                        (m_nModelIndex == MODEL_RCRAIDER || m_nModelIndex == MODEL_RCGOBLIN) ? 0.9f :
                        (m_nModelIndex == MODEL_SPARROW  || m_nModelIndex == MODEL_SEASPAR)    ? 0.8f :
                        (m_nModelIndex == MODEL_HUNTER)                                        ? 0.5f :
                                                                                                 1.0f;
                    if (status == STATUS_PLAYER || status == STATUS_FORCED_STOP) {
                        DoBladeCollision(rotorMat.GetPosition(), *m_matrix, -3, bladeRadius, damageMult);
                    }
                }
            }
        }
    }

    if (status == STATUS_PLAYER || status == STATUS_PHYSICS) {
        // Rotor downwash pushes the nearby wind modifiers
        if (m_fHeliRotorSpeed > 0.0075f) {
            CWindModifiers::RegisterOne(GetPosition(), 1, std::min(m_fHeliRotorSpeed * 6.66667f, 1.0f));
        }
    }

    // Blade sound when the camera is close to the rotor's disc
    if (m_nModelIndex != MODEL_RCRAIDER && m_nModelIndex != MODEL_RCGOBLIN) {
        if (m_fHeliRotorSpeed > 0.0044f && m_fHeliRotorSpeed <= 0.154f) {
            if (auto* const rotorNode = m_aCarNodes[HELI_MOVING_ROTOR]) {
                const auto camPos = TheCamera.GetPosition();
                if (DistanceBetweenPointsSquared(camPos, GetPosition()) < 400.0f
                    && std::fabs(m_fPropRotate - m_wheelRotation[1]) > 0.523599f
                ) {
                    // TODO(re): The original builds the rotor's matrix and multiplies a vector of it with the
                    //           heli's matrix (0x6C52DC..0x6C53B3) to obtain the direction compared here; the
                    //           x87 register flow is not recoverable byte-exactly, so the rotor's `at` axis is used.
                    CMatrix rotorMat{};
                    rotorMat.Attach(RwFrameGetMatrix(rotorNode), false);

                    const auto toCam = Normalized(camPos - GetPosition());
                    const auto bladeDir = Normalized(m_matrix->TransformVector(rotorMat.GetForward()));

                    if (std::fabs(DotProduct(bladeDir, toCam)) > 0.95f) {
                        m_vehicleAudio.AddAudioEvent(AE_HELI_BLADE, false);
                        m_fPropRotate = m_wheelRotation[1];
                    }
                }
            }
        }
    }
}

// 0x6C5420
void CHeli::PreRender() {
    CVehicle::PreRender();

    // Searchlight
    if (m_bSearchLightEnabled && m_fSearchLightIntensity > 0.0f && CClock::GetIsTimeInRange(19, 6)) {
        // TODO(re): The origin is built from the heli's matrix and a local offset via 0x59C890 (0x6C5500),
        //           which the decompilation does not resolve; the radius/corona-index arguments are unknown too.
        AddHeliSearchLight(
            GetMatrix().TransformVector(CVector{ 0.0f, 3.5f, -0.3f }),
            m_vecSearchLightTarget,
            20.0f,
            m_fSearchLightIntensity,
            0,
            true,
            true
        );
    }

    // Landing gear compression
    if (vehicleFlags.bVehicleColProcessed) {
        // TODO(re): A virtual call on vtable slot `+0xD0` happens here (0x6C555E); the slot isn't resolved.
        for (auto i = 0; i < 4; i++) {
            const float wheelRadius = 1.0f - m_aSuspensionSpringLength[i] / m_aSuspensionLineLength[i];
            const float compression = (m_fWheelsSuspensionCompression[i] - wheelRadius) / (1.0f - wheelRadius);

            CVector wheelPos;
            GetVehicleModelInfo()->GetWheelPosn(i, wheelPos, true);

            float gearZ = wheelPos.z + m_pHandlingData->m_fSuspensionUpperLimit;
            if (compression > 0.0f) {
                gearZ -= compression * m_aSuspensionSpringLength[i];
            }
            if (gearZ <= m_wheelPosition[i]
                && (!physicalFlags.bAddMovingCollisionSpeed || !(m_nHandlingFlagsIntValue & VEHICLE_HANDLING_HYDRAULIC_INST))
            ) {
                gearZ = (gearZ - m_wheelPosition[i]) * 0.75f + m_wheelPosition[i];
            }
            m_wheelPosition[i] = gearZ;
        }
    }

    UpdateWheelMatrix(4, 1);
    UpdateWheelMatrix(7, 1);
    UpdateWheelMatrix(2, 1);
    UpdateWheelMatrix(5, 1);

    if (m_nModelIndex != MODEL_RCRAIDER && m_nModelIndex != MODEL_RCGOBLIN) {
        DoHeliDustEffect(1.0f, 1.0f);
    }

    // Rotor spin
    const bool isPoliceHeli = m_nModelIndex == MODEL_SPARROW
                           || m_nModelIndex == MODEL_SEASPAR
                           || m_nModelIndex == MODEL_MAVERICK
                           || m_nModelIndex == MODEL_VCNMAV
                           || m_nModelIndex == MODEL_POLMAV;

    float rotorZ = std::bit_cast<float>(m_fRotorZ);
    rotorZ -= CTimer::GetTimeStep() * m_fHeliRotorSpeed * (isPoliceHeli ? 1.66f : 1.0f);
    while (rotorZ < -TWO_PI) {
        rotorZ += TWO_PI;
    }
    m_fRotorZ = std::bit_cast<int32>(rotorZ);

    float secondRotorZ = std::bit_cast<float>(m_fSecondRotorZ);
    secondRotorZ -= CTimer::GetTimeStep() * m_fHeliRotorSpeed * (m_nModelIndex == MODEL_LEVIATHN ? 2.0f : 2.3f);
    while (secondRotorZ > TWO_PI) {
        secondRotorZ -= TWO_PI;
    }
    m_fSecondRotorZ = std::bit_cast<int32>(secondRotorZ);

    // Rotate the rotor nodes
    const auto RotateNode = [this](eHeliNodes node, bool aroundZ) {
        auto* const frame = m_aCarNodes[node];
        if (!frame) {
            return;
        }
        CMatrix mat{};
        mat.Attach(RwFrameGetMatrix(frame), false);
        const CVector pos = mat.GetPosition();
        if (aroundZ) {
            mat.SetRotateZ(std::bit_cast<float>(m_fRotorZ));
        } else {
            mat.SetRotateX(std::bit_cast<float>(m_fSecondRotorZ));
        }
        mat.GetPosition() += pos;
        mat.UpdateRW();
    };

    RotateNode(HELI_STATIC_ROTOR,  true);
    RotateNode(HELI_MOVING_ROTOR,  true);
    RotateNode(HELI_STATIC_ROTOR2, false);
    RotateNode(HELI_MOVING_ROTOR2, false);

    CShadows::StoreShadowForVehicle(this, VEH_SHD_HELI);
}

// 0x6C7050
void CHeli::ProcessControl() {
    CAutomobile::ProcessControl();

    // Kill the rotor dust FX when the engine is off
    if (!vehicleFlags.bEngineOn && m_pDustParticle) {
        m_pDustParticle->Kill();
        m_pDustParticle     = nullptr;
        m_heliDustFxTimeConst = 0.0f;
    }

    auto* const pad = CPad::GetPad(m_pDriver && m_pDriver->GetPadNumber() == 1 ? 1 : 0);
    if (pad->HornJustDown()) {
        m_bSearchLightEnabled = !m_bSearchLightEnabled;
    }

    do {
        if (physicalFlags.bRenderScorched || CCullZones::PlayerNoRain()) {
            m_fSearchLightIntensity = 0.0f;
            break;
        }

        // Pick the entity to aim the searchlight at
        bool      attack{};
        CEntity*  target{};
        if (m_autoPilot.m_nCarMission == MISSION_HELI_POLICE_BEHAVIOUR) {
            auto* const playerVeh = FindPlayerVehicle(-1, false);
            if (!playerVeh || (playerVeh->m_nVehicleSubType != VEHICLE_TYPE_PLANE && playerVeh->m_nVehicleSubType != VEHICLE_TYPE_HELI)) {
                attack = true;
                target = FindPlayerEntity(-1);
            }
        } else if (m_autoPilot.m_nCarMission == MISSION_HELI_FOLLOW_ENTITY
                && m_autoPilot.m_TargetEntity
                && (m_nHeliFlags & 2)
        ) {
            target = reinterpret_cast<CEntity*>(m_autoPilot.m_TargetEntity);
            attack = true;
        } else if (GetStatus() == STATUS_PLAYER && m_nModelIndex == MODEL_POLMAV && m_bSearchLightEnabled) {
            attack = true;
        }

        if (physicalFlags.bSubmergedInWater) {
            attack = false;
        }

        m_bSearchLightEnabled = attack;
        if (!attack) {
            break;
        }

        // The point the searchlight is aimed at
        CVector aimPos;
        CVector aimSpeed;
        if (target) {
            aimPos   = target->GetPosition();
            aimSpeed = reinterpret_cast<CPhysical*>(target)->m_vecMoveSpeed; // TODO(re): the target isn't always a `CPhysical`
        } else {
            aimPos   = GetPosition() + GetUpVector() * -30.0f + GetForwardVector() * 10.0f;
            aimSpeed = m_vecMoveSpeed;
        }

        const auto previousTarget = m_vecSearchLightTarget;

        // The 5-sample history ring: `field_9C0`/`f9CC`/`f9D0` are the X samples, the two "history" arrays the Y ones
        // (see 0x6C7310). The header doesn't declare them as 5-entry arrays.
        auto* const historyX = reinterpret_cast<float*>(&field_9C0);                    // 0x9C0, 5 floats
        auto* const historyY = reinterpret_cast<float*>(&m_aSearchLightHistoryX[0]);     // 0x9D8, 5 floats

        const uint32 now = CTimer::GetTimeInMS();
        if (now - m_nSearchLightTimer > 1000) {
            const uint32 samplesToAdd = (now - m_nSearchLightTimer - 1000) / 1000 + 1;

            for (auto s = 0u; s < samplesToAdd; s++) {
                for (auto k = 4; k > 0; k--) {
                    historyX[k] = historyX[k - 1];
                    historyY[k] = historyY[k - 1];
                }
                // Extrapolate the new sample from the aim point and the aim entity's velocity
                // (`100.0f` is the constant at `0x858628`)
                historyX[0] = aimPos.x + aimSpeed.x * 100.0f;
                historyY[0] = aimPos.y + aimSpeed.y * 100.0f;
                m_nSearchLightTimer += 1000;
            }
        }

        m_vecSearchLightTarget.x = aimPos.x;
        // TODO(re): the two interpolated values written at 0x9F4/0x9F8 are blended from the history ring
        //           and the current sample (0x6C7352..0x6C73A2); not recoverable from the decompilation.
        m_vecSearchLightTarget.y = aimPos.y;
        m_vecSearchLightTarget.z = aimPos.z;

        // Intensity from the 2-D distance between the aim point and the heli
        const auto dist2D = (aimPos - GetPosition()).Magnitude2D();
        if (dist2D > 60.0f) {
            m_fSearchLightIntensity = 0.0f;
        } else if (dist2D < 40.0f) {
            m_fSearchLightIntensity = 1.0f;
        } else {
            m_fSearchLightIntensity = 1.0f - (dist2D - 40.0f) * 0.05f;
        }

        // Reset the firing timers when the light isn't focused or the target has moved far enough
        const float movedSq = sq(aimPos.x - previousTarget.x) + sq(aimPos.y - previousTarget.y);
        if (m_fSearchLightIntensity < 0.9f || movedSq > 49.0f) {
            m_nShootTimer            = now;
            m_nTimeForMinigunFiring  = now;
        } else if (now > m_nPoliceShoutTimer) {
            m_nPoliceShoutTimer = now + (CGeneral::GetRandomNumber() & 0xFFF) + 4500;
        }

        // Shout at / shoot the player
        {
            auto* const wanted = FindPlayerWanted(-1);

            int32 interval;
            switch (static_cast<int32>(wanted->m_WantedLevel)) {
            case 0:  interval = 999'999; break;
            case 1:  interval = 10'000;  break;
            case 2:  interval = 5'000;   break;
            case 3:  interval = 3'500;   break;
            case 4:  interval = 2'000;   break;
            default: interval = 0;       break; // TODO(re): the original uses a stack value here (0x6C750F)
            }

            if (wanted->m_WantedLevel != eWantedLevel::WANTED_CLEAN) {
                AudioEngine.SayPedless(
                    static_cast<eAudioEvents>(0x34),
                    static_cast<eGlobalSpeechContext>(0xAE),
                    this,
                    0,
                    1.0f,
                    false,
                    false,
                    false
                );
            }

            if (CCullZones::NoPolice()) {
                interval /= 2;
            }
            if (target != FindPlayerPed(-1)) {
                interval = 5'000;
            }

            if (FindPlayerWanted(-1)->m_Flags & 7) {
                m_nShootTimer           = now;
                m_nTimeForMinigunFiring = now;
            } else {
                // The gun's ray starts slightly above and behind the heli
                const auto rayStart = GetMatrix().TransformVector(CVector{ 0.0f, 3.5f, -1.0f });

                if (now > m_nShootTimer + interval
                    && ms_previousTimeInMilliseconds <= m_nShootTimer + interval
                    && !CWorld::GetIsLineOfSightClear(GetPosition(), rayStart, true, false, false, false, false, false, false)
                ) {
                    m_nShootTimer           = now;
                    m_nTimeForMinigunFiring = now;
                }

                if (now > m_nShootTimer + interval && now > m_nTimeForMinigunFiring) {
                    // Jitter the aim point, then fire along the (3m long) direction to it
                    aimPos.x += (float)((int32)(CGeneral::GetRandomNumber() & 0xFF) - 0x80) * 0.02f;
                    aimPos.y += (float)((int32)(CGeneral::GetRandomNumber() & 0xFF) - 0x80) * 0.02f;

                    const auto dir = Normalized(aimPos - GetPosition()) * 3.0f;
                    FireOneInstantHitRound(aimPos, aimPos + dir, 20);

                    AudioEngine.ReportWeaponEvent(0x91, static_cast<eWeaponType>(0x1F), this);

                    m_nTimeForMinigunFiring = now + (CGeneral::GetRandomNumberInRange(0.0f, 1.0f) < 0.15f ? 400 : 150);
                }
            }
        }
    } while (false);

    // Drop swat members
    if (m_autoPilot.m_nCarMission == MISSION_HELI_POLICE_BEHAVIOUR && m_nNumSwatOccupants > 0) {
        SendDownSwat();
        g_InterestingEvents.Add(CInterestingEvents::ZELDICK_OCCUPATION, this);
    }

    // Keep the ropes of the swat members being lowered in place
    for (auto i = 0; i < (int32)m_aSwatState.size(); i++) {
        if (!m_aSwatState[i]) {
            continue;
        }

        m_aSwatState[i]--;

        const auto ropeId = GetRopeId();
        const auto pos    = GetMatrix().TransformVector(FindSwatPositionRelativeToHeli(i)) + GetPosition();
        CRopes::RegisterRope(ropeId, 8, pos, false, 0, 0, nullptr, 20'000);

        if (!m_aSwatState[i]) {
            CRopes::SetSpeedOfTopNode(ropeId, GetMatrix().TransformVector(FindSwatPositionRelativeToHeli(i) * 0.125f));
        }
    }

    UpdateWinch();
    ProcessWeapons();

    // Fire this event much more often while the player is idling (the idle camera is active)
    if (gIdleCamFlags & 1) {
        float probability = CTimer::GetTimeStep() * 0.02f * 0.1f;
        probability *= 2.0f; // TODO(re): doubled when the heli is attacking (0x6C792F); the flag isn't tracked here

        if ((float)CGeneral::GetRandomNumber() * 3.05185e-05f < probability) {
            g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_21, this);
        }
    }
}
