#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

#include "NativePath.h"

//! RenderWare streams over stdio (no retail file system is available natively).
//!
//! The game opens TXD/DFF files through `RwStreamOpen(rwSTREAMFILENAME, ...)` and also uses
//! memory streams for already-loaded data; both are implemented here. Custom streams are
//! forwarded to the caller's callbacks.
//!
//! The file readers (`RwTexture.cpp`) drive librw over these streams, and librw rewinds over
//! chunk headers it already consumed, so `SeekRwStream` exists next to the forward-only public
//! `RwStreamSkip`.

namespace {

constexpr int32 kSeekSet = 0;
constexpr int32 kSeekCur = 1;
constexpr int32 kSeekEnd = 2;

FILE* AsFile(const RwStream* stream) {
    return static_cast<FILE*>(stream->Type.file.fpFile);
}

uint32 UnpackLibraryVersion(uint32 libraryID) {
    if (libraryID & 0xFFFF0000) {
        return ((libraryID >> 14 & 0x3FF00) + 0x30000) | (libraryID >> 16 & 0x3F);
    }
    return libraryID << 8;
}

} // namespace

RwStream* _rwStreamInitialize(
    RwStream*         stream,
    RwBool            rwOwned,
    RwStreamType      type,
    RwStreamAccessType accessType,
    const void*       data
) {
    if (!stream) {
        return nullptr;
    }

    std::memset(stream, 0, sizeof(RwStream));
    stream->type       = type;
    stream->accessType = accessType;
    stream->position   = 0;
    stream->rwOwned    = rwOwned;

    switch (type) {
    case rwSTREAMMEMORY: {
        const auto* memory = static_cast<const RwMemory*>(data);
        if (!memory) {
            return nullptr;
        }
        stream->Type.memory.position  = 0;
        stream->Type.memory.nSize     = memory->length;
        stream->Type.memory.memBlock  = memory->start;
        break;
    }
    case rwSTREAMFILENAME: {
        const char* filename = static_cast<const char*>(data);
        if (!filename) {
            return nullptr;
        }

        const char* mode = accessType == rwSTREAMREAD
            ? "rb"
            : (accessType == rwSTREAMWRITE ? "wb" : "ab");

        const auto path = notsa::native::NormalizePath(filename);
        auto* file = std::fopen(path.c_str(), mode);
        if (!file) {
            NOTSA_LOG_WARN("Rw: failed to open stream file '{}'", path);
            return nullptr;
        }
        stream->Type.file.fpFile = file;
        break;
    }
    case rwSTREAMFILE:
        if (!data) {
            return nullptr;
        }
        stream->Type.file = *static_cast<const RwStreamFile*>(data);
        break;
    case rwSTREAMCUSTOM:
        if (!data) {
            return nullptr;
        }
        stream->Type.custom = *static_cast<const RwStreamCustom*>(data);
        break;
    default:
        return nullptr;
    }

    return stream;
}

RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType accessType, const void* data) {
    auto* stream = static_cast<RwStream*>(notsa::fakerw::Malloc(sizeof(RwStream)));
    if (!stream) {
        return nullptr;
    }

    if (!_rwStreamInitialize(stream, TRUE, type, accessType, data)) {
        notsa::fakerw::Free(stream);
        return nullptr;
    }
    return stream;
}

RwBool RwStreamClose(RwStream* stream, void* data) {
    UNUSED(data);

    if (!stream) {
        return FALSE;
    }

    if (stream->rwOwned) {
        switch (stream->type) {
        case rwSTREAMFILENAME:
        case rwSTREAMFILE:
            if (stream->Type.file.fpFile) {
                std::fclose(AsFile(stream));
            }
            break;
        case rwSTREAMCUSTOM:
            if (stream->Type.custom.sfnclose) {
                stream->Type.custom.sfnclose(stream->Type.custom.data);
            }
            break;
        default:
            break;
        }
        notsa::fakerw::Free(stream);
    }

    return TRUE;
}

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length) {
    if (!stream || !buffer || !length) {
        return 0;
    }

    switch (stream->type) {
    case rwSTREAMMEMORY: {
        auto& memory = stream->Type.memory;
        const RwUInt32 remaining = memory.nSize > memory.position ? memory.nSize - memory.position : 0;
        const RwUInt32 count     = std::min(length, remaining);
        std::memcpy(buffer, memory.memBlock + memory.position, count);
        memory.position += count;
        stream->position += (RwInt32)count;
        return count;
    }
    case rwSTREAMFILE:
    case rwSTREAMFILENAME: {
        const auto count = (RwUInt32)std::fread(buffer, 1, length, AsFile(stream));
        stream->position += (RwInt32)count;
        return count;
    }
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnread) {
            const auto count = stream->Type.custom.sfnread(stream->Type.custom.data, buffer, length);
            stream->position += (RwInt32)count;
            return count;
        }
        return 0;
    default:
        return 0;
    }
}

RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length) {
    if (!stream || !buffer) {
        return nullptr;
    }

    switch (stream->type) {
    case rwSTREAMMEMORY: {
        auto& memory = stream->Type.memory;
        const RwUInt32 remaining = memory.nSize > memory.position ? memory.nSize - memory.position : 0;
        const RwUInt32 count     = std::min(length, remaining);
        std::memcpy(memory.memBlock + memory.position, buffer, count);
        memory.position += count;
        stream->position += (RwInt32)count;
        return stream;
    }
    case rwSTREAMFILE:
    case rwSTREAMFILENAME:
        stream->position += (RwInt32)std::fwrite(buffer, 1, length, AsFile(stream));
        return stream;
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnwrite) {
            stream->Type.custom.sfnwrite(stream->Type.custom.data, buffer, length);
            stream->position += (RwInt32)length;
            return stream;
        }
        return nullptr;
    default:
        return nullptr;
    }
}

RwStream* RwStreamSkip(RwStream* stream, RwUInt32 offset) {
    if (!stream) {
        return nullptr;
    }

    switch (stream->type) {
    case rwSTREAMMEMORY: {
        auto& memory = stream->Type.memory;
        if (memory.position + offset > memory.nSize) {
            memory.position = memory.nSize;
            return nullptr;
        }
        memory.position += offset;
        stream->position += (RwInt32)offset;
        return stream;
    }
    case rwSTREAMFILE:
    case rwSTREAMFILENAME:
        if (std::fseek(AsFile(stream), (long)offset, SEEK_CUR) != 0) {
            return nullptr;
        }
        stream->position += (RwInt32)offset;
        return stream;
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnskip && stream->Type.custom.sfnskip(stream->Type.custom.data, offset)) {
            stream->position += (RwInt32)offset;
            return stream;
        }
        return nullptr;
    default:
        return nullptr;
    }
}

RwStream* RwStreamReadChunkHeaderInfo(RwStream* stream, RwChunkHeaderInfo* chunkHeaderInfo) {
    struct {
        RwUInt32 type;
        RwUInt32 length;
        RwUInt32 libraryID;
    } header{};

    if (RwStreamRead(stream, &header, sizeof(header)) != sizeof(header)) {
        return nullptr;
    }

    if (!chunkHeaderInfo) {
        return stream;
    }

    chunkHeaderInfo->type      = header.type;
    chunkHeaderInfo->length    = header.length;
    chunkHeaderInfo->version   = UnpackLibraryVersion(header.libraryID);
    chunkHeaderInfo->buildNum  = header.libraryID & 0xFFFF;
    chunkHeaderInfo->isComplex = FALSE;
    return stream;
}

RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut) {
    RwChunkHeaderInfo header{};

    while (RwStreamReadChunkHeaderInfo(stream, &header)) {
        if (header.type == rwID_NAOBJECT) {
            return FALSE;
        }

        if (header.type == type) {
            if (lengthOut) {
                *lengthOut = header.length;
            }
            if (versionOut) {
                *versionOut = header.version;
            }
            return TRUE;
        }

        if (!RwStreamSkip(stream, header.length)) {
            return FALSE;
        }
    }

    return FALSE;
}

namespace notsa::fakerw {

RwBool SeekRwStream(RwStream* stream, int32 offset, int32 whence) {
    if (!stream) {
        return FALSE;
    }

    switch (stream->type) {
    case rwSTREAMMEMORY: {
        auto& memory = stream->Type.memory;
        const int64 base = whence == kSeekSet
            ? 0
            : (whence == kSeekEnd ? (int64)memory.nSize : (int64)memory.position);
        const int64 position = std::clamp<int64>(base + offset, 0, (int64)memory.nSize);
        stream->position += (RwInt32)(position - (int64)memory.position);
        memory.position = (RwUInt32)position;
        return TRUE;
    }
    case rwSTREAMFILE:
    case rwSTREAMFILENAME: {
        FILE* file = AsFile(stream);
        if (!file) {
            return FALSE;
        }
        const int stdioWhence = whence == kSeekSet ? SEEK_SET : (whence == kSeekEnd ? SEEK_END : SEEK_CUR);
        if (std::fseek(file, (long)offset, stdioWhence) != 0) {
            return FALSE;
        }
        stream->position = (RwInt32)std::ftell(file);
        return TRUE;
    }
    case rwSTREAMCUSTOM:
        // A custom stream only knows how to skip forward (and the game's own file layer, which
        // sets these callbacks, is not used natively).
        if (offset >= 0 && stream->Type.custom.sfnskip && stream->Type.custom.sfnskip(stream->Type.custom.data, (RwUInt32)offset)) {
            stream->position += offset;
            return TRUE;
        }
        return FALSE;
    default:
        return FALSE;
    }
}

} // namespace notsa::fakerw

#endif // NOTSA_NATIVE
