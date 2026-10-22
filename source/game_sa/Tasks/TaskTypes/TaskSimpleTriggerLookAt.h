/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskSimple.h"
#include "AnimBlendAssociation.h"
#include "Entity.h"

class CPed;
class CEvent;

class NOTSA_EXPORT_VTABLE CTaskSimpleTriggerLookAt : public CTaskSimple {
public:
    CEntity* m_pEntity;        // 0x08
    int32    m_time;           // 0x0C
    int32    m_nOffsetBoneTag; // 0x10
    RwV3d    m_vOffsetPos;     // 0x14
    bool     m_bUseTorso;      // 0x20
    float    m_Speed;          // 0x24
    int32    m_nBlendTime;     // 0x28
    bool     m_bEntityExist;   // 0x2C
    int8     m_nPriority;      // 0x2D

public:
    static constexpr auto Type = TASK_SIMPLE_TRIGGER_LOOK_AT;

    static void InjectHooks();

    // 0x634440
    CTaskSimpleTriggerLookAt(CEntity* entity,
                             int32 time,
                             int32 offsetBoneTag,
                             RwV3d offsetPos,
                             bool bUseTorso = true,
                             float speed = 0.25f,
                             int32 blendTime = 1000,
                             int32 priority = 3);
    // 0x634500
    ~CTaskSimpleTriggerLookAt() override;

    // 0x634560
    CTask*    Clone() const override;
    // 0x6344F0
    eTaskType GetTaskType() const override { return Type; }
    // 0x634610
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, CEvent const* event = nullptr) override;
    // 0x634620
    bool      ProcessPed(CPed* ped) override;

private: // Wrappers for hooks
    // 0x634440
    CTaskSimpleTriggerLookAt* Constructor(CEntity* entity, int32 time, int32 offsetBoneTag, RwV3d offsetPos, uint8 bUseTorso, float speed, int32 blendTime, int32 priority) {
        this->CTaskSimpleTriggerLookAt::CTaskSimpleTriggerLookAt(entity, time, offsetBoneTag, offsetPos, bUseTorso, speed, blendTime, priority);
        return this;
    }

    // 0x634500
    CTaskSimpleTriggerLookAt* Destructor() {
        this->CTaskSimpleTriggerLookAt::~CTaskSimpleTriggerLookAt();
        return this;
    }
};

VALIDATE_SIZE(CTaskSimpleTriggerLookAt, 0x30);
