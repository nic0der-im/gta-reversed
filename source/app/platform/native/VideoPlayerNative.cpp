#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "VideoPlayerNative.h"

namespace VideoPlayer {

namespace {
//! Set by `Play`, consumed by `OnGraphNotify` (which performs the game state transition a
//! real video would trigger on `EC_COMPLETE`).
bool s_VideoFinished{};
} // namespace

void InjectHooks() {
    // No hooks in the native build (see plan B1.3).
}

//! 0x746740
void Release() {
    s_VideoFinished = false;
}

//! 0x7467B0
void Shutdown() {
    Release();
}

//! 0x745F90
void WaitState(int32 state) {
    // `Play` never starts a video, so any state is reached immediately.
    UNUSED(state);
}

//! 0x745FB0
void OnGraphNotify() {
    if (!s_VideoFinished) {
        return;
    }
    s_VideoFinished = false;

    // Same transition as the `EC_COMPLETE` event in the Windows backend.
    ChangeGameStateTo([] {
        switch (gGameState) {
        case GAME_STATE_PLAYING_LOGO:  return GAME_STATE_TITLE;
        case GAME_STATE_PLAYING_INTRO: return GAME_STATE_FRONTEND_LOADING;
        default:                       return eGameState(gGameState); // No change
        }
    }());
}

//! 0x7466D0
void UpdateWindow() {
    // Nothing to position without a video window.
}

//! 0x747660
void Play(int32 nCmdShow, const char* path) {
    UNUSED(nCmdShow);

    NOTSA_LOG_DEBUG("Video playback is not implemented until B4, skipping `{}`", path);
    s_VideoFinished = true;
}

auto GetMediaControl() -> IMediaControl* {
    return nullptr;
}

} // namespace VideoPlayer

#endif // NOTSA_NATIVE
