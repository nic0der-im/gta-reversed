#include "StdInc.h"

#include "rwplcore.h"

RwMemoryFunctions* RwOsGetMemoryInterface() {
    return plugin::CallAndReturn<RwMemoryFunctions*, 0x802230>();
}

RwFreeList* _rwFreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment, RwUInt32 hint, const RwChar* fileCreate, RwUInt32 lineCreate) {
    return plugin::CallAndReturn<RwFreeList*, 0x801980, RwInt32, RwInt32, RwInt32, RwUInt32, const RwChar*, RwUInt32>(entrySize, entriesPerBlock, alignment, hint, fileCreate, lineCreate);
}

RwFreeList* RwFreeListCreateAndPreallocateSpace(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment, RwInt32 numBlocksToPreallocate, RwFreeList* inPlaceSpaceForFreeListStruct, RwUInt32 hint) {
    return plugin::CallAndReturn<RwFreeList*, 0x801B70, RwInt32, RwInt32, RwInt32, RwInt32, RwFreeList*, RwUInt32>(entrySize, entriesPerBlock, alignment, numBlocksToPreallocate, inPlaceSpaceForFreeListStruct, hint);
}

RwBool RwFreeListDestroy(RwFreeList* freelist) {
    return plugin::CallAndReturn<RwBool, 0x801B80, RwFreeList*>(freelist);
}

void RwFreeListSetFlags(RwFreeList* freeList, RwUInt32 flags) {
    plugin::Call<0x801C10, RwFreeList*, RwUInt32>(freeList, flags);
}

RwUInt32 RwFreeListGetFlags(RwFreeList* freeList) {
    return plugin::CallAndReturn<RwUInt32, 0x801C20, RwFreeList*>(freeList);
}

RwInt32 RwFreeListPurge(RwFreeList* freelist) {
    return plugin::CallAndReturn<RwInt32, 0x801E00, RwFreeList*>(freelist);
}

RwFreeList* RwFreeListForAllUsed(RwFreeList* freelist, RwFreeListCallBack fpCallBack, void* data) {
    return plugin::CallAndReturn<RwFreeList*, 0x801E90, RwFreeList*, RwFreeListCallBack, void*>(freelist, fpCallBack, data);
}

RwInt32 RwFreeListPurgeAllFreeLists() {
    return plugin::CallAndReturn<RwInt32, 0x801F90>();
}

void RwStreamSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x7EC760, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwStream* _rwStreamInitialize(RwStream* stream, RwBool rwOwned, RwStreamType type, RwStreamAccessType accessType, const void* data) {
    return plugin::CallAndReturn<RwStream*, 0x7EC810, RwStream*, RwBool, RwStreamType, RwStreamAccessType, const void*>(stream, rwOwned, type, accessType, data);
}

RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType accessType, const void* data) {
    return plugin::CallAndReturn<RwStream*, 0x7ECEF0, RwStreamType, RwStreamAccessType, const void*>(type, accessType, data);
}

RwBool RwStreamClose(RwStream* stream, void* data) {
    return plugin::CallAndReturn<RwBool, 0x7ECE20, RwStream*, void*>(stream, data);
}

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length) {
    return plugin::CallAndReturn<RwUInt32, 0x7EC9D0, RwStream*, void*, RwUInt32>(stream, buffer, length);
}

RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length) {
    return plugin::CallAndReturn<RwStream*, 0x7ECB30, RwStream*, const void*, RwUInt32>(stream, buffer, length);
}

RwStream* RwStreamSkip(RwStream* stream, RwUInt32 offset) {
    return plugin::CallAndReturn<RwStream*, 0x7ECD00, RwStream*, RwUInt32>(stream, offset);
}

RwBool _rwStringOpen() {
    return plugin::CallAndReturn<RwBool, 0x80A240>();
}

void _rwStringClose() {
    plugin::Call<0x80A440>();
}

RwBool _rwStringDestroy(RwChar* string) {
    return plugin::CallAndReturn<RwBool, 0x80A450, RwChar*>(string);
}

