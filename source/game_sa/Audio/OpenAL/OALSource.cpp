#include "StdInc.h"

#include "OALSource.h"
#include "OALBuffer.h"

#ifdef NOTSA_NATIVE

//! `AL_SOFT_loop_points` (see `alext.h`) - only the constant is needed, not the whole extension header.
#ifndef AL_LOOP_POINTS_SOFT
#define AL_LOOP_POINTS_SOFT 0x2015
#endif

OALSource::~OALSource() {
    if (!notsa::oal::IsDeviceOpen()) {
        // The context is gone already - just drop the references, there's nothing to free.
        for (auto* buffer : m_queuedBuffers) {
            buffer->Release();
        }
        m_queuedBuffers.clear();

        if (m_buffer) {
            std::exchange(m_buffer, nullptr)->Release();
        }
        m_sourceId = 0;
        return;
    }

    ReleaseQueuedBuffers();
    DetachBuffer();

    if (m_sourceId) {
        OAL_CHECKED(alDeleteSources(1, &m_sourceId));
        m_sourceId = 0;
    }
}

void OALSource::ObtainSource() {
    if (m_sourceId || !notsa::oal::IsDeviceOpen()) {
        return;
    }

    if (!notsa::oal::MakeContextCurrent()) {
        return;
    }

    OAL_CHECKED(alGenSources(1, &m_sourceId));
    if (!m_sourceId) {
        return;
    }

    // A DirectSound buffer without 3D processing behaves like a plain stereo voice.
    OAL_CHECKED(alSourcei(m_sourceId, AL_SOURCE_RELATIVE, AL_FALSE));
    OAL_CHECKED(alSourcef(m_sourceId, AL_ROLLOFF_FACTOR, 0.0f)); // The game attenuates by distance itself
    OAL_CHECKED(alSourcef(m_sourceId, AL_GAIN, m_volumeDb <= -100.0f ? 0.0f : std::pow(10.0f, m_volumeDb * 0.05f)));

    if (m_buffer && m_bufferId) {
        OAL_CHECKED(alSourcei(m_sourceId, AL_BUFFER, static_cast<ALint>(m_bufferId)));
    }
}

void OALSource::DetachBuffer() {
    if (m_bufferId && m_sourceId) {
        OAL_CHECKED(alSourcei(m_sourceId, AL_BUFFER, 0));
    }
    m_bufferId = 0;

    if (m_buffer) {
        std::exchange(m_buffer, nullptr)->Release();
    }
}

void OALSource::ReleaseQueuedBuffers() {
    if (m_sourceId) {
        OAL_CHECKED(alSourcei(m_sourceId, AL_BUFFER, 0)); // Unqueues everything
    }

    for (auto* buffer : m_queuedBuffers) {
        buffer->Release();
    }
    m_queuedBuffers.clear();
}

bool OALSource::SetBuffer(OALBuffer* buffer) {
    if (!buffer || !buffer->m_bufferId) {
        return false;
    }

    ObtainSource();
    if (!m_sourceId) {
        return false;
    }

    DetachBuffer();

    m_buffer    = buffer;
    m_bufferId  = buffer->m_bufferId;
    buffer->AddRef();

    if (!m_frequency) {
        m_frequency = buffer->m_frequency;
    }
    m_bufferFrequency = buffer->m_frequency;

    OAL_CHECKED(alSourcei(m_sourceId, AL_BUFFER, static_cast<ALint>(m_bufferId)));
    ApplyPitch();
    return true;
}

void OALSource::SetLoopPoints(uint32 startSample, uint32 endSample) {
    if (!m_sourceId || endSample <= startSample) {
        return;
    }

    if (!m_loopPointsSupported) {
        return; // Already reported
    }

    if (!alIsExtensionPresent("AL_SOFT_loop_points")) {
        m_loopPointsSupported = false;
        NOTSA_LOG_WARN("OpenAL: AL_SOFT_loop_points is not available - looping sounds will include their intro");
        return;
    }

    const ALint points[]{
        static_cast<ALint>(startSample),
        static_cast<ALint>(endSample)
    };
    OAL_CHECKED(alSourceiv(m_sourceId, AL_LOOP_POINTS_SOFT, points));
}

bool OALSource::QueueBuffer(OALBuffer* buffer) {
    if (!buffer || !buffer->m_bufferId) {
        return false;
    }

    ObtainSource();
    if (!m_sourceId) {
        return false;
    }

    OAL_CHECKED(alSourceQueueBuffers(m_sourceId, 1, &buffer->m_bufferId));

    m_queuedBuffers.push_back(buffer);
    buffer->AddRef();

    if (!m_frequency) {
        m_frequency = buffer->m_frequency;
    }
    m_bufferFrequency = buffer->m_frequency;
    ApplyPitch();
    return true;
}

