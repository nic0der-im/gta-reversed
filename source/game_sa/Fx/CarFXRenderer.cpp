#include "StdInc.h"

#include "CarFXRenderer.h"
#include "ClothesBuilder.h"
#include "CustomCarEnvMapPipeline.h"
#include "TxdStore.h"

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
bool CCarFXRenderer::InitialiseDirtTexture() {
    const auto txdSlot = CTxdStore::FindTxdSlot("vehicle");
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(txdSlot);

    const auto srcTex = RwTextureRead("vehiclegrunge256", nullptr);
    // Inlined `RwTextureSetFilterMode(srcTex, rwFILTERLINEAR)` (only the low byte is written)
    srcTex->filterAddressing = (srcTex->filterAddressing & ~0xFFu) | rwFILTERLINEAR;

    const auto srcRaster = RwTextureGetRaster(srcTex);
    const auto width     = RwRasterGetWidth(srcRaster);
    const auto height    = RwRasterGetHeight(srcRaster);

    for (auto i = 0; i < NUM_DIRT_TEXTURES; i++) {
        const auto tex = CClothesBuilder::CopyTexture(srcTex);
        ms_aDirtTextures[i] = tex;
        RwTextureSetName(tex, "vehiclegrunge256");

        const auto raster = RwTextureGetRaster(tex);
        const auto pixels = RwRasterLock(raster, 0, rwRASTERLOCKREAD | rwRASTERLOCKWRITE);
        if (!pixels) {
            NOTSA_LOG_WARN("Dirt texture {}: could not lock the raster", i);
            continue;
        }

        // The executable walks the rows with `width * 4` and always touches three colour bytes
        // per pixel, i.e. it assumes a 32-bit raster (D3D9 has no 24-bit texture formats).
        // Native rasters keep the format the GL pipeline loaded (`vehiclegrunge256` is 24-bit),
        // so the pitch and the pixel size are taken from the raster here - otherwise the fill
        // wrote 4 bytes per pixel over 3-byte rows and ran past the allocation.
        const auto stride         = RwRasterGetStride(raster);
        const auto bytesPerPixel  = RwRasterGetDepth(raster) / 8;
        if (bytesPerPixel < 3) {
            NOTSA_LOG_WARN("Dirt texture {}: unsupported raster depth {} (stride {})", i, RwRasterGetDepth(raster), stride);
            RwRasterUnlock(raster);
            continue;
        }

        const auto add = static_cast<uint8>(255 - 16 * i); // Pixel value at 0 (index 0 -> flat white)
        for (auto y = 0; y < height; y++) {
            const auto row = pixels + y * stride;
            for (auto x = 0; x < width; x++) {
                for (auto c = 0; c < 3; c++) { // Only the colour bytes, the 4th one (if any) is left untouched
                    auto& channel = row[x * bytesPerPixel + c];
                    channel = static_cast<uint8>(channel * i / 16 + add);
                }
            }
        }
        RwRasterUnlock(raster);
    }

    CTxdStore::PopCurrentTxd();
    return true;
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
