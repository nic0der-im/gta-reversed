#pragma once

#include "TaskComplex.h"

class CTaskComplexSequence;

enum ePartnerState : int8 {
    PARTNER_STATE_UNK_1 = 1,
    PARTNER_STATE_UNK_2,
    PARTNER_STATE_UNK_3,
    PARTNER_STATE_UNK_4,
    PARTNER_STATE_UNK_5,
    PARTNER_STATE_UNK_6
};

class NOTSA_EXPORT_VTABLE CTaskComplexPartner : public CTaskComplex {
public:
    int32         field_C;
    int32         field_10;
    char          m_commandName[32];
    int32         m_taskId;
    CPed*         m_partner;
    float         m_distanceMultiplier;
    CVector       m_point;
    CVector       m_targetPoint;
    bool          m_leadSpeaker;
    ePartnerState m_partnerState;
    int8          m_firstToTargetFlag;
    int8          m_updateDirectionCount;
    bool          m_taskCompleted;
    bool          m_makePedAlwaysFacePartner;
    char          m_animBlockName[16];
    bool          m_requiredAnimsStreamedIn;
    union {
        int32 field_70;              //!< Used by the derived tasks (e.g. `CTaskComplexPartnerGreet::m_handShakeType`)
        int16 m_goToPointIterations; //!< Low word - Frames spent on `TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL`
    };

public:
    static constexpr auto Type = TASK_COMPLEX_PARTNER;

    CTaskComplexPartner(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point);
    ~CTaskComplexPartner() override;

    eTaskType GetTaskType() const override { return Type; }
    CTask*       CreateNextSubTask(CPed* ped) override;
    CTask*       CreateFirstSubTask(CPed* ped) override;
    CTask*       ControlSubTask(CPed* ped) override;
    virtual void StreamRequiredAnims();
    virtual void RemoveStreamedAnims();
    virtual CTaskComplexSequence* GetPartnerSequence() = 0; //!< Overridden by every concrete partner task

private:
    // 0x6822B0
    int8 GetPartnerStage();

    // 0x681FE0
    void ComputePoints(CPed* ped, CVector& point, CVector& targetPoint);

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CTaskComplexPartner* Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point);
};

// 0x70 is the last field of the derived tasks (see `CTaskComplexPartnerChat`), which the
// base class methods access through `field_70`.
VALIDATE_SIZE(CTaskComplexPartner, 0x74);
