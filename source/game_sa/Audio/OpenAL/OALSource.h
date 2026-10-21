#pragma once

#ifdef NOTSA_NATIVE

#include <vector>

#include "OALBase.h"

class OALBuffer;

//! An OpenAL voice: a source plus the buffer(s) it plays.
//!
//! This is the native stand-in for a DirectSound secondary buffer: the game's channel classes
//! (`CAEAudioChannel` and its descendants) talk to a `tAudioBufferHandle`, which is an
//! `IDirectSoundBuffer` on Windows and an `OALSource` here.
//!
//! Volume is expressed in dB, exactly like `CAEAudioChannel::m_Volume` (DirectSound's
//! `SetVolume` takes the same value in 1/100 dB): 0 dB is unity gain, -100 dB is silence.
class OALSource final : public OALBase {
public:
    OALSource() = default;
    ~OALSource() override;

    //! Attaches a single buffer for static playback, replacing any previously attached one.
    bool SetBuffer(OALBuffer* buffer);
    //! Enables looping over `[startSample, endSample)` of the attached buffer.
    //! Does nothing when the device doesn't support `AL_SOFT_loop_points` (the whole buffer
    //! is looped then, which only differs for sounds with an intro before the loop point).
    void SetLoopPoints(uint32 startSample, uint32 endSample);

    //! Appends a buffer to the playback queue (streaming playback). The source keeps a reference.
    bool QueueBuffer(OALBuffer* buffer);
    //! Unqueues (and releases) every buffer the source has already played.
    //! @returns The total size (in bytes) of the unqueued data.
    uint32 UnqueueProcessed();
    //! Unqueues (and releases) every buffer, stopping playback.
    void ClearQueue();
    uint32 GetQueuedBufferCount() const;

    void Play(bool looping);
    void Pause();
    void Stop();
    //! Stops playback and rewinds to the begin of the buffer (or of the queue).
    void Rewind();
    bool IsPlaying() const;

    void  SetVolumeDb(float db);
    float GetVolumeDb() const { return m_volumeDb; }
    //! Sets the playback rate in Hz, relative to the sample rate of the attached buffer.
    void  SetFrequency(uint32 hz);
    void  SetPosition(const CVector& pos);
    void  SetVelocity(const CVector& vel);
    void  SetPlaybackByteOffset(uint32 offset);
    uint32 GetPlaybackByteOffset() const;
    //! Routes the source through an EFX auxiliary effect slot; `AL_EFFECTSLOT_NULL` detaches it.
    void  SetEffectSlot(ALuint slot);

private:
    void ObtainSource();
    void DetachBuffer();
    void ReleaseQueuedBuffers();
    void ApplyPitch();

private:
    ALuint  m_sourceId{};
    ALuint  m_bufferId{};              //!< Id of the buffer attached for static playback
    OALBuffer* m_buffer{};             //!< Ref-counted buffer attached for static playback
    std::vector<OALBuffer*> m_queuedBuffers{};

    float  m_volumeDb{-100.0f};        //!< Cached volume, in dB
    uint32 m_bufferFrequency{};        //!< Sample rate of the attached buffer(s), in Hz
    uint32 m_frequency{};              //!< Requested playback rate, in Hz
    uint32 m_resumeOffset{};           //!< Play position remembered by `Stop`, in bytes
    bool   m_loopPointsSupported{true};
};

#endif // NOTSA_NATIVE
