#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

//! Breakable plugin: chunk 0x0253F2FD, attached to `rw::Geometry`.
//!
//! Retail implementation: `BreakablePluginAttach` / `BreakableStreamRead` in
//! `game_sa/Plugins/BreakablePlugin/BreakablePlugin.cpp`. Payload: `uint32 sectionHeader`
//! (a non-zero value means a `BreakInfo_t` follows, then vertex/triangle/material arrays — see
//! `BreakablePlugin.h`). The parser lives in the game, so the payload is kept verbatim.

namespace notsa::fakerw::plugins {

namespace {

constexpr rw::uint32 kChunkId = RockstarChunkId(0xFD);

rw::int32 s_offset = -1;

void* BreakableConstructor(void* object, rw::int32 offset, rw::int32) {
    details::RawChunkInit(*PLUGINOFFSET(RawChunk, object, offset));
    return object;
}

void* BreakableDestructor(void* object, rw::int32 offset, rw::int32) {
    details::RawChunkFree(*PLUGINOFFSET(RawChunk, object, offset));
    return object;
}

void* BreakableCopy(void* dst, void* src, rw::int32 offset, rw::int32) {
    details::RawChunkCopy(
        *PLUGINOFFSET(RawChunk, dst, offset),
        *PLUGINOFFSET(RawChunk, src, offset)
    );
    return dst;
}

rw::Stream* BreakableStreamRead(rw::Stream* stream, rw::int32 length, void* object, rw::int32 offset, rw::int32) {
    details::ChunkCursor cursor{ stream, length, 0 };
    details::RawChunkRead(cursor, *PLUGINOFFSET(RawChunk, object, offset));
    return stream;
}

rw::Stream* BreakableStreamWrite(rw::Stream* stream, rw::int32, void* object, rw::int32 offset, rw::int32) {
    details::RawChunkWrite(stream, *PLUGINOFFSET(RawChunk, object, offset));
    return stream;
}

rw::int32 BreakableStreamGetSize(void* object, rw::int32 offset, rw::int32) {
    return (rw::int32)PLUGINOFFSET(RawChunk, object, offset)->size;
}

} // namespace

void RegisterBreakablePlugin() {
    if (const auto existing = rw::Geometry::getPluginOffset(kChunkId); existing >= 0) {
        s_offset = existing; // Already registered in this engine session.
        return;
    }
    s_offset = rw::Geometry::registerPlugin(sizeof(RawChunk), kChunkId, BreakableConstructor, BreakableDestructor, BreakableCopy);
    if (s_offset < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the breakable plugin");
        return;
    }
    if (rw::Geometry::registerPluginStream(kChunkId, BreakableStreamRead, BreakableStreamWrite, BreakableStreamGetSize) < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the breakable plugin stream");
        s_offset = -1;
    }
}

const RawChunk* RpGeometryGetBreakablePlugin(const rw::Geometry* geometry) {
    if (!geometry || s_offset < 0) {
        return nullptr;
    }
    const auto* const raw = PLUGINOFFSET(RawChunk, geometry, s_offset);
    return raw->data ? raw : nullptr;
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
