#include "StdInc.h"
#include "WaterLevel.h"
#include "PostEffects.h"
#include <sstream>

#define TRIANGLE_ARGS_OUT X1, Y1, P1, X2, Y2, P2, X3, Y3, P3

//! Scratch vector that receives the wave normal from `CalculateWavesOnlyForCoordinate`
static auto& ms_vecWaterWaveNormal = StaticRef<CVector>(0xC278D4);

//! When non-zero the high-detail rectangle mesh's indices aren't generated (so nothing is drawn)
static auto& ms_bDontGenerateWaterRectMesh = StaticRef<bool>(0xC278E0); // TODO(re): original name unknown

//! Unidentified counter bumped right before every Im3D flush (0x541330, `LittleTest`)
static auto& ms_nWaterFlushCount = StaticRef<uint32>(0xB73708); // TODO(re): original name unknown

//! Floats in `[0, 255]` the water tint is randomized from each frame (see `RenderWater`)
static auto& ms_fWaterColorRandomR = StaticRef<float>(0xB7C508); // TODO(re): producer not identified
static auto& ms_fWaterColorRandomG = StaticRef<float>(0xB7C50C);
static auto& ms_fWaterColorRandomB = StaticRef<float>(0xB7C510);
static auto& ms_fWaterColorRandomA = StaticRef<float>(0xB7C514);

//! Per-vertex color components kept between the two layers of a high-detail patch
static auto& ms_TempVertexColorR = StaticRef<std::array<uint8, 2048>>(0xC20968);
static auto& ms_TempVertexColorG = StaticRef<std::array<uint8, 2048>>(0xC20168);
static auto& ms_TempVertexColorB = StaticRef<std::array<uint8, 2048>>(0xC1F968);
static auto& ms_nTempVertexColorIdx = StaticRef<int32>(0xC1F960);

void CWaterLevel::InjectHooks() {
    RH_ScopedClass(CWaterLevel);
    RH_ScopedCategoryGlobal();

    RH_ScopedGlobalInstall(WaterLevelInitialise, 0x6EAE80);
    RH_ScopedGlobalInstall(Shutdown, 0x6E59E0);
    RH_ScopedGlobalInstall(RenderWaterTriangle, 0x6EE240);
    RH_ScopedGlobalInstall(RenderFlatWaterTriangle_OneLayer, 0x6E8ED0);
    RH_ScopedGlobalInstall(RenderFlatWaterTriangle, 0x6EE080);
    RH_ScopedGlobalInstall(SplitWaterTriangleAlongXLine, 0x6ECF00);
    RH_ScopedGlobalInstall(SplitWaterTriangleAlongYLine, 0x6EE5A0);

    RH_ScopedGlobalInstall(RenderWaterRectangle, 0x6EC5D0);
    RH_ScopedGlobalInstall(RenderFlatWaterRectangle_OneLayer, 0x6E9940);
    RH_ScopedGlobalInstall(RenderFlatWaterRectangle, 0x6EBEC0);

    RH_ScopedGlobalInstall(SplitWaterRectangleAlongXLine, 0x6E73A0);
    RH_ScopedGlobalInstall(SplitWaterRectangleAlongYLine, 0x6ED6D0);

    RH_ScopedGlobalInstall(PreRenderWater, 0x6EB710);
    RH_ScopedGlobalInstall(MarkQuadsAndPolysToBeRendered, 0x6E5810);
    RH_ScopedGlobalInstall(ScanThroughBlocks, 0x6E6D10);
    RH_ScopedGlobalInstall(BlockHit, 0x6E6CA0);

    // This one doesn't seem to work properly for whatever reason
    // It works for some quads, not for others... But then it works for that one too if only it's loaded from the file (eg.: you delete all others)
    // no clue what is the issue
    // one quad that doesn't load can be seen from -1610, 168
    // it's at water.dat:252
    RH_ScopedGlobalInstall(AddWaterLevelQuad, 0x6E7EF0);
    RH_ScopedGlobalInstall(AddWaterLevelTriangle, 0x6E7D40);
    RH_ScopedGlobalInstall(AddWaterLevelVertex, 0x6E5A40);

    RH_ScopedGlobalInstall(RenderBoatWakes, 0x6ED9A0);
    RH_ScopedGlobalInstall(RenderWakeSegment, 0x6EA260);

    RH_ScopedOverloadedInstall(GetWaterLevel, "", 0x6EB690, bool(*)(float, float, float, float&, uint8, CVector*));
    RH_ScopedGlobalInstall(SetUpWaterFog, 0x6EA9F0);
    RH_ScopedGlobalInstall(FindNearestWaterAndItsFlow, 0x6E9D70);
    RH_ScopedGlobalInstall(GetWaterLevelNoWaves, 0x6E8580);
    RH_ScopedGlobalInstall(RenderWaterFog, 0x6E7760);
    RH_ScopedGlobalInstall(CalculateWavesOnlyForCoordinate, 0x6E6EF0);
    RH_ScopedGlobalInstall(RenderWater, 0x6EF650);
    RH_ScopedGlobalInstall(AddWaveToResult, 0x6E81E0);
    RH_ScopedGlobalInstall(SetCameraRange, 0x6E9C80);
    RH_ScopedInstall(RenderHighDetailWaterRectangle, 0x6EB810);
    RH_ScopedInstall(GetWaterDepth, 0x6EA960);
    RH_ScopedInstall(CalculateWavesOnlyForCoordinate2, 0x6E7210);
    RH_ScopedOverloadedInstall(RenderHighDetailWaterTriangle, "", 0x6EDDC0, void(*)(int32, int32, CRenPar, int32, int32, CRenPar, int32, int32, CRenPar));
    RH_ScopedInstall(FillQuadsAndTrianglesList, 0x6E7B30);

    // Previously missing, now reversed
    RH_ScopedInstall(AddToQuadsAndTrianglesList, 0x6E5750);
    RH_ScopedInstall(TestQuadToGetWaterLevel, 0x6E5BB0);
    RH_ScopedInstall(TestTriangleToGetWaterLevel, 0x6E5E90);
    RH_ScopedInstall(TestLineAgainstWater, 0x6E61B0);
    RH_ScopedInstall(GetGroundLevel, 0x6EA8A0);
    RH_ScopedInstall(RenderSeaBedSegment, 0x6E6870);
    RH_ScopedInstall(RenderDetailedSeaBedSegment, 0x6E6A10);
    RH_ScopedInstall(RenderAndEmptyRenderBuffer, 0x6E7680);
    RH_ScopedInstall(RenderHighDetailWaterRectangle_OneLayer, 0x6E91D0);
    RH_ScopedInstall(RenderHighDetailWaterTriangle_OneLayer, 0x6E8780);
}

// NOTSA
bool CWaterLevel::LoadDataFile() {
    const auto file = CFileMgr::OpenFile(m_nWaterConfiguration == 1 ? "DATA//water1.dat" : "DATA//water.dat", "r");

    const notsa::ScopeGuard autoCloser{ [&] { CFileMgr::CloseFile(file); } };

    uint32 nline{}, ntri{}, nquad{};
    for (;; nline++) {
        const auto line = CFileLoader::LoadLine(file);
        if (!line) {
            break;
        }
        std::stringstream liness{ line };

        auto nvertices{0u};

        struct {
            CVector   pos{};
            CVector2D flow{};
            float     bigWaves{}, smallWaves{};
        } vertices[4]{};

        // Helper function to read a vertex from the stream
        const auto ReadNextVertex = [&]() {
            const auto orgpos = liness.tellg();

            auto& vtx = vertices[nvertices];
            liness
                >> vtx.pos.x
                >> vtx.pos.y
                >> vtx.pos.z
                >> vtx.flow.x
                >> vtx.flow.y
                >> vtx.bigWaves
                >> vtx.smallWaves;

            if (liness.good()) {
                nvertices++;
                return true;
            } else {
                liness.clear(); // reset error flags
                liness.seekg(orgpos); // go back to before
                return false;
            }

        };

        // If can't read first vertex just ignore line
        if (!ReadNextVertex()) {
            continue;
        }

        // Read 2/3 more vertices
        while (ReadNextVertex() && nvertices < 4);

        // Check if we have enough vertices
        if (nvertices < 3) {
            NOTSA_LOG_DEBUG("[Warning]: Not enough vertices, got {}, expected 3 or 4. [Line: {}]", nvertices, nline);
            continue;
            //return false; // Just stop here, this parser is way too primitive to be able to recover from errors
        }

        // Optional flag after vertices
        uint32 flags{};
        liness >> flags;

        // I'm sorry, but don't blame me I HAD NO OTHER CHOICE!
        #define VertexUnpack(n) \
            (int32)vertices[n].pos.x, (int32)vertices[n].pos.y, \
            CRenPar{vertices[n].pos.z, vertices[n].bigWaves, vertices[n].smallWaves, (int8)(vertices[n].flow.x * 64.f), (int8)(vertices[n].flow.y * 64.f)}

        // Add quad/triangle
        if (nvertices == 4) {
            CWaterLevel::AddWaterLevelQuad(
                VertexUnpack(0),
                VertexUnpack(1),
                VertexUnpack(2),
                VertexUnpack(3),
                flags
            );
            nquad++;
        } else {
            CWaterLevel::AddWaterLevelTriangle(
                VertexUnpack(0),
                VertexUnpack(1),
                VertexUnpack(2),
                flags
            );
            ntri++;
        }
        #undef ArgUnpack
    }
    NOTSA_LOG_DEBUG("Successfully loaded! [Quads: {}; Tris: {}]", nquad, ntri);
    return true;
}

// NOTSA: Code @ 0x6EB5F4
void CWaterLevel::LoadTextures() {
    CTxdStore::PushCurrentTxd();    
    CTxdStore::SetCurrentTxd(CTxdStore::FindTxdSlot("particle"));

    const auto DoTex = [](auto& inOutTex, auto& outRaster, const char* name) {
        if (!inOutTex) {
            inOutTex = RwTextureRead(name, nullptr);
        }
        outRaster = RwTextureGetRaster(inOutTex);
    };
    DoTex(texWaterclear256, waterclear256Raster, "waterclear256");
    DoTex(texSeabd32,       seabd32Raster,       "seabd32"      );
    DoTex(texWaterwake,     waterwakeRaster,     "waterwake"    );

    CTxdStore::PopCurrentTxd();
}

// 0x6EAE80
void CWaterLevel::WaterLevelInitialise() {
    NumWaterTriangles = 0;
    NumWaterQuads = 0;
    NumWaterVertices = 0;
    NumWaterZonePolys = 0;
    
    (void)LoadDataFile();
    FillQuadsAndTrianglesList();

    LoadTextures();
}

// 0x6E59E0
void CWaterLevel::Shutdown() {
    // Unload Textures
    for (auto tex : { &texWaterclear256, &texSeabd32, &texWaterwake }) {
        if (*tex) {
            RwTextureDestroy(*tex);
            *tex = nullptr;
        }
    }
}

// 0x6E81E0
void CWaterLevel::AddWaveToResult(float x, float y, float* pfWaterLevel, float bigWavesAmpl, float smallWavesAmpl, CVector* pVecNormal)
{
    const float hx = x * 0.5f;
    const float hy = y * 0.5f;

    // Fractional parts within the 2x2 node cell
    const float fx = hx - std::floor(hx);
    const float fy = hy - std::floor(hy);

    const int32 ix = (int32)std::floor(hx);
    const int32 iy = (int32)std::floor(hy);

    float h00{}, h10{}, h01{};
    if (fx + fy < 1.0f) {
        CalculateWavesOnlyForCoordinate2(ix,     iy,     bigWavesAmpl, smallWavesAmpl, &h00);
        CalculateWavesOnlyForCoordinate2(ix + 2, iy,     bigWavesAmpl, smallWavesAmpl, &h10);
        CalculateWavesOnlyForCoordinate2(ix,     iy + 2, bigWavesAmpl, smallWavesAmpl, &h01);

        *pfWaterLevel = (h01 - h00) * fy + (h10 - h00) * fx + *pfWaterLevel + h00;

        if (pVecNormal) {
            *pVecNormal = CrossProduct(
                CVector{ 2.0f, 0.0f, h10 - h00 },
                CVector{ 0.0f, 2.0f, h01 - h00 }
            );
            pVecNormal->Normalise();
        }
        return;
    }

    float h11{};
    CalculateWavesOnlyForCoordinate2(ix + 2, iy + 2, bigWavesAmpl, smallWavesAmpl, &h11);
    CalculateWavesOnlyForCoordinate2(ix,     iy + 2, bigWavesAmpl, smallWavesAmpl, &h01);
    CalculateWavesOnlyForCoordinate2(ix + 2, iy,     bigWavesAmpl, smallWavesAmpl, &h10);

    *pfWaterLevel = (1.0f - fy) * (h10 - h11) + (1.0f - fx) * (h01 - h11) + *pfWaterLevel + h11;

    if (pVecNormal) {
        *pVecNormal = CrossProduct(
            CVector{ -2.0f,  0.0f, h01 - h11 },
            CVector{  0.0f, -2.0f, h10 - h11 }
        );
        pVecNormal->Normalise();
    }
}

