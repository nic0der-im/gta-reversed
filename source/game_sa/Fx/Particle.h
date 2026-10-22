#pragma once

#include "ListItem_c.h"
#include "Vector.h"

class FxSystem_c;
class FxEmitterPrt_c;

struct Particle_c : public ListItem_c<Particle_c> {
    float       m_fTotalLife;
    float       m_fCurrentLife;
    CVector     m_Pos;
    CVector     m_Velocity;
    FxSystem_c* m_System;

    // NOTSA (`Particle_c` is the first base of `FxEmitterPrt_c`, so its address is unchanged)
    FxEmitterPrt_c* AsFxEmitterPrt() { return reinterpret_cast<FxEmitterPrt_c*>(this); }
};

// Offsets as written by the retail particle constructor `0x4A2580` (total life, current life and
// `m_System`), by its position/velocity block (`0x4A28E4`-`0x4A2AF7`) and by
// `FxEmitter_c::AddParticle` (`0x4A3EA0`/`0x4A4050`), which read `m_Pos`/`m_Velocity`/`m_System`.
// NOTE: `VALIDATE_SIZE`/`VALIDATE_OFFSET` are no-ops on the 64 bit native build (see Base.h).
VALIDATE_SIZE(Particle_c, 0x2C);
VALIDATE_OFFSET(Particle_c, m_fTotalLife,   0x08);
VALIDATE_OFFSET(Particle_c, m_fCurrentLife, 0x0C);
VALIDATE_OFFSET(Particle_c, m_Pos,          0x10);
VALIDATE_OFFSET(Particle_c, m_Velocity,     0x1C);
VALIDATE_OFFSET(Particle_c, m_System,       0x28);
