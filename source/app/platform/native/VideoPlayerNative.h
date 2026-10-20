#pragma once

struct IMediaControl;

namespace VideoPlayer {
/*!
* @brief Native counterpart of the DirectShow video player.
*
* Until B4 there is no video backend: `Play` skips the movie and marks it as finished, and
* `OnGraphNotify` performs the same game state transition the Windows backend does on
* `EC_COMPLETE`. The rest of the API mirrors `app/platform/win/VideoPlayer/VideoPlayer.h`.
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
