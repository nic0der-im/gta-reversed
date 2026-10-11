#include "StdInc.h"
#include "ConversationForPed.h"
#include "Conversations.h"

void CConversationForPed::InjectHooks() {
    RH_ScopedClass(CConversationForPed);
    RH_ScopedCategory("Conversations");
    RH_ScopedInstall(Update, 0x43C190, { .Reversed = false }); // not installed: CConversationForPed::InjectHooks() is never called
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
    const auto time = CTimer::GetTimeInMS();
    auto&      nodes = CConversations::m_Nodes;

    if (!m_pPed || !m_Enabled || !IsPlayerInPositionForConversation(false)) {
        m_Status = INACTIVE;
        return;
    }

    // Player answers (Yes/No) the current node
    const auto PlayerAnswers = [&](bool yes) {
        const auto& node = nodes[m_CurrentNode];
        if (!m_SuppressSubtitles) {
            CMessages::ClearSmallMessagesOnly();
            char key[16];
            strcpy_s(key, node.m_Name);
            strcat_s(key, yes ? "Y" : "N"); // 0x8594E0 / 0x8594E4
            CMessages::AddMessageJump(TheText.Get(key), 4000, 3, true);
        }
        const auto speech = yes ? node.m_SpeechY : node.m_SpeechN;
        if (speech > 0) {
            FindPlayerPed()->Say((eGlobalSpeechContext)(uint16)speech);
        }
        if (speech < 0) {
            CConversations::AwkwardSay(-speech, FindPlayerPed());
        }
        m_CurrentNode = yes ? node.m_NodeYes : node.m_NodeNo;
        m_LastChange  = time;
        if (!m_SuppressSubtitles) {
            CMessages::AddMessageQ(TheText.Get(nodes[m_CurrentNode].m_Name), 4000, 1, true);
        }
        m_Status = PLAYER_SPEAKING;
    };

    bool pedSpeaks = time > m_LastTimeWeWereCloseEnough + 10000; // 0x43C1C9
    if (!pedSpeaks && m_Status == PLAYER_SPEAKING) {
        pedSpeaks = time > m_LastChange + 4000; // 0x43C1E0
        if (!pedSpeaks) {
            const auto* pad = CPad::GetPad(0);
            pedSpeaks = pad->ConversationNoJustDown() || pad->ConversationYesJustDown();
        }
    }

    if (pedSpeaks) { // 0x43C4C6
        const auto& node = nodes[m_CurrentNode];
        if (!m_SuppressSubtitles) {
            CMessages::ClearSmallMessagesOnly();
            CMessages::AddMessageJump(TheText.Get(node.m_Name), 4000, 1, true);
        }
        if (node.m_Speech > 0) {
            m_pPed->Say((eGlobalSpeechContext)(uint16)node.m_Speech);
        }
        if (node.m_Speech < 0) {
            CConversations::AwkwardSay(-node.m_Speech, m_pPed);
        }
        m_Status     = PED_SPEAKING;
        m_LastChange = time;
    } else if (time > m_LastChange + 400 && (m_Status == PED_SPEAKING || m_Status == WAITINGFORINPUT || m_Status == INACTIVE)) { // 0x43C218
        const auto* pad  = CPad::GetPad(0);
        const auto& node = nodes[m_CurrentNode];
        if (pad->ConversationNoJustDown() && node.m_NodeNo >= 0) {
            PlayerAnswers(false);
        } else if (pad->ConversationYesJustDown() && node.m_NodeYes >= 0) {
            PlayerAnswers(true);
        }
    }

    m_LastTimeWeWereCloseEnough = time; // 0x43C55D
    if (m_Status == PED_SPEAKING && time > m_LastChange + 4000) {
        m_Status = WAITINGFORINPUT;
    }
}

// 0x43AC40
bool CConversationForPed::IsPlayerInPositionForConversation(bool randomConversation) {
    return plugin::CallMethodAndReturn<bool, 0x43AC40, CConversationForPed*, bool>(this, randomConversation);
}
