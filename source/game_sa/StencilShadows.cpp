#include "StdInc.h"
#include "StencilShadowObject.h"

// TODO: Statically allocate after reversing RenderForVehicle&RenderForObject.
static inline auto& s_ShadowTrianglePointsUnk         = StaticRef<RxVertexIndex*>(0xC6A170);
static inline auto& s_ShadowTrianglePoints            = StaticRef<CVector*>(0xC6A174);
static inline auto& s_TransformedShadowTrianglePoints = StaticRef<CVector*>(0xC6A178);
static inline auto& s_SunPosNrm                       = StaticRef<CVector>(0x8D5244); // CVector(1.0, 1.0, -2.0)

namespace {
//! Capacity (In `CVector`s) of `s_ShadowTrianglePoints` and `s_TransformedShadowTrianglePoints` (Allocated by `Process`)
constexpr uint32 NUM_SHADOW_VERTICES = 2'048;
//! Capacity (In edges) of `s_ShadowTrianglePointsUnk` (Allocated by `Process`)
constexpr uint32 NUM_SHADOW_EDGES = 6'144;

//! A shadow object is kept alive for as long as this metric stays below this value,
//! which (Since the metric is `(d - r)^2`) means the camera is no further than 50 units from the owner's bounds.
constexpr float MAX_SHADOW_DISTANCE = 2'500.0f;

/*!
 * @addr 0x711160, 0x7111F0 (The two are identical)
 * @brief Distance metric (Being `sign(d - r) * (d - r)^2`, where `d` is the distance from `cameraPos` to the
 *        entity's bounding sphere center, and `r` the sphere's radius) used to decide whether a shadow object should exist.
 * @note Both originals read `entity` from `EBX` and `cameraPos` from `ESI`.
 */
float GetShadowDistanceMetric(const CEntity* entity, const CVector& cameraPos) {
    const auto* const colModel = entity->GetColModel();
    const CVector     center   = entity->TransformFromObjectSpace(colModel->m_boundSphere.m_vecCenter);
    const float       distance = (cameraPos - center).Magnitude();
    const float       radius   = colModel->m_boundSphere.m_fRadius;
    return (distance < radius ? -1.0f : 1.0f) * (distance - radius) * (distance - radius);
}

/*!
 * @addr 0x70FA70
 * @brief Appends the edge `(a, b)` to `edges`, or removes it if it's already there (In either orientation).
 * @param edges     The array of edges (Each edge being 2 indices in `s_ShadowTrianglePointsUnk`)
 * @param numEdges  The number of edges currently in the array
 */
void AddOrRemoveEdge(RxVertexIndex* const edges, uint16& numEdges, RxVertexIndex a, RxVertexIndex b) {
    for (auto i = 0; i < numEdges; i++) {
        const auto* const edge = &edges[i * 2];
        if ((edge[0] == a && edge[1] == b) || (edge[0] == b && edge[1] == a)) {
            if (numEdges > 1) { // Move the last edge into this one's place
                edges[i * 2 + 0] = edges[(numEdges - 1) * 2 + 0];
                edges[i * 2 + 1] = edges[(numEdges - 1) * 2 + 1];
            }
            numEdges--;
            return;
        }
    }

#ifdef FIX_BUGS
    if (static_cast<uint32>(numEdges) >= NUM_SHADOW_EDGES) { // The original had no bounds check here, and would've overflowed the buffer
        return;
    }
#endif

    edges[numEdges * 2 + 0] = a;
    edges[numEdges * 2 + 1] = b;
    numEdges++;
}

/*!
 * @addr 0x710BA0
 * @return The active shadow object of `entity`, or null if there's none.
 */
CStencilShadowObject* FindShadowObject(CEntity* const entity) {
    return plugin::CallAndReturn<CStencilShadowObject*, 0x710BA0, CEntity*>(entity);
}

/*!
 * @addr 0x711280
 * @brief Initializes the shadow object `object` (Which must be taken from the free list) for `entity`.
 * @param type Either `eStencilShadowObjType::OBJECT` or `eStencilShadowObjType::VEHICLE`.
 *             (Passed as `int` because that's how the original passes it)
 * @return The created shadow object, or null if `entity` can't have a shadow (No collision data, or no shadow triangles)
 */
CStencilShadowObject* CreateShadowObject(CStencilShadowObject* const object, CEntity* const entity, const int type) {
    return plugin::CallMethodAndReturn<CStencilShadowObject*, 0x711280, CStencilShadowObject*, CEntity*, int>(object, entity, type);
}

/*!
 * @return Whether `entity` can't have a (Valid) stencil shadow because it's an object that's hidden, exploded or broken.
 */
bool IsInvalidObjectShadowCaster(CEntity* const entity) {
    if (!entity->GetIsTypeObject()) {
        return false;
    }
    const auto* const object = entity->AsObject();
    return !entity->m_bIsVisible
        || object->objectFlags.bIsExploded
        || object->objectFlags.bIsBroken;
}

/*!
 * @brief Fills `object`'s vertex buffer with the (World space) shadow mesh of its owner.
 * @param mat       The owner's matrix (Object space -> World space)
 * @param extrude   How far the silhouette's edges are extruded (Along the sun's direction)
 * @note `RenderForObject` and `RenderForVehicle` only differ in how the matrix and `extrude` are chosen.
 */
bool BuildStencilShadowMesh(CStencilShadowObject* const object, CMatrix& mat, const float extrude) {
    const auto* const colData = object->m_pOwner->GetColModel()->m_pColData;

    // Sun direction in the object's space. (Subtracting the transformed origin cancels the matrix' translation out)
    CMatrix inverse;
    Invert(mat, inverse);
    const CVector sunDirObj = inverse.TransformPoint(s_SunPosNrm) - inverse.TransformPoint(CVector{});
    const CVector extrudeOffset = s_SunPosNrm * extrude; // World space

    const auto numTris = static_cast<int32>(object->m_NumShadowFaces);
#ifdef FIX_BUGS
    // The staging buffers allocated by `Process` can't hold more points than this
    const auto numVerts = std::min(static_cast<int32>(colData->m_nNumShadowVertices), static_cast<int32>(NUM_SHADOW_VERTICES));
#else
    const auto numVerts = static_cast<int32>(colData->m_nNumShadowVertices);
#endif

    object->m_SizeOfShadowFacesData = 0;

    // Decompress the collision model's shadow mesh, and transform it to world space
    for (auto i = 0; i < numVerts; i++) {
        colData->GetShadTrianglePoint(s_ShadowTrianglePoints[i], i);
    }
    TransformPoints(
        reinterpret_cast<RwV3d*>(s_TransformedShadowTrianglePoints),
        numVerts,
        mat,
        reinterpret_cast<RwV3d*>(s_ShadowTrianglePoints)
    );

    // Project every triangle, and gather the silhouette's edges along the way
    uint16 numEdges = 0;
    for (auto t = 0; t < numTris; t++) {
        const auto& tri = colData->m_pShadowTriangles[t];

        // Object space points (Used to test which way the triangle is facing)
        const auto& objA = s_ShadowTrianglePoints[tri.vA];
        const auto& objB = s_ShadowTrianglePoints[tri.vB];
        const auto& objC = s_ShadowTrianglePoints[tri.vC];

        // World space points (Used when writing the mesh)
        const auto& worldA = s_TransformedShadowTrianglePoints[tri.vA];
        const auto& worldB = s_TransformedShadowTrianglePoints[tri.vB];
        const auto& worldC = s_TransformedShadowTrianglePoints[tri.vC];

        if ((objB - objA).Cross(objC - objA).Dot(sunDirObj) < 0.0f) { // Facing away from the sun?
            if (object->m_FaceID <= object->m_SizeOfShadowFacesData + 3) { // Not enough room left
                break;
            }
            auto* const dst = &object->m_ShadowFacesData[object->m_SizeOfShadowFacesData];
            dst[0] = worldA + extrudeOffset;
            dst[1] = worldC + extrudeOffset;
            dst[2] = worldB + extrudeOffset;
        } else {
            AddOrRemoveEdge(s_ShadowTrianglePointsUnk, numEdges, tri.vA, tri.vB);
            AddOrRemoveEdge(s_ShadowTrianglePointsUnk, numEdges, tri.vB, tri.vC);
            AddOrRemoveEdge(s_ShadowTrianglePointsUnk, numEdges, tri.vC, tri.vA);

            if (object->m_FaceID <= object->m_SizeOfShadowFacesData + 3) { // Not enough room left
                break;
            }
            auto* const dst = &object->m_ShadowFacesData[object->m_SizeOfShadowFacesData];
            dst[0] = worldA;
            dst[1] = worldC;
            dst[2] = worldB;
        }
        object->m_SizeOfShadowFacesData += 3;
    }

    // Extrude the silhouette's edges into quads
    for (auto e = 0; e < numEdges; e++) {
        const auto& p0 = s_TransformedShadowTrianglePoints[s_ShadowTrianglePointsUnk[e * 2 + 0]];
        const auto& p1 = s_TransformedShadowTrianglePoints[s_ShadowTrianglePointsUnk[e * 2 + 1]];

        if (object->m_FaceID <= object->m_SizeOfShadowFacesData + 6) { // Not enough room left
            break;
        }
        auto* const dst = &object->m_ShadowFacesData[object->m_SizeOfShadowFacesData];
        dst[0] = p0;
        dst[1] = p1;
        dst[2] = p0 + extrudeOffset;
        dst[3] = p1;
        dst[4] = p1 + extrudeOffset;
        dst[5] = p0 + extrudeOffset;
        object->m_SizeOfShadowFacesData += 6;
    }

    return true;
}
} // anonymous namespace

