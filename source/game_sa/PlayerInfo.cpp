#include "StdInc.h"

#include "PlayerInfo.h"
#include "FireManager.h"
#include "MenuSystem.h"
#include "Hud.h"
#include "Tasks/TaskTypes/TaskSimpleHoldEntity.h"
#include "Tasks/TaskTypes/TaskSimpleJetPack.h"
#include "Tasks/TaskTypes/TaskSimpleSwim.h"
#include "Tasks/TaskTypes/TaskComplexEnterCarAsPassenger.h"
#include "Tasks/TaskTypes/TaskComplexLeaveCar.h"

void CPlayerInfo::InjectHooks() {
    RH_ScopedClass(CPlayerInfo);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Constructor, 0x571920, { .State = HS::RedirectToGTA, .Locked = true }); // hooking ctor will produce bugs with weapons, you will never give weapon through cheat or something
    RH_ScopedInstall(CancelPlayerEnteringCars, 0x56E860);
    RH_ScopedInstall(FindObjectToSteal, 0x56DBD0);
    RH_ScopedInstall(EvaluateCarPosition, 0x56DAD0);
    RH_ScopedInstall(Process, 0x56F8D0, { .Reversed = false });
    RH_ScopedInstall(FindClosestCarSectorList, 0x56F4E0);
    RH_ScopedInstall(Clear, 0x56F330);
    RH_ScopedInstall(StreamParachuteWeapon, 0x56EB30);
    RH_ScopedInstall(AddHealth, 0x56EAB0);
    RH_ScopedInstall(ArrestPlayer, 0x56E5D0);
    RH_ScopedInstall(SetPlayerSkin, 0x5717F0);
    RH_ScopedInstall(LoadPlayerSkin, 0x56F7D0);
    RH_ScopedInstall(DeletePlayerSkin, 0x56EA80);
    RH_ScopedInstall(BlowUpRCBuggy, 0x56EA30);
    RH_ScopedInstall(MakePlayerSafe, 0x56E870);
    RH_ScopedInstall(PlayerFailedCriticalMission, 0x56E830);
    RH_ScopedInstall(WorkOutEnergyFromHunger, 0x56E610);
    RH_ScopedInstall(KillPlayer, 0x56E580);
    RH_ScopedInstall(IsRestartingAfterMissionFailed, 0x56E570);
    RH_ScopedInstall(IsRestartingAfterArrest, 0x56E560);
    RH_ScopedInstall(IsRestartingAfterDeath, 0x56E550);
    RH_ScopedInstall(IsPlayerInRemoteMode, 0x56DAB0);
    RH_ScopedInstall(GetPos_Hook, 0x56DFB0);
    RH_ScopedInstall(GetSpeed_Hook, 0x56DF50);
    RH_ScopedInstall(GivePlayerParachute, 0x56EC40);
    RH_ScopedInstall(SetLastTargetVehicle, 0x56DA80);
    RH_ScopedInstall(Load, 0x5D3B00);
    RH_ScopedInstall(Save, 0x5D3AC0);
}

// 0x571920
CPlayerInfo::CPlayerInfo() {
    // NOTE: `m_PlayerData` is constructed by `CPlayerPedData::CPlayerPedData` (which the exe inlines here).
    // Everything else is left alone on purpose: the exe keeps `CWorld::Players` in `.bss`, so those members
    // are zero there.
    m_pSkinTexture           = nullptr;
    m_bParachuteReferenced   = false;
    m_nRequireParachuteTimer = 0;
}

// 0x571920
CPlayerInfo* CPlayerInfo::Constructor() {
    this->CPlayerInfo::CPlayerInfo();
    return this;
}

CVector* CPlayerInfo::GetSpeed_Hook(CVector* out) {
    *out = GetSpeed();
    return out;
}

CVector* CPlayerInfo::GetPos_Hook(CVector* outPos) {
    *outPos = GetPos();
    return outPos;
}

// 0x56E860
void CPlayerInfo::CancelPlayerEnteringCars(CVehicle* vehicle) {
    // NOP
}

// 0x56DBD0
CEntity* CPlayerInfo::FindObjectToSteal(CPed* ped) {
    const auto& pedPos     = ped->GetPosition();
    const auto& pedForward = ped->GetForward();
    const auto& pedRight   = ped->GetRight();

    // The searched area is a 6x6 square around a point 1.5m in front of the ped
    const auto centre = pedPos + pedForward * 1.5f;

    const auto sectorLowX  = std::max(0, CWorld::GetSectorX(centre.x - 3.0f));
    const auto sectorLowY  = std::max(0, CWorld::GetSectorY(centre.y - 3.0f));
    const auto sectorHighX = std::min(MAX_SECTORS_X - 1, CWorld::GetSectorX(centre.x + 3.0f));
    const auto sectorHighY = std::min(MAX_SECTORS_Y - 1, CWorld::GetSectorY(centre.y + 3.0f));

    CWorld::AdvanceCurrentScanCode();

    if (sectorHighY < sectorLowY) {
        return nullptr;
    }

    CEntity* result = nullptr;
    for (auto y = sectorLowY; y <= sectorHighY; y++) {
        for (auto x = sectorLowX; x <= sectorHighX; x++) {
            for (CObject* object : CWorld::GetRepeatSector(x, y).Objects) {
                if (!object->objectFlags.bIsLiftable || object->IsScanCodeCurrent()) {
                    continue;
                }

                const auto delta = object->GetPosition() - centre;
                if (delta.SquaredMagnitude() >= 4.5f) {
                    continue;
                }

                // NOTE: Due to the distance check above the facing test below never actually rejects anything
                const auto fwdDot = delta.Dot(pedForward);
                const auto score  = fwdDot < 0.0f ? 10.0f - fwdDot : fwdDot;
                if (score + std::abs(delta.Dot(pedRight)) * 3.0f < 1000.0f) {
                    result = object; // NOTE: The exe keeps overwriting this - so the last object found wins
                }
            }
        }
    }
    return result;
}

// 0x56DAD0
void CPlayerInfo::EvaluateCarPosition(CEntity* car, CPed* ped, float pedToVehDist, float* outDistance, CVehicle** outVehicle) {
    const auto& carPosn = car->GetPosition();
    const auto& pedPosn = ped->GetPosition();
    const auto& forward = ped->GetForward();

    // Find our rotation (so that is, at which angle the forward vector is)
    const auto angleFront = CGeneral::GetATanOfXY(forward.x, forward.y);

    // Find car angle, not relative to our rotation
    const auto carAngle = CGeneral::GetATanOfXY(carPosn.x - pedPosn.x, carPosn.y - pedPosn.y);

    // Make car's angle relative to our rotation (notice: it's an abs value)
    // Basically, we calculate how much the car is in our FOV.
    const auto carAngleFromFront = std::abs(CGeneral::LimitRadianAngle(angleFront - carAngle));

    // Calculate imaginary distance based on the car's angle: The higher the angle the greater the distance
    const auto distance = (1.0f - carAngleFromFront / TWO_PI) * (10.0f - pedToVehDist);
    if (distance >= *outDistance) {
        *outDistance = distance;
        *outVehicle = car->AsVehicle();
    }
}

// 0x56F7D0
void CPlayerInfo::LoadPlayerSkin() {
    DeletePlayerSkin();
    m_pSkinTexture = CPlayerSkin::GetSkinTexture(m_szSkinName);
}

// 0x56EA80
void CPlayerInfo::DeletePlayerSkin() {
    if (m_pSkinTexture) {
        RwTextureDestroy(m_pSkinTexture);
        m_pSkinTexture = nullptr;
    }
}

// 0x5717F0
void CPlayerInfo::SetPlayerSkin(const char* name) {
    strcpy_s(m_szSkinName, name); // NOTSA: They used `strcpy`, we use `_s` for safety
    LoadPlayerSkin();
}

