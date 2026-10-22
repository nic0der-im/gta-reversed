#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"
#include "pipelines/SAPipelines.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

//! DFF streaming: clumps, atomics, geometries, materials and the frame list they hang from.
//!
//! The readers fill the *game* RenderWare structs (`rpworld.h`), never librw objects: the four
//! San Andreas GL3 pipelines read those structs directly. They mirror retail `RpClumpStreamRead`
//! (0x74B420), `RpGeometryStreamRead` (0x74D190), `RpMaterialStreamRead`, `RpAtomicStreamRead`
//! and the split GTA streaming pair `RpClumpGtaStreamRead1/2` (0x72E570/0x72E620) plus
//! `RpClumpGtaCancelStream` (0x72E700).
//!
//! Object plugin data (`rwID_EXTENSION`) is dispatched through the per-type registries, which is
//! what lets the game's own plugin readers (collision, node name, 2d effects, breakable, extra
//! vertex colours, env/spec materials, skin, HAnim) fill their plugin areas.
//!
//! Every reader is strict: an unexpected chunk, a short read or an out-of-range index makes it
//! return `nullptr` after releasing whatever it had built, so a malformed model can never leak.

using namespace notsa::fakerw;

namespace {


constexpr RwUInt32 kChunkHeaderSize = 12;

//! Library versions the retail readers accept (RW 3.4.0.0 .. 3.6.0.3).
constexpr RwUInt32 kMinLibraryVersion = 0x34000;
constexpr RwUInt32 kMaxLibraryVersion = 0x36003;

//! `rwID_MESHEXTENSION`: the bin mesh chunk (`MAKEPLUGINID(VEND_CRITERIONWORLD, 0x0E)`).
constexpr RwUInt32 kMeshChunkID = MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x0E);

//! Sanity cap for a single allocation driven by file data (vertices or indices).
constexpr RwUInt32 kMaxStreamArrayElements = 0x1000000;

//! Serial number handed to every mesh header (`_rpMeshGetNextSerialNumber`).
RwUInt16 g_meshSerialNum = 0;

//-----------------------------------------------------------------------------
// Stream helpers
//-----------------------------------------------------------------------------

//! Byte offset into `stream`, independent of the stream type.
//!
//! For memory streams this is the union slot at +0x0C (`Type.memory.position`), which is what
//! the retail GTA readers save; `stream->position` is the same byte count for every stream type
//! this layer creates.
int32 StreamPosition(RwStream* stream) {
    return stream->type == rwSTREAMMEMORY ? (int32)stream->Type.memory.position : stream->position;
}

//! Reads the next chunk header without consuming it.
bool PeekChunkHeader(RwStream* stream, RwChunkHeaderInfo& header) {
    if (!stream) {
        return false;
    }
    if (!RwStreamReadChunkHeaderInfo(stream, &header)) {
        return false; // end of stream (or a truncated header)
    }
    if (!SeekRwStream(stream, -(int32)kChunkHeaderSize, 1)) {
        WarnOnce("Rw: the stream cannot be rewound, so DFF plugin chunks will not be read");
        return false;
    }
    return true;
}

//! Skips `count` bytes (a zero skip always succeeds).
bool Skip(RwStream* stream, RwUInt32 count) {
    return count == 0 || RwStreamSkip(stream, count) != nullptr;
}

//! Advances the stream to `target` (used to recover from a failed chunk read).
bool SkipTo(RwStream* stream, int32 target) {
    const int32 current = StreamPosition(stream);
    if (current > target) {
        return false;
    }
    return Skip(stream, (RwUInt32)(target - current));
}

bool ReadExact(RwStream* stream, void* out, RwUInt32 size) {
    return RwStreamRead(stream, out, size) == size;
}

//! Logs (once) that a chunk of `chunkID` in `what` could not be read.
void BadChunk(const char* chunkName, uint32 chunkID, const char* what) {
    char message[128];
    std::snprintf(message, sizeof(message), "Rw: bad DFF chunk 0x%X (%s in %s)", chunkID, chunkName, what);
    WarnOnce(message);
}

//! Reads the optional `rwID_EXTENSION` chunk that follows an object's own data.
//!
//! The retail writer emits one extension chunk per frame/atomic/clump/geometry/material (often
//! empty), but a missing one is legal: only a *present* extension is consumed, so a reader can
//! never eat the next sibling object while looking for it.
bool ReadPlugins(RwStream* stream, const PluginRegistry& registry, void* object) {
    RwChunkHeaderInfo header{};
    if (!PeekChunkHeader(stream, header)) {
        // End of stream: the object simply has no plugin data. A truncated *object* is caught by
        // the chunk reads above; the caller's next `RwStreamFindChunk` reports a truncated file.
        return true;
    }
    if (header.type != rwID_EXTENSION) {
        return true;
    }
    return registry.StreamReadExtension(stream, object);
}

//-----------------------------------------------------------------------------
// Frame list
//-----------------------------------------------------------------------------

//! The frame record as stored in the file: a 4x3 matrix, the parent index and the matrix flags.
struct FrameRecord {
    RwV3d   right, up, at, pos;
    RwInt32 parentIndex;
    RwUInt32 flags;
};
static_assert(sizeof(FrameRecord) == 0x38, "Frame records are 0x38 bytes on disk");

float Length(const RwV3d& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float Dot(const RwV3d& a, const RwV3d& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

//! `_rwMatrixNormalError`: how far the three axes are from unit length.
float NormalError(const RwV3d& right, const RwV3d& up, const RwV3d& at) {
    return std::max({
        std::fabs(1.0f - Length(right)),
        std::fabs(1.0f - Length(up)),
        std::fabs(1.0f - Length(at)),
    });
}

//! `_rwMatrixOrthogonalError`: how far the three axes are from being perpendicular.
float OrthogonalError(const RwV3d& right, const RwV3d& up, const RwV3d& at) {
    return std::max({ std::fabs(Dot(right, up)), std::fabs(Dot(right, at)), std::fabs(Dot(up, at)) });
}

//! Determinant of the 3x3 part, computed as `right . (up x at)`.
float Determinant(const RwV3d& right, const RwV3d& up, const RwV3d& at) {
    return right.x * (up.y * at.z - up.z * at.y)
         + right.y * (up.z * at.x - up.x * at.z)
         + right.z * (up.x * at.y - up.y * at.x);
}

//! Applies the retail matrix-flag fixup of `_rwFrameListStreamRead` (0x807970).
RwUInt32 SanitizeMatrixFlags(const FrameRecord& record) {
    const bool orthonormal =
        NormalError(record.right, record.up, record.at) <= 0.01f &&
        OrthogonalError(record.right, record.up, record.at) <= 0.01f &&
        Determinant(record.right, record.up, record.at) >= 0.99f;

    // Always drop the internal "identity" marker; an invalid matrix also loses its type flags.
    return orthonormal ? (record.flags & 0xFFFDFFFF) : (record.flags & 0xFFFDFFFC);
}

//! Frees the frame array *and* destroys every frame hierarchy it roots.
//!
//! Used by the failure paths and by `RpClumpGtaCancelStream`. `_rwFrameListDeinitialize` keeps
//! the frames alive (the clump adopts them on a successful read).
void DestroyFrameList(rwFrameList& frameList);




//! Reads a frame list (`_rwFrameListStreamRead`, 0x807970): the `STRUCT` chunk with one record
//! per frame, then that frame's plugin chunks.



void DestroyFrameList(rwFrameList& frameList) {
    if (!frameList.frames) {
        return;
    }

    // `parentIndex` is not available here, but every frame stores its parent, so collect the
    // roots before destroying anything: destroying a root destroys its subtree.
    std::vector<RwFrame*> roots;
    roots.reserve((size_t)frameList.numFrames);
    for (int32 i = 0; i < frameList.numFrames; i++) {
        if (frameList.frames[i] && !frameList.frames[i]->object.parent) {
            roots.push_back(frameList.frames[i]);
        }
    }
    for (RwFrame* root : roots) {
        RwFrameDestroyHierarchy(root);
    }

    _rwFrameListDeinitialize(&frameList);
}

//-----------------------------------------------------------------------------
// Material
//-----------------------------------------------------------------------------

//! Reads one material chunk info (`_rpMaterialChunkInfoRead`).
RpMaterialChunkInfo* ReadMaterialChunkInfo(RwStream* stream, RpMaterialChunkInfo& info, RwInt32& bytesRead) {
    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, nullptr)) {
        BadChunk("STRUCT", rwID_STRUCT, "material");
        return nullptr;
    }
    if (length < 20) {
        BadChunk("STRUCT", rwID_STRUCT, "material");
        return nullptr;
    }

    std::memset(&info, 0, sizeof(info));

    const RwUInt32 toRead = std::min<RwUInt32>(length, sizeof(RpMaterialChunkInfo));
    if (!ReadExact(stream, &info, toRead)) {
        return nullptr;
    }
    if (toRead < sizeof(RpMaterialChunkInfo)) {
        // Before RW 3.4 the surface properties lived in the geometry struct instead.
        info.surfaceProps = { 1.0f, 1.0f, 1.0f };
    }
    if (!Skip(stream, length - toRead)) {
        return nullptr;
    }

    bytesRead = (RwInt32)length;
    return &info;
}













