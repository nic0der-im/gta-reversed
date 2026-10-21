#include "StdInc.h"

#include "AEStreamThread.h"
#include "AEAudioUtility.h"
#include "AEMP3TrackLoader.h"
#include "AEUserRadioTrackManager.h"
#include "AEVorbisDecoder.h"

#ifdef NOTSA_NATIVE
#include "Audio/OpenAL/OALBase.h"

#include <mutex>
#include <thread>
#endif

#ifdef NOTSA_NATIVE
struct CAEStreamThread::NativeState {
    std::thread thread;
    std::mutex  mutex;
};

CAEStreamThread::NativeState& CAEStreamThread::GetNativeState() {
    if (!m_nativeState) {
        // Only ever allocated by `Initialise`/`Start`, i.e. on the main thread,
        // before the worker exists.
        m_nativeState = new NativeState{};
    }
    return *m_nativeState;
}
#endif

void CAEStreamThread::InjectHooks() {
    RH_ScopedClass(CAEStreamThread);
    RH_ScopedCategory("Audio");

    RH_ScopedInstall(Initialise, 0x4F1680);
    RH_ScopedInstall(Start, 0x4F11F0);
    RH_ScopedInstall(Stop, 0x4F1590);
    RH_ScopedInstall(Pause, 0x4F1200);
    RH_ScopedInstall(Resume, 0x4F1210);
    RH_ScopedInstall(WaitForExit, 0x4F1220);
    RH_ScopedInstall(PlayTrack, 0x4F1230);
    RH_ScopedInstall(GetTrackPlayTime, 0x4F1530);
    RH_ScopedInstall(GetTrackLengthMs, 0x4F1550);
    RH_ScopedInstall(GetActiveTrackID, 0x4F1560);
    RH_ScopedInstall(GetPlayingTrackID, 0x4F1570);
    RH_ScopedInstall(StopTrack, 0x4F1580);
    RH_ScopedInstall(MainLoop, 0x4F15C0);
    RH_ScopedInstall(Service, 0x4F1290);
}

// 0x4F11E0
CAEStreamThread::~CAEStreamThread() {
#ifdef NOTSA_NATIVE
    Stop();
#else
    OS_MutexRelease(&m_criticalSection);
#endif
}

// 0x4F1680
bool CAEStreamThread::Initialise(CAEStreamingChannel* streamingChannel) {
    m_bActive          = true;
    m_bThreadActive    = false;
    m_bNeedsService = 0;
    m_bPreparingStream = 0;
    m_bStopRequest     = false;
    m_nPlayingTrackId  = streamingChannel->GetPlayingTrackID();
    m_nPlayTime        = streamingChannel->GetPlayTime();
    m_nActiveTrackId   = streamingChannel->GetActiveTrackID();
    m_nTrackLengthMs   = streamingChannel->GetLength();

#ifdef NOTSA_NATIVE
    // The worker thread itself is created by `Start`
#else
    m_nHandle = CreateThread(nullptr, 0, &CAEStreamThread::MainLoop, this, CREATE_SUSPENDED, (LPDWORD)(&m_lpThreadId));
    assert(m_nHandle);
    SetThreadPriority(m_nHandle, THREAD_PRIORITY_NORMAL);
    InitializeCriticalSection(&m_criticalSection);
    //TODO: NOTSA: Replace CriticalSection usages in this class with platform independant mutex from oswrapper
#endif
    m_pStreamingChannel = streamingChannel;

#ifdef NOTSA_NATIVE
    // No MP3/radio stream module natively (see `CAEAudioHardware::Initialise`)
    m_pMp3TrackLoader = nullptr;
#else
    m_pMp3TrackLoader = new CAEMP3TrackLoader();
    m_pMp3TrackLoader->Initialise();
#endif

    return m_bActive;
}

// 0x4F11F0
void CAEStreamThread::Start() {
    m_bThreadActive = true;
#ifdef NOTSA_NATIVE
    if (!GetNativeState().thread.joinable()) {
        GetNativeState().thread = std::thread(&CAEStreamThread::MainLoop, this);
    }
#else
    ResumeThread(m_nHandle);
#endif
}

