#include "StdInc.h"

#include "rpmatfx.h"

void RpMatFXMaterialDataSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    plugin::Call<0x810700, RwInt32, RwInt32>(blockSize, numBlocksToPrealloc);
}

RwBool RpMatFXPluginAttach() {
    return plugin::CallAndReturn<RwBool, 0x810AA0>();
}

RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic) {
    return plugin::CallAndReturn<RpAtomic*, 0x811C00, RpAtomic*>(atomic);
}

RwBool RpMatFXAtomicQueryEffects(RpAtomic* atomic) {
    return plugin::CallAndReturn<RwBool, 0x811C30, RpAtomic*>(atomic);
}

RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* worldSector) {
    return plugin::CallAndReturn<RpWorldSector*, 0x811C40, RpWorldSector*>(worldSector);
}

RwBool RpMatFXWorldSectorQueryEffects(RpWorldSector* worldSector) {
    return plugin::CallAndReturn<RwBool, 0x811C70, RpWorldSector*>(worldSector);
}

RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, RpMatFXMaterialFlags flags) {
    return plugin::CallAndReturn<RpMaterial*, 0x811C80, RpMaterial*, RpMatFXMaterialFlags>(material, flags);
}

RpMaterial* RpMatFXMaterialSetupBumpMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwReal coef) {
    return plugin::CallAndReturn<RpMaterial*, 0x811E00, RpMaterial*, RwTexture*, RwFrame*, RwReal>(material, texture, frame, coef);
}

RpMaterial* RpMatFXMaterialSetupEnvMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwBool useFrameBufferAlpha, RwReal coef) {
    return plugin::CallAndReturn<RpMaterial*, 0x811ED0, RpMaterial*, RwTexture*, RwFrame*, RwBool, RwReal>(material, texture, frame, useFrameBufferAlpha, coef);
}

RpMaterial* RpMatFXMaterialSetupDualTexture(RpMaterial* material, RwTexture* texture, RwBlendFunction srcBlendMode, RwBlendFunction dstBlendMode) {
    return plugin::CallAndReturn<RpMaterial*, 0x812040, RpMaterial*, RwTexture*, RwBlendFunction, RwBlendFunction>(material, texture, srcBlendMode, dstBlendMode);
}

RpMatFXMaterialFlags RpMatFXMaterialGetEffects(const RpMaterial* material) {
    return plugin::CallAndReturn<RpMatFXMaterialFlags, 0x812140, const RpMaterial*>(material);
}

RpMaterial* RpMatFXMaterialSetBumpMapTexture(RpMaterial* material, RwTexture* texture) {
    return plugin::CallAndReturn<RpMaterial*, 0x812160, RpMaterial*, RwTexture*>(material, texture);
}

RpMaterial* RpMatFXMaterialSetBumpMapFrame(RpMaterial* material, RwFrame* frame) {
    return plugin::CallAndReturn<RpMaterial*, 0x812320, RpMaterial*, RwFrame*>(material, frame);
}

RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, RwReal coef) {
    return plugin::CallAndReturn<RpMaterial*, 0x812380, RpMaterial*, RwReal>(material, coef);
}

RwTexture* RpMatFXMaterialGetBumpMapTexture(const RpMaterial* material) {
    return plugin::CallAndReturn<RwTexture*, 0x812430, const RpMaterial*>(material);
}

RwTexture* RpMatFXMaterialGetBumpMapBumpedTexture(const RpMaterial* material) {
    return plugin::CallAndReturn<RwTexture*, 0x8123E0, const RpMaterial*>(material);
}

RwFrame* RpMatFXMaterialGetBumpMapFrame(const RpMaterial* material) {
    return plugin::CallAndReturn<RwFrame*, 0x812480, const RpMaterial*>(material);
}

RwReal RpMatFXMaterialGetBumpMapCoefficient(const RpMaterial* material) {
    return plugin::CallAndReturn<RwReal, 0x8124D0, const RpMaterial*>(material);
}

