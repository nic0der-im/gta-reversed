#include "StdInc.h"

#include "TaskUtilityLineUpPedWithCar.h"
#include "PedPlacement.h"
#include "VehicleModelInfo.h"
#include "RenderWare/rw/rtslerp.h"

void CTaskUtilityLineUpPedWithCar::InjectHooks() {
    RH_ScopedClass(CTaskUtilityLineUpPedWithCar);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x64FBB0);
    RH_ScopedInstall(Destructor, 0x64FC00);

    RH_ScopedInstall(GetLocalPositionToOpenCarDoor, 0x64FC10);
    RH_ScopedInstall(GetPositionToOpenCarDoor, 0x650A80);
    RH_ScopedInstall(ProcessPed, 0x6513A0);
}

// 0x64FBB0
CTaskUtilityLineUpPedWithCar::CTaskUtilityLineUpPedWithCar(const CVector& offset, int32 time, int32 doorOpenPosType, int32 doorIdx) {
    m_Offset = offset;
    m_fDoorOpenPosZ = -999.99f;
    m_fTime = time;
    m_nDoorOpenPosType = doorOpenPosType;
    m_nDoorIdx = doorIdx;
}

namespace {

/*!
 * @brief Build a rotation matrix from `q`, normalizing it in the process.
 *
 * @note The original has this conversion inlined into `ProcessPed` (0x651FD7 and 0x652452). Unlike
 *       `CMatrix::SetRotate(const CQuaternion&)` it normalizes the quaternion - The squared magnitude
 *       is divided into `2.0f` and the resulting scale applied to the imaginary part.
 */
CMatrix RotationMatrixFromQuat(const RtQuat& q) {
    const CVector imag{ q.imag.x, q.imag.y, q.imag.z };
    const auto    imag2 = imag * (2.0f / (imag.SquaredMagnitude() + q.real * q.real));

    const auto x2x = imag2.x * imag.x;
    const auto y2x = imag2.y * imag.x;
    const auto z2x = imag2.z * imag.x;

    const auto y2y = imag2.y * imag.y;
    const auto z2y = imag2.z * imag.y;
    const auto z2z = imag2.z * imag.z;

    const auto x2r = imag2.x * q.real;
    const auto y2r = imag2.y * q.real;
    const auto z2r = imag2.z * q.real;

    CMatrix mat{};
    mat.GetRight().Set(1.0f - (z2z + y2y), z2r + y2x, z2x - y2r);
    mat.GetForward().Set(y2x - z2r, 1.0f - (z2z + x2x), x2r + z2y);
    mat.GetUp().Set(y2r + z2x, z2y - x2r, 1.0f - (y2y + x2x));
    return mat;
}

} // anonymous namespace

// The following 2 functions seem to have copy elision on the returned CVector, that the compiled functions
// took a vector ptr as their first arg. Now, hopefully our code will compile to the same stuff.
// If not, it might crash here, in that case a wrapper function should be used.

