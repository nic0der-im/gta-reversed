#include "StdInc.h"

#include "rwcore.h"

void RwCoreInjectHooks() {
    RH_ScopedCategory("Renderware");
    RH_ScopedNamespaceName("RwCore");

    RH_ScopedGlobalInstall(RwCameraBeginUpdate, 0x7EE190);
    RH_ScopedGlobalInstall(RwCameraEndUpdate, 0x7EE180);
    RH_ScopedGlobalInstall(RwCameraClear, 0x7EE340);
    RH_ScopedGlobalInstall(RwCameraShowRaster, 0x7EE370);
}

RxHeap* RxHeapCreate(RwUInt32 size) {
    return plugin::CallAndReturn<RxHeap*, 0x809F90, RwUInt32>(size);
}

void RxHeapDestroy(RxHeap* heap) {
    plugin::Call<0x809F30, RxHeap*>(heap);
}

RwBool _rxHeapReset(RxHeap* heap) {
    return plugin::CallAndReturn<RwBool, 0x809EC0, RxHeap*>(heap);
}

void* RxHeapAlloc(RxHeap* heap, RwUInt32 size) {
    return plugin::CallAndReturn<void*, 0x809AA0, RxHeap*, RwUInt32>(heap, size);
}

void RxHeapFree(RxHeap* heap, void* block) {
    plugin::Call<0x8098B0, RxHeap*, void*>(heap, block);
}

void* RxHeapRealloc(RxHeap* heap, void* block, RwUInt32 newSize, RwBool allowCopy) {
    return plugin::CallAndReturn<void*, 0x809D10, RxHeap*, void*, RwUInt32, RwBool>(heap, block, newSize, allowCopy);
}

void RxPipelineSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x804FC0, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwBool _rxPipelineOpen() {
    return plugin::CallAndReturn<RwBool, 0x804FE0>();
}

RwBool _rxPipelineClose() {
    return plugin::CallAndReturn<RwBool, 0x804F60>();
}

RxPipeline* RxPipelineCreate() {
    return plugin::CallAndReturn<RxPipeline*, 0x8057B0>();
}

void _rxPipelineDestroy(RxPipeline* Pipeline) {
    plugin::Call<0x805820, RxPipeline*>(Pipeline);
}

RxHeap* RxHeapGetGlobalHeap() {
    return plugin::CallAndReturn<RxHeap*, 0x8052F0>();
}

RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data, RwBool heapReset) {
    return plugin::CallAndReturn<RxPipeline*, 0x805710, RxPipeline*, void*, RwBool>(pipeline, data, heapReset);
}

RxPacket* RxPacketCreate(RxPipelineNode* node) {
    return plugin::CallAndReturn<RxPacket*, 0x805300, RxPipelineNode*>(node);
}

RxCluster* RxClusterSetStride(RxCluster* cluster, RwInt32 stride) {
    return plugin::CallAndReturn<RxCluster*, 0x805330, RxCluster*, RwInt32>(cluster, stride);
}

RxCluster* RxClusterSetExternalData(RxCluster* cluster, void* data, RwInt32 stride, RwInt32 numElements) {
    return plugin::CallAndReturn<RxCluster*, 0x805340, RxCluster*, void*, RwInt32, RwInt32>(cluster, data, stride, numElements);
}

RxCluster* RxClusterSetData(RxCluster* cluster, void* data, RwInt32 stride, RwInt32 numElements) {
    return plugin::CallAndReturn<RxCluster*, 0x8053A0, RxCluster*, void*, RwInt32, RwInt32>(cluster, data, stride, numElements);
}

void _rxPacketDestroy(RxPacket* Packet) {
    plugin::Call<0x80F070, RxPacket*>(Packet);
}

RxCluster* RxClusterInitializeData(RxCluster* cluster, RwUInt32 numElements, RwUInt16 stride) {
    return plugin::CallAndReturn<RxCluster*, 0x805400, RxCluster*, RwUInt32, RwUInt16>(cluster, numElements, stride);
}

RxCluster* RxClusterResizeData(RxCluster* CurrentCluster, RwUInt32 NumElements) {
    return plugin::CallAndReturn<RxCluster*, 0x805470, RxCluster*, RwUInt32>(CurrentCluster, NumElements);
}

RxCluster* RxClusterDestroyData(RxCluster* CurrentCluster) {
    return plugin::CallAndReturn<RxCluster*, 0x8054C0, RxCluster*>(CurrentCluster);
}

RxCluster* RxClusterLockWrite(RxPacket* packet, RwUInt32 clusterIndex, RxPipelineNode* node) {
    return plugin::CallAndReturn<RxCluster*, 0x8054F0, RxPacket*, RwUInt32, RxPipelineNode*>(packet, clusterIndex, node);
}

void RxClusterUnlock(RxCluster* cluster) {
    plugin::Call<0x8055C0, RxCluster*>(cluster);
}

RwUInt32 RxPipelineNodeSendConfigMsg(RxPipelineNode* dest, RwUInt32 msg, RwUInt32 intparam, void* ptrparam) {
    return plugin::CallAndReturn<RwUInt32, 0x8055D0, RxPipelineNode*, RwUInt32, RwUInt32, void*>(dest, msg, intparam, ptrparam);
}

RxPipelineNode* RxPipelineNodeForAllConnectedOutputs(RxPipelineNode* node, RxPipeline* pipeline, RxPipelineNodeOutputCallBack callbackfn, void* callbackdata) {
    return plugin::CallAndReturn<RxPipelineNode*, 0x8055F0, RxPipelineNode*, RxPipeline*, RxPipelineNodeOutputCallBack, void*>(node, pipeline, callbackfn, callbackdata);
}

RxPipelineCluster* RxPipelineNodeGetPipelineCluster(RxPipelineNode* node, RwUInt32 clustersOfInterestIndex) {
    return plugin::CallAndReturn<RxPipelineCluster*, 0x805680, RxPipelineNode*, RwUInt32>(node, clustersOfInterestIndex);
}

RwUInt32 RxPipelineClusterGetCreationAttributes(RxPipelineCluster* cluster) {
    return plugin::CallAndReturn<RwUInt32, 0x8056B0, RxPipelineCluster*>(cluster);
}

RxPipelineCluster* RxPipelineClusterSetCreationAttributes(RxPipelineCluster* cluster, RwUInt32 creationAttributes) {
    return plugin::CallAndReturn<RxPipelineCluster*, 0x8056C0, RxPipelineCluster*, RwUInt32>(cluster, creationAttributes);
}

RwUInt32 RxClusterGetAttributes(RxCluster* cluster) {
    return plugin::CallAndReturn<RwUInt32, 0x8056E0, RxCluster*>(cluster);
}

RxCluster* RxClusterSetAttributes(RxCluster* cluster, RwUInt32 attributes) {
    return plugin::CallAndReturn<RxCluster*, 0x8056F0, RxCluster*, RwUInt32>(cluster, attributes);
}

RxNodeOutput RxPipelineNodeFindOutputByName(RxPipelineNode* node, const RwChar* outputname) {
    return plugin::CallAndReturn<RxNodeOutput, 0x805D10, RxPipelineNode*, const RwChar*>(node, outputname);
}

