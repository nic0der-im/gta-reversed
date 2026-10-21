#pragma once

#include <cstddef>
#include "Vector.h"

class CVehicle;

/*!
 * @brief 32 bit (retail PC) representation of the matrix embedded in `CVehicleSaveStructure`.
 *
 * `CVehicleSaveStructure` is written/read verbatim by `CVehicle::Save`/`CVehicle::Load`, so it has to
 * keep the layout the original (32 bit) game had. The in-memory matrix holds a pointer to its attached
 * RenderWare matrix, so it's 80 bytes on 64 bit builds instead of 72, which is why the save struct
 * stores this pointer-free mirror instead (See `Construct`/`Extract`).
 */
class CVehicleSaveMatrix32 {
public:
    //! 0x00-0x40 - The RenderWare matrix (Same size & layout in every build)
    CVector m_right;   // 0x00
    uint32  m_flags;   // 0x0C
    CVector m_forward; // 0x10
    uint32  m_pad1;    // 0x1C
    CVector m_up;      // 0x20
    uint32  m_pad2;    // 0x2C
    CVector m_pos;     // 0x30
    uint32  m_pad3;    // 0x3C

    //! 0x40 - Attachment data of the in-memory matrix. It's not used by the save (The original game
    //! writes whatever was on the stack here), so it's zeroed on save and ignored on load.
    uint8   m_pad0x40[8]; // 0x40

public:
    //! Fill this in from a vehicle's in-memory matrix (Called before saving)
    void Construct(const CVehicle* v);

    //! Write this into a vehicle's in-memory matrix (Called when loading)
    void Extract(CVehicle* v) const;
};

//! Size of the RenderWare matrix that starts both this mirror and the in-memory matrix
inline constexpr size_t VEHICLE_SAVE_MATRIX_PAYLOAD_SIZE = offsetof(CVehicleSaveMatrix32, m_pad0x40);

static_assert(VEHICLE_SAVE_MATRIX_PAYLOAD_SIZE == 0x40);
VALIDATE_SIZE_ALWAYS(CVehicleSaveMatrix32, 0x48);

/*!
 * @brief 32 bit (retail PC) layout of a vehicle as stored in the save file.
 *
 * Contains no pointers (And no in-memory matrix), so it keeps the same size (And byte layout) on
 * 64 bit builds, which is required because `CVehicle::Save`/`CVehicle::Load` write/read it verbatim.
 */
class CVehicleSaveStructure {
public:
    CVehicleSaveMatrix32 m_matrix;              // 0x00
    uint8                field_48;              // 0x48 - Unused (The original game never initialised it)
    uint8                m_nPrimaryColor;       // 0x49
    uint8                m_nSecondaryColor;     // 0x4A
    uint8                m_nTertiaryColor;      // 0x4B
    uint8                m_nQuaternaryColor;    // 0x4C
    uint8                m_pad0x4D;             // 0x4D
    uint16               m_nAlarmState;         // 0x4E
    uint8                m_nMaxPassengers;      // 0x50
    uint8                m_pad0x51[3];          // 0x51
    float                m_fSteerAngle;         // 0x54
    float                m_fGasPedal;           // 0x58
    float                m_fBrakePedal;         // 0x5C
    uint32               m_nVehicleUpperFlags;  // 0x60
    uint32               m_nVehicleLowerFlags;  // 0x64

public:
    static void InjectHooks();

    //! Fill this in from a vehicle (Called before saving)
    void Construct(CVehicle* v);

    //! Fill `v` in from this (Called when loading)
    void Extract(CVehicle* v);
};

//! Written verbatim into the save file - Must keep this exact layout on 64 bit builds too
VALIDATE_SIZE_ALWAYS(CVehicleSaveStructure, 0x68);
