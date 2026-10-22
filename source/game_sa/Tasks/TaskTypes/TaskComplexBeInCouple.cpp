#include "StdInc.h"

#include "TaskComplexBeInCouple.h"
#include "TaskComplexWanderStandard.h"
#include "TaskComplexWalkAlongsidePed.h"
#include "Ragdoll/IKChainManager.h"

namespace {
/*!
 * @addr 0x683840
 * @brief On which side (From the partner's point of view) the `ped` is.
 * @returns `2` (`WalkSide::LEFT`) if the ped is on the partner's left, `1` (`WalkSide::RIGHT`) otherwise.
 *
 * @note In retail this is a method of `CTaskComplexBeInCouple`, but it isn't declared in the header
 *       (Which is out of this unit's ownership), so it's a free function here instead.
 */
int32 GetWalkSide(CPed* ped, CPed* partner) {
    const auto& right = partner->GetMatrix().GetRight();
    return right.Dot(ped->GetPosition()) - right.Dot(partner->GetPosition()) < 0.0f
        ? 2  // LEFT
        : 1; // RIGHT
}
} // namespace

// TODO: Inject Hooks

// 0x6836F0
CTaskComplexBeInCouple::CTaskComplexBeInCouple(
    CPed* ped,
    bool  isLeader,
    bool  holdHands,
    bool  lookAtEachOther,
    float giveUpDist
) :
    CTaskComplex(),
    m_partner{ped},
    m_isLeader{isLeader},
    m_holdHands{holdHands},
    m_lookAtEachOther{lookAtEachOther},
    m_giveUpDist{giveUpDist}
{
    CEntity::SafeRegisterRef(m_partner);
}

// 0x683780
CTaskComplexBeInCouple::~CTaskComplexBeInCouple() {
    CEntity::SafeCleanUpRef(m_partner);
}

// 0x6837E0
CTask* CTaskComplexBeInCouple::CreateFirstSubTask(CPed* ped) {
    return CreateNextSubTask(ped);
}

// 0x6837F0
void CTaskComplexBeInCouple::AbortArmIK(CPed* ped) {
    const auto DoAbortArmIK = [ped](eIKArm arm) {
        if (IKChainManager_c::IsArmPointing(arm, ped)) {
            IKChainManager_c::AbortPointArm(arm, ped, 250);
        }
    };
    DoAbortArmIK(eIKArm::IK_ARM_RIGHT);
    DoAbortArmIK(eIKArm::IK_ARM_LEFT);
}

// 0x6847C0
bool CTaskComplexBeInCouple::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (m_partner && event && event->HasEditableResponse()) {
        auto editable = static_cast<CEventEditableResponse*>(const_cast<CEvent*>(event)); // Okay let's go!

        bool bAddToEventGroup = editable->m_bAddToEventGroup;
        editable->m_bAddToEventGroup = false;
        switch (event->GetEventType()) {
        case EVENT_DAMAGE:
        case EVENT_GUN_AIMED_AT:
        case EVENT_SHOT_FIRED:
            m_partner->GetEventGroup().Add(editable, false);
            break;
        }
        editable->m_bAddToEventGroup = bAddToEventGroup;
    }
    AbortArmIK(ped);
    return true;
}

// 0x684840
CTask* CTaskComplexBeInCouple::CreateNextSubTask(CPed* ped) {
    if (!m_partner) {
        AbortArmIK(ped);
        return nullptr;
    }

    if (m_isLeader) {
        AbortArmIK(ped);
        return new CTaskComplexWanderStandard{
            PEDMOVE_WALK,
            (uint8)CGeneral::GetRandomNumberInRange(0, 8),
            true
        };
    }

    AbortArmIK(ped);
    return new CTaskComplexWalkAlongsidePed{ m_partner, m_giveUpDist };
}

