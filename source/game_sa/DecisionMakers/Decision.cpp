#include "StdInc.h"

#include "Decision.h"

void CDecision::InjectHooks() {    
    RH_ScopedClass(CDecision);
    RH_ScopedCategory("DecisionMakers");

    RH_ScopedInstall(SetDefault, 0x600530);
    RH_ScopedInstall(Set, 0x600570);
    RH_ScopedInstall(Add, 0x600600);
    RH_ScopedInstall(From, 0x6006B0);
    RH_ScopedInstall(HasResponse, 0x600710);
    RH_ScopedInstall(MakeDecision, 0x6040D0);
}

// 0x6040C0
CDecision::CDecision() {
    SetDefault();
}

// 0x600530
void CDecision::SetDefault() {
    m_Tasks.fill(TASK_INVALID);
    m_Probs.fill({});
    m_Bools.fill({});
}

// 0x6006B0
void CDecision::From(const CDecision& rhs) {
    *this = rhs;
}

// 0x600570
void CDecision::Set(
    const notsa::mdarray<int32, MAX_NUM_CHOICES>&    tasks,
    const notsa::mdarray<float, MAX_NUM_CHOICES, 4>& probs,
    const notsa::mdarray<int32, MAX_NUM_CHOICES, 2>& bools,
    const notsa::mdarray<float, MAX_NUM_CHOICES, 6>& /*facialProbs*/ // Unused by the exe
) {
    for (int32 i = 0; i < MAX_NUM_CHOICES; i++) {
        m_Tasks[i] = static_cast<eTaskType>(tasks[i]);
        for (size_t j = 0; j < m_Probs[i].size(); j++) {
            m_Probs[i][j] = static_cast<uint8>(probs[i][j]); // Truncated (exe: `__ftol2`), not rounded
        }
        m_Bools[i][0] = bools[i][0] != 0;
        m_Bools[i][1] = bools[i][1] != 0;
    }
}

// 0x600600
void CDecision::Add(eTaskType task, const float* responseChances, const int32* flags) {
    for (auto i = 0; i < MAX_NUM_CHOICES; i++) {
        // Only completely unused slots can be filled in
        if (m_Tasks[i] != TASK_INVALID) {
            continue;
        }
        if (rng::any_of(m_Probs[i], [](auto prob) { return prob != 0; })) {
            continue;
        }
        if (rng::any_of(m_Bools[i], [](auto b) { return b; })) {
            continue;
        }

        m_Tasks[i] = task;
        for (size_t j = 0; j < m_Probs[i].size(); j++) {
            m_Probs[i][j] = static_cast<uint8>(responseChances[j]); // Truncated (exe: `__ftol2`), not rounded
        }
        m_Bools[i][0] = flags[0] != 0;
        m_Bools[i][1] = flags[1] != 0;

        return; // The exe silently gives up if there's no free slot
    }
}

// 0x600710
bool CDecision::HasResponse() const {
    // NOTE: The sentinel here is `TASK_NONE` (Unlike `Add`/`MakeDecision` which treat `TASK_INVALID` as empty)
    return rng::any_of(m_Tasks, [](auto task) { return task != TASK_NONE; });
}

/*!
 * @addr 0x6007A0
 * @brief Picks one of the choices: The sought task if it's among them, otherwise a weighted random one.
 * @note This is `CDecisionSimple::MakeDecision` in the SDK (The exe keeps it out of line)
 * @param chosenIdx Index of the picked choice (The game doesn't use it)
 */
static void ChooseResponse(const CDecisionSimple& simple, eTaskType soughtTask, int16& taskType, int32& chosenIdx) {
    taskType  = TASK_NONE;
    chosenIdx = -1;

    if (soughtTask != TASK_INVALID) {
        for (uint32 i = 0; i < simple.m_nCount; i++) {
            if (simple.m_anTasks[i] == soughtTask) {
                taskType  = static_cast<int16>(simple.m_anTasks[i]);
                chosenIdx = static_cast<int32>(i);
                break;
            }
        }
    }

    if (taskType == TASK_NONE) {
        const auto r = static_cast<float>(CGeneral::GetRandomNumber()) * RAND_MAX_FLOAT_RECIPROCAL;
        for (uint32 i = 0; i < simple.m_nCount; i++) {
            if (r <= simple.m_afChances[i]) {
                taskType  = static_cast<int16>(simple.m_anTasks[i]);
                chosenIdx = static_cast<int32>(i);
                break;
            }
        }
    }
}

// 0x6040D0
void CDecision::MakeDecision(int32 relationship, bool bInVehicle, eTaskType taskToAvoid1, eTaskType taskToAvoid2, eTaskType taskToAvoid3, eTaskType taskToSeek, int16& taskType, int16& facialTaskType) {
    // Gather the choices that may be picked for this relationship/vehicle state
    std::array<eTaskType, MAX_NUM_CHOICES> candidateTasks;
    std::array<uint8, MAX_NUM_CHOICES>     candidateProbs{};
    int32                                  numCandidates{};
    candidateTasks.fill(TASK_NONE);
    for (auto i = 0; i < MAX_NUM_CHOICES; i++) {
        if (m_Probs[i][relationship] == 0) {
            continue;
        }
        if (m_Tasks[i] == TASK_INVALID) {
            continue;
        }
        if (m_Tasks[i] == taskToAvoid1 || m_Tasks[i] == taskToAvoid2 || m_Tasks[i] == taskToAvoid3) {
            continue;
        }
        if (!m_Bools[i][bInVehicle]) {
            continue;
        }

        candidateTasks[numCandidates] = m_Tasks[i];
        candidateProbs[numCandidates] = m_Probs[i][relationship];
        numCandidates++;
    }

    taskType = TASK_NONE;
    if (numCandidates == 0) {
        facialTaskType = -1;
        return;
    }

    // Convert the chances to a cumulative distribution over all `MAX_NUM_CHOICES` slots
    // (The unused ones keep `TASK_NONE` and a zero chance, as in the exe)
    CDecisionSimple simple{
        .m_anTasks   = candidateTasks,
        .m_afChances = {},
        .m_nCount    = MAX_NUM_CHOICES,
    };
    float total{};
    for (auto i = 0u; i < simple.m_nCount; i++) {
        total += static_cast<float>(candidateProbs[i]);
        simple.m_afChances[i] = total;
    }
    const auto invTotal = 1.0f / total;
    for (auto& chance : simple.m_afChances) {
        chance *= invTotal;
    }

    int32 chosenIdx{-1};
    ChooseResponse(simple, taskToSeek, taskType, chosenIdx);

    facialTaskType = -1;
}
