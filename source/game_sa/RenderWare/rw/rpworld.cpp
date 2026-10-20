#include "StdInc.h"

#include "rpworld.h"
#include <rpdbgerr.h>

void _rwD3D9VSSetActiveWorldMatrix(const RwMatrix* worldMatrix) {
    plugin::Call<0x764650, const RwMatrix*>(worldMatrix);
}

void _rwD3D9VSGetComposedTransformMatrix() {
    plugin::Call<0x7646E0>();
}

void _rwD3D9VSGetWorldViewTransposedMatrix() {
    plugin::Call<0x764730>();
}

void _rwD3D9VSGetWorldViewMatrix() {
    plugin::Call<0x764760>();
}

void _rwD3D9VSGetInverseWorldMatrix() {
    plugin::Call<0x7647B0>();
}

void _rwD3D9VSGetWorldMultiplyMatrix(RwMatrix* worldMultiplyMatrix, const RwMatrix* matrix) {
    plugin::Call<0x764920, RwMatrix*, const RwMatrix*>(worldMultiplyMatrix, matrix);
}

void _rwD3D9VSGetWorldMultiplyTransposeMatrix(void* worldMultiplyMatrix, const RwMatrix* matrix) {
    plugin::Call<0x764960, void*, const RwMatrix*>(worldMultiplyMatrix, matrix);
}

void _rwD3D9VSGetWorldViewMultiplyTransposeMatrix(void* worldViewMultiplyMatrix, const RwMatrix* matrix) {
    plugin::Call<0x7649D0, void*, const RwMatrix*>(worldViewMultiplyMatrix, matrix);
}

void _rwD3D9VSGetWorldNormalizedMultiplyTransposeMatrix(void* worldMultiplyMatrix, const RwMatrix* matrix) {
    plugin::Call<0x764A70, void*, const RwMatrix*>(worldMultiplyMatrix, matrix);
}

void _rwD3D9VSGetWorldNormalizedViewMultiplyTransposeMatrix(void* worldViewMultiplyMatrix, const RwMatrix* matrix) {
    plugin::Call<0x764B50, void*, const RwMatrix*>(worldViewMultiplyMatrix, matrix);
}

void _rwD3D9VSGetWorldNormalizedTransposeMatrix() {
    plugin::Call<0x764C60>();
}

void _rwD3D9VSGetProjectionTransposedMatrix() {
    plugin::Call<0x764D20>();
}

void _rwD3D9VSGetNormalInLocalSpace(const RwV3d* normalWorldSpace, RwV3d* normalLocalSpace) {
    plugin::Call<0x764D30, const RwV3d*, RwV3d*>(normalWorldSpace, normalLocalSpace);
}

void _rwD3D9VSGetPointInLocalSpace(const RwV3d* pointWorldSpace, RwV3d* pointLocalSpace) {
    plugin::Call<0x764E70, const RwV3d*, RwV3d*>(pointWorldSpace, pointLocalSpace);
}

void _rwD3D9VSGetRadiusInLocalSpace(RwReal radiusWorldSpace, RwReal* radiusLocalSpace) {
    plugin::Call<0x764F60, RwReal, RwReal*>(radiusWorldSpace, radiusLocalSpace);
}

void _rpD3D9VertexShaderCachePurge() {
    plugin::Call<0x75EE60>();
}

void* _rpD3D9GetVertexShader(const _rpD3D9VertexShaderDescriptor* desc, _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    return plugin::CallAndReturn<void*, 0x75EED0, const _rpD3D9VertexShaderDescriptor*, _rpD3D9VertexShaderDispatchDescriptor*>(desc, dispatch);
}

RwUInt32 _rpD3D9GetNumConstantsUsed(const _rpD3D9VertexShaderDescriptor* desc) {
    return plugin::CallAndReturn<RwUInt32, 0x75EDD0, const _rpD3D9VertexShaderDescriptor*>(desc);
}

void _rxD3D9VertexShaderDefaultBeginCallBack(void* object, RwUInt32 type, _rpD3D9VertexShaderDescriptor* desc) {
    plugin::Call<0x760DF0, void*, RwUInt32, _rpD3D9VertexShaderDescriptor*>(object, type, desc);
}

RwV4d* _rxD3D9VertexShaderDefaultLightingCallBack(void* object, RwUInt32 type, RwV4d* shaderConstantPtr, _rpD3D9VertexShaderDescriptor* desc) {
    return plugin::CallAndReturn<RwV4d*, 0x761170, void*, RwUInt32, RwV4d*, _rpD3D9VertexShaderDescriptor*>(object, type, shaderConstantPtr, desc);
}

void* _rxD3D9VertexShaderDefaultGetMaterialShaderCallBack(const RpMaterial* material, _rpD3D9VertexShaderDescriptor* desc, _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    return plugin::CallAndReturn<void*, 0x761010, const RpMaterial*, _rpD3D9VertexShaderDescriptor*, _rpD3D9VertexShaderDispatchDescriptor*>(material, desc, dispatch);
}

void _rxD3D9VertexShaderDefaultMeshRenderCallBack(RxD3D9ResEntryHeader* resEntryHeader, RxD3D9InstanceData* instancedMesh, const _rpD3D9VertexShaderDescriptor* desc, const _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    plugin::Call<0x761030, RxD3D9ResEntryHeader*, RxD3D9InstanceData*, const _rpD3D9VertexShaderDescriptor*, const _rpD3D9VertexShaderDispatchDescriptor*>(resEntryHeader, instancedMesh, desc, dispatch);
}

void _rxD3D9VertexShaderDefaultEndCallBack(void* object, RwUInt32 type, _rpD3D9VertexShaderDescriptor* desc) {
    plugin::Call<0x761000, void*, RwUInt32, _rpD3D9VertexShaderDescriptor*>(object, type, desc);
}

void _rpD3D9VertexShaderUpdateLightsColors(RwV4d* shaderConstantPtr, const _rpD3D9VertexShaderDescriptor* desc, RwReal ambientCoef, RwReal diffuseCoef) {
    plugin::Call<0x761720, RwV4d*, const _rpD3D9VertexShaderDescriptor*, RwReal, RwReal>(shaderConstantPtr, desc, ambientCoef, diffuseCoef);
}

void _rpD3D9VertexShaderUpdateMaterialColor(const RwRGBA* color, const _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    plugin::Call<0x761820, const RwRGBA*, const _rpD3D9VertexShaderDispatchDescriptor*>(color, dispatch);
}

void _rpD3D9VertexShaderUpdateFogData(const _rpD3D9VertexShaderDescriptor* desc, const _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    plugin::Call<0x7618B0, const _rpD3D9VertexShaderDescriptor*, const _rpD3D9VertexShaderDispatchDescriptor*>(desc, dispatch);
}

void _rpD3D9VertexShaderUpdateMorphingCoef(RwReal morphingCoef, const _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    plugin::Call<0x761950, RwReal, const _rpD3D9VertexShaderDispatchDescriptor*>(morphingCoef, dispatch);
}

void _rpD3DVertexShaderSetUVAnimMatrix(RwMatrix* matrix, const _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    plugin::Call<0x7619A0, RwMatrix*, const _rpD3D9VertexShaderDispatchDescriptor*>(matrix, dispatch);
}

void _rpD3D9VertexShaderSetEnvMatrix(RwFrame* frame, const _rpD3D9VertexShaderDescriptor* desc, const _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    plugin::Call<0x761A10, RwFrame*, const _rpD3D9VertexShaderDescriptor*, const _rpD3D9VertexShaderDispatchDescriptor*>(frame, desc, dispatch);
}