RwUInt32 _rwStringStreamGetSize(const RwChar* string) {
    return plugin::CallAndReturn<RwUInt32, 0x80A470, const RwChar*>(string);
}

const RwChar* _rwStringStreamWrite(const RwChar* string, RwStream* stream) {
    return plugin::CallAndReturn<const RwChar*, 0x80A4A0, const RwChar*, RwStream*>(string, stream);
}

RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream) {
    return plugin::CallAndReturn<RwChar*, 0x80A510, RwChar*, RwStream*>(string, stream);
}

void RwPluginRegistrySetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x8087B0, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwBool _rwPluginRegistrySetStaticPluginsSize(RwPluginRegistry* reg, RwInt32 size) {
    return plugin::CallAndReturn<RwBool, 0x808430, RwPluginRegistry*, RwInt32>(reg, size);
}

RwInt32 _rwPluginRegistryAddPlugin(RwPluginRegistry* reg, RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return plugin::CallAndReturn<RwInt32, 0x8084A0, RwPluginRegistry*, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy>(reg, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 _rwPluginRegistryGetPluginOffset(const RwPluginRegistry* reg, RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x808470, const RwPluginRegistry*, RwUInt32>(reg, pluginID);
}

const RwPluginRegistry* _rwPluginRegistryInitObject(const RwPluginRegistry* reg, void* object) {
    return plugin::CallAndReturn<const RwPluginRegistry*, 0x8086E0, const RwPluginRegistry*, void*>(reg, object);
}

const RwPluginRegistry* _rwPluginRegistryDeInitObject(const RwPluginRegistry* reg, void* object) {
    return plugin::CallAndReturn<const RwPluginRegistry*, 0x808740, const RwPluginRegistry*, void*>(reg, object);
}

const RwPluginRegistry* _rwPluginRegistryCopyObject(const RwPluginRegistry* reg, void* dstObject, const void* srcObject) {
    return plugin::CallAndReturn<const RwPluginRegistry*, 0x808770, const RwPluginRegistry*, void*, const void*>(reg, dstObject, srcObject);
}

RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return plugin::CallAndReturn<RwInt32, 0x8088E0, RwPluginRegistry*, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack>(reg, pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 _rwPluginRegistryAddPlgnStrmlwysCB(RwPluginRegistry* reg, RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return plugin::CallAndReturn<RwInt32, 0x808920, RwPluginRegistry*, RwUInt32, RwPluginDataChunkAlwaysCallBack>(reg, pluginID, alwaysCB);
}

RwInt32 _rwPluginRegistryAddPlgnStrmRightsCB(RwPluginRegistry* reg, RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB) {
    return plugin::CallAndReturn<RwInt32, 0x808950, RwPluginRegistry*, RwUInt32, RwPluginDataChunkRightsCallBack>(reg, pluginID, rightsCB);
}

const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream, void* object) {
    return plugin::CallAndReturn<const RwPluginRegistry*, 0x808980, const RwPluginRegistry*, RwStream*, void*>(reg, stream, object);
}

const RwPluginRegistry* _rwPluginRegistryWriteDataChunks(const RwPluginRegistry* reg, RwStream* stream, const void* object) {
    return plugin::CallAndReturn<const RwPluginRegistry*, 0x808B40, const RwPluginRegistry*, RwStream*, const void*>(reg, stream, object);
}

const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(const RwPluginRegistry* reg, RwStream* stream) {
    return plugin::CallAndReturn<const RwPluginRegistry*, 0x808C10, const RwPluginRegistry*, RwStream*>(reg, stream);
}

RwInt32 _rwPluginRegistryGetSize(const RwPluginRegistry* reg, const void* object) {
    return plugin::CallAndReturn<RwInt32, 0x808B00, const RwPluginRegistry*, const void*>(reg, object);
}

const RwPluginRegistry* _rwPluginRegistryInvokeRights(const RwPluginRegistry* reg, RwUInt32 id, void* obj, RwUInt32 extraData) {
    return plugin::CallAndReturn<const RwPluginRegistry*, 0x808AB0, const RwPluginRegistry*, RwUInt32, void*, RwUInt32>(reg, id, obj, extraData);
}

RwBool RwEngineGetMatrixTolerances(RwMatrixTolerance * const tolerance) {
    return plugin::CallAndReturn<RwBool, 0x7F1780, RwMatrixTolerance * const>(tolerance);
}

RwBool RwEngineSetMatrixTolerances(const RwMatrixTolerance * const tolerance) {
    return plugin::CallAndReturn<RwBool, 0x7F17B0, const RwMatrixTolerance*>(tolerance);
}

void RwMatrixSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x7F16A0, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwBool RwMatrixDestroy(RwMatrix* mpMat) {
    return plugin::CallAndReturn<RwBool, 0x7F2A20, RwMatrix*>(mpMat);
}

RwMatrix* RwMatrixCreate() {
    return plugin::CallAndReturn<RwMatrix*, 0x7F2A50>();
}

RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* MatrixIn1, const RwMatrix* matrixIn2) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F18B0, RwMatrix*, const RwMatrix*, const RwMatrix*>(matrixOut, MatrixIn1, matrixIn2);
}

