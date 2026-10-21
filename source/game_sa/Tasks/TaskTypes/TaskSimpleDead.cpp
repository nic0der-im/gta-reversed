#include "StdInc.h"

#include "TaskSimpleDead.h"

#include <extensions/utility.hpp>

#include "AccidentManager.h"
#include "AnimBlendAssociation.h"
#include "AnimManager.h"
#include "CarEnterExit.h"
#include "EntityScanner.h"
#include "Enums/eTargetDoor.h"
#include "Events/EventDeadPed.h"
#include "Events/EventGlobalGroup.h"
#include "Localisation.h"
#include "Ped.h"
#include "PedIK.h"
#include "PedIntelligence.h"
#include "PlayerPed.h"
#include "RwHelper.h"
#include "Shadows.h"
#include "Timer.h"
#include "WaterLevel.h"
#include "WeaponInfo.h"
#include "World.h"

void CTaskSimpleDead::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleDead, 0x86DEA4, 9);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(ProcessPed, 0x630600);
}

// NOTSA: *deathTime* originally int32
// 0x630590
CTaskSimpleDead::CTaskSimpleDead(uint32 deathTime, bool hasDrowned) :
    m_nDeathTimeMS{deathTime},
    m_bHasDrowned{hasDrowned}
{
}

// 0x636100
CTaskSimpleDead::CTaskSimpleDead(const CTaskSimpleDead& o) :
    CTaskSimpleDead{o.m_nDeathTimeMS, o.m_bHasDrowned}
{
}

// 0x630600
bool CTaskSimpleDead::ProcessPed(CPed* ped) {
    // Both of these are only ever set on the first update (They're the `BL` flag in the original)
    bool standingOrWasStanding = false;
    bool playerTargetsPed = false;

    if (m_bFirstTime) {
        if (ped->bInVehicle) {
            const auto door = static_cast<eTargetDoor>(CCarEnterExit::ComputeTargetDoorToExit(ped->m_pVehicle, ped));
            CAnimManager::BlendAnimation(
                ped->GetRpClump(),
                ANIM_GROUP_DEFAULT,
                door == TARGET_DOOR_DRIVER || door == TARGET_DOOR_REAR_LEFT
                    ? ANIM_ID_CAR_DEAD_LHS
                    : ANIM_ID_CAR_DEAD_RHS,
                4.0f
            );
        }

        ped->SetPedState(PEDSTATE_DEAD);
        m_bFirstTime = false;

        standingOrWasStanding = ped->bIsStanding || ped->bWasStanding;

        if (FindPlayerPed(0)->m_pTargetedObject == ped) {
            playerTargetsPed = true;
        } else if (const auto player1 = FindPlayerPed(1); player1 && player1->m_pTargetedObject == ped) {
            playerTargetsPed = true;
        }

        if (!m_bHasDrowned && !ped->m_standingOnEntity && !playerTargetsPed) {
            ped->m_bUsesCollision = false;
        }

        ped->m_fHealth = 0.0f;

        const auto* const weaponInfo = CWeaponInfo::GetWeaponInfo(ped->GetActiveWeapon().m_Type, eWeaponSkill::STD);
        ped->RemoveWeaponModel(weaponInfo->m_nModelId1);
        ped->m_nActiveWeaponSlot = 0;

        if (!ped->IsPlayer()) {
            ped->RemoveWeaponAnims(0, -1000.0f);
            ped->CreateDeadPedWeaponPickups();
            ped->CreateDeadPedMoney();
        }

        // 0x630761 - The event is a stack object (It's destroyed once the notifications were sent)
        CEventDeadPed event{ped, m_bHasDrowned, m_nDeathTimeMS};
        GetEventGlobalGroup()->Add(&event, false);
        CAccidentManager::GetInstance()->ReportAccident(ped);
    }

    // 0x6307B5 - The original re-reads the flags here (Bit 0 - `m_bFirstTime` - was cleared above)
    if (!m_bFirstTime && m_bHasDrowned && ped->bIsStanding) {
        // Drowned, but still standing - Lay down
        ped->m_bUsesCollision = false;
        m_bHasDrowned = false;

        auto* const anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_FLOOR_HIT_F, 8.0f);
        if (anim) { // No null check in the original
            anim->SetFlag(ANIMATION_IS_FINISH_AUTO_REMOVE, false); // Keep the animation on its last frame
        }
        goto slope;
    }

    // 0x6307F0
    if (ped->m_bUsesCollision
        && !m_bHasDrowned
        && ped->bIsStanding
        && !ped->m_standingOnEntity
        && FindPlayerPed(0)->m_pTargetedObject != ped
        && !(FindPlayerPed(1) && FindPlayerPed(1)->m_pTargetedObject == ped)
        && !playerTargetsPed
    ) {
        ped->m_bUsesCollision = false;
        goto slope;
    }

    if (!standingOrWasStanding) {
        goto blood;
    }

