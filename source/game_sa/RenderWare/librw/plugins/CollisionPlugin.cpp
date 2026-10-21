#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

//! Collision plugin: chunk 0x0253F2FA, attached to `rw::Clump` (retail) and `rw::Geometry`.
//!
//! Retail implementation: `CCollisionPlugin::PluginAttach` / `ClumpCollisionStreamRead` in
//! `game_sa/Plugins/CollisionPlugin/CollisionPlugin.cpp`. Retail registers on the clump, reads
//! the payload into scratch space and hands it to `CFileLoader::LoadCollisionModel{,Ver2,Ver3}`
//! (`COLL`/`COL2`/`COL3` magic). The parser lives in the game, so the bytes are kept verbatim.

namespace notsa::fakerw::plugins {

namespace {

constexpr rw::uint32 kChunkId = RockstarChunkId(0xFA);

rw::int32 s_clumpOffset    = -1;
rw::int32 s_geometryOffset = -1;

void* CollisionConstructor(void* object, rw::int32 offset, rw::int32) {
    details::RawChunkInit(*PLUGINOFFSET(RawChunk, object, offset));
    return object;
}

void* CollisionDestructor(void* object, rw::int32 offset, rw::int32) {
    details::RawChunkFree(*PLUGINOFFSET(RawChunk, object, offset));
    return object;
}

void* CollisionCopy(void* dst, void* src, rw::int32 offset, rw::int32) {
    details::RawChunkCopy(
        *PLUGINOFFSET(RawChunk, dst, offset),
        *PLUGINOFFSET(RawChunk, src, offset)
    );
    return dst;
}

rw::Stream* CollisionStreamRead(rw::Stream* stream, rw::int32 length, void* object, rw::int32 offset, rw::int32) {
    details::ChunkCursor cursor{ stream, length, 0 };
    details::RawChunkRead(cursor, *PLUGINOFFSET(RawChunk, object, offset));
    return stream;
}

rw::Stream* CollisionStreamWrite(rw::Stream* stream, rw::int32, void* object, rw::int32 offset, rw::int32) {
    details::RawChunkWrite(stream, *PLUGINOFFSET(RawChunk, object, offset));
    return stream;
}

rw::int32 CollisionStreamGetSize(void* object, rw::int32 offset, rw::int32) {
    return (rw::int32)PLUGINOFFSET(RawChunk, object, offset)->size;
}

//! Registers the data plugin and its stream callbacks on `Object` (`rw::Clump` or `rw::Geometry`).
template <typename Object>
rw::int32 AttachCollisionPlugin() {
    const auto offset = Object::registerPlugin(sizeof(RawChunk), kChunkId, CollisionConstructor, CollisionDestructor, CollisionCopy);
    if (offset < 0) {
        return -1;
    }
    return Object::registerPluginStream(kChunkId, CollisionStreamRead, CollisionStreamWrite, CollisionStreamGetSize) < 0
        ? -1
        : offset;
}

} // namespace

void RegisterCollisionPlugin() {
    s_clumpOffset    = rw::Clump::getPluginOffset(kChunkId);
    s_geometryOffset = rw::Geometry::getPluginOffset(kChunkId);

    if (s_clumpOffset < 0) {
        s_clumpOffset = AttachCollisionPlugin<rw::Clump>();
        if (s_clumpOffset < 0) {
            NOTSA_LOG_ERR("Rw: failed to register the collision plugin on clumps");
        }
    }

    if (s_geometryOffset < 0) {
        s_geometryOffset = AttachCollisionPlugin<rw::Geometry>();
        if (s_geometryOffset < 0) {
            NOTSA_LOG_ERR("Rw: failed to register the collision plugin on geometries");
        }
    }
}

const RawChunk* RpClumpGetCollisionPlugin(const rw::Clump* clump) {
    if (!clump || s_clumpOffset < 0) {
        return nullptr;
    }
    const auto* const raw = PLUGINOFFSET(RawChunk, clump, s_clumpOffset);
    return raw->data ? raw : nullptr;
}

const RawChunk* RpGeometryGetCollisionPlugin(const rw::Geometry* geometry) {
    if (!geometry || s_geometryOffset < 0) {
        return nullptr;
    }
    const auto* const raw = PLUGINOFFSET(RawChunk, geometry, s_geometryOffset);
    return raw->data ? raw : nullptr;
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
