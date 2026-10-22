#include "StdInc.h"

#include "TaskComplexPartner.h"

#include "TaskComplexPartnerChat.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskComplexWanderStandard.h"
#include "TaskSimpleStandStill.h"
#include "AEPedSpeechAudioEntity.h"
#include "Ragdoll/IKChainManager.h"

void CTaskComplexPartner::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartner, 0x870664, 14);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x681E70);
}

// 0x681E70
CTaskComplexPartner::CTaskComplexPartner(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point)
    : CTaskComplex()
{
    m_leadSpeaker              = leadSpeaker;
    m_makePedAlwaysFacePartner = makePedAlwaysFacePartner;
    m_distanceMultiplier       = distanceMultiplier;
    m_updateDirectionCount     = updateDirectionCount;
    m_point                    = point;
    m_partner                  = partner;
    m_partnerState             = PARTNER_STATE_UNK_1;
    m_taskCompleted            = false;
    m_firstToTargetFlag        = -1;
    m_requiredAnimsStreamedIn  = false;
    m_animBlockName[0]         = '\0';
    CEntity::SafeRegisterRef(m_partner);
}

/*!
 * @addr 0x683A40
 */
CTaskComplexPartner::~CTaskComplexPartner() {
    CEntity::SafeCleanUpRef(m_partner);

    if (m_requiredAnimsStreamedIn) {
        if (strcmp(m_animBlockName, "") != 0) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
        }
        m_requiredAnimsStreamedIn = false;
    }
}

CTaskComplexPartner* CTaskComplexPartner::Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, bool makePedAlwaysFacePartner, int8 updateDirectionCount, CVector point) {
#ifdef NOTSA_NATIVE
    // Abstract type - the wrapper is only used by the Windows hook installation.
    UNUSED(commandName, partner, leadSpeaker, distanceMultiplier, makePedAlwaysFacePartner, updateDirectionCount, point);
#else
    this->CTaskComplexPartner::CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, makePedAlwaysFacePartner, updateDirectionCount, point);
#endif
    return this;
}

// 0x6822B0
int8 CTaskComplexPartner::GetPartnerStage() {
    if (const auto partnerTask = m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId))) {
        return static_cast<CTaskComplexPartner*>(partnerTask)->m_partnerState;
    }
    return 0;
}

// 0x681FE0
void CTaskComplexPartner::ComputePoints(CPed* ped, CVector& point, CVector& targetPoint) {
    const auto& pedPos     = ped->GetPosition();
    const auto& partnerPos = m_partner->GetPosition();

    CVector diff = partnerPos - pedPos;
    CVector pos  = pedPos + diff * m_distanceMultiplier;
    diff.Normalise();

    if (GetTaskType() != TASK_COMPLEX_PARTNER_CHAT) {
        bool done = false;
        for (int32 i = 0; !done; i++) {
            if (i > 10) {
                m_taskCompleted = true;
                return;
            }

            const auto distToPed = CVector2D{ pos - pedPos }.Magnitude();
            if (distToPed < 0.7f) {
                pos += diff * (0.75f - distToPed);
            }

            const auto distToPartner = CVector2D{ pos - partnerPos }.Magnitude();
            if (distToPartner >= 0.7f) {
                if (distToPed >= 0.7f) {
                    done = true;
                }
            } else {
                pos -= diff * (0.75f - distToPartner);
            }
        }
    }

    point       = pos - diff * 0.5f;
    targetPoint = pos + diff * 0.5f;
}

