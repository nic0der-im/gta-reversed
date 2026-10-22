#include "StdInc.h"

#include "TaskComplexGoToPointAiming.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleGunControl.h"
#include "TaskSimpleUseGun.h"

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
        return new CTaskComplexGoToPointAndStandStill{
            m_moveState,
            m_movePos,
            m_moveTargetRadius,
            m_slowDownDistance,
            false,
            false
        };
    case TASK_SIMPLE_GUN_CTRL:
        return new CTaskSimpleGunControl{
            m_aimAtEntity,
            m_aimPos,
            CVector{}, // The original passes `nullptr` here
            GetTaskType() == TASK_COMPLEX_GO_TO_POINT_SHOOTING
                ? eGunCommand::FIREBURST
                : eGunCommand::NONE,
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

    const auto& weaponInfo = ped->GetActiveWeapon().GetWeaponInfo(ped);

    if (weaponInfo.flags.bAimWithArm) {
        // The ped needs a secondary "use gun" task while walking to the point
        if (!ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_ATTACK)) {
            ped->GetTaskManager().SetTaskSecondary(
                GetTaskType() == TASK_COMPLEX_GO_TO_POINT_SHOOTING
                    ? new CTaskSimpleUseGun{m_aimAtEntity, m_aimPos, eGunCommand::FIREBURST, 3, false}
                    : new CTaskSimpleUseGun{m_aimAtEntity, m_aimPos, eGunCommand::AIM,       1, false},
                TASK_SECONDARY_ATTACK
            );
        } else {
            ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_ATTACK)->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
        }
    } else if (   weaponInfo.flags.bCanAim
               && notsa::contains({ WEAPON_FIRE_INSTANT_HIT, WEAPON_FIRE_AREA_EFFECT }, weaponInfo.m_nWeaponFire)
    ) {
        return CreateSubTask(TASK_SIMPLE_GUN_CTRL); //> 0x66DEF7
    }

    //> 0x66DEF7 / 0x66DF17
    return new CTaskComplexGoToPointAndStandStill{
        m_moveState,
        m_movePos,
        m_moveTargetRadius,
        m_slowDownDistance,
        false,
        false
    };
}

// 0x6689E0
CTask* CTaskComplexGoToPointAiming::ControlSubTask(CPed* ped) {
    const auto& weaponInfo = ped->GetActiveWeapon().GetWeaponInfo(ped);

    //> 0x668A2D - A new target was set, restart from scratch
    if (m_newTargetSet) {
        return CreateFirstSubTask(ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL: { //> 0x668B31
        static_cast<CTaskComplexGoToPointAndStandStill*>(m_pSubTask)->GoToPoint(m_movePos, 0.5f, 2.f, false);

        if (!weaponInfo.flags.bAimWithArm) {
            break;
        }

        auto* const secondaryTask = ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_ATTACK);
        if (!secondaryTask) {
            ped->GetTaskManager().SetTaskSecondary(
                new CTaskSimpleUseGun{m_aimAtEntity, m_aimPos, eGunCommand::AIM, 1, false},
                TASK_SECONDARY_ATTACK
            );
            break;
        }

        auto* const useGun = ped->GetIntelligence()->GetTaskUseGun();
        if (!useGun) {
            secondaryTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
            break;
        }

        useGun->ControlGun(
            ped,
            m_aimAtEntity,
            GetTaskType() == TASK_COMPLEX_GO_TO_POINT_SHOOTING && CGeneral::GetRandomNumberInRange(0, 63) == 0
                ? eGunCommand::FIRE
                : eGunCommand::AIM
        );
        break;
    }
    case TASK_SIMPLE_GUN_CTRL: { //> 0x668A62
        auto* const useGun = ped->GetIntelligence()->GetTaskUseGun();
        if (!useGun) {
            break;
        }

        //> Movement direction towards `m_movePos`, in the ped's local space (X = right, Y = backwards)
        const auto delta = m_movePos - ped->GetPosition();
        const auto& mat  = ped->GetMatrix();

        CVector2D moveSpeed{ delta.Dot(mat.GetRight()), -delta.Dot(mat.GetForward()) };
        if (const auto lenSq = moveSpeed.SquaredMagnitude(); lenSq > sq(m_moveTargetRadius)) {
            moveSpeed /= std::sqrt(lenSq);
        } else {
            moveSpeed = {};
        }

        useGun->ControlGunMove(moveSpeed);
        break;
    }
    default:
        break;
    }

    return m_pSubTask;
}
