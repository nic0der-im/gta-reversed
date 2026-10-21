#include "StdInc.h"

#include "AEStaticChannel.h"

#include "AESmoothFadeThread.h"

#ifdef NOTSA_NATIVE
#include "Audio/OpenAL/OALBuffer.h"
#endif

void CAEStaticChannel::InjectHooks() {
    RH_ScopedVirtualClass(CAEStaticChannel, 0x85F3CC, 9);
    RH_ScopedCategory("Audio/Hardware");

    RH_ScopedVMTInstall(Service, 0x4F10D0);
    RH_ScopedVMTInstall(IsSoundPlaying, 0x4F0F40);
    RH_ScopedVMTInstall(GetPlayTime, 0x4F0F70);
    RH_ScopedVMTInstall(GetLength, 0x4F0FA0);
    RH_ScopedVMTInstall(Play, 0x4F0BD0);
    RH_ScopedVMTInstall(SynchPlayback, 0x4F1040);
    RH_ScopedVMTInstall(Stop, 0x4F0FB0);

    RH_ScopedInstall(SetAudioBuffer, 0x4F0C40, {.Reversed = false});
}

CAEStaticChannel::CAEStaticChannel(IDirectSound* pDirectSound, uint16 channelId, bool hardwareMixAvailable, uint32 samplesPerSec, uint16 bitsPerSample) :
    CAEAudioChannel(pDirectSound, channelId, samplesPerSec, bitsPerSample),
    m_bNeedData(false),
    m_bNeedsSynch(false),
    m_IsHardwareMixAvailable(hardwareMixAvailable)
{
#ifdef NOTSA_NATIVE
    // The channel is heap allocated, so unlike the DirectSound path all of these are already
    // defined - make sure they're explicitly initialised anyway.
    m_pBuffer              = nullptr;
    m_bUnkn2               = false;
    m_bUnkn4               = false;
    m_nCurrentBufferOffset = 0;
    field_68               = 0;
    field_6C               = 0;
    m_nSyncTime            = 0;
    field_74               = 0;
    m_dwLockOffset         = 0;
    m_nNumLockBytes        = 0;
    m_nNumLoops            = 0;
    field_82               = 0;
    field_88               = 0;
    field_8B               = 0;
    field_8C               = 0;
#endif
}

// 0x4F10D0
void CAEStaticChannel::Service() {
#ifdef NOTSA_NATIVE
    if (!m_pDirectSoundBuffer) {
        m_nBufferStatus = 0;
        return;
    }

    // NOTE: In DirectSound the looping tail of the buffer had to be re-copied into the ring
    // buffer whenever the write cursor entered it (`m_bNeedData`). An OpenAL buffer is static
    // data, so there's no such refill here.

    UpdateStatus();

    if (!m_bPaused && !IsBufferPlaying()) {
        --g_numSoundChannelsUsed;
        std::exchange(m_pDirectSoundBuffer, nullptr)->Release();
    }
#else
    if (!m_pDirectSoundBuffer) {
        m_nBufferStatus = 0;
        return;
    }

    if (m_bNeedData && (int32)(CTimer::GetTimeInMS() - m_nSyncTime) > field_74) {
        uint8* ppvAudioPtr1{};
        DWORD pdwAudioBytes{};

        VERIFY(SUCCEEDED(m_pDirectSoundBuffer->Lock(
            m_dwLockOffset,
            m_nNumLockBytes,
            reinterpret_cast<LPVOID*>(&ppvAudioPtr1),
            &pdwAudioBytes,
            nullptr,
            0,
            0
        )));

        for (auto i = 0u; i < m_nNumLoops; i++) {
            memcpy(
                &ppvAudioPtr1[i * m_nNumLockBytes],
                (uint8*)m_pBuffer + m_nCurrentBufferOffset,
                m_nNumLockBytes
            );
        }
        VERIFY(SUCCEEDED(m_pDirectSoundBuffer->Unlock(ppvAudioPtr1, pdwAudioBytes, nullptr, 0)));
        m_bNeedData = false;
    }

    UpdateStatus();

    if (!m_bPaused && !bufferStatus.Bit0x1) {
        if (const auto buf = std::exchange(m_pDirectSoundBuffer, nullptr)) {
            --g_numSoundChannelsUsed;
            buf->Release();
        }
    }
#endif // NOTSA_NATIVE
}

