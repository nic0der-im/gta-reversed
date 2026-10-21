#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"
#include "RwSkin.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

//! Skinning: the native `RpSkin`, its geometry/atomic plugins and the SA skin chunk reader.
//!
//! The object is `RpSkin` (see `RwSkin.h` for the on-disk layout); it hangs off the geometry as
//! plugin data (`rwID_SKINPLUGIN`) and the animation hierarchy hangs off the atomic under the
//! same id. The GL3 ped pipeline (`pipelines/SAPipelines.cpp`) only uses the object through the
//! `RpSkin*` API: `RpSkinGeometryGetSkin`, `RpSkinAtomicGetHAnimHierarchy`, `RpSkinGetNumBones`,
//! `RpSkinGetSkinToBoneMatrices`, `RpSkinGetVertexBoneWeights` and `RpSkinGetVertexBoneIndices`.
//! No librw object is involved: the game structs are authoritative.

using namespace notsa::fakerw;

namespace {

//! Plugin data offsets inside `RpGeometry` / `RpAtomic`, set by `RpSkinPluginAttach`.
//! `-1` means "not registered", and every accessor then behaves as if there were no skin.
int32 g_geometryOffset = -1;
int32 g_atomicOffset   = -1;

//! Cap for a single allocation driven by split-data arguments.
constexpr RwUInt64 kMaxSplitDataSize = 0x1000000;

//! Atomic plugin data: the hierarchy the skin is posed with, plus the skin pipeline flavour.
struct RpSkinAtomicData {
    RpHAnimHierarchy* hierarchy;
    RpSkinType        type;
};

//-----------------------------------------------------------------------------
// Stream helpers (a stream can be walked from another translation unit only through the API)
//-----------------------------------------------------------------------------

//! Byte offset into `stream`, independent of the stream type.
int32 StreamPosition(RwStream* stream) {
    return stream->type == rwSTREAMMEMORY ? (int32)stream->Type.memory.position : stream->position;
}

bool ReadExact(RwStream* stream, void* out, RwUInt32 size) {
    return RwStreamRead(stream, out, size) == size;
}

bool Skip(RwStream* stream, RwUInt32 count) {
    return count == 0 || RwStreamSkip(stream, count) != nullptr;
}

bool SkipTo(RwStream* stream, int32 target) {
    const int32 current = StreamPosition(stream);
    return current <= target && Skip(stream, (RwUInt32)(target - current));
}

//! Abandons the skin chunk: logs it once and skips to its end, leaving the geometry unskinned.
//! A missing model fraction is much worse than a static one, so a malformed skin chunk must not
//! fail the whole DFF (same rule the native layer follows for plugin chunks it cannot build).
RwStream* SkipSkinChunk(RwStream* stream, int32 start, RwInt32 length, const char* reason) {
    SkipTo(stream, start + length);
    char message[160];
    std::snprintf(
        message, sizeof(message),
        "Rw: skipped DFF chunk 0x%X (SKIN in geometry: %s)", (unsigned)rwID_SKINPLUGIN, reason
    );
    WarnOnce(message);
    return stream;
}

//-----------------------------------------------------------------------------
// Object lifetime
//-----------------------------------------------------------------------------

//! A zeroed skin with the arrays for `numVertices` vertices and `numBones` bones (no copy).
RpSkin* AllocSkin(RwUInt32 numVertices, RwUInt32 numBones) {
    auto* skin = static_cast<RpSkin*>(Malloc(sizeof(RpSkin)));
    if (!skin) {
        return nullptr;
    }
    std::memset(skin, 0, sizeof(RpSkin));
    skin->numVertices            = numVertices;
    skin->numBones               = numBones;
    skin->maxNumWeightsForVertex = 1;

    if (numBones) {
        skin->usedBones          = static_cast<bool*>(Malloc(sizeof(bool) * numBones));
        skin->skinToBoneMatrices = static_cast<RwMatrix*>(Malloc(sizeof(RwMatrix) * numBones));
        if (skin->usedBones) {
            std::memset(skin->usedBones, 0, sizeof(bool) * numBones);
        }
        if (skin->skinToBoneMatrices) {
            std::memset(skin->skinToBoneMatrices, 0, sizeof(RwMatrix) * numBones);
        }
    }
    if (numVertices) {
        skin->vertexBoneIndices = static_cast<RwUInt32*>(Malloc(sizeof(RwUInt32) * numVertices));
        skin->vertexBoneWeights = static_cast<RwMatrixWeights*>(Malloc(sizeof(RwMatrixWeights) * numVertices));
        if (skin->vertexBoneIndices) {
            std::memset(skin->vertexBoneIndices, 0, sizeof(RwUInt32) * numVertices);
        }
        if (skin->vertexBoneWeights) {
            std::memset(skin->vertexBoneWeights, 0, sizeof(RwMatrixWeights) * numVertices);
        }
    }

    const bool arraysMissing =
        (numBones && (!skin->usedBones || !skin->skinToBoneMatrices)) ||
        (numVertices && (!skin->vertexBoneIndices || !skin->vertexBoneWeights));
    if (arraysMissing) {
        RpSkinDestroy(skin);
        return nullptr;
    }
    return skin;
}

//! Fills `usedBones`/`numUsedBones` from the packed vertex indices.
void ComputeUsedBones(RpSkin* skin) {
    if (!skin || !skin->usedBones || !skin->vertexBoneIndices) {
        return;
    }
    for (RwUInt32 vertex = 0; vertex < skin->numVertices; vertex++) {
        const RwUInt8* bones = reinterpret_cast<const RwUInt8*>(&skin->vertexBoneIndices[vertex]);
        for (int32 i = 0; i < 4; i++) {
            if (bones[i] < skin->numBones && !skin->usedBones[bones[i]]) {
                skin->usedBones[bones[i]] = true;
                skin->numUsedBones++;
            }
        }
    }
}

//! True when the skin was created by `_rpSkinSplitDataCreate`.
RwUInt32 SplitDataSize(const RpSkin& skin) {
    return skin.numBones + 2 * (skin.numMeshes + skin.rleSize);
}

//-----------------------------------------------------------------------------
// Plugin callbacks
//-----------------------------------------------------------------------------

void* SkinConstruct(void* object, RwInt32 offsetInObject, RwInt32) {
    *RWPLUGINOFFSET(RpSkin*, object, offsetInObject) = nullptr;
    return object;
}

void* SkinDestruct(void* object, RwInt32 offsetInObject, RwInt32) {
    *RWPLUGINOFFSET(RpSkin*, object, offsetInObject) = RpSkinDestroy(*RWPLUGINOFFSET(RpSkin*, object, offsetInObject));
    return object;
}

void* SkinCopy(void* dstObject, const void* srcObject, RwInt32 offsetInObject, RwInt32) {
    const RpSkin* src = *RWPLUGINOFFSETCONST(RpSkin*, srcObject, offsetInObject);
    RpSkin*&       dst = *RWPLUGINOFFSET(RpSkin*, dstObject, offsetInObject);
    dst = nullptr;
    if (!src) {
        return dstObject;
    }

    RpSkin* copy = AllocSkin(src->numVertices, src->numBones);
    if (!copy) {
        return dstObject;
    }
    copy->numUsedBones            = src->numUsedBones;
    copy->maxNumWeightsForVertex  = src->maxNumWeightsForVertex;
    if (copy->usedBones && src->usedBones) {
        std::memcpy(copy->usedBones, src->usedBones, sizeof(bool) * std::min(src->numUsedBones, src->numBones));
    }
    if (copy->skinToBoneMatrices && src->skinToBoneMatrices) {
        std::memcpy(copy->skinToBoneMatrices, src->skinToBoneMatrices, sizeof(RwMatrix) * src->numBones);
    }
    if (copy->vertexBoneIndices && src->vertexBoneIndices) {
        std::memcpy(copy->vertexBoneIndices, src->vertexBoneIndices, sizeof(RwUInt32) * src->numVertices);
    }
    if (copy->vertexBoneWeights && src->vertexBoneWeights) {
        std::memcpy(copy->vertexBoneWeights, src->vertexBoneWeights, sizeof(RwMatrixWeights) * src->numVertices);
    }
    if (src->splitData) {
        _rpSkinSplitDataCreate(copy, src->boneLimit, src->numBones, src->numMeshes, src->rleSize);
        if (copy->splitData) {
            std::memcpy(copy->splitData, src->splitData, SplitDataSize(*copy));
        }
    }

    dst = copy;
    return dstObject;
}

void* AtomicSkinConstruct(void* object, RwInt32 offsetInObject, RwInt32) {
    auto* data = RWPLUGINOFFSET(RpSkinAtomicData, object, offsetInObject);
    data->hierarchy = nullptr;
    data->type      = rpNASKINTYPE;
    return object;
}

void* AtomicSkinDestruct(void* object, RwInt32 offsetInObject, RwInt32) {
    // The hierarchy belongs to the animation (HAnim frame plugin); only drop the reference.
    auto* data = RWPLUGINOFFSET(RpSkinAtomicData, object, offsetInObject);
    data->hierarchy = nullptr;
    data->type      = rpNASKINTYPE;
    return object;
}

void* AtomicSkinCopy(void* dstObject, const void* srcObject, RwInt32 offsetInObject, RwInt32) {
    *RWPLUGINOFFSET(RpSkinAtomicData, dstObject, offsetInObject) =
        *RWPLUGINOFFSETCONST(RpSkinAtomicData, srcObject, offsetInObject);
    return dstObject;
}

//! Returns the atomic skin plugin data, or null when there is none.
RpSkinAtomicData* GetAtomicData(const RpAtomic* atomic) {
    if (!atomic || g_atomicOffset <= 0) {
        return nullptr;
    }
    return const_cast<RpSkinAtomicData*>(RWPLUGINOFFSETCONST(RpSkinAtomicData, atomic, g_atomicOffset));
}

//-----------------------------------------------------------------------------
// Skin chunk (rwID_SKINPLUGIN inside the geometry's rwID_EXTENSION chunk)
//-----------------------------------------------------------------------------

RwStream* SkinChunkRead(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject, RwInt32) {
    auto* geometry = static_cast<RpGeometry*>(object);
    if (!stream || !geometry) {
        return nullptr;
    }

    const int32 start = StreamPosition(stream);

    // `numBones, numUsedBones, maxNumWeightsForVertex, pad`.
    RwUInt8 header[4];
    if (binaryLength < 4 || !ReadExact(stream, header, sizeof(header))) {
        return SkipSkinChunk(stream, start, binaryLength, "chunk is shorter than its header");
    }

    const RwUInt32 numBones     = header[0];
    const RwUInt32 numUsedBones = header[1]; // never larger than numBones in a valid file
    const RwUInt32 numVertices  = (RwUInt32)std::max(geometry->numVertices, 0);

    // usedBones + indices + weights + matrices, as laid out in `RwSkin.h`.
    const RwUInt64 required =
        4
        + numUsedBones
        + (RwUInt64)numBones * sizeof(RwMatrix)
        + (RwUInt64)numVertices * sizeof(RwUInt32)
        + (RwUInt64)numVertices * sizeof(RwMatrixWeights);
    if (numUsedBones > numBones || required > (RwUInt64)binaryLength) {
        return SkipSkinChunk(stream, start, binaryLength, "chunk is shorter than its header declares");
    }

    RpSkin* skin = AllocSkin(numVertices, numBones);
    if (!skin) {
        return SkipSkinChunk(stream, start, binaryLength, "out of memory");
    }
    skin->numUsedBones           = numUsedBones;
    skin->maxNumWeightsForVertex = (RwUInt8)std::clamp<RwUInt32>(header[2], 1, 4);

    const char* failure = nullptr;

    // usedBones: one byte per used bone (the header byte bounds the temporary).
    if (numUsedBones) {
        RwUInt8 used[256];
        if (!ReadExact(stream, used, numUsedBones)) {
            failure = "truncated used-bone list";
            goto done;
        }
        for (RwUInt32 i = 0; i < numUsedBones; i++) {
            if (used[i] < numBones) {
                skin->usedBones[used[i]] = true;
            }
        }
    }

    if (numVertices) {
        if (!ReadExact(stream, skin->vertexBoneIndices, sizeof(RwUInt32) * numVertices)) {
            failure = "truncated vertex bone indices";
            goto done;
        }
        if (!ReadExact(stream, skin->vertexBoneWeights, sizeof(RwMatrixWeights) * numVertices)) {
            failure = "truncated vertex bone weights";
            goto done;
        }
    }
    if (numBones && !ReadExact(stream, skin->skinToBoneMatrices, sizeof(RwMatrix) * numBones)) {
        failure = "truncated skin-to-bone matrices";
        goto done;
    }

    // Split-data header: `boneLimit, numMeshes, rleSize`. SA writes zeroes here because the
    // pipelines stream one skin per atomic; keep it if present so the `_rpSkin*` queries work.
    if (start + binaryLength - StreamPosition(stream) >= 12) {
        RwInt32 boneLimit = 0, numMeshes = 0, rleSize = 0;
        if (!ReadExact(stream, &boneLimit, sizeof(boneLimit)) ||
            !ReadExact(stream, &numMeshes, sizeof(numMeshes)) ||
            !ReadExact(stream, &rleSize, sizeof(rleSize))) {
            failure = "truncated split-data header";
            goto done;
        }
        boneLimit = std::max(boneLimit, 0);
        numMeshes = std::max(numMeshes, 0);
        rleSize   = std::max(rleSize, 0);

        if (numMeshes > 0) {
            const RwUInt32 remaining = (RwUInt32)std::max(start + binaryLength - StreamPosition(stream), 0);
            const RwUInt32 splitSize = (RwUInt32)numBones + 2 * ((RwUInt32)numMeshes + (RwUInt32)rleSize);
            if (splitSize > remaining) {
                failure = "truncated split data";
                goto done;
            }
            _rpSkinSplitDataCreate(skin, (RwUInt32)boneLimit, numBones, (RwUInt32)numMeshes, (RwUInt32)rleSize);
            if (!skin->splitData || !ReadExact(stream, skin->splitData, splitSize)) {
                failure = "cannot allocate the split data";
                goto done;
            }
        }
    }

    // Nothing of the chunk may be left unread: the next subchunk header follows immediately.
    SkipTo(stream, start + binaryLength);

    if (offsetInObject <= 0) {
        // The plugin was not registered when this geometry was created: nothing to attach to.
        RpSkinDestroy(skin);
        return stream;
    }
    *RWPLUGINOFFSET(RpSkin*, geometry, offsetInObject) = RpSkinDestroy(*RWPLUGINOFFSET(RpSkin*, geometry, offsetInObject));
    *RWPLUGINOFFSET(RpSkin*, geometry, offsetInObject) = skin;
    return stream;

done:
    RpSkinDestroy(skin);
    return SkipSkinChunk(stream, start, binaryLength, failure);
}

} // namespace

