#pragma once

#include <type_traits>

#include "vorbis/vorbisfile.h"

#include "AEStreamingDecoder.h"

class CAEVorbisDecoder : public CAEStreamingDecoder {
public:
    //! Return type of `ov_callbacks::tell_func`, kept identical to what libvorbis declares.
    using OVTellResult = std::invoke_result_t<decltype(ov_callbacks::tell_func), void*>;

    CAEVorbisDecoder(CAEDataStream* dataStream, int32 userTrack);
    ~CAEVorbisDecoder() override;

    bool   Initialise() override;
    size_t FillBuffer(void* dest, size_t size) override;
    int32  GetStreamLengthMs() override;
    int32  GetStreamPlayTimeMs() override;
    void   SetCursor(uint32 pos) override;
    int32  GetSampleRate() override;
    int32  GetStreamID() override;

private:
    ov_callbacks    m_sCallbacks;
    OggVorbis_File* m_pVorbisFileHandle;
    bool            m_bInitialized;
    bool            m_bIsUserTrack;
    CAEDataStream*  m_pDataStreamCopy;
    vorbis_info*    m_pVorbisInfo;

    static ov_callbacks staticCallbacks;

    static size_t ReadCallback(void* ptr, size_t size, size_t nmemb, void* opaque);
    static int32    CloseCallback(void* opaque);
    static int32    SeekCallback(void* opaque, ogg_int64_t offset, int32 whence);
    static OVTellResult TellCallback(void* opaque);

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CAEVorbisDecoder* Constructor(CAEDataStream* dataStream, int32 unknown);
    CAEVorbisDecoder* Destructor();
};

VALIDATE_SIZE(CAEVorbisDecoder, 0x28);