// 0x56DA80
void CPlayerInfo::SetLastTargetVehicle(CVehicle* vehicle) {
    CEntity::SafeCleanUpRef(m_pLastTargetVehicle);
    m_pLastTargetVehicle = vehicle;
    CEntity::SafeRegisterRef(m_pLastTargetVehicle);
}

// 0x56EC80
//! Integrates the pad's car-gun crosshair aiming and fires the car gun.
//! The exe's `this` is `&CPlayerInfo::m_nCrosshairActivated`; the two stack args are the player index and his pad.
static void UpdateCrosshairAndCarGun(CPlayerInfo& info, uint32 playerIndex, CPad* pad) {
    if (!info.m_nCrosshairActivated) {
        return;
    }

    const auto timeStep = CTimer::GetTimeStep();

    info.m_vecCrosshairTarget.x += (float)pad->GetSteeringLeftRight() * timeStep * 0.0003333333f;
    if (CPad::bInvertLook4Pad) {
        info.m_vecCrosshairTarget.y += (float)pad->GetSteeringUpDown() * timeStep * 0.0003333333f;
    } else {
        info.m_vecCrosshairTarget.y -= (float)pad->GetSteeringUpDown() * timeStep * 0.0003333333f;
    }

    info.m_vecCrosshairTarget.x = std::clamp(info.m_vecCrosshairTarget.x, -0.9f, 0.9f);
    info.m_vecCrosshairTarget.y = std::clamp(info.m_vecCrosshairTarget.y, -0.9f, 0.9f);

    if (!pad->GetCarGunFired()) {
        return;
    }

    CamShakeNoPos(&TheCamera, 0.2f);

    // TODO(re): The brief leaves the camera basis the exe uses unresolved - it reads three `CVector`s inside
    //           `TheCamera` (0xB6F9A0/0xB6F9AC/0xB6F9B8) and divides the `tan` argument by a runtime-initialised
    //           float (0xC3EFA4, BSS). The aim below uses the active camera's documented basis and `FindCamFOV`;
    //           it is the closest documented equivalent.
    const auto& cam        = TheCamera.GetActiveCam();
    const auto  front      = CVector{ cam.m_vecFront }.Normalized();
    const auto  up         = CVector{ cam.m_vecUp }.Normalized();
    const auto  right      = up.Cross(front);
    const auto  tanHalfFov = std::tan(TheCamera.FindCamFOV() * (PI / 360.0f));

    const CVector aim = front
                      + up    * (tanHalfFov * info.m_vecCrosshairTarget.y)
                      - right * (tanHalfFov * info.m_vecCrosshairTarget.x);

    CWeapon          gun{ WEAPON_M4, 5000 };
    CPlayerPed* const ped = CWorld::Players[playerIndex].m_pPed;

    CVector origin = cam.m_vecSource;
    CVector muzzle = cam.m_vecSource;
    CVector target = origin + aim;

    // NOTE: The exe clears `CPed::bCrouchWhenShooting` for the duration of the shot
    const auto wasCrouchWhenShooting = ped->bCrouchWhenShooting;
    ped->bCrouchWhenShooting = false;
    gun.FireInstantHit(ped, &origin, &muzzle, nullptr, &target, nullptr, true, true);
    ped->bCrouchWhenShooting = wasCrouchWhenShooting;
}

