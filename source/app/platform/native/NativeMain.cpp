#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include <SDL3/SDL.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "app/platform/native/ExeGlobals.h"
#include "app/platform/native/NativePath.h"
#include "app/platform/native/VideoPlayerNative.h"
#include "app/platform/SDLWrapper.hpp"

#include "app/app.h"
#include "LoadingScreen.h"
#include "ControllerConfigManager.h"
#include "Gamma.h"
#include "FileMgr.h"
#include "MenuManager.h"
#include "Game.h"
#include "extensions/utility.hpp"
#include "extensions/Configs/FastLoader.hpp"

namespace fs = std::filesystem;

//! Symbol which `app/platform/win/WinMain.cpp` provides on Windows (that file is
//! excluded from the native build). `IsForegroundApp` is inlined in `app.h` instead.
void MessageLoop() {
    notsa::SDLWrapper::ProcessEvents();
}

namespace {

constexpr auto NO_FOREGROUND_PAUSE = true;
constexpr auto NATIVE_CMD_SHOW     = 1; // `SW_SHOWNORMAL` equivalent

// Code from winmain, 0x748DCF (Without any Win32/DirectShow/video mode specific parts)
bool ProcessGameLogic(int32 nCmdShow) {
    ZoneScoped;
    if (RsGlobal.quit || FrontEndMenuManager.m_bStartGameLoading) {
        return false;
    }

    notsa::SDLWrapper::ProcessEvents();

    // Game is in background
    if (!NO_FOREGROUND_PAUSE && !ForegroundApp) {
        OS_ThreadSleep(100);
        return true;
    }

    switch (gGameState) {
    case GAME_STATE_INITIAL: {
        ZoneScopedN("INITIAL");

        const auto ProcessSplash = [](eLoadingLogo id) {
            CLoadingScreen::LoadSplashes(true, id);
            CLoadingScreen::Init(true, true);
            CLoadingScreen::DoPCTitleFadeIn();
            CLoadingScreen::DoPCTitleFadeOut();
            CLoadingScreen::Shutdown();
        };
        if (!g_FastLoaderConfig.NoEAX) {
            ProcessSplash(eLoadingLogo::EAX);
        }
        if (!g_FastLoaderConfig.NoNVidia) {
            ProcessSplash(eLoadingLogo::NVIDIA);
        }
        ChangeGameStateTo(GAME_STATE_LOGO);
        break;
    }
    case GAME_STATE_LOGO: {
        ZoneScopedN("LOGO");

        if (!g_FastLoaderConfig.NoLogo) {
            VideoPlayer::Play(nCmdShow, "movies\\Logo.mpg");
        }
        ChangeGameStateTo(g_FastLoaderConfig.NoLogo ? GAME_STATE_TITLE : GAME_STATE_PLAYING_LOGO);
        break;
    }
    case GAME_STATE_PLAYING_LOGO:
    case GAME_STATE_PLAYING_INTRO: { // 0x748B17
        ZoneScopedN("PLAYING_LOGO & PLAYING_INTRO");

        // Videos are skipped until B4: this performs the transition the Windows backend
        // does on `EC_COMPLETE`.
        VideoPlayer::OnGraphNotify();

        CPad::UpdatePads();
        auto* pad = CPad::GetPad();
        if (ControlsManager.GetJoyButtonJustDown()
            || pad->NewState.CheckForInput()
            || CPad::IsMouseLButtonPressed()
            || CPad::IsEnterJustPressed()
            || pad->IsStandardKeyJustPressed(VK_SPACE)
            || CPad::IsMenuKeyJustPressed()
            || CPad::IsTabJustPressed()
        ) {
            ChangeGameStateTo([] {
                switch (gGameState) {
                case GAME_STATE_PLAYING_LOGO:  return GAME_STATE_TITLE;
                case GAME_STATE_PLAYING_INTRO: return GAME_STATE_FRONTEND_LOADING;
                default:                       NOTSA_UNREACHABLE();
                }
            }());
        }
        break;
    }
    case GAME_STATE_TITLE: {
        ZoneScopedN("TITLE");

        if (!g_FastLoaderConfig.NoTitleOrIntro) {
            VideoPlayer::Shutdown();
            VideoPlayer::Play(nCmdShow, FrontEndMenuManager.GetMovieFileName());
        }
        ChangeGameStateTo(g_FastLoaderConfig.NoTitleOrIntro ? GAME_STATE_FRONTEND_LOADING : GAME_STATE_PLAYING_INTRO);
        break;
    }
    case GAME_STATE_FRONTEND_LOADING: {
        ZoneScopedN("FRONTEND_LOADING");

        VideoPlayer::Shutdown();
        CLoadingScreen::Init(true, false);
        if (!g_FastLoaderConfig.NoCopyright) {
            CLoadingScreen::DoPCTitleFadeIn();
        }
        if (!CGame::InitialiseEssentialsAfterRW()) {
            RsGlobal.quit = true;
        }
        CGame::InitialiseCoreDataAfterRW();
        ChangeGameStateTo(GAME_STATE_FRONTEND_LOADED);

        // NOTE: `anisotropySupportedByGFX` is queried from the D3D9 caps on Windows;
        // there's no such thing until the GL backend (B3) exists.

        break;
    }
    case GAME_STATE_FRONTEND_LOADED: {
        ZoneScopedN("FRONTEND_LOADED");

        FrontEndMenuManager.m_bActivateMenuNextFrame = true;
        FrontEndMenuManager.m_bMainMenuSwitch        = true;
        ChangeGameStateTo(GAME_STATE_FRONTEND_IDLE);
        if (g_FastLoaderConfig.NoCopyright) {
            CLoadingScreen::SkipCopyrightSplash();
        } else {
            CLoadingScreen::DoPCTitleFadeOut();
        }
        break;
    }
    case GAME_STATE_FRONTEND_IDLE: { // 0x748CB2
        ZoneScopedN("FRONTEND_IDLE");

        if (g_FastLoaderConfig.ShouldLoadSaveGame()) {
            // We need to still run the frontend processing once because it has some important stuff
            RsEventHandler(rsFRONTENDIDLE, nullptr);
            g_FastLoaderConfig.StartGame(g_FastLoaderConfig.SaveGameToLoad);
            g_FastLoaderConfig.TriedLoadingSaveGame = true;
        } else {
            RsEventHandler(rsFRONTENDIDLE, nullptr);
        }
        if (FrontEndMenuManager.m_bMenuActive && !FrontEndMenuManager.m_bLoadingData) {
            break;
        }
        ChangeGameStateTo(GAME_STATE_LOADING_STARTED);
        if (!FrontEndMenuManager.m_bLoadingData) {
            break;
        }
        NOTSA_SWCFALLTHRU; // Fall down and start loading
    }
    case GAME_STATE_LOADING_STARTED: {
        ZoneScopedN("LOADING_STARTED");

        if (!g_FastLoaderConfig.NoLoadingTune) {
            AudioEngine.StartLoadingTune();
        }

        InitialiseGame();
        ChangeGameStateTo(GAME_STATE_IDLE);
        FrontEndMenuManager.m_bMainMenuSwitch = false;

        AudioEngine.InitialisePostLoading();

        break;
    }
    case GAME_STATE_IDLE: { // 0x748AA1
        ZoneScopedN("IDLE");

        if (!RwInitialized) {
            break;
        }

        auto v9_1 = 1000.0f / (float)RsGlobal.frameLimit;
        auto v9_2 = (float)CTimer::GetCurrentTimeInCycles() / (float)CTimer::GetCyclesPerMillisecond();
        if (!FrontEndMenuManager.m_bPrefsFrameLimiter && CReplay::Mode != eReplayMode::MODE_PLAYBACK && !AudioEngine.IsBeatInfoPresent() || v9_1 < v9_2) {
            RsEventHandler(rsIDLE, (void*)true);
        }
        break;
    }
    }

    return true;
}

// Code from winmain, 0x7489FB
void MainLoop(int32 nCmdShow) {
    bool isNewGameFirstTime = true;
    while (true) {
        RwInitialized = true;

        gamma.Init();

        // Game logic main loop
        while (ProcessGameLogic(nCmdShow)) {
            FrameMark;
        }

        // 0x748DDA
        RwInitialized = false;
        FrontEndMenuManager.UnloadTextures();
        if (!FrontEndMenuManager.m_bStartGameLoading) {
            break;
        }

        // load game
        CCheat::ResetCheats();
        CTimer::Stop();

        if (FrontEndMenuManager.m_bLoadingData) {
            CGame::ShutDownForRestart();
            CGame::InitialiseWhenRestarting();
            FrontEndMenuManager.m_bLoadingData = false;
        } else if (isNewGameFirstTime) {
            CTimer::Stop();
            ChangeGameStateTo(
                FrontEndMenuManager.m_nGameState != 1
                    ? GAME_STATE_LOADING_STARTED
                    : GAME_STATE_FRONTEND_LOADED
            );
        } else {
            CCheat::ResetCheats();
            CGame::ShutDownForRestart();
            CTimer::Stop();
            CGame::InitialiseWhenRestarting();
        }

        isNewGameFirstTime = false;
        FrontEndMenuManager.m_nGameState = 0;
        FrontEndMenuManager.m_bStartGameLoading = false;
    }
}

int32 NativeMain(int argc, char** argv) {
    // 1. Locate the game directory.
    std::string gameDir;
    for (int32 i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--game-dir") == 0 && i + 1 < argc) {
            gameDir = argv[++i];
        }
    }
    if (gameDir.empty()) {
        if (const char* envDir = std::getenv("GTASA_DIR")) {
            gameDir = envDir;
        }
    }
    if (gameDir.empty()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GTA: San Andreas", "Pass --game-dir or set GTASA_DIR", nullptr);
        return 1;
    }

