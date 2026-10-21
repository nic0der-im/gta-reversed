#include "StdInc.h"

#include "CarCtrl.h"
#include "Shadows.h"

// 0xC1C83C - Unnamed switch: Place the wheels/chassis using the simulated suspension values
static auto& s_bUseSimulatedNodeOffsets = StaticRef<bool>(0xC1C83C);

void CBmx::InjectHooks() {
    RH_ScopedVirtualClass(CBmx, 0x871528, 67);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6BF820);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6BF9B0);
    RH_ScopedVMTInstall(BurstTyre, 0x6BF9C0);
    RH_ScopedVMTInstall(FindWheelWidth, 0x6C0550);
    RH_ScopedVMTInstall(ProcessControl, 0x6BFA30);
    RH_ScopedVMTInstall(ProcessDrivingAnims, 0x6BFB50);
    RH_ScopedVMTInstall(PreRender, 0x6C0810);
    RH_ScopedVMTInstall(ProcessAI, 0x6C1470);
    RH_ScopedInstall(ProcessBunnyHop, 0x6C0590);
    RH_ScopedInstall(LaunchBunnyHopCB, 0x6C0390);
}

// 0x6BF820
CBmx::CBmx(int32 modelIndex, eVehicleCreatedBy createdBy) :
    CBike(modelIndex, createdBy) 
{
    auto mi                     = CModelInfo::GetModelInfo(modelIndex);
    m_nVehicleSubType           = VEHICLE_TYPE_BMX;
    m_RideAnimData.AnimGroup = CAnimManager::GetAnimBlocks()[mi->GetAnimFileIndex()].GroupId;
    if (m_RideAnimData.AnimGroup < ANIM_GROUP_BMX || m_RideAnimData.AnimGroup > ANIM_GROUP_CHOPPA) {
        m_RideAnimData.AnimGroup = ANIM_GROUP_BMX;
    }

    m_fControlJump     = 0.0f;
    m_fControlPedaling = 0.0f;
    m_fSprintLeanAngle = 0.0f;
    m_fCrankAngle      = 0.0f;
    m_fPedalAngleL     = 0.0f;
    m_fPedalAngleR     = 0.0f;
    m_nFixLeftHand     = false;
    m_nFixRightHand    = false;
    m_bIsFreewheeling  = false;

    const auto Calc = [&](eBmxNodes node) -> float {
        RwMatrix matrix;
        RwFrame* wheelFront = m_aBikeNodes[node];
        matrix              = *RwFrameGetMatrix(wheelFront);

        auto parent = RwFrameGetParent(wheelFront);
        if (parent) {
            do {
                RwMatrixTransform(&matrix, RwFrameGetMatrix(parent), rwCOMBINEPOSTCONCAT);
                parent = RwFrameGetParent(parent);
            } while (parent != wheelFront && parent);
        }
        return matrix.pos.y;
    };
    auto wheelFrontPosY = Calc(BMX_WHEEL_FRONT);
    auto wheelRearPosY  = Calc(BMX_WHEEL_REAR);

    m_fMidWheelDistY = wheelFrontPosY - wheelRearPosY;
    m_fMidWheelFracY = wheelFrontPosY / m_fMidWheelDistY;
}

// 0x6BF9D0
CBmx::~CBmx() {
    m_vehicleAudio.Terminate();
}

// 0x6BF9B0
bool CBmx::SetUpWheelColModel(CColModel* wheelCol) {
    return false;
}

// 0x6BF9C0
bool CBmx::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6BFA30
void CBmx::ProcessControl() {
    const float BMX_SPRINT_LEANSTART = FRAC_PI_2;
    const float BMX_PEDAL_LEANSTART  = 0.0f;
    const float BMX_SPRINT_LEANMULT  = 0.3f;
    const float MTB_SPRINT_LEANMULT  = 0.087f;
    const float BMX_PEDAL_LEANMULT   = 0.07f;
    const float MTB_PEDAL_LEANMULT   = 0.02f;

    CBike::ProcessControl();

    if (GetWasPostponed() || GetStatus() != STATUS_PLAYER || !m_pDriver) {
        return;
    }

    auto animBikeSprint = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_SPRINT);
    bool isMountainBike = GetModelId() == MODEL_MTBIKE;

    if (animBikeSprint && animBikeSprint->GetBlendAmount() > 0.01f) {
        float mult         = isMountainBike ? MTB_SPRINT_LEANMULT : BMX_SPRINT_LEANMULT;
        m_fSprintLeanAngle = std::sin(animBikeSprint->GetCurrentTime() / animBikeSprint->GetHier()->GetTotalTime() * TWO_PI + BMX_SPRINT_LEANSTART) * animBikeSprint->GetBlendAmount() * mult;
    } else {
        auto animBikePedal = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_PEDAL);
        if (animBikePedal && animBikePedal->GetBlendAmount() > 0.01f) {
            float mult = isMountainBike ? MTB_PEDAL_LEANMULT : BMX_PEDAL_LEANMULT;
            GetRideAnimData()->LeanAngle += std::sin(animBikePedal->GetCurrentTime() / animBikePedal->GetHier()->GetTotalTime() * TWO_PI + BMX_PEDAL_LEANSTART) * animBikePedal->GetBlendAmount() * mult;
        }
        m_fSprintLeanAngle *= 0.95f;
    }
}