// 0x56F8D0
void CPlayerInfo::Process(uint32 playerIndex) {
    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    CPad* const pad = CPad::GetPad(playerIndex);

    // ---- Taxi fare ----
    if (m_bTaxiTimerScore && m_pPed->bInVehicle) {
        CVehicle* const veh = m_pPed->m_pVehicle;
        if ((veh->m_nModelIndex == MODEL_TAXI || veh->m_nModelIndex == MODEL_CABBIE)
            && veh->m_pDriver == m_pPed
            && veh->m_nNumPassengers != 0
        ) {
            // NOTE: The timer is only advanced while the fare is being collected
            if (const auto elapsed = CTimer::GetTimeInMS() - m_nTaxiTimer; elapsed > 999) {
                m_nMoney     += elapsed / 1000;
                m_nTaxiTimer += elapsed / 1000 * 1000;
            }
        } else {
            m_nTaxiTimer = CTimer::GetTimeInMS();
        }
    } else {
        m_nTaxiTimer = CTimer::GetTimeInMS();
    }

    // ---- Time spent on two wheels / wheelies / stoppies ----
    // NOTE: The counters are in milliseconds (see `CAutomobile::ProcessControl`, which compares
    //       `m_nCarLess3WheelCounter` against 500/1000).
    const auto frameMS = static_cast<uint32>(CTimer::GetTimeStepInMS());

    if (CVehicle* const veh = m_pPed->bInVehicle ? m_pPed->m_pVehicle : nullptr) {
        if (veh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
            CAutomobile* const car = veh->AsAutomobile();

            // Time spent with less than 3 wheels on the ground
            m_nCarLess3WheelCounter = car->m_nNumContactWheels < 3
                ? m_nCarLess3WheelCounter + frameMS
                : 0;

            const auto& prev = car->m_fWheelsSuspensionCompressionPrev;
            const auto  damageIntensity = car->m_fDamageIntensity;

            const auto RecordTwoWheels = [this] {
                m_fBestCarTwoWheelsDistM  = m_fCarTwoWheelDist;
                m_nBestCarTwoWheelsTimeMs = m_nCarTwoWheelCounter;
                CStats::SetNewRecordStat(STAT_LONGEST_2_WHEELS_TIME, (float)(m_nCarTwoWheelCounter / 1000));
                CStats::SetNewRecordStat(STAT_LONGEST_2_WHEELS_DISTANCE, m_fCarTwoWheelDist);
            };

            const auto StopTwoWheels = [&, this] {
                if (m_nCarTwoWheelCounter > 1999) {
                    RecordTwoWheels();
                }
                m_fCarTwoWheelDist    = 0.0f;
                m_nTempBufferCounter  = 0;
                m_nCarTwoWheelCounter = 0;
            };

            if (prev[CAR_WHEEL_FRONT_RIGHT] != 1.0f || prev[CAR_WHEEL_REAR_RIGHT] != 1.0f) {
                // At least one right wheel is touching the ground
                if (prev[CAR_WHEEL_FRONT_LEFT] == 1.0f && prev[CAR_WHEEL_REAR_LEFT] == 1.0f) {
                    // ... and both left wheels are in the air
                    if (prev[CAR_WHEEL_FRONT_RIGHT] < 1.0f && prev[CAR_WHEEL_REAR_RIGHT] < 1.0f && damageIntensity == 0.0f) {
                        // Driving on the two right wheels
                        m_nCarTwoWheelCounter += frameMS;
                        m_fCarTwoWheelDist    += car->m_fMovingSpeed;
                        m_nTempBufferCounter   = frameMS;
                    } else if (m_nCarTwoWheelCounter != 0 && m_nTempBufferCounter < 500) {
                        m_nTempBufferCounter -= frameMS;
                    } else {
                        StopTwoWheels();
                    }
                } else if (m_nCarTwoWheelCounter != 0) {
                    StopTwoWheels();
                }
            } else if (prev[CAR_WHEEL_FRONT_LEFT] >= 1.0f || prev[CAR_WHEEL_REAR_LEFT] >= 1.0f || damageIntensity != 0.0f) {
                // Both right wheels are in the air, but the left ones aren't on the ground either
                if (m_nCarTwoWheelCounter == 0 || m_nTempBufferCounter > 499) {
                    StopTwoWheels();
                } else {
                    m_nTempBufferCounter -= frameMS;
                }
            } else {
                // Driving on the two left wheels
                m_nCarTwoWheelCounter += frameMS;
                m_fCarTwoWheelDist    += car->m_fMovingSpeed;
                m_nTempBufferCounter   = frameMS;
            }

            m_nBikeRearWheelCounter  = 0;
            m_nBikeFrontWheelCounter = 0;
        } else if (veh->m_nVehicleType == VEHICLE_TYPE_BIKE) {
            CBike* const bike = veh->AsBike();

            // Ratios: 0/1 = front suspension lines, 2/3 = rear ones (1.0 = fully relaxed, i.e. in the air)
            const auto& ratio           = bike->m_aWheelRatios;
            const auto  damageIntensity = bike->m_fDamageIntensity;

            const auto RecordWheelie = [this] {
                m_fBestBikeWheelieDistM  = m_fBikeRearWheelDist;
                m_nBestBikeWheelieTimeMs = m_nBikeRearWheelCounter;
                CStats::SetNewRecordStat(STAT_LONGEST_WHEELIE_TIME, (float)(m_nBikeRearWheelCounter / 1000));
                CStats::SetNewRecordStat(STAT_LONGEST_WHEELIE_DISTANCE, m_fBikeRearWheelDist);
            };
            const auto RecordStoppie = [this] {
                m_fBestBikeStoppieDistM  = m_fBikeFrontWheelDist;
                m_nBestBikeStoppieTimeMs = m_nBikeFrontWheelCounter;
                CStats::SetNewRecordStat(STAT_LONGEST_STOPPIE_TIME, (float)(m_nBikeFrontWheelCounter / 1000));
                CStats::SetNewRecordStat(STAT_LONGEST_STOPPIE_DISTANCE, m_fBikeFrontWheelDist);
            };

            if (ratio[0] == 1.0f && ratio[1] == 1.0f && m_nBikeRearWheelCounter == 0) {
                if (ratio[2] < 1.0f || (ratio[3] < 1.0f && damageIntensity == 0.0f)) {
                    // Wheelie
                    m_nBikeRearWheelCounter += frameMS;
                    m_fBikeRearWheelDist    += bike->m_fMovingSpeed;
                    m_nTempBufferCounter     = frameMS;
                    m_nCarTwoWheelCounter    = 0;
                    m_nCarLess3WheelCounter  = 0;
                } else if (m_nBikeRearWheelCounter != 0 && m_nTempBufferCounter < 500) {
                    m_nTempBufferCounter    -= frameMS;
                    m_nCarTwoWheelCounter    = 0;
                    m_nCarLess3WheelCounter  = 0;
                } else {
                    if (m_nBikeRearWheelCounter >= 5000) {
                        RecordWheelie();
                    }
                    m_nBikeRearWheelCounter  = 0;
                    m_fBikeRearWheelDist     = 0.0f;
                    m_nTempBufferCounter     = 0;
                    m_nCarTwoWheelCounter    = 0;
                    m_nCarLess3WheelCounter  = 0;
                }
            } else {
                if (m_nBikeRearWheelCounter == 0) {
                    if (ratio[2] == 1.0f && ratio[3] == 1.0f) {
                        if (ratio[0] < 1.0f || (ratio[1] < 1.0f && damageIntensity == 0.0f)) {
                            // Stoppie
                            m_nBikeFrontWheelCounter += frameMS;
                            m_fBikeFrontWheelDist    += bike->m_fMovingSpeed;
                            m_nTempBufferCounter      = frameMS;
                            m_nCarTwoWheelCounter     = 0;
                            m_nCarLess3WheelCounter   = 0;
                        } else if (m_nBikeFrontWheelCounter != 0 && m_nTempBufferCounter < 500) {
                            m_nTempBufferCounter    -= frameMS;
                            m_nCarTwoWheelCounter    = 0;
                            m_nCarLess3WheelCounter  = 0;
                        } else if (m_nBikeFrontWheelCounter > 1999) {
                            RecordStoppie();
                        }
                    } else if (m_nBikeFrontWheelCounter > 1999) {
                        RecordStoppie();
                    }
                    m_nBikeFrontWheelCounter = 0;
                    m_fBikeFrontWheelDist    = 0.0f;
                } else {
                    if (m_nBikeRearWheelCounter > 4999) {
                        RecordWheelie();
                    }
                    m_nBikeRearWheelCounter = 0;
                    m_fBikeRearWheelDist    = 0.0f;
                }
                m_nTempBufferCounter     = 0;
                m_nCarTwoWheelCounter    = 0;
                m_nCarLess3WheelCounter  = 0;
            }
        } else {
            m_nCarLess3WheelCounter  = 0;
            m_nTempBufferCounter     = 0;
            m_nCarTwoWheelCounter    = 0;
            m_nBikeRearWheelCounter  = 0;
            m_nBikeFrontWheelCounter = 0;
        }
    } else {
        m_nCarLess3WheelCounter  = 0;
        m_nTempBufferCounter     = 0;
        m_nCarTwoWheelCounter    = 0;
        m_nBikeRearWheelCounter  = 0;
        m_nBikeFrontWheelCounter = 0;
    }

    WorkOutEnergyFromHunger();

    // ---- Money display "flashing" ladder ----
    if (m_nDisplayMoney != m_nMoney) {
        const int32 diff = m_nMoney - m_nDisplayMoney;
        int32       step = std::abs(diff);
        if (step < 100001) {
            if (step < 10001) {
                step = step < 1001
                    ? (step < 51 ? 1 : 42)
                    : 123;
            } else {
                step = 1234;
            }
        } else {
            step = 12345;
        }
        m_nDisplayMoney += diff < 0 ? -step : step;
    }

    // ---- Max health ----
    m_pPed->m_fMaxHealth = CStats::GetFatAndMuscleModifier(STAT_MOD_MAX_HEALTH);
    // TODO(re): The exe stores the result of its `_ftol2` call here, i.e. the same value it uses for the
    //           counter increments above (`CTimer::GetTimeStepInMS()`); the brief labels it "random" but its
    //           own notes say it is a truncated float, and no other operand is in scope.
    m_nMaxHealth = static_cast<uint8>(CTimer::GetTimeStepInMS());

    // ---- Parachute ----
    if (!m_pPed->bInVehicle || CStats::GetStatValue(STAT_FLYING_SKILL) < 400.0f) {
        if (m_bParachuteReferenced && CGameLogic::IsCoopGameGoingOn() && m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced   = false;
            m_nRequireParachuteTimer = 0;
        }
    } else {
        StreamParachuteWeapon(true);
    }

    // ---- Road density around the player ----
    if ((CTimer::GetFrameCounter() & 0xF) == 0) {
        const auto pos = m_pPed->bInVehicle ? m_pPed->m_pVehicle->GetPosition() : m_pPed->GetPosition();
        m_fRoadDensityAroundPlayer = ThePaths.CalcRoadDensity(pos.x, pos.y);
    }
    m_fRoadDensityAroundPlayer = (m_fRoadDensityAroundPlayer - 1.0f) * 0.6f + 1.0f;
    m_fRoadDensityAroundPlayer = std::max(m_fRoadDensityAroundPlayer, 0.5f);
    m_fRoadDensityAroundPlayer = std::min(m_fRoadDensityAroundPlayer, 1.45f);

    // ---- Exit-vehicle button ----
    if (!pad->GetTarget() && m_pPed->bCanExitCar && !m_pPed->bUsingMobilePhone
        && (pad->ExitVehicleJustDown() || (m_bTryingToExitCar && pad->GetExitVehicle() && m_pPed->bInVehicle))
    ) {
        m_bTryingToExitCar = false;

        if (!m_pPed->bInVehicle) {
            if (!m_pPed->m_pAttachedTo) {
                CEntity* const objectToSteal = FindObjectToSteal(m_pPed);
                auto&          taskMgr       = m_pPed->GetIntelligence()->GetTaskManager();
                CTask* const   simplestTask  = taskMgr.GetSimplestActiveTask();
                CTaskSimpleHoldEntity* const holdTask = m_pPed->GetIntelligence()->GetTaskHold(false);
                if (simplestTask && simplestTask->GetTaskType() != TASK_SIMPLE_CLIMB
                    && !m_pPed->GetIntelligence()->GetTaskFighting()
                    && (!holdTask || !holdTask->m_pEntityToHold)
                ) {
                    if (objectToSteal) {
                        // TODO(re): The exe builds `0x6919C0(entityToSteal, 0x51)` (a 0x34-byte task of an
                        //           unknown class - `CTaskComplexGoPickUpEntity` matches the size/purpose, but
                        //           the repo declares no ctor for it) and posts it via a `CEventScriptCommand`.
                        //           Until that class is constructible nothing is posted here.
                        NOTSA_LOG_WARN("CPlayerInfo::Process: picking up an object is not implemented");
                    } else {
                        // Look for a nearby car to get into
                        const auto  pedPos = m_pPed->GetPosition();
                        const float minX   = pedPos.x - 10.0f;
                        const float maxX   = pedPos.x + 10.0f;
                        const float minY   = pedPos.y - 10.0f;
                        const float maxY   = pedPos.y + 10.0f;

                        const auto lowX  = CWorld::GetSectorX(minX);
                        const auto lowY  = CWorld::GetSectorY(minY);
                        const auto highX = CWorld::GetSectorX(maxX);
                        const auto highY = CWorld::GetSectorY(maxY);

                        CWorld::AdvanceCurrentScanCode();

                        CVehicle* bestVehicle = nullptr;
                        float     bestDist    = 0.0f;
                        for (auto y = lowY; y <= highY; y++) {
                            for (auto x = lowX; x <= highX; x++) {
                                FindClosestCarSectorList(CWorld::GetRepeatSector(x, y).Vehicles, m_pPed, minX, minY, maxX, maxY, &bestDist, &bestVehicle);
                            }
                        }

                        if (bestVehicle) {
                            // Enter the towing vehicle instead of the trailer
                            if (bestVehicle->m_nVehicleSubType == VEHICLE_TYPE_TRAILER && bestVehicle->m_pTowingVehicle) {
                                bestVehicle = bestVehicle->m_pTowingVehicle;
                            }

                            if (bestVehicle->CanBeDriven()) {
                                // Leave the jetpack behind before getting in
                                if (CTaskSimpleJetPack* const jetPackTask = m_pPed->GetIntelligence()->GetTaskJetPack()) {
                                    jetPackTask->DropJetPack(m_pPed);
                                }

                                bool canEnter          = true;
                                bool forceEnterAsDriver = false;
                                if (bestVehicle->m_nVehicleType == VEHICLE_TYPE_BOAT) {
                                    // A boat can only be taken over when it's empty
                                    forceEnterAsDriver = true;
                                    if (bestVehicle->m_pDriver) {
                                        canEnter = false;
                                    } else if (CTask* const activeTask = taskMgr.GetActiveTask()) {
                                        canEnter = activeTask->MakeAbortable(m_pPed, ABORT_PRIORITY_URGENT, nullptr);
                                    }
                                } else if (CTask* const activeTask = taskMgr.GetActiveTask()) {
                                    // While swimming to a boat, first swim to its door
                                    canEnter = false;
                                    if (activeTask->GetTaskType() == TASK_COMPLEX_IN_WATER) {
                                        if (CTaskSimpleSwim* const swimTask = m_pPed->GetIntelligence()->GetTaskSwim()) {
                                            switch (bestVehicle->m_nModelIndex) {
                                            case MODEL_SKIMMER:
                                            case MODEL_VORTEX:
                                            case MODEL_SEASPAR:
                                            case MODEL_LEVIATHN: {
                                                CVector doorPos;
                                                int32   doorId = 0;
                                                if (CCarEnterExit::GetNearestCarDoor(m_pPed, bestVehicle, doorPos, doorId)) {
                                                    swimTask->m_vecPos    = doorPos;
                                                    swimTask->m_nTimeStep = 5000;
                                                    canEnter = true;
                                                }
                                                break;
                                            }
                                            }
                                        }
                                    }
                                    if (!canEnter) {
                                        canEnter = activeTask->MakeAbortable(m_pPed, ABORT_PRIORITY_URGENT, nullptr);
                                    }
                                }

                                if (canEnter) {
                                    CTask* task = nullptr;
                                    if (forceEnterAsDriver || !CWorld::Players[1].m_pPed || CGameLogic::bPlayersCanBeInSeparateCars) {
                                        task = new CTaskComplexEnterCarAsDriver{ bestVehicle };
                                    } else {
                                        // Two players: don't take the driver's seat if the other player is already
                                        // using this vehicle
                                        const auto otherPed = CWorld::Players[(playerIndex + 1) % MAX_PLAYERS].m_pPed;
                                        // TODO(re): The exe reads the other player's primary event-response tasks
                                        //           (`CPedIntelligence + 0x10`/`+0x14`, i.e. the temp. and non-temp.
                                        //           event responses) and, for the enter-car task types below, the
                                        //           target vehicle stored in the task.
                                        const auto GetEnterCarTaskTarget = [](CTask* task) -> CVehicle* {
                                            if (!task) {
                                                return nullptr;
                                            }
                                            switch (task->GetTaskType()) {
                                            case TASK_COMPLEX_ENTER_CAR_AS_PASSENGER:
                                            case TASK_COMPLEX_ENTER_CAR_AS_DRIVER:
                                            case TASK_COMPLEX_STEAL_CAR:
                                            case TASK_COMPLEX_DRAG_PED_FROM_CAR:
                                                return static_cast<CTaskComplexEnterCar*>(task)->GetTargetCar();
                                            }
                                            return nullptr;
                                        };
                                        CVehicle* const otherEnterTarget = otherPed ? GetEnterCarTaskTarget(otherPed->GetIntelligence()->GetTaskManager().GetTaskPrimary(TASK_PRIMARY_EVENT_RESPONSE_TEMP)) : nullptr;
                                        CVehicle* const otherVehicle     = otherPed ? otherPed->m_pVehicle : nullptr;
                                        if (otherEnterTarget == bestVehicle) {
                                            task = new CTaskComplexEnterCarAsPassenger{ bestVehicle };
                                        } else if (otherEnterTarget || otherVehicle) {
                                            task = nullptr; // Let the other player have it
                                        } else {
                                            task = new CTaskComplexEnterCarAsDriver{ bestVehicle };
                                        }
                                    }
                                    if (task) {
                                        taskMgr.SetTask(task, TASK_PRIMARY_PRIMARY, false);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else if (!m_pRemoteVehicle) {
            CVehicle* const veh = m_pPed->m_pVehicle;
            if ((!veh->m_pEntityWeAreOn || !CBridge::ThisIsABridgeObjectMovingUp(veh->m_pEntityWeAreOn->m_nModelIndex))
                && veh->GetStatus() != STATUS_WRECKED
                && veh->GetStatus() != STATUS_TRAIN_MOVING
                && veh->m_nDoorLock != CARLOCK_LOCKED_PLAYER_INSIDE
            ) {
                auto& taskMgr = m_pPed->GetIntelligence()->GetTaskManager();
                if (CTask* const activeTask = taskMgr.GetActiveTask();
                    !activeTask || activeTask->MakeAbortable(m_pPed, ABORT_PRIORITY_URGENT, nullptr)
                ) {
                    if (veh->m_nVehicleType == VEHICLE_TYPE_BOAT) {
                        taskMgr.SetTask(new CTaskComplexLeaveCar{ veh, 0, 0, true, false }, TASK_PRIMARY_PRIMARY, false);
                        m_pPed->bTryingToReachDryLand = true;
                    } else if (veh->CanPedStepOutCar(false) || veh->CanPedJumpOutCar(m_pPed)) {
                        taskMgr.SetTask(new CTaskComplexLeaveCar{ veh, 0, 0, true, false }, TASK_PRIMARY_PRIMARY, false);
                        m_bTryingToExitCar = true;
                        GivePlayerParachute();
                    } else if (veh->GetStatus() == STATUS_PLAYER) {
                        m_bTryingToExitCar = true;
                    }
                } else {
                    m_bTryingToExitCar = true;
                }
            }
        }
    } else {
        m_bTryingToExitCar = false;
    }

    // ---- After the remote-controlled vehicle blew up ----
    if (m_bAfterRemoteVehicleExplosion) {
        const auto msSinceExplosion = CTimer::GetTimeInMS() - m_nTimeOfRemoteVehicleExplosion;
        if (CTimer::GetPreviousTimeInMS() - m_nTimeOfRemoteVehicleExplosion < 1000
            && msSinceExplosion > 999
            && m_nPlayerState == PLAYERSTATE_PLAYING
            && m_bFadeAfterRemoteVehicleExplosion
        ) {
            TheCamera.SetFadeColour(0, 0, 0);
            TheCamera.Fade(1.0f, eFadeFlag::FADE_IN);
        }

        if (msSinceExplosion > 2000) {
            if (m_nPlayerState == PLAYERSTATE_PLAYING && m_bFadeAfterRemoteVehicleExplosion) {
                TheCamera.RestoreWithJumpCut();
                TheCamera.SetFadeColour(0, 0, 0);
                TheCamera.Fade(1.0f, eFadeFlag::FADE_OUT);
                TheCamera.Process();
                CTimer::Stop();
                CRenderer::RequestObjectsInFrustum(nullptr, 0);
                CStreaming::LoadAllRequestedModels(false);
                CTimer::Update();
            }
            m_bAfterRemoteVehicleExplosion = false;

            CPlayerInfo& focusInfo = CWorld::Players[CWorld::PlayerInFocus];
            if (focusInfo.m_pRemoteVehicle) {
                focusInfo.m_pRemoteVehicle->m_bRemoveFromWorld = true;
            }
            focusInfo.m_pRemoteVehicle = nullptr;

            if (CPed* const focusPed = focusInfo.m_pPed;
                focusPed && focusPed->bInVehicle && focusPed->m_pVehicle
            ) {
                focusPed->m_pVehicle->SetStatus(STATUS_PLAYER);
            }
        }
    }

    // ---- Upside-down vehicles blow up ----
    if ((CTimer::GetFrameCounter() & 0x1F) == 0) {
        CPlayerInfo&    focusInfo = CWorld::Players[CWorld::PlayerInFocus];
        CPed* const     focusPed  = focusInfo.m_pPed;
        CVehicle* const focusVeh  = focusPed && focusPed->bInVehicle ? focusPed->m_pVehicle : nullptr;

        if (!focusVeh || !m_pPed->bInVehicle) {
            m_nTimesUpsideDownInARow = 0;
        } else if (focusVeh->GetUp().z >= 0.0f || focusVeh->GetUp().Magnitude() >= 0.05f) {
            m_nTimesUpsideDownInARow = 0;
        } else if ((focusVeh->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE && focusVeh->m_nVehicleType != VEHICLE_TYPE_BOAT)
                || focusPed->physicalFlags.bSubmergedInWater
        ) {
            m_nTimesUpsideDownInARow = 0;
        } else {
            m_nTimesUpsideDownInARow += focusVeh->GetUp().z >= -0.5f ? 1 : 2;

            if (m_nTimesUpsideDownInARow > 6 && focusVeh->vehicleFlags.bCanBeDamaged) {
                focusVeh->m_fHealth = focusVeh->m_fHealth <= 249.0f ? focusVeh->m_fHealth : 250.0f;
                if (focusVeh->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
                    focusVeh->AsAutomobile()->m_damageManager.SetEngineStatus(225);
                    // TODO(re): The exe also clears a dword at `CVehicle + 0x928` right after `SetEngineStatus`;
                    //           that member was not identified, so it is left untouched here.
                }
            }
        }
    }

    // ---- Distance-travelled / flight-time stats ----
    CPed* const focusPedForStats = CWorld::Players[CWorld::PlayerInFocus].m_pPed;
    if (CVehicle* const focusVeh = focusPedForStats && focusPedForStats->bInVehicle ? focusPedForStats->m_pVehicle : nullptr) {
        if (focusVeh->m_nModelIndex == MODEL_CADDY) {
            CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_GOLF_CART, focusVeh->m_fMovingSpeed);
        } else if (focusVeh->m_nVehicleSubType == VEHICLE_TYPE_BMX) {
            CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_BICYCLE, focusVeh->m_fMovingSpeed);
        } else {
            switch (focusVeh->GetVehicleAppearance()) {
            case VEHICLE_APPEARANCE_HELI:
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_HELICOPTER, focusVeh->m_fMovingSpeed);
                break;
            case VEHICLE_APPEARANCE_PLANE:
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_PLANE, focusVeh->m_fMovingSpeed);
                break;
            case VEHICLE_APPEARANCE_AUTOMOBILE:
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_CAR, focusVeh->m_fMovingSpeed);
                break;
            case VEHICLE_APPEARANCE_BIKE:
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_MOTORBIKE, focusVeh->m_fMovingSpeed);
                break;
            case VEHICLE_APPEARANCE_BOAT:
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_BOAT, focusVeh->m_fMovingSpeed);
                break;
            }

            if ((focusVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE || focusVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI)
                && focusVeh->m_vecMoveSpeed.Magnitude() > 0.2f
            ) {
                CStats::IncrementStat(STAT_FLIGHT_TIME, CTimer::GetTimeStep() * 16.0f);
            }
        }
    } else if (m_pPed->m_fMovingSpeed > 0.0f) {
        // On foot: walking, swimming or jetpacking
        CPad* const pad0 = CPad::GetPad(0);
        if (!m_pPed->GetIntelligence()->GetTaskSwim()) {
            if (!m_pPed->GetIntelligence()->GetTaskJetPack()) {
                CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_ON_FOOT, m_pPed->m_fMovingSpeed);
            } else {
                CStats::IncrementStat(STAT_TIME_ON_JETPACK, CTimer::GetTimeStep() * 16.0f);
            }
        } else if (pad0->GetPedWalkLeftRight() != 0 || pad0->GetAccelerate() != 0) {
            CStats::IncrementStat(STAT_DISTANCE_TRAVELLED_BY_SWIMMING, m_pPed->m_fMovingSpeed);
        }
    }

    // ---- Driving-style stats (not while on a train) ----
    if (CVehicle* const focusVeh = focusPedForStats && focusPedForStats->bInVehicle ? focusPedForStats->m_pVehicle : nullptr;
        focusVeh && !FindPlayerTrain()
    ) {
        if (focusVeh->m_vecMoveSpeed.SquaredMagnitude() > 0.0f) {
            switch (focusVeh->GetVehicleAppearance()) {
            case VEHICLE_APPEARANCE_BIKE:
                if (focusVeh->m_nVehicleSubType != VEHICLE_TYPE_BMX) {
                    CStats::UpdateStatsWhenOnMotorBike(focusVeh->AsBike());
                }
                break;
            case VEHICLE_APPEARANCE_HELI:
            case VEHICLE_APPEARANCE_PLANE:
                CStats::UpdateStatsWhenFlying(focusVeh);
                break;
            case VEHICLE_APPEARANCE_BOAT:
                break;
            default:
                CStats::UpdateStatsWhenDriving(focusVeh);
                break;
            }
        }
    }

    // ---- Chase value ("heat") ----
    {
        // The exe keeps these in `.bss`; it samples the player's position every 20 seconds
        static auto& lastSampledCoors = StaticRef<CVector>(0xB9B994);
        static auto& foundNodeNearPlayer = StaticRef<bool>(0x8CDF20);
        static auto& playerMovedFarSinceLastSample = StaticRef<bool>(0x8CDF21);

        const auto wanted = m_pPed->GetPlayerData()->m_pWanted;
        if (!wanted->m_ChaosLevel || CTheScripts::IsPlayerOnAMission()) {
            m_fCurrentChaseValue = 0.0f;
        } else {
            if (CTimer::GetTimeInMS() / 20000 != CTimer::GetPreviousTimeInMS() / 20000) {
                const auto coors = FindPlayerCoors();
                playerMovedFarSinceLastSample = (lastSampledCoors - coors).Magnitude() >= 10.0f;
                lastSampledCoors              = coors;
                foundNodeNearPlayer           = ThePaths.FindNodeClosestToCoors(coors, PATH_TYPE_PED, 60.0f, 1, 0, 0, 0, 0).IsAreaValid();
            }

            float targetChaseValue = 0.0f;
            switch (wanted->m_ChaosLevel) {
            case 1: targetChaseValue = 31.0f;   break;
            case 2: targetChaseValue = 62.0f;   break;
            case 3: targetChaseValue = 125.0f;  break;
            case 4: targetChaseValue = 250.0f;  break;
            case 5: targetChaseValue = 500.0f;  break;
            case 6: targetChaseValue = 1000.0f; break;
            }

            const auto delta = (targetChaseValue - m_fCurrentChaseValue) * CTimer::GetTimeStep() * 0.0001f;
            if (delta < 0.0f
                || (playerMovedFarSinceLastSample
                    && foundNodeNearPlayer
                    && !CCullZones::NoPolice()
                    && !CCullZones::PoliceAbandonCars()
                    && CGame::currArea == 0
                )
            ) {
                m_fCurrentChaseValue += delta;
            }
        }
    }

    UpdateCrosshairAndCarGun(*this, playerIndex, pad);

    // ---- Clamp the money ----
    if (m_nMoney > 999'999'998) {
        m_nMoney = 999'999'999;
    }
    if (m_nDisplayMoney > 999'999'998) {
        m_nDisplayMoney = 999'999'999;
    }
}

