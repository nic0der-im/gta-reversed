// librw + GTA San Andreas asset spike.
//
// Stage "triangle": SDL3 window, librw GL3 engine, immediate-mode triangle.
// Stage "car":      infernus.dff (+ vehicle.txd/infernus.txd) spinning.
// Stage "ped":      male01.dff (+ male01.txd), skinned bind pose.
//
// The program renders for --seconds seconds, writes one PNG read back from the
// GL back buffer at --frame, prints diagnostics, and exits on its own.

#include <rw.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "img.h"

using namespace rw;
using spike::ImgArchive;
using spike::ImgEntry;

namespace {

// ---------------------------------------------------------------------------
// Options and global state
// ---------------------------------------------------------------------------

struct Options
{
    std::string stage       = "triangle";
    std::string gameDir;
    std::string outDir      = "/tmp/librw_spike";
    std::string dffName; // optional override, e.g. "admiral.dff"
    std::string txdName; // optional override, e.g. "male01.txd"
    double      seconds     = 8.0;
    int         captureFrame = 45;
    int         width       = 1280;
    int         height      = 800;
    bool        spin        = true;
    bool        librwReadback = false;
};

Options gOpt;

SDL_Window*     gWindow = nullptr;
Camera*         gCamera = nullptr;
World*          gWorld  = nullptr;
Clump*          gClump  = nullptr;
TexDictionary*  gTxd    = nullptr;
ImgArchive      gImg;
Matrix          gRestFrameMatrix;
bool            gHaveRestFrame = false;
bool            gQuit          = false;

void Log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    va_end(args);
    std::fflush(stdout);
}

// ---------------------------------------------------------------------------
// GL information (queried through SDL, no direct glad dependency)
// ---------------------------------------------------------------------------

constexpr unsigned kGlVendor      = 0x1F00;
constexpr unsigned kGlRenderer    = 0x1F01;
constexpr unsigned kGlVersion     = 0x1F02;
constexpr unsigned kGlExtensions  = 0x1F03;
constexpr unsigned kGlSlVersion   = 0x8B8C;
constexpr unsigned kGlNumExt      = 0x821D;
constexpr unsigned kGlMaxTexSize  = 0x0D33;

struct GlInfo
{
    std::string vendor, renderer, version, glsl;
    int  maxTextureSize = 0;
    bool s3tc           = false;
    bool dxt1           = false;
    bool dxt3           = false;
    bool dxt5           = false;
    int  numExtensions  = 0;
};

GlInfo QueryGlInfo()
{
    GlInfo info;
    using PFNGetString   = const unsigned char* (*)(unsigned);
    using PFNGetStringi  = const unsigned char* (*)(unsigned, unsigned);
    using PFNGetIntegerv = void (*)(unsigned, int*);

    auto glGetString   = reinterpret_cast<PFNGetString>(SDL_GL_GetProcAddress("glGetString"));
    auto glGetStringi  = reinterpret_cast<PFNGetStringi>(SDL_GL_GetProcAddress("glGetStringi"));
    auto glGetIntegerv = reinterpret_cast<PFNGetIntegerv>(SDL_GL_GetProcAddress("glGetIntegerv"));
    if (!glGetString)
        return info;

    auto str = [&](unsigned name) -> std::string {
        const unsigned char* s = glGetString(name);
        return s ? reinterpret_cast<const char*>(s) : std::string("<null>");
    };
    info.vendor   = str(kGlVendor);
    info.renderer = str(kGlRenderer);
    info.version  = str(kGlVersion);
    info.glsl     = str(kGlSlVersion);
    if (glGetIntegerv)
        glGetIntegerv(kGlMaxTexSize, &info.maxTextureSize);

    auto noteExtension = [&](const char* e) {
        if (!e)
            return;
        if (std::strstr(e, "GL_EXT_texture_compression_s3tc"))
            info.s3tc = true;
        if (std::strstr(e, "GL_EXT_texture_compression_dxt1") || std::strstr(e, "GL_ANGLE_texture_compression_dxt1"))
            info.dxt1 = true;
        if (std::strstr(e, "GL_ANGLE_texture_compression_dxt3"))
            info.dxt3 = true;
        if (std::strstr(e, "GL_ANGLE_texture_compression_dxt5"))
            info.dxt5 = true;
    };

    if (glGetStringi && glGetIntegerv) {
        glGetIntegerv(kGlNumExt, &info.numExtensions);
        for (int i = 0; i < info.numExtensions; ++i)
            noteExtension(reinterpret_cast<const char*>(glGetStringi(kGlExtensions, i)));
    } else {
        noteExtension(reinterpret_cast<const char*>(glGetString(kGlExtensions)));
    }
    return info;
}

