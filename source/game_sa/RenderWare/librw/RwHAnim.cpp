#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

#include "RenderWare/rw/rtslerp.h"

#include <cmath>

//! HAnim hierarchies on the game's own structs (`rw/rpHAnim.h`).
//!
//! The frame plugin (`rwID_HANIMPLUGIN`) stores the same `RpHAnimFrameExtension` the retail
//! library uses, so `RpHAnimFrameGetID`/`GetHierarchy` and the ped code that reads hierarchy
//! fields directly keep working. The hierarchy owns a game `RtAnimInterpolator` (see
//! `RwAnim.cpp`), not a librw one.

namespace {

constexpr RwInt32 kStdKeyFrameTypeID = rpHANIMSTDKEYFRAMETYPEID;

//! Size of one standard keyframe on disk (32 bit pointers): time, quaternion, translation and
//! the byte offset of the previous keyframe. The native struct is larger, so the stream
//! callbacks convert the stored offsets through the frame index.
constexpr RwInt32 kStdKeyFrameDiskSize = 4 + 16 + 12 + 4;

//! Offset of `RpHAnimFrameExtension` inside an `RwFrame`.
RwInt32 g_hanimFrameOffset = -1;

RpHAnimFrameExtension* GetFrameExtension(RwFrame* frame) {
    return RWPLUGINOFFSET(RpHAnimFrameExtension, frame, g_hanimFrameOffset);
}

const RpHAnimFrameExtension* GetFrameExtension(const RwFrame* frame) {
    return RWPLUGINOFFSETCONST(RpHAnimFrameExtension, frame, g_hanimFrameOffset);
}

//-----------------------------------------------------------------------------
// Keyframe math
//-----------------------------------------------------------------------------

void SetupSlerpCache(RtQuat* from, RtQuat* to, RtQuatSlerpCache* cache) {
    // Same math as `RtQuatSetupSlerpCache`, kept here so no Retained stub is involved.
    RwReal cosom = from->imag.x * to->imag.x
                 + from->imag.y * to->imag.y
                 + from->imag.z * to->imag.z
                 + from->real   * to->real;

    RwReal scaleFrom = 1.0f;
    RwReal scaleTo   = 1.0f;
    if (cosom < 0.0f) {
        cosom = -cosom;
        scaleTo = -1.0f;
    }

    if (1.0f - cosom > 1e-6f) {
        const RwReal sinom = std::sqrt(1.0f - cosom * cosom);
        cache->omega = std::atan2(sinom, cosom);
        if (sinom > 1e-6f) {
            scaleFrom /= sinom;
            scaleTo   /= sinom;
        }
        cache->nearlyZeroOm = FALSE;
    } else {
        cache->omega        = 0.0f;
        cache->nearlyZeroOm = TRUE;
    }

    cache->raFrom.imag.x = from->imag.x * scaleFrom;
    cache->raFrom.imag.y = from->imag.y * scaleFrom;
    cache->raFrom.imag.z = from->imag.z * scaleFrom;
    cache->raFrom.real   = from->real   * scaleFrom;

    cache->raTo.imag.x = to->imag.x * scaleTo;
    cache->raTo.imag.y = to->imag.y * scaleTo;
    cache->raTo.imag.z = to->imag.z * scaleTo;
    cache->raTo.real   = to->real   * scaleTo;
}

void SlerpQuat(RtQuat* out, RtQuat* from, RtQuat* to, RwReal t) {
    RtQuatSlerpCache cache;
    SetupSlerpCache(from, to, &cache);
    RtQuatSlerpMacro(out, from, to, t, &cache);
}

inline void SetIdentityMatrix(RwMatrix* matrix) {
    std::memset(matrix, 0, sizeof(*matrix));
    matrix->right = { 1.0f, 0.0f, 0.0f };
    matrix->up    = { 0.0f, 1.0f, 0.0f };
    matrix->at    = { 0.0f, 0.0f, 1.0f };
    matrix->flags = rwMATRIXTYPEORTHONORMAL;
}

inline RwReal Lerp(RwReal a, RwReal b, RwReal t) {
    return a + t * (b - a);
}

inline void LerpV3d(RwV3d* out, const RwV3d* a, const RwV3d* b, RwReal t) {
    out->x = Lerp(a->x, b->x, t);
    out->y = Lerp(a->y, b->y, t);
    out->z = Lerp(a->z, b->z, t);
}

//-----------------------------------------------------------------------------
// Frame plugin
//-----------------------------------------------------------------------------

void* HAnimFrameConstructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    UNUSED(sizeInObject);
    auto* extension = RWPLUGINOFFSET(RpHAnimFrameExtension, object, offsetInObject);
    extension->id        = -1;
    extension->hierarchy = nullptr;
    return object;
}

void* HAnimFrameDestructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    UNUSED(sizeInObject);
    auto* extension = RWPLUGINOFFSET(RpHAnimFrameExtension, object, offsetInObject);
    if (RpHAnimHierarchy* hierarchy = extension->hierarchy) {
        for (RwInt32 i = 0; i < hierarchy->numNodes; i++) {
            hierarchy->pNodeInfo[i].pFrame = nullptr;
        }
        if (hierarchy->parentFrame == static_cast<RwFrame*>(object)) {
            RpHAnimHierarchyDestroy(hierarchy);
        }
    }
    extension->id        = -1;
    extension->hierarchy = nullptr;
    return object;
}

void* HAnimFrameCopy(void* dstObject, const void* srcObject, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    UNUSED(sizeInObject);
    auto* dst = RWPLUGINOFFSET(RpHAnimFrameExtension, dstObject, offsetInObject);
    const auto* src = RWPLUGINOFFSETCONST(RpHAnimFrameExtension, srcObject, offsetInObject);

    dst->id        = src->id;
    dst->hierarchy = nullptr;

    if (src->hierarchy && !(src->hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
        dst->hierarchy = RpHAnimHierarchyCreateFromHierarchy(
            src->hierarchy,
            (RpHAnimHierarchyFlag)src->hierarchy->flags,
            src->hierarchy->currentAnim->maxInterpKeyFrameSize
        );
        if (dst->hierarchy) {
            dst->hierarchy->parentFrame = static_cast<RwFrame*>(dstObject);
        }
    }
    return dstObject;
}

RwStream* HAnimFrameStreamRead(RwStream* stream, RwInt32, void* object, RwInt32 offsetInObject, RwInt32) {
    auto* extension = RWPLUGINOFFSET(RpHAnimFrameExtension, object, offsetInObject);

    RwInt32 version = 0;
    RwInt32 numNodes = 0;
    if (RwStreamRead(stream, &version, sizeof(version)) != sizeof(version)
        || RwStreamRead(stream, &extension->id, sizeof(extension->id)) != sizeof(extension->id)
        || RwStreamRead(stream, &numNodes, sizeof(numNodes)) != sizeof(numNodes)
    ) {
        return nullptr;
    }

    if (version != rpHANIMSTREAMCURRENTVERSION) {
        NOTSA_LOG_WARN("Rw: unexpected HAnim chunk version 0x{:X}", (RwUInt32)version);
        return nullptr;
    }

    extension->hierarchy = nullptr;
    if (numNodes <= 0) {
        return stream;
    }

    RwInt32 flags = 0;
    RwInt32 maxInterpKeyFrameSize = 0;
    if (RwStreamRead(stream, &flags, sizeof(flags)) != sizeof(flags)
        || RwStreamRead(stream, &maxInterpKeyFrameSize, sizeof(maxInterpKeyFrameSize)) != sizeof(maxInterpKeyFrameSize)
    ) {
        return nullptr;
    }

    // The hierarchy stores native keyframe sizes; the streamed value is the 32 bit one.
    const RwInt32 maxKeySize = std::max<RwInt32>(maxInterpKeyFrameSize, (RwInt32)sizeof(RpHAnimInterpFrame));

    auto* nodeIDs   = static_cast<RwInt32*>(notsa::fakerw::Malloc(sizeof(RwInt32) * numNodes));
    auto* nodeFlags = static_cast<RwUInt32*>(notsa::fakerw::Malloc(sizeof(RwUInt32) * numNodes));
    if (!nodeIDs || !nodeFlags) {
        notsa::fakerw::Free(nodeIDs);
        notsa::fakerw::Free(nodeFlags);
        return nullptr;
    }

    RwBool ok = TRUE;
    for (RwInt32 i = 0; i < numNodes; i++) {
        RwInt32 index = 0;
        if (RwStreamRead(stream, &nodeIDs[i], sizeof(RwInt32)) != sizeof(RwInt32)
            || RwStreamRead(stream, &index, sizeof(index)) != sizeof(index)
            || RwStreamRead(stream, &nodeFlags[i], sizeof(RwUInt32)) != sizeof(RwUInt32)
        ) {
            ok = FALSE;
            break;
        }
    }

    if (ok) {
        extension->hierarchy = RpHAnimHierarchyCreate(
            numNodes,
            nodeFlags,
            nodeIDs,
            (RpHAnimHierarchyFlag)flags,
            maxKeySize
        );
        if (extension->hierarchy) {
            extension->hierarchy->parentFrame = static_cast<RwFrame*>(object);
        }
    }

    notsa::fakerw::Free(nodeIDs);
    notsa::fakerw::Free(nodeFlags);
    return (ok && extension->hierarchy) ? stream : nullptr;
}

RwStream* HAnimFrameStreamWrite(RwStream* stream, RwInt32, const void* object, RwInt32 offsetInObject, RwInt32) {
    const auto* extension = RWPLUGINOFFSETCONST(RpHAnimFrameExtension, object, offsetInObject);
    const auto* hierarchy = extension->hierarchy;

    const RwInt32 version  = rpHANIMSTREAMCURRENTVERSION;
    const RwInt32 numNodes = hierarchy ? hierarchy->numNodes : 0;
    RwStreamWrite(stream, &version, sizeof(version));
    RwStreamWrite(stream, &extension->id, sizeof(extension->id));
    RwStreamWrite(stream, &numNodes, sizeof(numNodes));

    if (!hierarchy) {
        return stream;
    }

    RwStreamWrite(stream, &hierarchy->flags, sizeof(hierarchy->flags));
    const RwInt32 maxKeySize = hierarchy->currentAnim->maxInterpKeyFrameSize;
    RwStreamWrite(stream, &maxKeySize, sizeof(maxKeySize));

    for (RwInt32 i = 0; i < hierarchy->numNodes; i++) {
        const auto& node = hierarchy->pNodeInfo[i];
        RwStreamWrite(stream, &node.nodeID, sizeof(node.nodeID));
        RwStreamWrite(stream, &node.nodeIndex, sizeof(node.nodeIndex));
        RwStreamWrite(stream, &node.flags, sizeof(node.flags));
    }
    return stream;
}

RwInt32 HAnimFrameStreamGetSize(const void* object, RwInt32 offsetInObject, RwInt32) {
    const auto* extension = RWPLUGINOFFSETCONST(RpHAnimFrameExtension, object, offsetInObject);
    if (!extension->hierarchy) {
        return 12; // version, id, numNodes
    }
    return 12 + 8 + extension->hierarchy->numNodes * 12;
}

//-----------------------------------------------------------------------------
// Frames attached to a hierarchy
//-----------------------------------------------------------------------------

bool IsFrameAttachedToHierarchy(const RpHAnimHierarchy* hierarchy, const RwFrame* frame) {
    for (RwInt32 i = 0; i < hierarchy->numNodes; i++) {
        if (hierarchy->pNodeInfo[i].pFrame == frame) {
            return true;
        }
    }
    return false;
}

RwFrame* FindUnattachedFrameWithID(RpHAnimHierarchy* hierarchy, RwFrame* frame, RwInt32 id) {
    while (frame) {
        if (GetFrameExtension(frame)->id == id && !IsFrameAttachedToHierarchy(hierarchy, frame)) {
            return frame;
        }
        if (frame->child) {
            if (RwFrame* found = FindUnattachedFrameWithID(hierarchy, frame->child, id)) {
                return found;
            }
        }
        frame = frame->next;
    }
    return nullptr;
}

void AttachNodeIndex(RpHAnimHierarchy* hierarchy, RwInt32 nodeIndex) {
    hierarchy->pNodeInfo[nodeIndex].pFrame = FindUnattachedFrameWithID(
        hierarchy,
        hierarchy->parentFrame,
        hierarchy->pNodeInfo[nodeIndex].nodeID
    );
}

} // namespace