// 0x6BFB50
void CBmx::ProcessDrivingAnims(CPed* driver, bool blend) {
    if (m_bOffscreen && (!driver || !driver->IsPlayer())) {
        return;
    }

    m_nFixLeftHand  = 1;
    m_nFixRightHand = 1;

    // While doing a bunny hop the crank is held at half a turn, blended in
    if (const auto bunnyHopAnim = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_BUNNYHOP)) {
        m_fCrankAngle  = bunnyHopAnim->GetBlendAmount() * HALF_PI + (1.0f - bunnyHopAnim->GetBlendAmount()) * m_fCrankAngle;
        m_fPedalAngleL = -m_fCrankAngle;
        m_fPedalAngleR = -m_fCrankAngle;
        return;
    }

    if (driver->GetPlayerData()) {
        driver->SetMoveState(PEDMOVE_NONE);
    }

    const auto forwardSpeed = DotProduct(m_matrix->GetForward(), m_vecMoveSpeed);

    auto* animPedal   = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_PEDAL);
    auto* animSprint  = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_SPRINT);
    auto* animLeft    = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_LEFT);
    auto* animRight   = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_RIGHT);
    auto* animFwd     = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_FWD);
    auto* animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYLHS);
    if (!animDriveBy) {
        animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYRHS);
        if (!animDriveBy) {
            animDriveBy = RpAnimBlendClumpGetAssociation(driver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYFT);
        }
    }

    // How much the bike has to lean before the legs stop pedalling
    const auto leanLimit = m_fControlPedaling <= 5.0f ? 0.4f : 0.7f;

    if (leanLimit <= std::abs(m_RideAnimData.LeanAngle)
        || leanLimit <= m_RideAnimData.LeanFwd
        || forwardSpeed <= 0.01f
        || animDriveBy
    ) {
        // Coasting (Or doing a drive-by) - Blend out the pedalling
        if ((animPedal && animPedal->GetBlendDelta() >= 0.0f && animPedal->GetBlendAmount() > 0.0f)
            || (animSprint && animSprint->GetBlendDelta() >= 0.0f && animSprint->GetBlendAmount() > 0.0f)
        ) {
            if (animPedal) {
                animPedal->SetFlag(ANIMATION_IS_PLAYING, false);
                animPedal->SetBlendDelta(-8.0f);
            }
            if (animSprint) {
                animSprint->SetFlag(ANIMATION_IS_PLAYING, false);
                animSprint->SetBlendDelta(-8.0f);
            }
            m_RideAnimData.AnimLeanLeft *= 0.95f;
            m_RideAnimData.AnimLeanFwd  *= 0.95f;
        } else {
            CBike::ProcessRiderAnims(driver, this, &m_RideAnimData, m_BikeHandling, 0);
        }

        if (!animPedal) {
            animPedal = animSprint;
        }
        m_fCrankAngle = animPedal
            ? -(animPedal->GetCurrentTime() / animPedal->GetHier()->GetTotalTime()) * TWO_PI
            : 0.0f;

        if (animLeft && animLeft->GetBlendAmount() > 0.1f) {
            m_bIsFreewheeling = true;
            m_fCrankAngle = PI * animLeft->GetBlendAmount() + (1.0f - animLeft->GetBlendAmount()) * m_fCrankAngle;
        } else if (animRight && animRight->GetBlendAmount() > 0.1f) {
            m_bIsFreewheeling = true;
            m_fCrankAngle = 0.0f * animRight->GetBlendAmount() + (1.0f - animRight->GetBlendAmount()) * m_fCrankAngle;
        } else if (animFwd && animFwd->GetBlendAmount() > 0.1f) {
            m_fCrankAngle = HALF_PI * animFwd->GetBlendAmount() + (1.0f - animFwd->GetBlendAmount()) * m_fCrankAngle;
        } else {
            m_fCrankAngle = std::pow(0.97f, CTimer::GetTimeStep()) * m_fCrankAngle;
        }

        if (animDriveBy) {
            m_nFixRightHand   = 0;
            m_bIsFreewheeling = true;
        }
    } else {
        // Pedalling - The pedal rate is limited by the speed and the gear
        float pedalSpeed;
        float maxPedalSpeed;
        if (m_nModelIndex == MODEL_MTBIKE) {
            maxPedalSpeed = 2.0f;
            pedalSpeed    = m_nCurrentGear < 1
                          ? 0.0f
                          : forwardSpeed * 5.0f / (m_nCurrentGear * m_pHandlingData->GetTransmission().m_MaxFlatVelocity - 0.25f);
        } else {
            maxPedalSpeed = 2.5f;
            pedalSpeed    = forwardSpeed * 3.0f;
        }

        bool blended = false;
        if (m_fControlPedaling <= 5.0f || pedalSpeed >= maxPedalSpeed) {
            if (!animPedal || (animPedal->GetBlendAmount() < 1.0f && animPedal->GetBlendDelta() <= 0.0f)) {
                animPedal = CAnimManager::BlendAnimation(driver->GetRpClump(), m_RideAnimData.AnimGroup, ANIM_ID_BIKE_PEDAL, 4.0f);
                blended   = true;
            }

            if (m_GasPedal != 0.0f || m_fControlPedaling > 0.0f || GetStatus() == STATUS_SIMPLE) {
                animPedal->SetFlag(ANIMATION_IS_PLAYING, true);
                animPedal->SetSpeed(pedalSpeed);
            } else {
                animPedal->SetFlag(ANIMATION_IS_PLAYING, false);
                if (!vehicleFlags.bIsHandbrakeOn
                    && std::ranges::any_of(m_aRatioHistory, [](float ratio) { return ratio < 1.0f; })
                ) {
                    m_bIsFreewheeling = true;
                }
            }
        } else {
            if (!animSprint || (animSprint->GetBlendAmount() < 1.0f && animSprint->GetBlendDelta() <= 0.0f)) {
                animSprint = CAnimManager::BlendAnimation(driver->GetRpClump(), m_RideAnimData.AnimGroup, ANIM_ID_BIKE_SPRINT, 4.0f);
                blended    = true;
            }
            animSprint->SetFlag(ANIMATION_IS_PLAYING, true);

            if (!animPedal) {
                animSprint->SetSpeed(pedalSpeed);
                animPedal = animSprint;
            } else {
                animPedal->SetFlag(ANIMATION_IS_PLAYING, true);
                animPedal->SetSpeed(pedalSpeed);
            }
        }

        if (!animPedal) {
            m_fCrankAngle = std::pow(0.97f, CTimer::GetTimeStep()) * m_fCrankAngle;
        } else if (!blended) {
            m_fCrankAngle = -(animPedal->GetCurrentTime() / animPedal->GetHier()->GetTotalTime()) * TWO_PI;
        } else {
            // Just started blending the pedalling anim - Set its time so that the crank is in phase
            // with how the rider is currently steering
            float steerAngle = -1000.0f; // Sentinel: No steering anim is playing
            if (animLeft && animLeft->GetBlendAmount() > 0.5f) {
                steerAngle = PI;
            } else if (animRight && animRight->GetBlendAmount() > 0.5f) {
                steerAngle = 0.0f;
            } else if (animFwd && animFwd->GetBlendAmount() > 0.5f) {
                steerAngle = HALF_PI;
            }

            const auto animTime = animPedal->GetHier()->GetTotalTime();
            if (steerAngle <= -1000.0f) {
                m_fCrankAngle = -(animPedal->GetCurrentTime() / animTime) * TWO_PI;
            } else {
                auto timeFrac = -steerAngle * FRAC_1_TAU;
                if (timeFrac < 0.0f) {
                    timeFrac += 1.0f;
                }
                animPedal->SetCurrentTime(animTime * timeFrac);
                m_fCrankAngle = timeFrac;
            }
        }

        if (std::abs(m_RideAnimData.AnimLeanLeft) > 0.05f || std::abs(m_RideAnimData.AnimLeanFwd) > 0.05f) {
            m_RideAnimData.AnimLeanLeft *= 0.95f;
            m_RideAnimData.AnimLeanFwd  *= 0.95f;
        }
    }

    // Let the camera know about wheelies/stoppies
    if (driver->IsPlayer()) {
        const auto upZ = m_matrix->GetUp().z;

        const auto frontWheelsOnGround = m_WheelCounts[0] > 0.0f || m_WheelCounts[1] > 0.0f;
        const auto rearWheelsOnGround  = m_WheelCounts[2] > 0.0f || m_WheelCounts[3] > 0.0f;

        bikeFlags.bWheelieForCamera = false;
        if (!frontWheelsOnGround && upZ > 0.0f && rearWheelsOnGround) {
            if (m_BikeHandling->m_fWheelieAng - upZ < m_BikeHandling->m_fWheelieAng * 0.5f) {
                bikeFlags.bWheelieForCamera = true;
            }
        } else if (!rearWheelsOnGround && upZ < 0.0f && frontWheelsOnGround) {
            if (m_BikeHandling->m_fStoppieAng - upZ > m_BikeHandling->m_fStoppieAng * 0.6f) {
                bikeFlags.bWheelieForCamera = true;
            }
        }
    }

    m_fPedalAngleL = -m_fCrankAngle;
    m_fPedalAngleR = -m_fCrankAngle;
}

