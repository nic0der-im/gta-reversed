#include "StdInc.h"

#include "DecisionMakerTypes.h"
#include "PedStats.h"

void CDecisionMakerTypes::InjectHooks() {
    RH_ScopedClass(CDecisionMakerTypes);
    RH_ScopedCategory("DecisionMakers");
    RH_ScopedInstall(AddDecisionMaker, 0x607050);
    RH_ScopedInstall(GetInstance, 0x4684F0);
    RH_ScopedOverloadedInstall(MakeDecision, "ped-ptr", 0x606E70, void(CDecisionMakerTypes::*)(CPed*, eEventType, int32, bool, eTaskType, eTaskType, eTaskType, eTaskType, bool, int16&, int16&));
    RH_ScopedInstall(RemoveDecisionMaker, 0x6043A0);
    RH_ScopedOverloadedInstall(MakeDecision, "pedgroup-ptr", 0x606F80, eTaskType(CDecisionMakerTypes::*)(CPedGroup*, eEventType, int32, bool, eTaskType, eTaskType, eTaskType, eTaskType));
    RH_ScopedInstall(AddEventResponse, 0x6044C0);
    RH_ScopedInstall(FlushDecisionMakerEventResponse, 0x604490);
    RH_ScopedInstall(LoadEventIndices, 0x600840);
}

/*!
 * @addr 0x606060
 * @brief Forwards to `CDecisionMaker::m_aDecisions[decisionIdx].MakeDecision`
 * @note The exe keeps this out of line with `ECX = CDecisionMaker*`
 */
static void MakeDecisionForEvent(CDecisionMaker& dm, int32 decisionIdx, int32 relationship, bool bInVehicle, eTaskType taskToAvoid1, eTaskType taskToAvoid2, eTaskType taskToAvoid3, eTaskType taskToSeek, int16& taskType, int16& facialTaskType) {
    dm.m_aDecisions[decisionIdx].MakeDecision(relationship, bInVehicle, taskToAvoid1, taskToAvoid2, taskToAvoid3, taskToSeek, taskType, facialTaskType);
}

/*!
 * @addr 0x5BB9F0
 * @brief Loads `<eventName> <eventIndex>` pairs from `data/decision/<fileName>`.
 *
 * The event's position among the parsed lines becomes its index within a decision maker,
 * so the file's line order is what defines the mapping (Not the second column).
 * @param indices The event index table to fill (`m_EventIndices`)
 */
static void LoadEventIndicesFromFile(int32* indices, const char* fileName) {
    std::fill_n(indices, +eEventType::EVENT_TOTAL_NUM_EVENTS, 0);

    CFileMgr::SetDir("");
    CFileMgr::SetDir("data\\decision\\");
    const auto file = CFileMgr::OpenFile(fileName, "r");
    CFileMgr::SetDir("");

    // NOTE: The exe doesn't check whether the file could be opened (It would crash on a null `FILE*` too)

    std::array<char, 256> line;
    int32 ordinal{};
    while (CFileMgr::ReadLine(file, line.data(), static_cast<int32>(line.size()))) {
        // Strip the line terminator - CRLF isn't translated to LF outside of Windows' text mode
        auto lineLen = strlen(line.data());
        while (lineLen > 0 && (line[lineLen - 1] == '\r' || line[lineLen - 1] == '\n')) {
            line[--lineLen] = '\0';
        }

        if (line[0] == '\0') {
            continue; // Blank lines don't consume an ordinal
        }

        int32 eventIdx;
        if (sscanf(line.data(), "%*s %d", &eventIdx) != 1) {
            // NOTE: The exe doesn't check the conversion result (It would write to a garbage index)
            continue;
        }

        // NOTE: No bounds check on `eventIdx` (The exe doesn't have one either)
        indices[eventIdx] = ordinal++;
    }

    CFileMgr::CloseFile(file);
}

