#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

#include <unordered_map>

//! Cameras and rasters.
//!
//! The game reads `RwCamera`/`RwRaster` fields directly (`farPlane`, `viewWindow`, `width`,
//! `stride`, ...), so the game structs stay authoritative. librw objects are created next to
//! them and kept in side tables; `RwCameraBeginUpdate` copies the game state into the librw
//! camera right before presenting.

namespace {

notsa::fakerw::PluginRegistry g_cameraPlugins;
notsa::fakerw::PluginRegistry g_rasterPlugins;

struct CameraMirror {
    rw::Camera* camera = nullptr;
    rw::Frame*  frame  = nullptr;
};

std::unordered_map<RwCamera*, CameraMirror> g_cameraMirrors;
std::unordered_map<RwRaster*, rw::Raster*>  g_rasterMirrors;
std::unordered_map<rw::Raster*, RwRaster*>  g_rasterOwners;

void SetIdentity(RwMatrix& m) {
    m.right = { 1.0f, 0.0f, 0.0f };
    m.up    = { 0.0f, 1.0f, 0.0f };
    m.at    = { 0.0f, 0.0f, 1.0f };
    m.pos   = { 0.0f, 0.0f, 0.0f };
    m.flags = rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL;
}

rw::Raster* LookupRaster(const RwRaster* raster) {
    if (!raster) {
        return nullptr;
    }
    const auto it = g_rasterMirrors.find(const_cast<RwRaster*>(raster));
    return it != g_rasterMirrors.end() ? it->second : nullptr;
}

//! Copies the game camera state into its librw mirror. Returns false when the camera is not
//! ready to render (no frame or no camera raster).
bool SyncCameraMirror(RwCamera* camera, CameraMirror& mirror) {
    if (!camera->frameBuffer) {
        return false;
    }

    auto* frameBuffer = LookupRaster(camera->frameBuffer);
    if (!frameBuffer) {
        notsa::fakerw::WarnOnce("Rw: camera raster was not created through RwRasterCreate");
        return false;
    }

    // The frame transform is what makes the camera look somewhere.
    auto* gameFrame = static_cast<RwFrame*>(rwObjectGetParent(&camera->object.object));
    if (!gameFrame) {
        return false;
    }
    const RwMatrix* ltm = RwFrameGetLTM(gameFrame);

    mirror.frame->matrix = notsa::fakerw::ToLibrw(*ltm);
    mirror.frame->ltm    = mirror.frame->matrix;
    mirror.frame->object.flags = 0;

    auto* lcam = mirror.camera;
    lcam->frameBuffer  = frameBuffer;
    lcam->zBuffer      = LookupRaster(camera->zBuffer);
    lcam->viewWindow   = notsa::fakerw::ToLibrw(camera->viewWindow);
    lcam->viewOffset   = notsa::fakerw::ToLibrw(camera->viewOffset);
    lcam->nearPlane    = camera->nearPlane;
    lcam->farPlane     = camera->farPlane;
    lcam->fogPlane     = camera->fogPlane;
    lcam->projection   = camera->projectionType == rwPARALLEL
        ? rw::Camera::PARALLEL
        : rw::Camera::PERSPECTIVE;

    return true;
}

CameraMirror* EnsureMirror(RwCamera* camera) {
    auto it = g_cameraMirrors.find(camera);
    if (it != g_cameraMirrors.end()) {
        return &it->second;
    }

    auto* lcam = rw::Camera::create();
    auto* lframe = rw::Frame::create();
    if (!lcam || !lframe) {
        return nullptr;
    }
    lcam->setFrame(lframe);

    return &g_cameraMirrors.emplace(camera, CameraMirror{ lcam, lframe }).first->second;
}

//! `RwObjectHasFrame::sync` for cameras: keep the librw mirror's transform current.
RwObjectHasFrame* CameraSync(RwObjectHasFrame* object) {
    auto* camera = reinterpret_cast<RwCamera*>(object);
    if (auto* mirror = EnsureMirror(camera)) {
        SyncCameraMirror(camera, *mirror);
    }
    return object;
}

void DestroyMirror(RwCamera* camera) {
    const auto it = g_cameraMirrors.find(camera);
    if (it == g_cameraMirrors.end()) {
        return;
    }

    auto& mirror = it->second;
    mirror.camera->setFrame(nullptr);
    mirror.camera->destroy();
    mirror.frame->destroy();
    g_cameraMirrors.erase(it);
}

} // namespace

//-----------------------------------------------------------------------------
// Cameras
//-----------------------------------------------------------------------------

