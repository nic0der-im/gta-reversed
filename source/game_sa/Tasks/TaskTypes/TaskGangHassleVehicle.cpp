#include "StdInc.h"

#include "TaskGangHassleVehicle.h"

#include "AnimManager.h"
#include "Collision.h"
#include "Game.h"
#include "Ragdoll/IKChainManager.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskComplexLeaveCar.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskComplexSmartFleeEntity.h"
#include "TaskComplexTrackEntity.h"
#include "TaskComplexWander.h"
#include "TaskGangHasslePed.h"
#include "TaskSimpleFight.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleShakeFist.h"

void CTaskGangHassleVehicle::InjectHooks() {
    RH_ScopedVirtualClass(CTaskGangHassleVehicle, 0x86F9D4, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65FAC0);
    RH_ScopedInstall(Destructor, 0x65FB60);
    RH_ScopedInstall(GetTargetHeading, 0x65FDD0);
    RH_ScopedInstall(CalcTargetOffset, 0x6641A0);
    RH_ScopedInstall(Clone, 0x65FC00, { .Reversed = false });
    RH_ScopedInstall(CreateNextSubTask, 0x65FC80);
    RH_ScopedInstall(CreateFirstSubTask, 0x664BA0);
    RH_ScopedInstall(ControlSubTask, 0x6637C0);
}

// 0x65FAC0
CTaskGangHassleVehicle::CTaskGangHassleVehicle(CVehicle* vehicle, int32 a3, uint8 a4, float a5, float a6) : CTaskComplex() {
    m_nTime = 0;
    dword3C = 0;
    byte40 = 0;
    byte41 = 0;
    byte18 = a4;
    dword1C = a5;
    m_Vehicle = vehicle;
    m_nHasslePosId = -1;
    m_fOffsetX = a6;
    m_bRemoveAnim = 0;
    m_pEntity = nullptr;
    CEntity::SafeRegisterRef(m_Vehicle);
}

// 0x65FB60
CTaskGangHassleVehicle::~CTaskGangHassleVehicle() {
    if (m_Vehicle) {
        if (m_nHasslePosId > -1) {
            m_Vehicle->SetHasslePosId(m_nHasslePosId, false);
        }
        CEntity::SafeCleanUpRef(m_Vehicle);
    }

    CEntity::SafeCleanUpRef(m_pEntity);

    if (m_bRemoveAnim) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bRemoveAnim = false;
    }
}

// 0x65FDD0
float CTaskGangHassleVehicle::GetTargetHeading(CPed* ped) {
    const auto& mat = m_Vehicle->GetMatrix();

    // The 2D direction the ped should face, depending on where it hassles the car from
    float x = mat.GetRight().x;
    float y = mat.GetRight().y;
    switch (m_nHasslePosId) {
    case 1:
    case 3:
        x = -x;
        y = -y;
        break;
    case 4:
        x = mat.GetForward().x;
        y = mat.GetForward().y;
        break;
    case 5:
        x = -mat.GetForward().x;
        y = -mat.GetForward().y;
        break;
    default: // 0, 2 and anything else: The vehicle's right axis
        break;
    }

    return CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(x, y, 0.f, 0.f));
}

// 0x6641A0
void CTaskGangHassleVehicle::CalcTargetOffset() {
    m_vecPosn = CVector{};

    const auto colModel = CModelInfo::GetModelInfo(m_Vehicle->m_nModelIndex)->GetColModel();
    const auto& box     = colModel->GetBoundingBox();
    const auto  off     = m_fOffsetX;

    switch (m_nHasslePosId) {
    case 0:
        m_vecPosn.x = box.m_vecMin.x - off;
        m_vecPosn.y = box.m_vecMax.y * 0.5f;
        break;
    case 1:
        m_vecPosn.x = box.m_vecMax.x + off;
        m_vecPosn.y = box.m_vecMax.y * 0.5f;
        break;
    case 2:
        m_vecPosn.x = box.m_vecMin.x - off;
        m_vecPosn.y = box.m_vecMin.y * 0.5f;
        break;
    case 3:
        m_vecPosn.x = box.m_vecMax.x + off;
        m_vecPosn.y = box.m_vecMin.y * 0.5f;
        break;
    case 4:
        m_vecPosn.y = box.m_vecMin.y - off;
        break;
    case 5:
        m_vecPosn.y = box.m_vecMax.y + off;
        break;
    default:
        break;
    }
}

