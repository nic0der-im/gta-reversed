/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Stats.h"
#include "MenuSystem.h"
#include "Hud.h"
#include "TagManager.h"
#include "StuntJumpManager.h"
#include "Text/Text.h"
#include "Audio/AudioEngine.h"
#include "Entity/Vehicle/Automobile.h"
#include "Entity/Vehicle/Bike.h"
#include "Entity/Vehicle/Bmx.h"
#include "Enums/eVehicleClass.h"

// 0x8CDEC8 - Previous value of `STAT_TOTAL_RESPECT` (only referenced from `UpdateRespectStat`)
static inline auto& s_fPreviousRespect = StaticRef<float>(0x8CDEC8);
// 0x8CDEC4 - Last respect value a message was shown for (only referenced from `UpdateRespectStat`)
static inline auto& s_fRespectThreshold = StaticRef<float>(0x8CDEC4);
// 0xB79530 - Last vehicle used to compute the sex appeal (holds a reference)
static inline auto& s_pAppearanceVehicle = StaticRef<CVehicle*>(0xB79530);
// 0xB79534 - Fat/muscle update message state machine (1 = fat, 2 = muscle up, 3 = muscle down)
static inline auto& s_nFatMuscleMessage = StaticRef<uint8>(0xB79534);
// 0xB794CC - Stat whose line `ConstructStatLine` is currently building
static inline auto& s_nStatLineStatId = StaticRef<int16>(0xB794CC);

// 0x719240
// Removes GXT formatting sequences (`~...~`) from `str` in-place
static void StripTildes(GxtChar* str) {
    GxtChar* out = str;
    for (GxtChar* p = str; *p; ) {
        if (*p == '~') {
            for (p++; *p && *p != '~'; p++) {}
            if (*p) {
                p++; // Skip the closing `~`
            }
        } else {
            *out++ = *p++;
        }
    }
    *out = '\0';
}

void CStats::InjectHooks() {
    RH_ScopedClass(CStats);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x55C0C0);
    RH_ScopedOverloadedInstall(GetStatValue, "-OG", 0x558E40, float(*)(eStats));
    RH_ScopedInstall(SetStatValue, 0x55A070);
    RH_ScopedInstall(IsStatFloat, 0x558E30);
    /*RH_ScopedInstall(GetFullFavoriteRadioStationList, 0x558F90); - different return type*/
    RH_ScopedInstall(FindCriminalRatingNumber, 0x559080);
    RH_ScopedInstall(GetPercentageProgress, 0x5591E0);
    RH_ScopedInstall(ConvertToMins, 0x559540);
    RH_ScopedInstall(ConvertToSecs, 0x559560);
    RH_ScopedInstall(SafeToShowThisStat, 0x559590);
    RH_ScopedInstall(CheckForThreshold, 0x5595F0);
    RH_ScopedInstall(IsStatCapped, 0x559630);
    RH_ScopedInstall(LoadActionReactionStats, 0x5599B0);
    RH_ScopedInstall(FindMaxNumberOfGroupMembers, 0x559A50);
    RH_ScopedInstall(ProcessReactionStatsOnDecrement, 0x559730);
    RH_ScopedInstall(DecrementStat, 0x559FA0);
    RH_ScopedInstall(SetNewRecordStat, 0x55C410);
    RH_ScopedInstall(RegisterFastestTime, 0x55A0B0);
    RH_ScopedInstall(RegisterBestPosition, 0x55A160);
    RH_ScopedInstall(ProcessReactionStatsOnIncrement, 0x55B900);
    RH_ScopedInstall(DisplayScriptStatUpdateMessage, 0x55B980);
    RH_ScopedInstall(IncrementStat, 0x55C180);
    RH_ScopedInstall(UpdateStatsWhenSprinting, 0x55C660);
    RH_ScopedInstall(UpdateStatsWhenRunning, 0x55C6F0);
    RH_ScopedInstall(UpdateStatsWhenOnMotorBike, 0x55CD60);
    RH_ScopedInstall(UpdateStatsWhenFighting, 0x55CFA0);
    RH_ScopedInstall(ModifyStat, 0x55D090);

    // unused
    RH_ScopedInstall(GetStatID, 0x558DE0);
    RH_ScopedInstall(GetTimesMissionAttempted, 0x558E70);
    RH_ScopedInstall(RegisterMissionAttempted, 0x558E80);
    RH_ScopedInstall(RegisterMissionPassed, 0x558EA0);

    RH_ScopedInstall(Load, 0x5D3BF0);
    RH_ScopedInstall(Save, 0x5D3B40);
    RH_ScopedInstall(PopulateFavoriteRadioStationList, 0x558EC0);
    RH_ScopedInstall(FindMostFavoriteRadioStation, 0x558FA0);
    RH_ScopedInstall(FindLeastFavoriteRadioStation, 0x559010);
    RH_ScopedInstall(BuildStatLine, 0x559230);
    RH_ScopedInstall(CheckForStatsMessage, 0x559760);
    RH_ScopedInstall(LoadStatUpdateConditions, 0x559860);
    RH_ScopedInstall(GetFatAndMuscleModifier, 0x559AF0);
    RH_ScopedInstall(FindCriminalRatingString, 0x55A210);
    RH_ScopedInstall(ConstructStatLine, 0x55A780);
    RH_ScopedInstall(UpdateRespectStat, 0x55BC50);
    RH_ScopedInstall(UpdateSexAppealStat, 0x55BF20);
    RH_ScopedInstall(UpdateFatAndMuscleStats, 0x55C470);
    RH_ScopedInstall(UpdateStatsWhenCycling, 0x55C780);
    RH_ScopedInstall(UpdateStatsWhenSwimming, 0x55C990);
    RH_ScopedInstall(UpdateStatsWhenDriving, 0x55CAC0);
    RH_ScopedInstall(UpdateStatsWhenFlying, 0x55CC00);
    RH_ScopedInstall(UpdateStatsWhenWeaponHit, 0x55CEB0);
    RH_ScopedInstall(UpdateStatsOnRespawn, 0x55CFC0);
    RH_ScopedInstall(UpdateStatsAddToHealth, 0x55D030);
}

// 0x55C0C0
void CStats::Init() {
    std::ranges::fill(StatTypesFloat, 0.0f);
    std::ranges::fill(StatTypesInt, 0);
    std::ranges::fill(PedsKilledOfThisType, 0);
    std::ranges::fill(TimesMissionAttempted, 0);

    bStatUpdateMessageDisplayed = false;
    CTimer::SetTimeInMS(0);
    std::ranges::fill(LastMissionPassedName, 0);
    m_SprintStaminaCounter = 0;
    m_CycleStaminaCounter = 0;
    m_SwimStaminaCounter = 0;
    m_DrivingCounter = 0;
    m_FlyingCounter = 0;
    m_BoatCounter = 0;
    m_BikeCounter = 0;
    m_FatCounter = 0;
    m_RunningCounter = 0;

    StatTypesFloat[STAT_MAX_HEALTH] = 569.0f;
    CheckForStatsMessage();
    StatTypesFloat[STAT_STAMINA] = 100.0f;
    CheckForStatsMessage();

    PopulateFavoriteRadioStationList();
    LoadActionReactionStats();
    LoadStatUpdateConditions();
}

// 0x558E40
float CStats::GetStatValue(eStats stat) {
    if (!IsStatFloat(stat)) { // int32
        assert(stat >= FIRST_INT_STAT);

        return static_cast<float>(StatTypesInt[stat - FIRST_INT_STAT]);
    }

    return StatTypesFloat[stat];
}

// 0x55A070
void CStats::SetStatValue(eStats stat, float value) {
    if (IsStatFloat(stat)) {
        StatTypesFloat[stat] = value;
    } else { // int32
        assert(stat >= FIRST_INT_STAT);

        StatTypesInt[stat - FIRST_INT_STAT] = static_cast<int32>(value);
    }
    CheckForStatsMessage();
}

// 0x558E30
bool CStats::IsStatFloat(eStats stat) {
    return stat < FIRST_UNUSED_STAT;
}

// 0x558EC0
bool CStats::PopulateFavoriteRadioStationList() {
    const int32* const listenTimes = AudioEngine.GetRadioStationListenTimes();

    bool allZero = true;
    for (auto i = 0; i < (int32)FavoriteRadioStationList.size(); i++) {
        FavoriteRadioStationList[i] = listenTimes[i];

        if (listenTimes[i] != 0) {
            allZero = false;
        }
    }

    return allZero;
}

// 0x558FA0
eRadioID CStats::FindMostFavoriteRadioStation() {
    int32 best    = 0;
    int32 bestIdx = 1; // Also the result when every station has 0 listen time

    for (auto i = 1; i <= 12; i++) {
        if (FavoriteRadioStationList[i] > best) {
            best    = FavoriteRadioStationList[i];
            bestIdx = i;
        }
    }

    return (eRadioID)bestIdx;
}

// 0x559010
int32 CStats::FindLeastFavoriteRadioStation() {
    int32 best    = FavoriteRadioStationList[1];
    int32 bestIdx = 1;

    for (auto i = 1; i <= 12; i++) {
        if (FavoriteRadioStationList[i] < best) {
            best    = FavoriteRadioStationList[i];
            bestIdx = i;
        }
    }

    return bestIdx;
}

