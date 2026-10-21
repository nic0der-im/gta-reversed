#include "StdInc.h"

#include <extensions/utility.hpp>

#include "AEStreamingChannel.h"
#include "AEAudioUtility.h"

#ifdef NOTSA_NATIVE
#include "Audio/OpenAL/OALBase.h"
#include "Audio/OpenAL/OALBuffer.h"
#include "Audio/OpenAL/OALSource.h"
#endif

#ifndef NOTSA_NATIVE
//! Stereo channel FX Param EQ presets [0x8CBA70]
static inline DSFXParamEq s_FXParamEqPresets[(+eBassSetting::NUM)][2]{
    {{ 0.0f, 0.0f,  0.0f }, { 0.f,   0.0f,  0.0f }}, // Preset NORMAL [Unused]
    {{ 80.f, 30.f,  4.0f }, { 180.f, 30.f,  1.5f }}, // Preset BOOST
    {{ 80.f, 36.f, -15.f }, { 80.f,  36.f, -15.f }}, // Preset CUT
};
#else
//! One `DSFXParamEq` band (0x8CBA70), to be mapped onto `AL_EFFECT_EQUALIZER`
struct tEQBand {
    float m_fCenter;    //!< In Hz
    float m_fBandwidth; //!< In 1/100 octaves (AL wants octaves)
    float m_fGain;      //!< In dB
};

//! Stereo channel FX Param EQ presets [0x8CBA70]
static constexpr tEQBand s_EQPresets[(+eBassSetting::NUM)][2]{
    {{  0.0f,  0.0f,   0.0f }, {   0.0f,  0.0f,  0.0f }}, // Preset NORMAL [Unused]
    {{ 80.0f, 30.0f,   4.0f }, { 180.0f, 30.0f,  1.5f }}, // Preset BOOST
    {{ 80.0f, 36.0f, -15.0f }, {  80.0f, 36.0f, -15.0f }}, // Preset CUT
};

namespace {
//! Size of one decoded chunk (0x6000 bytes ≈ 128ms of 48kHz stereo 16 bit audio)
constexpr uint32 STREAM_CHUNK_BYTES = 0x6000;
//! Number of chunks kept queued (≈ 512ms of buffered audio)
constexpr uint32 STREAM_CHUNK_COUNT = 4;
} // namespace
#endif // NOTSA_NATIVE

// 0x4F2200
CAEStreamingChannel::~CAEStreamingChannel() {
    DirectSoundBufferFadeToSilence();

    m_nState              = StreamingChannelState::Stopping;
    m_lStoppingFrameCount = 0;

#ifdef NOTSA_NATIVE
    if (m_bEQEnabled) {
        RemoveFX();
    }

    if (m_pDirectSoundBuffer) {
        m_pDirectSoundBuffer->ClearQueue();
        m_pDirectSoundBuffer->Stop();
    }
#else
    if (m_pDirectSoundBuffer8) {
        m_pDirectSoundBuffer8->SetFX(0, nullptr, nullptr);
    }

    if (m_pSilenceBuffer) {
        std::exchange(m_pSilenceBuffer, nullptr)->Release();
    }

    if (m_pDirectSoundBuffer) {
        std::exchange(m_pDirectSoundBuffer, nullptr)->Release();
    }
#endif

    if (m_pStreamingDecoder) {
        delete std::exchange(m_pStreamingDecoder, nullptr);
    }

    if (m_pNextStreamingDecoder) {
        delete std::exchange(m_pNextStreamingDecoder, nullptr);
    }
}