void CStencilShadows::InjectHooks() {
    RH_ScopedClass(CStencilShadows);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x70F9E0);
    RH_ScopedInstall(Shutdown, 0x711390);
    RH_ScopedInstall(Process, 0x711D90);
    RH_ScopedInstall(GraphicsHighQuality, 0x70F9B0);
    RH_ScopedInstall(UpdateHierarchy, 0x710BC0);
    RH_ScopedInstall(RegisterStencilShadows, 0x711760);
    RH_ScopedInstall(RenderStencilShadows, 0x7113B0);
    RH_ScopedInstall(RenderForVehicle, 0x70FAE0);
    RH_ScopedInstall(RenderForObject, 0x710310);
    RH_ScopedInstall(Render, 0x710D50);
    RH_ScopedInstall(RenderBuffer, 0x710B50);
    RH_ScopedInstall(SunSetPositionFromEntity, 0x710AF0);
    RH_ScopedInstall(sub_710CC0, 0x710CC0);
}

// 0x70F9E0
void CStencilShadows::Init() {
    ZoneScoped;

#ifndef NOTSA_NATIVE
    RwD3D9SetStencilClear(0);
#endif
    pFirstAvailableStencilShadowObject = m_StencilShadowObjects.data();
    pFirstActiveStencilShadowObject = nullptr;

    for (auto&& [i, obj] : rngv::enumerate(m_StencilShadowObjects)) {
        obj.m_pOwner                = nullptr;
        obj.m_NumShadowFaces        = 0;
        obj.m_Type                  = eStencilShadowObjType::NONE;
        obj.m_SizeOfShadowFacesData = 0;
        obj.m_FaceID                = 0;
        obj.m_ShadowFacesData       = 0;

        obj.m_pPrev = i ? &m_StencilShadowObjects[i - 1] : nullptr;
        obj.m_pNext = (i != m_StencilShadowObjects.size() - 1) ? &m_StencilShadowObjects[i + 1] : nullptr;
    }
}