RxNodeOutput RxPipelineNodeFindOutputByIndex(RxPipelineNode* node, RwUInt32 outputindex) {
    return plugin::CallAndReturn<RxNodeOutput, 0x805D70, RxPipelineNode*, RwUInt32>(node, outputindex);
}

RxNodeInput RxPipelineNodeFindInput(RxPipelineNode* node) {
    return plugin::CallAndReturn<RxNodeInput, 0x805DA0, RxPipelineNode*>(node);
}

RxNodeDefinition* RxPipelineNodeCloneDefinition(RxPipelineNode* node, RxClusterDefinition* cluster2add) {
    return plugin::CallAndReturn<RxNodeDefinition*, 0x8059C0, RxPipelineNode*, RxClusterDefinition*>(node, cluster2add);
}

RxPipeline* RxPipelineNodeRequestCluster(RxPipeline* pipeline, RxPipelineNode* node, RxClusterDefinition* clusterDef) {
    return plugin::CallAndReturn<RxPipeline*, 0x805DB0, RxPipeline*, RxPipelineNode*, RxClusterDefinition*>(pipeline, node, clusterDef);
}

RxPipeline* RxPipelineNodeReplaceCluster(RxPipeline* pipeline, RxPipelineNode* node, RxClusterDefinition* oldClusterDef, RxClusterDefinition* newClusterDef) {
    return plugin::CallAndReturn<RxPipeline*, 0x805E20, RxPipeline*, RxPipelineNode*, RxClusterDefinition*, RxClusterDefinition*>(pipeline, node, oldClusterDef, newClusterDef);
}

void* RxPipelineNodeGetInitData(RxPipelineNode* node) {
    return plugin::CallAndReturn<void*, 0x805F30, RxPipelineNode*>(node);
}

void* RxPipelineNodeCreateInitData(RxPipelineNode* node, RwUInt32 size) {
    return plugin::CallAndReturn<void*, 0x805EA0, RxPipelineNode*, RwUInt32>(node, size);
}

RxPipeline* RxPipelineClone(RxPipeline* pipeline) {
    return plugin::CallAndReturn<RxPipeline*, 0x806AC0, RxPipeline*>(pipeline);
}

RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name, RxPipelineNode* start, RwInt32* nodeIndex) {
    return plugin::CallAndReturn<RxPipelineNode*, 0x806B30, RxPipeline*, const RwChar*, RxPipelineNode*, RwInt32*>(pipeline, name, start, nodeIndex);
}

RxPipelineNode* RxPipelineFindNodeByIndex(RxPipeline* pipeline, RwUInt32 nodeindex) {
    return plugin::CallAndReturn<RxPipelineNode*, 0x806BC0, RxPipeline*, RwUInt32>(pipeline, nodeindex);
}

RxLockedPipe* RxPipelineLock(RxPipeline* pipeline) {
    return plugin::CallAndReturn<RxLockedPipe*, 0x806990, RxPipeline*>(pipeline);
}

RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline) {
    return plugin::CallAndReturn<RxPipeline*, 0x805F40, RxLockedPipe*>(pipeline);
}

RxPipeline* RxLockedPipeReplaceNode(RxLockedPipe* pipeline, RxPipelineNode* node, RxNodeDefinition* nodeDef) {
    return plugin::CallAndReturn<RxPipeline*, 0x806F20, RxLockedPipe*, RxPipelineNode*, RxNodeDefinition*>(pipeline, node, nodeDef);
}

RxPipeline* RxLockedPipeDeleteNode(RxLockedPipe* pipeline, RxPipelineNode* node) {
    return plugin::CallAndReturn<RxPipeline*, 0x807040, RxLockedPipe*, RxPipelineNode*>(pipeline, node);
}

RxPipeline* RxLockedPipeSetEntryPoint(RxLockedPipe* pipeline, RxNodeInput in) {
    return plugin::CallAndReturn<RxPipeline*, 0x807070, RxLockedPipe*, RxNodeInput>(pipeline, in);
}

RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline, RwUInt32* firstIndex, RxNodeDefinition* nodeDef0, RxNodeDefinition* __va_end__) {
    return plugin::CallAndReturn<RxPipeline*, 0x806BE0, RxLockedPipe*, RwUInt32*, RxNodeDefinition*, RxNodeDefinition*>(pipeline, firstIndex, nodeDef0, __va_end__);
}

RxPipelineNode* RxLockedPipeGetEntryPoint(RxLockedPipe* pipeline) {
    return plugin::CallAndReturn<RxPipelineNode*, 0x8070D0, RxLockedPipe*>(pipeline);
}

RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RxNodeOutput out, RxNodeInput in) {
    return plugin::CallAndReturn<RxPipeline*, 0x807100, RxLockedPipe*, RxNodeOutput, RxNodeInput>(pipeline, out, in);
}

RxPipeline* RxLockedPipeDeletePath(RxLockedPipe* pipeline, RxNodeOutput out, RxNodeInput in) {
    return plugin::CallAndReturn<RxPipeline*, 0x807170, RxLockedPipe*, RxNodeOutput, RxNodeInput>(pipeline, out, in);
}

RxPipeline* RxPipelineInsertDebugNode(RxPipeline* pipeline, RxPipelineNode* before, RxPipelineNode* after, RxNodeDefinition* debugNode) {
    return plugin::CallAndReturn<RxPipeline*, 0x8071B0, RxPipeline*, RxPipelineNode*, RxPipelineNode*, RxNodeDefinition*>(pipeline, before, after, debugNode);
}

void RwD3D9VertexBufferManagerChangeDefaultSize(RwUInt32 defaultSize) {
    plugin::Call<0x7F57F0, RwUInt32>(defaultSize);
}

RwBool RwD3D9CreateVertexBuffer(RwUInt32 stride, RwUInt32 size, void* vertexBuffer, RwUInt32* offset) {
    return plugin::CallAndReturn<RwBool, 0x7F5500, RwUInt32, RwUInt32, void*, RwUInt32*>(stride, size, vertexBuffer, offset);
}

void RwD3D9DestroyVertexBuffer(RwUInt32 stride, RwUInt32 size, void* vertexBuffer, RwUInt32 offset) {
    plugin::Call<0x7F56A0, RwUInt32, RwUInt32, void*, RwUInt32>(stride, size, vertexBuffer, offset);
}

RwBool RwD3D9DynamicVertexBufferCreate(RwUInt32 size, void* vertexBuffer) {
    return plugin::CallAndReturn<RwBool, 0x7F5A00, RwUInt32, void*>(size, vertexBuffer);
}

void RwD3D9DynamicVertexBufferDestroy() {
    plugin::Call<0x7F5AE0>();
}

RwBool RwD3D9DynamicVertexBufferLock(RwUInt32 vertexSize, RwUInt32 numVertex, void* vertexBufferOut, void* vertexDataOut, RwUInt32* baseIndexOut) {
    return plugin::CallAndReturn<RwBool, 0x7F5B10, RwUInt32, RwUInt32, void*, void*, RwUInt32*>(vertexSize, numVertex, vertexBufferOut, vertexDataOut, baseIndexOut);
}

RwBool RwD3D9DynamicVertexBufferUnlock() {
    return plugin::CallAndReturn<RwBool, 0x7F5C90>();
}

void RwRasterSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x7FB350, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) {
    return plugin::CallAndReturn<RwRaster*, 0x7FB230, RwInt32, RwInt32, RwInt32, RwInt32>(width, height, depth, flags);
}

RwBool RwRasterDestroy(RwRaster* raster) {
    return plugin::CallAndReturn<RwBool, 0x7FB020, RwRaster*>(raster);
}

RwRaster* RwRasterGetOffset(RwRaster* raster, RwInt16* xOffset, RwInt16* yOffset) {
    return plugin::CallAndReturn<RwRaster*, 0x7FAEA0, RwRaster*, RwInt16*, RwInt16*>(raster, xOffset, yOffset);
}

RwInt32 RwRasterGetNumLevels(RwRaster* raster) {
    return plugin::CallAndReturn<RwInt32, 0x7FB160, RwRaster*>(raster);
}

RwRaster* RwRasterSubRaster(RwRaster* subRaster, RwRaster* raster, RwRect* rect) {
    return plugin::CallAndReturn<RwRaster*, 0x7FB1D0, RwRaster*, RwRaster*, RwRect*>(subRaster, raster, rect);
}

RwRaster* RwRasterRenderFast(RwRaster* raster, RwInt32 x, RwInt32 y) {
    return plugin::CallAndReturn<RwRaster*, 0x7FAF50, RwRaster*, RwInt32, RwInt32>(raster, x, y);
}

RwRaster* RwRasterRender(RwRaster* raster, RwInt32 x, RwInt32 y) {
    return plugin::CallAndReturn<RwRaster*, 0x7FAFB0, RwRaster*, RwInt32, RwInt32>(raster, x, y);
}

RwRaster* RwRasterRenderScaled(RwRaster* raster, RwRect* rect) {
    return plugin::CallAndReturn<RwRaster*, 0x7FAE80, RwRaster*, RwRect*>(raster, rect);
}

RwRaster* RwRasterPushContext(RwRaster* raster) {
    return plugin::CallAndReturn<RwRaster*, 0x7FB060, RwRaster*>(raster);
}

RwRaster* RwRasterPopContext() {
    return plugin::CallAndReturn<RwRaster*, 0x7FB110>();
}

RwRaster* RwRasterGetCurrentContext() {
    return plugin::CallAndReturn<RwRaster*, 0x7FAE60>();
}

RwBool RwRasterClear(RwInt32 pixelValue) {
    return plugin::CallAndReturn<RwBool, 0x7FAEE0, RwInt32>(pixelValue);
}

RwBool RwRasterClearRect(RwRect* rpRect, RwInt32 pixelValue) {
    return plugin::CallAndReturn<RwBool, 0x7FAF90, RwRect*, RwInt32>(rpRect, pixelValue);
}

RwRaster* RwRasterShowRaster(RwRaster* raster, void* dev, RwUInt32 flags) {
    return plugin::CallAndReturn<RwRaster*, 0x7FB1A0, RwRaster*, void*, RwUInt32>(raster, dev, flags);
}

//! NOTE: This function is responsible for calculating the `stride` in the `raster`
RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode) {
    return plugin::CallAndReturn<RwUInt8*, 0x7FB2D0, RwRaster*, RwUInt8, RwInt32>(raster, level, lockMode);
}

RwRaster* RwRasterUnlock(RwRaster* raster) {
    return plugin::CallAndReturn<RwRaster*, 0x7FAEC0, RwRaster*>(raster);
}

RwUInt8* RwRasterLockPalette(RwRaster* raster, RwInt32 lockMode) {
    return plugin::CallAndReturn<RwUInt8*, 0x7FB0E0, RwRaster*, RwInt32>(raster, lockMode);
}

RwRaster* RwRasterUnlockPalette(RwRaster* raster) {
    return plugin::CallAndReturn<RwRaster*, 0x7FAFF0, RwRaster*>(raster);
}

RwInt32 RwRasterRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x7FB0B0, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RwRasterGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x7FAE40, RwUInt32>(pluginID);
}

RxRenderStateVector* RxRenderStateVectorSetDefaultRenderStateVector(RxRenderStateVector* rsvp) {
    return plugin::CallAndReturn<RxRenderStateVector*, 0x80EA40, RxRenderStateVector*>(rsvp);
}

RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth) {
    return plugin::CallAndReturn<RwImage*, 0x8026E0, RwInt32, RwInt32, RwInt32>(width, height, depth);
}

RwBool RwImageDestroy(RwImage* image) {
    return plugin::CallAndReturn<RwBool, 0x802740, RwImage*>(image);
}

RwImage* RwImageAllocatePixels(RwImage* image) {
    return plugin::CallAndReturn<RwImage*, 0x8027A0, RwImage*>(image);
}

RwImage* RwImageFreePixels(RwImage* image) {
    return plugin::CallAndReturn<RwImage*, 0x802860, RwImage*>(image);
}

RwImage* RwImageApplyMask(RwImage* image, const RwImage* mask) {
    return plugin::CallAndReturn<RwImage*, 0x802AF0, RwImage*, const RwImage*>(image, mask);
}

RwImage* RwImageMakeMask(RwImage* image) {
    return plugin::CallAndReturn<RwImage*, 0x802A20, RwImage*>(image);
}

RwImage* RwImageReadMaskedImage(const RwChar* imageName, const RwChar* maskname) {
    return plugin::CallAndReturn<RwImage*, 0x8035C0, const RwChar*, const RwChar*>(imageName, maskname);
}

RwImage* RwImageRead(const RwChar* imageName) {
    return plugin::CallAndReturn<RwImage*, 0x802FD0, const RwChar*>(imageName);
}

const RwChar* RwImageSetPath(const RwChar* path) {
    return plugin::CallAndReturn<const RwChar*, 0x802EA0, const RwChar*>(path);
}

RwBool RwImageSetGamma(RwReal gammaValue) {
    return plugin::CallAndReturn<RwBool, 0x803FE0, RwReal>(gammaValue);
}

RwImage* RwImageGammaCorrect(RwImage* image) {
    return plugin::CallAndReturn<RwImage*, 0x803E30, RwImage*>(image);
}

RwBool RwPalQuantInit(RwPalQuant* pq) {
    return plugin::CallAndReturn<RwBool, 0x80C470, RwPalQuant*>(pq);
}

void RwPalQuantTerm(RwPalQuant* pq) {
    plugin::Call<0x80C520, RwPalQuant*>(pq);
}

void RwPalQuantAddImage(RwPalQuant* pq, RwImage* img, RwReal weight) {
    plugin::Call<0x80AA80, RwPalQuant*, RwImage*, RwReal>(pq, img, weight);
}

RwInt32 RwPalQuantResolvePalette(RwRGBA* palette, RwInt32 maxcols, RwPalQuant* pq) {
    return plugin::CallAndReturn<RwInt32, 0x80AF60, RwRGBA*, RwInt32, RwPalQuant*>(palette, maxcols, pq);
}

void RwPalQuantMatchImage(RwUInt8* dstpixels, RwInt32 dststride, RwInt32 dstdepth, RwBool dstPacked, RwPalQuant* pq, RwImage* img) {
    plugin::Call<0x80BF20, RwUInt8*, RwInt32, RwInt32, RwBool, RwPalQuant*, RwImage*>(dstpixels, dststride, dstdepth, dstPacked, pq, img);
}

