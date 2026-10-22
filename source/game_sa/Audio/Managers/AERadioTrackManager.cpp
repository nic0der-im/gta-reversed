#include "StdInc.h"

#include "AERadioTrackManager.h"
#include "RadioStreamsPC.h"

#include "AEAudioHardware.h"
#include "AEUserRadioTrackManager.h"
#include "AEAudioUtility.h"
#include "AEAudioEnvironment.h"

auto& AERadioTrackManager = StaticRef<CAERadioTrackManager>(0x8CB6F8);

void CAERadioTrackManager::InjectHooks() {
    RH_ScopedClass(CAERadioTrackManager);
    RH_ScopedCategory("Audio/Managers");

    RH_ScopedInstall(Load, 0x5D40E0);
    RH_ScopedInstall(Save, 0x5D3EE0);
    RH_ScopedInstall(Initialise, 0x5B9390);
    RH_ScopedInstall(Service, 0x4EB9A0, { .Reversed = false });
    RH_ScopedInstall(DisplayRadioStationName, 0x4E9E50);
    RH_ScopedInstall(CheckForStationRetune, 0x4EB660);
    RH_ScopedInstall(CheckForPause, 0x4EA590);
    RH_ScopedInstall(IsVehicleRadioActive, 0x4E9800, { .Reversed = false });
    RH_ScopedInstall(AddDJBanterIndexToHistory, 0x4E97B0);
    RH_ScopedInstall(AddAdvertIndexToHistory, 0x4E9760);
    RH_ScopedInstall(AddIdentIndexToHistory, 0x4E9720);
    RH_ScopedInstall(AddMusicTrackIndexToHistory, 0x4E96C0);
    RH_ScopedOverloadedInstall(StartRadio, "manual", 0x4EB3C0, void (CAERadioTrackManager::*)(eRadioID, eBassSetting, float, bool));
    RH_ScopedOverloadedInstall(StartRadio, "with-settings", 0x4EB550, void (CAERadioTrackManager::*)(const tVehicleAudioSettings&));
    RH_ScopedInstall(CheckForStationRetuneDuringPause, 0x4EB890);
    RH_ScopedInstall(TrackRadioStation, 0x4EAC30);
    RH_ScopedInstall(ChooseTracksForStation, 0x4EB180);
    RH_ScopedInstall(CheckForTrackConcatenation, 0x4EA930);
    RH_ScopedInstall(QueueUpTracksForStation, 0x4EA670);
    RH_ScopedInstall(ChooseDJBanterIndex, 0x4EA2D0);
    RH_ScopedInstall(ChooseDJBanterIndexFromList, 0x4E95E0);
    RH_ScopedInstall(ChooseAdvertIndex, 0x4E9570);
    RH_ScopedInstall(ChooseIdentIndex, 0x4E94C0);
    RH_ScopedInstall(ChooseMusicTrackIndex, 0x4EA270);
    RH_ScopedInstall(ChooseTalkRadioShow, 0x4E8E40);
    RH_ScopedInstall(CheckForMissionStatsChanges, 0x4E8410);
    RH_ScopedInstall(StartTrackPlayback, 0x4EA640);
    RH_ScopedInstall(UpdateRadioVolumes, 0x4EA010);
    RH_ScopedInstall(PlayRadioAnnouncement, 0x4E8400);
    RH_ScopedInstall(GetCurrentRadioStationID, 0x4E83F0);
    RH_ScopedInstall(GetRadioStationListenTimes, 0x4E83E0);
    RH_ScopedInstall(GetRadioStationName, 0x4E9E10);
    RH_ScopedInstall(GetRadioStationNameKey, 0x4E8380);
    RH_ScopedInstall(HasRadioRetuneJustStarted, 0x4E8370);
    RH_ScopedInstall(StopRadio, 0x4E9820, { .Reversed = false });
    RH_ScopedInstall(IsRadioOn, 0x4E8350, { .Reversed = true });
    RH_ScopedInstall(InitialiseRadioStationID, 0x4E8330);
    RH_ScopedInstall(SetBassEnhanceOnOff, 0x4E9DB0);
    RH_ScopedInstall(SetBassSetting, 0x4E82F0);
    RH_ScopedInstall(SetRadioAutoRetuneOnOff, 0x4E82E0);
    RH_ScopedInstall(RetuneRadio, 0x4E8290, { .Reversed = true });
    RH_ScopedInstall(ResetStatistics, 0x4E8200);
    RH_ScopedInstall(Reset, 0x4E7F80, { .Reversed = true });
}

// Code from 0x5B9390
CAERadioTrackManager::CAERadioTrackManager(int32 hwClientHandle) :
    m_HwClientHandle{ hwClientHandle },
    m_nUserTrackPlayMode{ AEUserRadioTrackManager.GetUserTrackPlayMode() }
{
    // All constant value inits are done using member init lists

    rng::copy(CStats::GetFullFavoriteRadioStationList(), m_aListenTimes.begin());

    for (auto i = 0u; i < RADIO_COUNT; i++) {
        m_nMusicTrackIndexHistory[i].Reset();
        m_nDJBanterIndexHistory[i].Reset();
        m_nAdvertIndexHistory[i].Reset();
        m_nIdentIndexHistory[i].Reset();
    }

    // [1st radio, off]
    m_RequestedSettings = m_ActiveSettings = tRadioSettings{CAEAudioUtility::GetRandomRadioStation()};
}

// 0x5B9390
bool CAERadioTrackManager::Initialise(int32 channelId) {
    *this = CAERadioTrackManager{};
    return true;
}

// 0x4E8330
void CAERadioTrackManager::InitialiseRadioStationID(eRadioID id) {
    m_RequestedSettings.StationID = m_ActiveSettings.StationID = id;
}

// 0x4E7F80
void CAERadioTrackManager::Reset() {
    m_bInitialised = false;
    m_bDisplayStationName = false;
    rng::copy(CStats::GetFullFavoriteRadioStationList(), m_aListenTimes.begin());

    rng::for_each(m_nDJBanterIndexHistory, &DJBanterIndexHistory::Reset);
    rng::for_each(m_nAdvertIndexHistory, &AdvertIndexHistory::Reset);
    rng::for_each(m_nIdentIndexHistory, &IdentIndexHistory::Reset);
    rng::for_each(m_nMusicTrackIndexHistory, &MusicTrackHistory::Reset);
    rng::for_each(m_aRadioState, [](auto& s) { s.Reset(); });

    m_RequestedSettings = m_ActiveSettings = tRadioSettings{CAEAudioUtility::GetRandomRadioStation()};
    m_nStationsListed = m_nStationsListDown = 0;
    m_nTimeRadioStationRetuned = m_nTimeToDisplayRadioName = 0;
    m_prev = field_60 = 0;
    m_nRetuneStartedTime = 0;
    m_bEnabledInPauseMode = false;
    m_nSavedGameClockDays = m_nSavedGameClockHours = -1;
    m_bRadioAutoSelect = m_bBassEnhance = true;
    m_nSavedRadioStationId = m_iRadioStationMenuRequest = m_iRadioStationScriptRequest = RADIO_INVALID;
    m_nSpecialDJBanterPending = 3; // todo: enum
    m_nSpecialDJBanterIndex = -1;
    m_bPauseMode = m_bRetuneJustStarted = false;
    m_f80 = m_f84 = 0.0f;
    ResetStatistics();
}

// 0x4E8200
void CAERadioTrackManager::ResetStatistics() {
    m_nStatsCitiesPassed = 0;
    m_nStatsLastHitGameClockDays = -1;
    m_nStatsLastHitGameClockHours = -1;
    m_nStatsLastHitTimeOutHours = -1;
    m_nStatsPassedCasino3 = false;
    m_nStatsPassedCasino6 = false;
    m_nStatsPassedCasino10 = false;
    m_nStatsPassedCat1 = false;
    m_nStatsPassedDesert1 = false;
    m_nStatsPassedDesert3 = false;
    m_nStatsPassedDesert5 = false;
    m_nStatsPassedDesert8 = false;
    m_nStatsPassedDesert10 = false;
    m_nStatsPassedFarlie3 = false;
    m_nStatsPassedLAFin2 = false;
    m_nStatsPassedMansion2 = false;
    m_nStatsPassedRyder2 = false;
    m_nStatsPassedRiot1 = false;
    m_nStatsPassedSCrash1 = false;
    m_nStatsPassedStrap4 = false;
    m_nStatsPassedSweet2 = false;
    m_nStatsPassedTruth2 = false;
    m_nStatsPassedVCrash2 = false;
    m_nStatsStartedBadlands = false;
    m_nStatsStartedCat2 = false;
    m_nStatsStartedCrash1 = false;
}

