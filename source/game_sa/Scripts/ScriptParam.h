#pragma once

#include <Base.h>

// *** 🤝 UNION 🤝 ***
//! A script command argument (or a script variable).
//!
//! NOTE: Never store a pointer in here! The script space is reverted using 4 (or 8) byte values
//! only, and this type is also written verbatim into save games - So it must remain 4 bytes wide
//! on both 32 and 64 bit builds.
//! If a pointer has to be stored, store its offset into `CTheScripts::ScriptSpace` (Or a handle) instead.
union tScriptParam {
    uint8  u8Param;
    int8   i8Param;

    uint16 u16Param;
    int16  i16Param;

    uint32 uParam{0u};
    int32  iParam;

    float  fParam;
    bool   bParam;
};
VALIDATE_SIZE_ALWAYS(tScriptParam, 0x4);
