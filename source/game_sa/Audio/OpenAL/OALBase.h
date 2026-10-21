#pragma once

//! @file OALBase.h
//! @brief Base of the native OpenAL audio backend.
//!
//! The native build replaces DirectSound with OpenAL (Soft):
//!  * `notsa::oal::*` free functions manage the device/context and the fixed listener,
//!  * `OALBase` is the ref-counted base of the buffer/voice wrappers,
//!  * `OALBuffer` is a block of PCM data, `OALSource` is a voice (the native stand-in for
//!    a DirectSound secondary buffer).
//!
//! Everything here is only built for, and only included by, the native target.

#ifdef NOTSA_NATIVE

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>

#include <atomic>

#include "Vector.h"

//! Ref-counted base of every OpenAL object wrapper.
//! Instances are heap allocated and destroyed by `Release()` once the last reference is dropped.
class OALBase {
public:
    //! Number of living `OALBase` instances (diagnostics only).
    static inline std::atomic<uint32> livingCount{};

    OALBase();
    virtual ~OALBase();
    virtual void Release();
    virtual void AddRef();

private:
    std::atomic<uint32> m_refCount{1};
};

//! Reports (and clears) the error left by the previous OpenAL call.
//! Use it through the `OAL_CHECKED` macro so the failed call is part of the message.
bool OALCheckErrors(std::string_view call, std::string_view file, int32 line);
#define OAL_CHECKED(call) (call, ::OALCheckErrors(#call, __FILE__, __LINE__))

namespace notsa::oal {

//! Opens the default OpenAL device (48 kHz stereo unless told otherwise) and creates a context.
bool OpenDevice(uint32 sampleRate = 48'000);
//! Destroys the context and closes the device. Every OpenAL object must be deleted before.
void CloseDevice();
bool IsDeviceOpen();

ALCdevice*  Device();
ALCcontext* Context();

//! Makes the audio context current for the calling thread.
//! OpenAL's current context is per-thread: every thread that issues AL calls
//! (the smooth fade and the streaming threads) has to call this once.
bool MakeContextCurrent();
void ClearCurrentContext();

//! Sets up the listener exactly like the DirectSound build does: fixed at the origin,
//! not attenuated by distance (the game does its own distance attenuation) and without doppler.
bool InitListener();

//! Converts a vector expressed in the game's (DirectSound) listener frame - which is what
//! `CAEAudioEnvironment::GetPositionRelativeToCamera` produces - into OpenAL's right-handed,
//! Y-up listener frame.
CVector ToListenerSpace(const CVector& v);

//! EFX entry points (`ALC_EXT_EFX`), resolved through `alGetProcAddress`.
struct EFXFunctions {
    void (*GenEffects)(ALsizei, ALuint*);
    void (*DeleteEffects)(ALsizei, const ALuint*);
    void (*Effecti)(ALuint, ALenum, ALint);
    void (*Effectf)(ALuint, ALenum, ALfloat);
    void (*GenAuxiliaryEffectSlots)(ALsizei, ALuint*);
    void (*DeleteAuxiliaryEffectSlots)(ALsizei, const ALuint*);
    void (*AuxiliaryEffectSloti)(ALuint, ALenum, ALint);
};

//! Returns the EFX entry points, or `nullptr` when the device has no EFX support.
const EFXFunctions* GetEFX();

} // namespace notsa::oal

#endif // NOTSA_NATIVE