// 0x559080
int32 CStats::FindCriminalRatingNumber() {
    CPlayerInfo* playerInfo = FindPlayerPed()->GetPlayerInfoForThisPlayerPed();

    auto value = (int32)(
        GetStatValue(STAT_TOTAL_LEGITIMATE_KILLS)
        - (GetStatValue(STAT_TIMES_BUSTED) - GetStatValue(STAT_NUMBER_OF_HOSPITAL_VISITS)) * 3.0f
        + (GetStatValue(STAT_HIGHEST_FIREFIGHTER_MISSION_LEVEL) + GetStatValue(STAT_HIGHEST_PARAMEDIC_MISSION_LEVEL)) * 10.0f
        + int32((float)playerInfo->m_nMoney / 5000.0f)
        + GetStatValue(STAT_PLANES_HELICOPTERS_DESTROYED) * 30.0f
        + GetStatValue(STAT_TOTAL_FIRES_EXTINGUISHED)
        + GetStatValue(STAT_CRIMINALS_KILLED_ON_VIGILANTE_MISSION)
        + GetStatValue(STAT_PEOPLE_SAVED_IN_AN_AMBULANCE)
    );

    if (CCheat::m_bHasPlayerCheated || GetStatValue(STAT_TIMES_CHEATED) > 0.0f) {
        value -= 10 * (int32)GetStatValue(STAT_TIMES_CHEATED);

        value = std::max(value, -10000);
    } else {
        value = std::max(value, 0);
    }

    float bulletsFired = GetStatValue(STAT_BULLETS_FIRED);

    if (bulletsFired >= 100.0f) {
        value += (int32)(500 * (GetStatValue(STAT_BULLETS_THAT_HIT) / bulletsFired));
    }

    return value + (int32)(10 * GetPercentageProgress());
}

// 0x5591E0
float CStats::GetPercentageProgress() {
    return std::min(StatTypesFloat[STAT_PROGRESS_MADE] / 187.0f * 100.0f, 100.0f);
}

// 0x559230
void CStats::BuildStatLine(char* line, void* pValue1, int32 metrics, void* pValue2, int32 type) {
    if (!line) {
        return;
    }
    const char* const textId = line;

    gString2[0] = '\0';

    if (type == 1) { // Minutes:seconds
        const int32 secs = *(const int32*)pValue2;
        const int32 mins = *(const int32*)pValue1;

        sprintf(gString2, secs < 10 ? "%d:0%d" : "%d:%d", mins, secs);
    } else if (pValue2) {
        switch (metrics) {
        case 0:
            sprintf(gString2, " %d %s %d", *(const int32*)pValue1, GxtCharToUTF8(TheText.Get("FEST_OO")), *(const int32*)pValue2);
            break;
        case 1:
            sprintf(gString2, "%.2f %s %.2f", *(const float*)pValue1, GxtCharToUTF8(TheText.Get("FEST_OO")), *(const float*)pValue2);
            break;
        case 3:
            sprintf(gString2, "$%.2f %s $%.2f", *(const float*)pValue1, GxtCharToUTF8(TheText.Get("FEST_OO")), *(const float*)pValue2);
            break;
        default:
            break;
        }
    } else if (pValue1) {
        switch (metrics) {
        case 0:
            sprintf(gString2, "%d", *(const int32*)pValue1);
            break;
        case 1:
            sprintf(gString2, "%.2f", *(const float*)pValue1);
            break;
        case 2:
            sprintf(gString2, "%0.2f%%", *(const float*)pValue1);
            break;
        case 3:
            sprintf(gString2, "$%.2f", *(const float*)pValue1);
            break;
        case 4:
            sprintf(gString2, "%d|", *(const int32*)pValue1);
            break;
        case 5: {
            const int32 v = *(const int32*)pValue1;
            if (CLocalisation::Metric()) {
                // NOTE: Retail passes a `double` for a `%d` conversion here (prints garbage) - kept as-is
                sprintf(gString2, "%dkgs", (double)((float)v * 0.4536f));
            } else {
                sprintf(gString2, "%dlbs", v);
            }
            break;
        }
        case 6:
            sprintf(gString2, "%.2f %s", *(const float*)pValue1, GxtCharToUTF8(TheText.Get("ST_MILE")));
            break;
        case 7:
            sprintf(gString2, "%.2fm", *(const float*)pValue1);
            break;
        case 8:
            sprintf(gString2, "%.2fft", *(const float*)pValue1 * 3.3333333f);
            break;
        case 9:
            sprintf(gString2, "%d %s", *(const int32*)pValue1, GxtCharToUTF8(TheText.Get("ST_SECS")));
            break;
        default:
            break;
        }
    }

    GxtCharStrcpy(gGxtString, TheText.Get(textId));
    StripTildes(gGxtString);
    AsciiToGxtChar(gString2, gGxtString2);
}

// 0x559540
int32 CStats::ConvertToMins(int32 statValue) {
    if (statValue > 59)
        return (statValue - 60) / 60 + 1;
    return 0;
}

// 0x559560
int32 CStats::ConvertToSecs(int32 statValue) {
    int32 seconds = statValue;
    if (statValue > 59)
        seconds = -60 - 60 * ((statValue - 60) / 60) + statValue;
    if (seconds < 0)
        seconds = -seconds;
    return seconds;
}

// 0x559590
bool CStats::SafeToShowThisStat(eStats stat) {
    if (!CLocalisation::GermanGame()) {
        return true;
    }

    switch (stat) {
    case STAT_RAMPAGES_ATTEMPTED:
    case STAT_RAMPAGES_PASSED:
    case STAT_TOTAL_LEGITIMATE_KILLS:
    case STAT_HIGHEST_CIVILIAN_PEDS_KILLED_ON_RAMPAGE:
    case STAT_HIGHEST_POLICE_PEDS_KILLED_ON_RAMPAGE:
    case STAT_HIGHEST_CIVILIAN_VEHICLES_DESTROYED_ON_RAMPAGE:
    case STAT_HIGHEST_POLICE_VEHICLES_DESTROYED_ON_RAMPAGE:
    case STAT_HIGHEST_NUMBER_OF_TANKS_DESTROYED_ON_RAMPAGE:
        return false;
    default:
        return true;
    }
}

// 0x5595F0
bool CStats::CheckForThreshold(float* pValue, float range) {
    if (*pValue + 40.0f >= range && *pValue - 40.0f <= range) {
        return false;
    }
    *pValue = range;
    return true;
}

// 0x559630
bool CStats::IsStatCapped(eStats stat) {
    switch (stat) {
    case STAT_GIRLFRIEND_RESPECT:
    case STAT_CLOTHES_RESPECT:
    case STAT_FITNESS_RESPECT:
    case STAT_FAT:
    case STAT_STAMINA:
    case STAT_MUSCLE:
    case STAT_MAX_HEALTH:
    case STAT_SEX_APPEAL:
    case STAT_PISTOL_SKILL:
    case STAT_SILENCED_PISTOL_SKILL:
    case STAT_DESERT_EAGLE_SKILL:
    case STAT_SHOTGUN_SKILL:
    case STAT_SAWN_OFF_SHOTGUN_SKILL:
    case STAT_COMBAT_SHOTGUN_SKILL:
    case STAT_MACHINE_PISTOL_SKILL:
    case STAT_SMG_SKILL:
    case STAT_AK_47_SKILL:
    case STAT_M4_SKILL:
    case STAT_RIFLE_SKILL:
    case STAT_APPEARANCE:
    case STAT_ARMOR:
    case STAT_ENERGY:
    case STAT_DRIVING_SKILL:
    case STAT_FLYING_SKILL:
    case STAT_LUNG_CAPACITY:
    case STAT_BIKE_SKILL:
    case STAT_LUCK:
    case STAT_HORSESHOES_COLLECTED:
    case STAT_TOTAL_HORSESHOES:
    case STAT_OYSTERS_COLLECTED:
    case STAT_TOTAL_OYSTERS:
    case STAT_CYCLING_SKILL:
        return true;
    }
    return false;
}

// 0x559760
void CStats::CheckForStatsMessage() {
    if (CPad::GetPad(0)->JustOutOfFrontEnd
        || !bShowUpdateStats
        || TheCamera.m_bWideScreenOn
        || CCutsceneMgr::ms_cutsceneProcessing
        || CHud::HelpMessageDisplayed()
        || CMenuSystem::num_menus_in_use
    ) {
        return;
    }

    for (uint32 i = 0; i < StatMessage.size(); i++) {
        if (TotalNumStatMessages <= i) {
            return;
        }

        auto& msg = StatMessage[i];

        if (msg.displayed) {
            continue;
        }

        const float value = GetStatValue((eStats)msg.stat_num);

        if (msg.condition == STATMESSAGE_LESSTHAN ? value > msg.value : value < msg.value) {
            continue;
        }

        msg.displayed = true;
        CHud::SetHelpMessage(TheText.Get(msg.text_id), false, false, false);
        bStatUpdateMessageDisplayed = true;
    }
}

// 0x559860
void CStats::LoadStatUpdateConditions() {
    CFileMgr::SetDir("");

    FILE* const file = CFileMgr::OpenFile("DATA\\STATDISP.DAT", "rb");

    TotalNumStatMessages = 0;

    uint32 count = 0;
    for (char* line = CFileLoader::LoadLine(file); line; line = CFileLoader::LoadLine(file)) {
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        int32 statId;
        float value;
        char  name[84], condition[12], textId[8];

        sscanf(line, "%d %s %s %f %s", &statId, name, condition, &value, textId);

        auto& msg = StatMessage[count];

        msg.stat_num  = (int16)statId;
        msg.displayed = false;

        // NOTE: Retail leaves `condition` untouched when the token matches neither
        if (!strcmp(condition, "lessthan")) {
            msg.condition = STATMESSAGE_LESSTHAN;
        } else if (!strcmp(condition, "morethan")) {
            msg.condition = STATMESSAGE_MORETHAN;
        }

        msg.value = value;
        strcpy(msg.text_id, textId);

        count++;
    }

    TotalNumStatMessages = count;

    fclose(file);
}

// 0x5599B0
void CStats::LoadActionReactionStats() {
    CFileMgr::SetDir("");

    auto* file = CFileMgr::OpenFile("DATA\\AR_STATS.DAT", "rb");

    for (char* line = CFileLoader::LoadLine(file); line != nullptr; line = CFileLoader::LoadLine(file)) {
        int32 reactId;
        float reactValue;

        if (line[0] != '#' && line[0] != NULL) {
            VERIFY(sscanf_s(line, "%d %*s %f", &reactId, &reactValue) == 2);

            StatReactionValue[reactId] = reactValue;
        }
    }

    CFileMgr::CloseFile(file);
}

// 0x559A50
int32 CStats::FindMaxNumberOfGroupMembers() {
    float respect = StatTypesFloat[STAT_TOTAL_RESPECT];

    if (respect < 10.0f)
        return 0;
    if (respect < 60.0f)
        return 2;
    if (respect < 160.0f)
        return 3;
    if (respect < 330.0f)
        return 4;
    if (respect < 540.0f)
        return 5;
    if (respect < 800.0f)
        return 6;

    return 7;
}

