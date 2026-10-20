#pragma once

#ifdef NOTSA_NATIVE
#include "app/platform/native/audio_types.h"
#else
#include "dsound.h"
#endif

enum eSmoothFadeEntryStatus : uint32 {
    STATE_IDLE = 0,
    STATE_ACTIVE = 1,
    STATE_CANCELLED = 2,
};

struct tSmoothFadeEntry {
    IDirectSoundBuffer*    m_pSoundBuffer;
    float                  m_fStartVolume;
    float                  m_fTargetVolume;
    float                  m_fVolumeDiff;
    float                  m_fCurVolume;
    bool                   m_bStopBufferAfterFade;
    char                   _pad;
    uint16                 m_wFadeTime;
    eSmoothFadeEntryStatus m_nStatus;
    uint32                 m_nStartTime;
};
VALIDATE_SIZE(tSmoothFadeEntry, 0x20);

class CAESmoothFadeThread {
public:
    static constexpr int32 NUM_SMOOTHFADE_ENTRIES = 64;

#ifdef NOTSA_NATIVE
    void*            m_threadHandle{};
    uint32           m_dwThreadId{};
#else
    HANDLE           m_threadHandle;
    DWORD            m_dwThreadId;
#endif
    std::array<tSmoothFadeEntry, NUM_SMOOTHFADE_ENTRIES> m_aEntries;
    bool             m_bInitialized;
    bool             m_bActive;
    uint16           m_numRequests;
    uint16           m_lastRequest;
    uint16           m_oldestRequest;
    uint32           m_currentTime;
    bool             m_bSmoothFadesDisabled;
    char             _pad2[3];
    uint32           m_nNumAvailableBuffers;

public:
    static void InjectHooks();

    CAESmoothFadeThread();

    void Initialise();
    void InitialiseRequestSlots();
    void Start();
    void Stop();
    void WaitForExit();
    void Service();
    void CancelFade(IDirectSoundBuffer* buffer);
    bool RequestFade(IDirectSoundBuffer* buffer, float fTargetVolume, int16 fadeTime, bool bStopBufferAfterFade);
    void SetBufferVolume(IDirectSoundBuffer* buffer, float volume);

#ifdef NOTSA_NATIVE
    static void SmoothFadeProc(void* smoothFade);
#else
    static DWORD WINAPI SmoothFadeProc(void* smoothFade);
#endif
};
VALIDATE_SIZE(CAESmoothFadeThread, 0x81C);

extern CAESmoothFadeThread& AESmoothFadeThread;
