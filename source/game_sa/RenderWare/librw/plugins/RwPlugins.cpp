#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwPluginsInternal.h"

#include <algorithm>

namespace notsa::fakerw::plugins::details {

rw::uint32 ChunkCursor::Read(void* dst, rw::uint32 size) {
    const auto remaining = this->length > 0 && (rw::uint32)this->length > this->consumed
        ? (rw::uint32)this->length - this->consumed
        : 0u;
    const auto read = std::min(size, remaining);
    if (read) {
        this->stream->read8(dst, read);
        this->consumed += read;
    }
    if (read < size) {
        // Truncated chunk: never leak stack garbage into the plugin data.
        std::memset(static_cast<rw::uint8*>(dst) + read, 0, size - read);
    }
    return read;
}

void ChunkCursor::SkipRest() {
    if (this->length > 0 && (rw::uint32)this->length > this->consumed) {
        this->stream->seek((rw::int32)((rw::uint32)this->length - this->consumed));
        this->consumed = (rw::uint32)this->length;
    }
}

void RawChunkInit(RawChunk& raw) noexcept {
    raw.data = nullptr;
    raw.size = 0;
}

void RawChunkFree(RawChunk& raw) noexcept {
    if (raw.data) {
        rwFree(raw.data);
    }
    RawChunkInit(raw);
}

void RawChunkCopy(RawChunk& dst, const RawChunk& src) {
    RawChunkFree(dst);
    if (!src.data || !src.size) {
        return;
    }
    dst.data = static_cast<rw::uint8*>(rwMalloc(src.size, kMemHint));
    if (dst.data) {
        std::memcpy(dst.data, src.data, src.size);
        dst.size = src.size;
    }
}

void RawChunkRead(ChunkCursor& cursor, RawChunk& raw) {
    RawChunkFree(raw);
    if (cursor.length <= 0) {
        return;
    }
    raw.data = static_cast<rw::uint8*>(rwMalloc((size_t)cursor.length, kMemHint));
    if (!raw.data) {
        cursor.SkipRest();
        return;
    }
    raw.size = cursor.Read(raw.data, (rw::uint32)cursor.length);
}

void RawChunkWrite(rw::Stream* stream, const RawChunk& raw) {
    if (raw.data && raw.size) {
        stream->write8(raw.data, raw.size);
    }
}

} // namespace notsa::fakerw::plugins::details

namespace notsa::fakerw::plugins {

void RegisterRockstarPlugins() {
    // Every plugin reuses its offset when it is already registered, so this is safe to call
    // again (and after an engine term/init cycle, when librw dropped the plugin lists).
    RegisterNodeNamePlugin();
    RegisterCollisionPlugin();
    RegisterTwoDEffectPlugin();
    RegisterPipelinePlugin();
    RegisterBreakablePlugin();
    RegisterExtraVertColourPlugin();
    RegisterEnvMatPlugin();
    RegisterSpecMatPlugin();
}

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