// data is a ptr to CBmx
// 0x6C0390
void CBmx::LaunchBunnyHopCB(CAnimBlendAssociation* assoc, void* data) {
    auto bmx = static_cast<CBmx*>(data);
    if ((bmx->m_WheelCounts[0] > 0.0f || bmx->m_WheelCounts[1] > 0.0f) &&
        (bmx->m_WheelCounts[2] > 0.0f || bmx->m_WheelCounts[3] > 0.0f)
    ) {
        auto power = std::min(bmx->m_fControlJump / 25.0f, 1.0f) + 1.0f;
        if (bmx->GetStatus() == STATUS_PLAYER) {
            power *= CStats::GetFatAndMuscleModifier(STAT_MOD_6);
        }
        if (CCheat::IsActive(CHEAT_HUGE_BUNNY_HOP)) {
            power *= 5.0f;
        }
        bmx->ApplyMoveForce(0.06f * bmx->m_fMass * power * bmx->m_matrix->GetUp());
        bmx->ApplyTurnForce(0.01f * bmx->m_fTurnMass * power * bmx->m_matrix->GetUp(), bmx->m_matrix->GetForward());
    }
}

// 0x6C0500 | inlined | see 0x6C11F3
void CBmx::GetFrameOffset(float& fZOffset, float& fAngleOffset) {
    const auto d1 = m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0];
    const auto d2 = m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1];

    fZOffset     = (1.0f - m_fMidWheelFracY) * d1 + d2 * m_fMidWheelFracY;
    fAngleOffset = std::atan2(d1 - d2, m_fMidWheelDistY);
}

