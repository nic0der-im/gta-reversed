#include "StdInc.h"

#include "CustomBuildingRenderer.h"

#include "CustomBuildingDNPipeline.h"
#include "CustomBuildingPipeline.h"

void CCustomBuildingRenderer::InjectHooks() {
    RH_ScopedClass(CCustomBuildingRenderer);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Initialise, 0x5D7EC0);
    RH_ScopedInstall(Shutdown, 0x5D7EE0);
    RH_ScopedInstall(PluginAttach, 0x5D7EF0);
    RH_ScopedInstall(AtomicSetup, 0x5D7F00);
    RH_ScopedInstall(IsCBPCPipelineAttached, 0x5D7F40);
    RH_ScopedInstall(UpdateDayNightBalanceParam, 0x5D7F80);
    RH_ScopedInstall(Update, 0x5D8050);
}

// 0x5D7EC0
bool CCustomBuildingRenderer::Initialise() {
    auto pipe = CCustomBuildingPipeline::CreatePipe();
    if (pipe)
        return CCustomBuildingDNPipeline::CreatePipe();

    return pipe;
}

// 0x5D7EE0
void CCustomBuildingRenderer::Shutdown() {
    CCustomBuildingPipeline::DestroyPipe();
    CCustomBuildingDNPipeline::DestroyPipe();
}

// 0x5D7EF0
bool CCustomBuildingRenderer::PluginAttach() {
    return CCustomBuildingDNPipeline::ExtraVertColourPluginAttach();
}

constexpr auto CUSTOM_BUILDING_PIPELINE_ID = rpPDS_MAKEPIPEID(rwVENDORID_DEVELOPER, 0x9C); // same as CCustomBuildingPipeline's PLUGIN_ID

// 0x5D7F00
void CCustomBuildingRenderer::AtomicSetup(RpAtomic* atomic) {
    auto* const geometry = atomic->geometry;
    if (CCustomBuildingDNPipeline::GetExtraVertColourPtr(geometry)->NightColors && geometry->preLitLum) { // 0x5D6E90 reads ExtraVertColour::NightColors
        CCustomBuildingDNPipeline::CustomPipeAtomicSetup(atomic);
    } else {
        CCustomBuildingPipeline::CustomPipeAtomicSetup(atomic);
    }
}

// 0x5D7F40
bool CCustomBuildingRenderer::IsCBPCPipelineAttached(RpAtomic* atomic) {
    const auto id = GetPipelineID(atomic);
    if (id == CUSTOM_BUILDING_DN_PIPELINE_ID || id == CUSTOM_BUILDING_PIPELINE_ID) {
        return true;
    }
    auto* const geometry = atomic->geometry;
    return CCustomBuildingDNPipeline::GetExtraVertColourPtr(geometry)->NightColors && geometry->preLitLum;
}

// 0x5D7F80
void CCustomBuildingRenderer::UpdateDayNightBalanceParam() {
    const auto minutes = float(CClock::ms_nGameClockHours * 60 + CClock::ms_nGameClockMinutes) + float(CClock::ms_nGameClockSeconds) * (1.0f / 60.0f);
    auto& balance = CCustomBuildingDNPipeline::m_fDNBalanceParam;
    if (minutes >= 360.0f) {
        if (minutes < 420.0f) {
            balance = (420.0f - minutes) * (1.0f / 60.0f);
            return;
        }
        if (minutes < 1200.0f) {
            balance = 0.0f;
            return;
        }
        if (minutes < 1260.0f) {
            balance = 1.0f - (1260.0f - minutes) * (1.0f / 60.0f);
            return;
        }
    }
    balance = 1.0f;
}

// 0x5D8050
void CCustomBuildingRenderer::Update() {
    ZoneScoped;

    UpdateDayNightBalanceParam();

    // 0x5D6830
    static auto& s_Counter = StaticRef<uint32>(0xC02C14);
    static auto& s_Param   = StaticRef<uint32>(0xC02C18);
    s_Counter = (s_Counter + 1) & 15;
    s_Param   = 0;
}