RwBool RwTextureSetMipmapping(RwBool enable) {
    return plugin::CallAndReturn<RwBool, 0x7F3530, RwBool>(enable);
}

RwBool RwTextureGetMipmapping() {
    return plugin::CallAndReturn<RwBool, 0x7F3550>();
}

RwBool RwTextureSetAutoMipmapping(RwBool enable) {
    return plugin::CallAndReturn<RwBool, 0x7F3560, RwBool>(enable);
}

RwBool RwTextureGetAutoMipmapping() {
    return plugin::CallAndReturn<RwBool, 0x7F3580>();
}

RwBool RwTextureSetMipmapGenerationCallBack(RwTextureCallBackMipmapGeneration callback) {
    return plugin::CallAndReturn<RwBool, 0x7F3C70, RwTextureCallBackMipmapGeneration>(callback);
}

RwTextureCallBackMipmapGeneration RwTextureGetMipmapGenerationCallBack() {
    return plugin::CallAndReturn<RwTextureCallBackMipmapGeneration, 0x7F3C90>();
}

RwBool RwTextureSetMipmapNameCallBack(RwTextureCallBackMipmapName callback) {
    return plugin::CallAndReturn<RwBool, 0x7F3CA0, RwTextureCallBackMipmapName>(callback);
}

RwTextureCallBackMipmapName RwTextureGetMipmapNameCallBack() {
    return plugin::CallAndReturn<RwTextureCallBackMipmapName, 0x7F3CC0>();
}

RwBool RwTextureGenerateMipmapName(RwChar* name, RwChar* maskName, RwUInt8 mipLevel, RwInt32 format) {
    return plugin::CallAndReturn<RwBool, 0x7F3AA0, RwChar*, RwChar*, RwUInt8, RwInt32>(name, maskName, mipLevel, format);
}

RwBool RwTextureRasterGenerateMipmaps(RwRaster* raster, RwImage* image) {
    return plugin::CallAndReturn<RwBool, 0x7F3CD0, RwRaster*, RwImage*>(raster, image);
}

RwBool _rwTextureSetAutoMipMapState(RwBool enable) {
    return plugin::CallAndReturn<RwBool, 0x7F3590, RwBool>(enable);
}

RwBool _rwTextureGetAutoMipMapState() {
    return plugin::CallAndReturn<RwBool, 0x7F35C0>();
}

RwTextureCallBackRead RwTextureGetReadCallBack() {
    return plugin::CallAndReturn<RwTextureCallBackRead, 0x7F3520>();
}

RwBool RwTextureSetReadCallBack(RwTextureCallBackRead callBack) {
    return plugin::CallAndReturn<RwBool, 0x7F3500, RwTextureCallBackRead>(callBack);
}

RwTextureCallBackFind RwTextureGetFindCallBack() {
    return plugin::CallAndReturn<RwTextureCallBackFind, 0x7F34F0>();
}

RwBool RwTextureSetFindCallBack(RwTextureCallBackFind callBack) {
    return plugin::CallAndReturn<RwBool, 0x7F34D0, RwTextureCallBackFind>(callBack);
}

RwTexture* RwTextureSetName(RwTexture* texture, const RwChar* name) {
    return plugin::CallAndReturn<RwTexture*, 0x7F38A0, RwTexture*, const RwChar*>(texture, name);
}

RwTexture* RwTextureSetMaskName(RwTexture* texture, const RwChar* maskName) {
    return plugin::CallAndReturn<RwTexture*, 0x7F3910, RwTexture*, const RwChar*>(texture, maskName);
}

void RwTexDictionarySetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x7F3E80, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwTexDictionary* RwTexDictionaryCreate() {
    return plugin::CallAndReturn<RwTexDictionary*, 0x7F3600>();
}

RwBool RwTexDictionaryDestroy(RwTexDictionary* dict) {
    return plugin::CallAndReturn<RwBool, 0x7F36A0, RwTexDictionary*>(dict);
}

RwTexture* RwTextureCreate(RwRaster* raster) {
    return plugin::CallAndReturn<RwTexture*, 0x7F37C0, RwRaster*>(raster);
}

RwTexture* RwTextureRead(const char* name, const char* maskName) {
    return plugin::CallAndReturn<RwTexture*, 0x7F3AC0, const char*, const char*>(name, maskName);
}

RwBool RwTextureDestroy(RwTexture* texture) {
    return plugin::CallAndReturn<RwBool, 0x7F3820, RwTexture*>(texture);
}

RwTexture* RwTextureSetRaster(RwTexture* texture, RwRaster* raster) {
    return plugin::CallAndReturn<RwTexture*, 0x7F35D0, RwTexture*, RwRaster*>(texture, raster);
}

RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dict, RwTexture* texture) {
    return plugin::CallAndReturn<RwTexture*, 0x7F3980, RwTexDictionary*, RwTexture*>(dict, texture);
}

RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture) {
    return plugin::CallAndReturn<RwTexture*, 0x7F39C0, RwTexture*>(texture);
}

RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* dict, const RwChar* name) {
    return plugin::CallAndReturn<RwTexture*, 0x7F39F0, RwTexDictionary*, const RwChar*>(dict, name);
}

RwTexDictionary* RwTexDictionaryGetCurrent() {
    return plugin::CallAndReturn<RwTexDictionary*, 0x7F3A90>();
}

RwTexDictionary* RwTexDictionarySetCurrent(RwTexDictionary* dict) {
    return plugin::CallAndReturn<RwTexDictionary*, 0x7F3A70, RwTexDictionary*>(dict);
}

const RwTexDictionary* RwTexDictionaryForAllTextures(const RwTexDictionary* dict, RwTextureCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<const RwTexDictionary*, 0x7F3730, const RwTexDictionary*, RwTextureCallBack, void*>(dict, fpCallBack, data);
}

RwBool RwTexDictionaryForAllTexDictionaries(RwTexDictionaryCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<RwBool, 0x7F3770, RwTexDictionaryCallBack, void*>(fpCallBack, data);
}

RwInt32 RwTextureRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x7F3BB0, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RwTexDictionaryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x7F3C10, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RwTextureGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x7F3BE0, RwUInt32>(pluginID);
}

RwInt32 RwTexDictionaryGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x7F3C40, RwUInt32>(pluginID);
}

RwBool RwTextureValidatePlugins(const RwTexture* texture) {
    return plugin::CallAndReturn<RwBool, 0x7F3C00, const RwTexture*>(texture);
}

RwBool RwTexDictionaryValidatePlugins(const RwTexDictionary* dict) {
    return plugin::CallAndReturn<RwBool, 0x7F3C60, const RwTexDictionary*>(dict);
}

rwIm3DPool* _rwIm3DGetPool() {
    return plugin::CallAndReturn<rwIm3DPool*, 0x7EFDD0>();
}

void* RwIm3DTransform(RwIm3DVertex* pVerts, RwUInt32 numVerts, RwMatrix* ltm, RwUInt32 flags) {
    return plugin::CallAndReturn<void*, 0x7EF450, RwIm3DVertex*, RwUInt32, RwMatrix*, RwUInt32>(pVerts, numVerts, ltm, flags);
}

