#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

//! 2dEffect plugin: chunk 0x0253F2F8, attached to `rw::Geometry`.
//!
//! Retail implementation: `C2dEffect::PluginAttach` / `Rwt2dEffectPluginDataChunkReadCallBack` in
//! `game_sa/Plugins/TwoDEffectPlugin/2dEffect.cpp`. Payload layout (see `2dEffectStream.h`):
//!
//!     uint32 count;
//!     struct { CVector pos; uint32 type; uint32 size; uint8 data[size]; } entries[count];
//!
//! The entry parsers are already implemented in the game, so the payload is kept verbatim and
//! only the effect count is extracted.

namespace notsa::fakerw::plugins {

namespace {

constexpr rw::uint32 kChunkId = RockstarChunkId(0xF8);

rw::int32 s_offset = -1;

void* TwoDEffectConstructor(void* object, rw::int32 offset, rw::int32) {
    auto* const plg = PLUGINOFFSET(TwoDEffects, object, offset);
    plg->count = 0;
    details::RawChunkInit(plg->chunk);
    return object;
}

void* TwoDEffectDestructor(void* object, rw::int32 offset, rw::int32) {
    details::RawChunkFree(PLUGINOFFSET(TwoDEffects, object, offset)->chunk);
    return object;
}

void* TwoDEffectCopy(void* dst, void* src, rw::int32 offset, rw::int32) {
    auto* const       dstPlg = PLUGINOFFSET(TwoDEffects, dst, offset);
    const auto* const srcPlg = PLUGINOFFSET(TwoDEffects, src, offset);
    dstPlg->count = srcPlg->count;
    details::RawChunkCopy(dstPlg->chunk, srcPlg->chunk);
    return dst;
}

rw::Stream* TwoDEffectStreamRead(rw::Stream* stream, rw::int32 length, void* object, rw::int32 offset, rw::int32) {
    auto* const          plg    = PLUGINOFFSET(TwoDEffects, object, offset);
    details::ChunkCursor cursor{ stream, length, 0 };

    details::RawChunkRead(cursor, plg->chunk);
    if (plg->chunk.size >= sizeof(rw::uint32)) {
        std::memcpy(&plg->count, plg->chunk.data, sizeof(plg->count)); // DFF is little-endian, as the host.
    } else {
        plg->count = 0;
    }
    return stream;
}

rw::Stream* TwoDEffectStreamWrite(rw::Stream* stream, rw::int32, void* object, rw::int32 offset, rw::int32) {
    details::RawChunkWrite(stream, PLUGINOFFSET(TwoDEffects, object, offset)->chunk);
    return stream;
}

rw::int32 TwoDEffectStreamGetSize(void* object, rw::int32 offset, rw::int32) {
    const auto* const plg = PLUGINOFFSET(TwoDEffects, object, offset);
    return plg->count ? (rw::int32)plg->chunk.size : 0; // Retail writes no chunk when there are no effects.
}

} // namespace

void RegisterTwoDEffectPlugin() {
    if (const auto existing = rw::Geometry::getPluginOffset(kChunkId); existing >= 0) {
        s_offset = existing; // Already registered in this engine session.
        return;
    }
    s_offset = rw::Geometry::registerPlugin(sizeof(TwoDEffects), kChunkId, TwoDEffectConstructor, TwoDEffectDestructor, TwoDEffectCopy);
    if (s_offset < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the 2dEffect plugin");
        return;
    }
    if (rw::Geometry::registerPluginStream(kChunkId, TwoDEffectStreamRead, TwoDEffectStreamWrite, TwoDEffectStreamGetSize) < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the 2dEffect plugin stream");
        s_offset = -1;
    }
}

const TwoDEffects* RpGeometryGet2dFxPlugin(const rw::Geometry* geometry) {
    if (!geometry || s_offset < 0) {
        return nullptr;
    }
    const auto* const plg = PLUGINOFFSET(TwoDEffects, geometry, s_offset);
    return plg->chunk.data ? plg : nullptr;
}

rw::uint32 RpGeometryGet2dFxCount(const rw::Geometry* geometry) {
    const auto* const plg = RpGeometryGet2dFxPlugin(geometry);
    return plg ? plg->count : 0;
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