void _rpD3D9VertexShaderSetBumpMatrix(RwFrame* frame, RwReal factor, const _rpD3D9VertexShaderDispatchDescriptor* dispatch) {
    plugin::Call<0x761B70, RwFrame*, RwReal, const _rpD3D9VertexShaderDispatchDescriptor*>(frame, factor, dispatch);
}

RxNodeDefinition* RxNodeDefinitionGetD3D9WorldSectorAllInOne() {
    return plugin::CallAndReturn<RxNodeDefinition*, 0x75E9F0>();
}

RxNodeDefinition* RxNodeDefinitionGetD3D9AtomicAllInOne() {
    return plugin::CallAndReturn<RxNodeDefinition*, 0x7582E0>();
}

void RpMaterialSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x74D8C0, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RpMaterial* RpMaterialCreate() {
    return plugin::CallAndReturn<RpMaterial*, 0x74D990>();
}

RwBool RpMaterialDestroy(RpMaterial* material) {
    return plugin::CallAndReturn<RwBool, 0x74DA20, RpMaterial*>(material);
}

RpMaterial* RpMaterialClone(RpMaterial* material) {
    return plugin::CallAndReturn<RpMaterial*, 0x74DA80, RpMaterial*>(material);
}

RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture) {
    return plugin::CallAndReturn<RpMaterial*, 0x74DBC0, RpMaterial*, RwTexture*>(material, texture);
}

RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x74DBF0, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x74DC20, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RpMaterialSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x74DC50, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwInt32 RpMaterialSetStreamRightsCallBack(RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB) {
    return plugin::CallAndReturn<RwInt32, 0x74DC70, RwUInt32, RwPluginDataChunkRightsCallBack>(pluginID, rightsCB);
}

RwInt32 RpMaterialGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x74DC90, RwUInt32>(pluginID);
}

RwBool RpMaterialValidatePlugins(const RpMaterial* material) {
    return plugin::CallAndReturn<RwBool, 0x74DCB0, const RpMaterial*>(material);
}

RwUInt32 RpMaterialStreamGetSize(const RpMaterial* material) {
    return plugin::CallAndReturn<RwUInt32, 0x74E010, const RpMaterial*>(material);
}

RpMaterial* RpMaterialStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RpMaterial*, 0x74DD30, RwStream*>(stream);
}

const RpMaterial* RpMaterialStreamWrite(const RpMaterial* material, RwStream* stream) {
    return plugin::CallAndReturn<const RpMaterial*, 0x74E050, const RpMaterial*, RwStream*>(material, stream);
}

RpMaterialChunkInfo* _rpMaterialChunkInfoRead(RwStream* stream, RpMaterialChunkInfo* materialChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RpMaterialChunkInfo*, 0x74DCC0, RwStream*, RpMaterialChunkInfo*, RwInt32*>(stream, materialChunkInfo, bytesRead);
}

RpMaterialList* _rpMaterialListInitialize(RpMaterialList* matList) {
    return plugin::CallAndReturn<RpMaterialList*, 0x74E1B0, RpMaterialList*>(matList);
}

RpMaterialList* _rpMaterialListDeinitialize(RpMaterialList* matList) {
    return plugin::CallAndReturn<RpMaterialList*, 0x74E150, RpMaterialList*>(matList);
}

RpMaterial** _rpMaterialListAlloc(RwUInt32 count) {
    return plugin::CallAndReturn<RpMaterial**, 0x74E1C0, RwUInt32>(count);
}

RpMaterial* _rpMaterialListGetMaterial(const RpMaterialList* matList, RwInt32 matIndex) {
    return plugin::CallAndReturn<RpMaterial*, 0x74E2B0, const RpMaterialList*, RwInt32>(matList, matIndex);
}

RpMaterialList* _rpMaterialListSetSize(RpMaterialList* matList, RwInt32 size) {
    return plugin::CallAndReturn<RpMaterialList*, 0x74E2C0, RpMaterialList*, RwInt32>(matList, size);
}

RpMaterialList* _rpMaterialListCopy(RpMaterialList* matListOut, const RpMaterialList* matListIn) {
    return plugin::CallAndReturn<RpMaterialList*, 0x74E1F0, RpMaterialList*, const RpMaterialList*>(matListOut, matListIn);
}

RwInt32 _rpMaterialListAppendMaterial(RpMaterialList* matList, RpMaterial* material) {
    return plugin::CallAndReturn<RwInt32, 0x74E350, RpMaterialList*, RpMaterial*>(matList, material);
}

RwInt32 _rpMaterialListFindMaterialIndex(const RpMaterialList* matList, const RpMaterial* material) {
    return plugin::CallAndReturn<RwInt32, 0x74E420, const RpMaterialList*, const RpMaterial*>(matList, material);
}

RwUInt32 _rpMaterialListStreamGetSize(const RpMaterialList* matList) {
    return plugin::CallAndReturn<RwUInt32, 0x74E450, const RpMaterialList*>(matList);
}

RpMaterialList* _rpMaterialListStreamRead(RwStream* stream, RpMaterialList* matList) {
    return plugin::CallAndReturn<RpMaterialList*, 0x74E600, RwStream*, RpMaterialList*>(stream, matList);
}

const RpMaterialList* _rpMaterialListStreamWrite(const RpMaterialList* matList, RwStream* stream) {
    return plugin::CallAndReturn<const RpMaterialList*, 0x74E4B0, const RpMaterialList*, RwStream*>(matList, stream);
}

void* _rpMeshOpen(void* instance, RwInt32 offset, RwInt32 size) {
    return plugin::CallAndReturn<void*, 0x758970, void*, RwInt32, RwInt32>(instance, offset, size);
}

void* _rpMeshClose(void* instance, RwInt32 offset, RwInt32 size) {
    return plugin::CallAndReturn<void*, 0x758940, void*, RwInt32, RwInt32>(instance, offset, size);
}

RwInt16 _rpMeshGetNextSerialNumber(void) {
    return plugin::CallAndReturn<RwInt16, 0x7590E0>();
}

RpBuildMesh* _rpBuildMeshCreate(RwUInt32 bufferSize) {
    return plugin::CallAndReturn<RpBuildMesh*, 0x758A90, RwUInt32>(bufferSize);
}

RwBool _rpBuildMeshDestroy(RpBuildMesh* mesh) {
    return plugin::CallAndReturn<RwBool, 0x758B80, RpBuildMesh*>(mesh);
}

RwBool _rpMeshDestroy(RpMeshHeader* mesh) {
    return plugin::CallAndReturn<RwBool, 0x758BC0, RpMeshHeader*>(mesh);
}

RpBuildMesh* _rpBuildMeshAddTriangle(RpBuildMesh* mesh, RpMaterial* material, RwInt32 vert1, RwInt32 vert2, RwInt32 vert3, RwUInt16 matIndex, RwUInt16 textureIndex, RwUInt16 rasterIndex, RwUInt16 pipelineIndex) {
    return plugin::CallAndReturn<RpBuildMesh*, 0x758C00, RpBuildMesh*, RpMaterial*, RwInt32, RwInt32, RwInt32, RwUInt16, RwUInt16, RwUInt16, RwUInt16>(mesh, material, vert1, vert2, vert3, matIndex, textureIndex, rasterIndex, pipelineIndex);
}

RpMeshHeader* _rpMeshHeaderForAllMeshes(RpMeshHeader* meshHeader, RpMeshCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x758D30, RpMeshHeader*, RpMeshCallBack, void*>(meshHeader, fpCallBack, data);
}