RwBool RwIm3DEnd() {
    return plugin::CallAndReturn<RwBool, 0x7EF520>();
}

RwBool RwIm3DRenderLine(RwInt32 vert1, RwInt32 vert2) {
    return plugin::CallAndReturn<RwBool, 0x7EF900, RwInt32, RwInt32>(vert1, vert2);
}

RwBool RwIm3DRenderTriangle(RwInt32 vert1, RwInt32 vert2, RwInt32 vert3) {
    return plugin::CallAndReturn<RwBool, 0x7EF810, RwInt32, RwInt32, RwInt32>(vert1, vert2, vert3);
}

RwBool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primType, RwImVertexIndex* indices, RwInt32 numIndices) {
    return plugin::CallAndReturn<RwBool, 0x7EF550, RwPrimitiveType, RwImVertexIndex*, RwInt32>(primType, indices, numIndices);
}

RwBool RwIm3DRenderPrimitive(RwPrimitiveType primType) {
    return plugin::CallAndReturn<RwBool, 0x7EF6B0, RwPrimitiveType>(primType);
}

RxPipeline* RwIm3DGetTransformPipeline() {
    return plugin::CallAndReturn<RxPipeline*, 0x7EF9D0>();
}

RxPipeline* RwIm3DGetRenderPipeline(RwPrimitiveType primType) {
    return plugin::CallAndReturn<RxPipeline*, 0x7EF9E0, RwPrimitiveType>(primType);
}

RxPipeline* RwIm3DSetTransformPipeline(RxPipeline* pipeline) {
    return plugin::CallAndReturn<RxPipeline*, 0x7EFAC0, RxPipeline*>(pipeline);
}

RxPipeline* RwIm3DSetRenderPipeline(RxPipeline* pipeline, RwPrimitiveType primType) {
    return plugin::CallAndReturn<RxPipeline*, 0x7EFB10, RxPipeline*, RwPrimitiveType>(pipeline, primType);
}

RwRaster* RwD3D9RasterStreamReadDDS(RwStream* stream) {
    return plugin::CallAndReturn<RwRaster*, 0x81F360, RwStream*>(stream);
}

RwTexture* RwD3D9DDSTextureRead(const RwChar* name, const RwChar* maskname) {
    return plugin::CallAndReturn<RwTexture*, 0x820A10, const RwChar*, const RwChar*>(name, maskname);
}

RwBool RwD3D9RasterIsCompressed(const RwRaster* raster) {
    return plugin::CallAndReturn<RwBool, 0x820C90, const RwRaster*>(raster);
}

RwBool RwD3D9DeviceSupportsDXTTexture() {
    return plugin::CallAndReturn<RwBool, 0x7F9C30>();
}

void* RwD3D9GetCurrentD3DDevice() { // IDirect3DDevice9
    return plugin::CallAndReturn<void*, 0x7F9D50>();
}

RwUInt32 RwD3D9EngineGetMaxMultiSamplingLevels() {
    return plugin::CallAndReturn<RwUInt32, 0x7F84E0>();
}

void RwD3D9EngineSetMultiSamplingLevels(RwUInt32 numLevels) {
    plugin::Call<0x7F84F0, RwUInt32>(numLevels);
}

void RwD3D9EngineSetRefreshRate(RwUInt32 refreshRate) {
    plugin::Call<0x7F8580, RwUInt32>(refreshRate);
}

void RwD3D9EngineSetMultiThreadSafe(RwBool enable) {
    plugin::Call<0x7F8620, RwBool>(enable);
}

void RwD3D9EngineSetSoftwareVertexProcessing(RwBool enable) {
    plugin::Call<0x7F8630, RwBool>(enable);
}

void* RwD3D9GetCurrentD3DRenderTarget(RwUInt32 index) {
    return plugin::CallAndReturn<void*, 0x7F9E80, RwUInt32>(index);
}

RwBool RwD3D9SetRenderTarget(RwUInt32 index, RwRaster* raster) {
    return plugin::CallAndReturn<RwBool, 0x7F9E90, RwUInt32, RwRaster*>(index, raster);
}

RwBool RwD3D9ChangeVideoMode(RwInt32 modeIndex) {
    return plugin::CallAndReturn<RwBool, 0x7F8640, RwInt32>(modeIndex);
}

RwBool RwD3D9ChangeMultiSamplingLevels(RwUInt32 numLevels) {
    return plugin::CallAndReturn<RwBool, 0x7F8A90, RwUInt32>(numLevels);
}

RwBool RwD3D9CameraAttachWindow(void* camera, void* hwnd) {
    return plugin::CallAndReturn<RwBool, 0x7F8D70, void*, void*>(camera, hwnd);
}

void RwD3D9SetStreamSource(RwUInt32 streamNumber, void* streamData, RwUInt32 offset, RwUInt32 stride) {
    plugin::Call<0x7FA030, RwUInt32, void*, RwUInt32, RwUInt32>(streamNumber, streamData, offset, stride);
}

void _rwD3D9RenderStateFlushCache() {
    plugin::Call<0x7FC200>();
}

void _rwD3D9DrawIndexedPrimitiveUP(RwUInt32 primitiveType, RwUInt32 minIndex, RwUInt32 numVertices, RwUInt32 primitiveCount, const void* indexData, const void* vertexStreamZeroData, RwUInt32 VertexStreamZeroStride) {
   plugin::Call<0x7FA1F0, RwUInt32, RwUInt32, RwUInt32, RwUInt32, const void*, const void*, RwUInt32>(primitiveType, minIndex, numVertices, primitiveCount, indexData, vertexStreamZeroData, VertexStreamZeroStride);
}

void _rwD3D9DrawPrimitiveUP(RwUInt32 primitiveType, RwUInt32 primitiveCount, const void* vertexStreamZeroData, RwUInt32 VertexStreamZeroStride) {
   plugin::Call<0x7FA290, RwUInt32, RwUInt32, const void*, RwUInt32>(primitiveType, primitiveCount, vertexStreamZeroData, VertexStreamZeroStride);
}

void _rwD3D9DrawIndexedPrimitive(RwUInt32 primitiveType, RwInt32 baseVertexIndex, RwUInt32 minIndex, RwUInt32 numVertices, RwUInt32 startIndex, RwUInt32 primitiveCount) {
   plugin::Call<0x7FA320, RwUInt32, RwInt32, RwUInt32, RwUInt32, RwUInt32, RwUInt32>(primitiveType, baseVertexIndex, minIndex, numVertices, startIndex, primitiveCount);
}

void _rwD3D9SetVertexShaderConstant(RwUInt32 registerAddress, const void* constantData, RwUInt32 constantCount) {
   plugin::Call<0x7FACA0, RwUInt32, const void*, RwUInt32>(registerAddress, constantData, constantCount);
}

void _rwD3D9SetPixelShaderConstant(RwUInt32 registerAddress, const void* constantData, RwUInt32 constantCount) {
   plugin::Call<0x7FAD00, RwUInt32, const void*, RwUInt32>(registerAddress, constantData, constantCount);
}

