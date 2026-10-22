#include "StdInc.h"

#include "RwHelper.h"

void RwHelperInjectHooks() {
    RH_ScopedNamespaceName("RwHelper");
    RH_ScopedCategoryGlobal();

    RH_ScopedGlobalInstall(GetEventGlobalGroup, 0x4ABA50);
    RH_ScopedGlobalInstall(GetNameAndDamage, 0x5370A0);
    RH_ScopedGlobalInstall(GetFirstAtomicCallback, 0x734810);
    RH_ScopedGlobalInstall(GetFirstAtomic, 0x734820);
    RH_ScopedGlobalInstall(Get2DEffectAtomicCallback, 0x734850);
    RH_ScopedGlobalInstall(Get2DEffectAtomic, 0x734880);
    RH_ScopedGlobalInstall(GetFirstObjectCallback, 0x7348B0);
    RH_ScopedGlobalInstall(GetFirstObject, 0x7348C0);
    RH_ScopedGlobalInstall(GetFirstFrameCallback, 0x7348F0);
    RH_ScopedGlobalInstall(GetFirstChild, 0x734900);
    RH_ScopedGlobalInstall(SkinAtomicGetHAnimHierarchCB, 0x734A20);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromSkinClump, 0x734A40);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromFrame, 0x734AB0);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromClump, 0x734B10);
    RH_ScopedGlobalInstall(AtomicRemoveAnimFromSkinCB, 0x734B90);
    RH_ScopedGlobalInstall(RpAtomicConvertGeometryToTL, 0x734BE0);
    RH_ScopedGlobalInstall(RpAtomicConvertGeometryToTS, 0x734C20);
    RH_ScopedGlobalInstall(atomicConvertGeometryToTL, 0x734C60);
    RH_ScopedGlobalInstall(RpClumpConvertGeometryToTL, 0x734CB0);
    RH_ScopedGlobalInstall(atomicConvertGeometryToTS, 0x734CE0);
    RH_ScopedGlobalInstall(RpClumpConvertGeometryToTS, 0x734D30);
    RH_ScopedGlobalInstall(forceLinearFilteringAtomicsCB, 0x734DA0);
    RH_ScopedGlobalInstall(SetFilterModeOnClumpsTextures, 0x734DC0);
    RH_ScopedGlobalInstall(forceLinearFilteringMatTexturesCB, 0x734D60);
    RH_ScopedGlobalInstall(SetFilterModeOnAtomicsTextures, 0x734D80);
    RH_ScopedGlobalInstall(RpGeometryReplaceOldMaterialWithNewMaterial, 0x734DE0);
    RH_ScopedGlobalInstall(RwTexDictionaryFindHashNamedTexture, 0x734E50);
    RH_ScopedGlobalInstall(RpClumpGetBoundingSphere, 0x734FC0);
    RH_ScopedGlobalInstall(SkinGetBonePositions, 0x735140);
    RH_ScopedGlobalInstall(SkinSetBonePositions, 0x7352D0);
    RH_ScopedGlobalInstall(SkinGetBonePositionsToTable, 0x735360);
    RH_ScopedGlobalInstall(RemoveRefsCB, 0x7226D0);
}

// 0x4ABA50
CEventGlobalGroup* GetEventGlobalGroup() {
    static auto& globalEvents = StaticRef<CEventGlobalGroup*>(0xA9AF6C);

    if (globalEvents)
        return globalEvents;

    globalEvents = new CEventGlobalGroup(nullptr);
    return globalEvents;
}

// TODO: Check `outName` size (to avoid buffer overflow)
// 0x5370A0
void GetNameAndDamage(const char* name, char* objName, bool& bIsDamageModel) {
    const size_t nodesz = strlen(name);

    const auto TerminatedCopy = [=](size_t offset) {
        strncpy_s(objName, nodesz - offset + 1, name, nodesz - offset);
        objName[nodesz - offset] = 0;
    };

    // EndsWith "_dam"
    if (name[nodesz - 4] == '_' && name[nodesz - 3] == 'd' && name[nodesz - 2] == 'a' && name[nodesz - 1] == 'm'
    ) {
        bIsDamageModel = true;
        TerminatedCopy(sizeof("_dam") - 1);
    }
    else {
        bIsDamageModel = false;
        // EndsWith "_l0" or "_L0"
        if (name[nodesz - 3] == '_' &&
            (name[nodesz - 2] == 'L' || name[nodesz - 2] == 'l') && name[nodesz - 1] == '0'
        ) {
            TerminatedCopy(sizeof("_l0") - 1);
        } else
            strcpy_s(objName, strlen(name) + 1, name);
    }
}