// 0x6EE240
void CWaterLevel::RenderWaterTriangle(int32 X1, int32 Y1, CRenPar P1, int32 X2, int32 Y2, CRenPar P2, int32 X3, int32 Y3, CRenPar P3) {
    const auto [minX, maxX] = std::make_pair(X1, X2); // Assumes: Starting in top left vertex with clockwise order
    const auto [minY, maxY] = std::minmax(Y1, Y3);
    if (minX >= CameraRangeMaxX || maxX <= CameraRangeMinX || minY >= CameraRangeMaxY || maxY <= CameraRangeMinY) { // Lies outside (of camera) fully
        RenderFlatWaterTriangle(TRIANGLE_ARGS_OUT);
    } else if (minX < CameraRangeMinX || maxX > CameraRangeMaxX) { // Lies inside on X 
        SplitWaterTriangleAlongXLine(minX < CameraRangeMinX ? CameraRangeMinX : CameraRangeMaxX, TRIANGLE_ARGS_OUT);
    } else if (minY < CameraRangeMinY || maxY > CameraRangeMaxY) { // Lies inside of Y
        SplitWaterTriangleAlongYLine(minY < CameraRangeMinY ? CameraRangeMinY : CameraRangeMaxY, TRIANGLE_ARGS_OUT);
    } else { // Lies inside of camera fully
        RenderHighDetailWaterTriangle(TRIANGLE_ARGS_OUT);
    }
}

// NOTSA
auto CWaterLevel::GetWaterLayerTexInfo(int32 layer) -> WaterLayerTexInfo {
    switch (layer) {
    case 0: return { { TextureShiftFirstU,  TextureShiftFirstV  }, 25.0f };
    case 1: return { { TextureShiftSecondU, TextureShiftSecondV }, 12.5f };
    default: NOTSA_UNREACHABLE();
    }
}

// NOTSA
CRGBA CWaterLevel::GetWaterColorForRendering(CRGBA real, DebugWaterColor debug, int32 WaterLayer) {
    if (debug.active) {
        return debug.color;
    } else {
        real *= 0.577f; // AKA 1/sqrt3 OR E_CONST OR neither, but just a coincidence?
        real.a = WaterLayerAlpha[WaterLayer];
        return real;
    }
}

// notsa
auto CWaterLevel::GetTextureUV(int32 X1, int32 Y1, int32 Y3, int32 WaterLayer) -> TexUV {
    const auto txinfo = GetWaterLayerTexInfo(WaterLayer);
    const auto posUV  = CVector2D{ (float)X1, (float)Y1 } / txinfo.size + txinfo.shift;

    const auto CalcShift = [](float p, bool dir) {
        return p - std::floor(p) + (dir ? 7.f : -7.f);
    };

    return {
        .size      = txinfo.size,
        .pos       = posUV,
        .baseShift = CVector2D{
            CalcShift(posUV.x, false),
            CalcShift(posUV.y, Y3 - Y1 <= 0)
        }
    };
}

// 0x6E8ED0
void CWaterLevel::RenderFlatWaterTriangle_OneLayer(int32 X1, int32 Y1, CRenPar P1, int32 X2, int32 Y2, CRenPar P2, int32 X3, int32 Y3, CRenPar P3, int32 WaterLayer) {
    RenderBuffer::RenderIfDoesntFit(5, 3);

    // First(!) push indices
    RenderBuffer::PushIndices({ 0, 1, 2 }, true);

    // And push vertices into the buffer
    const auto PushVertex = [
        &,
        tex       = GetTextureUV(X1, Y1, Y3, WaterLayer),
        pos2DVtx1 = CVector2D{ (float)X1, (float)Y2 },
        color     = GetWaterColorForRendering(WaterColorTriangle, DebugWaterColors[DebugWaterColor::TRI], WaterLayer)
    ](int32 x, int32 y, CRenPar p) {
        const auto pos2DThis = CVector2D{ (float)x, (float)y };
        RenderBuffer::PushVertex(
            CVector{ pos2DThis, p.z },
            (pos2DThis - pos2DVtx1) / tex.size + tex.baseShift,
            color
        );
    };

    PushVertex(X1, Y1, P1);
    PushVertex(X2, Y2, P2);
    PushVertex(X3, Y3, P3);
}

// 0x6EE080
void CWaterLevel::RenderFlatWaterTriangle(int32 X1, int32 Y1, CRenPar P1, int32 X2, int32 Y2, CRenPar P2, int32 X3, int32 Y3, CRenPar P3) {
    if (bSplitBigPolys && X2 - X1 > BigPolySize) {
        SplitWaterTriangleAlongXLine((X1 + X2) / 2, X1, Y1, P1, X2, Y2, P2, X3, Y3, P3);
    } else {
        RenderFlatWaterTriangle_OneLayer(X1, Y1, P1, X2, Y2, P2, X3, Y3, P3, 0);
        RenderFlatWaterTriangle_OneLayer(X1, Y1, P1, X2, Y2, P2, X3, Y3, P3, 1);
    }
}

// 0x6EA260
void CWaterLevel::RenderWakeSegment(
    const CVector2D& vecA, const CVector2D& vecB,
    const CVector2D& vecC, const CVector2D& vecD,
    const float& widthA, const float& widthB,
    const float& alphaA, const float& alphaB,
    const float& wakeZ
) {
    constexpr auto  NUM_PARTS = 4;
    constexpr float ALPHA_MULTS[]{ 0.4f, 1.f, 0.2f, 1.f, 0.4f }; // 0x8D390C

    const auto angle      = (float)(CTimer::GetTimeInMS() % 4096) / (4096.f / (2.f * PI));
    const auto windRadius = CWeather::WindClipped * 0.4f + 0.2f;

    for (auto partIdx = 0; partIdx < NUM_PARTS; partIdx++) {
        RenderBuffer::RenderIfDoesntFit(6, 4);

        RenderBuffer::PushIndices({ 0, 2, 1, 0, 3, 2 }, true);

        const CVector2D corners[]{
            lerp(vecB, vecA, (float)(partIdx + 0) / (float)(NUM_PARTS)),
            lerp(vecB, vecA, (float)(partIdx + 1) / (float)(NUM_PARTS)),
            lerp(vecC, vecD, (float)(partIdx + 1) / (float)(NUM_PARTS)),
            lerp(vecC, vecD, (float)(partIdx + 0) / (float)(NUM_PARTS)),
        };

        CVector2D uvs[4]{};
        rng::transform(corners, uvs, [](const CVector2D& pos) -> CVector2D {
            return { pos.x / (float)(NUM_PARTS), pos.y / (float)(NUM_PARTS) };
        });
        rng::transform(uvs, uvs, // Isn't it beautiful?
            [
                minUV = CVector2D{
                    std::floor(rng::min(uvs, {}, &CVector2D::x).x),
                    std::floor(rng::min(uvs, {}, &CVector2D::y).y)
                }
            ](auto& uv) {
                return uv - minUV;
            }
        );

        const float alphas[]{
            alphaA * ALPHA_MULTS[partIdx + 0],
            alphaA * ALPHA_MULTS[partIdx + 1],
            alphaB * ALPHA_MULTS[partIdx + 1],
            alphaB * ALPHA_MULTS[partIdx + 0],
        };

        for (auto i = 0; i < 4; i++) {
            const auto CalcAngleOfPos = [&](float p) {
                p += 3072.f; // TODO: Magic number, but I think it's meaningless (as the integer part is discarded below)
                p /= 32.f;   // TODO: Magic number (maybe meaningful this time) 
                return p - std::floor(p); // Extract fractional part
            };
            const auto  z   = wakeZ + std::sin((CalcAngleOfPos(corners[i].x) + CalcAngleOfPos(corners[i].y)) * PI * 2.f + angle) * windRadius;
            const auto& rgb = WakeSegmentPartColors[i];
            RenderBuffer::PushVertex(
                CVector{ corners[i], z },
                uvs[i],
                { (uint8)(rgb.r * 255.f), (uint8)(rgb.g * 255.f), (uint8)(rgb.b * 255.f), (uint8)(alphas[i]) }
            );
        }
    }
}

// 0x6ED9A0
void CWaterLevel::RenderBoatWakes() {
    CBoat::RenderAllWakePointBoats();
}

// 0x6ECF00
void CWaterLevel::SplitWaterTriangleAlongXLine(int32 splitAtX, int32 X1, int32 Y1, CRenPar P1, int32 X2, int32 Y2, CRenPar P2, int32 X3, int32 Y3, CRenPar P3) {
    assert(Y1 == Y2); 

    // Ease of life
    const auto XS = splitAtX;

    const auto splitWidth = XS - X1;
    const auto triWidth   = X2 - X1;

    // Calculate position of split along Y axis
    const auto CalcSplitPosY = [&](int32 fromY, int32 toY) {
        return fromY + (toY - fromY) * splitWidth / triWidth;
    };

    // Interpolation value
    const auto t = (float)splitWidth / (float)triWidth;

    // New interpolations of RenPar's along a few segments
    const auto P12 = lerp(P1, P2, t);
    const auto P13 = lerp(P1, P3, t);
    const auto P23 = lerp(P2, P3, t);

    // Vertex 1 and 2 are always (top left), (top right)
    // Also the triangles always contain a 90deg corner at either the left or right side.

    if (X1 == X3) { // Vertex 3 => (bottom left)
        const auto YS = CalcSplitPosY(Y3, Y1);

        // Bottom
        RenderWaterTriangle(
            X1, YS, P12,
            XS, YS, P13,
            X3, Y3, P3
        );

        // Left
        RenderWaterRectangle(
            X1, XS,
            Y1, YS,
            P1, P12, P23, P13
        );

        // Right
        RenderWaterTriangle(
            XS, Y1, P12,
            X2, Y1, P2,
            XS, YS, P23
        );
    } else if (X2 == X3) { // Vertex 3 => (bottom right)
        const auto YS = CalcSplitPosY(Y1, Y3);

        // Left
        RenderWaterTriangle(
            X1, Y1, P1,
            XS, Y1, P12,
            XS, YS, P13
        );

        // Right
        RenderWaterRectangle(
            XS, X2,
            Y1, YS,
            P12, P2, P23, P12
        );

        // Bottom
        RenderWaterTriangle(
            XS, YS, P13,
            X2, YS, P23,
            X3, Y3, P3
        );
    } else {
        NOTSA_UNREACHABLE("Triangle has no 90deg corner => Very bad");
    }
}

// 0x6EE5A0
void CWaterLevel::SplitWaterTriangleAlongYLine(int32 splitAtY, int32 X1, int32 Y1, CRenPar P1, int32 X2, int32 Y2, CRenPar P2, int32 X3, int32 Y3, CRenPar P3) {
    if (DontRenderYSplitTri) { // NOTSA
        return;
    }

    const auto [minY, maxY] = std::minmax(Y1, Y3);
    const auto height = maxY - minY;
    const auto width  = X2 - X1;
    
    // Calulcate the X position where the Y line intersects the hypot
    // and using that we split the triangle. 
    // Same result as original code, but much easier.

    SplitWaterTriangleAlongXLine(
        X1 + (maxY - splitAtY) * width / height,
        X1, Y1, P1,
        X2, Y2, P2,
        X3, Y3, P3
    );
}

// 0x6EC5D0
void CWaterLevel::RenderWaterRectangle(int32 minX, int32 maxX, int32 Y1, int32 Y2, CRenPar P1, CRenPar P2, CRenPar P3, CRenPar P4) {
    const auto [minY, maxY] = std::minmax(Y1, Y2);
    if (minX >= CameraRangeMaxX || maxX <= CameraRangeMinX || minY >= CameraRangeMaxY || maxY <= CameraRangeMinY) { // Lies outside (of camera) fully
        RenderFlatWaterRectangle(minX, maxX, Y1, Y2, P1, P2, P3, P4);
    } else if (minX < CameraRangeMinX || maxX > CameraRangeMaxX) { // Lies inside on X
        SplitWaterRectangleAlongXLine(minX < CameraRangeMinX ? CameraRangeMinX : CameraRangeMaxX, minX, maxX, Y1, Y2, P1, P2, P3, P4);
    } else if (minY < CameraRangeMinY || maxY > CameraRangeMaxY) { // Lies inside of Y
        SplitWaterRectangleAlongYLine(minY < CameraRangeMinY ? CameraRangeMinY : CameraRangeMaxY, minX, maxX, Y1, Y2, P1, P2, P3, P4);
    } else { // Lies inside of camera fully
        RenderHighDetailWaterRectangle(minX, maxX, Y1, Y2, P1, P2, P3, P4);
    }
}