// 0x64FC10
CVector CTaskUtilityLineUpPedWithCar::GetLocalPositionToOpenCarDoor(CVehicle* vehicle, float animProgress, CAnimBlendAssociation* assoc) {
    const auto  door       = static_cast<eTargetDoor>(m_nDoorIdx);
    const auto* modelInfo  = vehicle->GetVehicleModelInfo();
    const auto& dummyPos   = modelInfo->m_pVehicleStruct->m_avDummyPos;

    // Vans don't use the seat offset for their rear doors
    if (!vehicle->vehicleFlags.bIsVan || !notsa::contains({TARGET_DOOR_REAR_RIGHT, TARGET_DOOR_REAR_LEFT}, door)) {
        animProgress *= vehicle->m_pHandlingData->m_fSeatOffsetDistance;
    } else {
        animProgress = 0.0f;
    }

    // Determine which door offset is to be used
    // NOTE: Anim ids that aren't mapped to any offset keep the vehicle's pointer as the index (A
    //       quirk of the original), meaning they never match any door below, and the pointer's
    //       value ends up being passed (as an `eVehAnimDoorOffset`) to `ComputeAnimDoorOffsets`.
    int32 doorOffsetId;
    if (!assoc) {
        doorOffsetId = ENTER_FRONT;
    } else {
        switch (assoc->m_AnimId.get()) {
        case ANIM_ID_CAR_ALIGN_LHS:
        case ANIM_ID_CAR_ALIGN_RHS:
        case ANIM_ID_CAR_ALIGNHI_LHS:
        case ANIM_ID_CAR_ALIGNHI_RHS:
        case ANIM_ID_CAR_OPEN_LHS:
        case ANIM_ID_CAR_OPEN_RHS:
        case ANIM_ID_CAR_OPEN_LHS_1:
        case ANIM_ID_CAR_OPEN_RHS_1:
        case ANIM_ID_CAR_GETIN_LHS_0:
        case ANIM_ID_CAR_GETIN_RHS_0:
        case ANIM_ID_CAR_GETIN_LHS_1:
        case ANIM_ID_CAR_GETIN_RHS_1:
        case ANIM_ID_CAR_GETIN_BIKE_FRONT:
        case ANIM_ID_CAR_PULLOUT_LHS:
        case ANIM_ID_CAR_PULLOUT_RHS:
        case ANIM_ID_UNKNOWN_15:
        case ANIM_ID_CAR_CLOSEDOOR_LHS_0:
        case ANIM_ID_CAR_CLOSEDOOR_RHS_0:
        case ANIM_ID_CAR_CLOSEDOOR_LHS_1:
        case ANIM_ID_CAR_CLOSEDOOR_RHS_1:
        case ANIM_ID_CAR_SHUFFLE_RHS_0:
        case ANIM_ID_CAR_SHUFFLE_RHS_1:
        case ANIM_ID_CAR_DOORLOCKED_LHS:
        case ANIM_ID_CAR_DOORLOCKED_RHS:
            doorOffsetId = ENTER_FRONT;
            break;
        case ANIM_ID_CAR_GETOUT_LHS_0:
        case ANIM_ID_CAR_GETOUT_RHS_0:
        case ANIM_ID_CAR_GETOUT_LHS_1:
        case ANIM_ID_CAR_GETOUT_RHS_1:
        case ANIM_ID_CAR_CLOSE_LHS_0:
        case ANIM_ID_CAR_CLOSE_RHS_0:
        case ANIM_ID_CAR_CLOSE_LHS_1:
        case ANIM_ID_CAR_CLOSE_RHS_1:
        case ANIM_ID_CAR_FALLOUT_LHS:
        case ANIM_ID_CAR_FALLOUT_RHS:
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_0:
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_1:
            doorOffsetId = EXIT_FRONT;
            break;
        case ANIM_ID_CAR_JACKEDLHS:
        case ANIM_ID_CAR_JACKEDRHS:
            doorOffsetId = JACK_PED_LEFT;
            break;
        case ANIM_ID_UNKNOWN_26:
        case ANIM_ID_CAR_ROLLOUT_LHS:
        case ANIM_ID_CAR_ROLLOUT_RHS:
        case ANIM_ID_CAR_ROLLDOOR:
        default:
            doorOffsetId = static_cast<int32>(reinterpret_cast<uintptr_t>(vehicle));
            break;
        }
    }

    // Remap the offset to the door that's actually being used
    if (doorOffsetId == ENTER_FRONT || doorOffsetId == EXIT_FRONT) {
        switch (door) {
        case TARGET_DOOR_FRONT_RIGHT:
        case TARGET_DOOR_DRIVER:
            doorOffsetId = ENTER_FRONT;
            break;
        case TARGET_DOOR_REAR_RIGHT:
        case TARGET_DOOR_REAR_LEFT:
            doorOffsetId = ENTER_REAR;
            break;
        case TARGET_DOOR_UNK:
            doorOffsetId = ENTER_BIKE_FRONT;
            break;
        default:
            break;
        }
    } else if (doorOffsetId == JACK_PED_LEFT) {
        doorOffsetId = ENTER_FRONT;
    }

    // When unlocking a door (And the anim group says so) the offsets of the locked door are blended in
    const auto useLockedDoorBlend = assoc
        && notsa::contains({ANIM_ID_CAR_DOORLOCKED_LHS, ANIM_ID_CAR_DOORLOCKED_RHS}, assoc->m_AnimId.get())
        && vehicle->GetAnimGroup().m_specialFlags.bRunSpecialLockedDoor;

    const auto doorOffset = static_cast<eVehAnimDoorOffset>(doorOffsetId);
    CVector    offsets;
    if (useLockedDoorBlend) {
        // NOTE: The second call uses the *first* anim group, not the vehicle's (Retail quirk)
        offsets = vehicle->GetAnimGroup().ComputeAnimDoorOffsets(doorOffset) * (1.0f - assoc->m_BlendAmount)
                + CVehicleAnimGroupData::GetVehicleAnimGroup(0).ComputeAnimDoorOffsets(doorOffset) * assoc->m_BlendAmount;
    } else {
        offsets = vehicle->GetAnimGroup().ComputeAnimDoorOffsets(doorOffset);
    }

    // The dummies the doors are placed at (The original uses the first dummy for boats, the second for
    // everything else as the "front door" dummy, and the fifth one for the rear doors)
    const auto& dummyFront = dummyPos[modelInfo->IsBoat() ? DUMMY_LIGHT_FRONT_MAIN : DUMMY_LIGHT_REAR_MAIN];
    const auto& dummyRear  = dummyPos[DUMMY_SEAT_REAR];

    const CVector* dummy = &dummyFront;
    CVector        pos;
    switch (door) {
    case TARGET_DOOR_FRONT_RIGHT:
        dummy = &dummyFront;
        pos = CVector{ dummy->x + animProgress + offsets.x, dummy->y - offsets.y, dummy->z - offsets.z };
        break;
    case TARGET_DOOR_REAR_RIGHT:
        dummy = &dummyRear;
        pos = CVector{ dummy->x + animProgress + offsets.x, dummy->y - offsets.y, dummy->z - offsets.z };
        break;
    case TARGET_DOOR_DRIVER:
        dummy = &dummyFront;
        pos = CVector{ -(dummy->x + animProgress) - offsets.x, dummy->y - offsets.y, dummy->z - offsets.z };
        break;
    case TARGET_DOOR_REAR_LEFT:
        dummy = &dummyRear;
        pos = CVector{ -(dummy->x + animProgress) - offsets.x, dummy->y - offsets.y, dummy->z - offsets.z };
        break;
    case TARGET_DOOR_UNK:
        dummy = &dummyFront;
        pos = CVector{ dummy->x + offsets.x, dummy->y + offsets.y, dummy->z + offsets.z };
        break;
    default:
        dummy = &dummyFront;
        pos   = *dummy;
        offsets.Set(0.0f, 0.0f, 0.0f); // Unused in this case, but the original still clears them (They're used by the bike path below)
        break;
    }

    if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE && door != TARGET_DOOR_UNK) {
        CVector correctedPos;
        vehicle->AsBike()->GetCorrectedWorldDoorPosition(correctedPos, offsets, *dummy);
        return vehicle->GetMatrix().InverseTransformVector(correctedPos - vehicle->GetPosition());
    }

    return pos;
}

