#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

//! Native `RwRenderStateSet`/`RwRenderStateGet` (`RWSRCGLOBAL(dOpenDevice).fpRenderStateSet/Get`).
//!
//! The game speaks the D3D9 RenderWare dialect, whose state *numbers* do not match librw's
//! `rw::RenderState` enum (the game has `rwRENDERSTATESHADEMODE`, `rwRENDERSTATEFOGTYPE` and the
//! stencil block in between), so every call is translated here and forwarded to librw.
//! The texture raster is resolved through the game-raster -> librw-raster mirror.
//!
//! librw has no equivalent for the fog type (its fog is always linear), for the shade mode or
//! for the border colour; those are remembered so that a matching `RwRenderStateGet` still
//! answers with what the game set.

namespace {

//! The states librw cannot store.
struct NonLibrwState {
    RwShadeMode shadeMode   = rwSHADEMODEGOURAUD;
    RwFogType   fogType     = rwFOGTYPELINEAR;
    RwUInt32    borderColor = 0;
} g_state;

//! librw's alpha test only knows "always", "greater or equal" and "less".
rw::AlphaTestFunc ToLibrwAlphaTest(RwAlphaTestFunction func) {
    switch (func) {
    case rwALPHATESTFUNCTIONLESS:
    case rwALPHATESTFUNCTIONLESSEQUAL:
        return rw::ALPHALESS;
    case rwALPHATESTFUNCTIONGREATER:
    case rwALPHATESTFUNCTIONGREATEREQUAL:
        return rw::ALPHAGREATEREQUAL;
    case rwALPHATESTFUNCTIONALWAYS:
        return rw::ALPHAALWAYS;
    default:
        notsa::fakerw::WarnOnce("Rw: the alpha test function has no librw equivalent, using ALWAYS");
        return rw::ALPHAALWAYS;
    }
}

RwAlphaTestFunction FromLibrwAlphaTest(rw::AlphaTestFunc func) {
    switch (func) {
    case rw::ALPHALESS:         return rwALPHATESTFUNCTIONLESS;
    case rw::ALPHAGREATEREQUAL: return rwALPHATESTFUNCTIONGREATEREQUAL;
    case rw::ALPHAALWAYS:
    default:                    return rwALPHATESTFUNCTIONALWAYS;
    }
}

} // namespace

