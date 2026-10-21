#pragma once

//! Native RenderWare ("fakerw") core on top of librw.
//!
//! In the native build (`NOTSA_NATIVE`) the retail RenderWare thunks in
//! `game_sa/RenderWare/rw/` are compiled out: they call into `gta_sa.exe`, which does not
//! exist there. This directory implements the same C entry points (`Rw*`, `Rp*`, `Rt*`) that
//! the rest of the game calls, so the shared game code keeps compiling and running unchanged.
//!
//! Only the startup path is implemented: the engine lifecycle, memory functions, streams,
//! cameras, rasters, frames and the matrix/vector math. Everything else logs
//! `Rw: stub <name>` once and returns a neutral value (see `RwStubs.generated.cpp`).
//!
//! The game object structs (from the plugin-sdk headers) stay the API surface, because the game
//! reads their fields directly (`camera->farPlane`, `raster->width`, ...). Where librw needs a
//! real object (camera/rasters) this layer keeps a side table mapping the game object to it.
//!
//! Include `StdInc.h` before this header: it relies on the game's RW types.

#ifdef NOTSA_NATIVE

#include "Librw.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace notsa::fakerw {

//! Log `Rw: stub <name>` once per name (the call itself is a no-op).
void Stub(const char* name);

//! Log `Rw: <message>` once per message.
void WarnOnce(const char* message);

//-----------------------------------------------------------------------------
// Memory
//-----------------------------------------------------------------------------

//! The memory callbacks the game installed through `RwEngineInit` (`psGetMemoryFunctions`).
const RwMemoryFunctions& MemoryFunctions();

void* Malloc(size_t size);
void* Realloc(void* p, size_t size);
void  Free(void* p);

//! Moves a stream (`whence`: 0 = set, 1 = current, 2 = end). Defined in `RwStream.cpp`, which
//! is the only place that knows the stream layout. Used by readers that rewind over a chunk
//! header they already consumed.
RwBool SeekRwStream(RwStream* stream, int32 offset, int32 whence);

//-----------------------------------------------------------------------------
// Engine
//-----------------------------------------------------------------------------

//! True once librw's engine was fully started (`RwEngineStart` succeeded).
bool IsStarted();

//! The `SDL_Window*` created by librw in `RwEngineStart`. Null before that and after stop.
void* GetWindow();

//! `RsGlobal.maximumWidth/Height` are the desired size at the time `RwEngineOpen` runs.
void GetRequestedSize(int32& width, int32& height);

//-----------------------------------------------------------------------------
// Device entry points
//-----------------------------------------------------------------------------
//
// `RwRenderStateSet`, `RwIm2D*` and the `fpIm3D*` half of `RwIm3D*` are macros over
// `RWSRCGLOBAL(dOpenDevice)`; `RwEngineInit` installs these on it. They translate the game's
// D3D9-flavoured state numbers/vertex layouts onto librw. See `RwRenderState.cpp`, `RwIm2D.cpp`.

RwBool DeviceSystem(RwInt32 request, void* out, void* in, RwInt32 numIn);

RwBool DeviceRenderStateSet(RwRenderState state, void* value);
RwBool DeviceRenderStateGet(RwRenderState state, void* value);

RwBool DeviceIm2DRenderLine(RwIm2DVertex* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2);
RwBool DeviceIm2DRenderTriangle(RwIm2DVertex* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2, RwInt32 vert3);
RwBool DeviceIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices);
RwBool DeviceIm2DRenderIndexedPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices, RwImVertexIndex* indices, RwInt32 numIndices);

RwBool DeviceIm3DRenderLine(RwInt32 vert1, RwInt32 vert2);
RwBool DeviceIm3DRenderTriangle(RwInt32 vert1, RwInt32 vert2, RwInt32 vert3);
RwBool DeviceIm3DRenderPrimitive(RwPrimitiveType primType);
RwBool DeviceIm3DRenderIndexedPrimitive(RwPrimitiveType primType, RwImVertexIndex* indices, RwInt32 numIndices);

//-----------------------------------------------------------------------------
// Object plugin data
//-----------------------------------------------------------------------------
//
// RenderWare lets plugins reserve `size` bytes after an object. The game relies on the
// returned byte offset (`RWPLUGINOFFSET`), so objects created by this layer are allocated with
// that trailing space and the plugin callbacks are invoked here.

class PluginRegistry {
public:
    //! Registers plugin data behind an object of `baseSize` bytes. Returns the offset, or -1.
    int32 Register(
        int32                     baseSize,
        int32                     size,
        uint32                    pluginID,
        RwPluginObjectConstructor constructCB,
        RwPluginObjectDestructor  destructCB,
        RwPluginObjectCopy        copyCB
    );

    //! Registers the stream callbacks of the plugin identified by `pluginID`. Returns the byte
    //! offset of the data plugin registered with the same ID (`0` when there is none, which is
    //! also the offset of a zero-sized plugin registered by `Register`).
    int32 RegisterStream(
        uint32                           pluginID,
        RwPluginDataChunkReadCallBack    readCB,
        RwPluginDataChunkWriteCallBack   writeCB,
        RwPluginDataChunkGetSizeCallBack getSizeCB
    );

