#include "StdInc.h"

#include "rphanim.h"

void RpHAnimHierarchySetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x7C45E0, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RpHAnimHierarchy* RpHAnimHierarchyCreate(RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs, RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C4C30, RwInt32, RwUInt32*, RwInt32*, RpHAnimHierarchyFlag, RwInt32>(numNodes, nodeFlags, nodeIDs, flags, maxInterpKeyFrameSize);
}

RpHAnimHierarchy* RpHAnimHierarchyCreateFromHierarchy(RpHAnimHierarchy* hierarchy, RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C4ED0, RpHAnimHierarchy*, RpHAnimHierarchyFlag, RwInt32>(hierarchy, flags, maxInterpKeyFrameSize);
}

RpHAnimHierarchy* RpHAnimHierarchyDestroy(RpHAnimHierarchy* hierarchy) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C4D30, RpHAnimHierarchy*>(hierarchy);
}

RpHAnimHierarchy* RpHAnimHierarchyCreateSubHierarchy(RpHAnimHierarchy* parentHierarchy, RwInt32 startNode, RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C4DB0, RpHAnimHierarchy*, RwInt32, RpHAnimHierarchyFlag, RwInt32>(parentHierarchy, startNode, flags, maxInterpKeyFrameSize);
}

RpHAnimHierarchy* RpHAnimHierarchyAttach(RpHAnimHierarchy* hierarchy) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C4F40, RpHAnimHierarchy*>(hierarchy);
}

RpHAnimHierarchy* RpHAnimHierarchyDetach(RpHAnimHierarchy* hierarchy) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C4FF0, RpHAnimHierarchy*>(hierarchy);
}

RpHAnimHierarchy* RpHAnimHierarchyAttachFrameIndex(RpHAnimHierarchy* hierarchy, RwInt32 nodeIndex) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C5020, RpHAnimHierarchy*, RwInt32>(hierarchy, nodeIndex);
}

RpHAnimHierarchy* RpHAnimHierarchyDetachFrameIndex(RpHAnimHierarchy* hierarchy, RwInt32 nodeIndex) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C5100, RpHAnimHierarchy*, RwInt32>(hierarchy, nodeIndex);
}

RwBool RpHAnimFrameSetHierarchy(RwFrame* frame, RpHAnimHierarchy* hierarchy) {
    return plugin::CallAndReturn<RwBool, 0x7C5130, RwFrame*, RpHAnimHierarchy*>(frame, hierarchy);
}

RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C5160, RwFrame*>(frame);
}

RwMatrix* RpHAnimHierarchyGetMatrixArray(RpHAnimHierarchy* hierarchy) {
    return plugin::CallAndReturn<RwMatrix*, 0x7C5120, RpHAnimHierarchy*>(hierarchy);
}

// NOTSA
RwMatrix* RpHAnimHierarchyGetNodeMatrix(RpHAnimHierarchy* hierarchy, RwInt32 nodeID) {
    const auto i = RpHAnimIDGetIndex(hierarchy, nodeID);
    assert(i >= 0 && i < hierarchy->numNodes);
    return &RpHAnimHierarchyGetMatrixArray(hierarchy)[i];
}

RwBool RpHAnimHierarchyUpdateMatrices(RpHAnimHierarchy* hierarchy) {
    return plugin::CallAndReturn<RwBool, 0x7C51D0, RpHAnimHierarchy*>(hierarchy);
}

RwInt32 RpHAnimIDGetIndex(RpHAnimHierarchy* hierarchy, RwInt32 ID) {
    return plugin::CallAndReturn<RwInt32, 0x7C51A0, RpHAnimHierarchy*, RwInt32>(hierarchy, ID);
}

RwBool RpHAnimPluginAttach() {
    return plugin::CallAndReturn<RwBool, 0x7C4600>();
}

void RpHAnimKeyFrameApply(void* matrix, void* voidIFrame) {
    plugin::Call<0x7C5B80, void*, void*>(matrix, voidIFrame);
}

void RpHAnimKeyFrameBlend(void* voidOut, void* voidIn1, void* voidIn2, RwReal alpha) {
    plugin::Call<0x7C60C0, void*, void*, void*, RwReal>(voidOut, voidIn1, voidIn2, alpha);
}

void RpHAnimKeyFrameInterpolate(void* voidOut, void* voidIn1, void* voidIn2, RwReal time, void* customData) {
    plugin::Call<0x7C5CA0, void*, void*, void*, RwReal, void*>(voidOut, voidIn1, voidIn2, time, customData);
}

void RpHAnimKeyFrameAdd(void* voidOut, void* voidIn1, void* voidIn2) {
    plugin::Call<0x7C6720, void*, void*, void*>(voidOut, voidIn1, voidIn2);
}

void RpHAnimKeyFrameMulRecip(void* voidFrame, void* voidStart) {
    plugin::Call<0x7C65C0, void*, void*>(voidFrame, voidStart);
}

RtAnimAnimation* RpHAnimKeyFrameStreamRead(RwStream* stream, RtAnimAnimation* animation) {
    return plugin::CallAndReturn<RtAnimAnimation*, 0x7C64C0, RwStream*, RtAnimAnimation*>(stream, animation);
}

RwBool RpHAnimKeyFrameStreamWrite(const RtAnimAnimation* animation, RwStream* stream) {
    return plugin::CallAndReturn<RwBool, 0x7C6540, const RtAnimAnimation*, RwStream*>(animation, stream);
}

RwInt32 RpHAnimKeyFrameStreamGetSize(const RtAnimAnimation* animation) {
    return plugin::CallAndReturn<RwInt32, 0x7C65B0, const RtAnimAnimation*>(animation);
}

RwBool RpHAnimFrameSetID(RwFrame* frame, RwInt32 id) {
    return plugin::CallAndReturn<RwBool, 0x7C5170, RwFrame*, RwInt32>(frame, id);
}

RwInt32 RpHAnimFrameGetID(RwFrame* frame) {
    return plugin::CallAndReturn<RwInt32, 0x7C5190, RwFrame*>(frame);
}
