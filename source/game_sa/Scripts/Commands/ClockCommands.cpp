#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

#include "CommandParser/Parser.hpp"
#include "UserDisplay.h"
using namespace notsa::script;

/*!
* Various clock commands
*/

namespace {
/// Get hours and minutes
MultiRet<uint8, uint8> GetTimeOfDay() {
    return { CClock::ms_nGameClockHours, CClock::ms_nGameClockMinutes };
}

void SetTimeOfDay(uint8 hours, uint8 minutes) {
    CClock::SetGameClock(hours, minutes, 0);
}

uint16 GetMinutesToTimeOfDay(uint8 hours, uint8 minutes) {
    return CClock::GetGameClockMinutesUntil(hours, minutes);
}

void StoreClock() {
    CClock::StoreClock();
}

void RestoreClock() {
    CClock::RestoreClock();
}

/// CLEAR_ONSCREEN_TIMER(014F)
void ClearOnscreenTimer(CRunningScript& S) {
    CUserDisplay::OnscnTimer.ClearClock(S.GetIndexOfGlobalVariable());
}

/// GET_GAME_TIMER(01BD)
uint32 GetGameTimer() {
    return CTimer::GetTimeInMS();
}

/// DISPLAY_ONSCREEN_TIMER_WITH_STRING(03C3)
void DisplayOnscreenTimerWithString(CRunningScript& S) {
    const auto varId = S.GetIndexOfGlobalVariable();

    S.CollectParameters(1);
    const auto direction = ScriptParams[0].iParam != 0;

    char label[SHORT_STRING_SIZE + 1]{};
    S.ReadTextLabelFromScript(label, SHORT_STRING_SIZE);
    (void)TheText.Get(label); // Original calls this just for the side effects

    CUserDisplay::OnscnTimer.AddClock(varId, label, static_cast<eTimerDirection>(direction));
}

/// SET_TIME_ONE_DAY_FORWARD(088E)
void SetTimeOneDayForward() {
    CClock::OffsetClockByADay(1);
}
};

void notsa::script::commands::clock::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Clock");

    REGISTER_COMMAND_HANDLER(COMMAND_GET_TIME_OF_DAY, GetTimeOfDay);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_TIME_OF_DAY, SetTimeOfDay);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_MINUTES_TO_TIME_OF_DAY, GetMinutesToTimeOfDay);

    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_ONSCREEN_TIMER, ClearOnscreenTimer);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_GAME_TIMER, GetGameTimer);
    REGISTER_COMMAND_HANDLER(COMMAND_DISPLAY_ONSCREEN_TIMER_WITH_STRING, DisplayOnscreenTimerWithString);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_TIME_ONE_DAY_FORWARD, SetTimeOneDayForward);

    REGISTER_COMMAND_HANDLER(COMMAND_STORE_CLOCK, StoreClock);
    REGISTER_COMMAND_HANDLER(COMMAND_RESTORE_CLOCK, RestoreClock);
}