// 0x56F4E0
void CPlayerInfo::FindClosestCarSectorList(CPtrListDoubleLink<CVehicle*>& ptrList, CPed* ped, float /*minX*/, float /*minY*/, float /*maxX*/, float /*maxY*/, float* outVehDist, CVehicle** outVehicle) {
    // NOTE: `minX`/`minY`/`maxX`/`maxY` are part of the original signature, but the exe never reads them
    const auto& pedPos = ped->GetPosition();

    for (CVehicle* veh : ptrList) {
        if (veh->IsScanCodeCurrent() || !veh->GetUsesCollision() || !veh->GetIsTypeVehicle()) {
            continue;
        }

        const auto status = veh->GetStatus();
        veh->SetCurrentScanCode();
        if (status == STATUS_WRECKED || status == STATUS_TRAIN_MOVING) {
            continue;
        }

        if (!veh->m_matrix) {
            veh->AllocateMatrix();
            veh->m_placement.UpdateMatrix(veh->m_matrix);
        }

        // Skip anything that's (almost) lying on its side, except for bikes
        if (veh->GetUp().z <= 0.3f && !veh->IsBike()) {
            continue;
        }

        CVector boundCentre;
        veh->GetBoundCentre(boundCentre); // NOTE: The result is discarded (the exe overwrites it with the position below)

        const auto& vehPos     = veh->GetPosition();
        const auto  vehBottomZ = vehPos.z - veh->GetHeightAboveRoad() + 1.0f;

        // NOTE: The horizontal terms are multiplied by the `0.0f` constant at 0x858B50 - keep it that way
        auto vehToPedDist = ((pedPos.y - vehPos.y) + (pedPos.x - vehPos.x)) * 0.0f + (pedPos.z - vehBottomZ);

        // The AT400 gets an extra vertical door check
        bool accepted = false;
        if (veh->GetModelIndex() == MODEL_AT400) {
            CVector doorPos{};
            int32   doorId{};
            if (CCarEnterExit::GetNearestCarDoor(ped, veh, doorPos, doorId)) {
                const auto doorZDist = std::abs(pedPos.z - doorPos.z);
                if (doorZDist < 1.0f) {
                    vehToPedDist = doorZDist;
                    accepted     = true;
                }
            }
        }

        if (!accepted) {
            if (std::abs(vehToPedDist) < 2.0f) {
                accepted = true;
            } else if (veh->IsBoat()) {
                // Unless he's standing in it (i.e. the hull is below him by less than 4 units)
                // a boat is only considered when it's what the ped is currently in contact with
                const auto insideHull = vehBottomZ < pedPos.z && vehBottomZ > pedPos.z - 4.0f;
                accepted = insideHull || ped->m_pContactEntity == veh;
            }
        }

        if (!accepted) {
            continue;
        }

        if (!veh->vehicleFlags.bConsideredByPlayer) {
            continue;
        }

        CVector doorPos{};
        int32   doorId{};
        if (!CCarEnterExit::GetNearestCarDoor(ped, veh, doorPos, doorId)) {
            continue;
        }

        auto pedToVehDist = CVector{pedPos.x - vehPos.x, pedPos.y - vehPos.y, 0.0f}.Magnitude();
        bool insideBoundingBox = false;
        if (10.0f < pedToVehDist) {
            if (CPedGeometryAnalyser::LiesInsideBoundingBox(*ped, pedPos, *veh)) {
                pedToVehDist     = 10.0f;
                insideBoundingBox = true; // Skips the distance check below
            }
        }
        if (!insideBoundingBox && !(pedToVehDist < 10.0f)) {
            continue;
        }

        if (CCranes::IsThisCarBeingCarriedByAnyCrane(veh)) {
            continue;
        }
        EvaluateCarPosition(veh, ped, pedToVehDist, outVehDist, outVehicle);
    }
}

