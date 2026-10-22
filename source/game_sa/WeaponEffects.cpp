/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "WeaponEffects.h"

void CWeaponEffects::InjectHooks() {
    RH_ScopedClass(CWeaponEffects);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x742AB0);
    RH_ScopedInstall(Shutdown, 0x742B80);
    RH_ScopedInstall(IsLockedOn, 0x742BD0);
    RH_ScopedInstall(MarkTarget, 0x742BF0);
    RH_ScopedInstall(ClearCrossHair, 0x742C60);
    RH_ScopedInstall(ClearCrossHairs, 0x742C80);
    RH_ScopedInstall(ClearCrossHairImmediately, 0x742CA0);
    RH_ScopedInstall(ClearCrossHairsImmediately, 0x742CC0);
    RH_ScopedInstall(Render, 0x742CF0);
}

// 0x742AB0
void CWeaponEffects::Init() {
    for (auto& crossHair : gCrossHair) {
        crossHair.m_vecPosn = CVector();
        crossHair.m_bActive = false;
        crossHair.m_color = CRGBA(255, 0, 0, 127);
        crossHair.m_fSize = 1.0f;
        crossHair.field_1C = 0;
        crossHair.m_nTimeWhenToDeactivate = 0;
        crossHair.m_bClearImmediately = false;
        crossHair.m_fRotation = 0.0f;
    }

    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(CTxdStore::FindTxdSlot("particle"));
    gpCrossHairTex          = RwTextureRead("target256",  "target256m");
    gpCrossHairTexFlight[0] = RwTextureRead("lockon",     "lockonA");
    gpCrossHairTexFlight[1] = RwTextureRead("lockonFire", "lockonFireA");
    CTxdStore::PopCurrentTxd();
}

// 0x742B80
void CWeaponEffects::Shutdown() {
    for (auto& crossHair : gpCrossHairTexFlight) {
        RwTextureDestroy(crossHair);
        crossHair = nullptr;
    }

    RwTextureDestroy(gpCrossHairTex);
    gpCrossHairTex = nullptr;
}

// 0x742BD0
bool CWeaponEffects::IsLockedOn(CrossHairId id) {
    return gCrossHair[id].m_fRotation;
}

// 0x742BF0
void CWeaponEffects::MarkTarget(CrossHairId id, CVector posn, uint8 red, uint8 green, uint8 blue, uint8 alpha, float size, bool bClearImmediately) {
    auto& crossHair = gCrossHair[id];
    crossHair.m_vecPosn               = posn;
    crossHair.m_color.r               = red;
    crossHair.m_color.g               = green;
    crossHair.m_color.b               = blue;
    crossHair.m_color.a               = alpha;
    crossHair.m_bActive               = true;
    crossHair.m_fSize                 = size;
    crossHair.m_nTimeWhenToDeactivate = -1;
    crossHair.m_bClearImmediately     = bClearImmediately;
}

// 0x742C60
void CWeaponEffects::ClearCrossHair(CrossHairId id) {
    gCrossHair[id].m_nTimeWhenToDeactivate = static_cast<int32>(CTimer::GetTimeInMS() + 400);
}

// 0x742C80
void CWeaponEffects::ClearCrossHairs() {
    for (auto& crossHair : gCrossHair) {
        crossHair.m_bActive = false;
    }
}

// 0x742CA0
void CWeaponEffects::ClearCrossHairImmediately(CrossHairId id) {
    gCrossHair[id].m_nTimeWhenToDeactivate = static_cast<int32>(CTimer::GetTimeInMS() - 100);
    gCrossHair[id].m_bActive = false;
}

// 0x742CC0
void CWeaponEffects::ClearCrossHairsImmediately() {
    for (auto i = 0u; i < gCrossHair.size(); i++) {
        ClearCrossHair(i);
    }
}

//! Per-player crosshair trail state (See `DisplayCrossHairs`)
struct CrossHairTrail {
    uint32 m_nLastUpdateTime;    //!< `CTimer::m_snTimeInMilliseconds` at the time of the last update
    float  m_aPrevTargetY[4];    //!< The 4 previous `ms_aCrossHairTargetY` entries
};
VALIDATE_SIZE(CrossHairTrail, 0x14);