    // 2. Validate the game directory (case-insensitively) and make it the working directory.
    const auto normalizedGameDir = notsa::native::NormalizePath(gameDir);
    const auto gameImg           = notsa::native::NormalizePath(normalizedGameDir + "/models/gta3.img");
    if (!fs::exists(gameImg)) {
        const auto msg = std::format("`{}/models/gta3.img` not found", normalizedGameDir);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GTA: San Andreas", msg.c_str(), nullptr);
        return 1;
    }

    std::error_code ec;
    fs::current_path(normalizedGameDir, ec);
    if (ec) {
        const auto msg = std::format("Failed to enter `{}`: {}", normalizedGameDir, ec.message());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GTA: San Andreas", msg.c_str(), nullptr);
        return 1;
    }

    // 3. Logging and executable globals.
    fs::create_directories("logs", ec);
    notsa::Logging::CreateInstance();
    notsa::ScopeGuard cleanupLogging{ [] { notsa::Logging::DestroyInstance(); } };

    notsa::native::ExeGlobals::ReportOverlaps();

    // 4. Neutral part of `NOTSA_WinMain`.
    if (rsEVENTERROR == RsEventHandler(rsINITIALIZE, nullptr)) {
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC)) {
        NOTSA_LOG_ERR("Failed to initialize SDL: {}", SDL_GetError());
        return 1;
    }

    for (int32 i = 0; i < argc; i++) {
        RsEventHandler(rsPREINITCOMMANDLINE, argv[i]);
    }

    SDL_Window* sdlWnd = SDL_CreateWindow(
        "GTA: San Andreas",
        APP_DEFAULT_WIDTH, APP_DEFAULT_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!sdlWnd) {
        NOTSA_LOG_ERR("Failed to create the SDL window: {}", SDL_GetError());
        return 1;
    }
    PSGLOBAL(sdlWindow) = sdlWnd;
    PSGLOBAL(window)    = reinterpret_cast<HWND>(sdlWnd);

    ControlsManager.ReinitControls();

    // 0x748847
    if (RsEventHandler(rsRWINITIALIZE, PSGLOBAL(window)) == rsEVENTERROR) {
        SDL_DestroyWindow(sdlWnd);
        RsEventHandler(rsTERMINATE, nullptr);
        return 1;
    }

    // 0x7488EE
    for (int32 i = 0; i < argc; i++) {
        RsEventHandler(rsCOMMANDLINE, argv[i]);
    }

    RwRect rect{ 0, 0, RsGlobal.maximumWidth, RsGlobal.maximumHeight };
    RsEventHandler(rsCAMERASIZE, &rect);

    SDL_SetWindowFocusable(sdlWnd, true);
    SDL_SetWindowPosition(sdlWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RaiseWindow(sdlWnd);

    // 0x748995
    CFileMgr::SetDirMyDocuments();
    if (auto* file = CFileMgr::OpenFile("gta_sa.set", "rb")) {
        if (!ControlsManager.LoadSettings(file)) {
            ControlsManager.ReinitControls();
        }
        CFileMgr::CloseFile(file);
    }
    CFileMgr::SetDir("");

    // 0x7489FB
    MainLoop(NATIVE_CMD_SHOW);

    // if game is loaded, shut it down
    if (gGameState == GAME_STATE_IDLE) {
        CGame::Shutdown();
    }

    // now quit 0x748E75
    AudioEngine.Shutdown();
    RsEventHandler(rsRWTERMINATE, nullptr);
    SDL_DestroyWindow(sdlWnd);
    RsEventHandler(rsTERMINATE, nullptr);
    SDL_ShowCursor();

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    return NativeMain(argc, argv);
}

#endif // NOTSA_NATIVE
