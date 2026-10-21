#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include <SDL3/SDL.h>

// The OpenGL functions come from librw's glad loadout (`librw::librw` exposes its source root as
// an include dir), so the video blit uses the very same function pointers librw loaded.
#include <src/gl/glad/glad.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <vector>

//! `pl_mpeg` (MIT) is vendored - see the provenance comment at the top of the header.
#define PL_MPEG_IMPLEMENTATION
#include "third_party/pl_mpeg.h"

#include "VideoPlayerNative.h"

#include "app/platform/native/NativePath.h"

namespace VideoPlayer {

namespace {

//! How far ahead of the video the MP2 audio is decoded, so the SDL stream never starves.
constexpr auto AUDIO_LEAD_TIME = 0.25;

//! Upper bound for the queued audio; if the device can't keep up the stale samples are dropped
//! instead of growing the latency for the whole movie.
constexpr auto MAX_QUEUED_AUDIO_SECONDS = 1.0;

//! DirectShow's `OAFilterState` values (the Windows backend's `WaitState` contract).
constexpr int32 FILTER_STATE_STOPPED = 0;
constexpr int32 FILTER_STATE_PAUSED  = 1;
constexpr int32 FILTER_STATE_RUNNING = 2;

//! A triangle that covers the whole clip space (the classic "fullscreen triangle"): a quad
//! would only add a diagonal seam and a fourth vertex.
//! The texture is flipped vertically because `pl_mpeg` hands us top-down rows while GL's `v`
//! coordinate grows upwards.
constexpr float FULLSCREEN_TRIANGLE[] = {
    // x     y     u     v
    -1.f, -1.f, 0.f,  1.f,
     3.f, -1.f, 2.f,  1.f,
    -1.f,  3.f, 0.f, -1.f,
};

constexpr const char* VERTEX_SHADER_SOURCE = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    vTexCoord   = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT_SHADER_SOURCE = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D uTexture;
void main() {
    fragColor = texture(uTexture, vTexCoord);
}
)";

//! State of the movie currently being played (there's never more than one).
struct VideoState {
    plm_t*           decoder{};
    SDL_AudioStream* audio{};
    int32            audioSampleRate{};

    //! Last decoded frame, as RGBA pixels (`plm_frame_to_rgba` layout, `width * 4` stride).
    std::vector<uint8_t> pixels;
    int32                width{};
    int32                height{};
    bool                 framePending{};

    //! GL objects, created on the first frame and reused by the following movies.
    GLuint  program{};
    GLint   samplerLocation{-1};
    GLuint  vao{};
    GLuint  vbo{};
    GLuint  texture{};
    int32   textureWidth{};
    int32   textureHeight{};

    //! Pacing: the game loop calls `OnGraphNotify` in a tight loop, the movie follows the clock.
    std::chrono::steady_clock::time_point lastDecode{};
    std::chrono::steady_clock::time_point nextFrameTime{};
};

VideoState g_Video;

//! Set when the movie ended (or couldn't be played at all); consumed by `OnGraphNotify`, which
//! performs the game state transition the Windows backend does on `EC_COMPLETE`.
bool s_VideoFinished{};

//! Saves and restores the GL state the video blit touches.
//!
//! librw keeps its own copy of the state it sets (`oldGlState`/`curGlState`, the bound
//! program/VAO/texture/framebuffer) and only issues a driver call when that copy changes, so
//! the driver state *must* be left exactly as found - otherwise librw would render with our
//! program, texture or VAO bound.
struct GlStateGuard {
    GLint     framebuffer{};
    GLint     viewport[4]{};
    GLint     vao{};
    GLint     arrayBuffer{};
    GLint     program{};
    GLint     activeTexture{};
    GLint     texture2D{};
    GLint     depthFunc{};
    GLint     blendSrc{};
    GLint     blendDst{};
    GLboolean depthTest{};
    GLboolean depthWrite{};
    GLboolean blend{};
    GLboolean cull{};
    GLboolean stencil{};
    GLboolean scissor{};