// 0x4F22F0
void CAEStreamingChannel::Initialise() {
#ifdef NOTSA_NATIVE
    m_WaveFormat.wFormatTag      = WAVE_FORMAT_PCM;
    m_WaveFormat.cbSize          = 0;
    m_WaveFormat.nChannels       = 2;
    m_WaveFormat.wBitsPerSample  = 16;
    m_WaveFormat.nSamplesPerSec  = 48000;
    m_WaveFormat.nAvgBytesPerSec = 192000;
    m_WaveFormat.nBlockAlign     = 4;

    // The OpenAL voice the stream is played through
    if (!m_pDirectSoundBuffer) {
        m_pDirectSoundBuffer = new OALSource{};
    }

    m_bInitialized = true;
    SetOriginalFrequency(m_WaveFormat.nSamplesPerSec);
    m_pBuffer  = m_aBuffer;
    m_bLooped  = true;

    InitialiseSilence();
#else
    VERIFY(SUCCEEDED(CoInitialize(nullptr)));

    DSBUFFERDESC bufferDesc{};
    bufferDesc.guid3DAlgorithm = GUID_NULL;
    bufferDesc.lpwfxFormat = &m_WaveFormat;
    bufferDesc.dwSize = 36;
    bufferDesc.dwBufferBytes = 0xC0000;
    bufferDesc.dwReserved = 0;
    bufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLFX | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;

    m_WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    m_WaveFormat.cbSize = 0;
    m_WaveFormat.nChannels = 2;
    m_WaveFormat.wBitsPerSample = 16;
    m_WaveFormat.nSamplesPerSec = 48000;
    m_WaveFormat.nAvgBytesPerSec = 192000;
    m_WaveFormat.nBlockAlign = 4;

    if (SUCCEEDED(m_pDirectSound->CreateSoundBuffer(
        &bufferDesc,
        &m_pDirectSoundBuffer,
        0
    ))) {
        m_bInitialized = true;
        SetOriginalFrequency(m_WaveFormat.nSamplesPerSec);
        m_pBuffer = m_aBuffer;
        m_bLooped = true;
    }
    InitialiseSilence();
#endif
}

// 0x4F1C70
void CAEStreamingChannel::InitialiseSilence() {
#ifdef NOTSA_NATIVE
    // DirectSound needed a second, silently playing buffer to keep the (hardware mixed)
    // primary buffer from starving. OpenAL simply plays nothing until buffers are queued,
    // so there's nothing to set up here.
#else
    DSBUFFERDESC bufferDesc{};
    bufferDesc.dwSize = 36;
    bufferDesc.lpwfxFormat = &m_WaveFormat;
    bufferDesc.dwBufferBytes = 0x8000;
    bufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE;

    if (SUCCEEDED(m_pDirectSound->CreateSoundBuffer(&bufferDesc, &m_pSilenceBuffer, 0))) {
        void *audioPtr;
        DWORD audioPtrBytes;

        m_pSilenceBuffer->Lock(0, 0, &audioPtr, &audioPtrBytes, nullptr, 0, DSBLOCK_ENTIREBUFFER);
        memset(audioPtr, 0, audioPtrBytes);
        m_pSilenceBuffer->Unlock(audioPtr, audioPtrBytes, nullptr, 0);
        m_pSilenceBuffer->Play(0, 0, DSBPLAY_LOOPING);

    } else {
        NOTSA_LOG_WARN("Creating silence buffer failed.");
    }
#endif
}

// 0x4F1FF0
void CAEStreamingChannel::SetReady() {
    switch (m_nState) {
    case StreamingChannelState::Stopping:
        if (m_pDirectSoundBuffer) {
            AESmoothFadeThread.CancelFade(m_pDirectSoundBuffer);
            m_pDirectSoundBuffer->Stop();
        }
        m_nState = StreamingChannelState::Stopped;

        break;
    case StreamingChannelState::Stopped:
        if (m_pStreamingDecoder)
            m_nState = StreamingChannelState::UNK_MINUS_2;
        break;
    }
}