void _rwD3D9SetFVF(RwUInt32 fvf) {
   plugin::Call<0x7F9F30, RwUInt32>(fvf);
}

void _rwD3D9SetVertexShader(void *shader) {
   plugin::Call<0x7F9FB0, void *>(shader);
}

void _rwD3D9SetPixelShader(void *shader) {
   plugin::Call<0x7F9FF0, void *>(shader);
}

void RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value) {
   plugin::Call<0x7FC2D0, RwUInt32, RwUInt32>(state, value);
}

void RwD3D9GetRenderState(RwUInt32 state, void* value) {
   plugin::Call<0x7FC320, RwUInt32, void*>(state, value);
}

void RwD3D9SetTextureStageState(RwUInt32 stage, RwUInt32 type, RwUInt32 value) {
   plugin::Call<0x7FC340, RwUInt32, RwUInt32, RwUInt32>(stage, type, value);
}

void RwD3D9GetTextureStageState(RwUInt32 stage, RwUInt32 type, void* value) {
   plugin::Call<0x7FC3A0, RwUInt32, RwUInt32, void*>(stage, type, value);
}

void RwD3D9SetSamplerState(RwUInt32 stage, RwUInt32 type, RwUInt32 value) {
   plugin::Call<0x7FC3C0, RwUInt32, RwUInt32, RwUInt32>(stage, type, value);
}

void RwD3D9GetSamplerState(RwUInt32 stage, RwUInt32 type, void* value) {
   plugin::Call<0x7FC400, RwUInt32, RwUInt32, void*>(stage, type, value);
}

void RwD3D9SetStencilClear(RwUInt32 stencilClear) {
   plugin::Call<0x7F9D30, RwUInt32>(stencilClear);
}

RwUInt32 RwD3D9GetStencilClear() {
   return plugin::CallAndReturn<RwUInt32, 0x7F9D40>();
}

RwBool RwD3D9SetTexture(RwTexture* texture, RwUInt32 stage) {
   return plugin::CallAndReturn<RwBool, 0x7FDE70, RwTexture*, RwUInt32>(texture, stage);
}

RwBool RwD3D9SetTransform(RwUInt32 state, const void* matrix) {
   return plugin::CallAndReturn<RwBool, 0x7FA390, RwUInt32, const void*>(state, matrix);
}

void RwD3D9GetTransform(RwUInt32 state, void* matrix) {
   plugin::Call<0x7FA4F0, RwUInt32, void*>(state, matrix);
}

RwBool RwD3D9SetMaterial(const void* material) {
   return plugin::CallAndReturn<RwBool, 0x7FC430, const void*>(material);
}

RwBool RwD3D9SetClipPlane(RwUInt32 index, const RwV4d* plane) {
   return plugin::CallAndReturn<RwBool, 0x7FC4A0, RwUInt32, const RwV4d*>(index, plane);
}

RwBool RwD3D9SetTransformWorld(const RwMatrix* matrix) {
   return plugin::CallAndReturn<RwBool, 0x7FA520, const RwMatrix*>(matrix);
}

RwBool RwD3D9SetSurfaceProperties(const RwSurfaceProperties* surfaceProps, const RwRGBA* color, RwUInt32 flags) {
   return plugin::CallAndReturn<RwBool, 0x7FC4D0, const RwSurfaceProperties*, const RwRGBA*, RwUInt32>(surfaceProps, color, flags);
}

RwBool RwD3D9SetLight(RwInt32 index, const void* light) {
   return plugin::CallAndReturn<RwBool, 0x7FA660, RwInt32, const void*>(index, light);
}

void RwD3D9GetLight(RwInt32 index, void* light) {
   plugin::Call<0x7FA820, RwInt32, void*>(index, light);
}

RwBool RwD3D9EnableLight(RwInt32 index, RwBool enable) {
   return plugin::CallAndReturn<RwBool, 0x7FA860, RwInt32, RwBool>(index, enable);
}

RwBool RwD3D9IndexBufferCreate(RwUInt32 numIndices, void* indexBuffer) {
   return plugin::CallAndReturn<RwBool, 0x4C9970, RwUInt32, void*>(numIndices, indexBuffer);
}

RwBool RwD3D9CreateVertexDeclaration(const void* elements, void* vertexdeclaration) {
   return plugin::CallAndReturn<RwBool, 0x7FAA30, const void*, void*>(elements, vertexdeclaration);
}

void RwD3D9DeleteVertexDeclaration() {
   plugin::Call<0x7FAC10>();
}

void RwD3D9DeleteVertexShader() {
   plugin::Call<0x7FAC90>();
}

RwBool RwD3D9CreatePixelShader(const RwUInt32* function, void* shader) {
   return plugin::CallAndReturn<RwBool, 0x7FACC0, const RwUInt32*, void*>(function, shader);
}

void RwD3D9DeletePixelShader() {
   plugin::Call<0x7FACF0>();
}

void _rwD3D9SetStreams(const RxD3D9VertexStream* streams, RwBool useOffsets) {
    plugin::CallAndReturn<void, 0x7FA090>(streams, useOffsets);
}

const void* RwD3D9GetCaps() { // D3DCAPS9*
   return plugin::CallAndReturn<const void*, 0x7FAD20>();
}

RwBool RwD3D9CameraIsSphereFullyInsideFrustum(const void* camera, const void* sphere) {
   return plugin::CallAndReturn<RwBool, 0x7FAD30, const void*, const void*>(camera, sphere);
}

RwBool RwD3D9CameraIsBBoxFullyInsideFrustum(const void* camera, const void* boundingBox) {
   return plugin::CallAndReturn<RwBool, 0x7FAD90, const void*, const void*>(camera, boundingBox);
}

void _rwD3D9RasterConvertToNonPalettized(RwRaster* raster) {
   plugin::Call<0x4CD250, RwRaster*>(raster);
}

RwBool _rwDeviceRegisterPlugin() {
   return plugin::CallAndReturn<RwBool, 0x7F5F60>();
}

void _rwD3D9DeviceSetRestoreCallback(rwD3D9DeviceRestoreCallBack callback) {
   plugin::Call<0x7FAE20, rwD3D9DeviceRestoreCallBack>(callback);
}

rwD3D9DeviceRestoreCallBack _rwD3D9DeviceGetRestoreCallback() {
   return plugin::CallAndReturn<rwD3D9DeviceRestoreCallBack, 0x7FAE30>();
}

RwImage* RwImageResample(RwImage* dstImage, const RwImage* srcImage) {
    return plugin::CallAndReturn<RwImage*, 0x80C600, RwImage*, const RwImage*>(dstImage, srcImage);
}

RwImage* RwImageCreateResample(const RwImage* srcImage, RwInt32 width, RwInt32 height) {
    return plugin::CallAndReturn<RwImage*, 0x80CD10, const RwImage*, RwInt32, RwInt32>(srcImage, width, height);
}

RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster) {
    return plugin::CallAndReturn<RwImage*, 0x804250, RwImage*, RwRaster*>(image, raster);
}

RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image) {
    return plugin::CallAndReturn<RwRaster*, 0x804290, RwRaster*, RwImage*>(raster, image);
}

RwRaster* RwRasterRead(const RwChar* filename) {
    return plugin::CallAndReturn<RwRaster*, 0x8043F0, const RwChar*>(filename);
}