// 0x559AF0
float CStats::GetFatAndMuscleModifier(eStatModAbilities statMod) {
    const auto Fat     = [] { return StatTypesFloat[STAT_FAT]; };
    const auto Muscle  = [] { return StatTypesFloat[STAT_MUSCLE]; };
    const auto Stamina = [] { return StatTypesFloat[STAT_STAMINA]; };
    const auto MaxHP   = [] { return StatTypesFloat[STAT_MAX_HEALTH]; };
    const auto Cycle   = [] { return (float)StatTypesInt[STAT_CYCLING_SKILL - FIRST_INT_STAT]; };
    const auto Bike    = [] { return (float)StatTypesInt[STAT_BIKE_SKILL - FIRST_INT_STAT]; };
    const auto Drive   = [] { return (float)StatTypesInt[STAT_DRIVING_SKILL - FIRST_INT_STAT]; };
    const auto Lung    = [] { return (float)StatTypesInt[STAT_LUNG_CAPACITY - FIRST_INT_STAT]; };

    switch (statMod) {
    case STAT_MOD_0:
        if (Fat() <= 800.0f) {
            return Fat() <= 400.0f ? 0.0f : 1.0f;
        }
        return 2.0f;
    case STAT_MOD_1: {
        const float a = std::max(0.0f, (Fat() - 200.0f) / (1000.0f - 200.0f));
        return std::max((Muscle() - 50.0f) * 0.2f / (1000.0f - 50.0f) + a * (-0.4f) + 1.0f, 0.7f);
    }
    case STAT_MOD_2: {
        const float a = std::max(0.0f, (Fat() - 200.0f) / (1000.0f - 200.0f));
        return std::max((Muscle() - 50.0f) * (-0.1f) / (1000.0f - 50.0f) + a * (-0.2f) + 1.0f, 0.8f);
    }
    case STAT_MOD_3:
        return std::max((Fat() - 200.0f) * (-0.2f) / (1000.0f - 200.0f)
                      + (Muscle() - 50.0f) * (-0.1f) / (1000.0f - 50.0f) + 1.0f, 0.8f);
    case STAT_MOD_4:
        return std::min((Muscle() - 50.0f) * 1.0f / (1000.0f - 50.0f)
                      + (Fat() - 200.0f) * 0.5f / (1000.0f - 200.0f) + 1.0f, 2.0f);
    case STAT_MOD_5:
        return std::clamp(Cycle() * 0.001f * 1.0f
                        + Bike() * 0.001f * 0.5f
                        + (Muscle() - 50.0f) * 0.5f / (1000.0f - 50.0f)
                        + (Fat() - 200.0f) * (-1.0f) / (1000.0f - 200.0f) + 1.0f, 0.25f, 2.0f);
    case STAT_MOD_6:
        return std::clamp(Cycle() * 0.001f * 1.0f
                        + (Fat() - 200.0f) * (-0.5f) / (1000.0f - 200.0f) + 1.0f, 0.5f, 2.0f);
    case STAT_MOD_TIME_CAN_RUN:
        return Stamina() * 0.001f * 3000.0f + 150.0f;
    case STAT_MOD_AIR_IN_LUNG:
        return (Lung() + Stamina()) * 0.0005f * 3000.0f + 1000.0f;
    case STAT_MOD_MAX_HEALTH:
        return MaxHP() * 0.001f * 176.0f;
    case STAT_MOD_10:
        return 176.0f;
    case STAT_MOD_11:
        return std::min(1.0f, Bike() * 0.001f) * 0.5f + 1.0f;
    case STAT_MOD_12:
        return std::min(1.0f, Bike() * 0.001f) * 0.3f + 1.0f;
    case STAT_MOD_13:
        return std::min(1.0f, Bike() * 0.001f) * 0.25f + 1.0f;
    case STAT_MOD_DRIVING_SKILL:
        return std::min(Drive() * 0.001f * 0.5f
                      + (Fat() - 200.0f) * 0.5f / (1000.0f - 200.0f) + 1.0f, 1.0f);
    case STAT_MOD_15:
        return std::max((Muscle() - 50.0f) * (-0.25f) / (1000.0f - 50.0f)
                      + (Fat() - 200.0f) * (-0.5f) / (1000.0f - 200.0f) + 1.0f, 0.5f);
    }
    return 1.0f;
}

// 0x559730
void CStats::ProcessReactionStatsOnDecrement(eStats stat) {
    if (stat == STAT_ENERGY && GetStatValue(STAT_ENERGY) < 0.0f)
        DecrementStat(STAT_FAT, 23.0f);
}

// 0x559FA0
void CStats::DecrementStat(eStats stat, float value) {
    if (value <= 0.0f)
        return;

    float oldValue = GetStatValue(stat);

    SetStatValue(stat, std::max(oldValue - value, 0.0f));

    ProcessReactionStatsOnDecrement(stat);
    CheckForStatsMessage();
}

// 0x55C410
void CStats::SetNewRecordStat(eStats stat, float value) {
    float currentValue = GetStatValue(stat);

    if (currentValue < value)
        SetStatValue(stat, value);

    CheckForStatsMessage();
}

// 0x55A0B0
void CStats::RegisterFastestTime(eStats stat, int32 fastestTime) {
    SetNewRecordStat(stat, (float)fastestTime);
}

// 0x55A160
void CStats::RegisterBestPosition(eStats stat, int32 position) {
    SetNewRecordStat(stat, (float)position);
}

// 0x55A210
GxtChar* CStats::FindCriminalRatingString() {
    const auto Get = [](const char* key) { return const_cast<GxtChar*>(TheText.Get(key)); };

    const int32 rating = FindCriminalRatingNumber();

    if (rating < 0) {
        if (rating > -500)  return Get("RATNG53");
        if (rating > -2000) return Get("RATNG54");
        if (rating > -4000) return Get("RATNG55");
        if (rating > -6000) return Get("RATNG56");
        return Get("RATNG57");
    }

    if (rating < 20)     return Get("RATNG1");
    if (rating < 50)     return Get("RATNG2");
    if (rating < 75)     return Get("RATNG3");
    if (rating < 100)    return Get("RATNG4");
    if (rating < 120)    return Get("RATNG5");
    if (rating < 150)    return Get("RATNG6");
    if (rating < 200)    return Get("RATNG7");
    if (rating < 240)    return Get("RATNG8");
    if (rating < 270)    return Get("RATNG9");
    if (rating < 300)    return Get("RATNG10");
    if (rating < 335)    return Get("RATNG11");
    if (rating < 370)    return Get("RATNG12");
    if (rating < 400)    return Get("RATNG13");
    if (rating < 450)    return Get("RATNG14");
    if (rating < 500)    return Get("RATNG15");
    if (rating < 550)    return Get("RATNG16");
    if (rating < 600)    return Get("RATNG17");
    if (rating < 610)    return Get("RATNG18");
    if (rating < 650)    return Get("RATNG19");
    if (rating < 700)    return Get("RATNG20");
    if (rating < 850)    return Get("RATNG21");
    if (rating < 1000)   return Get("RATNG22");
    if (rating < 1005)   return Get("RATNG23");
    if (rating < 1150)   return Get("RATNG24");

    if (rating < 1300) {
        return Get(GetStatValue<int32>(STAT_TIMES_BUSTED) > 0 ? "RATNG25" : "RATNG24");
    }

    if (rating < 1500)   return Get("RATNG26");
    if (rating < 1700)   return Get("RATNG27");
    if (rating < 2000)   return Get("RATNG28");
    if (rating < 2100)   return Get("RATNG29");
    if (rating < 2300)   return Get("RATNG30");
    if (rating < 2500)   return Get("RATNG31");
    if (rating < 2750)   return Get("RATNG32");
    if (rating < 3000)   return Get("RATNG33");
    if (rating < 3500)   return Get("RATNG34");
    if (rating < 4000)   return Get("RATNG35");
    if (rating < 5000)   return Get("RATNG36");
    if (rating < 7500)   return Get("RATNG37");
    if (rating < 10000)  return Get("RATNG38");
    if (rating < 20000)  return Get("RATNG39");
    if (rating < 30000)  return Get("RATNG40");
    if (rating < 40000)  return Get("RATNG41");
    if (rating < 50000)  return Get("RATNG42");
    if (rating < 65000)  return Get("RATNG43");
    if (rating < 80000)  return Get("RATNG44");
    if (rating < 100000) return Get("RATNG45");
    if (rating < 150000) return Get("RATNG46");
    if (rating < 200000) return Get("RATNG47");
    if (rating < 300000) return Get("RATNG48");
    if (rating < 375000) return Get("RATNG49");

    if (rating < 500000) {
        const int32 flightMinutes = (int32)(GetStatValue<int32>(STAT_FLIGHT_TIME) * (1.0f / 60000.0f) * (1.0f / 60.0f));
        return Get(flightMinutes > 10 ? "RATNG50" : "RATNG49");
    }

    if (rating >= 1000000 && CWorld::Players[CWorld::PlayerInFocus].m_nDisplayMoney > 10000000) {
        return Get("RATNG52");
    }

    return Get("RATNG51");
}