// 0x4E8350
bool CAERadioTrackManager::IsRadioOn() const {
    return m_nMode != eRadioTrackMode::RADIO_STOPPED || m_bInitialised || m_nStationsListed || m_nStationsListDown;
}

// 0x4E8370
bool CAERadioTrackManager::HasRadioRetuneJustStarted() const {
    return m_bRetuneJustStarted;
}

// 0x4E83E0
int32* CAERadioTrackManager::GetRadioStationListenTimes() {
    return m_aListenTimes.data();
}

// 0x4E83F0
eRadioID CAERadioTrackManager::GetCurrentRadioStationID() const {
    return m_ActiveSettings.StationID == RADIO_INVALID ? RADIO_OFF : m_ActiveSettings.StationID;
}

// 0x4E82E0
void CAERadioTrackManager::SetRadioAutoRetuneOnOff(bool enable) {
    m_bRadioAutoSelect = enable;
}

// 0x4E82F0
void CAERadioTrackManager::SetBassSetting(eBassSetting bassSetting, float bassGrain) {
    m_RequestedSettings.BassGain = m_ActiveSettings.BassGain = bassGrain;
    m_RequestedSettings.BassSetting = m_ActiveSettings.BassSetting = bassSetting;
    AEAudioHardware.SetBassSetting(m_bBassEnhance ? bassSetting : eBassSetting::NORMAL, bassGrain);
}

// 0x4E9DB0
void CAERadioTrackManager::SetBassEnhanceOnOff(bool enable) {
    m_bBassEnhance = enable;
    if (m_nMode == eRadioTrackMode::RADIO_PLAYING) {
        m_RequestedSettings.BassSetting = m_ActiveSettings.BassSetting;
        m_RequestedSettings.BassGain = m_ActiveSettings.BassGain;
        if (enable) {
            AEAudioHardware.SetBassSetting(m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain);
        } else {
            AEAudioHardware.SetBassSetting(eBassSetting::NORMAL, m_ActiveSettings.BassGain);
        }
    }
}

// 0x4E8290
void CAERadioTrackManager::RetuneRadio(eRadioID id) {
    const auto retunedStation = [id] {
        if (id == RADIO_USER_TRACKS && !AEUserRadioTrackManager.m_nUserTracksCount) {
            return RADIO_OFF;
        } else {
            return id;
        }
    }();

    if (CTimer::GetIsPaused()) {
        m_iRadioStationMenuRequest = retunedStation;
        m_nRetuneStartedTime = CTimer::GetTimeInMSPauseMode();
    } else {
        m_iRadioStationScriptRequest = retunedStation;
    }
}

// 0x4E9E50
void CAERadioTrackManager::DisplayRadioStationName() {
    if (CTimer::GetIsPaused())
        return;

    if (TheCamera.m_bWideScreenOn)
        return;

    if (!FindPlayerVehicle())
        return;

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    if (m_bDisplayStationName && IsVehicleRadioActive()) {
        m_nTimeToDisplayRadioName = CTimer::GetTimeInMS() + 2500;
        m_bDisplayStationName = false;
    }

    if (CTimer::GetTimeInMS() < m_nTimeToDisplayRadioName) {
        int station = m_nStationsListed + m_RequestedSettings.StationID;
        if (station) {
            if (station >= RADIO_COUNT) {
                station -= RADIO_COUNT - 1;
            } else if (station <= 0) {
                station += RADIO_COUNT - 1;
            }

            CFont::SetFontStyle(eFontStyle::FONT_MENU);
            CFont::SetJustify(false);
            CFont::SetBackground(false, false);
            CFont::SetScale(SCREEN_SCALE_X(0.6f), SCREEN_SCALE_Y(0.9f));
            CFont::SetProportional(true);
            CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);
            CFont::SetRightJustifyWrap(0.0f);
            CFont::SetEdge(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, 255));
            CFont::SetColor(HudColour.GetRGB(m_nStationsListed || m_nStationsListDown ? HUD_COLOUR_DARK_GRAY : HUD_COLOUR_GOLD));
            CFont::PrintString(SCREEN_WIDTH / 2, SCREEN_SCALE_Y(22.0f), GetRadioStationName((eRadioID)station));
            CFont::DrawFonts();
        }
    }
}

// 0x4E9E10
const GxtChar* CAERadioTrackManager::GetRadioStationName(eRadioID id) {
    if (id <= 0) {
        NOTSA_UNREACHABLE();
        return nullptr;
    }

    char key[8];
    GetRadioStationNameKey(id, key);
    return TheText.Get(key);
}

// 0x4E8380
void CAERadioTrackManager::GetRadioStationNameKey(eRadioID id, char* outStr) {
    switch (id) {
    case RADIO_OFF:
        *std::format_to_n(outStr, 7u, "FEA_NON").out = '\0';
        break;
    case RADIO_USER_TRACKS:
        *std::format_to_n(outStr, 7u, "FEA_MP3").out = '\0';
        break;
    default:
        assert(0 <= id && id < RADIO_USER_TRACKS);
        *std::format_to_n(outStr, 7u, "FEA_R{:d}", (int32)id - 1).out = '\0';
        break;
    }
}

// 0x4E9800
bool CAERadioTrackManager::IsVehicleRadioActive() {
    if (const auto opts = CAEVehicleAudioEntity::StaticGetPlayerVehicleAudioSettingsForRadio()) {
        switch (opts->RadioType) {
        case AE_RT_CIVILIAN:
        case AE_RT_EMERGENCY:
        case AE_RT_UNKNOWN:
            return true;
        default:
            break;
        }
    }
    return false;
}

// 0x4E8410
void CAERadioTrackManager::CheckForMissionStatsChanges() {
    if (m_nSpecialDJBanterPending != 3) {
        if (CClock::GetGameClockHours() + 24 * CClock::GetNumberOfDaysPassed(m_nStatsLastHitGameClockDays) - m_nStatsLastHitGameClockHours >= m_nStatsLastHitTimeOutHours) {
            m_nSpecialDJBanterPending = 3;
        }
    }

    const auto statsCitiesPassed = CStats::GetStatValue<uint8>(STAT_CITY_UNLOCKED);
    if (m_nStatsCitiesPassed < statsCitiesPassed) {
        m_nStatsCitiesPassed = statsCitiesPassed;
        if (statsCitiesPassed == 1 || statsCitiesPassed == 2) {
            m_nStatsLastHitGameClockDays = CClock::GetGameClockDays();
            m_nStatsLastHitGameClockHours = CClock::GetGameClockHours();
            m_nStatsLastHitTimeOutHours = 24;
            m_nSpecialDJBanterPending = 1;
            m_nSpecialDJBanterIndex = m_nStatsCitiesPassed - 1;
        }
    }

    const auto Update = [](uint8& inputStat, const eStats stat, const auto specialDJBanterIndex) {
        const auto statValue = CStats::GetStatValue<uint8>(stat);
        if (inputStat < statValue) {
            inputStat = statValue;
            if (inputStat == 1) {
                m_nStatsLastHitGameClockDays = CClock::GetGameClockDays();
                m_nStatsLastHitGameClockHours = CClock::GetGameClockHours();
                m_nStatsLastHitTimeOutHours = 24 * 7;
                m_nSpecialDJBanterPending = 2;
                m_nSpecialDJBanterIndex = specialDJBanterIndex;
            }
        }
    };

    Update(m_nStatsPassedCasino3, STAT_LEAST_FAVORITE_RADIO_STATION, 0);
    Update(m_nStatsPassedCasino6, STAT_CURRENT_WEAPON_SKILL, 1);
    Update(m_nStatsPassedCasino10, STAT_WEAPON_SKILL_LEVELS, 2);
    Update(m_nStatsPassedCat1, STAT_LOCAL_LIQUOR_STORE_MISSION_ACCOMPLISHED, 3);
    Update(m_nStatsPassedDesert1, STAT_PLAYING_TIME, 4);
    Update(m_nStatsPassedDesert3, STAT_PILOT_RANKING, 5);
    Update(m_nStatsPassedDesert5, STAT_STRONGEST_GANG, 6);
    Update(m_nStatsPassedDesert8, STAT_2ND_STRONGEST_GANG, 7);
    Update(m_nStatsPassedDesert10, STAT_3RD_STRONGEST_GANG, 8);
    Update(m_nStatsPassedFarlie3, STAT_MIKE_TORENO_MISSION_ACCOMPLISHED, 9);
    Update(m_nStatsPassedLAFin2, STAT_LEAST_FAVORITE_GANG, 10);
    Update(m_nStatsPassedMansion2, STAT_A_HOME_IN_THE_HILLS_MISSION_ACCOMPLISHED, 11);
    Update(m_nStatsPassedRyder2, STAT_RYDERS_MISSION_ROBBING_UNCLE_SAM_ACCOMPLISHED, 12);
    Update(m_nStatsPassedRiot1, STAT_RIOT_MISSION_ACCOMPLISHED, 13);
    Update(m_nStatsPassedSCrash1, STAT_GANG_STRENGTH, 14);
    Update(m_nStatsPassedStrap4, STAT_TERRITORY_UNDER_CONTROL, 15);
    Update(m_nStatsPassedSweet2, STAT_DRIVE_THRU_MISSION_ACCOMPLISHED, 16);
    Update(m_nStatsPassedTruth2, STAT_ARE_YOU_GOING_TO_SAN_FIERRO_MISSION_ACCOMPLISHED, 17);
    Update(m_nStatsPassedVCrash2, STAT_HIGH_NOON_MISSION_ACCOMPLISHED, 18);
    Update(m_nStatsStartedBadlands, STAT_THE_GREEN_SABRE_MISSION_ACCOMPLISHED, 19);
    Update(m_nStatsStartedCat2, STAT_MAYBE_CATALINA_MEETING, 20);
    Update(m_nStatsStartedCrash1, STAT_MAYBE_WU_ZI_MEETING, 21);
}

