#include "StdInc.h"

#include "TaskComplexGoToPointAiming.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleGunControl.h"
#include "TaskSimpleUseGun.h"
#include "WeaponInfo.h"

void CTaskComplexGoToPointAiming::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGoToPointAiming, 0x86fe00, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x668790);
    RH_ScopedInstall(Destructor, 0x668870);
    RH_ScopedInstall(CreateSubTask, 0x6688D0);

    RH_ScopedVMTInstall(Clone, 0x66CD80);
    RH_ScopedVMTInstall(GetTaskType, 0x668860);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x66DD70);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x66DDB0);
    RH_ScopedVMTInstall(ControlSubTask, 0x6689E0);
}

// 0x668790
CTaskComplexGoToPointAiming::CTaskComplexGoToPointAiming(
    eMoveState moveState,
    const CVector& movePos,
    CEntity* aimAtEntity,
    CVector aimPos,
    const float targetRadius,
    const float slowDownDist
) :
    m_aimPos{aimPos},
    m_aimAtEntity{aimAtEntity},
    m_moveState{moveState},
    m_movePos{movePos},
    m_moveTargetRadius{targetRadius},
    m_slowDownDistance{slowDownDist}
{
    CEntity::SafeRegisterRef(m_aimAtEntity);
}

CTaskComplexGoToPointAiming::CTaskComplexGoToPointAiming(const CTaskComplexGoToPointAiming& o) :
    CTaskComplexGoToPointAiming{
        o.m_moveState,
        o.m_movePos,
        o.m_aimAtEntity,
        o.m_aimPos,
        o.m_moveTargetRadius,
        o.m_slowDownDistance
    }
{
}

// 0x668870
CTaskComplexGoToPointAiming::~CTaskComplexGoToPointAiming() {
    CEntity::SafeCleanUpRef(m_aimAtEntity);
}

// 0x6688D0
CTask* CTaskComplexGoToPointAiming::CreateSubTask(eTaskType taskType) {
    switch (taskType) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
        return new CTaskComplexGoToPointAndStandStill{m_moveState, m_movePos, m_moveTargetRadius, m_slowDownDistance};
    case TASK_SIMPLE_GUN_CTRL:
        return new CTaskSimpleGunControl{
            m_aimAtEntity,
            m_aimPos,
            CVector{},
            GetTaskType() == TASK_COMPLEX_GO_TO_POINT_SHOOTING ? eGunCommand::FIREBURST : eGunCommand::NONE,
            1,
            600'000
        };
    default:
        return nullptr;
    }
}

// 0x66DD70
CTask* CTaskComplexGoToPointAiming::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
    case TASK_SIMPLE_GUN_CTRL:
        return CreateSubTask(TASK_FINISHED);
    default:
        NOTSA_UNREACHABLE();
    }
}

// 0x66DDB0
CTask* CTaskComplexGoToPointAiming::CreateFirstSubTask(CPed* ped) {
    m_newTargetSet = false;

    const auto* const wi = CWeaponInfo::GetWeaponInfo(ped);
    if (wi->flags.bAimWithArm) {
        auto& taskMgr = ped->GetIntelligence()->GetTaskManager();
        if (auto* const attack = taskMgr.GetTaskSecondary(TASK_SECONDARY_ATTACK)) {
            attack->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
        } else {
            const auto isShooting = GetTaskType() == TASK_COMPLEX_GO_TO_POINT_SHOOTING;
            taskMgr.SetTaskSecondary(
                new CTaskSimpleUseGun{
                    m_aimAtEntity,
                    m_aimPos,
                    isShooting ? eGunCommand::FIREBURST : eGunCommand::AIM,
                    (uint16)(isShooting ? 3 : 1),
                    false
                },
                TASK_SECONDARY_ATTACK
            );
        }
        return CreateSubTask(TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL);
    }
    if (wi->flags.bCanAim && (wi->m_nWeaponFire == WEAPON_FIRE_INSTANT_HIT || wi->m_nWeaponFire == WEAPON_FIRE_AREA_EFFECT)) {
        return CreateSubTask(TASK_SIMPLE_GUN_CTRL);
    }
    return CreateSubTask(TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL);
}

// 0x6689E0
CTask* CTaskComplexGoToPointAiming::ControlSubTask(CPed* ped) {
    if (m_newTargetSet) {
        return CreateFirstSubTask(ped);
    }

    auto* const intel = ped->GetIntelligence();
    auto& taskMgr = intel->GetTaskManager();
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: {
        static_cast<CTaskComplexGoToPointAndStandStill*>(m_pSubTask)->GoToPoint(m_movePos, 0.5f, 2.f, false); // 0x86FC84, 0x86FC88
        if (!CWeaponInfo::GetWeaponInfo(ped)->flags.bAimWithArm) {
            break;
        }
        auto* const attack = taskMgr.GetTaskSecondary(TASK_SECONDARY_ATTACK);
        if (!attack) {
            taskMgr.SetTaskSecondary(new CTaskSimpleUseGun{m_aimAtEntity, m_aimPos, eGunCommand::AIM, 1, false}, TASK_SECONDARY_ATTACK);
        } else if (intel->GetTaskUseGun()) {
            auto* const useGun = static_cast<CTaskSimpleUseGun*>(attack);
            if (GetTaskType() == TASK_COMPLEX_GO_TO_POINT_SHOOTING && rand() % 64 == 0) {
                useGun->ControlGun(ped, m_aimAtEntity, eGunCommand::FIRE);
            } else {
                useGun->ControlGun(ped, m_aimAtEntity, eGunCommand::AIM);
            }
        } else {
            attack->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
        }
        break;
    }
    case TASK_SIMPLE_GUN_CTRL: {
        if (!intel->GetTaskUseGun()) {
            break;
        }
        // Move direction in the ped's local space (y is flipped), normalized unless within the target radius
        const auto dir = m_movePos - ped->GetPosition();
        CVector2D move{dir.Dot(ped->GetRight()), dir.Dot(ped->GetForward()) * -1.f}; // 0x858C1C = -1.0f
        if (!(move.SquaredMagnitude() > sq(m_moveTargetRadius))) { // negated form: NaN zeroes, as the original
            move = {};
        } else {
            move /= move.Magnitude();
        }
        static_cast<CTaskSimpleUseGun*>(taskMgr.GetTaskSecondary(TASK_SECONDARY_ATTACK))->ControlGunMove(move);
        break;
    }
    default:
        break;
    }
    return m_pSubTask;
}
