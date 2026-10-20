#include "StdInc.h"

#include "rpskin.h"

void RpSkinSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x7C67F0, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwBool RpSkinPluginAttach() {
    return plugin::CallAndReturn<RwBool, 0x7C6820>();
}

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, RpHAnimHierarchy* hierarchy) {
    return plugin::CallAndReturn<RpAtomic*, 0x7C7520, RpAtomic*, RpHAnimHierarchy*>(atomic, hierarchy);
}

RpHAnimHierarchy* RpSkinAtomicGetHAnimHierarchy(const RpAtomic* atomic) {
    return plugin::CallAndReturn<RpHAnimHierarchy*, 0x7C7540, const RpAtomic*>(atomic);
}

RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin) {
    return plugin::CallAndReturn<RpGeometry*, 0x7C7560, RpGeometry*, RpSkin*>(geometry, skin);
}

RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry) {
    return plugin::CallAndReturn<RpSkin*, 0x7C7550, RpGeometry*>(geometry);
}

RpSkin* RpSkinCreate(RwUInt32 numVertices, RwUInt32 numBones, RwMatrixWeights* vertexWeights, RwUInt32* vertexIndices, RwMatrix* inverseMatrices) {
    return plugin::CallAndReturn<RpSkin*, 0x7C75B0, RwUInt32, RwUInt32, RwMatrixWeights*, RwUInt32*, RwMatrix*>(numVertices, numBones, vertexWeights, vertexIndices, inverseMatrices);
}

RpSkin* RpSkinDestroy(RpSkin* skin) {
    return plugin::CallAndReturn<RpSkin*, 0x7C77A0, RpSkin*>(skin);
}

RwUInt32 RpSkinGetNumBones(RpSkin* skin) {
    return plugin::CallAndReturn<RwUInt32, 0x7C77E0, RpSkin*>(skin);
}

/*const*/ RwMatrixWeights* RpSkinGetVertexBoneWeights(RpSkin* skin) {
    return plugin::CallAndReturn<RwMatrixWeights*, 0x7C77F0, RpSkin*>(skin);
}

const RwUInt32* RpSkinGetVertexBoneIndices(RpSkin* skin) {
    return plugin::CallAndReturn<const RwUInt32*, 0x7C7800, RpSkin*>(skin);
}

const RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin) {
    return plugin::CallAndReturn<const RwMatrix*, 0x7C7810, RpSkin*>(skin);
}

RwBool RpSkinIsSplit(RpSkin* skin) {
    return plugin::CallAndReturn<RwBool, 0x7C7820, RpSkin*>(skin);
}

RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type) {
    return plugin::CallAndReturn<RpAtomic*, 0x7C7830, RpAtomic*, RpSkinType>(atomic, type);
}

RpSkinType RpSkinAtomicGetType(RpAtomic* atomic) {
    return plugin::CallAndReturn<RpSkinType, 0x7C7880, RpAtomic*>(atomic);
}

RpGeometry* _rpSkinInitialize(RpGeometry* geometry) {
    return plugin::CallAndReturn<RpGeometry*, 0x7C8740, RpGeometry*>(geometry);
}

RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry) {
    return plugin::CallAndReturn<RpGeometry*, 0x7C8820, RpGeometry*>(geometry);
}

RwUInt8* _rpSkinGetMeshBoneRemapIndices(RpSkin* skin) {
    return plugin::CallAndReturn<RwUInt8*, 0x7C8A40, RpSkin*>(skin);
}

RwUInt8* _rpSkinGetMeshBoneRLECount(RpSkin* skin) {
    return plugin::CallAndReturn<RwUInt8*, 0x7C8A50, RpSkin*>(skin);
}

RwUInt8* _rpSkinGetMeshBoneRLE(RpSkin* skin) {
    return plugin::CallAndReturn<RwUInt8*, 0x7C8A60, RpSkin*>(skin);
}

RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, RwUInt32 boneLimit, RwUInt32 numMatrices, RwUInt32 numMeshes, RwUInt32 numRLE) {
    return plugin::CallAndReturn<RpSkin*, 0x7C8A70, RpSkin*, RwUInt32, RwUInt32, RwUInt32, RwUInt32>(skin, boneLimit, numMatrices, numMeshes, numRLE);
}

RwBool _rpSkinSplitDataDestroy(RpSkin* skin) {
    return plugin::CallAndReturn<RwBool, 0x7C8B10, RpSkin*>(skin);
}

RxNodeDefinition* RxNodeDefinitionGetD3D9SkinAtomicAllInOne() {
    return plugin::CallAndReturn<RxNodeDefinition*, 0x7CB2A0>();
}