// 0x4EA930
void CAERadioTrackManager::CheckForTrackConcatenation() {
    int8 trackCount = 1;

    // User tracks are concatenated (played without a gap) when the user track mode changed
    if (m_ActiveSettings.StationID == RADIO_USER_TRACKS) {
        const int8 userTrackPlayMode = m_nUserTrackPlayMode;
        if (userTrackPlayMode != AEUserRadioTrackManager.GetUserTrackPlayMode()) {
            if ((userTrackPlayMode == 2 || AEUserRadioTrackManager.GetUserTrackPlayMode() == 2)
                && m_ActiveSettings.PlayTime != -4
            ) {
                AEUserRadioTrackManager.SetUserTrackIndex(m_ActiveSettings.TrackQueue[0]);

                m_ActiveSettings.TrackQueue[1]   = AEUserRadioTrackManager.SelectUserTrackIndex();
                m_ActiveSettings.TrackTypes[1]   = TYPE_USER_TRACK;
                m_ActiveSettings.TrackIndices[1] = (int8)m_ActiveSettings.TrackQueue[1];

                AEAudioHardware.PlayTrack(
                    m_ActiveSettings.TrackQueue[0],
                    m_ActiveSettings.TrackQueue[1],
                    0,
                    m_ActiveSettings.TrackFlags,
                    m_ActiveSettings.TrackTypes[0] == TYPE_USER_TRACK,
                    m_ActiveSettings.TrackTypes[1] == TYPE_USER_TRACK
                );
            }
            m_nUserTrackPlayMode = AEUserRadioTrackManager.GetUserTrackPlayMode();
        }
    }

    // Move on to the next track as soon as the queued one starts playing
    if (AEAudioHardware.GetActiveTrackID() != m_ActiveSettings.TrackQueue[1] || m_ActiveSettings.TrackQueue[1] < 0) {
        return;
    }
    m_ActiveSettings.SwitchToNextTrack();

    // Refill the queue when nothing else is left to play
    if (m_ActiveSettings.TrackQueue[1] == -1) {
        const auto station = m_ActiveSettings.StationID;
        if (station == RADIO_USER_TRACKS) {
            if (!FrontEndMenuManager.m_RadioMode && CAEAudioUtility::ResolveProbability(0.17f)) {
                m_ActiveSettings.TrackQueue[trackCount] = ChooseAdvertIndex(RADIO_USER_TRACKS);
                m_ActiveSettings.TrackTypes[trackCount] = TYPE_ADVERT;
                trackCount++;
            }

            m_ActiveSettings.TrackQueue[trackCount]   = AEUserRadioTrackManager.SelectUserTrackIndex();
            m_ActiveSettings.TrackTypes[trackCount]   = TYPE_USER_TRACK;
            m_ActiveSettings.TrackIndices[trackCount] = (int8)m_ActiveSettings.TrackQueue[trackCount];
            trackCount++;

            m_ActiveSettings.TrackQueue[trackCount]   = AEUserRadioTrackManager.SelectUserTrackIndex();
            m_ActiveSettings.TrackTypes[trackCount]   = TYPE_USER_TRACK;
            m_ActiveSettings.TrackIndices[trackCount] = (int8)m_ActiveSettings.TrackQueue[trackCount];
        } else {
            switch (m_ActiveSettings.TrackTypes[0]) {
            case TYPE_INTRO:
            case TYPE_TRACK:
            case TYPE_OUTRO: {
                if (station == RADIO_EMERGENCY_AA) {
                    QueueUpTracksForStation(station, &trackCount, TYPE_DJ_BANTER, m_ActiveSettings);
                    break;
                }
                if (m_nTracksInARow[station] < 2 && CAEAudioUtility::ResolveProbability(0.5f)) {
                    if (CAEAudioUtility::ResolveProbability(0.5f)) {
                        QueueUpTracksForStation(station, &trackCount, TYPE_INDENT, m_ActiveSettings);
                    }
                    QueueUpTracksForStation(station, &trackCount, TYPE_INTRO, m_ActiveSettings);
                } else {
                    if (CAEAudioUtility::ResolveProbability(0.5f)) {
                        QueueUpTracksForStation(station, &trackCount, TYPE_INDENT, m_ActiveSettings);
                    }
                    if (!QueueUpTracksForStation(station, &trackCount, TYPE_DJ_BANTER, m_ActiveSettings)) {
                        QueueUpTracksForStation(station, &trackCount, TYPE_ADVERT, m_ActiveSettings);
                    }
                }
                break;
            }
            default: // TYPE_NONE, TYPE_INDENT, TYPE_ADVERT, TYPE_DJ_BANTER, TYPE_USER_TRACK
                QueueUpTracksForStation(station, &trackCount, TYPE_INTRO, m_ActiveSettings);
                break;
            }
        }
    }

    AEAudioHardware.PlayTrack(
        m_ActiveSettings.TrackQueue[0],
        m_ActiveSettings.TrackQueue[1],
        0,
        m_ActiveSettings.TrackFlags,
        m_ActiveSettings.TrackTypes[0] == TYPE_USER_TRACK,
        m_ActiveSettings.TrackTypes[1] == TYPE_USER_TRACK
    );
}

// 0x4EB660
void CAERadioTrackManager::CheckForStationRetune() {
    if (m_ActiveSettings.StationID == RADIO_EMERGENCY_AA) {
        return;
    }

    m_bRetuneJustStarted = false;

    // Only allow retuning while the radio has something going on
    const bool radioActive = m_nMode == eRadioTrackMode::RADIO_STARTING
                          || m_nMode == eRadioTrackMode::RADIO_WAITING_TO_PLAY
                          || m_nMode == eRadioTrackMode::RADIO_PLAYING
                          || m_bInitialised
                          || m_nStationsListed
                          || m_nStationsListDown
                          || m_ActiveSettings.StationID == RADIO_OFF;

    bool canRetune = false;
    if (radioActive && !AudioEngine.IsAmbienceRadioActive()) {
        const auto* vehAudioSettings = CAEVehicleAudioEntity::StaticGetPlayerVehicleAudioSettingsForRadio();
        if (vehAudioSettings
            && notsa::contains({AE_RT_CIVILIAN, AE_RT_UNKNOWN, AE_RT_EMERGENCY}, vehAudioSettings->RadioType)
            && CReplay::Mode != MODE_PLAYBACK
        ) {
            if (vehAudioSettings->RadioType != AE_RT_CIVILIAN) {
                return;
            }

            canRetune = true;
            if (m_iRadioStationScriptRequest >= RADIO_EMERGENCY_AA) {
                // A script asked for a specific station
                m_nStationsListDown         = m_nStationsListed;
                m_nStationsListed           = m_iRadioStationScriptRequest - m_RequestedSettings.StationID;
                m_iRadioStationScriptRequest = RADIO_INVALID;
                m_nTimeRadioStationRetuned  = CTimer::GetTimeInMS();
            } else if (CPad::GetPad(0)->NextStationJustUp()) {
                m_nStationsListDown = m_nStationsListed;
                m_nStationsListed++;
                m_nTimeRadioStationRetuned = CTimer::GetTimeInMS();
            } else if (CPad::GetPad(0)->LastStationJustUp()) {
                m_nStationsListDown = m_nStationsListed;
                m_nStationsListed--;
                m_nTimeRadioStationRetuned = CTimer::GetTimeInMS();
            } else {
                canRetune = false;
            }

            if (canRetune) {
                m_bDisplayStationName = true;
                m_bRetuneJustStarted  = true;
            }
        }
    }

    if (m_nStationsListed == 0 && m_nStationsListDown == 0) {
        return;
    }

    // Wrap the newly selected station into the valid range (RADIO_EMERGENCY_AA+1 .. RADIO_OFF)
    int8 station = (int8)(m_RequestedSettings.StationID + m_nStationsListed);
    if (station <= 0) {
        station += RADIO_OFF;
    } else if (station > RADIO_OFF) {
        station -= RADIO_OFF;
    }

    if (station == RADIO_OFF || (station == RADIO_USER_TRACKS && AEUserRadioTrackManager.m_nUserTracksCount == 0)) {
        // No station to tune into - turn the radio off
        StopRadio(nullptr, false);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_OFF);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
    } else {
        if (m_ActiveSettings.StationID == RADIO_OFF) {
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_ON);
            m_ActiveSettings.StationID = RADIO_INVALID;
        } else {
            StopRadio(nullptr, false);
        }
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_START);
    }

    // Wait a bit before actually switching, so the retune sound can be heard
    const uint32 now         = CTimer::GetTimeInMS();
    const uint32 retuneDelay = TheCamera.m_fPedOrientForBehindOrInFront > 0.9f ? 4000u : 2000u;
    if (now <= m_nTimeRadioStationRetuned + 1500u) {
        return;
    }
    if (now <= field_60 + retuneDelay) {
        return;
    }

    StartRadio((eRadioID)station, m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain, false);
    m_nStationsListed   = 0;
    m_nStationsListDown = 0;
}