RwMatrix* RwMatrixTransform(RwMatrix* matrix, const RwMatrix* transform, RwOpCombineType combineOp) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F25A0, RwMatrix*, const RwMatrix*, RwOpCombineType>(matrix, transform, combineOp);
}

RwMatrix* RwMatrixOrthoNormalize(RwMatrix* matrixOut, const RwMatrix* matrixIn) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F1920, RwMatrix*, const RwMatrix*>(matrixOut, matrixIn);
}

RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F2070, RwMatrix*, const RwMatrix*>(matrixOut, matrixIn);
}

RwMatrix* RwMatrixScale(RwMatrix* matrix, const RwV3d* scale, RwOpCombineType combineOp) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F22C0, RwMatrix*, const RwV3d*, RwOpCombineType>(matrix, scale, combineOp);
}

RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation, RwOpCombineType combineOp) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F2450, RwMatrix*, const RwV3d*, RwOpCombineType>(matrix, translation, combineOp);
}

RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle, RwOpCombineType combineOp) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F1FD0, RwMatrix*, const RwV3d*, RwReal, RwOpCombineType>(matrix, axis, angle, combineOp);
}

RwMatrix* RwMatrixRotateOneMinusCosineSine(RwMatrix* matrix, const RwV3d* unitAxis, RwReal oneMinusCosine, RwReal sine, RwOpCombineType combineOp) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F1D00, RwMatrix*, const RwV3d*, RwReal, RwReal, RwOpCombineType>(matrix, unitAxis, oneMinusCosine, sine, combineOp);
}

const RwMatrix* RwMatrixQueryRotate(const RwMatrix* matrix, RwV3d* unitAxis, RwReal* angle, RwV3d* center) {
    return plugin::CallAndReturn<const RwMatrix*, 0x7F2720, const RwMatrix*, RwV3d*, RwReal*, RwV3d*>(matrix, unitAxis, angle, center);
}

RwMatrix* RwMatrixUpdate(RwMatrix* matrix) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F18A0, RwMatrix*>(matrix);
}

RwMatrix* RwMatrixOptimize(RwMatrix* matrix, const RwMatrixTolerance* tolerance) {
    return plugin::CallAndReturn<RwMatrix*, 0x7F17E0, RwMatrix*, const RwMatrixTolerance*>(matrix, tolerance);
}

RwReal _rwMatrixDeterminant(const RwMatrix* matrix) {
    return plugin::CallAndReturn<RwReal, 0x7F1450, const RwMatrix*>(matrix);
}

RwReal _rwMatrixNormalError(const RwMatrix* matrix) {
    return plugin::CallAndReturn<RwReal, 0x7F1500, const RwMatrix*>(matrix);
}