// TODO(re): The OG names of these three arrays (And of `CrossHairTrail`) aren't known.
static auto& ms_aCrossHairTrail   = StaticRef<std::array<CrossHairTrail, MAX_PLAYERS>>(0xB9B8F8);
static auto& ms_aCrossHairTargetY = StaticRef<std::array<std::array<float, 5>, MAX_PLAYERS>>(0xB9B920);
static auto& ms_aCrossHairTargetX = StaticRef<std::array<std::array<float, 5>, MAX_PLAYERS>>(0xB9B948);

// 0x56EF90
//! Draws the in-car ("drive by") crosshair as a fading spiral of sprites around the crosshair position
//! (`CPlayerInfo::m_vecCrosshairTarget`, in screen space), keeping the last 5 frames as a trail.
//! NOTE: In the OG this is `CPlayerInfo::DisplayCrossHairs` (Its `this` is `&CPlayerInfo::m_nCrosshairActivated`
//!       and its only stack argument the player index); it's kept here because `CWeaponEffects::Render` is its
//!       only caller.
static void DisplayCrossHairs(CPlayerInfo& info, int32 playerIdx) {
    if (!info.m_nCrosshairActivated) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVDESTALPHA));

    // 0x8CDF1C holds `CORONATYPE_STREAK` (8); it's never written by the exe
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(gpCoronaTexture[CORONATYPE_STREAK]->raster));

    //> Trail color (Green-ish for the 1st player, blue-ish for the 2nd one)
    uint32 red   = playerIdx != 0 ? 0x32 : 0x00;
    uint32 green = playerIdx != 0 ? 0x00 : 0xFF;
    uint32 blue  = playerIdx != 0 ? 0xFF : 0x32;

    auto& targetX = ms_aCrossHairTargetX[playerIdx];
    auto& targetY = ms_aCrossHairTargetY[playerIdx];
    auto& trail   = ms_aCrossHairTrail[playerIdx];

    //> Shift the history by one frame (The trail keeps the positions of the last 5 frames)
    for (auto k = 4; k > 0; k--) {
        trail.m_aPrevTargetY[k - 1] = targetY[k - 1];
        targetY[k]                  = targetY[k - 1];
        targetX[k]                  = targetX[k - 1];
    }
    targetX[0]              = info.m_vecCrosshairTarget.x;
    targetY[0]              = info.m_vecCrosshairTarget.y;
    trail.m_nLastUpdateTime = CTimer::GetTimeInMS();

    //> The sprites pulsate between 0.8 and 1.2
    const auto scale = std::sin((float)(trail.m_nLastUpdateTime & 0x3FF) * (2.f * PI / 1024.f)) * 0.2f + 1.f;

    //! NOTE: `i` isn't reset between trail entries (An OG quirk, mirrored as-is), so the sprites are only
    //!       spread out fully on the first entry.
    int32 i = 0;
    for (auto trailIdx = 0u; trailIdx < 5u; trailIdx++) {
        do {
            const auto radius = (float)i * 10.f + 20.f;
            for (auto j = 0; j < 4; j++) {
                const auto angle = (float)j * (PI / 2.f) + PI / 4.f; // 45°, 135°, 225°, 315°

                CSprite::RenderOneXLUSprite_Rotate_Aspect(
                    {
                        std::sin(angle) * radius * scale + (targetX[trailIdx] + 1.f) * SCREEN_WIDTH  * 0.5f,
                        std::cos(angle) * radius * scale + (targetY[trailIdx] + 1.f) * SCREEN_HEIGHT * 0.5f,
                        100.f
                    },
                    { 15.f, 15.f },
                    (uint8)red, (uint8)green, (uint8)blue,
                    255,   // intensity
                    0.01f, // rz
                    0.f,   // rotation
                    255    // alpha
                );
            }
        } while (++i < 3);

        red   >>= 1;
        green >>= 1;
        blue  >>= 1;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,  RWRSTATE(TRUE));
}