// 0x734810
RpAtomic* GetFirstAtomicCallback(RpAtomic* atomic, void* data) {
    *(RpAtomic**)(data) = atomic;
    return nullptr;
}

// 0x734820
RpAtomic* GetFirstAtomic(RpClump* clump) {
    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, GetFirstAtomicCallback, &atomic);
    return atomic;
}

// 0x734850
RpAtomic* Get2DEffectAtomicCallback(RpAtomic* atomic, void* data) {
    // Stops the traversal at the first atomic whose geometry has any 2d effects
    if (RpGeometryGet2dFxCount(atomic->geometry) == 0) {
        return atomic; // Keep iterating
    }
    *static_cast<RpAtomic**>(data) = atomic;
    return nullptr; // Stop iterating
}

// 0x734880
RpAtomic* Get2DEffectAtomic(RpClump* clump) {
    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, Get2DEffectAtomicCallback, &atomic);
    return atomic;
}

// 0x7348B0
RwObject* GetFirstObjectCallback(RwObject* object, void* data) {
    *(RwObject**)(data) = object;
    return nullptr;
}

// 0x7348C0
RwObject* GetFirstObject(RwFrame* frame) {
    RwObject* obj{};
    RwFrameForAllObjects(frame, GetFirstObjectCallback, &obj);
    return obj;
}

// 0x7348F0
RwFrame* GetFirstFrameCallback(RwFrame* frame, void* data) {
    *(RwFrame**)(data) = frame;
    return nullptr;
}

// 0x734900
RwFrame* GetFirstChild(RwFrame* frame) {
    RwFrame* child{};
    RwFrameForAllChildren(frame, GetFirstFrameCallback, &child);
    return child;
}

// 0x734A70
static RwFrame* GetAnimHierarchyFromChildCB(RwFrame* frame, void* data) {
    RpHAnimHierarchy* hier = RpHAnimFrameGetHierarchy(frame);
    if (!hier) {
        RwFrameForAllChildren(frame, GetAnimHierarchyFromChildCB, data);
        return frame; // Keep iterating
    }
    *static_cast<RpHAnimHierarchy**>(data) = hier;
    return nullptr; // Stop iterating
}

// 0x734AB0
RpHAnimHierarchy* GetAnimHierarchyFromFrame(RwFrame* frame) {
    assert(frame);

    RpHAnimHierarchy* hier{};
    if (RpHAnimHierarchy* own = RpHAnimFrameGetHierarchy(frame)) {
        return own;
    }
    // NOTE: The exe runs this very same traversal twice - the second pass can never find anything else
    RwFrameForAllChildren(frame, GetAnimHierarchyFromChildCB, &hier);
    return hier;
}

// 0x734B10
RpHAnimHierarchy* GetAnimHierarchyFromClump(RpClump* clump) {
    assert(clump);
    return GetAnimHierarchyFromFrame(RpClumpGetFrame(clump));
}

// 0x734A40
RpHAnimHierarchy* GetAnimHierarchyFromSkinClump(RpClump* clump) {
    RpHAnimHierarchy* anim{};
    RpClumpForAllAtomics(clump, SkinAtomicGetHAnimHierarchCB, &anim);
    return anim;
}

// name not from Android
// 0x734A20
RpAtomic* SkinAtomicGetHAnimHierarchCB(RpAtomic* atomic, void* data) {
    *(RpHAnimHierarchy**)(data) = RpSkinAtomicGetHAnimHierarchy(atomic);
    return nullptr;
}

// 0x734B90
RpAtomic* AtomicRemoveAnimFromSkinCB(RpAtomic* atomic, void* data) {
    if (RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic))) {
        if (RpHAnimHierarchy* hier = RpSkinAtomicGetHAnimHierarchy(atomic)) {
            RtAnimAnimation*& currAnim = hier->currentAnim->pCurrentAnim;
            if (currAnim) {
                RtAnimAnimationDestroy(currAnim);
            }
            currAnim = nullptr;
        }
    }
    return atomic;
}

