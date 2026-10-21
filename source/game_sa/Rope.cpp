#include "StdInc.h"

#include "Rope.h"
#include "Ropes.h"

void CRope::InjectHooks() {
    RH_ScopedClass(CRope);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(ReleasePickedUpObject, 0x556030);
    RH_ScopedInstall(CreateHookObjectForRope, 0x556070);
    RH_ScopedInstall(UpdateWeightInRope, 0x5561B0);
    RH_ScopedInstall(Remove, 0x556780);
    RH_ScopedInstall(Render, 0x556800);
    RH_ScopedInstall(PickUpObject, 0x5569C0);
    RH_ScopedInstall(Update, 0x557530);
}

// inlined see 0x557959
// use switch like in Android?
// 0x555FB0
bool CRope::DoControlsApply() const {
    return    m_nType == eRopeType::CRANE_MAGNO && CRopes::PlayerControlsCrane == eControlledCrane::MAGNO_CRANE
           || m_nType == eRopeType::WRECKING_BALL && CRopes::PlayerControlsCrane == eControlledCrane::WRECKING_BALL
           || m_nType == eRopeType::CRANE_TROLLEY && CRopes::PlayerControlsCrane == eControlledCrane::LAS_VEGAS_CRANE
           || m_nType == eRopeType::QUARRY_CRANE_ARM && CRopes::PlayerControlsCrane == eControlledCrane::QUARRY_CRANE
           || m_nType == eRopeType::CRANE_MAGNET1
           || m_nType == eRopeType::MAGNET
           || m_nType == eRopeType::CRANE_HARNESS;
}

// 0x556030
void CRope::ReleasePickedUpObject() {
    if (m_pRopeAttachObject) {
        m_pRopeAttachObject->AsPhysical()->physicalFlags.bAttachedToEntity = false;
        m_pRopeAttachObject->AsPhysical()->physicalFlags.bCarriedByRope = false;
        m_pRopeAttachObject = nullptr;
    }
    m_pAttachedEntity->SetUsesCollision(true);
    m_nFlags1 = 60; // 6th, 7th bits set
}

// 0x556070
void CRope::CreateHookObjectForRope() {
    if (m_pAttachedEntity)
        return;

    using namespace ModelIndices;

    const auto modelIndex = [&]() -> ModelIndex {
        switch (m_nType) {
        case eRopeType::CRANE_MAGNET1:
        case eRopeType::CRANE_MAGNO:
        case eRopeType::QUARRY_CRANE_ARM:
        case eRopeType::CRANE_TROLLEY:
            return MI_CRANE_MAGNET;
        case eRopeType::CRANE_HARNESS:
            return MI_CRANE_HARNESS;
        case eRopeType::MAGNET:
            return MI_MINI_MAGNET;
        case eRopeType::WRECKING_BALL:
            return MI_WRECKING_BALL;
        case eRopeType::SWAT:
            return MODEL_INVALID; // Just so the assert below wont be hit.
        default:
            NOTSA_UNREACHABLE(); //assert(0);
        }
    }();
    if (modelIndex == ModelIndex{ MODEL_INVALID }) { // Must do it like this because `ModelIndex` is u16, `MODEL_ID` is i32, and u16 -1 casted to int32 is 0xffff...
        return;
    }

    auto* obj = new CObject(modelIndex, true);
    m_pAttachedEntity = obj;

    obj->RegisterReference(reinterpret_cast<CEntity**>(&m_pAttachedEntity));
    obj->SetPosn(m_aSegments[NUM_ROPE_SEGMENTS - 1]);
    obj->m_nObjectType = OBJECT_TYPE_DECORATION;
    obj->SetIsStatic(false);
    obj->physicalFlags.bAttachedToEntity = true;

    CWorld::Add(m_pAttachedEntity);

    m_pRopeAttachObject = nullptr;
    m_nFlags1 = 0;
}