// 0x6C0550
float CBmx::FindWheelWidth(bool bRear) {
    return 0.07f;
}

// 0x6C0560
void CBmx::BlowUpCar(CEntity* damager, bool bHideExplosion) {
    // NOP
}

// 0x6C0590
void CBmx::ProcessBunnyHop() {
    auto* anim = m_pDriver
        ? RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_BUNNYHOP)
        : nullptr;

    if (GetStatus() != STATUS_PLAYER || !m_pDriver || !m_pDriver->IsPlayer()) {
        if (anim) {
            anim->SetFlag(ANIMATION_IS_PLAYING, true);
            anim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE, true);
            anim->SetBlendDelta(-8.0f);
        }
        return;
    }

    auto pad = m_pDriver->AsPlayer()->GetPadFromPlayer();

    if (pad->IsLeftShoulder1Pressed() && !pad->DisablePlayerControls && m_fControlJump == 0.0f) {
        m_fControlJump += CTimer::GetTimeStep();
        anim = CAnimManager::BlendAnimation(m_pDriver->GetRpClump(), m_RideAnimData.AnimGroup, ANIM_ID_BIKE_BUNNYHOP, 8.0f);
        if (anim) {
            anim->SetCurrentTime(0.0f);
            anim->SetFlag(ANIMATION_IS_PLAYING, false);
        }
    }

    if (m_fControlJump > 0.0f) {
        if (!pad->DisablePlayerControls) {
            if (!anim) {
                m_fControlJump = 0.0f;
                return;
            }

            if (pad->IsLeftShoulder1()) {
                if (!anim->IsPlaying()) {
                    m_fControlJump = std::min(m_fControlJump + CTimer::GetTimeStep(), 25.0f);
                    anim->SetCurrentTime(m_fControlJump / 25.0f * 0.2f);
                }
            } else if (!anim->IsPlaying()) {
                if (anim->GetCurrentTime() < 0.2f) {
                    anim->SetCurrentTime((0.2f - anim->GetCurrentTime()) / 0.2f * (anim->GetHier()->GetTotalTime() - 0.2f) + 0.2f);
                }
                anim->SetFlag(ANIMATION_IS_PLAYING, true);
                anim->SetSpeed(1.5f);
                anim->SetFinishCallback(CBmx::LaunchBunnyHopCB, this);
            }
        } else {
            m_fControlJump = 0.0f;
        }
    }

    if (anim) {
        if (anim->GetBlendAmount() > 0.5f) {
            m_GasPedal                                   = 0.0f;
            FindPlayerPed()->GetPlayerData()->m_fMoveSpeed = 0.0f;
            if (!vehicleFlags.bIsHandbrakeOn && (m_aWheelRatios[0] < 1.0f || m_aWheelRatios[1] < 1.0f || m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f)) {
                m_bIsFreewheeling = true;
            }
        }
    }
}