// 0x734BE0
bool RpAtomicConvertGeometryToTL(RpAtomic* atomic) {
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    auto flags = RpGeometryGetFlags(geometry);
    if (flags & rpGEOMETRYNATIVE || !(flags & rpGEOMETRYTRISTRIP))
        return false;

    RpGeometryLock(geometry, rpGEOMETRYLOCKALL);
    RpGeometrySetFlags(geometry, flags & ~rpGEOMETRYTRISTRIP);
    RpGeometryUnlock(geometry);

    return true;
}

// 0x734C20
bool RpAtomicConvertGeometryToTS(RpAtomic* atomic) {
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    auto flags = RpGeometryGetFlags(geometry);
    if (flags & rpGEOMETRYNATIVE || flags & rpGEOMETRYTRISTRIP)
        return false;

    RpGeometryLock(geometry, rpGEOMETRYLOCKALL);
    RpGeometrySetFlags(geometry, flags | rpGEOMETRYTRISTRIP);
    RpGeometryUnlock(geometry);

    return true;
}

// 0x734C60
RpAtomic* atomicConvertGeometryToTL(RpAtomic* atomic, void* data) {
    if (!RpAtomicConvertGeometryToTL(atomic)) {
        *(bool*)(data) = false;
    }
    return atomic;
}

// 0x734CB0
bool RpClumpConvertGeometryToTL(RpClump* clump) {
    bool success{ true };
    RpClumpForAllAtomics(clump, atomicConvertGeometryToTL, &success);
    return success;
}

// 0x734CE0
RpAtomic* atomicConvertGeometryToTS(RpAtomic* atomic, void* data) {
    if (!RpAtomicConvertGeometryToTS(atomic)) {
        *(bool*)(data) = false;
    }
    return atomic;
}

// 0x734D30
bool RpClumpConvertGeometryToTS(RpClump* clump) {
    bool success{ true };
    RpClumpForAllAtomics(clump, atomicConvertGeometryToTS, &success);
    return success;
}

// 0x734D60
RpMaterial* forceLinearFilteringMatTexturesCB(RpMaterial* material, void* data) {
    if (RwTexture* texture = RpMaterialGetTexture(material)) {
        RwTextureSetFilterMode(texture, (RwTextureFilterMode)((unsigned)data));
    }
    return material;
}

// 0x734D80
bool SetFilterModeOnAtomicsTextures(RpAtomic* atomic, RwTextureFilterMode filtering) {
    RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), forceLinearFilteringMatTexturesCB, (void*)(unsigned)filtering);
    return true;
}

// 0x734DA0
RpAtomic* forceLinearFilteringAtomicsCB(RpAtomic* atomic, void* data) {
    SetFilterModeOnAtomicsTextures(atomic, (RwTextureFilterMode)((unsigned)data));
    return atomic;
}

// 0x734DC0
bool SetFilterModeOnClumpsTextures(RpClump* clump, RwTextureFilterMode filtering) {
    RpClumpForAllAtomics(clump, forceLinearFilteringAtomicsCB, (void*)(unsigned)filtering);
    return true;
}

// 0x734DE0
bool RpGeometryReplaceOldMaterialWithNewMaterial(RpGeometry* geometry, RpMaterial* oldMaterial, RpMaterial* newMaterial) {
    bool replaced = false;

    for (uint16 i = 0; i < geometry->mesh->numMeshes; i++) {
        RpMesh* mesh = RpGeometryGetMesh(geometry, i);
        if (mesh->material != oldMaterial) {
            continue;
        }

        const auto index = _rpMaterialListFindMaterialIndex(&geometry->matList, oldMaterial);
        RpMaterialDestroy(oldMaterial);
        geometry->matList.materials[index] = newMaterial;
        mesh->material                     = newMaterial;
        newMaterial->refCount++;
        replaced = true; // NOTE: The exe doesn't break out - it keeps replacing any further hit
    }

    return replaced;
}