//-----------------------------------------------------------------------------
// Plugin attach
//-----------------------------------------------------------------------------

RwBool RpSkinPluginAttach() {
    static bool attached = false;
    if (!attached) {
        // Registering the data plugin first lets `RegisterStream` resolve the skin offset.
        g_geometryOffset = RpGeometryRegisterPlugin(
            sizeof(RpSkin*),
            rwID_SKINPLUGIN,
            SkinConstruct,
            SkinDestruct,
            SkinCopy
        );
        const int32 streamOffset = RpGeometryRegisterPluginStream(
            rwID_SKINPLUGIN,
            SkinChunkRead,
            nullptr,
            nullptr
        );
        g_atomicOffset = RpAtomicRegisterPlugin(
            sizeof(RpSkinAtomicData),
            rwID_SKINPLUGIN,
            AtomicSkinConstruct,
            AtomicSkinDestruct,
            AtomicSkinCopy
        );

        if (g_geometryOffset <= 0 || streamOffset <= 0 || g_atomicOffset <= 0) {
            // Only a duplicate registration can get here; the accessors stay null-safe.
            NOTSA_LOG_ERR(
                "Rw: RpSkinPluginAttach got no plugin storage (geometry {}, stream {}, atomic {})",
                g_geometryOffset, streamOffset, g_atomicOffset
            );
            g_geometryOffset = -1;
            g_atomicOffset   = -1;
        }
        attached = true;
    }
    return TRUE;
}

void RpSkinSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    // Nothing is pooled: every skin is a handful of allocations with the RW memory callbacks.
    UNUSED(blockSize);
    UNUSED(numBlocksToPrealloc);
}

//-----------------------------------------------------------------------------
// Geometry plugin
//-----------------------------------------------------------------------------

RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin) {
    if (!geometry || g_geometryOffset <= 0) {
        return geometry;
    }
    *RWPLUGINOFFSET(RpSkin*, geometry, g_geometryOffset) = skin;
    return geometry;
}

RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry) {
    if (!geometry || g_geometryOffset <= 0) {
        return nullptr;
    }
    return *RWPLUGINOFFSET(RpSkin*, geometry, g_geometryOffset);
}

//-----------------------------------------------------------------------------
// Atomic plugin
//-----------------------------------------------------------------------------

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, RpHAnimHierarchy* hierarchy) {
    if (auto* data = GetAtomicData(atomic)) {
        data->hierarchy = hierarchy;
    }
    return atomic;
}

RpHAnimHierarchy* RpSkinAtomicGetHAnimHierarchy(const RpAtomic* atomic) {
    if (const auto* data = GetAtomicData(atomic)) {
        return data->hierarchy;
    }
    return nullptr;
}

RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type) {
    // The native ped pipeline is selected on the atomic by `ClumpModelInfo` (the GL3 pipeline
    // mirrors all four retail skin pipelines in one shader), so only the value is stored.
    if (auto* data = GetAtomicData(atomic)) {
        data->type = type;
    }
    return atomic;
}

