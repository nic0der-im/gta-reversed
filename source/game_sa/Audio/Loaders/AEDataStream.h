#pragma once

#include <atomic>

#ifndef NOTSA_NATIVE
#include <objidl.h>
#endif

#ifdef NOTSA_NATIVE
//! The native build has no COM. Only the file-backed part of the stream is used by the
//! decoders, so the `IStream` interface isn't implemented (nothing calls it).
class CAEDataStream {
#else
class CAEDataStream : public IStream {
#endif
public:
    FILESTREAM m_pFileHandle;
    char*      m_pszFilename;
    bool       m_bIsOpen;
    uint32     m_nCurrentPosition;
    uint32     m_nStartPosition;
    uint32     m_nLength;
    uint32     m_nTrackId;
    bool       m_bIsEncrypted;
    uint32     m_lRefCount;

public:
    CAEDataStream(int32 trackId, char* filename, uint32 startPosition, uint32 length, bool encrypted);
    ~CAEDataStream();

#ifndef NOTSA_NATIVE
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** objout) override;
    ULONG STDMETHODCALLTYPE   AddRef() override;
    ULONG STDMETHODCALLTYPE   Release() override;
    HRESULT STDMETHODCALLTYPE Read(void* dest, ULONG size, ULONG* readed) override;
    HRESULT STDMETHODCALLTYPE Write(const void* src, ULONG size, ULONG* written) override;
    HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER offset, DWORD whence, ULARGE_INTEGER* newOffset) override;
    HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER newSize) override;
    HRESULT STDMETHODCALLTYPE CopyTo(IStream* target, ULARGE_INTEGER size, ULARGE_INTEGER* readed, ULARGE_INTEGER* written) override;
    HRESULT STDMETHODCALLTYPE Commit(DWORD flags) override;
    HRESULT STDMETHODCALLTYPE Revert() override;
    HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD type) override;
    HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
    HRESULT STDMETHODCALLTYPE Stat(STATSTG* statout, DWORD flags) override;
    HRESULT STDMETHODCALLTYPE Clone(IStream** target) override;
#endif

    bool   Initialise();
    size_t FillBuffer(void* dest, size_t size);
    uint32 GetCurrentPosition();
    uint32 Seek(long offset, int32 whence);
    bool   Close();

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CAEDataStream* Constructor(int32 trackID, char* filename, uint32 startPosition, uint32 length, int32 encrypted);
    void           Destructor();
};

VALIDATE_SIZE(CAEDataStream, 0x28);
