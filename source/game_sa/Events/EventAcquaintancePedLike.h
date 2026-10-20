#pragma once

#include "EventAcquaintancePed.h"

#include "EventEditableResponse.h"
#include "Vector.h"
#include "EventAcquaintancePed.h"


class NOTSA_EXPORT_VTABLE CEventAcquaintancePedLike : public CEventAcquaintancePed {
public:
    explicit CEventAcquaintancePedLike(CPed* ped, eTaskType taskType = TASK_NONE) : CEventAcquaintancePed(ped, taskType) {}

    eEventType GetEventType() const override { return EVENT_ACQUAINTANCE_PED_LIKE; }
    int32 GetEventPriority() const override { return 22; }
    CEventEditableResponse* CloneEditable() const noexcept override { return new CEventAcquaintancePedLike(m_AcquaintancePed); }
};
VALIDATE_SIZE(CEventAcquaintancePedLike, 0x18);
