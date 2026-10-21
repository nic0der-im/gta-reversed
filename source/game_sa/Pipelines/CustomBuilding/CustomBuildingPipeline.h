#pragma once

#include "RenderWare.h"

constexpr auto CUSTOM_BUILDING_PIPELINE_ID = rpPDS_MAKEPIPEID(rwVENDORID_DEVELOPER, 0x9C);

class CCustomBuildingPipeline {
public:
    static inline auto& ObjPipeline = StaticRef<RxPipeline*>(0xC02C68);

public:
    static void InjectHooks();

    static bool        CreatePipe();
    static void        DestroyPipe();
    static RpAtomic*   CustomPipeAtomicSetup(RpAtomic* atomic);
    static RxPipeline* CreateCustomObjPipe();

private:
    static RpMaterial* CustomPipeMaterialSetup(RpMaterial* material, void* data);
    static void CustomPipeRenderCB(RwResEntry* repEntry, void* object, RwUInt8 type, RwUInt32 flags);
    static RwBool CustomPipeReinstanceCB(void* object, RwResEntry* resEntry, RxD3D9AllInOneInstanceCallBack instanceCallback);
};