namespace notsa::fakerw {

RwBool DeviceRenderStateSet(RwRenderState state, void* value) {
    const auto val = (RwUInt32)(uintptr)value;

    switch (state) {
    case rwRENDERSTATETEXTURERASTER: {
        auto* raster = static_cast<RwRaster*>(value);
        auto* librwRaster = raster ? GetLibrwRaster(raster) : nullptr;
        if (raster && !librwRaster) {
            WarnOnce("Rw: a texture raster that was not created through RwRasterCreate cannot be bound");
        }
        rw::SetRenderStatePtr(rw::TEXTURERASTER, librwRaster);
        return TRUE;
    }
    case rwRENDERSTATETEXTUREADDRESS:
        rw::SetRenderState(rw::TEXTUREADDRESS, val);
        return TRUE;
    case rwRENDERSTATETEXTUREADDRESSU:
        rw::SetRenderState(rw::TEXTUREADDRESSU, val);
        return TRUE;
    case rwRENDERSTATETEXTUREADDRESSV:
        rw::SetRenderState(rw::TEXTUREADDRESSV, val);
        return TRUE;
    case rwRENDERSTATETEXTUREFILTER:
        rw::SetRenderState(rw::TEXTUREFILTER, val);
        return TRUE;

    case rwRENDERSTATEZTESTENABLE:
        rw::SetRenderState(rw::ZTESTENABLE, val);
        return TRUE;
    case rwRENDERSTATEZWRITEENABLE:
        rw::SetRenderState(rw::ZWRITEENABLE, val);
        return TRUE;
    case rwRENDERSTATESRCBLEND:
        rw::SetRenderState(rw::SRCBLEND, val);
        return TRUE;
    case rwRENDERSTATEDESTBLEND:
        rw::SetRenderState(rw::DESTBLEND, val);
        return TRUE;
    case rwRENDERSTATEVERTEXALPHAENABLE:
        rw::SetRenderState(rw::VERTEXALPHA, val);
        return TRUE;
    case rwRENDERSTATECULLMODE:
        rw::SetRenderState(rw::CULLMODE, val);
        return TRUE;

    case rwRENDERSTATEFOGENABLE:
        rw::SetRenderState(rw::FOGENABLE, val);
        return TRUE;
    case rwRENDERSTATEFOGCOLOR:
        rw::SetRenderState(rw::FOGCOLOR, val);
        return TRUE;
    case rwRENDERSTATEFOGTYPE:
        // librw only implements linear fog; the game always asks for linear anyway.
        g_state.fogType = (RwFogType)val;
        return TRUE;
    case rwRENDERSTATEFOGDENSITY:
        // Only meaningful for the exponential fog types, which librw does not have.
        return TRUE;

    case rwRENDERSTATEALPHATESTFUNCTION:
        rw::SetRenderState(rw::ALPHATESTFUNC, (rw::uint32)ToLibrwAlphaTest((RwAlphaTestFunction)val));
        return TRUE;
    case rwRENDERSTATEALPHATESTFUNCTIONREF:
        rw::SetRenderState(rw::ALPHATESTREF, val);
        return TRUE;

    case rwRENDERSTATESHADEMODE:
        g_state.shadeMode = (RwShadeMode)val;
        return TRUE;
    case rwRENDERSTATEBORDERCOLOR:
        g_state.borderColor = val;
        return TRUE;

    case rwRENDERSTATETEXTUREPERSPECTIVE:
        // Perspective correction is always on in the GL backend.
        return TRUE;

    default:
        WarnOnce("Rw: RwRenderStateSet on a render state without a native backend");
        return FALSE;
    }
}

RwBool DeviceRenderStateGet(RwRenderState state, void* value) {
    if (!value) {
        return FALSE;
    }

    switch (state) {
    case rwRENDERSTATETEXTURERASTER:
        *static_cast<RwRaster**>(value) = GetGameRaster(static_cast<rw::Raster*>(rw::GetRenderStatePtr(rw::TEXTURERASTER)));
        return TRUE;
    case rwRENDERSTATETEXTUREADDRESS:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::TEXTUREADDRESS);
        return TRUE;
    case rwRENDERSTATETEXTUREADDRESSU:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::TEXTUREADDRESSU);
        return TRUE;
    case rwRENDERSTATETEXTUREADDRESSV:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::TEXTUREADDRESSV);
        return TRUE;
    case rwRENDERSTATETEXTUREFILTER:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::TEXTUREFILTER);
        return TRUE;

    case rwRENDERSTATEZTESTENABLE:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::ZTESTENABLE);
        return TRUE;
    case rwRENDERSTATEZWRITEENABLE:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::ZWRITEENABLE);
        return TRUE;
    case rwRENDERSTATESRCBLEND:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::SRCBLEND);
        return TRUE;
    case rwRENDERSTATEDESTBLEND:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::DESTBLEND);
        return TRUE;
    case rwRENDERSTATEVERTEXALPHAENABLE:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::VERTEXALPHA);
        return TRUE;
    case rwRENDERSTATECULLMODE:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::CULLMODE);
        return TRUE;

    case rwRENDERSTATEFOGENABLE:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::FOGENABLE);
        return TRUE;
    case rwRENDERSTATEFOGCOLOR:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::FOGCOLOR);
        return TRUE;
    case rwRENDERSTATEFOGTYPE:
        *static_cast<RwUInt32*>(value) = g_state.fogType;
        return TRUE;

    case rwRENDERSTATEALPHATESTFUNCTION:
        *static_cast<RwUInt32*>(value) = FromLibrwAlphaTest((rw::AlphaTestFunc)rw::GetRenderState(rw::ALPHATESTFUNC));
        return TRUE;
    case rwRENDERSTATEALPHATESTFUNCTIONREF:
        *static_cast<RwUInt32*>(value) = rw::GetRenderState(rw::ALPHATESTREF);
        return TRUE;

    case rwRENDERSTATESHADEMODE:
        *static_cast<RwUInt32*>(value) = g_state.shadeMode;
        return TRUE;
    case rwRENDERSTATEBORDERCOLOR:
        *static_cast<RwUInt32*>(value) = g_state.borderColor;
        return TRUE;

    default:
        WarnOnce("Rw: RwRenderStateGet on a render state without a native backend");
        return FALSE;
    }
}

} // namespace notsa::fakerw

#endif // NOTSA_NATIVE