namespace {
// Entry of the stat line tables (retail stride is exactly 6 bytes)
struct tStatLineEntry {
    int16 statId;  // eStats, -99 terminates the group
    uint8 metrics; // Value format selector
    uint8 showIf;  // 0 = only show when the stat value is above 0
    uint8 special; // 1 = stat specific line
    uint8 pad;
};
VALIDATE_SIZE(tStatLineEntry, 6);

constexpr int16 STAT_LINE_END = -99;

// 0x8CD8A0
constexpr tStatLineEntry gStatLinesGroup0[] = {
    {21, 10, 1, 0},  {22, 10, 1, 0},  {64, 10, 1, 0},  {225, 10, 1, 0}, {25, 10, 1, 0},
    {23, 10, 1, 0},  {160, 10, 1, 0}, {223, 10, 1, 0}, {229, 10, 1, 0}, {230, 10, 1, 0},
    {164, 0, 1, 1},  {81, 10, 0, 0},  {330, 0, 1, 1},  {170, 0, 1, 0},  {135, 0, 1, 0},
    {184, 0, 0, 0},  {186, 0, 0, 0},  {188, 0, 0, 0},  {185, 0, 0, 0},  {187, 0, 0, 0},
    {189, 0, 0, 0},  {190, 0, 0, 0},  {200, 0, 1, 0},  {233, 0, 1, 0},  {STAT_LINE_END, 0, 0, 0}
};

// 0x8CD938
constexpr tStatLineEntry gStatLinesGroup1[] = {
    {53, 3, 0, 0},  {13, 3, 1, 0}, {14, 3, 1, 0}, {15, 3, 1, 0}, {16, 3, 1, 0},
    {30, 3, 1, 0},  {31, 3, 1, 0}, {33, 3, 1, 0}, {54, 3, 1, 0}, {55, 3, 1, 0},
    {20, 3, 1, 0},  {62, 3, 1, 0}, {35, 3, 0, 0}, {37, 3, 0, 0}, {334, 3, 0, 1},
    {38, 3, 0, 0},  {39, 3, 0, 0}, {41, 3, 0, 0}, {40, 3, 0, 0}, {STAT_LINE_END, 0, 0, 0}
};

// 0x8CD9B0
constexpr tStatLineEntry gStatLinesGroup2[] = {
    {328, 0, 1, 1}, {329, 0, 1, 1}, {126, 0, 1, 0}, {127, 0, 1, 0}, {128, 0, 1, 0}, {STAT_LINE_END, 0, 0, 0}
};

// 0x8CD9D8
constexpr tStatLineEntry gStatLinesGroup3[] = {
    {191, 0, 0, 0},  {192, 0, 0, 0},  {182, 0, 0, 0},  {183, 0, 0, 0},  {194, 0, 0, 0},
    {131, 0, 1, 0},  {132, 0, 1, 0},  {133, 0, 1, 0},  {155, 0, 0, 0},  {217, 0, 0, 0},
    {218, 0, 0, 0},  {325, 0, 1, 1},  {324, 0, 1, 1},  {120, 0, 1, 0},  {121, 0, 1, 0},
    {122, 0, 1, 0},  {123, 0, 1, 0},  {124, 0, 1, 0},  {125, 3, 1, 0},  {129, 0, 1, 0},
    {130, 0, 1, 0},  {216, 0, 1, 0},  {176, 0, 0, 0},  {177, 0, 1, 0},  {STAT_LINE_END, 0, 0, 0}
};

// 0x8CDA70
constexpr tStatLineEntry gStatLinesGroup4[] = {
    {323, 0, 1, 1},  {234, 0, 0, 0}, {235, 0, 0, 0}, {337, 0, 0, 1}, {236, 0, 0, 0},
    {237, 0, 0, 0},  {238, 0, 0, 0}, {163, 0, 0, 0}, {239, 0, 0, 0}, {240, 0, 0, 0},
    {331, 0, 1, 1},  {332, 0, 1, 1}, {333, 0, 1, 1}, {STAT_LINE_END, 0, 0, 0}
};

// 0x8CDAC8
constexpr tStatLineEntry gStatLinesGroup5[] = {
    {252, 0, 0, 1},  {253, 0, 0, 1},  {254, 0, 0, 1},  {255, 0, 0, 1},  {256, 0, 0, 1},
    {257, 0, 0, 1},  {3, 7, 0, 0},    {26, 7, 0, 0},   {4, 7, 0, 0},    {27, 7, 0, 0},
    {5, 7, 0, 0},    {6, 7, 0, 0},    {7, 7, 0, 0},    {8, 7, 0, 0},    {9, 7, 0, 0},
    {28, 7, 0, 0},   {169, 0, 1, 1},  {173, 0, 1, 1},  {44, 9, 0, 0},   {29, 7, 0, 0},
    {45, 9, 0, 0},   {143, 0, 1, 1},  {144, 0, 1, 1},  {145, 0, 1, 1},  {10, 7, 1, 0},
    {17, 9, 1, 0},   {11, 7, 1, 0},   {18, 9, 1, 0},   {12, 7, 1, 0},   {19, 9, 1, 0},
    {139, 7, 1, 0},  {140, 0, 1, 0},  {141, 4, 1, 0},  {142, 4, 1, 0},  {52, 9, 1, 0},
    {51, 9, 1, 0},   {156, 0, 0, 0},  {2, 7, 0, 0},    {46, 5, 0, 0},   {47, 5, 0, 0},
    {205, 0, 0, 0},  {206, 0, 0, 0},  {207, 0, 0, 0},  {208, 0, 0, 0},  {209, 0, 0, 0},
    {322, 0, 1, 1},  {231, 0, 1, 1},  {243, 0, 1, 1},  {241, 0, 1, 1},  {STAT_LINE_END, 0, 0, 0}
};

// 0x8CDBF8
constexpr tStatLineEntry gStatLinesGroup6[] = {
    {146, 0, 1, 0}, {147, 0, 1, 0}, {157, 0, 0, 0}, {152, 0, 0, 0}, {159, 0, 0, 0}, {153, 0, 0, 0},
    {158, 0, 0, 0}, {151, 0, 0, 0}, {149, 3, 0, 0}, {150, 0, 0, 0}, {161, 0, 0, 0}, {162, 3, 0, 0},
    {210, 0, 0, 0}, {171, 0, 0, 0}, {36, 3, 0, 0},  {213, 0, 0, 1}, {214, 0, 0, 0}, {201, 3, 0, 0},
    {202, 9, 0, 0}, {154, 0, 0, 0}, {166, 0, 0, 0}, {167, 0, 0, 1}, {168, 0, 0, 1}, {246, 9, 0, 0},
    {247, 9, 0, 0}, {248, 9, 0, 0}, {249, 9, 0, 0}, {250, 9, 0, 0}, {172, 0, 0, 0}, {48, 9, 0, 0},
    {212, 9, 0, 0}, {221, 0, 0, 0}, {220, 9, 0, 0}, {219, 9, 0, 0}, {49, 9, 0, 0},  {222, 9, 0, 0},
    {174, 0, 0, 1}, {175, 0, 0, 1}, {179, 9, 0, 0}, {178, 0, 0, 0}, {211, 0, 0, 0}, {215, 0, 0, 0},
    {258, 0, 0, 0}, {259, 9, 0, 0}, {260, 0, 0, 0}, {261, 9, 0, 0}, {262, 0, 0, 0}, {263, 9, 0, 0},
    {264, 0, 0, 0}, {265, 9, 0, 0}, {266, 0, 0, 0}, {267, 9, 0, 0}, {268, 0, 0, 0}, {269, 9, 0, 0},
    {270, 0, 0, 0}, {271, 9, 0, 0}, {272, 0, 0, 0}, {273, 9, 0, 0}, {274, 0, 0, 0}, {275, 9, 0, 0},
    {276, 0, 0, 0}, {277, 9, 0, 0}, {278, 0, 0, 0}, {279, 9, 0, 0}, {280, 0, 0, 0}, {281, 9, 0, 0},
    {282, 0, 0, 0}, {283, 9, 0, 0}, {284, 0, 0, 0}, {285, 9, 0, 0}, {286, 0, 0, 0}, {287, 9, 0, 0},
    {288, 0, 0, 0}, {289, 9, 0, 0}, {290, 0, 0, 0}, {291, 9, 0, 0}, {292, 0, 0, 0}, {293, 9, 0, 0},
    {294, 0, 0, 0}, {295, 9, 0, 0}, {296, 9, 0, 0}, {297, 9, 0, 0}, {298, 9, 0, 0}, {299, 9, 0, 0},
    {300, 9, 0, 0}, {301, 9, 0, 0}, {STAT_LINE_END, 0, 0, 0}
};

// 0x8CDE08 (Also used for any group above 6)
constexpr tStatLineEntry gStatLinesGroup7[] = {
    {0, 0, 1, 1},    {320, 0, 1, 1}, {134, 0, 1, 0}, {136, 0, 1, 0}, {137, 0, 0, 0},
    {138, 0, 1, 0},  {326, 0, 1, 1}, {327, 0, 1, 1}, {195, 0, 0, 0}, {196, 0, 0, 0},
    {197, 0, 0, 0},  {198, 0, 0, 0}, {STAT_LINE_END, 0, 0, 0}
};

constexpr const tStatLineEntry* gStatLinesGroups[] = {
    gStatLinesGroup0, gStatLinesGroup1, gStatLinesGroup2, gStatLinesGroup3,
    gStatLinesGroup4, gStatLinesGroup5, gStatLinesGroup6, gStatLinesGroup7
};

// 0x96AB2C / 0x96AB38 - Written by `CGangWars::UpdateTerritoryUnderControlPercentage`
static inline auto& s_aGangRankValues = StaticRef<std::array<int32, 3>>(0x96AB2C);
static inline auto& s_aGangRankIds    = StaticRef<std::array<int32, 3>>(0x96AB38);
}

