#include "StdInc.h"

#include "platform.h"

struct PresetView {
    CVector m_Translation;
    float m_RotX;
    float m_RotY;
    float m_NearClip;
    float m_FarClip;
    char* m_Description;
    PresetView* m_Next;
};

// 3x3 identity matrix
constexpr CVector PresetViewAxisX = {1.0f, 0.0f, 0.0f}; // 0x8D2E00
constexpr CVector PresetViewAxisY = {0.0f, 1.0f, 0.0f}; // 0x8D2E0C
constexpr CVector PresetViewAxisZ = {0.0f, 0.0f, 1.0f}; // 0x8D2E18

constexpr auto ViewsFileName = "./views.txt"; // 0x8D2E24;

auto& PresetViews = StaticRef<PresetView*>(0xC1707C);
auto& NumPresetViews = StaticRef<int32>(0xC17080);
auto& CurrentPresetView = StaticRef<int32>(0x8D2E30); // -1

// 0x619780
bool RsSetPresetView(RwCamera* camera, int32 viewNum) {
    if (!camera || !NumPresetViews || viewNum >= NumPresetViews || viewNum < 0)
        return false;

    auto* pv = PresetViews;
    int v3 = NumPresetViews - viewNum - 1u;
    for (CurrentPresetView = viewNum; v3 > 0; --v3) {
        if (!pv) {
            NOTSA_UNREACHABLE();
            break;
        }

        pv = pv->m_Next;
    }

    RwFrame* parent = RwCameraGetFrame(camera);
    RwFrameSetIdentity(parent);
    RwFrameRotate(parent, &PresetViewAxisX, -pv->m_RotX, rwCOMBINEREPLACE);
    RwFrameRotate(parent, &PresetViewAxisY, pv->m_RotY, rwCOMBINEPOSTCONCAT);
    RwFrameTranslate(parent, &pv->m_Translation, rwCOMBINEPOSTCONCAT);
    RwFrameUpdateObjects(parent);
    RwCameraSetNearClipPlane(camera, pv->m_NearClip);
    RwCameraSetFarClipPlane(camera, pv->m_FarClip);

    return true;
}

// 0x619840
void RsSetNextPresetView(RwCamera* camera) {
    if (!camera)
        return;

    if (!NumPresetViews)
        return;

    auto viewNum = ++CurrentPresetView;
    if (CurrentPresetView >= NumPresetViews) {
        viewNum = 0;
        CurrentPresetView = 0;
    }

    RsSetPresetView(camera, viewNum);
}

// 0x619880
void RsSetPreviousPresetView(RwCamera* camera) {
    if (!camera || !NumPresetViews)
        return;

    if (--CurrentPresetView < 0)
        CurrentPresetView = NumPresetViews - 1;

    RsSetPresetView(camera, CurrentPresetView);
}

// 0x6198C0
void RsDestroyPresetViews() {
    auto pv = PresetViews;
    if (PresetViews) {
        PresetView* m_Next;
        do {
            m_Next = pv->m_Next;
            if (pv->m_Description) {
                CMemoryMgr::Free(pv->m_Description);
            }
            CMemoryMgr::Free(pv);
            pv = m_Next;
        } while (m_Next);
    }
    PresetViews = nullptr;
    NumPresetViews = 0;
}

// 0x619AB0
void* RsGetPresetViewDescription() {
    if (!NumPresetViews || CurrentPresetView == -1) {
        return nullptr;
    }

    auto* pv = PresetViews;
    for (auto i = NumPresetViews - CurrentPresetView - 1; i > 0; --i) {
        if (!pv) {
            break;
        }
        pv = pv->m_Next;
    }
    if (!pv) {
        NOTSA_UNREACHABLE();
    }

    return pv->m_Description;
}

auto& PresetViewErrorCallbackEnabled = StaticRef<int32>(0xC1705C);
auto& PresetViewErrorCallback        = StaticRef<void (*)(int32, void*)>(0xC17060);

