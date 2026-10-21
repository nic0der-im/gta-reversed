#pragma once

#ifdef NOTSA_NATIVE
#include "app/platform/native/audio_types.h"
#include "Audio/OpenAL/OALTypes.h"
#else
#include "dsound.h"

#include "Audio/OpenAL/OALTypes.h"
#endif

enum eSmoothFadeEntryStatus : uint32 {
    STATE_IDLE = 0,
    STATE_ACTIVE = 1,
    STATE_CANCELLED = 2,
};

struct tSmoothFadeEntry {
    tAudioBufferHandle*    m_pSoundBuffer;
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
    //! The worker thread and the lock guarding `m_aEntries` (both threads touch it).
    //! Lazily allocated: this object lives in the game's `.bss`, which natively is zero-filled
    //! and never runs a constructor, so it can't hold C++ objects directly.
    struct NativeState;
    NativeState*     m_nativeState{};
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
    void CancelFade(tAudioBufferHandle* buffer);
    bool RequestFade(tAudioBufferHandle* buffer, float fTargetVolume, int16 fadeTime, bool bStopBufferAfterFade);
    void SetBufferVolume(tAudioBufferHandle* buffer, float volume);

#ifdef NOTSA_NATIVE
    static void SmoothFadeProc(void* smoothFade);

    NativeState& GetNativeState();
#else
    static DWORD WINAPI SmoothFadeProc(void* smoothFade);
#endif
};
VALIDATE_SIZE(CAESmoothFadeThread, 0x81C);

extern CAESmoothFadeThread& AESmoothFadeThread;