// 0x55A780
int32 CStats::ConstructStatLine(int32 line, uint8 group) {
    const tStatLineEntry* const base = gStatLinesGroups[group <= 6 ? group : 7];

    // Emits the label (from `gString`) and the value (from `gString2`)
    const auto EmitLabelAndValue = [] {
        GxtCharStrcpy(gGxtString, TheText.Get(gString));
        StripTildes(gGxtString);
        AsciiToGxtChar(gString2, gGxtString2);
        return 0;
    };

    // Emits both the label and the value as GXT keys (`gString` / `gString2`)
    const auto EmitLabelAndKeyValue = [] {
        GxtCharStrcpy(gGxtString, TheText.Get(gString));
        GxtCharStrcpy(gGxtString2, TheText.Get(gString2));
        return 0;
    };

    // Emits an empty label and `key` as the value
    const auto EmitKeyValue = [](const char* key) {
        gGxtString[0] = 0;
        GxtCharStrcpy(gGxtString2, TheText.Get(key));
        return 0;
    };

    // Formats `" %d %s %d"` (thousands separator between the two values)
    const auto FormatTwoValues = [](int32 first, int32 second) {
        sprintf(gString2, " %d %s %d", first, GxtCharToUTF8(TheText.Get("FEST_OO")), second);
    };

    int32  lineIdx  = 0; // Number of lines emitted (or skipped) so far
    uint32 entryIdx = 0;

    for (;;) {
        const tStatLineEntry& entry = base[entryIdx];

        if (entry.statId == STAT_LINE_END) {
            return lineIdx;
        }

        const auto stat = (eStats)entry.statId;

        if (entry.showIf == 0 && GetStatValue(stat) <= 0.0f) {
            ++entryIdx;
            continue;
        }

        sprintf(gString, stat < 10 ? "STAT00%d" : stat < 100 ? "STAT0%d" : "STAT%d", stat);

        if (CLocalisation::GermanGame()) {
            switch (stat) { // Censored in the German version
            case STAT_RAMPAGES_ATTEMPTED:
            case STAT_RAMPAGES_PASSED:
            case STAT_TOTAL_LEGITIMATE_KILLS:
            case STAT_HIGHEST_CIVILIAN_PEDS_KILLED_ON_RAMPAGE:
            case STAT_HIGHEST_POLICE_PEDS_KILLED_ON_RAMPAGE:
            case STAT_HIGHEST_CIVILIAN_VEHICLES_DESTROYED_ON_RAMPAGE:
            case STAT_HIGHEST_POLICE_VEHICLES_DESTROYED_ON_RAMPAGE:
            case STAT_HIGHEST_NUMBER_OF_TANKS_DESTROYED_ON_RAMPAGE:
                ++entryIdx;
                continue;
            default:
                break;
            }
        }

        s_nStatLineStatId = 0;

        // Metrics based value
        if (entry.special == 0) {
            if (entry.metrics == 9) { // Minutes:seconds
                if (lineIdx != line) {
                    ++lineIdx;
                    ++entryIdx;
                    continue;
                }

                int32 mins = ConvertToMins((int32)GetStatValue(stat));
                int32 secs = ConvertToSecs((int32)GetStatValue(stat));

                BuildStatLine(gString, &mins, 0, &secs, 1);
                return 0;
            }

            if (entry.metrics == 10) { // Sets the stat the menu should draw as a bar chart
                s_nStatLineStatId = stat;
            }

            if (lineIdx != line) {
                ++lineIdx;
                ++entryIdx;
                continue;
            }

            switch (entry.metrics) {
            case 1:
                sprintf(gString2, "%.2f", GetStatValue(stat));
                break;
            case 3:
                sprintf(gString2, "$%.2f", GetStatValue(stat));
                break;
            case 4:
                sprintf(gString2, "%d|", (int32)GetStatValue(stat));
                break;
            case 5: {
                const int32 value = (int32)GetStatValue(stat);
                if (CLocalisation::Metric()) {
                    // NOTE: Retail passes a `double` for the `%d` conversion here - kept as-is
                    sprintf(gString2, "%dkgs", (double)((float)value * 0.4536f));
                } else {
                    sprintf(gString2, "%dlbs", value);
                }
                break;
            }
            case 6:
                sprintf(gString2, "%.2f %s", GetStatValue(stat), GxtCharToUTF8(TheText.Get("ST_MILE")));
                break;
            case 7: {
                const float value = GetStatValue(stat);
                if (CLocalisation::Metric()) {
                    sprintf(gString2, "%.2fm", value);
                } else {
                    sprintf(gString2, "%.2fft", value * 3.3333333f);
                }
                break;
            }
            default: // 0, 2, 8, 10 and anything else
                sprintf(gString2, "%d", (int32)GetStatValue(stat));
                break;
            }

            return EmitLabelAndValue();
        }

        // Stat specific lines
        switch (stat) {
        case STAT_PROGRESS_MADE:
            if (lineIdx != line) {
                break;
            }
            sprintf(gString2, "%0.2f%%", GetPercentageProgress());
            return EmitLabelAndValue();

        case STAT_BEST_INSANE_STUNT_AWARDED: {
            if (lineIdx == line) {
                gString2[0] = '\0';
                return EmitLabelAndValue();
            }
            ++lineIdx;

            static constexpr const char* aStuntAwards[] = {
                "INSTUN", "PRINST", "DBINST", "DBPINS", "TRINST", "PRTRST", "QUINST", "PQUINS"
            };
            const uint32 idx = (uint32)((int32)GetStatValue(stat) - 1);
            const char*  key = idx <= 7 ? aStuntAwards[idx] : "NOSTUC";

            if (lineIdx != line) {
                break;
            }
            return EmitKeyValue(key);
        }

        case STAT_UNIQUE_JUMPS_FOUND:
            if (lineIdx == line) {
                FormatTwoValues((int32)GetStatValue(stat), CStuntJumpManager::m_iNumJumps);
                return EmitLabelAndValue();
            }
            break;

        case STAT_UNIQUE_JUMPS_DONE:
            if (lineIdx == line) {
                FormatTwoValues((int32)GetStatValue(stat), CStuntJumpManager::m_iNumJumps);
                return EmitLabelAndValue();
            }
            break;

        case STAT_ARMOR:
            if (lineIdx == line) {
                sprintf(gString2, "%d", (int32)CWorld::Players[0].m_pPed->m_fArmour);
                return EmitLabelAndValue();
            }
            break;

        case STAT_FLIGHT_TIME: {
            const int32 seconds = (int32)(GetStatValue(STAT_FLIGHT_TIME) * (1.0f / 60000.0f));
            int32       mins    = seconds / 60;
            int32       secs    = seconds % 60;

            if (lineIdx != line) {
                break;
            }
            BuildStatLine(gString, &mins, 0, &secs, 1);
            return 0;
        }

        case STAT_TIME_ON_JETPACK: {
            const int32 seconds = (int32)(GetStatValue(STAT_TIME_ON_JETPACK) * (1.0f / 60000.0f));
            int32       mins    = seconds / 60;
            int32       secs    = seconds % 60;

            if (mins <= 0 && secs <= 0) {
                ++entryIdx;
                continue;
            }
            if (lineIdx != line) {
                break;
            }
            BuildStatLine(gString, &mins, 0, &secs, 1);
            return 0;
        }

        case STAT_SHOOTING_RANGE_LEVELS_PASSED:
            if (lineIdx == line) {
                FormatTwoValues((int32)GetStatValue(stat), 12);
                return EmitLabelAndValue();
            }
            break;

        case STAT_MOST_CARS_PARKED_ON_VALET_PARKING:
            if (lineIdx == line) {
                FormatTwoValues((int32)GetStatValue(stat), 25);
                return EmitLabelAndValue();
            }
            break;

        case STAT_NUMBER_OF_VEHICLES_EXPORTED:
            if (lineIdx == line) {
                FormatTwoValues((int32)GetStatValue(stat), 30);
                return EmitLabelAndValue();
            }
            break;

        case STAT_SNAPSHOTS_TAKEN:
            if (lineIdx == line) {
                FormatTwoValues((int32)GetStatValue(STAT_SNAPSHOTS_TAKEN), (int32)GetStatValue(STAT_TOTAL_SNAPSHOTS));
                return EmitLabelAndValue();
            }
            break;

        case STAT_HORSESHOES_COLLECTED:
            if (lineIdx == line) {
                FormatTwoValues((int32)GetStatValue(STAT_HORSESHOES_COLLECTED), (int32)GetStatValue(STAT_TOTAL_HORSESHOES));
                return EmitLabelAndValue();
            }
            break;

        case STAT_OYSTERS_COLLECTED:
            if (lineIdx == line) {
                FormatTwoValues((int32)GetStatValue(STAT_OYSTERS_COLLECTED), (int32)GetStatValue(STAT_TOTAL_OYSTERS));
                return EmitLabelAndValue();
            }
            break;

        case STAT_PROGRESS_WITH_DENISE:
        case STAT_PROGRESS_WITH_MICHELLE:
        case STAT_PROGRESS_WITH_HELENA:
        case STAT_PROGRESS_WITH_BARBARA:
        case STAT_PROGRESS_WITH_KATIE:
        case STAT_PROGRESS_WITH_MILLIE: {
            const float value = GetStatValue(stat);

            if (value <= 0.0f) { // Doesn't take up a line either
                ++entryIdx;
                continue;
            }
            if (lineIdx != line) {
                break;
            }
            sprintf(gString2, "%0.2f%%", value);
            return EmitLabelAndValue();
        }

        case STAT_PLAYING_TIME: {
            const uint32 totalMinutes = CTimer::GetTimeInMS() / 60000;
            int32        mins         = totalMinutes / 60;
            int32        secs         = totalMinutes % 60;

            if (lineIdx != line) {
                break;
            }
            BuildStatLine(gString, &mins, 0, &secs, 1);
            return 0;
        }

        case STAT_TAGS_SPRAYED:
            if (lineIdx == line) {
                FormatTwoValues(CTagManager::ms_numTagged, CTagManager::ms_numTags);
                return EmitLabelAndValue();
            }
            break;

        case STAT_LEAST_FAVORITE_GANG: {
            int32 gang = 0;
            int32 best = 0;

            for (int32 i = 7; i < 15; i++) {
                if (PedsKilledOfThisType[i] > best) {
                    best = PedsKilledOfThisType[i];
                    gang = i;
                }
            }

            if (gang == 0) {
                ++entryIdx;
                continue;
            }
            if (lineIdx == line) {
                gString2[0] = '\0';
                return EmitLabelAndValue();
            }
            ++lineIdx;

            const uint32 idx = (uint32)(gang - 7);
            if (idx > 7) {
                ++entryIdx;
                continue;
            }
            sprintf(gString2, "ST_GNG%d", idx);

            if (lineIdx != line) {
                break;
            }
            return EmitKeyValue(gString2);
        }

        case STAT_GANG_MEMBERS_WASTED: {
            if (lineIdx == line) {
                int32 total = 0;
                for (int32 i = 7; i <= 15; i++) {
                    total += PedsKilledOfThisType[i];
                }
                sprintf(gString2, "%d", total);
                return EmitLabelAndValue();
            }
            break;
        }

        case STAT_CRIMINALS_WASTED:
            if (lineIdx == line) {
                sprintf(gString2, "%d", PedsKilledOfThisType[20]);
                return EmitLabelAndValue();
            }
            break;

        case STAT_MOST_FAVORITE_RADIO_STATION: {
            if (PopulateFavoriteRadioStationList()) {
                ++entryIdx;
                continue;
            }
            if (lineIdx == line) {
                gString2[0] = '\0';
                return EmitLabelAndValue();
            }
            ++lineIdx;

            char stationKey[32];
            AudioEngine.GetRadioStationNameKey(FindMostFavoriteRadioStation(), stationKey);

            if (lineIdx != line) {
                break;
            }
            return EmitKeyValue(stationKey);
        }

        case STAT_LEAST_FAVORITE_RADIO_STATION: {
            if (PopulateFavoriteRadioStationList()) {
                ++entryIdx;
                continue;
            }
            if (lineIdx == line) {
                gString2[0] = '\0';
                return EmitLabelAndValue();
            }
            ++lineIdx;

            char stationKey[32];
            AudioEngine.GetRadioStationNameKey((eRadioID)FindLeastFavoriteRadioStation(), stationKey);

            if (lineIdx != line) {
                break;
            }
            return EmitKeyValue(stationKey);
        }

        case STAT_CURRENT_WEAPON_SKILL: {
            CPed* const ped = FindPlayerPed();
            eWeaponType weapon = ped->m_aWeapons[(size_t)ped->m_nWeaponSkill].m_Type;

            if (weapon == WEAPON_TEC9) {
                weapon = WEAPON_MICRO_UZI; // Shares the Micro Uzi's skill
            } else if (weapon < WEAPON_PISTOL || weapon > WEAPON_TEC9) {
                ++entryIdx;
                continue;
            }

            const uint8 idx = (uint8)((int32)weapon - (int32)WEAPON_PISTOL + 1);

            if (lineIdx == line) {
                sprintf(gString2, "%d", (int32)GetStatValue((eStats)(STAT_RESPECT + idx)));
                return EmitLabelAndValue();
            }
            ++lineIdx;

            // NOTE: Retail re-reads the weapon here (same result unless it changed mid-frame)
            const uint8 labelIdx = ped->m_aWeapons[(size_t)ped->m_nWeaponSkill].m_Type == WEAPON_TEC9
                ? 11
                : idx;

            sprintf(gString, labelIdx < 10 ? "STWE00%d" : labelIdx < 100 ? "STWE0%d" : "STWE%d", labelIdx);

            if (lineIdx != line) {
                break;
            }
            return EmitKeyValue(gString);
        }

        case STAT_WEAPON_SKILL_LEVELS: {
            if (lineIdx == line) {
                gString2[0] = '\0';
                return EmitLabelAndValue();
            }
            ++lineIdx;

            for (uint8 weapon = 1; weapon <= 10; weapon++) {
                sprintf(gString, weapon < 10 ? "STWE00%d" : "STWE0%d", weapon);

                const auto skill = FindPlayerPed()->GetWeaponSkill((eWeaponType)((int32)weapon + (int32)WEAPON_PISTOL - 1));

                switch (skill) {
                case eWeaponSkill::STD: strcpy(gString2, "WS_STD");  break;
                case eWeaponSkill::PRO: strcpy(gString2, "WS_PRO");  break;
                default:                strcpy(gString2, "WS_POOR"); break;
                }

                if (lineIdx == line) {
                    return EmitLabelAndKeyValue();
                }
                ++lineIdx;
            }

            ++entryIdx;
            continue;
        }

        case STAT_STRONGEST_GANG:
        case STAT_2ND_STRONGEST_GANG:
        case STAT_3RD_STRONGEST_GANG: {
            const auto idx = (uint8)(stat - STAT_STRONGEST_GANG);

            if (s_aGangRankIds[idx] < 0) {
                ++entryIdx;
                continue;
            }

            sprintf(gString,  "ST_LAB%d", idx);
            sprintf(gString2, "ST_GNG%d", s_aGangRankIds[idx]);

            if (lineIdx == line) {
                return EmitLabelAndKeyValue();
            }
            ++lineIdx;

            sprintf(gString, "%d", s_aGangRankValues[idx]);
            AsciiToGxtChar(gString, gGxtString2);

            if (lineIdx == line) {
                gGxtString[0] = 0;
                return 0;
            }
            break;
        }

        case STAT_MONEY_LOST_GAMBLING: {
            const float value = std::max(
                StatTypesFloat[STAT_MONEY_SPENT_GAMBLING] - StatTypesFloat[STAT_MONEY_WON_GAMBLING],
                0.0f
            );

            if (lineIdx != line) {
                break;
            }
            sprintf(gString2, "$%.2f", value);
            return EmitLabelAndValue();
        }

        case STAT_TERRITORY_UNDER_CONTROL:
            if (lineIdx == line) {
                sprintf(gString2, "%0.2f%%", CGangWars::TerritoryUnderControlPercentage * 100.0f);
                return EmitLabelAndValue();
            }
            break;

        default: // Not a stat with a dedicated line
            ++entryIdx;
            continue;
        }

        ++lineIdx;
        ++entryIdx;
    }
}

