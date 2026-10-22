#pragma once

#include "TaskComplex.h"
#include "TaskTimer.h"
#include "Vector.h"

class CPedGroup;

class NOTSA_EXPORT_VTABLE CTaskComplexGangFollower : public CTaskComplex {
public:
    static constexpr auto Type = eTaskType::TASK_COMPLEX_GANG_FOLLOWER;

    static constexpr bool ms_bUseClimbing = true; // 0x8D2EDC

    CTaskComplexGangFollower(CPedGroup* pedGroup, CPed* ped, uint8 a4, CVector pos, float a6);
    CTaskComplexGangFollower(const CTaskComplexGangFollower&);
    ~CTaskComplexGangFollower() override;

    CTask*    Clone() const override { return new CTaskComplexGangFollower{ *this }; }
    eTaskType GetTaskType() const override { return Type; }
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    CTask*    CreateNextSubTask(CPed* ped) override;
    CTask*    CreateFirstSubTask(CPed* ped) override;
    CTask*    ControlSubTask(CPed* ped) override;

    CVector CalculateOffsetPosition();

private:
    friend void InjectHooksMain();
    static void InjectHooks();
    CTaskComplexGangFollower* Constructor(CPedGroup* pedGroup, CPed* ped, uint8 uint8, CVector pos, float a6) { this->CTaskComplexGangFollower::CTaskComplexGangFollower(pedGroup, ped, uint8, pos, a6); return this; }
    CTaskComplexGangFollower* Destructor() { this->CTaskComplexGangFollower::~CTaskComplexGangFollower(); return this; }

public:
    CPedGroup* m_PedGroup{};
    CPed*      m_Leader{};
    CVector    m_PedPosn{};      // Leader position used to decide whether to recompute the standing offsets
    CVector    dword20{};        // Current offset from the leader
    CVector    dword2C{};        // Offset from the leader, as passed to the constructor
    float      dword38{};        // Target radius
    uint8      byte3C{};         // Member index in the group (indexes `CTaskComplexFollowLeaderInFormation::ms_offsets`)
    union {
        uint8 m_Flags;
        struct {
            uint8 m_AnimsRef : 1;                    // 0x01 - "gangs" anim block is referenced
            uint8 m_LeaveGroup : 1;                  // 0x02 - The ped is about to leave the group
            uint8 m_FollowLeader : 1;                // 0x04 - Chase the leader
            uint8 m_IsInPlayersGroup : 1;            // 0x08 - Leader is the player
            uint8 m_IsUsingStandingStillOffsets : 1; // 0x10 - `dword20` holds the standing-still offsets
        };
    };
    CTaskTimer m_ExhaleTimer{};  // Used to spawn the "exhale" FX while smoking
};
VALIDATE_SIZE(CTaskComplexGangFollower, 0x4C);
