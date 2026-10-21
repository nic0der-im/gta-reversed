#pragma once

//! Helpers shared by the Rockstar plugin implementations (`RwPlugins*.cpp`).
//!
//! Each stream callback receives the chunk length from the DFF header; `ChunkCursor` tracks how
//! much of it the callback consumed so the remainder can be skipped. librw's plugin reader
//! subtracts the whole chunk length from the extension length regardless of what the callback
//! does, so leaving bytes unread would desynchronise the stream.

#ifdef NOTSA_NATIVE

#include "RwPlugins.h"

namespace notsa::fakerw::plugins::details {

//! Read cursor over one plugin chunk.
struct ChunkCursor {
    rw::Stream* stream   = nullptr;
    rw::int32   length   = 0; //!< Chunk length from the stream header.
    rw::uint32  consumed = 0;

    //! Reads `size` bytes; bytes past the end of the chunk are zero-filled.
    rw::uint32 Read(void* dst, rw::uint32 size);

    //! Skips whatever the chunk still has after `consumed`.
    void SkipRest();
};

//! Allocation tag for the memory owned by these plugins (same one librw uses for geometry).
constexpr rw::uint32 kMemHint = rw::MEMDUR_EVENT | rw::ID_GEOMETRY;

void RawChunkInit(RawChunk& raw) noexcept;
void RawChunkFree(RawChunk& raw) noexcept;
void RawChunkCopy(RawChunk& dst, const RawChunk& src);

//! Reads the whole chunk (all `cursor.length` bytes) into `raw`.
void RawChunkRead(ChunkCursor& cursor, RawChunk& raw);

//! Writes the whole payload; the caller wrote the chunk header.
void RawChunkWrite(rw::Stream* stream, const RawChunk& raw);

} // namespace notsa::fakerw::plugins::details

#endif // NOTSA_NATIVE