// 0x607050
int32 CDecisionMakerTypes::AddDecisionMaker(CDecisionMaker* decisionMaker, eDecisionTypes decisionMakerType, bool bDecisionMakerForMission) {
    // Non-mission decision makers take slots [0, 15), while mission ones take [15, 20)
    // (These bounds are hard-coded in the exe as `0xF` and `0x14`)
    constexpr auto MISSION_DM_SLOT_BEGIN = static_cast<int32>(eDecisionMakerType::MISSION5);
    const auto begin = bDecisionMakerForMission ? MISSION_DM_SLOT_BEGIN : 0;
    const auto end   = bDecisionMakerForMission ? static_cast<int32>(eDecisionMakerType::COUNT_TOTAL) : MISSION_DM_SLOT_BEGIN;

    for (auto slot = begin; slot < end; slot++) {
        if (m_IsActive[slot]) {
            continue;
        }

        m_IsActive[slot]               = true;
        m_DecisionMakerSlotTypes[slot] = static_cast<uint8>(decisionMakerType);
        for (auto decisionIdx = 0; decisionIdx < CDecision::MAX_NUM_DECISIONS; decisionIdx++) {
            m_DecisionMakers[slot].m_aDecisions[decisionIdx].From(decisionMaker->m_aDecisions[decisionIdx]);
        }
        ++m_NoOfDecisionMakers;

        return slot;
    }

    return -1; // No free slot
}

// 0x4650F0
CDecisionMakerTypes::CDecisionMakerTypes() {
    // The instance members are initialized by their default member initializers:
    // `m_NoOfDecisionMakers` is zeroed, and every `CDecisionMaker` (The 20 slots and the 5 defaults)
    // is default constructed, which sets every `CDecision` to its default.
    // The exe additionally runs `CDecision::SetDefault` once more over the 5 defaults, which is idempotent.
    // `m_EventIndices` isn't touched by the exe either - It's filled later by `LoadEventIndices`.

    // The exe's ctor also resets these (Non-member) statics - Important for the native port,
    // where they aren't zero-initialized on startup.
    m_DecisionMakerSlotTypes.fill(0);
    ScriptReferenceIndex.fill(1);
    m_IsActive.fill(false);
}

// 0x4684F0
CDecisionMakerTypes* CDecisionMakerTypes::GetInstance() {
    static auto& instance = StaticRef<CDecisionMakerTypes*>(0xC0B030); // Not thread safe (The exe doesn't lock either)
    if (!instance) {
        if (void* const mem = ::operator new(sizeof(CDecisionMakerTypes))) { // The exe explicitly checks for null
            instance = new (mem) CDecisionMakerTypes{};
        }
    }
    return instance;
}

// 0x606E70
void CDecisionMakerTypes::MakeDecision(CPed* ped, eEventType eventType, int32 eventSourceType, bool bIsPedInVehicle, eTaskType taskTypeToAvoid1, eTaskType taskTypeToAvoid2, eTaskType taskTypeToAvoid3, eTaskType taskTypeToSeek, bool bUseInGroupDecisionMaker, int16& taskType, int16& facialTaskType) {
    auto* const intel = ped->GetIntelligence();
    const auto dmType = bUseInGroupDecisionMaker
        ? intel->m_nDecisionMakerTypeInGroup
        : intel->m_nDecisionMakerType;
    const auto eventIdx = m_EventIndices[eventType];

    taskType       = TASK_NONE;
    facialTaskType = -1;

    switch (dmType) {
    case -2: // Player ped (See `CPed::SetPedDefaultDecisionMaker`)
        m_DefaultPlayerPedDecisionMaker.m_aDecisions[eventIdx].MakeDecision(eventSourceType, bIsPedInVehicle, taskTypeToAvoid1, taskTypeToAvoid2, taskTypeToAvoid3, taskTypeToSeek, taskType, facialTaskType);
        break;
    case -1: { // No decision maker assigned - Use one of the defaults
        auto& dm = ped->IsCreatedByMission()
            ? m_DefaultMissionPedDecisionMaker
            : m_DefaultRandomPedDecisionMaker;
        MakeDecisionForEvent(dm, eventIdx, eventSourceType, bIsPedInVehicle, taskTypeToAvoid1, taskTypeToAvoid2, taskTypeToAvoid3, taskTypeToSeek, taskType, facialTaskType);
        break;
    }
    default:
        m_DecisionMakers[dmType].m_aDecisions[eventIdx].MakeDecision(eventSourceType, bIsPedInVehicle, taskTypeToAvoid1, taskTypeToAvoid2, taskTypeToAvoid3, taskTypeToSeek, taskType, facialTaskType);
        break;
    }
}

