#include "StdInc.h"

#include "FxEmitterBP.h"
#include "FxEmitter.h"
#include "FxPrimBP.h"
#include "FxEmitterPrt.h"
#include "FxInfo.h"
#include "FxInfoManager.h"

#include "Particle.h"
#include "FxTools.h"
#include "Fx.h"
#include "WaterLevel.h"

// `rand()` scaled into `[0, 1)` (The game uses MSVC's `rand()`, which returns `[0, 32767]`)
static float FxRand01() {
    return (float)CGeneral::GetRandomNumber() / 32768.0f;
}

// `blendFuncs` as used by the game (indexed by `FxPrimBP_c::m_nSrcBlendId`/`m_nDstBlendId`)
static constexpr RwBlendFunction g_BlendFunctions[] = {
    rwBLENDZERO,      rwBLENDONE,          rwBLENDSRCCOLOR,  rwBLENDINVSRCCOLOR,  rwBLENDSRCALPHA, rwBLENDINVSRCALPHA,
    rwBLENDDESTALPHA, rwBLENDINVDESTALPHA, rwBLENDDESTCOLOR, rwBLENDINVDESTCOLOR, rwBLENDSRCALPHASAT
};

// Emits a textured quad built from the given billboard basis.
static void RenderParticleQuad(
    const CVector& pos,
    const CVector& rightVec,
    const CVector& upVec,
    float          edgeL,
    float          edgeR,
    float          edgeB,
    float          edgeT,
    float          sizeX,
    float          sizeY,
    int32          red,
    int32          green,
    int32          blue,
    int32          alpha
) {
    const CVector p0 = pos + rightVec * (edgeL * sizeX) + upVec * (edgeT * sizeY);
    const CVector p1 = pos + rightVec * (edgeR * sizeX) + upVec * (edgeB * sizeY);
    const CVector p2 = pos + rightVec * (edgeR * sizeX) + upVec * (edgeT * sizeY);
    const CVector p3 = pos + rightVec * (edgeL * sizeX) + upVec * (edgeB * sizeY);

    RenderAddTri_(
        p0.x, p0.y, p0.z,
        p1.x, p1.y, p1.z,
        p2.x, p2.y, p2.z,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        red, green, blue, alpha,
        red, green, blue, alpha,
        red, green, blue, alpha
    );

    RenderAddTri_(
        p1.x, p1.y, p1.z,
        p0.x, p0.y, p0.z,
        p3.x, p3.y, p3.z,
        1.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
        red, green, blue, alpha,
        red, green, blue, alpha,
        red, green, blue, alpha
    );
}

// Picks the raster of the texture slot selected by the render info, falling back to the first one.
static RwRaster* GetParticleRaster(const std::array<RwTexture*, 4>& textures, bool hasAnimTextures, int8 texId) {
    if (hasAnimTextures) {
        const int32 index = texId - 1;
        if (index >= 1 && index <= 3 && textures[index]) {
            return textures[index]->raster;
        }
    }
    return textures[0]->raster;
}

void FxEmitterBP_c::InjectHooks() {
    RH_ScopedVirtualClass(FxEmitterBP_c, 0x85A788, 7);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(Constructor, 0x4A18D0);
    RH_ScopedInstall(RenderHeatHaze, 0x4A1940);
    RH_ScopedInstall(UpdateParticle, 0x4A21D0);
    RH_ScopedVMTInstall(CreateInstance, 0x4A2B40, {.Reversed = false}); // bad
    RH_ScopedVMTInstall(Update, 0x4A2BC0, {.Reversed = false});
    RH_ScopedVMTInstall(Load, 0x5C25F0, {.Reversed = false});
    RH_ScopedVMTInstall(LoadTextures, 0x5C0A30, {.Reversed = true});
    RH_ScopedVMTInstall(Render, 0x4A2C40);
    RH_ScopedVMTInstall(FreePrtFromPrim, 0x4A2510);
}