// 0x55B900
void CStats::ProcessReactionStatsOnIncrement(eStats stat) {
    if (stat != STAT_STAMINA && stat != STAT_ENERGY && stat != STAT_LUNG_CAPACITY)
        return;

    float energy = GetStatValue(STAT_ENERGY);

    if (stat == STAT_STAMINA || stat == STAT_LUNG_CAPACITY) {
        if (energy < 0.0f) {
            StatTypesFloat[STAT_FAT] = std::max(StatTypesFloat[STAT_FAT] - 23.0f, 0.0f);
            CheckForStatsMessage();
        }
        return;
    }

    if (energy > 1000.0f)
        IncrementStat(STAT_FAT, energy - 1000.0f);
}

// 0x55B980
void CStats::DisplayScriptStatUpdateMessage(eStatUpdateState state, eStats stat, float value) {
    if (CPad::GetPad(0)->JustOutOfFrontEnd
        || !bShowUpdateStats
        || TheCamera.m_bWideScreenOn
        || CHud::HelpMessageDisplayed()
        || bStatUpdateMessageDisplayed
        || CMenuSystem::num_menus_in_use)
    {
        bStatUpdateMessageDisplayed = false;

        return;
    }

    if (IsStatCapped(stat) && GetStatValue(stat) >= 1000.0f)
        return;

    switch (stat) {
    case STAT_FAT:
    case STAT_STAMINA:
    case STAT_MUSCLE:
    case STAT_MAX_HEALTH:
    case STAT_SEX_APPEAL:
    case STAT_PISTOL_SKILL:
    case STAT_SILENCED_PISTOL_SKILL:
    case STAT_DESERT_EAGLE_SKILL:
    case STAT_SHOTGUN_SKILL:
    case STAT_SAWN_OFF_SHOTGUN_SKILL:
    case STAT_COMBAT_SHOTGUN_SKILL:
    case STAT_MACHINE_PISTOL_SKILL:
    case STAT_SMG_SKILL:
    case STAT_AK_47_SKILL:
    case STAT_M4_SKILL:
    case STAT_RIFLE_SKILL:
    case STAT_GAMBLING:
    case STAT_DRIVING_SKILL:
    case STAT_ARMOR:
    case STAT_ENERGY:
        if (value > 1.0f)
            CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_TOTAL_RESPECT:
        CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_FLYING_SKILL:
    case STAT_LUNG_CAPACITY:
    case STAT_BIKE_SKILL:
    case STAT_CYCLING_SKILL:
    case STAT_LUCK:
        if (value > 1.0f)
            CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_PROGRESS_WITH_DENISE:
    case STAT_PROGRESS_WITH_MICHELLE:
    case STAT_PROGRESS_WITH_HELENA:
    case STAT_PROGRESS_WITH_BARBARA:
    case STAT_PROGRESS_WITH_KATIE:
    case STAT_PROGRESS_WITH_MILLIE:
        CHud::SetHelpMessageStatUpdate(state, stat, value, 100.0f);
        break;

    case STAT_PIMPING_LEVEL:
        CHud::SetHelpMessageStatUpdate(state, STAT_PIMPING_LEVEL, value, 10.0f);
        break;

    case STAT_GANG_STRENGTH: {
        if (auto player = FindPlayerPed())
        {
            auto maxGroup = std::min<uint8>(FindMaxNumberOfGroupMembers(), player->GetPlayerData()->m_nScriptLimitToGangSize);
            CHud::SetHelpMessageStatUpdate(state, stat, value, maxGroup);
        }
        break;
    }
    default:
        return;
    }
}

// 0x55BC50
void CStats::UpdateRespectStat(uint8 arg0) {
    if (arg0) {
        s_fPreviousRespect  = -99.0f;
        s_fRespectThreshold = -99.0f;
        return;
    }

    float value = StatTypesFloat[STAT_RESPECT] * 0.4f;

    value += ((float)StatTypesInt[STAT_RESPECT_MISSION - FIRST_INT_STAT] * 1000.0f)
           / std::max((float)StatTypesInt[STAT_RESPECT_MISSION_TOTAL - FIRST_INT_STAT], 1.0f) * 0.36f;
    value += StatTypesFloat[STAT_GIRLFRIEND_RESPECT] * 0.03f;
    value += std::max(CGangWars::TerritoryUnderControlPercentage - 0.2f, 0.0f) * 1250.0f * 0.05f;
    value += std::min((float)CWorld::Players[CWorld::PlayerInFocus].m_nMoney * 1e-07f, 1.0f) * 1000.0f * 0.05f;
    value += StatTypesFloat[STAT_MUSCLE] * 0.03f;
    value += StatTypesFloat[STAT_CLOTHES_RESPECT] * 0.03f;
    value += (float)(CTagManager::GetPercentageTagged() * 10) * 0.05f;

    if (value < 0.0f) {
        value = 0.0f;
    }
    if (CCheat::m_aCheatsActive[CHEAT_MAX_RESPECT]) {
        value = 1000.0f;
    }

    if ((int32)s_fPreviousRespect == (int32)value) {
        return;
    }

    StatTypesFloat[STAT_TOTAL_RESPECT] = value;
    CheckForStatsMessage();

    if (value > s_fRespectThreshold || s_fPreviousRespect < 2.0f) {
        // Respect went up
        if (s_fPreviousRespect != -99.0f && s_fRespectThreshold != -99.0f
            && (CheckForThreshold(&s_fRespectThreshold, value) || s_fPreviousRespect < 2.0f)
        ) {
            DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_TOTAL_RESPECT, value);
        }
        if (value < s_fPreviousRespect) {
            s_fRespectThreshold = value;
        }
    } else {
        // Respect went down
        if (s_fPreviousRespect != -99.0f && s_fRespectThreshold != -99.0f
            && (CheckForThreshold(&s_fRespectThreshold, value) || value < 2.0f)
        ) {
            DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_TOTAL_RESPECT, value);
        }
        if (value > s_fPreviousRespect) {
            s_fRespectThreshold = value;
            s_fPreviousRespect  = value;
            return;
        }
    }

    s_fPreviousRespect = value;
}