RwStream* _rpMeshWrite(const RpMeshHeader* meshHeader, const void* object, RwStream* stream, const RpMaterialList* matList) {
    return plugin::CallAndReturn<RwStream*, 0x758D70, const RpMeshHeader*, const void*, RwStream*, const RpMaterialList*>(meshHeader, object, stream, matList);
}

RpMeshHeader* _rpMeshRead(RwStream* stream, const void* object, const RpMaterialList* matList) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x758EC0, RwStream*, const void*, const RpMaterialList*>(stream, object, matList);
}

RwInt32 _rpMeshSize(const RpMeshHeader* meshHeader, const void* object) {
    return plugin::CallAndReturn<RwInt32, 0x759090, const RpMeshHeader*, const void*>(meshHeader, object);
}

void _rpMeshHeaderDestroy(RpMeshHeader* meshHeader) {
    plugin::Call<0x758910, RpMeshHeader*>(meshHeader);
}

RpMeshHeader* _rpMeshHeaderCreate(RwUInt32 size) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x758920, RwUInt32>(size);
}

RpTriStripPolygon* RpTriStripPolygonFollowStrip(RpTriStripPolygon* curr, RpTriStripPolygon* prev) {
    return plugin::CallAndReturn<RpTriStripPolygon*, 0x75C330, RpTriStripPolygon*, RpTriStripPolygon*>(curr, prev);
}

RwUInt32 RpTriStripDefaultCost(RpTriStripPolygon* startPolygon, RwUInt32 testFrame, void* data) {
    return plugin::CallAndReturn<RwUInt32, 0x75B500, RpTriStripPolygon*, RwUInt32, void*>(startPolygon, testFrame, data);
}

RpTriStripMesh* RpTriStripMeshTunnel(RpTriStripMesh* mesh, void* data) {
    return plugin::CallAndReturn<RpTriStripMesh*, 0x75B780, RpTriStripMesh*, void*>(mesh, data);
}

RpTriStripMesh* RpTriStripMeshQuick(RpTriStripMesh* mesh, void* data) {
    return plugin::CallAndReturn<RpTriStripMesh*, 0x75BD80, RpTriStripMesh*, void*>(mesh, data);
}

RpMeshHeader* RpBuildMeshGenerateTriStrip(RpBuildMesh* buildMesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x75C380, RpBuildMesh*, void*>(buildMesh, data);
}

RpMeshHeader* RpBuildMeshGenerateTrivialTriStrip(RpBuildMesh* buildMesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x759100, RpBuildMesh*, void*>(buildMesh, data);
}

RpMeshHeader* RpBuildMeshGenerateDefaultTriStrip(RpBuildMesh* buildmesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x7591B0, RpBuildMesh*, void*>(buildmesh, data);
}

RpMeshHeader* RpBuildMeshGeneratePreprocessTriStrip(RpBuildMesh* buildmesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x75A900, RpBuildMesh*, void*>(buildmesh, data);
}

RpMeshHeader* RpBuildMeshGenerateExhaustiveTriStrip(RpBuildMesh* buildmesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x75A940, RpBuildMesh*, void*>(buildmesh, data);
}

RpMeshHeader* RpBuildMeshGenerateDefaultIgnoreWindingTriStrip(RpBuildMesh* buildmesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x75A8E0, RpBuildMesh*, void*>(buildmesh, data);
}

RpMeshHeader* RpBuildMeshGeneratePreprocessIgnoreWindingTriStrip(RpBuildMesh* buildmesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x75A920, RpBuildMesh*, void*>(buildmesh, data);
}

RpMeshHeader* RpBuildMeshGenerateExhaustiveIgnoreWindingTriStrip(RpBuildMesh* buildmesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x75B4E0, RpBuildMesh*, void*>(buildmesh, data);
}

RwBool RpMeshSetTriStripMethod(RpTriStripMeshCallBack callback, void* data) {
    return plugin::CallAndReturn<RwBool, 0x75D4C0, RpTriStripMeshCallBack, void*>(callback, data);
}

RwBool RpMeshGetTriStripMethod(RpTriStripMeshCallBack* callback, void* data) {
    return plugin::CallAndReturn<RwBool, 0x75D500, RpTriStripMeshCallBack*, void*>(callback, data);
}

RpMeshHeader* _rpTriListMeshGenerate(RpBuildMesh* buildMesh, void* data) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x75D530, RpBuildMesh*, void*>(buildMesh, data);
}

RpMeshHeader* _rpMeshOptimise(RpBuildMesh* buildmesh, RwUInt32 flags) {
    return plugin::CallAndReturn<RpMeshHeader*, 0x75D970, RpBuildMesh*, RwUInt32>(buildmesh, flags);
}

RpGeometry* RpGeometryTransform(RpGeometry* geometry, const RwMatrix* matrix) {
    return plugin::CallAndReturn<RpGeometry*, 0x74BFE0, RpGeometry*, const RwMatrix*>(geometry, matrix);
}

RpGeometry* RpGeometryCreateSpace(RwReal radius) {
    return plugin::CallAndReturn<RpGeometry*, 0x74C130, RwReal>(radius);
}

const RpMorphTarget* RpMorphTargetCalcBoundingSphere(const RpMorphTarget* morphTarget, RwSphere* boundingSphere) {
    return plugin::CallAndReturn<const RpMorphTarget*, 0x74C200, const RpMorphTarget*, RwSphere*>(morphTarget, boundingSphere);
}

RwInt32 RpGeometryAddMorphTargets(RpGeometry* geometry, RwInt32 mtcount) {
    return plugin::CallAndReturn<RwInt32, 0x74C310, RpGeometry*, RwInt32>(geometry, mtcount);
}

RwInt32 RpGeometryAddMorphTarget(RpGeometry* geometry) {
    return plugin::CallAndReturn<RwInt32, 0x74C4D0, RpGeometry*>(geometry);
}

RpGeometry* RpGeometryRemoveMorphTarget(RpGeometry* geometry, RwInt32 morphTarget) {
    return plugin::CallAndReturn<RpGeometry*, 0x74C4E0, RpGeometry*, RwInt32>(geometry, morphTarget);
}

const RpGeometry* RpGeometryTriangleSetVertexIndices(const RpGeometry* geometry, RpTriangle* triangle, RwUInt16 vert1, RwUInt16 vert2, RwUInt16 vert3) {
    return plugin::CallAndReturn<const RpGeometry*, 0x74C690, const RpGeometry*, RpTriangle*, RwUInt16, RwUInt16, RwUInt16>(geometry, triangle, vert1, vert2, vert3);
}

RpGeometry* RpGeometryTriangleSetMaterial(RpGeometry* geometry, RpTriangle* triangle, RpMaterial* material) {
    return plugin::CallAndReturn<RpGeometry*, 0x74C6C0, RpGeometry*, RpTriangle*, RpMaterial*>(geometry, triangle, material);
}

const RpGeometry* RpGeometryTriangleGetVertexIndices(const RpGeometry* geometry, const RpTriangle* triangle, RwUInt16* vert1, RwUInt16* vert2, RwUInt16* vert3) {
    return plugin::CallAndReturn<const RpGeometry*, 0x74C720, const RpGeometry*, const RpTriangle*, RwUInt16*, RwUInt16*, RwUInt16*>(geometry, triangle, vert1, vert2, vert3);
}

RpMaterial* RpGeometryTriangleGetMaterial(const RpGeometry* geometry, const RpTriangle* triangle) {
    return plugin::CallAndReturn<RpMaterial*, 0x74C760, const RpGeometry*, const RpTriangle*>(geometry, triangle);
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<RpGeometry*, 0x74C790, RpGeometry*, RpMaterialCallBack, void*>(geometry, fpCallBack, data);
}