// 0x4A18D0
FxEmitterBP_c::FxEmitterBP_c() : FxPrimBP_c() {
    m_Type = 0;
}

// 0x4A1940
void FxEmitterBP_c::RenderHeatHaze(RwCamera* camera, uint32 txdHashKey, float brightness) {
    if (!m_Particles.GetNumItems())
        return;

    RwRaster* raster = m_apTextures[0]->raster;
    if (!m_Particles.GetNumItems())
        return;

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(1));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDONE));

    RenderBegin(raster, nullptr, rwIM3D_VERTEXUV);

    RwRaster* currRaster = raster;

    const RwMatrix& camFrameMat = *RwFrameGetMatrix(RwCameraGetFrame(camera));
    const CVector   camFramePos = camFrameMat.pos;

    for (auto* prt = (FxEmitterPrt_c*)m_Particles.GetHead(); prt; prt = (FxEmitterPrt_c*)m_Particles.GetNext(prt)) {
        CVector pos;
        if (prt->m_bLocalToSystem) {
            RwMatrix* mat = g_fxMan.FxRwMatrixCreate();
            prt->m_System->GetCompositeMatrix(mat);
            RwV3dTransformPoint(&pos, &prt->m_Pos, mat);
            g_fxMan.FxRwMatrixDestroy(mat);
        } else {
            pos = prt->m_Pos;
        }

        RenderInfo_t renderInfo{};
        m_FxInfoManager.ProcessRenderInfo(
            prt->m_System->m_fCurrentTime,
            prt->m_fCurrentLife / prt->m_fTotalLife,
            0.0f,
            prt->m_System->m_SystemBP->m_fLength,
            false,
            &renderInfo
        );

        CVector right, up, at;
        if (!renderInfo.m_bHasDir) {
            const RwMatrix& src = renderInfo.m_bIsFlat ? renderInfo.m_FlatMatrix : camFrameMat;
            right = src.right;
            up    = src.up;
            at    = src.at;
        } else {
            if (renderInfo.m_bUseVel) {
                up = prt->m_Velocity;
                if (up.x == 0.0f && up.y == 0.0f && up.z == 0.0f) {
                    up.z = 1.0f;
                }
            } else {
                up = renderInfo.m_Direction;
            }
            up.Normalise();

            CVector toPrt = pos - camFramePos;
            toPrt.Normalise();

            right = up.Cross(toPrt);
            at    = up.Cross(right);
        }

        if (prt->m_RotZ != -1) {
            prt->m_CurrentRotation = 2.0f * (float)prt->m_RotZ;
        }
        float& rotation = prt->m_CurrentRotation;
        while (rotation < 0.0f) {
            rotation += 360.0f;
        }
        while (rotation >= 360.0f) {
            rotation -= 360.0f;
        }
        if (rotation > 0.0f) {
            RotateVecAboutVec(right, right, at, rotation * 0.017453292f);
            up = at.Cross(right);
        }

        float sizeX = renderInfo.m_fSizeX + ((float)prt->m_RandR * (1.0f / 255.0f) - 0.5f) * renderInfo.m_fSizeXBias;
        float sizeY = renderInfo.m_fSizeY + ((float)prt->m_RandG * (1.0f / 255.0f) - 0.5f) * renderInfo.m_fSizeYBias;

        const float multSize = (float)prt->m_MultSize;
        if (multSize != 1.0f) {
            sizeX *= multSize;
            sizeY *= multSize;
        }

        constexpr int32 red = 0, green = 0, blue = 0;
        const int32     alpha = renderInfo.m_Color1.alpha;

        raster = GetParticleRaster(m_apTextures, renderInfo.m_bHasAnimTextures, renderInfo.m_nCurrentTexId);
        if (raster != currRaster) {
            RenderEnd();
            currRaster = raster;
            RenderBegin(raster, nullptr, rwIM3D_VERTEXUV);
        }

        RenderParticleQuad(
            pos, right, up,
            renderInfo.m_fSpriteLeft, renderInfo.m_fSpriteRight,
            renderInfo.m_fSpriteBottom, renderInfo.m_fSpriteTop,
            sizeX, sizeY,
            red, green, blue, alpha
        );
    }

    RenderEnd();
}