// 0x56F330
void CPlayerInfo::Clear() {
    // TODO: This should just use the constructor and `swap`,
    // like to just swap ourselves to a default constructed object
    // and do all the cleanup in the destructor

    m_pPed = nullptr;
    m_pRemoteVehicle = nullptr;
    if (m_pSpecCar) {
        m_pSpecCar->physicalFlags.bAddMovingCollisionSpeed = false;
        m_pSpecCar = nullptr;
    }
    m_nDisplayMoney = 0;
    m_nMoney = 0;
    m_nPlayerState = PLAYERSTATE_PLAYING;
    m_nCarDensityForCurrentZone = 0;
    m_fRoadDensityAroundPlayer = 1.0f;
    m_bAfterRemoteVehicleExplosion = false;
    m_bCreateRemoteVehicleExplosion = false;
    m_bFadeAfterRemoteVehicleExplosion = false;
    m_bTryingToExitCar = false;
    m_bTaxiTimerScore = false;
    m_nTaxiTimer = 0;
    m_nVehicleTimeCounter = CTimer::GetTimeInMS();
    m_nMaxArmour = 100;
    m_nMaxHealth = 100;
    m_bCanDoDriveBy = true;
    m_nCollectablesPickedUp = 0;
    m_nTotalNumCollectables = 3;
    m_nLastTimeEnergyLost = 0;
    m_nLastTimeArmourLost = 0;
    m_nLastTimeBigGunFired = 0;
    m_nTimesStuckInARow = 0;
    m_nTimesUpsideDownInARow = 0;
    m_nCarTwoWheelCounter = 0;
    m_fCarTwoWheelDist = 0.0f;
    m_nCarLess3WheelCounter = 0;
    m_nBikeRearWheelCounter = 0;
    m_fBikeRearWheelDist = 0.0f;
    m_nBikeFrontWheelCounter = 0;
    m_fBikeFrontWheelDist = 0.0f;
    m_nTempBufferCounter = 0;
    m_nBestCarTwoWheelsTimeMs = 0;
    m_fBestCarTwoWheelsDistM = 0.0f;
    m_nBestBikeWheelieTimeMs = 0;
    m_fBestBikeWheelieDistM = 0.0f;
    m_nBestBikeStoppieTimeMs = 0;
    m_fBestBikeStoppieDistM = 0.0f;
    m_bDoesNotGetTired = false;
    m_bFastReload = false;
    m_bFireProof = false;
    m_bGetOutOfJailFree = false;
    m_bFreeHealthCare = false;
    m_nTimeOfLastCarExplosionCaused = 0;
    m_nExplosionMultiplier = 0;
    m_nHavocCaused = 0;
    FindPlayerInfo().m_nNumHoursDidntEat = 0;
    m_nLastBustMessageNumber = 1;
    m_fCurrentChaseValue = 0.0f;
    m_nBustedAudioStatus = 0;
    m_nCrosshairActivated = 0;

    m_nRequireParachuteTimer = 0;

    if (m_bParachuteReferenced) {
        CGameLogic::IsCoopGameGoingOn();
        if (m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced = false;
            m_nRequireParachuteTimer = 0;
        }
    }
}