// 0x683AD0
CTask* CTaskComplexPartner::CreateNextSubTask(CPed* ped) {
    if (!m_partner) {
        return nullptr;
    }

    const auto partnerTask = m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId));
    if (!partnerTask || static_cast<CTaskComplexPartner*>(partnerTask)->m_partner != ped) {
        return nullptr;
    }

    switch (m_partnerState) {
    case PARTNER_STATE_UNK_6: {
        if (!m_requiredAnimsStreamedIn || !m_updateDirectionCount) {
            return nullptr;
        }
        --m_updateDirectionCount;
        return GetPartnerSequence();
    }
    case PARTNER_STATE_UNK_5: {
        if (!m_makePedAlwaysFacePartner) {
            if (GetPartnerStage() >= 5) {
                const auto wander = ped->GetTaskManager().m_aPrimaryTasks[TASK_PRIMARY_DEFAULT];
                if (   wander
                    && wander->GetTaskType() == TASK_COMPLEX_WANDER
                    && static_cast<CTaskComplexWander*>(wander)->GetWanderType() == WANDER_TYPE_STANDARD
                ) {
                    const auto forward = ped->GetForwardVector();
                    auto* const newWander = new CTaskComplexWanderStandard{
                        PEDMOVE_RUN,
                        static_cast<uint8>(CGeneral::GetNodeHeadingFromVector(-forward.x, -forward.y)),
                        true
                    };
                    newWander->m_nMinNextScanTime = CTimer::GetTimeInMS() + 100'000;
                    ped->GetTaskManager().SetTask(newWander, TASK_PRIMARY_DEFAULT);
                }
                m_partnerState = PARTNER_STATE_UNK_6;
            }
            return new CTaskSimpleStandStill{50, false, false, 8.f};
        }

        if (m_firstToTargetFlag == 1) {
            if (GetPartnerStage() == 6) {
                m_partnerState = PARTNER_STATE_UNK_6;
            }
            return new CTaskSimpleStandStill{50, false, false, 8.f};
        }

        {
            const auto& pedPos     = ped->GetPosition();
            const auto& partnerPos = m_partner->GetPosition();

            CVector2D delta{ pedPos.x - partnerPos.x, pedPos.y - partnerPos.y };
            const auto dist = delta.Magnitude();
            if (dist >= 0.99f && dist <= 1.01f) {
                m_partnerState = PARTNER_STATE_UNK_6;
            } else {
                delta.Normalise();
                CVector2D nudge = delta + CVector2D{ partnerPos.x - pedPos.x, partnerPos.y - pedPos.y };
                if (nudge.Magnitude() >= 0.02f) {
                    nudge = nudge.Normalized() * 0.02f;
                }

                const auto& mat = ped->GetMatrix();
                ped->m_vecAnimMovingShiftLocal.x = mat.GetRight().x * nudge.x + mat.GetRight().y * nudge.y;
                ped->m_vecAnimMovingShiftLocal.y = mat.GetForward().x * nudge.x + mat.GetForward().y * nudge.y;
            }
        }

        return new CTaskSimpleStandStill{50, false, false, 8.f};
    }
    case PARTNER_STATE_UNK_4: {
        if (GetPartnerStage() == 4 || GetPartnerStage() == 5) {
            if (m_firstToTargetFlag == -1) {
                m_firstToTargetFlag = 0;
            }
            if (const auto partnerTask2 = m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId))) {
                auto* const partner = static_cast<CTaskComplexPartner*>(partnerTask2);
                if (partner->m_firstToTargetFlag == -1) {
                    partner->m_firstToTargetFlag = 1;
                }
            }
            m_partnerState = PARTNER_STATE_UNK_5;
            return new CTaskSimpleStandStill{50, false, false, 8.f};
        }

        if (m_firstToTargetFlag == -1) {
            m_firstToTargetFlag = 1;
        }
        if (const auto partnerTask2 = m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId))) {
            auto* const partner = static_cast<CTaskComplexPartner*>(partnerTask2);
            if (partner->m_firstToTargetFlag == -1) {
                partner->m_firstToTargetFlag = 0;
            }
            partner->m_point = m_targetPoint;
        }
        return new CTaskSimpleStandStill{50, false, false, 8.f};
    }
    case PARTNER_STATE_UNK_3: {
        if (m_pSubTask->GetTaskType() == TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) {
            return new CTaskComplexTurnToFaceEntityOrCoord{ m_partner, 0.5f, 0.2f };
        }
        if (m_pSubTask->GetTaskType() != TASK_COMPLEX_TURN_TO_FACE_ENTITY) {
            return nullptr;
        }
        m_partnerState = PARTNER_STATE_UNK_4;
        return new CTaskSimpleStandStill{50, false, false, 8.f};
    }
    case PARTNER_STATE_UNK_2: {
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
            return nullptr;
        }
        if (m_point.x != 0.f || m_point.y != 0.f) {
            m_partnerState = PARTNER_STATE_UNK_3;
            m_goToPointIterations = 0;
            return new CTaskComplexGoToPointAndStandStill{ PEDMOVE_WALK, m_point, 0.1f, 0.0f, false, true };
        }
        return new CTaskSimpleStandStill{50, false, false, 8.f};
    }
    case PARTNER_STATE_UNK_1: {
        if (m_pSubTask->GetTaskType() != TASK_SIMPLE_STAND_STILL) {
            return nullptr;
        }
        if (m_leadSpeaker) {
            ComputePoints(ped, m_point, m_targetPoint);
            if (const auto partnerTask2 = m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId))) {
                static_cast<CTaskComplexPartner*>(partnerTask2)->m_point = m_targetPoint;
            }
        }
        m_partnerState = PARTNER_STATE_UNK_2;
        return new CTaskSimpleStandStill{50, false, false, 8.f};
    }
    default:
        return nullptr;
    }
}

