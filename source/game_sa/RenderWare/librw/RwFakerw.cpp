#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

#include "pipelines/SAPipelines.h"
#include "plugins/RwPlugins.h"

#include <unordered_set>

//! Engine core of the native RenderWare layer: lifecycle, memory functions, video modes and
//! the `RwGlobals` state the plugin-sdk headers dereference through `RWSRCGLOBAL`.
//!
//! See `RwFakerw.h` for the overall design.

namespace notsa::fakerw {

// Engine state, private to this file.
RwMemoryFunctions g_memFuncs{};

bool     g_started      = false;
void*    g_window       = nullptr;
int32    g_width        = 640;
int32    g_height       = 480;
RwGlobals* g_globals    = nullptr;

std::unordered_set<std::string> g_reported;

namespace {

// Defaults, used when the game passes no memory functions (it always does in practice).
void* DefaultMalloc(size_t size, RwUInt32) { return std::malloc(size); }
void* DefaultRealloc(void* p, size_t size, RwUInt32) { return std::realloc(p, size); }
void* DefaultCalloc(size_t numObj, size_t sizeObj, RwUInt32) { return std::calloc(numObj, sizeObj); }
void  DefaultFree(void* p) { std::free(p); }

//! librw allocates its own objects; route them through the game's allocator.
rw::MemoryFunctions MakeLibrwMemoryFunctions() {
    rw::MemoryFunctions f{};
    f.rwmalloc     = [](size_t size, rw::uint32 hint) -> void* { return g_memFuncs.rwmalloc(size, hint); };
    f.rwrealloc    = [](void* p, size_t size, rw::uint32 hint) -> void* { return g_memFuncs.rwrealloc(p, size, hint); };
    f.rwfree       = [](void* p) { g_memFuncs.rwfree(p); };
    f.rwmustmalloc = [](size_t size, rw::uint32 hint) -> void* {
        auto* p = g_memFuncs.rwmalloc(size, hint);
        if (!p) {
            NOTSA_LOG_ERR("Rw: out of memory ({} bytes)", size);
            std::abort();
        }
        return p;
    };
    f.rwmustrealloc = [](void* p, size_t size, rw::uint32 hint) -> void* {
        auto* out = g_memFuncs.rwrealloc(p, size, hint);
        if (!out) {
            NOTSA_LOG_ERR("Rw: out of memory while reallocating ({} bytes)", size);
            std::abort();
        }
        return out;
    };
    return f;
}

// The device entry points the plugin-sdk exposes as macros (`RwRenderStateSet`, `RwIm2D*`,
// `RwIm3D*`). Rendering is not implemented yet, so they accept and ignore everything.
RwBool DeviceSystem(RwInt32, void*, void*, RwInt32) { return TRUE; }
RwBool DeviceRenderStateSet(RwRenderState, void*) { return TRUE; }
RwBool DeviceRenderStateGet(RwRenderState, void*) { return FALSE; }
RwBool DeviceIm2DRenderLine(RwIm2DVertex*, RwInt32, RwInt32, RwInt32) { return TRUE; }
RwBool DeviceIm2DRenderTriangle(RwIm2DVertex*, RwInt32, RwInt32, RwInt32, RwInt32) { return TRUE; }
RwBool DeviceIm2DRenderPrimitive(RwPrimitiveType, RwIm2DVertex*, RwInt32) { return TRUE; }
RwBool DeviceIm2DRenderIndexedPrimitive(RwPrimitiveType, RwIm2DVertex*, RwInt32, RwImVertexIndex*, RwInt32) { return TRUE; }
RwBool DeviceIm3DRenderLine(RwInt32, RwInt32) { return TRUE; }
RwBool DeviceIm3DRenderTriangle(RwInt32, RwInt32, RwInt32) { return TRUE; }
RwBool DeviceIm3DRenderPrimitive(RwPrimitiveType) { return TRUE; }
RwBool DeviceIm3DRenderIndexedPrimitive(RwPrimitiveType, RwImVertexIndex*, RwInt32) { return TRUE; }

void SetupDeviceState(RwGlobals& globals) {
    auto& device = globals.dOpenDevice;

    device.gammaCorrection = 1.0f;
    device.zBufferNear     = 0.0f;
    device.zBufferFar      = 1.0f;

    device.fpSystem                    = DeviceSystem;
    device.fpRenderStateSet            = DeviceRenderStateSet;
    device.fpRenderStateGet            = DeviceRenderStateGet;
    device.fpIm2DRenderLine            = DeviceIm2DRenderLine;
    device.fpIm2DRenderTriangle        = DeviceIm2DRenderTriangle;
    device.fpIm2DRenderPrimitive       = DeviceIm2DRenderPrimitive;
    device.fpIm2DRenderIndexedPrimitive = DeviceIm2DRenderIndexedPrimitive;
    device.fpIm3DRenderLine            = DeviceIm3DRenderLine;
    device.fpIm3DRenderTriangle        = DeviceIm3DRenderTriangle;
    device.fpIm3DRenderPrimitive       = DeviceIm3DRenderPrimitive;
    device.fpIm3DRenderIndexedPrimitive = DeviceIm3DRenderIndexedPrimitive;

    WarnOnce("Rw: render state and immediate mode are no-ops until the GL backend (plan B3)");
}

void RegisterLibrwPlugins() {
    rw::registerMeshPlugin();
    rw::registerNativeDataPlugin();
    rw::registerAtomicRightsPlugin();
    rw::registerMaterialRightsPlugin();
    rw::registerUserDataPlugin();
}

} // namespace

void Stub(const char* name) {
    if (g_reported.insert(name).second) {
        NOTSA_LOG_WARN("Rw: stub {}", name);
    }
}

void WarnOnce(const char* message) {
    if (g_reported.insert(message).second) {
        NOTSA_LOG_WARN("{}", message);
    }
}

const RwMemoryFunctions& MemoryFunctions() {
    return g_memFuncs;
}

void* Malloc(size_t size) {
    return g_memFuncs.rwmalloc(size, 0);
}

void* Realloc(void* p, size_t size) {
    return g_memFuncs.rwrealloc(p, size, 0);
}

void Free(void* p) {
    if (p) {
        g_memFuncs.rwfree(p);
    }
}

bool IsStarted() {
    return g_started;
}

void* GetWindow() {
    return g_window;
}

void GetRequestedSize(int32& width, int32& height) {
    width  = g_width;
    height = g_height;
}

//-----------------------------------------------------------------------------
// PluginRegistry
//-----------------------------------------------------------------------------

int32 PluginRegistry::Register(
    int32                     baseSize,
    int32                     size,
    uint32                    pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor  destructCB,
    RwPluginObjectCopy        copyCB
) {
    if (size <= 0) {
        // RW allows zero-sized plugins (they only carry stream callbacks); they get no storage.
        return 0;
    }

    const size_t offset = (m_plugins.empty() ? (size_t)baseSize : m_end + 3) & ~size_t{3};
    m_base = m_plugins.empty() ? (size_t)baseSize : m_base;
    m_end  = offset + size;

    m_plugins.push_back({ (int32)offset, size, pluginID, constructCB, destructCB, copyCB });
    return (int32)offset;
}

int32 PluginRegistry::RegisterStream(
    uint32,
    RwPluginDataChunkReadCallBack,
    RwPluginDataChunkWriteCallBack,
    RwPluginDataChunkGetSizeCallBack
) {
    // Stream callbacks would be dispatched by the DFF/TXD reader, which does not exist yet.
    Stub("plugin stream registration");
    return 0;
}

size_t PluginRegistry::TotalSize(size_t baseSize) const {
    if (m_plugins.empty()) {
        return baseSize;
    }
    if (m_base != baseSize) {
        NOTSA_LOG_WARN("Rw: plugin registered for a {} byte object, but a {} byte object was created", m_base, baseSize);
        return baseSize > m_end ? baseSize : m_end;
    }
    return m_end;
}

void PluginRegistry::Construct(void* object, size_t baseSize) const {
    for (const auto& e : m_plugins) {
        if (e.ctor) {
            e.ctor(object, e.offset, e.size);
        }
    }
}

void PluginRegistry::Destruct(void* object, size_t baseSize) const {
    for (auto it = m_plugins.rbegin(); it != m_plugins.rend(); ++it) {
        if (it->dtor) {
            it->dtor(object, it->offset, it->size);
        }
    }
}

void PluginRegistry::Copy(void* dst, const void* src, size_t baseSize) const {
    for (const auto& e : m_plugins) {
        if (e.copy) {
            e.copy(dst, src, e.offset, e.size);
        }
    }
}

void* AllocObject(size_t baseSize, const PluginRegistry& registry) {
    auto* object = static_cast<uint8*>(Malloc(registry.TotalSize(baseSize)));
    if (!object) {
        NOTSA_LOG_ERR("Rw: failed to allocate {} bytes", registry.TotalSize(baseSize));
        std::abort();
    }
    std::memset(object, 0, registry.TotalSize(baseSize));
    return object;
}

void FreeObject(void* object, size_t baseSize, const PluginRegistry& registry) {
    if (!object) {
        return;
    }
    registry.Destruct(object, baseSize);
    Free(object);
}

} // namespace notsa::fakerw

