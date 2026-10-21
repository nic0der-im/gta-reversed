#include "StdInc.h"

#include "OALBase.h"

#ifdef NOTSA_NATIVE

OALBase::OALBase() {
    livingCount.fetch_add(1, std::memory_order_relaxed);
}

OALBase::~OALBase() {
    livingCount.fetch_sub(1, std::memory_order_relaxed);
}

void OALBase::AddRef() {
    m_refCount.fetch_add(1, std::memory_order_relaxed);
}

void OALBase::Release() {
    if (m_refCount.fetch_sub(1, std::memory_order_acq_rel) != 1) {
        return;
    }
    delete this;
}

bool OALCheckErrors(std::string_view call, std::string_view file, int32 line) {
    const auto error = alGetError();
    if (error == AL_NO_ERROR) {
        return true;
    }

    const auto message = [error] {
        switch (error) {
        case AL_INVALID_NAME:
            return "Invalid name(ID) passed to AL call.";
        case AL_INVALID_ENUM:
            return "Invalid enumeration passed to AL call.";
        case AL_INVALID_VALUE:
            return "Invalid value passed to AL call.";
        case AL_INVALID_OPERATION:
            return "Illegal AL call.";
        case AL_OUT_OF_MEMORY:
            return "Not enough memory to execute the AL call.";
        default:
            return "Unknown?";
        }
    }();

    NOTSA_LOG_ERR("OpenAL error at '{}' ({}:{})", message, file, line);
    NOTSA_LOG_ERR("  in call: {}", call);
    return false;
}

namespace {
ALCdevice*  s_device{};
ALCcontext* s_context{};

bool CheckALCErrors(std::string_view call, std::string_view file, int32 line) {
    if (const auto error = alcGetError(s_device); error == ALC_NO_ERROR) {
        return true;
    } else {
        NOTSA_LOG_ERR("OpenAL(ALC) error 0x{:X} in '{}' ({}:{})", static_cast<unsigned>(error), call, file, line);
        return false;
    }
}
#define OALC_CHECKED(call) (call, ::CheckALCErrors(#call, __FILE__, __LINE__))
} // namespace

namespace notsa::oal {

bool OpenDevice(uint32 sampleRate) {
    if (s_device) {
        return true;
    }

    s_device = alcOpenDevice(nullptr);
    if (!s_device) {
        NOTSA_LOG_ERR("Failed to open the default OpenAL device");
        return false;
    }

    const ALCint attributes[] = {
        ALC_FREQUENCY, static_cast<ALCint>(sampleRate),
        ALC_REFRESH,   60,
        0
    };
    s_context = alcCreateContext(s_device, attributes);
    if (!s_context) {
        OALC_CHECKED(alcCreateContext(s_device, attributes));
        alcCloseDevice(s_device);
        s_device = nullptr;
        return false;
    }

    if (!alcMakeContextCurrent(s_context)) {
        OALC_CHECKED(alcMakeContextCurrent(s_context));
        alcDestroyContext(s_context);
        alcCloseDevice(s_device);
        s_context = nullptr;
        s_device  = nullptr;
        return false;
    }

    NOTSA_LOG_INFO(
        "OpenAL: {} ({}), {}",
        alGetString(AL_RENDERER),
        alcGetString(s_device, ALC_DEVICE_SPECIFIER),
        alGetString(AL_VERSION)
    );
    return true;
}

void CloseDevice() {
    if (!s_device) {
        return;
    }

    OALC_CHECKED(alcMakeContextCurrent(nullptr));
    OALC_CHECKED(alcDestroyContext(s_context));
    s_context = nullptr;

    if (alcCloseDevice(s_device) == ALC_FALSE) {
        NOTSA_LOG_ERR("OpenAL: Failed to close the device");
    }
    s_device = nullptr;
}

bool IsDeviceOpen() {
    return s_device != nullptr;
}

ALCdevice* Device() {
    return s_device;
}

ALCcontext* Context() {
    return s_context;
}

bool MakeContextCurrent() {
    if (!s_context) {
        return false;
    }
    if (alcGetCurrentContext() == s_context) {
        return true;
    }
    return OALC_CHECKED(alcMakeContextCurrent(s_context));
}

void ClearCurrentContext() {
    if (s_device) {
        OALC_CHECKED(alcMakeContextCurrent(nullptr));
    } else {
        alcMakeContextCurrent(nullptr);
    }
}

bool InitListener() {
    if (!MakeContextCurrent()) {
        return false;
    }

    // Same setup as `CAEAudioHardware::InitDirectSoundListener`: the listener never moves
    // (the sound positions are already listener relative) and neither doppler nor
    // distance attenuation are applied - the game does its own.
    static constexpr ALfloat kOrientation[]{
        0.0f, 0.0f, -1.0f, // "At" (forward)
        0.0f, 1.0f,  0.0f  // "Up"
    };

    OAL_CHECKED(alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f));
    OAL_CHECKED(alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f));
    OAL_CHECKED(alListenerfv(AL_ORIENTATION, kOrientation));
    OAL_CHECKED(alListenerf(AL_GAIN, 1.0f));
    OAL_CHECKED(alDopplerFactor(0.0f));
    return OAL_CHECKED(alDistanceModel(AL_INVERSE_DISTANCE));
}

CVector ToListenerSpace(const CVector& v) {
    // See `CAEAudioEnvironment::GetPositionRelativeToCamera`: the game's audio frame uses
    // +X right, +Y forward and -Z up (that's the orientation handed to DirectSound3D).
    // OpenAL is right-handed with +Y up and -Z forward, hence (x, -z, -y).
    return {v.x, -v.z, -v.y};
}

const EFXFunctions* GetEFX() {
    static const EFXFunctions functions = [] {
        EFXFunctions fn{};
        if (!s_device || !alcIsExtensionPresent(s_device, "ALC_EXT_EFX")) {
            return fn;
        }

        const auto Load = [](const char* name) {
            return alGetProcAddress(name);
        };
        fn.GenEffects                = reinterpret_cast<decltype(fn.GenEffects)>(Load("alGenEffects"));
        fn.DeleteEffects             = reinterpret_cast<decltype(fn.DeleteEffects)>(Load("alDeleteEffects"));
        fn.Effecti                   = reinterpret_cast<decltype(fn.Effecti)>(Load("alEffecti"));
        fn.Effectf                   = reinterpret_cast<decltype(fn.Effectf)>(Load("alEffectf"));
        fn.GenAuxiliaryEffectSlots   = reinterpret_cast<decltype(fn.GenAuxiliaryEffectSlots)>(Load("alGenAuxiliaryEffectSlots"));
        fn.DeleteAuxiliaryEffectSlots = reinterpret_cast<decltype(fn.DeleteAuxiliaryEffectSlots)>(Load("alDeleteAuxiliaryEffectSlots"));
        fn.AuxiliaryEffectSloti      = reinterpret_cast<decltype(fn.AuxiliaryEffectSloti)>(Load("alAuxiliaryEffectSloti"));

        const bool complete = fn.GenEffects && fn.DeleteEffects && fn.Effecti && fn.Effectf
                           && fn.GenAuxiliaryEffectSlots && fn.DeleteAuxiliaryEffectSlots && fn.AuxiliaryEffectSloti;
        if (!complete) {
            NOTSA_LOG_WARN("ALC_EXT_EFX is advertised but its entry points could not be resolved");
            return EFXFunctions{};
        }
        return fn;
    }();

    return functions.GenEffects ? &functions : nullptr;
}

} // namespace notsa::oal

#endif // NOTSA_NATIVE