// 0x4F1590
void CAEStreamThread::Stop() {
    m_bThreadActive = false;
#ifdef NOTSA_NATIVE
    // Join before releasing the track loader - the worker may still be using it
    if (m_nativeState && m_nativeState->thread.joinable()) {
        m_nativeState->thread.join();
    }
#endif
    if (std::exchange(m_bActive, false)) {
        delete m_pMp3TrackLoader;
    }
}

// 0x4F1200
void CAEStreamThread::Pause() const {
#ifdef NOTSA_NATIVE
    // Nothing to suspend: the worker only plays while the game is in the foreground
    // (see `MainLoop`), and it has to keep servicing the stream otherwise.
#else
    SuspendThread(m_nHandle);
#endif
}

// 0x4F1210
void CAEStreamThread::Resume() const {
#ifndef NOTSA_NATIVE
    ResumeThread(m_nHandle);
#endif
}

// 0x4F1220
void CAEStreamThread::WaitForExit() const {
#ifdef NOTSA_NATIVE
    if (m_nativeState && m_nativeState->thread.joinable()) {
        m_nativeState->thread.join();
    }
#else
    WaitForSingleObject(m_nHandle, INFINITE);
#endif
}

// 0x4F1530
int16 CAEStreamThread::GetTrackPlayTime() const {
    if (m_bPreparingStream)
        return -8;
    else
        return m_pStreamingChannel->GetPlayTime();
}

// 0x4F1550
int32 CAEStreamThread::GetTrackLengthMs() const {
    return m_nTrackLengthMs;
}

// 0x4F1560
int32 CAEStreamThread::GetActiveTrackID() const {
    return m_nActiveTrackId;
}

// 0x4F1570
int32 CAEStreamThread::GetPlayingTrackID() const {
    return m_nPlayingTrackId;
}

// 0x4F1230
void CAEStreamThread::PlayTrack(uint32 iTrackId, int32 iNextTrackId, uint32 a3, int32 a4, bool bIsUserTrack, bool bNextIsUserTrack) {
#ifdef NOTSA_NATIVE
    std::lock_guard lock{ GetNativeState().mutex };
#else
    EnterCriticalSection(&m_criticalSection);
#endif

    if (m_pStreamingChannel->GetPlayTime() == -2) {
        m_pStreamingChannel->Stop();
    }

    m_iTrackId         = iTrackId;
    m_iNextTrackId     = iNextTrackId;
    field_1C           = a3;
    m_TrackFlags = a4;
    m_bIsUserTrack     = bIsUserTrack;
    m_bNextIsUserTrack = bNextIsUserTrack;
    m_bNeedsService = 1;

#ifndef NOTSA_NATIVE
    LeaveCriticalSection(&m_criticalSection);
#endif
}

// 0x4F1580
void CAEStreamThread::StopTrack() {
    m_bStopRequest = true;
}

