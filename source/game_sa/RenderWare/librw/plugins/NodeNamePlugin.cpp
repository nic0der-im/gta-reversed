#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

//! NodeName plugin: chunk 0x0253F2FE, attached to `rw::Frame`.
//!
//! Retail implementation: `NodeNamePluginAttach` / `GetFrameNodeName` in
//! `game_sa/Plugins/NodeNamePlugin/NodeName.cpp`. The payload is the name as ASCII; retail
//! stores it in a 24 byte buffer (23 characters + terminator) and writes `strlen(name)` bytes.

namespace notsa::fakerw::plugins {

namespace {

constexpr rw::uint32 kChunkId = RockstarChunkId(0xFE);

//! Per-frame storage: `char name[24]`.
struct NodeName {
    char name[kNodeNameLength + 1];
};

rw::int32 s_offset = -1;

void* NodeNameConstructor(void* object, rw::int32 offset, rw::int32) {
    PLUGINOFFSET(NodeName, object, offset)->name[0] = '\0';
    return object;
}

void* NodeNameDestructor(void* object, rw::int32, rw::int32) {
    return object;
}

void* NodeNameCopy(void* dst, void* src, rw::int32 offset, rw::int32) {
    auto* const       dstName = PLUGINOFFSET(NodeName, dst, offset);
    const auto* const srcName = PLUGINOFFSET(NodeName, src, offset);
    std::strncpy(dstName->name, srcName->name, kNodeNameLength);
    dstName->name[kNodeNameLength] = '\0';
    return dst;
}

rw::Stream* NodeNameStreamRead(rw::Stream* stream, rw::int32 length, void* object, rw::int32 offset, rw::int32) {
    auto* const   plg    = PLUGINOFFSET(NodeName, object, offset);
    details::ChunkCursor cursor{ stream, length, 0 };

    const auto n = cursor.Read(plg->name, (rw::uint32)std::clamp(length, 0, kNodeNameLength));
    plg->name[n] = '\0';
    cursor.SkipRest();
    return stream;
}

rw::Stream* NodeNameStreamWrite(rw::Stream* stream, rw::int32, void* object, rw::int32 offset, rw::int32) {
    const auto* const plg = PLUGINOFFSET(NodeName, object, offset);
    stream->write8(plg->name, (rw::uint32)std::strlen(plg->name));
    return stream;
}

rw::int32 NodeNameStreamGetSize(void* object, rw::int32 offset, rw::int32) {
    const auto* const plg = PLUGINOFFSET(NodeName, object, offset);
    return (rw::int32)std::strlen(plg->name); // An unnamed frame writes no chunk, like retail.
}

} // namespace

void RegisterNodeNamePlugin() {
    if (const auto existing = rw::Frame::getPluginOffset(kChunkId); existing >= 0) {
        s_offset = existing; // Already registered in this engine session.
        return;
    }
    s_offset = rw::Frame::registerPlugin(sizeof(NodeName), kChunkId, NodeNameConstructor, NodeNameDestructor, NodeNameCopy);
    if (s_offset < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the NodeName plugin");
        return;
    }
    if (rw::Frame::registerPluginStream(kChunkId, NodeNameStreamRead, NodeNameStreamWrite, NodeNameStreamGetSize) < 0) {
        NOTSA_LOG_ERR("Rw: failed to register the NodeName plugin stream");
        s_offset = -1;
    }
}

const char* RwFrameGetNodeName(const rw::Frame* frame) {
    return frame && s_offset >= 0
        ? PLUGINOFFSET(NodeName, frame, s_offset)->name
        : nullptr;
}

void RwFrameSetNodeName(rw::Frame* frame, const char* name) {
    if (!frame || s_offset < 0) {
        return;
    }
    auto* const plg = PLUGINOFFSET(NodeName, frame, s_offset);
    std::strncpy(plg->name, name ? name : "", kNodeNameLength);
    plg->name[kNodeNameLength] = '\0';
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
