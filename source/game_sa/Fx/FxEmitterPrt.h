#pragma once

#include "Particle.h"
#include "RGBA.h"
#include "extensions/FixedFloat.hpp"

//! One FX particle.
//!
//! `Particle_c` already derives from `ListItem_c<Particle_c>` and that is the link node used by
//! the particle lists (`FxEmitterBP_c::m_Particles` and `FxManager_c::m_FxEmitterParticles` are
//! both `TList_c<Particle_c>`), so this class must NOT derive from a second `ListItem_c`.
//! Retail (`0x4A2580`, the particle constructor) has exactly the fields below: `Particle_c`'s
//! `0x2C` bytes followed by 11 one-byte fields and `m_CurrentRotation` at `+0x38` (`0x3C` total).
class FxEmitterPrt_c : public Particle_c {
public:
    CRGBA                       m_MultColor;          // +0x2C RGBA, each channel quantised with `* 255.0f`
    FixedFloat<uint8, 255.0f>   m_MultSize;           // +0x30
    FixedFloat<uint8, 255.0f>   m_MultRot;            // +0x31
    uint8                       m_RandR;              // +0x32 random 0..255
    uint8                       m_RandG;              // +0x33
    uint8                       m_RandB;              // +0x34
    FixedFloat<uint8, 100.0f>   m_Brightness;         // +0x35
    int8                        m_RotZ;               // +0x36 `-1` = keep `m_CurrentRotation`; else `floor(rotZ * 0.5f)`
    bool                        m_bLocalToSystem;     // +0x37
    float                       m_CurrentRotation;    // +0x38 degrees, randomised from the emission info

public:
    FxEmitterPrt_c() = default;  // 0x4A94E0
    ~FxEmitterPrt_c() = default; // 0x4A94F0

    static void InjectHooks();

    static void* operator new[](size_t size);
};

// Offsets as written by the retail particle constructor `0x4A2580` (and by
// `FxEmitter_c::AddParticle` `0x4A3EA0`/`0x4A4050`, which store to `m_RotZ` at `+0x36`).
// NOTE: `VALIDATE_SIZE`/`VALIDATE_OFFSET` are no-ops on the 64 bit native build (see Base.h).
VALIDATE_SIZE(FxEmitterPrt_c, 0x3C);
VALIDATE_OFFSET(FxEmitterPrt_c, m_MultColor,       0x2C);
VALIDATE_OFFSET(FxEmitterPrt_c, m_MultSize,        0x30);
VALIDATE_OFFSET(FxEmitterPrt_c, m_MultRot,         0x31);
VALIDATE_OFFSET(FxEmitterPrt_c, m_RandR,           0x32);
VALIDATE_OFFSET(FxEmitterPrt_c, m_RandG,           0x33);
VALIDATE_OFFSET(FxEmitterPrt_c, m_RandB,           0x34);
VALIDATE_OFFSET(FxEmitterPrt_c, m_Brightness,      0x35);
VALIDATE_OFFSET(FxEmitterPrt_c, m_RotZ,            0x36);
VALIDATE_OFFSET(FxEmitterPrt_c, m_bLocalToSystem,  0x37);
VALIDATE_OFFSET(FxEmitterPrt_c, m_CurrentRotation, 0x38);
