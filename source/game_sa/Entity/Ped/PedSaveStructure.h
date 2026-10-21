#pragma once

#include "Vector.h"
#include "Weapon.h"
#include "Ped.h"

/*!
 * @brief 32 bit (retail PC) representation of a `CWeapon` as stored in `CPedSaveStructure`.
 *
 * `CWeapon` holds a pointer (its effect system), so it's 32 bytes on 64 bit builds instead of 28.
 * Save games always store the original (32 bit) layout, so weapons are converted to this struct
 * explicitly when saving (See `CWeaponSave32::Construct`).
 */
struct CWeaponSave32 {
    eWeaponType  m_Type;                            // 0x00
    eWeaponState m_State;                           // 0x04
    uint32       m_AmmoInClip;                      // 0x08
    uint32       m_TotalAmmo;                       // 0x0C
    uint32       m_TimeForNextShotMs;               // 0x10
    bool         m_IsFirstPersonWeaponModeSelected; // 0x14
    bool         m_DontPlaceInHand;                 // 0x15
    uint8        m_pad0x16[2];                      // 0x16
    uint32       m_nUnused;                         // 0x18 - Runtime-only state of the original game (Never read back on load)

public:
    //! Fill this in from a weapon (Called before saving)
    void Construct(const CWeapon& weapon);
};
VALIDATE_SIZE_ALWAYS(CWeaponSave32, 0x1C);

/*!
 * @brief 32 bit (retail PC) layout of a ped as stored in the save file.
 *
 * Contains no pointers (And no `CWeapon`), so it keeps the same size (And byte layout) on 64 bit
 * builds, which is required because `CPed::Save`/`CPed::Load` write/read it verbatim.
 */
class CPedSaveStructure {
public:
    CVector       m_pos;                 // 0x000
    float         m_health;              // 0x00C
    float         m_armor;               // 0x010
    CWeaponSave32 m_weapons[13];         // 0x014
    ePedCreatedBy m_createdBy;           // 0x180
    int8          m_activeWeaponSlot;    // 0x181
    int8          m_areaCode;            // 0x182
    uint8         m_pad0x183;            // 0x183
    int32         m_nExitIndex;          // 0x184
    uint8         m_nFightingStyle;      // 0x188
    uint8         m_nAllowedAttackMoves; // 0x189
    uint8         m_pad0x18A[2];         // 0x18A

public:
    //! Fill this in from a ped (Called before saving)
    void Construct(CPed* ped);

    //! Fill `ped` in from this (Called when loading)
    void Extract(CPed* ped);
};

//! Written verbatim into the save file - Must keep this exact layout on 64 bit builds too
VALIDATE_SIZE_ALWAYS(CPedSaveStructure, 0x18C);