    //! Reads the `rwID_EXTENSION` chunk found in `stream`, dispatching every subchunk to the
    //! read callback of the plugin registered with its ID. Subchunks of unknown plugins are
    //! skipped. `object` is the game object the plugin data belongs to.
    //!
    //! Returns `false` when the chunk contents could not be read. When the stream has no
    //! extension chunk at all there is nothing to dispatch and the call succeeds.
    bool StreamReadExtension(RwStream* stream, void* object) const;

    //! Bytes needed for one object: `baseSize` plus every registered plugin.
    size_t TotalSize(size_t baseSize) const;

    void Construct(void* object, size_t baseSize) const;
    void Destruct(void* object, size_t baseSize) const;
    void Copy(void* dst, const void* src, size_t baseSize) const;

private:
    struct Entry {
        int32                     offset;
        int32                     size;
        uint32                    id;
        RwPluginObjectConstructor ctor;
        RwPluginObjectDestructor  dtor;
        RwPluginObjectCopy        copy;
    };

    struct StreamEntry {
        uint32                           id;
        RwPluginDataChunkReadCallBack    read;
        RwPluginDataChunkWriteCallBack   write;
        RwPluginDataChunkGetSizeCallBack getSize;
    };

    const Entry*       FindData(uint32 pluginID) const;
    const StreamEntry* FindStream(uint32 pluginID) const;
    StreamEntry*       FindStream(uint32 pluginID);

    std::vector<Entry>       m_plugins;
    std::vector<StreamEntry> m_streams;
    size_t                   m_base = 0;
    size_t                   m_end  = 0;
};

//! The registry of the object type that carries plugin data. The stream readers in `RwDff.cpp`
//! dispatch `rwID_EXTENSION` chunks through these, so they must be the same instances the
//! game's plugins register with (`RpGeometryRegisterPluginStream`, `RwFrameRegisterPlugin`, ...).
PluginRegistry& MaterialPlugins();
PluginRegistry& GeometryPlugins();
PluginRegistry& AtomicPlugins();
PluginRegistry& ClumpPlugins();
PluginRegistry& FramePlugins();

//! Zeroed allocation of `baseSize` bytes plus the plugin area described by `registry`.
void* AllocObject(size_t baseSize, const PluginRegistry& registry);

//! Runs the plugin destructors and releases an object from `AllocObject`.
void FreeObject(void* object, size_t baseSize, const PluginRegistry& registry);

//-----------------------------------------------------------------------------
// Circular lists (`RwLinkList` holds a sentinel `RwLLLink`)
//-----------------------------------------------------------------------------

inline void ListInit(RwLinkList& list) {
    list.link.next = &list.link;
    list.link.prev = &list.link;
}

inline bool ListEmpty(const RwLinkList& list) {
    return list.link.next == &list.link;
}

inline void ListAppend(RwLinkList& list, RwLLLink& link) {
    link.prev          = list.link.prev;
    link.next          = &list.link;
    list.link.prev->next = &link;
    list.link.prev     = &link;
}

inline void ListRemove(RwLLLink& link) {
    if (link.next) {
        link.next->prev = link.prev;
    }
    if (link.prev) {
        link.prev->next = link.next;
    }
    link.next = link.prev = nullptr;
}

//-----------------------------------------------------------------------------
// Conversions between the game structs and librw
//-----------------------------------------------------------------------------

inline rw::V3d ToLibrw(const RwV3d& v) { return {v.x, v.y, v.z}; }
inline rw::V2d ToLibrw(const RwV2d& v) { return {v.x, v.y}; }

//! Both structs declare their corners in the same order (`sup` first), so keep it.
inline rw::BBox ToLibrw(const RwBBox& box) {
    return { ToLibrw(box.sup), ToLibrw(box.inf) };
}

inline rw::Matrix ToLibrw(const RwMatrix& m) {
    rw::Matrix out{};
    out.right = ToLibrw(m.right);
    out.up    = ToLibrw(m.up);
    out.at    = ToLibrw(m.at);
    out.pos   = ToLibrw(m.pos);
    return out;
}

//! The game object that owns a `rw::V3d*` pointer, for the RW matrix API.
inline RwV3d* FromLibrw(rw::V3d& v) { return reinterpret_cast<RwV3d*>(&v); }

//! librw raster behind a game raster (null when the raster was not created by this layer).
rw::Raster* GetLibrwRaster(RwRaster* raster);

//! Game raster behind a librw raster (null when the raster was not created by this layer).
RwRaster* GetGameRaster(rw::Raster* raster);

//! librw world behind a game world (null when the world was not created by this layer).
rw::World* GetLibrwWorld(RpWorld* world);

//! Game world behind a librw world (null when the world was not created by this layer).
RpWorld* GetGameWorld(rw::World* world);

//! librw light behind a game light, refreshed from the authoritative game struct (null when the
//! light has no mirror). Defined in `RwLight.cpp`.
rw::Light* GetLibrwLight(RpLight* light);

//! librw camera behind a game camera, creating its mirror on demand (null on allocation
//! failure). Defined in `RwCamera.cpp`.
rw::Camera* GetLibrwCamera(RwCamera* camera);

} // namespace notsa::fakerw

#endif // NOTSA_NATIVE
