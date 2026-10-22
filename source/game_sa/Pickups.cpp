/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include <extensions/utility.hpp>

#include "Pickups.h"
#include "Garages.h"
#include "tPickupMessage.h"
#include "Radar.h"
#include "Shadows.h"
#include "Coronas.h"
#include "PostEffects.h"
#include "TaskSimpleJetPack.h"

using namespace ModelIndices;

#ifdef NOTSA_NATIVE
//! 0x8A5FB0, file-backed in the executable's `.data`: the HUD colours used by pickup messages.
tPickupMessageColour CPickups::aMessageColoursX[NUM_PICKUP_MESSAGE_COLOURS] = {
    { { 128, 128, 128, 0 }, 1.0f },
    { { 128, 128, 128, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  97,  97,  97, 0 }, 1.0f },
    { {  97,  97,  97, 0 }, 1.0f },
    { {  97,  97,  97, 0 }, 1.0f },
    { {  97,  97,  97, 0 }, 1.0f },
    { {  97,  97,  97, 0 }, 1.0f },
    { {  97,  97,  97, 0 }, 1.0f },
    { {  27,  89, 130, 0 }, 1.0f },
    { {  27,  89, 130, 0 }, 1.0f },
    { {  27,  89, 130, 0 }, 1.0f },
    { {  27,  89, 130, 0 }, 1.0f },
    { {  27,  89, 130, 0 }, 1.0f },
    { {  27,  89, 130, 0 }, 1.0f },
    { { 149, 194,  24, 0 }, 1.0f },
    { { 149, 194,  24, 0 }, 1.0f },
    { { 149, 194,  24, 0 }, 1.0f },
    { {  45, 155,  90, 0 }, 1.0f },
    { {  45, 155,  90, 0 }, 1.0f },
    { {  45, 155,  90, 0 }, 1.0f },
    { { 255, 227,  79, 0 }, 1.0f },
    { { 255, 227,  79, 0 }, 1.0f },
    { { 254, 137,   0, 0 }, 1.0f },
    { { 254, 137,   0, 0 }, 1.0f },
    { { 255, 227,  79, 0 }, 1.0f },
    { { 249, 131, 215, 0 }, 1.0f },
    { { 249, 131, 215, 0 }, 1.0f },
    { { 164,  40, 178, 0 }, 1.0f },
    { { 164,  40, 178, 0 }, 1.0f },
    { { 164,  40, 178, 0 }, 1.0f },
    { { 164,  40, 178, 0 }, 1.0f },
    { { 164,  40, 178, 0 }, 1.0f },
    { { 164,  40, 178, 0 }, 1.0f },
    { {  97, 194, 247, 0 }, 1.0f },
    { {  69,  69,  69, 0 }, 1.0f },
    { {  69,  69,  69, 0 }, 1.0f },
    { {  69,  69,  69, 0 }, 1.0f },
    { {  69,  69,  69, 0 }, 1.0f },
    { {  69,  69,  69, 0 }, 1.0f },
    { { 255, 100, 100, 0 }, 1.0f },
    { { 128, 255, 128, 0 }, 1.0f },
    { { 100, 100, 255, 0 }, 1.0f },
    { { 255, 255, 100, 0 }, 1.0f },
    { { 255, 100, 100, 0 }, 1.0f },
    { {  54, 104,  44, 0 }, 1.0f },
    { {  54, 104,  44, 0 }, 1.0f },
};
#endif

void CPickups::InjectHooks() {
    RH_ScopedClass(CPickups);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x454A70);
    RH_ScopedInstall(ReInit, 0x456E60);
    RH_ScopedInstall(AddToCollectedPickupsArray, 0x455240);
    RH_ScopedOverloadedInstall(CreatePickupCoorsCloseToCoors, "", 0x458A80, void(*)(float, float, float, float&, float&, float&));
    RH_ScopedInstall(CreateSomeMoney, 0x458970);
    RH_ScopedInstall(DetonateMinesHitByGunShot, 0x4590C0);
    RH_ScopedInstall(DoCollectableEffects, 0x455E20);
    RH_ScopedInstall(DoMineEffects, 0x4560E0);
    RH_ScopedInstall(DoMoneyEffects, 0x454E80);
    RH_ScopedInstall(DoPickUpEffects, 0x455720);
    RH_ScopedInstall(FindPickUpForThisObject, 0x4551C0);

    RH_ScopedInstall(GenerateNewOne, 0x456F20);

    RH_ScopedInstall(GenerateNewOne_WeaponType, 0x457380);
    RH_ScopedInstall(GetActualPickupIndex, 0x4552A0);
    RH_ScopedInstall(GetNewUniquePickupIndex, 0x456A30);
    RH_ScopedInstall(GetUniquePickupIndex, 0x455280);
    RH_ScopedInstall(GivePlayerGoodiesWithPickUpMI, 0x4564F0);
    RH_ScopedInstall(IsPickUpPickedUp, 0x454B40);
    RH_ScopedInstall(ModelForWeapon, 0x454AC0);
    RH_ScopedInstall(PassTime, 0x455200);
    RH_ScopedInstall(PickedUpHorseShoe, 0x455390);
    RH_ScopedInstall(PickedUpOyster, 0x4552D0);
    RH_ScopedInstall(PictureTaken, 0x456A70);
    RH_ScopedInstall(PlayerCanPickUpThisWeaponTypeAtThisMoment, 0x4554C0);
    RH_ScopedInstall(RemoveMissionPickUps, 0x456DE0);
    RH_ScopedInstall(RemovePickUp, 0x4573D0);
    RH_ScopedInstall(RemovePickUpsInArea, 0x456D30);
    RH_ScopedInstall(RemovePickupObjects, 0x455470);
    RH_ScopedInstall(RemoveUnnecessaryPickups, 0x4563A0);
    RH_ScopedInstall(RenderPickUpText, 0x455000);
    RH_ScopedInstall(TestForPickupsInBubble, 0x456450);
    RH_ScopedInstall(TryToMerge_WeaponType, 0x4555A0);
    RH_ScopedInstall(Update, 0x458DE0);
    RH_ScopedInstall(UpdateMoneyPerDay, 0x455680);
    RH_ScopedInstall(WeaponForModel, 0x454AE0);
    RH_ScopedInstall(Load, 0x5D35A0);
    RH_ScopedInstall(Save, 0x5D3540);

    RH_ScopedGlobalInstall(ModifyStringLabelForControlSetting, 0x454B70);
}

