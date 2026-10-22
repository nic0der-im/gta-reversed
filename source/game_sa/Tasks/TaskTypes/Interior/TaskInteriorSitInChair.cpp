#include "StdInc.h"

#include "TaskInteriorSitInChair.h"

#include "Interior/InteriorInfo_t.h"
#include "Interior/Interior_c.h"
#include "Interior/InteriorGroup_c.h"
#include "Interior/InteriorManager_c.h"
#include "Ragdoll/IKChainManager.h"

void CTaskInteriorSitInChair::InjectHooks() {
    RH_ScopedVirtualClass(CTaskInteriorSitInChair, 0x870314, 9);
    RH_ScopedCategory("Tasks/TaskTypes/Interior");

    RH_ScopedInstall(Constructor, 0x675C30);
    RH_ScopedInstall(Destructor, 0x675C90);

    RH_ScopedInstall(FinishAnimCB, 0x675DD0);

    RH_ScopedVMTInstall(Clone, 0x675CF0);
    RH_ScopedVMTInstall(GetTaskType, 0x675C80);
    RH_ScopedVMTInstall(MakeAbortable, 0x675D60);
    RH_ScopedVMTInstall(ProcessPed, 0x676D30);
}

// 0x675C30
CTaskInteriorSitInChair::CTaskInteriorSitInChair(int32 duration, InteriorInfo_t* interiorInfo, bool bDoInstantly) :
    m_Duration{duration},
    m_InteriorInfo{interiorInfo},
    m_bDoInstantly{bDoInstantly}
{
}

// 0x675CF0
CTaskInteriorSitInChair::CTaskInteriorSitInChair(const CTaskInteriorSitInChair& o) :
    CTaskInteriorSitInChair{o.m_Duration, o.m_InteriorInfo, o.m_bDoInstantly}
{
}

// 0x675C90
CTaskInteriorSitInChair::~CTaskInteriorSitInChair() {
    if (m_Anim) {
        m_Anim->SetDefaultDeleteCallback();
    }
}

// 0x675DD0
void CTaskInteriorSitInChair::FinishAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = notsa::cast<CTaskInteriorSitInChair>(static_cast<CTask*>(data));

    assert(self && anim);
    assert(anim == self->m_Anim);

    self->m_PrevAnimId = anim->GetAnimId();

    if (anim->GetAnimId() == ANIM_ID_LOU_OUT || (self->m_bTaskAborting && self->m_PrevAnimId == ANIM_ID_LOU_IN)) {
        anim->SetBlendDelta(-1000.f);
        self->m_bTaskFinished = true;
    }

    self->m_Anim = nullptr;
}

// 0x675D60
bool CTaskInteriorSitInChair::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority != ABORT_PRIORITY_IMMEDIATE) {
        m_bTaskAborting = true;
        return false;
    }
    if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 250u);
    }
    if (m_Anim) {
        m_Anim->SetBlendDelta(-1000.f);
        m_Anim->SetDefaultDeleteCallback();
        m_Anim = nullptr;
    }
    return true;
}