//! Reads the texture chunk of a textured material, bounded by the chunk's own length.
bool ReadMaterialTexture(RwStream* stream, RpMaterial* material) {
    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_TEXTURE, &length, nullptr)) {
        BadChunk("TEXTURE", rwID_TEXTURE, "material");
        return false;
    }

    const int32 start = StreamPosition(stream);
    material->texture = RwTextureStreamRead(stream);

    // `RwTextureStreamRead` stops after the mask string; skip anything the chunk still holds so
    // the material's extension stays in sync.
    const int32 consumed = StreamPosition(stream) - start;
    if (consumed < 0 || (RwUInt32)consumed > length) {
        BadChunk("TEXTURE", rwID_TEXTURE, "material");
        return false;
    }
    return Skip(stream, length - (RwUInt32)consumed);
}




//-----------------------------------------------------------------------------
// Material list
//-----------------------------------------------------------------------------

























//! Reads a MATLIST chunk (the `STRUCT` with the material count, then one chunk per material).



//-----------------------------------------------------------------------------
// Geometry
//-----------------------------------------------------------------------------































//-----------------------------------------------------------------------------
// Mesh (bin mesh chunk)
//-----------------------------------------------------------------------------




//! The `binMesh` chunk header: the mesh header flags, then one entry per mesh and the indices.
struct MeshStreamHeader {
    RwUInt32 flags;
    RwUInt32 numMeshes;
    RwUInt32 totalIndices;
};
static_assert(sizeof(MeshStreamHeader) == 12);

//! Reads a bin mesh chunk body into `geometry->mesh` (one allocation for header, meshes and
//! indices, exactly the layout `InstanceAtomic` walks).



//! Stream callback for the `binMesh` subchunk of the geometry extension.
RwStream* MeshChunkRead(RwStream* stream, RwInt32 length, void* object, RwInt32, RwInt32) {
    if (!stream || !object || length < (RwInt32)sizeof(MeshStreamHeader)) {
        return nullptr;
    }

    const int32 start = StreamPosition(stream);

    // Validate the chunk length against the header before allocating anything.
    MeshStreamHeader header{};
    if (!ReadExact(stream, &header, sizeof(header)) || !SeekRwStream(stream, -(int32)sizeof(header), 1)) {
        return nullptr;
    }
    const RwUInt64 expected = sizeof(MeshStreamHeader)
                            + (RwUInt64)header.numMeshes * sizeof(RwUInt32) * 2
                            + (RwUInt64)header.totalIndices * sizeof(RwUInt32);
    if (expected != (RwUInt64)length) {
        BadChunk("BINMESH", kMeshChunkID, "geometry");
        SkipTo(stream, start + length);
        return nullptr;
    }

    if (!_rpMeshRead(stream, object, &static_cast<RpGeometry*>(object)->matList)) {
        BadChunk("BINMESH", kMeshChunkID, "geometry");
        SkipTo(stream, start + length);
        return nullptr;
    }
    return stream;
}

//! Registers the bin mesh reader on the geometry plugin registry (idempotent).
void EnsureMeshStreamReader() {
    static bool registered = false;
    if (!registered) {
        GeometryPlugins().RegisterStream(kMeshChunkID, MeshChunkRead, nullptr, nullptr);
        registered = true;
    }
}

//-----------------------------------------------------------------------------
// Geometry stream reader
//-----------------------------------------------------------------------------




//! `RpGeometrySortByMaterial`: duplicates vertices so each material owns a contiguous range.



//-----------------------------------------------------------------------------
// Atomics
//-----------------------------------------------------------------------------

//! Atomic chunk info (`_rpAtomicChunkInfoRead`); the geometry list layout depends on `version`.
struct AtomicChunkInfo {
    RwInt32  frameIndex;
    RwInt32  geometryIndex; //!< Only for RW 3.4+; before that this slot holds the flags.
    RwUInt32 flags;
    RwUInt32 pad;
};

//! Flags of an atomic chunk read with `info` (the layout differs before RW 3.4).
RwUInt8 AtomicFlags(const AtomicChunkInfo& info, RwUInt32 version) {
    const RwUInt32 flags = version < 0x30400 ? (RwUInt32)info.geometryIndex : info.flags;
    return (RwUInt8)(flags & 0xFF);
}

bool ReadAtomicChunkInfo(RwStream* stream, AtomicChunkInfo& info, RwUInt32& version) {
    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, &version) || length < 12) {
        BadChunk("STRUCT", rwID_STRUCT, "atomic");
        return false;
    }

    std::memset(&info, 0, sizeof(info));
    const RwUInt32 toRead = std::min<RwUInt32>(length, sizeof(info));
    if (!ReadExact(stream, &info, toRead) || !Skip(stream, length - toRead)) {
        return false;
    }
    return true;
}

//! Reads the geometry that follows an atomic chunk when the file carries it inline (pre-3.4, or
//! a stand-alone atomic).
RpGeometry* ReadInlineAtomicGeometry(RwStream* stream) {
    if (!RwStreamFindChunk(stream, rwID_GEOMETRY, nullptr, nullptr)) {
        BadChunk("GEOMETRY", rwID_GEOMETRY, "atomic");
        return nullptr;
    }
    return RpGeometryStreamRead(stream);
}

//! Reads one `rwID_ATOMIC` chunk body that belongs to a clump: the frame and geometry come from
//! the clump's lists.
RpAtomic* AtomicStreamReadFromClump(RwStream* stream, const rwFrameList& frames, RpGeometry** geometries, RwInt32 numGeometries) {
    AtomicChunkInfo info{};
    RwUInt32        version = 0;
    if (!ReadAtomicChunkInfo(stream, info, version)) {
        return nullptr;
    }

    auto* atomic = RpAtomicCreate();
    if (!atomic) {
        return nullptr;
    }

    atomic->object.object.flags = AtomicFlags(info, version);

    RpGeometry* geometry = nullptr;
    if (version < 0x30400) {
        geometry = ReadInlineAtomicGeometry(stream);
        if (!geometry) {
            RpAtomicDestroy(atomic);
            return nullptr;
        }
        RpAtomicSetGeometry(atomic, geometry, 0);
        RpGeometryDestroy(geometry); // the atomic holds the reference now
    } else {
        if (info.geometryIndex < 0 || info.geometryIndex >= numGeometries || !geometries[info.geometryIndex]) {
            BadChunk("ATOMIC", rwID_ATOMIC, "atomic geometry index");
            RpAtomicDestroy(atomic);
            return nullptr;
        }
        RpAtomicSetGeometry(atomic, geometries[info.geometryIndex], 0);
    }

    if (info.frameIndex < 0 || info.frameIndex >= frames.numFrames || !frames.frames[info.frameIndex]) {
        BadChunk("ATOMIC", rwID_ATOMIC, "atomic frame index");
        RpAtomicDestroy(atomic);
        return nullptr;
    }
    RpAtomicSetFrame(atomic, frames.frames[info.frameIndex]);

    if (!ReadPlugins(stream, AtomicPlugins(), atomic)) {
        BadChunk("EXTENSION", rwID_EXTENSION, "atomic");
        RpAtomicDestroy(atomic);
        return nullptr;
    }

    return atomic;
}







//-----------------------------------------------------------------------------
// Clumps
//-----------------------------------------------------------------------------




//! Reads the `rwID_CLUMP` struct (`numAtomics`, and for 3.4+ `numLights`, `numCameras`).
bool ReadClumpChunkInfo(RwStream* stream, RpClumpChunkInfo& info, RwUInt32& version) {
    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, &version) || length < sizeof(RwInt32)) {
        BadChunk("STRUCT", rwID_STRUCT, "clump");
        return false;
    }

    std::memset(&info, 0, sizeof(info));
    const RwUInt32 toRead = std::min<RwUInt32>(length, sizeof(info));
    if (!ReadExact(stream, &info, toRead) || !Skip(stream, length - toRead)) {
        return false;
    }
    return true;
}

//! Reads one `rwID_LIGHT` chunk of a clump and attaches it to a frame.
bool ReadClumpLight(RwStream* stream, const rwFrameList& frames, RpClump* clump) {
    RwUInt32 structLength = 0;
    RwInt32  frameIndex   = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &structLength, nullptr) || structLength < sizeof(frameIndex)) {
        BadChunk("STRUCT", rwID_STRUCT, "clump light");
        return false;
    }
    if (!ReadExact(stream, &frameIndex, sizeof(frameIndex)) || !Skip(stream, structLength - sizeof(frameIndex))) {
        return false;
    }

    // `RpLightStreamRead` finds and consumes the `rwID_LIGHT` chunk itself, so its header must
    // not be eaten here: doing so made every light-bearing model fail to load.
    RpLight* light = RpLightStreamRead(stream);
    if (!light) {
        // `RpLightStreamRead` is not implemented yet (RwStubs) -- drop the light, keep the model.
        WarnOnce("Rw: a clump light could not be read; the light was skipped");
        return true;
    }

    if (frameIndex >= 0 && frameIndex < frames.numFrames && frames.frames[frameIndex]) {
        _rwObjectHasFrameSetFrame(light, frames.frames[frameIndex]);
    }

    if (!RpClumpAddLight(clump, light)) {
        // No clump light list yet (the game's `RpClump` has no light link): free the light.
        RpLightDestroy(light);
    }
    return true;
}

//! Skips one `rwID_CAMERA` chunk of a clump (no native camera streaming yet).
bool SkipClumpCamera(RwStream* stream) {
    RwUInt32 structLength = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &structLength, nullptr) ||
        !Skip(stream, structLength)) {
        BadChunk("STRUCT", rwID_STRUCT, "clump camera");
        return false;
    }

    RwUInt32 cameraLength = 0;
    if (!RwStreamFindChunk(stream, rwID_CAMERA, &cameraLength, nullptr) || !Skip(stream, cameraLength)) {
        BadChunk("CAMERA", rwID_CAMERA, "clump");
        return false;
    }

    WarnOnce("Rw: clump cameras are not supported; the camera was skipped");
    return true;
}







