#pragma once

//! GTA San Andreas ("Rockstar") RenderWare chunks as librw plugins.
//!
//! librw implements the stock RenderWare chunks and silently skips everything Rockstar added
//! (vendor id 0x0253F2). These plugins register stream callbacks on the librw object types, so
//! the chunks are read from, and written back to, PC DFF/TXD files with the retail layout, and
//! expose the data the game consumes.
//!
//! `RegisterRockstarPlugins()` must be called once, right after `rw::Engine::init()` (see
//! `RwEngineInit` in `RwFakerw.cpp`), before any clump or texture dictionary is streamed in:
//! librw sizes every object with the plugins registered at creation time.
//!
//! Chunk ids are `MAKECHUNKID(rwVENDORID_DEVELOPER, lowByte)` == `(0x0253F2 << 8) | lowByte`.
//!
//! The data lives on the *librw* objects (`rw::Frame`, `rw::Geometry`, `rw::Atomic`,
//! `rw::Material`, `rw::Clump`). The game structs (`RwFrame`, `RpGeometry`, ...) keep using their
//! own registrations (`RwFrameRegisterPlugin`, `RpGeometryRegisterPlugin`, ... from
//! `Plugins/` and `Pipelines/`), so a bridge that copies a streamed librw object into a game
//! object must copy the plugin data across using the accessors below.

#ifdef NOTSA_NATIVE

#include "../Librw.h"

namespace notsa::fakerw::plugins {

//! `MAKECHUNKID(rwVENDORID_DEVELOPER, lowByte)`: the Rockstar vendor id is 0x0253F2.
constexpr rw::uint32 RockstarChunkId(rw::uint32 lowByte) {
    return (0x0253F2u << 8) | (lowByte & 0xFFu);
}

//! Registers every plugin below on the librw object types that carry them.
void RegisterRockstarPlugins();

// Per-plugin registration functions, called by `RegisterRockstarPlugins()` in this order.
void RegisterNodeNamePlugin();
void RegisterCollisionPlugin();
void RegisterTwoDEffectPlugin();
void RegisterPipelinePlugin();
void RegisterBreakablePlugin();
void RegisterExtraVertColourPlugin();
void RegisterEnvMatPlugin();
void RegisterSpecMatPlugin();

//-----------------------------------------------------------------------------
// Data exposed to the game
//-----------------------------------------------------------------------------

//! Verbatim payload of a chunk whose parser lives in the game code.
struct RawChunk {
    rw::uint8* data = nullptr;
    rw::uint32 size = 0;
};

//-----------------------------------------------------------------------------
// NodeName (frame)
//-----------------------------------------------------------------------------

//! Chunk 0x0253F2FE. Payload: the node name as ASCII, at most `kNodeNameLength` bytes.
constexpr rw::int32 kNodeNameLength = 23; //!< Longest name, terminator excluded (as in retail).

//! Node name of `frame`, or nullptr when the plugin is not attached to it.
const char* RwFrameGetNodeName(const rw::Frame* frame);

//! Copies at most `kNodeNameLength` characters of `name` into the frame's plugin data.
void RwFrameSetNodeName(rw::Frame* frame, const char* name);

//-----------------------------------------------------------------------------
// Collision (clump)
//-----------------------------------------------------------------------------

//! Chunk 0x0253F2FA. Payload: the raw `COLL`/`COL2`/`COL3` block read by `CBaseModelInfo`.
//! Retail registers it on the clump; the geometry list is covered as well in case a DFF puts the
//! chunk inside a geometry extension.
const RawChunk* RpClumpGetCollisionPlugin(const rw::Clump* clump);
const RawChunk* RpGeometryGetCollisionPlugin(const rw::Geometry* geometry);

//-----------------------------------------------------------------------------
// 2dEffect (geometry)
//-----------------------------------------------------------------------------

//! Chunk 0x0253F2F8. Payload: `uint32 count`, then `count` entries laid out as
//! `CVector pos; uint32 type; uint32 size; uint8 data[size];` (see `2dEffectStream.h`).
struct TwoDEffects {
    rw::uint32 count = 0; //!< Number of effects at the start of `chunk.data`.
    RawChunk   chunk{};   //!< The whole payload, count included.
};

const TwoDEffects* RpGeometryGet2dFxPlugin(const rw::Geometry* geometry);

//! Number of effects, 0 when the geometry has none.
rw::uint32 RpGeometryGet2dFxCount(const rw::Geometry* geometry);

//-----------------------------------------------------------------------------
// Pipeline id (atomic)
//-----------------------------------------------------------------------------

//! Chunk 0x0253F2F3. Payload: `uint32 pipelineId`.
rw::uint32 RpAtomicGetPipelineID(const rw::Atomic* atomic);
void       RpAtomicSetPipelineID(rw::Atomic* atomic, rw::uint32 pipelineId);

//-----------------------------------------------------------------------------
// Breakable (geometry)
//-----------------------------------------------------------------------------

//! Chunk 0x0253F2FD. Payload: `uint32 sectionHeader`, then the break data when it is non-zero.
const RawChunk* RpGeometryGetBreakablePlugin(const rw::Geometry* geometry);

//-----------------------------------------------------------------------------
// Extra vertex colours / night colours (geometry)
//-----------------------------------------------------------------------------

//! Chunk 0x0253F2F9. Payload: `uint32 hasColors`, then `numVertices` colours (RGBA, 4 bytes).
struct ExtraVertColours {
    rw::RGBA*  nightColours = nullptr; //!< `count` entries, owned by the plugin.
    rw::uint32 count        = 0;
    rw::float32 dnBalance   = 0.0f; //!< Day/night blend; 1.0 right after loading (as in retail).
};

const ExtraVertColours* RpGeometryGetExtraVertColourPlugin(const rw::Geometry* geometry);

//-----------------------------------------------------------------------------
// EnvMap material (material)
//-----------------------------------------------------------------------------

//! Chunk 0x0253F2FC. Payload: 24 bytes (`RwV2d` scale, `RwV2d` translation scale, shininess, and
//! the texture pointer slot). Retail keeps a valid default for every material (scale 1/1), and
//! always writes the chunk, so the accessor returns data as soon as the plugin is attached.
struct EnvMat {
    bool        present     = false; //!< True once the chunk was read from a stream.
    rw::float32 scaleX      = 1.0f;
    rw::float32 scaleY      = 1.0f;
    rw::float32 transScaleX = 1.0f;
    rw::float32 transScaleY = 1.0f;
    rw::float32 shininess   = 0.0f;
    //! The value stored on disk: a 32-bit pointer from the D3D9 era. Never dereferenced.
    rw::uint32  texturePointer = 0;
    //! Resolved at runtime by the car env map pipeline; not written back to the stream.
    rw::Texture* texture = nullptr;
};

const EnvMat* RpMaterialGetEnvMatPlugin(const rw::Material* material);

//-----------------------------------------------------------------------------
// Specular material (material)
//-----------------------------------------------------------------------------

//! Chunk 0x0253F2F6. Payload: `float specularity; char textureName[24];` (28 bytes).
//! Retail always writes the chunk (zero-filled for materials without specular data) and only
//! keeps data when the specularity is non-zero, which is what `present` reports.
struct SpecMat {
    bool        present     = false;
    rw::float32 specularity = 0.0f;
    char        textureName[24]{}; //!< NUL terminated.
};

const SpecMat* RpMaterialGetSpecMatPlugin(const rw::Material* material);

} // namespace notsa::fakerw::plugins

#endif // NOTSA_NATIVE