// 0x619D60
void RsLoadPresetViews() {
    RsDestroyPresetViews();
    CurrentPresetView = -1;

    auto* file = std::fopen(ViewsFileName, "r");
    if (!file) {
        return;
    }

    char line[1024];
    while (std::fgets(line, 1023, file)) {
        // Drop every non-printable character in place - this also removes the trailing newline,
        // so the name conversion of `sscanf` below always runs to the end of the buffer.
        char* dst = line;
        for (const char* src = line; *src; ++src) {
            if (std::isprint(static_cast<uint8>(*src))) {
                *dst++ = *src;
            }
        }
        *dst = '\0';

        float values[7];
        char  description[64];
        // NOTE: The executable's format literal is `"%f%f%f%f%f%f%f %[^"` followed by a NUL and a
        // stray `"]"`, i.e. the scanset runs past the end of the string literal. The established
        // behaviour is "seven floats and an optional name", which is what this format expresses
        // (the line no longer contains the newline at this point).
        const int numParsed = std::sscanf(
            line,
            "%f%f%f%f%f%f%f %[^\n]",
            &values[0], &values[1], &values[2], &values[3], &values[4], &values[5], &values[6],
            description
        );
        if (numParsed <= 6) {
            continue;
        }

        if (numParsed == 7) {
            std::sprintf(description, "%s%d", "View", NumPresetViews);
        }

        auto* view = static_cast<PresetView*>(CMemoryMgr::Malloc(sizeof(PresetView)));
        if (!view) {
            RsErrorMessage("Out of memory - not all preset views loaded");
            return;
        }

        view->m_Translation = CVector{ values[0], values[1], values[2] };
        view->m_RotX        = values[3];
        view->m_RotY        = values[4];
        view->m_NearClip    = values[5];
        view->m_FarClip     = values[6];

        view->m_Description = static_cast<char*>(CMemoryMgr::Malloc(std::strlen(description) + 1));
        if (!view->m_Description) {
            RsErrorMessage("Out of memory - not all preset views loaded");
            CMemoryMgr::Free(view); // Retail releases the node with `_free`; the matching deallocator here is `CMemoryMgr::Free` (see `RsDestroyPresetViews`)
            return;
        }
        std::strcpy(view->m_Description, description);

        NumPresetViews++;
        view->m_Next = PresetViews;
        PresetViews  = view;
    }

    std::fclose(file);
}

constexpr float PresetViewRadToDeg = 57.295776f; // 0x859878

// 0x619910
//! Reads the current view out of `camera`.
//! Retail passes the camera in `EAX`, the translation out-pointer in `EDI`, the pitch in a stack
//! slot and the yaw out-pointer in `EBX`; the register layout is folded into ordinary arguments.
static void RsGetCurrentView(RwCamera* camera, CVector& translation, float& rotX, float& rotY) {
    const RwMatrix* const matrix = RwFrameGetMatrix(RwCameraGetFrame(camera));

    translation = matrix->pos;

    // Pitch: the camera's forward axis projected onto the Y axis.
    const float atDotY = std::clamp(DotProduct(matrix->at, PresetViewAxisY), -1.0f, 1.0f);
    rotX = 90.0f - static_cast<float>(std::asin(atDotY)) * PresetViewRadToDeg;

    // Yaw: `at` projected onto the plane perpendicular to Y, then measured against the Z axis.
    CVector atOnPlane = matrix->at - PresetViewAxisY * atDotY;
    atOnPlane.Normalise();
    rotY = static_cast<float>(std::asin(std::clamp(DotProduct(atOnPlane, PresetViewAxisZ), -1.0f, 1.0f))) * PresetViewRadToDeg;
    if (DotProduct(atOnPlane, PresetViewAxisX) < 0.0f) {
        rotY = -rotY;
    }
}

// 0x619FA0
//! Appends the current view to `./views.txt`, reloads the list and selects the new entry.
//! Retail returns `int` and takes the camera as its only argument; `platform.h` declares it
//! `void` with no arguments, so the active scene camera is used and the result is dropped.
static int SavePresetView(RwCamera* camera) {
    auto* file = std::fopen(ViewsFileName, "a");
    if (!file) {
        if (PresetViewErrorCallbackEnabled) {
            int32 event = 0x420;
            PresetViewErrorCallback(0x1D, &event);
        }
        RsErrorMessage("Cannot open preset view file");
        return 0;
    }

    CVector translation;
    float   rotX, rotY;
    RsGetCurrentView(camera, translation, rotX, rotY);

    char line[1028];
    std::sprintf(
        line,
        "%0.6f %0.6f %0.6f  %0.6f %0.6f  %0.6f %0.6f %s%d\n",
        translation.x, translation.y, translation.z,
        rotX, rotY,
        RwCameraGetNearClipPlane(camera), RwCameraGetFarClipPlane(camera),
        "View", NumPresetViews
    );

    if (std::fputs(line, file) < 0) {
        RsErrorMessage("Cannot write to preset view file");
        std::fclose(file);
        return 0;
    }

    // Retail's file system writes straight through, so the reload below sees the new line while
    // the append handle is still open; `stdio` has to be flushed for that to hold.
    std::fflush(file);

    if (RsLoadPresetViews(), NumPresetViews) {
        RsSetPresetView(camera, NumPresetViews - 1);
        std::fclose(file);
        return 1;
    }

    std::fclose(file);
    return 0;
}

// 0x619FA0
void RsSavePresetView() {
    SavePresetView(Scene.m_pRwCamera);
}