// 0x711390
void CStencilShadows::Shutdown() {
    for (auto* obj = pFirstActiveStencilShadowObject; obj;) {
        auto* next = obj->m_pNext;
        obj->Destroy();
        obj = next;
    }
}

// 0x710D50
bool CStencilShadows::Render(const CRGBA& color) {
    uiTempBufferIndicesStored = 0;
    uiTempBufferVerticesStored = 0;

    // RW expects the color to be packed as `rwRGBA`, i.e. `a << 24 | r << 16 | g << 8 | b`
    // (Note that this is NOT what `CRGBA::ToInt` returns)
    const auto packedColor = (uint32(color.a) << 24) | (uint32(color.r) << 16) | (uint32(color.g) << 8) | uint32(color.b);

    for (auto* shadow = pFirstActiveStencilShadowObject; shadow; shadow = shadow->m_pNext) {
        auto* faces = shadow->m_ShadowFacesData;

        // 6 vertex groups (2 triangles each)
        const auto numGroups = shadow->m_SizeOfShadowFacesData / 6;
        for (auto g = 0u; g < numGroups; g++, faces += 6) {
            sub_710CC0(6, 6);

            const auto firstVert = uiTempBufferVerticesStored;
            const auto firstIdx  = uiTempBufferIndicesStored;
            for (auto v = 0u; v < 6; v++) {
                auto& vert     = TempBufferVertices.m_3d[firstVert + v];
                vert.objVertex = faces[v];
                vert.color     = packedColor;

                aTempBufferIndices[firstIdx + v] = static_cast<RxVertexIndex>(firstIdx + v);
            }
            uiTempBufferVerticesStored += 6;
            uiTempBufferIndicesStored  += 6;
        }

        // An eventual leftover triangle (I.e. if the number of triangles was odd)
        if ((shadow->m_SizeOfShadowFacesData / 3) & 1) {
            sub_710CC0(3, 3);

            const auto firstVert = uiTempBufferVerticesStored;
            const auto firstIdx  = uiTempBufferIndicesStored;
            for (auto v = 0u; v < 3; v++) {
                auto& vert     = TempBufferVertices.m_3d[firstVert + v];
                vert.objVertex = faces[v];
                vert.color     = packedColor;

                aTempBufferIndices[firstIdx + v] = static_cast<RxVertexIndex>(firstIdx + v);
            }
            uiTempBufferVerticesStored += 3;
            uiTempBufferIndicesStored  += 3;
        }
    }

    if (uiTempBufferVerticesStored && uiTempBufferIndicesStored) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
        LittleTest();
        if (RwIm3DTransform(TempBufferVertices.m_3d, uiTempBufferVerticesStored, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, uiTempBufferIndicesStored);
            RwIm3DEnd();
        }
        uiTempBufferVerticesStored = 0;
        uiTempBufferIndicesStored  = 0;
    }

    return true;
}