// 0x742CF0
void CWeaponEffects::Render() {
    ZoneScoped;

    // `TheCamera.m_bWideScreenOn` - The `CCamera` header's offsets are stale for this field, so read it directly
    static auto& s_bWideScreenOn  = StaticRef<bool>(0xB6F065);      // TheCamera + 0x3D
    static auto& s_fPulse         = StaticRef<float[2]>(0xC8A8A0);  // Per-crosshair pulse offset
    static auto& s_bPulseDir      = StaticRef<uint8[2]>(0x8D6144);  // Per-crosshair pulse grow/shrink direction
    static auto& s_fSpinX         = StaticRef<float>(0xC8A89C);     // Lock-on ring offset X
    static auto& s_fSpinY         = StaticRef<float>(0xC8A898);     // Lock-on ring offset Y
    static auto& s_nAimTargetTime = StaticRef<int32>(0xC8A890);     // When the 2P aim target last changed
    static auto& s_pAimTarget     = StaticRef<CEntity*>(0xC8A894);  // Last 2P aim target

    if (s_bWideScreenOn) {
        return;
    }

    const auto now = static_cast<int32>(CTimer::GetTimeInMS());

    for (auto i = 0u; i < gCrossHair.size(); i++) {
        auto& ch = gCrossHair[i];

        // Handle expiry
        if (ch.m_nTimeWhenToDeactivate != 0 && static_cast<uint32>(ch.m_nTimeWhenToDeactivate) < static_cast<uint32>(now)) {
            ch.m_bActive = false;
            ch.m_nTimeWhenToDeactivate = 0;
            s_fPulse[i] = 0.0f;
        }

        // -1 means "never expires"
        if (ch.m_nTimeWhenToDeactivate != -1) {
            s_fPulse[i] = 0.0f;
        }

        if (!CWorld::Players[i].m_pPed) {
            ch.m_bActive = false;
        }

        if (!ch.m_bActive) {
            continue;
        }

        if (ch.m_bClearImmediately) { // Lock-on/heat-seeker crosshair
            const CVector posn = ch.m_vecPosn;
            CVector scr;
            float w, h;

            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
            RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
            RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(gpCrossHairTexFlight[0]));

            if (CSprite::CalcScreenCoors(posn, &scr, &w, &h, true, true)) {
                if (20.0f / h > 1.0f) {
                    w *= 20.0f / h;
                    h *= 20.0f / h;
                }

                constexpr std::array<float, 2> kAspect{ 0.95f, 1.05f };
                for (const auto aspect : kAspect) {
                    CSprite::RenderOneXLUSprite_Rotate_Aspect(
                        scr,
                        { std::min(w, 28.0f) * aspect * 1.8f, std::min(h, 20.0f) * aspect * 1.8f },
                        0, 0, 0, 255, 0.01f, 0.0f, 255
                    );
                }
                CSprite::RenderOneXLUSprite_Rotate_Aspect(
                    scr,
                    { std::min(w, 28.0f) * 1.8f, std::min(h, 20.0f) * 1.8f },
                    ch.m_color.r, ch.m_color.g, ch.m_color.b, 255, 0.01f, 0.0f, ch.m_color.a
                );

                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(gpCrossHairTexFlight[1]));

                const float ang = static_cast<float>(now & 0x3FF) * 0.0061359233f; // 2*pi/1024
                ch.field_20 -= 20.0f * ang;
                if (ch.field_20 < 0.0f) {
                    ch.field_20 = 0.0f;
                }
                ch.m_fRotation = ch.field_20 == 0.0f
                    ? 1.0f
                    : 0.0f;
                s_fSpinX = std::sin(ang) * ch.field_20;
                s_fSpinY = std::cos(ang) * ch.field_20;

                const CVector scr2{ scr.x - s_fSpinX, scr.y - s_fSpinY, scr.z };
                for (const auto aspect : kAspect) {
                    CSprite::RenderOneXLUSprite_Rotate_Aspect(
                        scr2,
                        { std::min(w, 28.0f) * aspect * 0.8f, std::min(h, 20.0f) * aspect * 0.8f },
                        0, 0, 0, 255, 0.01f, ang, 255
                    );
                }
                CSprite::RenderOneXLUSprite_Rotate_Aspect(
                    scr2,
                    { std::min(w, 28.0f) * 0.8f, std::min(h, 20.0f) * 0.8f },
                    ch.m_color.r, ch.m_color.g, ch.m_color.b, 255, 0.01f, ang, ch.m_color.a
                );
            }

            // NOTE: Only ZWrite/ZTest are restored here, unlike in the aiming branch below
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE,  RWRSTATE(TRUE));
        } else { // Normal aiming crosshair
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(NULL));

            CVector scr;
            float w, h;
            CSprite::CalcScreenCoors(ch.m_vecPosn, &scr, &w, &h, false, false);

            if (scr.x > SCREEN_WIDTH || scr.x < 0.0f || scr.y > SCREEN_HEIGHT || scr.y < 0.0f) {
                if (CGameLogic::IsCoopGameGoingOn()) { // Off-screen: Draw an arrow pointing towards the target
                    const auto& camMatrix = TheCamera.m_mCameraMatrix;
                    const auto& camPos    = camMatrix.GetPosition();
                    const auto& camRight  = camMatrix.GetRight();
                    const auto& camAt     = camMatrix.GetForward();

                    const CVector p = ch.m_vecPosn;
                    const CVector q = CWorld::Players[i].m_pPed->GetPosition();

                    const float a1 = std::atan2((p - camPos).Dot(camAt), (p - camPos).Dot(camRight));
                    const float a2 = std::atan2((p - q).Dot(camAt), (p - q).Dot(camRight));

                    const float s1 = std::sin(a1);
                    const float c1 = std::cos(a1);
                    const float s2 = std::sin(a2);
                    const float c2 = std::cos(a2);
                    constexpr float kArrowSize = 15.0f;

                    const CVector2D base{
                        (1.0f - c1) * SCREEN_WIDTH * 0.5f,
                        (1.0f - s1) * SCREEN_HEIGHT * 0.5f
                    };
                    const std::array<CVector2D, 3> pts{
                        base,
                        CVector2D{ (s2 * 0.5f + c2) * kArrowSize + base.x, kArrowSize * s2 + base.y - c2 * kArrowSize * 0.5f },
                        CVector2D{ c2 * kArrowSize + base.x - kArrowSize * s2 * 0.5f, (c2 * 0.5f + s2) * kArrowSize + base.y }
                    };
                    CSprite::RenderOneXLUSprite_Triangle(pts[0], pts[1], pts[2], 2.5f, 0, 0, 0, 255, 1.0f, 255);

                    const CVector2D base2{ base.x + c2 * kArrowSize * 0.1f, base.y + s2 * kArrowSize * 0.1f };
                    const auto shrink = [&](const CVector2D& pt) {
                        return CVector2D{ (pt.x - base2.x) * 0.8f + base2.x, (pt.y - base2.y) * 0.8f + base2.y };
                    };
                    CSprite::RenderOneXLUSprite_Triangle(
                        shrink(pts[0]), shrink(pts[1]), shrink(pts[2]), 2.5f,
                        ch.m_color.r, ch.m_color.g, ch.m_color.b, 255, 1.0f, 255
                    );
                }
            } else { // On-screen: Spinning three-arm reticle
                ch.m_fSize = std::max(std::min(ch.m_fSize, 1.2f), 0.3f);

                float inner = 5.0f * ch.m_fSize;
                float outer = 25.0f * ch.m_fSize;
                if (scr.z < 2.5f) {
                    scr.z = 2.5f;
                }

                const float radius = CWorld::Players[i].m_pPed->GetWeaponRadiusOnScreen();
                if (radius > 0.0f) {
                    inner = radius * 30.0f;
                    outer = ch.m_fSize * 20.0f + inner;
                }

                RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(rwBLENDZERO));
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDZERO));

                for (auto arm = 0; arm < 3; arm++) {
                    const float deg = static_cast<float>(arm) * 120.0f;
                    const float a0  = deg * 0.017453292f + ch.field_1C;
                    const float aL  = (deg + 15.0f) * 0.017453292f + ch.field_1C;
                    const float aR  = (deg - 15.0f) * 0.017453292f + ch.field_1C;

                    const float r0 = inner + s_fPulse[i];
                    const float r1 = outer + s_fPulse[i];

                    CVector2D p0{ scr.x - std::sin(a0) * r0, scr.y - std::cos(a0) * r0 };
                    CVector2D p1{ scr.x - std::sin(aL) * r1, scr.y - std::cos(aL) * r1 };
                    CVector2D p2{ scr.x - std::sin(aR) * r1, scr.y - std::cos(aR) * r1 };

                    // Black backing triangle
                    CSprite::RenderOneXLUSprite_Triangle(p0, p1, p2, scr.z, 0, 0, 0, 255, 1.0f, ch.m_color.a);

                    // Shrink each corner 0.75 towards the centroid
                    const float cx = (p2.x + p1.x + p0.x) * 0.33333334f;
                    const float cy = (p2.y + p1.y + p0.y) * 0.33333334f;
                    p0 = { (p0.x - cx) * 0.75f + cx, (p0.y - cy) * 0.75f + cy };
                    p1 = { (p1.x - cx) * 0.75f + cx, (p1.y - cy) * 0.75f + cy };
                    p2 = { (p2.x - cx) * 0.75f + cx, (p2.y - cy) * 0.75f + cy };

                    RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(rwBLENDSRCALPHA));
                    RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
                    RwRenderStateSet(rwRENDERSTATECULLMODE,  RWRSTATE(rwCULLMODECULLNONE));
                    CSprite::RenderOneXLUSprite_Triangle(
                        p0, p1, p2, scr.z,
                        ch.m_color.r, ch.m_color.g, ch.m_color.b, 255, 1.0f, ch.m_color.a
                    );
                }

                if (ch.m_nTimeWhenToDeactivate == 0 || ch.m_nTimeWhenToDeactivate == -1) {
                    ch.field_1C += 0.05f;
                } else {
                    ch.field_1C += 0.75f;
                    s_fPulse[i]  += 2.0f;
                    ch.m_fSize   *= 0.9f;
                }
                if (ch.field_1C > 6.2831855f) {
                    ch.field_1C = 0.0f;
                }

                if (ch.m_nTimeWhenToDeactivate == 0) { // Only the "manual" state
                    if (!s_bPulseDir[i]) {
                        s_fPulse[i] -= 2.0f;
                        if (s_fPulse[i] < 0.0f) {
                            s_bPulseDir[i] = true;
                        }
                    } else {
                        s_fPulse[i] += ch.m_fSize * 2.0f;
                        if (s_fPulse[i] > ch.m_fSize * 20.0f) {
                            s_bPulseDir[i] = false;
                        }
                    }
                }
            }

            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(FALSE));
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(TRUE));
        }
    }

    // Two-player "in car and shooting" marker
    const auto& cam = TheCamera.GetActiveCamera();
    if (cam.m_nMode == MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
        RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
        RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(gpCrossHairTex));

        const float ang = static_cast<float>(now & 0x3FF) * 0.0061359233f; // 2*pi/1024

        // Fixed reticle at the camera's normalised targeting position
        CSprite::RenderOneXLUSprite_Rotate_Aspect(
            {
                (cam.m_fX_Targetting + 1.0f) * SCREEN_WIDTH  * 0.5f,
                (cam.m_fY_Targetting + 1.0f) * SCREEN_HEIGHT * 0.5f,
                100.0f
            },
            { 10.0f, 10.0f }, 255, 128, 0, 128, 0.01f, ang, 255
        );

        auto* ped = CWorld::Players[1].m_pPed;
        if (CWorld::Players[0].m_pPed
            && CWorld::Players[0].m_pPed->m_pVehicle
            && CWorld::Players[0].m_pPed->m_pVehicle->m_pDriver != CWorld::Players[0].m_pPed
        ) {
            ped = CWorld::Players[0].m_pPed; // The passenger is the shooter
        }

        CEntity* target = nullptr;
        if (ped) {
            const auto* const weaponInfo = CWeaponInfo::GetWeaponInfo(
                ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_Type,
                ped->GetWeaponSkill()
            );
            target = CWeapon::FindNearestTargetEntityWithScreenCoors(
                cam.m_fX_Targetting,
                cam.m_fY_Targetting,
                2.0f * weaponInfo->m_fWeaponRange,
                ped->GetPosition()
            );
        }

        if (target != s_pAimTarget) {
            s_nAimTargetTime = now;
            s_pAimTarget     = target;
        }

        if (target) {
            CVector scr;
            float w, h;
            if (CSprite::CalcScreenCoors(target->GetPosition(), &scr, &w, &h, true, true)) {
                if (20.0f / h > 1.0f) {
                    w *= 20.0f / h;
                    h *= 20.0f / h;
                }
                const int32 elapsed = now - s_nAimTargetTime;
                const float scale   = std::max(3.0f - static_cast<float>(elapsed) * 0.001953125f, 1.0f); // 1/512 per ms

                int32 alpha = (elapsed + ((elapsed >> 31) & 3)) >> 2; // elapsed / 4 (rounded towards zero)
                alpha       = std::min(alpha + 0x46, 255);

                CSprite::RenderOneXLUSprite_Rotate_Aspect(
                    scr,
                    { w * scale, h * scale },
                    static_cast<uint8>(alpha), 0, 0, static_cast<int16>(alpha),
                    0.01f, ang, 255
                );
            }
        }

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE,  RWRSTATE(TRUE));
    }

    for (auto i = 0; i < MAX_PLAYERS; i++) {
        DisplayCrossHairs(CWorld::Players[i], i); // 0x56EF90
    }
}