// 0x6EBEC0
void CWaterLevel::RenderFlatWaterRectangle(int32 minX, int32 maxX, int32 Y1, int32 Y2, CRenPar P1, CRenPar P2, CRenPar P3, CRenPar P4) {
    if (bSplitBigPolys && maxX - minX > BigPolySize) {
        SplitWaterRectangleAlongXLine((minX + maxX) / 2,  minX, maxX, Y1, Y2, P1, P2, P3, P4);
#ifdef FIX_BUGS
    } else if (const auto [minY, maxY] = std::minmax(Y1, Y2); bSplitBigPolys && (maxY - minY) > BigPolySize) {
#else
    } else if (bSplitBigPolys && Y2 - Y1 > BigPolySize) {
#endif
        SplitWaterRectangleAlongYLine((Y2 + Y1) / 2, minX, maxX, Y1, Y2, P1, P2, P3, P4);
    } else {
        for (int32 lyr = 0; lyr < 2; lyr++) {
            RenderFlatWaterRectangle_OneLayer(minX, maxX, Y1, Y2, P1, P2, P3, P4, lyr);
        }
    }
}

// 0x6E9940
void CWaterLevel::RenderFlatWaterRectangle_OneLayer(int32 minX, int32 maxX, int32 Y1, int32 Y2, CRenPar P1, CRenPar P2, CRenPar P3, CRenPar P4, int32 WaterLayer) {
    RenderBuffer::RenderIfDoesntFit(6, 4);

    // First(!) push indices
    RenderBuffer::PushIndices({ 0, 1, 2, 2, 3, 0 }, true);

    // Get texture UV stuff
    const auto texuv = GetTextureUV(minX, Y1, Y2, WaterLayer);

    const auto PushVertex = [
        &,
        color = GetWaterColorForRendering(WaterColor, DebugWaterColors[DebugWaterColor::RECT], WaterLayer)
    ](int32 x, int32 y, const CRenPar& p, CVector2D vtxUVOffset) {
        RenderBuffer::PushVertex({ (float)x, (float)y, p.z }, texuv.baseShift + vtxUVOffset, color);
    };

    // Bottom right corner position on texture (In UV coords)
    const auto bruv{ CVector2D{ (float)(maxX - minX), (float)(Y2 - Y1) } / texuv.size };

    PushVertex(minX, Y1, P1, { 0.f,    0.f }); // Top Left
    PushVertex(maxX, Y1, P2, { bruv.x, 0.f    }); // Top Right
    PushVertex(maxX, Y2, P3, { bruv.x, bruv.y }); // Bottom Right
    PushVertex(minX, Y2, P4, { 0.f,    bruv.y }); // Bottom Left
} 

// 0x6EB810
void CWaterLevel::RenderHighDetailWaterRectangle(int32 minX, int32 maxX, int32 Y1, int32 Y2, CRenPar P1, CRenPar P2, CRenPar P3, CRenPar P4) {
    const CVector center{ (float)(minX + maxX) * 0.5f, (float)(Y1 + Y2) * 0.5f, P1.z };
    const float radius = std::sqrt(sq((float)(maxX - minX) * 0.5f) + sq((float)(Y1 - Y2) * 0.5f));

    if (!TheCamera.IsSphereVisible(center, radius, (RwMatrix*)&TheCamera.m_mMatInverse)) {
        if (!TheCamera.m_bMirrorActive) {
            return;
        }
        if (!TheCamera.IsSphereVisible(center, radius, (RwMatrix*)&TheCamera.m_mMatMirrorInverse)) {
            return;
        }
    }

    const int32 halfX = (maxX - minX) / 2;
    const int32 hiY   = std::max(Y1, Y2);
    const int32 loY   = std::min(Y1, Y2);
    const int32 halfY = (hiY - loY) / 2;

    const int32 numVertices = halfY * halfX * 2;
    const int32 numRows     = (halfY + 1) * (halfX + 1);

    if (halfY * halfX * 6 < 0x1000 && numRows < 0x800) {
        SetUpWaterFog(minX, loY, maxX, hiY);

        RenderHighDetailWaterRectangle_OneLayer(minX, maxX, Y1, Y2, P1, P2, P3, P4, 0, numVertices, numRows, halfX, halfY);
        RenderHighDetailWaterRectangle_OneLayer(minX, maxX, Y1, Y2, P1, P2, P3, P4, 1, numVertices, numRows, halfX, halfY);
        return;
    }

    // Too big to be rendered at once, so split it
    if (halfX <= halfY) {
        const int32 splitY = loY + (halfY / 2) * 2;
        const float t = (float)(splitY - Y1) / (float)(Y2 - Y1);

        const auto P13 = lerp(P1, P3, t);
        const auto P24 = lerp(P2, P4, t);

        RenderWaterRectangle(minX, maxX, Y1, splitY, P1, P2, P13, P24);

        const auto P12 = lerp(P1, P2, t);
        RenderWaterRectangle(minX, maxX, splitY, Y2, P12, P24, P3, P4);
        return;
    }

    SplitWaterRectangleAlongXLine(minX + (halfX / 2) * 2, minX, maxX, Y1, Y2, P1, P2, P3, P4);
}

// 0x6E91D0
void CWaterLevel::RenderHighDetailWaterRectangle_OneLayer(
    int32 minX, int32 maxX, int32 Y1, int32 Y2,
    CRenPar P1, CRenPar P2, CRenPar P3, CRenPar P4,
    int32 WaterLayer, int32 numVertices, int32 numRows, int32 halfX, int32 halfY
) {
    (void)numVertices;
    (void)numRows;

    ms_nTempVertexColorIdx = 0;
    RenderAndEmptyRenderBuffer();

    const float invHalfX = 1.0f / (float)halfX;
    const float invHalfY = 1.0f / (float)halfY;

    const int32 xStep = (maxX - minX) / halfX;
    const int32 xBase = ((minX - maxX) / halfX) * halfX + maxX;
    const int32 yStep = (Y2 - Y1) / halfY;
    const int32 yBase = ((Y1 - Y2) / halfY) * halfY + Y2;

    // Height/wave gradients, relative to `P1` and `P4`
    const float dz1dx   = (P2.z          - P1.z         ) * invHalfX;
    const float dz1dy   = (P3.z          - P1.z         ) * invHalfY;
    const float dBig1dx = (P2.bigWaves   - P1.bigWaves  ) * invHalfX;
    const float dBig1dy = (P3.bigWaves   - P1.bigWaves  ) * invHalfY;
    const float dSml1dx = (P2.smallWaves - P1.smallWaves) * invHalfX;
    const float dSml1dy = (P3.smallWaves - P1.smallWaves) * invHalfY;

    const float dz4dx   = (P3.z          - P4.z         ) * invHalfX;
    const float dz4dy   = (P2.z          - P4.z         ) * invHalfY;
    const float dBig4dx = (P3.bigWaves   - P4.bigWaves  ) * invHalfX;
    const float dBig4dy = (P2.bigWaves   - P4.bigWaves  ) * invHalfY;
    const float dSml4dx = (P3.smallWaves - P4.smallWaves) * invHalfX;
    const float dSml4dy = (P2.smallWaves - P4.smallWaves) * invHalfY;

    float texU{}, texV{};
    if (WaterLayer == 0) {
        texU = (float)minX * 0.08f + TextureShiftSecondU;
        texV = (float)Y1   * 0.08f + TextureShiftSecondV;
    } else if (WaterLayer == 1) {
        texU = (float)minX * 0.04f + TextureShiftFirstU;
        texV = (float)Y1   * 0.04f + TextureShiftFirstV;
    }
    if (WaterLayer <= 1) {
        texU -= std::floor(texU);
        texV -= std::floor(texV);
    }

    const auto& camPos = TheCamera.GetPosition();

    int32 yOffset    = 0;
    int32 yFromP4    = yBase;
    int32 yRemaining = halfY;

    for (int32 y = 0; y <= halfY; y++) {
        const float fy     = (float)y;
        const float yFrac  = fy * invHalfY;

        int32 xOffset    = 0;
        int32 xFromP4    = xBase;
        int32 xRemaining = halfX;

        for (int32 x = 0; x <= halfX; x++) {
            const float fx = (float)x;

            float z, bigWaves, smallWaves;
            int32 iX, iY;

            if (1.0f <= fx * invHalfX + yFrac) {
                const float rx = (float)xRemaining;
                const float ry = (float)yRemaining;

                z          = rx * dz4dx    + ry * dz4dy    + P4.z;
                bigWaves   = rx * dBig4dx  + ry * dBig4dy  + P4.bigWaves;
                smallWaves = rx * dSml4dx  + ry * dSml4dy  + P4.smallWaves;

                iX = xFromP4;
                iY = yFromP4;
            } else {
                z          = fx * dz1dx    + fy * dz1dy    + P1.z;
                bigWaves   = fx * dBig1dx  + fy * dBig1dy  + P1.bigWaves;
                smallWaves = fy * dSml1dy  + fx * dSml1dx  + P1.smallWaves;

                iX = minX + xOffset;
                iY = Y1   + yOffset;
            }

            const float dx = camPos.x - (float)iX;
            const float dy = camPos.y - (float)iY;
            const float distToCam = std::sqrt(dx * dx + dy * dy) / (float)DETAILEDWATERDIST;

            float fadeMul;
            if (distToCam <= 1.0f) {
                fadeMul = 1.0f;
                if (distToCam > 0.75f) {
                    fadeMul = (1.0f - distToCam) * 4.0f;
                }
            } else {
                fadeMul = 0.0f;
            }

            if (WaterLayer == 0) {
                float colorMult, glare;
                CalculateWavesOnlyForCoordinate(iX, iY, fadeMul * bigWaves, smallWaves * fadeMul, z, colorMult, glare, ms_vecWaterWaveNormal);

                auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored];
                vtx.u         = (float)xOffset * 0.08f + texU;
                vtx.v         = (float)yOffset * 0.08f + texV;
                vtx.objVertex = { (float)iX, (float)iY, z };

                const uint8 r = (uint8)(int32)((float)WaterColor.r * colorMult);
                const uint8 g = (uint8)(int32)((float)WaterColor.g * colorMult);
                const uint8 b = (uint8)(int32)((float)WaterColor.b * colorMult);

                ms_TempVertexColorR[ms_nTempVertexColorIdx] = r;
                ms_TempVertexColorG[ms_nTempVertexColorIdx] = g;
                ms_TempVertexColorB[ms_nTempVertexColorIdx] = b;

                vtx.color = ((WaterLayerAlpha[0] & 0xFF) << 24) | ((uint32)r << 16) | ((uint32)g << 8) | b;
                ms_nTempVertexColorIdx++;
            } else if (WaterLayer == 1) {
                auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored];
                vtx.u = (float)xOffset * 0.04f + texU;
                vtx.v = (float)yOffset * 0.04f + texV;

                const uint32 r = ms_TempVertexColorR[ms_nTempVertexColorIdx];
                const uint32 g = ms_TempVertexColorG[ms_nTempVertexColorIdx];
                const uint32 b = ms_TempVertexColorB[ms_nTempVertexColorIdx];

                vtx.color = ((WaterLayerAlphaRand & 0xFF) << 24) | (r << 16) | (g << 8) | b;
                ms_nTempVertexColorIdx++;
            }

            if (!ms_bDontGenerateWaterRectMesh) {
                const uint16 v = (uint16)uiTempBufferVerticesStored;

                if (y != 0 && x != 0) {
                    auto& idx = aTempBufferIndices;
                    const auto i = uiTempBufferIndicesStored;
                    idx[i + 0] = (RxVertexIndex)(v - (uint16)halfX - 2);
                    idx[i + 1] = (RxVertexIndex)(v - (uint16)halfX - 1);
                    idx[i + 2] = (RxVertexIndex)(v - 1);
                    idx[i + 3] = (RxVertexIndex)v;
                    idx[i + 4] = (RxVertexIndex)(v - (uint16)halfX - 1);
                    idx[i + 5] = (RxVertexIndex)(v - 1);
                    uiTempBufferIndicesStored += 6;
                }

                uiTempBufferVerticesStored++;
            }

            xFromP4   += xStep;
            xRemaining--;
            xOffset   += xStep;
        }

        yFromP4    += yStep;
        yRemaining--;
        yOffset    += yStep;
    }
}