//-----------------------------------------------------------------------------
// GTA's split clump streaming (retail 0x72E570 / 0x72E620 / 0x72E700)
//-----------------------------------------------------------------------------
//
// `CStreaming` can start a clump read in one pass and finish it in another, over a buffer that
// covers the same model. The state between the two calls lives in these statics: only one clump
// may be mid-read at a time (which is what the retail globals at 0xC87AE8..0xC87B00 encode).

struct GtaClumpStreamState {
    RpClumpChunkInfo chunkInfo{};
    rwFrameList      frameList{};
    int32            resumePosition = 0;
    RpGeometry**     geometries     = nullptr;
    int32            numGeometries  = 0;
    int32            numGeometriesRead = 0;
};

GtaClumpStreamState g_gtaClumpStream;

//! Destroys the partially read geometry list (retail `GeometryListDestroy`, 0x72E360).
void DestroyGeometryList() {
    for (int32 i = 0; i < g_gtaClumpStream.numGeometries; i++) {
        if (g_gtaClumpStream.geometries && g_gtaClumpStream.geometries[i]) {
            RpGeometryDestroy(g_gtaClumpStream.geometries[i]);
        }
    }
    Free(g_gtaClumpStream.geometries);
    g_gtaClumpStream.geometries     = nullptr;
    g_gtaClumpStream.numGeometries  = 0;
    g_gtaClumpStream.numGeometriesRead = 0;
}

//! Reads the geometry-list header and the first half of its geometries (0x72E140).
bool GeometryListStreamReadHalf(RwStream* stream) {
    g_gtaClumpStream.numGeometriesRead = 0;

    RwUInt32 listStructLength = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &listStructLength, nullptr) ||
        listStructLength < sizeof(RwInt32)) {
        BadChunk("STRUCT", rwID_STRUCT, "geometry list");
        return false;
    }

    RwInt32 count = 0;
    if (!ReadExact(stream, &count, sizeof(count)) || !Skip(stream, listStructLength - sizeof(count))) {
        return false;
    }
    if (count < 0 || count > (RwInt32)kMaxStreamArrayElements) {
        BadChunk("GEOMETRYLIST", rwID_GEOMETRYLIST, "clump");
        return false;
    }

    g_gtaClumpStream.numGeometries     = count;
    g_gtaClumpStream.numGeometriesRead = count / 2;

    if (count > 0) {
        g_gtaClumpStream.geometries = static_cast<RpGeometry**>(Malloc(sizeof(RpGeometry*) * (size_t)count));
        if (!g_gtaClumpStream.geometries) {
            return false;
        }
        std::memset(g_gtaClumpStream.geometries, 0, sizeof(RpGeometry*) * (size_t)count);
    }

    for (int32 i = 0; i < g_gtaClumpStream.numGeometriesRead; i++) {
        if (!RwStreamFindChunk(stream, rwID_GEOMETRY, nullptr, nullptr)) {
            BadChunk("GEOMETRY", rwID_GEOMETRY, "geometry list");
            return false;
        }
        g_gtaClumpStream.geometries[i] = RpGeometryStreamRead(stream);
        if (!g_gtaClumpStream.geometries[i]) {
            return false;
        }
    }
    return true;
}

//! Reads the geometries the first call left (0x72E220).
bool GeometryListStreamReadRest(RwStream* stream) {
    for (int32 i = g_gtaClumpStream.numGeometriesRead; i < g_gtaClumpStream.numGeometries; i++) {
        if (!RwStreamFindChunk(stream, rwID_GEOMETRY, nullptr, nullptr)) {
            BadChunk("GEOMETRY", rwID_GEOMETRY, "geometry list");
            return false;
        }
        g_gtaClumpStream.geometries[i] = RpGeometryStreamRead(stream);
        if (!g_gtaClumpStream.geometries[i]) {
            return false;
        }
    }
    return true;
}

} // namespace

rwFrameList* _rwFrameListDeinitialize(rwFrameList* frameList) {
    if (!frameList) {
        return nullptr;
    }
    Free(frameList->frames);
    frameList->frames    = nullptr;
    frameList->numFrames = 0;
    return frameList;
}

rwFrameList* _rwFrameListStreamRead(RwStream* stream, rwFrameList* frameList) {
    if (!stream || !frameList) {
        return nullptr;
    }

    frameList->frames    = nullptr;
    frameList->numFrames = 0;

    RwUInt32 structLength = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &structLength, nullptr) || structLength < sizeof(RwInt32)) {
        BadChunk("STRUCT", rwID_STRUCT, "frame list");
        return nullptr;
    }

    RwInt32 numFrames = 0;
    if (!ReadExact(stream, &numFrames, sizeof(numFrames))) {
        return nullptr;
    }
    constexpr RwInt32 kMaxFrames = 0x10000;
    if (numFrames < 0 || numFrames > kMaxFrames ||
        (RwUInt64)numFrames * sizeof(FrameRecord) > structLength - sizeof(numFrames)) {
        BadChunk("STRUCT", rwID_STRUCT, "frame list");
        return nullptr;
    }

    if (numFrames > 0) {
        frameList->frames = static_cast<RwFrame**>(Malloc(sizeof(RwFrame*) * (size_t)numFrames));
        if (!frameList->frames) {
            return nullptr;
        }
        std::memset(frameList->frames, 0, sizeof(RwFrame*) * (size_t)numFrames);
    }
    frameList->numFrames = numFrames;

    std::vector<RwInt32> parents((size_t)numFrames, -1);

    for (RwInt32 i = 0; i < numFrames; i++) {
        FrameRecord record{};
        if (!ReadExact(stream, &record, sizeof(record))) {
            goto fail;
        }

        RwFrame* frame = RwFrameCreate();
        if (!frame) {
            goto fail;
        }
        frameList->frames[i] = frame;
        parents[i]           = record.parentIndex;

        frame->modelling.right = record.right;
        frame->modelling.up    = record.up;
        frame->modelling.at    = record.at;
        frame->modelling.pos   = record.pos;
        frame->modelling.flags = SanitizeMatrixFlags(record);
        frame->ltm             = frame->modelling;
    }

    // Link after every frame exists: a record may name a parent that comes later in the list.
    for (RwInt32 i = 0; i < numFrames; i++) {
        const RwInt32 parent = parents[i];
        if (parent >= 0 && parent < i && frameList->frames[parent]) {
            RwFrameAddChild(frameList->frames[parent], frameList->frames[i]);
        }
    }

    if (!Skip(stream, structLength - sizeof(numFrames) - (RwUInt32)numFrames * sizeof(FrameRecord))) {
        goto fail;
    }

    for (RwInt32 i = 0; i < numFrames; i++) {
        if (!ReadPlugins(stream, FramePlugins(), frameList->frames[i])) {
            BadChunk("EXTENSION", rwID_EXTENSION, "frame");
            goto fail;
        }
    }

    return frameList;

fail:
    DestroyFrameList(*frameList);
    return nullptr;
}

RpMaterialChunkInfo* _rpMaterialChunkInfoRead(RwStream* stream, RpMaterialChunkInfo* materialChunkInfo, RwInt32* bytesRead) {
    if (!materialChunkInfo) {
        return nullptr;
    }
    RwInt32 read = 0;
    if (!ReadMaterialChunkInfo(stream, *materialChunkInfo, read)) {
        return nullptr;
    }
    if (bytesRead) {
        *bytesRead = read;
    }
    return materialChunkInfo;
}

RpMaterial* RpMaterialCreate() {
    auto* material = static_cast<RpMaterial*>(AllocObject(sizeof(RpMaterial), MaterialPlugins()));
    if (!material) {
        return nullptr;
    }

    material->texture      = nullptr;
    material->color        = { 255, 255, 255, 255 };
    material->pipeline     = nullptr;
    material->surfaceProps = { 1.0f, 1.0f, 1.0f };
    material->refCount     = 1;

    MaterialPlugins().Construct(material, sizeof(RpMaterial));
    return material;
}

RwBool RpMaterialDestroy(RpMaterial* material) {
    if (!material) {
        return FALSE;
    }

    if (material->refCount > 1) {
        material->refCount--;
        return TRUE;
    }

    if (material->texture) {
        RwTextureDestroy(material->texture);
        material->texture = nullptr;
    }

    FreeObject(material, sizeof(RpMaterial), MaterialPlugins());
    return TRUE;
}

RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture) {
    if (material) {
        material->texture = texture;
    }
    return material;
}

RpMaterial* RpMaterialStreamRead(RwStream* stream) {
    RpMaterialChunkInfo info{};
    RwInt32             bytesRead = 0;
    if (!ReadMaterialChunkInfo(stream, info, bytesRead)) {
        return nullptr;
    }

    auto* material = RpMaterialCreate();
    if (!material) {
        return nullptr;
    }

    material->color        = info.color;
    material->surfaceProps = info.surfaceProps;

    if (info.textured) {
        // A material whose texture is not in any loaded dictionary keeps a null texture (and
        // `RwTextureRead` already logged it); that must not reject the whole model.
        if (!ReadMaterialTexture(stream, material)) {
            RpMaterialDestroy(material);
            return nullptr;
        }
    }

    if (!ReadPlugins(stream, MaterialPlugins(), material)) {
        BadChunk("EXTENSION", rwID_EXTENSION, "material");
        RpMaterialDestroy(material);
        return nullptr;
    }

    return material;
}

RpMaterialList* _rpMaterialListInitialize(RpMaterialList* matList) {
    if (!matList) {
        return nullptr;
    }
    matList->materials     = nullptr;
    matList->numMaterials  = 0;
    matList->space         = 0;
    return matList;
}

