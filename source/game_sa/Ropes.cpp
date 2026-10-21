/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Rope.h"
#include "Ropes.h"

void CRopes::InjectHooks() {
    RH_ScopedClass(CRopes);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x555DC0);
    RH_ScopedInstall(Shutdown, 0x556B10);
    RH_ScopedInstall(Update, 0x558D70);
    RH_ScopedInstall(Render, 0x556AE0);
    RH_ScopedInstall(RegisterRope, 0x556B40);
    RH_ScopedInstall(FindPickupHeight, 0x556760);
    RH_ScopedInstall(FindRope, 0x556000);
    RH_ScopedInstall(FindCoorsAlongRope, 0x555E40);
    RH_ScopedInstall(CreateRopeForSwatPed, 0x558D10);
    RH_ScopedInstall(IsCarriedByRope, 0x555F80);
    RH_ScopedInstall(SetSpeedOfTopNode, 0x555DF0);
}

// 0x555DC0
void CRopes::Init() {
    for (auto& rope : aRopes) {
        rope.m_nType = eRopeType::NONE;
    }
    PlayerControlsCrane = eControlledCrane::NONE;
}

// 0x556B10
void CRopes::Shutdown() {
    for (auto& rope : aRopes) {
        if (rope.m_nType == eRopeType::NONE)
            continue;

        rope.Remove();
    }
}

// 0x558D70
void CRopes::Update() {
    ZoneScoped;

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE)
            rope.Update();
    }
}

// 0x556AE0
void CRopes::Render() {
    ZoneScoped;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE)
            rope.Render();
    }
}

// Must be used in loop to make attached to holder
// 0x556B40
bool CRopes::RegisterRope(uint32 ropeID, uint32 ropeType, CVector startPos, bool bExpires, uint8 segmentCount, uint8 flags, CPhysical* holder, uint32 timeExpire) {
    // Re-register an already existing rope (Of the same id)
    for (auto& rope : aRopes) {
        if (rope.m_nType == eRopeType::NONE || rope.m_nId != ropeID) {
            continue;
        }

        rope.m_aSegments[0] = startPos;
        rope.m_aSpeed[0]    = CVector{};
        rope.m_nFlags2     |= 1; // "Registered this frame"
        rope.m_nSegments    = segmentCount;
        for (auto i = 0u; i <= (uint32)rope.m_nSegments; i++) { // NOTE: Inclusive
            rope.m_aSegments[i] = startPos;
            rope.m_aSpeed[i]    = CVector{};
        }

        rope.CreateHookObjectForRope();
        return true;
    }

    // Find a free slot
    CRope* rope = nullptr;
    for (auto& candidate : aRopes) {
        if (candidate.m_nType == eRopeType::NONE) {
            rope = &candidate;
            break;
        }
    }
    if (!rope) {
        return false;
    }

    rope->m_nId               = ropeID;
    rope->m_aSegments[0]      = startPos;
    rope->m_aSpeed[0]         = CVector{};
    rope->m_nSegments         = segmentCount;
    rope->m_nFlags2           = (uint8)(((flags & 1) << 2) | (rope->m_nFlags2 & 0xF9) | 1); // Bit 0: "Registered this frame", Bit 2: "On ground", bits 1, 3-7 preserved
    rope->m_fGroundZ          = 0.0f;
    rope->m_pAttachedEntity   = nullptr;
    rope->m_pRopeAttachObject = nullptr;
    rope->m_fLengthFraction   = (holder && holder->GetType() == ENTITY_TYPE_VEHICLE) ? 0.9f : 0.5f;
    rope->m_nFlags1           = 0;
    rope->m_pRopeHolder       = holder;
    rope->m_nType             = (eRopeType)ropeType;

    if (holder) {
        holder->RegisterReference(&rope->m_pRopeHolder);
    }

    // NOTE: This is an expiry timestamp, not a creation time!
    rope->m_nTime = bExpires ? CTimer::GetTimeInMS() + timeExpire : 0;

    // The rope's length depends on its type
    switch (rope->m_nType) {
    case eRopeType::MAGNET:
        rope->m_fRopeLength = 10.0f;
        break;
    case eRopeType::CRANE_MAGNO:
        rope->m_fRopeLength = 50.0f;
        break;
    case eRopeType::WRECKING_BALL:
    case eRopeType::QUARRY_CRANE_ARM:
    case eRopeType::CRANE_TROLLEY:
        rope->m_fRopeLength = 68.0f;
        break;
    default:
        rope->m_fRopeLength = 20.0f;
        break;
    }
    rope->m_fSegmentLength = rope->m_fRopeLength / (float)(NUM_ROPE_SEGMENTS - 1);

    // Lay out the segments
    if (rope->m_nType < eRopeType::CRANE_MAGNO || rope->m_nType > eRopeType::CRANE_TROLLEY) {
        // Zig-zag along the X axis
        for (auto i = 1u; i < NUM_ROPE_SEGMENTS; i++) {
            rope->m_aSegments[i].x = rope->m_aSegments[i - 1].x + ((i & 1) ? rope->m_fSegmentLength : -rope->m_fSegmentLength);
            rope->m_aSegments[i].y = rope->m_aSegments[i - 1].y;
            rope->m_aSegments[i].z = rope->m_aSegments[i - 1].z;
            rope->m_aSpeed[i]      = CVector{};
        }
    } else {
        // Hangs straight down (NOTE: The speeds aren't reset on this path - Not even in retail)
        for (auto i = 1u; i < NUM_ROPE_SEGMENTS; i++) {
            rope->m_aSegments[i].x = rope->m_aSegments[i - 1].x;
            rope->m_aSegments[i].y = rope->m_aSegments[i - 1].y;
            rope->m_aSegments[i].z = rope->m_aSegments[i - 1].z - rope->m_fSegmentLength;
        }
    }

    rope->CreateHookObjectForRope();
    return true;
}