// 0x6E73A0
void CWaterLevel::SplitWaterRectangleAlongXLine(int32 splitAtX, int32 minX, int32 maxX, int32 Y1, int32 Y2, CRenPar P1, CRenPar P2, CRenPar P3, CRenPar P4) {
    const auto t = (float)(splitAtX - minX) / (float)(maxX - minX);

    const auto P12 = lerp(P1, P2, t);
    const auto P34 = lerp(P3, P4, t);

    // Left
    RenderWaterRectangle(
        minX, splitAtX,
        Y1, Y2,
        P1, P12, P3, P34
    );

    // Right
    RenderWaterRectangle(
        splitAtX, maxX,
        Y1, Y2,
        P12, P2, P34, P4
    );
}

// 0x6ED6D0 - Though fully inlined into `RenderWaterRectangle`
void CWaterLevel::SplitWaterRectangleAlongYLine(int32 splitAtY, int32 minX, int32 maxX, int32 Y1, int32 Y2, CRenPar P1, CRenPar P2, CRenPar P3, CRenPar P4) {
    const auto [minY, maxY] = std::minmax(Y1, Y2);

    const auto t = (float)(splitAtY - minY) / (float)(maxY - minY);

    const auto P13 = lerp(P1, P3, t);
    const auto P24 = lerp(P2, P4, t);

    // Top
    RenderWaterRectangle(
        minX, maxX,
        Y1, splitAtY,
        P1, P2, P13, P24
    );

    // Bottom
    RenderWaterRectangle(
        minX,     maxX,
        splitAtY, Y2,
        P13, P24, P3, P4
    );
}

// 0x6EB710
void CWaterLevel::PreRenderWater() {
    ZoneScoped;

    if (CGame::CanSeeWaterFromCurrArea()) {
        ScanThroughBlocks();
        UpdateFlow();
        HandleBeachToysStuff();
    }
}

// NOTSA: From PreRenderWater()
void CWaterLevel::UpdateFlow() {
    if (CTimer::GetFrameCounter() % 32 == 29) {
        CWaterLevel::FindNearestWaterAndItsFlow();
    }

    const auto CalculateFlowOnAxis = [
        step = CTimer::GetTimeStep() / 1000.f
    ](float desired, float curr) {
        const auto delta = desired - curr;
        return std::abs(delta) < step
            ? desired
            : curr + std::copysign(step, delta);
    };

    m_CurrentFlow = {
        CalculateFlowOnAxis(m_CurrentDesiredFlow.x, m_CurrentFlow.x),
        CalculateFlowOnAxis(m_CurrentDesiredFlow.y, m_CurrentFlow.y)
    };
}

// 0x6EB690
bool CWaterLevel::GetWaterLevel(float x, float y, float z, float& pOutWaterLevel, uint8 bTouchingWater, CVector* pVecNormals) {
    float smallWaves, bigWaves;
    if (!GetWaterLevelNoWaves({x, y, z}, &pOutWaterLevel, &smallWaves, &bigWaves)) {
        return false;
    }
     
    if ((pOutWaterLevel - z > 3.0F) && !bTouchingWater) {
        pOutWaterLevel = 0.0F;
        return false;
    }

    AddWaveToResult(x, y, &pOutWaterLevel, smallWaves, bigWaves, pVecNormals);

    return true;
}

// 0x6EA9F0
void CWaterLevel::SetUpWaterFog(int32 minX, int32 minY, int32 maxX, int32 maxY) {
    if (!CWaterLevel::m_bWaterFog || gWaterFogIndex >= 70) {
        return;
    }

    const auto fogZ = [&] {
        if (float waterLvl, bigWaves, smallWaves; GetWaterLevelNoWaves({(float)minX, (float)minY, 0.f}, & waterLvl, & bigWaves, & smallWaves)) {
            if (bigWaves != 0.f || smallWaves != 0.f) {
                return waterLvl;
            }
        }
        return 0.f;
    }();

    const auto plyrPos = FindPlayerCoors();
    gbPlayerIsInsideWaterFog = m_fWaterFogHeight + fogZ > plyrPos.z && CRect{ (float)minX, (float)minY, (float)maxX, (float)maxY }.IsPointInside(plyrPos);

    const auto idx = gWaterFogIndex++;
    ms_WaterFog.minX[idx] = minX;
    ms_WaterFog.minY[idx] = minY;
    ms_WaterFog.maxX[idx] = maxX;
    ms_WaterFog.maxY[idx] = maxY;
    ms_WaterFog.z[idx]    = fogZ;
}

// 0x6E9D70
void CWaterLevel::FindNearestWaterAndItsFlow() {
    const auto& camPos = TheCamera.GetPosition();
    const float camX = camPos.x;
    const float camY = camPos.y;

    if (camX < -3000.0f || camX >= 3000.0f || camY < -3000.0f || camY >= 3000.0f) {
        TheCamera.m_fDistanceToWater      = 0.0f;
        TheCamera.m_fHeightOfNearestWater = 0.0f;
        m_CurrentDesiredFlow.x = 0.0f;
        m_CurrentDesiredFlow.y = 0.0f;
        return;
    }

    float nearestWithWavesDist = 1.0e7f;
    float nearestWithWavesZ    = 0.0f;
    float nearestDist          = 1.0e7f;

    for (auto i = 0u; i < NumWaterQuads; i++) {
        const auto& quad = WaterQuads[i];

        const auto& v0 = m_aVertices[quad.verts[0]];
        const auto& v1 = m_aVertices[quad.verts[1]];
        const auto& v2 = m_aVertices[quad.verts[2]];
        const auto& v3 = m_aVertices[quad.verts[3]];

        // Distance from the camera to the quad's X/Y bounding rectangle
        const float dx = camX < (float)v0.x
            ? (float)v0.x - camX
            : (camX > (float)v1.x ? camX - (float)v1.x : 0.0f);
        const float dy = camY < (float)v0.y
            ? (float)v0.y - camY
            : (camY > (float)v2.y ? camY - (float)v2.y : 0.0f);
        const float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < nearestWithWavesDist && (
                v0.rp.bigWaves != 0.0f || v0.rp.smallWaves != 0.0f ||
                v1.rp.bigWaves != 0.0f || v1.rp.smallWaves != 0.0f ||
                v2.rp.bigWaves != 0.0f || v2.rp.smallWaves != 0.0f ||
                v3.rp.bigWaves != 0.0f || v3.rp.smallWaves != 0.0f)
        ) {
            nearestWithWavesZ    = v0.rp.z;
            nearestWithWavesDist = dist;
        }

        if (dist < nearestDist) {
            nearestDist = dist;

            const float d0 = sq(camY - (float)v0.y) + sq(camX - (float)v0.x);
            const float d1 = sq(camY - (float)v1.y) + sq(camX - (float)v1.x);
            const float d2 = sq(camY - (float)v2.y) + sq(camX - (float)v2.x);
            const float d3 = sq(camY - (float)v3.y) + sq(camX - (float)v3.x);

            const CWaterVertex* nearest;
            if (d1 <= d0 || d2 <= d0 || d3 <= d0) {
                if (d2 <= d1 || d3 <= d1) {
                    nearest = d3 <= d2 ? &v3 : &v2;
                } else {
                    nearest = &v1;
                }
            } else {
                nearest = &v0;
            }

            m_CurrentDesiredFlow = {
                (float)(int8)nearest->rp.flowX * 0.015625f,
                (float)(int8)nearest->rp.flowY * 0.015625f
            };
        }
    }

    TheCamera.m_fDistanceToWater      = nearestWithWavesDist;
    TheCamera.m_fHeightOfNearestWater = nearestWithWavesZ;
}

// 0x6E5BB0
bool CWaterLevel::TestQuadToGetWaterLevel(CWaterQuad* quad, float x, float y, float z, float* outLevel, float* outBigWaves, float* outSmallWaves) {
    const auto& v0 = m_aVertices[quad->verts[0]];
    const auto& v1 = m_aVertices[quad->verts[1]];
    const auto& v2 = m_aVertices[quad->verts[2]];
    const auto& v3 = m_aVertices[quad->verts[3]];

    if (x < (float)v0.x || x > (float)v1.x || y < (float)v0.y || y > (float)v2.y) {
        return false;
    }

    const float fx = (x - (float)v0.x) / (float)(v1.x - v0.x);
    const float fy = (y - (float)v0.y) / (float)(v2.y - v0.y);

    if (fx + fy <= 1.0f) {
        *outLevel = (v1.rp.z - v0.rp.z) * fx + (v2.rp.z - v0.rp.z) * fy + v0.rp.z;

        if (outBigWaves) {
            *outBigWaves   = (v1.rp.bigWaves   - v0.rp.bigWaves  ) * fx + (v2.rp.bigWaves   - v0.rp.bigWaves  ) * fy + v0.rp.bigWaves;
            *outSmallWaves = (v1.rp.smallWaves - v0.rp.smallWaves) * fx + (v2.rp.smallWaves - v0.rp.smallWaves) * fy + v0.rp.smallWaves;
        }
    } else {
        // NOTE: OG swaps `fx`/`fy` for the height here (but not for the wave amplitudes), mirrored as-is
        *outLevel = (v1.rp.z - v3.rp.z) * (1.0f - fy) + (v2.rp.z - v3.rp.z) * (1.0f - fx) + v3.rp.z;

        if (outBigWaves) {
            *outBigWaves   = (v2.rp.bigWaves   - v3.rp.bigWaves  ) * (1.0f - fx) + (v1.rp.bigWaves   - v3.rp.bigWaves  ) * (1.0f - fy) + v3.rp.bigWaves;
            *outSmallWaves = (v2.rp.smallWaves - v3.rp.smallWaves) * (1.0f - fx) + (v1.rp.smallWaves - v3.rp.smallWaves) * (1.0f - fy) + v3.rp.smallWaves;
        }
    }

    // The water level is only valid within a vertical band (and always, if the depth isn't limited)
    if ((z >= *outLevel - 6.0f || !quad->bLimitedDepth) && z <= *outLevel + 20.0f) {
        return true;
    }
    return false;
}

// 0x6E5E90
bool CWaterLevel::TestTriangleToGetWaterLevel(CWaterTriangle* tri, float x, float y, float z, float* outLevel, float* outBigWaves, float* outSmallWaves) {
    const auto& v0 = m_aVertices[tri->verts[0]];
    const auto& v1 = m_aVertices[tri->verts[1]];
    const auto& v2 = m_aVertices[tri->verts[2]];

    if (x < (float)v0.x || x > (float)v1.x) {
        return false;
    }

    const int32 loY = std::min<int32>(v0.y, v2.y);
    const int32 hiY = std::max<int32>(v0.y, v2.y);
    if (y < (float)loY || y > (float)hiY) {
        return false;
    }

    const float fx = (x - (float)v0.x) / (float)(v1.x - v0.x);
    const float fy = (y - (float)v0.y) / (float)(v2.y - v0.y);

    if (v0.x == v2.x) { // Vertical edge on the left/right
        if (fx + fy > 1.0f) {
            return false;
        }
        *outLevel = (v1.rp.z - v0.rp.z) * fx + (v2.rp.z - v0.rp.z) * fy + v0.rp.z;

        if (outBigWaves) {
            *outBigWaves   = (v2.rp.bigWaves   - v0.rp.bigWaves  ) * fy + (v1.rp.bigWaves   - v0.rp.bigWaves  ) * fx + v0.rp.bigWaves;
            *outSmallWaves = (v1.rp.smallWaves - v0.rp.smallWaves) * fx + (v2.rp.smallWaves - v0.rp.smallWaves) * fy + v0.rp.smallWaves;
        }
    } else {
        if (fx < fy) {
            return false;
        }
        *outLevel = (v0.rp.z - v1.rp.z) * (1.0f - fx) + (v2.rp.z - v1.rp.z) * fy + v1.rp.z;

        if (outBigWaves) {
            *outBigWaves   = (v2.rp.bigWaves   - v1.rp.bigWaves  ) * fy + (v0.rp.bigWaves   - v1.rp.bigWaves  ) * (1.0f - fx) + v1.rp.bigWaves;
            *outSmallWaves = (v2.rp.smallWaves - v1.rp.smallWaves) * fy + (v0.rp.smallWaves - v1.rp.smallWaves) * (1.0f - fx) + v1.rp.smallWaves;
        }
    }

    if ((z >= *outLevel - 6.0f || !tri->bLimitedDepth) && z <= *outLevel + 20.0f) {
        return true;
    }
    return false;
}