// 0x55BF20
void CStats::UpdateSexAppealStat() {
    const float appearanceHalf = StatTypesFloat[STAT_APPEARANCE] * 0.5f;
    float       fromVehicle    = 0.0f;

    if (FindPlayerVehicle(-1, false)) {
        s_pAppearanceVehicle = FindPlayerVehicle(-1, false);
        s_pAppearanceVehicle->RegisterReference(reinterpret_cast<CEntity**>(&s_pAppearanceVehicle));
    }

    if (s_pAppearanceVehicle) {
        const CVector& vehPos    = s_pAppearanceVehicle->GetPosition();
        const CVector  playerPos = FindPlayerCoors(-1);
        const float    dx = vehPos.x - playerPos.x;
        const float    dy = vehPos.y - playerPos.y;

        if (sqrtf(dx * dx + dy * dy) < 35.0f) {
            float value = s_pAppearanceVehicle->m_fHealth * 2.0f - 1000.0f;

            if (value >= 1000.0f) {
                value = 1000.0f;
            } else if (value < 0.0f) {
                value = 0.0f;
            }

            switch (CModelInfo::GetModelInfo(s_pAppearanceVehicle->m_nModelIndex)->AsVehicleModelInfoPtr()->m_nVehicleClass) {
            case VEHICLE_CLASS_POORFAMILY:
            case VEHICLE_CLASS_BIG:
            case VEHICLE_CLASS_MOPED:
            case VEHICLE_CLASS_BICYCLE:
                value *= 0.1f;
                break;
            case VEHICLE_CLASS_RICHFAMILY:
            case VEHICLE_CLASS_EXECUTIVE:
            case VEHICLE_CLASS_LEISUREBOAT:
                break;
            case VEHICLE_CLASS_WORKER:
            case VEHICLE_CLASS_TAXI:
            case VEHICLE_CLASS_WORKERBOAT:
                value *= 0.3f;
                break;
            default:
                value *= 0.5f;
                break;
            }

            fromVehicle = value;
        }
    }

    float sexAppeal = fromVehicle * 0.5f + appearanceHalf;

    if (sexAppeal < 0.0f) {
        sexAppeal = 0.0f;
    }
    if (sexAppeal > 1000.0f) {
        sexAppeal = 1000.0f;
    }
    if (CCheat::m_aCheatsActive[CHEAT_MAX_SEX_APPEAL]) {
        sexAppeal = 1000.0f;
    }

    StatTypesFloat[STAT_SEX_APPEAL] = sexAppeal;

    CheckForStatsMessage();
}

// 0x55C180
void CStats::IncrementStat(eStats stat, float value)
{
    if (value <= 0.0f)
        return;

    if (IsStatFloat(stat)) { // float
        StatTypesFloat[stat] += value;

        if (IsStatCapped(stat))
            StatTypesFloat[stat] = std::min(StatTypesFloat[stat], 1000.0f);

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    CPlayerPed* player = FindPlayerPed();
    CPlayerInfo* playerInfo = player->GetPlayerInfoForThisPlayerPed();

    if (stat == STAT_CALORIES) {
        float healthDiff = playerInfo->m_nMaxHealth - player->m_fHealth;

        IncrementStat(STAT_RIOT_MISSION_ACCOMPLISHED, value);

        if (value > healthDiff) {
            float avg = (value - healthDiff) / 2.0f;

            IncrementStat(STAT_FAT, avg);
        }

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    if (stat != STAT_RIOT_MISSION_ACCOMPLISHED) {
        assert(stat >= FIRST_INT_STAT);

        StatTypesInt[stat - FIRST_INT_STAT] += (int32)value;

        if (IsStatCapped(stat))
            StatTypesInt[stat - FIRST_INT_STAT] = std::min(StatTypesInt[stat - FIRST_INT_STAT], 1000);

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    // STAT_RIOT_MISSION_ACCOMPLISHED increment, enum name incorrect?

    float kcals = playerInfo->m_nNumHoursDidntEat - value / 2.0f;
    kcals = std::clamp(kcals, 0.0f, 36.0f);

    float healthDiff = playerInfo->m_nMaxHealth - player->m_fHealth;

    if (value >= healthDiff) {
        playerInfo->m_nNumHoursDidntEat = 0;
    }

    player->m_fHealth += value;
    UpdateStatsAddToHealth((uint32)value);
    ProcessReactionStatsOnIncrement(stat);
    CheckForStatsMessage();
}

// 0x55C470
void CStats::UpdateFatAndMuscleStats(uint32 value) {
    // Fat / muscle
    if ((float)m_FatCounter <= StatReactionValue[STAT_TIMELIMIT_FAT_ADJUST] * 1000.0f) {
        const int32 timeStep = (int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

        m_FatCounter += (uint32)(timeStep * value) / 10u;
    } else {
        m_FatCounter = 0;

        if (StatTypesFloat[STAT_FAT] > 0.0f) {
            if (StatReactionValue[STAT_DEC_FAT] > 0.0f) {
                StatTypesFloat[STAT_FAT] -= StatReactionValue[STAT_DEC_FAT];
                if (StatTypesFloat[STAT_FAT] <= 0.0f) {
                    StatTypesFloat[STAT_FAT] = 0.0f;
                }
                CheckForStatsMessage();
            }

            IncrementStat(STAT_MUSCLE, StatReactionValue[STAT_INC_BODY_MUSCLE]);

            if (s_nFatMuscleMessage == 1) {
                s_nFatMuscleMessage = 2;
                DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MUSCLE, StatReactionValue[STAT_INC_BODY_MUSCLE]);
            } else {
                s_nFatMuscleMessage = 1;
                DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_FAT, StatReactionValue[STAT_DEC_FAT]);
            }
        } else {
            if (StatReactionValue[STAT_DEC_BODY_MUSCLE] > 0.0f) {
                StatTypesFloat[STAT_MUSCLE] -= StatReactionValue[STAT_DEC_BODY_MUSCLE];
                if (StatTypesFloat[STAT_MUSCLE] <= 0.0f) {
                    StatTypesFloat[STAT_MUSCLE] = 0.0f;
                }
                CheckForStatsMessage();
            }

            s_nFatMuscleMessage = 3;
            DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MUSCLE, StatReactionValue[STAT_DEC_BODY_MUSCLE]);
        }
    }

    // Max health
    if ((float)m_MaxHealthCounter > StatReactionValue[STAT_TIMELIMIT_MAX_HEALTH] * 1000.0f) {
        IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        m_MaxHealthCounter = 0;
        return;
    }
    m_MaxHealthCounter += (uint32)(int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);
}

// 0x55C660
void CStats::UpdateStatsWhenSprinting() {
    UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[STAT_EXERCISE_RATE_SPRINT]));
    if (StatReactionValue[STAT_TIMELIMIT_SPRINT_STAMINA] * 1000.0f >= static_cast<float>(m_SprintStaminaCounter)) {
        m_SprintStaminaCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        m_SprintStaminaCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_SPRINT_STAMINA]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_SPRINT_STAMINA]);
    }
}

// 0x55C6F0
void CStats::UpdateStatsWhenRunning() {
    UpdateFatAndMuscleStats((uint32)StatReactionValue[STAT_EXERCISE_RATE_RUN]);
    if (StatReactionValue[STAT_TIMELIMIT_RUNNING] * 1000.0f >= static_cast<float>(m_RunningCounter)) {
        m_RunningCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        m_RunningCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_RUNNING]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_RUNNING]);
    }
}