RwRaster* RwRasterReadMaskedRaster(const RwChar* filename, const RwChar* maskname) {
    return plugin::CallAndReturn<RwRaster*, 0x8044E0, const RwChar*, const RwChar*>(filename, maskname);
}

RwImage* RwImageFindRasterFormat(RwImage* ipImage, RwInt32 nRasterType, RwInt32* npWidth, RwInt32* npHeight, RwInt32* npDepth, RwInt32* npFormat) {
    return plugin::CallAndReturn<RwImage*, 0x8042C0, RwImage*, RwInt32, RwInt32*, RwInt32*, RwInt32*, RwInt32*>(ipImage, nRasterType, npWidth, npHeight, npDepth, npFormat);
}

RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callBack, void* data) {
    return plugin::CallAndReturn<RwFrame*, 0x7F1200, RwFrame*, RwObjectCallBack, void*>(frame, callBack, data);
}

RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* v, RwOpCombineType combine) {
    return plugin::CallAndReturn<RwFrame*, 0x7F0E30, RwFrame*, const RwV3d*, RwOpCombineType>(frame, v, combine);
}

RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, RwReal angle, RwOpCombineType combine) {
    return plugin::CallAndReturn<RwFrame*, 0x7F1010, RwFrame*, const RwV3d*, RwReal, RwOpCombineType>(frame, axis, angle, combine);
}

RwFrame* RwFrameScale(RwFrame* frame, const RwV3d* v, RwOpCombineType combine) {
    return plugin::CallAndReturn<RwFrame*, 0x7F0ED0, RwFrame*, const RwV3d*, RwOpCombineType>(frame, v, combine);
}

RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* m, RwOpCombineType combine) {
    return plugin::CallAndReturn<RwFrame*, 0x7F0F70, RwFrame*, const RwMatrix*, RwOpCombineType>(frame, m, combine);
}

RwFrame* RwFrameOrthoNormalize(RwFrame* frame) {
    return plugin::CallAndReturn<RwFrame*, 0x7F1170, RwFrame*>(frame);
}

RwFrame* RwFrameSetIdentity(RwFrame* frame) {
    return plugin::CallAndReturn<RwFrame*, 0x7F10B0, RwFrame*>(frame);
}

RwFrame* RwFrameCloneHierarchy(RwFrame* root) {
    return plugin::CallAndReturn<RwFrame*, 0x7F0250, RwFrame*>(root);
}

RwBool RwFrameDestroyHierarchy(RwFrame* frame) {
    return plugin::CallAndReturn<RwBool, 0x7F08A0, RwFrame*>(frame);
}

RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callBack, void* data) {
    return plugin::CallAndReturn<RwFrame*, 0x7F0DC0, RwFrame*, RwFrameCallBack, void*>(frame, callBack, data);
}

RwFrame* RwFrameRemoveChild(RwFrame* child) {
    return plugin::CallAndReturn<RwFrame*, 0x7F0CD0, RwFrame*>(child);
}

RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child) {
    return plugin::CallAndReturn<RwFrame*, 0x7F0B00, RwFrame*, RwFrame*>(parent, child);
}

RwFrame* RwFrameAddChildNoUpdate(RwFrame* parent, RwFrame* child) {
    return plugin::CallAndReturn<RwFrame*, 0x7F09C0, RwFrame*, RwFrame*>(parent, child);
}

RwFrame* RwFrameGetRoot(const RwFrame* frame) {
    return plugin::CallAndReturn<RwFrame*, 0x7F09B0, const RwFrame*>(frame);
}

RwMatrix* RwFrameGetLTM(RwFrame* frame) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F0990, RwFrame*>(frame);
}

RwFrame* RwFrameUpdateObjects(RwFrame* frame) {
    return plugin::CallAndReturn<RwFrame*, 0x7F0910, RwFrame*>(frame);
}

void RwFrameSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x7EFED0, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwFrame* RwFrameCreate() {
    return plugin::CallAndReturn<RwFrame*, 0x7F0410>();
}

RwBool RwFrameDestroy(RwFrame* frame) {
    assert(frame);
    return plugin::CallAndReturn<RwBool, 0x7F05A0, RwFrame*>(frame);
}

RwBool _rwFrameInit(RwFrame* frame) {
    return plugin::CallAndReturn<RwBool, 0x7F0450, RwFrame*>(frame);
}

RwBool _rwFrameDeInit(RwFrame* frame) {
    return plugin::CallAndReturn<RwBool, 0x7F06F0, RwFrame*>(frame);
}

RwBool RwFrameDirty(const RwFrame* frame) {
    return plugin::CallAndReturn<RwBool, 0x7F0340, const RwFrame*>(frame);
}

RwInt32 RwFrameCount(RwFrame* frame) {
    return plugin::CallAndReturn<RwInt32, 0x7F0E00, RwFrame*>(frame);
}

RwInt32 RwFrameRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x7F1260, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RwFrameGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x7F1290, RwUInt32>(pluginID);
}

RwBool RwFrameValidatePlugins(const RwFrame* frame) {
    return plugin::CallAndReturn<RwBool, 0x7F12B0, const RwFrame*>(frame);
}

RwFrame* _rwFrameCloneAndLinkClones(RwFrame* root) {
    return plugin::CallAndReturn<RwFrame*, 0x7EFFB0, RwFrame*>(root);
}

RwFrame* _rwFramePurgeClone(RwFrame* root) {
    return plugin::CallAndReturn<RwFrame*, 0x7F01A0, RwFrame*>(root);
}

void _rwObjectHasFrameReleaseFrame() {
    plugin::Call<0x804F40>();
}

RwBool _rwFrameSyncDirty() {
    return plugin::CallAndReturn<RwBool, 0x809550>();
}

void _rwFrameSyncHierarchyLTM(RwFrame* frame) {
    plugin::Call<0x8097D0, RwFrame*>(frame);
}

RwInt32 RwTextureRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x804550, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RwTextureSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x804580, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwUInt32 RwTextureStreamGetSize(const RwTexture* texture) {
    return plugin::CallAndReturn<RwUInt32, 0x8045A0, const RwTexture*>(texture);
}

RwTexture* RwTextureStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RwTexture*, 0x8046E0, RwStream*>(stream);
}

const RwTexture* RwTextureStreamWrite(const RwTexture* texture, RwStream* stream) {
    return plugin::CallAndReturn<const RwTexture*, 0x8045E0, const RwTexture*, RwStream*>(texture, stream);
}

RwInt32 RwTexDictionaryRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x8048E0, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RwTexDictionarySetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x804910, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwUInt32 RwTexDictionaryStreamGetSize(const RwTexDictionary* texDict) {
    return plugin::CallAndReturn<RwUInt32, 0x804930, const RwTexDictionary*>(texDict);
}

RwTexDictionary* RwTexDictionaryStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RwTexDictionary*, 0x804C30, RwStream*>(stream);
}

const RwTexDictionary* RwTexDictionaryStreamWrite(const RwTexDictionary* texDict, RwStream* stream) {
    return plugin::CallAndReturn<const RwTexDictionary*, 0x8049F0, const RwTexDictionary*, RwStream*>(texDict, stream);
}