// 0x6E61B0
bool CWaterLevel::TestLineAgainstWater(CVector start, CVector end, CVector* out) {
    //! World coordinate -> water block index.
    //! NOTE: The OG's conversion truncates towards zero (`__ftol2`), it doesn't `floor`.
    const auto ToBlockIdx = [](float coord) {
        return (int32)(coord * 0.002f + 6.0f);
    };

    const auto minZ = std::min(start.z, end.z);
    const auto maxZ = std::max(start.z, end.z);

    const int32 loBlockX = ToBlockIdx(std::min(start.x, end.x));
    const int32 hiBlockX = ToBlockIdx(std::max(start.x, end.x));
    const int32 loBlockY = ToBlockIdx(std::min(start.y, end.y));
    const int32 hiBlockY = ToBlockIdx(std::max(start.y, end.y));

    if (loBlockX > hiBlockX) {
        return false;
    }

    //! Point where the line crosses the water plane (`z = 0`)
    const auto CalcWaterPlaneIntersection = [&] {
        const float t = std::abs(start.z) / (maxZ - minZ);
        *out = start + (end - start) * t;
    };

    //! NOTE: Only the quad's X/Y bounding box is considered (The diagonal split is ignored, as in the OG)
    const auto IsPointInQuad = [&](const CWaterQuad& quad) {
        const auto& v0 = m_aVertices[quad.verts[0]];
        const auto& v1 = m_aVertices[quad.verts[1]];
        const auto& v2 = m_aVertices[quad.verts[2]];

        return out->x > (float)v0.x && out->x <= (float)v1.x
            && out->y > (float)v0.y && out->y <= (float)v2.y;
    };

    //! Tests the line against a single quad (Only if it even crosses the water plane)
    const auto TestQuad = [&](const CWaterQuad& quad) {
        if (start.z * end.z >= 0.0f) { // The line doesn't cross the water plane
            return false;
        }
        CalcWaterPlaneIntersection();
        return IsPointInQuad(quad);
    };

    using PType = PolyInfo::PType;

    for (int32 blockX = loBlockX; blockX <= hiBlockX; blockX++) {
        for (int32 blockY = loBlockY; blockY <= hiBlockY; blockY++) {
            if (blockX < 0 || blockX >= NUM_WATER_BLOCKS_ROWCOL || blockY < 0 || blockY >= NUM_WATER_BLOCKS_ROWCOL) {
                //! Outside of the world everything at the water plane (`z = 0`) is water
                if (minZ >= 0.0f || maxZ <= 0.0f) {
                    continue;
                }
                CalcWaterPlaneIntersection();
                if (ToBlockIdx(out->x) != blockX || ToBlockIdx(out->y) != blockY) {
                    continue;
                }
                return true;
            }

            const auto& blockInfo = m_BlockPolyInfo[blockX][blockY];
            switch (blockInfo.Type()) {
            case PType::SINGLE_QUAD:
                if (TestQuad(WaterQuads[blockInfo.Id()])) {
                    return true;
                }
                break;
            case PType::COMBO:
                //! NOTE: Only quads are tested, triangles within a combo are skipped (As in the OG)
                for (auto idx = blockInfo.Id(); m_PolyCombos[idx].Type() != PType::NONE; idx++) {
                    if (m_PolyCombos[idx].Type() == PType::SINGLE_QUAD && TestQuad(WaterQuads[m_PolyCombos[idx].Id()])) {
                        return true;
                    }
                }
                break;
            default: // `NONE`/`SINGLE_TRI`
                break;
            }
        }
    }

    return false;
}

// 0x6E8580
bool CWaterLevel::GetWaterLevelNoWaves(CVector pos, float * pOutWaterLevel, float * pOutBigWaves, float * pOutSmallWaves) {
    const int32 blockX = (int32)std::floor(pos.x * 0.002f + 6.0f);
    const int32 blockY = (int32)std::floor(pos.y * 0.002f + 6.0f);

    if (blockX < 0 || blockX > NUM_WATER_BLOCKS_ROWCOL - 1 || blockY < 0 || blockY > NUM_WATER_BLOCKS_ROWCOL - 1) {
        *pOutWaterLevel = 0.0f;
        if (pOutBigWaves) {
            *pOutBigWaves = 1.0f;
        }
        if (pOutSmallWaves) {
            *pOutSmallWaves = 0.0f;
        }
        return true;
    }

    using PType = PolyInfo::PType;

    const auto info = m_BlockPolyInfo[blockX][blockY];
    switch (info.Type()) {
    case PType::NONE:
        return false;
    case PType::SINGLE_QUAD:
        return TestQuadToGetWaterLevel(&WaterQuads[info.Id()], pos.x, pos.y, pos.z, pOutWaterLevel, pOutBigWaves, pOutSmallWaves);
    case PType::SINGLE_TRI:
        return TestTriangleToGetWaterLevel(&WaterTriangles[info.Id()], pos.x, pos.y, pos.z, pOutWaterLevel, pOutBigWaves, pOutSmallWaves);
    case PType::COMBO: {
        for (const auto& combo : m_PolyCombos | rng::views::drop(info.Id())) {
            switch (combo.Type()) {
            case PType::SINGLE_QUAD:
                if (TestQuadToGetWaterLevel(&WaterQuads[combo.Id()], pos.x, pos.y, pos.z, pOutWaterLevel, pOutBigWaves, pOutSmallWaves)) {
                    return true;
                }
                break;
            case PType::SINGLE_TRI:
                if (TestTriangleToGetWaterLevel(&WaterTriangles[combo.Id()], pos.x, pos.y, pos.z, pOutWaterLevel, pOutBigWaves, pOutSmallWaves)) {
                    return true;
                }
                break;
            default: // End of the sequence
                return false;
            }
        }
        return false;
    }
    default:
        return false;
    }
}

// 0x6EA8A0
bool CWaterLevel::GetGroundLevel(const CVector& pos, float* outGroundZ, ColData* outColData, float maxHeight) {
    CColPoint colPoint{};
    CEntity*  hitEntity{};

    if (!CWorld::ProcessVerticalLine(
            CVector{ pos.x, pos.y, pos.z + maxHeight },
            -maxHeight,
            colPoint,
            hitEntity,
            true, false, false, false, true, false, nullptr
        )
    ) {
        return false;
    }

    *outGroundZ = colPoint.m_vecPoint.z;
    if (outColData) {
        outColData->material = (uint8)colPoint.m_nSurfaceTypeB;
        outColData->piece    = colPoint.m_nPieceTypeB;
    }
    return true;
}

// 0x6EA960
bool CWaterLevel::GetWaterDepth(const CVector& vecPos, float* pOutWaterDepth, float* pOutWaterLevel, float* pOutGroundLevel)
{
    float waterLevel;
    if (!GetWaterLevelNoWaves(vecPos, &waterLevel, nullptr, nullptr)) {
        return false;
    }

    float groundZ;
    if (!GetGroundLevel(vecPos, &groundZ, nullptr, 30.0f)) {
        groundZ = -100.0f;
    }

    if (pOutWaterDepth) {
        *pOutWaterDepth = waterLevel - groundZ;
    }
    if (pOutWaterLevel) {
        *pOutWaterLevel = waterLevel;
    }
    if (pOutGroundLevel) {
        *pOutGroundLevel = groundZ;
    }
    return true;
}

// 0x6E7760
void CWaterLevel::RenderWaterFog() {
    ZoneScoped;

    if (!m_bWaterFog || !m_bWaterFogScript) {
        return;
    }

    if (CWeather::UnderWaterness >= CPostEffects::m_fWaterFXStartUnderWaterness) {
        gWaterFogIndex = 0;
        return;
    }

    const int32 numSlices = (int32)((float)m_WaterFogDensity * CWeather::WaterFogFXControl);
    if (!numSlices) {
        gWaterFogIndex = 0;
        return;
    }

    const int32 numFogBoxes = (int32)gWaterFogIndex;
    const float sliceZStep  = m_fWaterFogHeight / (float)numSlices;
    gWaterFogIndex = 0;

    if (gbPlayerIsInsideWaterFog) {
        WaterFogFade += CTimer::GetTimeStep() * m_fWaterFogFadeSpeed;
        if (WaterFogFade > 1.0f) {
            WaterFogFade = 1.0f;
        }
        WaterFogTimer = 40.0f;
    } else {
        WaterFogTimer -= CTimer::GetTimeStep();
        if (WaterFogTimer <= 0.0f) {
            WaterFogTimer = 0.0f;
            WaterFogFade -= CTimer::GetTimeStep() * m_fWaterFogFadeSpeed;
            if (WaterFogFade < 0.0f) {
                WaterFogFade = 0.0f;
            }
        }
    }
    gbPlayerIsInsideWaterFog = false;

    // Full-screen tint while the player is (or just was) inside the fog
    if (WaterFogFade > 0.0f) {
        const int32 alpha = (int32)((float)m_WaterFogInsideCol.a * WaterFogFade);

        CPostEffects::ImmediateModeRenderStatesStore();
        CPostEffects::ImmediateModeRenderStatesSet();
        CPostEffects::DrawQuad(
            0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT,
            m_WaterFogInsideCol.r,
            m_WaterFogInsideCol.g,
            m_WaterFogInsideCol.b,
            (uint8)(int32)((float)alpha * CWeather::WaterFogFXControl),
            nullptr
        );
        CPostEffects::ImmediateModeRenderStatesReStore();

        if (WaterFogFade == 1.0f) {
            return;
        }
    }

    CPostEffects::ImmediateModeRenderStatesStore();
    CPostEffects::ImmediateModeRenderStatesSet();
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,   RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(NULL));

    const int32 numSlicesToDraw = (int32)((1.0f - WaterFogFade) * (float)numSlices);
    const uint32 col = ((uint32)m_WaterFogCol.a << 24)
                     | ((uint32)m_WaterFogCol.r << 16)
                     | ((uint32)m_WaterFogCol.g << 8)
                     |  (uint32)m_WaterFogCol.b;

    int32 vertexCount{};

    const auto Flush = [&] {
        if (RwIm3DTransform(TempBufferVertices.m_3d, vertexCount, nullptr, rwIM3D_VERTEXXYZ)) {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
            RwIm3DEnd();
        }
        vertexCount = 0;
    };

    for (int32 i = 0; i < numFogBoxes; i++) {
        const CVector2D corners[]{
            { (float)ms_WaterFog.minX[i], (float)ms_WaterFog.minY[i] },
            { (float)ms_WaterFog.maxX[i], (float)ms_WaterFog.minY[i] },
            { (float)ms_WaterFog.maxX[i], (float)ms_WaterFog.maxY[i] },
            { (float)ms_WaterFog.minX[i], (float)ms_WaterFog.minY[i] },
            { (float)ms_WaterFog.maxX[i], (float)ms_WaterFog.maxY[i] },
            { (float)ms_WaterFog.minX[i], (float)ms_WaterFog.maxY[i] },
        };

        float z = ms_WaterFog.z[i];
        for (int32 slice = 0; slice < numSlicesToDraw; slice++) {
            for (const auto& corner : corners) {
                auto& vtx = TempBufferVertices.m_3d[vertexCount++];
                vtx.objVertex = { corner.x, corner.y, z };
                vtx.color     = col;

                if (vertexCount == 0x7FE) {
                    Flush();
                }
            }
            z += sliceZStep;
        }
    }

    if (vertexCount > 0) {
        Flush();
    }

    CPostEffects::ImmediateModeRenderStatesReStore();
}

// 0x6E6EF0
void CWaterLevel::CalculateWavesOnlyForCoordinate(
    int32 x, int32 y,
    float bigWavesAmplitude,
    float smallWavesAmplitude,
    float& outWave,
    float& colorMult,
    float& glare,
    CVector& vecNormal
)
{
    x = std::abs(x);
    y = std::abs(y);
    vecNormal = CVector(0.f, 0.f, 1.f);

    const float waveMult = faWaveMultipliersX[(x / 2) % 8] * faWaveMultipliersY[(y / 2) % 8] * CWeather::Wavyness;
    float fX = (float)x, fY = (float)y;

    // literal AIDS code
    const auto CalculateWave = [&](int32 offset, float angularFreqX, float angularFreqY, float amplitude) {
        const float freqOffsetMult = TWO_PI / static_cast<float>(offset);
        const CVector2D waveVector{ TWO_PI * angularFreqX, TWO_PI * angularFreqY }; // w = angular frequency

        const auto step  = (CTimer::GetTimeInMS() - m_nWaterTimeOffset) % offset;
        const auto wavePhase = step * freqOffsetMult + fX * waveVector.x + fY * waveVector.y;

        const auto sinPhase = CMaths::GetSinFast(wavePhase);
        const auto cosPhase = CMaths::GetCosFast(wavePhase);
        outWave += sinPhase * waveMult * amplitude;

        // Wave normal calculation - seems broken but maybe R* just knows something that we don't :D
        // Normal generation is completely skipped on later releases of the game (android / definitive)
        switch (offset) {
        case 5000: {
            const auto normalDerivative = -cosPhase * waveMult * amplitude * waveVector.x;
            vecNormal += { normalDerivative, normalDerivative, 0.0f };
            break;
        }
        case 3500: {
            const auto normalDerivative = cosPhase * waveMult * amplitude * waveVector.x;
            vecNormal += { normalDerivative, normalDerivative, 0.0f };
            break;
        }
        case 3000: {
            const auto normalDerivative = cosPhase * waveMult * amplitude * (PI / 10.0f);
            vecNormal += { normalDerivative, 0.0f, 0.0f };
            break;
        }
        }
    };

    CalculateWave(5000, 1.f / 64.0f, 1.f / 64.0f, 2.0f * bigWavesAmplitude);
    CalculateWave(3500, 1.f / 26.0f, 1.f / 52.0f, 1.0f * smallWavesAmplitude);
    CalculateWave(3000, 0.0f,        1.f / 20.0f, 0.5f * smallWavesAmplitude);

    vecNormal.Normalise();
    const auto glareLevel = (vecNormal.x + vecNormal.y + vecNormal.z) * E_CONST;

    colorMult = std::max(glareLevel, 0.0f) * 0.65f + 0.27f;
    glare = std::clamp(8.0f * glareLevel - 5.0f, 0.0f, 0.99f) * CWeather::SunGlare;
}