// 0x681F20
CTask* CTaskComplexPartner::CreateFirstSubTask(CPed* ped) {
    if (m_leadSpeaker && m_taskId == TASK_COMPLEX_PARTNER_CHAT) {
        // The conversation fields live in `CTaskComplexPartnerChat` (guarded by the task id above)
        auto* const chat = static_cast<CTaskComplexPartnerChat*>(this);
        if (chat->m_conversationEnabled) {
            if (!CAEPedSpeechAudioEntity::RequestPedConversation(ped, m_partner)) {
                if (!chat->field_75) {
                    return nullptr;
                }
                chat->m_conversationEnabled = false;
            } else {
                chat->m_pedConversationLoaded = true;
            }
        }
    }

    ped->StopPlayingHandSignal();
    return new CTaskSimpleStandStill{50, false, false, 8.f};
}

// 0x6840D0
CTask* CTaskComplexPartner::ControlSubTask(CPed* ped) {
    bool shouldAbort = m_partnerState > PARTNER_STATE_UNK_1 || !m_partner || m_taskCompleted;
    if (shouldAbort) {
        if (   m_partner
            && m_taskCompleted == false
            && [&] {
                const auto partnerTask = m_partner->GetTaskManager().FindActiveTaskByType(static_cast<eTaskType>(m_taskId));
                return partnerTask && static_cast<CTaskComplexPartner*>(partnerTask)->m_partner == ped;
            }()
        ) {
            shouldAbort = false;
        }
    }
    if (shouldAbort && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return nullptr;
    }

    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) {
        ++m_goToPointIterations;
        if (m_goToPointIterations > 150 && m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return nullptr;
        }
    }

    ped->DropEntityThatThisPedIsHolding(true);
    StreamRequiredAnims();

    if (g_ikChainMan.IsLooking(ped)) {
        g_ikChainMan.AbortLookAt(ped, 500);
    }

    return m_pSubTask;
}

// 0x682310
void CTaskComplexPartner::StreamRequiredAnims() {
    if (m_requiredAnimsStreamedIn) {
        return;
    }

    if (m_animBlockName[0] != '\0') {
        const auto blkIdx = CAnimManager::GetAnimationBlockIndex(m_animBlockName);
        if (CAnimManager::GetAnimBlocks()[blkIdx].IsLoaded) {
            CAnimManager::AddAnimBlockRef(blkIdx);
        } else {
            CStreaming::RequestModel(IFPToModelId(blkIdx), STREAMING_KEEP_IN_MEMORY);
        }
    }

    m_requiredAnimsStreamedIn = true;
}

// 0x682370
void CTaskComplexPartner::RemoveStreamedAnims() {
    if (!m_requiredAnimsStreamedIn) {
        return;
    }

    if (m_animBlockName[0] != '\0') {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_animBlockName));
    }

    m_requiredAnimsStreamedIn = false;
}