RpSkinType RpSkinAtomicGetType(RpAtomic* atomic) {
    if (const auto* data = GetAtomicData(atomic)) {
        return data->type;
    }
    return rpNASKINTYPE;
}

//-----------------------------------------------------------------------------
// Skin object
//-----------------------------------------------------------------------------

RpSkin* RpSkinCreate(
    RwUInt32        numVertices,
    RwUInt32        numBones,
    RwMatrixWeights* vertexWeights,
    RwUInt32*        vertexIndices,
    RwMatrix*        inverseMatrices
) {
    RpSkin* skin = AllocSkin(numVertices, numBones);
    if (!skin) {
        return nullptr;
    }

    if (numVertices) {
        if (vertexWeights) {
            std::memcpy(skin->vertexBoneWeights, vertexWeights, sizeof(RwMatrixWeights) * numVertices);
        }
        if (vertexIndices) {
            std::memcpy(skin->vertexBoneIndices, vertexIndices, sizeof(RwUInt32) * numVertices);
        }
    }
    if (numBones && inverseMatrices) {
        std::memcpy(skin->skinToBoneMatrices, inverseMatrices, sizeof(RwMatrix) * numBones);
    }

    if (vertexIndices) {
        ComputeUsedBones(skin);
    }

    if (vertexWeights) {
        RwUInt8 maxWeights = 1;
        for (RwUInt32 vertex = 0; vertex < numVertices; vertex++) {
            const RwMatrixWeights& w = skin->vertexBoneWeights[vertex];
            const RwUInt8 count = (w.w0 != 0.0f) + (w.w1 != 0.0f) + (w.w2 != 0.0f) + (w.w3 != 0.0f);
            maxWeights = std::max(maxWeights, count);
        }
        skin->maxNumWeightsForVertex = (RwUInt8)std::clamp<RwUInt32>(maxWeights, 1, 4);
    }
    return skin;
}