// ---------------------------------------------------------------------------
// librw engine
// ---------------------------------------------------------------------------

void RegisterPlugins()
{
    registerMeshPlugin();
    registerNativeDataPlugin();
    registerAtomicRightsPlugin();
    registerMaterialRightsPlugin();
    registerAnisotropyPlugin();
    registerUserDataPlugin();
    registerHAnimPlugin();
    registerMatFXPlugin();
    registerSkinPlugin();
}

bool InitLibrw(const char* title)
{
    if (!Engine::init()) {
        Log("[librw] Engine::init() FAILED\n");
        return false;
    }
    RegisterPlugins();
    Log("[librw] plugins: mesh, nativeData, atomicRights, materialRights, anisotropy, userData, hanim, matfx, skin\n");

    EngineOpenParams params;
    params.window      = &gWindow;
    params.fullscreen  = 0;
    params.width       = gOpt.width;
    params.height      = gOpt.height;
    params.windowtitle = title;

    if (!Engine::open(&params)) {
        Log("[librw] Engine::open() FAILED\n");
        return false;
    }

    const int numSub = Engine::getNumSubSystems();
    Log("[librw] subsystems: %d\n", numSub);
    for (int i = 0; i < numSub; ++i) {
        SubSystemInfo info;
        if (Engine::getSubSystemInfo(&info, i))
            Log("  [%d] %s\n", i, info.name);
    }
    if (numSub > 0)
        Engine::setSubSystem(numSub - 1);

    VideoMode mode;
    const int numModes = Engine::getNumVideoModes();
    if (Engine::getVideoModeInfo(&mode, Engine::getCurrentVideoMode()))
        Log("[librw] video modes: %d, current: %dx%dx%d flags 0x%X\n",
            numModes, mode.width, mode.height, mode.depth, mode.flags);

    if (!Engine::start()) {
        Log("[librw] Engine::start() FAILED (SDL: %s)\n", SDL_GetError());
        return false;
    }

    int w = 0, h = 0;
    SDL_GetWindowSize(gWindow, &w, &h);
    Log("[librw] window created: %dx%d\n", w, h);
    const int sdlVersion = SDL_GetVersion();
    Log("[librw] SDL version %d, rw::platform = %d (PLATFORM_GL3 expected 12)\n",
        sdlVersion, rw::platform);

    Charset::open();
    Image::setSearchPath("./");
    return true;
}

void ShutdownLibrw()
{
    Charset::close();
    Engine::stop();
    Engine::close();
    Engine::term();
}

// ---------------------------------------------------------------------------
// Files and assets
// ---------------------------------------------------------------------------

bool ReadWholeFile(const std::string& path, std::vector<uint8_t>* out)
{
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return false;
    if (std::fseek(f, 0, SEEK_END) != 0) {
        std::fclose(f);
        return false;
    }
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out->resize(size > 0 ? static_cast<size_t>(size) : 0);
    const size_t got = out->empty() ? 0 : std::fread(out->data(), 1, out->size(), f);
    std::fclose(f);
    return got == out->size();
}

// Looks for models/<name> on disk first, then inside gta3.img.
bool GetAsset(const std::string& name, std::vector<uint8_t>* out, std::string* how)
{
    const std::string diskPath = gOpt.gameDir + "/models/" + name;
    if (ReadWholeFile(diskPath, out)) {
        *how = diskPath;
        return true;
    }
    const ImgEntry* entry = gImg.Find(name);
    if (entry) {
        std::string err;
        if (gImg.Read(*entry, out, &err)) {
            *how = "gta3.img:" + name;
            return true;
        }
        Log("[asset] %s: %s\n", name.c_str(), err.c_str());
        return false;
    }
    *how = "";
    return false;
}

// Returns the number of textures converted to the current platform.
int ConvertTxdTextures(TexDictionary* txd, const char* label)
{
    int count = 0, converted = 0, failed = 0;
    FORLIST(lnk, txd->textures) {
        Texture* tex = Texture::fromDict(lnk);
        ++count;
        Raster* raster = tex->raster;
        if (!raster) {
            Log("  [txd] '%.32s': no raster\n", tex->name);
            continue;
        }
        Log("  [txd] '%.32s': %dx%d depth %d platform %d",
            tex->name, raster->width, raster->height, raster->depth, raster->platform);
        if (raster->platform != rw::platform) {
            Raster* newRaster = Raster::convertTexToCurrentPlatform(raster);
            if (newRaster) {
                tex->raster = newRaster;
                ++converted;
                Log(" -> converted to platform %d\n", rw::platform);
            } else {
                tex->raster = nullptr; // do not let the GL3 backend use a foreign raster
                ++failed;
                Log(" -> CONVERSION FAILED\n");
            }
        } else {
            Log("\n");
        }
    }
    Log("[txd] %s: %d textures, %d converted, %d failed\n", label, count, converted, failed);
    return converted;
}