// 0x6C0810
void CBmx::PreRender() {
    CVehicle::PreRender();

    const auto colModel = GetColModel();
    const auto colData  = colModel->m_pColData;

    if (vehicleFlags.bVehicleColProcessed) {
        DoBurstAndSoftGroundRatios();

        // Front suspension
        {
            const auto frac  = 1.0f - m_fSuspensionLength[0] / m_fLineLength[0];
            const auto ratio = (std::min(m_aWheelRatios[0], m_aWheelRatios[1]) - frac) / (1.0f - frac);
            auto       radius = colData->m_pLines[0].m_vecStart.z;
            if (ratio > 0.0f) {
                radius -= ratio * m_fSuspensionLength[0];
            }
            m_aWheelSuspensionHeights[0] += (radius - m_aWheelSuspensionHeights[0]) * 0.75f;
        }

        // Rear suspension
        {
            const auto frac  = 1.0f - m_fSuspensionLength[2] / m_fLineLength[2];
            const auto ratio = (std::min(m_aWheelRatios[2], m_aWheelRatios[3]) - frac) / (1.0f - frac);
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

            auto flags = (i == 0 || m_WheelStates[1] == WHEEL_STATE_FIXED) ? 4u : 0u;
            if (m_bWheelBloody[i]) {
                flags += 1;
            }
            if (m_bMoreSkidMarks[i]) {
                flags += 2;
            }

            // Lean the wheels sideways so they still line up with the (leaning) bike
            auto wheelPos = m_aWheelColPoints[colPointIdx].m_vecPoint
                          + m_matrix->GetRight() * (std::sin(m_RideAnimData.LeanAngle) * GetColModel()->m_boundBox.m_vecMin.z * 0.8f);

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
    CShadows::StoreShadowForVehicle(this, VEH_SHD_BIKE);

    const auto mi = GetVehicleModelInfo();

    CMatrix mat;

    // Front forks - Rotated by the steering
    if (m_aBikeNodes[BMX_FORKS_FRONT]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_FORKS_FRONT]), false);
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

        // Mudguard - Follows the forks
        if (m_aBikeNodes[BMX_MUDGUARD]) {
            CMatrix matMudguard;
            matMudguard.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_MUDGUARD]), false);
            const auto posMudguard = matMudguard.GetPosition();
            if (GetStatus() == STATUS_ABANDONED || GetStatus() == STATUS_WRECKED) {
                matMudguard.SetUnity();
                matMudguard *= quatMtx;
                matMudguard.GetPosition() += posMudguard;
            } else {
                matMudguard.SetTranslate(posMudguard);
            }
            matMudguard.UpdateRW();
        }
    }

    // Rear forks - Rotated by the suspension
    if (m_aBikeNodes[BMX_FORKS_REAR]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_FORKS_REAR]), false);
        const auto pos = mat.GetPosition();
        mat.SetRotate(-std::asin((m_aWheelSuspensionHeights[1] - m_aWheelOrigHeights[1]) / m_fSwingArmLength), 0.0f, 0.0f);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // Front wheel
    if (m_aBikeNodes[BMX_WHEEL_FRONT]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_WHEEL_FRONT]), false);
        auto pos = mat.GetPosition();
        if (s_bUseSimulatedNodeOffsets) {
            pos.z = m_aWheelSuspensionHeights[0] - m_fForkZOffset;
            pos.y = (colData->m_pLines[0].m_vecStart.y + colData->m_pLines[1].m_vecStart.y) * 0.5f - m_fForkYOffset
                  - (m_aWheelSuspensionHeights[0] - m_aWheelOrigHeights[0]) * m_fSteerAngleTan;
        }
        if (m_nWheelStatus[0] == 1) {
            const auto pitch = m_aWheelPitchAngles[0];
            mat.SetRotate(pitch, 0.0f, std::sin(pitch) * 0.02f);
        } else {
            mat.SetRotateX(m_aWheelPitchAngles[0]);
        }
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // Rear wheel
    if (m_aBikeNodes[BMX_WHEEL_REAR]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_WHEEL_REAR]), false);
        auto pos = mat.GetPosition();
        if (s_bUseSimulatedNodeOffsets && !m_aBikeNodes[BMX_FORKS_REAR]) {
            pos.z = m_aWheelSuspensionHeights[1];
        }
        if (m_nWheelStatus[1] == 1) {
            const auto pitch = m_aWheelPitchAngles[1];
            mat.SetRotate(pitch, 0.0f, std::sin(pitch) * 0.04f);
        } else {
            mat.SetRotateX(m_aWheelPitchAngles[1]);
        }
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // Chassis
    if (m_aBikeNodes[BMX_CHASSIS]) {
        float fZOffset = 0.0f;
        float fAngleOffset = 0.0f;
        if (!s_bUseSimulatedNodeOffsets) {
            GetFrameOffset(fZOffset, fAngleOffset);
        }

        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_CHASSIS]), false);
        auto pos = mat.GetPosition();
        pos.z = (1.0f - std::cos(m_RideAnimData.LeanAngle)) * colModel->m_boundBox.m_vecMin.z * 0.9f + fZOffset;
        mat.SetRotateX(std::abs(m_RideAnimData.LeanAngle) * -0.05f + fAngleOffset);
        mat.RotateY(m_fSprintLeanAngle + m_RideAnimData.LeanAngle);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // Chainset
    if (m_aBikeNodes[BMX_CHAINSET]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_CHAINSET]), false);
        const auto pos = mat.GetPosition();
        mat.SetRotate(m_fCrankAngle, 0.0f, 0.0f);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }

    // Pedals (Both angles are always the same, see ProcessDrivingAnims)
    if (m_aBikeNodes[BMX_PEDAL_R]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_PEDAL_R]), false);
        const auto pos = mat.GetPosition();
        mat.SetRotate(m_fPedalAngleL, 0.0f, 0.0f);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }
    if (m_aBikeNodes[BMX_PEDAL_L]) {
        mat.Attach(RwFrameGetMatrix(m_aBikeNodes[BMX_PEDAL_L]), false);
        const auto pos = mat.GetPosition();
        mat.SetRotate(m_fPedalAngleR, 0.0f, 0.0f);
        mat.GetPosition() += pos;
        mat.UpdateRW();
    }
}

