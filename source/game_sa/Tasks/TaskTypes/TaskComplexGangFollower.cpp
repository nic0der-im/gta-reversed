#include "StdInc.h"

#include "EventPassObject.h"
#include "TaskSimpleRunAnim.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskComplexGangLeader.h"
#include "TaskSimpleHoldEntity.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskComplexPassObject.h"
#include "TaskComplexFollowNodeRoute.h"
#include "TaskSimpleGoToPoint.h"
#include "SeekEntity/PosCalculators/EntitySeekPosCalculatorXYOffset.h"
#include "TaskComplexSeekEntity.h"
#include "TaskComplexSignalAtPed.h"
#include "TaskComplexGangFollower.h"
#include "TaskComplexFollowLeaderInFormation.h"
#include "TaskComplexWanderGang.h"
#include "TaskSimpleStandStill.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimplePause.h"
#include "TaskSimpleCarDrive.h"
#include "TaskComplexLeaveCar.h"

void CTaskComplexGangFollower::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexGangFollower, 0x86F938, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65EAA0);
    RH_ScopedInstall(Destructor, 0x65EBB0);
    //RH_ScopedInstall(CalculateOffsetPosition, 0x65ED40, { .Reversed = false }); // not hooked because i want to keep CVector return, but original function took a CVector&
    RH_ScopedInstall(Clone, 0x65ECB0);
    RH_ScopedInstall(MakeAbortable, 0x65EC30);
    RH_ScopedInstall(CreateNextSubTask, 0x665E00);
    RH_ScopedInstall(CreateFirstSubTask, 0x666160);
    RH_ScopedInstall(ControlSubTask, 0x662A10);
}

// 0x65EAA0
CTaskComplexGangFollower::CTaskComplexGangFollower(CPedGroup* pedGroup, CPed* ped, uint8 a4, CVector pos, float a6) :
    m_PedGroup{pedGroup},
    m_Leader{ped},
    m_PedPosn{},
    dword20{pos},
    dword2C{pos},
    dword38{a6},
    byte3C{a4}
{
    m_Flags = 0;
    m_FollowLeader = true;
    m_IsUsingStandingStillOffsets = true;

    if (m_Leader) {
        CEntity::SafeRegisterRef(m_Leader);
        m_PedPosn = m_Leader->GetPosition();
    }

    m_IsInPlayersGroup = m_Leader == FindPlayerPed();
}

CTaskComplexGangFollower::CTaskComplexGangFollower(const CTaskComplexGangFollower& o) :
    CTaskComplexGangFollower{
        o.m_PedGroup,
        o.m_Leader,
        o.byte3C,
        o.dword20,
        o.dword38
    }
{
    m_FollowLeader = o.m_FollowLeader;
}

// 0x65EBB0
CTaskComplexGangFollower::~CTaskComplexGangFollower() {
    CEntity::SafeCleanUpRef(m_Leader);
    if (m_AnimsRef) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
    }
}

// 0x65ED40
CVector CTaskComplexGangFollower::CalculateOffsetPosition() {
    if (notsa::contains({ PEDMOVE_WALK, PEDMOVE_RUN, PEDMOVE_SPRINT }, m_Leader->m_nMoveState)) {
        dword20 = m_Leader->GetMatrix().TransformVector(CVector{ CTaskComplexFollowLeaderInFormation::ms_offsets.MovingOffsets[byte3C] });
        m_IsUsingStandingStillOffsets = false;
    } else if (!m_IsUsingStandingStillOffsets || (m_Leader->GetPosition() - m_PedPosn).SquaredMagnitude() > sq(3.f)) {
        m_PedPosn = m_Leader->GetPosition();
        dword20   = CVector{ CTaskComplexFollowLeaderInFormation::ms_offsets.Offsets[byte3C] };
        m_IsUsingStandingStillOffsets = true;
    }
    return dword20;
}

// 0x65EC30
bool CTaskComplexGangFollower::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (m_pSubTask && !m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }
    ped->bDontAcceptIKLookAts           = false;
    ped->bMoveAnimSpeedHasBeenSetByTask = false;
    return true;
}