RpGeometry* RpGeometryLock(RpGeometry* geometry, RwInt32 lockMode) {
    return plugin::CallAndReturn<RpGeometry*, 0x74C7D0, RpGeometry*, RwInt32>(geometry, lockMode);
}

RpGeometry* RpGeometryUnlock(RpGeometry* geometry) {
    return plugin::CallAndReturn<RpGeometry*, 0x74C800, RpGeometry*>(geometry);
}

const RpGeometry* RpGeometryForAllMeshes(const RpGeometry* geometry, RpMeshCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<const RpGeometry*, 0x74CA60, const RpGeometry*, RpMeshCallBack, void*>(geometry, fpCallBack, data);
}

RpGeometry* RpGeometryCreate(RwInt32 numVert, RwInt32 numTriangles, RwUInt32 format) {
    return plugin::CallAndReturn<RpGeometry*, 0x74CA90, RwInt32, RwInt32, RwUInt32>(numVert, numTriangles, format);
}

RwBool RpGeometryDestroy(RpGeometry* geometry) {
    return plugin::CallAndReturn<RwBool, 0x74CCC0, RpGeometry*>(geometry);
}

RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x74CD70, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpGeometryRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x74CDA0, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RpGeometrySetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x74CDD0, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwInt32 RpGeometryGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x74CDF0, RwUInt32>(pluginID);
}

RwBool RpGeometryValidatePlugins(const RpGeometry* geometry) {
    return plugin::CallAndReturn<RwBool, 0x74CE10, const RpGeometry*>(geometry);
}

RwUInt32 RpGeometryStreamGetSize(const RpGeometry* geometry) {
    return plugin::CallAndReturn<RwUInt32, 0x74CE20, const RpGeometry*>(geometry);
}

const RpGeometry* RpGeometryStreamWrite(const RpGeometry* geometry, RwStream* stream) {
    return plugin::CallAndReturn<const RpGeometry*, 0x74CED0, const RpGeometry*, RwStream*>(geometry, stream);
}

RpGeometry* RpGeometryStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RpGeometry*, 0x74D190, RwStream*>(stream);
}

RpGeometryChunkInfo* _rpGeometryChunkInfoRead(RwStream* stream, RpGeometryChunkInfo* geometryChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RpGeometryChunkInfo*, 0x74D750, RwStream*, RpGeometryChunkInfo*, RwInt32*>(stream, geometryChunkInfo, bytesRead);
}

RpWorldSector* RpWorldSectorRender(RpWorldSector* worldSector) {
    return plugin::CallAndReturn<RpWorldSector*, 0x761C50, RpWorldSector*>(worldSector);
}

const RpWorldSector* RpWorldSectorForAllMeshes(const RpWorldSector* sector, RpMeshCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<const RpWorldSector*, 0x761C60, const RpWorldSector*, RpMeshCallBack, void*>(sector, fpCallBack, data);
}

RwInt32 RpWorldSectorRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x761C90, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpWorldSectorRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x761CC0, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RpWorldSectorSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x761CF0, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwInt32 RpWorldSectorSetStreamRightsCallBack(RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB) {
    return plugin::CallAndReturn<RwInt32, 0x761D10, RwUInt32, RwPluginDataChunkRightsCallBack>(pluginID, rightsCB);
}

RwInt32 RpWorldSectorGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x761D30, RwUInt32>(pluginID);
}

RwBool RpWorldSectorValidatePlugins(const RpWorldSector* sector) {
    return plugin::CallAndReturn<RwBool, 0x761D50, const RpWorldSector*>(sector);
}

void RpLightSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x752250, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RpLight* RpLightCreate(RwInt32 type) {
    return plugin::CallAndReturn<RpLight*, 0x752110, RwInt32>(type);
}

RwBool RpLightDestroy(RpLight* light) {
    return plugin::CallAndReturn<RwBool, 0x7520D0, RpLight*>(light);
}

RpLight* RpLightSetRadius(RpLight* light, RwReal radius) {
    return plugin::CallAndReturn<RpLight*, 0x751A70, RpLight*, RwReal>(light, radius);
}

RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color) {
    return plugin::CallAndReturn<RpLight*, 0x751A90, RpLight*, const RwRGBAReal*>(light, color);
}

RwReal RpLightGetConeAngle(const RpLight* light) {
    return plugin::CallAndReturn<RwReal, 0x751AE0, const RpLight*>(light);
}

RpLight* RpLightSetConeAngle(RpLight* ight, RwReal angle) {
    return plugin::CallAndReturn<RpLight*, 0x751D20, RpLight*, RwReal>(ight, angle);
}

RwUInt32 RpLightStreamGetSize(const RpLight* light) {
    return plugin::CallAndReturn<RwUInt32, 0x751E10, const RpLight*>(light);
}

RpLight* RpLightStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RpLight*, 0x751F00, RwStream*>(stream);
}

RpLightChunkInfo* _rpLightChunkInfoRead(RwStream* stream, RpLightChunkInfo* lightChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RpLightChunkInfo*, 0x752060, RwStream*, RpLightChunkInfo*, RwInt32*>(stream, lightChunkInfo, bytesRead);
}

RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x751D60, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpLightRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x751D90, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RpLightSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x751DC0, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwInt32 RpLightGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x751DE0, RwUInt32>(pluginID);
}

RwBool RpLightValidatePlugins(const RpLight* light) {
    return plugin::CallAndReturn<RwBool, 0x751E00, const RpLight*>(light);
}

void RpD3D9LightSetAttenuationParams(RpLight* light, const RpD3D9AttenuationParams* params) {
    plugin::Call<0x755D20, RpLight*, const RpD3D9AttenuationParams*>(light, params);
}

void RpD3D9LightGetAttenuationParams(const RpLight* light, RpD3D9AttenuationParams* params) {
    plugin::Call<0x755D50, const RpLight*, RpD3D9AttenuationParams*>(light, params);
}

RwBool _rwD3D9LightsOpen(void) {
    return plugin::CallAndReturn<RwBool, 0x755D80>();
}

RwBool _rwD3D9LightsGlobalEnable(RpLightFlag flags) {
    return plugin::CallAndReturn<RwBool, 0x756070, RpLightFlag>(flags);
}

RwBool _rwD3D9LightDirectionalEnable(RpLight* light) {
    return plugin::CallAndReturn<RwBool, 0x756260, RpLight*>(light);
}

RwBool _rwD3D9LightLocalEnable(RpLight* light) {
    return plugin::CallAndReturn<RwBool, 0x756390, RpLight*>(light);
}

void _rwD3D9LightsEnable(RwBool enable, RwUInt32 type) {
    plugin::Call<0x756600, RwBool, RwUInt32>(enable, type);
}

void _rwD3D9LightsClose(void) {
    plugin::Call<0x755FE0>();
}

void RpAtomicSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x749720, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

void RpClumpSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x749740, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic) {
    return plugin::CallAndReturn<RpAtomic *, 0x7491C0, RpAtomic*>(atomic);
}

void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic) {
    plugin::Call<0x7491F0, RpAtomic*>(atomic);
}

const RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic) {
    return plugin::CallAndReturn<const RwSphere*, 0x749330, RpAtomic*>(atomic);
}

void RpClumpGtaCancelStream() {
    plugin::Call<0x72E700>();
}

RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* data) {
    return plugin::CallAndReturn<RpClump*, 0x749B70, RpClump*, RpAtomicCallBack, void*>(clump, callback, data);
}

