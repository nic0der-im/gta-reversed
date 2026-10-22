/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"
#include <imgui.h>
#include <extensions/utility.hpp>
#include "Shadows.h"
#include "Collision/ColHelpers.h"
#include "FireManager.h"
#include <CustomBuildingDNPipeline.h>

void CShadows::InjectHooks() {
    RH_ScopedClass(CShadows);
    RH_ScopedCategory("Shadows");

    RH_ScopedInstall(Init, 0x706CD0);
    RH_ScopedInstall(Shutdown, 0x706ED0);
    RH_ScopedInstall(TidyUpShadows, 0x707770);
    RH_ScopedInstall(AddPermanentShadow, 0x706F60);
    RH_ScopedInstall(UpdatePermanentShadows, 0x70C950);
    RH_ScopedOverloadedInstall(StoreShadowToBeRendered, "Texture", 0x707390, void(*)(uint8, RwTexture*, const CVector&, float, float, float, float, int16, uint8, uint8, uint8, float, bool, float, CRealTimeShadow*, bool));
    RH_ScopedOverloadedInstall(StoreShadowToBeRendered, "Type", 0x707930, void(*)(uint8, const CVector&, float, float, float, float, int16, uint8, uint8, uint8));
    RH_ScopedInstall(SetRenderModeForShadowType, 0x707460);
    RH_ScopedInstall(RemoveOilInArea, 0x7074F0);
    RH_ScopedInstall(GunShotSetsOilOnFire, 0x707550);
    RH_ScopedInstall(PrintDebugPoly, 0x7076B0);
    RH_ScopedInstall(CalcPedShadowValues, 0x7076C0);
    RH_ScopedInstall(AffectColourWithLighting, 0x707850);
    RH_ScopedInstall(StoreShadowForPedObject, 0x707B40);
    RH_ScopedInstall(StoreRealTimeShadow, 0x707CA0);
    RH_ScopedInstall(UpdateStaticShadows, 0x707F40);
    RH_ScopedInstall(RenderExtraPlayerShadows, 0x707FA0);
    RH_ScopedInstall(RenderStaticShadows, 0x708300);
    RH_ScopedInstall(CastShadowEntityXY, 0x7086B0);
    RH_ScopedInstall(CastShadowEntityXYZ, 0x70A040);
    RH_ScopedInstall(CastPlayerShadowSectorList<CPtrListSingleLink<CPhysical*>>, 0x70A470);
    RH_ScopedInstall(CastShadowSectorList<CPtrListSingleLink<CPhysical*>>, 0x70A630);
    RH_ScopedInstall(CastRealTimeShadowSectorList<CPtrListSingleLink<CPhysical*>>, 0x70A7E0);
    RH_ScopedInstall(RenderStoredShadows, 0x70A960);
    RH_ScopedInstall(GeneratePolysForStaticShadow, 0x70B730);
    RH_ScopedInstall(StoreStaticShadow, 0x70BA00);
    RH_ScopedInstall(StoreShadowForVehicle, 0x70BDA0);
    RH_ScopedInstall(StoreCarLightShadow, 0x70C500);
    RH_ScopedInstall(StoreShadowForPole, 0x70C750);
    RH_ScopedInstall(RenderIndicatorShadow, 0x70CCB0);
    RH_ScopedGlobalInstall(ShadowRenderTriangleCB, 0x709CF0);
}

void CStaticShadow::InjectHooks() {
    RH_ScopedClass(CStaticShadow);
    RH_ScopedCategory("Shadows");

    RH_ScopedInstall(Free, 0x707670);
    //RH_ScopedInstall(Init, 0x0);
}

// 0x707670 
void CStaticShadow::Free() {
    if (m_pPolyBunch) {
        const auto prevHead = CShadows::pEmptyBunchList;
        CShadows::pEmptyBunchList = m_pPolyBunch;

        // Find last in the list and make it point to the previous head
        auto it{ m_pPolyBunch };
        while (it->m_pNext) {
            it = static_cast<CPolyBunch*>(it->m_pNext);
        }
        it->m_pNext = prevHead;

        m_pPolyBunch = nullptr;
        m_nId = 0;
    }
}

// 0x706CD0
void CShadows::Init() {
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(CTxdStore::FindTxdSlot("particle"));

    gpShadowCarTex         = RwTextureRead("shad_car",     nullptr);
    gpShadowPedTex         = RwTextureRead("shad_ped",     nullptr);
    gpShadowHeliTex        = RwTextureRead("shad_heli",    nullptr);
    gpShadowBikeTex        = RwTextureRead("shad_bike",    nullptr);
    gpShadowBaronTex       = RwTextureRead("shad_rcbaron", nullptr);
    gpShadowExplosionTex   = RwTextureRead("shad_exp",     nullptr);
    gpShadowHeadLightsTex  = RwTextureRead("headlight",    nullptr);
    gpShadowHeadLightsTex2 = RwTextureRead("headlight1",   nullptr);
    gpBloodPoolTex         = RwTextureRead("bloodpool_64", nullptr);
    gpHandManTex           = RwTextureRead("handman",      nullptr);
    gpCrackedGlassTex      = RwTextureRead("wincrack_32",  nullptr);
    gpPostShadowTex        = RwTextureRead("lamp_shad_64", nullptr);

    CTxdStore::PopCurrentTxd();

    g_ShadowVertices = { 0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 5, 4, 0, 6, 5, 0, 7, 6, 0, 8, 7, 0, 9, 8 };

    std::ranges::for_each(aStaticShadows, [&](auto& shadow) { shadow.Init(); });

    pEmptyBunchList = aPolyBunches.data();

    for (auto i = 0u; i < aPolyBunches.size() - 1; i++) {
        aPolyBunches[i].m_pNext = &aPolyBunches[i + 1];
    }
    aPolyBunches.back().m_pNext = nullptr;

    std::ranges::for_each(aPermanentShadows, [&](auto& shadow) { shadow.Init(); });
}

// 0x706ED0
void CShadows::Shutdown() {
    RwTextureDestroy(gpShadowCarTex);
    RwTextureDestroy(gpShadowPedTex);
    RwTextureDestroy(gpShadowHeliTex);
    RwTextureDestroy(gpShadowBikeTex);
    RwTextureDestroy(gpShadowBaronTex);
    RwTextureDestroy(gpShadowExplosionTex);
    RwTextureDestroy(gpShadowHeadLightsTex);
    RwTextureDestroy(gpShadowHeadLightsTex2);
    RwTextureDestroy(gpBloodPoolTex);
    RwTextureDestroy(gpHandManTex);
    RwTextureDestroy(gpCrackedGlassTex);
    RwTextureDestroy(gpPostShadowTex);
}

// 0x707770
void CShadows::TidyUpShadows() {
    std::ranges::for_each(
        aPermanentShadows,
        [&](auto& shadow) { shadow.m_nType = SHADOW_NONE; }
    );
}

// 0x706F60
void CShadows::AddPermanentShadow(uint8 type, RwTexture* texture, CVector* posn, float topX, float topY, float rightX, float rightY, int16 intensity, uint8 red, uint8 greeb, uint8 blue, float drawDistance, uint32 time, float upDistance) {
    // Find the first unused one
    int32 slot = MAX_PERMANENT_SHADOWS;
    for (int32 i = 0; i < MAX_PERMANENT_SHADOWS; i++) {
        if (aPermanentShadows[i].m_nType == SHADOW_NONE) {
            slot = i;
            break;
        }
    }

    // Otherwise, find the oldest small one
    if (slot == MAX_PERMANENT_SHADOWS) {
        uint32 oldest = 0xFFFFFFFF;
        for (int32 i = 0; i < MAX_PERMANENT_SHADOWS; i++) {
            const auto& shdw = aPermanentShadows[i];
            if (shdw.m_fFrontX * shdw.m_fFrontX + shdw.m_fFrontY * shdw.m_fFrontY < 0.25f
             && shdw.m_fSideX * shdw.m_fSideX + shdw.m_fSideY * shdw.m_fSideY < 0.25f
             && shdw.m_nTimeCreated < oldest
            ) {
                slot   = i;
                oldest = shdw.m_nTimeCreated;
            }
        }
    }

    if (slot == MAX_PERMANENT_SHADOWS) {
        return;
    }

    auto& shdw = aPermanentShadows[slot];

    shdw.m_vecPosn       = *posn;
    shdw.m_fFrontX       = topX;
    shdw.m_fFrontY       = topY;
    shdw.m_fSideX        = rightX;
    shdw.m_fSideY        = rightY;
    shdw.m_fZDistance    = drawDistance;
    shdw.m_fScale        = upDistance;
    shdw.m_nTimeCreated  = CTimer::GetTimeInMS();
    shdw.m_nTimeDuration = time;
    shdw.m_pTexture      = texture;
    shdw.m_nIntensity    = intensity;
    shdw.m_nType         = (eShadowType)type;
    shdw.m_nRed          = red;
    shdw.m_nGreen        = greeb;
    shdw.m_nBlue         = blue;
}

