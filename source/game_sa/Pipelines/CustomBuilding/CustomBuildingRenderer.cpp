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

// 0x5D7F00
void CCustomBuildingRenderer::AtomicSetup(RpAtomic* atomic) {
    const auto geometry = RpAtomicGetGeometry(atomic);
    if (CCustomBuildingDNPipeline::GetExtraVertColourPtr(geometry)->NightColors && RpGeometryGetPreLightColors(geometry)) {
        CCustomBuildingDNPipeline::CustomPipeAtomicSetup(atomic);
    } else {
        CCustomBuildingPipeline::CustomPipeAtomicSetup(atomic);
    }
}

// 0x5D7F40
bool CCustomBuildingRenderer::IsCBPCPipelineAttached(RpAtomic* atomic) {
    const auto pipeId = GetPipelineID(atomic);
    if (pipeId == CUSTOM_BUILDING_PIPELINE_ID || pipeId == CUSTOM_BUILDING_DN_PIPELINE_ID) {
        return true;
    }

    const auto geometry = RpAtomicGetGeometry(atomic);
    return CCustomBuildingDNPipeline::GetExtraVertColourPtr(geometry)->NightColors && RpGeometryGetPreLightColors(geometry);
}

// 0x5D7F80
void CCustomBuildingRenderer::UpdateDayNightBalanceParam() {
    constexpr auto oneOver60 = 1.0f / 60.0f;

    // Minutes since midnight (NOTE: Retail multiplies by `1/60` instead of dividing by 60)
    const auto minutes = (float)(CClock::ms_nGameClockHours * 60u + CClock::ms_nGameClockMinutes) + (float)CClock::ms_nGameClockSeconds * oneOver60;

    if (minutes < 360.0f) { // Before 06:00
        CCustomBuildingDNPipeline::m_fDNBalanceParam = 1.0f;
    } else if (minutes < 420.0f) { // 06:00 - 07:00, Dawn
        CCustomBuildingDNPipeline::m_fDNBalanceParam = (420.0f - minutes) * oneOver60;
    } else if (minutes < 1200.0f) { // 07:00 - 20:00, Day
        CCustomBuildingDNPipeline::m_fDNBalanceParam = 0.0f;
    } else if (minutes < 1260.0f) { // 20:00 - 21:00, Dusk
        CCustomBuildingDNPipeline::m_fDNBalanceParam = 1.0f - (1260.0f - minutes) * oneOver60;
    } else { // 21:00 - 06:00, Night
        CCustomBuildingDNPipeline::m_fDNBalanceParam = 1.0f;
    }
}

// 0x5D6830
//! Unnamed in the retail exe - Advances the DN working index, and sets the `force prelit update` flag.
//! Both globals are also read by `CCustomBuildingDNPipeline::PreRenderUpdate`.
static void AdvanceDNWorkingIndex(uint32 forcePrelitUpdate) {
    auto& workingIndex = StaticRef<uint32>(0xC02C14);
    auto& forceUpdate  = StaticRef<uint32>(0xC02C18);

    workingIndex = (workingIndex + 1) & 0xF;
    forceUpdate  = forcePrelitUpdate;
}

// 0x5D8050
void CCustomBuildingRenderer::Update() {
    UpdateDayNightBalanceParam();
    AdvanceDNWorkingIndex(0);
}
