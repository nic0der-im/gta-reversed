#include "StdInc.h"
#include "TaskInteriorSitAtDesk.h"

#include "Interior/InteriorInfo_t.h"
#include "Interior/InteriorManager_c.h"

void CTaskInteriorSitAtDesk::InjectHooks() {
    RH_ScopedVirtualClass(CTaskInteriorSitAtDesk, 0x87035c, 9);
    RH_ScopedCategory("Tasks/TaskTypes/Interior");

    RH_ScopedInstall(Constructor, 0x676010);
    RH_ScopedInstall(Destructor, 0x676080);

    RH_ScopedInstall(FinishAnimCB, 0x676190);
    RH_ScopedInstall(StartRandomLoopAnim, 0x677780);
    RH_ScopedInstall(StartRandomOneOffAnim, 0x677880);
    RH_ScopedVMTInstall(Clone, 0x6760E0);
    RH_ScopedVMTInstall(GetTaskType, 0x676070);
    RH_ScopedVMTInstall(MakeAbortable, 0x676150);
    RH_ScopedVMTInstall(ProcessPed, 0x677920);
}

// 0x676010
CTaskInteriorSitAtDesk::CTaskInteriorSitAtDesk(int32 duration, InteriorInfo_t* interiorInfo, bool bDoInstantly) :
    m_bDoInstantly{bDoInstantly},
    m_InteriorInfo{interiorInfo},
    m_Duration{duration}
{
}

// 0x6760E0
CTaskInteriorSitAtDesk::CTaskInteriorSitAtDesk(const CTaskInteriorSitAtDesk& o) :
    CTaskInteriorSitAtDesk{o.m_Duration, o.m_InteriorInfo, o.m_bDoInstantly}
{
}

// 0x676080
CTaskInteriorSitAtDesk::~CTaskInteriorSitAtDesk() {
    if (m_Anim) {
        m_Anim->SetDefaultFinishCallback();
    }
}

// 0x676190
void CTaskInteriorSitAtDesk::FinishAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = notsa::cast<CTaskInteriorSitAtDesk>(static_cast<CTask*>(data));

    assert(self && anim);
    assert(anim == self->m_Anim);

    if (anim->m_AnimId == ANIM_ID_OFF_SIT_2IDLE_180 || self->m_bTaskAborting && self->m_PrevAnimId == ANIM_ID_OFF_SIT_IN) {
        anim->SetBlendDelta(-1000.f);
        self->m_bTaskFinished = true;
    }

    self->m_Anim = nullptr;
}

// 0x677780
void CTaskInteriorSitAtDesk::StartRandomLoopAnim(CPed* ped, float blendDelta) {
    using CGeneral::GetRandomNumberInRange;

    const auto chance = GetRandomNumberInRange(100u);
    StartAnim(
        ped,
        chance > 40
            ? ANIM_ID_OFF_SIT_TYPE_LOOP
            : chance > 10
                ? ANIM_ID_OFF_SIT_BORED_LOOP
                : ANIM_ID_OFF_SIT_IDLE_LOOP,
        blendDelta
    );
    m_AnimTimer.Start(chance > 40 ? GetRandomNumberInRange(2000, 5000) : GetRandomNumberInRange(7000, 12000));
}

// 0x677880
void CTaskInteriorSitAtDesk::StartRandomOneOffAnim(CPed* ped) {
    const auto chance = CGeneral::GetRandomNumberInRange(100u);
    StartAnim(
        ped,
        chance > 60
            ? ANIM_ID_OFF_SIT_READ
            : chance > 35
                ? ANIM_ID_OFF_SIT_DRINK
                : chance > 10
                    ? ANIM_ID_OFF_SIT_WATCH
                    : ANIM_ID_OFF_SIT_CRASH,
        4.f
    );
}

// 0x676150
bool CTaskInteriorSitAtDesk::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        if (m_Anim) {
            m_Anim->SetBlendDelta(-1000.f);
            m_Anim->SetDefaultFinishCallback();
            m_Anim = nullptr;
        }
        return true;
    }
    m_bTaskAborting = true;
    return false;
}