// 0x5561B0
int8 CRope::UpdateWeightInRope(float x, float y, float z, int32 /*a5*/, float* out) {
    // NOTE: The anchor is the last segment that's fixed in place - It's read *before* `m_aSegments[31]` is overwritten
    const CVector anchor = m_aSegments[m_nSegments];

    m_aSegments[NUM_ROPE_SEGMENTS - 1] = CVector{ x, y, z };

    // The length of the part of the rope that's still free to move
    const float maxLen = (float)((int32)NUM_ROPE_SEGMENTS - 1 - (int32)m_nSegments) * m_fSegmentLength;

    const CVector delta = CVector{ x, y, z } - anchor;
    const float   dist  = delta.Magnitude();

    if (maxLen <= dist) {
        // The rope is too short to reach the target - Lay out the remaining segments in a straight line towards it
        const float   k       = maxLen / dist;
        const CVector clipped = delta * k;

        out[0] = anchor.x + clipped.x;
        out[1] = anchor.y + clipped.y;
        out[2] = anchor.z + clipped.z;

        for (auto i = (int32)m_nSegments + 1; i < (int32)NUM_ROPE_SEGMENTS; i++) {
            CVector dir = clipped;
            dir.Normalise();
            m_aSegments[i] = anchor + dir * m_fSegmentLength * (float)(i - (int32)m_nSegments);
        }

        return 1;
    }

    // Shrink the segments that are too far apart from each other (6 passes)
    for (auto pass = 0; pass < 6; pass++) {
        // Pull each segment towards the one below it
        if ((int32)m_nSegments + 1 < (int32)NUM_ROPE_SEGMENTS - 1) {
            for (auto i = (int32)NUM_ROPE_SEGMENTS - 1; i > (int32)m_nSegments + 1; i--) {
                const CVector old    = m_aSegments[i - 1];
                const CVector d      = m_aSegments[i] - old;
                const float   distTo = d.Magnitude();

                if (!(distTo > m_fSegmentLength)) {
                    continue;
                }

                m_aSegments[i - 1] = m_aSegments[i] + (old - m_aSegments[i]) * (m_fSegmentLength / distTo);

                const float invDt = 1.0f / CTimer::GetTimeStep();
                m_aSpeed[i - 1] = (m_aSegments[i - 1] - old) * invDt;
            }
        }

        // Pull each segment towards the one above it (NOTE: The last pair is handled by the loop below)
        if ((int32)m_nSegments + 1 < (int32)NUM_ROPE_SEGMENTS - 1) {
            for (auto j = (int32)m_nSegments + 1; j < (int32)NUM_ROPE_SEGMENTS - 1; j++) {
                const CVector d = m_aSegments[j] - m_aSegments[j - 1];

                if (!(d.Magnitude() > m_fSegmentLength)) {
                    continue;
                }

                m_aSegments[j] = m_aSegments[j - 1] + d * (m_fSegmentLength / d.Magnitude());
            }
        }
    }

    // Finally, pull the segments of the last pair (until one of them is close enough)
    if ((int32)m_nSegments + 1 < (int32)NUM_ROPE_SEGMENTS - 1) {
        for (auto j = (int32)NUM_ROPE_SEGMENTS - 1; j > (int32)m_nSegments + 1; j--) {
            CVector&      cur    = m_aSegments[j];
            CVector&      prev   = m_aSegments[j - 1];
            const CVector d      = cur - prev;
            const float   distTo = d.Magnitude();

            if (distTo <= m_fSegmentLength) {
                break;
            }

            prev = cur + (prev - cur) * (m_fSegmentLength / distTo);
        }
    }

    return 0;
}

// 0x556780
void CRope::Remove() {
    m_nType = eRopeType::NONE;
    if (m_pRopeAttachObject)
        ReleasePickedUpObject();

    if (m_pAttachedEntity) {
        CWorld::Remove(m_pAttachedEntity);
        delete m_pAttachedEntity;
        m_pAttachedEntity = nullptr;
    }
}