// 0x684930
CTask* CTaskComplexBeInCouple::ControlSubTask(CPed* ped) {
    if (!m_partner || !m_partner->IsAlive()) {
        AbortArmIK(ped);
        return nullptr;
    }

    // The partner has to be running the counterpart task (The leader's partner is a follower, and vice-versa)
    const auto partnerTask = m_partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_BE_IN_COUPLE);
    if (!partnerTask
        || static_cast<CTaskComplexBeInCouple*>(partnerTask)->m_isLeader == m_isLeader
        || static_cast<CTaskComplexBeInCouple*>(partnerTask)->m_partner != ped
    ) {
        AbortArmIK(ped);
        return nullptr;
    }

    const auto walkSide = static_cast<WalkSide>(GetWalkSide(ped, m_partner)); // 0x683840

    // The follower has to walk on the side of the partner it's on
    if (!m_isLeader) {
        if (ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_WALK_ALONGSIDE_PED)) {
            // TODO(re): 0x684A23 - Assign `CTaskComplexWalkAlongsidePed::m_Offset` (+0x14) the value
            //            `{ walkSide == LEFT ? -1.05f : 1.05f, 0.0f, 0.0f }`.
            //            Not possible: `m_Offset` is private and `TaskComplexWalkAlongsidePed.h` belongs to another unit.
        }
    }

    const auto delta  = m_partner->GetPosition() - ped->GetPosition();
    const auto distSq = delta.SquaredMagnitude2D();

    if (distSq > m_giveUpDist * m_giveUpDist) {
        AbortArmIK(ped);
        return nullptr;
    }

    if (!ped->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_BE_IN_COUPLE)
     || !m_partner->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_BE_IN_COUPLE)
    ) {
        AbortArmIK(ped);
        return nullptr;
    }

    // Too far apart - The leader goes back for the partner
    if (distSq > 4.0f) {
        if (m_isLeader && !ped->GetIntelligence()->FindTaskByType(TASK_COMPLEX_WAIT_FOR_PED)) {
            // TODO(re): 0x683340 - `return new CTaskComplexWaitForPed{ m_partner, 0.75f, 20'000, false };`
            //            Not possible: `CTaskComplexWaitForPed` (`TaskComplexWaitForPed.h`, another unit) declares
            //            no constructor and no `CTask` overrides, so the class is abstract.
        }
    }

    if (m_holdHands) {
        // NOTE: Retail swaps the X/Y components when building the offset (See 0x684B43)
        CVector handTargetPos = ped->GetPosition() + CVector{ delta.y * 0.5f, delta.x * 0.5f, 0.0f };

        if (distSq < 2.25f) {
            if (walkSide != static_cast<WalkSide>(m_prevSide)) {
                AbortArmIK(ped);
            }

            // The arm closest to the partner is the one to hold hands with
            if (walkSide == LEFT) {
                g_ikChainMan.PointArm("CTaskComplexBeInCouple", eIKArm::IK_ARM_RIGHT, ped, nullptr, eBoneTag::BONE_UNKNOWN, &handTargetPos, 0.5f, 250, 30.f);
                m_prevSide = static_cast<int8>(walkSide);
            } else if (walkSide == RIGHT) {
                g_ikChainMan.PointArm("CTaskComplexBeInCouple", eIKArm::IK_ARM_LEFT, ped, nullptr, eBoneTag::BONE_UNKNOWN, &handTargetPos, 0.5f, 250, 30.f);
                m_prevSide = static_cast<int8>(walkSide);
            }
        } else {
            AbortArmIK(ped);
        }
    }

    if (m_lookAtEachOther) {
        // Each ped randomly glances at the other one, unless it's already looking at it
        if (!g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 100) > 80) {
            g_ikChainMan.LookAt("TaskBeInCouple", ped, m_partner, CGeneral::GetRandomNumberInRange(2'000, 4'000), BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
        }
        if (!g_ikChainMan.IsLooking(m_partner) && CGeneral::GetRandomNumberInRange(0, 100) > 80) {
            g_ikChainMan.LookAt("TaskBeInCouple", m_partner, ped, CGeneral::GetRandomNumberInRange(2'000, 4'000), BONE_HEAD, nullptr, false, 0.25f, 500, 3, false);
        }
    }

    return m_pSubTask;
}
