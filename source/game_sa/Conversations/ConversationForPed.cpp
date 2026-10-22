#include "StdInc.h"

#include "ConversationForPed.h"
#include "Conversations.h"

#include "Entity/Ped/Ped.h"
#include "Entity/Ped/PlayerPed.h"
#include "Pad.h"
#include "Messages.h"
#include "Text/Text.h"

void CConversationForPed::InjectHooks() {
    RH_ScopedClass(CConversationForPed);
    RH_ScopedCategory("Conversations");
    RH_ScopedInstall(Update, 0x43C190);
    RH_ScopedInstall(IsPlayerInPositionForConversation, 0x43AC40);
}

void CConversationForPed::Clear(bool dontClearNodes) {
    if (!dontClearNodes) {
        CConversations::m_Nodes[m_FirstNode].ClearRecursively();
    }
    m_FirstNode                 = -1;
    m_CurrentNode               = -1;
    m_pPed                      = nullptr;
    m_LastChange                = 0;
    m_LastTimeWeWereCloseEnough = 0;
}

// 0x43C190
void CConversationForPed::Update() {
    const auto now = CTimer::GetTimeInMS();

    if (!m_pPed || !m_Enabled || !IsPlayerInPositionForConversation(false)) {
        m_Status = INACTIVE;
        return;
    }

    auto& currNode = CConversations::m_Nodes[m_CurrentNode];

    // Display the node's text and let the ped speak it
    const auto ShowCurrentNode = [&] {
        if (!m_SuppressSubtitles) {
            CMessages::ClearSmallMessagesOnly();
            CMessages::AddMessageJump(TheText.Get(currNode.m_Name), 4000, 1, true);
        }
        if (currNode.m_Speech > 0) {
            m_pPed->Say(static_cast<eGlobalSpeechContext>((int16)currNode.m_Speech), 0, 1.0f, false, false, false);
        } else if (currNode.m_Speech < 0) {
            CConversations::AwkwardSay(-currNode.m_Speech, m_pPed);
        }
        m_Status     = PED_SPEAKING;
        m_LastChange = now;
    };

    // The player answered (Yes/No)
    const auto ShowAnswer = [&](char suffix, int32 answerSpeech, int16 nextNode) {
        if (!m_SuppressSubtitles) {
            // The answer's text key is the node's name with `Y`/`N` appended
            char key[sizeof(currNode.m_Name) + 2]{};
            std::strncpy(key, currNode.m_Name, sizeof(currNode.m_Name));
            key[std::strlen(key)] = suffix;

            CMessages::ClearSmallMessagesOnly();
            CMessages::AddMessageJump(TheText.Get(key), 4000, 3, true);
        }
        if (answerSpeech > 0) {
            FindPlayerPed()->Say(static_cast<eGlobalSpeechContext>((int16)answerSpeech), 0, 1.0f, false, false, false);
        } else if (answerSpeech < 0) {
            CConversations::AwkwardSay(-answerSpeech, FindPlayerPed());
        }

        m_CurrentNode = nextNode;
        m_LastChange  = now;

        if (!m_SuppressSubtitles) {
            CMessages::AddMessageQ(TheText.Get(CConversations::m_Nodes[m_CurrentNode].m_Name), 4000, 1, true);
        }
        m_Status = PLAYER_SPEAKING;
    };

    // Only continue the conversation while the player stays around,
    // and while the player doesn't answer (or pause too long) during their own line
    if (now <= m_LastTimeWeWereCloseEnough + 10'000) {
        if (m_Status != PLAYER_SPEAKING) {
            // Ped (or nobody) is speaking, so the player may answer
            if (now > m_LastChange + 400 && (m_Status == PED_SPEAKING || m_Status == WAITINGFORINPUT || m_Status == INACTIVE)) {
                if (CPad::GetPad()->ConversationNoJustDown() && currNode.m_NodeNo >= 0) {
                    ShowAnswer('N', currNode.m_SpeechN, currNode.m_NodeNo);
                } else if (CPad::GetPad()->ConversationYesJustDown() && currNode.m_NodeYes >= 0) {
                    ShowAnswer('Y', currNode.m_SpeechY, currNode.m_NodeYes);
                }
            }
        } else if (now > m_LastChange + 4000 || CPad::GetPad()->ConversationNoJustDown() || CPad::GetPad()->ConversationYesJustDown()) {
            ShowCurrentNode();
        }
    } else {
        ShowCurrentNode();
    }

    m_LastTimeWeWereCloseEnough = now;
    if (m_Status == PED_SPEAKING && now > m_LastChange + 4000) {
        m_Status = WAITINGFORINPUT;
    }
}

// 0x43AC40
bool CConversationForPed::IsPlayerInPositionForConversation(bool randomConversation) {
    if ((FindPlayerCoors() - m_pPed->GetPosition()).Magnitude() > 4.0f) {
        return false;
    }

    // Make sure the ped and the player face each other
    if (DotProduct(FindPlayerPed()->GetForward(), m_pPed->GetForward()) > 0.0f) {
        return false;
    }

    // The player has to be standing still
    if (FindPlayerPed()->m_vecMoveSpeed.Magnitude2D() > 0.01f) {
        return false;
    }

    // The player has recently damaged this ped
    if (m_pPed->m_pLastEntityDamage == FindPlayerPed() && CTimer::GetTimeInMS() < (uint32)m_pPed->field_768 + 6000) {
        return false;
    }

    // The player already has company
    if (randomConversation && FindPlayerPed()->GetPlayerGroup().GetMembership().CountMembersExcludingLeader() >= 1) {
        return false;
    }

    if (!FindPlayerPed()->PedIsReadyForConversation(randomConversation)) {
        return false;
    }

    return m_pPed->PedIsReadyForConversation(randomConversation);
}
