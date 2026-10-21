#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

//! Direct3D 9 has no counterpart in the native build: the GL3 backend replaces it (plan B3).
//! Everything here accepts and ignores its arguments, returning neutral values, and never logs
//! per call (the game calls these every frame).

namespace {
rwD3D9DeviceRestoreCallBack g_restoreCallBack = nullptr;
}

RwBool RwD3D9CameraAttachWindow(void* camera, void* hwnd) {
    UNUSED(camera, hwnd);
    return TRUE;
}

RwBool RwD3D9ChangeMultiSamplingLevels(RwUInt32 numLevels) {
    UNUSED(numLevels);
    return TRUE;
}

RwBool RwD3D9ChangeVideoMode(RwInt32 modeIndex) {
    UNUSED(modeIndex);
    return TRUE;
}

RwBool RwD3D9DeviceSupportsDXTTexture() {
    // The GL3 backend supports S3TC on this hardware (see spikes/librw/FINDINGS.md).
    return TRUE;
}

RwBool RwD3D9EnableLight(RwInt32 index, RwBool enable) {
    UNUSED(index, enable);
    return TRUE;
}

RwUInt32 RwD3D9EngineGetMaxMultiSamplingLevels() {
    return 0;
}

void RwD3D9EngineSetMultiSamplingLevels(RwUInt32 numLevels) {
    UNUSED(numLevels);
}

void RwD3D9EngineSetRefreshRate(RwUInt32 refreshRate) {
    UNUSED(refreshRate);
}

const void* RwD3D9GetCaps() {
    return nullptr;
}

void* RwD3D9GetCurrentD3DDevice() {
    return nullptr;
}

void RwD3D9GetRenderState(RwUInt32 state, void* value) {
    UNUSED(state, value);
}

RwBool RwD3D9SetLight(RwInt32 index, const void* light) {
    UNUSED(index, light);
    return TRUE;
}

RwBool RwD3D9SetMaterial(const void* material) {
    UNUSED(material);
    return TRUE;
}

void RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value) {
    UNUSED(state, value);
}

void RwD3D9SetStencilClear(RwUInt32 stencilClear) {
    UNUSED(stencilClear);
}

RwBool RwD3D9SetSurfaceProperties(const RwSurfaceProperties* surfaceProps, const RwRGBA* color, RwUInt32 flags) {
    UNUSED(surfaceProps, color, flags);
    return TRUE;
}

RwBool RwD3D9SetTexture(RwTexture* texture, RwUInt32 stage) {
    UNUSED(texture, stage);
    return TRUE;
}

void RwD3D9SetTextureStageState(RwUInt32 stage, RwUInt32 type, RwUInt32 value) {
    UNUSED(stage, type, value);
}

RwBool RwD3D9SetTransform(RwUInt32 state, const void* matrix) {
    UNUSED(state, matrix);
    return TRUE;
}

rwD3D9DeviceRestoreCallBack _rwD3D9DeviceGetRestoreCallback() {
    return g_restoreCallBack;
}

void _rwD3D9DeviceSetRestoreCallback(rwD3D9DeviceRestoreCallBack callback) {
    g_restoreCallBack = callback;
}

void _rwD3D9SetStreams(const RxD3D9VertexStream* streams, RwBool useOffsets) {
    UNUSED(streams, useOffsets);
}

//-----------------------------------------------------------------------------
// D3DResourceSystem (game-side wrapper around D3D9 buffers)
//-----------------------------------------------------------------------------

void D3DResourceSystem::CancelBuffering() {}

uint32 D3DResourceSystem::GetTotalIndexDataSize() {
    return 0;
}

uint32 D3DResourceSystem::GetTotalPixelsSize() {
    return 0;
}

void D3DResourceSystem::Init() {}

void D3DResourceSystem::SetUseD3DResourceBuffering(bool bUse) {
    UNUSED(bUse);
}

void D3DResourceSystem::Shutdown() {}

void D3DResourceSystem::TidyUpD3DIndexBuffers(uint32 count) {
    UNUSED(count);
}

void D3DResourceSystem::TidyUpD3DTextures(uint32 count) {
    UNUSED(count);
}

int32 D3DResourceSystem::CreateIndexBuffer(uint32 numIndices, uint32 format, void** ppIndexBuffer) {
    UNUSED(numIndices, format);
    if (ppIndexBuffer) {
        *ppIndexBuffer = nullptr;
    }
    return 0;
}

int32 D3DResourceSystem::CreateTexture(int32 width, int32 height, uint32 format, void** ppTexture) {
    UNUSED(width, height, format);
    if (ppTexture) {
        *ppTexture = nullptr;
    }
    return 0;
}

void D3DResourceSystem::DestroyIndexBuffer(void* pIndexBuffer) {
    UNUSED(pIndexBuffer);
}

void D3DResourceSystem::DestroyTexture(void* texture) {
    UNUSED(texture);
}

#endif // NOTSA_NATIVE
