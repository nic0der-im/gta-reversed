#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

//! EnvMap material plugin: chunk 0x0253F2FC, attached to `rw::Material`.
//!
//! Retail implementation: `CCustomCarEnvMapPipeline::RegisterPlugin` /
//! `pluginEnvMatStreamReadCB` in `game_sa/Pipelines/CustomCarEnvMap/CustomCarEnvMapPipeline_EnvMat.cpp`,
//! with the on-disk layout of `EnvMapPipeMaterialDataBuffer`:
//!
//!     RwV2d  scale;            // 1.0, 1.0 by default
//!     RwV2d  translationScale; // 1.0, 1.0 by default
//!     float  shininess;
//!     void*  texture;          // 32-bit pointer as stored by the original D3D9 build
//!
//! The texture slot is preserved verbatim (it is not a valid pointer in a 64-bit build); the
//! runtime texture is a separate field, set by the car env map pipeline.

namespace notsa::fakerw::plugins {

namespace {

constexpr rw::uint32 kChunkId = RockstarChunkId(0xFC);

//! On-disk payload. 32-bit pointers only exist on disk, hence the `rw::uint32` slot.
struct EnvMatStreamData {
    rw::float32 scaleX;
    rw::float32 scaleY;
    rw::float32 transScaleX;
    rw::float32 transScaleY;
    rw::float32 shininess;
    rw::uint32  texturePointer;
};
static_assert(sizeof(EnvMatStreamData) == 24, "EnvMap material chunk is 24 bytes");

rw::int32 s_offset = -1;

void* EnvMatConstructor(void* object, rw::int32 offset, rw::int32) {
    auto* const plg = PLUGINOFFSET(EnvMat, object, offset);
    *plg = EnvMat{}; // Same defaults as retail's fake material data (scale 1, 1).
    return object;
}

void* EnvMatDestructor(void* object, rw::int32 offset, rw::int32) {
    // The texture is owned by a texture dictionary, and the pipeline owns the reference.
    EnvMatConstructor(object, offset, 0);
    return object;
}

void* EnvMatCopy(void* dst, void* src, rw::int32 offset, rw::int32) {
    // Retail copies its pool struct as-is (the texture reference count is untouched).
    *PLUGINOFFSET(EnvMat, dst, offset) = *PLUGINOFFSET(EnvMat, src, offset);
    return dst;
}

rw::Stream* EnvMatStreamRead(rw::Stream* stream, rw::int32 length, void* object, rw::int32 offset, rw::int32) {
    auto* const          plg    = PLUGINOFFSET(EnvMat, object, offset);
    details::ChunkCursor cursor{ stream, length, 0 };

    if (length >= (rw::int32)sizeof(EnvMatStreamData)) {
        EnvMatStreamData raw{};
        cursor.Read(&raw, (rw::uint32)sizeof(raw));
        plg->present        = true;
        plg->scaleX         = raw.scaleX;
        plg->scaleY         = raw.scaleY;
        plg->transScaleX    = raw.transScaleX;
        plg->transScaleY    = raw.transScaleY;
        plg->shininess      = raw.shininess;
        plg->texturePointer = raw.texturePointer;
    }
    cursor.SkipRest();
    return stream;
}

rw::Stream* EnvMatStreamWrite(rw::Stream* stream, rw::int32, void* object, rw::int32 offset, rw::int32) {
    const auto* const plg = PLUGINOFFSET(EnvMat, object, offset);
    const EnvMatStreamData raw{
        .scaleX         = plg->scaleX,
        .scaleY         = plg->scaleY,
        .transScaleX    = plg->transScaleX,
        .transScaleY    = plg->transScaleY,
        .shininess      = plg->shininess,
        .texturePointer = plg->texturePointer,
    };
    stream->write8(&raw, (rw::uint32)sizeof(raw));
    return stream;
}

rw::int32 EnvMatStreamGetSize(void*, rw::int32, rw::int32) {
    return (rw::int32)sizeof(EnvMatStreamData); // Retail always writes the chunk, defaults included.
}

} // namespace

void RegisterEnvMatPlugin() {
    if (const auto existing = rw::Material::getPluginOffset(kChunkId); existing >= 0) {
        s_offset = existing; // Already registered in this engine session.
        return;
    }
    s_offset = rw::Material::registerPlugin(sizeof(EnvMat), kChunkId, EnvMatConstructor, EnvMatDestructor, EnvMatCopy);
    if (s_offset < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the env map material plugin");
        return;
    }
    if (rw::Material::registerPluginStream(kChunkId, EnvMatStreamRead, EnvMatStreamWrite, EnvMatStreamGetSize) < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the env map material plugin stream");
        s_offset = -1;
    }
}

const EnvMat* RpMaterialGetEnvMatPlugin(const rw::Material* material) {
    return material && s_offset >= 0
        ? PLUGINOFFSET(EnvMat, material, s_offset)
        : nullptr;
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