// 0x56EC40
void CPlayerInfo::GivePlayerParachute() const {
    if (m_nRequireParachuteTimer) {
        if (CStreaming::IsModelLoaded(MODEL_GUN_PARA)) {
            m_pPed->GiveWeapon(WEAPON_PARACHUTE, 1, true);
            m_pPed->SetSavedWeapon(WEAPON_PARACHUTE);
        }
    }
}

// 0x56EB30
void CPlayerInfo::StreamParachuteWeapon(bool unk) {
    if (CGameLogic::IsCoopGameGoingOn()) {
        if (unk) {
            return;
        }
        if (m_bParachuteReferenced) {
            CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
            m_bParachuteReferenced = false;
            m_nRequireParachuteTimer = 0;
        }
        return;
    }

    if (m_pPed && m_pPed->IsInVehicle()) {
        if (m_pPed->m_pVehicle->IsSubPlane() || m_pPed->m_pVehicle->IsSubHeli()) {
            if (m_nRequireParachuteTimer <= (uint32)CTimer::GetTimeStepInMS()) {
                const auto groundHeight = TheCamera.CalculateGroundHeight(eGroundHeightType::ENTITY_BB_BOTTOM);
                const auto vehToGroundZDist = m_pPed->m_pVehicle->GetPosition().z - groundHeight;
                m_nRequireParachuteTimer = (vehToGroundZDist <= 50.f) ? 0 : 5000;
            } else {
                m_nRequireParachuteTimer -= (uint32)CTimer::GetTimeStepInMS();
            }
        }
    }

    if (m_nRequireParachuteTimer) {
        CStreaming::RequestModel(MODEL_GUN_PARA, STREAMING_MISSION_REQUIRED);
        m_bParachuteReferenced = true;
        return;
    }

    if (m_bParachuteReferenced) {
        CStreaming::SetModelIsDeletable(MODEL_GUN_PARA);
        m_bParachuteReferenced = false;
        m_nRequireParachuteTimer = 0;
    }
}