// 0x6E5810
void CWaterLevel::MarkQuadsAndPolysToBeRendered(int32 blockX, int32 blockY, bool isInInterior) {
    using PType = PolyInfo::PType;

    // Horrible naming, sorry.
    const auto ProcessPoly = [&](PolyInfo data) {
        switch (data.Type()) {
        case PType::SINGLE_QUAD:
            WaterQuads[data.Id()].DoMarkToBeRendered(isInInterior);
            break;
        case PType::SINGLE_TRI:
            WaterTriangles[data.Id()].DoMarkToBeRendered(isInInterior);
            break;
        }
    };

    auto& blockPolyInfo = m_BlockPolyInfo[blockX][blockY];
    switch (blockPolyInfo.Type()) {
    case PType::SINGLE_QUAD:
    case PType::SINGLE_TRI:
        ProcessPoly(blockPolyInfo);
        break;
    case PType::COMBO: {
        for (auto& comboPoly : m_PolyCombos | rng::views::drop(blockPolyInfo.Id())) {
            if (comboPoly.Type() == PType::NONE) {
                break; // End of sequence
            }
            ProcessPoly(comboPoly);
        }
        break;
    }
    }
}

// 0x6E7210
void CWaterLevel::CalculateWavesOnlyForCoordinate2( // TODO: Original name didn't have a 2 in it... I'm just lazy!
    int32 x, int32 y,
    float bigWavesAmpl,
    float smallWavesAmpl,
    float* pResultHeight
) {
    x = std::abs(x);
    y = std::abs(y);

    const float waveMult = faWaveMultipliersX[(x / 2) & 7] * faWaveMultipliersY[(y / 2) & 7] * CWeather::Wavyness;

    const uint32 t = CTimer::GetTimeInMS() - m_nWaterTimeOffset;

    // Term 1 (period 5000) - The only one that uses the big wave amplitude
    float phase = (float)(t % 5000) * 0.00125f + ((float)y + (float)x) * 0.0982f;
    *pResultHeight += CMaths::GetSinFast(phase) * 2.0f * waveMult * bigWavesAmpl;

    // Term 2 (period 3500) - OG feeds the accumulated height back into the phase, mirrored as-is
    phase = (float)(t % 3500) * 0.00179f + (float)y * 0.12083f + *pResultHeight * 0.2416f;
    *pResultHeight += CMaths::GetSinFast(phase) * 1.0f * waveMult * smallWavesAmpl;

    // Term 3 (period 3000)
    phase = (float)(t % 3000) * 0.0020945f + (float)y * 0.31392f;
    *pResultHeight += CMaths::GetSinFast(phase) * 0.5f * waveMult * smallWavesAmpl;
}

// 0x6E6CA0
void CWaterLevel::BlockHit(int32 blockX, int32 blockY) {
    if (blockX >= 0 && blockX < NUM_WATER_BLOCKS_ROWCOL && blockY >= 0 && blockY < NUM_WATER_BLOCKS_ROWCOL) {
        MarkQuadsAndPolysToBeRendered(blockX, blockY, CGame::currArea != AREA_CODE_NORMAL_WORLD);
    }

    // Blocks at the edge of the world (index 0 and 11) need to be handled both ways, the quads and polys are to be rendered, but also the general ocean plane needs to be rendered on them
    if (blockX <= 0 || blockX >= (NUM_WATER_BLOCKS_ROWCOL - 1) || blockY <= 0 || blockY >= (NUM_WATER_BLOCKS_ROWCOL - 1)) {
        if (m_NumBlocksOutsideWorldToBeRendered < (uint32)m_MaxNumBlocksOutsideWorldToBeRendered) {
            const auto idx                         = m_NumBlocksOutsideWorldToBeRendered++;
            m_BlocksToBeRenderedOutsideWorldX[idx] = blockX;
            m_BlocksToBeRenderedOutsideWorldY[idx] = blockY;
        }
    }
}

// 0x6E6D10
void CWaterLevel::ScanThroughBlocks() {
    m_NumBlocksOutsideWorldToBeRendered = 0;

    const auto frustumPts = TheCamera.GetFrustumPoints();
    CVector2D scanPts[5]{};
    for (auto i = 0; i < 5; i++) {
        scanPts[i] = CVector2D{ frustumPts[i] } / (float)WATER_BLOCK_SIZE + CVector2D{6.f, 6.f};
    }
    CWorldScan::ScanWorld(scanPts, 5, BlockHit);
}

// 0x6EDDC0
void CWaterLevel::RenderHighDetailWaterTriangle(int32 X1, int32 Y1, CRenPar P1, int32 X2, int32 Y2, CRenPar P2, int32 X3, int32 Y3, CRenPar P3) {
    const CVector center{ (float)(X1 + X2 + X3) * 0.3333333f, (float)(Y1 + Y2 + Y3) * 0.3333333f, P1.z };
    const float radius = (float)(X2 - X1) * 0.71f;

    const bool visible = TheCamera.IsSphereVisible(center, radius, (RwMatrix*)&TheCamera.m_mMatInverse)
        || (TheCamera.m_bMirrorActive && TheCamera.IsSphereVisible(center, radius, (RwMatrix*)&TheCamera.m_mMatMirrorInverse));
    if (!visible) {
        return;
    }

    const int32 halfSpan = (X2 - X1) / 2;
    const int32 numTris  = halfSpan * halfSpan;

    int32 numRows{};
    for (int32 i = 1; i <= halfSpan + 1; i++) {
        numRows += i;
    }

    if (numTris * 3 < 0x1000 && numRows < 0x800) {
        RenderHighDetailWaterTriangle_OneLayer(X1, Y1, P1, X2, Y2, P2, X3, Y3, P3, 0, numTris, numRows, halfSpan);
        RenderHighDetailWaterTriangle_OneLayer(X1, Y1, P1, X2, Y2, P2, X3, Y3, P3, 1, numTris, numRows, halfSpan);
        return;
    }

    SplitWaterTriangleAlongXLine(X1 + (halfSpan / 2) * 2, X1, Y1, P1, X2, Y2, P2, X3, Y3, P3);
}

// 0x6E8780
void CWaterLevel::RenderHighDetailWaterTriangle_OneLayer(
    int32 X1, int32 Y1, CRenPar P1,
    int32 X2, int32 Y2, CRenPar P2,
    int32 X3, int32 Y3, CRenPar P3,
    int32 WaterLayer, int32 numTris, int32 numRows, int32 halfSpan
) {
    (void)numTris;
    (void)numRows;

    ms_nTempVertexColorIdx = 0;
    RenderAndEmptyRenderBuffer();

    const float invHalfSpan = 1.0f / (float)halfSpan;

    // The triangle is always tessellated from its "top" vertex towards the horizontal edge,
    // hence the origin is either `(X1, Y1)` or `(X2, Y2)` depending on the shape.
    float dzdx, dBigdx, dSmldx, zOrigin;
    int32 dx, xOrigin;

    if (X1 == X3) {
        dzdx    = P2.z          - P1.z;
        dx      = X2 - X1;
        xOrigin = X1;
        dBigdx  = P2.bigWaves   - P1.bigWaves;
        dSmldx  = P2.smallWaves - P1.smallWaves;
        zOrigin = P1.z;
    } else {
        dzdx    = P1.z          - P2.z;
        dx      = X1 - X2;
        xOrigin = X2;
        dBigdx  = P1.bigWaves   - P2.bigWaves;
        dSmldx  = P1.smallWaves - P2.smallWaves;
        P1.bigWaves   = P2.bigWaves;
        P1.smallWaves = P2.smallWaves;
        Y1      = Y2;
        zOrigin = P2.z;
    }

    dSmldx  *= invHalfSpan;                                        // Step along `dx`
    const float dzdy    = (P3.z          - zOrigin     ) * invHalfSpan; // Step along `dy`
    dBigdx  *= invHalfSpan;
    dx      /= halfSpan;
    const float dBigdy  = (P3.bigWaves   - P1.bigWaves ) * invHalfSpan;
    dzdx    *= invHalfSpan;
    const int32 dy      = (Y3 - Y1) / halfSpan;
    const float dSmldy  = (P3.smallWaves - P1.smallWaves) * invHalfSpan;
    const float smlOrig = P1.smallWaves;
    const float bigOrig = P1.bigWaves;

    float texU{}, texV{};
    if (WaterLayer == 0) {
        texU = (float)xOrigin * 0.08f + TextureShiftSecondU;
        texV = (float)Y1      * 0.08f + TextureShiftSecondV;
    } else if (WaterLayer == 1) {
        texU = (float)xOrigin * 0.04f + TextureShiftFirstU;
        texV = (float)Y1      * 0.04f + TextureShiftFirstV;
    }
    if (WaterLayer <= 1) {
        texU -= std::floor(texU);
        texV -= std::floor(texV);
    }

    const auto& camPos = TheCamera.GetPosition();

    int32 yTexOffset = 0;
    int32 yCoord     = Y1;
    int32 numInRow   = halfSpan;

    for (int32 row = 0; row <= halfSpan; row++) {
        const float rowF = (float)row;

        int32 xTexOffset = 0;
        int32 xCoord     = xOrigin;
        const float zRow   = dzdy   * rowF;
        const float bigRow = dBigdy * rowF;
        const float smlRow = rowF   * dSmldy;
        const float yF     = (float)yCoord;

        for (int32 col = 0; col <= numInRow; col++) {
            const float colF = (float)col;

            float z     = dzdx  * colF + zRow   + zOrigin;
            float small = colF  * dSmldx + smlRow + smlOrig;
            const float xF = (float)xCoord;

            const float dxCam = camPos.x - xF;
            const float dyCam = camPos.y - yF;
            const float distToCam = std::sqrt(dxCam * dxCam + dyCam * dyCam) / (float)DETAILEDWATERDIST;

            float fadeMul;
            if (distToCam <= 1.0f) {
                fadeMul = 1.0f;
                if (distToCam > 0.75f) {
                    fadeMul = (1.0f - distToCam) * 4.0f;
                }
            } else {
                fadeMul = 0.0f;
            }

            if (WaterLayer == 0) {
                float colorMult, glare;
                CalculateWavesOnlyForCoordinate(
                    xCoord, yCoord,
                    fadeMul * (dBigdx * colF + bigRow + bigOrig),
                    small * fadeMul,
                    z, colorMult, glare, ms_vecWaterWaveNormal
                );

                auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored];
                vtx.u         = (float)xTexOffset * 0.08f + texU;
                vtx.v         = (float)yTexOffset * 0.08f + texV;
                vtx.objVertex = { xF, yF, z };

                // NOTE: Unlike the rectangle path, OG uses a constant color multiplier here
                const uint8 r = (uint8)(int32)((float)WaterColor.r * 0.577f);
                const uint8 g = (uint8)(int32)((float)WaterColor.g * 0.577f);
                const uint8 b = (uint8)(int32)((float)WaterColor.b * 0.577f);

                ms_TempVertexColorR[ms_nTempVertexColorIdx] = r;
                ms_TempVertexColorG[ms_nTempVertexColorIdx] = g;
                ms_TempVertexColorB[ms_nTempVertexColorIdx] = b;
                ms_nTempVertexColorIdx++;

                vtx.color = ((WaterLayerAlpha[0] & 0xFF) << 24) | ((uint32)r << 16) | ((uint32)g << 8) | b;
            } else if (WaterLayer == 1) {
                auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored];
                vtx.u = (float)xTexOffset * 0.04f + texU;
                vtx.v = (float)yTexOffset * 0.04f + texV;

                const uint32 r = ms_TempVertexColorR[ms_nTempVertexColorIdx];
                const uint32 g = ms_TempVertexColorG[ms_nTempVertexColorIdx];
                const uint32 b = ms_TempVertexColorB[ms_nTempVertexColorIdx];
                ms_nTempVertexColorIdx++;

                vtx.color = ((WaterLayerAlphaRand & 0xFF) << 24) | (r << 16) | (g << 8) | b;
            }
            // TODO(re): OG also has a `WaterLayer == 2` debug branch here (jittered UVs, grey color,
            // z offset by 0.1), but no caller ever passes it, so it's omitted.

            if (row != 0 && col != 0) {
                const uint16 v = (uint16)uiTempBufferVerticesStored;
                auto& idx = aTempBufferIndices;
                const auto i = uiTempBufferIndicesStored;

                idx[i + 0] = (RxVertexIndex)v;
                idx[i + 1] = (RxVertexIndex)(v - 1);
                idx[i + 2] = (RxVertexIndex)(v + row - (uint16)halfSpan - 3);
                idx[i + 3] = (RxVertexIndex)v;
                idx[i + 4] = (RxVertexIndex)(v + row - (uint16)halfSpan - 2);
                idx[i + 5] = (RxVertexIndex)(v + row - (uint16)halfSpan - 3);

                uiTempBufferIndicesStored += 6;
            }

            uiTempBufferVerticesStored++;

            xTexOffset += dx;
            xCoord     += dx;
        }

        // Close the row with the single triangle left over by the shorter row above
        if (row != 0) {
            const uint16 v = (uint16)uiTempBufferVerticesStored;
            auto& idx = aTempBufferIndices;
            const auto i = uiTempBufferIndicesStored;

            idx[i + 0] = (RxVertexIndex)(v - 1);
            idx[i + 1] = (RxVertexIndex)(v + row - (uint16)halfSpan - 2);
            idx[i + 2] = (RxVertexIndex)(v + row - (uint16)halfSpan - 3);

            uiTempBufferIndicesStored += 3;
        }

        yTexOffset += dy;
        yCoord     += dy;
        numInRow--;
    }
}