// unused
// 0x710AF0
void CStencilShadows::SunSetPositionFromEntity(const CEntity* entity) {
    if (!entity) {
        return;
    }
    s_SunPosNrm = entity->GetPosition().Normalized();
}

// 0x710B50
void CStencilShadows::RenderBuffer(const CVector& pos) {
    s_SunPosNrm = pos.Normalized(); 
}

// 0x710CC0
void CStencilShadows::sub_710CC0(int32 indices, int32 vertices) {
    if (uiTempBufferIndicesStored + indices < TOTAL_TEMP_BUFFER_INDICES
        && uiTempBufferVerticesStored + vertices < TOTAL_TEMP_BUFFER_3DVERTICES) {
        return;
    }

    if (!uiTempBufferIndicesStored || !uiTempBufferVerticesStored) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
    LittleTest();
    if (RwIm3DTransform(TempBufferVertices.m_3d, uiTempBufferVerticesStored, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, uiTempBufferIndicesStored);
        RwIm3DEnd();
    }
    uiTempBufferVerticesStored = uiTempBufferIndicesStored = 0;
}

// 0x7113B0
void CStencilShadows::RenderStencilShadows() {
    ZoneScoped;

    if (!GraphicsHighQuality()) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEFLAT));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,                RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,            RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,                 RWRSTATE(rwBLENDZERO));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,                RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONMASK,      RWRSTATE(uint32(-1)));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONWRITEMASK, RWRSTATE(uint32(-1)));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTION,          RWRSTATE(rwSTENCILFUNCTIONALWAYS));
    RwRenderStateSet(rwRENDERSTATESTENCILFAIL,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILPASS,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONREF,       RWRSTATE(0));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONINCR));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLFRONT));

    Render(CRGBA{ 0, 0, 0, 255 });

    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONDECR));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));

    Render(CRGBA{ 0, 0, 0, 255 });

    // WTF is up with these states?
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(FALSE)); // same state
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,                RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(TRUE)); // same state
    RwRenderStateSet(rwRENDERSTATESRCBLEND,                 RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,                RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEFLAT));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,            RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLNONE));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONREF,       RWRSTATE(1u));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTION,          RWRSTATE(rwSTENCILFUNCTIONLESSEQUAL));
    RwRenderStateSet(rwRENDERSTATESTENCILFAIL,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILPASS,              RWRSTATE(rwSTENCILOPERATIONKEEP));

    CSprite2d::InitPerFrame();
    CSprite2d::DrawRect(
        CRect{ 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT },
        CRGBA{ 0, 0, 0, (uint8)(50u * CTimeCycle::m_CurrentColours.m_nShadowStrength / 256) }
    );

    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));
}