// 0x454A70
void CPickups::Init() {
    ZoneScoped;

    NumMessages = 0;
    for (auto& pickup : aPickUps) {
        pickup.m_nPickupType = PICKUP_NONE;
        pickup.m_nReferenceIndex = 1;
        pickup.m_pObject = nullptr;
    }
    aPickUpsCollected.fill(0);
    CollectedPickUpIndex = 0u;
    DisplayHelpMessage = 10u;
}

// 0x456E60
void CPickups::ReInit() {
    rng::for_each(GetAllActivePickups(), [](auto& pickup) { pickup.Remove(); });
    Init();
}

// 0x455240
void CPickups::AddToCollectedPickupsArray(int32 pickupIndex) {
    aPickUpsCollected[CollectedPickUpIndex++] = GetUniquePickupIndex(pickupIndex).num;
    CollectedPickUpIndex %= std::size(aPickUpsCollected);
}

/*!
 * @addr 0x458A80
 * @brief Created a pickup close to pos (inX, inY, inZ)
 *
 * @param [out] outX, outY, outZ Created pickup's position
 */
void CPickups::CreatePickupCoorsCloseToCoors(float inX, float inY, float inZ, float& outX, float& outY, float& outZ) {
    for (int32 i = 0; i < 32; i++) {
        // Candidate is on a circle of radius 1.5 around the given position, in one of 256 directions
        const float angle = (float)(rand() & 0xFF) * 0.024543693f; // 2PI / 256
        const float x     = inX + std::sinf(angle) * 1.5f;
        const float y     = inY + std::cosf(angle) * 1.5f;

        bool        groundFound;
        const float z = CWorld::FindGroundZFor3DCoord({ x, y, inZ }, &groundFound) + 0.5f;
        if (!groundFound) {
            continue;
        }

        // Line of sight is checked from slightly above the original position to 0.4 units past the candidate
        const CVector start{ inX, inY, inZ + 0.3f };
        const CVector delta{ x - start.x, y - start.y, z - start.z };
        const float   len   = delta.Magnitude();
        const CVector end   = start + delta * ((len + 0.4f) / len);

        if (const auto playerPos = FindPlayerCoors(); DistanceBetweenPoints2D(CVector2D{ x, y }, playerPos) > 2.0f) {
            // Far enough from the player, but don't create it next to another pickup (During the first 17 attempts at least)
            if (i <= 16 && TestForPickupsInBubble({ x, y, z }, 1.3f)) {
                continue;
            }
        } else if (i <= 16) {
            // Too close to the player - Only accept it once 17 attempts have been made
            continue;
        }

        if (!CWorld::GetIsLineOfSightClear(end, start, true, i < 16, false, i < 16, false, false, false)) {
            continue;
        }

        if (i <= 16 && CWorld::TestSphereAgainstWorld({ x, y, z }, 1.2f, nullptr, false, true, false, false, false, false)) {
            continue;
        }

        outX = x;
        outY = y;
        outZ = z;
        return;
    }

    // No suitable position was found
    outX = inX;
    outY = inY;
    outZ = inZ + 0.4f;
}

/*!
 * @addr 0x458970
 * @brief Creates wads of money that is worth amount of money at the position coors.
 */
void CPickups::CreateSomeMoney(CVector coors, int32 amount) {
    const auto wads = std::min(amount / 20 + 1, 7);
    const auto perWad = amount / wads;

    for (auto i = 0; i < wads; i++) {
        bool result;
        coors.x += std::sinf(CGeneral::GetRandomNumberInRange(0.f, TWO_PI)) * 1.5f;
        coors.y += std::cosf(CGeneral::GetRandomNumberInRange(0.f, TWO_PI)) * 1.5f;
        coors.z = CWorld::FindGroundZFor3DCoord(coors, &result, nullptr) + 0.5f;

        if (result) {
            GenerateNewOne(coors, MI_MONEY, PICKUP_MONEY, perWad + (CGeneral::GetRandomNumber() % 4));
        }
    }
}

// 0x4590C0
void CPickups::DetonateMinesHitByGunShot(const CVector& shotOrigin, const CVector& shotTarget) {
    for (auto& pickup : aPickUps) {
        if (pickup.m_nPickupType == PICKUP_NAUTICAL_MINE_ARMED) {
            pickup.ProcessGunShot(shotOrigin, shotTarget);
        }
    }
}