// 0x6C1470
bool CBmx::ProcessAI(uint32& extraHandlingFlags) {
    m_autoPilot.carCtrlFlags.bHonkAtCar = false;
    m_autoPilot.carCtrlFlags.bHonkAtPed = false;

    m_bIsFreewheeling = false;

    switch (GetStatus()) {
    case STATUS_PLAYER: {
        extraHandlingFlags += 2;
        bikeFlags.bGettingPickedUp = false;

        if (!m_pDriver || !m_pDriver->IsPlayer()) {
            break;
        }

        ProcessControlInputs((uint8)m_pDriver->m_nPedType);

        auto* pad = m_pDriver->AsPlayer()->GetPadFromPlayer();

        // The lean is implemented by moving the centre of mass
        if (m_RideAnimData.LeanFwd < 0.0f) {
            m_vecCentreOfMass.y = m_BikeHandling->m_fLeanBakCOM * m_RideAnimData.LeanFwd + m_pHandlingData->m_vecCentreOfMass.y;

            if ((m_GasPedal == 0.0f && !vehicleFlags.bIsHandbrakeOn) || m_nNoOfContactWheels == 0) {
                const auto mag = std::min(m_vecMoveSpeed.Magnitude(), 0.1f);
                const auto leanF = m_nModelIndex == MODEL_SANCHEZ
                                 ? m_GasPedal * 0.7f + 0.3f
                                 : (m_GasPedal + 1.0f) * 0.5f;
                const auto force = leanF * m_BikeHandling->m_fLeanBakForce * m_fTurnMass * m_RideAnimData.LeanFwd * mag;
                ApplyTurnForce(m_matrix->GetUp() * -(CTimer::GetTimeStep() * force), m_vecCentreOfMass + m_matrix->GetForward());
            }
        } else {
            m_vecCentreOfMass.y = m_BikeHandling->m_fLeanFwdCOM * m_RideAnimData.LeanFwd + m_pHandlingData->m_vecCentreOfMass.y;

            if (m_BrakePedal < 0.0f || m_nNoOfContactWheels == 0) {
                const auto mag   = std::min(m_vecMoveSpeed.Magnitude(), 0.1f);
                const auto force = m_BikeHandling->m_fLeanFwdForce * m_fTurnMass * mag * m_RideAnimData.LeanFwd;
                ApplyTurnForce(m_matrix->GetUp() * -(CTimer::GetTimeStep() * force), m_vecCentreOfMass + m_matrix->GetForward());
            }
        }

        PruneReferences();
        if (GetStatus() == STATUS_PLAYER) {
            DoDriveByShootings();
        }
        DoSoftGroundResistance(extraHandlingFlags);

        // Steering while airborne
        if (std::ranges::all_of(m_aWheelRatios, [](float ratio) { return ratio == 1.0f; })) {
            const auto turnAlongUp = DotProduct(m_vecTurnSpeed, m_matrix->GetUp());
            const auto steer       = (float)pad->GetSteeringLeftRight();

            if ((turnAlongUp < 0.04f && steer < 0.0f) || (turnAlongUp > -0.04f && steer > 0.0f)) {
                ApplyTurnForce(
                    m_matrix->GetRight() * (steer * 0.0078125f * m_fTurnMass * CTimer::GetTimeStep() * 0.002f),
                    m_matrix->GetForward()
                );
            }
        }

        ProcessBunnyHop();

        // Sprinting
        auto* animSprint = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_SPRINT);
        if (m_pDriver->AsPlayer()->ControlButtonSprint(SPRINT_BMX) > 1.2f
            || (m_pDriver->AsPlayer()->GetButtonSprintResults(SPRINT_BMX) > 1.0f
                && animSprint
                && animSprint->GetBlendAmount() > 0.5f
            )
        ) {
            bikeFlags.bPlayerBoost = true;
            m_fControlPedaling     = m_pDriver->GetPlayerData()->m_fMoveSpeed;
        } else {
            const auto accelerate = (float)pad->GetAccelerate();
            m_pDriver->AsPlayer()->HandleSprintEnergy(false, std::max(1.0f - accelerate * (1.0f / 255.0f) * 0.5f, 0.5f));

            if (m_pDriver->AsPlayer()->GetButtonSprintResults(SPRINT_BMX) > 0.0f) {
                m_fControlPedaling = 5.4f;
                if (m_GasPedal == 0.0f && m_BrakePedal == 0.0f) {
                    m_GasPedal = 1.0f;
                }
            } else if (m_pDriver->GetPlayerData()->m_fTimeCanRun >= 0.0f) {
                m_fControlPedaling = 0.0f;
            } else if (pad->GetAccelerateJustDown()) {
                m_fControlPedaling = 5.4f;
            } else {
                m_fControlPedaling = std::max(m_fControlPedaling - 0.4f, 0.0f);
            }
        }

        CStats::UpdateStatsWhenCycling(bikeFlags.bPlayerBoost, this);

        if (pad->CarGunJustDown()) {
            ActivateBomb();
        }
        break;
    }
    case STATUS_PLAYER_PLAYBACK_FROM_BUFFER:
        extraHandlingFlags += 2;
        break;
    case STATUS_SIMPLE: {
        CCarAI::UpdateCarAI(this);
        CPhysical::ProcessControl();
        CCarCtrl::UpdateCarOnRails(this);

        const auto numDriveWheelsOnGround  = m_NumDriveWheelsOnGround;
        m_nNoOfContactWheels               = 2;
        m_NumDriveWheelsOnGroundLastFrame  = numDriveWheelsOnGround;
        m_NumDriveWheelsOnGround           = 2;

        const auto mi = GetVehicleModelInfo();
        m_pHandlingData->GetTransmission().CalculateGearForSimpleCar(m_autoPilot.m_speed * 0.02f, m_nCurrentGear);

        m_aWheelPitchAngles[0] += ProcessWheelRotation(WHEEL_STATE_NORMAL, m_matrix->GetForward(), m_vecMoveSpeed, mi->m_fWheelSizeFront * 0.5f);
        m_aWheelPitchAngles[1] += ProcessWheelRotation(WHEEL_STATE_NORMAL, m_matrix->GetForward(), m_vecMoveSpeed, mi->m_fWheelSizeRear * 0.5f);

        CBike::PlayHornIfNecessary();
        CBike::ReduceHornCounter();

        vehicleFlags.bVehicleColProcessed = false;
        vehicleFlags.bAudioChangingGear   = false;
        bikeFlags.bWheelieForCamera       = false;
        m_fControlPedaling                = 0.0f;
        break;
    }
    case STATUS_PHYSICS: {
        CCarAI::UpdateCarAI(this);
        CCarCtrl::SteerAICarWithPhysics(this);
        CBike::PlayHornIfNecessary();
        extraHandlingFlags += 2;

        bikeFlags.bWheelieForCamera = false;
        if (vehicleFlags.bIsBeingCarJacked) {
            m_GasPedal                  = 0.0f;
            m_BrakePedal                = 1.0f;
            vehicleFlags.bIsHandbrakeOn = true;
        } else {
            bikeFlags.bGettingPickedUp = false;
        }

        if (m_fControlPedaling > 0.0f) {
            float deceleration;
            if (m_fControlPedaling <= 5.0f) {
                deceleration = CTimer::GetTimeStep() * 0.01f;
                if (vehicleFlags.bUseCarCheats) {
                    deceleration *= 2.0f;
                }
            } else {
                deceleration = CTimer::GetTimeStep() * 0.02f;
            }

            m_fControlPedaling -= deceleration;
            if (m_fControlPedaling < 0.0f) {
                m_fControlPedaling = 0.0f;
            }
        }
        break;
    }
    case STATUS_ABANDONED: {
        m_BrakePedal = 0.0f;

        if (m_vecMoveSpeed.SquaredMagnitude() < 0.01f || bikeFlags.bOnSideStand) {
            vehicleFlags.bIsHandbrakeOn = true;
        } else {
            vehicleFlags.bIsHandbrakeOn = false;
        }

        m_GasPedal            = 0.0f;
        m_nVehicleWeaponInUse = CAR_WEAPON_NOT_USED;

        if ((m_pDriver || m_apPassengers[0] || vehicleFlags.bIsBeingCarJacked) && !bikeFlags.bOnSideStand) {
            extraHandlingFlags += 2;
        }

        bikeFlags.bWheelieForCamera = false;
        m_RideAnimData.AnimLeanLeft = 0.0f;
        m_RideAnimData.AnimLeanFwd  = 0.0f;
        m_fControlPedaling          = 0.0f;

        if (vehicleFlags.bIsBeingCarJacked) {
            m_GasPedal                  = 0.0f;
            m_BrakePedal                = 1.0f;
            vehicleFlags.bIsHandbrakeOn = true;
        }
        break;
    }
    case STATUS_WRECKED: {
        vehicleFlags.bIsHandbrakeOn = true;
        m_BrakePedal                = 0.05f;
        m_fSteerAngle               = 0.0f;
        m_GasPedal                  = 0.0f;
        m_nVehicleWeaponInUse       = CAR_WEAPON_NOT_USED;
        bikeFlags.bWheelieForCamera = false;
        m_fControlPedaling          = 0.0f;
        m_RideAnimData.AnimLeanLeft = 0.0f;
        m_RideAnimData.AnimLeanFwd  = 0.0f;
        break;
    }
    case STATUS_IS_TOWED: {
        if (m_vecMoveSpeed.SquaredMagnitude() < 0.01f) {
            m_BrakePedal                = 1.0f;
            vehicleFlags.bIsHandbrakeOn = true;
        } else {
            m_BrakePedal                = 0.0f;
            vehicleFlags.bIsHandbrakeOn = false;
        }

        m_fSteerAngle               = 0.0f;
        m_GasPedal                  = 0.0f;
        m_nVehicleWeaponInUse       = CAR_WEAPON_NOT_USED;
        bikeFlags.bWheelieForCamera = false;
        m_fControlPedaling          = 0.0f;

        extraHandlingFlags += 2;
        break;
    }
    }

    // Rider animation phase
    if (m_pDriver) {
        auto* animFwdBack = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_FWD);
        if (!animFwdBack || animFwdBack->GetBlendAmount() < 0.5f) {
            animFwdBack = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_BACK);
        }

        auto* animDriveBy = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYLHS);
        if (!animDriveBy || animDriveBy->GetBlendAmount() < 0.5f) {
            animDriveBy = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYRHS);
        }
        if (!animDriveBy || animDriveBy->GetBlendAmount() < 0.5f) {
            animDriveBy = RpAnimBlendClumpGetAssociation(m_pDriver->GetRpClump(), ANIM_ID_BIKE_DRIVEBYFT);
        }

        if ((animFwdBack && animFwdBack->GetBlendAmount() > 0.5f) || (animDriveBy && animDriveBy->GetBlendAmount() > 0.5f)) {
            m_GasPedal = 0.0f;

            if (vehicleFlags.bIsHandbrakeOn) {
                return true;
            }

            if (std::ranges::all_of(m_aWheelRatios, [](float ratio) { return ratio >= 1.0f; })) {
                return true;
            }

            m_bIsFreewheeling = true;
            return true;
        }
    }

    // Push the bike forward when pedalling hard
    if (m_fControlPedaling > 5.0f && (m_aWheelRatios[2] < 1.0f || m_aWheelRatios[3] < 1.0f)) {
        auto push = 2.4f - DotProduct(m_vecMoveSpeed, m_matrix->GetForward()) / m_pHandlingData->GetTransmission().m_MaxFlatVelocity * 1.5f;
        push = std::clamp(push, 0.0f, 2.0f);

        if (GetStatus() == STATUS_PLAYER) {
            push *= CStats::GetFatAndMuscleModifier(STAT_MOD_5);
        } else if (vehicleFlags.bUseCarCheats) {
            push *= 1.25f;
        }

        ApplyMoveForce(m_matrix->GetForward() * (CTimer::GetTimeStep() * m_fMass * 0.3f * push * 0.008f));
    }

    return true;
}