// 0x70C950
void CShadows::UpdatePermanentShadows() {
    ZoneScoped;

    // Store the still alive ones as static shadows (Fading them out during their last quarter)
    for (auto& shdw : aPermanentShadows) {
        if (shdw.m_nType == SHADOW_NONE) {
            continue;
        }

        const auto elapsed = CTimer::GetTimeInMS() - shdw.m_nTimeCreated;
        if (elapsed < shdw.m_nTimeDuration) {
            auto intensity = shdw.m_nIntensity;
            auto red       = shdw.m_nRed;
            auto green     = shdw.m_nGreen;
            auto blue      = shdw.m_nBlue;

            const auto fadeStart = shdw.m_nTimeDuration * 3 / 4;
            if (elapsed >= fadeStart) {
                const auto mult = 1.f - (float)(elapsed - fadeStart) / (float)(shdw.m_nTimeDuration / 4);
                intensity = (int16)(mult * (float)intensity);
                red       = (uint8)(mult * (float)red);
                green     = (uint8)(mult * (float)green);
                blue      = (uint8)(mult * (float)blue);
            }

            const auto stored = StoreStaticShadow(
                notsa::PtrToId(&shdw),
                shdw.m_nType,
                shdw.m_pTexture,
                shdw.m_vecPosn,
                shdw.m_fFrontX, shdw.m_fFrontY,
                shdw.m_fSideX,  shdw.m_fSideY,
                intensity,
                red, green, blue,
                shdw.m_fZDistance,
                1.f,
                40.f,
                false,
                0.f
            );

            if (stored || shdw.m_nType == SHADOW_OIL_5) {
                continue;
            }
        }

        shdw.m_nType = SHADOW_NONE;
    }

    // Spread the fires between the oil shadows
    if ((CTimer::GetFrameCounter() & 3u) != 0u) {
        return;
    }

    for (auto& oshdw : aPermanentShadows) {
        if (oshdw.m_nType != SHADOW_OIL_2) {
            continue;
        }

        oshdw.m_nType         = SHADOW_OIL_3;
        oshdw.m_nTimeCreated  = CTimer::GetTimeInMS();
        oshdw.m_nTimeDuration = 2000;

        // Find the closest oil spot to ignite
        CPermanentShadow* nearest{};
        auto              nearestDist = 3.f;
        for (auto& ishdw : aPermanentShadows) {
            if (ishdw.m_nType != SHADOW_OIL_1 && ishdw.m_nType != SHADOW_OIL_5) {
                continue;
            }

            const auto dist = (ishdw.m_vecPosn - oshdw.m_vecPosn).Magnitude();
            if (dist < nearestDist) {
                nearest     = &ishdw;
                nearestDist = dist;
            }
        }

        if (!nearest) {
            continue;
        }

        nearest->m_nType = SHADOW_OIL_4;
        gFireManager.StartFire(nearest->m_vecPosn, 1.8f, 0, nullptr, 2000u, 0, 1);

        // And the closest one behind this shadow (So the fire spreads in the opposite direction)
        const auto ignitedDir2D = CVector2D{ nearest->m_vecPosn - oshdw.m_vecPosn };
        nearest     = nullptr;
        nearestDist = 3.f;
        for (auto& ishdw : aPermanentShadows) {
            if (ishdw.m_nType != SHADOW_OIL_1 && ishdw.m_nType != SHADOW_OIL_5) {
                continue;
            }

            const auto toShdw = ishdw.m_vecPosn - oshdw.m_vecPosn;
            const auto dist   = toShdw.Magnitude();
            if (dist < nearestDist && CVector2D{ toShdw }.Dot(ignitedDir2D) < 0.f) {
                nearest     = &ishdw;
                nearestDist = dist;
            }
        }

        if (nearest) {
            nearest->m_nType = SHADOW_OIL_4;
            gFireManager.StartFire(nearest->m_vecPosn, 1.8f, 0, nullptr, 2000u, 0, 1);
        }
    }

    // The already burnt ones go back to being just "oil on fire"
    for (auto& shdw : aPermanentShadows) {
        if (shdw.m_nType == SHADOW_OIL_4) {
            shdw.m_nType = SHADOW_OIL_2;
        }
    }
}

// 0x707930
void CShadows::StoreShadowToBeRendered(uint8 type, const CVector& posn, float frontX, float frontY, float sideX, float sideY, int16 intensity, uint8 red, uint8 green, uint8 blue) {
    const auto Store = [=](auto mtype, auto texture) {
        StoreShadowToBeRendered(mtype, texture, posn, frontX, frontY, sideX, sideY, intensity, red, green, blue, 15.0f, 0, 1.0f, nullptr, 0);
    };

    switch (type) {
    case SHADOW_DEFAULT:
        Store(SHADOW_TEX_CAR, gpShadowCarTex);
        break;
    case SHADOW_ADDITIVE:
        Store(SHADOW_TEX_CAR, gpShadowPedTex);
        break;
    case SHADOW_INVCOLOR:
        Store(SHADOW_TEX_PED, gpShadowExplosionTex);
        break;
    case SHADOW_OIL_1:
        Store(SHADOW_TEX_CAR, gpShadowHeliTex);
        break;
    case SHADOW_OIL_2:
        Store(SHADOW_TEX_PED, gpShadowHeadLightsTex);
        break;
    case SHADOW_OIL_3:
        Store(SHADOW_TEX_CAR, gpBloodPoolTex);
        break;
    default:
        return;
    }
}

// 0x707390
void CShadows::StoreShadowToBeRendered(uint8 type, RwTexture* texture, const CVector& posn, float topX, float topY, float rightX, float rightY, int16 intensity, uint8 red, uint8 green, uint8 blue, float zDistance, bool drawOnWater, float scale, CRealTimeShadow* realTimeShadow, bool drawOnBuildings) {
    if (ShadowsStoredToBeRendered >= asShadowsStored.size())
        return;

    auto& shadow = asShadowsStored[ShadowsStoredToBeRendered];

    shadow.m_nType      = (eShadowType)type;
    shadow.m_pTexture   = texture;
    shadow.m_vecPosn    = posn;
    shadow.m_Front.x    = topX;
    shadow.m_Front.y    = topY;
    shadow.m_Side.x     = rightX;
    shadow.m_Side.y     = rightY;
    shadow.m_nIntensity = intensity;
    shadow.m_nRed       = red;
    shadow.m_nGreen     = green;
    shadow.m_nBlue      = blue;
    shadow.m_fZDistance = zDistance;
    shadow.m_bDrawOnWater     = drawOnWater;
    shadow.m_bDrawOnBuildings = drawOnBuildings;
    shadow.m_fScale     = scale;
    shadow.m_pRTShadow  = realTimeShadow;

    ShadowsStoredToBeRendered++;
}

void CShadows::StoreShadowToBeRendered(eShadowType type, RwTexture* tex, const CVector& posn, CVector2D top, CVector2D right, int16 intensity, uint8 red, uint8 green, uint8 blue, float zDistance, bool drawOnWater, float scale, CRealTimeShadow* realTimeShadow, bool drawOnBuildings) {
    StoreShadowToBeRendered(
        type,
        tex,
        posn,
        top.x, top.y,
        right.x, right.y,
        intensity,
        red, green, blue,
        zDistance,
        drawOnWater,
        scale,
        realTimeShadow,
        drawOnBuildings
    );
}

// 0x707460
void CShadows::SetRenderModeForShadowType(eShadowType type) {
    switch (type) {
    case SHADOW_DEFAULT:
    case SHADOW_OIL_1:
    case SHADOW_OIL_2:
    case SHADOW_OIL_3:
    /* case SHADOW_OIL_4: */ // missing
    case SHADOW_OIL_5:
        RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(rwBLENDSRCALPHA));
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
        break;
    case SHADOW_ADDITIVE:
        RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(rwBLENDONE));
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDONE));
        break;
    case SHADOW_INVCOLOR:
        RwRenderStateSet(rwRENDERSTATESRCBLEND,  RWRSTATE(rwBLENDZERO));
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCCOLOR));
        break;
    default:
        return;
    }
}

// 0x7074F0
void CShadows::RemoveOilInArea(float minX, float maxX, float minY, float maxY) {
    CRect rect{ {minX, minY}, {maxX, maxY} };
    for (auto& shadow : aPermanentShadows) {
        switch (shadow.m_nType) {
        case SHADOW_OIL_1:
        case SHADOW_OIL_5:
            break;
        default:
            continue;
        }
        if (rect.IsPointInside(shadow.m_vecPosn)) {
            shadow.m_nType = SHADOW_NONE;
        }
    }
}

// 0x707550
void CShadows::GunShotSetsOilOnFire(const CVector& shotOrigin, const CVector& shotTarget) {
    const auto V3DChangeZ = [] (const CVector& pos) {
        return CVector{ pos.x, pos.y, pos.z * 0.27f };
    };

    const CColLine line{V3DChangeZ(shotOrigin), V3DChangeZ(shotTarget)};

    // Find closest shadow (but not further than 1 unit)
    CPermanentShadow* closest{};
    auto              closestDist{1.f};
    for (auto& shdw : aPermanentShadows) {
        switch (shdw.m_nType) {
        case SHADOW_OIL_1:
        case SHADOW_OIL_5:
            break;
        default:
            continue;
        }

        const auto dist = line.DistTo(V3DChangeZ(shdw.m_vecPosn));
        if (dist < closestDist) {
            closestDist = dist;
            closest     = &shdw;
        }
    }

    if (closest) {
        closest->m_nType = SHADOW_OIL_2;
        gFireManager.StartFire(closest->m_vecPosn, 1.8f, 0, nullptr, 2000u, 0, 1);
    }
}

// 0x7076B0
void CShadows::PrintDebugPoly(CVector* a, CVector* b, CVector* c) {
    // NOP
}

// 0x7076C0
void CShadows::CalcPedShadowValues(
    CVector sunPosn,
    float& frontX,        float& frontY,
    float& sideX,         float& sideY,
    float& displacementX, float& displacementY
) {
    const auto sunDist = sunPosn.Magnitude2D();
    const auto recip = 1.0f / sunDist;

    const auto mult = (sunDist + 1.0f) * recip;
    frontX = -sunPosn.x * mult / 2.0f;
    frontY = -sunPosn.y * mult / 2.0f;

    sideX = -sunPosn.y * recip / 2.0f;
    sideY = +sunPosn.x * recip / 2.0f;

    displacementX = -sunPosn.x / 2.0f;
    displacementY = -sunPosn.y / 2.0f;
}

// 0x707850
void CShadows::AffectColourWithLighting(
    eShadowType shadowType,
    uint8 dayNightIntensity, // packed 2x4 bits for day/night
    uint8 r, uint8 g, uint8 b,
    uint8& outR, uint8& outG, uint8& outB
) {
    if (shadowType != SHADOW_ADDITIVE) {
        const auto mult = std::min(
            0.4f + 0.6f * (1.f - CCustomBuildingDNPipeline::m_fDNBalanceParam),
            0.3f + 0.7f * lerp(
                (float)(dayNightIntensity >> 0 & 0b1111) / 30.f,
                (float)(dayNightIntensity >> 4 & 0b1111) / 30.f,
                CCustomBuildingDNPipeline::m_fDNBalanceParam
            )
        );
        outR = (uint8)((float)r * mult);
        outG = (uint8)((float)g * mult);
        outB = (uint8)((float)b * mult);
    } else {
        outR = r;
        outG = g;
        outB = b;
    }
}