// 0x455E20
void CPickups::DoCollectableEffects(CEntity* entity) {
    const auto& entityPos = entity->GetPosition();

    if (const auto d = DistanceBetweenPoints(TheCamera.GetPosition(), entityPos); d < 14.0f) {
        // shade of gray
        const auto t = (uint8)((std::sinf((float)(((uint16)std::bit_cast<uintptr_t>(entity) + (uint16)CTimer::GetTimeInMS()) % 2048) * 0.0030664064f) + 1.0f) / 2.0f * ((14.0f - d) / 14.0f) * 255.0f);

        CShadows::StoreStaticShadow(
            notsa::PtrToId(entity),
            SHADOW_ADDITIVE,
            gpShadowExplosionTex,
            entityPos,
            2.0f,
            0.0f,
            0.0f,
            -2.0f,
            0,
            t,
            t,
            t,
            4.0f,
            1.0f,
            40.0f,
            false,
            0.0f
        );
        CCoronas::RegisterCorona(
            notsa::PtrToId(entity),
            nullptr,
            t,
            t,
            t,
            255,
            entityPos,
            0.6f,
            40.0f,
            CORONATYPE_TORUS,
            FLARETYPE_NONE,
            eCoronaReflType::CORREFL_NONE,
            eCoronaLOSCheck::LOSCHECK_OFF,
            eCoronaTrail::TRAIL_OFF,
            0.0f,
            false,
            1.5f,
            0,
            15.0f,
            false,
            false
        );
    }

    entity->GetMatrix().SetRotateZOnly(static_cast<float>(CTimer::GetTimeInMS() % 4096) * 0.0015283204f);
}

// 0x4560E0
void CPickups::DoMineEffects(CEntity* entity) {
    const auto& entityPos = entity->GetPosition();

    if (const auto d = DistanceBetweenPoints(TheCamera.GetPosition(), entityPos); d < 20.0f) {
        // shade of red
        const auto t = (uint8)((std::sinf((float)(((uint16)std::bit_cast<uintptr_t>(entity) + (uint16)CTimer::GetTimeInMS()) % 512) * 0.012265625f) + 1.0f) / 2.0f * ((20.0f - d) / 20.0f) * 64.0f);

        CShadows::StoreStaticShadow(
            notsa::PtrToId(entity),
            SHADOW_ADDITIVE,
            gpShadowExplosionTex,
            entityPos,
            2.0f,
            0.0f,
            0.0f,
            -2.0f,
            0,
            t,
            0,
            0,
            4.0f,
            1.0f,
            40.0f,
            false,
            0.0f
        );
        CCoronas::RegisterCorona(
            notsa::PtrToId(entity),
            nullptr,
            t,
            0,
            0,
            255,
            entityPos,
            0.6f,
            40.0f,
            CORONATYPE_TORUS,
            FLARETYPE_NONE,
            eCoronaReflType::CORREFL_NONE,
            eCoronaLOSCheck::LOSCHECK_OFF,
            eCoronaTrail::TRAIL_OFF,
            0.0f,
            false,
            1.5f,
            0,
            15.0f,
            false,
            false
        );
    }

    entity->GetMatrix().SetRotateZOnly(static_cast<float>(CTimer::GetTimeInMS() % 1024) * 0.0061132815f);
}

// 0x454E80
void CPickups::DoMoneyEffects(CEntity* entity) {
    entity->GetMatrix().SetRotateZOnly(static_cast<float>(CTimer::GetTimeInMS() % 2048) * 0.0030566407f);
}