RpClump* RpClumpForAllLights(RpClump* clump, RpLightCallBack callback, void* data) {
    return plugin::CallAndReturn<RpClump*, 0x749C00, RpClump*, RpLightCallBack, void*>(clump, callback, data);
}

RpClump* RpClumpForAllCameras(RpClump* clump, RwCameraCallBack callback, void* data) {
    return plugin::CallAndReturn<RpClump*, 0x749BB0, RpClump*, RwCameraCallBack, void*>(clump, callback, data);
}

RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame) {
    return plugin::CallAndReturn<RpAtomic*, 0x74BF20, RpAtomic*, RwFrame*>(atomic, frame);
}

RpClump* RpClumpCreateSpace(const RwV3d* position, RwReal radius) {
    return plugin::CallAndReturn<RpClump*, 0x749970, const RwV3d*, RwReal>(position, radius);
}

RpClump* RpClumpRender(RpClump* clump) {
    return plugin::CallAndReturn<RpClump*, 0x749B20, RpClump*>(clump);
}

RpClump* RpClumpRemoveAtomic(RpClump* clump, RpAtomic* atomic) {
    return plugin::CallAndReturn<RpClump*, 0x74A4C0, RpClump*, RpAtomic*>(clump, atomic);
}

RpClump* RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic) {
    return plugin::CallAndReturn<RpClump*, 0x74A490, RpClump*, RpAtomic*>(clump, atomic);
}

RpClump* RpClumpRemoveLight(RpClump* clump, RpLight* light) {
    return plugin::CallAndReturn<RpClump*, 0x74A520, RpClump*, RpLight*>(clump, light);
}

RpClump* RpClumpAddLight(RpClump* clump, RpLight* light) {
    return plugin::CallAndReturn<RpClump*, 0x74A4F0, RpClump*, RpLight*>(clump, light);
}

RpClump* RpClumpRemoveCamera(RpClump* clump, RwCamera* camera) {
    return plugin::CallAndReturn<RpClump*, 0x74A580, RpClump*, RwCamera*>(clump, camera);
}

RpClump* RpClumpAddCamera(RpClump* clump, RwCamera* camera) {
    return plugin::CallAndReturn<RpClump*, 0x74A550, RpClump*, RwCamera*>(clump, camera);
}

RwBool RpClumpDestroy(RpClump* clump) {
    return plugin::CallAndReturn<RwBool, 0x74A310, RpClump*>(clump);
}

RpClump* RpClumpCreate(void) {
    return plugin::CallAndReturn<RpClump*, 0x74A290>();
}

RpClump* RpClumpClone(RpClump* clump) {
    return plugin::CallAndReturn<RpClump*, 0x749F70, RpClump*>(clump);
}

RwBool RpAtomicDestroy(RpAtomic* atomic) {
    return plugin::CallAndReturn<RwBool, 0x749DC0, RpAtomic*>(atomic);
}

RpAtomic* RpAtomicClone(RpAtomic* atomic) {
    return plugin::CallAndReturn<RpAtomic*, 0x749E60, RpAtomic*>(atomic);
}

RpAtomic* RpAtomicCreate(void) {
    return plugin::CallAndReturn<RpAtomic*, 0x749C50>();
}

RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, RwUInt32 flags) {
    return plugin::CallAndReturn<RpAtomic*, 0x749D40, RpAtomic*, RpGeometry*, RwUInt32>(atomic, geometry, flags);
}

RpClump* RpClumpSetCallBack(RpClump* clump, RpClumpCallBack callback) {
    return plugin::CallAndReturn<RpClump*, 0x74A240, RpClump*, RpClumpCallBack>(clump, callback);
}

RpClumpCallBack RpClumpGetCallBack(const RpClump* clump) {
    return plugin::CallAndReturn<RpClumpCallBack, 0x74A270, const RpClump*>(clump);
}

RwInt32 RpClumpGetNumAtomics(RpClump* clump) {
    return plugin::CallAndReturn<RwInt32, 0x7498E0, RpClump*>(clump);
}

RwInt32 RpClumpGetNumLights(RpClump* clump) {
    return plugin::CallAndReturn<RwInt32, 0x749910, RpClump*>(clump);
}

RwInt32 RpClumpGetNumCameras(RpClump* clump) {
    return plugin::CallAndReturn<RwInt32, 0x749940, RpClump*>(clump);
}

RpClump* RpLightGetClump(const RpLight* light) {
    return plugin::CallAndReturn<RpClump*, 0x749E40, const RpLight*>(light);
}

RpClump* RwCameraGetClump(const RwCamera* camera) {
    return plugin::CallAndReturn<RpClump*, 0x749E50, const RwCamera*>(camera);
}

RwUInt32 RpAtomicStreamGetSize(RpAtomic* atomic) {
    return plugin::CallAndReturn<RwUInt32, 0x74A5B0, RpAtomic*>(atomic);
}

RpAtomic* RpAtomicStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RpAtomic*, 0x74B030, RwStream*>(stream);
}

RpAtomic* RpAtomicStreamWrite(RpAtomic* atomic, RwStream* stream) {
    return plugin::CallAndReturn<RpAtomic*, 0x74A850, RpAtomic*, RwStream*>(atomic, stream);
}

RwUInt32 RpClumpStreamGetSize(RpClump* clump) {
    return plugin::CallAndReturn<RwUInt32, 0x74A5E0, RpClump*>(clump);
}

RpClump* RpClumpStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RpClump*, 0x74B420, RwStream*>(stream);
}

RpClump* RpClumpStreamWrite(RpClump* clump, RwStream* stream) {
    return plugin::CallAndReturn<RpClump*, 0x74AA10, RpClump*, RwStream*>(clump, stream);
}

RpClumpChunkInfo* _rpClumpChunkInfoRead(RwStream* stream, RpClumpChunkInfo* clumpChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RpClumpChunkInfo*, 0x74BD40, RwStream*, RpClumpChunkInfo*, RwInt32*>(stream, clumpChunkInfo, bytesRead);
}

RwInt32 RpAtomicRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x74BDA0, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpClumpRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x74BDD0, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpAtomicRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x74BE00, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RpAtomicSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x74BE30, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwInt32 RpAtomicSetStreamRightsCallBack(RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB) {
    return plugin::CallAndReturn<RwInt32, 0x74BE50, RwUInt32, RwPluginDataChunkRightsCallBack>(pluginID, rightsCB);
}

RwInt32 RpClumpRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x74BE70, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RpClumpSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x74BEA0, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwInt32 RpAtomicGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x74BEC0, RwUInt32>(pluginID);
}

RwInt32 RpClumpGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x74BEE0, RwUInt32>(pluginID);
}

RwBool RpAtomicValidatePlugins(const RpAtomic* atomic) {
    return plugin::CallAndReturn<RwBool, 0x74BF00, const RpAtomic*>(atomic);
}

RwBool RpClumpValidatePlugins(const RpClump* clump) {
    return plugin::CallAndReturn<RwBool, 0x74BF10, const RpClump*>(clump);
}

RwBool RpAtomicInstance(RpAtomic* atomic) {
    return plugin::CallAndReturn<RwBool, 0x74BF40, RpAtomic*>(atomic);
}

RwBool _rpWorldFindBBox(RpWorld* world, RwBBox* boundingBox) {
    return plugin::CallAndReturn<RwBool, 0x74EFA0, RpWorld*, RwBBox*>(world, boundingBox);
}

RpWorld* _rpWorldSetupSectorBoundingBoxes(RpWorld* world) {
    return plugin::CallAndReturn<RpWorld*, 0x74F020, RpWorld*>(world);
}

