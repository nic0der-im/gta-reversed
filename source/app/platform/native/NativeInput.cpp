#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include <SDL3/SDL.h>

#include <mutex>
#include <vector>

#include "NativeInput.h"

#include "Game.h"
#include "MenuManager.h"
#include "Pad.h"

namespace notsa::native::Input {

namespace {

SDL_Window* s_Window{};

//! Last mode applied through `SDL_SetWindowRelativeMouseMode`.
bool s_MouseCaptured{};

//! F7 latch: the debug UI is compiled out of the native build until the GL tools menu exists.
bool s_DebugMenuOpen{};
bool s_F7WasDown{};

bool s_EventWatchInstalled{};

//! Gamepad hot-plug events, queued by the SDL event watch (which may run on any thread) and
//! forwarded to the pads from `Update`, on the main thread.
std::mutex             s_PendingEventsMutex;
std::vector<SDL_Event> s_PendingEvents;

bool WindowHasFocus(SDL_Window* window) {
    return window && (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS);
}

bool IsKeyDown(SDL_Scancode scancode) {
    const bool* const keys = SDL_GetKeyboardState(nullptr);
    return keys && keys[scancode];
}

//! Forwards a queued gamepad event to every pad's SDL handler.
void ForwardToPads(const SDL_Event& event) {
    for (auto& pad : CPad::Pads) {
        pad.ProcessGamepadEvent(event, pad.PCTempJoyState);
    }
}

bool OnSdlEvent(void* userdata, SDL_Event* event) {
    UNUSED(userdata);

    switch (event->type) {
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED: {
        NOTSA_LOG_INFO(
            "SDL: Gamepad (instance {}) {}",
            event->gdevice.which,
            event->type == SDL_EVENT_GAMEPAD_ADDED ? "added" : "removed"
        );

        const std::scoped_lock lock{ s_PendingEventsMutex };
        s_PendingEvents.push_back(*event);
        break;
    }
    }

    return true; // The return value is ignored for watches; keep the event
}

void UpdateMouseCapture() {
    if (!s_Window) {
        return;
    }

    const bool inGame      = gGameState == GAME_STATE_IDLE && !FrontEndMenuManager.m_bMenuActive;
    const bool wantCapture = inGame && !s_DebugMenuOpen && WindowHasFocus(s_Window);
    if (wantCapture == s_MouseCaptured) {
        return;
    }

    s_MouseCaptured = wantCapture;
    if (!SDL_SetWindowRelativeMouseMode(s_Window, wantCapture)) {
        NOTSA_LOG_WARN("SDL: Failed to {} relative mouse mode: {}", wantCapture ? "enable" : "disable", SDL_GetError());
    }
}

} // namespace

void Initialize(SDL_Window* window) {
    s_Window        = window;
    s_MouseCaptured = false;
    s_DebugMenuOpen = false;
    s_F7WasDown     = false;

    if (!s_EventWatchInstalled) {
        s_EventWatchInstalled = SDL_AddEventWatch(&OnSdlEvent, nullptr);
        if (!s_EventWatchInstalled) {
            NOTSA_LOG_WARN("SDL: Failed to install the gamepad hot-plug watch: {}", SDL_GetError());
        }
    }

    Update(); // Apply the policy right away
}

void Shutdown() {
    if (s_EventWatchInstalled) {
        SDL_RemoveEventWatch(&OnSdlEvent, nullptr);
        s_EventWatchInstalled = false;
    }

    if (s_Window && s_MouseCaptured) {
        SDL_SetWindowRelativeMouseMode(s_Window, false);
    }

    {
        const std::scoped_lock lock{ s_PendingEventsMutex };
        s_PendingEvents.clear();
    }

    s_Window        = nullptr;
    s_MouseCaptured = false;
}

void Update() {
    // Gamepad hot-plug, forwarded on the main thread.
    std::vector<SDL_Event> pending;
    {
        const std::scoped_lock lock{ s_PendingEventsMutex };
        pending.swap(s_PendingEvents);
    }
    for (const auto& event : pending) {
        ForwardToPads(event);
    }

    // F7 releases the mouse on Windows too: there it opens the debug UI, whose
    // `UIRenderer::SetIsActive` disables the relative mouse mode.
    const bool f7Down = IsKeyDown(SDL_SCANCODE_F7);
    if (f7Down && !s_F7WasDown) {
        SetDebugMenuOpen(!s_DebugMenuOpen);
    }
    s_F7WasDown = f7Down;

    UpdateMouseCapture();
}

bool IsMouseCaptured() {
    return s_MouseCaptured;
}

bool IsDebugMenuOpen() {
    return s_DebugMenuOpen;
}

void SetDebugMenuOpen(bool open) {
    s_DebugMenuOpen = open;
}

} // namespace notsa::native::Input

#endif // NOTSA_NATIVE