// NOTSA
uint16 CalculateShadowStrength(float currDist, float maxDist, uint16 maxStrength) {
    assert(maxDist >= currDist); // Otherwise integer underflow will occur

    const auto halfMaxDist = maxDist / 2.f;
    if (currDist >= halfMaxDist) { // Anything further than half the distance is faded out
        return (uint16)((1.f - (currDist - halfMaxDist) / halfMaxDist) * maxStrength);
    } else { // Anything closer than half the max distance is full strength
        return (uint16)maxStrength;
    }
}

// 0x707B40
void CShadows::StoreShadowForPedObject(CPed* ped, float displacementX, float displacementY, float frontX, float frontY, float sideX, float sideY) {
    // Okay, so.
    // This function is called from `CCutsceneObject::PreRender`
    // And you might ask "what the fuck, an object is not a ped!!"
    // well, in R* world it is.
    // it has bones (as anything can have bones actually, that's a known fact)
    // the `GetBonePosition` below *just works* because it doesn't access any
    // `CPed` specific member variables.
    // But we really should fix this in a sensible way in the future.
    assert(ped->GetIsTypePed() || ped->GetIsTypeObject());

    const auto  bonePos           = ped->GetBonePosition(BONE_ROOT);
    const auto& camPos            = TheCamera.GetPosition();
    const auto  boneToCamDist2DSq = (bonePos - camPos).SquaredMagnitude2D();

    // Check if ped is close enough
    if (boneToCamDist2DSq >= MAX_DISTANCE_PED_SHADOWS_SQR) {
        return;
    }

    const auto isPlayerPed = FindPlayerPed() == ped;

    // Check if ped is visible to the camera
    if (!isPlayerPed) {  // Optimization: Assume player ped is always visible
        if (!TheCamera.IsSphereVisible(ped->GetPosition(), 2.f)) {
            return;
        }
    }

    // Now store a shadow to be rendered
    const auto strength = (uint8)CalculateShadowStrength(std::sqrt(boneToCamDist2DSq), MAX_DISTANCE_PED_SHADOWS, CTimeCycle::m_CurrentColours.m_nShadowStrength);
    StoreShadowToBeRendered(
        SHADOW_DEFAULT,
        gpShadowPedTex,
        bonePos + CVector{displacementX, displacementY, 0.f},
        frontX, frontY,
        sideX, sideY,
        strength,
        strength, strength, strength,
        4.f,
        false,
        1.f,
        nullptr,
        isPlayerPed || g_fx.GetFxQuality() >= FX_QUALITY_VERY_HIGH // NOTSA: At higher FX quality draw all ped's shadows
    );
}

// 0x707CA0
void CShadows::StoreRealTimeShadow(CPhysical* physical, float displacementX, float displacementY, float frontX, float frontY, float sideX, float sideY) {
    const auto rtshdw = physical->m_pShadowData;
    if (!rtshdw) {
        return;
    }
    const auto& camPos = TheCamera.GetPosition();
    const auto  shdwPos = physical->GetIsTypePed()
        ? physical->AsPed()->GetBonePosition(BONE_ROOT)
        : physical->GetPosition();
    const auto shdwToCamDist2DSq = (shdwPos - camPos).SquaredMagnitude2D();

    // Check distance to camera
    if (shdwToCamDist2DSq > MAX_DISTANCE_PED_SHADOWS_SQR) {
        return;
    }

    // Check if the object is visible to the camera
    if (FindPlayerPed() != physical) {  // Optimization: Assume player ped is always visible
        if (!TheCamera.IsSphereVisible(shdwPos, 2.f)) {
            return;
        }
    }

    const auto strength = (float)CalculateShadowStrength(std::sqrt(shdwToCamDist2DSq), MAX_DISTANCE_PED_SHADOWS, CTimeCycle::m_CurrentColours.m_nShadowStrength);
    const auto cc       = (uint8)((float)rtshdw->m_nIntensity / 100.f * strength);

    const auto& vecToSun = CTimeCycle::m_VectorToSun[CTimeCycle::m_CurrentStoredValue];
    const auto lightFrame = rtshdw->SetLightProperties(
        RWRAD2DEG(+std::atan2(-vecToSun.x, -vecToSun.y)),
        RWRAD2DEG(-std::atan2(+vecToSun.x, -vecToSun.z)),
        true
    );
    CalcPedShadowValues(
        *RwMatrixGetAt(RwFrameGetMatrix(lightFrame)),
        displacementX, displacementY,
        frontX, frontY,
        sideX, sideY
    );
    StoreShadowToBeRendered(
        SHADOW_INVCOLOR,
        rtshdw->GetShadowRwTexture(),
        shdwPos - CVector{ displacementX, displacementY, 0.f } * 2.5f,
        frontX * 1.5f, frontY * 1.5f,
        sideX * 1.5f, sideY * 1.5f,
        cc,
        cc, cc, cc,
        4.f,
        false,
        1.f,
        rtshdw,
        g_fx.GetFxQuality() >= FX_QUALITY_VERY_HIGH // NOTSA: At higher FX quality draw all shadows on buildings too
    );
}

// 0x707F40
void CShadows::UpdateStaticShadows() {
    // Remove shadows that have no polies/are temporary and have expired
    for (auto& sshdw : aStaticShadows) {
        if (!sshdw.m_pPolyBunch || sshdw.m_bJustCreated) {
            goto skip; // Not even created fully
        }

        if (sshdw.m_bTemporaryShadow && CTimer::GetTimeInMS() <= sshdw.m_nTimeCreated + 5000u) {
            goto skip; // Not expired yet
        }

        sshdw.Free();

    skip:
        sshdw.m_bJustCreated = false;
    }
}

// 0x707FA0
void CShadows::RenderExtraPlayerShadows() {
    if (!CTimeCycle::m_CurrentColours.m_nShadowStrength) {
        return;
    }

    const auto plyrVeh = FindPlayerVehicle();
    if (!plyrVeh) {
        return;
    }

    if (plyrVeh->m_nModelIndex == eModelID::MODEL_RCBANDIT) {
        return;
    }

    if (plyrVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) {
        return;
    }

    switch (plyrVeh->m_nVehicleType) {
    case VEHICLE_TYPE_BIKE:
    case VEHICLE_TYPE_FPLANE:
    case VEHICLE_TYPE_BOAT:
        return;
    }

    const auto  plyrPos    = FindPlayerCoors();
    const auto& plyrVehPos = plyrVeh->GetPosition();
    const auto& plyVehMat  = plyrVeh->GetMatrix();
    for (auto& ptl : CPointLights::GetActiveLights()) {
        if (ptl.m_nType != ePointLightType::PLTYPE_POINTLIGHT) {
            continue;
        }
        if (!ptl.m_bGenerateShadows) {
            continue;
        }
        if (ptl.m_fColorRed == 0.f && ptl.m_fColorGreen == 0.f && ptl.m_fColorBlue == 0.f) { // If it's black => ignore
            continue;
        }

        const auto lightToPlyr{ ptl.m_vecPosn - plyrPos };
        const auto lightToPlyrDist = lightToPlyr.Magnitude();
        if (lightToPlyrDist >= ptl.m_fRadius || lightToPlyrDist == 0.f) { // NOTSA: Zero check to prevent (possible) division-by-zero
            continue;
        }

        const auto lightToPlyrDir  = lightToPlyr / lightToPlyrDist; // Normalize vector

        const auto& plyrVehBB = plyrVeh->GetColModel()->GetBoundingBox();
        const auto  shdwSize  = CVector2D{ plyrVehBB.GetSize() } / 2.f;

        StoreShadowToBeRendered(
            SHADOW_DEFAULT,
            gpShadowCarTex,
            plyrVehPos - CVector{
                  CVector2D{ lightToPlyr / lightToPlyrDist } * 1.2f // Compensate for elvation of the light
                - CVector2D{ plyVehMat.GetForward() } * (shdwSize.y - plyrVehBB.m_vecMax.y) // Move point to center (as the shadow's position is it's center)
            },
            CVector2D{ plyVehMat.GetForward() } * shdwSize.y,
            CVector2D{ plyVehMat.GetRight() } * (plyVehMat.GetUp().z >= 0.f ? shdwSize.x : -shdwSize.x), // If vehicle is flipped, we gotta flip the `right` vector back
            CalculateShadowStrength( // 0x70809A
                lightToPlyrDist,
                ptl.m_fRadius,
                5 * CTimeCycle::m_CurrentColours.m_nShadowStrength / 8u // Same as mult by `0.625` and then casting to int
            ),
            0, 0, 0,
            4.5f,
            false,
            1.f,
            nullptr,
            false
        );
    }
}

// 0x708300
void CShadows::RenderStaticShadows() {
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,        RWRSTATE(rwFILTERLINEAR));

    RenderBuffer::ClearRenderBuffer();

    // Mark all as not-yet-rendered
    for (auto& shdw : aStaticShadows) {
        shdw.m_bRendered = false;
    }

    // Render all in batches
    for (auto& oshdw : aStaticShadows) {
        if (!oshdw.m_pPolyBunch || oshdw.m_bRendered) {
            continue;
        }

        // Setup additional render states for this shadow
        SetRenderModeForShadowType(oshdw.m_nType);
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(RwTextureGetRaster(oshdw.m_pTexture)));
         
        // Batch all other shadows with the same texture and type into the buffer
        for (auto& ishdw : aStaticShadows) {
            if (!ishdw.m_pPolyBunch) {
                continue;
            }
            if (ishdw.m_nType != oshdw.m_nType) {
                continue;
            }
            if (ishdw.m_pTexture != oshdw.m_pTexture) {
                continue;
            }
            // No need to check if this one was rendered (because of the batching)
            
            // Render polies of this shadow
            for (auto poly = ishdw.m_pPolyBunch; poly; poly = poly->m_pNext) {
                // 0x70841F: Calculate color
                uint8 r, g, b;
                CShadows::AffectColourWithLighting(
                    ishdw.m_nType,
                    ishdw.m_nDayNightIntensity,
                    ishdw.m_nRed, ishdw.m_nGreen, ishdw.m_nBlue,
                    r, g, b
                );

                const auto totalNoIdx = 3 * (poly->m_wNumVerts - 2); // Total no. of indices we'll use

                // 0x708432: Begin render buffer store
                RwIm3DVertex*    vtxIt{};
                RwImVertexIndex* vtxIdxIt{};
                RenderBuffer::StartStoring(
                    totalNoIdx,
                    poly->m_wNumVerts,
                    vtxIdxIt, vtxIt
                );

                // 0x70851D: Write vertices (`if` not necessary, it's part of the loop condition)
                const auto a = (uint8)((float)ishdw.m_nIntensity * (1.f - CWeather::Foggyness * 0.5f));
                for (auto i{ 0 }; i < poly->m_wNumVerts; i++, vtxIt++) {
                    const auto& pos = poly->m_avecPosn[i];
                    RwIm3DVertexSetPos(vtxIt, pos.x, pos.y, pos.z + 0.06f);
                    RwIm3DVertexSetRGBA(vtxIt, r, g, b, a);
                    RwIm3DVertexSetU(vtxIt, (float)poly->m_aU[i] / 200.f);
                    RwIm3DVertexSetV(vtxIt, (float)poly->m_aV[i] / 200.f);
                }

                // 0x7085BC: Write indices  (`if` not necessary, it's part of the loop condition)
                for (auto i = 0; i < totalNoIdx; i++) {
                    *vtxIdxIt++ = g_ShadowVertices[i];
                }
                
                // Finish storing 
                RenderBuffer::StopStoring();

                // Mark this as rendered
                ishdw.m_bRendered = true;
            }
        }

        // Render out this batch
        RenderBuffer::RenderStuffInBuffer();
    }

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
}

