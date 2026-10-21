#include "StdInc.h"

#include "FxPrimBP.h"
#include "FxTools.h"

void FxPrimBP_c::InjectHooks() {
    RH_ScopedClass(FxPrimBP_c);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(GetRWMatrix, 0x4A9DC0);
    RH_ScopedInstall(Load, 0x5C2010);
}

// 0x4A9CF0
FxPrimBP_c::FxPrimBP_c() {
    m_apTextures.fill(nullptr);
}

// 0x4A9D20
FxPrimBP_c::~FxPrimBP_c() {
    // FIX_BUGS: They forgot destroy third texture (texture 3 destroyed twice)
    for (auto& texture : m_apTextures) {
        RwTextureDestroy(texture);
        texture = nullptr;
    }
}

// 0x4A9DC0
void FxPrimBP_c::GetRWMatrix(RwMatrix& outMatrix) {
    if (m_pMatrixBuffered) {
        m_pMatrixBuffered->CopyToRwMatrix(outMatrix);
    } else {
        RwMatrixSetIdentity(&outMatrix);
    }
}

// 0x5C2010
bool FxPrimBP_c::Load(FILESTREAM file, int32 version, FxName32_t* textureNames) {
    char line[256], field[128];

    ReadField<void>(file);               // `FX_PRIM_BASE_DATA:` (Content is discarded)
    ReadFieldImpl(file, field, "NAME:"); // Name is parsed and then discarded (Retail scans it into `field` twice)

    // Matrix line: right, up, at, pos
    CVector mat[4];
    ReadLine(file, line, sizeof(line));
    VERIFY(sscanf(
        line,
        "%s %f %f %f %f %f %f %f %f %f %f %f %f",
        field,
        &mat[0].x, &mat[0].y, &mat[0].z,
        &mat[1].x, &mat[1].y, &mat[1].z,
        &mat[2].x, &mat[2].y, &mat[2].z,
        &mat[3].x, &mat[3].y, &mat[3].z
    ) == 13);

    // If it's the identity matrix, don't allocate it
    if (mat[0] == CVector{1.0f, 0.0f, 0.0f} &&
        mat[1] == CVector{0.0f, 1.0f, 0.0f} &&
        mat[2] == CVector{0.0f, 0.0f, 1.0f} &&
        mat[3] == CVector{0.0f, 0.0f, 0.0f}
    ) {
        m_pMatrixBuffered = nullptr;
    } else {
        // NOTE: Retail allocates with alignment 2 (Not 4 as `FxManager_c::Allocate` would),
        // which affects the pool cursor (And hence the addresses of later allocations)
        m_pMatrixBuffered = static_cast<FxBufferedMatrix*>(g_fxMan.GetMemPool().GetMem(sizeof(FxBufferedMatrix), 2));
        m_pMatrixBuffered->CopyFromVectors(mat[0], mat[1], mat[2], mat[3]);
    }

    assert(textureNames);

    // NOTSA/FIX_BUGS: Retail scans the names with an unbounded `%s`, overflowing the 32-byte
    // buffers if a name in the file is longer than that.
    const auto ReadTextureName = [file](FxName32_t& textureName, const char* fieldName) {
        char line[256], field[128];
        ReadLine(file, line, sizeof(line));
#ifdef FIX_BUGS
        const auto numScanned = sscanf(line, "%s %31s", field, textureName);
#else
        const auto numScanned = sscanf(line, "%s %s", field, textureName);
#endif
        VERIFY(numScanned == 2);
        if (strcmp(field, fieldName) != 0) {
            NOTSA_UNREACHABLE("Fx Project field mismatch!\nExpected:\t{}\nGot:\t{}", fieldName, field);
        }
    };
    ReadTextureName(textureNames[0], "TEXTURE:");

    if (version > 101) {
        ReadTextureName(textureNames[1], "TEXTURE2:");
        ReadTextureName(textureNames[2], "TEXTURE3:");
        ReadTextureName(textureNames[3], "TEXTURE4:");
    }

    // Only the low byte is stored (Retail does `(uint8)ReadIntField`)
    m_bAlphaOn    = (uint8)ReadField<int32>(file, "ALPHAON:") != 0;
    m_nSrcBlendId = ReadField<uint8>(file, "SRCBLENDID:");
    m_nDstBlendId = ReadField<uint8>(file, "DSTBLENDID:");

    m_FxInfoManager.Load(file, version);
    return true;
}