TexDictionary* LoadTxdFromData(const std::vector<uint8_t>& data, const char* label)
{
    StreamMemory in;
    in.open(const_cast<uint8*>(data.data()), static_cast<uint32>(data.size()));
    TexDictionary* txd = nullptr;
    if (findChunk(&in, ID_TEXDICTIONARY, nullptr, nullptr))
        txd = TexDictionary::streamRead(&in);
    in.close();
    if (!txd) {
        Log("[txd] %s: streamRead FAILED\n", label);
        return nullptr;
    }
    ConvertTxdTextures(txd, label);
    return txd;
}

void MergeTexDictionary(TexDictionary* from, TexDictionary* into)
{
    std::vector<Texture*> textures;
    FORLIST(lnk, from->textures)
        textures.push_back(Texture::fromDict(lnk));
    for (Texture* tex : textures)
        into->add(tex);
}

Clump* LoadClumpFromData(const std::vector<uint8_t>& data, const char* label)
{
    StreamMemory in;
    in.open(const_cast<uint8*>(data.data()), static_cast<uint32>(data.size()));
    Clump* clump = nullptr;
    if (findChunk(&in, ID_CLUMP, nullptr, nullptr))
        clump = Clump::streamRead(&in);
    in.close();
    if (!clump)
        Log("[dff] %s: streamRead FAILED\n", label);
    else
        Log("[dff] %s: %u bytes, loaded\n", label, static_cast<unsigned>(data.size()));
    return clump;
}

void DumpClump(Clump* clump)
{
    int atomics = 0, vertices = 0, triangles = 0, skinned = 0, textured = 0, missingTex = 0, matfx = 0;
    FORLIST(lnk, clump->atomics) {
        Atomic* atomic = Atomic::fromClump(lnk);
        Geometry* geo  = atomic->geometry;
        if (!geo)
            continue;
        ++atomics;
        vertices  += geo->numVertices;
        triangles += geo->numTriangles;
        Skin* skin = Skin::get(geo);
        if (skin) {
            ++skinned;
            Log("  [atomic %d] verts %d tris %d flags 0x%08X uvSets %d SKIN bones %d\n",
                atomics, geo->numVertices, geo->numTriangles, geo->flags, geo->numTexCoordSets, skin->numBones);
        } else {
            Log("  [atomic %d] verts %d tris %d flags 0x%08X uvSets %d\n",
                atomics, geo->numVertices, geo->numTriangles, geo->flags, geo->numTexCoordSets);
        }
        if (geo->colors && geo->numVertices > 0)
            Log("    prelit vertex color[0]: %u,%u,%u,%u\n",
                geo->colors[0].red, geo->colors[0].green, geo->colors[0].blue, geo->colors[0].alpha);
        for (int i = 0; i < geo->matList.numMaterials; ++i) {
            Material* mat = geo->matList.materials[i];
            if (!mat)
                continue;
            if (mat->texture) {
                ++textured;
                Log("    material %d: color %u,%u,%u,%u texture '%.32s' raster %s\n", i,
                    mat->color.red, mat->color.green, mat->color.blue, mat->color.alpha,
                    mat->texture->name,
                    mat->texture->raster ? "ok" : "MISSING/FAILED");
                if (!mat->texture->raster)
                    ++missingTex;
            } else {
                Log("    material %d: color %u,%u,%u,%u (untextured)\n", i,
                    mat->color.red, mat->color.green, mat->color.blue, mat->color.alpha);
            }
            const uint32 fx = MatFX::getEffects(mat);
            if (fx) {
                ++matfx;
                if (matfx <= 8)
                    Log("      MatFX effects 0x%X\n", fx);
            }
        }
    }
    Log("[dff] clump: %d atomics, %d vertices, %d triangles, %d skinned atomics, %d textured materials (%d without raster), %d MatFX materials\n",
        atomics, vertices, triangles, skinned, textured, missingTex, matfx);
    Log("[dff] frames: root has %d children\n",
        clump->getFrame() ? clump->getFrame()->count() : 0);
}

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