/*!
* @brief Clip a convex polygon (2D positions + matching uvs) against the half-plane to the
*        left of the directed edge `e0 -> e1` (Sutherland-Hodgman)
* @returns The number of vertices of the resulting polygon
*/
static int32 ClipShadowPoly(
    const CVector*    inPos,
    const CVector2D*  inUV,
    int32             numIn,
    CVector*          outPos,
    CVector2D*        outUV,
    const CVector2D&  e0,
    const CVector2D&  e1
) {
    const auto edge = e1 - e0;

    const auto SignedDist = [&](const CVector& p) {
        return (p.x - e0.x) * edge.y - (p.y - e0.y) * edge.x;
    };

    // 0 - Start, 1 - Previous was inside, 2 - Previous was outside
    int32 prevState = 0;
    int32 numOut    = 0;

    for (auto i = 0; i < numIn; i++) {
        const auto curDist = SignedDist(inPos[i]);

        // Intersection between the previous and the current vertex
        const auto EmitIntersection = [&] {
            const auto prevDist = SignedDist(inPos[i - 1]);
            const auto t        = std::abs(prevDist) / (std::abs(prevDist) + std::abs(curDist));
            outPos[numOut] = inPos[i - 1] + (inPos[i] - inPos[i - 1]) * t;
            outUV[numOut]  = inUV[i - 1] + (inUV[i] - inUV[i - 1]) * t;
            numOut++;
        };

        if (curDist <= 0.f) { // Outside
            if (prevState == 1) {
                EmitIntersection();
            }
            prevState = 2;
        } else { // Inside
            if (prevState == 0 || prevState == 1) {
                outPos[numOut] = inPos[i];
                outUV[numOut]  = inUV[i];
                numOut++;
            } else { // prevState == 2
                EmitIntersection();
                outPos[numOut] = inPos[i];
                outUV[numOut]  = inUV[i];
                numOut++;
            }
            prevState = 1;
        }
    }

    // The wrap-around edge (Last vertex -> first vertex)
    const auto firstDist = SignedDist(inPos[0]);
    if ((firstDist > 0.f && prevState == 2) || (firstDist < 0.f && prevState == 1)) {
        const auto lastDist = SignedDist(inPos[numIn - 1]);
        const auto t        = std::abs(lastDist) / (std::abs(lastDist) + std::abs(firstDist));
        outPos[numOut] = inPos[numIn - 1] + (inPos[0] - inPos[numIn - 1]) * t;
        outUV[numOut]  = inUV[numIn - 1] + (inUV[0] - inUV[numIn - 1]) * t;
        numOut++;
    }

    return numOut;
}

// 0x7086B0
void CShadows::CastShadowEntityXY(CEntity* entity, float conrerAX, float cornerAY, float cornerBX, float cornerBY, CVector* posn, float frontX, float frontY, float sideX, float sideY, int16 intensity, uint8 red, uint8 green, uint8 blue, float zDistance, float scale, CPolyBunch** ppPolyBunch, uint8* pDayNightIntensity, int32 shadowType) {
    const auto colModel = entity->GetColModel();
    auto* const colData = colModel->m_pColData;
    if (!colData || !colData->m_nNumTriangles) {
        return;
    }

    CCollision::CalculateTrianglePlanes(colModel);

    const auto& mat       = entity->GetMatrix();
    const auto& right     = mat.GetRight();
    const auto& forward   = mat.GetForward();
    const auto  entityPos = mat.GetPosition();

    // The shadow quad, expressed in the space spanned by the entity's `right` & `forward` axes
    const auto frontRight = frontX * right.x   + frontY * right.y;
    const auto frontFwd   = frontX * forward.x + frontY * forward.y;
    const auto sideRight  = sideX  * right.x   + sideY  * right.y;
    const auto sideFwd    = sideX  * forward.x + sideY  * forward.y;
    const auto relRight   = (posn->x - entityPos.x) * right.x   + (posn->y - entityPos.y) * right.y;
    const auto relFwd     = (posn->x - entityPos.x) * forward.x + (posn->y - entityPos.y) * forward.y;

    // The quad corners (Also used as a scratch buffer for the clipping passes)
    CVector quadPos[4]{
        { relRight + frontRight - sideRight, relFwd + frontFwd - sideFwd, 0.f },
        { relRight + frontRight + sideRight, relFwd + frontFwd + sideFwd, 0.f },
        { relRight - frontRight + sideRight, relFwd - frontFwd + sideFwd, 0.f },
        { relRight - frontRight - sideRight, relFwd - frontFwd - sideFwd, 0.f }
    };
    CVector2D quadUV[4]{ // And their texture coordinates
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        { 0.f, 1.f }
    };

    const auto minX = std::min({ quadPos[0].x, quadPos[1].x, quadPos[2].x, quadPos[3].x });
    const auto maxX = std::max({ quadPos[0].x, quadPos[1].x, quadPos[2].x, quadPos[3].x });
    const auto minY = std::min({ quadPos[0].y, quadPos[1].y, quadPos[2].y, quadPos[3].y });
    const auto maxY = std::max({ quadPos[0].y, quadPos[1].y, quadPos[2].y, quadPos[3].y });

    const auto quadZ = posn->z - entityPos.z;

    // Clipping buffers (A polygon can gain at most 1 vertex per clip)
    CVector   polyPos[8];
    CVector2D polyUV[8];

    const auto numFaceGroups = colData->bHasFaceGroups ? (int32)colData->GetNumFaceGroups() : 0;
    for (auto fgIdx = 0; fgIdx <= numFaceGroups; fgIdx++) {
        // If there are face groups only the triangles of the ones overlapping the quad are considered
        int32 firstTri, lastTri;
        if (colData->bHasFaceGroups) {
            if (fgIdx == numFaceGroups) {
                break;
            }

            const auto& fg = colData->GetFaceGroups()[fgIdx];
            if (!(fg.bb.m_vecMin.x < maxX && minX < fg.bb.m_vecMax.x
               && fg.bb.m_vecMin.y < maxY && minY < fg.bb.m_vecMax.y)
            ) {
                continue;
            }

            firstTri = fg.first;
            lastTri  = fg.last + 1;
        } else {
            firstTri = 0;
            lastTri  = colData->m_nNumTriangles;
        }

        for (auto triIdx = firstTri; triIdx < lastTri; triIdx++) {
            const auto& tri      = colData->m_pTriangles[triIdx];
            const auto& triPlane = colData->m_pTrianglePlanes[triIdx];

            const auto triNormal = triPlane.GetNormal();

            // Ignore (Nearly) vertical triangles
            if (std::abs(triNormal.z) <= 0.1f) {
                continue;
            }

            CVector triPos[3];
            colData->GetTrianglePoint(triPos[0], tri.vA);
            colData->GetTrianglePoint(triPos[1], tri.vB);
            colData->GetTrianglePoint(triPos[2], tri.vC);

            // Cull against the shadow quad
            if (!( (minX < triPos[0].x || minX < triPos[1].x || minX < triPos[2].x)
                && (triPos[0].x < maxX || triPos[1].x < maxX || triPos[2].x < maxX)
                && (minY < triPos[0].y || minY < triPos[1].y || minY < triPos[2].y)
                && (triPos[0].y < maxY || triPos[1].y < maxY || triPos[2].y < maxY)
                && ( (triPos[0].z < quadZ || triPos[1].z < quadZ || triPos[2].z < quadZ)
                  && (quadZ - zDistance < triPos[0].z || quadZ - zDistance < triPos[1].z || quadZ - zDistance < triPos[2].z))
            )) {
                continue;
            }

            // Clip the quad against the triangle
            auto numVerts = ClipShadowPoly(quadPos, quadUV, 4, polyPos, polyUV, CVector2D{ triPos[0] }, CVector2D{ triPos[1] });
            numVerts = ClipShadowPoly(polyPos, polyUV, numVerts, quadPos, quadUV, CVector2D{ triPos[1] }, CVector2D{ triPos[2] });
            numVerts = ClipShadowPoly(quadPos, quadUV, numVerts, polyPos, polyUV, CVector2D{ triPos[2] }, CVector2D{ triPos[0] });

            if (numVerts <= 2) {
                continue;
            }

            // Where each vertex is on the triangle's plane
            const auto planeOffset = triNormal.Dot(triPos[0]);
            for (auto i = 0; i < numVerts; i++) {
                polyPos[i].z = (triNormal.x * polyPos[i].x + triNormal.y * polyPos[i].y - planeOffset) * (-1.f / triNormal.z);
            }

            // Into the world
            for (auto i = 0; i < numVerts; i++) {
                const auto x = polyPos[i].x;
                const auto y = polyPos[i].y;
                polyPos[i].x = right.x   * x + forward.x * y + entityPos.x;
                polyPos[i].y = right.y   * x + forward.y * y + entityPos.y;
                polyPos[i].z = entityPos.z + polyPos[i].z;
            }

            *pDayNightIntensity = tri.m_nLight.value;

            if (!ppPolyBunch) {
                // Render it right away
                const auto numIndices = numVerts * 3 - 6;

                RwImVertexIndex* firstIndex{};
                RwIm3DVertex*    firstVtx{};
                RenderBuffer::StartStoring(numIndices, numVerts, firstIndex, firstVtx);

                uint8 litRed, litGreen, litBlue;
                AffectColourWithLighting((eShadowType)shadowType, *pDayNightIntensity, red, green, blue, litRed, litGreen, litBlue);
                const auto color = (uint32)intensity << 24 | (uint32)litRed << 16 | (uint32)litGreen << 8 | (uint32)litBlue;

                for (auto i = 0; i < numVerts; i++) {
                    auto& vtx = firstVtx[i];

                    vtx.objVertex = polyPos[i];
                    vtx.objVertex.z += 0.06f;
                    vtx.u = scale * polyUV[i].x;
                    vtx.v = scale * polyUV[i].y;
                    vtx.color = color;
                }

                std::copy_n(g_ShadowVertices.data(), numIndices, firstIndex);

                RenderBuffer::StopStoring();
            } else if (pEmptyBunchList) {
                const auto bunch = pEmptyBunchList;
                pEmptyBunchList   = bunch->m_pNext;

                bunch->m_pNext = *ppPolyBunch;
                *ppPolyBunch   = bunch;

                bunch->m_wNumVerts = (int16)numVerts;
                for (auto i = 0; i < numVerts; i++) {
                    bunch->m_avecPosn[i] = polyPos[i];
                    bunch->m_aU[i]       = (uint8)(polyUV[i].x * 200.f);
                    bunch->m_aV[i]       = (uint8)(polyUV[i].y * 200.f);
                }
            }
        }
    }
}