// 0x4A21D0
bool FxEmitterBP_c::UpdateParticle(float deltaTime, FxEmitterPrt_c* emitter) {
    FxSystem_c* system = emitter->m_System;

    const float step = (float)system->m_nTimeMult * 0.001f * deltaTime;

    emitter->m_fCurrentLife += step;
    if (emitter->m_fTotalLife <= emitter->m_fCurrentLife)
        return true;

    emitter->m_Pos += emitter->m_Velocity * step;

    MovementInfo_t movementInfo{};
    movementInfo.m_Pos = emitter->m_Pos;
    movementInfo.m_Vel = emitter->m_Velocity;

    m_FxInfoManager.ProcessMovementInfo(
        system->m_fCurrentTime,
        emitter->m_fCurrentLife / emitter->m_fTotalLife,
        step,
        system->m_SystemBP->m_fLength,
        false,
        &movementInfo
    );

    emitter->m_Pos      = movementInfo.m_Pos;
    emitter->m_Velocity = movementInfo.m_Vel;

    if (movementInfo.m_bHasFloatInfo || movementInfo.m_bHasUnderwaterInfo) {
        float      waterLevel{};
        const bool inWater = CWaterLevel::GetWaterLevel(emitter->m_Pos, waterLevel, true, nullptr);

        if (movementInfo.m_bHasFloatInfo && inWater && emitter->m_Pos.z < waterLevel) {
            emitter->m_Pos.z = waterLevel;
        }

        if (movementInfo.m_bHasUnderwaterInfo) {
            if (!inWater)
                return true;
            if (waterLevel < emitter->m_Pos.z)
                return true;
        }
    }

    constexpr float kInv255  = 1.0f / 255.0f;
    constexpr float kInv128  = 1.0f / 128.0f;
    const float     rot0     = movementInfo.m_Rot[0];
    const float     rot1     = movementInfo.m_Rot[1];
    const float     rot2     = movementInfo.m_Rot[2];
    const float     rot3     = movementInfo.m_Rot[3];
    const float     multRot  = (float)emitter->m_MultRot;

    if ((rot0 <= 0.0f && rot1 <= 0.0f) || (rot2 <= 0.0f && rot3 <= 0.0f)) {
        if (rot0 > 0.0f || rot1 > 0.0f) {
            emitter->m_CurrentRotation += ((rot1 - rot0) * (float)emitter->m_RandR * kInv255 + rot0) * multRot * step * kInv255;
            return false;
        }
        if (rot2 <= 0.0f && rot3 <= 0.0f)
            return false;

        const float delta = ((rot3 - rot2) * (float)emitter->m_RandR * kInv255 + rot2) * multRot;
        emitter->m_CurrentRotation -= delta * step * kInv255;
        return false;
    }

    const uint8 randR = emitter->m_RandR;
    if (randR < 128) {
        emitter->m_CurrentRotation += ((rot1 - rot0) * (float)randR * kInv128 + rot0) * multRot * step * kInv255;
        return false;
    }

    const float delta = ((rot3 - rot2) * ((float)randR - 128.0f) * kInv128 + rot2) * multRot;
    emitter->m_CurrentRotation -= delta * step * kInv255;
    return false;
}

// 0x4A2B40


FxPrim_c* FxEmitterBP_c::CreateInstance() {
    return new FxEmitter_c();
}

// 0x4A2BC0


void FxEmitterBP_c::Update(float deltaTime) {
    for (auto it = m_Particles.GetHead(); it; it = m_Particles.GetNext(it)) {
        if (it->m_System->m_nKillStatus == eFxSystemKillStatus::FX_3) {
            it->m_System->m_nKillStatus = eFxSystemKillStatus::FX_KILLED;
        }

        // wrong casts or smth
        if (it->m_System->m_nPlayStatus != eFxSystemPlayStatus::T2 && UpdateParticle(deltaTime, reinterpret_cast<FxEmitterPrt_c*>(it))) {
            m_Particles.RemoveItem(it);
            g_fxMan.ReturnParticle(reinterpret_cast<FxEmitterPrt_c*>(it));
        }
    }
}