// 0x65FC80
CTask* CTaskGangHassleVehicle::CreateNextSubTask(CPed* ped) {
    if (!m_Vehicle) {
        return nullptr;
    }

    const auto subTaskType  = m_pSubTask ? m_pSubTask->GetTaskType() : (eTaskType)-1;
    const bool hasSubTask   = m_pSubTask != nullptr;

    // Already fleeing? Keep it
    if (hasSubTask && subTaskType == TASK_COMPLEX_SMART_FLEE_ENTITY) {
        return nullptr;
    }

    if (m_Vehicle->m_fHealth >= 250.f) { // Still in one piece: Track the vehicle
        if (hasSubTask && (subTaskType == TASK_COMPLEX_GANG_HASSLE_PED || subTaskType == TASK_COMPLEX_TRACK_ENTITY)) {
            return nullptr;
        }
        return new CTaskComplexTrackEntity(m_Vehicle, m_vecPosn, 1, -1, 10.f, 40.f, 1);
    }

    // The vehicle is nearly wrecked - back off
    // TODO(re: The last argument is `DAT_00C18CF0`, which is runtime-initialised, so its value can't
    // be read from the static image.
    return new CTaskComplexSmartFleeEntity(m_Vehicle, false, 30.f, 1'000'000, 1000, 0.f);
}

// 0x664BA0
CTask* CTaskGangHassleVehicle::CreateFirstSubTask(CPed* ped) {
    if (!m_Vehicle) {
        return nullptr;
    }

    // The ped hassles the driver
    m_pEntity = m_Vehicle->m_pDriver;
    CEntity::SafeRegisterRef(m_pEntity);

    const auto& box = CModelInfo::GetModelInfo(m_Vehicle->m_nModelIndex)->GetColModel()->GetBoundingBox();

    // Only relatively small vehicles can be hassled
    if (box.m_vecMax.x - box.m_vecMin.x > 4.f || box.m_vecMax.y - box.m_vecMin.y > 8.f) {
        return nullptr;
    }

    m_nHasslePosId = m_Vehicle->GetSpareHasslePosId();
    if (m_nHasslePosId == -1) { // All the slots are taken
        return nullptr;
    }

    m_Vehicle->SetHasslePosId(m_nHasslePosId, true);
    CalcTargetOffset();

    m_b31 = false;
    ped->DropEntityThatThisPedIsHolding(true);
    m_nTime = CTimer::GetTimeInMS();
    dword3C = CGeneral::GetRandomNumberInRange(150'000, 250'000);
    byte40  = true;

    if (!ped->bInVehicle || !ped->m_pVehicle) {
        return CreateNextSubTask(ped);
    }
    return new CTaskComplexLeaveCar(ped->m_pVehicle, 0, 0, true, false);
}