// 0x70A040
void CShadows::CastShadowEntityXYZ(CEntity* entity, CVector* posn, float frontX, float frontY, float sideX, float sideY, int16 intensity, uint8 red, uint8 green, uint8 blue, float zDistance, float scale, CPolyBunch** ppPolyBunch, CRealTimeShadow* realTimeShadow) {
    if (!realTimeShadow) {
        return;
    }

    const auto rwShadowCamera = realTimeShadow->GetShadowCamera()->m_pRwCamera;

    const auto colModel = entity->GetColModel();
    auto* const colData = colModel->m_pColData;
    if (!colData) {
        return;
    }

    CCollision::CalculateTrianglePlanes(colModel);

    _ProjectionParam param{};

    { // Build the matrix that projects a world space point into the shadow camera's viewport (`[0, 1]` range)
        const auto camFrameMatrix = RwFrameGetMatrix(RwCameraGetFrame(rwShadowCamera));

        param.invMatrix = *camFrameMatrix;

        const RwV3d xAxis{ 1.f, 0.f, 0.f };
        RwMatrixRotate(&param.invMatrix, &xAxis, -45.f, rwCOMBINEPRECONCAT);

        param.at = param.invMatrix.up; // TODO(re): Retail copies the (already rotated) camera matrix's `up` vector here, but the exact destination offsets in the exe couldn't be resolved

        RwMatrixInvert(&param.invMatrix, &param.invMatrix);

        const auto frustum = rwShadowCamera->viewWindow.x;
        const RwV3d scale{ -0.5f / (frustum * 0.9f), -0.5f / (frustum * 0.9f), 1.f / (frustum * 0.8f) };
        RwMatrixScale(&param.invMatrix, &scale, rwCOMBINEPOSTCONCAT);

        const RwV3d offset{ 0.5f, 0.f, 0.f };
        RwMatrixTranslate(&param.invMatrix, &offset, rwCOMBINEPOSTCONCAT);
    }

    param.shadowValue = (uint8)intensity;
    param.fade        = false;

    { // The matrix that moves an object space point into the world
        RwMatrix entityMatrix;
        entity->GetMatrix().CopyToRwMatrix(&entityMatrix);
        RwMatrixInvert(&param.entityMatrix, &entityMatrix);
    }

    // The sphere the shadow casting triangles are looked up with (Object space!)
    CColSphere sphere;
    {
        CVector center{
            realTimeShadow->m_baseSphere.m_vecCenter.x - frontX * 1.1f,
            realTimeShadow->m_baseSphere.m_vecCenter.y - frontY * 1.1f,
            realTimeShadow->m_baseSphere.m_vecCenter.z - 0.5f
        };
        RwV3dTransformPoints(&center, &center, 1, &param.entityMatrix);
        sphere.Set(2.f, center, SURFACE_DEFAULT, 0, tColLighting{ 0xFF });
    }

    for (auto i = 0; i < (int32)colData->m_nNumTriangles; i++) {
        const auto& tri      = colData->m_pTriangles[i];
        const auto& triPlane = colData->m_pTrianglePlanes[i];

        if (!CCollision::TestSphereTriangle(sphere, colData->m_pVertices, tri, triPlane)) {
            continue;
        }

        // Lift the triangle slightly along its normal
        auto       normal = triPlane.GetNormal();
        const auto offset = normal * 0.028f;

        CVector triPos[3];
        colData->GetTrianglePoint(triPos[0], tri.vA);
        colData->GetTrianglePoint(triPos[1], tri.vB);
        colData->GetTrianglePoint(triPos[2], tri.vC);
        for (auto& p : triPos) {
            p += offset;
        }

        if (!ShadowRenderTriangleCB(&normal, triPos, &param)) {
            return;
        }
    }
}

// 0x70A470
template<typename PtrListType>
void CShadows::CastPlayerShadowSectorList(
    PtrListType& ptrList,
    float cornerAX,
    float cornerAY,
    float cornerBX,
    float cornerBY,
    CVector* posn,
    float frontX,
    float frontY,
    float sideX,
    float sideY,
    int16 intensity,
    uint8 red,
    uint8 green,
    uint8 blue,
    float zDistance,
    float scale,
    CPolyBunch** ppPolyBunch,
    uint8* pDayNightIntensity,
    int32 shadowType
) {
    const CRect shadowRect{
        cornerAX, cornerAY,
        cornerBX, cornerBY
    };
    for (auto* const entity : ptrList) {
        if (entity->IsScanCodeCurrent()) {
            continue;
        }
        entity->SetCurrentScanCode();

        if (!entity->m_bUsesCollision || entity->m_bDontCastShadowsOn) {
            continue;
        }

        if (!entity->IsInCurrentArea()) {
            continue;
        }

        // If slightly tilted, ignore
        if (entity->GetMatrix().GetUp().z <= 0.97f) {
            continue;
        }

        // 0x70A526
        if (!entity->GetBoundRect().OverlapsWith(shadowRect)) {
            continue;
        }

        // Quick Z height check of the bounding box
        const auto& cm         = entity->GetColModel();
        const auto  entityPosZ = entity->GetPosition().z;
        if (cm->m_boundBox.m_vecMax.z + entityPosZ <= posn->z - zDistance) {
            continue;
        }
        if (cm->m_boundBox.m_vecMin.z + entityPosZ >= posn->z) {
            continue;
        }

        CastShadowEntityXY(
            entity,
            cornerAX,
            cornerAY,
            cornerBX,
            cornerBY,
            posn,
            frontX,
            frontY,
            sideX,
            sideY,
            intensity,
            red,
            green,
            blue,
            zDistance,
            scale,
            ppPolyBunch,
            pDayNightIntensity,
            shadowType
        );
    }
}

// 0x70A630
template<typename PtrListType>
void CShadows::CastShadowSectorList(PtrListType& ptrList, float conrerAX, float cornerAY, float cornerBX, float cornerBY, CVector* posn, float frontX, float frontY, float sideX, float sideY, int16 intensity, uint8 red, uint8 green, uint8 blue, float zDistance, float scale, CPolyBunch** ppPolyBunch, uint8* pDayNightIntensity, int32 shadowType) {
    // Nearly identical to `CastPlayerShadowSectorList`, the difference is 1 check is missing (`m_bDontCastShadowsOn`)... :D
    const CRect shadowRect{
        conrerAX, cornerAY,
        cornerBX, cornerBY
    };
    for (auto* const entity : ptrList) {
        if (entity->IsScanCodeCurrent()) {
            continue;
        }
        entity->SetCurrentScanCode();

        if (!entity->m_bUsesCollision || !entity->IsInCurrentArea()) {
            continue;
        }

        // If slightly tilted, ignore
        if (entity->GetMatrix().GetUp().z <= 0.97f) {
            continue;
        }

        // 0x70A526
        if (!entity->GetBoundRect().OverlapsWith(shadowRect)) {
            continue;
        }

        // Quick Z height check of the bounding box
        const auto& cm         = entity->GetColModel();
        const auto  entityPosZ = entity->GetPosition().z;
        if (cm->m_boundBox.m_vecMax.z + entityPosZ <= posn->z - zDistance) {
            continue;
        }
        if (cm->m_boundBox.m_vecMin.z + entityPosZ >= posn->z) {
            continue;
        }

        CastShadowEntityXY(
            entity,
            conrerAX,
            cornerAY,
            cornerBX,
            cornerBY,
            posn,
            frontX,
            frontY,
            sideX,
            sideY,
            intensity,
            red,
            green,
            blue,
            zDistance,
            scale,
            ppPolyBunch,
            pDayNightIntensity,
            shadowType
        );
    }
}