RwReal _rwMatrixOrthogonalError(const RwMatrix* matrix) {
    return plugin::CallAndReturn<RwReal, 0x7F14A0, const RwMatrix*>(matrix);
}

RwReal _rwMatrixIdentityError(const RwMatrix* matrix) {
    return plugin::CallAndReturn<RwReal, 0x7F1590, const RwMatrix*>(matrix);
}

RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in) {
    return plugin::CallAndReturn<RwReal, 0x7ED9B0, RwV3d*, const RwV3d*>(out, in);
}

RwReal RwV3dLength(const RwV3d* in) {
    return plugin::CallAndReturn<RwReal, 0x7EDAC0, const RwV3d*>(in);
}

RwReal RwV2dLength(const RwV2d* in) {
    return plugin::CallAndReturn<RwReal, 0x7EDBF0, const RwV2d*>(in);
}

RwReal RwV2dNormalize(RwV2d* out, const RwV2d* in) {
    return plugin::CallAndReturn<RwReal, 0x7EDC60, RwV2d*, const RwV2d*>(out, in);
}

RwV3d* RwV3dTransformPoint(RwV3d* pointOut, const RwV3d* pointIn, const RwMatrix* matrix) {
    return plugin::CallAndReturn<RwV3d*, 0x7EDD60, RwV3d*, const RwV3d*, const RwMatrix*>(pointOut, pointIn, matrix);
}

RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix) {
    return plugin::CallAndReturn<RwV3d*, 0x7EDD90, RwV3d*, const RwV3d*, RwInt32, const RwMatrix*>(pointsOut, pointsIn, numPoints, matrix);
}

RwV3d* RwV3dTransformVector(RwV3d* vectorOut, const RwV3d* vectorIn, const RwMatrix* matrix) {
    return plugin::CallAndReturn<RwV3d*, 0x7EDDC0, RwV3d*, const RwV3d*, const RwMatrix*>(vectorOut, vectorIn, matrix);
}

RwV3d* RwV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints, const RwMatrix* matrix) {
    return plugin::CallAndReturn<RwV3d*, 0x7EDDF0, RwV3d*, const RwV3d*, RwInt32, const RwMatrix*>(vectorsOut, vectorsIn, numPoints, matrix);
}

RwReal _rwSqrt(const RwReal num) {
    return plugin::CallAndReturn<RwReal, 0x7EDB30, const RwReal>(num);
}

RwReal _rwInvSqrt(const RwReal num) {
    return plugin::CallAndReturn<RwReal, 0x7EDB90, const RwReal>(num);
}

RwReal _rwV3dNormalize(RwV3d* out, const RwV3d* in) {
    return plugin::CallAndReturn<RwReal, 0x7ED910, RwV3d*, const RwV3d*>(out, in);
}

RwSList* _rwSListCreate(RwInt32 size, RwUInt32 hint) {
    return plugin::CallAndReturn<RwSList*, 0x809160, RwInt32, RwUInt32>(size, hint);
}

RwBool _rwSListDestroy(RwSList* sList) {
    return plugin::CallAndReturn<RwBool, 0x809440, RwSList*>(sList);
}

void _rwSListDestroyEndEntries(RwSList* sList, RwInt32 amount) {
    plugin::Call<0x809400, RwSList*, RwInt32>(sList, amount);
}

void* _rwSListGetEntry(RwSList* sList, RwInt32 entry) {
    return plugin::CallAndReturn<void*, 0x809510, RwSList*, RwInt32>(sList, entry);
}

void* _rwSListGetNewEntry(RwSList* sList, RwUInt32 hint) {
    return plugin::CallAndReturn<void*, 0x809240, RwSList*, RwUInt32>(sList, hint);
}

RwInt32 _rwSListGetNumEntries(const RwSList* sList) {
    return plugin::CallAndReturn<RwInt32, 0x8094B0, const RwSList*>(sList);
}

void* _rwSListGetBegin(RwSList* sList) {
    return plugin::CallAndReturn<void*, 0x809530, RwSList*>(sList);
}