//! Releases the material array only, matching retail `_rpMaterialListDeinitialize` (0x74E150).
//! The list does not own the materials it points at: `CVehicleModelInfo::ReduceMaterialsInVehicle`
//! builds a temporary list out of the clump's live materials, and destroying them there would
//! leave every geometry of that clump with dangling material pointers.
RpMaterialList* _rpMaterialListDeinitialize(RpMaterialList* matList) {
    if (!matList) {
        return nullptr;
    }

    Free(matList->materials);

    matList->materials    = nullptr;
    matList->numMaterials = 0;
    matList->space        = 0;
    return matList;
}

//! Destroys the materials the list owns (dropping their reference), then releases the array.
//! Used where the list really owns them: `RpGeometryDestroy` and a failed material-list read.
static RpMaterialList* _rpMaterialListDestroy(RpMaterialList* matList) {
    if (!matList) {
        return nullptr;
    }

    for (int32 i = 0; i < matList->numMaterials; i++) {
        RpMaterialDestroy(matList->materials[i]);
    }
    return _rpMaterialListDeinitialize(matList);
}

RpMaterial** _rpMaterialListAlloc(RwUInt32 count) {
    if (count == 0) {
        return nullptr;
    }
    auto** materials = static_cast<RpMaterial**>(Malloc(sizeof(RpMaterial*) * count));
    if (materials) {
        std::memset(materials, 0, sizeof(RpMaterial*) * count);
    }
    return materials;
}

RpMaterial* _rpMaterialListGetMaterial(const RpMaterialList* matList, RwInt32 matIndex) {
    if (!matList || !matList->materials || matIndex < 0 || matIndex >= matList->numMaterials) {
        return nullptr;
    }
    return matList->materials[matIndex];
}

RpMaterialList* _rpMaterialListSetSize(RpMaterialList* matList, RwInt32 size) {
    if (!matList || size < 0) {
        return nullptr;
    }
    if (size == 0) {
        Free(matList->materials);
        matList->materials    = nullptr;
        matList->space        = 0;
        matList->numMaterials = 0;
        return matList;
    }
    if (size == matList->space) {
        return matList;
    }

    auto** materials = static_cast<RpMaterial**>(Realloc(matList->materials, sizeof(RpMaterial*) * (size_t)size));
    if (!materials) {
        return nullptr;
    }

    if (size > matList->space) {
        std::memset(materials + matList->space, 0, sizeof(RpMaterial*) * (size_t)(size - matList->space));
    }

    matList->materials = materials;
    matList->space     = size;
    if (matList->numMaterials > size) {
        matList->numMaterials = size;
    }
    return matList;
}

RpMaterialList* _rpMaterialListCopy(RpMaterialList* matListOut, const RpMaterialList* matListIn) {
    if (!matListOut || !matListIn) {
        return nullptr;
    }
    if (matListOut == matListIn) {
        return matListOut;
    }

    _rpMaterialListInitialize(matListOut);
    if (matListIn->numMaterials == 0) {
        return matListOut;
    }
    if (!_rpMaterialListSetSize(matListOut, matListIn->numMaterials)) {
        return nullptr;
    }

    for (int32 i = 0; i < matListIn->numMaterials; i++) {
        matListOut->materials[i] = matListIn->materials[i];
        if (matListOut->materials[i]) {
            matListOut->materials[i]->refCount++;
        }
    }
    matListOut->numMaterials = matListIn->numMaterials;
    return matListOut;
}

RwInt32 _rpMaterialListAppendMaterial(RpMaterialList* matList, RpMaterial* material) {
    if (!matList || !material) {
        return -1;
    }

    if (matList->numMaterials >= matList->space) {
        const RwInt32 newSize = matList->space > 0 ? matList->space * 2 : 4;
        if (!_rpMaterialListSetSize(matList, newSize)) {
            return -1;
        }
    }

    matList->materials[matList->numMaterials] = material;
    // The list takes its own reference (retail `_rpMaterialListAppendMaterial`, 0x74E350);
    // callers that created the material drop theirs with `RpMaterialDestroy`.
    material->refCount++;
    return matList->numMaterials++;
}

RwInt32 _rpMaterialListFindMaterialIndex(const RpMaterialList* matList, const RpMaterial* material) {
    if (!matList || !matList->materials) {
        return -1;
    }
    for (int32 i = 0; i < matList->numMaterials; i++) {
        if (matList->materials[i] == material) {
            return i;
        }
    }
    return -1;
}

RpMaterialList* _rpMaterialListStreamRead(RwStream* stream, RpMaterialList* matList) {
    if (!matList) {
        return nullptr;
    }

    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_MATLIST, &length, nullptr)) {
        BadChunk("MATLIST", rwID_MATLIST, "geometry");
        return nullptr;
    }

    RwUInt32 structLength = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &structLength, nullptr) || structLength < sizeof(RwInt32)) {
        BadChunk("STRUCT", rwID_STRUCT, "material list");
        return nullptr;
    }

    RwInt32 numMaterials = 0;
    if (!ReadExact(stream, &numMaterials, sizeof(numMaterials)) ||
        !Skip(stream, structLength - sizeof(numMaterials))) {
        return nullptr;
    }
    if (numMaterials < 0 || numMaterials > (RwInt32)kMaxStreamArrayElements) {
        BadChunk("MATLIST", rwID_MATLIST, "geometry");
        return nullptr;
    }

    _rpMaterialListInitialize(matList);
    for (RwInt32 i = 0; i < numMaterials; i++) {
        if (!RwStreamFindChunk(stream, rwID_MATERIAL, nullptr, nullptr)) {
            BadChunk("MATERIAL", rwID_MATERIAL, "material list");
            goto fail;
        }
        if (RpMaterial* material = RpMaterialStreamRead(stream)) {
            const bool appended = _rpMaterialListAppendMaterial(matList, material) >= 0;
            // The list keeps its own reference now, so drop the reader's one (retail
            // `_rpMaterialListStreamRead`, 0x74E600, does the same).
            RpMaterialDestroy(material);
            if (appended) {
                continue;
            }
        }
        goto fail;
    }
    return matList;

fail:
    _rpMaterialListDestroy(matList);
    return nullptr;
}

RwInt32 RpGeometryAddMorphTargets(RpGeometry* geometry, RwInt32 mtcount) {
    if (!geometry) {
        return 0;
    }
    if (mtcount <= 0) {
        return geometry->numMorphTargets;
    }

    const RwInt32 oldCount = geometry->numMorphTargets;
    const RwInt32 newCount = oldCount + mtcount;

    auto* targets = static_cast<RpMorphTarget*>(Realloc(geometry->morphTarget, sizeof(RpMorphTarget) * (size_t)newCount));
    if (!targets) {
        return oldCount;
    }
    geometry->morphTarget = targets;
    std::memset(targets + oldCount, 0, sizeof(RpMorphTarget) * (size_t)mtcount);

    for (RwInt32 i = oldCount; i < newCount; i++) {
        auto& target = targets[i];
        target.parentGeom     = geometry;
        target.boundingSphere = {};
        target.verts          = nullptr;
        target.normals        = nullptr;

        if (geometry->numVertices > 0) {
            target.verts = static_cast<RwV3d*>(Malloc(sizeof(RwV3d) * (size_t)geometry->numVertices));
            target.normals = static_cast<RwV3d*>(Malloc(sizeof(RwV3d) * (size_t)geometry->numVertices));
            if (!target.verts || !target.normals) {
                // Roll back: the caller keeps the targets it already had, and the partial
                // target must not leak its buffers.
                for (RwInt32 j = oldCount; j <= i; j++) {
                    Free(targets[j].verts);
                    Free(targets[j].normals);
                }
                std::memset(targets + oldCount, 0, sizeof(RpMorphTarget) * (size_t)mtcount);
                return oldCount;
            }
            std::memset(target.verts, 0, sizeof(RwV3d) * (size_t)geometry->numVertices);
            std::memset(target.normals, 0, sizeof(RwV3d) * (size_t)geometry->numVertices);
        }
    }

    geometry->numMorphTargets = newCount;
    return newCount;
}

RwInt32 RpGeometryAddMorphTarget(RpGeometry* geometry) {
    return RpGeometryAddMorphTargets(geometry, 1);
}