// 0x455720
void CPickups::DoPickUpEffects(CEntity* entity) {
    auto* const object = static_cast<CObject*>(entity);
    auto* const pickup = FindPickUpForThisObject(object);

    if (entity->m_nModelIndex == MI_PICKUP_CAMERA) {
        if (TheCamera.GetActiveCam().m_nMode == MODE_CAMERA) {
            object->objectFlags.bDoNotRender = false;
        } else {
            object->objectFlags.bDoNotRender = true;

            if (CClock::GetGameClockHours() < 5 || CPostEffects::IsVisionFXActive()) {
                // Warm-white flashing light, with a random brightness
                const auto colour = 100 - (int32)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * -50.0f);
                const auto radius = (std::sinf((float)(CTimer::GetTimeInMS() & 0x1FFF) * 0.0007666016f) + 1.7f) * 3.7f;

                CCoronas::RegisterCorona(
                    notsa::PtrToId(entity), // The original passes the address of a stack variable here
                    nullptr,
                    (uint8)colour,
                    (uint8)(int32)((float)colour * 0.7f),
                    (uint8)(int32)((float)colour * 0.7f),
                    255,
                    entity->GetPosition(),
                    radius,
                    100.0f,
                    CORONATYPE_HEADLIGHT,
                    FLARETYPE_NONE,
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
        }
    } else {
        object->objectFlags.bDoNotRender = pickup->PickUpShouldBeInvisible();
    }

    if (object->objectFlags.bDoNotRender) {
        return;
    }

    // Colours of the on-screen messages
    // NOTE: The original never assigns a colour index for `MI_PICKUP_BRIBE`, `MI_PICKUP_INFO`, `MI_PICKUP_KILLFRENZY`,
    //       `MI_PICKUP_PROPERTY` and `MI_PICKUP_SAVEGAME` (The variable ends up aliased with the already-consumed argument slot),
    //       so for those the colour index is the low 16 bits of the entity pointer, and the colour is read from out-of-bounds memory.
    //       This (deterministic) behaviour is preserved.
    const int32 colourIdx = [&]() -> int32 {
        const auto model = entity->m_nModelIndex;
        if (   model == MI_PICKUP_ADRENALINE
            || model == MI_PICKUP_HEALTH
            || model == MI_PICKUP_BONUS
            || model == MI_PICKUP_PROPERTY_FORSALE
            || model == MI_PICKUP_CLOTHES
        ) {
            return 0x2F;
        } else if (model == MI_PICKUP_BODYARMOUR) {
            return 0x30;
        } else if (model == MI_PICKUP_REVENUE) {
            return 0x35;
        } else if (   model == MI_PICKUP_BRIBE
                   || model == MI_PICKUP_INFO
                   || model == MI_PICKUP_KILLFRENZY
                   || model == MI_PICKUP_PROPERTY
                   || model == MI_PICKUP_SAVEGAME
        ) {
            return (int32)(int16)(uintptr_t)entity;
        }
        return (int32)WeaponForModel(model);
    }();

    const auto& colour = aMessageColours[colourIdx];

    // Add a screen message if there's anything to show
    if (   object->objectFlags.bPickupPropertyForSale
        || object->objectFlags.bPickupInShopOutOfStock
        || object->m_nBonusValue != 0
        || object->m_wCostValue != 0
    ) {
        const auto dist = DistanceBetweenPoints2D(TheCamera.GetPosition(), entity->GetPosition());
        if (dist < 14.0f && (uint32)NumMessages < MAX_PICKUP_MESSAGES) {
            const CVector msgPos = entity->GetPosition() + CVector{ 0.0f, 0.0f, 0.7f };

            CVector scrPos;
            float   width, height;
            if (CSprite::CalcScreenCoors(msgPos, &scrPos, &width, &height, true, true)) {
                auto& msg = aMessages[NumMessages];

                msg.pos.x  = scrPos.x;
                msg.pos.y  = scrPos.y;
                msg.pos.z  = std::bit_cast<float>((uint32)WeaponForModel(entity->m_nModelIndex)); // NB: The weapon id is stored over `pos.z`
                msg.width  = width;
                msg.height = height;
                msg.color.r = colour.colour.r;
                msg.color.g = colour.colour.g;
                msg.color.b = colour.colour.b;
                msg.color.a = (uint8)(255.0f * (1.0f - dist * (1.0f / 14.0f)));
                // NB: Flags are read-modified-written, exactly like in the original
                msg.flags = (uint8)(msg.flags & ~1);
                if (object->objectFlags.bPickupInShopOutOfStock) {
                    msg.flags = (uint8)(msg.flags | 1);
                }
                msg.field_19 = (char)object->m_nBonusValue;
                msg.price    = object->m_wCostValue * 5u;

                if (entity->m_nModelIndex == MI_PICKUP_PROPERTY) {
                    msg.text  = const_cast<GxtChar*>(TheText.Get(CPickup::FindStringForTextIndex((ePickupPropertyText)pickup->m_nFlags.nPropertyTextIndex)));
                    msg.flags = (uint8)(msg.flags & ~2);
                } else if (entity->m_nModelIndex == MI_PICKUP_PROPERTY_FORSALE) {
                    msg.text  = const_cast<GxtChar*>(TheText.Get(CPickup::FindStringForTextIndex((ePickupPropertyText)pickup->m_nFlags.nPropertyTextIndex)));
                    msg.flags = (uint8)(msg.flags | 2);
                } else {
                    msg.text  = nullptr;
                    msg.flags = (uint8)(msg.flags & ~2);
                }

                NumMessages++;
            }
        }
    }

    // Scale the pickup so it's not too large for its collision box
    const auto& bb = CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel()->m_boundBox;
    float scale = 1.2f / std::max({ bb.m_vecMax.x - bb.m_vecMin.x, bb.m_vecMax.y - bb.m_vecMin.y, bb.m_vecMax.z - bb.m_vecMin.z });
    scale = std::max(scale, 1.0f);
    scale = (scale - 1.0f) * 0.6f + 1.0f;
    if (entity->m_nModelIndex == MODEL_MINIGUN) {
        scale = 1.2f;
    }

    // Spin it around the Z axis
    const auto angle = (float)(CTimer::GetTimeInMS() & 0x7FF) * 0.0030566407f;
    const auto sinA  = std::sinf(angle);
    const auto cosA  = std::cosf(angle);

    auto& mat = entity->GetMatrix();
    mat.GetRight()   = CVector( cosA * scale, sinA * scale, 0.0f);
    mat.GetForward() = CVector(-sinA * scale, cosA * scale, 0.0f);
    mat.GetUp()      = CVector( 0.0f,         0.0f,         scale);
}

// 0x4551C0
CPickup* CPickups::FindPickUpForThisObject(CObject* object) {
    for (auto& pickup : GetAllActivePickups()) {
        if (pickup.m_pObject == object) {
            return &pickup;
        }
    }
    return aPickUps.data();
}

// returns pickup handle
// 0x456F20
tPickupReference CPickups::GenerateNewOne(CVector coors, uint32 modelId, ePickupType pickupType, uint32 ammo, uint32 moneyPerDay, bool isEmpty, char* message) {
    // Find a slot for the new pickup
    int32 slotIndex = -1;

    // These kinds of pickups are searched for in reverse order, so they end up in the last free slots
    if (pickupType == PICKUP_FLOATINGPACKAGE || pickupType == PICKUP_NAUTICAL_MINE_INACTIVE || isEmpty) {
        for (int32 i = (int32)MAX_NUM_PICKUPS - 1; i >= 0; i--) {
            if (aPickUps[i].m_nPickupType == PICKUP_NONE) {
                slotIndex = i;
                break;
            }
        }
    }

    if (slotIndex < 0) {
        for (int32 i = 0; i < (int32)MAX_NUM_PICKUPS; i++) {
            if (aPickUps[i].m_nPickupType == PICKUP_NONE) {
                slotIndex = i;
                break;
            }
        }
    }

    // No free slot, so an existing pickup is recycled instead (Deleting its object first)
    if (slotIndex < 0) {
        for (int32 i = 0; i < (int32)MAX_NUM_PICKUPS; i++) {
            if (aPickUps[i].m_nPickupType == PICKUP_MONEY) {
                slotIndex = i;
                break;
            }
        }

        if (slotIndex < 0) {
            for (int32 i = 0; i < (int32)MAX_NUM_PICKUPS; i++) {
                const auto type = aPickUps[i].m_nPickupType;
                if (type == PICKUP_ONCE_TIMEOUT || type == PICKUP_ONCE_TIMEOUT_SLOW) {
                    slotIndex = i;
                    break;
                }
            }
        }

        if (slotIndex < 0) {
            return tPickupReference(-1);
        }

        aPickUps[slotIndex].GetRidOfObjects();
    }

    auto& pickup = aPickUps[slotIndex];

    pickup.m_nAmmo        = ammo;
    pickup.m_nMoneyPerDay = (uint16)moneyPerDay;
    pickup.m_nFlags.bDisabled             = false;
    pickup.m_nFlags.bEmpty                = isEmpty;
    pickup.m_nFlags.bHelpMessageDisplayed = false;
    pickup.m_nPickupType   = pickupType;
    pickup.m_fRevenueValue = 0.0f;

    const auto time = CTimer::GetTimeInMS();
    pickup.m_nRegenerationTime = time;

    switch (pickupType) {
    case PICKUP_ONCE_TIMEOUT:
        pickup.m_nRegenerationTime = time + 20'000;
        break;
    case PICKUP_ONCE_TIMEOUT_SLOW:
        pickup.m_nRegenerationTime = time + 120'000;
        break;
    case PICKUP_MONEY:
        pickup.m_nRegenerationTime = time + 30'000;
        break;
    case PICKUP_MINE_INACTIVE:
    case PICKUP_MINE_ARMED:
        pickup.m_nPickupType       = PICKUP_MINE_INACTIVE;
        pickup.m_nRegenerationTime = time + 1'500;
        break;
    case PICKUP_NAUTICAL_MINE_INACTIVE:
    case PICKUP_NAUTICAL_MINE_ARMED:
        pickup.m_nPickupType       = PICKUP_NAUTICAL_MINE_INACTIVE;
        pickup.m_nRegenerationTime = time + 1'500;
        break;
    default:
        break;
    }

    pickup.m_nModelIndex = (int16)modelId;

    pickup.m_nFlags.nPropertyTextIndex = (uint8)CPickup::FindTextIndexForString(message);

    // Compress the position (Truncating conversion, just like the exe's x87 -> int64 helper)
    pickup.m_vecPos = coors;

    pickup.m_nFlags.bVisible = pickup.IsVisible();

    pickup.m_pObject = nullptr;
    if (pickup.m_nFlags.bVisible) {
        pickup.GiveUsAPickUpObject(pickup.m_pObject, -1);
        if (pickup.m_pObject) {
            CWorld::Add(pickup.m_pObject);
        }
    }

    // Bump the reference index so that the old handles of this slot become invalid
    auto& refIndex = pickup.m_nReferenceIndex;
    if ((uint16)refIndex < 0xFFFE) {
        refIndex++;
    } else {
        refIndex = 1;
    }

    return tPickupReference((int16)slotIndex, refIndex);
}

/*!
 *
 * @param coors Position of new pickup
 * @param weaponType Weapon type
 * @param pickupType Pickup type
 * @param ammo
 * @param isEmpty
 * @param message
 * @return Pickup handle
 * @addr 0x457380
 */
tPickupReference CPickups::GenerateNewOne_WeaponType(CVector coors, eWeaponType weaponType, ePickupType pickupType, uint32 ammo, bool isEmpty, char* message) {
    return GenerateNewOne(coors, CWeaponInfo::GetWeaponInfo(weaponType)->m_nModelId1, pickupType, ammo, 0u, isEmpty, message);
}

/*!
 * @param pickupIndex Index of pickup
 * @return -1 if this index is not actual
 * @addr 0x4552A0
 */
int32 CPickups::GetActualPickupIndex(tPickupReference pickupRef) {
    if (pickupRef.num == -1)
        return -1;

    if (pickupRef.refIndex != aPickUps.at(pickupRef.index).m_nReferenceIndex)
        return -1;

    return pickupRef.index;
}

// 0x456A30
tPickupReference CPickups::GetNewUniquePickupIndex(int32 pickupIndex) {
    auto& refIdx = aPickUps[pickupIndex].m_nReferenceIndex;
    refIdx = (refIdx == -1) ? 1 : refIdx + 1;

    return GetUniquePickupIndex(pickupIndex);
}

// returns pickup handle
// 0x455280
tPickupReference CPickups::GetUniquePickupIndex(int32 pickupIndex) {
    return tPickupReference(pickupIndex, aPickUps.at(pickupIndex).m_nReferenceIndex);
}

// returns TRUE if player got goodies
// 0x4564F0
bool CPickups::GivePlayerGoodiesWithPickUpMI(uint16 modelId, int32 playerId) {
    auto* ped = FindPlayerPed(playerId);

    if (modelId == MI_PICKUP_ADRENALINE) {
        ped->GetPlayerData()->m_bAdrenaline = true;
        ped->GetPlayerData()->m_nAdrenalineEndTime = CTimer::GetTimeInMS() + 20'000;
        ped->ResetSprintEnergy();
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_ADRENALINE);
        return true;
    }

    if (modelId == MI_PICKUP_BODYARMOUR) {
        ped->m_fArmour = (float)FindPlayerInfo(playerId).m_nMaxArmour;
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_BODY_ARMOUR);
        return true;
    }

    if (modelId == MI_PICKUP_INFO) {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MI_PICKUP_HEALTH) {
        auto maxHealth = FindPlayerInfo(playerId).m_nMaxHealth;
        CStats::UpdateStatsAddToHealth((uint32)((float)maxHealth - ped->m_fHealth));
        ped->m_fHealth = (float)maxHealth;
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_HEALTH);
        return true;
    }

    if (modelId == MI_PICKUP_BONUS) {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MI_PICKUP_BRIBE) {
        auto wantedLevel = std::max(+eWantedLevel::WANTED_CLEAN, +FindPlayerPed()->GetWantedLevel() - +eWantedLevel::WANTED_LEVEL_1);
        FindPlayerPed(0)->SetWantedLevel((eWantedLevel)wantedLevel);
        CStats::IncrementStat(STAT_NUMBER_OF_POLICE_BRIBES, 1.0f);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MI_PICKUP_KILLFRENZY) {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MODEL_JETPACK) {
        auto* task = new CTaskSimpleJetPack(nullptr, 10.0f, 0, nullptr);
        CEventScriptCommand event(3, task, 0);
        ped->GetEventGroup().Add(&event, false);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_INFO);
        return true;
    }

    if (modelId == MI_OYSTER) {
        PickedUpOyster();
        return true;
    }

    if (modelId == MI_HORSESHOE) {
        PickedUpHorseShoe();
        return true;
    }

    return false;
}