    GlStateGuard() {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture2D);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_RGB, &blendDst);
        depthTest  = glIsEnabled(GL_DEPTH_TEST);
        depthWrite = GL_FALSE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
        blend      = glIsEnabled(GL_BLEND);
        cull       = glIsEnabled(GL_CULL_FACE);
        stencil    = glIsEnabled(GL_STENCIL_TEST);
        scissor    = glIsEnabled(GL_SCISSOR_TEST);
    }

    ~GlStateGuard() {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glUseProgram(program);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
        glBindTexture(GL_TEXTURE_2D, texture2D);
        glDepthFunc(depthFunc);
        glDepthMask(depthWrite);
        glBlendFunc(blendSrc, blendDst);
        (depthTest ? glEnable : glDisable)(GL_DEPTH_TEST);
        (blend ? glEnable : glDisable)(GL_BLEND);
        (cull ? glEnable : glDisable)(GL_CULL_FACE);
        (stencil ? glEnable : glDisable)(GL_STENCIL_TEST);
        (scissor ? glEnable : glDisable)(GL_SCISSOR_TEST);
    }
};

//! Compiles one shader, logging (and returning 0 on) failure.
GLuint CompileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[1024]{};
        glGetShaderInfoLog(shader, sizeof(log) - 1, nullptr, log);
        NOTSA_LOG_ERR("Video: {} shader failed to compile: {}", type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

//! Builds the program that blits the movie texture, or returns 0 on failure.
GLuint CreateVideoProgram() {
    const GLuint vertexShader   = CompileShader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE);
    const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE);
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader) {
            glDeleteShader(fragmentShader);
        }
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024]{};
        glGetProgramInfoLog(program, sizeof(log) - 1, nullptr, log);
        NOTSA_LOG_ERR("Video: the blit program failed to link: {}", log);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

//! Creates the GL objects, if they don't exist yet. The movie size must be known.
bool CreateGlObjects() {
    if (!g_Video.program) {
        const GlStateGuard guard; // Creating/binding our objects mustn't disturb librw's state

        g_Video.program = CreateVideoProgram();
        if (!g_Video.program) {
            return false;
        }
        g_Video.samplerLocation = glGetUniformLocation(g_Video.program, "uTexture");

        glGenVertexArrays(1, &g_Video.vao);
        glBindVertexArray(g_Video.vao);

        glGenBuffers(1, &g_Video.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, g_Video.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FULLSCREEN_TRIANGLE), FULLSCREEN_TRIANGLE, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    }

    if (!g_Video.texture) {
        const GlStateGuard guard;

        glGenTextures(1, &g_Video.texture);
        glBindTexture(GL_TEXTURE_2D, g_Video.texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    return true;
}

//! Uploads the decoded frame and draws it over the whole SDL window.
bool PresentFrame() {
    auto* const window = PSGLOBAL(sdlWindow);
    if (!window || !g_Video.width || !g_Video.height) {
        return false;
    }
    if (!CreateGlObjects()) {
        return false;
    }

    GLint activeTexture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);

    int32 drawableWidth = 0, drawableHeight = 0;
    SDL_GetWindowSizeInPixels(window, &drawableWidth, &drawableHeight);
    if (!drawableWidth || !drawableHeight) {
        return true; // Minimised - nothing to draw on, but the movie keeps running
    }

    {
        const GlStateGuard guard;

        // Draw over the window itself (librw owns its camera framebuffers as well).
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, drawableWidth, drawableHeight);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glUseProgram(g_Video.program);
        glBindVertexArray(g_Video.vao);

        // The sampler reads from the texture unit that is already active, so the blit never
        // changes `glActiveTexture` (librw caches the binding of every unit separately).
        glUniform1i(g_Video.samplerLocation, activeTexture - GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_Video.texture);

        if (g_Video.textureWidth != g_Video.width || g_Video.textureHeight != g_Video.height) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_Video.width, g_Video.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            g_Video.textureWidth  = g_Video.width;
            g_Video.textureHeight = g_Video.height;
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_Video.width, g_Video.height, GL_RGBA, GL_UNSIGNED_BYTE, g_Video.pixels.data());

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    SDL_GL_SwapWindow(window);
    return true;
}

