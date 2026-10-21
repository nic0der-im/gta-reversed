#pragma once

#ifdef NOTSA_NATIVE

#include "OALBase.h"

//! An OpenAL buffer: a block of 16 bit PCM data (mono or stereo).
class OALBuffer final : public OALBase {
public:
    ALuint m_bufferId{};
    uint32 m_size{};      //!< Size of the PCM data, in bytes
    uint32 m_frequency{}; //!< Sample rate of the PCM data, in Hz
    uint32 m_channels{};  //!< 1 (mono) or 2 (stereo)

public:
    OALBuffer(const void* data, uint32 size, uint32 frequency, uint32 channels);
    ~OALBuffer() override;

    OALBuffer(const OALBuffer&)            = delete;
    OALBuffer& operator=(const OALBuffer&) = delete;
};

#endif // NOTSA_NATIVE
