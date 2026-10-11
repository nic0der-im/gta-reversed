#include "StdInc.h"
#include "TaskInteriorSitAtDesk.h"
#include "InteriorInfo_t.h"
#include "InteriorManager_c.h"
#include "CarEnterExit.h"

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
    constexpr float ORIGINAL_PI = 3.1415927f; // 0x858CB8 (Not the same as `PI`)

    const AnimationId animId = m_Anim ? m_Anim->GetAnimId() : ANIM_ID_UNDEFINED;

    ped->SetMoveState(PEDMOVE_STILL);

    // Turn around after the stand up animation finished
    if (m_bTaskFinished && !RpAnimBlendClumpGetAssociation(ped->GetRpClump(), (uint32)ANIM_ID_OFF_SIT_2IDLE_180)) {
        const auto angle = CGeneral::LimitRadianAngle(ped->m_fCurrentRotation + ORIGINAL_PI);
        ped->m_fAimingRotation = ped->m_fCurrentRotation = angle;
        if (ped->m_matrix) {
            ped->m_matrix->SetRotateZOnly(angle);
        } else {
            ped->m_placement.m_fHeading = angle;
        }
        return true;
    }

    // 0x6779C0
    if (m_bTaskAborting) {
        if (!InteriorManager_c::AreAnimsLoaded(2)) {
            return true;
        }
        if (animId == ANIM_ID_OFF_SIT_IN) {
            m_Anim->SetBlendDelta(-8.f);
        } else if (animId < ANIM_ID_OFF_SIT_IDLE_LOOP || animId > ANIM_ID_OFF_SIT_WATCH) {
            if (animId == ANIM_ID_OFF_SIT_2IDLE_180) {
                m_Anim->m_Speed = 2.f;
            }
        } else if (!m_bUpdatePedPos) {
            m_Anim->SetDefaultDeleteCallback();
            m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_OFFICE, ANIM_ID_OFF_SIT_2IDLE_180, 1000.f);
            m_Anim->SetFinishCallback(FinishAnimCB, this);
            m_bUpdatePedPos = true;
            return false;
        }
    }

    // 0x677A5E
    if (!m_Anim) {
        if (!InteriorManager_c::AreAnimsLoaded(2)) {
            return false;
        }
        if (m_PrevAnimId == ANIM_ID_UNDEFINED) {
            if (!m_bDoInstantly) {
                m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_OFFICE, ANIM_ID_OFF_SIT_IN, 4.f);
                m_Anim->SetFinishCallback(FinishAnimCB, this);
                return false;
            }
        } else if (m_PrevAnimId != ANIM_ID_OFF_SIT_IN) {
            if (m_PrevAnimId >= ANIM_ID_OFF_SIT_BORED_LOOP + 1 && m_PrevAnimId <= ANIM_ID_OFF_SIT_WATCH) { // 0x14D..0x150
                StartRandomLoopAnim(ped, 4.f);
            }
            return false;
        }
        m_TaskTimer.Start(m_Duration); // Does nothing if `m_Duration < 0`
        StartRandomLoopAnim(ped, 1000.f);
        m_bUpdatePedPos = true;
        return false;
    }

    // 0x677B3A
    if (m_bUpdatePedPos) {
        const CVector pos = ped->GetPosition();
        CVector       newPos = pos;
        if (animId >= ANIM_ID_OFF_SIT_IDLE_LOOP && animId <= ANIM_ID_OFF_SIT_BORED_LOOP) {
            newPos = ped->GetMatrix().TransformPoint(CCarEnterExit::ms_vecPedDeskAnimOffset);
        } else if (animId == ANIM_ID_OFF_SIT_2IDLE_180) {
            newPos = ped->GetMatrix().TransformPoint(-CCarEnterExit::ms_vecPedDeskAnimOffset);
        }
        newPos.z = pos.z; // Z is always kept
        ped->SetPosn(newPos);
        m_bUpdatePedPos = false;
    }

    // 0x677BF2
    if (m_TaskTimer.m_bStarted && m_TaskTimer.IsOutOfTime()) {
        if (m_Anim->m_AnimId != ANIM_ID_OFF_SIT_2IDLE_180) {
            m_Anim->SetDefaultDeleteCallback();
            m_Anim = CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_INT_OFFICE, ANIM_ID_OFF_SIT_2IDLE_180, 1000.f);
            m_Anim->SetFinishCallback(FinishAnimCB, this);
            m_bUpdatePedPos = true;
        }
    } else if (m_AnimTimer.m_bStarted && m_AnimTimer.IsOutOfTime()) { // 0x677C49
        m_AnimTimer.m_bStarted = false;
        if (rand() < 0x3FFF) { // NOTE: Not `CGeneral::GetRandomNumber`, it's the CRT `rand`
            StartRandomOneOffAnim(ped);
        } else {
            StartRandomLoopAnim(ped, 4.f);
        }
    }

    // 0x677C81 - Move ped towards the interior's position while sitting down
    if (m_Anim->m_AnimId == ANIM_ID_OFF_SIT_IN) {
        const auto  toInterior = m_InteriorInfo->Pos - ped->GetPosition();
        const float dist       = toInterior.Magnitude();
        const float limit      = dist < 0.02f ? dist : 0.02f;
        const auto  shift      = toInterior * (1.f / dist) * limit;

        const auto& mat = ped->GetMatrix();
        ped->m_vecAnimMovingShiftLocal.x = mat.GetRight().Dot(shift);
        ped->m_vecAnimMovingShiftLocal.y = mat.GetForward().Dot(shift);

        ped->m_fAimingRotation = CGeneral::LimitRadianAngle(
            CGeneral::GetRadianAngleBetweenPoints(m_InteriorInfo->Dir.x, m_InteriorInfo->Dir.y, 0.f, 0.f)
        );
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
