#pragma once

//! @file OALTypes.h
//! @brief The per-channel audio buffer type.
//!
//! On Windows a channel owns a DirectSound secondary buffer (`IDirectSoundBuffer`); the native
//! build replaces it with an OpenAL voice (`OALSource`). Both expose the small API the channel
//! classes and the smooth fade thread use (`Play`, `Stop`, `SetVolumeDb`, `IsPlaying`, ...).

#ifdef NOTSA_NATIVE
#include "Audio/OpenAL/OALSource.h"
using tAudioBufferHandle = OALSource;
#else
struct IDirectSoundBuffer;
using tAudioBufferHandle = IDirectSoundBuffer;
#endif