// 0x4EB890
void CAERadioTrackManager::CheckForStationRetuneDuringPause() {
    if (m_ActiveSettings.StationID == RADIO_EMERGENCY_AA && IsRadioOn() || m_iRadioStationMenuRequest <= RADIO_INVALID)
        return;

    if (m_iRadioStationMenuRequest != RADIO_OFF) {
        if (m_ActiveSettings.StationID == RADIO_OFF) {
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_ON);
            m_ActiveSettings.StationID = RADIO_INVALID;
        } else {
            AudioEngine.StopRadio(nullptr, true);
        }

        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_START);
        if (CTimer::GetTimeInMSPauseMode() > m_nRetuneStartedTime + 700u) {
            StartRadio((eRadioID)m_iRadioStationMenuRequest, m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain, 0);
            m_iRadioStationMenuRequest = RADIO_INVALID;
        }
    } else {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_OFF);
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
        StartRadio(RADIO_OFF, m_ActiveSettings.BassSetting, m_ActiveSettings.BassGain, 0);
        m_iRadioStationMenuRequest = RADIO_INVALID;
    }
}

// 0x4EA640
void CAERadioTrackManager::StartTrackPlayback() {
    AEAudioHardware.SetChannelFlags(m_HwClientHandle, 0, 55);
    AEAudioHardware.StartTrackPlayback();
    UpdateRadioVolumes();
}

// 0x4EA010
void CAERadioTrackManager::UpdateRadioVolumes() {
    float volume = -4.0f;

    if (!CTimer::GetIsPaused() || !m_bEnabledInPauseMode) {
        const float effectsScalingFactor = AEAudioHardware.GetEffectsMasterScalingFactor();
        const float musicScalingFactor   = AEAudioHardware.GetMusicMasterScalingFactor();

        if (CTimer::GetIsSlowMotionActive()) {
            volume = -100.0f;
        } else if (TheCamera.m_bWideScreenOn) {
            volume = -16.0f;
        } else if (effectsScalingFactor > 0.0f && musicScalingFactor > 0.0f) {
            //! Duck the radio while mission audio is playing somewhere else
            bool missionAudioIsPlaying = false;
            if (!CAEPedSpeechAudioEntity::s_bForceAudible) {
                for (int32 i = 0; i < 2 && !missionAudioIsPlaying; i++) {
                    if (AudioEngine.IsMissionAudioSampleFinished(i)) {
                        continue;
                    }
                    if (AudioEngine.GetMissionAudioEvent(i) == 0xFFFF) {
                        continue;
                    }
                    if (const auto* missionAudioPos = AudioEngine.GetMissionAudioPosition(i)) {
                        missionAudioIsPlaying = CAEAudioEnvironment::GetPositionRelativeToCamera(*missionAudioPos).Magnitude() > 15.0f;
                    } else {
                        missionAudioIsPlaying = true;
                    }
                }
            } else {
                missionAudioIsPlaying = true;
            }

            if (missionAudioIsPlaying) {
                // Base the volume on how much louder the effects are compared to the music
                m_f80 = std::max(CAEAudioUtility::AudioLog10(effectsScalingFactor / musicScalingFactor) * 20.0f - 9.0f, 0.0f);
                m_f84 = -0.02f * m_f80;
                volume = m_f80 - 4.0f;
            } else if (m_f80 >= 0.0f) {
                m_f80 = std::min(m_f80 + m_f84, 0.0f);
                volume = m_f80 - 4.0f;
            }
        }

        if (AudioEngine.IsAmbienceRadioActive()) {
            volume -= 20.0f;
        }
    }

    if (m_bBassEnhance && m_ActiveSettings.BassSetting != eBassSetting::NORMAL) {
        switch (m_ActiveSettings.BassSetting) {
        case eBassSetting::BOOST:
            volume -= 2.0f;
            break;
        case eBassSetting::CUT:
            volume += 1.5f;
            break;
        default:
            break;
        }
    }

    AEAudioHardware.SetChannelVolume((int16)m_HwClientHandle, 0, volume, 0);
}

// 0x4E8400
void CAERadioTrackManager::PlayRadioAnnouncement(uint32) {
    // NOP
}

// 0x4EB550
void CAERadioTrackManager::StartRadio(const tVehicleAudioSettings& settings) {
    // plugin::CallMethod<0x4EB550, CAERadioTrackManager*, tVehicleAudioSettings*>(this, settings);

    if (CReplay::Mode == MODE_PLAYBACK)
        return;

    if (settings.RadioType == AE_RT_EMERGENCY) {
        StartRadio(RADIO_EMERGENCY_AA, settings.BassSetting, settings.BassFactor, 0);
        return;
    }

    if (settings.RadioType != AE_RT_CIVILIAN)
        return;

    const bool needsRetune = [&] {
       if (!m_bRadioAutoSelect)
           return false;

       const auto savedId = m_nSavedRadioStationId;
       if (savedId < 0 || savedId == settings.RadioStation || savedId == RADIO_OFF || savedId == RADIO_EMERGENCY_AA)
           return false;

       if (CTimer::GetTimeInMS() > m_nSavedTimeMs + 60'000)
           return false;

       if (m_nSavedGameClockHours < 0 || m_nSavedGameClockDays < 0)
           return false;

       if (CClock::GetGameClockHours() + 24 * CClock::GetNumberOfDaysPassed(m_nSavedGameClockDays) - m_nSavedGameClockHours > 5)
           return false;

       return true;
    }();

    if (needsRetune) {
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_START);
        StartRadio((eRadioID)m_nSavedRadioStationId, settings.BassSetting, settings.BassFactor, 0);
    } else {
        StartRadio(settings.RadioStation, settings.BassSetting, settings.BassFactor, 0);
    }
}