// 0x70FAE0
bool CStencilShadows::RenderForVehicle(CStencilShadowObject* object) {
    auto* const veh = static_cast<CVehicle*>(object->m_pOwner);

    const auto* const colData = veh->GetColModel()->m_pColData;
    if (!colData || static_cast<int32>(colData->m_nNumShadowTriangles) != object->m_NumShadowFaces) { // Col model changed since the object was created
        return false;
    }

    // Aircrafts get a much longer "shadow" than the rest
    const auto isAircraft = veh->GetType() == ENTITY_TYPE_VEHICLE
                         && (veh->m_nVehicleSubType == VEHICLE_TYPE_HELI || veh->m_nVehicleSubType == VEHICLE_TYPE_PLANE);
    const auto extrude = isAircraft ? 40.0f : 5.0f;

    auto* mat = &veh->GetMatrix();
    if (veh->GetType() == ENTITY_TYPE_VEHICLE && veh->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        auto* const bike = static_cast<CBike*>(veh);
        bike->CalculateLeanMatrix();
        mat = &bike->m_mLeanMatrix;
    }

    return BuildStencilShadowMesh(object, *mat, extrude);
}

// 0x710310
bool CStencilShadows::RenderForObject(CStencilShadowObject* object) {
    auto* const entity = object->m_pOwner;

    const auto* const colData = entity->GetColModel()->m_pColData;
    if (!colData || static_cast<int32>(colData->m_nNumShadowTriangles) != object->m_NumShadowFaces) { // Col model changed since the object was created
        return false;
    }

    return BuildStencilShadowMesh(object, entity->GetMatrix(), 60.0f);
}