void _rpWorldSectorDeinstanceAll(RpSector* sector) {
    plugin::Call<0x74ECA0, RpSector*>(sector);
}

void _rpWorldSectorDestroyRecurse(RpSector* sector) {
    plugin::Call<0x74ED50, RpSector*>(sector);
}

RwBool _rpWorldForAllGlobalLights(RpLightCallBack callBack, void* data) {
    return plugin::CallAndReturn<RwBool, 0x74EF10, RpLightCallBack, void*>(callBack, data);
}

RpWorldSector* _rpWorldSectorForAllLocalLights(RpWorldSector* sector, RpLightCallBack callBack, void* data) {
    return plugin::CallAndReturn<RpWorldSector*, 0x74EF60, RpWorldSector*, RpLightCallBack, void*>(sector, callBack, data);
}

RpWorldSector* _rpSectorDefaultRenderCallBack(RpWorldSector* sector) {
    return plugin::CallAndReturn<RpWorldSector*, 0x74EEC0, RpWorldSector*>(sector);
}

RpWorld* RpWorldLock(RpWorld* world) {
    return plugin::CallAndReturn<RpWorld*, 0x74F1A0, RpWorld*>(world);
}

RpWorld* RpWorldUnlock(RpWorld* world) {
    return plugin::CallAndReturn<RpWorld*, 0x74F210, RpWorld*>(world);
}

void _rpWorldRegisterWorld(RpWorld* world, RwUInt32 memorySize) {
    plugin::Call<0x74F0C0, RpWorld*, RwUInt32>(world, memorySize);
}

void _rpWorldUnregisterWorld(RpWorld* world) {
    plugin::Call<0x74F140, RpWorld*>(world);
}

RwBool RpWorldPluginAttach() {
    return plugin::CallAndReturn<RwBool, 0x74FDA0>();
}

RpWorld* RpWorldForAllClumps(RpWorld* world, RpClumpCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<RpWorld*, 0x74FB80, RpWorld*, RpClumpCallBack, void*>(world, fpCallBack, data);
}

RpWorld* RpWorldForAllMaterials(RpWorld* world, RpMaterialCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<RpWorld*, 0x74FBC0, RpWorld*, RpMaterialCallBack, void*>(world, fpCallBack, data);
}

RpWorld* RpWorldForAllLights(RpWorld* world, RpLightCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<RpWorld*, 0x74FC00, RpWorld*, RpLightCallBack, void*>(world, fpCallBack, data);
}

RpWorld* RpWorldForAllWorldSectors(RpWorld* world, RpWorldSectorCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<RpWorld*, 0x74FC70, RpWorld*, RpWorldSectorCallBack, void*>(world, fpCallBack, data);
}

RpWorld* RpWorldRender(RpWorld* world) {
    return plugin::CallAndReturn<RpWorld*, 0x74F570, RpWorld*>(world);
}

RwBool RpWorldDestroy(RpWorld* world) {
    return plugin::CallAndReturn<RwBool, 0x74F610, RpWorld*>(world);
}

RpWorld* RpWorldCreate(RwBBox* boundingBox) {
    return plugin::CallAndReturn<RpWorld*, 0x74F760, RwBBox*>(boundingBox);
}

RpWorld* RpWorldSetSectorRenderCallBack(RpWorld* world, RpWorldSectorCallBackRender fpCallBack) {
    return plugin::CallAndReturn<RpWorld*, 0x74F730, RpWorld*, RpWorldSectorCallBackRender>(world, fpCallBack);
}

RpWorldSectorCallBackRender RpWorldGetSectorRenderCallBack(const RpWorld* world) {
    return plugin::CallAndReturn<RpWorldSectorCallBackRender, 0x74F750, const RpWorld*>(world);
}

RpWorld* RpWorldSectorGetWorld(const RpWorldSector* sector) {
    return plugin::CallAndReturn<RpWorld*, 0x74F4E0, const RpWorldSector*>(sector);
}

RwInt32 RpWorldRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x74FCD0, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpWorldRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x74FD00, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RpWorldSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x74FD30, RwUInt32, RwPluginDataChunkAlwaysCallBack>(pluginID, alwaysCB);
}

RwInt32 RpWorldSetStreamRightsCallBack(RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB) {
    return plugin::CallAndReturn<RwInt32, 0x74FD50, RwUInt32, RwPluginDataChunkRightsCallBack>(pluginID, rightsCB);
}

RwInt32 RpWorldGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x74FD70, RwUInt32>(pluginID);
}

RwBool RpWorldValidatePlugins(RpWorld* world) {
    return plugin::CallAndReturn<RwBool, 0x74FD90, RpWorld*>(world);
}

void RpD3D9GeometrySetUsageFlags(RpGeometry* geometry, RpD3D9GeometryUsageFlag flags) {
    plugin::Call<0x7588B0, RpGeometry*, RpD3D9GeometryUsageFlag>(geometry, flags);
}

RpD3D9GeometryUsageFlag RpD3D9GeometryGetUsageFlags(const RpGeometry* geometry) {
    return plugin::CallAndReturn<RpD3D9GeometryUsageFlag, 0x7588D0, const RpGeometry*>(geometry);
}

void RpD3D9WorldSectorSetUsageFlags(RpWorldSector* sector, RpD3D9WorldSectorUsageFlag flags) {
    plugin::Call<0x7588E0, RpWorldSector*, RpD3D9WorldSectorUsageFlag>(sector, flags);
}

RpD3D9WorldSectorUsageFlag RpD3D9WorldSectorGetUsageFlags(const RpWorldSector* sector) {
    return plugin::CallAndReturn<RpD3D9WorldSectorUsageFlag, 0x758900, const RpWorldSector*>(sector);
}

void RxD3D9AllInOneSetInstanceCallBack(RxPipelineNode* node, RxD3D9AllInOneInstanceCallBack callback) {
    plugin::Call<0x757380, RxPipelineNode*, RxD3D9AllInOneInstanceCallBack>(node, callback);
}

RxD3D9AllInOneInstanceCallBack RxD3D9AllInOneGetInstanceCallBack(RxPipelineNode* node) {
    return plugin::CallAndReturn<RxD3D9AllInOneInstanceCallBack, 0x757390, RxPipelineNode*>(node);
}

void RxD3D9AllInOneSetReinstanceCallBack(RxPipelineNode* node, RxD3D9AllInOneReinstanceCallBack callback) {
    plugin::Call<0x7573A0, RxPipelineNode*, RxD3D9AllInOneReinstanceCallBack>(node, callback);
}

RxD3D9AllInOneReinstanceCallBack RxD3D9AllInOneGetReinstanceCallBack(RxPipelineNode* node) {
    return plugin::CallAndReturn<RxD3D9AllInOneReinstanceCallBack, 0x7573B0, RxPipelineNode*>(node);
}

void RxD3D9AllInOneSetLightingCallBack(RxPipelineNode* node, RxD3D9AllInOneLightingCallBack callback) {
    plugin::Call<0x7573C0, RxPipelineNode*, RxD3D9AllInOneLightingCallBack>(node, callback);
}

RxD3D9AllInOneLightingCallBack RxD3D9AllInOneGetLightingCallBack(RxPipelineNode* node) {
    return plugin::CallAndReturn<RxD3D9AllInOneLightingCallBack, 0x7573D0, RxPipelineNode*>(node);
}

void RxD3D9AllInOneSetRenderCallBack(RxPipelineNode* node, RxD3D9AllInOneRenderCallBack callback) {
    plugin::Call<0x7573E0, RxPipelineNode*, RxD3D9AllInOneRenderCallBack>(node, callback);
}