RpGeometry* RpGeometryCreate(RwInt32 numVert, RwInt32 numTriangles, RwUInt32 format) {
    numVert       = std::max(numVert, 0);
    numTriangles  = std::max(numTriangles, 0);

    auto* geometry = static_cast<RpGeometry*>(AllocObject(sizeof(RpGeometry), GeometryPlugins()));
    if (!geometry) {
        return nullptr;
    }

    rwObjectInitialize(&geometry->object, rpGEOMETRY, 0);
    // The texture-coord-set count lives in the format's bits 16..23 and is stored in its own
    // field, like retail `RpGeometryCreate`.
    geometry->flags              = format & 0xFF00FFFF;
    geometry->lockedSinceLastInst = 0;
    geometry->refCount           = 1;
    geometry->numTriangles       = numTriangles;
    geometry->numVertices        = numVert;
    geometry->numMorphTargets    = 0;
    geometry->numTexCoordSets    = (RwInt32)((format >> 16) & 0xFF);
    if (geometry->numTexCoordSets == 0) {
        geometry->numTexCoordSets = (format & rpGEOMETRYTEXTURED2) ? 2
                                  : (format & rpGEOMETRYTEXTURED)  ? 1
                                                                    : 0;
    }
    geometry->numTexCoordSets = std::clamp(geometry->numTexCoordSets, 0, (RwInt32)rwMAXTEXTURECOORDS);

    geometry->matList  = {};
    geometry->triangles = nullptr;
    geometry->preLitLum = nullptr;
    for (auto& texCoords : geometry->texCoords) {
        texCoords = nullptr;
    }
    geometry->mesh       = nullptr;
    geometry->repEntry   = nullptr;
    geometry->morphTarget = nullptr;

    if (numTriangles > 0) {
        geometry->triangles = static_cast<RpTriangle*>(Malloc(sizeof(RpTriangle) * (size_t)numTriangles));
        if (!geometry->triangles) {
            Free(geometry);
            return nullptr;
        }
        for (RwInt32 i = 0; i < numTriangles; i++) {
            geometry->triangles[i] = { { 0, 0, 0 }, 0xFFFF };
        }
    }

    if ((format & rpGEOMETRYPRELIT) && numVert > 0) {
        geometry->preLitLum = static_cast<RwRGBA*>(Malloc(sizeof(RwRGBA) * (size_t)numVert));
        if (!geometry->preLitLum) {
            Free(geometry->triangles);
            Free(geometry);
            return nullptr;
        }
        std::memset(geometry->preLitLum, 0xFF, sizeof(RwRGBA) * (size_t)numVert);
    }

    if (numVert > 0) {
        for (RwInt32 set = 0; set < geometry->numTexCoordSets; set++) {
            geometry->texCoords[set] = static_cast<RwTexCoords*>(Malloc(sizeof(RwTexCoords) * (size_t)numVert));
            if (!geometry->texCoords[set]) {
                for (auto& texCoords : geometry->texCoords) {
                    Free(texCoords);
                }
                Free(geometry->preLitLum);
                Free(geometry->triangles);
                Free(geometry);
                return nullptr;
            }
            std::memset(geometry->texCoords[set], 0, sizeof(RwTexCoords) * (size_t)numVert);
        }
    }

    if (RpGeometryAddMorphTargets(geometry, 1) != 1) {
        for (auto& texCoords : geometry->texCoords) {
            Free(texCoords);
        }
        Free(geometry->preLitLum);
        Free(geometry->triangles);
        Free(geometry);
        return nullptr;
    }

    // Retail constructs the plugins last, once every core array exists.
    GeometryPlugins().Construct(geometry, sizeof(RpGeometry));
    return geometry;
}

RwBool RpGeometryDestroy(RpGeometry* geometry) {
    if (!geometry) {
        return FALSE;
    }

    if (geometry->refCount > 1) {
        geometry->refCount--;
        return TRUE;
    }

    // The SA pipelines cache GL buffers per geometry; drop them before the struct is freed.
    notsa::native::sapipelines::ForgetGeometry(geometry);

    // Plugins must be destructed before the storage they may point into is released.
    GeometryPlugins().Destruct(geometry, sizeof(RpGeometry));

    if (geometry->morphTarget) {
        for (RwInt32 i = 0; i < geometry->numMorphTargets; i++) {
            Free(geometry->morphTarget[i].verts);
            Free(geometry->morphTarget[i].normals);
        }
        Free(geometry->morphTarget);
    }
    Free(geometry->triangles);
    Free(geometry->preLitLum);
    for (auto& texCoords : geometry->texCoords) {
        Free(texCoords);
    }
    Free(geometry->mesh);
    _rpMaterialListDestroy(&geometry->matList);

    Free(geometry);
    return TRUE;
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* data) {
    if (!geometry || !fpCallBack) {
        return geometry;
    }

    for (RwInt32 i = 0; i < geometry->matList.numMaterials; i++) {
        if (!fpCallBack(geometry->matList.materials[i], data)) {
            return nullptr;
        }
    }
    return geometry;
}

// 0x74C6C0
RpGeometry* RpGeometryTriangleSetMaterial(RpGeometry* geometry, RpTriangle* triangle, RpMaterial* material) {
    if (!geometry || !triangle) {
        return nullptr;
    }

    if (!material) {
        triangle->matIndex = (RwUInt16)-1; // No material
        return geometry;
    }

    RwInt32 index = _rpMaterialListFindMaterialIndex(&geometry->matList, material);
    if (index < 0) {
        index = _rpMaterialListAppendMaterial(&geometry->matList, material);
        if (index < 0) {
            return nullptr;
        }
    }

    triangle->matIndex = (RwUInt16)index;
    return geometry;
}

// 0x74C760
RpMaterial* RpGeometryTriangleGetMaterial(const RpGeometry* geometry, const RpTriangle* triangle) {
    if (!geometry || !triangle || triangle->matIndex == (RwUInt16)-1) {
        return nullptr;
    }
    return _rpMaterialListGetMaterial(&geometry->matList, triangle->matIndex);
}

const RpGeometry* RpGeometryForAllMeshes(const RpGeometry* geometry, RpMeshCallBack fpCallBack, void* data) {
    if (!geometry || !fpCallBack) {
        return geometry;
    }
    if (!geometry->mesh || geometry->mesh->numMeshes == 0) {
        return geometry;
    }

    auto* header = const_cast<RpMeshHeader*>(geometry->mesh);
    auto* meshes = reinterpret_cast<RpMesh*>(reinterpret_cast<uint8*>(header) + sizeof(RpMeshHeader) + header->firstMeshOffset);
    for (RwUInt32 i = 0; i < header->numMeshes; i++) {
        if (!fpCallBack(&meshes[i], header, data)) {
            return nullptr;
        }
    }
    return geometry;
}

RpGeometry* RpGeometryLock(RpGeometry* geometry, RwInt32 lockMode) {
    if (!geometry) {
        return nullptr;
    }
    // The mesh comes straight from the file's bin mesh chunk; there is no triangle-list builder
    // to rebuild it from, so a polygon lock only marks the geometry for re-instancing.
    geometry->lockedSinceLastInst |= (RwUInt16)lockMode;
    return geometry;
}

RpGeometry* RpGeometryUnlock(RpGeometry* geometry) {
    if (!geometry) {
        return nullptr;
    }
    // `InstanceAtomic` rebuilds the GL buffers when this is non-zero and clears it afterwards.
    geometry->lockedSinceLastInst |= (RwUInt16)rpGEOMETRYLOCKALL;
    return geometry;
}

const RpMorphTarget* RpMorphTargetCalcBoundingSphere(const RpMorphTarget* morphTarget, RwSphere* boundingSphere) {
    if (!morphTarget || !boundingSphere) {
        return nullptr;
    }

    const RwV3d* verts = morphTarget->verts;
    const auto   count = morphTarget->parentGeom ? morphTarget->parentGeom->numVertices : 0;
    if (!verts || count <= 0) {
        *boundingSphere = {};
        return morphTarget;
    }

    RwBBox box{ verts[0], verts[0] };
    for (RwInt32 i = 1; i < count; i++) {
        box.inf.x = std::min(box.inf.x, verts[i].x);
        box.inf.y = std::min(box.inf.y, verts[i].y);
        box.inf.z = std::min(box.inf.z, verts[i].z);
        box.sup.x = std::max(box.sup.x, verts[i].x);
        box.sup.y = std::max(box.sup.y, verts[i].y);
        box.sup.z = std::max(box.sup.z, verts[i].z);
    }

    boundingSphere->center = {
        (box.inf.x + box.sup.x) * 0.5f,
        (box.inf.y + box.sup.y) * 0.5f,
        (box.inf.z + box.sup.z) * 0.5f,
    };

    RwReal radiusSq = 0.0f;
    for (RwInt32 i = 0; i < count; i++) {
        const RwV3d d = {
            verts[i].x - boundingSphere->center.x,
            verts[i].y - boundingSphere->center.y,
            verts[i].z - boundingSphere->center.z,
        };
        radiusSq = std::max(radiusSq, Dot(d, d));
    }
    boundingSphere->radius = std::sqrt(radiusSq);

    return morphTarget;
}

RpGeometryChunkInfo* _rpGeometryChunkInfoRead(RwStream* stream, RpGeometryChunkInfo* geometryChunkInfo, RwInt32* bytesRead) {
    if (!geometryChunkInfo) {
        return nullptr;
    }

    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, nullptr) || length < sizeof(RpGeometryChunkInfo)) {
        BadChunk("STRUCT", rwID_STRUCT, "geometry");
        return nullptr;
    }

    const RwUInt32 toRead = std::min<RwUInt32>(length, sizeof(RpGeometryChunkInfo));
    if (!ReadExact(stream, geometryChunkInfo, toRead) || !Skip(stream, length - toRead)) {
        return nullptr;
    }

    if (bytesRead) {
        *bytesRead = (RwInt32)toRead;
    }
    return geometryChunkInfo;
}

RwInt16 _rpMeshGetNextSerialNumber() {
    return (RwInt16)g_meshSerialNum++;
}