// 0x56EAB0
void CPlayerInfo::AddHealth(int32 amount) const {
    const auto newValue = std::min((float)m_nMaxHealth, m_pPed->m_fHealth + (float)amount); // Clamp to m_nMaxHealth
    m_pPed->m_fHealth = std::max(newValue, m_pPed->m_fHealth); // Don't change health to a lower value
}

// 0x56EA30
void CPlayerInfo::BlowUpRCBuggy(bool bExplode) const {
    if (m_pRemoteVehicle && !m_pRemoteVehicle->m_bRemoveFromWorld) {
        CRemote::TakeRemoteControlledCarFromPlayer(bExplode);
        if (bExplode)
            m_pRemoteVehicle->BlowUpCar(FindPlayerPed(), false);
    }
}

// 0x56E870
void CPlayerInfo::MakePlayerSafe(bool enable, float radius) {
    // Not quite SA, but this is the way to do it (instead of copy pasting it twice)
    auto& flags = m_pPed->physicalFlags;
    flags.bInvulnerable = enable;
    flags.bBulletProof = enable;
    flags.bFireProof = enable;
    flags.bExplosionProof = enable;
    flags.bCollisionProof = enable;
    flags.bMeleeProof = enable;
    m_PlayerData.m_bCanBeDamaged = !enable;
    m_PlayerData.m_pWanted->m_bEverybodyBackOff = enable;
    m_pPed->GetPadFromPlayer()->bPlayerSafe = enable;
    CWorld::SetAllCarsCanBeDamaged(!enable);

    if (enable) {
        CWorld::StopAllLawEnforcersInTheirTracks();
        CPad::StopPadsShaking();

        m_pPed->ClearAdrenaline();
        m_PlayerData.m_fTimeCanRun = std::max(m_PlayerData.m_fTimeCanRun, 0.f);
        m_pPed->GetIntelligence()->ClearTasks(true, false);

        gFireManager.ExtinguishPoint(GetPos(), radius);
        CExplosion::RemoveAllExplosionsInArea(GetPos(), 4000.f);
        CProjectileInfo::RemoveAllProjectiles();
        CWorld::ExtinguishAllCarFiresInArea(GetPos(), radius);
        CReplay::DisableReplays();
        m_pPed->ClearWeaponTarget();
    } else {
        CReplay::EnableReplays();
    }
}

// 0x56E830
void CPlayerInfo::PlayerFailedCriticalMission() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_FAILED_MISSION;
        CGameLogic::SetMissionFailed();
        CDarkel::ResetOnPlayerDeath();
    }
}

// 0x56E610
void CPlayerInfo::WorkOutEnergyFromHunger() {

    static auto& s_lastTimeHungryStateProcessedInitialized = StaticRef<bool>(0xB9B8F4); // false
    static auto& s_lastTimeHungryStateProcessed = StaticRef<uint8>(0xB9B8F2);
    static auto& s_LastHungryState = StaticRef<int8>(0xB9B8F1);
    static auto& s_bHungryMessageShown = StaticRef<bool>(0xB9B8F0);

    if (CCheat::IsActive(CHEAT_NEVER_GET_HUNGRY)) {
        return;
    }

    if (!s_lastTimeHungryStateProcessedInitialized) {
        s_lastTimeHungryStateProcessedInitialized = true;
        s_lastTimeHungryStateProcessed = CClock::GetGameClockHours();
    }

    auto pad = CPad::GetPad();
    if (   pad->ArePlayerControlsDisabled()
        || CMenuSystem::num_menus_in_use
        || TheCamera.m_bWideScreenOn
        || CCutsceneMgr::ms_running
        || CGameLogic::IsCoopGameGoingOn()
        || m_pRemoteVehicle
    ) {
        return;
    }

    if (!m_pPed)
        return;

    if (m_pPed->m_pAttachedTo)
        return;

    if (CClock::GetGameClockHours() != s_lastTimeHungryStateProcessed) {
        if (!m_nNumHoursDidntEat)
            s_LastHungryState = 0;
        m_nNumHoursDidntEat += 1;
    }

    if (m_nNumHoursDidntEat <= 48) {
        s_bHungryMessageShown = false;
    } else {
        if (CClock::GetGameClockHours() == s_lastTimeHungryStateProcessed)
            return;

        m_pPed->Say(CTX_GLOBAL_STOMACH_RUMBLE);
        pad->StartShake(400, 110u, 0);

        if (s_bHungryMessageShown) {
            bool bDecreaseHealth{};
            if (CStats::GetStatValue(STAT_FAT) > 0.0f) {
                CStats::DecrementStat(STAT_FAT, 25.0f);
                CStats::DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_FAT, 25.0f);
                bDecreaseHealth = true;
                if (!s_LastHungryState) {
                    s_LastHungryState = m_nNumHoursDidntEat + 24;
                }
            }

            if (CStats::GetStatValue(STAT_MUSCLE) <= 0.0f || m_nNumHoursDidntEat <= s_LastHungryState && s_LastHungryState) {
                if (!bDecreaseHealth) {
                    m_pPed->m_fHealth -= 2.0f;
                }
            } else {
                CStats::DecrementStat(STAT_MUSCLE, 25.0);
                CStats::DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MUSCLE, 25.0f);
            }
        } else {
            CHud::SetHelpMessage(TheText.Get("NOTEAT"), true, false, true);
            s_bHungryMessageShown = true;
        }
    }

    if (CClock::GetGameClockHours() != s_lastTimeHungryStateProcessed) {
        s_lastTimeHungryStateProcessed = CClock::GetGameClockHours();
    }
}

// 0x56E5D0
void CPlayerInfo::ArrestPlayer() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_HAS_BEEN_ARRESTED;
        m_nBustedAudioStatus = 0;
        CDarkel::ResetOnPlayerDeath();
        CStats::IncrementStat(STAT_TIMES_BUSTED, 1.0f);
        CGangWars::EndGangWar(false);
    }
}

