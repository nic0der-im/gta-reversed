#pragma once

#include "AEStreamingChannel.h"

#ifdef NOTSA_NATIVE
#include <mutex>
#endif

class CAEMP3TrackLoader;

class CAEStreamThread {
public:
#ifdef NOTSA_NATIVE
    //! The silent backend doesn't run a streaming thread (nothing to stream until B4).
    std::mutex            m_mutex;
#else
    HANDLE m_nHandle;
    LPDWORD               m_lpThreadId;
#endif
    CAEStreamingChannel*  m_pStreamingChannel;
    CAEMP3TrackLoader*    m_pMp3TrackLoader;
    bool                  m_bActive;
    bool                  m_bThreadActive;
    char                  m_bNeedsService;
    bool                  m_bIsUserTrack;
    bool                  m_bNextIsUserTrack;
    char                  m_bPreparingStream;
    bool                  m_bStopRequest;
    char                  field_17;
    uint32                m_iTrackId;
    uint32                field_1C;
    int32                 m_iNextTrackId;
    int8                  m_TrackFlags;
    char                  field_25;
    char                  field_26;
    char                  field_27;
    uint32                m_nPlayTime;
    int32                 m_nTrackLengthMs;
    int32                 m_nActiveTrackId;
    int32                 m_nPlayingTrackId;
#ifndef NOTSA_NATIVE
    _RTL_CRITICAL_SECTION m_criticalSection;
#endif

public:
    static void InjectHooks();

    CAEStreamThread() = default; // 0x4F11D0
    ~CAEStreamThread();

    bool Initialise(CAEStreamingChannel* streamingChannel);

    void Start();
    void Stop();
    void Pause() const;
    void Resume() const;
    void WaitForExit() const;

    void PlayTrack(uint32 iTrackId, int32 iNextTrackId, uint32 a3, int32 a4, bool bIsUserTrack, bool bNextIsUserTrack);
    void StopTrack();

    int16 GetTrackPlayTime() const;
    int32 GetTrackLengthMs() const;
    int32 GetActiveTrackID() const;
    int32 GetPlayingTrackID() const;

#ifdef NOTSA_NATIVE
    static void MainLoop(CAEStreamThread* self);
#else
    static DWORD WINAPI MainLoop(LPVOID);
#endif
    void Service();
};

VALIDATE_SIZE(CAEStreamThread, 0x50);
