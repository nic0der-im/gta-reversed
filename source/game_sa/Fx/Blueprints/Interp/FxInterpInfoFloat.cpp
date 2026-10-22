#include "StdInc.h"

#include "FxInterpInfoFloat.h"
#include "FxManager.h"

// 0x4A8440
FxInterpInfoFloat_c::FxInterpInfoFloat_c() : FxInterpInfo_c() {
    m_Keys = nullptr;
}

// 0x5C16F0
void FxInterpInfoFloat_c::Load(FILESTREAM file) {
    for (auto i = 0; i < m_nCount; i++) {
        ReadField<void>(file);
        ReadField<void>(file, "FX_INTERP_DATA:");

        m_bLooped  = ReadField<bool>(file);
        m_nNumKeys = ReadField<int8>(file);

        if (i == 0) {
            m_pTimes = g_fxMan.Allocate<uint16>(m_nNumKeys);
        }

        m_Keys[i] = g_fxMan.Allocate<float>(m_nNumKeys);
        for (auto j = 0; j < m_nNumKeys; j++) {
            ReadField<void>(file, "FX_KEYFLOAT_DATA:");
            m_pTimes[j] = uint16(ReadField<float>(file) * 256.f);
            m_Keys[i][j] = ReadField<float>(file);
        }
    }
}

// NOTSA
void FxInterpInfoFloat_c::Allocate(int32 count) {
    m_nCount = count;
    m_Keys = g_fxMan.Allocate<float*>(count);
}

// 0x4A85C0
float FxInterpInfoFloat_c::GetVal(int32 attrib, float time, float deltaTime) {
    if (m_nNumKeys == 1) {
        return deltaTime * m_Keys[attrib][0];
    }

    const auto startTime = time - deltaTime;

    // Find the first key that is past the start of the frame (the frame's interval is [startTime, time])
    int32 k = 0;
    while (k < m_nNumKeys && (float)m_pTimes[k] / 256.0f <= startTime) {
        k++;
    }

    if (k == m_nNumKeys) { // The whole frame is past the last key
        return deltaTime * m_Keys[attrib][m_nNumKeys - 1];
    }

    // Value of the attribute at the start of the frame
    float prevValue;
    if (k == 0) {
        prevValue = m_Keys[attrib][0];
    } else {
        const auto t1 = (float)m_pTimes[k] / 256.0f;
        const auto t0 = (float)m_pTimes[k - 1] / 256.0f;
        prevValue = (m_Keys[attrib][k] - m_Keys[attrib][k - 1]) * ((startTime - t0) / (t1 - t0)) + m_Keys[attrib][k - 1];
    }

    // Integrate the (linearly interpolated) attribute over the frame, using one trapezoid per key interval.
    float prevTime = startTime;
    float value    = 0.0f;
    for (; k < m_nNumKeys; k++) {
        const auto t1 = (float)m_pTimes[k] / 256.0f;

        if (t1 >= time) { // The frame ends within this key interval (or exactly on this key)
            const auto t0 = (float)m_pTimes[k - 1] / 256.0f;
            const auto endValue = (m_Keys[attrib][k] - m_Keys[attrib][k - 1]) * ((time - t0) / (t1 - t0)) + m_Keys[attrib][k - 1];
            value += ((endValue - prevValue) * 0.5f + prevValue) * (time - prevTime);
            return value;
        }

        value    += ((m_Keys[attrib][k] - prevValue) * 0.5f + prevValue) * (t1 - prevTime);
        prevValue = m_Keys[attrib][k];
        prevTime  = t1;
    }

    // The frame continues past the last key - integrate up to it
    const auto lastTime = (float)m_pTimes[m_nNumKeys - 1] / 256.0f;
    value += ((m_Keys[attrib][m_nNumKeys - 1] - prevValue) * 0.5f + prevValue) * (lastTime - prevTime);
    return value;
}

// 0x4A8470
void FxInterpInfoFloat_c::GetVal(float* outValues, float delta) {
    if (m_nNumKeys == 1) {
        for (auto i = 0; i < m_nCount; i++) {
            outValues[i] = m_Keys[i][0];
        }
        return;
    }

    if (m_bLooped) { // Wrap the time into one period
        const auto period = (float)m_pTimes[m_nNumKeys - 1] / 256.0f;
        delta -= std::floor(delta / period) * period;
    }

    // Linear interpolation between the keys; outside of the key range the extreme values are used
    for (auto k = 1; k < m_nNumKeys; k++) {
        const auto t1 = (float)m_pTimes[k] / 256.0f;
        if (delta < t1) {
            const auto t0 = (float)m_pTimes[k - 1] / 256.0f;
            const auto f  = (delta - t0) / (t1 - t0);
            for (auto i = 0; i < m_nCount; i++) {
                outValues[i] = (m_Keys[i][k] - m_Keys[i][k - 1]) * f + m_Keys[i][k - 1];
            }
            return;
        }
    }

    for (auto i = 0; i < m_nCount; i++) {
        outValues[i] = m_Keys[i][m_nNumKeys - 1];
    }
}