RwCamera* RwCameraCreate() {
    auto* camera = static_cast<RwCamera*>(notsa::fakerw::AllocObject(sizeof(RwCamera), g_cameraPlugins));
    if (!camera) {
        return nullptr;
    }

    rwObjectInitialize(&camera->object.object, rwID_CAMERA, 0);
    camera->object.sync = CameraSync;

    camera->projectionType = rwPERSPECTIVE;
    camera->beginUpdate    = RwCameraBeginUpdate;
    camera->endUpdate      = RwCameraEndUpdate;

    SetIdentity(camera->viewMatrix);

    camera->frameBuffer   = nullptr;
    camera->zBuffer       = nullptr;
    camera->viewWindow    = { 1.0f, 1.0f };
    camera->recipViewWindow = { 1.0f, 1.0f };
    camera->viewOffset    = { 0.0f, 0.0f };
    camera->nearPlane     = 0.05f;
    camera->farPlane      = 10.0f;
    camera->fogPlane      = 5.0f;

    g_cameraPlugins.Construct(camera, sizeof(RwCamera));
    return camera;
}

RwBool RwCameraDestroy(RwCamera* camera) {
    if (!camera) {
        return FALSE;
    }

    DestroyMirror(camera);

    if (camera->object.object.parent) {
        _rwObjectHasFrameSetFrame(camera, nullptr);
    }

    notsa::fakerw::FreeObject(camera, sizeof(RwCamera), g_cameraPlugins);
    return TRUE;
}

RwCamera* RwCameraClone(RwCamera* camera) {
    if (!camera) {
        return nullptr;
    }

    auto* clone = RwCameraCreate();
    if (!clone) {
        return nullptr;
    }

    clone->projectionType = camera->projectionType;
    clone->viewMatrix     = camera->viewMatrix;
    clone->frameBuffer    = camera->frameBuffer;
    clone->zBuffer        = camera->zBuffer;
    clone->viewWindow     = camera->viewWindow;
    clone->recipViewWindow = camera->recipViewWindow;
    clone->viewOffset     = camera->viewOffset;
    clone->nearPlane      = camera->nearPlane;
    clone->farPlane       = camera->farPlane;
    clone->fogPlane       = camera->fogPlane;
    clone->zScale         = camera->zScale;
    clone->zShift         = camera->zShift;
    clone->beginUpdate    = camera->beginUpdate;
    clone->endUpdate      = camera->endUpdate;

    g_cameraPlugins.Copy(clone, camera, sizeof(RwCamera));
    return clone;
}

RwCamera* RwCameraBeginUpdate(RwCamera* camera) {
    if (!camera) {
        return nullptr;
    }

    auto* mirror = EnsureMirror(camera);
    if (!mirror || !SyncCameraMirror(camera, *mirror)) {
        return nullptr;
    }

    mirror->camera->beginUpdate();
    return camera;
}

RwCamera* RwCameraEndUpdate(RwCamera* camera) {
    if (!camera) {
        return nullptr;
    }
    if (const auto it = g_cameraMirrors.find(camera); it != g_cameraMirrors.end()) {
        it->second.camera->endUpdate();
    }
    return camera;
}

RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, RwInt32 clearMode) {
    if (!camera) {
        return nullptr;
    }

    auto* mirror = EnsureMirror(camera);
    if (!mirror || !SyncCameraMirror(camera, *mirror)) {
        return nullptr;
    }

    mirror->camera->clear(reinterpret_cast<rw::RGBA*>(colour), (rw::uint32)clearMode);
    return camera;
}

RwCamera* RwCameraShowRaster(RwCamera* camera, void* pDev, RwUInt32 flags) {
    UNUSED(pDev);

    if (!camera) {
        return nullptr;
    }

    if (const auto it = g_cameraMirrors.find(camera); it != g_cameraMirrors.end()) {
        it->second.camera->showRaster(flags);
        RWSRCGLOBAL(renderFrame)++;
        return camera;
    }
    return nullptr;
}

RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, RwReal farClip) {
    if (camera) {
        camera->farPlane = farClip;
    }
    return camera;
}

RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, RwReal nearClip) {
    if (camera) {
        camera->nearPlane = nearClip;
    }
    return camera;
}

RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow) {
    if (camera && viewWindow) {
        camera->viewWindow      = *viewWindow;
        camera->recipViewWindow = { 1.0f / viewWindow->x, 1.0f / viewWindow->y };
    }
    return camera;
}

RwCamera* RwCameraSetViewOffset(RwCamera* camera, const RwV2d* offset) {
    if (camera && offset) {
        camera->viewOffset = *offset;
    }
    return camera;
}

RwCamera* RwCameraSetProjection(RwCamera* camera, RwCameraProjection projection) {
    if (camera) {
        camera->projectionType = projection;
    }
    return camera;
}

//-----------------------------------------------------------------------------
// Rasters
//-----------------------------------------------------------------------------

RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) {
    auto* raster = static_cast<RwRaster*>(notsa::fakerw::AllocObject(sizeof(RwRaster), g_rasterPlugins));
    if (!raster) {
        return nullptr;
    }

    // librw's raster format bits have the same layout as RenderWare's.
    auto* librwRaster = rw::Raster::create(width, height, depth, flags);
    if (!librwRaster) {
        notsa::fakerw::FreeObject(raster, sizeof(RwRaster), g_rasterPlugins);
        return nullptr;
    }

    raster->parent   = raster;
    raster->width    = width;
    raster->height   = height;
    raster->depth    = depth;
    raster->stride   = librwRaster->stride;
    raster->cType    = (RwUInt8)(flags & 0xFF);
    raster->cFlags   = (RwUInt8)((flags >> 8) & 0xFF);
    raster->cFormat  = (RwUInt8)((flags >> 8) & 0xFF);

    g_rasterMirrors.emplace(raster, librwRaster);
    g_rasterOwners.emplace(librwRaster, raster);

    g_rasterPlugins.Construct(raster, sizeof(RwRaster));
    return raster;
}

RwBool RwRasterDestroy(RwRaster* raster) {
    if (!raster) {
        return FALSE;
    }

    if (const auto it = g_rasterMirrors.find(raster); it != g_rasterMirrors.end()) {
        g_rasterOwners.erase(it->second);
        it->second->destroy();
        g_rasterMirrors.erase(it);
    }

    notsa::fakerw::FreeObject(raster, sizeof(RwRaster), g_rasterPlugins);
    return TRUE;
}

RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode) {
    auto* librwRaster = LookupRaster(raster);
    if (!librwRaster) {
        notsa::fakerw::Stub("RwRasterLock");
        return nullptr;
    }

    auto* pixels = librwRaster->lock(level, lockMode);
    if (pixels) {
        raster->cpPixels = pixels;
        raster->stride   = librwRaster->stride;
    }
    return pixels;
}

RwRaster* RwRasterUnlock(RwRaster* raster) {
    auto* librwRaster = LookupRaster(raster);
    if (!librwRaster) {
        return raster;
    }

    librwRaster->unlock(0);
    raster->cpPixels = nullptr;
    return raster;
}

RwRaster* RwRasterUnlockPalette(RwRaster* raster) {
    if (auto* librwRaster = LookupRaster(raster)) {
        librwRaster->unlockPalette();
    }
    return raster;
}

RwRaster* RwRasterPushContext(RwRaster* raster) {
    if (!raster) {
        return nullptr;
    }

    if (auto* librwRaster = LookupRaster(raster)) {
        return g_rasterOwners.contains(rw::Raster::pushContext(librwRaster)) ? raster : nullptr;
    }
    return nullptr;
}

RwRaster* RwRasterPopContext() {
    const auto it = g_rasterOwners.find(rw::Raster::popContext());
    return it != g_rasterOwners.end() ? it->second : nullptr;
}

RwRaster* RwRasterGetCurrentContext() {
    const auto it = g_rasterOwners.find(rw::Raster::getCurrentContext());
    return it != g_rasterOwners.end() ? it->second : nullptr;
}

RwRaster* RwRasterRenderFast(RwRaster* raster, RwInt32 x, RwInt32 y) {
    auto* librwRaster = LookupRaster(raster);
    if (librwRaster && librwRaster->renderFast(x, y)) {
        return raster;
    }
    return nullptr;
}

RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image) {
    auto* librwRaster = LookupRaster(raster);
    if (!librwRaster || !image) {
        return nullptr;
    }

    rw::Image source{};
    source.flags   = image->flags;
    source.width   = image->width;
    source.height  = image->height;
    source.depth   = image->depth;
    source.bpp     = image->depth / 8;
    source.stride  = image->stride;
    source.pixels  = image->cpPixels;
    source.palette = reinterpret_cast<rw::uint8*>(image->palette);

    return librwRaster->setFromImage(&source) ? raster : nullptr;
}

RwUInt32 RwRGBAToPixel(RwRGBA* rgbIn, RwInt32 rasterFormat) {
    if (!rgbIn) {
        return 0;
    }

    const RwUInt32 r = rgbIn->red;
    const RwUInt32 g = rgbIn->green;
    const RwUInt32 b = rgbIn->blue;
    const RwUInt32 a = rgbIn->alpha;

    switch (rasterFormat & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMAT1555:
        return ((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
    case rwRASTERFORMAT565:
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    case rwRASTERFORMAT4444:
        return ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
    case rwRASTERFORMAT888:
        return (r << 16) | (g << 8) | b;
    default:
        return (a << 24) | (r << 16) | (g << 8) | b;
    }
}

namespace notsa::fakerw {

rw::Raster* GetLibrwRaster(RwRaster* raster) {
    const auto it = g_rasterMirrors.find(raster);
    return it != g_rasterMirrors.end() ? it->second : nullptr;
}

} // namespace notsa::fakerw

#endif // NOTSA_NATIVE