// 0x70A7E0
template<typename PtrListType>
void CShadows::CastRealTimeShadowSectorList(PtrListType& ptrList, float conrerAX, float cornerAY, float cornerBX, float cornerBY, CVector* posn, float frontX, float frontY, float sideX, float sideY, int16 intensity, uint8 red, uint8 green, uint8 blue, float zDistance, float scale, CPolyBunch** ppPolyBunch, CRealTimeShadow* realTimeShadow, uint8* pDayNightIntensity) {
    const CRect shadowRect{
        conrerAX, cornerAY,
        cornerBX, cornerBY
    };
    for (auto* const entity : ptrList) {
        if (entity->IsScanCodeCurrent()) {
            continue;
        }
        entity->SetCurrentScanCode();

        if (!entity->m_bUsesCollision || entity->m_bDontCastShadowsOn || !entity->IsInCurrentArea()) {
            continue;
        }

        // NOTE: Unlike the other sector list functions, tilted entities aren't ignored here
        if (!entity->GetBoundRect().OverlapsWith(shadowRect)) {
            continue;
        }

        // Quick Z height check of the bounding box
        const auto& cm         = entity->GetColModel();
        const auto  entityPosZ = entity->GetPosition().z;
        if (cm->m_boundBox.m_vecMax.z + entityPosZ <= posn->z - zDistance) {
            continue;
        }
        if (cm->m_boundBox.m_vecMin.z + entityPosZ >= posn->z) {
            continue;
        }

        CastShadowEntityXYZ(
            entity,
            posn,
            frontX,
            frontY,
            sideX,
            sideY,
            intensity,
            red,
            green,
            blue,
            zDistance,
            scale,
            ppPolyBunch,
            realTimeShadow
        );
    }
}

// 0x70A960
void CShadows::RenderStoredShadows() {
    // Originally renderstates are still set even though there are no shadows to be rendered
    // I don't think this is necessary, so we early-out here
    if (!CShadows::ShadowsStoredToBeRendered) {
        return;
    }

    RenderBuffer::ClearRenderBuffer();

    for (auto i = 0; i < ShadowsStoredToBeRendered; i++) {
        asShadowsStored[i].m_bAlreadyRenderedInBatch = false;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(rwRENDERSTATENARENDERSTATE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,    RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,       RWRSTATE(rwTEXTUREADDRESSCLAMP));
    RwRenderStateSet(rwRENDERSTATECULLMODE,             RWRSTATE(rwCULLMODECULLNONE));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,        RWRSTATE(rwFILTERLINEAR));

    const auto GetShadowRect = [](CRegisteredShadow& shdw) {
        return CRect{
            shdw.m_vecPosn,
            shdw.m_vecPosn + abs(shdw.m_Side) + abs(shdw.m_Front)
        };
    };

    for (auto o = 0; o < ShadowsStoredToBeRendered; o++) {
        auto& oshdw = asShadowsStored[o];

        // Setup additional render states for this shadow (and others in the batch below)
        SetRenderModeForShadowType(oshdw.m_nType);
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(RwTextureGetRaster(oshdw.m_pTexture)));

        //
        // Render onto ground (If not already)
        //
        if (!oshdw.m_bAlreadyRenderedInBatch) {
            // We do a batched rendering here:
            // All shadows of the same type and texture are rendered together to save on drawcalls

            for (auto i = o; i < ShadowsStoredToBeRendered; i++) {
                auto& ishdw = asShadowsStored[i];

                if (&ishdw != &oshdw) {
                    if (ishdw.m_nType != oshdw.m_nType) {
                        continue;
                    }
                    if (ishdw.m_pTexture != oshdw.m_pTexture) {
                        continue;
                    }
                }

                CWorld::AdvanceCurrentScanCode();

                const auto ishdwRect = GetShadowRect(ishdw);

                // Do shadow casting
                CWorld::IterateSectorsOverlappedByRect(
                    ishdwRect,
                    [&] (int32 x, int32 y) -> bool {
                        auto& sector = CWorld::GetSector(x, y);

                        if (const auto rtshdw = ishdw.m_pRTShadow) {
                            CastRealTimeShadowSectorList(
                                sector.Buildings,
                                ishdwRect.left, ishdwRect.bottom,
                                ishdwRect.right, ishdwRect.top,
                                &ishdw.m_vecPosn,
                                ishdw.m_Front.x, ishdw.m_Front.y,
                                ishdw.m_Side.x, ishdw.m_Side.y,
                                ishdw.m_nIntensity,
                                ishdw.m_nRed, ishdw.m_nGreen, ishdw.m_nBlue,
                                ishdw.m_fZDistance,
                                ishdw.m_fScale,
                                nullptr,
                                rtshdw,
                                nullptr // Unused
                            );
                        } else {
                            uint8 unused{};
                            if (ishdw.m_bDrawOnBuildings) {
                                CastShadowSectorList(
                                    sector.Buildings,
                                    ishdwRect.left, ishdwRect.bottom,
                                    ishdwRect.right, ishdwRect.top,
                                    &ishdw.m_vecPosn,
                                    ishdw.m_Front.x, ishdw.m_Front.y,
                                    ishdw.m_Side.x, ishdw.m_Side.y,
                                    ishdw.m_nIntensity,
                                    ishdw.m_nRed, ishdw.m_nGreen, ishdw.m_nBlue,
                                    ishdw.m_fZDistance,
                                    ishdw.m_fScale,
                                    nullptr,
                                    &unused,
                                    ishdw.m_nType
                                );
                            } else {
                                CastPlayerShadowSectorList(
                                    sector.Buildings,
                                    ishdwRect.left, ishdwRect.bottom,
                                    ishdwRect.right, ishdwRect.top,
                                    &ishdw.m_vecPosn,
                                    ishdw.m_Front.x, ishdw.m_Front.y,
                                    ishdw.m_Side.x, ishdw.m_Side.y,
                                    ishdw.m_nIntensity,
                                    ishdw.m_nRed, ishdw.m_nGreen, ishdw.m_nBlue,
                                    ishdw.m_fZDistance,
                                    ishdw.m_fScale,
                                    nullptr,
                                    &unused,
                                    ishdw.m_nType
                                );
                            }
                        }
                        return true; // Inisde lambda -> Continue sector loop
                    }
                );

                // Mark this as rendered
                ishdw.m_bAlreadyRenderedInBatch = true;
            }

            // Render out shadows (Can't batch together with next iteration, as renderstates change)
            RenderBuffer::RenderStuffInBuffer();
        }
        
        // Render onto water (If needed)
        if (oshdw.m_bDrawOnWater) {  
            float waterLevelZ{}, bigWavesZ{}, smallWavesZ{};
            if (!CWaterLevel::GetWaterLevelNoWaves(oshdw.m_vecPosn, &waterLevelZ, &bigWavesZ, &smallWavesZ)) {
                continue;
            }

            // Shadow under water?
            if (waterLevelZ >= oshdw.m_vecPosn.z) {
                continue;
            }

            // Clear the buffer as this is a new render pass (separate from the other)
            RenderBuffer::ClearRenderBuffer();

            // The smaller this value the detailed the shadows, but also (exponentially?) slower
            // This value basically represents the side of a square that is projected onto the 
            const auto STEP_SIZE = 2;

            const auto shdwSideMagSq = oshdw.m_Side.SquaredMagnitude();
            const auto shdwFrontMagSq = oshdw.m_Front.SquaredMagnitude();

            const auto oshdwRect = GetShadowRect(oshdw);

            // Iterate shadow sectors (Using `CWorld::IterateSectors` for convenience)
            CWorld::IterateSectors(
                (int32)(std::floor(oshdwRect.left / (float)STEP_SIZE)),
                (int32)(std::floor(oshdwRect.top / (float)STEP_SIZE)),
                (int32)(std::ceil(oshdwRect.right / (float)STEP_SIZE)),
                (int32)(std::ceil(oshdwRect.bottom / (float)STEP_SIZE)),
                [&](int32 ox, int32 oy) {
                    // 0x70B000 - 0x70B0DA: Start storing 
                    RwIm3DVertex* vtxIt{};
                    RwImVertexIndex* vtxIdxIt{};
                    RenderBuffer::StartStoring(6, 4, vtxIdxIt, vtxIt);

                    // Function to process 1 vertex (out of the 4)
                    const auto ProcessOneVertex = [&] (int32 x, int32 y) {
                        // The x, y passed in are sector coords, so make them into world coords
                        x *= STEP_SIZE;
                        y *= STEP_SIZE;

                        const auto currPos = CVector2D{ (float)x, (float)y };

                        // Set color
                        RwIm3DVertexSetRGBA(vtxIt, oshdw.m_nRed, oshdw.m_nGreen, oshdw.m_nBlue, (uint8)((float)oshdw.m_nIntensity * 0.6f));

                        // Set texture coords (UV)
                        const auto CalcTexCoord  = [
                            currPosToShdw = CVector2D{ currPos - oshdw.m_vecPosn }
                        ](CVector2D v, float vmagsq) {
                            return (currPosToShdw.Dot(v) / vmagsq + 1.f) * 0.5f;
                        };
                        RwIm3DVertexSetU(vtxIt, CalcTexCoord(oshdw.m_Side, shdwSideMagSq));
                        RwIm3DVertexSetV(vtxIt, CalcTexCoord(oshdw.m_Front, shdwFrontMagSq));

                        // Set position
                        RwIm3DVertexSetPos(
                            vtxIt,
                            currPos.x,
                            currPos.y,
                            CWaterLevel::CalculateWavesOnlyForCoordinate2_Direct(x, y, waterLevelZ, bigWavesZ, smallWavesZ) + 0.06f // Z pos of the wave at the given coord
                        );
                    };

                    // Process 4 vertices of this square
                    ProcessOneVertex(ox,     oy    ); // top left
                    ProcessOneVertex(ox + 1, oy    ); // top right
                    ProcessOneVertex(ox,     oy + 1); // bottom left
                    ProcessOneVertex(ox + 1, oy + 1); // bottom right

                    // Copy vertices into index buffer
                    rng::copy(std::to_array({ 0, 1, 2, 1, 3, 2 }), vtxIdxIt);

                    // And we're done with this one... onto the next!
                    RenderBuffer::StopStoring();

                    // Continue iteration...
                    return true;
                }
            );

            // Render it all
            RenderBuffer::RenderStuffInBuffer();
        }
    }

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,    RWRSTATE(rwTEXTUREADDRESSWRAP));
    RwRenderStateSet(rwRENDERSTATECULLMODE,          RWRSTATE(rwCULLMODECULLBACK));

    ShadowsStoredToBeRendered = 0;
}

