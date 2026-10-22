#pragma once

#include "TaskSimple.h"

class CVehicle;

class NOTSA_EXPORT_VTABLE CTaskSimpleCarForcePedOut : public CTaskSimple {
public:
    CVehicle* m_vehicle;
    int32     m_door;

public:
    static constexpr auto Type = TASK_SIMPLE_CAR_FORCE_PED_OUT;

    CTaskSimpleCarForcePedOut(CVehicle* vehicle, int32 door); // 0x647710
    ~CTaskSimpleCarForcePedOut() override;                   // 0x647790

    eTaskType GetTaskType() const override { return Type; }                                                          // 0x647770
    CTask* Clone() const override { return new CTaskSimpleCarForcePedOut(m_vehicle, m_door); }                        // 0x649EE0
    bool MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override { return false; } // 0x647780
    bool ProcessPed(CPed* ped) override;                                                                              // 0x6477F0
};
VALIDATE_SIZE(CTaskSimpleCarForcePedOut, 0x10);