void* _rwSListGetEnd(RwSList* sList) {
    return plugin::CallAndReturn<void*, 0x809540, RwSList*>(sList);
}

RwUInt32 RwEngineGetVersion() {
    return plugin::CallAndReturn<RwUInt32, 0x7F2BA0>();
}

RwBool RwEngineInit(const RwMemoryFunctions* memFuncs, RwUInt32 initFlags, RwUInt32 resArenaSize) {
    return plugin::CallAndReturn<RwBool, 0x7F3170, const RwMemoryFunctions*, RwUInt32, RwUInt32>(memFuncs, initFlags, resArenaSize);
}

RwInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor initCB, RwPluginObjectDestructor termCB) {
    return plugin::CallAndReturn<RwInt32, 0x7F2BB0, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor>(size, pluginID, initCB, termCB);
}

RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID) {
    return plugin::CallAndReturn<RwInt32, 0x7F2BE0, RwUInt32>(pluginID);
}

RwBool RwEngineOpen(RwEngineOpenParams* initParams) {
    return plugin::CallAndReturn<RwBool, 0x7F2F70, RwEngineOpenParams*>(initParams);
}

RwBool RwEngineStart() {
    return plugin::CallAndReturn<RwBool, 0x7F2E70>();
}

RwBool RwEngineStop() {
    return plugin::CallAndReturn<RwBool, 0x7F2E20>();
}

RwBool RwEngineClose() {
    return plugin::CallAndReturn<RwBool, 0x7F2F00>();
}

RwBool RwEngineTerm() {
    return plugin::CallAndReturn<RwBool, 0x7F3130>();
}

RwInt32 RwEngineGetNumSubSystems() {
    return plugin::CallAndReturn<RwInt32, 0x7F2C00>();
}

RwSubSystemInfo* RwEngineGetSubSystemInfo(RwSubSystemInfo* subSystemInfo, RwInt32 subSystemIndex) {
    return plugin::CallAndReturn<RwSubSystemInfo*, 0x7F2C30, RwSubSystemInfo*, RwInt32>(subSystemInfo, subSystemIndex);
}

RwInt32 RwEngineGetCurrentSubSystem() {
    return plugin::CallAndReturn<RwInt32, 0x7F2C60>();
}

RwBool RwEngineSetSubSystem(RwInt32 subSystemIndex) {
    return plugin::CallAndReturn<RwBool, 0x7F2C90, RwInt32>(subSystemIndex);
}

RwInt32 RwEngineGetNumVideoModes() {
    return plugin::CallAndReturn<RwInt32, 0x7F2CC0>();
}

RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* vmi, RwInt32 modeIndex) {
    return plugin::CallAndReturn<RwVideoMode*, 0x7F2CF0>(vmi, modeIndex);
}

RwInt32 RwEngineGetCurrentVideoMode() {
    return plugin::CallAndReturn<RwInt32, 0x7F2D20>();
}

RwBool RwEngineSetVideoMode(RwInt32 modeIndex) {
    return plugin::CallAndReturn<RwBool, 0x7F2D50, RwInt32>(modeIndex);
}

RwInt32 RwEngineGetTextureMemorySize() {
    return plugin::CallAndReturn<RwInt32, 0x7F2D80>();
}

RwInt32 RwEngineGetMaxTextureSize() {
    return plugin::CallAndReturn<RwInt32, 0x7F2DB0>();
}

RwBool RwEngineSetFocus(RwBool enable) {
    return plugin::CallAndReturn<RwBool, 0x7F2DE0, RwBool>(enable);
}

RwMetrics* RwEngineGetMetrics() {
    return plugin::CallAndReturn<RwMetrics*, 0x7F2E10>();
}

//RwFileFunctions* RwOsGetFileInterface() {
//    return plugin::CallAndReturn<RwFileFunctions*, 0x804130>();
//}