// 0x734E50
RwTexture* RwTexDictionaryFindHashNamedTexture(RwTexDictionary* txd, uint32 hash) {
    // Walks the dictionary's intrusive list of textures
    const RwLLLink* head = &txd->texturesInDict.link;
    for (const RwLLLink* node = head->next; node != head; node = node->next) {
        RwTexture* texture = rwLLLinkGetData(node, RwTexture, lInDictionary);
        if (CKeyGen::GetUppercaseKey(texture->name) == hash) {
            return texture;
        }
    }
    return nullptr;
}

// 0x008D60BC - Whether to use the frame's LTM (or its modelling matrix) when transforming the atomics
static bool g_bUseLTM = true;

// 0x734970
static RpAtomic* ClumpBoundingSphereCentreCB(RpAtomic* atomic, void* data) {
    RpAtomicGetWorldBoundingSphere(atomic); // Recomputes the world binding sphere if it's dirty (the exe checks `privateFlags` itself)

    const auto frame = RpAtomicGetFrame(atomic);
    const auto mat   = g_bUseLTM ? RwFrameGetLTM(frame) : &frame->modelling;

    RwV3d centre;
    RwV3dTransformPoints(&centre, &atomic->boundingSphere.center, 1, mat);

    auto& sum = *static_cast<RwV3d*>(data);
    sum.x += centre.x;
    sum.y += centre.y;
    sum.z += centre.z;
    return atomic;
}

// 0x734ED0
static RpAtomic* ClumpBoundingSphereRadiusCB(RpAtomic* atomic, void* data) {
    RpAtomicGetWorldBoundingSphere(atomic); // See above

    const auto frame = RpAtomicGetFrame(atomic);
    const auto mat   = g_bUseLTM ? RwFrameGetLTM(frame) : &frame->modelling;

    RwV3d centre;
    RwV3dTransformPoints(&centre, &atomic->boundingSphere.center, 1, mat);

    auto&      sphere = *static_cast<RwSphere*>(data);
    const auto radius = DistanceBetweenPoints(centre, sphere.center) + atomic->boundingSphere.radius;
    if (radius > sphere.radius) {
        sphere.radius = radius;
    }
    return atomic;
}

// 0x734FC0
RpClump* RpClumpGetBoundingSphere(RpClump* clump, RwSphere* sphere, bool bUseLTM) {
    g_bUseLTM = bUseLTM; // NOTE: Global side effect, exactly as in the exe

    if (!clump || !sphere) {
        return nullptr;
    }

    sphere->center = {};
    sphere->radius = 0.0f;

    const auto numAtomics = RpClumpGetNumAtomics(clump);
    if (numAtomics < 1) {
        return nullptr;
    }

    RwV3d sum{};
    RpClumpForAllAtomics(clump, ClumpBoundingSphereCentreCB, &sum);
    const auto invNumAtomics = 1.0f / static_cast<float>(numAtomics);
    sum.x *= invNumAtomics;
    sum.y *= invNumAtomics;
    sum.z *= invNumAtomics;

    RwSphere result{};
    result.center = sum;
    result.radius = 0.0f;
    RpClumpForAllAtomics(clump, ClumpBoundingSphereRadiusCB, &result);

    const auto frame = RpClumpGetFrame(clump);
    RwMatrix   mat;
    RwMatrixCopy(&mat, g_bUseLTM ? RwFrameGetLTM(frame) : &frame->modelling);
    RwV3dTransformPoints(&result.center, &result.center, 1, &mat);

    *sphere = result;
    return clump;
}

//! One entry of the bone-position table written by `SkinGetBonePositions` (0xC88258, stride 0x10)
struct SkinBoneSlot {
    int32 parentIndex;
    RwV3d position;
};
VALIDATE_SIZE(SkinBoneSlot, 0x10);

//! The re-entrancy flag lives right after the table (at 0xC88658), which gives the table's size
static constexpr size_t SKIN_BONE_SLOT_COUNT = 64;

struct SkinBoneSlotTable {
    SkinBoneSlot slots[SKIN_BONE_SLOT_COUNT];
};

static SkinBoneSlotTable& GetSkinBoneSlotTable() {
    return StaticRef<SkinBoneSlotTable>(0xC88258);
}

//! 0xC88658 - Re-entrancy guard of `SkinGetBonePositions` (NOTE: Nothing ever clears it in the exe)
static bool& SkinGetBonePositionsBusy() {
    return StaticRef<bool>(0xC88658);
}