// 0x55C780
void CStats::UpdateStatsWhenCycling(bool arg0, CBmx* bmx) {
    UpdateFatAndMuscleStats((uint32)((std::fabs(bmx->m_fBrakeCount) > 0.0f || arg0)
        ? StatReactionValue[STAT_EXERCISE_RATE_CYCLE_SPRINT]
        : StatReactionValue[STAT_EXERCISE_RATE_CYCLE]
    ));

    // Stamina
    if ((float)m_CycleStaminaCounter > StatReactionValue[STAT_TIMELIMIT_CYCLE_STAMINA] * 1000.0f) {
        m_CycleStaminaCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_CYCLE_STAMINA]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_CYCLE_STAMINA]);
    } else {
        m_CycleStaminaCounter += (uint32)(int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);
    }

    // Cycling skill
    if ((float)m_CycleSkillCounter > StatReactionValue[STAT_TIMELIMIT_CYCLE_SKILL] * 1000.0f) {
        m_CycleSkillCounter = 0;
        IncrementStat(STAT_CYCLING_SKILL, StatReactionValue[STAT_INC_CYCLE_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_CYCLING_SKILL, StatReactionValue[STAT_INC_CYCLE_SKILL]);
        return;
    }

    const float speedSq = bmx->m_vecMoveSpeed.SquaredMagnitude();

    float factor;
    if (bmx->m_nNoOfContactWheels > 1 || speedSq <= 0.0025f) {
        if (speedSq <= 0.04f) {
            return;
        }
        factor = arg0 ? 1.5f : 1.0f;
    } else {
        factor = 3.0f;
    }

    if (factor <= 0.0f) { // Never true, kept for exactness
        return;
    }

    const int32 timeStep = (int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

    m_CycleSkillCounter += (uint32)(int32)ceil((float)timeStep * factor);
}

// 0x55C990
void CStats::UpdateStatsWhenSwimming(bool arg0, bool arg1) {
    UpdateFatAndMuscleStats((uint32)((arg0 || arg1)
        ? StatReactionValue[STAT_EXERCISE_RATE_SWIM_SPRINT]
        : StatReactionValue[STAT_EXERCISE_RATE_SWIM]
    ));

    // Stamina
    if ((float)m_SwimStaminaCounter > StatReactionValue[STAT_TIMELIMIT_SWIM_STAMINA] * 1000.0f) {
        m_SwimStaminaCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_SWIM_STAMINA]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_SWIM_STAMINA]);
    } else {
        m_SwimStaminaCounter += (uint32)(int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);
    }

    // Breath (only while submerged)
    if (!arg0) {
        return;
    }

    if ((float)m_SwimUnderWaterCounter > StatReactionValue[STAT_TIMELIMIT_BREATH_UNDERWATER] * 1000.0f) {
        m_SwimUnderWaterCounter = 0;
        IncrementStat(STAT_LUNG_CAPACITY, StatReactionValue[STAT_INC_BREATH_UNDERWATER]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_LUNG_CAPACITY, StatReactionValue[STAT_INC_BREATH_UNDERWATER]);
        return;
    }
    m_SwimUnderWaterCounter += (uint32)(int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);
}

// 0x55CAC0
void CStats::UpdateStatsWhenDriving(CVehicle* vehicle) {
    CAutomobile* const car = vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE ? vehicle->AsAutomobile() : nullptr;

    if ((float)m_DrivingCounter > StatReactionValue[STAT_TIMELIMIT_DRIVING_SKILL] * 1000.0f) {
        m_DrivingCounter = 0;
        IncrementStat(STAT_DRIVING_SKILL, StatReactionValue[STAT_INC_DRIVING_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_DRIVING_SKILL, StatReactionValue[STAT_INC_DRIVING_SKILL]);
        return;
    }

    const float speed = vehicle->m_vecMoveSpeed.Magnitude();

    if (speed > 0.8f || (car && car->m_nNumContactWheels != 0)) {
        if (speed > 0.2f) {
            const int32 timeStep = (int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

            m_DrivingCounter = (uint32)((float)timeStep * 0.5f + (float)m_DrivingCounter);
        }
    } else {
        const int32 timeStep = (int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

        m_DrivingCounter = (uint32)((float)timeStep * 1.5f + (float)m_DrivingCounter);
    }
}

// 0x55CC00
void CStats::UpdateStatsWhenFlying(CVehicle* vehicle) {
    if (vehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE) {
        return;
    }

    if ((float)m_FlyingCounter > StatReactionValue[STAT_TIMELIMIT_FLYING_SKILL] * 1000.0f) {
        m_FlyingCounter = 0;
        IncrementStat(STAT_FLYING_SKILL, StatReactionValue[STAT_INC_FLYING_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_FLYING_SKILL, StatReactionValue[STAT_INC_FLYING_SKILL]);
        return;
    }

    if (vehicle->AsAutomobile()->m_nNumContactWheels != 0) {
        return;
    }

    const float speed = vehicle->m_vecMoveSpeed.Magnitude();

    if (speed > 1.3f || vehicle->GetMatrix().GetForward().z < 0.0f) { // Fast or pointing down
        const int32 timeStep = (int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

        m_FlyingCounter = (uint32)((float)timeStep * 1.5f + (float)m_FlyingCounter);
    } else if (speed > 0.5f) {
        const int32 timeStep = (int32)(CTimer::GetTimeStep() * 0.02f * 1000.0f);

        m_FlyingCounter = (uint32)((float)timeStep * 0.5f + (float)m_FlyingCounter);
    }
}

// 0x55CD60
void CStats::UpdateStatsWhenOnMotorBike(CBike* bike) {
    auto bikeCounter = static_cast<float>(m_BikeCounter);
    if (StatReactionValue[STAT_TIMELIMIT_MOTORBIKE_SKILL] * 1000.0f >= bikeCounter) {
        const float bikeMoveSpeed = bike->m_vecMoveSpeed.Magnitude();
        const auto  fTimeStep = CTimer::GetTimeStepInMS();

        if (bikeMoveSpeed > 0.6f || bike->m_nNoOfContactWheels < 3u && bikeMoveSpeed > 0.1f)
            m_BikeCounter = static_cast<uint32>(fTimeStep * 1.5f + bikeCounter);
        else if (bikeMoveSpeed > 0.2f)
            m_BikeCounter = static_cast<uint32>(fTimeStep * 0.5f + bikeCounter);
    } else {
        m_BikeCounter = 0;
        IncrementStat(STAT_BIKE_SKILL, StatReactionValue[STAT_INC_MOTORBIKE_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_BIKE_SKILL, StatReactionValue[STAT_INC_MOTORBIKE_SKILL]);
    }
}

// 0x55CEB0
void CStats::UpdateStatsWhenWeaponHit(eWeaponType weaponType) {
    const eStats stat = CWeaponInfo::GetSkillStatIndex(weaponType);
    const int32  idx  = (int32)stat - STAT_PISTOL_SKILL;

    if (CGameLogic::IsCoopGameGoingOn()) {
        return;
    }

    if (GetStatValue(stat) >= 1000.0f) {
        return;
    }

    IncrementStat(stat, StatReactionValue[STAT_INC_PISTOL_SKILL + idx]);

    if (m_LastWeaponTypeFired != (uint32)idx) {
        m_LastWeaponTypeFired = idx;
        m_WeaponCounter       = 0;
        return;
    }

    const float counter = (float)m_WeaponCounter;

    if (counter > StatReactionValue[STAT_TIMELIMIT_PISTOL_SKILL + idx]) {
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, stat, counter * StatReactionValue[STAT_INC_PISTOL_SKILL + idx]);
        m_WeaponCounter = 0;
        return;
    }

    m_WeaponCounter++;
}

// 0x55CFA0
void CStats::UpdateStatsWhenFighting() {
    UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[STAT_EXERCISE_RATE_FIGHT]));
}

// 0x55CFC0
void CStats::UpdateStatsOnRespawn() {
    // NOTE: No `* 1000.0f` on the limit here (unlike the other counters)
    if ((float)m_DeathCounter > StatReactionValue[STAT_TIMELIMIT_DEATH_HEALTH]) {
        if (StatTypesFloat[STAT_MAX_HEALTH] > 400.0f) {
            IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_DEC_MAX_HEALTH]);
            DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_DEC_MAX_HEALTH]);
        }
        m_DeathCounter = 0;
        return;
    }

    m_DeathCounter++;
}

// 0x55D030
void CStats::UpdateStatsAddToHealth(uint32 addToHealth) {
    m_AddToHealthCounter += addToHealth;

    if ((float)m_AddToHealthCounter > StatReactionValue[STAT_TIMELIMIT_ADD_TO_HEALTH]) {
        IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        m_AddToHealthCounter = 0;
    }
}

// 0x55D090
void CStats::ModifyStat(eStats stat, float value) {
    if (value < 0.0f) {
        CStats::DecrementStat(stat, -value);
    } else {
        CStats::IncrementStat(stat, value);
    }
}

// 0x5D3B40
bool CStats::Save() {
    IncrementStat(STAT_TOTAL_LEGITIMATE_KILLS, GetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT));
    SetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT, 0.0f);

    CGenericGameStorage::SaveDataToWorkBuffer(StatTypesFloat);
    CGenericGameStorage::SaveDataToWorkBuffer(StatTypesInt);
    CGenericGameStorage::SaveDataToWorkBuffer(PedsKilledOfThisType);
    CGenericGameStorage::SaveDataToWorkBuffer(LastMissionPassedName);
    CGenericGameStorage::SaveDataToWorkBuffer(FavoriteRadioStationList);
    CGenericGameStorage::SaveDataToWorkBuffer(TimesMissionAttempted);
    // TODO: NOTSA: CGenericGameStorage::SaveDataToWorkBuffer(StatMessage);
    for (auto& statMessage : StatMessage) {
        CGenericGameStorage::SaveDataToWorkBuffer(statMessage.displayed);
    }
    return true;
}

// 0x5D3BF0
bool CStats::Load() {
    CGenericGameStorage::LoadDataFromWorkBuffer(StatTypesFloat);
    CGenericGameStorage::LoadDataFromWorkBuffer(StatTypesInt);
    CGenericGameStorage::LoadDataFromWorkBuffer(PedsKilledOfThisType);
    CGenericGameStorage::LoadDataFromWorkBuffer(LastMissionPassedName);
    CGenericGameStorage::LoadDataFromWorkBuffer(FavoriteRadioStationList);
    CGenericGameStorage::LoadDataFromWorkBuffer(TimesMissionAttempted);
    // TODO: NOTSA: CGenericGameStorage::LoadDataFromWorkBuffer(StatMessage);
    for (auto& statMessage : StatMessage) {
        CGenericGameStorage::LoadDataFromWorkBuffer(statMessage.displayed);
    }
    return true;
}

// Unused
// 0x558DE0
char* CStats::GetStatID(eStats stat) {
    if (!IsStatFloat(stat)) // int32
        sprintf_s(gString, "stat_i_%d", stat);
    else
        sprintf_s(gString, "stat_f_%d", stat);

    return gString;
}

// Unused
// 0x558E70
int8 CStats::GetTimesMissionAttempted(uint8 missionId) {
    return TimesMissionAttempted[missionId];
}

// Unused
// 0x558E80
void CStats::RegisterMissionAttempted(uint8 missionId) {
    if (TimesMissionAttempted[missionId] != -1) {
        TimesMissionAttempted[missionId]++;
    }
}

// Unused
// 0x558EA0
void CStats::RegisterMissionPassed(uint8 missionId) {
    TimesMissionAttempted[missionId] = -1;
}
