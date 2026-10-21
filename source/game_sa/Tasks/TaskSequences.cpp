#include "StdInc.h"

void CTaskSequences::InjectHooks() {
    RH_ScopedClass(CTaskSequences);
    RH_ScopedCategory("Tasks");

    RH_ScopedInstall(Init, 0x632D90);
    RH_ScopedInstall(CleanUpForShutdown, 0x632DD0);
    RH_ScopedInstall(GetAvailableSlot, 0x632E00);
}

// 0x632D90
void CTaskSequences::Init() {
    ms_iActiveSequence = -1;
    CleanUpForShutdown();
}

// 0x632DD0
void CTaskSequences::CleanUpForShutdown() {
    for (auto i = 0; i < NUM_SEQUENCES; i++) {
        ms_bIsOpened[i] = false;
        auto taskSequence = &ms_taskSequence[i];
        taskSequence->Flush();
    }
}

// 0x632E00
int32 CTaskSequences::GetAvailableSlot(uint8 slot) {
    // `slot` is `CRunningScript::m_UsesMissionCleanup`: Mission scripts use the upper half of the slots, other scripts the lower one
    const int32 start = slot ? NUM_SEQUENCES / 2 : 0;
    const int32 end   = slot ? NUM_SEQUENCES : NUM_SEQUENCES / 2;

    for (auto i = start; i < end; i++) {
        if (!ms_bIsOpened[i] && !ms_taskSequence[i].IsActive()) {
            return i;
        }
    }
    return -1;
}