// 0x556760
float CRopes::FindPickupHeight(CEntity* entity) {
    return CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel()->GetBoundingBox().m_vecMax.z;
}

// Returns id to array
// 0x556000
int32 CRopes::FindRope(uint32 id) {
    for (auto ropeId = 0; ropeId < MAX_NUM_ROPES; ropeId++) {
        if (aRopes[ropeId].m_nType != eRopeType::NONE && aRopes[ropeId].m_nId == id)
            return ropeId;
    }
    return -1;
}

// a4 always nullptr
// 0x555E40
bool CRopes::FindCoorsAlongRope(uint32 ropeId, float fDistAlongRope, CVector* outPosn, CVector* outSpeed) {
    const auto ropeIt = std::find_if(aRopes.begin(), aRopes.end(), [ropeId](const CRope& rope) {
        return rope.m_nType != eRopeType::NONE && rope.m_nId == ropeId;
    });
    if (ropeIt == aRopes.end()) {
        return false;
    }
    const auto& rope = *ropeIt;

    // NOTE: The distance along the rope is a normalized value, not a meter value
    const float dist = std::clamp(fDistAlongRope, 0.0f, 0.999f) * (float)(NUM_ROPE_SEGMENTS - 1);

    const auto  segmentIndex = (int32)dist;
    const float frac         = dist - (float)segmentIndex;

    const auto& seg0 = rope.m_aSegments[segmentIndex];
    const auto& seg1 = rope.m_aSegments[segmentIndex + 1];

    // Interpolate the position between the two segments the distance falls between
    outPosn->x = (1.0f - frac) * seg0.x + frac * seg1.x;
    outPosn->y = (1.0f - frac) * seg0.y + frac * seg1.y;
    outPosn->z = (1.0f - frac) * seg0.z + frac * seg1.z;

    // NOTE: The speed isn't interpolated, but taken from the lower segment
    if (outSpeed) {
        *outSpeed = rope.m_aSpeed[segmentIndex + 1];
    }
    return true;
}

// 0x558D10
int32 CRopes::CreateRopeForSwatPed(const CVector& startPos) {
    int32 newRopeId = m_nRopeIdCreationCounter + 100;
    if (RegisterRope(newRopeId, static_cast<uint32>(eRopeType::SWAT), startPos, true, 0, 0, nullptr, 4000)) {
        return -1;
    }

    m_nRopeIdCreationCounter += 1;
    return newRopeId;
}

// 0x555F80
bool CRopes::IsCarriedByRope(CPhysical* entity) {
    if (!entity)
        return false;

    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE && rope.m_pRopeAttachObject == entity)
            return true;
    }
    return false;
}

// 0x555DF0
void CRopes::SetSpeedOfTopNode(uint32 ropeId, CVector dirSpeed) {
    for (auto& rope : aRopes) {
        if (rope.m_nType != eRopeType::NONE && rope.m_nId == ropeId) {
            rope.m_aSpeed[0] = dirSpeed;
            return;
        }
    }
}
