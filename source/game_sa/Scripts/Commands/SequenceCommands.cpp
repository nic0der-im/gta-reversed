#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

/*!
* Various Sequence commands
*/

namespace {
void SetSequenceToRepeat(int32 index, bool repeat) {
    const auto actualIndex = CTheScripts::GetActualScriptThingIndex(index, SCRIPT_THING_SEQUENCE_TASK);
    if (actualIndex < 0 || actualIndex >= MAX_NUM_SCRIPT_SEQUENCE_TASKS) {
        NOTSA_LOG_DEBUG("COMMAND_SET_SEQUENCE_TO_REPEAT: Index should be in [0, 64], it is {}", actualIndex);
        return;
    }
    CTaskSequences::GetActiveSequence().SetRepeatMode(repeat);
}

/// OPEN_SEQUENCE_TASK(0615)
void OpenSequenceTask(CRunningScript& S, int32& sequence) {
    const auto slot = CTaskSequences::GetAvailableSlot(S.m_UsesMissionCleanup);

    if (slot < 0 || slot >= (int32)CTaskSequences::NUM_SEQUENCES) {
        sequence = -1;
        return;
    }

    CTaskSequences::ms_bIsOpened[slot] = true;
    CTaskSequences::Get(slot)->Flush();
    CTaskSequences::ms_iActiveSequence = slot;

    sequence = CTheScripts::GetNewUniqueScriptThingIndex(slot, SCRIPT_THING_SEQUENCE_TASK);

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.AddEntityToList(sequence, MISSION_CLEANUP_ENTITY_TYPE_TASK_SEQUENCE);
    }
}

/// CLOSE_SEQUENCE_TASK(0616)
void CloseSequenceTask(int32 sequence) {
    const auto index = CTheScripts::GetActualScriptThingIndex(sequence, SCRIPT_THING_SEQUENCE_TASK);
    if (index < 0 || index >= (int32)CTaskSequences::NUM_SEQUENCES) {
        return;
    }

    CTaskSequences::ms_bIsOpened[index] = false;
    CTaskSequences::ms_iActiveSequence  = -1;
}

/// PERFORM_SEQUENCE_TASK(0618)
void PerformSequenceTask(CRunningScript& S, int32 pedHandle, int32 sequence) {
    const auto index = CTheScripts::GetActualScriptThingIndex(sequence, SCRIPT_THING_SEQUENCE_TASK);
    if (index < 0 || index >= (int32)CTaskSequences::NUM_SEQUENCES) {
        return;
    }

    S.GivePedScriptedTask(pedHandle, new CTaskComplexUseSequence(index), COMMAND_PERFORM_SEQUENCE_TASK);
}

/// CLEAR_SEQUENCE_TASK(061B)
void ClearSequenceTask(CRunningScript& S, int32 sequence) {
    const auto index = CTheScripts::GetActualScriptThingIndex(sequence, SCRIPT_THING_SEQUENCE_TASK);
    if (index >= 0 && index < (int32)CTaskSequences::NUM_SEQUENCES) {
        CTaskSequences::ms_bIsOpened[index] = false;
        CTaskSequences::Get(index)->SetCanBeEmptied(true);
        CTheScripts::ScriptSequenceTaskArray[index].m_bUsed = false;
    }

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(sequence, MISSION_CLEANUP_ENTITY_TYPE_TASK_SEQUENCE);
    }
}

/// GET_SEQUENCE_PROGRESS(0646)
int32 GetSequenceProgress(CPed& ped) {
    if (CPedScriptedTaskRecord::GetStatus(&ped, COMMAND_PERFORM_SEQUENCE_TASK) == eScriptedTaskStatus::EVENT_ASSOCIATED) {
        return -1;
    }

    const auto task = ped.GetTaskManager().GetTaskPrimary(TASK_PRIMARY_PRIMARY);
    return task ? static_cast<CTaskComplexUseSequence*>(task)->m_nCurrentTaskIndex : -1;
}
};

void notsa::script::commands::sequence::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Sequence");

    REGISTER_COMMAND_HANDLER(COMMAND_SET_SEQUENCE_TO_REPEAT, SetSequenceToRepeat);
    REGISTER_COMMAND_HANDLER(COMMAND_OPEN_SEQUENCE_TASK, OpenSequenceTask);
    REGISTER_COMMAND_HANDLER(COMMAND_CLOSE_SEQUENCE_TASK, CloseSequenceTask);
    REGISTER_COMMAND_HANDLER(COMMAND_PERFORM_SEQUENCE_TASK, PerformSequenceTask);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_SEQUENCE_TASK, ClearSequenceTask);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_SEQUENCE_PROGRESS, GetSequenceProgress);
}