bool CreateCameraAndWorld()
{
    gCamera = Camera::create();
    if (!gCamera) {
        Log("[scene] Camera::create FAILED\n");
        return false;
    }
    gCamera->setFrame(Frame::create());
    gCamera->frameBuffer = Raster::create(gOpt.width, gOpt.height, 0, Raster::CAMERA);
    gCamera->zBuffer     = Raster::create(gOpt.width, gOpt.height, 0, Raster::ZBUFFER);
    if (!gCamera->frameBuffer || !gCamera->zBuffer) {
        Log("[scene] camera raster creation FAILED\n");
        return false;
    }
    gCamera->setNearPlane(0.05f);
    gCamera->setFarPlane(5000.0f);
    gCamera->setFOV(60.0f, static_cast<float>(gOpt.width) / gOpt.height);
    Log("[scene] camera: framebuffer %dx%d, zbuffer %dx%d\n",
        gCamera->frameBuffer->width, gCamera->frameBuffer->height,
        gCamera->zBuffer->width, gCamera->zBuffer->height);

    gWorld = World::create();
    if (!gWorld) {
        Log("[scene] World::create FAILED\n");
        return false;
    }
    gWorld->addCamera(gCamera);

    Light* ambient = Light::create(Light::AMBIENT);
    ambient->setColor(0.32f, 0.32f, 0.36f);
    gWorld->addLight(ambient);

    V3d xAxis = { 1.0f, 0.0f, 0.0f };
    Light* directional = Light::create(Light::DIRECTIONAL);
    directional->setColor(0.62f, 0.62f, 0.58f);
    directional->setFrame(Frame::create());
    directional->getFrame()->rotate(&xAxis, 140.0f, COMBINEREPLACE);
    gWorld->addLight(directional);
    Log("[scene] world: 1 ambient + 1 directional light\n");
    return true;
}

bool ResizeCameraRaster(int width, int height)
{
    if (!gCamera || width <= 0 || height <= 0)
        return false;
    Raster* frameBuffer = Raster::create(width, height, 0, Raster::CAMERA);
    Raster* zBuffer     = Raster::create(width, height, 0, Raster::ZBUFFER);
    if (!frameBuffer || !zBuffer) {
        Log("[resize] raster creation FAILED for %dx%d\n", width, height);
        return false;
    }
    if (gCamera->frameBuffer)
        gCamera->frameBuffer->destroy();
    if (gCamera->zBuffer)
        gCamera->zBuffer->destroy();
    gCamera->frameBuffer = frameBuffer;
    gCamera->zBuffer     = zBuffer;
    gCamera->setFOV(60.0f, static_cast<float>(width) / height);
    Log("[resize] render target now %dx%d\n", width, height);
    return true;
}

void HandleWindowResize()
{
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(gWindow, &width, &height);
    if (!gCamera || !gCamera->frameBuffer)
        ResizeCameraRaster(width, height);
    else if (width != gCamera->frameBuffer->width || height != gCamera->frameBuffer->height)
        ResizeCameraRaster(width, height);
}

void LookAt(Camera* cam, const V3d& position, const V3d& target, const V3d& up)
{
    V3d forward = normalize(sub(target, position));
    V3d left    = normalize(cross(up, forward));
    V3d newUp   = cross(forward, left);

    Frame* frame = cam->getFrame();
    frame->matrix.right = left;
    frame->matrix.up    = newUp;
    frame->matrix.at    = forward;
    frame->matrix.pos   = position;
    frame->matrix.optimize();
    frame->updateObjects();
}

bool FrameClump(Clump* clump)
{
    V3d    center = { 0.0f, 0.0f, 0.0f };
    float  radius = 0.0f;
    int    n      = 0;

    FORLIST(lnk, clump->atomics) {
        Sphere* sphere = Atomic::fromClump(lnk)->getWorldBoundingSphere();
        center = add(center, sphere->center);
        ++n;
    }
    if (n == 0) {
        Log("[scene] clump has no atomics, cannot frame\n");
        return false;
    }
    center = scale(center, 1.0f / n);

    radius = 0.0f;
    FORLIST(lnk, clump->atomics) {
        Sphere* sphere = Atomic::fromClump(lnk)->getWorldBoundingSphere();
        const float distance = length(sub(sphere->center, center)) + sphere->radius;
        if (distance > radius)
            radius = distance;
    }
    if (radius < 0.001f)
        radius = 1.0f;

    const V3d target   = center;
    const V3d position = add(center, V3d{ 0.0f, -radius * 2.6f, radius * 0.9f });
    const V3d up       = { 0.0f, 0.0f, 1.0f };
    LookAt(gCamera, position, target, up);
    gCamera->setNearPlane(std::max(0.05f, radius * 0.01f));
    gCamera->setFarPlane(std::max(100.0f, radius * 20.0f));
    Log("[scene] clump center (%.2f, %.2f, %.2f) radius %.2f; camera at (%.2f, %.2f, %.2f)\n",
        center.x, center.y, center.z, radius,
        position.x, position.y, position.z);
    return true;
}