// 0x70B730
void CShadows::GeneratePolysForStaticShadow(int16 staticShadowIndex) {
    auto& shdw = aStaticShadows[staticShadowIndex];

    auto posn = shdw.m_vecPosn;

    const auto minX = posn.x - (std::abs(shdw.m_fFrontX) + std::abs(shdw.m_fSideX));
    const auto maxX = posn.x + std::abs(shdw.m_fFrontX) + std::abs(shdw.m_fSideX);
    const auto minY = posn.y - (std::abs(shdw.m_fFrontY) + std::abs(shdw.m_fSideY));
    const auto maxY = posn.y + std::abs(shdw.m_fFrontY) + std::abs(shdw.m_fSideY);

    // The sectors it spans (Note: Each sector is 50 units wide)
    const auto ToCell = [](float v) { return (int32)(v * 0.02f + 60.f); };
    const auto cellMinX = std::max(ToCell(minX), 0);
    const auto cellMinY = std::max(ToCell(minY), 0);
    const auto cellMaxX = std::min(ToCell(maxX), 119);
    const auto cellMaxY = std::min(ToCell(maxY), 119);

    CWorld::AdvanceCurrentScanCode();

    for (auto y = cellMinY; y <= cellMaxY; y++) {
        for (auto x = cellMinX; x <= cellMaxX; x++) {
            CastPlayerShadowSectorList(
                CWorld::GetSector(x, y).Buildings,
                minX, minY,
                maxX, maxY,
                &posn,
                shdw.m_fFrontX, shdw.m_fFrontY,
                shdw.m_fSideX,  shdw.m_fSideY,
                0, 0, 0, 0,
                shdw.m_fZDistance,
                shdw.m_fScale,
                &shdw.m_pPolyBunch,
                &shdw.m_nDayNightIntensity,
                0
            );
        }
    }
}

// 0x70BA00
bool CShadows::StoreStaticShadow(uint32 id, eShadowType type, RwTexture* texture, const CVector& posn, float frontX, float frontY, float sideX, float sideY, int16 intensity, uint8 red, uint8 green, uint8 blue, float zDistane, float scale, float drawDistance, bool temporaryShadow, float upDistance) {
    // Fade out (And eventually ignore) shadows too far away from the camera
    {
        const auto& camPos = TheCamera.GetPosition();
        const auto  dist2DSq = sq(posn.x - camPos.x) + sq(posn.y - camPos.y);

        if (dist2DSq < sq(drawDistance)) {
            if (drawDistance != 0.f && drawDistance * 0.75f <= std::sqrt(dist2DSq)) {
                const auto mult = 1.f - (std::sqrt(dist2DSq) - drawDistance * 0.75f) * (4.f / drawDistance);
                intensity = (int16)(mult * (float)intensity);
                red       = (uint8)(mult * (float)red);
                green     = (uint8)(mult * (float)green);
                blue      = (uint8)(mult * (float)blue);
            }
        } else if (drawDistance != 0.f) {
            return true;
        }
    }

    int32 slot = -1;

    // Update an already existing shadow with the same id (Or reuse its slot)
    for (auto i = 0; i < (int32)MAX_STATIC_SHADOWS; i++) {
        auto& shdw = aStaticShadows[i];

        if (shdw.m_nId != id || !shdw.m_pPolyBunch) {
            continue;
        }

        const auto samePlace =
               std::abs(posn.x - shdw.m_vecPosn.x) < upDistance
            && std::abs(posn.y - shdw.m_vecPosn.y) < upDistance;
        const auto sameShape =
               std::abs(posn.x - shdw.m_vecPosn.x) < 0.05f
            && std::abs(posn.y - shdw.m_vecPosn.y) < 0.05f
            && std::abs(posn.z - shdw.m_vecPosn.z) < 2.f
            && frontX == shdw.m_fFrontX
            && frontY == shdw.m_fFrontY
            && sideX  == shdw.m_fSideX
            && sideY  == shdw.m_fSideY;

        if (samePlace || sameShape) {
            shdw.m_pTexture         = texture;
            shdw.m_nType            = type;
            shdw.m_nRed             = red;
            shdw.m_nGreen           = green;
            shdw.m_nBlue            = blue;
            shdw.m_nIntensity       = intensity;
            shdw.m_fScale           = scale;
            shdw.m_fZDistance       = zDistane;
            shdw.m_nTimeCreated     = CTimer::GetTimeInMS();
            shdw.m_bJustCreated     = true;
            shdw.m_bTemporaryShadow = temporaryShadow;
            return true;
        }

        shdw.Free();
        slot = i;
        break;
    }

    // Otherwise find a free one
    if (slot < 0) {
        for (auto i = 0; i < (int32)MAX_STATIC_SHADOWS; i++) {
            if (!aStaticShadows[i].m_pPolyBunch) {
                slot = i;
                break;
            }
        }
    }

    if (slot < 0) {
        return true;
    }

    auto& shdw = aStaticShadows[slot];

    shdw.m_nType            = type;
    shdw.m_pTexture         = texture;
    shdw.m_nIntensity       = intensity;
    shdw.m_nRed             = red;
    shdw.m_nGreen           = green;
    shdw.m_nBlue            = blue;
    shdw.m_fZDistance       = zDistane;
    shdw.m_fScale           = scale;
    shdw.m_nId              = id;
    shdw.m_vecPosn          = posn;
    shdw.m_fFrontX          = frontX;
    shdw.m_fFrontY          = frontY;
    shdw.m_fSideX           = sideX;
    shdw.m_fSideY           = sideY;
    shdw.m_bJustCreated     = true;
    shdw.m_bTemporaryShadow = temporaryShadow;
    shdw.m_nTimeCreated     = CTimer::GetTimeInMS();

    GeneratePolysForStaticShadow((int16)slot);

    return shdw.m_pPolyBunch != nullptr;
}

// 0x70BDA0
void CShadows::StoreShadowForVehicle(CVehicle* vehicle, VEH_SHD_TYPE vehShadowType) {
    const auto shdwStrength = CTimeCycle::m_CurrentColours.m_nShadowStrength;

    if (GraphicsHighQuality() || !shdwStrength) {
        return;
    }

    const auto& camPos = TheCamera.GetPosition();
    const auto& vehPos = vehicle->GetPosition();

    auto camToVehDist2DSq = (vehPos - camPos).SquaredMagnitude2D();
    if (CCutsceneMgr::IsRunning()) {
        camToVehDist2DSq /= sq(TheCamera.m_fLODDistMultiplier) * 4.f;
    }

    const auto maxDist = [&] {
        switch (vehShadowType) {
        case VEH_SHD_HELI:
        case VEH_SHD_PLANE:
        case VEH_SHD_RC:
            return 144.f;
        case VEH_SHD_BIG_PLANE:
            return 288.f;
        default:
            return 18.f;
        }
    }();

    if (camToVehDist2DSq >= sq(maxDist)) {
        return;
    }

    const auto camToVehDist2D = std::sqrt(camToVehDist2DSq);

    // Fade out the shadows that are far away
    auto strength = shdwStrength;
    if (camToVehDist2D >= maxDist * 0.75f) {
        strength = (uint8)((1.f - (camToVehDist2D - maxDist * 0.75f) / (camToVehDist2D * 0.25f)) * (float)shdwStrength);
    }

    const auto& vehBB = vehicle->GetColModel()->GetBoundingBox();

    auto sizeX     = vehBB.m_vecMax.x - vehBB.m_vecMin.x;
    auto sizeY     = vehBB.m_vecMax.y - vehBB.m_vecMin.y;
    auto sizeMultY = 1.f;

    switch ((eModelID)vehicle->m_nModelIndex) {
    case MODEL_LEVIATHN:
    case MODEL_HUNTER:
    case MODEL_SEASPAR:
    case MODEL_SPARROW:
    case MODEL_MAVERICK:
    case MODEL_POLMAV:
        sizeMultY = 0.5f;
        sizeX *= 3.f;
        sizeY *= 1.4f;
        break;
    case MODEL_PIZZABOY:
    case MODEL_PCJ600:
    case MODEL_FAGGIO:
    case MODEL_BMX:
    case MODEL_BIKE:
    case MODEL_MTBIKE:
    case MODEL_FCR900:
    case MODEL_NRG500:
        sizeMultY = 0.05f;
        sizeY *= 1.2f;
        break;
    case MODEL_FREEWAY:
    case MODEL_SANCHEZ:
    case MODEL_COPBIKE:
        sizeMultY = 0.03f;
        sizeY *= 1.5f;
        break;
    case MODEL_RCRAIDER:
    case MODEL_RCGOBLIN:
        sizeMultY = 0.2f;
        sizeY *= 1.5f;
        sizeX *= 2.f;
        break;
    case MODEL_VORTEX:
        return;
    default:
        break;
    }

    const auto& vehMat = vehicle->GetMatrix();

    // Move the shadow slightly towards the vehicle's rear
    const auto yOffset = (sizeY * 0.5f - vehBB.m_vecMax.y) * sizeMultY;
    CVector    pos{
        vehPos.x - yOffset * vehMat.GetForward().x,
        vehPos.y - yOffset * vehMat.GetForward().y,
        vehPos.z
    };

    // RC vehicles are always treated as the player's one
    const auto isPlyrVeh = vehicle->vehicleFlags.bIsRCVehicle || FindPlayerVehicle(-1, false) == vehicle;

    auto zDistance = 4.5f;

    RwTexture* texture{};
    switch (vehShadowType) {
    case VEH_SHD_BIKE: {
        auto sizeMult = std::abs(vehicle->AsBike()->GetRideAnimData()->LeanAngle) * 5.092958f + 1.f;

        const auto rightZ = std::abs(vehMat.GetRight().z);
        if (vehicle->GetStatus() == STATUS_ABANDONED && rightZ > 0.6f) {
            sizeX *= sizeMult + rightZ * 4.f;
        } else {
            sizeX *= sizeMult;
        }

        texture = gpShadowBikeTex;
        break;
    }
    case VEH_SHD_HELI:
        if (isPlyrVeh) {
            zDistance = 50.f;
        }
        texture = gpShadowHeliTex;
        break;
    case VEH_SHD_PLANE:
    case VEH_SHD_BIG_PLANE:
        strength = shdwStrength; // Planes don't fade out
        if (isPlyrVeh) {
            zDistance = 50.f;
        }
        texture = gpShadowBaronTex;
        break;
    case VEH_SHD_RC:
        sizeY *= 1.5f;
        sizeX *= 2.2f;
        texture = gpShadowBaronTex;
        break;
    case VEH_SHD_CAR:
    default:
        texture = gpShadowCarTex;
        break;
    }

    // The shadow's "side" vector is perpendicular to the vehicle's right axis
    auto side = CrossProduct(vehMat.GetRight(), CVector{ 0.f, 0.f, 1.f });
    if (const auto sideLen = std::sqrt(sq(side.x) + sq(side.y)); sideLen < 0.5f) {
        const auto inv = 1.f / sideLen;
        side.x = vehMat.GetRight().y * inv * 0.5f;
        side.y = -vehMat.GetRight().x * inv * 0.5f;
    }
    if (vehMat.GetUp().z < 0.f) {
        side.x = -side.x;
        side.y = -side.y;
    }

    // The shadow's "front" vector (Which is the vehicle's right axis by default)
    auto frontX = vehMat.GetRight().x;
    auto frontY = vehMat.GetRight().y;

    switch (vehShadowType) {
    case VEH_SHD_BIKE:
        if (std::abs(vehMat.GetRight().z) > 0.6f) {
            frontX = vehMat.GetUp().x;
            frontY = vehMat.GetUp().y;
        }
        break;
    case VEH_SHD_HELI:
        if (std::abs(vehMat.GetRight().z) > 0.57f) {
            frontX = vehMat.GetUp().x;
            frontY = vehMat.GetUp().y;
        }
        if (std::abs(vehMat.GetForward().z) > 0.57f) {
            side.x = vehMat.GetUp().x;
            side.y = vehMat.GetUp().y;
        }
        break;
    }

    // Half of the size is used, and the whole thing is mirrored if the vehicle is upside down
    const auto sideSign   = vehMat.GetUp().z <= 0.f ? -1.f : 1.f;
    const auto frontOutX  = sizeY * 0.5f * frontX;
    const auto frontOutY  = sizeY * 0.5f * frontY;
    const auto sideOutX   = sideSign * sizeX * 0.5f * side.x;
    const auto sideOutY   = sideSign * sizeX * 0.5f * side.y;

    if (vehicle->GetMoveSpeed().Magnitude() * CTimer::GetTimeStep() <= 0.1f && !isPlyrVeh) {
        // (Nearly) stationary vehicles use a static shadow
        StoreStaticShadow(
            notsa::PtrToId(vehicle) + 1,
            SHADOW_DEFAULT,
            texture,
            pos,
            frontOutX, frontOutY,
            sideOutX,  sideOutY,
            strength, strength, strength, strength,
            4.5f,
            1.f,
            0.f,
            false,
            0.1f
        );
    } else {
        StoreShadowToBeRendered(
            SHADOW_DEFAULT,
            texture,
            pos,
            frontOutX, frontOutY,
            sideOutX,  sideOutY,
            strength,
            strength, strength, strength,
            zDistance,
            isPlyrVeh,
            1.f,
            nullptr,
            isPlyrVeh
        );
    }
}