// 0x4EB3C0
void CAERadioTrackManager::StartRadio(eRadioID id, eBassSetting bassSetting, float bassGain, bool skipTrack) {
    id = std::min(id, RADIO_OFF);

    if (CTimer::GetIsPaused()) {
        m_bEnabledInPauseMode = true;

        if (IsRadioOn() && id == m_ActiveSettings.StationID) {
            m_aRadioState[id].m_iTimeInPauseModeInMs = CTimer::GetTimeInMSPauseMode();
            return;
        }
    }

    if (id != RADIO_OFF && CAudioEngine::IsAmbienceTrackActive()) {
        if (!CTimer::GetIsPaused() && CAudioEngine::DoesAmbienceTrackOverrideRadio()) {
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
            return;
        }
        AudioEngine.StopAmbienceTrack(false);
    }

    auto* const rs = &m_RequestedSettings;
    rs->StationID = id;
    rs->BassSetting             = bassSetting;
    rs->BassGain            = bassGain;
    if (id == RADIO_OFF) {
        rs->Reset();
    } else if (m_aRadioState[id].m_iTimeInMs < 0 || !TrackRadioStation(id, skipTrack)) {
        ChooseTracksForStation(rs->StationID);
        rs->PlayTime = CAEAudioUtility::GetRandomNumberInRange(0, 300'000);
    }
    switch (m_nMode) {
    case eRadioTrackMode::RADIO_STARTING:
    case eRadioTrackMode::RADIO_WAITING_TO_PLAY:
    case eRadioTrackMode::RADIO_PLAYING:
        m_nMode = eRadioTrackMode::RADIO_STOPPING;
    }
    m_aRadioState[rs->StationID].m_iTimeInPauseModeInMs = -1;

    m_bInitialised = true;
}

// 0x4EAC30
bool CAERadioTrackManager::TrackRadioStation(eRadioID id, bool skipTrack) {
    const auto station = (int32)id;
    auto&      state   = m_aRadioState[station];

    // The station remembers where it was for up to 5 game hours
    if (state.m_nGameClockHours >= 0 && state.m_nGameClockDays >= 0) {
        int32 daysElapsed = CClock::ms_nGameClockDays - state.m_nGameClockDays;
        if (daysElapsed < 0) {
            int32 month = CClock::ms_nGameClockMonth - 1;
            if (month < 0) {
                month += 12;
            }
            daysElapsed += CClock::daysInMonth[month];
        }
        if (daysElapsed * 24 - state.m_nGameClockHours + CClock::ms_nGameClockHours > 5) {
            return false;
        }
    }

    int32 elapsed = CTimer::GetTimeInMS() - state.m_iTimeInMs;
    if (elapsed < 7000) {
        elapsed = 7000;
    }
    if (skipTrack && elapsed <= state.m_aElapsed[0] + 1) {
        elapsed = state.m_aElapsed[0] + 1;
    }

    int8 trackCount = 0;
    rng::fill(m_RequestedSettings.TrackQueue, -1);
    rng::fill(m_RequestedSettings.TrackTypes, TYPE_NONE);
    rng::fill(m_RequestedSettings.TrackIndices, -1);

    // Find which of the three tracks of the station's queue is/was playing
    int32 trackIndex = 0;
    int32 elapsedSum = 0;
    for (; trackIndex < 3; trackIndex++) {
        elapsedSum += state.m_aElapsed[trackIndex];
        if (elapsed <= elapsedSum) {
            break;
        }
    }

    if (trackIndex == 3) {
        // The whole queue has been consumed already
        if (elapsed > elapsedSum + 7000) {
            if (elapsed <= elapsedSum + 155000) {
                // Still within the last track's length - just keep playing
                QueueUpTracksForStation(id, &trackCount, TYPE_TRACK, m_RequestedSettings);
                m_RequestedSettings.PlayTime = elapsed - elapsedSum - 5000;
                return true;
            }
            if (elapsed > elapsedSum + 160000) {
                return false;
            }
            if (id == RADIO_USER_TRACKS) {
                QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
                if (!FrontEndMenuManager.m_RadioMode && CAEAudioUtility::ResolveProbability(0.17f)) {
                    QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_ADVERT, m_RequestedSettings);
                }
            } else {
                QueueUpTracksForStation(id, &trackCount, TYPE_OUTRO, m_RequestedSettings);
                AddMusicTrackIndexToHistory(id, m_RequestedSettings.TrackIndices[0]);
                if (id == RADIO_EMERGENCY_AA) {
                    QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings);
                } else if (CAEAudioUtility::ResolveProbability(0.5f)) {
                    if (CAEAudioUtility::ResolveProbability(0.5f)) {
                        QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
                    }
                    QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
                } else {
                    if (CAEAudioUtility::ResolveProbability(0.5f)) {
                        QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
                    }
                    if (!QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings)) {
                        QueueUpTracksForStation(id, &trackCount, TYPE_ADVERT, m_RequestedSettings);
                    }
                }
            }
            m_RequestedSettings.PlayTime = elapsed - elapsedSum - 155000;
            return true;
        }

        // The station was only recently stopped - keep its queue going
        if (id == RADIO_USER_TRACKS) {
            QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
            QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
        } else {
            QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        }
        m_RequestedSettings.PlayTime = std::min(elapsed - elapsedSum, 5000);
        return true;
    }

    // Restore where inside the currently playing track the station was
    m_RequestedSettings.PlayTime = trackIndex == 0
        ? state.m_iTrackPlayTime + elapsed
        : state.m_aElapsed[trackIndex] - elapsedSum + elapsed;

    const auto currTrackType = state.m_aTrackTypes[trackIndex];
    switch (currTrackType) {
    case TYPE_INDENT:
    case TYPE_ADVERT:
    case TYPE_DJ_BANTER:
        m_RequestedSettings.TrackQueue[0] = state.m_aTrackQueue[trackIndex];
        m_RequestedSettings.TrackTypes[0] = currTrackType;
        if (id == RADIO_USER_TRACKS) {
            QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
        } else {
            QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        }
        return true;
    case TYPE_INTRO: // Intro, music track and outro are still queued
        for (int32 i = 0; i < 3; i++) {
            m_RequestedSettings.TrackQueue[i] = state.m_aTrackQueue[trackIndex + i];
            m_RequestedSettings.TrackTypes[i] = state.m_aTrackTypes[trackIndex + i];
        }
        return true;
    case TYPE_TRACK: // Music track and outro are still queued
        for (int32 i = 0; i < 2; i++) {
            m_RequestedSettings.TrackQueue[i] = state.m_aTrackQueue[trackIndex + i];
            m_RequestedSettings.TrackTypes[i] = state.m_aTrackTypes[trackIndex + i];
        }
        return true;
    case TYPE_OUTRO:
        m_RequestedSettings.TrackQueue[0] = state.m_aTrackQueue[trackIndex];
        m_RequestedSettings.TrackTypes[0] = currTrackType;
        if (id == RADIO_EMERGENCY_AA) {
            QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings);
        } else {
            if (CAEAudioUtility::ResolveProbability(0.5f)) {
                QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
            }
            if (!QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings)) {
                QueueUpTracksForStation(id, &trackCount, TYPE_ADVERT, m_RequestedSettings);
            }
        }
        return true;
    case TYPE_USER_TRACK:
        for (int32 i = 0; i < 2; i++) {
            m_RequestedSettings.TrackQueue[i] = state.m_aTrackQueue[trackIndex + i];
            m_RequestedSettings.TrackTypes[i] = state.m_aTrackTypes[trackIndex + i];
        }
        if (m_RequestedSettings.TrackQueue[1] == -1) {
            m_RequestedSettings.TrackQueue[1]   = AEUserRadioTrackManager.SelectUserTrackIndex();
            m_RequestedSettings.TrackTypes[1]   = TYPE_USER_TRACK;
            m_RequestedSettings.TrackIndices[1] = (int8)m_RequestedSettings.TrackQueue[1];
        }
        return true;
    default: // TYPE_NONE
        return false;
    }
}

