#include "StdInc.h"

#include "OALBuffer.h"

#ifdef NOTSA_NATIVE

OALBuffer::OALBuffer(const void* data, uint32 size, uint32 frequency, uint32 channels) {
    if (!notsa::oal::IsDeviceOpen()) {
        NOTSA_LOG_ERR("OALBuffer: No OpenAL device");
        return;
    }

    OAL_CHECKED(alGenBuffers(1, &m_bufferId));

    const auto format = channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
    OAL_CHECKED(alBufferData(m_bufferId, format, data, static_cast<ALsizei>(size), static_cast<ALsizei>(frequency)));

    m_size      = size;
    m_frequency = frequency;
    m_channels  = channels;
}

OALBuffer::~OALBuffer() {
    if (m_bufferId && notsa::oal::IsDeviceOpen()) {
        OAL_CHECKED(alDeleteBuffers(1, &m_bufferId));
    }
    m_bufferId = 0;
}

#endif // NOTSA_NATIVE
