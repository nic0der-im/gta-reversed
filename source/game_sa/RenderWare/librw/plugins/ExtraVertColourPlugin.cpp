#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

//! Extra vertex colours (night colours) plugin: chunk 0x0253F2F9, attached to `rw::Geometry`.
//!
//! Retail implementation: `CCustomBuildingDNPipeline::ExtraVertColourPluginAttach` /
//! `pluginExtraVertColourStreamReadCB` in `game_sa/Pipelines/CustomBuilding/CustomBuildingDNPipeline.cpp`.
//! Payload: `uint32 hasColors` (retail writes a pointer, only its non-zeroness is tested), then
//! `numVertices` colours of 4 bytes. The day colours are the geometry's pre-light colours; only
//! the night colours are streamed.

namespace notsa::fakerw::plugins {

namespace {

constexpr rw::uint32 kChunkId = RockstarChunkId(0xF9);

rw::int32 s_offset = -1;

void* ExtraVertColourConstructor(void* object, rw::int32 offset, rw::int32) {
    auto* const plg = PLUGINOFFSET(ExtraVertColours, object, offset);
    plg->nightColours = nullptr;
    plg->count        = 0;
    plg->dnBalance    = 0.0f;
    return object;
}

void* ExtraVertColourDestructor(void* object, rw::int32 offset, rw::int32) {
    auto* const plg = PLUGINOFFSET(ExtraVertColours, object, offset);
    if (plg->nightColours) {
        rwFree(plg->nightColours);
    }
    ExtraVertColourConstructor(object, offset, 0);
    return object;
}

void* ExtraVertColourCopy(void* dst, void* src, rw::int32 offset, rw::int32) {
    auto* const       dstPlg = PLUGINOFFSET(ExtraVertColours, dst, offset);
    const auto* const srcPlg = PLUGINOFFSET(ExtraVertColours, src, offset);

    if (dstPlg->nightColours) {
        rwFree(dstPlg->nightColours);
    }
    dstPlg->nightColours = nullptr;
    dstPlg->count        = srcPlg->count;
    dstPlg->dnBalance    = srcPlg->dnBalance;

    if (srcPlg->nightColours && srcPlg->count) {
        const size_t bytes = sizeof(rw::RGBA) * (size_t)srcPlg->count;
        if ((dstPlg->nightColours = static_cast<rw::RGBA*>(rwMalloc(bytes, details::kMemHint)))) {
            std::memcpy(dstPlg->nightColours, srcPlg->nightColours, bytes);
        } else {
            dstPlg->count = 0;
        }
    }
    return dst;
}

rw::Stream* ExtraVertColourStreamRead(rw::Stream* stream, rw::int32 length, void* object, rw::int32 offset, rw::int32) {
    auto* const            plg      = PLUGINOFFSET(ExtraVertColours, object, offset);
    const auto* const      geometry = static_cast<const rw::Geometry*>(object);
    details::ChunkCursor   cursor{ stream, length, 0 };

    if (plg->nightColours) {
        rwFree(plg->nightColours);
        plg->nightColours = nullptr;
        plg->count        = 0;
    }

    rw::uint32 hasColours = 0;
    cursor.Read(&hasColours, (rw::uint32)sizeof(hasColours));
    if (hasColours && geometry->numVertices > 0) {
        const size_t bytes = sizeof(rw::RGBA) * (size_t)geometry->numVertices;
        if ((plg->nightColours = static_cast<rw::RGBA*>(rwMalloc(bytes, details::kMemHint)))) {
            cursor.Read(plg->nightColours, (rw::uint32)bytes);
            plg->count = (rw::uint32)geometry->numVertices;
        }
    }
    plg->dnBalance = plg->nightColours ? 1.0f : 0.0f;
    cursor.SkipRest();
    return stream;
}

rw::Stream* ExtraVertColourStreamWrite(rw::Stream* stream, rw::int32, void* object, rw::int32 offset, rw::int32) {
    const auto* const plg = PLUGINOFFSET(ExtraVertColours, object, offset);

    // Only the presence of the pointer is meaningful on disk; a 32-bit pointer would not fit.
    stream->writeU32(plg->nightColours ? 1u : 0u);
    if (plg->nightColours && plg->count) {
        stream->write8(plg->nightColours, (rw::uint32)(sizeof(rw::RGBA) * (size_t)plg->count));
    }
    return stream;
}

rw::int32 ExtraVertColourStreamGetSize(void* object, rw::int32 offset, rw::int32) {
    const auto* const plg = PLUGINOFFSET(ExtraVertColours, object, offset);
    return (rw::int32)(sizeof(rw::uint32) + (plg->nightColours ? sizeof(rw::RGBA) * (size_t)plg->count : 0));
}

} // namespace

void RegisterExtraVertColourPlugin() {
    if (const auto existing = rw::Geometry::getPluginOffset(kChunkId); existing >= 0) {
        s_offset = existing; // Already registered in this engine session.
        return;
    }
    s_offset = rw::Geometry::registerPlugin(
        sizeof(ExtraVertColours),
        kChunkId,
        ExtraVertColourConstructor,
        ExtraVertColourDestructor,
        ExtraVertColourCopy
    );
    if (s_offset < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the extra vertex colour plugin");
        return;
    }
    if (rw::Geometry::registerPluginStream(kChunkId, ExtraVertColourStreamRead, ExtraVertColourStreamWrite, ExtraVertColourStreamGetSize) < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the extra vertex colour plugin stream");
        s_offset = -1;
    }
}

const ExtraVertColours* RpGeometryGetExtraVertColourPlugin(const rw::Geometry* geometry) {
    if (!geometry || s_offset < 0) {
        return nullptr;
    }
    const auto* const plg = PLUGINOFFSET(ExtraVertColours, geometry, s_offset);
    return plg->nightColours ? plg : nullptr;
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