RpMeshHeader* _rpMeshRead(RwStream* stream, const void* object, const RpMaterialList* matList) {
    auto* geometry = const_cast<RpGeometry*>(static_cast<const RpGeometry*>(object));
    if (!geometry) {
        return nullptr;
    }

    MeshStreamHeader headerInfo{};

    if (!ReadExact(stream, &headerInfo, sizeof(headerInfo))) {
        return nullptr;
    }
    if (headerInfo.numMeshes == 0) {
        return nullptr;
    }
    if (headerInfo.numMeshes > kMaxStreamArrayElements / sizeof(RpMesh) ||
        headerInfo.totalIndices > kMaxStreamArrayElements) {
        return nullptr;
    }

    const size_t size = sizeof(RpMeshHeader)
                      + sizeof(RpMesh) * (size_t)headerInfo.numMeshes
                      + sizeof(RxVertexIndex) * (size_t)headerInfo.totalIndices;

    auto* header = static_cast<RpMeshHeader*>(Malloc(size));
    if (!header) {
        return nullptr;
    }
    std::memset(header, 0, size);

    header->flags               = headerInfo.flags;
    header->numMeshes           = (RwUInt16)headerInfo.numMeshes;
    header->serialNum           = (RwUInt16)_rpMeshGetNextSerialNumber();
    header->totalIndicesInMesh  = headerInfo.totalIndices;
    header->firstMeshOffset     = 0;

    auto* meshes  = reinterpret_cast<RpMesh*>(header + 1);
    auto* indices = reinterpret_cast<RxVertexIndex*>(meshes + headerInfo.numMeshes);

    RwUInt32 indicesWritten = 0;
    for (RwUInt32 i = 0; i < headerInfo.numMeshes; i++) {
        struct MeshChunk {
            RwUInt32 numIndices;
            RwInt32  matIndex;
        } meshInfo{};

        if (!ReadExact(stream, &meshInfo, sizeof(meshInfo))) {
            Free(header);
            return nullptr;
        }
        if (meshInfo.numIndices > headerInfo.totalIndices - indicesWritten) {
            Free(header);
            return nullptr;
        }

        RpMesh& mesh = meshes[i];
        mesh.indices    = indices + indicesWritten;
        mesh.numIndices = meshInfo.numIndices;
        mesh.material   = (matList && meshInfo.matIndex >= 0 && meshInfo.matIndex < matList->numMaterials)
            ? matList->materials[meshInfo.matIndex]
            : nullptr;

        // The file stores 32 bit indices; the game uses 16 bit ones.
        for (RwUInt32 j = 0; j < meshInfo.numIndices; j++) {
            RwUInt32 index = 0;
            if (!ReadExact(stream, &index, sizeof(index))) {
                Free(header);
                return nullptr;
            }
            if (index > 0xFFFF) {
                Free(header);
                return nullptr;
            }
            mesh.indices[j] = (RxVertexIndex)index;
        }
        indicesWritten += meshInfo.numIndices;
    }

    if (indicesWritten != headerInfo.totalIndices) {
        Free(header);
        return nullptr;
    }

    Free(geometry->mesh);
    geometry->mesh = header;

    // Keep the geometry's tri-strip flag in sync with the mesh, which is what the game's
    // `RpAtomicConvertGeometryToTL/TS` toggles.
    if (header->flags & rpMESHHEADERTRISTRIP) {
        geometry->flags |= rpGEOMETRYTRISTRIP;
    } else {
        geometry->flags &= ~rpGEOMETRYTRISTRIP;
    }

    return header;
}

RpGeometry* RpGeometryStreamRead(RwStream* stream) {
    if (!stream) {
        return nullptr;
    }

    RwUInt32 length  = 0;
    RwUInt32 version = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, &version)) {
        BadChunk("STRUCT", rwID_STRUCT, "geometry");
        return nullptr;
    }
    if (version < kMinLibraryVersion || version > kMaxLibraryVersion) {
        NOTSA_LOG_ERR("Rw: geometry chunk has the unsupported RW version 0x{:X}", version);
        return nullptr;
    }
    if (length < sizeof(RpGeometryChunkInfo)) {
        BadChunk("STRUCT", rwID_STRUCT, "geometry");
        return nullptr;
    }

    // The STRUCT chunk holds the chunk info *and* the geometry's vertex data (prelit colours,
    // texture coordinates, triangles and morph targets). Only the info is read here; the data
    // follows immediately and must not be skipped, or the reader would walk into the material
    // list and the sibling chunks.
    const int32 structEnd = StreamPosition(stream) + (int32)length;

    RpGeometryChunkInfo info{};
    if (!ReadExact(stream, &info, sizeof(info))) {
        return nullptr;
    }

    RwSurfaceProperties defaultSurfaceProps{ 1.0f, 1.0f, 1.0f };
    if (version < 0x34000) {
        // Legacy geometries carry the geometry-wide surface properties here; materials before
        // RW 3.4 rely on them.
        if (!ReadExact(stream, &defaultSurfaceProps, sizeof(defaultSurfaceProps))) {
            return nullptr;
        }
    }

    if (info.numVertices < 0 || info.numTriangles < 0 ||
        info.numVertices > (RwInt32)kMaxStreamArrayElements ||
        info.numTriangles > (RwInt32)kMaxStreamArrayElements ||
        info.numMorphTargets < 1 || info.numMorphTargets > 0x1000 ||
        (((RwUInt32)info.format >> 16) & 0xFF) > rwMAXTEXTURECOORDS) {
        BadChunk("STRUCT", rwID_STRUCT, "geometry");
        return nullptr;
    }

    auto* geometry = RpGeometryCreate(info.numVertices, info.numTriangles, (RwUInt32)info.format);
    if (!geometry) {
        return nullptr;
    }

    if (RpGeometryAddMorphTargets(geometry, info.numMorphTargets - 1) != info.numMorphTargets) {
        BadChunk("STRUCT", rwID_STRUCT, "geometry");
        RpGeometryDestroy(geometry);
        return nullptr;
    }

    // Vertex data, in the order the file stores it.
    if (geometry->preLitLum && !ReadExact(stream, geometry->preLitLum, sizeof(RwRGBA) * (RwUInt32)info.numVertices)) {
        goto bad;
    }

    for (RwInt32 set = 0; set < geometry->numTexCoordSets; set++) {
        if (!ReadExact(stream, geometry->texCoords[set], sizeof(RwTexCoords) * (RwUInt32)info.numVertices)) {
            goto bad;
        }
    }

    for (RwInt32 i = 0; i < info.numTriangles; i++) {
        RwUInt32 triangle[2]{};
        if (!ReadExact(stream, triangle, sizeof(triangle))) {
            goto bad;
        }
        geometry->triangles[i].vertIndex[0] = (RwUInt16)(triangle[0] >> 16);
        geometry->triangles[i].vertIndex[1] = (RwUInt16)(triangle[0] & 0xFFFF);
        geometry->triangles[i].vertIndex[2] = (RwUInt16)(triangle[1] >> 16);
        geometry->triangles[i].matIndex     = (RwUInt16)(triangle[1] & 0xFFFF);
    }

    for (RwInt32 m = 0; m < info.numMorphTargets; m++) {
        RpMorphTarget& target = geometry->morphTarget[m];

        RwSphere sphere{};
        RwInt32  hasVertices = 0;
        RwInt32  hasNormals  = 0;
        if (!ReadExact(stream, &sphere, sizeof(sphere)) ||
            !ReadExact(stream, &hasVertices, sizeof(hasVertices)) ||
            !ReadExact(stream, &hasNormals, sizeof(hasNormals))) {
            goto bad;
        }
        target.boundingSphere = sphere;

        if (hasVertices) {
            if (!target.verts || !ReadExact(stream, target.verts, sizeof(RwV3d) * (RwUInt32)info.numVertices)) {
                goto bad;
            }
        }
        if (hasNormals) {
            if (!target.normals || !ReadExact(stream, target.normals, sizeof(RwV3d) * (RwUInt32)info.numVertices)) {
                goto bad;
            }
        }
    }

    // The vertex data is stored inside the STRUCT chunk, so the chunk must end exactly here. A
    // mismatch means the declared sizes and the data disagree.
    if (!SkipTo(stream, structEnd)) {
        BadChunk("STRUCT", rwID_STRUCT, "geometry vertex data");
        goto bad;
    }

    if (!_rpMaterialListStreamRead(stream, &geometry->matList)) {
        goto bad;
    }

    if (version < 0x34000) {
        // The material chunk info carried no surface properties; apply the geometry-wide ones.
        for (RwInt32 i = 0; i < geometry->matList.numMaterials; i++) {
            if (geometry->matList.materials[i]) {
                geometry->matList.materials[i]->surfaceProps = defaultSurfaceProps;
            }
        }
    }

    EnsureMeshStreamReader();
    if (!ReadPlugins(stream, GeometryPlugins(), geometry)) {
        BadChunk("EXTENSION", rwID_EXTENSION, "geometry");
        goto bad;
    }

    return geometry;

bad:
    RpGeometryDestroy(geometry);
    return nullptr;
}