/*!
 * @brief Check if pickup was picked up, and then mark it as not picked up.
 * @addr 0x454B40
 */
bool CPickups::IsPickUpPickedUp(tPickupReference pickupRef) {
    if (const auto it = rng::find(aPickUpsCollected, pickupRef.num); it != aPickUpsCollected.end()) {
        *it = 0; // Reset
        return true;
    }
    return false;
}

/*!
 * @addr 0x454AC0
 * @returns The `nModelId1` of the given weapon type.
 */
int32 CPickups::ModelForWeapon(eWeaponType weaponType) {
    return CWeaponInfo::GetWeaponInfo(weaponType)->m_nModelId1;
}

/*!
 * @brief Update each pickup's (except if of type NONE or ASSET_REVENUE) `nRegenerationTime` field.
 * @addr 0x455200
 */
void CPickups::PassTime(uint32 time) {
    for (auto& pickup : aPickUps) {
        switch (pickup.m_nPickupType) {
        case PICKUP_NONE:
        case PICKUP_ASSET_REVENUE:
            continue;
        }

        if (pickup.m_nRegenerationTime <= time) {
            pickup.m_nRegenerationTime = 0;
        } else {
            pickup.m_nRegenerationTime -= time;
        }
    }
}

// 0x455390
void CPickups::PickedUpHorseShoe() {
    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_COLLECTABLE1);

    CStats::IncrementStat(STAT_HORSESHOES_COLLECTED);
    CStats::IncrementStat(STAT_LUCK, 1000.f / CStats::GetStatValue(STAT_TOTAL_HORSESHOES)); // TODO: Is this some inlined stuff? (The division part)

    FindPlayerInfo().m_nMoney += 100; // originally rewarded to the player 1.

    const auto collected = CStats::GetStatValue(STAT_HORSESHOES_COLLECTED);
    const auto total = CStats::GetStatValue(STAT_TOTAL_HORSESHOES);
    if (collected == total) {
        CGarages::TriggerMessage("HO_ALL");
        FindPlayerInfo().m_nMoney += 100'000; // originally rewarded to the player 1.
    } else {
        CGarages::TriggerMessage("HO_ONE", static_cast<int16>(collected), 5000u, static_cast<int16>(total));
    }
}