RpMaterial* RpMatFXMaterialSetEnvMapTexture(RpMaterial* material, RwTexture* texture) {
    return plugin::CallAndReturn<RpMaterial*, 0x812530, RpMaterial*, RwTexture*>(material, texture);
}

RpMaterial* RpMatFXMaterialSetEnvMapFrame(RpMaterial* material, RwFrame* frame) {
    return plugin::CallAndReturn<RpMaterial*, 0x8125B0, RpMaterial*, RwFrame*>(material, frame);
}

RpMaterial* RpMatFXMaterialSetEnvMapFrameBufferAlpha(RpMaterial* material, RwBool useFrameBufferAlpha) {
    return plugin::CallAndReturn<RpMaterial*, 0x812610, RpMaterial*, RwBool>(material, useFrameBufferAlpha);
}

RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, RwReal coef) {
    return plugin::CallAndReturn<RpMaterial*, 0x812680, RpMaterial*, RwReal>(material, coef);
}

RwTexture* RpMatFXMaterialGetEnvMapTexture(const RpMaterial* material) {
    return plugin::CallAndReturn<RwTexture*, 0x8126F0, const RpMaterial*>(material);
}

RwFrame* RpMatFXMaterialGetEnvMapFrame(const RpMaterial* material) {
    return plugin::CallAndReturn<RwFrame*, 0x812740, const RpMaterial*>(material);
}

RwBool RpMatFXMaterialGetEnvMapFrameBufferAlpha(const RpMaterial* material) {
    return plugin::CallAndReturn<RwBool, 0x812790, const RpMaterial*>(material);
}

RwReal RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial* material) {
    return plugin::CallAndReturn<RwReal, 0x8127E0, const RpMaterial*>(material);
}

RpMaterial* RpMatFXMaterialSetDualTexture(RpMaterial* material, RwTexture* texture) {
    return plugin::CallAndReturn<RpMaterial*, 0x812830, RpMaterial*, RwTexture*>(material, texture);
}

RpMaterial* RpMatFXMaterialSetDualBlendModes(RpMaterial* material, RwBlendFunction srcBlendMode, RwBlendFunction dstBlendMode) {
    return plugin::CallAndReturn<RpMaterial*, 0x8128C0, RpMaterial*, RwBlendFunction, RwBlendFunction>(material, srcBlendMode, dstBlendMode);
}

RwTexture* RpMatFXMaterialGetDualTexture(const RpMaterial* material) {
    return plugin::CallAndReturn<RwTexture*, 0x812930, const RpMaterial*>(material);
}

const RpMaterial* RpMatFXMaterialGetDualBlendModes(const RpMaterial* material, RwBlendFunction* srcBlendMode, RwBlendFunction* dstBlendMode) {
    return plugin::CallAndReturn<const RpMaterial*, 0x812980, const RpMaterial*, RwBlendFunction*, RwBlendFunction*>(material, srcBlendMode, dstBlendMode);
}

RpMaterial* RpMatFXMaterialSetUVTransformMatrices(RpMaterial* material, RwMatrix* baseTransform, RwMatrix* dualTransform) {
    return plugin::CallAndReturn<RpMaterial*, 0x8129E0, RpMaterial*, RwMatrix*, RwMatrix*>(material, baseTransform, dualTransform);
}

const RpMaterial* RpMatFXMaterialGetUVTransformMatrices(const RpMaterial* material, RwMatrix* baseTransform, RwMatrix* dualTransform) {
    return plugin::CallAndReturn<const RpMaterial*, 0x812A50, const RpMaterial*, RwMatrix*, RwMatrix*>(material, baseTransform, dualTransform);
}

RxPipeline* RpMatFXGetD3D9Pipeline(RpMatFXD3D9Pipeline d3d9Pipeline) {
    return plugin::CallAndReturn<RxPipeline*, 0x8162F0, RpMatFXD3D9Pipeline>(d3d9Pipeline);
}