//-----------------------------------------------------------------------------
// Engine lifecycle
//-----------------------------------------------------------------------------

RwBool RwEngineInit(const RwMemoryFunctions* memFuncs, RwUInt32 initFlags, RwUInt32 resArenaSize) {
    using namespace notsa::fakerw;

    if (g_globals) {
        return TRUE; // Already initialized
    }

    if (memFuncs) {
        g_memFuncs = *memFuncs;
    } else {
        g_memFuncs = { DefaultMalloc, DefaultFree, DefaultRealloc, DefaultCalloc };
    }

    auto librwMemFuncs = MakeLibrwMemoryFunctions();
    if (!rw::Engine::init(&librwMemFuncs)) {
        NOTSA_LOG_ERR("Rw: librw engine init failed");
        return FALSE;
    }

    RegisterLibrwPlugins();
    // Rockstar's DFF/TXD chunks (NodeName, Collision, 2dEffect, pipelines, breakable,
    // extra vertex colours, env/spec materials). Must run before any DFF/TXD is streamed.
    notsa::fakerw::plugins::RegisterRockstarPlugins();

    g_globals = static_cast<RwGlobals*>(Malloc(sizeof(RwGlobals)));
    if (!g_globals) {
        rw::Engine::term();
        return FALSE;
    }
    std::memset(g_globals, 0, sizeof(RwGlobals));
    RwEngineInstance = g_globals;

    g_globals->memoryFuncs     = g_memFuncs;
    g_globals->engineStatus    = rwENGINESTATUSINITED;
    g_globals->resArenaInitSize = resArenaSize;

    std::memset(&g_globals->dirtyFrameList, 0, sizeof(g_globals->dirtyFrameList));

    SetupDeviceState(*g_globals);

    NOTSA_LOG_INFO("Rw: engine initialized (librw GL3, resArena={})", resArenaSize);
    return TRUE;
}