// 0x735140
void SkinGetBonePositions(RpClump* clump) {
    if (SkinGetBonePositionsBusy()) {
        return;
    }
    SkinGetBonePositionsBusy() = true;

    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, GetFirstAtomicCallback, &atomic);
    RpSkin* skin = RpSkinGeometryGetSkin(atomic->geometry);

    RpHAnimHierarchy* hier{};
    RpClumpForAllAtomics(clump, SkinAtomicGetHAnimHierarchCB, &hier);

    auto& slots = GetSkinBoneSlotTable();
    slots.slots[0].position    = {};
    slots.slots[0].parentIndex = -1;

    const auto numBones = static_cast<int32>(RpSkinGetNumBones(skin));
    if (numBones <= 1) {
        return;
    }

    int32  parentStack[50];
    int32* stackPtr = parentStack;
    int32  parent   = 0;
    for (int32 node = 1; node < numBones; node++) {
        const RwMatrix* skinMatrices = RpSkinGetSkinToBoneMatrices(skin);

        const RwV3d origin{};
        RwV3dTransformPoints(&slots.slots[node].position, &origin, 1, &skinMatrices[parent]);
        slots.slots[node].parentIndex = parent;

        // Walk the hierarchy: push the parent when descending into children, pop when moving to a sibling
        const auto flags = hier->pNodeInfo[node].flags;
        if (flags & rpHANIMPUSHPARENTMATRIX) {
            *++stackPtr = parent;
        }
        parent = (flags & rpHANIMPOPPARENTMATRIX) ? *stackPtr-- : node;
    }
}

// 0x7352D0
void SkinSetBonePositions(RpClump* clump) {
    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, GetFirstAtomicCallback, &atomic);
    RpSkin* skin = RpSkinGeometryGetSkin(atomic->geometry);

    RpHAnimHierarchy* hier{};
    RpClumpForAllAtomics(clump, SkinAtomicGetHAnimHierarchCB, &hier);

    RwMatrix*  matrices = RpHAnimHierarchyGetMatrixArray(hier);
    const auto numBones = static_cast<int32>(RpSkinGetNumBones(skin));
    if (numBones <= 1) {
        return;
    }

    // Re-place every bone relative to the parent saved by `SkinGetBonePositions`
    const auto& slots = GetSkinBoneSlotTable();
    for (int32 node = 1; node < numBones; node++) {
        const SkinBoneSlot& slot = slots.slots[node];
        RwV3dTransformPoints(&matrices[node].pos, &slot.position, 1, &matrices[slot.parentIndex]);
    }
}

// 0x735360
void SkinGetBonePositionsToTable(RpClump* clump, RwV3d* table) {
    if (!table) {
        return;
    }

    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, GetFirstAtomicCallback, &atomic);
    RpSkin* skin = RpSkinGeometryGetSkin(atomic->geometry);

    RpHAnimHierarchy* hier{};
    RpClumpForAllAtomics(clump, SkinAtomicGetHAnimHierarchCB, &hier);

    table[0] = {};

    const auto numBones = static_cast<int32>(RpSkinGetNumBones(skin));
    if (numBones <= 1) {
        return;
    }

    int32  parentStack[32];
    int32* stackPtr = parentStack;
    int32  parent   = 0;
    for (int32 node = 1; node < numBones; node++) {
        const RwMatrix* skinMatrices = RpSkinGetSkinToBoneMatrices(skin);

        const RwV3d origin{};
        RwV3dTransformPoints(&table[node], &origin, 1, &skinMatrices[parent]);

        const auto flags = hier->pNodeInfo[node].flags;
        if (flags & rpHANIMPUSHPARENTMATRIX) {
            *++stackPtr = parent;
        }
        parent = (flags & rpHANIMPOPPARENTMATRIX) ? *stackPtr-- : node;
    }
}

// 0x7226D0
RpAtomic* RemoveRefsCB(RpAtomic* atomic, void* data) {
    UNUSED(data);
    auto* modelInfo = CVisibilityPlugins::GetModelInfo(atomic);
    modelInfo->RemoveRef();
    return atomic;
}

// 0x7226F0
void RemoveRefsForAtomic(RpClump* clump) {
    RpClumpForAllAtomics(clump, RemoveRefsCB, nullptr);
}
