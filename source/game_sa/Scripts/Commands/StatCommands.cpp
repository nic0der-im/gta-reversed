#include <StdInc.h>
#include <Stats.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

using namespace notsa::script;

namespace {
// INCREMENT_INT_STAT (1571)
// INCREMENT_FLOAT_STAT (1572)
template<typename T>
void IncrementStat(eStats stat, T by) {
    CStats::IncrementStat(stat, (float)(by));
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_INCREASE, stat, (float)(by));
}

// DECREMENT_INT_STAT (1573)
// DECREMENT_FLOAT_STAT (1574)
template<typename T>
void DecrementStat(eStats stat, T by) {
    CStats::DecrementStat(stat, (float)(by));
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_DECREASE, stat, (float)(by));
}

// REGISTER_INT_STAT (1575)
// REGISTER_FLOAT_STAT (1576)
template<typename T>
void RegisterStat(eStats stat, T value) {
    CStats::SetNewRecordStat(stat, (float)(value));
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_INCREASE, stat, (float)(value));
}

// SET_INT_STAT (1577)
// SET_FLOAT_STAT (1578)
template<typename T>
void SetStat(eStats stat, T value) {
    CStats::SetStatValue(stat, (float)(value));
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_INCREASE, stat, (float)(value));
}

// GET_INT_STAT (1618)
// GET_FLOAT_STAT (1619)
template<typename T>
T GetStat(eStats stat) {
    return (T)(CStats::GetStatValue(stat));
}

/// SET_DEATHARREST_STATE(0111)
void SetDeathArrestState(CRunningScript& S, int32 state) {
    S.m_IsDeathArrestCheckEnabled = state == 1;
}

/// GET_WHEELIE_STATS(04FC)
MultiRet<uint32, float, uint32, float, uint32, float> GetWheelieStats(CPlayerInfo& player) {
    const MultiRet<uint32, float, uint32, float, uint32, float> stats{
        player.m_nBestCarTwoWheelsTimeMs,
        player.m_fBestCarTwoWheelsDistM,
        player.m_nBestBikeWheelieTimeMs,
        player.m_fBestBikeWheelieDistM,
        player.m_nBestBikeStoppieTimeMs,
        player.m_fBestBikeStoppieDistM
    };

    player.m_nBestCarTwoWheelsTimeMs = 0;
    player.m_fBestCarTwoWheelsDistM  = 0.0f;
    player.m_nBestBikeWheelieTimeMs  = 0;
    player.m_fBestBikeWheelieDistM   = 0.0f;
    player.m_nBestBikeStoppieTimeMs  = 0;
    player.m_fBestBikeStoppieDistM   = 0.0f;

    return stats;
}

/// GET_SCRIPT_TASK_STATUS(062E)
int32 GetScriptTaskStatus(CPed& ped, int32 task) {
    const auto status = CPedScriptedTaskRecord::GetStatus(&ped, task);
    return status == eScriptedTaskStatus::NONE ? 7 : (int32)(status); // The original reports `NONE` as `7`
}

/// STORE_CLOTHES_STATE(0793)
void StoreClothesState() {
    CShopping::StoreClothesState();
}

/// RESTORE_CLOTHES_STATE(0794)
void RestoreClothesState() {
    CShopping::RestoreClothesState();
}

/// SHOW_UPDATE_STATS(08F8)
void ShowUpdateStats(bool show) {
    CStats::bShowUpdateStats = show;
}

/// GET_BEAT_TRACK_STATUS(0953)
int32 GetBeatTrackStatus() {
    return AudioEngine.GetBeatTrackStatus();
}
};

void notsa::script::commands::stat::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Stat");

    REGISTER_COMMAND_HANDLER(COMMAND_INCREMENT_INT_STAT, IncrementStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_INCREMENT_FLOAT_STAT, IncrementStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_DECREMENT_INT_STAT, DecrementStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_DECREMENT_FLOAT_STAT, DecrementStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_INT_STAT, RegisterStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_FLOAT_STAT, RegisterStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_SET_INT_STAT, SetStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_FLOAT_STAT, SetStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_GET_INT_STAT, GetStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_FLOAT_STAT, GetStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_SET_DEATHARREST_STATE, SetDeathArrestState);

    REGISTER_COMMAND_HANDLER(COMMAND_GET_WHEELIE_STATS, GetWheelieStats);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_SCRIPT_TASK_STATUS, GetScriptTaskStatus);

    REGISTER_COMMAND_HANDLER(COMMAND_STORE_CLOTHES_STATE, StoreClothesState);
    REGISTER_COMMAND_HANDLER(COMMAND_RESTORE_CLOTHES_STATE, RestoreClothesState);

    REGISTER_COMMAND_HANDLER(COMMAND_SHOW_UPDATE_STATS, ShowUpdateStats);

    REGISTER_COMMAND_HANDLER(COMMAND_GET_BEAT_TRACK_STATUS, GetBeatTrackStatus);

    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_STAT_CHANGE_AMOUNT);
}
