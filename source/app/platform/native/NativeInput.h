#pragma once

#ifdef NOTSA_NATIVE

//! Native input glue for the SDL3 window, wired from `NativeMain.cpp` only.
//!
//! It owns two pieces of platform behaviour the SDL backend of the game doesn't cover:
//! - The mouse capture policy: the relative (mouselook) mode is enabled while the player is in
//!   game, and disabled in the menus, when the window loses focus and while the debug UI is
//!   open (F7).
//! - Gamepad hot-plug: `SDL_EVENT_GAMEPAD_ADDED`/`SDL_EVENT_GAMEPAD_REMOVED` are forwarded to
//!   `CPad::ProcessGamepadEvent` (joystick events already go through `SDLWrapper::ProcessEvents`
//!   into `CPad::ProcessEvent`).

struct SDL_Window;

namespace notsa::native::Input {

//! Enables the input glue for `window`. Call once the SDL window exists, before the main loop.
void Initialize(SDL_Window* window);

//! Removes the event watch and gives the mouse back (absolute, visible).
void Shutdown();

//! Applies the mouse capture policy for the current frame. Call once per frame.
void Update();

//! Whether the mouse is currently captured (relative mode enabled).
bool IsMouseCaptured();

//! Whether the debug UI is open. The Windows debug UI (`UIRenderer::SetIsActive`) opens with F7
//! and releases the mouse; the native build has no debug UI until the GL tools menu returns, so
//! F7 toggles this flag directly. When the tools menu comes back it should call
//! `SetDebugMenuOpen` from `UIRenderer::SetIsActive` instead of toggling the mouse itself.
bool IsDebugMenuOpen();
void SetDebugMenuOpen(bool open);

} // namespace notsa::native::Input

#endif // NOTSA_NATIVE
