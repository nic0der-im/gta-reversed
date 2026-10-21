#include "StdInc.h"

#include "PedSaveStructure.h"

#include "EntryExitManager.h"
#include "Streaming.h"
#include "WeaponInfo.h"

//! @notsa
void CWeaponSave32::Construct(const CWeapon& weapon) {
    m_Type                            = weapon.m_Type;
    m_State                           = weapon.m_State;
    m_AmmoInClip                      = weapon.m_AmmoInClip;
    m_TotalAmmo                       = weapon.m_TotalAmmo;
    m_TimeForNextShotMs               = weapon.m_TimeForNextShotMs;
    m_IsFirstPersonWeaponModeSelected = weapon.m_IsFirstPersonWeaponModeSelected;
    m_DontPlaceInHand                 = weapon.m_DontPlaceInHand;
    m_pad0x16[0] = m_pad0x16[1] = 0;

    // The original game writes the raw (runtime) value of this slot, but never reads it back
    // (See `CPedSaveStructure::Extract`), so it's only kept to preserve the file's layout.
    m_nUnused = static_cast<uint32>(reinterpret_cast<uintptr_t>(weapon.m_FxSystem));
}

// 0x5D43D0
void CPedSaveStructure::Construct(CPed* ped) {
    m_pos    = ped->GetPosition();
    m_health = ped->m_fHealth;
    m_armor  = ped->m_fArmour;

    m_createdBy           = static_cast<ePedCreatedBy>(ped->GetCreatedBy());
    m_activeWeaponSlot    = ped->m_nActiveWeaponSlot;
    m_areaCode            = static_cast<int8>(ped->GetAreaCode());
    m_pad0x183            = 0;
    m_nFightingStyle      = ped->m_nFightingStyle;
    m_nAllowedAttackMoves = ped->m_nAllowedAttackMoves;
    std::memset(m_pad0x18A, 0, sizeof(m_pad0x18A));

    // Entry exits are saved as their pool index, and only when the ped is inside an interior
    m_nExitIndex = -1;
    if (const auto enex = ped->m_pEnex) {
        if (enex->GetMyOrLinkedArea() != 0) {
            m_nExitIndex = static_cast<int32>(CEntryExitManager::GetPool()->GetIndex(enex));
        }
    }

    for (size_t i = 0; i < std::size(m_weapons); i++) {
        m_weapons[i].Construct(ped->m_aWeapons[i]);
    }
}

// 0x5D44B0
void CPedSaveStructure::Extract(CPed* ped) {
    ped->GetPosition() = m_pos;

    ped->m_fHealth = m_health;
    ped->m_fArmour = m_armor;

    ped->m_nActiveWeaponSlot = m_activeWeaponSlot;
    ped->SetCharCreatedBy(m_createdBy);
    ped->m_nFightingStyle      = static_cast<eFightingStyle>(m_nFightingStyle);
    ped->m_nAllowedAttackMoves = static_cast<char>(m_nAllowedAttackMoves);

    for (size_t i = 0; i < std::size(m_weapons); i++) {
        const auto& weapon = m_weapons[i];
        if (weapon.m_Type == WEAPON_UNARMED) {
            continue;
        }

        // Make sure the weapon's models are available
        for (const auto model : CWeaponInfo::GetWeaponInfo(weapon.m_Type, eWeaponSkill::STD)->GetModels()) {
            if (model != -1) {
                CStreaming::RequestModel(model, STREAMING_KEEP_IN_MEMORY);
                CStreaming::LoadAllRequestedModels(false);
            }
        }

        ped->GiveWeapon(weapon.m_Type, weapon.m_TotalAmmo, false);
        ped->m_aWeapons[i].m_AmmoInClip = weapon.m_AmmoInClip;
    }

    ped->SetCurrentWeapon(m_activeWeaponSlot);
    ped->SetAreaCode(static_cast<eAreaCodes>(m_areaCode));

    // Restore the entry exit (if any) - `GetInSlot` also takes care of slots that were freed meanwhile
    ped->m_pEnex = m_nExitIndex == -1
        ? nullptr
        : CEntryExitManager::GetInSlot(m_nExitIndex);
}