RpGeometry* RpGeometrySortByMaterial(const RpGeometry* geometry, RpGeometrySortByMaterialCallBack callback) {
    if (!geometry || geometry->numVertices <= 0 || geometry->numTriangles <= 0 ||
        !geometry->triangles || geometry->matList.numMaterials <= 0) {
        return nullptr;
    }

    const RwInt32 numMaterials = geometry->matList.numMaterials;
    const RwInt32 numVertices  = geometry->numVertices;

    if ((RwInt64)numMaterials * numVertices > (RwInt64)kMaxStreamArrayElements) {
        return nullptr;
    }

    // `newIndex[material * numVertices + vertex]`, -1 while the pair is unused.
    std::vector<RwInt32> newIndex((size_t)numMaterials * (size_t)numVertices, -1);
    std::vector<RwUInt16> remap;
    remap.reserve((size_t)numVertices);

    // Material-major vertex assignment keeps every material's vertex range contiguous.
    for (RwInt32 material = 0; material < numMaterials; material++) {
        for (RwInt32 tri = 0; tri < geometry->numTriangles; tri++) {
            if (geometry->triangles[tri].matIndex != material) {
                continue;
            }
            for (int corner = 0; corner < 3; corner++) {
                const RwUInt16 vertex = geometry->triangles[tri].vertIndex[corner];
                if (vertex >= numVertices) {
                    return nullptr;
                }
                RwInt32& index = newIndex[(size_t)material * numVertices + vertex];
                if (index < 0) {
                    index = (RwInt32)remap.size();
                    remap.push_back(vertex);
                }
            }
        }
    }

    const RwInt32 newNumVertices = (RwInt32)remap.size();
    if (newNumVertices <= 0) {
        return nullptr;
    }

    auto* sorted = RpGeometryCreate(
        newNumVertices,
        geometry->numTriangles,
        geometry->flags | ((RwUInt32)geometry->numTexCoordSets << 16)
    );
    if (!sorted) {
        return nullptr;
    }
    if (RpGeometryAddMorphTargets(sorted, geometry->numMorphTargets - 1) != geometry->numMorphTargets) {
        RpGeometryDestroy(sorted);
        return nullptr;
    }

    // Core vertex data, remapped.
    for (RwInt32 m = 0; m < geometry->numMorphTargets && m < sorted->numMorphTargets; m++) {
        const RpMorphTarget& src = geometry->morphTarget[m];
        RpMorphTarget&       dst = sorted->morphTarget[m];
        dst.boundingSphere = src.boundingSphere;
        for (RwInt32 i = 0; i < newNumVertices; i++) {
            const RwUInt16 source = remap[i];
            if (src.verts) {
                dst.verts[i] = src.verts[source];
            }
            if (src.normals) {
                dst.normals[i] = src.normals[source];
            }
        }
    }
    for (RwInt32 i = 0; i < newNumVertices; i++) {
        const RwUInt16 source = remap[i];
        if (geometry->preLitLum && sorted->preLitLum) {
            sorted->preLitLum[i] = geometry->preLitLum[source];
        }
        for (RwInt32 set = 0; set < sorted->numTexCoordSets; set++) {
            if (geometry->texCoords[set] && sorted->texCoords[set]) {
                sorted->texCoords[set][i] = geometry->texCoords[set][source];
            }
        }
    }

    if (!_rpMaterialListCopy(&sorted->matList, &geometry->matList)) {
        RpGeometryDestroy(sorted);
        return nullptr;
    }

    // Triangles, with the remapped indices and the material as the dense index.
    for (RwInt32 tri = 0; tri < geometry->numTriangles; tri++) {
        const RpTriangle& src = geometry->triangles[tri];
        RpTriangle&       dst = sorted->triangles[tri];

        const RwUInt16 material = src.matIndex;
        if (material >= numMaterials) {
            RpGeometryDestroy(sorted);
            return nullptr;
        }
        for (int corner = 0; corner < 3; corner++) {
            const RwInt32 index = newIndex[(size_t)material * numVertices + src.vertIndex[corner]];
            if (index < 0 || index > 0xFFFF) {
                RpGeometryDestroy(sorted);
                return nullptr;
            }
            dst.vertIndex[corner] = (RwUInt16)index;
        }
        dst.matIndex = material;
    }

    // One triangle-list mesh per material that has triangles.
    RwUInt32 numMeshes = 0;
    for (RwInt32 material = 0; material < numMaterials; material++) {
        for (RwInt32 tri = 0; tri < geometry->numTriangles; tri++) {
            if (geometry->triangles[tri].matIndex == material) {
                numMeshes++;
                break;
            }
        }
    }
    if (numMeshes == 0) {
        RpGeometryDestroy(sorted);
        return nullptr;
    }

    const size_t meshSize = sizeof(RpMeshHeader)
                          + sizeof(RpMesh) * (size_t)numMeshes
                          + sizeof(RxVertexIndex) * 3 * (size_t)geometry->numTriangles;
    auto* header = static_cast<RpMeshHeader*>(Malloc(meshSize));
    if (!header) {
        RpGeometryDestroy(sorted);
        return nullptr;
    }
    std::memset(header, 0, meshSize);
    header->flags              = 0; // triangle list
    header->numMeshes          = (RwUInt16)numMeshes;
    header->serialNum          = (RwUInt16)_rpMeshGetNextSerialNumber();
    header->totalIndicesInMesh = (RwUInt32)geometry->numTriangles * 3;
    header->firstMeshOffset    = 0;

    auto* meshes  = reinterpret_cast<RpMesh*>(header + 1);
    auto* indices = reinterpret_cast<RxVertexIndex*>(meshes + numMeshes);

    RwUInt32 meshIndex    = 0;
    RwUInt32 indicesSoFar = 0;
    for (RwInt32 material = 0; material < numMaterials; material++) {
        RpMesh& mesh = meshes[meshIndex];
        mesh.indices    = indices + indicesSoFar;
        mesh.numIndices = 0;
        mesh.material   = sorted->matList.materials[material];

        for (RwInt32 tri = 0; tri < geometry->numTriangles; tri++) {
            if (geometry->triangles[tri].matIndex != material) {
                continue;
            }
            const RpTriangle& dst = sorted->triangles[tri];
            indices[indicesSoFar + 0] = dst.vertIndex[0];
            indices[indicesSoFar + 1] = dst.vertIndex[1];
            indices[indicesSoFar + 2] = dst.vertIndex[2];
            indicesSoFar += 3;
            mesh.numIndices += 3;
        }

        if (mesh.numIndices > 0) {
            meshIndex++;
        }
    }

    sorted->mesh  = header;
    sorted->flags &= ~rpGEOMETRYTRISTRIP;

    if (callback) {
        callback(geometry, sorted, remap.data(), (RwUInt16)newNumVertices);
    }
    return sorted;
}

RpAtomic* RpAtomicStreamRead(RwStream* stream) {
    if (!stream) {
        return nullptr;
    }

    AtomicChunkInfo info{};
    RwUInt32        version = 0;
    if (!ReadAtomicChunkInfo(stream, info, version)) {
        return nullptr;
    }

    auto* atomic = RpAtomicCreate();
    if (!atomic) {
        return nullptr;
    }

    atomic->object.object.flags = AtomicFlags(info, version);

    // Without a clump there is no frame or geometry list, so the geometry must be inline (the
    // frame is assigned by whoever attaches the atomic).
    RpGeometry* geometry = ReadInlineAtomicGeometry(stream);
    if (!geometry) {
        RpAtomicDestroy(atomic);
        return nullptr;
    }
    RpAtomicSetGeometry(atomic, geometry, 0);
    RpGeometryDestroy(geometry);

    if (!ReadPlugins(stream, AtomicPlugins(), atomic)) {
        BadChunk("EXTENSION", rwID_EXTENSION, "atomic");
        RpAtomicDestroy(atomic);
        return nullptr;
    }

    return atomic;
}

RpAtomic* RpAtomicClone(RpAtomic* atomic) {
    if (!atomic) {
        return nullptr;
    }

    auto* clone = RpAtomicCreate();
    if (!clone) {
        return nullptr;
    }

    // Copy the render-relevant object state; the frame is left for the caller to assign
    // (exactly what the `RpAtomicClone` call sites in the game do).
    clone->object.object.type        = atomic->object.object.type;
    clone->object.object.subType     = atomic->object.object.subType;
    clone->object.object.flags       = atomic->object.object.flags;
    clone->object.object.privateFlags = atomic->object.object.privateFlags;
    clone->object.object.privateFlags |= rpATOMICPRIVATEWORLDBOUNDDIRTY;

    clone->renderCallBack = atomic->renderCallBack;
    clone->pipeline       = atomic->pipeline;
    clone->interpolator   = atomic->interpolator;

    RpAtomicSetGeometry(clone, atomic->geometry, rpATOMICSAMEBOUNDINGSPHERE);
    clone->boundingSphere      = atomic->boundingSphere;
    clone->worldBoundingSphere = atomic->worldBoundingSphere;

    AtomicPlugins().Copy(clone, atomic, sizeof(RpAtomic));
    return clone;
}

RpClumpChunkInfo* _rpClumpChunkInfoRead(RwStream* stream, RpClumpChunkInfo* clumpChunkInfo, RwInt32* bytesRead) {
    if (!clumpChunkInfo) {
        return nullptr;
    }

    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, nullptr) || length < sizeof(RwInt32)) {
        BadChunk("STRUCT", rwID_STRUCT, "clump");
        return nullptr;
    }

    std::memset(clumpChunkInfo, 0, sizeof(*clumpChunkInfo));
    const RwUInt32 toRead = std::min<RwUInt32>(length, sizeof(*clumpChunkInfo));
    if (!ReadExact(stream, clumpChunkInfo, toRead) || !Skip(stream, length - toRead)) {
        return nullptr;
    }

    if (bytesRead) {
        *bytesRead = (RwInt32)toRead;
    }
    return clumpChunkInfo;
}