// 0x556800
void CRope::Render() {
    // Note: Probably needs adjustments if `NUM_ROPE_SEGMENTS` is changed
    if (!TheCamera.IsSphereVisible(m_aSegments[NUM_ROPE_SEGMENTS / 2], 20.0f))
        return;

    if ((TheCamera.GetPosition() - m_aSegments[0]).Magnitude2D() >= 120.0f)
        return;

    DefinedState();

    const auto GetVertex = [](unsigned i) {
        return &TempBufferVertices.m_3d[i];
    };

    const RwRGBA color = { 0, 0, 0, 128 };
    for (auto i = 0u; i < NUM_ROPE_SEGMENTS; i++) {
        RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color);
        RxObjSpace3DVertexSetPos(GetVertex(i), &m_aSegments[i]);
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,     RWRSTATE(rwFILTERLINEAR));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(FALSE));

    if (RwIm3DTransform(TempBufferVertices.m_3d, NUM_ROPE_SEGMENTS, nullptr, 0)) {
        RxVertexIndex indices[] = { // *(RxVertexIndex(*)[64])0x8CD818
            0,  1,  1,  2,  2,  3,  3,  4,
            4,  5,  5,  6,  6,  7,  7,  8,
            8,  9,  9,  10, 10, 11, 11, 12,
            12, 13, 13, 14, 14, 15, 15, 16,
            16, 17, 17, 18, 18, 19, 19, 20,
            20, 21, 21, 22, 22, 23, 23, 24,
            24, 25, 25, 26, 26, 27, 27, 28,
            28, 29, 29, 30, 30, 31, 31, 32
        };
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST, indices, std::size(indices) - 2); // the last two indexes are not used
        RwIm3DEnd();
    }

    if (m_nType == eRopeType::QUARRY_CRANE_ARM) {
        const CVector pos[] = { m_aSegments[0], { 709.32f, 916.20f, 53.0f } }; // Hunter Quarry
        for (auto i = 0u; i < std::size(pos); i++) {
            RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color);
            RxObjSpace3DVertexSetPos(GetVertex(i), &pos[i]);
        }
        if (RwIm3DTransform(TempBufferVertices.m_3d, std::size(pos), nullptr, 0)) {
            RxVertexIndex indices[] = { 0, 1 };
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST, indices, std::size(indices));
            RwIm3DEnd();
        }
    }
}

// 0x5569C0
void CRope::PickUpObject(CEntity* obj) {
    if (m_pRopeAttachObject == obj)
        return;

    if (m_pRopeAttachObject)
        ReleasePickedUpObject();

    obj->RegisterReference(&m_pAttachedEntity);
    m_pRopeAttachObject = obj;

    // TODO: Move model => world space translation into CEntity
    // MultiplyMatrixWithVector should be used here
    CVector height = { {}, {}, CRopes::FindPickupHeight(obj) };
    m_pAttachedEntity->SetPosn(obj->GetPosition() + obj->GetMatrix().TransformVector(height));
    m_pAttachedEntity->SetUsesCollision(false);

    obj->AsPhysical()->physicalFlags.bAttachedToEntity = true;
    if (obj->GetIsTypeVehicle()) {
        if (obj->GetStatus() == STATUS_SIMPLE)
        {
            obj->SetStatus(STATUS_PHYSICS);
        }
    } else if (obj->GetIsTypeObject()) {
        if (obj->GetIsStatic()) {
            obj->AsObject()->SetIsStatic(false);
            obj->AsObject()->AddToMovingList();
            obj->AsObject()->m_nFakePhysics = 0;
        }
    }
}