RwTextureChunkInfo* _rwTextureChunkInfoRead(RwStream* stream, RwTextureChunkInfo* textureChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RwTextureChunkInfo*, 0x804E60, RwStream*, RwTextureChunkInfo*, RwInt32*>(stream, textureChunkInfo, bytesRead);
}

void RwFrameListSetAutoUpdate(RwBool flag) {
    plugin::Call<0x807570, RwBool>(flag);
}

RwInt32 RwFrameRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x807580, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RwFrameSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x8075B0, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

rwFrameList* _rwFrameListInitialize(rwFrameList* frameList, RwFrame* frame) {
    return plugin::CallAndReturn<rwFrameList*, 0x8075D0, rwFrameList*, RwFrame*>(frameList, frame);
}

RwBool _rwFrameListFindFrame(const rwFrameList* frameList, const RwFrame* frame, RwInt32* npIndex) {
    return plugin::CallAndReturn<RwBool, 0x8076E0, const rwFrameList*, const RwFrame*, RwInt32*>(frameList, frame, npIndex);
}

rwFrameList* _rwFrameListDeinitialize(rwFrameList* frameList) {
    return plugin::CallAndReturn<rwFrameList*, 0x807720, rwFrameList*>(frameList);
}

RwUInt32 _rwFrameListStreamGetSize(const rwFrameList* frameList) {
    return plugin::CallAndReturn<RwUInt32, 0x807750, const rwFrameList*>(frameList);
}

rwFrameList* _rwFrameListStreamRead(RwStream* stream, rwFrameList* fl) {
    return plugin::CallAndReturn<rwFrameList*, 0x807970, RwStream*, rwFrameList*>(stream, fl);
}

const rwFrameList* _rwFrameListStreamWrite(const rwFrameList* frameList, RwStream* stream) {
    return plugin::CallAndReturn<const rwFrameList*, 0x8077A0, const rwFrameList*, RwStream*>(frameList, stream);
}

RwBBox* RwBBoxCalculate(RwBBox* boundBox, const RwV3d* verts, RwInt32 numVerts) {
    return plugin::CallAndReturn<RwBBox*, 0x808F60, RwBBox*, const RwV3d*, RwInt32>(boundBox, verts, numVerts);
}

RwBBox* RwBBoxInitialize(RwBBox* boundBox, const RwV3d* vertex) {
    return plugin::CallAndReturn<RwBBox*, 0x809020, RwBBox*, const RwV3d*>(boundBox, vertex);
}

RwBBox* RwBBoxAddPoint(RwBBox* boundBox, const RwV3d* vertex) {
    return plugin::CallAndReturn<RwBBox*, 0x809060, RwBBox*, const RwV3d*>(boundBox, vertex);
}

RwBool RwBBoxContainsPoint(const RwBBox* boundBox, const RwV3d* vertex) {
    return plugin::CallAndReturn<RwBool, 0x8090E0, const RwBBox*, const RwV3d*>(boundBox, vertex);
}

RwCamera* RwCameraBeginUpdate(RwCamera* camera) {
    ZoneScoped;
    return camera->beginUpdate(camera);
}

RwCamera* RwCameraEndUpdate(RwCamera* camera) {
    ZoneScoped;
    return camera->endUpdate(camera);
}

RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, RwInt32 clearMode) {
    ZoneScoped;
    return RwEngineInstance->stdFunc[rwSTANDARDCAMERACLEAR](camera, colour, clearMode) ? camera : nullptr;
}

RwCamera* RwCameraShowRaster(RwCamera* camera, void* pDev, RwUInt32 flags) {
    ZoneScoped;
    return RwRasterShowRaster(RwCameraGetRaster(camera), pDev, flags) != 0 ? camera : nullptr;
}

void RwCameraSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x7EE0F0, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwBool RwCameraDestroy(RwCamera* camera) {
    return plugin::CallAndReturn<RwBool, 0x7EE4B0, RwCamera*>(camera);
}

RwCamera* RwCameraCreate() {
    return plugin::CallAndReturn<RwCamera*, 0x7EE4F0>();
}

RwCamera* RwCameraClone(RwCamera* camera) {
    return plugin::CallAndReturn<RwCamera*, 0x7EF3B0, RwCamera*>(camera);
}

RwCamera* RwCameraSetViewOffset(RwCamera* camera, const RwV2d* offset) {
    return plugin::CallAndReturn<RwCamera*, 0x7EE1A0, RwCamera*, const RwV2d*>(camera, offset);
}

RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow) {
    return plugin::CallAndReturn<RwCamera*, 0x7EE410, RwCamera*, const RwV2d*>(camera, viewWindow);
}

RwCamera* RwCameraSetProjection(RwCamera* camera, RwCameraProjection projection) {
    return plugin::CallAndReturn<RwCamera*, 0x7EE3A0, RwCamera*, RwCameraProjection>(camera, projection);
}

RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, RwReal nearClip) {
    return plugin::CallAndReturn<RwCamera*, 0x7EE1D0, RwCamera*, RwReal>(camera, nearClip);
}

RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, RwReal farClip) {
    return plugin::CallAndReturn<RwCamera*, 0x7EE2A0, RwCamera*, RwReal>(camera, farClip);
}

RwInt32 RwCameraRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x7EE450, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RwCameraGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x7EE480, RwUInt32>(pluginID);
}

RwBool RwCameraValidatePlugins(const RwCamera* camera) {
    return plugin::CallAndReturn<RwBool, 0x7EE4A0, const RwCamera*>(camera);
}

RwFrustumTestResult RwCameraFrustumTestSphere(const RwCamera* camera, const RwSphere* sphere) {
    return plugin::CallAndReturn<RwFrustumTestResult, 0x7EE2D0, const RwCamera*, const RwSphere*>(camera, sphere);
}

RwInt32 RwCameraRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x808C90, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RwCameraSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x808CC0, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwUInt32 RwCameraStreamGetSize(const RwCamera* camera) {
    return plugin::CallAndReturn<RwUInt32, 0x808CE0, const RwCamera*>(camera);
}

RwCamera* RwCameraStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RwCamera*, 0x808DE0, RwStream*>(stream);
}

const RwCamera* RwCameraStreamWrite(const RwCamera* camera, RwStream* stream) {
    return plugin::CallAndReturn<const RwCamera*, 0x808D00, const RwCamera*, RwStream*>(camera, stream);
}

RwCameraChunkInfo* RwCameraChunkInfoRead(RwStream* stream, RwCameraChunkInfo* cameraChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RwCameraChunkInfo*, 0x808EF0, RwStream*, RwCameraChunkInfo*, RwInt32*>(stream, cameraChunkInfo, bytesRead);
}

void _rwObjectHasFrameSetFrame(void *object, RwFrame *frame) {
    plugin::Call<0x804EF0, void*, RwFrame*>(object, frame);
}

void _rwObjectHasFrameReleaseFrame(void *object) {
    plugin::Call<0x804F40, void*>(object);
}

RwUInt32 RwRGBAToPixel(RwRGBA* rgbIn, RwInt32 rasterFormat) {
    return plugin::CallAndReturn<RwUInt32, 0x803740, RwRGBA*, RwInt32>(rgbIn, rasterFormat);
}