RpClump* RpClumpStreamRead(RwStream* stream) {
    if (!stream) {
        return nullptr;
    }

    RpClumpChunkInfo info{};
    RwUInt32         version = 0;
    if (!ReadClumpChunkInfo(stream, info, version)) {
        return nullptr;
    }
    if (version < kMinLibraryVersion || version > kMaxLibraryVersion) {
        NOTSA_LOG_ERR("Rw: clump chunk has the unsupported RW version 0x{:X}", version);
        return nullptr;
    }
    if (info.numAtomics < 0 || info.numAtomics > (RwInt32)kMaxStreamArrayElements ||
        info.numLights < 0 || info.numCameras < 0) {
        BadChunk("STRUCT", rwID_STRUCT, "clump");
        return nullptr;
    }

    auto* clump = RpClumpCreate();
    if (!clump) {
        return nullptr;
    }

    rwFrameList frames{};
    RpGeometry** geometries    = nullptr;
    RwInt32      numGeometries = 0;

    // Frame list.
    if (!RwStreamFindChunk(stream, rwID_FRAMELIST, nullptr, nullptr)) {
        BadChunk("FRAMELIST", rwID_FRAMELIST, "clump");
        goto fail;
    }
    if (!_rwFrameListStreamRead(stream, &frames)) {
        BadChunk("FRAMELIST", rwID_FRAMELIST, "clump");
        goto fail;
    }
    if (frames.numFrames > 0) {
        clump->object.parent = frames.frames[0];
    }

    // Geometry list: every geometry is read once and shared through the atomic indices.
    if (version >= 0x30400) {
        RwUInt32 listStructLength = 0;
        if (!RwStreamFindChunk(stream, rwID_GEOMETRYLIST, nullptr, nullptr) ||
            !RwStreamFindChunk(stream, rwID_STRUCT, &listStructLength, nullptr) ||
            listStructLength < sizeof(RwInt32)) {
            BadChunk("GEOMETRYLIST", rwID_GEOMETRYLIST, "clump");
            goto fail;
        }
        if (!ReadExact(stream, &numGeometries, sizeof(numGeometries)) ||
            !Skip(stream, listStructLength - sizeof(numGeometries))) {
            goto fail;
        }
        if (numGeometries < 0 || numGeometries > (RwInt32)kMaxStreamArrayElements) {
            BadChunk("GEOMETRYLIST", rwID_GEOMETRYLIST, "clump");
            goto fail;
        }

        if (numGeometries > 0) {
            geometries = static_cast<RpGeometry**>(Malloc(sizeof(RpGeometry*) * (size_t)numGeometries));
            if (!geometries) {
                goto fail;
            }
            std::memset(geometries, 0, sizeof(RpGeometry*) * (size_t)numGeometries);
        }
        for (RwInt32 i = 0; i < numGeometries; i++) {
            if (!RwStreamFindChunk(stream, rwID_GEOMETRY, nullptr, nullptr)) {
                BadChunk("GEOMETRY", rwID_GEOMETRY, "clump");
                goto fail;
            }
            geometries[i] = RpGeometryStreamRead(stream);
            if (!geometries[i]) {
                goto fail;
            }
        }
    }

    // Atomics.
    for (RwInt32 i = 0; i < info.numAtomics; i++) {
        if (!RwStreamFindChunk(stream, rwID_ATOMIC, nullptr, nullptr)) {
            BadChunk("ATOMIC", rwID_ATOMIC, "clump");
            goto fail;
        }
        RpAtomic* atomic = AtomicStreamReadFromClump(stream, frames, geometries, numGeometries);
        if (!atomic) {
            goto fail;
        }
        RpClumpAddAtomic(clump, atomic);
    }

    // Lights and cameras.
    for (RwInt32 i = 0; i < info.numLights; i++) {
        if (!ReadClumpLight(stream, frames, clump)) {
            goto fail;
        }
    }
    for (RwInt32 i = 0; i < info.numCameras; i++) {
        if (!SkipClumpCamera(stream)) {
            goto fail;
        }
    }

    // Clump plugin data (the collision plugin reads the model's collision data here).
    if (!ReadPlugins(stream, ClumpPlugins(), clump)) {
        BadChunk("EXTENSION", rwID_EXTENSION, "clump");
        goto fail;
    }

    // The geometries only kept the reader's reference; the atomics own them now.
    for (RwInt32 i = 0; i < numGeometries; i++) {
        if (geometries[i]) {
            RpGeometryDestroy(geometries[i]);
        }
    }
    Free(geometries);
    _rwFrameListDeinitialize(&frames);
    return clump;

fail:
    // Destroy the clump first (which unlinks and destroys the atomics from the still-alive
    // frames), then release the reader's geometry references and the frame hierarchies. The
    // clump must not free the frames itself: the list owns them here.
    clump->object.parent = nullptr;
    RpClumpDestroy(clump);

    for (RwInt32 i = 0; i < numGeometries; i++) {
        if (geometries && geometries[i]) {
            RpGeometryDestroy(geometries[i]);
        }
    }
    Free(geometries);
    DestroyFrameList(frames);
    return nullptr;
}

RpClump* RpClumpClone(RpClump* clump) {
    if (!clump) {
        return nullptr;
    }

    auto* clone = RpClumpCreate();
    if (!clone) {
        return nullptr;
    }

    auto* originalFrame = static_cast<RwFrame*>(clump->object.parent);
    RwFrame* cloneFrame = nullptr;
    if (originalFrame) {
        cloneFrame = _rwFrameCloneAndLinkClones(originalFrame);
        if (!cloneFrame) {
            RpClumpDestroy(clone);
            return nullptr;
        }
        // Adopt the clone tree: the cloned clump owns it from now on.
        clone->object.parent = cloneFrame;
    }

    bool ok = true;
    for (RwLLLink* link = clump->atomicList.link.next; link != &clump->atomicList.link; link = link->next) {
        auto* atomic = rwLLLinkGetData(link, RpAtomic, inClumpLink);

        RpAtomic* atomicClone = RpAtomicClone(atomic);
        if (!atomicClone) {
            ok = false;
            break;
        }

        if (auto* frame = RpAtomicGetFrame(atomic)) {
            // While the link is up, every original points at the clone that replaced it.
            RpAtomicSetFrame(atomicClone, frame->root);
        }
        RpClumpAddAtomic(clone, atomicClone);
    }

    if (originalFrame) {
        _rwFramePurgeClone(originalFrame);
    }

    if (!ok) {
        RpClumpDestroy(clone);
        return nullptr;
    }

    ClumpPlugins().Copy(clone, clump, sizeof(RpClump));
    return clone;
}

//-----------------------------------------------------------------------------
// GTA split streaming entry points
//-----------------------------------------------------------------------------

bool RpClumpGtaStreamRead1(RwStream* stream) {
    if (!stream) {
        return false;
    }

    // Drop anything an abandoned earlier read left behind (`read2`/cancel clear this too).
    DestroyGeometryList();
    DestroyFrameList(g_gtaClumpStream.frameList);
    g_gtaClumpStream = {};

    RwUInt32 version = 0;
    if (!ReadClumpChunkInfo(stream, g_gtaClumpStream.chunkInfo, version)) {
        return false;
    }

    if (!RwStreamFindChunk(stream, rwID_FRAMELIST, nullptr, nullptr)) {
        BadChunk("FRAMELIST", rwID_FRAMELIST, "clump");
        return false;
    }
    if (!_rwFrameListStreamRead(stream, &g_gtaClumpStream.frameList)) {
        BadChunk("FRAMELIST", rwID_FRAMELIST, "clump");
        return false;
    }

    if (!RwStreamFindChunk(stream, rwID_GEOMETRYLIST, nullptr, nullptr)) {
        BadChunk("GEOMETRYLIST", rwID_GEOMETRYLIST, "clump");
        DestroyFrameList(g_gtaClumpStream.frameList);
        return false;
    }
    if (!GeometryListStreamReadHalf(stream)) {
        DestroyGeometryList();
        DestroyFrameList(g_gtaClumpStream.frameList);
        return false;
    }

    g_gtaClumpStream.resumePosition = StreamPosition(stream);
    return true;
}

RpClump* RpClumpGtaStreamRead2(RwStream* stream) {
    if (!stream) {
        return nullptr;
    }

    auto* clump = RpClumpCreate();
    if (!clump) {
        return nullptr;
    }

    // `CStreaming` hands the finishing read a stream over the start of the same model buffer.
    const int32 delta = g_gtaClumpStream.resumePosition - StreamPosition(stream);
    if (delta > 0) {
        if (!RwStreamSkip(stream, (RwUInt32)delta)) {
            RpClumpDestroy(clump);
            return nullptr;
        }
    } else if (delta < 0) {
        if (!SeekRwStream(stream, delta, 1)) {
            RpClumpDestroy(clump);
            return nullptr;
        }
    }

    if (!GeometryListStreamReadRest(stream)) {
        goto fail;
    }

    if (g_gtaClumpStream.frameList.numFrames > 0) {
        clump->object.parent = g_gtaClumpStream.frameList.frames[0];
    }

    for (int32 i = 0; i < g_gtaClumpStream.chunkInfo.numAtomics; i++) {
        if (!RwStreamFindChunk(stream, rwID_ATOMIC, nullptr, nullptr)) {
            BadChunk("ATOMIC", rwID_ATOMIC, "clump");
            goto fail;
        }
        RpAtomic* atomic = AtomicStreamReadFromClump(
            stream,
            g_gtaClumpStream.frameList,
            g_gtaClumpStream.geometries,
            g_gtaClumpStream.numGeometries
        );
        if (!atomic) {
            goto fail;
        }
        RpClumpAddAtomic(clump, atomic);
    }

    DestroyGeometryList();
    // The clump owns the frames; only the pointer array goes away.
    _rwFrameListDeinitialize(&g_gtaClumpStream.frameList);
    g_gtaClumpStream = {};
    return clump;

fail:
    // Same order as `RpClumpStreamRead`: unlink the atomics while the frames are alive, then
    // release the reader's references.
    clump->object.parent = nullptr;
    RpClumpDestroy(clump);
    DestroyGeometryList();
    DestroyFrameList(g_gtaClumpStream.frameList);
    g_gtaClumpStream = {};
    return nullptr;
}

void RpClumpGtaCancelStream() {
    // Nothing is in flight after a completed read (`read2` cleared the state), so this is safe
    // to call on every model unload.
    DestroyGeometryList();
    DestroyFrameList(g_gtaClumpStream.frameList);
    g_gtaClumpStream = {};
}

#endif // NOTSA_NATIVE