// 0x557530
void CRope::Update() {
    using namespace ModelIndices;

    // NOTE: The speed relaxation factor is `0.8 ^ timeStep`
    const float alpha = (float)std::pow(0.8, (double)CTimer::GetTimeStep());
    const float beta  = 1.0f - alpha;

    // Ropes that are too far away from the camera aren't updated
    if ((TheCamera.GetPosition() - m_aSegments[0]).Magnitude2D() >= 200.0f) {
        return;
    }

    // The top segment is free to move unless the rope is anchored or hasn't expired yet
    if (!(m_nFlags2 & 1) && m_nTime < CTimer::GetTimeInMS()) {
        m_aSpeed[0].z -= CTimer::GetTimeStep() * 0.0015f;
        m_aSegments[0] += m_aSpeed[0] * CTimer::GetTimeStep();
    }

    // "On ground" ropes
    if (m_nFlags2 & 4) {
        if ((CTimer::GetFrameCounter() & 7) == 2) {
            m_fGroundZ = CWorld::FindGroundZFor3DCoord(m_aSegments[0]);
        }

        if (m_pRopeAttachObject) {
            const auto modelIndex = m_pRopeAttachObject->m_nModelIndex;
            if (   m_pRopeAttachObject->GetType() == ENTITY_TYPE_VEHICLE
                || modelIndex == MI_OBJECTFORMAGNOCRANE1
                || modelIndex == MI_OBJECTFORMAGNOCRANE2
                || modelIndex == MI_OBJECTFORMAGNOCRANE3
                || modelIndex == MI_OBJECTFORMAGNOCRANE5
            ) {
                m_fGroundZ = m_pAttachedEntity->GetPosition().z - 0.5f;
            }
        }
    }

    // Integrate and constrain the segments below the anchored ones
    for (auto i = (uint32)m_nSegments + 1; i < NUM_ROPE_SEGMENTS; i++) {
        const CVector old = m_aSegments[i];

        m_aSpeed[i].x += (float)((rand() & 0xF) - 8) * 0.001f;
        m_aSpeed[i].y += (float)((rand() & 0xF) - 8) * 0.001f;

        m_aSpeed[i].x = beta * m_aSpeed[i - 1].x + alpha * m_aSpeed[i].x;
        m_aSpeed[i].y = beta * m_aSpeed[i - 1].y + alpha * m_aSpeed[i].y;
        m_aSpeed[i].z = beta * m_aSpeed[i - 1].z + alpha * m_aSpeed[i].z;

        m_aSegments[i].z -= CTimer::GetTimeStep() * 0.15f;

        if (m_nFlags2 & 4) {
            m_aSegments[i].z = std::max(m_aSegments[i].z, m_fGroundZ + 0.3f);
        }

        // Constraint the segment to be exactly one segment length away from the previous one
        const CVector delta = m_aSegments[i] - m_aSegments[i - 1];
        m_aSegments[i] = m_aSegments[i - 1] + delta * (m_fSegmentLength / delta.Magnitude());

        const float invDt = 1.0f / CTimer::GetTimeStep();
        m_aSpeed[i] = (m_aSegments[i] - old) * invDt;
    }

    // Crane controls
    if (DoControlsApply()) {
        if (m_nType <= eRopeType::MAGNET) {
            const int16 upDown = CPad::GetPad(0)->GetCarGunUpDown();
            if ((upDown < 0 && CTheScripts::bEnableCraneRaise) || (0 < upDown && CTheScripts::bEnableCraneLower)) {
                m_fLengthFraction -= (float)upDown * CTimer::GetTimeStep() * 1e-5f;
            }
            m_fLengthFraction = std::min(m_fLengthFraction, 0.84f);
        } else {
            if (CTheScripts::bEnableCraneRaise) {
                const float delta = (float)CPad::GetPad(0)->NewState.ButtonSquare * CTimer::GetTimeStep() * 1e-5f;
                if (0.0f < delta && m_fLengthFraction + delta < 0.9f) {
                    AudioEngine.ReportMissionAudioEvent(AE_CRANE_WINCH_MOVE, m_pRopeHolder->AsPhysical(), 0.0f, 1.0f);
                }
                m_fLengthFraction += delta;
            }
            if (CTheScripts::bEnableCraneLower) {
                const float delta = (float)CPad::GetPad(0)->NewState.ButtonCross * CTimer::GetTimeStep() * 1e-5f;
                if (0.0f < delta && 0.01f < m_fLengthFraction - delta) {
                    AudioEngine.ReportMissionAudioEvent(AE_CRANE_WINCH_MOVE, m_pRopeHolder->AsPhysical(), 0.0f, 1.0f);
                }
                m_fLengthFraction -= delta;
            }
        }
        m_fLengthFraction = std::clamp(m_fLengthFraction, 0.01f, 0.9f);
    }

    // Figure out what (if anything) is being held by the rope, and how strongly it should follow it
    CPhysical* held      = nullptr;
    float      physAlpha = 0.06f;
    if (m_pRopeAttachObject) {
        held = m_pRopeAttachObject->AsPhysical();

        const float mass = (m_pRopeAttachObject->m_nModelIndex == MODEL_SECURICA)
            ? 750.0f
            : held->m_fMass;
        physAlpha = (0.13f * (1.0f / 1200.0f)) * mass + 0.06f;
        if (physAlpha > 0.5f) {
            physAlpha = 0.5f;
        }

        held->m_nFakePhysics = 0;
    } else if (m_pAttachedEntity) { // NOTE: Retail dereferences `m_pAttachedEntity` without checking (It's null for `eRopeType::SWAT` ropes)
        held = m_pAttachedEntity->AsPhysical();

        held->SetUsesCollision(true);
    }

    if (held) {
        CVector       newPos;
        const CVector heldPos = held->GetPosition();
        if (UpdateWeightInRope(heldPos.x, heldPos.y, heldPos.z, 0, &newPos.x)) {
            held->SetPosn(newPos);

            // Kill the velocity component along the rope
            const CVector holderPos   = m_pRopeHolder->GetPosition();
            CVector       holderSpeed = m_pRopeHolder->AsPhysical()->GetSpeed(m_aSegments[0] - holderPos);
            if (m_nType >= eRopeType::CRANE_MAGNO && m_nType <= eRopeType::CRANE_TROLLEY) {
                holderSpeed = CVector{}; // Cranes are anchored in place
            }

            CVector dir = held->GetPosition() - m_aSegments[0];
            dir.Normalise();

            CVector     v    = held->m_vecMoveSpeed - holderSpeed;
            const float proj = DotProduct(dir, v);
            if (proj > 0.0f) {
                v -= dir * proj;
            }

            // Apply most of the correction to the held object, and the rest (in the opposite direction) to the holder
            const CVector delta = (v + holderSpeed) - held->m_vecMoveSpeed;
            held->m_vecMoveSpeed += delta * (1.0f - physAlpha);
            m_pRopeHolder->AsPhysical()->m_vecMoveSpeed -= delta * physAlpha;

            // Orient the held object along the rope
            if (!m_pRopeAttachObject) {
                held->GetMatrix().ForceUpVector(-dir);
            } else {
                CMatrix upCorrected = held->GetMatrix();
                upCorrected.ForceUpVector(-dir);

                const CMatrix original = held->GetMatrix();

                CMatrix& mat  = held->GetMatrix();
                mat.GetRight()   = upCorrected.GetRight()   * 0.1f + original.GetRight()   * 0.9f;
                mat.GetForward() = upCorrected.GetForward() * 0.1f + original.GetForward() * 0.9f;
                mat.GetUp()      = upCorrected.GetUp()      * 0.1f + original.GetUp()      * 0.9f;
            }

            // The hook object follows the carried object
            if (m_pRopeAttachObject) {
                m_pAttachedEntity->GetMatrix() = m_pRopeAttachObject->GetMatrix();
                m_pAttachedEntity->AsPhysical()->m_vecMoveSpeed = m_pRopeAttachObject->AsPhysical()->m_vecMoveSpeed;
                m_pAttachedEntity->SetPosn(m_pRopeAttachObject->GetMatrix().TransformPoint(
                    { 0.0f, 0.0f, CRopes::FindPickupHeight(m_pRopeAttachObject) }
                ));
            }
        }
    }

    if (m_pRopeAttachObject) {
        // Release the carried object on request
        if (DoControlsApply() && CTheScripts::bEnableCraneRelease && CPad::GetPad(0)->CarGunJustDown()) {
            ReleasePickedUpObject();
        }

        // Let go of destroyed objects
        if (m_pRopeAttachObject && m_pRopeAttachObject->AsPhysical()->physicalFlags.bRenderScorched) {
            auto* const phys = m_pRopeAttachObject->AsPhysical();
            phys->physicalFlags.bRenderScorched = false;
            phys->physicalFlags.bCarriedByRope  = false;
            m_pAttachedEntity->SetUsesCollision(true);
            m_pRopeAttachObject = nullptr;
            m_nFlags1           = 60;
        }

        // Let go of bikes that got a rider in the meantime
        if (m_pRopeAttachObject
            && m_pRopeAttachObject->GetType() == ENTITY_TYPE_VEHICLE
            && m_pRopeAttachObject->AsVehicle()->m_nVehicleSubType == VEHICLE_TYPE_BIKE
            && m_pRopeAttachObject->AsVehicle()->m_pDriver
        ) {
            ReleasePickedUpObject();
        }
    } else if (m_nFlags1 != 0) {
        // Wait a bit before trying to pick up something again
        --m_nFlags1;
    } else {
        bool wantMagnoCrates      = false;
        bool wantBuildingSiteCrane = false;
        bool wantQuarryRocks      = false;
        bool wantWongDish         = false;
        bool wantKmbRock          = false;
        bool wantKmbPlank         = false;
        bool wantKmbBomb          = false;
        bool wantOnlyVehicle234   = false;
        bool tryVehicles          = false;
        bool tryPeds              = false;

        switch (m_nType) {
        case eRopeType::CRANE_MAGNET1:
            tryVehicles = true;
            break;
        case eRopeType::CRANE_HARNESS:
            tryPeds = true;
            break;
        case eRopeType::MAGNET:
            tryVehicles        = true;
            wantWongDish       = true;
            wantKmbRock        = true;
            wantKmbPlank       = true;
            wantKmbBomb        = true;
            wantOnlyVehicle234 = true;
            break;
        case eRopeType::CRANE_MAGNO:
            tryVehicles     = true;
            wantMagnoCrates = true;
            break;
        case eRopeType::QUARRY_CRANE_ARM:
            tryVehicles     = true;
            wantQuarryRocks = true;
            break;
        case eRopeType::CRANE_TROLLEY:
            wantBuildingSiteCrane = true;
            break;
        default:
            break; // Nothing to pick up
        }

        if (tryVehicles) {
            for (auto& v : GetVehiclePool()->GetAllValid()) {
                if (!(   v.m_nVehicleSubType == VEHICLE_TYPE_AUTOMOBILE
                      || (v.m_nVehicleSubType == VEHICLE_TYPE_BIKE && !v.m_pDriver)
                      || v.m_nVehicleSubType == VEHICLE_TYPE_MTRUCK
                      || v.m_nModelIndex == MODEL_DINGHY
                      || v.m_nModelIndex == MODEL_VORTEX)
                ) {
                    continue;
                }
                if (v.physicalFlags.bRenderScorched) {
                    continue;
                }
                if (!v.vehicleFlags.bWinchCanPickMeUp) {
                    continue;
                }
                if (wantOnlyVehicle234 && v.m_nModelIndex != MODEL_RCTIGER) {
                    continue;
                }
                if (v.m_ropeType) {
                    continue;
                }

                const CVector pickupPos = v.GetMatrix().TransformPoint({ 0.0f, 0.0f, CRopes::FindPickupHeight(&v) });
                if ((pickupPos - m_aSegments[NUM_ROPE_SEGMENTS - 1]).Magnitude() >= 2.5f) {
                    continue;
                }

                m_pRopeAttachObject = &v;
                v.RegisterReference(&m_pRopeAttachObject);
                v.physicalFlags.bCarriedByRope = true;
                if (v.GetStatus() == STATUS_SIMPLE) {
                    v.SetStatus(STATUS_PHYSICS);
                }

                m_pAttachedEntity->SetPosn(pickupPos);
                m_pAttachedEntity->SetUsesCollision(false);

                if (0.0f < m_aSegments[NUM_ROPE_SEGMENTS - 1].z - pickupPos.z) {
                    const float dz = m_aSegments[NUM_ROPE_SEGMENTS - 1].z - pickupPos.z;
                    m_fLengthFraction = std::max(
                        (m_fLengthFraction - dz * (1.0f / m_fRopeLength)) - (1.0f / m_fRopeLength) * m_fSegmentLength,
                        0.01f
                    );
                }
                break;
            }
        }

        if (tryPeds) {
            for (auto& p : GetPedPool()->GetAllValid()) {
                if (p.IsStateDead() || p.IsPlayer() || p.bInVehicle) {
                    continue;
                }

                const CVector pickupPos = p.GetPosition();
                if ((pickupPos - m_aSegments[NUM_ROPE_SEGMENTS - 1]).Magnitude() >= 2.5f) {
                    continue;
                }

                m_pRopeAttachObject = &p;
                p.RegisterReference(&m_pRopeAttachObject);
                p.physicalFlags.bCarriedByRope = true;

                m_pAttachedEntity->SetPosn(p.GetMatrix().TransformPoint({ 0.0f, 0.0f, CRopes::FindPickupHeight(&p) }));
                m_pAttachedEntity->SetUsesCollision(false);

                if (0.0f < m_aSegments[NUM_ROPE_SEGMENTS - 1].z - pickupPos.z) {
                    const float dz = m_aSegments[NUM_ROPE_SEGMENTS - 1].z - pickupPos.z;
                    m_fLengthFraction = std::max(
                        (m_fLengthFraction - dz * (1.0f / m_fRopeLength)) - (1.0f / m_fRopeLength) * m_fSegmentLength,
                        0.01f
                    );
                }
                break;
            }
        }

        // NOTE: `wantBuildingSiteCrane` (rope type `eRopeType::CRANE_TROLLEY`) jumps straight into the object loop in retail
        if (wantMagnoCrates || wantBuildingSiteCrane || wantQuarryRocks || wantWongDish || wantKmbRock || wantKmbPlank || wantKmbBomb) {
            if (!m_pRopeAttachObject) {
                for (auto& o : GetObjectPool()->GetAllValid()) {
                    if (!o.objectFlags.bCanBeAttachedToMagnet) {
                        continue;
                    }

                    const auto modelIndex = o.m_nModelIndex;
                    if (!(   (wantMagnoCrates      && (modelIndex == MI_OBJECTFORMAGNOCRANE1
                                                    || modelIndex == MI_OBJECTFORMAGNOCRANE2
                                                    || modelIndex == MI_OBJECTFORMAGNOCRANE3
                                                    || modelIndex == MI_OBJECTFORMAGNOCRANE4
                                                    || modelIndex == MI_OBJECTFORMAGNOCRANE5))
                          || (wantBuildingSiteCrane && modelIndex == MI_OBJECTFORBUILDINGSITECRANE1)
                          || (wantQuarryRocks     && (modelIndex == MI_QUARY_ROCK1
                                                   || modelIndex == MI_QUARY_ROCK2
                                                   || modelIndex == MI_QUARY_ROCK3
                                                   || modelIndex == MI_DEAD_TIED_COP)
                                                 && !o.m_pAttachedTo)
                          || (wantWongDish        && modelIndex == MI_WONG_DISH)
                          || (wantKmbRock         && modelIndex == MI_KMB_ROCK)
                          || (wantKmbPlank        && modelIndex == MI_KMB_PLANK)
                          || (wantKmbBomb         && modelIndex == MI_KMB_BOMB))
                    ) {
                        continue;
                    }

                    // Find the highest of the candidate pickup points
                    const float   h   = CRopes::FindPickupHeight(&o);
                    const CMatrix mat = o.GetMatrix();

                    CVector best = mat.TransformPoint(CVector{ 0.0f, 0.0f, h });
                    int32   rot  = 0;
                    if (   modelIndex == MI_OBJECTFORMAGNOCRANE1
                        || modelIndex == MI_OBJECTFORMAGNOCRANE2
                        || modelIndex == MI_OBJECTFORMAGNOCRANE3
                        || modelIndex == MI_OBJECTFORMAGNOCRANE5
                    ) {
                        CVector point = mat.TransformPoint(CVector{ 0.0f, 0.0f, -h });
                        if (best.z < point.z) {
                            best = point;
                            rot  = 2;
                        }
                        point = mat.TransformPoint(CVector{ h, 0.0f, 0.0f });
                        if (best.z < point.z) {
                            best = point;
                            rot  = 3;
                        }
                        point = mat.TransformPoint(CVector{ -h, 0.0f, 0.0f });
                        if (best.z < point.z) {
                            best = point;
                            rot  = 1;
                        }
                    }

                    if ((best - m_aSegments[NUM_ROPE_SEGMENTS - 1]).Magnitude() >= 2.5f) {
                        continue;
                    }

                    m_pRopeAttachObject = &o;
                    o.RegisterReference(&m_pRopeAttachObject);
                    o.physicalFlags.bCarriedByRope = true;

                    m_pAttachedEntity->SetPosn(o.GetMatrix().TransformPoint(CVector{ 0.0f, 0.0f, h }));
                    m_pAttachedEntity->SetUsesCollision(false);

                    if (o.m_bIsStatic || o.m_bIsStaticWaitingForCollision) {
                        o.SetIsStatic(false);
                        o.AddToMovingList();
                    }
                    o.physicalFlags.bAttachedToEntity = true;

                    if (0.0f < m_aSegments[NUM_ROPE_SEGMENTS - 1].z - best.z) {
                        const float dz = m_aSegments[NUM_ROPE_SEGMENTS - 1].z - best.z;
                        m_fLengthFraction = std::max(
                            (m_fLengthFraction - dz * (1.0f / m_fRopeLength)) - (1.0f / m_fRopeLength) * m_fSegmentLength,
                            0.01f
                        );
                    }

                    // Rotate the object so its up vector points towards the rope
                    for (; rot != 0; rot--) {
                        CMatrix&      oMat  = o.GetMatrix();
                        const CVector right = oMat.GetRight();
                        oMat.GetRight() = oMat.GetUp();
                        oMat.GetUp()    = -right;
                    }
                    break;
                }
            }
        }
    }

    // Remove ropes that fell into the void
    if (!(m_nFlags2 & 1) && m_aSegments[0].z < -50.0f) {
        Remove();
    }
    m_nFlags2 &= 0xFE;
}