// 0x711D90
void CStencilShadows::Process(CVector& cameraPos) {
    ZoneScoped;

    if (!GraphicsHighQuality()) {
        return;
    }

    static uint8 s_RegisterShadowCounter{}, s_RenderForObjCounter{};

    RegisterStencilShadows(cameraPos, (++s_RegisterShadowCounter % 8) != 0);

    // why do we even do this?
    s_ShadowTrianglePointsUnk         = (RxVertexIndex*)CMemoryMgr::Malloc(12'288 * sizeof(RxVertexIndex));
    s_ShadowTrianglePoints            = (CVector*)CMemoryMgr::Malloc(2'048 * sizeof(CVector));
    s_TransformedShadowTrianglePoints = (CVector*)CMemoryMgr::Malloc(2'048 * sizeof(CVector));

    auto i{ 0 };
    for (auto* obj = pFirstActiveStencilShadowObject; obj; obj = obj->m_pNext) {
        switch (obj->m_Type) {
        case eStencilShadowObjType::OBJECT:
            if ((i++ % 4) == s_RenderForObjCounter) {
                RenderForObject(obj);
            }
            break;
        case eStencilShadowObjType::VEHICLE:
            RenderForVehicle(obj);
            break;
        default:
            break;
        }
    }
    s_RenderForObjCounter = (s_RenderForObjCounter + 1) % 4;

    CMemoryMgr::Free(std::exchange(s_ShadowTrianglePointsUnk, nullptr));
    CMemoryMgr::Free(std::exchange(s_ShadowTrianglePoints, nullptr));
    CMemoryMgr::Free(std::exchange(s_TransformedShadowTrianglePoints, nullptr));
}

// 0x70F9B0
bool CStencilShadows::GraphicsHighQuality() {
    return ::GraphicsHighQuality();
}

// 0x710BC0
void CStencilShadows::UpdateHierarchy(CStencilShadowObject*& firstAvailable, CStencilShadowObject*& firstActive, CStencilShadowObject* newOne) {
    if (auto* prev = newOne->m_pPrev) {
        auto* next = newOne->m_pNext;
        if (next) {
            next->m_pPrev = prev;
            newOne->m_pPrev->m_pNext = newOne->m_pNext;
        } else {
            prev->m_pNext = nullptr;
        }
    } else {
        auto* next     = newOne->m_pNext;
        firstAvailable = next;
        if (next) {
            next->m_pPrev = nullptr;
        }
    }
    newOne->m_pNext = firstActive;
    newOne->m_pPrev = nullptr;
    firstActive     = newOne;

    if (newOne->m_pNext) {
        newOne->m_pNext->m_pPrev = newOne;
    }
}

// 0x711760
bool CStencilShadows::RegisterStencilShadows(CVector& cameraPos, bool doNotCreateNew) {
    if (doNotCreateNew) { // Only prune the active list
        for (auto* obj = pFirstActiveStencilShadowObject; obj;) {
            auto* const next  = obj->m_pNext;
            auto* const owner = obj->m_pOwner;
            if (!owner || IsInvalidObjectShadowCaster(owner)) {
                obj->Destroy();
            }
            obj = next;
        }
        return true;
    }

    // Prune the active list, and check up on the objects that are left
    for (auto* obj = pFirstActiveStencilShadowObject; obj;) {
        auto* const next  = obj->m_pNext;
        auto* const owner = obj->m_pOwner;
        if (!owner || IsInvalidObjectShadowCaster(owner)) {
            obj->Destroy();
        } else if (obj->m_Type == eStencilShadowObjType::OBJECT) {
            if (   GetShadowDistanceMetric(owner, cameraPos) > MAX_SHADOW_DISTANCE
                || !owner->IsInCurrentArea()
            ) {
                obj->Destroy();
            }
        } else if (obj->m_Type == eStencilShadowObjType::VEHICLE) {
            if (GetShadowDistanceMetric(owner, cameraPos) > MAX_SHADOW_DISTANCE) {
                obj->Destroy();
            }
        }
        obj = next;
    }

    if (!pFirstAvailableStencilShadowObject) {
        return true;
    }

    // Create shadow objects for the vehicles in the pool
    auto* const vehPool = GetVehiclePool();
    for (auto i = 0u; i < vehPool->GetSize(); i++) {
        auto* const veh = vehPool->GetAt(i);
        if (!veh) {
            continue;
        }

        const auto* const colModel = veh->GetColModel();
        if (!colModel) {
            continue;
        }
        const auto* const colData = colModel->m_pColData;
        if (!colData || !colData->bHasShadowInfo) {
            continue;
        }
        if (!veh->m_pCollisionList.m_node) { // Only vehicles that have collided with something have a shadow
            continue;
        }
        if (FindShadowObject(veh)) {
            continue;
        }
        if (GetShadowDistanceMetric(veh, cameraPos) > MAX_SHADOW_DISTANCE) {
            continue;
        }

        if (!pFirstAvailableStencilShadowObject) {
            return false;
        }
        CreateShadowObject(pFirstAvailableStencilShadowObject, veh, static_cast<int>(eStencilShadowObjType::VEHICLE));
        if (!pFirstAvailableStencilShadowObject) {
            return false;
        }
    }

    if (!pFirstAvailableStencilShadowObject) {
        return true;
    }

    // Scan the sectors in a 100 x 100 units window around the camera for objects to create shadows for
    // (The original rounds via `fistp`, so `std::lrint` - with the default round-to-nearest mode - matches it)
    const auto ToSectorMin = [](float coord) { return std::max(0, static_cast<int32>(std::lrint((coord - 50.0f) * 0.02f + 60.0f))); };
    const auto ToSectorMax = [](float coord) { return std::min(MAX_SECTORS_X - 1, static_cast<int32>(std::lrint((coord + 50.0f) * 0.02f + 60.0f))); };

    const auto minX = ToSectorMin(cameraPos.x);
    const auto minY = ToSectorMin(cameraPos.y);
    const auto maxX = ToSectorMax(cameraPos.x);
    const auto maxY = ToSectorMax(cameraPos.y);

    CWorld::AdvanceCurrentScanCode();

    //! @return `false` if the free list got exhausted (In which case the scan has to be aborted)
    const auto RegisterShadowCaster = [&](CEntity* entity) {
        const auto* const colModel = entity->GetColModel();
        if (!colModel) {
            return true;
        }
        const auto* const colData = colModel->m_pColData;
        if (!colData || !colData->bHasShadowInfo) {
            return true;
        }

        if (entity->GetScanCode() == CWorld::ms_nCurrentScanCode) { // Already visited
            return true;
        }
        entity->SetScanCode(CWorld::ms_nCurrentScanCode);

        if (!entity->IsInCurrentArea()) {
            return true;
        }

        if (CModelInfo::ms_modelInfoPtrs[entity->m_nModelIndex]->GetModelType() != MODEL_INFO_ATOMIC) {
            return true;
        }

        if (FindShadowObject(entity)) {
            return true;
        }

        if (GetShadowDistanceMetric(entity, cameraPos) > MAX_SHADOW_DISTANCE) {
            return true;
        }

        if (!pFirstAvailableStencilShadowObject) {
            return false;
        }
        CreateShadowObject(pFirstAvailableStencilShadowObject, entity, static_cast<int>(eStencilShadowObjType::OBJECT));
        return pFirstAvailableStencilShadowObject != nullptr;
    };

    for (auto y = minY; y <= maxY; y++) {
        for (auto x = minX; x <= maxX; x++) {
            // First the sector's buildings...
            for (auto* node = CWorld::GetSector(x, y).Buildings.GetNode(); node; node = node->Next) {
                auto* const entity = node->Item;
                if (entity->m_bIsProcObject || IsInvalidObjectShadowCaster(entity)) {
                    continue;
                }
                if (!RegisterShadowCaster(entity)) {
                    return false;
                }
            }
            // ...and then the repeat sector's objects
            for (auto* node = CWorld::GetRepeatSector(x, y).Objects.GetNode(); node; node = node->Next) {
                auto* const entity = node->Item;
                if (entity->m_bIsProcObject || IsInvalidObjectShadowCaster(entity)) {
                    continue;
                }
                if (!RegisterShadowCaster(entity)) {
                    return false;
                }
            }
        }
    }

    return true;
}