slope:
    // Original computes these against the matrix' `Up` (offset 0x10, i.e. `CPlaceable::GetForward`) and `Right` (offset 0x0) rows
    ped->m_pedIK.m_fSlopeRoll = std::asin(std::clamp(DotProduct(ped->field_578, ped->GetRight()), -1.0f, 1.0f)); // ped+0x520
    ped->m_pedIK.m_fSlopePitch = std::asin(std::clamp(DotProduct(ped->field_578, ped->GetForward()), -1.0f, 1.0f)); // ped+0x518

blood:
    ped->DeadPedMakesTyresBloody();

    if (CLocalisation::Blood() && !m_bHasDrowned) {
        const auto elapsed = CTimer::GetTimeInMS() - m_nDeathTimeMS;

        // Radius of the puddle (And of the area in which nearby peds start leaving bloody footprints)
        float radius;
        if (elapsed < 2000) {
            radius = 0.0f;
        } else if (elapsed <= 7000) {
            radius = static_cast<float>(elapsed - 2000) * 0.00015f;
        } else {
            radius = 0.75000006f;
        }

        for (auto* const entity : ped->GetIntelligence()->GetPedScanner().m_apEntities) {
            if (!entity) {
                continue;
            }

            auto* const nearbyPed = entity->AsPed();
            if ((nearbyPed->GetPosition() - ped->GetPosition()).SquaredMagnitude() < radius * radius) {
                nearbyPed->m_nDeathTimeMS = 200; // Doubles as the bloody footprints counter
                nearbyPed->bDoBloodyFootprints = true;
            }
        }

        if (elapsed > 2000 && !m_bBloodPuddleCreated) {
            CVector pos = ped->GetPosition();

            // Only check the water level on the frame in which the 2s mark is crossed
            if (elapsed - 2000 <= CTimer::GetTimeInMS() - CTimer::GetPreviousTimeInMS()) {
                float waterLevel;
                if (CWaterLevel::GetWaterLevelNoWaves(pos, &waterLevel, nullptr, nullptr) && pos.z < waterLevel) {
                    m_bBloodPuddleCreated = true; // No puddle when (already) under water
                }
            }

            if (!m_bBloodPuddleCreated && CLocalisation::Blood()) {
                if (elapsed - 2000 < 5000) {
                    CShadows::StoreStaticShadow(
                        notsa::PtrToId(this) + 0x11,
                        SHADOW_DEFAULT, gpBloodPoolTex, pos,
                        static_cast<float>(elapsed - 2000) * 0.00015f,
                        0.0f,
                        0.0f,
                        static_cast<float>(elapsed - 2000) * -0.00015f,
                        0xFF, 200, 0, 0,
                        4.0f, 1.0f, 40.0f, false, 0.0f
                    );
                } else {
                    CShadows::AddPermanentShadow(
                        SHADOW_DEFAULT, gpBloodPoolTex, &pos,
                        0.75000006f, 0.0f, 0.0f, -0.75000006f,
                        0xFF, 200, 0, 0,
                        4.0f, 40000, 1.0f
                    );
                    m_bBloodPuddleCreated = true;
                }
            }
        }
    }

    if (!m_bHasDrowned) {
        ped->m_pedIK.m_nFlags |= 8; // bSlopePitch
        ped->m_vecMoveSpeed = CVector{0.0f, 0.0f, 0.0f};
    } else {
        ped->bIsStanding = false;
        ped->bWasStanding = false;
    }

    return false; // The task is never finished by itself
}