// 0x650A80
CVector CTaskUtilityLineUpPedWithCar::GetPositionToOpenCarDoor(CVehicle* vehicle, float animProgress, CAnimBlendAssociation* assoc) {
    // A local copy of the vehicle's matrix is used (The copy ctor zeroes the attach fields, so the
    // copy doesn't take ownership of the vehicle's matrix)
    const CMatrix mat{ vehicle->GetMatrix() };
    return mat.TransformVector(GetLocalPositionToOpenCarDoor(vehicle, animProgress, assoc)) + vehicle->GetPosition();
}

// 0x6513A0
bool CTaskUtilityLineUpPedWithCar::ProcessPed(CPed* ped, CVehicle* vehicle, CAnimBlendAssociation* assoc) {
    const auto door = static_cast<eTargetDoor>(m_nDoorIdx);

    bool vehTilted = false; // Set if the vehicle is tilted enough that the ped shouldn't be rotated with it

    // NOTE: The original uses the ped's movement speed as a (decaying) Z offset
    if (m_nDoorOpenPosType == 0) {
        ped->m_vecMoveSpeed.Set(0.0f, 0.0f, 0.0f);
    }

    // Determine the heading the ped should face
    {
        const auto GetVehHeading = [vehicle] {
            return vehicle->m_matrix
                ? vehicle->GetMatrix().GetUp().Heading()
                : vehicle->m_placement.m_fHeading;
        };

        if (vehicle->GetMatrix().GetUp().z <= -0.8f) {
            vehTilted = true;
            ped->m_fAimingRotation = GetVehHeading()
                - (notsa::contains({TARGET_DOOR_FRONT_RIGHT, TARGET_DOOR_REAR_RIGHT}, door) ? PI : 0.0f);
        } else if (door == TARGET_DOOR_UNK) {
            ped->m_fAimingRotation = GetVehHeading() + PI;
        } else if (m_nDoorOpenPosType != 2) {
            ped->m_fAimingRotation = GetVehHeading();
        }
    }

    float animProgress  = 0.0f; // Passed to `GetPositionToOpenCarDoor` as the anim's progress
    float blendProgress = 0.0f; // How much of the end Z to blend in
    if (assoc) {
        const auto animId       = assoc->m_AnimId.get();
        const auto animTimeNorm = assoc->m_CurrentTime / assoc->m_BlendHier->GetTotalTime(); // [0..1]

        const auto GetCriticalBlendTime = [&] { return vehicle->GetAnimGroup().ComputeCriticalBlendTime(animId); };

        switch (animId) {
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_0:
        case ANIM_ID_DEFAULT_CAR_CRAWLOUTRHS_1:
            animProgress = animTimeNorm;
            break;
        case ANIM_ID_CAR_ALIGN_LHS:
        case ANIM_ID_CAR_ALIGN_RHS:
        case ANIM_ID_CAR_ALIGNHI_LHS:
        case ANIM_ID_CAR_ALIGNHI_RHS: {
            animProgress = 1.0f;
            const auto crit = GetCriticalBlendTime();
            if (std::abs(crit) >= 10.0f) {
                const auto ramp = std::abs(crit) - 11.0f;
                if (crit - 11.0f <= 0.0f) {
                    blendProgress = animTimeNorm < ramp ? 0.0f : (animTimeNorm - ramp) / (1.0f - ramp);
                } else {
                    blendProgress = animTimeNorm < ramp ? animTimeNorm / ramp : 1.0f;
                }
            }
            if (vehicle->GetModelId() == MODEL_AT400) {
                blendProgress = 1.0f;
            }
            break;
        }
        case ANIM_ID_CAR_OPEN_LHS:
        case ANIM_ID_CAR_OPEN_RHS:
        case ANIM_ID_CAR_OPEN_LHS_1:
        case ANIM_ID_CAR_OPEN_RHS_1:
            animProgress  = 1.0f;
            blendProgress = std::abs(GetCriticalBlendTime()) >= 10.0f ? 1.0f : 0.0f;
            if (vehicle->GetModelId() == MODEL_AT400) {
                blendProgress = 1.0f;
            }
            break;
        case ANIM_ID_CAR_GETIN_LHS_0:
        case ANIM_ID_CAR_GETIN_RHS_0:
        case ANIM_ID_CAR_GETIN_LHS_1:
        case ANIM_ID_CAR_GETIN_RHS_1:
        case ANIM_ID_CAR_GETIN_BIKE_FRONT: {
            animProgress = 1.0f - animTimeNorm;
            const auto crit = GetCriticalBlendTime();
            const auto ramp = std::abs(crit);
            if (ramp >= 10.0f) {
                blendProgress = 1.0f;
            } else if (crit > 0.0f) {
                blendProgress = animTimeNorm < ramp ? animTimeNorm / ramp : 1.0f;
            } else {
                blendProgress = animTimeNorm < ramp ? 0.0f : (animTimeNorm - ramp) / (1.0f - ramp);
            }
            if (vehicle->GetModelId() == MODEL_AT400) {
                blendProgress = 1.0f;
            }
            break;
        }
        case ANIM_ID_CAR_PULLOUT_LHS:
        case ANIM_ID_CAR_PULLOUT_RHS:
        case ANIM_ID_UNKNOWN_15:
        case ANIM_ID_CAR_CLOSE_LHS_0:
        case ANIM_ID_CAR_CLOSE_RHS_0:
        case ANIM_ID_CAR_CLOSE_LHS_1:
        case ANIM_ID_CAR_CLOSE_RHS_1:
            animProgress = 1.0f;
            break;
        case ANIM_ID_CAR_CLOSEDOOR_LHS_0:
        case ANIM_ID_CAR_CLOSEDOOR_RHS_0:
        case ANIM_ID_CAR_CLOSEDOOR_LHS_1:
        case ANIM_ID_CAR_CLOSEDOOR_RHS_1:
        case ANIM_ID_CAR_SHUFFLE_RHS_0:
        case ANIM_ID_CAR_SHUFFLE_RHS_1:
        case ANIM_ID_CAR_ROLLDOOR:
            blendProgress = 1.0f;
            break;
        case ANIM_ID_CAR_GETOUT_LHS_0:
        case ANIM_ID_CAR_GETOUT_RHS_0:
        case ANIM_ID_CAR_GETOUT_LHS_1:
        case ANIM_ID_CAR_GETOUT_RHS_1:
        case ANIM_ID_CAR_JACKEDLHS:
        case ANIM_ID_CAR_JACKEDRHS:
        case ANIM_ID_CAR_ROLLOUT_LHS:
        case ANIM_ID_CAR_ROLLOUT_RHS:
        case ANIM_ID_CAR_FALLOUT_LHS:
        case ANIM_ID_CAR_FALLOUT_RHS: {
            animProgress = animTimeNorm;
            const auto crit = GetCriticalBlendTime();
            const auto ramp = std::abs(crit);
            blendProgress = crit <= 0.0f
                ? (animTimeNorm < ramp ? 1.0f : 1.0f - (animTimeNorm - ramp) / (1.0f - ramp))
                : (animTimeNorm < ramp ? 1.0f - animTimeNorm / ramp : 0.0f);
            break;
        }
        case ANIM_ID_CAR_DOORLOCKED_LHS:
        case ANIM_ID_CAR_DOORLOCKED_RHS:
            animProgress  = 1.0f;
            blendProgress = vehicle->GetAnimGroup().m_specialFlags.bRunSpecialLockedDoor ? 1.0f : 0.0f;
            break;
        default:
            break; // Both stay 0
        }
    }

    // Get the position of the door (At the start of the anim, and at its end)
    CVector start;
    if (m_nDoorOpenPosType == 2) {
        start = ped->GetPosition();
    } else {
        start = GetPositionToOpenCarDoor(vehicle, animProgress, assoc);
    }
    CVector end = start;
    if (vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE && m_nDoorOpenPosType != 2) {
        end = GetPositionToOpenCarDoor(vehicle, 1.0f, assoc);
    }

    // Fix the end position's Z
    if (!vehicle->physicalFlags.bSubmergedInWater) {
        const auto& vehPos = vehicle->GetPosition();
        const auto  right  = vehicle->m_matrix
            ? vehicle->GetMatrix().GetRight()
            : CVector{ std::cos(vehicle->m_placement.m_fHeading), std::sin(vehicle->m_placement.m_fHeading), 0.0f };
        const auto  minZ   = (vehPos.z - vehicle->GetHeightAboveRoad()) + 1.0f + right.z * (end - vehPos).Dot(right);

        end = std::get<CVector>(CPedPlacement::FindZCoorForPed(end)); // Sets the Z of its argument

        if (minZ - 0.5f > end.z) {
            end.z = minZ;
        }
    } else if (vehicle->m_nVehicleType == VEHICLE_TYPE_BOAT && vehicle->IsUpsideDown()) {
        end.z += 1.0f;
    }
    m_fDoorOpenPosZ = end.z;

    // Clamp the Z of the start position
    if (m_nDoorOpenPosType == 1 || m_nDoorOpenPosType == 2) {
        const auto pedZ     = ped->GetPosition().z;
        const auto moveSpeedZ = ped->m_vecMoveSpeed.z - CTimer::GetTimeStep() * 0.008f; // Decays by ~0.008 per frame
        if (moveSpeedZ + pedZ < end.z) {
            start.z = end.z;
            ped->m_vecMoveSpeed.Set(0.0f, 0.0f, 0.0f);
        } else {
            ped->m_vecMoveSpeed.z = moveSpeedZ;
            start.z               = pedZ;
        }
    }

    // Blend the Z of the start position
    if (m_fDoorOpenPosZ <= start.z) {
        if (m_nDoorOpenPosType == 0) {
            start.z = (start.z - m_fDoorOpenPosZ) * blendProgress + m_fDoorOpenPosZ;
        }
    } else {
        if (vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE && assoc && assoc->m_AnimId != ANIM_ID_CAR_GETIN_BIKE_FRONT) {
            float factor;
            if (notsa::contains({ANIM_ID_CAR_GETOUT_LHS_0, ANIM_ID_CAR_GETOUT_RHS_0, ANIM_ID_CAR_GETOUT_LHS_1, ANIM_ID_CAR_GETOUT_RHS_1}, assoc->m_AnimId.get())) {
                factor = 1.0f - animProgress;
            } else if (notsa::contains({ANIM_ID_CAR_GETIN_LHS_0, ANIM_ID_CAR_GETIN_RHS_0, ANIM_ID_CAR_GETIN_LHS_1, ANIM_ID_CAR_GETIN_RHS_1}, assoc->m_AnimId.get())) {
                factor = std::min(2.0f * (assoc->m_CurrentTime / assoc->m_BlendHier->GetTotalTime()), 1.0f);
            } else {
                factor = 0.0f;
            }
            start.z = ((vehicle->GetPosition().z - vehicle->GetHeightAboveRoad() + 1.0f) - m_fDoorOpenPosZ) * factor + m_fDoorOpenPosZ;
        } else {
            start.z = (start.z - m_fDoorOpenPosZ) * blendProgress + m_fDoorOpenPosZ;
        }
    }

    // Rotate the ped towards the target heading
    if (CTimer::GetTimeInMS() >= static_cast<uint32>(m_fTime)) {
        ped->m_fCurrentRotation = ped->m_fAimingRotation;
    } else {
        const auto oldRotation = ped->m_fCurrentRotation;
        const auto factor      = static_cast<float>(static_cast<uint32>(m_fTime) - CTimer::GetTimeInMS()) * (1.0f / 600.0f); // The m_fTime is a deadline, and the blend takes m_fTime ms
        if (factor <= 0.0f) {
            // Unreachable (There's still time left, so `factor` is positive), but it's in the original
            m_Offset.x = 0.0f;
            m_Offset.y = 0.0f;
        }
        m_Offset.z = 0.0f; // NOTE: `m_Offset` is mutated by the original

        start.x -= factor * m_Offset.x;
        start.y -= factor * m_Offset.y;

        auto angle = CGeneral::LimitRadianAngle(ped->m_fAimingRotation);
        if (angle > oldRotation + PI) {
            angle -= TWO_PI;
        } else if (angle < oldRotation - PI) {
            angle += TWO_PI;
        }

        ped->m_fCurrentRotation = oldRotation - (1.0f - factor) * (oldRotation - angle);
    }

    // Blend the ped's rotation with the vehicle's (Or the other way around, depending on the anim)
    if (assoc) {
        const auto animId = assoc->m_AnimId.get();

        // NOTE: The `RtQuatSlerp` below uses `RtQuatSetupSlerpCache`, so the quats are the ones converted
        //       from the matrices, while `rT` is the anim's progress.
        const auto animTimeNorm = assoc->m_CurrentTime / assoc->m_BlendHier->GetTotalTime();

        if (notsa::contains({ANIM_ID_CAR_GETIN_LHS_0, ANIM_ID_CAR_GETIN_RHS_0, ANIM_ID_CAR_GETIN_LHS_1, ANIM_ID_CAR_GETIN_RHS_1, ANIM_ID_CAR_GETIN_BIKE_FRONT}, animId)) {
            // NOTE: The copies' attach fields are zeroed by the copy ctor, so they never take ownership
            //       of the matrices they were copied from.
            CMatrix vehMat{ vehicle->GetMatrix() };
            if (animId == ANIM_ID_CAR_GETIN_BIKE_FRONT) {
                CMatrix rotMat{};
                rotMat.SetRotateZ(PI);
                vehMat *= rotMat;
            }

            RwMatrix rwMatrix{};
            RtQuat   qVeh;
            vehMat.CopyToRwMatrix(&rwMatrix);
            RtQuatConvertFromMatrix(&qVeh, &rwMatrix);

            CMatrix pedMat{ ped->GetMatrix() };
            RtQuat  qPed;
            pedMat.CopyToRwMatrix(&rwMatrix);
            RtQuatConvertFromMatrix(&qPed, &rwMatrix);

            RtQuatSlerpCache slerpCache;
            RtQuatSetupSlerpCache(&qPed, &qVeh, &slerpCache);

            RtQuat q;
            RtQuatSlerp(&q, &qPed, &qVeh, animTimeNorm, &slerpCache);

            auto mat = RotationMatrixFromQuat(q);
            mat.GetPosition() = start;
            ped->SetMatrix(mat);

            return false; // The original returns here, skipping the placement below
        } else if (notsa::contains({ANIM_ID_CAR_GETOUT_LHS_0, ANIM_ID_CAR_GETOUT_RHS_0, ANIM_ID_CAR_GETOUT_LHS_1, ANIM_ID_CAR_GETOUT_RHS_1, ANIM_ID_UNKNOWN_26}, animId)) {
            RwMatrix rwMatrix{};
            RtQuat   qVeh;
            CMatrix vehMat{ vehicle->GetMatrix() };
            vehMat.CopyToRwMatrix(&rwMatrix);
            RtQuatConvertFromMatrix(&qVeh, &rwMatrix);

            CMatrix pedMat{ ped->GetMatrix() };
            pedMat.SetRotateZOnly(ped->m_fCurrentRotation);

            RtQuat qPed;
            pedMat.CopyToRwMatrix(&rwMatrix);
            RtQuatConvertFromMatrix(&qPed, &rwMatrix);

            RtQuatSlerpCache slerpCache;
            RtQuatSetupSlerpCache(&qVeh, &qPed, &slerpCache);

            RtQuat q;
            RtQuatSlerp(&q, &qVeh, &qPed, animTimeNorm, &slerpCache);

            auto mat = RotationMatrixFromQuat(q);
            mat.GetPosition() = start;
            ped->SetMatrix(mat);

            return false; // The original returns here, skipping the placement below
        }
    }

    // Place the ped
    if (animProgress > 0.2f || vehTilted || vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_QUAD) {
        ped->GetPosition() = start;
        ped->SetOrientation(0.0f, 0.0f, ped->m_fCurrentRotation);
    } else {
        CMatrix mat{ vehicle->GetMatrix() };
        mat.GetPosition() += mat.TransformVector(GetLocalPositionToOpenCarDoor(vehicle, 0.0f, assoc));
        ped->SetMatrix(mat);
    }

    return false;
}