// 0x676D30
bool CTaskInteriorSitInChair::ProcessPed(CPed* ped) {
    const auto currAnimId = m_Anim
        ? m_Anim->GetAnimId()
        : ANIM_ID_UNDEFINED;

    ped->SetMoveState(PEDMOVE_STILL);

    if (m_bTaskFinished && !RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_LOU_OUT)) { // The get-up anim has really finished
        return true;
    }

    if (m_bTaskAborting) {
        if (!InteriorManager_c::AreAnimsLoaded(0)) { // `int_house`
            return true;
        }

        // Note: If there's no anim `currAnimId` is `ANIM_ID_UNDEFINED`, in which case none of the branches below apply
        if (currAnimId == ANIM_ID_LOU_IN) {
            m_Anim->SetBlendDelta(-8.f);
        } else if (currAnimId == ANIM_ID_LOU_LOOP) {
            if (!m_bUpdatePedPos) { // Currently sitting, so get up
                StartAnim(ped, ANIM_ID_LOU_OUT, 1000.f);
                m_bUpdatePedPos = true;
                return false;
            }
        } else if (currAnimId == ANIM_ID_LOU_OUT) {
            m_Anim->m_Speed = 3.f;
        }
    }

    if (!m_Anim) {
        if (InteriorManager_c::AreAnimsLoaded(0)) { // `int_house`
            if (m_PrevAnimId == ANIM_ID_UNDEFINED) {
                if (!m_bDoInstantly) {
                    StartAnim(ped, ANIM_ID_LOU_IN, 4.f);
                } else {
                    m_TaskTimer.Start(m_Duration);
                    StartAnim(ped, ANIM_ID_LOU_LOOP, 1000.f);
                    m_bUpdatePedPos = true;
                }
            } else if (m_PrevAnimId == ANIM_ID_LOU_IN) { // Sitting down anim finished, start idling
                m_TaskTimer.Start(m_Duration);
                StartAnim(ped, ANIM_ID_LOU_LOOP, 1000.f);
                m_bUpdatePedPos = true;
            }
        }
    } else {
        if (m_bUpdatePedPos) {
            UpdatePedPos(ped);
        }

        if (m_TaskTimer.m_bStarted && m_TaskTimer.IsOutOfTime()) { // Time to get up
            if (m_Anim->GetAnimId() != ANIM_ID_LOU_OUT) { // Not already getting up
                StartAnim(ped, ANIM_ID_LOU_OUT, 1000.f);
                m_bUpdatePedPos = true;
            }
        }

        if (m_Anim->GetAnimId() == ANIM_ID_LOU_IN) { // Slide onto the chair (While sitting down)
            const auto delta = m_InteriorInfo->Pos - ped->GetPosition();
            const auto dist  = delta.Magnitude();
            const auto step  = std::min(dist, 0.02f); // Don't move more than 0.02 units per frame
#ifdef FIX_BUGS
            const auto invDist = dist == 0.f ? 0.f : 1.f / dist; // Avoid a NaN shift when dividing by zero
#else
            const auto invDist = 1.f / dist;
#endif
            const CVector shiftWS = { delta.x * invDist * step, delta.y * invDist * step, delta.z * invDist * step };

            // Rotate the world-space shift into the ped's local space
            ped->m_vecAnimMovingShiftLocal = {
                shiftWS.Dot(ped->GetRight()),
                shiftWS.Dot(ped->GetForward()),
            };

            ped->m_fAimingRotation = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(m_InteriorInfo->Dir.x, m_InteriorInfo->Dir.y, 0.f, 0.f));
        }
    }

    // While sitting, occasionally look around
    if (m_Anim && m_Anim->GetAnimId() == ANIM_ID_LOU_LOOP) {
        if (!g_ikChainMan.IsLooking(ped) && CGeneral::GetRandomNumberInRange(0, 1000) > 980) {
            if (const auto group = g_interiorMan.GetPedsInteriorGroup(ped)) {
                InteriorInfo_t* info{};
                Interior_c*     interior{};
                float           distSq{};
                // TODO(re): Retail passes `0` as the info type here, which never matches any `eInteriorInfoType` (They start at 1),
                //           so in retail this look-at never happens. Reproduced as-is.
                if (group->FindClosestInteriorInfo(0, ped->GetPosition(), 10.f, &info, &interior, &distSq) && info) {
                    g_ikChainMan.LookAt(
                        "TaskSitInChair",
                        ped,
                        nullptr,
                        CGeneral::GetRandomNumberInRange(10'000, 20'000),
                        BONE_UNKNOWN,
                        &info->Pos,
                        false,
                        0.25f,
                        500,
                        3,
                        false
                    );
                }
            }
        }
    } else if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 250u);
    }

    return false;
}

void CTaskInteriorSitInChair::StartAnim(CPed* ped, AnimationId animId, float blendDelta) {
    if (m_Anim) {
        m_Anim->SetDefaultDeleteCallback();
    }
    m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_HOUSE, animId, blendDelta);
    m_Anim->SetFinishCallback(FinishAnimCB, this);
}

void CTaskInteriorSitInChair::UpdatePedPos(CPed* ped) {
    const auto currAnimId = m_Anim
        ? m_Anim->GetAnimId()
        : ANIM_ID_UNDEFINED;

    CVector pos = ped->GetPosition();

    if (currAnimId == ANIM_ID_LOU_LOOP || currAnimId == ANIM_ID_LOU_OUT) {
        const auto animOffsetOS = CCarEnterExit::ms_vecPedChairAnimOffset;
        const auto animOffsetWS = ped->m_matrix->TransformPoint(animOffsetOS);
        pos.x = animOffsetWS.x;
        pos.y = animOffsetWS.y;
    }

    if (currAnimId == ANIM_ID_LOU_LOOP) { // Face the opposite direction (Away from the chair's back)
        const auto angle = CGeneral::LimitRadianAngle(ped->m_fCurrentRotation + PI);
        ped->m_fAimingRotation = angle;
        ped->m_fCurrentRotation = angle;
        if (ped->m_matrix) {
            ped->m_matrix->SetRotateZOnly(angle);
        } else {
            ped->m_placement.m_fHeading = angle;
        }
    }

    ped->SetPosn(pos);
    m_bUpdatePedPos = false;
}