// 0x4EA670
bool CAERadioTrackManager::QueueUpTracksForStation(eRadioID id, int8* iTrackCount, int8 radioState, tRadioSettings& settings) {
    // Per-station/per-track tables (only the 12 track stations have them)
    static constexpr size_t NUM_STATIONS_WITH_TRACKS = 12;
    static constexpr size_t NUM_TRACK_INDICES        = 31;
    using tTrackRanges = int32[NUM_STATIONS_WITH_TRACKS][NUM_TRACK_INDICES][2];

    static auto& s_TrackIds    = StaticRef<int32[NUM_STATIONS_WITH_TRACKS][NUM_TRACK_INDICES]>(0x8C9040); //!< Music track IDs
    static auto& s_IntroRanges = StaticRef<tTrackRanges>(0x8C9610); //!< Intro track ranges
    static auto& s_OutroRanges = StaticRef<tTrackRanges>(0x8CA1B0); //!< Outro track ranges

    const auto station = (int32)id;
    int8&      count   = *iTrackCount;

    switch (radioState) {
    case TYPE_INDENT: {
        const auto index = ChooseIdentIndex(id);
        settings.TrackQueue[count] = index;
        if (index == -1) {
            return false;
        }
        settings.TrackTypes[count] = TYPE_INDENT;
        count++;
        return true;
    }
    case TYPE_ADVERT:
        settings.TrackQueue[count] = ChooseAdvertIndex(id);
        settings.TrackTypes[count] = TYPE_ADVERT;
        count++;
        return true;
    case TYPE_DJ_BANTER: {
        const auto index = ChooseDJBanterIndex(id);
        settings.TrackQueue[count] = index;
        if (index == -1) {
            return false;
        }
        settings.TrackTypes[count] = TYPE_DJ_BANTER;
        count++;
        return true;
    }
    case TYPE_INTRO: {
        if (id == RADIO_USER_TRACKS) {
            return false;
        }
        settings.TrackIndices[count] = ChooseMusicTrackIndex(id);
        settings.TrackQueue[count]   = CAEAudioUtility::GetRandomNumberInRange(
            s_IntroRanges[station][settings.TrackIndices[count]][0],
            s_IntroRanges[station][settings.TrackIndices[count]][1]
        );
        settings.TrackTypes[count] = TYPE_INTRO;
        count++;

        settings.TrackIndices[count] = settings.TrackIndices[count - 1];
        settings.TrackQueue[count]   = s_TrackIds[station][settings.TrackIndices[count]];
        settings.TrackTypes[count]   = TYPE_TRACK;
        count++;

        settings.TrackIndices[count] = settings.TrackIndices[count - 1];
        settings.TrackQueue[count]   = CAEAudioUtility::GetRandomNumberInRange(
            s_OutroRanges[station][settings.TrackIndices[count]][0],
            s_OutroRanges[station][settings.TrackIndices[count]][1]
        );
        settings.TrackTypes[count] = TYPE_OUTRO;
        count++;
        return true;
    }
    case TYPE_TRACK: {
        if (id == RADIO_USER_TRACKS) {
            settings.TrackQueue[count]   = AEUserRadioTrackManager.SelectUserTrackIndex();
            settings.TrackTypes[count]   = TYPE_USER_TRACK;
            settings.TrackIndices[count] = (int8)settings.TrackQueue[count];
            count++;
            return true;
        }
        settings.TrackIndices[count] = ChooseMusicTrackIndex(id);
        settings.TrackQueue[count]   = s_TrackIds[station][settings.TrackIndices[count]];
        settings.TrackTypes[count]   = TYPE_TRACK;
        count++;

        settings.TrackIndices[count] = settings.TrackIndices[count - 1];
        settings.TrackQueue[count]   = CAEAudioUtility::GetRandomNumberInRange(
            s_OutroRanges[station][settings.TrackIndices[count]][0],
            s_OutroRanges[station][settings.TrackIndices[count]][1]
        );
        settings.TrackTypes[count] = TYPE_OUTRO;
        count++;
        return true;
    }
    case TYPE_OUTRO: {
        if (id == RADIO_USER_TRACKS) {
            return false;
        }
        settings.TrackIndices[count] = ChooseMusicTrackIndex(id);
        settings.TrackQueue[count]   = CAEAudioUtility::GetRandomNumberInRange(
            s_OutroRanges[station][settings.TrackIndices[count]][0],
            s_OutroRanges[station][settings.TrackIndices[count]][1]
        );
        settings.TrackTypes[count] = TYPE_OUTRO;
        count++;
        return true;
    }
    default: // TYPE_NONE, TYPE_USER_TRACK
        return false;
    }
}

// 0x4E9820
void CAERadioTrackManager::StopRadio(tVehicleAudioSettings* settings, bool duringPause) {
#ifdef NOTSA_NATIVE
    // No radio stream module natively: drop the radio state so `IsRadioOn` reports false.
    (void)settings;
    m_nMode           = eRadioTrackMode::RADIO_STOPPED;
    m_bInitialised    = false;
    m_bPauseMode      = duringPause;
    m_nStationsListed = 0;
    m_nStationsListDown = 0;
#else
    plugin::CallMethod<0x4E9820, CAERadioTrackManager*, tVehicleAudioSettings*, bool>(this, settings, duringPause);
#endif
}

// 0x4E94C0
int32 CAERadioTrackManager::ChooseIdentIndex(eRadioID id) {
    static auto& s_IdentRanges = StaticRef<int32[RADIO_COUNT][2]>(0x8C8FB0); // {firstTrack, lastTrack} per station

    const auto i = static_cast<int32>(id);
    if (s_IdentRanges[i][0] == 0x782) { // station has no idents
        return -1;
    }

    while (true) {
        const int32 candidate = CAEAudioUtility::GetRandomNumberInRange(s_IdentRanges[i][0], s_IdentRanges[i][1]);

        // Radio Los Santos' ident #1100 only plays once the relevant stat has been earned
        if (id == RADIO_MODERN_HIP_HOP && candidate == 0x44c && CStats::GetStatValue((eStats)0x152) <= 0.0f) {
            continue;
        }

        const int32 historyDepth = std::min(s_IdentRanges[i][1] - s_IdentRanges[i][0] - 1, IDENT_INDEX_HISTORY_COUNT);
        bool        recent       = false;
        for (int32 j = 0; j < historyDepth; j++) {
            if (candidate == m_nIdentIndexHistory[i].indices[j]) {
                recent = true;
                break;
            }
        }
        if (!recent) {
            return candidate;
        }
    }
}

// 0x4E9570
int32 CAERadioTrackManager::ChooseAdvertIndex(eRadioID id) {
    static auto& s_AdvertFirstTrack = StaticRef<int32>(0x8C8B88);
    static auto& s_AdvertLastTrack  = StaticRef<int32>(0x8C8B8C);
    static auto& s_StationAdvertExclusions = StaticRef<int32[RADIO_COUNT][23]>(0x8CADD0); // adverts to skip per station

    const auto i = static_cast<int32>(id);
    while (true) {
        const int32 candidate = CAEAudioUtility::GetRandomNumberInRange(s_AdvertFirstTrack, s_AdvertLastTrack);

        bool reject = false;
        for (int32 j = 0; j < 23; j++) {
            if (candidate == s_StationAdvertExclusions[i][j]) {
                reject = true;
                break;
            }
        }
        if (reject) {
            continue;
        }
        for (int32 j = 0; j < ADVERT_INDEX_HISTORY_COUNT; j++) {
            if (candidate == m_nAdvertIndexHistory[i].indices[j]) {
                reject = true;
                break;
            }
        }
        if (!reject) {
            return candidate;
        }
    }
}

// 0x4EA270
int8 CAERadioTrackManager::ChooseMusicTrackIndex(eRadioID id) {
    if (id == RADIO_TALK) {
        return ChooseTalkRadioShow();
    }

    static auto& s_MusicTrackCounts = StaticRef<int32[RADIO_COUNT]>(0x8C9010); // number of music tracks per station
    const auto   i = static_cast<int32>(id);

    while (true) {
        const auto  candidate    = static_cast<int8>(CAEAudioUtility::GetRandomNumberInRange(0, s_MusicTrackCounts[i] - 1));
        const int32 historyDepth = std::min(s_MusicTrackCounts[i] - 2, MUSIC_TRACK_HISTORY_COUNT);
        bool        recent       = false;
        for (int32 j = 0; j < historyDepth; j++) {
            if (candidate == m_nMusicTrackIndexHistory[i].indices[j]) {
                recent = true;
                break;
            }
        }
        if (!recent) {
            return candidate;
        }
    }
}

