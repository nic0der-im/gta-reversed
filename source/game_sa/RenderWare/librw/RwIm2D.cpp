#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

#include <vector>

//! Native immediate mode: `RwIm2D*` (`RWSRCGLOBAL(dOpenDevice).fpIm2D*`), `RwIm3D*` and the
//! screen-space helpers.
//!
//! The game's vertices are the D3D9 layouts and librw's are not: `RwIm2DVertex` carries the
//! pre-transformed `x, y, z, rhw` plus a packed `0xAARRGGBB` colour, while librw's `Im2DVertex`
//! wants the camera-space `w`, split colour channels and a (perspective-correct) `u, v`.
//! `RwIm3DVertex` only differs in the colour byte order. Both are converted into reusable
//! buffers; the converted data is consumed synchronously by librw's GL upload, so one buffer per
//! kind is enough.
//!
//! `RwIm2DGetNearScreenZ`/`RwIm2DGetFarScreenZ` are macros over `dOpenDevice.zBufferNear/Far`,
//! which `SetupDeviceState` fills with the GL depth range (0..1). `RwIm2DVertexGetRecipCameraZ`
//! is a macro over the game vertex itself.

namespace {

std::vector<rw::gl3::Im2DVertex> g_im2dVertices;
std::vector<rw::gl3::Im3DVertex> g_im3dVertices;

//! The game packs vertex colours as `0xAARRGGBB`; librw stores one byte per channel.
void ReadColor(RwUInt32 argb, RwUInt8& red, RwUInt8& green, RwUInt8& blue, RwUInt8& alpha) {
    red   = (RwUInt8)((argb >> 16) & 0xFF);
    green = (RwUInt8)((argb >> 8) & 0xFF);
    blue  = (RwUInt8)(argb & 0xFF);
    alpha = (RwUInt8)((argb >> 24) & 0xFF);
}

rw::PrimitiveType ToLibrwPrimitive(RwPrimitiveType type) {
    return static_cast<rw::PrimitiveType>(type); // The enumerations match, `rwPRIMTYPELINELIST` first
}

//! Converts one game vertex: the D3D9 pre-transformed layout (x, y, z, rhw, 0xAARRGGBB, u, v)
//! into the one librw's GL3 immediate mode draws.
void ConvertIm2D(const RwIm2DVertex& s, rw::gl3::Im2DVertex& d) {
    d.setScreenX(s.x);
    d.setScreenY(s.y);
    d.setScreenZ(s.z);

    // `rhw` is 1/cameraZ; librw wants the camera Z itself to undo GL's divide.
    if (s.rhw != 0.0f) {
        d.setRecipCameraZ(s.rhw);
    } else {
        d.setCameraZ(1.0f);
    }

    RwUInt8 red, green, blue, alpha;
    ReadColor(s.emissiveColor, red, green, blue, alpha);
    d.setColor(red, green, blue, alpha);

    d.setU(s.u, s.rhw);
    d.setV(s.v, s.rhw);
}

void ConvertIm2D(const RwIm2DVertex* src, RwInt32 numVertices) {
    g_im2dVertices.resize(numVertices);
    for (RwInt32 i = 0; i < numVertices; i++) {
        ConvertIm2D(src[i], g_im2dVertices[i]);
    }
}

void ConvertIm3D(const RwIm3DVertex* src, RwInt32 numVertices) {
    g_im3dVertices.resize(numVertices);

    for (RwInt32 i = 0; i < numVertices; i++) {
        const RwIm3DVertex& s = src[i];
        rw::gl3::Im3DVertex& d = g_im3dVertices[i];

        d.position = { s.objVertex.x, s.objVertex.y, s.objVertex.z };
        d.normal   = { s.objNormal.x, s.objNormal.y, s.objNormal.z };

        ReadColor(s.color, d.r, d.g, d.b, d.a);

        d.u = s.u;
        d.v = s.v;
    }
}

} // namespace

//-----------------------------------------------------------------------------
// Im2D
//-----------------------------------------------------------------------------