//-----------------------------------------------------------------------------
// Plugin attach
//-----------------------------------------------------------------------------

RwBool RpHAnimPluginAttach() {
    static bool attached = false;
    if (!attached) {
        g_hanimFrameOffset = RwFrameRegisterPlugin(
            sizeof(RpHAnimFrameExtension),
            rwID_HANIMPLUGIN,
            HAnimFrameConstructor,
            HAnimFrameDestructor,
            HAnimFrameCopy
        );
        if (g_hanimFrameOffset == -1) {
            return FALSE;
        }

        RwFrameRegisterPluginStream(
            rwID_HANIMPLUGIN,
            HAnimFrameStreamRead,
            HAnimFrameStreamWrite,
            HAnimFrameStreamGetSize
        );

        static RtAnimInterpolatorInfo stdInfo{
            .typeID                  = kStdKeyFrameTypeID,
            .interpKeyFrameSize      = (RwInt32)sizeof(RpHAnimInterpFrame),
            .animKeyFrameSize        = (RwInt32)sizeof(RpHAnimKeyFrame),
            .keyFrameApplyCB         = RpHAnimKeyFrameApply,
            .keyFrameBlendCB         = RpHAnimKeyFrameBlend,
            .keyFrameInterpolateCB   = RpHAnimKeyFrameInterpolate,
            .keyFrameAddCB           = RpHAnimKeyFrameAdd,
            .keyFrameMulRecipCB      = RpHAnimKeyFrameMulRecip,
            .keyFrameStreamReadCB    = RpHAnimKeyFrameStreamRead,
            .keyFrameStreamWriteCB   = RpHAnimKeyFrameStreamWrite,
            .keyFrameStreamGetSizeCB = RpHAnimKeyFrameStreamGetSize,
            .customDataSize          = 0
        };
        RtAnimRegisterInterpolationScheme(&stdInfo);

        attached = true;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Hierarchy
//-----------------------------------------------------------------------------

void RpHAnimHierarchySetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    // Native allocations do not use free lists.
    UNUSED(blockSize);
    UNUSED(numBlocksToPrealloc);
}

RpHAnimHierarchy* RpHAnimHierarchyCreate(
    RwInt32              numNodes,
    RwUInt32*            nodeFlags,
    RwInt32*             nodeIDs,
    RpHAnimHierarchyFlag flags,
    RwInt32              maxInterpKeyFrameSize
) {
    if (numNodes <= 0) {
        return nullptr;
    }

    if (maxInterpKeyFrameSize <= 0) {
        maxInterpKeyFrameSize = (RwInt32)sizeof(RpHAnimInterpFrame);
    }

    auto* hierarchy = static_cast<RpHAnimHierarchy*>(notsa::fakerw::Malloc(sizeof(RpHAnimHierarchy)));
    if (!hierarchy) {
        return nullptr;
    }
    std::memset(hierarchy, 0, sizeof(*hierarchy));

    hierarchy->currentAnim = RtAnimInterpolatorCreate(numNodes, maxInterpKeyFrameSize);
    if (!hierarchy->currentAnim) {
        notsa::fakerw::Free(hierarchy);
        return nullptr;
    }

    hierarchy->flags             = flags;
    hierarchy->numNodes          = numNodes;
    hierarchy->parentFrame       = nullptr;
    hierarchy->parentHierarchy   = hierarchy;
    hierarchy->rootParentOffset  = 0;

    if (flags & rpHANIMHIERARCHYNOMATRICES) {
        hierarchy->pMatrixArray          = nullptr;
        hierarchy->pMatrixArrayUnaligned = nullptr;
    } else {
        const size_t bytes = sizeof(RwMatrix) * numNodes + 15;
        hierarchy->pMatrixArrayUnaligned = notsa::fakerw::Malloc(bytes);
        if (!hierarchy->pMatrixArrayUnaligned) {
            RtAnimInterpolatorDestroy(hierarchy->currentAnim);
            notsa::fakerw::Free(hierarchy);
            return nullptr;
        }
        std::memset(hierarchy->pMatrixArrayUnaligned, 0, bytes);
        hierarchy->pMatrixArray = reinterpret_cast<RwMatrix*>(
            (reinterpret_cast<uintptr_t>(hierarchy->pMatrixArrayUnaligned) + 15) & ~uintptr_t{15}
        );
    }

    hierarchy->pNodeInfo = static_cast<RpHAnimNodeInfo*>(notsa::fakerw::Malloc(sizeof(RpHAnimNodeInfo) * numNodes));
    if (!hierarchy->pNodeInfo) {
        notsa::fakerw::Free(hierarchy->pMatrixArrayUnaligned);
        RtAnimInterpolatorDestroy(hierarchy->currentAnim);
        notsa::fakerw::Free(hierarchy);
        return nullptr;
    }

    for (RwInt32 i = 0; i < numNodes; i++) {
        auto& node    = hierarchy->pNodeInfo[i];
        node.nodeID   = nodeIDs ? nodeIDs[i] : 0;
        node.nodeIndex = i;
        node.flags    = nodeFlags ? (RwInt32)nodeFlags[i] : 0;
        node.pFrame   = nullptr;
    }

    return hierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyCreateFromHierarchy(
    RpHAnimHierarchy*    hierarchy,
    RpHAnimHierarchyFlag flags,
    RwInt32              maxInterpKeyFrameSize
) {
    if (!hierarchy) {
        return nullptr;
    }

    RpHAnimHierarchy* newHierarchy = RpHAnimHierarchyCreate(
        hierarchy->numNodes,
        nullptr,
        nullptr,
        flags,
        maxInterpKeyFrameSize
    );
    if (!newHierarchy) {
        return nullptr;
    }

    for (RwInt32 i = 0; i < hierarchy->numNodes; i++) {
        newHierarchy->pNodeInfo[i].nodeID = hierarchy->pNodeInfo[i].nodeID;
        newHierarchy->pNodeInfo[i].flags  = hierarchy->pNodeInfo[i].flags;
    }
    return newHierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyCreateSubHierarchy(
    RpHAnimHierarchy*    parentHierarchy,
    RwInt32              startNode,
    RpHAnimHierarchyFlag flags,
    RwInt32              maxInterpKeyFrameSize
) {
    if (!parentHierarchy || startNode < 0 || startNode >= parentHierarchy->numNodes) {
        return nullptr;
    }

    const RwInt32 numNodes = parentHierarchy->numNodes - startNode;
    RpHAnimHierarchy* hierarchy = RpHAnimHierarchyCreate(
        numNodes,
        nullptr,
        nullptr,
        (RpHAnimHierarchyFlag)(flags | rpHANIMHIERARCHYSUBHIERARCHY),
        maxInterpKeyFrameSize
    );
    if (!hierarchy) {
        return nullptr;
    }

    hierarchy->parentHierarchy  = parentHierarchy;
    hierarchy->parentFrame      = parentHierarchy->parentFrame;
    hierarchy->rootParentOffset = startNode;

    for (RwInt32 i = 0; i < numNodes; i++) {
        const auto& src = parentHierarchy->pNodeInfo[startNode + i];
        auto&       dst = hierarchy->pNodeInfo[i];
        dst.nodeID = src.nodeID;
        dst.flags  = src.flags;
        dst.pFrame = src.pFrame;
    }
    return hierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyDestroy(RpHAnimHierarchy* hierarchy) {
    if (!hierarchy) {
        return nullptr;
    }

    if (hierarchy->currentAnim) {
        RtAnimInterpolatorDestroy(hierarchy->currentAnim);
    }
    notsa::fakerw::Free(hierarchy->pMatrixArrayUnaligned);
    notsa::fakerw::Free(hierarchy->pNodeInfo);
    notsa::fakerw::Free(hierarchy);
    return nullptr;
}

RpHAnimHierarchy* RpHAnimHierarchyAttach(RpHAnimHierarchy* hierarchy) {
    if (!hierarchy || !hierarchy->parentFrame) {
        return hierarchy;
    }
    for (RwInt32 i = 0; i < hierarchy->numNodes; i++) {
        AttachNodeIndex(hierarchy, i);
    }
    return hierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyDetach(RpHAnimHierarchy* hierarchy) {
    if (!hierarchy) {
        return nullptr;
    }
    for (RwInt32 i = 0; i < hierarchy->numNodes; i++) {
        hierarchy->pNodeInfo[i].pFrame = nullptr;
    }
    return hierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyAttachFrameIndex(RpHAnimHierarchy* hierarchy, RwInt32 nodeIndex) {
    if (!hierarchy || nodeIndex < 0 || nodeIndex >= hierarchy->numNodes || !hierarchy->parentFrame) {
        return hierarchy;
    }
    AttachNodeIndex(hierarchy, nodeIndex);
    return hierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyDetachFrameIndex(RpHAnimHierarchy* hierarchy, RwInt32 nodeIndex) {
    if (!hierarchy || nodeIndex < 0 || nodeIndex >= hierarchy->numNodes) {
        return hierarchy;
    }
    hierarchy->pNodeInfo[nodeIndex].pFrame = nullptr;
    return hierarchy;
}

RwBool RpHAnimFrameSetHierarchy(RwFrame* frame, RpHAnimHierarchy* hierarchy) {
    if (!frame || g_hanimFrameOffset < 0) {
        return FALSE;
    }

    GetFrameExtension(frame)->hierarchy = hierarchy;

    if (hierarchy && !hierarchy->parentFrame) {
        hierarchy->parentFrame = frame;
        if (!(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
            RpHAnimHierarchyAttach(hierarchy);
        }
    }
    return TRUE;
}

RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame) {
    if (!frame || g_hanimFrameOffset < 0) {
        return nullptr;
    }
    return GetFrameExtension(frame)->hierarchy;
}

RwBool RpHAnimFrameSetID(RwFrame* frame, RwInt32 id) {
    if (!frame || g_hanimFrameOffset < 0) {
        return FALSE;
    }
    GetFrameExtension(frame)->id = id;
    return TRUE;
}

RwInt32 RpHAnimFrameGetID(RwFrame* frame) {
    if (!frame || g_hanimFrameOffset < 0) {
        return -1;
    }
    return GetFrameExtension(frame)->id;
}

//-----------------------------------------------------------------------------
// Matrices
//-----------------------------------------------------------------------------

RwMatrix* RpHAnimHierarchyGetMatrixArray(RpHAnimHierarchy* hierarchy) {
    return hierarchy ? hierarchy->pMatrixArray : nullptr;
}

RwMatrix* RpHAnimHierarchyGetNodeMatrix(RpHAnimHierarchy* hierarchy, RwInt32 nodeID) {
    const RwInt32 index = RpHAnimIDGetIndex(hierarchy, nodeID);
    if (index < 0 || index >= hierarchy->numNodes) {
        return nullptr;
    }
    return &hierarchy->pMatrixArray[index];
}

RwInt32 RpHAnimIDGetIndex(RpHAnimHierarchy* hierarchy, RwInt32 ID) {
    if (!hierarchy) {
        return -1;
    }
    for (RwInt32 i = 0; i < hierarchy->numNodes; i++) {
        if (hierarchy->pNodeInfo[i].nodeID == ID) {
            return i;
        }
    }
    return -1;
}

RwBool RpHAnimHierarchyUpdateMatrices(RpHAnimHierarchy* hierarchy) {
    // Mirrors the retail `RpHAnimHierarchyUpdateMatrices` (0x7C51D0):
    //  - node matrices accumulate down the tree using the node push/pop flags;
    //  - `rpHANIMHIERARCHYUPDATEMODELLINGMATRICES` writes the *local* animation matrix to the
    //    node's frame, so the frame tree itself replays the animation;
    //  - `rpHANIMHIERARCHYUPDATELTMS` writes the accumulated matrix straight to the frame LTM.
    if (!hierarchy || !hierarchy->currentAnim || !hierarchy->currentAnim->pCurrentAnim) {
        return FALSE;
    }

    const RwInt32 flags   = hierarchy->flags;
    const bool    hasArray = !(flags & rpHANIMHIERARCHYNOMATRICES);
    if (hasArray && !hierarchy->pMatrixArray) {
        return FALSE;
    }

    // The DFF reader assigns the hierarchy while the frame list is still being built, so the
    // node -> frame mapping can only be resolved once the tree exists: attach on first update.
    if (hierarchy->parentFrame && hierarchy->numNodes > 0 && !hierarchy->pNodeInfo[0].pFrame) {
        RpHAnimHierarchyAttach(hierarchy);
    }

    RwMatrix identityMatrix;
    SetIdentityMatrix(&identityMatrix);

    // World-space root: the LTM of the frame the hierarchy's parent frame hangs from.
    RwMatrix        worldRootStorage;
    const RwMatrix* worldRoot = &identityMatrix;
    if (hierarchy->parentFrame) {
        if (RwFrame* parent = RwFrameGetParent(hierarchy->parentFrame)) {
            if (RwMatrix* parentLTM = RwFrameGetLTM(parent)) {
                worldRootStorage = *parentLTM;
                worldRoot        = &worldRootStorage;
            }
        }
    }

    // A sub-hierarchy continues from the matrix of its first node in the parent hierarchy.
    const bool isSubHierarchy = (flags & rpHANIMHIERARCHYSUBHIERARCHY)
        && hierarchy->rootParentOffset >= 0
        && hierarchy->parentHierarchy
        && hierarchy->parentHierarchy->pMatrixArray
        && hierarchy->rootParentOffset < hierarchy->parentHierarchy->numNodes;

    const RwMatrix* parentMatrix = isSubHierarchy
        ? &hierarchy->parentHierarchy->pMatrixArray[hierarchy->rootParentOffset]
        : ((flags & rpHANIMHIERARCHYLOCALSPACEMATRICES) ? &identityMatrix : worldRoot);

    const RwInt32 numNodes = hierarchy->numNodes;

    // Without a matrix array the accumulated matrices are still needed for the push/pop stack.
    RwMatrix* const matrices = hasArray
        ? hierarchy->pMatrixArray
        : static_cast<RwMatrix*>(notsa::fakerw::Malloc(sizeof(RwMatrix) * numNodes));
    if (!matrices) {
        return FALSE;
    }

    const RwMatrix* stack[64];
    RwInt32         stackSize = 0;

    // The interpolated frames are laid out by the interpolator's *current* scheme: `RpAnimBlend`
    // stores a headerless `RpHAnimBlendInterpFrame`, so its own apply callback must be used.
    RtAnimKeyFrameApplyCallBack applyCB = hierarchy->currentAnim->keyFrameApplyCB;

    for (RwInt32 i = 0; i < numNodes; i++) {
        RwMatrix animMatrix;
        SetIdentityMatrix(&animMatrix);

        // Local (parent-relative) animation matrix of this node.
        void* interpFrame = rtANIMGETINTERPFRAME(hierarchy->currentAnim, i);
        if (applyCB) {
            applyCB(&animMatrix, interpFrame);
        } else {
            RpHAnimKeyFrameApply(&animMatrix, interpFrame);
        }

        RwMatrix* current = &matrices[i];
        RwMatrixMultiply(current, &animMatrix, parentMatrix);

        if (RwFrame* frame = hierarchy->pNodeInfo[i].pFrame) {
            if (flags & rpHANIMHIERARCHYUPDATEMODELLINGMATRICES) {
                frame->modelling = animMatrix;
                if (!(flags & rpHANIMHIERARCHYUPDATELTMS)) {
                    RwFrameUpdateObjects(frame); // Syncs the objects attached to this bone.
                }
            }

            if (flags & rpHANIMHIERARCHYUPDATELTMS) {
                if (flags & rpHANIMHIERARCHYLOCALSPACEMATRICES) {
                    RwMatrixMultiply(&frame->ltm, current, worldRoot);
                } else {
                    frame->ltm = *current;
                }
                frame->object.privateFlags = (frame->object.privateFlags & ~0x04) | 0x08;
            }
        }

        const RwInt32 nodeFlags = hierarchy->pNodeInfo[i].flags & 3;
        if (nodeFlags == rpHANIMPUSHPARENTMATRIX) {
            if (stackSize < (RwInt32)(sizeof(stack) / sizeof(stack[0]))) {
                stack[stackSize++] = parentMatrix;
            }
            parentMatrix = current;
        } else if (nodeFlags == rpHANIMPOPPARENTMATRIX) {
            if (stackSize > 0) {
                parentMatrix = stack[--stackSize];
            }
        } else {
            parentMatrix = current;
        }
    }

    // The frame tree now carries the animation, so refresh the LTMs and the objects attached
    // to the frames (the retail library defers this through the frame dirty list).
    if ((flags & rpHANIMHIERARCHYUPDATELTMS) && (flags & rpHANIMHIERARCHYUPDATEMODELLINGMATRICES)
        && hierarchy->parentFrame
    ) {
        RwFrameUpdateObjects(hierarchy->parentFrame);
    }

    if (!hasArray) {
        notsa::fakerw::Free(matrices);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Standard keyframe callbacks
//-----------------------------------------------------------------------------

void RpHAnimKeyFrameApply(void* matrix, void* voidIFrame) {
    RpHAnimKeyFrameToMatrixMacro(static_cast<RwMatrix*>(matrix), voidIFrame);
}

void RpHAnimKeyFrameBlend(void* voidOut, void* voidIn1, void* voidIn2, RwReal alpha) {
    auto* out = static_cast<RpHAnimInterpFrame*>(voidOut);
    const auto* in1 = static_cast<const RpHAnimInterpFrame*>(voidIn1);
    const auto* in2 = static_cast<const RpHAnimInterpFrame*>(voidIn2);

    SlerpQuat(&out->q, const_cast<RtQuat*>(&in1->q), const_cast<RtQuat*>(&in2->q), alpha);
    LerpV3d(&out->t, &in1->t, &in2->t, alpha);
}

void RpHAnimKeyFrameInterpolate(void* voidOut, void* voidIn1, void* voidIn2, RwReal time, void* customData) {
    UNUSED(customData);

    auto* out = static_cast<RpHAnimInterpFrame*>(voidOut);
    const auto* in1 = static_cast<const RpHAnimKeyFrame*>(voidIn1);
    const auto* in2 = static_cast<const RpHAnimKeyFrame*>(voidIn2);

    const RwReal span = in2->time - in1->time;
    RwReal alpha;
    if (span > 0.0f) {
        alpha = (time - in1->time) / span;
        alpha = std::clamp(alpha, 0.0f, 1.0f);
    } else {
        alpha = 1.0f;
    }

    SlerpQuat(&out->q, const_cast<RtQuat*>(&in1->q), const_cast<RtQuat*>(&in2->q), alpha);
    LerpV3d(&out->t, &in1->t, &in2->t, alpha);
}

void RpHAnimKeyFrameAdd(void* voidOut, void* voidIn1, void* voidIn2) {
    auto* out = static_cast<RpHAnimInterpFrame*>(voidOut);
    auto* in1 = static_cast<RpHAnimInterpFrame*>(voidIn1);
    auto* in2 = static_cast<RpHAnimInterpFrame*>(voidIn2);

    RtQuatMultiply(&out->q, &in1->q, &in2->q);
    out->t.x = in1->t.x + in2->t.x;
    out->t.y = in1->t.y + in2->t.y;
    out->t.z = in1->t.z + in2->t.z;
}

void RpHAnimKeyFrameMulRecip(void* voidFrame, void* voidStart) {
    auto* frame = static_cast<RpHAnimKeyFrame*>(voidFrame);
    auto* start = static_cast<RpHAnimKeyFrame*>(voidStart);

    RtQuat reciprocal;
    RtQuatReciprocal(&reciprocal, &start->q);

    // `RtQuatMultiplyMacro` requires an output distinct from its inputs.
    const RtQuat frameRotation = frame->q;
    RtQuatMultiply(&frame->q, &reciprocal, &frameRotation);

    frame->t.x -= start->t.x;
    frame->t.y -= start->t.y;
    frame->t.z -= start->t.z;
}

RtAnimAnimation* RpHAnimKeyFrameStreamRead(RwStream* stream, RtAnimAnimation* animation) {
    if (!stream || !animation || !animation->pFrames) {
        return nullptr;
    }

    auto* frames = static_cast<RwUInt8*>(animation->pFrames);
    const RwInt32 frameStride = animation->interpInfo->animKeyFrameSize;

    for (RwInt32 i = 0; i < animation->numFrames; i++) {
        auto* keyFrame = reinterpret_cast<RpHAnimKeyFrame*>(frames + i * frameStride);

        RwReal time = 0.0f;
        RtQuat quat{};
        RwV3d  translation{};
        RwInt32 prevOffset = 0;

        if (RwStreamRead(stream, &time, sizeof(time)) != sizeof(time)
            || RwStreamRead(stream, &quat, sizeof(quat)) != sizeof(quat)
            || RwStreamRead(stream, &translation, sizeof(translation)) != sizeof(translation)
            || RwStreamRead(stream, &prevOffset, sizeof(prevOffset)) != sizeof(prevOffset)
        ) {
            return nullptr;
        }

        keyFrame->time = time;
        keyFrame->q    = quat;
        keyFrame->t    = translation;

        // The stored offset is in 32 bit keyframe units.
        const RwInt32 prevIndex = prevOffset / kStdKeyFrameDiskSize;
        keyFrame->prevFrame = (prevIndex >= 0 && prevIndex < animation->numFrames)
            ? reinterpret_cast<RpHAnimKeyFrame*>(frames + prevIndex * frameStride)
            : keyFrame;
    }
    return animation;
}

RwBool RpHAnimKeyFrameStreamWrite(const RtAnimAnimation* animation, RwStream* stream) {
    if (!animation || !stream || !animation->pFrames) {
        return FALSE;
    }

    const auto* frames = static_cast<const RwUInt8*>(animation->pFrames);
    const RwInt32 frameStride = animation->interpInfo->animKeyFrameSize;
    for (RwInt32 i = 0; i < animation->numFrames; i++) {
        const auto* keyFrame = reinterpret_cast<const RpHAnimKeyFrame*>(frames + i * frameStride);

        const RwInt32 prevIndex = keyFrame->prevFrame
            ? (RwInt32)(reinterpret_cast<const RwUInt8*>(keyFrame->prevFrame) - frames) / frameStride
            : i;
        const RwInt32 prevOffset = prevIndex * kStdKeyFrameDiskSize;

        RwStreamWrite(stream, &keyFrame->time, sizeof(keyFrame->time));
        RwStreamWrite(stream, &keyFrame->q, sizeof(keyFrame->q));
        RwStreamWrite(stream, &keyFrame->t, sizeof(keyFrame->t));
        RwStreamWrite(stream, &prevOffset, sizeof(prevOffset));
    }
    return TRUE;
}

RwInt32 RpHAnimKeyFrameStreamGetSize(const RtAnimAnimation* animation) {
    return animation ? animation->numFrames * kStdKeyFrameDiskSize : 0;
}

#endif // NOTSA_NATIVE
