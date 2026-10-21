#pragma once

struct IMediaControl;

namespace VideoPlayer {
/*!
* @brief Native counterpart of the DirectShow video player.
*
* The movies (`movies/Logo.mpg`, `movies/GTAtitles.mpg`, ...) are decoded with the vendored
* `pl_mpeg` decoder (`third_party/pl_mpeg.h`) and presented full-screen through the librw GL
* context, with the audio pushed to a plain SDL3 audio stream.
*
* Like the Windows backend, `Play` only *starts* the movie: the frames are decoded and
* presented by `OnGraphNotify`, which the game loop calls once per iteration while in
* `GAME_STATE_PLAYING_LOGO`/`GAME_STATE_PLAYING_INTRO`. When the movie ends - or when the
* game skips it, which calls `Shutdown` on the next game state - the decoder is released.
* The rest of the API mirrors `app/platform/win/VideoPlayer/VideoPlayer.h`.
*/

void InjectHooks();

void Shutdown();
void WaitState(int32 state);
void OnGraphNotify();
void UpdateWindow();
void Release();
void Play(int32 nCmdShow, const char* path);
auto GetMediaControl() -> IMediaControl*;

} // namespace VideoPlayer