namespace notsa::fakerw {

RwBool DeviceIm2DRenderLine(RwIm2DVertex* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2) {
    if (!vertices || vert1 < 0 || vert2 < 0 || vert1 >= numVertices || vert2 >= numVertices) {
        return FALSE;
    }
    // librw only reads the referenced vertices, so only those are converted.
    rw::gl3::Im2DVertex converted[2];
    ConvertIm2D(vertices[vert1], converted[0]);
    ConvertIm2D(vertices[vert2], converted[1]);
    rw::im2d::RenderLine(converted, 2, 0, 1);
    return TRUE;
}

RwBool DeviceIm2DRenderTriangle(RwIm2DVertex* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2, RwInt32 vert3) {
    if (!vertices || vert1 < 0 || vert2 < 0 || vert3 < 0 || vert1 >= numVertices || vert2 >= numVertices || vert3 >= numVertices) {
        return FALSE;
    }
    rw::gl3::Im2DVertex converted[3];
    ConvertIm2D(vertices[vert1], converted[0]);
    ConvertIm2D(vertices[vert2], converted[1]);
    ConvertIm2D(vertices[vert3], converted[2]);
    rw::im2d::RenderTriangle(converted, 3, 0, 1, 2);
    return TRUE;
}

RwBool DeviceIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices) {
    if (!vertices || numVertices <= 0) {
        return FALSE;
    }
    ConvertIm2D(vertices, numVertices);
    rw::im2d::RenderPrimitive(ToLibrwPrimitive(primType), g_im2dVertices.data(), numVertices);
    return TRUE;
}

RwBool DeviceIm2DRenderIndexedPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices, RwImVertexIndex* indices, RwInt32 numIndices) {
    if (!vertices || numVertices <= 0 || !indices || numIndices <= 0) {
        return FALSE;
    }
    ConvertIm2D(vertices, numVertices);
    rw::im2d::RenderIndexedPrimitive(ToLibrwPrimitive(primType), g_im2dVertices.data(), numVertices, indices, numIndices);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Im3D
//-----------------------------------------------------------------------------

RwBool DeviceIm3DRenderLine(RwInt32 vert1, RwInt32 vert2) {
    rw::im3d::RenderLine(vert1, vert2);
    return TRUE;
}

RwBool DeviceIm3DRenderTriangle(RwInt32 vert1, RwInt32 vert2, RwInt32 vert3) {
    rw::im3d::RenderTriangle(vert1, vert2, vert3);
    return TRUE;
}

RwBool DeviceIm3DRenderPrimitive(RwPrimitiveType primType) {
    rw::im3d::RenderPrimitive(ToLibrwPrimitive(primType));
    return TRUE;
}

RwBool DeviceIm3DRenderIndexedPrimitive(RwPrimitiveType primType, RwImVertexIndex* indices, RwInt32 numIndices) {
    rw::im3d::RenderIndexedPrimitive(ToLibrwPrimitive(primType), indices, numIndices);
    return TRUE;
}

} // namespace notsa::fakerw

// The game calls these directly (they are real functions, not device macros).
void* RwIm3DTransform(RwIm3DVertex* pVerts, RwUInt32 numVerts, RwMatrix* ltm, RwUInt32 flags) {
    if (!pVerts || numVerts == 0) {
        return nullptr;
    }

    ConvertIm3D(pVerts, (RwInt32)numVerts);

    rw::Matrix world;
    rw::Matrix* worldPtr = nullptr;
    if (ltm) {
        world    = notsa::fakerw::ToLibrw(*ltm);
        worldPtr = &world;
    }

    rw::im3d::Transform(g_im3dVertices.data(), (rw::int32)numVerts, worldPtr, flags);
    return pVerts;
}

RwBool RwIm3DEnd() {
    rw::im3d::End();
    return TRUE;
}

RwBool RwIm3DRenderLine(RwInt32 vert1, RwInt32 vert2) {
    return notsa::fakerw::DeviceIm3DRenderLine(vert1, vert2);
}

RwBool RwIm3DRenderTriangle(RwInt32 vert1, RwInt32 vert2, RwInt32 vert3) {
    return notsa::fakerw::DeviceIm3DRenderTriangle(vert1, vert2, vert3);
}

RwBool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primType, RwImVertexIndex* indices, RwInt32 numIndices) {
    return notsa::fakerw::DeviceIm3DRenderIndexedPrimitive(primType, indices, numIndices);
}

RwBool RwIm3DRenderPrimitive(RwPrimitiveType primType) {
    return notsa::fakerw::DeviceIm3DRenderPrimitive(primType);
}

#endif // NOTSA_NATIVE