// 0x4552D0
void CPickups::PickedUpOyster() {
    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_COLLECTABLE1);

    CStats::IncrementStat(STAT_OYSTERS_COLLECTED);
    FindPlayerInfo().m_nMoney += 100; // originally rewarded to the player 1.

    const auto collected = CStats::GetStatValue(STAT_OYSTERS_COLLECTED);
    const auto total = CStats::GetStatValue(STAT_TOTAL_OYSTERS);
    if (collected == total) {
        CGarages::TriggerMessage("OY_ALL");
        FindPlayerInfo().m_nMoney += 100'000; // originally rewarded to the player 1.
    } else {
        CGarages::TriggerMessage("OY_ONE", static_cast<int16>(collected), 5000u, static_cast<int16>(total));
    }
}

// 0x456A70
void CPickups::PictureTaken() {
    std::optional<size_t> capturedPickup{};
    auto lastFoundDist = 999'999.88f; // maybe FLT_MAX

    for (auto&& [i, pickup] : rngv::enumerate(aPickUps)) {
        if (pickup.m_nPickupType != PICKUP_SNAPSHOT)
            continue;

        const auto pupPos = pickup.GetPosn();
        const auto dist = DistanceBetweenPoints(TheCamera.GetPosition(), pupPos);

        if (90.0f / TheCamera.FindCamFOV() * 20.0f > dist && dist < lastFoundDist) {
            if (TheCamera.IsSphereVisible(pupPos, 0.2f) || TheCamera.IsSphereVisibleInMirror(pupPos, 0.2f)) {
                capturedPickup = i;
                lastFoundDist = dist;
            }
        }
    }

    if (!capturedPickup.has_value())
        return;

    aPickUps[*capturedPickup].Remove();

    FindPlayerInfo().m_nMoney += 100'000; // originally rewarded to the player 1.

    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_COLLECTABLE1);
    CStats::IncrementStat(STAT_SNAPSHOTS_TAKEN, 1.0f);

    const auto taken = CStats::GetStatValue(STAT_SNAPSHOTS_TAKEN);
    const auto total = CStats::GetStatValue(STAT_TOTAL_SNAPSHOTS);
    if (taken == total) {
        CGarages::TriggerMessage("SN_ALL");
        FindPlayerInfo().m_nMoney += 100'000;
    } else {
        CGarages::TriggerMessage("SN_ONE", static_cast<int16>(taken), 5000u, static_cast<int16>(total));
    }
}

