#pragma once

#include "RenderWare.h"

class CCustomBuildingRenderer {
public:
    static void InjectHooks();

    CCustomBuildingRenderer();
    ~CCustomBuildingRenderer();

    static bool Initialise();
    static void Shutdown();
    static bool PluginAttach();
    static void AtomicSetup(RpAtomic* atomic);
    static int32 IsCBPCPipelineAttached(RpAtomic* atomic);
    static void UpdateDayNightBalanceParam();
    static void Update();
};