RwError* RwErrorGet(RwError* code) {
    return plugin::CallAndReturn<RwError*, 0x808880, RwError*>(code);
}

RwError* RwErrorSet(RwError* code) {
    return plugin::CallAndReturn<RwError*, 0x808820, RwError*>(code);
}

RwInt32 _rwerror(RwInt32 code, ...) {
    return code;
}

RwBool RwResourcesSetArenaSize(RwUInt32 size) {
    return plugin::CallAndReturn<RwBool, 0x8080C0, RwUInt32>(size);
}

RwInt32 RwResourcesGetArenaSize() {
    return plugin::CallAndReturn<RwInt32, 0x8081B0>();
}

RwInt32 RwResourcesGetArenaUsage() {
    return plugin::CallAndReturn<RwInt32, 0x8081C0>();
}

RwBool RwResourcesEmptyArena() {
    return plugin::CallAndReturn<RwBool, 0x8081F0>();
}

RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry* ownerRef, RwInt32 size, RwResEntryDestroyNotify destroyNotify) {
    return plugin::CallAndReturn<RwResEntry*, 0x807ED0, void*, RwResEntry*, RwInt32, RwResEntryDestroyNotify>(owner, ownerRef, size, destroyNotify);
}

RwBool RwResourcesFreeResEntry(RwResEntry* entry) {
    return plugin::CallAndReturn<RwBool, 0x807DE0, RwResEntry*>(entry);
}

void _rwResourcesPurge() {
    plugin::Call<0x807E50>();
}

RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut) {
    return plugin::CallAndReturn<RwBool, 0x7ED2D0, RwStream*, RwUInt32, RwUInt32*, RwUInt32*>(stream, type, lengthOut, versionOut);
}

RwStream* _rwStreamWriteVersionedChunkHeader(RwStream* stream, RwInt32 type, RwInt32 size, RwUInt32 version, RwUInt32 buildNum) {
    return plugin::CallAndReturn<RwStream*, 0x7ED270, RwStream*, RwInt32, RwInt32, RwUInt32, RwUInt32>(stream, type, size, version, buildNum);
}

RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* reals, RwUInt32 numBytes) {
    return plugin::CallAndReturn<RwStream*, 0x7ED3D0, RwStream*, const RwReal*, RwUInt32>(stream, reals, numBytes);
}

RwStream* RwStreamWriteInt32(RwStream* stream, const RwInt32* ints, RwUInt32 numBytes) {
    return plugin::CallAndReturn<RwStream*, 0x7ED460, RwStream*, const RwInt32*, RwUInt32>(stream, ints, numBytes);
}

RwStream* RwStreamWriteInt16(RwStream* stream, const RwInt16* ints, RwUInt32 numBytes) {
    return plugin::CallAndReturn<RwStream*, 0x7ED480, RwStream*, const RwInt16*, RwUInt32>(stream, ints, numBytes);
}

RwStream* RwStreamReadReal(RwStream* stream, RwReal* reals, RwUInt32 numBytes) {
    return plugin::CallAndReturn<RwStream*, 0x7ED4F0, RwStream*, RwReal*, RwUInt32>(stream, reals, numBytes);
}

RwStream* RwStreamReadInt32(RwStream* stream, RwInt32* ints, RwUInt32 numBytes) {
    return plugin::CallAndReturn<RwStream*, 0x7ED540, RwStream*, RwInt32*, RwUInt32>(stream, ints, numBytes);
}

RwStream* RwStreamReadInt16(RwStream* stream, RwInt16* ints, RwUInt32 numBytes) {
    return plugin::CallAndReturn<RwStream*, 0x7ED4A0, RwStream*, RwInt16*, RwUInt32>(stream, ints, numBytes);
}

RwStream* RwStreamReadChunkHeaderInfo(RwStream* stream, RwChunkHeaderInfo* chunkHeaderInfo) {
    return plugin::CallAndReturn<RwStream*, 0x7ED590, RwStream*, RwChunkHeaderInfo*>(stream, chunkHeaderInfo);
}