// 0x6043A0
void CDecisionMakerTypes::RemoveDecisionMaker(eDecisionTypes dm) {
    if (!m_IsActive[dm]) {
        return; // Already removed - Nothing is decremented
    }

    if (m_DecisionMakerSlotTypes[dm] == DEFAULT_DECISION_MAKER) { // Per-ped decision maker - Reset all peds using it
        for (auto i = static_cast<int32>(GetPedPool()->GetSize()) - 1; i >= 0; i--) {
            CPed* const ped = GetPedPool()->GetAt(i);
            if (!ped) {
                continue;
            }
            auto* const intel = ped->GetIntelligence();
            if (intel->m_nDecisionMakerType == static_cast<int32>(dm)) {
                intel->SetPedDecisionMakerType(ped->m_pStats->m_nDefaultDecisionMaker);
            }
        }
    } else { // Per-group decision maker - Reset all groups using it
        for (auto i = 0u; i < std::size(CPedGroups::ms_activeGroups); i++) {
            auto& intel = CPedGroups::ms_groups[i].GetIntelligence();
            if (CPedGroups::ms_activeGroups[i] && intel.GetGroupDecisionMakerType() == static_cast<eDecisionMakerType>(dm)) {
                intel.SetGroupDecisionMakerType(eDecisionMakerType::UNKNOWN);
            }
        }
    }

    m_IsActive[dm]               = false;
    m_DecisionMakerSlotTypes[dm] = DEFAULT_DECISION_MAKER;
    --m_NoOfDecisionMakers;
}

// 0x606F80
eTaskType CDecisionMakerTypes::MakeDecision(CPedGroup* pedGroup, eEventType eventType, int32 eventSourceType, bool bIsPedInVehicle, eTaskType taskId1, eTaskType taskId2, eTaskType taskId3, eTaskType taskId4) {
    const auto eventIdx = m_EventIndices[eventType];

    int16 taskType = TASK_NONE;
    int16 facialTaskType{-1}; // Unused (The exe writes it through the dead `taskId4` argument slot)
    if (pedGroup->GetIntelligence().GetGroupDecisionMakerType() == eDecisionMakerType::UNKNOWN) {
        auto& dm = pedGroup->m_bIsMissionGroup
            ? m_DefaultMissionPedGroupDecisionMaker
            : m_DefaultRandomPedGroupDecisionMaker;
        dm.m_aDecisions[eventIdx].MakeDecision(eventSourceType, bIsPedInVehicle, taskId1, taskId2, taskId3, taskId4, taskType, facialTaskType);
    } else {
        m_DecisionMakers[static_cast<int32>(pedGroup->GetIntelligence().GetGroupDecisionMakerType())].m_aDecisions[eventIdx].MakeDecision(eventSourceType, bIsPedInVehicle, taskId1, taskId2, taskId3, taskId4, taskType, facialTaskType);
    }

    return static_cast<eTaskType>(taskType);
}

// 0x6044C0
void CDecisionMakerTypes::AddEventResponse(int32 decisionMakerIndex, eEventType eventType, eTaskType taskId, float* responseChances, int32* flags) {
    m_DecisionMakers[decisionMakerIndex].m_aDecisions[m_EventIndices[eventType]].Add(taskId, responseChances, flags);
}

// 0x604490
void CDecisionMakerTypes::FlushDecisionMakerEventResponse(int32 decisionMakerIndex, eEventType eventId) {
    m_DecisionMakers[decisionMakerIndex].m_aDecisions[m_EventIndices[eventId]].SetDefault();
}

// 0x600840
void CDecisionMakerTypes::LoadEventIndices() {
    LoadEventIndicesFromFile(m_EventIndices.data(), "PedEvent.txt");
}