RpSkin* RpSkinDestroy(RpSkin* skin) {
    if (!skin) {
        return nullptr;
    }
    Free(skin->usedBones);
    Free(skin->skinToBoneMatrices);
    Free(skin->vertexBoneIndices);
    Free(skin->vertexBoneWeights);
    Free(skin->splitData);
    Free(skin);
    return nullptr;
}

RwUInt32 RpSkinGetNumBones(RpSkin* skin) {
    return skin ? skin->numBones : 0;
}

RwUInt32 RpSkinGetNumUsedBones(RpSkin* skin) {
    return skin ? skin->numUsedBones : 0;
}

bool* RpSkinGetUsedBones(RpSkin* skin) {
    return skin ? skin->usedBones : nullptr;
}

RwUInt8 RpSkinGetMaxNumWeightsForVertex(RpSkin* skin) {
    return skin ? skin->maxNumWeightsForVertex : 0;
}

RwMatrixWeights* RpSkinGetVertexBoneWeights(RpSkin* skin) {
    return skin ? skin->vertexBoneWeights : nullptr;
}

const RwUInt32* RpSkinGetVertexBoneIndices(RpSkin* skin) {
    // Four packed `RwUInt8` per vertex, exposed as `RwUInt32` exactly like the retail API.
    return skin ? skin->vertexBoneIndices : nullptr;
}

const RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin) {
    return skin ? skin->skinToBoneMatrices : nullptr;
}

RpSkin* RpSkinSetSkinToBoneMatrices(RpSkin* skin, const RwMatrix* matrices) {
    if (skin && skin->skinToBoneMatrices && matrices) {
        std::memcpy(skin->skinToBoneMatrices, matrices, sizeof(RwMatrix) * skin->numBones);
    }
    return skin;
}

RwBool RpSkinIsSplit(RpSkin* skin) {
    return (skin && skin->numMeshes != 0) ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------
// Internal API
//-----------------------------------------------------------------------------

RpGeometry* _rpSkinInitialize(RpGeometry* geometry) {
    // Retail builds the split/RLE remaps here, for the D3D skin pipelines. The native GL3 ped
    // pipeline poses all bones at once from `skinToBoneMatrices`, so there is nothing to build;
    // `_rpSkinSplitDataCreate` still creates that data on demand.
    return geometry;
}

RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry) {
    if (geometry) {
        if (RpSkin* skin = RpSkinGeometryGetSkin(geometry)) {
            _rpSkinSplitDataDestroy(skin);
        }
    }
    return geometry;
}

RwUInt8* _rpSkinGetMeshBoneRemapIndices(RpSkin* skin) {
    return (skin && skin->splitData) ? skin->splitData : nullptr;
}

RwUInt8* _rpSkinGetMeshBoneRLECount(RpSkin* skin) {
    return (skin && skin->splitData) ? skin->splitData + skin->numBones : nullptr;
}

RwUInt8* _rpSkinGetMeshBoneRLE(RpSkin* skin) {
    return (skin && skin->splitData) ? skin->splitData + skin->numBones + 2 * skin->numMeshes : nullptr;
}

RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, RwUInt32 boneLimit, RwUInt32 numMatrices, RwUInt32 numMeshes, RwUInt32 numRLE) {
    if (!skin) {
        return nullptr;
    }
    UNUSED(numMatrices); // The matrices are already in `skinToBoneMatrices`.

    _rpSkinSplitDataDestroy(skin);

    skin->boneLimit = boneLimit;
    skin->numMeshes = numMeshes;
    skin->rleSize   = numRLE;
    if (numMeshes) {
        // `remapIndices[numBones]`, `rleCount[numMeshes]`, `rle[numRLE]` (u16 each), as in librw.
        const RwUInt64 size = (RwUInt64)skin->numBones + 2 * ((RwUInt64)numMeshes + numRLE);
        if (size > kMaxSplitDataSize) {
            NOTSA_LOG_ERR("Rw: skin split data of {} bytes is too large", (unsigned)size);
            return nullptr;
        }
        skin->splitData = static_cast<RwUInt8*>(Malloc((size_t)size));
        if (!skin->splitData) {
            _rpSkinSplitDataDestroy(skin); // do not report a split skin without its data
            return nullptr;
        }
        std::memset(skin->splitData, 0, (size_t)size);
    }
    return skin;
}

RwBool _rpSkinSplitDataDestroy(RpSkin* skin) {
    if (!skin) {
        return FALSE;
    }
    Free(skin->splitData);
    skin->splitData = nullptr;
    skin->boneLimit = 0;
    skin->numMeshes = 0;
    skin->rleSize   = 0;
    return TRUE;
}

#endif // NOTSA_NATIVE