// 0x6E6870
void CWaterLevel::RenderSeaBedSegment(int32 blockX, int32 blockY, float x1, float x2, float y1, float y2) {
    const float u1 = x1 * 8.0f;
    const float v1 = y1 * 8.0f;
    const float px1 = ((float)blockX + x1) * 500.0f - 3000.0f;
    const float py1 = ((float)blockY + y1) * 500.0f - 3000.0f;
    const float py2 = ((float)blockY + y2) * 500.0f - 3000.0f;
    const float u2  = x2 * 8.0f;
    const float v2  = y2 * 8.0f;
    const float px2 = ((float)blockX + x2) * 500.0f - 3000.0f;

    constexpr uint32 COLOR = 0xFF505050;

    {
        auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored + 0];
        vtx.u = u1;
        vtx.v = v1;
        vtx.objVertex = { px1, py1, -70.0f };
        vtx.color = COLOR;
    }
    {
        auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored + 1];
        vtx.u = u1;
        vtx.v = v2;
        vtx.objVertex = { px1, py2, -70.0f };
        vtx.color = COLOR;
    }
    {
        auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored + 2];
        vtx.u = u2;
        vtx.v = v1;
        vtx.objVertex = { px2, py1, -70.0f };
        vtx.color = COLOR;
    }
    {
        auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored + 3];
        vtx.u = u2;
        vtx.v = v2;
        vtx.objVertex = { px2, py2, -70.0f };
        vtx.color = COLOR;
    }

    const auto base = (RxVertexIndex)uiTempBufferVerticesStored;
    auto& idx = aTempBufferIndices;
    const auto i = uiTempBufferIndicesStored;
    idx[i + 0] = base;
    idx[i + 1] = base + 1;
    idx[i + 2] = base + 2;
    idx[i + 3] = base + 3;
    idx[i + 4] = base + 1;
    idx[i + 5] = base + 2;

    uiTempBufferVerticesStored += 4;
    uiTempBufferIndicesStored  += 6;
}

// 0x6E6A10
void CWaterLevel::RenderDetailedSeaBedSegment(int32 blockX, int32 blockY, float x1, float x2, float y1, float y2) {
    const int32 numX = std::max(1, (int32)((x2 - x1) * 4.0f));
    const int32 numY = std::max(1, (int32)((y2 - y1) * 4.0f));

    constexpr uint32 COLOR = 0xFF505050;

    for (int32 ix = 0; ix < numX; ix++) {
        const float fx0 = (float)ix       * (x2 - x1) / (float)numX + x1;
        const float fx1 = (float)(ix + 1) * (x2 - x1) / (float)numX + x1;

        const float u0 = fx0 * 8.0f;
        const float u1 = fx1 * 8.0f;
        const float px0 = ((float)blockX + fx0) * 500.0f - 3000.0f;
        const float px1 = ((float)blockX + fx1) * 500.0f - 3000.0f;

        for (int32 iy = 0; iy < numY; iy++) {
            const float fy0 = (float)iy       * (y2 - y1) / (float)numY + y1;
            const float fy1 = (float)(iy + 1) * (y2 - y1) / (float)numY + y1;

            const float v0 = fy0 * 8.0f;
            const float v1 = fy1 * 8.0f;
            const float py0 = ((float)blockY + fy0) * 500.0f - 3000.0f;
            const float py1 = ((float)blockY + fy1) * 500.0f - 3000.0f;

            const auto base = (RxVertexIndex)uiTempBufferVerticesStored;

            {
                auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored + 0];
                vtx.u = u0; vtx.v = v0;
                vtx.objVertex = { px0, py0, -70.0f };
                vtx.color = COLOR;
            }
            {
                auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored + 1];
                vtx.u = u0; vtx.v = v1;
                vtx.objVertex = { px0, py1, -70.0f };
                vtx.color = COLOR;
            }
            {
                auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored + 2];
                vtx.u = u1; vtx.v = v0;
                vtx.objVertex = { px1, py0, -70.0f };
                vtx.color = COLOR;
            }
            {
                auto& vtx = TempBufferVertices.m_3d[uiTempBufferVerticesStored + 3];
                vtx.u = u1; vtx.v = v1;
                vtx.objVertex = { px1, py1, -70.0f };
                vtx.color = COLOR;
            }

            auto& idx = aTempBufferIndices;
            const auto i = uiTempBufferIndicesStored;
            idx[i + 0] = base;
            idx[i + 1] = base + 1;
            idx[i + 2] = base + 2;
            idx[i + 3] = base + 3;
            idx[i + 4] = base + 1;
            idx[i + 5] = base + 2;

            uiTempBufferVerticesStored += 4;
            uiTempBufferIndicesStored  += 6;
        }
    }
}

// 0x6E7680
void CWaterLevel::RenderAndEmptyRenderBuffer() {
    if (uiTempBufferVerticesStored) {
        ms_nWaterFlushCount++; // `LittleTest`
        if (RwIm3DTransform(TempBufferVertices.m_3d, uiTempBufferVerticesStored, nullptr, rwIM3D_VERTEXUV)) {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, uiTempBufferIndicesStored);
            RwIm3DEnd();
        }
    }
    uiTempBufferVerticesStored = 0;
    uiTempBufferIndicesStored  = 0;
}

// 0x6EF650
void CWaterLevel::RenderWater() {
    ZoneScoped;

    if (!CGame::CanSeeWaterFromCurrArea()) {
        return;
    }

    SetCameraRange();
    DefinedState();

    uiTempBufferVerticesStored = 0;
    uiTempBufferIndicesStored  = 0;

    // ---- Pass 1: the sea bed (Only the blocks that lie outside the world) ----
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,         RWRSTATE(seabd32Raster));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,             RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,              RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,             RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF,  RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,     RWRSTATE(TRUE));

    const auto& camPos = TheCamera.GetPosition();

    for (auto i = 0u; i < m_NumBlocksOutsideWorldToBeRendered; i++) {
        const int32 blockX = m_BlocksToBeRenderedOutsideWorldX[i];
        const int32 blockY = m_BlocksToBeRenderedOutsideWorldY[i];

        const float dx = camPos.x - (((float)blockX + 0.5f) * (float)WATER_BLOCK_SIZE - 3000.0f);
        const float dy = camPos.y - (((float)blockY + 0.5f) * (float)WATER_BLOCK_SIZE - 3000.0f);
        const bool  isFar = WaterFarDist <= std::sqrt(dx * dx + dy * dy);

        float x1 = 0.0f, x2 = 1.0f;
        float y1 = 0.0f, y2 = 1.0f;
        bool  renderX = false, renderY = false;

        const bool outsideWorld = blockX < 0 || blockX >= NUM_WATER_BLOCKS_ROWCOL
                               || blockY < 0 || blockY >= NUM_WATER_BLOCKS_ROWCOL;
        if (!outsideWorld) {
            // Blocks at the edges of the world are faded out towards it
            if (blockX == 0) {
                x1      = 0.04f;
                renderX = true;
            } else if (blockX == NUM_WATER_BLOCKS_ROWCOL - 1) {
                x1      = 0.96f;
                x2      = 1.0f;
                renderX = true;
            }
            if (blockY == 0) {
                y1      = 0.04f;
                renderY = true;
            } else if (blockY == NUM_WATER_BLOCKS_ROWCOL - 1) {
                y1      = 0.96f;
                y2      = 1.0f;
                renderY = true;
            }
        }

        const auto RenderSegment = [&](float sx1, float sx2, float sy1, float sy2) {
            if (isFar) {
                RenderSeaBedSegment(blockX, blockY, sx1, sx2, sy1, sy2);
            } else {
                RenderDetailedSeaBedSegment(blockX, blockY, sx1, sx2, sy1, sy2);
            }
        };

        if (outsideWorld || renderX) {
            RenderSegment(x1, x2, 0.0f, 1.0f);
        }
        if (renderY) {
            RenderSegment(0.0f, 1.0f, y1, y2);
        }
    }

    RenderAndEmptyRenderBuffer();

    // ---- Water surface ----
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,    RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,  RWRSTATE(WaterTextureAddressMode));

    // Drift the texture along the current flow
    const float flowStepX = CTimer::GetTimeStep() * m_CurrentFlow.x * WaterTexFlowScale;
    const float flowStepY = CTimer::GetTimeStep() * m_CurrentFlow.y * WaterTexFlowScale;

    waterTexOfsU0 += flowStepX * 0.08f;
    waterTexOfsV0 += flowStepY * 0.08f;
    if (waterTexOfsU0 > 1.0f) waterTexOfsU0 -= 1.0f;
    if (waterTexOfsV0 > 1.0f) waterTexOfsV0 -= 1.0f;

    waterTexOfsU1 += flowStepX * 0.04f;
    waterTexOfsV1 += flowStepY * 0.04f;
    if (waterTexOfsU1 > 1.0f) waterTexOfsU1 -= 1.0f;
    if (waterTexOfsV1 > 1.0f) waterTexOfsV1 -= 1.0f;

    const float angle0 = (float)(CTimer::GetTimeInMS() & 0xFFF) * 0.0015339808f;
    const float sin0   = std::sin(angle0);
    const float cos0   = std::cos(angle0);

    TextureShiftSecondU = sin0 * CWeather::Wavyness * 0.08f + waterTexOfsU0;
    TextureShiftSecondV = cos0 * CWeather::Wavyness * 0.08f + waterTexOfsV0;

    const float angle1 = (float)(CTimer::GetTimeInMS() & 0x1FFF) * 0.0007669904f;
    TextureShiftFirstU = waterTexOfsU1;

    const float cos1 = std::cos(angle1);
    TextureShiftFirstV = cos1 * 0.024f + waterTexOfsV1;

    TextureShiftJitterU = (float)rand() * (1.0f / 32768.0f) * WaterTexJitter;
    TextureShiftJitterU += std::sin(angle1) * WaterTexJitterAmp;
    TextureShiftJitterV = cos1 * WaterTexJitterAmp + (float)rand() * (1.0f / 32768.0f) * WaterTexJitter;

    // Randomize the water tint (The alpha is left as-is)
    WaterColor.r = (uint8)(int32)ms_fWaterColorRandomR;
    WaterColor.g = (uint8)(int32)ms_fWaterColorRandomG;
    WaterColor.b = (uint8)(int32)ms_fWaterColorRandomB;

    WaterColorTriangle.r = WaterColor.r;
    WaterColorTriangle.g = WaterColor.g;
    WaterColorTriangle.b = WaterColor.b;

    WaterLayerAlphaRand = (uint32)(int32)(ms_fWaterColorRandomA * 0.5f);

    const int32 layerAlpha = (int32)((WaterLayerAlphaRand << 8) / (0x100 - WaterLayerAlphaRand));
    WaterLayerAlpha[0] = 0xFF;
    if (layerAlpha < 0x100) {
        WaterLayerAlpha[0] = layerAlpha;
    }

    // ---- Pass 2: The marked water polygons ----
    uiTempBufferVerticesStored = 0;
    uiTempBufferIndicesStored  = 0;

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(waterclear256Raster));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,         RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));

    for (auto i = 0u; i < NumWaterTriangles; i++) {
        auto& tri = WaterTriangles[i];
        if (!tri.bToBeRendered) {
            continue;
        }

        const auto& v0 = tri.GetVertex(0);
        const auto& v1 = tri.GetVertex(1);
        const auto& v2 = tri.GetVertex(2);

        RenderWaterTriangle(v0.x, v0.y, v0.rp, v1.x, v1.y, v1.rp, v2.x, v2.y, v2.rp);
        tri.bToBeRendered = false;
    }

    for (auto i = 0u; i < NumWaterQuads; i++) {
        auto& quad = WaterQuads[i];
        if (!quad.bToBeRendered) {
            continue;
        }

        if (bRandomizeWaterColorPerQuad) {
            WaterColor.r = (uint8)(int32)ms_fWaterColorRandomR;
            WaterColor.g = (uint8)(int32)ms_fWaterColorRandomG;
            WaterColor.b = (uint8)(int32)ms_fWaterColorRandomB;
            WaterColor.a = (uint8)(int32)ms_fWaterColorRandomA;

            WaterColorTriangle = WaterColor;
        }

        const auto& v0 = quad.GetVertex(0);
        const auto& v1 = quad.GetVertex(1);
        const auto& v2 = quad.GetVertex(2);
        const auto& v3 = quad.GetVertex(3);

        RenderWaterRectangle(v0.x, v1.x, v0.y, v2.y, v0.rp, v1.rp, v2.rp, v3.rp);
        quad.bToBeRendered = false;
    }

    // ---- Pass 3: The sea bed once more, to fill in whatever the surface didn't cover ----
    for (auto i = 0u; i < m_NumBlocksOutsideWorldToBeRendered; i++) {
        const int32 blockX = m_BlocksToBeRenderedOutsideWorldX[i];
        const int32 blockY = m_BlocksToBeRenderedOutsideWorldY[i];

        if (blockX < 0 || blockX >= NUM_WATER_BLOCKS_ROWCOL || blockY < 0 || blockY >= NUM_WATER_BLOCKS_ROWCOL) {
            // TODO(re): `0x6EFEAC` builds all four `CRenPar`s as `{0, 1.0f, 0, 0}` and the four
            // coordinate arguments from the block's corners; the exact argument pairing couldn't be
            // recovered from the stack arithmetic.
            const CRenPar par{ 0.0f, 1.0f, 0.0f };
            RenderWaterRectangle(
                blockX * WATER_BLOCK_SIZE - 3000, blockX * WATER_BLOCK_SIZE + WATER_BLOCK_SIZE - 3000,
                blockY * WATER_BLOCK_SIZE - 3000, blockY * WATER_BLOCK_SIZE + WATER_BLOCK_SIZE - 3000,
                par, par, par, par
            );
        }
    }

    RenderAndEmptyRenderBuffer();

    RenderBoatWakes();
    DefinedState();
}