// 0x4F1F30
void CAEStreamingChannel::SetBassEQ(eBassSetting mode, float gain) {
#ifdef NOTSA_NATIVE
    if (mode == eBassSetting::NORMAL) {
        if (m_bEQEnabled) {
            RemoveFX();
        }
        return;
    }

    if (!m_bEQEnabled && !AddFX()) {
        return;
    }

    const auto  efx    = notsa::oal::GetEFX();
    const auto& preset = s_EQPresets[+mode];
    if (!efx) {
        return;
    }

    // `DSFXParamEq` is a single peaking EQ per channel - `AL_EFFECT_EQUALIZER` has a low shelf
    // and two peaking bands, which is where the two presets are mapped onto.
    switch (mode) {
    case eBassSetting::BOOST:
        efx->Effectf(m_effectId, AL_EQUALIZER_LOW_CUTOFF, preset[0].m_fCenter);
        efx->Effectf(m_effectId, AL_EQUALIZER_LOW_GAIN, preset[0].m_fGain * gain);
        efx->Effectf(m_effectId, AL_EQUALIZER_MID1_CENTER, preset[1].m_fCenter);
        efx->Effectf(m_effectId, AL_EQUALIZER_MID1_WIDTH, preset[1].m_fBandwidth / 100.0f);
        efx->Effectf(m_effectId, AL_EQUALIZER_MID1_GAIN, preset[1].m_fGain * gain);
        break;
    case eBassSetting::CUT:
        efx->Effectf(m_effectId, AL_EQUALIZER_LOW_CUTOFF, preset[0].m_fCenter);
        efx->Effectf(m_effectId, AL_EQUALIZER_LOW_GAIN, preset[0].m_fGain * gain);
        break;
    default:
        break;
    }

    // The slot has to be told about the changed parameters again
    efx->AuxiliaryEffectSloti(m_effectSlot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(m_effectId));
#else
    if (mode == eBassSetting::NORMAL) {
        if (m_bEQEnabled) {
            RemoveFX();
        }
        return;
    }

    if (!m_bEQEnabled && !AddFX()) {
        return;
    }

    for (auto ch = 0; ch < 2; ch++) {
        IDirectSoundFXParamEq* fxEqParam;
        if (FAILED(m_pDirectSoundBuffer8->GetObjectInPath(
            GUID_All_Objects,
            ch,
            IID_IDirectSoundFXParamEq,
            reinterpret_cast<LPVOID*>(&fxEqParam)
        ))) {
            continue;
        }

        DSFXParamEq savedFxExParam{ s_FXParamEqPresets[(+mode)][ch] };
        savedFxExParam.fGain *= gain;
        VERIFY(SUCCEEDED(fxEqParam->SetAllParameters(&savedFxExParam)));
        fxEqParam->Release();
    }
#endif // NOTSA_NATIVE
}