// 0x70C500
void CShadows::StoreCarLightShadow(CVehicle* vehicle, uint32 id, RwTexture* texture, const CVector& posn, float frontX, float frontY, float sideX, float sideY, uint8 red, uint8 green, uint8 blue, float maxViewAngleCosine) {
    // Maximum distance (from camera to `posn`) after which shadows aren't stored (and rendered)
    constexpr auto MAX_CAM_TO_LIGHT_DIST = 27.f;

    if ([] { // Maybe ignore camera distance?
        switch (CCamera::GetActiveCamera().m_nMode) {
        case MODE_TOPDOWN:
        case MODE_TOP_DOWN_PED:
            return false;
        }

        return !CCutsceneMgr::IsRunning();
    }()) {
        const auto shdwToCam2D       = CVector2D{ TheCamera.GetPosition() - posn };
        const auto shdwToCamDist2DSq = shdwToCam2D.SquaredMagnitude();

        if (shdwToCamDist2DSq >= sq(MAX_CAM_TO_LIGHT_DIST)) {
            return;
        }

        // Check if the camera is facing the lights closely (in which case the camera can't see the shadow)
        if (shdwToCam2D.Dot(TheCamera.GetFrontNormal2D()) > maxViewAngleCosine) {
            return;
        }

        // If far enough from the camera, start fading out
        if (const auto dist = std::sqrt(shdwToCamDist2DSq); dist >= MAX_CAM_TO_LIGHT_DIST * 0.75f) {
            const auto t = 1.f - invLerp(MAX_CAM_TO_LIGHT_DIST * (2.f / 3.f), MAX_CAM_TO_LIGHT_DIST, dist);
            red   = (uint8)((float)red * t);
            green = (uint8)((float)green * t);
            blue  = (uint8)((float)blue * t);
        }
    }

    const auto isPlyrVeh = FindPlayerVehicle() == vehicle;
    if (isPlyrVeh || vehicle->GetMoveSpeed().Magnitude() * CTimer::GetTimeStep() >= 0.4f) {
        StoreShadowToBeRendered(
            SHADOW_ADDITIVE,
            texture,
            posn,
            frontX, frontY,
            sideX, sideY,
            128,
            red, green, blue,
            6.f,
            false,
            1.f,
            nullptr,
            isPlyrVeh || g_fx.GetFxQuality() >= FX_QUALITY_VERY_HIGH // NOTSA: At higher FX quality draw all vehicles's shadows on buildings too
        );
    } else {
        StoreStaticShadow(
            notsa::PtrToId(vehicle) + id,
            SHADOW_ADDITIVE,
            texture,
            posn,
            frontX, frontY,
            sideX, sideY,
            128,
            red, green, blue,
            6.f,
            1.f,
            0.f,
            false,
            0.4f
        );
    }
}

// 0x70C750
void CShadows::StoreShadowForPole(CEntity* entity, float offsetX, float offsetY, float offsetZ, float poleHeight, float poleWidth, uint32 localId) {
    if (GraphicsHighQuality() || !CTimeCycle::m_CurrentColours.m_nPoleShadowStrength) {
        return;
    }

    const auto& mat = entity->GetMatrix();

    if (mat.GetUp().z < .5f) { // More than 45 deg tilted
        return;
    }

    const auto intensity = 2.f * (mat.GetUp().z - 0.5f) * CTimeCycle::m_CurrentColours.m_nPoleShadowStrength;

    const auto front     = CVector2D{ CTimeCycle::GetVectorToSun() } * (-poleHeight / 2.f);
    const auto right     = CVector2D{ CTimeCycle::GetShadowSide() } * poleWidth;

    StoreStaticShadow(
        notsa::PtrToId(entity->GetLod()) + localId + 3,
        SHADOW_DEFAULT,
        gpPostShadowTex,
        mat.GetPosition() + CVector{ front, 0.f } + CVector{
            offsetX * mat.GetRight().x + offsetY * mat.GetForward().x, // Simplified matrix transform (Ignoring the Z axis)
            offsetX * mat.GetRight().y + offsetY * mat.GetForward().y, // >^^^
            offsetZ
        },
        front.x, front.y,
        right.x, right.y,
        2 * (int16)intensity / 3,
        0, 0, 0,
        9.f,
        1.f,
        40.f,
        false,
        0.f
    );
}

// 0x70CCB0
void CShadows::RenderIndicatorShadow(
    uint32 id,
    eShadowType,
    RwTexture*,
    const CVector& posn,
    float frontX, float frontY,
    float sideX, float sideY,
    int16 /*intensity*/
) {
    const auto size = std::max(frontX, -sideX);
    for (auto mult : { 0.8f, 0.9f, 1.0f }) {
        CVector markerPos = posn;
        C3dMarkers::PlaceMarkerSet(id, MARKER3D_CYLINDER, markerPos, size * mult, 255, 0, 0, 255u, 2048, 0.2f, 0);
    }
}

// 0x709CF0
CVector* ShadowRenderTriangleCB(CVector* normal, CVector* trianglePos, _ProjectionParam* param) {
    // `trianglePos` is in object space, move it into the world (This is what the shadow is drawn in)
    CVector worldPos[3];
    RwV3dTransformPoints(worldPos, trianglePos, 3, &param->entityMatrix);

    // Ignore triangles facing away from the camera
    if (param->at.x * normal->x + param->at.y * normal->y + param->at.z * normal->z > 0.f) {
        return trianglePos;
    }

    // Project into the shadow camera's viewport space (In the `[0, 1]` range)
    CVector proj[3];
    RwV3dTransformPoints(proj, worldPos, 3, &param->invMatrix);

    // At least a part of it has to be inside the viewport (Retail uses a per-coordinate `or` over the vertices)
    const bool isVisible =
           (proj[0].z >= 0.f || proj[1].z >= 0.f || proj[2].z >= 0.f)
        && (proj[0].x >= 0.f || proj[1].x >= 0.f || proj[2].x >= 0.f)
        && (proj[0].x <= 1.f || proj[1].x <= 1.f || proj[2].x <= 1.f)
        && (proj[0].y >= 0.f || proj[1].y >= 0.f || proj[2].y >= 0.f)
        && (proj[0].y <= 1.f || proj[1].y <= 1.f || proj[2].y <= 1.f);
    if (!isVisible) {
        return trianglePos;
    }

    RwImVertexIndex* firstIndex{};
    RwIm3DVertex*    firstVtx{};
    RenderBuffer::StartStoring(3, 3, firstIndex, firstVtx);

    for (auto i = 0; i < 3; i++) {
        auto& vtx = firstVtx[i];

        vtx.objVertex = worldPos[i];
        vtx.objVertex.z += 0.06f; // Lift it a tiny bit so it doesn't z-fight with the ground
        vtx.u = proj[i].x;
        vtx.v = proj[i].y;

        // The shadow gets more transparent with the distance from the camera
        const auto alpha = [&] {
            if (!param->fade) {
                return param->shadowValue;
            }
            if (const auto sqZ = 1.f - sq(proj[i].z); sqZ >= 0.f) {
                return (uint8)(sqZ * (float)param->shadowValue);
            }
            return (uint8)0;
        }();
        vtx.color = (uint32)alpha << 24 | (uint32)alpha << 16 | (uint32)alpha << 8 | (uint32)alpha;
    }

    firstIndex[0] = 0;
    firstIndex[1] = 1;
    firstIndex[2] = 2;

    RenderBuffer::StopStoring();

    return trianglePos;
}
