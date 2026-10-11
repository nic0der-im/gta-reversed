#include "StdInc.h"

#include "CarFXRenderer.h"
#include "CustomCarEnvMapPipeline.h"
#include "TxdStore.h"
#include "ClothesBuilder.h"

void CCarFXRenderer::InjectHooks() {
    RH_ScopedClass(CCarFXRenderer);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(RegisterPlugins, 0x5D5B00);
    RH_ScopedInstall(Initialise, 0x5D5AC0);
    RH_ScopedInstall(InitialiseDirtTexture, 0x5D5BC0);
    RH_ScopedInstall(Shutdown, 0x5D5AD0);
    RH_ScopedInstall(PreRenderUpdate, 0x5D5B10);
    RH_ScopedInstall(IsCCPCPipelineAttached, 0x5D5B80);
    RH_ScopedInstall(CustomCarPipeAtomicSetup, 0x5D5B20);
    RH_ScopedInstall(CustomCarPipeClumpSetup, 0x5D5B40);
    RH_ScopedInstall(SetCustomFXAtomicRenderPipelinesVMICB, 0x5D5B60);
    RH_ScopedInstall(SetFxEnvMapLightMult, 0x5D5BA0);
}

// 0x5D5B00
bool CCarFXRenderer::RegisterPlugins() {
    return CCustomCarEnvMapPipeline::RegisterPlugin() != 0;
}

// 0x5D5AC0
bool CCarFXRenderer::Initialise() {
    return CCustomCarEnvMapPipeline::CreatePipe() != 0;
}

// 0x5D5BC0
void CCarFXRenderer::InitialiseDirtTexture() {
    CTxdStore::ScopedTXDSlot txd{"vehicle"};

    auto* const grunge = RwTextureRead("vehiclegrunge256", nullptr);
    grunge->filterAddressing = rwFILTERLINEAR; // 2 (0x50)

    const auto width  = grunge->raster->width;
    const auto height = grunge->raster->height;

    // 16 darkened copies: level 0 = flat white, level 15 = original grunge texture
    for (int32 level = 0; level < NUM_DIRT_TEXTURES; level++) {
        auto* const tex = CClothesBuilder::CopyTexture(grunge);
        ms_aDirtTextures[level] = tex;
        RwTextureSetName(tex, "vehiclegrunge256");

        auto* pixels = RwRasterLock(tex->raster, 0, rwRASTERLOCKREAD | rwRASTERLOCKWRITE); // 3
        const auto base = (uint8)((0xFF0 - level * 0xFF) / 16);
        for (auto i = 0; i < width * height; i++, pixels += 4) { // NOTE: raster stride is ignored, as in the original
            for (auto c = 0; c < 3; c++) { // RGB, alpha untouched
                pixels[c] = (uint8)((uint32)pixels[c] * level / 16 + base);
            }
        }
        RwRasterUnlock(tex->raster);
    }
}

// 0x5D5AD0
void CCarFXRenderer::Shutdown() {
    for (auto& texture : ms_aDirtTextures) {
        RwTextureDestroy(texture);
    }
    CCustomCarEnvMapPipeline::DestroyPipe();
}

// 0x5D5B10
void CCarFXRenderer::PreRenderUpdate() {
    CCustomCarEnvMapPipeline::PreRenderUpdate();
}

// 0x5D5B80
bool CCarFXRenderer::IsCCPCPipelineAttached(RpAtomic* atomic) {
    return GetPipelineID(atomic) == 0x53F2009A;
}

// 0x5D5B20
void CCarFXRenderer::CustomCarPipeAtomicSetup(RpAtomic* atomic) {
    CCustomCarEnvMapPipeline::CustomPipeAtomicSetup(atomic);
}

// 0x5D5B40
void CCarFXRenderer::CustomCarPipeClumpSetup(RpClump* clump) {
    RpClumpForAllAtomics(clump, SetCustomFXAtomicRenderPipelinesVMICB, nullptr);
}

// 0x5D5B60
RpAtomic* CCarFXRenderer::SetCustomFXAtomicRenderPipelinesVMICB(RpAtomic* atomic, void* data) {
    CCustomCarEnvMapPipeline::CustomPipeAtomicSetup(atomic);
    return atomic;
}

// 0x5D5BA0
void CCarFXRenderer::SetFxEnvMapLightMult(float multiplier) {
    gSpecIntensity = multiplier;
}