// 0x4F2060
void CAEStreamingChannel::SetFrequencyScalingFactor(float factor) {
#ifdef NOTSA_NATIVE
    if (factor == 0.0f) {
        if (!m_pDirectSoundBuffer || m_nState == StreamingChannelState::Paused || !IsBufferPlaying())
            return;

        DirectSoundBufferFadeToSilence();
        m_nState = StreamingChannelState::Paused;
    } else {
        SetFrequency(static_cast<uint32>((float)m_nOriginalFrequency * factor));

        if (m_nState != StreamingChannelState::Paused)
            return;

        if (!m_pDirectSoundBuffer)
            return;

        m_pDirectSoundBuffer->SetVolumeDb(-100.0f);
        m_pDirectSoundBuffer->Play(false);

        if (!AESmoothFadeThread.RequestFade(m_pDirectSoundBuffer, m_Volume, 35, true))
            m_pDirectSoundBuffer->SetVolumeDb(m_Volume);

        m_nState = StreamingChannelState::Started;
    }
#else
    if (factor == 0.0f) {
        if (!m_pDirectSoundBuffer || m_nState == StreamingChannelState::Paused || !IsBufferPlaying())
            return;

        DirectSoundBufferFadeToSilence();
        m_nState = StreamingChannelState::Paused;
    } else {
        SetFrequency(static_cast<uint32>((float)m_nOriginalFrequency * factor));

        if (m_nState != StreamingChannelState::Paused)
            return;

        if (!m_pDirectSoundBuffer)
            return;

        m_pDirectSoundBuffer->SetVolume(-10'000);
        m_pDirectSoundBuffer->Play(0, 0, m_bLooped ? DSBPLAY_LOOPING : 0);

        if (!AESmoothFadeThread.RequestFade(m_pDirectSoundBuffer, m_Volume, 35, true))
            m_pDirectSoundBuffer->SetVolume(static_cast<int32>(m_Volume * 100.0f));

        m_nState = StreamingChannelState::Started;
    }
#endif // NOTSA_NATIVE
}

// 0x4F1870
void CAEStreamingChannel::SynchPlayback() {
    // NOP
}

// 0x4F1E20
uint32 CAEStreamingChannel::FillBuffer(void* buffer, uint32 size) {
    auto filled = m_pStreamingDecoder->FillBuffer(buffer, size);
    if (filled < size) {
        if (m_bLoopTrack) {
            while (filled < size) {
                m_pStreamingDecoder->SetCursor(1);
                filled += m_pStreamingDecoder->FillBuffer((uint8*)buffer + filled, size - filled);
            }
        } else {
            if (m_pNextStreamingDecoder) {
                if (m_pNextStreamingDecoder->GetSampleRate() == m_pStreamingDecoder->GetSampleRate()) {
                    if (m_pStreamingDecoder)
                        delete m_pStreamingDecoder;

                    m_pStreamingDecoder = m_pNextStreamingDecoder;
                    m_pNextStreamingDecoder = nullptr;

                    filled += m_pStreamingDecoder->FillBuffer((uint8*)buffer + filled, size - filled);
                } else {
                    memset((uint8*)buffer + filled, 0, size - filled);
                    filled = size;
                    m_bSilenced = true;
                }
            } else {
                m_nState = StreamingChannelState::Finished;
            }
        }
    }

    m_nStreamPlayTimeMs = m_pStreamingDecoder->GetStreamLengthMs();
    return filled;
}

// 0x4F23D0, broken af
void CAEStreamingChannel::PrepareStream(CAEStreamingDecoder* newDecoder, int8 soundFlags, uint32 audioBytes) {
#ifdef NOTSA_NATIVE
    if (!newDecoder)
        return;
#else
    if (!newDecoder || !m_pDirectSoundBuffer)
        return;
#endif

    if (audioBytes != 0) {
        switch (m_nState) {
        case StreamingChannelState::UNK_MINUS_3:
        case StreamingChannelState::Finished:
        case StreamingChannelState::Started:
            DirectSoundBufferFadeToSilence();
            break;
        default:
            break;
        }

        m_nState = StreamingChannelState::Stopped;
    }

    if (m_pStreamingDecoder)
        delete m_pStreamingDecoder;

    m_bLoopTrack = (soundFlags & 1) != 0;
    m_pStreamingDecoder = newDecoder;
#ifdef NOTSA_NATIVE
    // Drop whatever is left of the previous stream and restart the play time accounting
    if (m_pDirectSoundBuffer) {
        m_pDirectSoundBuffer->Stop();
        m_pDirectSoundBuffer->ClearQueue();
    }
    m_nPlayedBytes      = 0;
    m_nStreamPlayTimeMs = 0;
    m_nPlayTime         = 0;
    m_nLastUpdateTime   = CAEAudioUtility::GetCurrentTimeInMS();
#else
    if (SUCCEEDED(m_pDirectSoundBuffer->Lock(
        0,
        0,
        (LPVOID*)(&newDecoder),
        (DWORD*)&audioBytes,
        nullptr,
        0,
        DSBLOCK_ENTIREBUFFER
    ))) {
        const auto written = FillBuffer(newDecoder, audioBytes);
        if (written == audioBytes) {
            m_bNeedToFinish = 0;
        } else {
            memset(reinterpret_cast<uint8*>(newDecoder) + written, 0, audioBytes - written);
            m_bNeedToFinish = 1;
        }

        // ?
        if (audioBytes & 0xFFFFFFFC) {
            for (auto i = 0u; i < audioBytes >> 2; i++) {
                // *((uint32*)&stream->_vftable + i) |= 0x10001u;
            }
        }

        m_pDirectSoundBuffer->Unlock(newDecoder, audioBytes, nullptr, 0);
    }
#endif

    switch (m_nState) {
    case StreamingChannelState::UNK_MINUS_3:
    case StreamingChannelState::Started:
    case StreamingChannelState::Paused:
        m_nState = StreamingChannelState::UNK_MINUS_2;
        break;
    default:
        break;
    }

    m_lastSlot = 0;
    SetOriginalFrequency(newDecoder->GetSampleRate());
#ifdef NOTSA_NATIVE
    // The play time is derived from the number of bytes played, so keep the format in sync
    m_WaveFormat.nSamplesPerSec  = m_nOriginalFrequency;
    m_WaveFormat.nAvgBytesPerSec = m_nOriginalFrequency * m_WaveFormat.nBlockAlign;
    m_bNeedToFinish = false;
#endif
    m_bSilenced = false;
    m_bNeedSwitch = false;
    m_bShouldStop = false;
#ifdef NOTSA_NATIVE
    if (m_pDirectSoundBuffer) {
        m_pDirectSoundBuffer->SetPlaybackByteOffset(0);

        // Prime the queue (the DirectSound code filled the whole ring buffer right here)
        RefillQueue();
    }
#else
    m_pDirectSoundBuffer->SetCurrentPosition(0);
#endif
}

// 0x4F1DE0
void CAEStreamingChannel::SetNextStream(CAEStreamingDecoder* decoder) {
    if (m_pNextStreamingDecoder) {
        delete m_pNextStreamingDecoder;
    }
    m_pNextStreamingDecoder = decoder;
}

// 0x4F1A60
int32 CAEStreamingChannel::GetPlayingTrackID() {
    switch (m_nState) {
    case StreamingChannelState::UNK_MINUS_3:
    case StreamingChannelState::Finished:
    case StreamingChannelState::Started:
    case StreamingChannelState::Stopping:
        if (m_pStreamingDecoder)
            return m_pStreamingDecoder->GetStreamID();

        [[fallthrough]];
    default:
        return -1;
    }
}

// 0x4F1A40
int32 CAEStreamingChannel::GetActiveTrackID() {
    return m_pStreamingDecoder ? m_pStreamingDecoder->GetStreamID() : -1;
}

// 0x4F18A0
int32 CAEStreamingChannel::UpdatePlayTime() {
    if (m_nState != StreamingChannelState::Started && m_nState != StreamingChannelState::UNK_MINUS_3) {
        m_nPlayTime = static_cast<int32>(m_nState);
        return m_nPlayTime;
    }

#ifdef NOTSA_NATIVE
    const auto now = CAEAudioUtility::GetCurrentTimeInMS();
    if (now - m_nLastUpdateTime < 1'000) {
        return GetPlayTime();
    }

    // Position = everything unqueued so far + the offset inside the buffer being played
    uint32 playedBytes = m_nPlayedBytes;
    if (m_pDirectSoundBuffer) {
        playedBytes += m_pDirectSoundBuffer->GetPlaybackByteOffset();
    }
    m_nPlayTime = static_cast<int32>(
        static_cast<float>(playedBytes) * 1000.0f / static_cast<float>(m_WaveFormat.nAvgBytesPerSec)
    );

    m_nLastUpdateTime = now;
    return m_nPlayTime;
#else
    if (CAEAudioUtility::GetCurrentTimeInMS() - m_nLastUpdateTime < 1'000) {
        return GetPlayTime();
    }

    DWORD currentPlayCursor;
    m_pDirectSoundBuffer->GetCurrentPosition(&currentPlayCursor, nullptr);
    auto freqFactor = m_nFrequency / 250.f;
    auto upperLimit = 393216.f / freqFactor; //TODO: Magic number (393.216 khz can be found often with oscilators / clock generators, also that's 0x60000 in hex)
    auto curStep    = currentPlayCursor / freqFactor;
    if ((uint32(freqFactor) / 0x60000) > 0) {
        if (m_lastWrittenSlot == 1) {
            m_nPlayTime = static_cast<int32>(m_nStreamPlayTimeMs + curStep - (2 * upperLimit));
        } else {
            m_nPlayTime = static_cast<int32>(m_nStreamPlayTimeMs + curStep - upperLimit);
        }
    } else if (m_lastWrittenSlot == 0) {
        m_nPlayTime = static_cast<int32>(m_nStreamPlayTimeMs + curStep - upperLimit);
    } else {
        m_nPlayTime = static_cast<int32>(m_nStreamPlayTimeMs + curStep);
    }

    if (m_nPlayTime >= upperLimit) {
        m_nPlayTime -= static_cast<int32>(upperLimit);
    }

    m_nLastUpdateTime = CAEAudioUtility::GetCurrentTimeInMS();
    m_nPlayTime       = std::max(0, m_nPlayTime); // Originally a bitwise hack: x &= (x <= 0) - 1
    return m_nPlayTime;
#endif // NOTSA_NATIVE
}

// 0x4F1AE0
bool CAEStreamingChannel::AddFX() {
#ifdef NOTSA_NATIVE
    if (m_bEQEnabled)
        return true;

    const auto efx = notsa::oal::GetEFX();
    if (!efx || !m_pDirectSoundBuffer) {
        NOTSA_LOG_WARN("OpenAL: EFX is not available - the stream equalizer is disabled");
        return false;
    }

    efx->GenEffects(1, &m_effectId);
    efx->Effecti(m_effectId, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);

    efx->GenAuxiliaryEffectSlots(1, &m_effectSlot);
    efx->AuxiliaryEffectSloti(m_effectSlot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(m_effectId));

    m_pDirectSoundBuffer->SetEffectSlot(m_effectSlot);

    m_bEQEnabled = true;
    return true;
#else
    if (m_bEQEnabled)
        return true;

    DSEFFECTDESC effectDesc[2]{};
    for (auto& desc : effectDesc) {
        desc.guidDSFXClass = GUID_DSFX_STANDARD_PARAMEQ;
        desc.dwFlags = 0;
        desc.dwReserved1 = desc.dwReserved2 = 0;
        desc.dwSize = 32;
    }

    bool wasStopped = false;
    if (bufferStatus.Bit0x1) {
        m_pDirectSoundBuffer->Stop();
        wasStopped = true;
    }

    if (FAILED(m_pDirectSoundBuffer8->SetFX(
        std::size(effectDesc),
        effectDesc,
        nullptr
    )))
        return false;

    for (auto i = 0; i < 2; i++) { // Set both channels
        IDirectSoundFXParamEq* fxParamEq{};
        m_pDirectSoundBuffer8->GetObjectInPath(
            GUID_All_Objects,
            i,
            IID_IDirectSoundFXParamEq,
            reinterpret_cast<LPVOID*>(&fxParamEq)
        );
        fxParamEq->SetAllParameters(&s_FXParamEqPresets[0][i]);
        fxParamEq->Release();
    }

    if (wasStopped)
        m_pDirectSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

    m_bEQEnabled = true;
    return true;
#endif // NOTSA_NATIVE
}

// 0x4F1C20
void CAEStreamingChannel::RemoveFX() {
#ifdef NOTSA_NATIVE
    if (m_pDirectSoundBuffer) {
        m_pDirectSoundBuffer->SetEffectSlot(AL_EFFECTSLOT_NULL);
    }

    if (const auto efx = notsa::oal::GetEFX()) {
        if (m_effectSlot) {
            efx->DeleteAuxiliaryEffectSlots(1, &m_effectSlot);
            m_effectSlot = 0;
        }
        if (m_effectId) {
            efx->DeleteEffects(1, &m_effectId);
            m_effectId = 0;
        }
    }

    m_bEQEnabled = false;
#else
    bool stopped{false};
    if (bufferStatus.Bit0x1) {
        m_pDirectSoundBuffer->Stop();
        stopped = true;
    }

    m_pDirectSoundBuffer8->SetFX(0, nullptr, nullptr);

    if (stopped)
        m_pDirectSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

    m_bEQEnabled = false;
#endif // NOTSA_NATIVE
}

// 0x4F2040
bool CAEStreamingChannel::IsSoundPlaying() {
    switch (m_nState) {
    case StreamingChannelState::UNK_MINUS_3:
    case StreamingChannelState::Finished:
    case StreamingChannelState::Stopping:
    case StreamingChannelState::Started:
        return true;
    default:
        return false;
    }
}

// 0x4F19E0
int16 CAEStreamingChannel::GetPlayTime() {
    switch (m_nState) {
    case StreamingChannelState::Started:
    case StreamingChannelState::UNK_MINUS_3:
        if (CAEAudioUtility::GetCurrentTimeInMS() - m_nLastUpdateTime >= 1000u)
            return UpdatePlayTime();
        else
            return static_cast<int16>(m_nPlayTime - static_cast<uint32>(m_nLastUpdateTime) + CAEAudioUtility::GetCurrentTimeInMS());
        break;
    default:
        break;
    }

    return static_cast<int16>(m_nState); // possibly not intended.
}

// 0x4F1880
uint16 CAEStreamingChannel::GetLength() {
    return static_cast<uint16>(m_pStreamingDecoder ? m_pStreamingDecoder->GetStreamLengthMs() : -1);
}

// 0x4F1D40
void CAEStreamingChannel::Play(int16 startOffsetMs, int8 soundFlags, float freqFac) {
#ifdef NOTSA_NATIVE
    if (!m_pStreamingDecoder || !m_pDirectSoundBuffer)
        return;

    SetOriginalFrequency(m_pStreamingDecoder->GetSampleRate());

    if (m_nState != StreamingChannelState::Paused) {
        m_bLoopTrack = (soundFlags & 1) != 0;
    }

    if (m_nState == StreamingChannelState::Stopping) {
        AESmoothFadeThread.CancelFade(m_pDirectSoundBuffer);
        m_pDirectSoundBuffer->Stop();

        m_nState = StreamingChannelState::Stopped;
    }

    m_nState = StreamingChannelState::Started;
    m_pDirectSoundBuffer->SetVolumeDb(m_Volume);
    // NOTSA: The queue must not loop - a single track is looped by `FillBuffer` instead.
    m_pDirectSoundBuffer->Play(false);
#else
    if (!m_pStreamingDecoder || !m_pDirectSoundBuffer)
        return;

    SetOriginalFrequency(m_pStreamingDecoder->GetSampleRate());

    if (m_nState != StreamingChannelState::Paused) {
        m_bLoopTrack = (soundFlags & 1) != 0;
    }

    if (m_nState == StreamingChannelState::Stopping) {
        AESmoothFadeThread.CancelFade(m_pDirectSoundBuffer);
        m_pDirectSoundBuffer->Stop();

        m_nState = StreamingChannelState::Stopped;
    }

    m_nState = StreamingChannelState::Started;
    m_pDirectSoundBuffer->SetVolume(static_cast<int32>(m_Volume * 100.0f));
    m_pDirectSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
#endif
}

// 0x4F2170
void CAEStreamingChannel::Pause() {
    if (!m_pDirectSoundBuffer)
        return;

    DirectSoundBufferFadeToSilence();

    m_nState = StreamingChannelState::Paused;
}

// 0x4F1A90
void CAEStreamingChannel::Stop(bool bUpdateState) {
    DirectSoundBufferFadeToSilence();

    if (bUpdateState) {
        m_nState = StreamingChannelState::Stopping;
        m_lStoppingFrameCount = 0;
    }
}

// unused
// 0x4F21C0
void CAEStreamingChannel::Stop() {
    DirectSoundBufferFadeToSilence();
    m_nState = StreamingChannelState::Stopping;
    m_lStoppingFrameCount = 0;
}

#ifdef NOTSA_NATIVE
//! Keeps the queue topped up, replacing the DirectSound ring buffer refill of 0x4F2550.
void CAEStreamingChannel::RefillQueue() {
    auto* const voice = m_pDirectSoundBuffer;
    if (!voice || !m_pStreamingDecoder) {
        return;
    }

    // Release the chunks that already played, and account for them in the play time
    m_nPlayedBytes += voice->UnqueueProcessed();

    uint32 queued = voice->GetQueuedBufferCount();
    while (queued < STREAM_CHUNK_COUNT) {
        if (m_bNeedSwitch) { // The next stream has to be switched to
            auto* const next = std::exchange(m_pNextStreamingDecoder, nullptr);

            m_nState = StreamingChannelState::Paused;
            Stop(false);
            PrepareStream(next, 0, 0);
            Play(0, 0, 1.0f);
            return;
        }

        if (m_bSilenced) { // The next stream has a different sample rate - queue silence meanwhile
            memset(m_pBuffer, 0, STREAM_CHUNK_BYTES);

            auto* const chunk = new OALBuffer{m_pBuffer, STREAM_CHUNK_BYTES, m_nOriginalFrequency, 2};
            voice->QueueBuffer(chunk);
            chunk->Release();

            m_bSilenced   = false;
            m_bNeedSwitch = true;
            continue;
        }

        if (m_bNeedToFinish) {
            m_nState = StreamingChannelState::Finished;
        }

        if (m_nState == StreamingChannelState::Finished) {
            // Let the already queued audio play out, then stop
            if (!m_bShouldStop) {
                m_bShouldStop = true;
            } else if (queued == 0) {
                Stop();
                m_nState      = StreamingChannelState::Stopped;
                m_bShouldStop = false;
            }
            return;
        }

        const auto filled = FillBuffer(m_pBuffer, STREAM_CHUNK_BYTES);
        if (filled < STREAM_CHUNK_BYTES) {
            memset(static_cast<uint8*>(m_pBuffer) + filled, 0, STREAM_CHUNK_BYTES - filled);
        }

        auto* const chunk = new OALBuffer{m_pBuffer, STREAM_CHUNK_BYTES, m_nOriginalFrequency, 2};
        const bool  ok    = voice->QueueBuffer(chunk);
        chunk->Release();
        if (!ok) {
            return;
        }
        ++queued;
    }
}
#endif // NOTSA_NATIVE

// 0x4F2550
void CAEStreamingChannel::Service() {
#ifdef NOTSA_NATIVE
    UpdateStatus();

    const bool playing = IsBufferPlaying();

    if (!playing) {
        if (m_nState == StreamingChannelState::Stopping) {
            m_nState = StreamingChannelState::Stopped;
        }
    } else {
        switch (m_nState) {
        case StreamingChannelState::Started:
            m_nState = StreamingChannelState::UNK_MINUS_3;
            break;
        case StreamingChannelState::UNK_MINUS_3:
        case StreamingChannelState::Finished:
            break;
        case StreamingChannelState::Stopping:
            if (++m_lStoppingFrameCount > 6) {
                Stop();
            }
            break;
        default:
            m_lStoppingFrameCount = 0;
            if (m_nState == StreamingChannelState::Stopped) {
                Stop();
            }
            break;
        }
    }

    if (m_nState == StreamingChannelState::UNK_MINUS_3 && !playing) {
        m_nState = StreamingChannelState::Paused;
    }

    if (m_nState != StreamingChannelState::UNK_MINUS_3
     && m_nState != StreamingChannelState::Finished
     && m_nState != StreamingChannelState::Paused
    ) {
        return;
    }

    if (!m_pStreamingDecoder) {
        return;
    }

    // The voice has to play at the decoder's sample rate
    if (const auto rate = m_pStreamingDecoder->GetSampleRate(); rate != m_nOriginalFrequency) {
        SetOriginalFrequency(rate);
    }

    RefillQueue();
#else
    plugin::CallMethod<0x4F2550, CAEStreamingChannel*>(this);
#endif // NOTSA_NATIVE
}

void CAEStreamingChannel::InjectHooks() {
    RH_ScopedVirtualClass(CAEStreamingChannel, 0x85F3F0, 9);
    RH_ScopedCategory("Audio/Hardware");

    RH_ScopedInstall(Constructor, 0x4F1800, { .Reversed = false }); // makes game not load radio
    RH_ScopedInstall(Destructor, 0x4F2200);
    RH_ScopedInstall(SynchPlayback, 0x4F1870);
    RH_ScopedInstall(PrepareStream, 0x4F23D0, { .Reversed = false });
    RH_ScopedInstall(Initialise, 0x4F22F0);
    RH_ScopedInstall(Pause, 0x4F2170);
    RH_ScopedInstall(SetReady, 0x4F1FF0);
    RH_ScopedInstall(SetBassEQ, 0x4F1F30);
    RH_ScopedInstall(FillBuffer, 0x4F1E20);
    RH_ScopedInstall(InitialiseSilence, 0x4F1C70);
    RH_ScopedInstall(SetNextStream, 0x4F1DE0);
    RH_ScopedInstall(AddFX, 0x4F1AE0);
    RH_ScopedOverloadedInstall(Stop, "class", 0x4F1A90, void(CAEStreamingChannel::*)(bool));
    RH_ScopedInstall(GetPlayingTrackID, 0x4F1A60);
    RH_ScopedInstall(GetActiveTrackID, 0x4F1A40);
    RH_ScopedInstall(UpdatePlayTime, 0x4F18A0);
    RH_ScopedInstall(RemoveFX, 0x4F1C20);
    RH_ScopedVMTInstall(Service, 0x4F2550, { .Reversed = false });
    RH_ScopedVMTInstall(IsSoundPlaying, 0x4F2040);
    RH_ScopedVMTInstall(GetPlayTime, 0x4F19E0);
    RH_ScopedVMTInstall(GetLength, 0x4F1880);
    RH_ScopedVMTInstall(Play, 0x4F1D40);
    RH_ScopedVMTOverloadedInstall(Stop, "virtual", 0x4F21C0, void(CAEStreamingChannel::*)());
    RH_ScopedVMTInstall(SetFrequencyScalingFactor, 0x4F2060);
}