// 0x5C25F0


bool FxEmitterBP_c::Load(FILESTREAM file, int32 version, FxName32_t* textureNames) {
    FxPrimBP_c::Load(file, version, textureNames);

    m_nLodStart = uint16(ReadField<float>(file, "LODSTART:") * 64.0f);
    m_nLodEnd   = uint16(ReadField<float>(file, "LODEND:") * 64.0f);

    return true;
}

// 0x5C0A30


bool FxEmitterBP_c::LoadTextures(FxName32_t* textureNames, int32 version) {
    assert(textureNames);

    const auto LoadTexture = [&](auto ind) -> RwTexture* {
        char mask[64];
        sprintf(mask, "%sm", textureNames[ind]);

        auto* texture = RwTextureRead(textureNames[ind], mask);
        return texture ? texture : RwTextureRead(textureNames[ind], nullptr);
    };

    const auto LoadTextureIfExists = [=](auto ind) -> RwTexture* {
        assert(&textureNames[ind]);
        return strncmp(textureNames[ind], "NULL", 5u) != 0 ? LoadTexture(ind) : nullptr;
    };

    m_apTextures[0] = LoadTexture(0);

    if (version > 101) {
        m_apTextures[1] = LoadTextureIfExists(1);
        m_apTextures[2] = LoadTextureIfExists(2);
        m_apTextures[3] = LoadTextureIfExists(3);
    }

    return true;
}

