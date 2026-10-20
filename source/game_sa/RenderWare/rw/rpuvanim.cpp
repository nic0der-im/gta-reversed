#include "StdInc.h"

#include "rpuvanim.h"

_rpUVAnimCustomData* _rpUVAnimCustomDataStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<_rpUVAnimCustomData*, 0x7CBF70, RwStream*>(stream);
}

const _rpUVAnimCustomData* _rpUVAnimCustomDataStreamWrite(const _rpUVAnimCustomData* customData, RwStream* stream) {
    return plugin::CallAndReturn<const _rpUVAnimCustomData*, 0x7CBFD0, const _rpUVAnimCustomData*, RwStream*>(customData, stream);
}

RwUInt32 _rpUVAnimCustomDataStreamGetSize(const _rpUVAnimCustomData* customData) {
    return plugin::CallAndReturn<RwUInt32, 0x7CC010, const _rpUVAnimCustomData*>(customData);
}

RwBool RpUVAnimPluginAttach() {
    return plugin::CallAndReturn<RwBool, 0x7CB940>();
}

void RpUVAnimLinearKeyFrameApply(void* matrix, void* voidIFrame) {
    plugin::Call<0x7CC9F0, void*, void*>(matrix, voidIFrame);
}

void RpUVAnimLinearKeyFrameBlend(void* voidOut, void* voidIn1, void* voidIn2, RwReal alpha) {
    plugin::Call<0x7CCAC0, void*, void*, void*, RwReal>(voidOut, voidIn1, voidIn2, alpha);
}

void RpUVAnimLinearKeyFrameInterpolate(void* voidOut, void* voidIn1, void* voidIn2, RwReal time, void* customData) {
    plugin::Call<0x7CCA40, void*, void*, void*, RwReal, void*>(voidOut, voidIn1, voidIn2, time, customData);
}

void RpUVAnimLinearKeyFrameAdd(void* voidOut, void* voidIn1, void* voidIn2) {
    plugin::Call<0x7CCBE0, void*, void*, void*>(voidOut, voidIn1, voidIn2);
}

void RpUVAnimLinearKeyFrameMulRecip(void* voidFrame, void* voidStart) {
    plugin::Call<0x7CCB30, void*, void*>(voidFrame, voidStart);
}

void RpUVAnimParamKeyFrameApply(void* matrix, void* voidIFrame) {
    plugin::Call<0x7CC560, void*, void*>(matrix, voidIFrame);
}

void RpUVAnimParamKeyFrameBlend(void* voidOut, void* voidIn1, void* voidIn2, RwReal alpha) {
    plugin::Call<0x7CC6A0, void*, void*, void*, RwReal>(voidOut, voidIn1, voidIn2, alpha);
}

void RpUVAnimParamKeyFrameInterpolate(void* voidOut, void* voidIn1, void* voidIn2, RwReal time, void* customData) {
    plugin::Call<0x7CC600, void*, void*, void*, RwReal, void*>(voidOut, voidIn1, voidIn2, time, customData);
}

void RpUVAnimParamKeyFrameAdd(void* voidOut, void* voidIn1, void* voidIn2) {
    plugin::Call<0x7CC750, void*, void*, void*>(voidOut, voidIn1, voidIn2);
}

void RpUVAnimParamKeyFrameMulRecip(void* voidFrame, void* voidStart) {
    plugin::Call<0x7CC740, void*, void*>(voidFrame, voidStart);
}

RtAnimAnimation* RpUVAnimKeyFrameStreamRead(RwStream* stream, RtAnimAnimation* animation) {
    return plugin::CallAndReturn<RtAnimAnimation*, 0x7CC870, RwStream*, RtAnimAnimation*>(stream, animation);
}

RwBool RpUVAnimKeyFrameStreamWrite(const RtAnimAnimation* animation, RwStream* stream) {
    return plugin::CallAndReturn<RwBool, 0x7CC920, const RtAnimAnimation*, RwStream*>(animation, stream);
}

RwInt32 RpUVAnimKeyFrameStreamGetSize(const RtAnimAnimation* animation) {
    return plugin::CallAndReturn<RwInt32, 0x7CC9D0, const RtAnimAnimation*>(animation);
}

RpUVAnim* RpUVAnimCreate(const RwChar* name, RwUInt32 numNodes, RwUInt32 numFrames, RwReal duration, RwUInt32* nodeIndexToUVChannelMap, RpUVAnimKeyFrameType keyframeType) {
    return plugin::CallAndReturn<RpUVAnim*, 0x7CC020, const RwChar*, RwUInt32, RwUInt32, RwReal, RwUInt32*, RpUVAnimKeyFrameType>(name, numNodes, numFrames, duration, nodeIndexToUVChannelMap, keyframeType);
}

RwBool RpUVAnimDestroy(RpUVAnim* anim) {
    return plugin::CallAndReturn<RwBool, 0x7CC0C0, RpUVAnim*>(anim);
}

RpUVAnim* RpUVAnimAddRef(RpUVAnim* anim) {
    return plugin::CallAndReturn<RpUVAnim*, 0x7CC100, RpUVAnim*>(anim);
}

const RwChar* RpUVAnimGetName(const RpUVAnim* anim) {
    return plugin::CallAndReturn<const RwChar*, 0x7CC3A0, const RpUVAnim*>(anim);
}

RpMaterial* RpMaterialSetUVAnim(RpMaterial* material, RpUVAnim* anim, RwUInt32 slot) {
    return plugin::CallAndReturn<RpMaterial*, 0x7CC3B0, RpMaterial*, RpUVAnim*, RwUInt32>(material, anim, slot);
}

RpUVAnimInterpolator* RpMaterialUVAnimGetInterpolator(RpMaterial* material, RwUInt32 slot) {
    return plugin::CallAndReturn<RpUVAnimInterpolator*, 0x7CC430, RpMaterial*, RwUInt32>(material, slot);
}

RpMaterial* RpMaterialUVAnimSetInterpolator(RpMaterial* material, RpUVAnimInterpolator* interp, RwUInt32 slot) {
    return plugin::CallAndReturn<RpMaterial*, 0x7CC450, RpMaterial*, RpUVAnimInterpolator*, RwUInt32>(material, interp, slot);
}

RpMaterial* RpMaterialUVAnimSetCurrentTime(RpMaterial* material, RwReal time) {
    return plugin::CallAndReturn<RpMaterial*, 0x7CC470, RpMaterial*, RwReal>(material, time);
}

RpMaterial* RpMaterialUVAnimAddAnimTime(RpMaterial* material, RwReal deltaTime) {
    return plugin::CallAndReturn<RpMaterial*, 0x7CC4B0, RpMaterial*, RwReal>(material, deltaTime);
}

RpMaterial* RpMaterialUVAnimSubAnimTime(RpMaterial* material, RwReal deltaTime) {
    return plugin::CallAndReturn<RpMaterial*, 0x7CC4F0, RpMaterial*, RwReal>(material, deltaTime);
}

RpMaterial* RpMaterialUVAnimApplyUpdate(RpMaterial* material)
{
    return plugin::CallAndReturn<RpMaterial*, 0x7CC110, RpMaterial*>(material);
}

RwBool RpMaterialUVAnimExists(const RpMaterial* material) {
    return plugin::CallAndReturn<RwBool, 0x7CC530, const RpMaterial*>(material);
}