// 0x4F0F40
bool CAEStaticChannel::IsSoundPlaying() {
    if (!m_pDirectSoundBuffer)
        return false;

    if (m_bPaused || m_bNeedsSynch)
        return true;

    return CAEAudioChannel::IsBufferPlaying();
}

// 0x4F0F70
int16 CAEStaticChannel::GetPlayTime() {
    if (!IsSoundPlaying())
        return -1;

    const auto curPos = CAEAudioChannel::GetCurrentPlaybackPosition();
    return CAEAudioChannel::ConvertFromBytesToMS(curPos);
}

// 0x4F0FA0
uint16 CAEStaticChannel::GetLength() {
    return CAEAudioChannel::ConvertFromBytesToMS(m_nLengthInBytes);
}

// 0x4F0BD0
void CAEStaticChannel::Play(int16 timeInMs, int8 unused, float scalingFactor) {
#ifdef NOTSA_NATIVE
    if (!m_pDirectSoundBuffer)
        return;

    if (m_bLooped && m_nCurrentBufferOffset != 0 || !timeInMs) {
        m_bUnkn2 = false;
    } else {
        m_pDirectSoundBuffer->SetPlaybackByteOffset(ConvertFromMsToBytes(timeInMs));
        m_bUnkn2 = true;
    }
    m_bNeedsSynch = true;
    m_bPaused = scalingFactor == 0.0f;
#else
    if (m_bLooped && m_nCurrentBufferOffset != 0 || !timeInMs) {
        m_bUnkn2 = false;
    } else {
        m_pDirectSoundBuffer->SetCurrentPosition(ConvertFromMsToBytes(timeInMs));
        m_bUnkn2 = true;
    }
    m_bNeedsSynch = true;
    m_bPaused = scalingFactor == 0.0f;
#endif
}

// 0x4F1040
void CAEStaticChannel::SynchPlayback() {
#ifdef NOTSA_NATIVE
    if (!m_pDirectSoundBuffer || !m_bNeedsSynch || m_bPaused)
        return;

    if (m_bUnkn2) {
        m_pDirectSoundBuffer->SetVolumeDb(-100.0F);
        if (!AESmoothFadeThread.RequestFade(m_pDirectSoundBuffer, m_Volume, -2, false)) {
            m_pDirectSoundBuffer->SetVolumeDb(m_Volume);
        }
    }

    m_pDirectSoundBuffer->Play(m_bLooped);
    m_nSyncTime = CTimer::GetTimeInMS();
    m_bNeedsSynch = false;
#else
    if (!m_pDirectSoundBuffer || !m_bNeedsSynch || m_bPaused)
        return;

    if (m_bUnkn2) {
        m_pDirectSoundBuffer->SetVolume(-10000);
        if (!AESmoothFadeThread.RequestFade(m_pDirectSoundBuffer, m_Volume, -2, false)) {
            const auto dwVolume = static_cast<LONG>(m_Volume * 100.0F);
            m_pDirectSoundBuffer->SetVolume(dwVolume);
        }
    }

    m_pDirectSoundBuffer->Play(0, 0, m_bLooped);
    m_nSyncTime = CTimer::GetTimeInMS();
    m_bNeedsSynch = false;
#endif // NOTSA_NATIVE
}