bool LoadCarStage()
{
    const std::string dffName = gOpt.dffName.empty() ? "infernus.dff" : gOpt.dffName;
    const std::string txdName = gOpt.txdName.empty() ? "infernus.txd" : gOpt.txdName;

    std::vector<uint8_t> vehicleTxdData, modelTxdData, dffData;
    std::string how;

    // Generic vehicle textures first, then the model-specific dictionary.
    if (!ReadWholeFile(gOpt.gameDir + "/models/generic/vehicle.txd", &vehicleTxdData)) {
        Log("[car] models/generic/vehicle.txd not found\n");
        return false;
    }
    Log("[car] vehicle.txd: %u bytes\n", static_cast<unsigned>(vehicleTxdData.size()));
    TexDictionary* base = LoadTxdFromData(vehicleTxdData, "vehicle.txd");
    if (!base)
        return false;

    if (GetAsset(txdName, &modelTxdData, &how)) {
        Log("[car] %s: %u bytes from %s\n", txdName.c_str(), static_cast<unsigned>(modelTxdData.size()), how.c_str());
        if (TexDictionary* extra = LoadTxdFromData(modelTxdData, txdName.c_str())) {
            MergeTexDictionary(extra, base);
            extra->destroy(); // now empty
        }
    } else {
        Log("[car] %s not found (continuing with generic textures only)\n", txdName.c_str());
    }

    gTxd = base;
    TexDictionary::setCurrent(gTxd);

    if (!GetAsset(dffName, &dffData, &how)) {
        Log("[car] %s not found\n", dffName.c_str());
        return false;
    }
    Log("[car] %s from %s\n", dffName.c_str(), how.c_str());

    gClump = LoadClumpFromData(dffData, dffName.c_str());
    if (!gClump)
        return false;

    gWorld->addClump(gClump);
    DumpClump(gClump);
    if (!FrameClump(gClump))
        return false;

    gRestFrameMatrix = gClump->getFrame()->matrix;
    gHaveRestFrame = true;
    return true;
}