//! Stops the movie and frees its decoder and audio stream (the GL objects are kept for the next
//! one). Does not touch the game state.
void StopPlayback() {
    if (g_Video.decoder) {
        plm_destroy(g_Video.decoder);
        g_Video.decoder = nullptr;
    }
    if (g_Video.audio) {
        SDL_DestroyAudioStream(g_Video.audio); // Also closes the device it opened
        g_Video.audio = nullptr;
    }
    g_Video.audioSampleRate = 0;
    g_Video.framePending    = false;
    g_Video.nextFrameTime   = {};

    // The movie is drawn over the whole window, so the cursor was hidden while it played (as the
    // DirectShow video window did on Windows).
    SDL_ShowCursor();
}

void DestroyGlObjects() {
    if (!SDL_GL_GetCurrentContext()) {
        // There's no context left to delete objects in (engine already shut down); the driver
        // reclaims them together with the context.
        g_Video.program = g_Video.vao = g_Video.vbo = g_Video.texture = 0;
        return;
    }

    if (g_Video.texture) {
        glDeleteTextures(1, &g_Video.texture);
    }
    if (g_Video.vbo) {
        glDeleteBuffers(1, &g_Video.vbo);
    }
    if (g_Video.vao) {
        glDeleteVertexArrays(1, &g_Video.vao);
    }
    if (g_Video.program) {
        glDeleteProgram(g_Video.program);
    }
    g_Video.program         = 0;
    g_Video.vao             = 0;
    g_Video.vbo             = 0;
    g_Video.texture         = 0;
    g_Video.samplerLocation = -1;
    g_Video.textureWidth    = 0;
    g_Video.textureHeight   = 0;
}

//! `pl_mpeg` video callback: keeps the newest frame as RGBA pixels.
void OnVideoFrame(plm_t* decoder, plm_frame_t* frame, void* userdata) {
    UNUSED(decoder);
    UNUSED(userdata);

    const auto width  = static_cast<int32>(frame->width);
    const auto height = static_cast<int32>(frame->height);
    if (width <= 0 || height <= 0) {
        return;
    }

    if (g_Video.width != width || g_Video.height != height) {
        g_Video.width  = width;
        g_Video.height = height;
        // `plm_frame_to_rgba` never writes the alpha byte, so pre-filling with 0xFF keeps the
        // frame opaque.
        g_Video.pixels.assign(static_cast<size_t>(width) * height * 4, 0xFF);
    }

    plm_frame_to_rgba(frame, g_Video.pixels.data(), width * 4);
    g_Video.framePending = true;
}

//! `pl_mpeg` audio callback: pushes the decoded samples to the SDL stream.
void OnAudioSamples(plm_t* decoder, plm_samples_t* samples, void* userdata) {
    UNUSED(decoder);
    UNUSED(userdata);

    if (!g_Video.audio) {
        return;
    }

    const auto queuedLimit = static_cast<int32>(g_Video.audioSampleRate * MAX_QUEUED_AUDIO_SECONDS * 2 * sizeof(float));
    if (SDL_GetAudioStreamQueued(g_Video.audio) > queuedLimit) {
        SDL_ClearAudioStream(g_Video.audio);
    }

    SDL_PutAudioStreamData(g_Video.audio, samples->interleaved, static_cast<int32>(samples->count * 2 * sizeof(float)));
}

//! Decodes and presents the movie according to the wall clock, and detects its end.
void Pump() {
    if (!g_Video.decoder) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();

    // `OnGraphNotify` is called from the game's busy loop; wait for the next frame instead of
    // spinning a core for the whole title sequence.
    if (now < g_Video.nextFrameTime) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(g_Video.nextFrameTime - now);
        SDL_Delay(static_cast<uint32>(std::max<int64>(std::min(remaining, std::chrono::milliseconds(5)).count(), 1)));
        return;
    }

    plm_decode(g_Video.decoder, std::chrono::duration<double>(now - g_Video.lastDecode).count());
    g_Video.lastDecode = now;

    if (g_Video.framePending) {
        g_Video.framePending = false;
        if (!PresentFrame()) {
            NOTSA_LOG_ERR("Video: presenting the movie failed, skipping it");
            StopPlayback();
            s_VideoFinished = true;
            return;
        }
        if (const auto frameRate = plm_get_framerate(g_Video.decoder); frameRate > 0.0) {
            const auto frameDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(1.0 / frameRate)
            );
            g_Video.nextFrameTime = std::chrono::steady_clock::now() + frameDuration;
        }
    }

    if (plm_has_ended(g_Video.decoder)) {
        NOTSA_LOG_DEBUG("Video: playback finished");
        StopPlayback();
        s_VideoFinished = true; // `OnGraphNotify` performs the `EC_COMPLETE` transition
    }
}

} // namespace

