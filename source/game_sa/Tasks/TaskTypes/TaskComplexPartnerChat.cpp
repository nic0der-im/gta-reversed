#include "StdInc.h"

#include "TaskComplexPartnerChat.h"

#include "AEPedSpeechAudioEntity.h"
#include "TaskComplexChat.h"
#include "TaskComplexSequence.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleChat.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexPartnerChat::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPartnerChat, 0x8707C4, 14);
    RH_ScopedCategory("Tasks/TaskTypes");
    RH_ScopedInstall(Constructor, 0x684290);
}

// 0x684290
CTaskComplexPartnerChat::CTaskComplexPartnerChat(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, bool conversationEnabled, bool a8, CVector point) :
    CTaskComplexPartner(commandName, partner, leadSpeaker, distanceMultiplier, false, updateDirectionCount, point)
{
    m_taskId = TASK_COMPLEX_PARTNER_CHAT;
    m_pedConversationLoaded = 0;
    m_conversationEnabled = conversationEnabled;
    if (conversationEnabled) {
        m_updateDirectionCount = 4;
    }
    field_75 = a8;
    strcpy_s(m_commandName, commandName);
}

// 0x684320
CTaskComplexPartnerChat::~CTaskComplexPartnerChat() {
    if (m_conversationEnabled && m_pedConversationLoaded) {
        CAEPedSpeechAudioEntity::ReleasePedConversation();
    }
}

CTaskComplexPartnerChat* CTaskComplexPartnerChat::Constructor(const char* commandName, CPed* partner, bool leadSpeaker, float distanceMultiplier, int8 updateDirectionCount, bool conversationEnabled, bool a8, CVector point)
{
    this->CTaskComplexPartnerChat::CTaskComplexPartnerChat(commandName, partner, leadSpeaker, distanceMultiplier, updateDirectionCount, conversationEnabled, a8, point);
    return this;
}

// 0x682C60
bool CTaskComplexPartnerChat::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (!m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }

    if (m_conversationEnabled && m_pedConversationLoaded) {
        CAEPedSpeechAudioEntity::ReleasePedConversation();
        m_pedConversationLoaded = false;
    }

    return true;
}

// 0x684380
CTaskComplexSequence* CTaskComplexPartnerChat::GetPartnerSequence() {
    auto* const seq = new CTaskComplexSequence{};

    seq->AddTask(new CTaskComplexTurnToFaceEntityOrCoord{ m_partner, 0.5f, 0.001f });

    // Animation/duration selector - Derived from the point the task was created with
    const auto animId     = (int32)((float)(m_updateDirectionCount * 16) + m_point.x + m_point.y + m_point.z) & 0x7F;
    const auto animIdNorm = (float)animId * (1.0f / 128.0f); // [0, 1)
    const auto durationA  = (int32)(animIdNorm * 3000.0f + 3500.0f);
    const auto durationB  = (int32)((1.0f - animIdNorm) * 3000.0f + 3500.0f);

    if (!m_leadSpeaker) {
        // The follower talks with the same context as the leader
        // NOTE: Retail doesn't check the found task for null (The partner is expected to be running the counterpart task),
        //       but we do, to avoid a crash in case it isn't.
        if (const auto partnerTask = m_partner->GetIntelligence()->FindTaskByType(TASK_COMPLEX_PARTNER_CHAT)) {
            m_conversationEnabled = static_cast<CTaskComplexPartnerChat*>(partnerTask)->m_conversationEnabled;
        }
    }

    if (!m_conversationEnabled) {
        // No conversation - Only do the "chat" animation and stand still
        if (m_leadSpeaker) {
            seq->AddTask(new CTaskSimpleChat{ (uint32)durationA });
            seq->AddTask(new CTaskSimpleStandStill{ durationB, false, false, 8.0f });
        } else {
            seq->AddTask(new CTaskSimpleStandStill{ durationA, false, false, 8.0f });
            seq->AddTask(new CTaskSimpleChat{ (uint32)durationB });
        }
    } else {
        // The conversation contexts are laid out as pairs (`[leader, follower]`) per turn
        const auto conversationTurn = 3 - m_updateDirectionCount;
        const auto leaderContext    = CAEPedSpeechAudioEntity::s_Conversation[conversationTurn * 2 + 0];
        const auto followerContext  = CAEPedSpeechAudioEntity::s_Conversation[conversationTurn * 2 + 1];

        // The conversation is over - Start a new one next time
        if (conversationTurn == CAEPedSpeechAudioEntity::s_ConversationLength) {
            m_updateDirectionCount = 0;
        }

        if (m_leadSpeaker) {
            seq->AddTask(new CTaskComplexChat{ true,  m_partner, m_updateDirectionCount, leaderContext });
            seq->AddTask(new CTaskComplexChat{ false, m_partner, m_updateDirectionCount, leaderContext });
        } else {
            seq->AddTask(new CTaskComplexChat{ false, m_partner, m_updateDirectionCount, followerContext });
            seq->AddTask(new CTaskComplexChat{ true,  m_partner, m_updateDirectionCount, followerContext });
        }
    }

    return seq;
}