// 0x4EA2D0
int32 CAERadioTrackManager::ChooseDJBanterIndex(eRadioID id) {
    // Banter ranges are `{ firstTrack, lastTrack }` pairs per station (only the 12 track stations have them)
    static constexpr size_t NUM_STATIONS_WITH_TRACKS = 12;
    using tBanterRanges = int32[NUM_STATIONS_WITH_TRACKS][2];

    static auto& s_SpecialBanterRanges = StaticRef<int32[RADIO_COUNT][2]>(0x8C8BF0);
    static auto& s_CityBanterRanges    = StaticRef<int32[RADIO_COUNT][2]>(0x8C8C50);
    static auto& s_CityBanterIndices   = StaticRef<int32[RADIO_COUNT][22]>(0x8CB280); //!< Per city-unlocked banter
    static auto& s_DefaultRanges       = StaticRef<int32[RADIO_COUNT][2]>(0x8C8CB0);
    static auto& s_EveningRanges       = StaticRef<tBanterRanges>(0x8C8D70);
    static auto& s_MorningRanges       = StaticRef<tBanterRanges>(0x8C8DD0);
    static auto& s_LateNightRanges     = StaticRef<tBanterRanges>(0x8C8E30);
    static auto& s_RainyRanges         = StaticRef<tBanterRanges>(0x8C8E90);
    static auto& s_FoggyRanges         = StaticRef<tBanterRanges>(0x8C8F50);

    const auto station = (int32)id;
    int32      index   = -1;

    // 1. Banter reserved by `CheckForMissionStatsChanges` (if any)
    switch (m_nSpecialDJBanterPending) {
    case 0:
        index = s_SpecialBanterRanges[station][0];
        break;
    case 1:
        switch (m_nSpecialDJBanterIndex) {
        case 0:
            index = s_CityBanterRanges[station][0];
            break;
        case 1:
            if (s_CityBanterRanges[station][0] != s_CityBanterRanges[station][1]) {
                index = s_CityBanterRanges[station][1];
            }
            break;
        default:
            break;
        }
        break;
    case 2:
        index = s_CityBanterIndices[station][m_nSpecialDJBanterIndex];
        break;
    default:
        break;
    }

    // 2. Reject the "no banter" marker (0x782) and anything played recently
    if (index == 0x782) {
        index = -1;
    } else if (index >= 0) {
        for (const auto recent : m_nDJBanterIndexHistory[station].indices) {
            if (index == recent) {
                index = -1;
                break;
            }
        }
    }
    if (index != -1) {
        return index;
    }

    // 3. Otherwise pick one depending on the current situation
    if (id == RADIO_EMERGENCY_AA) {
        return ChooseDJBanterIndexFromList(id, (int32**)(CGameLogic::LaRiotsActiveHere() ? s_LateNightRanges : s_DefaultRanges));
    }

    if (!CAEAudioUtility::ResolveProbability(0.6f)) {
        return -1;
    }
    if (CGame::currArea) {
        return -1;
    }

    const uint8 hour = CClock::ms_nGameClockHours;
    if (CWeather::ForecastWeather(WEATHER_RAINY_COUNTRYSIDE, 3) || CWeather::ForecastWeather(WEATHER_RAINY_SF, 3)) {
        if (CAEAudioUtility::ResolveProbability(0.5f)) {
            if (const auto idx = ChooseDJBanterIndexFromList(id, (int32**)s_RainyRanges); idx != -1) {
                return idx;
            }
        }
    } else if (CWeather::ForecastWeather(WEATHER_EXTRASUNNY_LA, 3)
            || CWeather::ForecastWeather(WEATHER_EXTRASUNNY_SMOG_LA, 3)
            || CWeather::ForecastWeather(WEATHER_EXTRASUNNY_COUNTRYSIDE, 3)
            || CWeather::ForecastWeather(WEATHER_EXTRASUNNY_SF, 3)
            || CWeather::ForecastWeather(WEATHER_EXTRASUNNY_VEGAS, 3)
            || CWeather::ForecastWeather(WEATHER_EXTRASUNNY_DESERT, 3)) {
        // NOTE: Retail also requires `hour >= 8 && hour < 6`, which can never be true,
        //       so the extra-sunny banter list (`0x8C8EF0`) is never played.
    } else if (CWeather::ForecastWeather(WEATHER_FOGGY_SF, 3)) {
        if (CAEAudioUtility::ResolveProbability(0.5f)) {
            if (const auto idx = ChooseDJBanterIndexFromList(id, (int32**)s_FoggyRanges); idx != -1) {
                return idx;
            }
        }
    }

    // 4. Time-of-day specific banter
    if (hour >= 6 && hour < 9) {
        if (CAEAudioUtility::ResolveProbability(0.3f)) {
            if (const auto idx = ChooseDJBanterIndexFromList(id, (int32**)s_MorningRanges); idx != -1) {
                return idx;
            }
        }
    } else if (hour >= 18 && hour < 21) {
        if (CAEAudioUtility::ResolveProbability(0.3f)) {
            if (const auto idx = ChooseDJBanterIndexFromList(id, (int32**)s_EveningRanges); idx != -1) {
                return idx;
            }
        }
    } else if (hour >= 22 || hour < 3) {
        if (CAEAudioUtility::ResolveProbability(0.3f)) {
            if (const auto idx = ChooseDJBanterIndexFromList(id, (int32**)s_LateNightRanges); idx != -1) {
                return idx;
            }
        }
    }

    return ChooseDJBanterIndexFromList(id, (int32**)s_DefaultRanges);
}

// 0x4E95E0
int32 CAERadioTrackManager::ChooseDJBanterIndexFromList(eRadioID id, int32** list) {
    // `list` is really a flat {firstTrack, lastTrack} pair per station (stride 2 int32).
    static auto& s_DefaultDJBanterRanges = StaticRef<int32[RADIO_COUNT][2]>(0x8C8CB0);

    const auto  i      = static_cast<int32>(id);
    const auto* ranges = reinterpret_cast<const int32*>(list);
    const int32 first  = ranges[i * 2];
    if (first == 0x782) { // station has no DJ banter
        return -1;
    }

    const int32 spread = ranges[i * 2 + 1] - first; // last - first
    const int32 count  = spread + 1;
    if (count < 1) {
        return -1;
    }

    const int32 rnd          = CAEAudioUtility::GetRandomNumberInRange(0, spread);
    const int32 historyDepth = std::min(s_DefaultDJBanterRanges[i][1] - s_DefaultDJBanterRanges[i][0] - 1, DJBANTER_INDEX_HISTORY_COUNT);

    for (int32 offset = 0; offset < count; offset++) {
        const int32 candidate = (offset + rnd) % count + first;
        bool        recent    = false;
        for (int32 j = 0; j < historyDepth; j++) {
            if (candidate == m_nDJBanterIndexHistory[i].indices[j]) {
                recent = true;
                break;
            }
        }
        if (!recent) {
            return candidate;
        }
    }
    return -1;
}

// 0x4EB180
void CAERadioTrackManager::ChooseTracksForStation(eRadioID id) {
    int8 trackCount = 0;

    for (auto i = 0u; i < tRadioSettings::NUM_TRACKS; i++) {
        m_RequestedSettings.TrackTypes[i] = TYPE_NONE;
        m_RequestedSettings.TrackQueue[i] = -1;
        m_RequestedSettings.TrackIndices[i] = -1;
    }

    if (!CAEAudioUtility::ResolveProbability(0.95f)) {
        if (id) {
            if (CAEAudioUtility::ResolveProbability(0.5f))
                QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);

            if (!QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings))
                QueueUpTracksForStation(id, &trackCount, TYPE_ADVERT, m_RequestedSettings);

            if (id == RADIO_USER_TRACKS) {
                QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
                return;
            }
        } else {
            QueueUpTracksForStation(RADIO_EMERGENCY_AA, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings);
        }
        QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        return;
    }

    if (id == RADIO_USER_TRACKS) {
        QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
        QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_TRACK, m_RequestedSettings);
        if (!FrontEndMenuManager.m_RadioMode && CAEAudioUtility::ResolveProbability(0.17f)) {
            QueueUpTracksForStation(RADIO_USER_TRACKS, &trackCount, TYPE_ADVERT, m_RequestedSettings);
        }
        return;
    }

    if (CAEAudioUtility::ResolveProbability(0.9f)) {
        QueueUpTracksForStation(id, &trackCount, TYPE_TRACK, m_RequestedSettings);
        return;
    }

    if (CAEAudioUtility::ResolveProbability(0.5f)) {
        if (CAEAudioUtility::ResolveProbability(0.5f)) {
            QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
        }
        QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        return;
    }

    QueueUpTracksForStation(id, &trackCount, TYPE_OUTRO, m_RequestedSettings);
    AddMusicTrackIndexToHistory(id, m_RequestedSettings.TrackIndices[trackCount - 1]);

    if (id == RADIO_EMERGENCY_AA) {
        QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings);
        return;
    }

    if (CAEAudioUtility::ResolveProbability(0.5f)) {
        if (CAEAudioUtility::ResolveProbability(0.5f)) {
            QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);
        }
        QueueUpTracksForStation(id, &trackCount, TYPE_INTRO, m_RequestedSettings);
        return;
    }

    if (CAEAudioUtility::ResolveProbability(0.5f))
        QueueUpTracksForStation(id, &trackCount, TYPE_INDENT, m_RequestedSettings);

    if (!QueueUpTracksForStation(id, &trackCount, TYPE_DJ_BANTER, m_RequestedSettings))
        QueueUpTracksForStation(id, &trackCount, TYPE_ADVERT, m_RequestedSettings);
}