// 0x4F15C0
#ifdef NOTSA_NATIVE
void CAEStreamThread::MainLoop(CAEStreamThread* self) {
#else
DWORD WINAPI CAEStreamThread::MainLoop(LPVOID data) {
#endif
#ifdef TRACY_ENABLE
    tracy::SetThreadName("AEStreamThread");
#endif

#ifndef NOTSA_NATIVE
    const auto self = reinterpret_cast<CAEStreamThread*>(data);
#else
    // OpenAL's current context is per-thread
    notsa::oal::MakeContextCurrent();
#endif

    bool wasForeground = true;
    bool play = false;

    while (self->m_bThreadActive) {
        ZoneScoped;

        bool isForeground = IsForegroundApp();
        if (isForeground) {
            if (!wasForeground && play){
                self->m_pStreamingChannel->Play(0, 0, 1.0f);
            }

            auto start = CAEAudioUtility::GetCurrentTimeInMS();
            self->Service();
            self->m_pStreamingChannel->Service();
            auto end = CAEAudioUtility::GetCurrentTimeInMS();
            if (end - start < 5) {
                OS_ThreadSleep(uint32(start - end + 5));
            }
        } else if (wasForeground) {
            if (self->m_pStreamingChannel->IsBufferPlaying()) {
                self->m_pStreamingChannel->Pause();
                play = true;
            } else {
                play = false;
            }
        }
        wasForeground = isForeground;
        OS_ThreadSleep(100);
    }

#ifndef NOTSA_NATIVE
    return 0;
#else
    notsa::oal::ClearCurrentContext();
#endif
}

// 0x4F1290
void CAEStreamThread::Service() {
    m_pStreamingChannel->UpdatePlayTime();
    if (m_bStopRequest) {
        m_pStreamingChannel->Stop();
        m_bStopRequest = false;
    }

    const auto SaveStreamingState = [&] {
        if (m_bPreparingStream)
            return;

        m_nPlayingTrackId = m_pStreamingChannel->GetPlayingTrackID();
        m_nPlayTime = m_pStreamingChannel->GetPlayTime();
        m_nActiveTrackId = m_pStreamingChannel->GetActiveTrackID();
        m_nTrackLengthMs = m_pStreamingChannel->GetLength();
    };

    if (!m_bNeedsService) {
        SaveStreamingState();
        return;
    }

#ifdef NOTSA_NATIVE
    std::lock_guard lock{ GetNativeState().mutex };
#else
    EnterCriticalSection(&m_criticalSection);
#endif
    m_bNeedsService = false;

    const auto LoadDecoder = [&](bool userTrackCheck, uint32 trackId) {
        CAEStreamingDecoder* ptr{};
        if (userTrackCheck) {
            ptr = AEUserRadioTrackManager.LoadUserTrack(trackId);
        } else {
#ifdef NOTSA_NATIVE
            // No radio stream track loader natively - only user tracks (Vorbis/WAV) exist
            ptr = nullptr;
#else
            ptr = new CAEVorbisDecoder(m_pMp3TrackLoader->GetDataStream(trackId), 0);
#endif
        }

        if (ptr && !ptr->Initialise()) {
            delete std::exchange(ptr, nullptr);
        }
        return ptr;
    };

    CAEStreamingDecoder* nextDecoder{};
    if (m_iNextTrackId != -1) { // RADIO_INVALID
         nextDecoder = LoadDecoder(m_bNextIsUserTrack, m_iNextTrackId);
    } else {
        m_pStreamingChannel->SetNextStream(nullptr);
    }

    if (m_iNextTrackId == -1 || m_pStreamingChannel->GetPlayingTrackID() != m_iTrackId) {
        auto* currDecoder = LoadDecoder(m_bIsUserTrack, m_iTrackId);
        if (currDecoder) {
            currDecoder->SetCursor(m_iNextTrackId % currDecoder->GetStreamLengthMs());

            m_pStreamingChannel->SetNextStream(nextDecoder);
            m_pStreamingChannel->PrepareStream(currDecoder, m_TrackFlags, 1u);
            m_bPreparingStream = false;
        } else {
            m_pStreamingChannel->SetNextStream(nullptr);
            if (nextDecoder) {
                m_pStreamingChannel->PrepareStream(nextDecoder, m_TrackFlags, 1u);
                m_bPreparingStream = false;
            } else {
                m_bPreparingStream = true;
            }
        }
    } else {
        if (nextDecoder)
            m_pStreamingChannel->SetNextStream(nextDecoder);
        m_pStreamingChannel->SetReady();
    }

    if (m_bPreparingStream) {
        m_nPlayingTrackId = m_iNextTrackId == -1 ? m_iTrackId : m_iNextTrackId;
        m_nActiveTrackId = m_iNextTrackId == -1 ? m_iTrackId : m_iNextTrackId;
    } else {
        SaveStreamingState();
    }

#ifndef NOTSA_NATIVE
    LeaveCriticalSection(&m_criticalSection);
#endif
}