// 0x4554C0
bool CPickups::PlayerCanPickUpThisWeaponTypeAtThisMoment(eWeaponType weaponType) {
    if (!CWeaponInfo::GetWeaponInfo(weaponType)->flags.bAimWithArm) {
        if (FindPlayerPed()->GetIntelligence()->GetTaskJetPack()) {
            return false;
        }
    }
    return true;
}

// 0x456DE0
void CPickups::RemoveMissionPickUps() {
    for (auto&& [i, pickup] : rngv::enumerate(aPickUps)) {
        switch (pickup.m_nPickupType) {
        case PICKUP_ONCE_FOR_MISSION: {
            CRadar::ClearBlipForEntity(BLIP_PICKUP, GetUniquePickupIndex(i).num);
            pickup.GetRidOfObjects();

            pickup.m_nFlags.bDisabled = true;
            pickup.m_nPickupType = PICKUP_NONE;
            break;
        }
        }
    }
}

// 0x4573D0
void CPickups::RemovePickUp(tPickupReference pickupRef) {
    if (const auto i = GetActualPickupIndex(pickupRef); i != -1) {
        aPickUps[i].Remove();
    }
}

// 0x456D30
void CPickups::RemovePickUpsInArea(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
    CBoundingBox bb{ { minX, minY, minZ }, { maxX, maxY, maxZ } }; // They didn't use a bounding box, but it's nicer to do so.

    for (auto& pickup : GetAllActivePickups()) {
        if (bb.IsPointWithin(pickup.GetPosn())) {
            pickup.Remove();
        }
    }
}

// 0x455470
void CPickups::RemovePickupObjects() {
    for (auto& pickup : GetAllActivePickups()) {
        if (pickup.m_pObject) {
            pickup.GetRidOfObjects();
            pickup.m_nFlags.bVisible = false;
        }
    }
}

// remove pickups with types PICKUP_ONCE_TIMEOUT and PICKUP_MONEY in area
// 0x4563A0
void CPickups::RemoveUnnecessaryPickups(const CVector& posn, float radius) {
    for (auto& pickup : aPickUps) {
        switch (pickup.m_nPickupType) {
        case PICKUP_ONCE_TIMEOUT:
        case PICKUP_MONEY: {
            if (IsPointInSphere(pickup.GetPosn(), posn, radius)) {
                pickup.Remove();
            }
            break;
        }
        }
    }
}

// 0x455000
void CPickups::RenderPickUpText() {
    GxtChar msgText[352]{};
    for (const auto& message : std::span{ aMessages.data(), NumMessages }) {
        if (message.price == 0u) {
            if (!message.text)
                continue;

            if (message.flags & 2) {
                CMessages::InsertNumberInString(message.text, 0, 0, 0, 0, 0, 0, msgText);
            }
        } else {
            AsciiToGxtChar(std::format("${:d}", message.price).c_str(), msgText);
        }

        // TODO: scaled wrong in windowed mode, but it's fine in fullscreen.
        auto scaleX = std::min(SCREEN_STRETCH_X(1.0f), message.width / 30.0f);
        auto scaleY = std::min(SCREEN_STRETCH_X(1.0f), message.height / 30.0f);

        CFont::SetProportional(true);
        CFont::SetBackground(false, false);
        CFont::SetScale(scaleX, scaleY);
        CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);
        CFont::SetCentreSize(SCREEN_WIDTH);
        CFont::SetColor(message.color);
        CFont::SetFontStyle(eFontStyle::FONT_PRICEDOWN);
        CFont::PrintString(message.pos.x, message.pos.y, msgText);
    }
    NumMessages = 0;
}

// check for pickups in area
// 0x456450
bool CPickups::TestForPickupsInBubble(const CVector posn, float radius) {
    // NOTE: (Possible bug) - They dont check if the pickup is active (eg: type != NONE)
    return rng::any_of(aPickUps, [sp = CSphere{ posn, radius }](const CPickup& p) {
        return sp.IsPointWithin(p.GetPosn()); // They obviously didn't use `CSphere` here, but it's nicer.
    });
}

// search for pickup in area (radius = 5.5 units) with this weapon model and pickup type and add ammo to this pickup; returns TRUE if merged
// 0x4555A0
bool CPickups::TryToMerge_WeaponType(CVector posn, eWeaponType weaponType, ePickupType pickupType, uint32 ammo, bool arg4) {
    const auto mi = CWeaponInfo::GetWeaponInfo(weaponType)->m_nModelId1;

    for (auto& pickup : aPickUps) {
        if (mi == pickup.m_nModelIndex && pickup.m_nPickupType == pickupType && IsPointInSphere(pickup.GetPosn(), posn, 5.0f)) {
            pickup.m_nAmmo += ammo;

            return true;
        }
    }
    return false;
}

