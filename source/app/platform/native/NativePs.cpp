#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include <SDL3/SDL.h>

#include "app/platform/platform.h"
#include "app/platform/native/NativePath.h"

#include "LoadingScreen.h"
#include "FileMgr.h"
#include "C_PcSave.h"
#include "MenuManager.h"

//! Native counterpart of `app/platform/win/WinPs.cpp`.
//!
//! Only the functions in the `ps*` contract of `app/platform/platform.h` are implemented here;
//! the game logic in `psInitialize` matches the platform independent part of the Windows one.

// 0x747420
RwBool psInitialize() {
    auto* ps = new psGlobalType;
    RsGlobal.ps = ps;

    ps->lastMousePos = {};
    ps->fullScreen   = false;
    ps->diInterface  = nullptr;
    ps->diMouse      = nullptr;
    ps->diDevice1    = nullptr;
    ps->diDevice2    = nullptr;
#ifdef NOTSA_USE_SDL3
    ps->sdlWindow    = nullptr;
#endif

    CFileMgr::Initialise();
    const auto usrdir = InitUserDirectories();
    s_PcSaveHelper.SetSaveDirectory(usrdir);

    gGameState = GAME_STATE_INITIAL;

    // Load settings only after everything was initialized.
    FrontEndMenuManager.LoadSettings();

    return true;
}

// 0x7458A0
void psTerminate() {
    delete RsGlobal.ps;
    RsGlobal.ps = nullptr;
}

// 0x7451B0
void psWindowSetText(const char* str) {
    if (auto* window = PSGLOBAL(sdlWindow)) {
        SDL_SetWindowTitle(window, str);
    }
}

// 0x7451D0
void psErrorMessage(const char* str) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, RsGlobal.appName, str, PSGLOBAL(sdlWindow));
}

// 0x7451F0
void psWarningMessage(const char* str) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, RsGlobal.appName, str, PSGLOBAL(sdlWindow));
}

// 0x745210
bool psCameraBeginUpdate(RwCamera* camera) {
    UNUSED(camera);

    if (RwCameraBeginUpdate(Scene.m_pRwCamera)) {
        return true;
    }
    RsEventHandler(rsACTIVATE, nullptr);
    return false;
}

// 0x745240
RwCamera* psCameraShowRaster(RwCamera* camera) {
    const auto flags = FrontEndMenuManager.m_bPrefsFrameLimiter || CLoadingScreen::IsActive()
        ? rwRASTERFLIPWAITVSYNC
        : rwRASTERFLIPDONTWAIT;
    return RwCameraShowRaster(camera, PSGLOBAL(window), flags);
}

// 0x745270
uint32 psTimer() {
    return static_cast<uint32>(SDL_GetTicks());
}

// 0x7452B0
RwImage* psGrabScreen(RwCamera* camera) {
    UNUSED(camera);
    return nullptr; // Needs the GL backend (see plan B3)
}

// 0x7453E0
void psMouseSetVisibility(bool visible) {
    if (visible) {
        SDL_ShowCursor();
    } else {
        SDL_HideCursor();
    }
}

// 0x7453F0
void psMouseSetPos(RwV2d* pos) {
    if (auto* window = PSGLOBAL(sdlWindow)) {
        SDL_WarpMouseInWindow(window, pos->x, pos->y);
    }
    PSGLOBAL(lastMousePos) = *pos;
}

// 0x745470
char* psPathnameCreate(const char* buffer) {
    const auto pathSize = std::strlen(buffer) + 1u;
    auto       path     = static_cast<char*>(CMemoryMgr::Malloc(pathSize));
    if (path) {
        std::memcpy(path, buffer, pathSize);

        while (auto ch = std::strchr(path, '/')) {
            *ch = psPathGetSeparator();
        }
    }

    return path;
}

// 0x7454E0
void psPathnameDestroy(char* buffer) {
    if (buffer) {
        CMemoryMgr::Free(buffer);
    }
}

// 0x745500
char psPathGetSeparator() {
    return '/';
}

// 0x745520
bool psInstallFileSystem() {
    return true;
}

// 0x745530
bool psNativeTextureSupport() {
    // The GL backend (B3) always supports compressed textures.
    return true;
}

// 0x745540
RsEventStatus psDebugMessageHandler(RsEvent event, void* param) {
    UNUSED(event, param);
    return rsEVENTNOTPROCESSED;
}

// 0x7458B0
bool psAlwaysOnTop(bool alwaysOnTop) {
    auto* window = PSGLOBAL(sdlWindow);
    if (!window) {
        return false;
    }
    return SDL_SetWindowAlwaysOnTop(window, alwaysOnTop);
}

// 0x746190
bool psSelectDevice() {
    // There's a single (GL) device, selected when the window is created.
    return true;
}

#endif // NOTSA_NATIVE
