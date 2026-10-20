#pragma once

//! @file audio_types.h
//! @brief Minimal stand-ins for the Windows audio types used by `source/game_sa/Audio`.
//!
//! The native build has no DirectSound, no `mmreg.h` and no COM. The audio headers still need
//! the *shapes* of a few of those types to keep their class layouts compiling, so this header
//! provides POD equivalents of the handful of structures they store, and opaque declarations
//! of the interfaces they only ever hold pointers to.
//!
//! This file is only included from the audio headers under `NOTSA_NATIVE`; the real Windows
//! SDK declarations are used everywhere else.

#include <cstdint>

namespace notsa::native {
//! Opaque DirectSound/COM interfaces. Nothing in the native build dereferences them.
struct IDirectSound;
struct IDirectSound8;
struct IDirectSoundBuffer;
struct IDirectSoundBuffer8;
struct IDirectSound3DBuffer;
struct IDirectSound3DListener;
} // namespace notsa::native

#ifdef NOTSA_NATIVE
using notsa::native::IDirectSound;
using notsa::native::IDirectSound8;
using notsa::native::IDirectSoundBuffer;
using notsa::native::IDirectSoundBuffer8;
using notsa::native::IDirectSound3DBuffer;
using notsa::native::IDirectSound3DListener;

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

//! Equivalent of the Windows SDK's `WAVEFORMATEX` (18 bytes).
//! `mmreg.h` includes `pshpack1.h`, so the struct has alignment 1 (18, not 20 bytes).
#pragma pack(push, 1)
struct WAVEFORMATEX {
    uint16 wFormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;
    uint16 cbSize;
};
static_assert(sizeof(WAVEFORMATEX) == 18);

//! Equivalent of the Windows SDK's `WAVEFORMAT` (16 bytes, i.e. `WAVEFORMATEX` without `cbSize`).
struct WAVEFORMAT {
    uint16 wFormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;
};
static_assert(sizeof(WAVEFORMAT) == 16);
#pragma pack(pop)

//! Equivalent of the Windows SDK's `DSCAPS` (96 bytes). Only used as storage by the silent backend.
struct DSCAPS {
    uint32 dwSize;
    uint32 dwFlags;
    uint32 dwMinSecondarySampleRate;
    uint32 dwMaxSecondarySampleRate;
    uint32 dwPrimaryBuffers;
    uint32 dwMaxHwMixingAllBuffers;
    uint32 dwMaxHwMixingStaticBuffers;
    uint32 dwMaxHwMixingStreamingBuffers;
    uint32 dwFreeHwMixingAllBuffers;
    uint32 dwFreeHwMixingStaticBuffers;
    uint32 dwFreeHwMixingStreamingBuffers;
    uint32 dwMaxHw3DAllBuffers;
    uint32 dwMaxHw3DStaticBuffers;
    uint32 dwMaxHw3DStreamingBuffers;
    uint32 dwFreeHw3DAllBuffers;
    uint32 dwFreeHw3DStaticBuffers;
    uint32 dwFreeHw3DStreamingBuffers;
    uint32 dwTotalHwMemBytes;
    uint32 dwFreeHwMemBytes;
    uint32 dwMaxContigFreeHwMemBytes;
    uint32 dwUnlockTransferRateHwBuffers;
    uint32 dwPlayCpuOverheadSwBuffers;
    uint32 dwReserved1;
    uint32 dwReserved2;
};
static_assert(sizeof(DSCAPS) == 96);
#endif
