#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

//! Pipeline id plugin: chunk 0x0253F2F3, attached to `rw::Atomic`.
//!
//! Retail implementation: `PipelinePluginAttach` / `GetPipelineID` / `SetPipelineID` in
//! `game_sa/Plugins/PipelinePlugin/PipelinePlugin.cpp`. Payload: `uint32 pipelineId`, the id
//! allocated with `rpPDS_MAKEPIPEID` that tells the game which custom pipeline to use.

namespace notsa::fakerw::plugins {

namespace {

constexpr rw::uint32 kChunkId = RockstarChunkId(0xF3);

rw::int32 s_offset = -1;

void* PipelineConstructor(void* object, rw::int32 offset, rw::int32) {
    *PLUGINOFFSET(rw::uint32, object, offset) = 0;
    return object;
}

void* PipelineCopy(void* dst, void* src, rw::int32 offset, rw::int32) {
    *PLUGINOFFSET(rw::uint32, dst, offset) = *PLUGINOFFSET(rw::uint32, src, offset);
    return dst;
}

rw::Stream* PipelineStreamRead(rw::Stream* stream, rw::int32 length, void* object, rw::int32 offset, rw::int32) {
    auto* const          id     = PLUGINOFFSET(rw::uint32, object, offset);
    details::ChunkCursor cursor{ stream, length, 0 };

    *id = 0;
    cursor.Read(id, (rw::uint32)sizeof(*id));
    cursor.SkipRest();
    return stream;
}

rw::Stream* PipelineStreamWrite(rw::Stream* stream, rw::int32, void* object, rw::int32 offset, rw::int32) {
    stream->writeU32(*PLUGINOFFSET(rw::uint32, object, offset));
    return stream;
}

rw::int32 PipelineStreamGetSize(void*, rw::int32, rw::int32) {
    return (rw::int32)sizeof(rw::uint32); // Retail always writes the id, zero included.
}

} // namespace

void RegisterPipelinePlugin() {
    if (const auto existing = rw::Atomic::getPluginOffset(kChunkId); existing >= 0) {
        s_offset = existing; // Already registered in this engine session.
        return;
    }
    s_offset = rw::Atomic::registerPlugin(sizeof(rw::uint32), kChunkId, PipelineConstructor, nullptr, PipelineCopy);
    if (s_offset < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the pipeline id plugin");
        return;
    }
    if (rw::Atomic::registerPluginStream(kChunkId, PipelineStreamRead, PipelineStreamWrite, PipelineStreamGetSize) < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the pipeline id plugin stream");
        s_offset = -1;
    }
}

rw::uint32 RpAtomicGetPipelineID(const rw::Atomic* atomic) {
    return atomic && s_offset >= 0
        ? *PLUGINOFFSET(rw::uint32, atomic, s_offset)
        : 0;
}

void RpAtomicSetPipelineID(rw::Atomic* atomic, rw::uint32 pipelineId) {
    if (atomic && s_offset >= 0) {
        *PLUGINOFFSET(rw::uint32, atomic, s_offset) = pipelineId;
    }
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