uint32 OALSource::UnqueueProcessed() {
    ALint processed{};
    if (m_sourceId) {
        OAL_CHECKED(alGetSourcei(m_sourceId, AL_BUFFERS_PROCESSED, &processed));
    }

    uint32 unqueuedBytes = 0;
    for (ALint i = 0; i < processed; i++) {
        ALuint bufferId{};
        OAL_CHECKED(alSourceUnqueueBuffers(m_sourceId, 1, &bufferId));

        const auto it = rng::find_if(m_queuedBuffers, [bufferId](const OALBuffer* buffer) {
            return buffer->m_bufferId == bufferId;
        });
        if (it == m_queuedBuffers.end()) {
            continue;
        }

        unqueuedBytes += (*it)->m_size;
        (*it)->Release();
        m_queuedBuffers.erase(it);
    }
    return unqueuedBytes;
}

void OALSource::ClearQueue() {
    ReleaseQueuedBuffers();
}

uint32 OALSource::GetQueuedBufferCount() const {
    ALint queued{};
    if (m_sourceId) {
        OAL_CHECKED(alGetSourcei(m_sourceId, AL_BUFFERS_QUEUED, &queued));
    }
    return static_cast<uint32>(queued);
}

void OALSource::Play(bool looping) {
    ObtainSource();
    if (!m_sourceId) {
        return;
    }

    OAL_CHECKED(alSourcei(m_sourceId, AL_LOOPING, looping ? AL_TRUE : AL_FALSE));

    // DirectSound's `Play` continues from the buffer's current position, so do we
    // (a plain `alSourcePlay` after a stop would restart the buffer from the beginning).
    if (m_resumeOffset && m_queuedBuffers.empty()) {
        OAL_CHECKED(alSourcei(m_sourceId, AL_BYTE_OFFSET, static_cast<ALint>(m_resumeOffset)));
    }
    m_resumeOffset = 0;

    OAL_CHECKED(alSourcePlay(m_sourceId));
}

void OALSource::Pause() {
    if (m_sourceId) {
        OAL_CHECKED(alSourcePause(m_sourceId));
    }
}

void OALSource::Stop() {
    if (!m_sourceId) {
        return;
    }

    m_resumeOffset = GetPlaybackByteOffset();
    OAL_CHECKED(alSourceStop(m_sourceId));
}

void OALSource::Rewind() {
    m_resumeOffset = 0;
    if (m_sourceId) {
        OAL_CHECKED(alSourceRewind(m_sourceId));
    }
}

bool OALSource::IsPlaying() const {
    ALint state{};
    if (!m_sourceId) {
        return false;
    }
    OAL_CHECKED(alGetSourcei(m_sourceId, AL_SOURCE_STATE, &state));
    return state == AL_PLAYING;
}

void OALSource::SetVolumeDb(float db) {
    const auto volume = std::clamp(db, -100.0f, 0.0f);
    if (m_volumeDb == volume) {
        return;
    }

    m_volumeDb = volume;
    if (m_sourceId) {
        OAL_CHECKED(alSourcef(m_sourceId, AL_GAIN, volume <= -100.0f ? 0.0f : std::pow(10.0f, volume * 0.05f)));
    }
}

void OALSource::SetFrequency(uint32 hz) {
    m_frequency = hz;
    ApplyPitch();
}

void OALSource::ApplyPitch() {
    if (!m_sourceId || !m_bufferFrequency) {
        return;
    }

    const auto pitch = m_frequency
        ? static_cast<float>(m_frequency) / static_cast<float>(m_bufferFrequency)
        : 1.0f;
    OAL_CHECKED(alSourcef(m_sourceId, AL_PITCH, pitch));
}

void OALSource::SetPosition(const CVector& pos) {
    ObtainSource();
    if (!m_sourceId) {
        return;
    }

    const auto alPos = notsa::oal::ToListenerSpace(pos);
    OAL_CHECKED(alSource3f(m_sourceId, AL_POSITION, alPos.x, alPos.y, alPos.z));
}

void OALSource::SetVelocity(const CVector& vel) {
    ObtainSource();
    if (!m_sourceId) {
        return;
    }

    const auto alVel = notsa::oal::ToListenerSpace(vel);
    OAL_CHECKED(alSource3f(m_sourceId, AL_VELOCITY, alVel.x, alVel.y, alVel.z));
}

void OALSource::SetPlaybackByteOffset(uint32 offset) {
    m_resumeOffset = 0;
    if (m_sourceId) {
        OAL_CHECKED(alSourcei(m_sourceId, AL_BYTE_OFFSET, static_cast<ALint>(offset)));
    } else {
        m_resumeOffset = offset;
    }
}

uint32 OALSource::GetPlaybackByteOffset() const {
    ALint offset{};
    if (m_sourceId) {
        OAL_CHECKED(alGetSourcei(m_sourceId, AL_BYTE_OFFSET, &offset));
    }
    return static_cast<uint32>(offset);
}

void OALSource::SetEffectSlot(ALuint slot) {
    ObtainSource();
    if (!m_sourceId) {
        return;
    }

    OAL_CHECKED(alSource3i(m_sourceId, AL_AUXILIARY_SEND_FILTER, static_cast<ALint>(slot), 0, AL_FILTER_NULL));
}

#endif // NOTSA_NATIVE
