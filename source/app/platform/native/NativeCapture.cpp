#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "app/platform/native/NativeCapture.h"

#include <SDL3/SDL.h>
#include <src/gl/glad/glad.h>

#include <cstdlib>
#include <cstring>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations" // stb's HDR writer still uses `sprintf`
#include "app/platform/native/third_party/stb_image_write.h"
#pragma clang diagnostic pop

namespace {

constexpr int32 kDefaultCaptureFrame = 90;

//! GL reads rows bottom-up, PNG expects them top-down.
void FlipVerticallyInPlace(uint8* pixels, int32 height, size_t stride) {
    std::vector<uint8> row(stride);
    for (int32 y = 0; y < height / 2; y++) {
        uint8* top    = pixels + (size_t)y * stride;
        uint8* bottom = pixels + (size_t)(height - 1 - y) * stride;
        std::memcpy(row.data(), top, stride);
        std::memcpy(top, bottom, stride);
        std::memcpy(bottom, row.data(), stride);
    }
}

} // namespace

namespace notsa::native {

void CaptureOnPresent() {
    static const char* path = std::getenv("GTASA_CAPTURE");
    static bool        done = false;
    if (!path || done) {
        return;
    }

    static int32 frame       = 0;
    static int32 targetFrame = [] {
        const char* value  = std::getenv("GTASA_CAPTURE_FRAME");
        const int32 parsed = value ? std::atoi(value) : 0;
        return parsed > 0 ? parsed : kDefaultCaptureFrame;
    }();

    if (++frame < targetFrame) {
        return;
    }
    done = true;

    auto* window = RsGlobal.ps ? RsGlobal.ps->sdlWindow : nullptr;
    int32 width = 0, height = 0;
    if (!window || !SDL_GetWindowSizeInPixels(window, &width, &height) || width <= 0 || height <= 0) {
        NOTSA_LOG_ERR("Capture: there is no window to read the frame from");
        RsGlobal.quit = TRUE;
        return;
    }

    // The presented image lives in the default framebuffer; librw may have left a render target
    // and a non-default pack alignment behind.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    const size_t       stride = (size_t)width * 4;
    std::vector<uint8> pixels((size_t)height * stride);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    FlipVerticallyInPlace(pixels.data(), height, stride);

    if (stbi_write_png(path, width, height, 4, pixels.data(), (int)stride)) {
        NOTSA_LOG_INFO("Capture: wrote {}x{} frame {} to '{}'", width, height, frame, path);
    } else {
        NOTSA_LOG_ERR("Capture: failed to write '{}'", path);
    }
    RsGlobal.quit = TRUE;
}

} // namespace notsa::native

#endif // NOTSA_NATIVE