// 0x677920
bool CTaskInteriorSitAtDesk::ProcessPed(CPed* ped) {
    const auto currAnimId = m_Anim
        ? m_Anim->GetAnimId()
        : ANIM_ID_UNDEFINED;

    ped->SetMoveState(PEDMOVE_STILL);

    if (m_bTaskFinished && !RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_OFF_SIT_2IDLE_180)) { // The stand-up anim has really finished, so turn around
        const auto angle = CGeneral::LimitRadianAngle(ped->m_fCurrentRotation + PI);
        ped->m_fAimingRotation = angle;
        ped->m_fCurrentRotation = angle;
        if (ped->m_matrix) {
            ped->m_matrix->SetRotateZOnly(angle);
        } else {
            ped->m_placement.m_fHeading = angle;
        }
        return true;
    }

    const auto CreateAnim = [&, this](AnimationId animId, float blendDelta) {
        m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_OFFICE, animId, blendDelta);
        m_Anim->SetFinishCallback(FinishAnimCB, this);
    };

    const auto StandUp = [&, this] {
        // Note: `SetDeleteCallback` replaces `FinishAnimCB` set by `CreateAnim` (They share the same slot)
        m_Anim->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB);
        CreateAnim(ANIM_ID_OFF_SIT_2IDLE_180, 1000.f);
        m_bUpdatePedPos = true;
    };

    if (m_bTaskAborting) {
        if (!InteriorManager_c::AreAnimsLoaded(2)) { // `int_office`
            return true;
        }

        // Note: If there's no anim `currAnimId` is `ANIM_ID_UNDEFINED`, in which case none of the branches below apply
        if (currAnimId == ANIM_ID_OFF_SIT_IN) {
            m_Anim->SetBlendDelta(-8.f);
        } else if (currAnimId < ANIM_ID_OFF_SIT_IDLE_LOOP || currAnimId > ANIM_ID_OFF_SIT_WATCH) {
            if (currAnimId == ANIM_ID_OFF_SIT_2IDLE_180) {
                m_Anim->m_Speed = 2.f;
            }
        } else if (!m_bUpdatePedPos) { // Currently in a loop/one-off anim, so stand up
            StandUp();
            return false;
        }
    }

    if (!m_Anim) {
        if (!InteriorManager_c::AreAnimsLoaded(2)) { // `int_office`
            return false;
        }

        if (m_PrevAnimId == ANIM_ID_UNDEFINED) {
            if (!m_bDoInstantly) {
                CreateAnim(ANIM_ID_OFF_SIT_IN, 2.f);
                return false;
            }
        } else if (m_PrevAnimId != ANIM_ID_OFF_SIT_IN) {
            if (m_PrevAnimId < ANIM_ID_OFF_SIT_CRASH || m_PrevAnimId > ANIM_ID_OFF_SIT_WATCH) { // Not a loop/one-off anim
                return false;
            }
            StartRandomLoopAnim(ped, 2.f);
            return false;
        }

        if (m_Duration >= 0) {
            m_TaskTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_TaskTimer.m_nInterval  = m_Duration;
            m_TaskTimer.m_bStarted   = true;
        }

        StartRandomLoopAnim(ped, 1000.f); // Sit down
        m_bUpdatePedPos = true;
        return false;
    }

    if (m_bUpdatePedPos) {
        CVector pos = ped->GetPosition();
        // These anims have a seat offset that has to be applied
        if (currAnimId == ANIM_ID_OFF_SIT_2IDLE_180 || (currAnimId >= ANIM_ID_OFF_SIT_IDLE_LOOP && currAnimId <= ANIM_ID_OFF_SIT_BORED_LOOP)) {
            auto animOffsetOS = CCarEnterExit::ms_vecPedDeskAnimOffset;
            if (currAnimId == ANIM_ID_OFF_SIT_2IDLE_180) {
                animOffsetOS = -animOffsetOS;
            }
            const auto animOffsetWS = ped->m_matrix->TransformPoint(animOffsetOS); // Transform to world space
            pos.x = animOffsetWS.x;
            pos.y = animOffsetWS.y;
        }
        ped->SetPosn(pos);
        m_bUpdatePedPos = false;
    }

    if (m_TaskTimer.m_bStarted && m_TaskTimer.IsOutOfTime()) { // Time to get up
        if (m_Anim->GetAnimId() != ANIM_ID_OFF_SIT_2IDLE_180) { // Not already standing up
            StandUp();
        }
    } else if (m_AnimTimer.m_bStarted && m_AnimTimer.IsOutOfTime()) { // Play another random anim
        m_AnimTimer.m_bStarted = false;
        if (rand() < 0x3FFF) {
            StartRandomOneOffAnim(ped);
        } else {
            StartRandomLoopAnim(ped, 2.f);
        }
    }

    if (m_Anim->GetAnimId() == ANIM_ID_OFF_SIT_IN) { // Slide onto the seat (While sitting down)
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

    return false;
}

void CTaskInteriorSitAtDesk::StartAnim(CPed* ped, AnimationId animId, float blendDelta) {
    if (m_Anim) {
        m_Anim->SetDefaultFinishCallback();
    }
    m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_OFFICE, animId, blendDelta);
    m_Anim->SetFinishCallback(FinishAnimCB, this);
}