// 0x4E8E40
int8 CAERadioTrackManager::ChooseTalkRadioShow() {
    // Build the list of shows that are currently unlocked by mission/stat progress
    int8  shows[32]{};
    int32 numShows = 0;

    const auto Unlocked = [](eStats stat) { return CStats::GetStatValue(stat) != 0.0f; };

    if (!Unlocked(STAT_RYDERS_MISSION_ROBBING_UNCLE_SAM_ACCOMPLISHED) && Unlocked(STAT_MIKE_TORENO_MISSION_ACCOMPLISHED)) {
        shows[0] = 14;
        numShows = 1;
    }
    if (!Unlocked(STAT_ARCHITECTURAL_ESPIONAGE_MISSION_ACCOMPLISHED)) {
        shows[0] = 15;
        numShows = 1;
    }
    if (Unlocked(STAT_JIZZY_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 12;
    } else if (Unlocked(STAT_ARCHITECTURAL_ESPIONAGE_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 13;
    }
    if (!Unlocked(STAT_SMALL_TOWN_BANK_MISSION_ACCOMPLISHED) && Unlocked(STAT_PHOTO_OPPORTUNITY_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 6;
    }
    if (!Unlocked(STAT_DRIVE_THRU_MISSION_ACCOMPLISHED) && Unlocked(STAT_REUNITING_THE_FAMILIES_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 3;
    } else if (!Unlocked(STAT_PHOTO_OPPORTUNITY_MISSION_ACCOMPLISHED) && Unlocked(STAT_DON_PEYOTE_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 4;
    } else if (!Unlocked(STAT_DON_PEYOTE_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 5;
    }
    shows[numShows++] = Unlocked(STAT_LOCAL_LIQUOR_STORE_MISSION_ACCOMPLISHED) ? 7 : 8;
    if (Unlocked(STAT_BADLANDS_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 9;
    } else if (Unlocked(STAT_555_WE_TIP_MISSION_ACCOMPLISHED) && Unlocked(STAT_PLAYING_TIME)) {
        shows[numShows++] = 10;
    } else if (!Unlocked(STAT_PLAYING_TIME)) {
        shows[numShows++] = 11;
    }
    shows[numShows++] = Unlocked(STAT_HIDDEN_PACKAGES_FOUND) ? 0x1B : 0x1C;
    shows[numShows++] = Unlocked(STAT_TAGS_SPRAYED) ? 0x1D : 0x1E;
    if (Unlocked(STAT_LEAST_FAVORITE_GANG)) {
        shows[numShows++] = 0;
    } else if (!Unlocked(STAT_GANG_MEMBERS_WASTED) && Unlocked(STAT_CRIMINALS_WASTED)) {
        shows[numShows++] = 1;
    } else if (!Unlocked(STAT_MOST_FAVORITE_RADIO_STATION)) {
        shows[numShows++] = 2;
    }
    if (Unlocked(STAT_DRIVE_THRU_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 0x10;
    } else if (Unlocked(STAT_MANAGEMENT_ISSUES_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 0x11;
    } else if (Unlocked(STAT_LEAST_FAVORITE_GANG)) {
        shows[numShows++] = 0x12;
    } else if (Unlocked(STAT_555_WE_TIP_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 0x13;
    } else if (Unlocked(STAT_YAY_KA_BOOM_BOOM_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 0x14;
    } else if (Unlocked(STAT_FISH_IN_A_BARREL_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 0x15;
    } else if (Unlocked(STAT_BREAKING_THE_BANK_AT_CALIGULAS_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 0x16;
    } else if (Unlocked(STAT_A_HOME_IN_THE_HILLS_MISSION_ACCOMPLISHED)) {
        shows[numShows++] = 0x17;
    } else if (Unlocked(STAT_MAYBE_SET_RIOT_MODE)) {
        shows[numShows++] = 0x18;
    } else if (CStats::GetStatValue(STAT_CITY_UNLOCKED) == 4.0f) {
        // Nothing left to unlock once every city is available
    } else {
        shows[numShows++] = 0x1A;
    }

    // Pick a random show that wasn't played recently (only the last `numShows - 1` ones count)
    for (;;) {
        const int8 candidate = shows[CAEAudioUtility::GetRandomNumberInRange(0, numShows - 1)];
        bool       recent    = false;
        for (int32 i = 0; i < numShows - 1; i++) {
            if (candidate == m_nMusicTrackIndexHistory[RADIO_TALK].indices[i]) {
                recent = true;
                break;
            }
        }
        if (!recent) {
            return candidate;
        }
    }
}

// 0x4E96C0
void CAERadioTrackManager::AddMusicTrackIndexToHistory(eRadioID id, int8 trackIndex) {
    if (trackIndex >= 0 && m_nMusicTrackIndexHistory[id].indices[0] != trackIndex) {
        m_nMusicTrackIndexHistory[id].PutAtFirst(trackIndex);
        m_nTracksInARow[id]++;
    }
}

// 0x4E9720
void CAERadioTrackManager::AddIdentIndexToHistory(eRadioID id, int8 trackIndex) {
    if (m_nIdentIndexHistory[id].indices[0] != trackIndex)
        m_nIdentIndexHistory[id].PutAtFirst(trackIndex);
}

// 0x4E9760
void CAERadioTrackManager::AddAdvertIndexToHistory(eRadioID id, int8 trackIndex) {
    if (m_nAdvertIndexHistory[id].indices[0] != trackIndex) {
        m_nAdvertIndexHistory[id].PutAtFirst(trackIndex);
        m_nTracksInARow[id] = 0;
    }
}

// 0x4E97B0
void CAERadioTrackManager::AddDJBanterIndexToHistory(eRadioID id, int8 trackIndex) {
    if (m_nDJBanterIndexHistory[id].indices[0] != trackIndex) {
        m_nDJBanterIndexHistory[id].PutAtFirst(trackIndex);
        m_nTracksInARow[id] = 0;
    }
}

// 0x4EA590
void CAERadioTrackManager::CheckForPause() {
    if (CTimer::GetIsPaused()) {
        m_bPauseMode = true;
        AEAudioHardware.SetChannelFrequencyScalingFactor(m_HwClientHandle, 0, m_bEnabledInPauseMode ? 1.0f : 0.0f);
    } else {
        const auto* settings = CAEVehicleAudioEntity::StaticGetPlayerVehicleAudioSettingsForRadio();
        
        if (settings && notsa::contains({
                AE_RT_CIVILIAN,
                AE_RT_EMERGENCY,
                AE_RT_UNKNOWN
            }, settings->RadioType)
            || CAudioEngine::IsAmbienceRadioActive()
        ) {
            m_bPauseMode = false;
            AEAudioHardware.SetChannelFrequencyScalingFactor(m_HwClientHandle, 0, 1.0f);
        } else {
            StopRadio(nullptr, false);
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_RETUNE_STOP);
            m_bPauseMode = false;
        }
    }
}

// 0x4EB9A0
void CAERadioTrackManager::Service(int32 playTime) {
#ifdef NOTSA_NATIVE
    // No MP3/radio stream module natively: there is never a track to service.
    (void)playTime;
#else
    plugin::CallMethod<0x4EB9A0, CAERadioTrackManager*, int32>(this, playTime);
#endif
}

// 0x5D40E0
void CAERadioTrackManager::Load() {
    for (auto r = 0; r < RADIO_COUNT; r++) {
        for (auto& historyIndex : m_nMusicTrackIndexHistory[r].indices) {
            CGenericGameStorage::LoadDataFromWorkBuffer(historyIndex);
        }

        for (auto& identIndex : m_nIdentIndexHistory[r].indices) {
            CGenericGameStorage::LoadDataFromWorkBuffer(identIndex);
        }

        for (auto& advertIndex : m_nAdvertIndexHistory[r].indices) {
            CGenericGameStorage::LoadDataFromWorkBuffer(advertIndex);
        }

        for (auto& banterIndex : m_nDJBanterIndexHistory[r].indices) {
            CGenericGameStorage::LoadDataFromWorkBuffer(banterIndex);
        }
    }

    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsCitiesPassed);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedCasino3);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedCasino6);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedCasino10);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedCat1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert3);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert5);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert8);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedDesert10);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedFarlie3);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedLAFin2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedMansion2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedRyder2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedRiot1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedSCrash1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedStrap4);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedSweet2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedTruth2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsPassedVCrash2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsStartedBadlands);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsStartedCat2);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsStartedCrash1);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsLastHitGameClockDays);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsLastHitGameClockHours);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nStatsLastHitTimeOutHours);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nSpecialDJBanterPending);
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nSpecialDJBanterIndex);
}

// 0x5D3EE0
void CAERadioTrackManager::Save() {
    for (auto r = 0; r < RADIO_COUNT; r++) {
        for (auto& historyIndex : m_nMusicTrackIndexHistory[r].indices) {
            CGenericGameStorage::SaveDataToWorkBuffer(historyIndex);
        }

        for (auto& identIndex : m_nIdentIndexHistory[r].indices) {
            CGenericGameStorage::SaveDataToWorkBuffer(identIndex);
        }

        for (auto& advertIndex : m_nAdvertIndexHistory[r].indices) {
            CGenericGameStorage::SaveDataToWorkBuffer(advertIndex);
        }

        for (auto& banterIndex : m_nDJBanterIndexHistory[r].indices) {
            CGenericGameStorage::SaveDataToWorkBuffer(banterIndex);
        }
    }

    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsCitiesPassed);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedCasino3);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedCasino6);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedCasino10);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedCat1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert3);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert5);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert8);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedDesert10);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedFarlie3);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedLAFin2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedMansion2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedRyder2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedRiot1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedSCrash1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedStrap4);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedSweet2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedTruth2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsPassedVCrash2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsStartedBadlands);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsStartedCat2);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsStartedCrash1);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsLastHitGameClockDays);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsLastHitGameClockHours);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nStatsLastHitTimeOutHours);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nSpecialDJBanterPending);
    CGenericGameStorage::SaveDataToWorkBuffer(m_nSpecialDJBanterIndex);
}
