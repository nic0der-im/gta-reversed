#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include <SDL3/SDL.h>

#include <cstdlib>
#include <filesystem>
#include <string_view>

#include "app/platform/native/ExeData.generated.h"
#include "app/platform/native/ExeGlobals.h"
#include "app/platform/native/NativeCrash.h"
#include "app/platform/native/NativeGameDir.h"
#include "app/platform/native/NativeInput.h"
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
#include "extensions/Configs/ConfigLoader.h"
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

//! `GTASA_NEW_GAME=1` starts a new game as soon as the frontend is idle, skipping the menu input.
bool IsNewGameRequested() {
    static const bool requested = [] {
        const auto env = std::getenv("GTASA_NEW_GAME");
        return env && std::string_view{env} == "1";
    }();
    return requested;
}

// Code from winmain, 0x748DCF (Without any Win32/DirectShow/video mode specific parts)
bool ProcessGameLogic(int32 nCmdShow) {
    ZoneScoped;
    if (RsGlobal.quit || FrontEndMenuManager.m_bStartGameLoading) {
        return false;
    }

    notsa::SDLWrapper::ProcessEvents();
    notsa::native::Input::Update(); // Mouse capture policy + queued gamepad hot-plug

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

        // Decodes and presents the movie. On end of file it performs the same transition the
        // Windows backend does on `EC_COMPLETE`.
        VideoPlayer::OnGraphNotify();

        CPad::UpdatePads();
        auto* pad = CPad::GetPad();

        // `OnGraphNotify` may have just advanced the game state; in that case the movie is over
        // and there's nothing left to skip (the state switch below has no branch for it).
        const auto IsPlayingMovie = [] {
            return gGameState == GAME_STATE_PLAYING_LOGO || gGameState == GAME_STATE_PLAYING_INTRO;
        };

        // ESC (menu key), Enter and Space (among others) skip the movie: stop the player here,
        // the game state change below is what actually ends the movie state.
        if (IsPlayingMovie() && (
                ControlsManager.GetJoyButtonJustDown()
                || pad->NewState.CheckForInput()
                || CPad::IsMouseLButtonPressed()
                || CPad::IsEnterJustPressed()
                || pad->IsStandardKeyJustPressed(VK_SPACE)
                || CPad::IsMenuKeyJustPressed()
                || CPad::IsTabJustPressed()
            )
        ) {
            VideoPlayer::Shutdown();
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

            // Dev harness: `GTASA_NEW_GAME=1` starts a new game without any menu input.
            static bool startedNewGame = false;
            if (IsNewGameRequested() && !startedNewGame) {
                startedNewGame = true;
                NOTSA_LOG_INFO("Native: GTASA_NEW_GAME -> starting a new game");
                FrontEndMenuManager.SimulateGameLoad(true, 0);
            }
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
    // 1. Locate and validate the game directory: `--game-dir`, `GTASA_DIR`, the directory picked
    //    on an earlier run, or a folder picker (see `NativeGameDir::Resolve`).
    const auto gameDir = notsa::native::NativeGameDir::Resolve(argc, argv);
    if (gameDir.empty()) {
        return 1;
    }

    // 2. Make the game directory the working directory.
    std::error_code ec;
    fs::current_path(gameDir, ec);
    if (ec) {
        const auto msg = std::format("Failed to enter `{}`: {}", gameDir, ec.message());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GTA: San Andreas", msg.c_str(), nullptr);
        return 1;
    }

    // 3. Logging and executable globals.
    fs::create_directories("logs", ec);
    notsa::Logging::CreateInstance();
    notsa::ScopeGuard cleanupLogging{ [] { notsa::Logging::DestroyInstance(); } };

    notsa::native::ExeGlobals::ReportOverlaps();
    notsa::native::ApplyExeData(); // Replays the executable's CRT static initializers

    // 3b. Configurations (`gta-reversed.ini` is optional) and the dev harness switches.
    notsa::LoadConfigurations();
    if (IsNewGameRequested()) {
        // Nothing is shown before the game itself: no splash screens, no movies, no loading tune.
        g_FastLoaderConfig.NoEAX          = true;
        g_FastLoaderConfig.NoNVidia       = true;
        g_FastLoaderConfig.NoLogo         = true;
        g_FastLoaderConfig.NoTitleOrIntro = true;
        g_FastLoaderConfig.NoLoadingTune  = true;
        g_FastLoaderConfig.SaveGameToLoad = -2; // A new game never auto-loads a save slot
    }

    // 4. Neutral part of `NOTSA_WinMain`.
    if (rsEVENTERROR == RsEventHandler(rsINITIALIZE, nullptr)) {
        return 1;
    }

    // `SDL_INIT_AUDIO` is only used by the movie player (see `VideoPlayerNative.cpp`); the game's
    // audio engine has its own backend.
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC)) {
        NOTSA_LOG_ERR("Failed to initialize SDL: {}", SDL_GetError());
        return 1;
    }

    for (int32 i = 0; i < argc; i++) {
        RsEventHandler(rsPREINITCOMMANDLINE, argv[i]);
    }

    ControlsManager.ReinitControls();

    // 0x748847
    // The native RenderWare core (librw) creates the window and the GL context in
    // `RwEngineStart`, which runs inside `rsRWINITIALIZE`. Nothing to create here.
    if (RsEventHandler(rsRWINITIALIZE, nullptr) == rsEVENTERROR) {
        RsEventHandler(rsTERMINATE, nullptr);
        return 1;
    }

    SDL_Window* sdlWnd = PSGLOBAL(sdlWindow);
    if (!sdlWnd) {
        NOTSA_LOG_ERR("The RenderWare backend did not create a window");
        RsEventHandler(rsRWTERMINATE, nullptr);
        RsEventHandler(rsTERMINATE, nullptr);
        return 1;
    }

    notsa::native::Input::Initialize(sdlWnd);

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
    VideoPlayer::Shutdown();  // Release the movie (and its GL objects) while the context is alive
    notsa::native::Input::Shutdown();
    RsEventHandler(rsRWTERMINATE, nullptr); // librw destroys its window in RwEngineStop
    RsEventHandler(rsTERMINATE, nullptr);
    SDL_ShowCursor();

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    notsa::native::InstallCrashHandler();
    return NativeMain(argc, argv);
}

#endif // NOTSA_NATIVE