RwBool RwEngineOpen(RwEngineOpenParams* initParams) {
    using namespace notsa::fakerw;

    if (!g_globals || g_globals->engineStatus != rwENGINESTATUSINITED) {
        return FALSE;
    }

    g_width  = RsGlobal.maximumWidth  > 0 ? RsGlobal.maximumWidth  : 640;
    g_height = RsGlobal.maximumHeight > 0 ? RsGlobal.maximumHeight : 480;
    g_window = nullptr;

    rw::EngineOpenParams params{};
    params.window      = reinterpret_cast<SDL_Window**>(&g_window);
    params.fullscreen  = 0;
    params.width       = g_width;
    params.height      = g_height;
    params.windowtitle = RsGlobal.appName ? RsGlobal.appName : "GTA: San Andreas";

    if (!rw::Engine::open(&params)) {
        NOTSA_LOG_ERR("Rw: librw engine open failed");
        return FALSE;
    }

    // librw's GL3 backend exposes a single subsystem; select it like the spike does.
    const int32 subsystems = rw::Engine::getNumSubSystems();
    if (subsystems > 0) {
        rw::Engine::setSubSystem(subsystems - 1);
    }

    g_globals->engineStatus = rwENGINESTATUSOPENED;
    return TRUE;
}

RwBool RwEngineStart() {
    using namespace notsa::fakerw;

    if (!g_globals || g_globals->engineStatus != rwENGINESTATUSOPENED) {
        return FALSE;
    }

    if (!rw::Engine::start()) {
        NOTSA_LOG_ERR("Rw: librw engine start failed (SDL: {})", SDL_GetError());
        return FALSE;
    }

    g_started = true;
    g_globals->engineStatus = rwENGINESTATUSSTARTED;

    // The GL context now exists: build the San Andreas object pipelines. The game's
    // `CCustom*Pipeline::CreatePipe` (called later, from `CGame::InitialiseEssentialsAfterRW`)
    // picks these handles up through `sapipelines::GetPipeline`.
    notsa::native::sapipelines::RegisterSAPipelines();

    // The window is owned by librw; the platform layer only points at it.
    PSGLOBAL(sdlWindow) = static_cast<SDL_Window*>(g_window);
    PSGLOBAL(window)    = g_window;

    int32 windowWidth = 0, windowHeight = 0;
    if (g_window) {
        SDL_GetWindowSize(static_cast<SDL_Window*>(g_window), &windowWidth, &windowHeight);
    }
    NOTSA_LOG_INFO("Rw: engine started ({}x{} window)", windowWidth, windowHeight);
    return TRUE;
}