// 0x665E00
CTask* CTaskComplexGangFollower::CreateNextSubTask(CPed* ped) {
    if (!m_Leader) {
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    const auto sttype = m_pSubTask->GetTaskType();

    if (m_LeaveGroup && sttype == TASK_COMPLEX_SIGNAL_AT_PED) {
        m_PedGroup->GetMembership().RemoveMember(ped);
        ped->GetTaskManager().SetTask(new CTaskComplexWanderGang{
            PEDMOVE_WALK,
            CGeneral::RandomNodeHeading(),
            30'000,
            true,
            0.5f
        }, TASK_PRIMARY_DEFAULT);
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    if (   ped->GetIntelligence()->m_AnotherStaticCounter >= 31
        || (sttype == TASK_COMPLEX_SEEK_ENTITY && notsa::contains({ PEDMOVE_NONE, PEDMOVE_STILL, PEDMOVE_TURN_L, PEDMOVE_TURN_R }, m_Leader->m_nMoveState))
    ) {
        if (m_Leader->IsPlayer()) {
            m_Leader->Say(CTX_GLOBAL_FOLLOW_ARRIVE, 0, 0.3f);
        }
        return new CTaskSimpleStandStill{ 500 };
    }

    if (notsa::contains({ TASK_SIMPLE_STAND_STILL, TASK_COMPLEX_HANDSIGNAL_ANIM }, sttype)) {
        if (m_LeaveGroup) {
            return new CTaskComplexSignalAtPed{ m_Leader, -1, false };
        }
        if (CGeneral::GetRandomNumberInRange(0, 30) == 20) {
            auto rndMember = m_PedGroup->GetMembership().GetRandom();
            if (rndMember == ped) {
                rndMember = m_PedGroup->GetMembership().GetLeader();
            }
            if (rndMember) {
                return new CTaskComplexTurnToFaceEntityOrCoord{ rndMember };
            }
        }
        return new CTaskSimplePause{ 50 };
    }

    if (sttype == TASK_SIMPLE_CAR_DRIVE) {
        return CreateFirstSubTask(ped);
    }

    if (!m_FollowLeader) {
        return new CTaskSimpleStandStill{ 500 };
    }

    auto* const seekTask = new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
        m_Leader,
        50'000,
        1'000,
        0.5f,
        5.f,
        2.f,
        false,
        false,
        CEntitySeekPosCalculatorXYOffset{dword20}
    };
    seekTask->SetMoveState(PEDMOVE_SPRINT);
    return seekTask;
}

// 0x666160
CTask* CTaskComplexGangFollower::CreateFirstSubTask(CPed* ped) {
    if (!m_Leader) {
        ped->bMoveAnimSpeedHasBeenSetByTask = false;
        return nullptr;
    }

    if (ped->IsInVehicle()) {
        if (ped->m_pVehicle == m_Leader->m_pVehicle) {
            return new CTaskSimpleCarDrive{ ped->m_pVehicle };
        }
        return new CTaskComplexLeaveCar{ ped->m_pVehicle, 0, 0, true, false };
    }

    if (m_FollowLeader && ped->GetIntelligence()->m_AnotherStaticCounter <= 30) {
        auto* const seekTask = new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>{
            m_Leader,
            50'000,
            1'000,
            0.5f,
            5.f,
            2.f,
            false,
            true,
            CEntitySeekPosCalculatorXYOffset{dword20}
        };
        seekTask->SetMoveState(PEDMOVE_SPRINT);
        return seekTask;
    }

    return new CTaskSimpleStandStill{ 500 };
}

// 0x662A10
CTask* CTaskComplexGangFollower::ControlSubTask(CPed* ped) {
    ped->bDontAcceptIKLookAts = false;

    if (!m_Leader) {
        if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            ped->bMoveAnimSpeedHasBeenSetByTask = false;
            return nullptr;
        }
        return m_pSubTask;
    }

    const auto sttype = m_pSubTask->GetTaskType();
    if (sttype == TASK_SIMPLE_CAR_DRIVE) {
        return m_pSubTask;
    }

    const auto walkAnim        = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_WALK);
    const auto isPedMovingFast = notsa::contains({ PEDMOVE_WALK, PEDMOVE_RUN, PEDMOVE_SPRINT }, ped->GetIntelligence()->GetMoveStateFromGoToTask());

    dword20 = CalculateOffsetPosition();

    const auto leaderToPed       = m_Leader->GetPosition() - ped->GetPosition();
    float      leaderToPedDist2D;
    CVector2D{ leaderToPed }.Normalized(&leaderToPedDist2D);

    if (sttype == TASK_COMPLEX_SEEK_ENTITY && m_FollowLeader) {
        const auto stSeekEntity = static_cast<CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>*>(m_pSubTask);
        stSeekEntity->GetSeekPosCalculator().SetOffset(dword20);
        stSeekEntity->SetEntityMinDist2D(2.f);

        const auto tGoToPoint = ped->GetTaskManager().Find<CTaskSimpleGoToPoint>();
        bool hasChangedAnimSpeed = false;
        if (tGoToPoint && !ped->GetTaskManager().Find<CTaskComplexFollowNodeRoute>()) {
            auto       goToPoint         = m_Leader->GetPosition() + dword20;
            const auto goToPointToPedDir = CVector2D{ goToPoint - ped->GetPosition() };
            if (isPedMovingFast && m_Leader->GetPosition().Dot(m_Leader->GetForward()) <= goToPointToPedDir.Dot(CVector2D{ m_Leader->GetForward() })) {
                if (goToPointToPedDir.SquaredMagnitude() >= sq(tGoToPoint->m_fRadius)) {
                    if (walkAnim) {
                        const auto prevWalkAnimSpeed = walkAnim->GetSpeed();
                        ped->SetMoveAnimSpeed(walkAnim);
                        const auto newWalkAnimSpeed = walkAnim->GetSpeed();
                        if (!approxEqual(prevWalkAnimSpeed, newWalkAnimSpeed, 0.013f)) {
                            walkAnim->SetSpeed(newWalkAnimSpeed < prevWalkAnimSpeed
                                ? prevWalkAnimSpeed - 0.0125f
                                : prevWalkAnimSpeed + 0.0125f
                            );
                            hasChangedAnimSpeed = true;
                        }
                    }
                } else {
                    goToPoint += m_Leader->GetForward() * 2.f;
                    if (walkAnim) {
                        walkAnim->SetSpeed(std::max(0.85f, walkAnim->GetSpeed() - 0.0125f));
                        hasChangedAnimSpeed = true;
                    }
                }
                tGoToPoint->UpdatePoint(goToPoint);
            }
        }
        if (ped->GetIntelligence()->m_AnotherStaticCounter > 8 && leaderToPedDist2D <= 8.f && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return new CTaskSimpleStandStill{ 500 };
        }
        ped->bMoveAnimSpeedHasBeenSetByTask = hasChangedAnimSpeed;
    }

    {
        bool animsRef = m_AnimsRef;
        CAnimManager::StreamAnimBlock("gangs", CTaskComplexGangLeader::ShouldLoadGangAnims(), animsRef);
        m_AnimsRef = animsRef;
    }

    if (!m_PedGroup->m_bIsMissionGroup && !m_IsInPlayersGroup && m_PedGroup->GetMembership().CountMembers() > 3 && CGeneral::GetRandomNumberInRange(0, 2000) == 500) {
        m_LeaveGroup = true;
    }

    if (m_ExhaleTimer.IsStarted() && m_ExhaleTimer.IsOutOfTime() && ped->GetRpClump()) {
        if (const auto fx = g_fxMan.CreateFxSystem("exhale", CVector{ 0.f, 0.1f, 0.f }, RwFrameGetMatrix(RpClumpGetFrame(ped->GetRpClump())), false)) {
            fx->AttachToBone(ped, BONE_HEAD);
            fx->PlayAndKill();
        }
        m_ExhaleTimer.Stop();
    }

    if (!ped->IsVisible()) {
        return m_pSubTask;
    }

    if (!g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 100) > 95) {
        auto rndMember = m_PedGroup->GetMembership().GetRandom();
        if (rndMember == ped) {
            rndMember = m_PedGroup->GetMembership().GetLeader();
        }
        if (rndMember) {
            g_ikChainMan.LookAt(
                "TaskGangFollower",
                ped,
                rndMember,
                CGeneral::GetRandomNumberInRange(3000, 5000),
                BONE_HEAD,
                nullptr,
                true,
                0.15f,
                500,
                3,
                false
            );
        }
    }

    if (m_AnimsRef == 0 || ped->m_nMoveState > PEDMOVE_JOG) {
        return m_pSubTask;
    }

    if (const auto pedHeldEntity = ped->GetEntityThatThisPedIsHolding()) {
        const auto GetPedAnim = [&](AnimationId id) {
            return RpAnimBlendClumpGetAssociation(ped->GetRpClump(), id);
        };

        const auto drnkBrAnim  = GetPedAnim(ANIM_ID_DRNKBR_PRTL);
        const auto drnkBrAnimF = GetPedAnim(ANIM_ID_DRNKBR_PRTL_F);
        const auto smkCigAnim  = GetPedAnim(ANIM_ID_SMKCIG_PRTL);
        const auto smkCigAnimF = GetPedAnim(ANIM_ID_SMKCIG_PRTL_F);

        if (drnkBrAnim || drnkBrAnimF || smkCigAnim || smkCigAnimF) {
            if (g_ikChainMan.IsLooking(ped)) {
                g_ikChainMan.AbortLookAt(ped, 250);
            }
            ped->bDontAcceptIKLookAts = true;
            if (   ((smkCigAnim && smkCigAnim->GetCurrentTime() < 0.5f) || (smkCigAnimF && smkCigAnimF->GetCurrentTime() < 0.5f))
                && !m_ExhaleTimer.IsStarted()
            ) {
                m_ExhaleTimer.Start(2700);
            }
        }

        if (ped->GetTaskManager().Has<CTaskComplexPassObject>()) {
            return m_pSubTask;
        }

        if (CGeneral::GetRandomNumberInRange(0, 500) != 200) {
            if (CGeneral::GetRandomNumberInRange(0, 100) == 50) {
                if (const auto holdEntityTask = ped->GetTaskManager().Find<CTaskSimpleHoldEntity>()) {
                    holdEntityTask->PlayAnim(CTaskComplexGangLeader::GetRandomGangAmbientAnim(ped, pedHeldEntity), ANIM_GROUP_GANGS);
                }
            }
            return m_pSubTask;
        }

        if (drnkBrAnim || drnkBrAnimF || smkCigAnim || smkCigAnimF) {
            return m_pSubTask;
        }

        const auto passHeldEntityToPed = CTaskComplexGangLeader::TryToPassObject(ped, m_PedGroup);
        if (!passHeldEntityToPed || passHeldEntityToPed->GetEntityThatThisPedIsHolding() || !ped->GetActiveWeapon().IsTypeMelee()) {
            return m_pSubTask;
        }

        if (pedHeldEntity->GetModelIndex() == ModelIndices::MI_GANG_DRINK) {
            if (CGeneral::GetRandomNumberInRange(0, 500) < 250) {
                passHeldEntityToPed->Say(CTX_GLOBAL_BOOZE_REQUEST);
            } else {
                ped->Say(CTX_GLOBAL_BOOZE_RECEIVE, 1500);
            }
        } else if (pedHeldEntity->GetModelIndex() == ModelIndices::MI_GANG_SMOKE) {
            if (CGeneral::GetRandomNumberInRange(0, 500) < 250) {
                passHeldEntityToPed->Say(CTX_GLOBAL_SPLIFF_REQUEST);
            } else {
                ped->Say(CTX_GLOBAL_SPLIFF_RECEIVE, 1500);
            }
        }

        passHeldEntityToPed->GetEventGroup().Add(CEventPassObject{ ped, true });
        return new CTaskComplexPassObject{ passHeldEntityToPed, true };
    }

    if (ped->IsPlayingHandSignal() || ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
        return m_pSubTask;
    }

    const auto rndNum = CGeneral::GetRandomNumberInRange(0, 500);
    if (rndNum > 50 && rndNum < 56) {
        ped->GetTaskManager().SetTaskSecondary(
            new CTaskSimpleRunAnim{ANIM_GROUP_GANGS, CAnimManager::GetRandomGangTalkAnim()},
            TASK_SECONDARY_PARTIAL_ANIM
        );
        if (ped->m_nMoveState == PEDMOVE_STILL) {
            ped->Say([] {
                switch (CGeneral::GetRandomNumberInRange(0, 10)) {
                case 0:
                case 1:
                case 2: return CTX_GLOBAL_CHAT;
                case 3:
                case 4:
                case 5:
                case 6:
                case 7: return CTX_GLOBAL_PCONV_GREET_MALE;
                case 8: return CTX_GLOBAL_BOOZE_REQUEST;
                case 9: return CTX_GLOBAL_SPLIFF_REQUEST;
                }
                NOTSA_UNREACHABLE();
            }());
        }
    } else if (rndNum == 100 && ped->m_nMoveState == PEDMOVE_STILL) {
        ped->GetTaskManager().SetTaskSecondary(new CTaskComplexPlayHandSignalAnim{}, TASK_SECONDARY_PARTIAL_ANIM);
        ped->Say([] {
            switch (CGeneral::GetRandomNumberInRange(0, 10)) {
            case 0:
            case 1:
            case 2: return CTX_GLOBAL_CHAT;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7: return CTX_GLOBAL_PCONV_GREET_MALE;
            case 8: return CTX_GLOBAL_BOOZE_REQUEST;
            case 9: return CTX_GLOBAL_SPLIFF_REQUEST;
            }
            NOTSA_UNREACHABLE();
        }());
    }

    return m_pSubTask;
}
