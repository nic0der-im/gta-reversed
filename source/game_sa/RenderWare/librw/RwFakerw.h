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

    //! Stream callbacks are registered but never dispatched: no DFF/TXD reader exists yet.
    int32 RegisterStream(
        uint32                           pluginID,
        RwPluginDataChunkReadCallBack    readCB,
        RwPluginDataChunkWriteCallBack   writeCB,
        RwPluginDataChunkGetSizeCallBack getSizeCB
    );

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

    std::vector<Entry> m_plugins;
    size_t             m_base = 0;
    size_t             m_end  = 0;
};

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

} // namespace notsa::fakerw

#endif // NOTSA_NATIVE