// 0x458DE0
void CPickups::Update() {
    ZoneScoped;

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    auto start = 620 * (CTimer::GetFrameCounter() % 32) / 32;
    auto end   = 620 * (CTimer::GetFrameCounter() % 32 + 1) / 32;
    for (auto i = start; i < end; i++) {
        auto& pickup = aPickUps[i];
        if (pickup.m_nPickupType == PICKUP_NONE)
            continue;

        if (pickup.m_nFlags.bVisible = pickup.IsVisible()) {
            if (!pickup.m_nFlags.bDisabled && !pickup.m_pObject) {
                pickup.GiveUsAPickUpObject(pickup.m_pObject);

                if (auto& obj = pickup.m_pObject; obj) {
                    CWorld::Add(obj);
                }
            }
        } else {
            pickup.GetRidOfObjects();
        }
    }

    const auto pad = CPad::GetPad();
    if (pad->CollectPickupJustDown()) {
        CollectPickupBuffer = 6;
    } else if (CollectPickupBuffer) {
        CollectPickupBuffer--;
    }

    if (PlayerOnWeaponPickup) {
        PlayerOnWeaponPickup--;
    }

    if (pad->GetTarget()) {
        CollectPickupBuffer = 0;
    }

    const auto player1 = FindPlayerPed(PED_TYPE_PLAYER1);
    const auto p1Busy = player1->GetIntelligence()->FindTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER) || player1->GetIntelligence()->FindTaskByType(TASK_COMPLEX_USE_MOBILE_PHONE);

    start = 620 * (CTimer::GetFrameCounter() % 6) / 6;
    end   = 620 * (CTimer::GetFrameCounter() % 6 + 1) / 6;
    for (auto i = start; i < end; i++) {
        auto& pickup = aPickUps[i];

        if (pickup.m_nPickupType == PICKUP_NONE || !pickup.m_nFlags.bVisible)
            continue;

        if (!p1Busy) {
            if (pickup.Update(FindPlayerPed(), FindPlayerVehicle(), CWorld::PlayerInFocus)) {
                AddToCollectedPickupsArray(i);
            }
        } else if (FindPlayerPed(PED_TYPE_PLAYER2)) {
            if (pickup.Update(FindPlayerPed(1), FindPlayerVehicle(1), PED_TYPE_PLAYER2)) {
                AddToCollectedPickupsArray(i);
            }
        }
    }
}

// 0x455680
void CPickups::UpdateMoneyPerDay(tPickupReference pickupRef, uint16 money) {
    if (auto idx = GetActualPickupIndex(pickupRef); idx != -1) {
        aPickUps[idx].m_nMoneyPerDay = money;
    }
}

// 0x454AE0
eWeaponType CPickups::WeaponForModel(int32 modelId) {
    if (modelId == MI_PICKUP_BODYARMOUR) {
        return WEAPON_ARMOUR;
    }
    if (modelId == MI_PICKUP_HEALTH) {
        return WEAPON_LAST_WEAPON;
    }
    if (modelId == MI_PICKUP_ADRENALINE) {
        return WEAPON_ARMOUR;
    }

    switch (modelId) {
    case MODEL_JETPACK:
        return WEAPON_LAST_WEAPON;

    case MODEL_INVALID:
        return WEAPON_UNARMED;
    }

    if (auto mi = CModelInfo::GetModelInfo(modelId); mi->GetModelType() == MODEL_INFO_WEAPON) {
        return mi->AsWeaponModelInfoPtr()->GetWeaponInfo();
    }

    return WEAPON_UNARMED;
}

// 0x5D35A0
void CPickups::Load() {
    for (auto& pickup : aPickUps) {
        CGenericGameStorage::LoadDataFromWorkBuffer(pickup);
        if (pickup.m_nPickupType != PICKUP_NONE && pickup.m_pObject) {
            pickup.m_pObject = nullptr;
            pickup.m_nFlags.bVisible = false;
        }
    }
    NumMessages = 0u;
    CGenericGameStorage::LoadDataFromWorkBuffer(CPickups::CollectedPickUpIndex);
    CGenericGameStorage::LoadDataFromWorkBuffer(CPickups::DisplayHelpMessage);

    for (auto& collected : aPickUpsCollected) {
        CGenericGameStorage::LoadDataFromWorkBuffer(collected);
    }
}
// 0x5D3540
void CPickups::Save() {
    for (auto& pickup : aPickUps) {
        CGenericGameStorage::SaveDataToWorkBuffer(pickup);
    }
    CGenericGameStorage::SaveDataToWorkBuffer(CPickups::CollectedPickUpIndex);
    CGenericGameStorage::SaveDataToWorkBuffer(CPickups::DisplayHelpMessage);

    for (auto& collected : aPickUpsCollected) {
        CGenericGameStorage::SaveDataToWorkBuffer(collected);
    }
}

// 0x454B70
void ModifyStringLabelForControlSetting(char* stringLabel) {
    const auto len = strlen(stringLabel);

    if (len < 2 || stringLabel[len - 2] != '_')
        return;

    switch (CPad::GetPad(0)->Mode) {
    case 0:
    case 1:
        stringLabel[len - 1] = 'L';
        break;
    case 2:
        stringLabel[len - 1] = 'T';
        break;
    case 3:
        stringLabel[len - 1] = 'C';
        break;
    }
}