void CWaterLevel::SyncWater() {
    m_nWaterTimeOffset = CTimer::GetTimeInMS();
}

// NOTSA
bool CWaterLevel::IsPointUnderwaterNoWaves(const CVector& point) {
    float level{};
    if (GetWaterLevelNoWaves(point, &level, nullptr, nullptr))
        return level > point.z;
    return false;
}

bool CWaterLevel::GetWaterLevel(const CVector& pos, float& outWaterLevel, bool touchingWater, CVector* normals) {
    return GetWaterLevel(pos.x, pos.y, pos.z, outWaterLevel, touchingWater, normals);
}

// 0x6E5A40
uint32 CWaterLevel::AddWaterLevelVertex(int32 X, int32 Y, CRenPar P) {
    // Make sure point is inside world bounds
    if (CVector2D pt{ (float)X, (float)Y }; WORLD_BOUNDS.DoConstrainPoint(pt)) {
        X = (int32)pt.x;
        Y = (int32)pt.y;

        P = {};
    }

    // Try finding a vertex with the same coords, and use that
    for (auto&& [id, vtx] : rngv::enumerate(m_aVertices | rng::views::take(NumWaterVertices))) {
        if (vtx.x == X && vtx.y == Y && vtx.rp.z == P.z) {
            return id;
        }
    }

    const auto idx = NumWaterVertices++;
    m_aVertices[idx] = { (int16)X, (int16)Y, P };
    return idx;
}

struct SortableVtx {
    SortableVtx(int32 x, int32 y, const CRenPar& rp) :
        idx{ CWaterLevel::AddWaterLevelVertex(x, y, rp) },
        x{ CWaterLevel::m_aVertices[idx].x },
        y{ CWaterLevel::m_aVertices[idx].y }
    {
    }

    uint32 idx;
    int32  x, y;
};

//! NOTSA
//! Sort vertices in clockwise order (With a few assumptions)
template<size_t N>
auto DoVtxSortAndGetRange(SortableVtx (&verts)[N]) {
    const auto VertexComparator = [&](SortableVtx& a, SortableVtx& b) {
        if (a.y == b.y) {
            return a.x < b.x; // Sort by x if y is the same
        }
        return a.y < b.y; // Otherwise, sort by y
    };

    rng::sort(verts, VertexComparator);

    // Return a range of vertex indices that can be passed to the constructor of `CWaterPolygon`
    return verts | rng::views::transform([](auto& vtx) {
        return vtx.idx;
    });
}

// 0x6E7EF0
void CWaterLevel::AddWaterLevelQuad(int32 X1, int32 Y1, CRenPar P1, int32 X2, int32 Y2, CRenPar P2, int32 X3, int32 Y3, CRenPar P3, int32 X4, int32 Y4, CRenPar P4, uint32 Flags) {
    if ((X1 == X2 && X1 == X3 && X1 == X4) || (Y1 == Y2 && Y1 == Y3 && Y1 == Y4)) {
        return;
    }

    // Seemingly only axis aligned rectangles can be used as quads
    // NOTSA: to verify the above.
    assert(X1 == X2 || X1 == X3 || X1 == X4 || X2 == X3 || X2 == X4 || X3 == X4);
    assert(Y1 == Y2 || Y1 == Y3 || Y1 == Y4 || Y2 == Y3 || Y2 == Y4 || Y3 == Y4);

    SortableVtx verts[]{
        {X1,  Y1, P1},
        { X2, Y2, P2},
        { X3, Y3, P3},
        { X4, Y4, P4},
    };

    // Now actually create the quad
    WaterQuads[NumWaterQuads++] = CWaterQuad{
        (Flags & 1) == 0,
        (Flags & 2) != 0,
        DoVtxSortAndGetRange(verts)
    };
}

// 0x6E7D40
void CWaterLevel::AddWaterLevelTriangle(int32 X1, int32 Y1, CRenPar P1, int32 X2, int32 Y2, CRenPar P2, int32 X3, int32 Y3, CRenPar P3, uint32 Flags) {
    if ((X1 == X2 && X1 == X3) || (Y1 == Y2 && Y1 == Y3)) {
        return;
    }

    // Sorting seemingly always cares about only 2 of 3 vertices, looking at water.dat, in every single case 2 of 3 vertices have the same y coordinate,
    // I assume that's a limitation, and at least 2 of 3 vertices need to fall on the same x/y axis.
    // NOTSA: to verify the above.
    assert(X1 == X2 || X1 == X3 || X2 == X3);
    assert(Y1 == Y2 || Y1 == Y3 || Y2 == Y3);

    SortableVtx verts[]{
        {X1,  Y1, P1},
        { X2, Y2, P2},
        { X3, Y3, P3},
    };

    int16_t indices[3];
    if (verts[0].y == verts[1].y) {
        if (verts[0].x < verts[1].x) {
            indices[0] = verts[0].idx;
            indices[1] = verts[1].idx;
        } else {
            indices[0] = verts[1].idx;
            indices[1] = verts[0].idx;
        }
        indices[2] = verts[2].idx;
    } else if (verts[0].y == verts[2].y) {
        if (verts[0].x >= verts[2].x) {
            indices[0] = verts[2].idx;
            indices[1] = verts[0].idx;
        } else {
            indices[0] = verts[0].idx;
            indices[1] = verts[2].idx;
        }
        indices[2] = verts[1].idx;
    } else {
        if (verts[1].x >= verts[2].x) {
            indices[0] = verts[2].idx;
            indices[1] = verts[1].idx;
        } else {
            indices[0] = verts[1].idx;
            indices[1] = verts[2].idx;
        }
        indices[2] = verts[0].idx;
    }

    // Now actually create the triangle
    WaterTriangles[NumWaterTriangles++] = CWaterTriangle{
        (Flags & 1) == 0,
        (Flags & 2) != 0,
        indices | rng::views::all
    };
}

// 0x6E5750
void CWaterLevel::AddToQuadsAndTrianglesList(int32 blockX, int32 blockY, int32 polyIdx, uint32 type) {
    using PType = PolyInfo::PType;

    auto& info = m_BlockPolyInfo[blockX][blockY];
    if (info.Type() == PType::NONE) {
        info = PolyInfo{ (PType)type, (uint16)polyIdx };
    } else if (info.Type() == PType::COMBO) {
        // Append to the existing combo
        m_PolyCombos[NumWaterZonePolys]     = PolyInfo{ (PType)type, (uint16)polyIdx };
        m_PolyCombos[NumWaterZonePolys + 1] = PolyInfo{};
        NumWaterZonePolys++;
    } else {
        // Convert the single polygon into a combo
        m_PolyCombos[NumWaterZonePolys]     = info;
        m_PolyCombos[NumWaterZonePolys + 1] = PolyInfo{ (PType)type, (uint16)polyIdx };
        m_PolyCombos[NumWaterZonePolys + 2] = PolyInfo{};

        const auto comboIdx = (uint16)NumWaterZonePolys;
        NumWaterZonePolys += 3;

        info = PolyInfo{ PType::COMBO, comboIdx };
    }
}

// 0x6E7B30
void CWaterLevel::FillQuadsAndTrianglesList() {
    for (int32 blockX = 0, x = 0; x < NUM_WATER_BLOCKS_ROWCOL * WATER_BLOCK_SIZE; x += WATER_BLOCK_SIZE, blockX++) {
        const float minX = (float)x - 3000.0f;
        const float maxX = minX + (float)WATER_BLOCK_SIZE;

        for (int32 blockY = 0, y = 0; y < NUM_WATER_BLOCKS_ROWCOL * WATER_BLOCK_SIZE; y += WATER_BLOCK_SIZE, blockY++) {
            const float minY = (float)y - 3000.0f;
            const float maxY = minY + (float)WATER_BLOCK_SIZE;

            for (auto i = 0u; i < NumWaterQuads; i++) {
                const auto& quad = WaterQuads[i];
                if (minX < (float)m_aVertices[quad.verts[1]].x && (float)m_aVertices[quad.verts[0]].x < maxX
                 && minY < (float)m_aVertices[quad.verts[2]].y && (float)m_aVertices[quad.verts[0]].y < maxY
                ) {
                    AddToQuadsAndTrianglesList(blockX, blockY, i, 1);
                }
            }

            for (auto i = 0u; i < NumWaterTriangles; i++) {
                const auto& tri = WaterTriangles[i];

                const int32 vtxY0 = m_aVertices[tri.verts[0]].y;
                const int32 vtxY1 = m_aVertices[tri.verts[1]].y;

                if (minX < (float)m_aVertices[tri.verts[2]].x && (float)m_aVertices[tri.verts[0]].x < maxX
                 && minY < (float)std::max(vtxY0, vtxY1) && (float)std::min(vtxY0, vtxY1) < maxY
                ) {
                    AddToQuadsAndTrianglesList(blockX, blockY, i, 2);
                }
            }
        }
    }
}

// 0x6E9C80
void CWaterLevel::SetCameraRange() {
    if (DontUpdateCameraRange) {
        return;
    }

    const auto& cmpos = TheCamera.GetPosition();

    const auto CalcMin = [](float p) { return 2 * (int32)std::floor((p - (float)DETAILEDWATERDIST) / 2.f); };
    const auto CalcMax = [](float p) { return 2 * (int32)std::ceil((p + (float)DETAILEDWATERDIST) / 2.f); };

    CameraRangeMinX = CalcMin(cmpos.x);
    CameraRangeMaxX = CalcMax(cmpos.x);

    CameraRangeMinY = CalcMin(cmpos.y);
    CameraRangeMaxY = CalcMax(cmpos.y);
}

// 0x6EAB50
void CWaterLevel::HandleBeachToysStuff() {
    /* nothing special (10 lines), but it uses 3 static variables, and they aren't used anywhere else, so I won't bother */
}

template<size_t NumVerts>
CWaterVertex CWaterPolygon<NumVerts>::GetVertex(uint16 idx) const {
    return CWaterLevel::m_aVertices[verts[idx]];
}