// 0x56E580
void CPlayerInfo::KillPlayer() {
    if (m_nPlayerState == PLAYERSTATE_PLAYING) {
        m_nPlayerState = PLAYERSTATE_HAS_DIED;
        CDarkel::ResetOnPlayerDeath();
        CMessages::AddBigMessage(TheText.Get("DEAD"), 4000, STYLE_WHITE_MIDDLE);
        CStats::IncrementStat(STAT_NUMBER_OF_HOSPITAL_VISITS, 1.0f);
        CGangWars::EndGangWar(false);
    }
}

// 0x56E570
bool CPlayerInfo::IsRestartingAfterMissionFailed() const {
    return m_nPlayerState == PLAYERSTATE_FAILED_MISSION;
}

// 0x56E560
bool CPlayerInfo::IsRestartingAfterArrest() const {
    return m_nPlayerState == PLAYERSTATE_HAS_BEEN_ARRESTED;
}

// 0x56E550
bool CPlayerInfo::IsRestartingAfterDeath() const {
    return m_nPlayerState == PLAYERSTATE_HAS_DIED;
}

// 0x56DAB0
bool CPlayerInfo::IsPlayerInRemoteMode() const {
    return m_pRemoteVehicle || m_bAfterRemoteVehicleExplosion;
}

// 0x56DFB0
// Return occupied vehicle's (if in any) or player's ped position
CVector CPlayerInfo::GetPos() const {
    return m_pPed->IsInVehicle() ? m_pPed->m_pVehicle->GetPosition() : m_pPed->GetPosition();
}

// 0x56DF50
// Return occupied vehicle's (if in any) or player's ped move speed
CVector CPlayerInfo::GetSpeed() const {
    return m_pPed->IsInVehicle() ? m_pPed->m_pVehicle->GetMoveSpeed() : m_pPed->GetMoveSpeed();
}

// 0x5D3B00
bool CPlayerInfo::Load() {
    CGenericGameStorage::LoadDataFromWorkBuffer<int32>(); // Discarded
    auto data = CGenericGameStorage::LoadDataFromWorkBuffer<CPlayerInfoSaveStructure>();
    data.Extract(this);
    return true;
}

// 0x5D3AC0
bool CPlayerInfo::Save() {
    CPlayerInfoSaveStructure data;
    data.Construct(this);
    CGenericGameStorage::SaveDataToWorkBuffer(sizeof(CPlayerInfoSaveStructure));
    CGenericGameStorage::SaveDataToWorkBuffer(data);
    return true;
}

// 0x45DEF0
CPlayerInfo& CPlayerInfo::operator=(const CPlayerInfo& rhs) {
    m_pPed                             = rhs.m_pPed;
    m_PlayerData                       = rhs.m_PlayerData;
    m_pRemoteVehicle                   = rhs.m_pRemoteVehicle;
    m_pSpecCar                         = rhs.m_pSpecCar;
    m_nMoney                           = rhs.m_nMoney;
    m_nDisplayMoney                    = rhs.m_nDisplayMoney;
    m_nCollectablesPickedUp            = rhs.m_nCollectablesPickedUp;
    m_nTotalNumCollectables            = rhs.m_nTotalNumCollectables;
    m_nLastBumpPlayerCarTimer          = rhs.m_nLastBumpPlayerCarTimer;
    m_nTaxiTimer                       = rhs.m_nTaxiTimer;
    m_nVehicleTimeCounter              = rhs.m_nVehicleTimeCounter;
    m_bTaxiTimerScore                  = rhs.m_bTaxiTimerScore;
    m_bTryingToExitCar                 = rhs.m_bTryingToExitCar;
    m_pLastTargetVehicle               = rhs.m_pLastTargetVehicle;
    m_nPlayerState                     = rhs.m_nPlayerState;
    m_bAfterRemoteVehicleExplosion     = rhs.m_bAfterRemoteVehicleExplosion;
    m_bCreateRemoteVehicleExplosion    = rhs.m_bCreateRemoteVehicleExplosion;
    m_bFadeAfterRemoteVehicleExplosion = rhs.m_bFadeAfterRemoteVehicleExplosion;
    m_nTimeOfRemoteVehicleExplosion    = rhs.m_nTimeOfRemoteVehicleExplosion;
    m_nLastTimeEnergyLost              = rhs.m_nLastTimeEnergyLost;
    m_nLastTimeArmourLost              = rhs.m_nLastTimeArmourLost;
    m_nLastTimeBigGunFired             = rhs.m_nLastTimeBigGunFired;
    m_nTimesUpsideDownInARow           = rhs.m_nTimesUpsideDownInARow;
    m_nTimesStuckInARow                = rhs.m_nTimesStuckInARow;
    m_nCarTwoWheelCounter              = rhs.m_nCarTwoWheelCounter;
    m_fCarTwoWheelDist                 = rhs.m_fCarTwoWheelDist;
    m_nCarLess3WheelCounter            = rhs.m_nCarLess3WheelCounter;
    m_nBikeRearWheelCounter            = rhs.m_nBikeRearWheelCounter;
    m_fBikeRearWheelDist               = rhs.m_fBikeRearWheelDist;
    m_nBikeFrontWheelCounter           = rhs.m_nBikeFrontWheelCounter;
    m_fBikeFrontWheelDist              = rhs.m_fBikeFrontWheelDist;
    m_nTempBufferCounter               = rhs.m_nTempBufferCounter;
    m_nBestCarTwoWheelsTimeMs          = rhs.m_nBestCarTwoWheelsTimeMs;
    m_fBestCarTwoWheelsDistM           = rhs.m_fBestCarTwoWheelsDistM;
    m_nBestBikeWheelieTimeMs           = rhs.m_nBestBikeWheelieTimeMs;
    m_fBestBikeWheelieDistM            = rhs.m_fBestBikeWheelieDistM;
    m_nBestBikeStoppieTimeMs           = rhs.m_nBestBikeStoppieTimeMs;
    m_fBestBikeStoppieDistM            = rhs.m_fBestBikeStoppieDistM;
    m_nCarDensityForCurrentZone        = rhs.m_nCarDensityForCurrentZone;
    m_fRoadDensityAroundPlayer         = rhs.m_fRoadDensityAroundPlayer;
    m_nTimeOfLastCarExplosionCaused    = rhs.m_nTimeOfLastCarExplosionCaused;
    m_nExplosionMultiplier             = rhs.m_nExplosionMultiplier;
    m_nHavocCaused                     = rhs.m_nHavocCaused;
    m_nNumHoursDidntEat                = rhs.m_nNumHoursDidntEat;
    m_fCurrentChaseValue               = rhs.m_fCurrentChaseValue;
    m_bDoesNotGetTired                 = rhs.m_bDoesNotGetTired;
    m_bFastReload                      = rhs.m_bFastReload;
    m_bFireProof                       = rhs.m_bFireProof;
    m_nMaxHealth                       = rhs.m_nMaxHealth;
    m_nMaxArmour                       = rhs.m_nMaxArmour;
    m_bGetOutOfJailFree                = rhs.m_bGetOutOfJailFree;
    m_bFreeHealthCare                  = rhs.m_bFreeHealthCare;
    m_bCanDoDriveBy                    = rhs.m_bCanDoDriveBy;
    m_nBustedAudioStatus               = rhs.m_nBustedAudioStatus;
    m_nLastBustMessageNumber           = rhs.m_nLastBustMessageNumber;
    m_nCrosshairActivated              = rhs.m_nCrosshairActivated;
    m_vecCrosshairTarget               = rhs.m_vecCrosshairTarget;
    m_pSkinTexture                     = rhs.m_pSkinTexture;
    m_bParachuteReferenced             = rhs.m_bParachuteReferenced;
    m_nRequireParachuteTimer           = rhs.m_nRequireParachuteTimer;
    strcpy_s(m_szSkinName, rhs.m_szSkinName);
    return *this;
}