// 0x6637C0
CTask* CTaskGangHassleVehicle::ControlSubTask(CPed* ped) {
    // The vehicle's gone (e.g. it drove off) - abort whatever we're doing
    if (m_pSubTask && m_pSubTask->GetTaskType() != (eTaskType)1000 && !m_Vehicle) {
        if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return nullptr;
        }
        return m_pSubTask;
    }

    // Keep the "gangs" animations loaded while they're needed
    if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        if (!m_bRemoveAnim) {
            const auto blk = CAnimManager::GetAnimationBlockIndex("gangs");
            if (CAnimManager::GetAnimBlocks()[blk].IsLoaded) {
                CAnimManager::AddAnimBlockRef(blk);
                m_bRemoveAnim = true;
            } else {
                CStreaming::RequestModel(IFPToModelId(blk), STREAMING_KEEP_IN_MEMORY);
            }
        }
    } else if (m_bRemoveAnim) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bRemoveAnim = false;
    }

    // Time to get aggressive?
    if (byte40) {
        if (byte41) {
            m_nTime = CTimer::GetTimeInMS();
            byte41  = false;
        }
        if ((uint32)(m_nTime + dword3C) <= CTimer::GetTimeInMS() && byte18) {
            if (m_pSubTask->GetTaskType() != TASK_COMPLEX_KILL_PED_ON_FOOT) { // (See `0x6639xx`)
                return new CTaskComplexKillPedOnFoot(static_cast<CPed*>(m_pEntity), -1, 0, 0, 0, 1);
            }
            return m_pSubTask;
        }
    }

    // The driver got out - hassle him instead
    if (m_Vehicle->m_pDriver == nullptr && m_pEntity) {
        if (m_pSubTask->GetTaskType() != TASK_COMPLEX_GANG_HASSLE_PED) {
            // NOTE: The vanilla code sets the state to `3` here (Which no `case` of the state machine
            // below matches, so the state machine is effectively parked). `m_b31` is a `bool` in the
            // repo, so only `true` can be stored - that lands in the "face the vehicle" state instead.
            m_b31 = true;
            return new CTaskGangHasslePed(static_cast<CPed*>(m_pEntity), byte18 ? 2 : 1, 12'000, 20'000);
        }
    }

    // Look at the driver every now and then
    {
        const auto trackTask = m_pSubTask->GetTaskType() == TASK_COMPLEX_TRACK_ENTITY
            ? static_cast<CTaskComplexTrackEntity*>(m_pSubTask)
            : nullptr;
        const auto distToVehicleSq = trackTask ? trackTask->m_distToTargetSq : 100.f;

        if (m_Vehicle->IsVisible() && distToVehicleSq < 4.f && !g_ikChainMan.IsLooking(ped)) {
            if (CGeneral::GetRandomNumberInRange(0, 100) > 60) {
                if (const auto driver = m_Vehicle->m_pDriver) {
                    g_ikChainMan.LookAt(
                        "TaskHassleVehicle", ped, driver, CGeneral::GetRandomNumberInRange(1000, 3000),
                        BONE_HEAD, nullptr, true, 0.3f, 500, 3, false
                    );
                }
            }
        }
    }

    // The hassle state machine only runs while tracking/hassling the ped
    const auto subTaskType = m_pSubTask->GetTaskType();
    if (subTaskType != TASK_COMPLEX_TRACK_ENTITY && subTaskType != TASK_COMPLEX_SMART_FLEE_ENTITY) {
        return m_pSubTask;
    }

    const auto trackTask = subTaskType == TASK_COMPLEX_TRACK_ENTITY
        ? static_cast<CTaskComplexTrackEntity*>(m_pSubTask)
        : nullptr;
    const auto distToVehicleSq = trackTask ? trackTask->m_distToTargetSq : 100.f;

    // The vehicle is nearly wrecked: give the ped a new task (See `CreateNextSubTask`)
    if (m_Vehicle->m_fHealth < 250.f && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return CreateNextSubTask(ped);
    }

    switch (m_b31) {
    case 0: { // Approach the vehicle
        if (!ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM) && CGeneral::GetRandomNumberInRange(0, 100) > 60) {
            ped->GetTaskManager().SetTaskSecondary(new CTaskSimpleShakeFist(), TASK_SECONDARY_PARTIAL_ANIM);
        }

        const auto threshold = dword1C * dword1C;
        if (distToVehicleSq < threshold) {
            m_b31 = true;
        }

        ped->Say((eGlobalSpeechContext)0x101, 0, 1.f);
        return m_pSubTask;
    }
    case 1: { // Face the vehicle
        ped->m_fAimingRotation = GetTargetHeading(ped);

        if (subTaskType != TASK_COMPLEX_TRACK_ENTITY) {
            return m_pSubTask;
        }

        if (distToVehicleSq > dword1C * dword1C) {
            m_b31 = false;
            return m_pSubTask;
        }

        if (std::abs(ped->GetHeading() - ped->m_fAimingRotation) < 0.05f) {
            m_b31 = 2;
        }
        return m_pSubTask;
    }
    case 2: { // Hassle the vehicle
        if (subTaskType == TASK_COMPLEX_TRACK_ENTITY && dword1C * dword1C + 0.05f < distToVehicleSq) {
            m_b31 = false;
        }

        if (std::abs(ped->GetHeading() - GetTargetHeading(ped)) >= 0.1f) {
            m_b31 = 1;
        }

        switch (CGeneral::GetRandomNumberInRange(0, 3)) {
        case 0: ped->Say((eGlobalSpeechContext)0xFE, 0, 1.f); break;
        case 1: ped->Say((eGlobalSpeechContext)0xFF, 0, 1.f); break;
        case 2: ped->Say((eGlobalSpeechContext)0x100, 0, 1.f); break;
        default: break;
        }

        if (!m_bRemoveAnim || ped->m_nMoveState > 5) {
            return m_pSubTask;
        }

        if (!ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
            // Pick a random gesture/animation
            const auto rnd = CGeneral::GetRandomNumberInRange<float>(0.f, 200.f);
            CTask* secondaryTask = nullptr;
            if (rnd <= 40.f) {
                // Nothing
            } else if (rnd <= 60.f) {
                m_b31 = false;
            } else if (rnd <= 70.f) {
                if (!ped->IsPlayingHandSignal()) {
                    secondaryTask = new CTaskComplexPlayHandSignalAnim(ANIM_ID_UNDEFINED, 4.f);
                }
            } else if (rnd <= 100.f) {
                secondaryTask = new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, static_cast<AnimationId>(0x12A), 4.f, false);
            } else if (rnd <= 133.f) {
                secondaryTask = new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, static_cast<AnimationId>(0x129), 4.f, false);
            } else if (rnd <= 166.f) {
                secondaryTask = new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, static_cast<AnimationId>(0x128), 4.f, false);
            } else {
                secondaryTask = new CTaskSimpleRunAnim(ANIM_GROUP_GANGS, static_cast<AnimationId>(CGeneral::GetRandomNumberInRange(0, 3) + 0x117), 4.f, false);
            }
            if (secondaryTask) {
                ped->GetTaskManager().SetTaskSecondary(secondaryTask, TASK_SECONDARY_PARTIAL_ANIM);
            }
            return m_pSubTask;
        }

        if (distToVehicleSq < dword1C * dword1C) {
            m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
        }

        // Push the car once the gesture is far enough along
        auto* const anim = [&]() -> CAnimBlendAssociation* {
            for (const auto animId : { 0x128, 0x129, 0x12A }) {
                if (auto* assoc = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), static_cast<AnimationId>(animId))) {
                    return assoc;
                }
            }
            return nullptr;
        }();

        if (!anim) {
            return m_pSubTask;
        }

        const auto threshold = anim->GetAnimId() == static_cast<AnimationId>(0x129) ? 0.7f : 0.5f;
        if (anim->m_CurrentTime <= threshold || anim->m_CurrentTime - anim->m_BlendAmount < threshold) {
            return m_pSubTask;
        }

        // Tipping force, applied along the side the ped is hassling from
        const auto& mat  = m_Vehicle->GetMatrix();
        const auto force = m_Vehicle->m_fMass * 0.02f;

        CVector direction;
        switch (m_nHasslePosId) {
        case 0:
        case 2:
            direction = -mat.GetRight();
            break;
        case 1:
        case 3:
            direction = mat.GetRight();
            break;
        case 4:
            direction = -mat.GetForward();
            break;
        case 5:
            direction = mat.GetForward();
            break;
        default:
            direction = CVector{ 0.f, 0.f, force };
            break;
        }
        m_Vehicle->ApplyTurnForce(CVector{ 0.f, 0.f, force }, direction);

        // The vehicle shouldn't be wrecked by the group - restore its health after the probe
        const auto healthBefore = m_Vehicle->m_fHealth;

        // The vanilla code builds a throwaway `CTaskSimpleFight` here (0x61C470) and uses its
        // (uninitialised) combo state to set the collision model up and to run the hit.
        CTaskSimpleFight fight{ m_Vehicle, 0xB, 20'000 };

        CMatrix pedMat = ped->GetMatrix();
        pedMat.GetPosition() = pedMat.GetPosition() + ped->GetMatrix().GetForward();

        fight.FightSetUpCol(0.5f);

        if (CCollision::ProcessColModels(
                pedMat, col1[0],
                m_Vehicle->GetMatrix(), *m_Vehicle->GetColModel(),
                CWorld::m_aTempColPts, nullptr, nullptr, false
            ) > 0
        ) {
            auto& colPoint = CWorld::m_aTempColPts[0];
            fight.FightHitCar(ped, m_Vehicle, colPoint.m_vecPoint, colPoint.m_vecNormal, colPoint.m_nPieceTypeB, (int8)colPoint.m_nSurfaceTypeB);
        }

        m_Vehicle->m_fHealth = healthBefore;

        m_Vehicle->m_vehicleAudio.AddAudioEvent((eAudioEvents)0x6C, 0.f);
        return m_pSubTask;
    }
    default:
        return m_pSubTask;
    }
}
