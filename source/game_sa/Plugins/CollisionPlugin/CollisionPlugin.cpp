#include "StdInc.h"

#include "CollisionPlugin.h"

auto& gCollisionPluginOffset = StaticRef<RwInt32>(0x9689DC);

void CCollisionPlugin::InjectHooks() {
    RH_ScopedClass(CCollisionPlugin);
    RH_ScopedCategory("Plugins");

    RH_ScopedInstall(PluginAttach, 0x41B310);
    RH_ScopedInstall(SetModelInfo, 0x41B350);
}

// internal
// 0x41B1A0
static void* ClumpCollisionConstructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return object;
}

// internal
// 0x41B1C0
static void* ClumpCollisionDestructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return object;
}

// internal
// 0x41B1B0
static void* ClumpCollisionCopyConstructor(void* dstObject, const void* srcObject, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return dstObject;
}

// internal
// 0x41B1D0
static RwStream* ClumpCollisionStreamRead(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    CMemoryMgr::LockScratchPad();
    RwStreamRead(stream, PC_Scratch, binaryLength);

    auto* model = new CColModel();

    // The collision data starts past the 0x20 byte chunk header, except for the
    // unrecognized-fourCC path, which passes the buffer unshifted (retail quirk).
    switch (*reinterpret_cast<uint32*>(&PC_Scratch[0])) {
    case MakeFourCC("COLL"):
        CFileLoader::LoadCollisionModel(reinterpret_cast<uint8*>(&PC_Scratch[0x20]), *model);
        break;
    case MakeFourCC("COL2"):
        CFileLoader::LoadCollisionModelVer2(reinterpret_cast<uint8*>(&PC_Scratch[0x20]), *reinterpret_cast<uint32*>(&PC_Scratch[4]) - 0x18, *model, nullptr);
        break;
    case MakeFourCC("COL3"):
        CFileLoader::LoadCollisionModelVer3(reinterpret_cast<uint8*>(&PC_Scratch[0x20]), *reinterpret_cast<uint32*>(&PC_Scratch[4]) - 0x18, *model, nullptr);
        break;
    default:
        CFileLoader::LoadCollisionModel(reinterpret_cast<uint8*>(&PC_Scratch[0]), *model);
        break;
    }

    model->MakeMultipleAlloc();
    CCollisionPlugin::ms_currentModel->SetColModel(model, true);
    CCollisionPlugin::ms_currentModel->bDontWriteZBuffer = true;

    CMemoryMgr::ReleaseScratchPad();
    return stream;
}

// internal
// 0x41B2F0
static RwStream* ClumpCollisionStreamWrite(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return stream;
}

// internal
// 0x41B300
static RwInt32 ClumpCollisionGetSize(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return -1;
}

// 0x41B310
bool CCollisionPlugin::PluginAttach() {
    // 0x9689DC unused
    gCollisionPluginOffset = RpClumpRegisterPlugin(
        0,
        rwID_COLLISIONPLUGIN,
        ClumpCollisionConstructor,
        ClumpCollisionDestructor,
        ClumpCollisionCopyConstructor
    );

    RpClumpRegisterPluginStream(
        rwID_COLLISIONPLUGIN,
        ClumpCollisionStreamRead,
        ClumpCollisionStreamWrite,
        ClumpCollisionGetSize
    );

    return TRUE;
}

// 0x41B350
void CCollisionPlugin::SetModelInfo(CClumpModelInfo* modelInfo) {
    ms_currentModel = modelInfo;
}