void InjectHooks() {
    // No hooks in the native build (see plan B1.3).
}

//! 0x746740
void Release() {
    StopPlayback();
    DestroyGlObjects();
    s_VideoFinished = false;
}

//! 0x7467B0
void Shutdown() {
    Release();
}

//! 0x745F90
void WaitState(int32 state) {
    // There's no filter-graph state machine natively: the movie runs while a decoder is open,
    // and there's no paused state.
    switch (state) {
    case FILTER_STATE_RUNNING:
    case FILTER_STATE_PAUSED:
        return;
    case FILTER_STATE_STOPPED:
        // Blocks until the movie is over, like waiting for the DirectShow graph to stop.
        while (g_Video.decoder) {
            Pump();
        }
        return;
    default:
        return;
    }
}

//! 0x745FB0
void OnGraphNotify() {
    Pump();

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
    // The Windows backend repositions its DirectShow child window over the client rect. Natively
    // every frame is drawn over the whole drawable (see `PresentFrame`), so there's nothing to
    // position.
}

//! 0x747660
void Play(int32 nCmdShow, const char* path) {
    UNUSED(nCmdShow);

    Release(); // A movie is never played on top of another one

    if (!path || !*path) {
        s_VideoFinished = true;
        return;
    }

    if (!PSGLOBAL(sdlWindow)) {
        NOTSA_LOG_WARN("Video: there's no window yet, skipping `{}`", path);
        s_VideoFinished = true;
        return;
    }

    // The game passes Windows separators (`movies\\Logo.mpg`) and the retail files are
    // referenced in mixed case; `NormalizePath` resolves both.
    const auto fileName = notsa::native::NormalizePath(path);

    auto* const decoder = plm_create_with_filename(fileName.c_str());
    if (!decoder) {
        // Let the game continue as if the movie had already finished (`EC_COMPLETE`).
        NOTSA_LOG_WARN("Video: failed to open `{}`, skipping it", fileName);
        s_VideoFinished = true;
        return;
    }

    if (!plm_has_headers(decoder)) {
        NOTSA_LOG_WARN("Video: `{}` isn't a valid MPEG-PS file, skipping it", fileName);
        plm_destroy(decoder);
        s_VideoFinished = true;
        return;
    }

    g_Video.decoder = decoder;
    g_Video.lastDecode = std::chrono::steady_clock::now();
    plm_set_video_decode_callback(decoder, OnVideoFrame, nullptr);

    // Audio: a plain SDL stream. If the audio device can't be opened the movie plays silently.
    if (const auto sampleRate = plm_get_samplerate(decoder); plm_get_num_audio_streams(decoder) > 0 && sampleRate > 0) {
        const SDL_AudioSpec spec{ SDL_AUDIO_F32, 2, sampleRate };
        if (auto* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr)) {
            g_Video.audio           = stream;
            g_Video.audioSampleRate = sampleRate;

            plm_set_audio_lead_time(decoder, AUDIO_LEAD_TIME);
            plm_set_audio_decode_callback(decoder, OnAudioSamples, nullptr);

            SDL_ResumeAudioStreamDevice(stream);
        } else {
            NOTSA_LOG_WARN("Video: no audio output ({}), playing `{}` silently", SDL_GetError(), fileName);
        }
    }

    s_VideoFinished = false;

    // The movie covers the whole window; `StopPlayback` shows the cursor again.
    SDL_HideCursor();

    NOTSA_LOG_DEBUG(
        "Video: playing `{}` ({}x{}, {:.2f} fps, {:.2f}s)",
        fileName,
        plm_get_width(decoder),
        plm_get_height(decoder),
        plm_get_framerate(decoder),
        plm_get_duration(decoder)
    );
}

auto GetMediaControl() -> IMediaControl* {
    return nullptr; // No DirectShow graph natively
}

} // namespace VideoPlayer

#endif // NOTSA_NATIVE
