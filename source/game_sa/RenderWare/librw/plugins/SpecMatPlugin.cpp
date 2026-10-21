#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

//! Specular material plugin: chunk 0x0253F2F6, attached to `rw::Material`.
//!
//! Retail implementation: `CCustomCarEnvMapPipeline::RegisterPlugin` /
//! `pluginSpecMatStreamReadCB` in `game_sa/Pipelines/CustomCarEnvMap/CustomCarEnvMapPipeline_SpecMat.cpp`,
//! with the on-disk layout of `SpecMapPipeMaterialDataBuffer`:
//!
//!     float specularity;
//!     char  textureName[24];
//!
//! Retail turns a non-zero specularity into a `RwTextureRead(textureName)`; that resolution is
//! left to the pipeline, so the name is kept here.

namespace notsa::fakerw::plugins {

namespace {

constexpr rw::uint32 kChunkId = RockstarChunkId(0xF6);

struct SpecMatStreamData {
    rw::float32 specularity;
    char        textureName[24];
};
static_assert(sizeof(SpecMatStreamData) == 28, "Specular material chunk is 28 bytes");

rw::int32 s_offset = -1;

void* SpecMatConstructor(void* object, rw::int32 offset, rw::int32) {
    *PLUGINOFFSET(SpecMat, object, offset) = SpecMat{};
    return object;
}

void* SpecMatCopy(void* dst, void* src, rw::int32 offset, rw::int32) {
    *PLUGINOFFSET(SpecMat, dst, offset) = *PLUGINOFFSET(SpecMat, src, offset);
    return dst;
}

rw::Stream* SpecMatStreamRead(rw::Stream* stream, rw::int32 length, void* object, rw::int32 offset, rw::int32) {
    auto* const          plg    = PLUGINOFFSET(SpecMat, object, offset);
    details::ChunkCursor cursor{ stream, length, 0 };

    *plg = SpecMat{};
    if (length >= (rw::int32)sizeof(SpecMatStreamData)) {
        SpecMatStreamData raw{};
        cursor.Read(&raw, (rw::uint32)sizeof(raw));
        if (raw.specularity != 0.0f) { // Retail only keeps the data when it is present.
            plg->present     = true;
            plg->specularity = raw.specularity;
            std::strncpy(plg->textureName, raw.textureName, sizeof(plg->textureName) - 1);
            plg->textureName[sizeof(plg->textureName) - 1] = '\0';
        }
    }
    cursor.SkipRest();
    return stream;
}

rw::Stream* SpecMatStreamWrite(rw::Stream* stream, rw::int32, void* object, rw::int32 offset, rw::int32) {
    const auto* const plg = PLUGINOFFSET(SpecMat, object, offset);

    SpecMatStreamData raw{};
    if (plg->present) {
        raw.specularity = plg->specularity;
        std::strncpy(raw.textureName, plg->textureName, sizeof(raw.textureName));
    }
    stream->write8(&raw, (rw::uint32)sizeof(raw));
    return stream;
}

rw::int32 SpecMatStreamGetSize(void*, rw::int32, rw::int32) {
    return (rw::int32)sizeof(SpecMatStreamData); // Retail always writes the chunk, zero-filled when absent.
}

} // namespace

void RegisterSpecMatPlugin() {
    if (const auto existing = rw::Material::getPluginOffset(kChunkId); existing >= 0) {
        s_offset = existing; // Already registered in this engine session.
        return;
    }
    s_offset = rw::Material::registerPlugin(sizeof(SpecMat), kChunkId, SpecMatConstructor, nullptr, SpecMatCopy);
    if (s_offset < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the specular material plugin");
        return;
    }
    if (rw::Material::registerPluginStream(kChunkId, SpecMatStreamRead, SpecMatStreamWrite, SpecMatStreamGetSize) < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the specular material plugin stream");
        s_offset = -1;
    }
}

const SpecMat* RpMaterialGetSpecMatPlugin(const rw::Material* material) {
    if (!material || s_offset < 0) {
        return nullptr;
    }
    const auto* const plg = PLUGINOFFSET(SpecMat, material, s_offset);
    return plg->present ? plg : nullptr;
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