RxD3D9AllInOneRenderCallBack RxD3D9AllInOneGetRenderCallBack(RxPipelineNode* node) {
    return plugin::CallAndReturn<RxD3D9AllInOneRenderCallBack, 0x7573F0, RxPipelineNode*>(node);
}

void _rpD3D9AddDynamicGeometry(RpGeometry* geometry) {
    plugin::Call<0x74E970, RpGeometry*>(geometry);
}

void _rpD3D9RemoveDynamicGeometry(RpGeometry* geometry) {
    plugin::Call<0x74E9E0, RpGeometry*>(geometry);
}

RwUInt32 _rpD3D9VertexDeclarationGetSize(RwUInt32 type) {
    return plugin::CallAndReturn<RwUInt32, 0x7522E0, RwUInt32>(type);
}

RwUInt32 _rpD3D9VertexDeclarationGetStride(const void* vertexDeclaration) {
    return plugin::CallAndReturn<RwUInt32, 0x7522F0, const void*>(vertexDeclaration);
}

RwUInt32 _rpD3D9VertexDeclarationInstV3d(RwUInt32 type, RwUInt8* mem, const RwV3d* src, RwInt32 numVerts, RwUInt32 stride) {
    return plugin::CallAndReturn<RwUInt32, 0x752AD0, RwUInt32, RwUInt8*, const RwV3d*, RwInt32, RwUInt32>(type, mem, src, numVerts, stride);
}

RwUInt32 _rpD3D9VertexDeclarationInstV3dComp(RwUInt32 type, RwUInt8* mem, const RpVertexNormal* src, RwInt32 numVerts, RwUInt32 stride) {
    return plugin::CallAndReturn<RwUInt32, 0x7531B0, RwUInt32, RwUInt8*, const RpVertexNormal*, RwInt32, RwUInt32>(type, mem, src, numVerts, stride);
}

RwUInt32 _rpD3D9VertexDeclarationInstV3dMorph(RwUInt32 type, RwUInt8* mem, const RwV3d* src1, const RwV3d* src2, RwReal scale, RwInt32 numVerts, RwUInt32 stride) {
    return plugin::CallAndReturn<RwUInt32, 0x753B60, RwUInt32, RwUInt8*, const RwV3d*, const RwV3d*, RwReal, RwInt32, RwUInt32>(type, mem, src1, src2, scale, numVerts, stride);
}

RwUInt32 _rpD3D9VertexDeclarationInstV2d(RwUInt32 type, RwUInt8* mem, const RwV2d* src, RwInt32 numVerts, RwUInt32 stride) {
    return plugin::CallAndReturn<RwUInt32, 0x7544E0, RwUInt32, RwUInt8*, const RwV2d*, RwInt32, RwUInt32>(type, mem, src, numVerts, stride);
}

RwBool _rpD3D9VertexDeclarationInstColor(RwUInt8* mem, const RwRGBA* color, RwInt32 numVerts, RwUInt32 stride) {
    return plugin::CallAndReturn<RwBool, 0x754AE0, RwUInt8*, const RwRGBA*, RwInt32, RwUInt32>(mem, color, numVerts, stride);
}

RwUInt32 _rpD3D9VertexDeclarationInstIndices(RwUInt32 type, RwUInt8* mem, const RwUInt32* indices, RwInt32 numVerts, RwUInt32 stride) {
    return plugin::CallAndReturn<RwUInt32, 0x754B40, RwUInt32, RwUInt8*, const RwUInt32*, RwInt32, RwUInt32>(type, mem, indices, numVerts, stride);
}

RwUInt32 _rpD3D9VertexDeclarationInstIndicesRemap(RwUInt32 type, RwUInt8* mem, const RwUInt32* indices, const RwUInt8* remap, RwInt32 numVerts, RwUInt32 stride) {
    return plugin::CallAndReturn<RwUInt32, 0x754C80, RwUInt32, RwUInt8*, const RwUInt32*, const RwUInt8*, RwInt32, RwUInt32>(type, mem, indices, remap, numVerts, stride);
}

RwUInt32 _rpD3D9VertexDeclarationInstWeights(RwUInt32 type, RwUInt8* mem, const RwV4d* src, RwInt32 numVerts, RwUInt32 stride) {
    return plugin::CallAndReturn<RwUInt32, 0x752320, RwUInt32, RwUInt8*, const RwV4d*, RwInt32, RwUInt32>(type, mem, src, numVerts, stride);
}

RwUInt32 _rpD3D9VertexDeclarationInstTangent(RwUInt32 type, RwUInt8* mem, const RwV3d* pos, const RwTexCoords* texCoord, RxD3D9ResEntryHeader* meshHeader, RwUInt32 stride) {
    return plugin::CallAndReturn<RwUInt32, 0x754E20, RwUInt32, RwUInt8*, const RwV3d*, const RwTexCoords*, RxD3D9ResEntryHeader*, RwUInt32>(type, mem, pos, texCoord, meshHeader, stride);
}

void _rpD3D9VertexDeclarationUnInstV3d(RwUInt32 type, RwV3d* dst, const RwUInt8* src) {
    plugin::Call<0x7551F0, RwUInt32, RwV3d*, const RwUInt8*>(type, dst, src);
}

void _rpD3D9VertexDeclarationUnInstV2d(RwUInt32 type, RwV2d* dst, const RwUInt8* src) {
    plugin::Call<0x7555E0, RwUInt32, RwV2d*, const RwUInt8*>(type, dst, src);
}

void _rpD3D9GetMinMaxValuesV3d(const RwV3d* src, RwInt32 numElements, RwV3d* min, RwV3d* max) {
    plugin::Call<0x755830, const RwV3d*, RwInt32, RwV3d*, RwV3d*>(src, numElements, min, max);
}

void _rpD3D9GetMinMaxValuesV2d(const RwV2d* src, RwInt32 numElements, RwV2d* min, RwV2d* max) {
    plugin::Call<0x7558F0, const RwV2d*, RwInt32, RwV2d*, RwV2d*>(src, numElements, min, max);
}

RwUInt32 _rpD3D9FindFormatV3d(const RwV3d* src, RwInt32 numElements) {
    return plugin::CallAndReturn<RwUInt32, 0x755980, const RwV3d*, RwInt32>(src, numElements);
}

RwUInt32 _rpD3D9FindFormatV2d(const RwV2d* src, RwInt32 numElements) {
    return plugin::CallAndReturn<RwUInt32, 0x755AA0, const RwV2d*, RwInt32>(src, numElements);
}

RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline) {
    return plugin::CallAndReturn<RxPipeline*, 0x75E270, RxPipeline*>(pipeline);
}

RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline) {
    return plugin::CallAndReturn<RxPipeline*, 0x75E2A0, RxPipeline*>(pipeline);
}

RxPipeline* RpMaterialSetDefaultPipeline(RxPipeline* pipeline) {
    return plugin::CallAndReturn<RxPipeline*, 0x75E2D0, RxPipeline*>(pipeline);
}

const RpGeometry* RpGeometryIsCorrectlySorted(const RpGeometry* geometry, RwBool* result) {
    return plugin::CallAndReturn<const RpGeometry*, 0x75D9D0, const RpGeometry*, RwBool*>(geometry, result);
}

RpGeometry* RpGeometrySortByMaterial(const RpGeometry* geometry, RpGeometrySortByMaterialCallBack callback) {
    return plugin::CallAndReturn<RpGeometry*, 0x75DAE0, const RpGeometry*, RpGeometrySortByMaterialCallBack>(geometry, callback);
}

void RpTieSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x74FF60, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

void RpLightTieSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x74FF80, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera) {
    return plugin::CallAndReturn<RpWorld*, 0x750F50, RpWorld*, RwCamera*>(world, camera);
}

RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera) {
    return plugin::CallAndReturn<RpWorld*, 0x750F20, RpWorld*, RwCamera*>(world, camera);
}

RpWorld* RwCameraGetWorld(const RwCamera* camera) {
    return plugin::CallAndReturn<RpWorld*, 0x750F80, const RwCamera*>(camera);
}

RpWorld* RpWorldRemoveAtomic(RpWorld* world, RpAtomic* atomic) {
    return plugin::CallAndReturn<RpWorld*, 0x750FC0, RpWorld*, RpAtomic*>(world, atomic);
}

RpWorld* RpWorldAddAtomic(RpWorld* world, RpAtomic* atomic) {
    return plugin::CallAndReturn<RpWorld*, 0x750F90, RpWorld*, RpAtomic*>(world, atomic);
}

RpWorld* RpAtomicGetWorld(const RpAtomic* atomic) {
    return plugin::CallAndReturn<RpWorld*, 0x751050, const RpAtomic*>(atomic);
}

RpWorld* RpWorldAddClump(RpWorld* world, RpClump* clump) {
    return plugin::CallAndReturn<RpWorld*, 0x751300, RpWorld*, RpClump*>(world, clump);
}

RpWorld* RpWorldRemoveClump(RpWorld* world, RpClump* clump) {
    return plugin::CallAndReturn<RpWorld*, 0x751460, RpWorld*, RpClump*>(world, clump);
}

RpWorld* RpClumpGetWorld(const RpClump* clump) {
    return plugin::CallAndReturn<RpWorld*, 0x7516B0, const RpClump*>(clump);
}

RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light) {
    return plugin::CallAndReturn<RpWorld*, 0x751910, RpWorld*, RpLight*>(world, light);
}

RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light) {
    return plugin::CallAndReturn<RpWorld*, 0x751960, RpWorld*, RpLight*>(world, light);
}

RpWorld* RpLightGetWorld(const RpLight* light) {
    return plugin::CallAndReturn<RpWorld*, 0x7519E0, const RpLight*>(light);
}

RwCamera* RwCameraForAllClumpsInFrustum(RwCamera* camera, void* data) {
    return plugin::CallAndReturn<RwCamera*, 0x7516C0, RwCamera*, void*>(camera, data);
}

RwCamera* RwCameraForAllAtomicsInFrustum(RwCamera* camera, RpAtomicCallBack callback, void* data) {
    return plugin::CallAndReturn<RwCamera*, 0x7517F0, RwCamera*, RpAtomicCallBack, void*>(camera, callback, data);
}

RwCamera* RwCameraForAllSectorsInFrustum(RwCamera* camera, RpWorldSectorCallBack callBack, void* data) {
    return plugin::CallAndReturn<RwCamera*, 0x751660, RwCamera*, RpWorldSectorCallBack, void*>(camera, callBack, data);
}

RpLight* RpLightForAllWorldSectors(RpLight* light, RpWorldSectorCallBack callback, void* data) {
    return plugin::CallAndReturn<RpLight*, 0x7519F0, RpLight*, RpWorldSectorCallBack, void*>(light, callback, data);
}

RpAtomic* RpAtomicForAllWorldSectors(RpAtomic* atomic, RpWorldSectorCallBack callback, void* data) {
    return plugin::CallAndReturn<RpAtomic*, 0x751060, RpAtomic*, RpWorldSectorCallBack, void*>(atomic, callback, data);
}

RpWorldSector* RpWorldSectorForAllAtomics(RpWorldSector* sector, RpAtomicCallBack callback, void* data) {
    return plugin::CallAndReturn<RpWorldSector*, 0x7510A0, RpWorldSector*, RpAtomicCallBack, void*>(sector, callback, data);
}

RpWorldSector* RpWorldSectorForAllCollisionAtomics(RpWorldSector* sector, RpAtomicCallBack callback, void* data) {
    return plugin::CallAndReturn<RpWorldSector*, 0x751140, RpWorldSector*, RpAtomicCallBack, void*>(sector, callback, data);
}

RpWorldSector* RpWorldSectorForAllLights(RpWorldSector* sector, RpLightCallBack callback, void* data) {
    return plugin::CallAndReturn<RpWorldSector*, 0x7511E0, RpWorldSector*, RpLightCallBack, void*>(sector, callback, data);
}

RwUInt32 RpWorldStreamGetSize(const RpWorld* world) {
    return plugin::CallAndReturn<RwUInt32, 0x761EA0, const RpWorld*>(world);
}

RpWorld* RpWorldStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RpWorld*, 0x762960, RwStream*>(stream);
}

const RpWorld* RpWorldStreamWrite(const RpWorld* world, RwStream* stream) {
    return plugin::CallAndReturn<const RpWorld*, 0x762150, const RpWorld*, RwStream*>(world, stream);
}

RpWorldSectorChunkInfo* _rpWorldSectorChunkInfoRead(RwStream* stream, RpWorldSectorChunkInfo* worldSectorChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RpWorldSectorChunkInfo*, 0x7635B0, RwStream*, RpWorldSectorChunkInfo*, RwInt32*>(stream, worldSectorChunkInfo, bytesRead);
}

RpPlaneSectorChunkInfo* _rpPlaneSectorChunkInfoRead(RwStream* stream, RpPlaneSectorChunkInfo* planeSectorChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RpPlaneSectorChunkInfo*, 0x763620, RwStream*, RpPlaneSectorChunkInfo*, RwInt32*>(stream, planeSectorChunkInfo, bytesRead);
}

RpWorldChunkInfo* _rpWorldChunkInfoRead(RwStream* stream, RpWorldChunkInfo* worldChunkInfo, RwInt32* bytesRead) {
    return plugin::CallAndReturn<RpWorldChunkInfo*, 0x763690, RwStream*, RpWorldChunkInfo*, RwInt32*>(stream, worldChunkInfo, bytesRead);
}


/****************************************************************************
_rwD3D9EnableClippingIfNeeded
Purpose:
On entry:
On exit :
*/
void
_rwD3D9EnableClippingIfNeeded(void *object,
    RwUInt32 type)
{
#ifndef NOTSA_NATIVE
    RwCamera    *camera;
    RwBool      fullyInsideFrustum;

    RWFUNCTION(RWSTRING("_rwD3D9EnableClippingIfNeeded"));
    RWASSERT(object != NULL);

    camera = RwCameraGetCurrentCamera();
    RWASSERT(camera != NULL);

    if (type == rpATOMIC)
    {
        RpAtomic    *atomic;

        atomic = (RpAtomic *)object;

        fullyInsideFrustum =
            RwD3D9CameraIsSphereFullyInsideFrustum(camera,
                RpAtomicGetWorldBoundingSphere(atomic));
    }
    else
    {
        const RpWorldSector   *worldSector;

        RWASSERT(type == ((RwUInt8)rwSECTORATOMIC));

        worldSector = (const RpWorldSector *)object;

        fullyInsideFrustum =
            RwD3D9CameraIsBBoxFullyInsideFrustum(camera,
                RpWorldSectorGetTightBBox(worldSector));
    }

    RwD3D9SetRenderState(D3DRS_CLIPPING, (fullyInsideFrustum == FALSE));

    RWRETURNVOID();
#else
    // D3D9 specific - replaced by the librw backend in B3
    UNUSED(object);
    UNUSED(type);
#endif
}
