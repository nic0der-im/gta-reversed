#include "StdInc.h"

#include "EventSource.h"

// 0x4ABAC0
int32 CEventSource::ComputeEventSourceType(const CEvent& event, const CPed& ped) {
    const auto src = event.GetSourceEntity();
    if (!src || src->GetType() != ENTITY_TYPE_PED) {
        return 0;
    }

    const auto srcPed = static_cast<const CPed*>(src);
    const auto intel  = ped.GetIntelligence();

    if (intel->IsThreatenedBy(*srcPed)) {
        return 3;
    }
    if (intel->IsFriendlyWith(*srcPed)) {
        return 2;
    }
    if (srcPed->IsPlayer()) {
        return 1;
    }
    return 0;
}