// 0x4A2C40
void FxEmitterBP_c::Render(RwCamera* camera, uint32 txdHashKey, float brightness, bool doHeatHaze) {
    if (doHeatHaze) {
        if (m_FxInfoManager.m_bHasHeatHazeParticleEmitter) {
            RenderHeatHaze(camera, txdHashKey, brightness);
        }
        return;
    }

    if (IsFxInfoPresent(FX_INFO_HEATHAZE_DATA)) {
        if (m_Particles.GetNumItems() > 0) {
            g_fxMan.m_bHeatHazeEnabled = true;
        }
        return;
    }

    if (!m_Particles.GetNumItems())
        return;

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(m_bAlphaOn));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(g_BlendFunctions[m_bAlphaOn ? m_nSrcBlendId : 1]));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(g_BlendFunctions[m_bAlphaOn ? m_nDstBlendId : 0]));

    RwRaster* raster     = m_apTextures[0]->raster;
    RwRaster* currRaster = raster;
    RenderBegin(raster, nullptr, rwIM3D_VERTEXUV);

    const RwMatrix& camFrameMat = *RwFrameGetMatrix(RwCameraGetFrame(camera));
    const CVector   camFramePos = camFrameMat.pos;

    for (auto* prt = (FxEmitterPrt_c*)m_Particles.GetHead(); prt; prt = (FxEmitterPrt_c*)m_Particles.GetNext(prt)) {
        CVector pos;
        if (prt->m_bLocalToSystem) {
            RwMatrix* mat = g_fxMan.FxRwMatrixCreate();
            prt->m_System->GetCompositeMatrix(mat);
            RwV3dTransformPoint(&pos, &prt->m_Pos, mat);
            g_fxMan.FxRwMatrixDestroy(mat);
        } else {
            pos = prt->m_Pos;
        }

        RenderInfo_t renderInfo{};
        m_FxInfoManager.ProcessRenderInfo(
            prt->m_System->m_fCurrentTime,
            prt->m_fCurrentLife / prt->m_fTotalLife,
            0.0f,
            prt->m_System->m_SystemBP->m_fLength,
            false,
            &renderInfo
        );

        // Smoke emitters spawn their smoke particles from the render pass
        if (renderInfo.m_SmokeType >= 0) {
            const float jitter = renderInfo.m_SmokeBrightness * (2.0f * FxRand01() - 1.0f) / 255.0f;

            FxPrtMult_c prtMult;
            prtMult.m_Color.red   = std::clamp(renderInfo.m_SmokeColor.red   / 255.0f + jitter, 0.0f, 1.0f);
            prtMult.m_Color.green = std::clamp(renderInfo.m_SmokeColor.green / 255.0f + jitter, 0.0f, 1.0f);
            prtMult.m_Color.blue  = std::clamp(renderInfo.m_SmokeColor.blue  / 255.0f + jitter, 0.0f, 1.0f);
            prtMult.m_Color.alpha = renderInfo.m_SmokeColor.alpha / 255.0f;
            prtMult.m_fSize       = renderInfo.m_SmokeSize;
            prtMult.m_Rot         = 1.0f;
            prtMult.m_fLife       = renderInfo.m_SmokeLife;

            g_fx.m_SmokeHuge->AddParticle(prt->m_Pos, CVector{}, 0.0f, prtMult, -1.0f, 1.2f, 0.6f, false);
        }

        const int32 mode = renderInfo.m_nTrailScreenMode;

        CVector right, up, at;
        CVector trail; // Unnormalized trail vector (only used when `mode > 0`)

        if (mode < 1) {
            if (!renderInfo.m_bHasDir) {
                const RwMatrix& src = renderInfo.m_bIsFlat ? renderInfo.m_FlatMatrix : camFrameMat;
                right = src.right;
                up    = src.up;
                at    = src.at;
            } else {
                if (renderInfo.m_bUseVel) {
                    up = prt->m_Velocity;
                    if (up.x == 0.0f && up.y == 0.0f && up.z == 0.0f) {
                        up.z = 1.0f;
                    }
                } else {
                    up = renderInfo.m_Direction;
                }
                up.Normalise();

                CVector toPrt = pos - camFramePos;
                toPrt.Normalise();

                right = up.Cross(toPrt);
                at    = up.Cross(right);
            }
        } else {
            if (mode == 2) {
                const float   trailDt  = CTimer::GetTimeStep() * 0.02f;
                const CVector camPos   = TheCamera.GetPosition();
                const CVector prevPos  = pos - prt->m_Velocity * trailDt;
                trail.x = ((pos.x - camFramePos.x) - (prevPos.x - camPos.x)) * renderInfo.m_fTrailTime;
                trail.y = ((pos.y - camFramePos.y) - (prevPos.y - camPos.y)) * renderInfo.m_fTrailTime;
                trail.z = ((pos.z - camFramePos.z) - (prevPos.z - camPos.z)) * renderInfo.m_fTrailTime;
            } else {
                trail = prt->m_Velocity * renderInfo.m_fTrailTime;
            }

            up = trail;
            if (up.x == 0.0f && up.y == 0.0f && up.z == 0.0f) {
                up.z = 1.0f;
            } else {
                up.Normalise();
            }

            CVector toPrt = pos - camFramePos;
            toPrt.Normalise();

            right = up.Cross(toPrt);
            at    = up.Cross(right);
        }

        if (prt->m_RotZ != -1) {
            prt->m_CurrentRotation = 2.0f * (float)prt->m_RotZ;
        }
        float& rotation = prt->m_CurrentRotation;
        while (rotation < 0.0f) {
            rotation += 360.0f;
        }
        while (rotation >= 360.0f) {
            rotation -= 360.0f;
        }
        if (rotation > 0.0f) {
            RotateVecAboutVec(right, right, at, rotation * 0.017453292f);
            up = at.Cross(right);
        }

        float sizeX = renderInfo.m_fSizeX + ((float)prt->m_RandR * (1.0f / 255.0f) - 0.5f) * renderInfo.m_fSizeXBias;
        float sizeY = renderInfo.m_fSizeY + ((float)prt->m_RandG * (1.0f / 255.0f) - 0.5f) * renderInfo.m_fSizeYBias;

        const float multSize = (float)prt->m_MultSize;
        if (multSize != 1.0f) {
            sizeX *= multSize;
            sizeY *= multSize;
        }

        float red   = (float)renderInfo.m_Color1.red;
        float green = (float)renderInfo.m_Color1.green;
        float blue  = (float)renderInfo.m_Color1.blue;
        float alpha = (float)renderInfo.m_Color1.alpha;

        if (renderInfo.m_nColorType == ERenderColorType::RANGE) {
            red   += ((float)renderInfo.m_Color2.red   - red)   * (float)prt->m_RandR / 255.0f;
            green += ((float)renderInfo.m_Color2.green - green) * (float)prt->m_RandG / 255.0f;
            blue  += ((float)renderInfo.m_Color2.blue  - blue)  * (float)prt->m_RandB / 255.0f;
        } else if (renderInfo.m_nColorType == ERenderColorType::BRIGHT) {
            const float add = ((float)prt->m_RandR * (1.0f / 128.0f) - 1.0f) * (float)renderInfo.m_Color2.alpha;
            red   = std::clamp(red   + add, 0.0f, 255.0f);
            green = std::clamp(green + add, 0.0f, 255.0f);
            blue  = std::clamp(blue  + add, 0.0f, 255.0f);
        }

        if (prt->m_MultColor.r != 0xFF) {
            red *= (float)prt->m_MultColor.r * (1.0f / 255.0f);
        }
        if (prt->m_MultColor.g != 0xFF) {
            green *= (float)prt->m_MultColor.g * (1.0f / 255.0f);
        }
        if (prt->m_MultColor.b != 0xFF) {
            blue *= (float)prt->m_MultColor.b * (1.0f / 255.0f);
        }
        if (prt->m_MultColor.a != 0xFF) {
            alpha *= (float)prt->m_MultColor.a * (1.0f / 255.0f);
        }

        if (!renderInfo.m_bSelfLit) {
            const float brightnessMult = (float)prt->m_Brightness;
            if (brightnessMult <= 1.0f) {
                red   *= brightnessMult;
                green *= brightnessMult;
                blue  *= brightnessMult;
            }
        }

        const int32 redI   = (int32)red;
        const int32 greenI = (int32)green;
        const int32 blueI  = (int32)blue;
        const int32 alphaI = (int32)alpha;

        raster = GetParticleRaster(m_apTextures, renderInfo.m_bHasAnimTextures, renderInfo.m_nCurrentTexId);
        if (raster != currRaster) {
            RenderEnd();
            currRaster = raster;
            RenderBegin(raster, nullptr, rwIM3D_VERTEXUV);
        }

        float spriteTop    = renderInfo.m_fSpriteTop;
        float spriteBottom = renderInfo.m_fSpriteBottom;
        if (mode > 0) {
            spriteTop    = trail.Magnitude();
            spriteBottom = 0.0f;
        }

        RenderParticleQuad(
            pos, right, up,
            renderInfo.m_fSpriteLeft, renderInfo.m_fSpriteRight,
            spriteBottom, spriteTop,
            sizeX, sizeY,
            redI, greenI, blueI, alphaI
        );
    }

    RenderEnd();
}

// 0x4A2510
bool FxEmitterBP_c::FreePrtFromPrim(FxSystem_c* system) {
    for (auto* prt = m_Particles.GetHead(); prt; prt = m_Particles.GetNext(prt)) {
        if (prt->m_System == system) {
            m_Particles.RemoveItem(prt);
            g_fxMan.ReturnParticle(prt->AsFxEmitterPrt());
            return true;
        }
    }
    return false;
}

// todo: eFxInfo
// 0x4A24D0
bool FxEmitterBP_c::IsFxInfoPresent(eFxInfoType type) const {
    if (m_FxInfoManager.m_nNumInfos <= 0)
        return false;

    for (auto& info : m_FxInfoManager.GetInfos()) {
        if (info->m_nType == type) {
            return true;
        }
    }
    return false;
}