bool LoadPedStage()
{
    const std::string dffName = gOpt.dffName.empty() ? "male01.dff" : gOpt.dffName;
    const std::string txdName = gOpt.txdName.empty() ? "male01.txd" : gOpt.txdName;

    std::vector<uint8_t> txdData, dffData;
    std::string how;

    if (!GetAsset(txdName, &txdData, &how)) {
        Log("[ped] %s not found\n", txdName.c_str());
        return false;
    }
    Log("[ped] %s: %u bytes from %s\n", txdName.c_str(), static_cast<unsigned>(txdData.size()), how.c_str());
    gTxd = LoadTxdFromData(txdData, txdName.c_str());
    if (!gTxd)
        return false;
    TexDictionary::setCurrent(gTxd);

    if (!GetAsset(dffName, &dffData, &how)) {
        Log("[ped] %s not found\n", dffName.c_str());
        return false;
    }
    Log("[ped] %s: %u bytes from %s\n", dffName.c_str(), static_cast<unsigned>(dffData.size()), how.c_str());

    gClump = LoadClumpFromData(dffData, dffName.c_str());
    if (!gClump)
        return false;

    // SA ped DFFs are authored Y-up (head towards +Y). Rotate the root so the
    // bind pose stands along +Z like the game world.
    {
        Frame* root = gClump->getFrame();
        V3d xAxis = { 1.0f, 0.0f, 0.0f };
        root->matrix.rotate(&xAxis, 90.0f, COMBINEPOSTCONCAT);
        root->matrix.optimize();
        root->updateObjects();
        Log("[ped] rotated root +90 deg around X (Y-up DFF -> Z-up world)\n");
    }

    gWorld->addClump(gClump);
    DumpClump(gClump);
    if (!FrameClump(gClump))
        return false;

    gRestFrameMatrix = gClump->getFrame()->matrix;
    gHaveRestFrame = true;
    return true;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void DrawTriangle(float angleDegrees)
{
    static gl3::Im3DVertex verts[3];
    static bool initialized = false;
    if (!initialized) {
        verts[0].setX(-1.3f); verts[0].setY(-0.75f); verts[0].setZ(0.0f);
        verts[0].setColor(240, 60, 60, 255);
        verts[1].setX(1.3f);  verts[1].setY(-0.75f); verts[1].setZ(0.0f);
        verts[1].setColor(60, 240, 60, 255);
        verts[2].setX(0.0f);  verts[2].setY(1.3f);   verts[2].setZ(0.0f);
        verts[2].setColor(80, 120, 255, 255);
        for (int i = 0; i < 3; ++i) {
            verts[i].setU(0.0f);
            verts[i].setV(0.0f);
            verts[i].setNormalX(0.0f);
            verts[i].setNormalY(0.0f);
            verts[i].setNormalZ(1.0f);
        }
        initialized = true;
    }

    Matrix transform;
    transform.setIdentity();
    V3d zAxis = { 0.0f, 0.0f, 1.0f };
    transform.rotate(&zAxis, angleDegrees);

    SetRenderState(ZTESTENABLE, 1);
    SetRenderState(ZWRITEENABLE, 1);
    SetRenderState(SRCBLEND, BLENDONE);
    SetRenderState(DESTBLEND, BLENDZERO);
    SetRenderState(TEXTUREFILTER, Texture::FilterMode::LINEAR);
    SetRenderState(CULLMODE, CULLNONE);
    SetRenderStatePtr(TEXTURERASTER, nullptr);

    const uint32 flags = im3d::VERTEXXYZ | im3d::VERTEXRGBA | im3d::ALLOPAQUE;
    im3d::Transform(verts, 3, &transform, flags);
    im3d::RenderPrimitive(PRIMTYPETRILIST);
    im3d::End();
}

void AnalyzeImage(Image* img, const char* path)
{
    std::set<uint32_t> distinct;
    uint64_t sumR = 0, sumG = 0, sumB = 0;
    const int bpp = img->bpp;
    const int step = std::max(1, img->height / 120); // sample rows to stay fast
    int sampled = 0;
    for (int y = 0; y < img->height; y += step) {
        const uint8_t* row = img->pixels + static_cast<size_t>(y) * img->stride;
        for (int x = 0; x < img->width; x += 3) {
            const uint8_t* p = row + static_cast<size_t>(x) * bpp;
            const uint8_t r = p[0], g = bpp >= 3 ? p[1] : p[0], b = bpp >= 3 ? p[2] : p[0];
            sumR += r; sumG += g; sumB += b;
            distinct.insert((static_cast<uint32_t>(r >> 3) << 10) |
                            (static_cast<uint32_t>(g >> 3) << 5) |
                            (static_cast<uint32_t>(b >> 3)));
            ++sampled;
        }
    }
    if (sampled == 0) {
        Log("[capture] %s: empty image\n", path);
        return;
    }
    Log("[capture] %s: %dx%d depth %d, sampled %d px, mean rgb (%llu, %llu, %llu), %zu distinct 5:5:5 colors\n",
        path, img->width, img->height, img->depth, sampled,
        static_cast<unsigned long long>(sumR / sampled),
        static_cast<unsigned long long>(sumG / sampled),
        static_cast<unsigned long long>(sumB / sampled),
        distinct.size());
}

bool CaptureFrameLibrw(const std::string& path)
{
    Image* img = gCamera->frameBuffer->toImage();
    if (!img) {
        Log("[capture] librw frame buffer readback FAILED\n");
        return false;
    }
    AnalyzeImage(img, path.c_str());
    writePNG(img, path.c_str());
    img->destroy();
    return true;
}

// Reads the window's back buffer directly. librw's own camera readback
// (gl3raster rasterLock, CAMERA case) forgets GL_PACK_ALIGNMENT=1, so rows
// are padded by the driver whenever width*3 is not a multiple of 4 and the
// image shears. Keeping a correct path here makes the spike independent of
// that bug; --librw-readback reproduces it.
bool CaptureFrame(const std::string& path)
{
    if (gOpt.librwReadback)
        return CaptureFrameLibrw(path);

    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(gWindow, &width, &height);
    if (width <= 0 || height <= 0) {
        Log("[capture] invalid window size %dx%d\n", width, height);
        return false;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glFinish();

    Image* img = Image::create(width, height, 24);
    if (!img) {
        Log("[capture] Image::create FAILED\n");
        return false;
    }
    img->allocate();

    std::vector<uint8_t> raw(static_cast<size_t>(width) * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, raw.data());
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        Log("[capture] glReadPixels FAILED: 0x%X\n", error);
        img->destroy();
        return false;
    }

    // GL rows run bottom-up, PNG rows top-down.
    for (int y = 0; y < height; ++y) {
        const uint8_t* src = raw.data() + static_cast<size_t>(height - 1 - y) * width * 3;
        uint8_t*       dst = img->pixels + static_cast<size_t>(y) * img->stride;
        std::memcpy(dst, src, static_cast<size_t>(width) * 3);
    }

    int viewport[4] = { 0, 0, 0, 0 };
    glGetIntegerv(GL_VIEWPORT, viewport);
    Log("[capture] window px %dx%d, GL_VIEWPORT %d,%d %dx%d, camera raster %dx%d, glError 0x%X\n",
        width, height, viewport[0], viewport[1], viewport[2], viewport[3],
        gCamera->frameBuffer->width, gCamera->frameBuffer->height, error);

    AnalyzeImage(img, path.c_str());
    writePNG(img, path.c_str());
    img->destroy();
    return true;
}

void RunLoop()
{
    const std::string capturePath = gOpt.outDir + "/" + gOpt.stage + ".png";
    const auto start = std::chrono::steady_clock::now();
    const auto stop  = start + std::chrono::milliseconds(static_cast<long>(gOpt.seconds * 1000.0));

    RGBA clearColor = { 22, 26, 40, 255 };
    if (gOpt.stage == "car")
        clearColor = RGBA{ 58, 66, 82, 255 };
    else if (gOpt.stage == "ped")
        clearColor = RGBA{ 70, 58, 70, 255 };

    bool captured = false;
    int frame = 0;
    gQuit = false;

    while (!gQuit) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= stop)
            break;
        const float t = std::chrono::duration<float>(now - start).count();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                gQuit = true;
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
                gQuit = true;
            else if (event.type == SDL_EVENT_WINDOW_RESIZED ||
                     event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
                     event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED)
                HandleWindowResize();
        }
        if (gQuit)
            break;

        if (gClump && gHaveRestFrame && gOpt.spin) {
            Frame* root = gClump->getFrame();
            root->matrix = gRestFrameMatrix;
            V3d zAxis = { 0.0f, 0.0f, 1.0f };
            root->matrix.rotate(&zAxis, t * 40.0f);
            root->matrix.optimize();
            root->updateObjects();
        }

        gCamera->clear(&clearColor, Camera::CLEARIMAGE | Camera::CLEARZ);
        gCamera->beginUpdate();
        if (gWorld)
            gWorld->render();
        if (gOpt.stage == "triangle")
            DrawTriangle(t * 40.0f);
        gCamera->endUpdate();

        if (!captured && frame >= gOpt.captureFrame) {
            CaptureFrame(capturePath);
            captured = true;
        }

        gCamera->showRaster(Raster::FLIPWAITVSYNCH);
        ++frame;
    }

    const float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    Log("[loop] %d frames in %.2f s (%.1f fps)\n", frame, elapsed, elapsed > 0.0f ? frame / elapsed : 0.0f);
    if (!captured)
        Log("[capture] no frame captured (frames=%d, captureFrame=%d)\n", frame, gOpt.captureFrame);
}