RwBool RwEngineStop() {
    using namespace notsa::fakerw;

    if (!g_globals || g_globals->engineStatus != rwENGINESTATUSSTARTED) {
        return FALSE;
    }

    rw::Engine::stop();
    g_started = false;
    g_window  = nullptr;

    PSGLOBAL(sdlWindow) = nullptr;
    PSGLOBAL(window)    = nullptr;

    g_globals->engineStatus = rwENGINESTATUSOPENED;
    return TRUE;
}

RwBool RwEngineClose() {
    using namespace notsa::fakerw;

    if (!g_globals || g_globals->engineStatus != rwENGINESTATUSOPENED) {
        return FALSE;
    }

    rw::Engine::close();
    g_globals->engineStatus = rwENGINESTATUSINITED;
    return TRUE;
}

RwBool RwEngineTerm() {
    using namespace notsa::fakerw;

    if (!g_globals) {
        return TRUE;
    }

    rw::Engine::term();
    Free(g_globals);
    g_globals = nullptr;
    RwEngineInstance = nullptr;
    g_started = false;
    g_window  = nullptr;
    return TRUE;
}

RwUInt32 RwEngineGetVersion() {
    // RenderWare 3.6.0.3, the version San Andreas was built against.
    return 0x36003;
}

RwMemoryFunctions* RwOsGetMemoryInterface() {
    return &notsa::fakerw::g_memFuncs;
}

//-----------------------------------------------------------------------------
// Subsystems and video modes
//-----------------------------------------------------------------------------

RwInt32 RwEngineGetNumSubSystems() {
    return rw::Engine::getNumSubSystems();
}

RwInt32 RwEngineGetCurrentSubSystem() {
    return rw::Engine::getCurrentSubSystem();
}

RwBool RwEngineSetSubSystem(RwInt32 subSystemIndex) {
    return rw::Engine::setSubSystem(subSystemIndex) ? TRUE : FALSE;
}

RwSubSystemInfo* RwEngineGetSubSystemInfo(RwSubSystemInfo* subSystemInfo, RwInt32 subSystemIndex) {
    rw::SubSystemInfo info{};
    if (!rw::Engine::getSubSystemInfo(&info, subSystemIndex)) {
        return nullptr;
    }
    std::strncpy(subSystemInfo->name, info.name, sizeof(subSystemInfo->name) - 1);
    subSystemInfo->name[sizeof(subSystemInfo->name) - 1] = '\0';
    return subSystemInfo;
}

RwInt32 RwEngineGetNumVideoModes() {
    return rw::Engine::getNumVideoModes();
}

RwInt32 RwEngineGetCurrentVideoMode() {
    return rw::Engine::getCurrentVideoMode();
}

RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* vmi, RwInt32 modeIndex) {
    if (!vmi) {
        return nullptr;
    }

    rw::VideoMode mode{};
    if (!rw::Engine::getVideoModeInfo(&mode, modeIndex)) {
        return nullptr;
    }

    vmi->width   = mode.width;
    vmi->height  = mode.height;
    vmi->depth   = mode.depth;
    vmi->flags   = static_cast<RwVideoModeFlag>((mode.flags & rw::VIDEOMODEEXCLUSIVE) ? rwVIDEOMODEEXCLUSIVE : 0);
    vmi->refRate = 0;
    vmi->format  = 0;
    return vmi;
}

RwBool RwEngineSetVideoMode(RwInt32 modeIndex) {
    if (!rw::Engine::setVideoMode(modeIndex)) {
        return FALSE;
    }

    rw::VideoMode mode{};
    if (rw::Engine::getVideoModeInfo(&mode, modeIndex)) {
        notsa::fakerw::g_width  = mode.width;
        notsa::fakerw::g_height = mode.height;
    }
    return TRUE;
}

#endif // NOTSA_NATIVE