// 0x4F0FB0
void CAEStaticChannel::Stop() {
#ifdef NOTSA_NATIVE
    if (m_pDirectSoundBuffer &&
        CAEAudioChannel::IsBufferPlaying() &&
        !AESmoothFadeThread.RequestFade(m_pDirectSoundBuffer, -100.0F, -1, true)
    ) {
        m_pDirectSoundBuffer->Stop();
    }

    if (m_pDirectSoundBuffer) {
        --g_numSoundChannelsUsed;
        std::exchange(m_pDirectSoundBuffer, nullptr)->Release();
    }
#else
    if (m_pDirectSoundBuffer &&
        CAEAudioChannel::IsBufferPlaying() &&
        !AESmoothFadeThread.RequestFade(m_pDirectSoundBuffer, -100.0F, -1, true)
    ) {
        m_pDirectSoundBuffer->Stop();
    }

    { // todo: Same as CAEAudioChannel::~CAEAudioChannel
    if (m_pDirectSoundBuffer) {
        --g_numSoundChannelsUsed;
        m_pDirectSoundBuffer->Release();
        m_pDirectSoundBuffer = nullptr;
    }

    if (m_pDirectSound3DBuffer) {
        m_pDirectSound3DBuffer->Release();
        m_pDirectSound3DBuffer = nullptr;
    }
    }
#endif // NOTSA_NATIVE
}

// 0x4F0C40
bool CAEStaticChannel::SetAudioBuffer(IDirectSound3DBuffer* buffer, uint32 size, int16 f88, int16 f8c, int16 loopOffset, uint16 frequency) {
#ifdef NOTSA_NATIVE
    // `buffer` is the raw PCM data (16 bit mono) of the sound - the DirectSound signature (and
    // the header) call it a 3D buffer, but 0x4F0C40 copies it straight into the ring buffer.
    if (!buffer || !size || !frequency) {
        return false;
    }

    // Release whatever was playing (usually already released by `Service`)
    if (m_pDirectSoundBuffer) {
        --g_numSoundChannelsUsed;
        std::exchange(m_pDirectSoundBuffer, nullptr)->Release();
    }

    m_pBuffer              = buffer;
    m_nCurrentBufferOffset = 0;
    field_68               = 0;
    m_bLooped              = false;
    m_bNeedData            = false;
    m_bPaused              = false;
    field_6C               = 0;
    m_nLengthInBytes       = size;
    field_88               = f88;
    field_8C               = f8c;

    // (As in 0x4F0C40) A loop offset other than -1 marks the sound as looping:
    // `[loopOffset, size)` is the loop body, `[0, loopOffset)` is the intro.
    uint32 loopStart = 0;
    if (loopOffset != -1) {
        m_bLooped              = true;
        m_nCurrentBufferOffset = static_cast<uint32>(loopOffset) * 2; // Sample -> byte offset (16 bit mono)
        loopStart              = m_nCurrentBufferOffset;
        field_68               = size;
    }

    m_WaveFormat.wFormatTag      = WAVE_FORMAT_PCM;
    m_WaveFormat.nChannels       = 1;
    m_WaveFormat.nBlockAlign     = 2;
    m_WaveFormat.wBitsPerSample  = 16;
    m_WaveFormat.cbSize          = 0;
    m_WaveFormat.nSamplesPerSec  = frequency;
    m_WaveFormat.nAvgBytesPerSec = frequency * 2;

    m_nFrequency         = frequency;
    m_nOriginalFrequency = frequency;

    const auto voice  = new OALSource{};
    const auto sample = new OALBuffer{m_pBuffer, size, frequency, 1};

    // NOTE: `SetBuffer` takes a reference
    const bool attached = voice->SetBuffer(sample);
    sample->Release();
    if (!attached) {
        voice->Release();
        return false;
    }

    voice->SetLoopPoints(loopStart / 2, size / 2);
    voice->SetVolumeDb(-100.0F);
    voice->SetFrequency(frequency);

    // The original starts the buffer right away too (0x4F0C40 does `Play(0, 0, 0)`) -
    // it plays silently until `SynchPlayback` sets the volume.
    voice->Play(false);

    m_pDirectSoundBuffer = voice;
    m_Volume             = -100.0F;
    ++g_numSoundChannelsUsed;

    return true;
#else
    return false;
#endif // NOTSA_NATIVE
}