void CleanupScene()
{
    if (gClump && gWorld) {
        gWorld->removeClump(gClump);
        gClump->destroy();
        gClump = nullptr;
    }
    if (gCamera && gWorld)
        gWorld->removeCamera(gCamera);
    if (gCamera) {
        if (gCamera->frameBuffer) {
            gCamera->frameBuffer->destroy();
            gCamera->frameBuffer = nullptr;
        }
        if (gCamera->zBuffer) {
            gCamera->zBuffer->destroy();
            gCamera->zBuffer = nullptr;
        }
        Frame* frame = gCamera->getFrame();
        if (frame) {
            gCamera->setFrame(nullptr);
            frame->destroy();
        }
        gCamera->destroy();
        gCamera = nullptr;
    }
    if (gWorld) {
        gWorld->destroy();
        gWorld = nullptr;
    }
    if (gTxd) {
        TexDictionary::setCurrent(nullptr);
        gTxd->destroy();
        gTxd = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Argument parsing
// ---------------------------------------------------------------------------

void Usage(const char* argv0)
{
    Log("usage: %s --stage=triangle|car|ped [options]\n", argv0);
    Log("  --game-dir=PATH   game root (default $GTASA_DIR or ~/Games/GTASA)\n");
    Log("  --out-dir=PATH    where the screenshot PNG goes (default /tmp/librw_spike)\n");
    Log("  --seconds=N       run time (default 8)\n");
    Log("  --frame=N         capture frame (default 45)\n");
    Log("  --width=N --height=N\n");
    Log("  --dff=NAME --txd=NAME  asset overrides\n");
    Log("  --no-spin              do not rotate the model\n");
    Log("  --librw-readback       use librw's camera->toImage (reproduces its GL_PACK_ALIGNMENT bug)\n");
}

bool ParseArgs(int argc, char** argv)
{
    const char* home = std::getenv("HOME");
    gOpt.gameDir = home ? std::string(home) + "/Games/GTASA" : std::string(".");
    if (const char* env = std::getenv("GTASA_DIR"))
        gOpt.gameDir = env;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&](const char* prefix) -> std::string {
            const size_t n = std::strlen(prefix);
            return arg.compare(0, n, prefix) == 0 ? arg.substr(n) : std::string();
        };
        if (arg == "--help" || arg == "-h") {
            Usage(argv[0]);
            return false;
        } else if (arg == "--no-spin") {
            gOpt.spin = false;
        } else if (arg == "--librw-readback") {
            gOpt.librwReadback = true;
        } else if (arg.compare(0, 8, "--stage=") == 0) {
            gOpt.stage = value("--stage=");
        } else if (arg.compare(0, 11, "--game-dir=") == 0) {
            gOpt.gameDir = value("--game-dir=");
        } else if (arg.compare(0, 10, "--out-dir=") == 0) {
            gOpt.outDir = value("--out-dir=");
        } else if (arg.compare(0, 10, "--seconds=") == 0) {
            gOpt.seconds = std::atof(value("--seconds=").c_str());
        } else if (arg.compare(0, 8, "--frame=") == 0) {
            gOpt.captureFrame = std::atoi(value("--frame=").c_str());
        } else if (arg.compare(0, 8, "--width=") == 0) {
            gOpt.width = std::atoi(value("--width=").c_str());
        } else if (arg.compare(0, 9, "--height=") == 0) {
            gOpt.height = std::atoi(value("--height=").c_str());
        } else if (arg.compare(0, 6, "--dff=") == 0) {
            gOpt.dffName = value("--dff=");
        } else if (arg.compare(0, 6, "--txd=") == 0) {
            gOpt.txdName = value("--txd=");
        } else {
            Log("unknown argument: %s\n", arg.c_str());
            Usage(argv[0]);
            return false;
        }
    }

    if (gOpt.stage != "triangle" && gOpt.stage != "car" && gOpt.stage != "ped") {
        Log("unknown stage '%s'\n", gOpt.stage.c_str());
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (!ParseArgs(argc, argv))
        return 1;

    std::error_code ec;
    std::filesystem::create_directories(gOpt.outDir, ec);

    Log("== librw SA spike == commit %s stage '%s'\n", LIBRW_COMMIT, gOpt.stage.c_str());

    if (gOpt.stage != "triangle") {
        const std::string imgPath = gOpt.gameDir + "/models/gta3.img";
        std::string error;
        if (!gImg.Open(imgPath, &error)) {
            Log("[img] %s\n", error.c_str());
            return 1;
        }
        Log("[img] %s: %zu entries\n", imgPath.c_str(), gImg.Count());
    }

    if (!InitLibrw("librw SA spike")) {
        Log("FAILED: librw engine startup\n");
        return 1;
    }

    const GlInfo gl = QueryGlInfo();
    Log("[gl] GL_VENDOR   : %s\n", gl.vendor.c_str());
    Log("[gl] GL_RENDERER : %s\n", gl.renderer.c_str());
    Log("[gl] GL_VERSION  : %s\n", gl.version.c_str());
    Log("[gl] GLSL        : %s\n", gl.glsl.c_str());
    Log("[gl] max texture size: %d, extensions: %d\n", gl.maxTextureSize, gl.numExtensions);
    Log("[gl] S3TC (DXT): %s (dxt1 %s, dxt3 %s, dxt5 %s)\n",
        gl.s3tc ? "yes" : "no", gl.dxt1 ? "yes" : "no", gl.dxt3 ? "yes" : "no", gl.dxt5 ? "yes" : "no");

    if (!CreateCameraAndWorld()) {
        Log("FAILED: scene creation\n");
        return 1;
    }

    bool ok = true;
    if (gOpt.stage == "triangle") {
        const V3d position = { 0.0f, -4.0f, 1.2f };
        const V3d target   = { 0.0f, 0.0f, 0.15f };
        const V3d up       = { 0.0f, 0.0f, 1.0f };
        LookAt(gCamera, position, target, up);
        Log("[triangle] camera at (0, -4, 1.2) looking at (0, 0, 0.15)\n");
    } else if (gOpt.stage == "car") {
        ok = LoadCarStage();
    } else {
        ok = LoadPedStage();
    }

    if (ok) {
        RunLoop();
    }

    CleanupScene();
    ShutdownLibrw();
    Log("== done (stage '%s') ==\n", gOpt.stage.c_str());
    return ok ? 0 : 1;
}
