#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>
#include "AudioEngine.h"
#include "CullZones.h"
#include "MenuManager.h"
#include "World.h"
#include "Stats.h"
#include "TaskComplexProstituteSolicit.h"
#include "Population.h"
#include "PostEffects.h"
#include "Restart.h"
#include "Garages.h"
#include "Garage.h"
#include "MenuSystem.h"
#include "Shadows.h"
#include "Radar.h"
#include "Camera.h"
#include "Weather.h"
#include "Pickups.h"
#include "Pickup.h"
#include "Gangs.h"
#include "Game.h"
#include "Streaming.h"
#include "StreamingInfo.h"
#include "StreamedScripts.h"
#include "ScriptsForBrains.h"
#include "Scripts/TheScripts.h"
#include "ScriptResourceManager.h"
#include "EntryExitManager.h"
#include "EntryExit.h"
#include "3dMarkers.h"
#include "IplStore.h"
#include "TagManager.h"
#include "Shopping.h"
#include "GangWars.h"
#include "GameLogic.h"
#include "Messages.h"
#include "Hud.h"
#include "UserDisplay.h"
#include "Pad.h"
#include "HudColours.h"
#include "Text/Text.h"
#include "Animation/AnimManager.h"
#include "Animation/AnimBlock.h"
#include "PedType.h"
#include "Acquaintance.h"
#include "PedGroups.h"
#include "PedGroup.h"
#include "DecisionMakers/DecisionMakerTypes.h"
#include "Pools/Pools.h"
#include "Models/ModelInfo.h"
#include "Models/VehicleModelInfo.h"
#include "Entity/Ped/Ped.h"
#include "Entity/Ped/PlayerPed.h"
#include "Entity/Vehicle/Vehicle.h"
#include "Entity/Vehicle/Train.h"
#include "Tasks/TaskManager.h"
#include "Tasks/TaskTypes/TaskSimpleStandStill.h"
#include "Tasks/TaskTypes/TaskComplexDie.h"
#include "Tasks/TaskTypes/TaskComplexGoToPointAndStandStill.h"
#include "Tasks/TaskTypes/TaskComplexGoToPointAndStandStillTimed.h"
#include "TimeCycle.h"
#include "Localisation.h"
#include "Collision/ColStore.h"
#include "ModelIndices.h"
#include "Pools/PedPool.h"
#include "Pools/VehiclePool.h"
#include "Checkpoint.h"
#include "CutsceneMgr.h"
#include "Explosion.h"
#include "FireManager.h"
#include "Timer.h"
#include "VehicleRecording.h"
#include "Wanted.h"
#include "Entity/Object/Object.h"
#include "Tasks/TaskSequences.h"
#include "Tasks/TaskTypes/TaskComplexFollowNodeRoute.h"
#include "Tasks/TaskTypes/TaskComplexFollowPointRoute.h"
#include "Tasks/TaskTypes/TaskComplexGoToPointAnyMeans.h"
#include "Tasks/TaskTypes/TaskSimpleAchieveHeading.h"
#include "Tasks/TaskTypes/TaskSimpleDuckToggle.h"
#include "Tasks/TaskTypes/TaskSimplePause.h"
#include "Tasks/TaskTypes/TaskSimpleSetStayInSamePlace.h"

/*!
* Various game related commands
*/

namespace {
void SetAllTaxisHaveNitro(bool enabled) {
    (enabled ? CCheat::ApplyCheat : CCheat::Disable)(CHEAT_ALL_TAXIS_NITRO);
}

void ActivatePimpCheat(bool enabled) {
    (enabled ? CCheat::ApplyCheat : CCheat::Disable)(CHEAT_PROSTITUTES_PAY_YOU);
}

bool AreAnyCarCheatsActivated() {
    return rng::any_of(std::array{ CHEAT_PERFECT_HANDLING, CHEAT_CARS_ON_WATER, CHEAT_BOATS_FLY }, CCheat::IsActive);
}

void FailCurrentMission() {
    CTheScripts::FailCurrentMission = 2;
}

float GetGroundZFor3DCoord(CVector coord) {
    return CWorld::FindGroundZFor3DCoord(coord, nullptr, nullptr);
}

void PlayerMadeProgress(int32 progress) {
    return CStats::IncrementStat(STAT_PROGRESS_MADE, float(progress));
}

void RegisterMissionGiven() {
    CStats::IncrementStat(STAT_MISSION_ATTEMPTS);
}

void SetPedDensityMultiplier(float mult) {
    CPopulation::PedDensityMultiplier = mult;
}

bool IsJapaneseVersion() {
    return false; // TODO: make this configurable perhaps?
}

bool IsWidescreenOnInOptions() {
    return FrontEndMenuManager.m_bWidescreenOn;
}

void GetRidOfPlayerProstitute() {
    CTaskComplexProstituteSolicit::GetRidOfPlayerProstitute();
}

void SetAreaName(const char* key) {
    CHud::SetZoneName(TheText.Get(key), true);
}

void SetRadioToPlayersFavorite() {
    AudioEngine.RetuneRadio(CStats::FindMostFavoriteRadioStation());
}

void SetDisableMilitaryZones(bool disable) {
    CCullZones::bMilitaryZonesDisabled = disable;
}

void IncrementFloatStatNoMessage(eStats stat, float value) {
    CStats::IncrementStat(stat, value);
}

void TakePhoto(bool save) {
    CWeapon::ms_bTakePhoto               = true;
    CPostEffects::m_bSavePhotoFromScript = save;
}

void SetNoResprays(bool enabled) {
    CGarages::NoResprays = enabled;
    CGarages::AllRespraysCloseOrOpen(enabled);
}

void SetRespawnPointForDurationOfMission(CVector point) {
    CRestart::SetRespawnPointForDurationOfMission(point);
}

eLanguage GetCurrentLanguage() {
    return FrontEndMenuManager.m_nPrefsLanguage;
}

void DoWeaponStuffAtStartOf2PlayerGame() {
    CGameLogic::DoWeaponStuffAtStartOf2PlayerGame(true);
}

void DisplayRadar(bool enable) {
    // izzotop: CTheScripts::HideAllFrontEndMapBlips = enable;
    CHud::bScriptDontDisplayRadar = !enable;
}

void RegisterBestPosition(uint8 stat, int32 position) {
    CStats::RegisterBestPosition(static_cast<eStats>(stat), position);
}

// vvv Originally unsupported
void RegisterJumpDistance(float distance) {
    CStats::SetStatValue(STAT_MAXIMUM_INSANE_JUMP_DISTANCE, std::max(CStats::GetStatValue(STAT_MAXIMUM_INSANE_JUMP_DISTANCE), distance));
}

void RegisterJumpHeight(float height) {
    CStats::SetStatValue(STAT_MAXIMUM_INSANE_JUMP_FLIPS, std::max(CStats::GetStatValue(STAT_MAXIMUM_INSANE_JUMP_FLIPS), height));
}

void RegisterJumpSpins(float rotation) {
    CStats::SetStatValue(STAT_MAXIMUM_INSANE_JUMP_ROTATION, std::max(CStats::GetStatValue(STAT_MAXIMUM_INSANE_JUMP_ROTATION), rotation));
}

void RegisterJumpStunt(float stunt) {
    CStats::SetStatValue(STAT_BEST_INSANE_STUNT_AWARDED, std::max(CStats::GetStatValue(STAT_BEST_INSANE_STUNT_AWARDED), stunt));
}

void RegisterUniqueJumpFound() {
    CStats::IncrementStat(STAT_UNIQUE_JUMPS_FOUND);
}

void SetUniqueJumpsTotal(float amount) {
    CStats::SetStatValue(STAT_UNIQUE_JUMPS_DONE, amount);
}

void RegisterPassengerDroppedOffTaxi() {
    CStats::IncrementStat(STAT_PASSENGERS_DROPPED_OFF);
}

void RegisterMoneyMadeTaxi(float amount) {
    CStats::SetStatValue(STAT_CASH_MADE_IN_A_TAXI, amount);
}

/// IS_EXPLOSION_IN_AREA(0356)
bool IsExplosionInArea(eExplosionType type, CVector a, CVector b) {
    return CExplosion::TestForExplosionInArea(
        type,
        std::min(a.x, b.x),
        std::max(a.x, b.x),
        std::min(a.y, b.y),
        std::max(a.y, b.y),
        std::min(a.z, b.z),
        std::max(a.z, b.z)
    );
}

/// SHAKE_PAD(015B)
void ShakePad(uint32 padId, int time, int intensity) {
    CPad::GetPad(padId)->StartShake(time, intensity, 0);
}

/// DRAW_SHADOW(016F)
void DrawShadow(eShadowTextureType type, CVector coors, float angle, float length, int16 intensity, uint8 r, uint8 g, uint8 b) {
    const float x = sin(angle) * length;
    const float y = cos(angle) * length;
    CShadows::StoreShadowToBeRendered(type, coors, -x, y, y, x, intensity, r, g, b);
}


/// IS_XBOX_VERSION(0A49)
bool IsXboxVersion() {
    #if BUILD_XBOX
        return true;
    #else
        return false;
    #endif
}

// DO_DEBUG_STUFF(0A4E) - PC
// DO_DEBUG_STUFF(0A50) - Xbox
void DoDebugStuff() {
    CTheScripts::UnknownDebugStuff++;
}

//
// Onscreen timers/counters
//

/// CLEAR_ONSCREEN_COUNTER(0151)
void ClearOnscreenCounter(CRunningScript& S) {
    CUserDisplay::OnscnTimer.ClearCounter(S.GetIndexOfGlobalVariable());
}

/// DISPLAY_NTH_ONSCREEN_COUNTER_WITH_STRING(04F7)
void DisplayNthOnscreenCounterWithString(CRunningScript& S) {
    const auto varId = S.GetIndexOfGlobalVariable();

    S.CollectParameters(2);
    const auto type  = static_cast<eOnscreenCounter>(ScriptParams[0].iParam & 0xFFFF);
    const auto slot  = ScriptParams[1].iParam;

    char label[SHORT_STRING_SIZE + 1]{};
    S.ReadTextLabelFromScript(label, SHORT_STRING_SIZE);

    (void)TheText.Get(label); // Original calls this just for the side effects
    CUserDisplay::OnscnTimer.AddCounter(varId, type, label, static_cast<uint16>(slot - 1));
}

//
// Blips
//

/// REMOVE_BLIP(0164)
void RemoveBlip(tBlipHandle blip) {
    CRadar::ClearBlip(blip);
}

/// CHANGE_BLIP_DISPLAY(018B)
void ChangeBlipDisplay(tBlipHandle blip, int32 display) {
    CRadar::ChangeBlipDisplay(blip, static_cast<eBlipDisplay>(display));
}

/// DOES_BLIP_EXIST(075C)
bool DoesBlipExist(int32 blip) {
    return CRadar::GetActualBlipArrayIndex(static_cast<tBlipHandle>(blip)) != -1;
}

/// ADD_BLIP_FOR_COORD(018A)
tBlipHandle AddBlipForCoord(CRunningScript& S, float x, float y, float z) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y);
    }
    const auto blip = CRadar::SetCoordBlip(BLIP_COORD, posn, BLIP_COLOUR_REDCOPY, BLIP_DISPLAY_BOTH, S.m_szName);
    CRadar::ChangeBlipScale(blip, 3);
    return blip;
}

/// ADD_SPRITE_BLIP_FOR_CONTACT_POINT(02A7)
tBlipHandle AddSpriteBlipForContactPoint(CRunningScript& S, float x, float y, float z, int32 sprite) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y);
    }
    const auto blip = CRadar::SetCoordBlip(BLIP_CONTACT_POINT, posn, BLIP_COLOUR_BLUE, BLIP_DISPLAY_BOTH, S.m_szName);
    CRadar::SetBlipSprite(blip, static_cast<eRadarSprite>(sprite));
    return blip;
}

/// ADD_SHORT_RANGE_SPRITE_BLIP_FOR_COORD(04CE)
tBlipHandle AddShortRangeSpriteBlipForCoord(CRunningScript& S, float x, float y, float z, int32 sprite) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y);
    }
    const auto blip = CRadar::SetShortRangeCoordBlip(BLIP_COORD, posn, BLIP_COLOUR_REDCOPY, BLIP_DISPLAY_BOTH, S.m_szName);
    CRadar::SetBlipSprite(blip, static_cast<eRadarSprite>(sprite));
    return blip;
}

/// ADD_SHORT_RANGE_SPRITE_BLIP_FOR_CONTACT_POINT(0570)
tBlipHandle AddShortRangeSpriteBlipForContactPoint(CRunningScript& S, float x, float y, float z, int32 sprite) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y);
    }
    const auto blip = CRadar::SetShortRangeCoordBlip(BLIP_CONTACT_POINT, posn, BLIP_COLOUR_BLUE, BLIP_DISPLAY_BOTH, S.m_szName);
    CRadar::SetBlipSprite(blip, static_cast<eRadarSprite>(sprite));
    return blip;
}

//
// Camera, weather & world
//

/// SET_FADING_COLOUR(0169)
void SetFadingColour(int32 r, int32 g, int32 b) {
    TheCamera.SetFadeColour(static_cast<uint8>(r), static_cast<uint8>(g), static_cast<uint8>(b));
}

/// SWITCH_WIDESCREEN(02A3)
void SwitchWidescreen(bool on) {
    if (on) {
        TheCamera.SetWideScreenOn();
    } else {
        TheCamera.SetWideScreenOff();
    }
}

/// FORCE_WEATHER_NOW(01B6)
void ForceWeatherNow(int32 type) {
    CWeather::ForceWeatherNow(static_cast<eWeatherType>(type & 0xFFFF));
}

/// RELEASE_WEATHER(01B7)
void ReleaseWeather() {
    CWeather::ReleaseWeather();
}

/// SWITCH_WORLD_PROCESSING(03B7)
void SwitchWorldProcessing(bool state) {
    CWorld::bProcessCutsceneOnly = !state;
}

/// CLEAR_AREA(0395)
void ClearArea(float x, float y, float z, float radius, int32 flags) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y);
    }
    CWorld::ClearExcitingStuffFromArea(posn, radius, static_cast<uint8>(flags));
}

/// SET_AREA_VISIBLE(04BB)
void SetAreaVisible(int32 area) {
    CGame::currArea = static_cast<eAreaCodes>(area);
    CStreaming::RemoveBuildingsNotInArea(static_cast<eAreaCodes>(area));
}

/// GET_AREA_VISIBLE(077E)
int32 GetAreaVisible() {
    return static_cast<int32>(CGame::currArea);
}

/// REQUEST_COLLISION(04E4)
void RequestCollision(float x, float y) {
    CColStore::RequestCollision(CVector{x, y, 0.0f}, static_cast<eAreaCodes>(CGame::currArea));
}

//
// Player & stats
//

/// DECLARE_MISSION_FLAG(0180)
void DeclareMissionFlag(CRunningScript& S) {
    S.GetAtIPAs<uint8>(); // Parameter type - Unused by the original
    CTheScripts::OnAMissionFlag = S.GetAtIPAs<uint16>();
}

/// STORE_WANTED_LEVEL(01C0)
int32 StoreWantedLevel(CRunningScript& S) {
    // The original collects a (unused) parameter, then stores the player's wanted level into it
    S.CollectParameters(1);

    const auto player = FindPlayerPed();
    return player ? static_cast<int32>(player->GetWantedLevel()) : 0;
}

/// SQRT(01FB)
float Sqrt(float value) {
    return std::sqrt(value);
}

/// SET_PROGRESS_TOTAL(030D)
void SetProgressTotal(int32 total) {
    CStats::SetStatValue(STAT_TOTAL_PROGRESS, static_cast<float>(total));
}

/// GET_PROGRESS_PERCENTAGE(058C)
float GetProgressPercentage() {
    return CStats::GetPercentageProgress();
}

/// SET_TOTAL_NUMBER_OF_MISSIONS(042C)
void SetTotalNumberOfMissions(int32 num) {
    CStats::SetStatValue(STAT_TOTAL_NUMBER_OF_MISSIONS_IN_GAME, static_cast<float>(num));
}

/// SET_MISSION_RESPECT_TOTAL(0997)
void SetMissionRespectTotal(int32 num) {
    CStats::SetStatValue(STAT_RESPECT_MISSION_TOTAL, static_cast<float>(num));
}

/// FIND_MAX_NUMBER_OF_GROUP_MEMBERS(0956)
int32 FindMaxNumberOfGroupMembers() {
    return CStats::FindMaxNumberOfGroupMembers();
}

/// ARE_MEASUREMENTS_IN_METRES(0424)
bool AreMeasurementsInMetres() {
    return CLocalisation::Metric();
}

/// CONVERT_METRES_TO_FEET(0425)
float ConvertMetresToFeet(float metres) {
    return metres * 3.333333f;
}

/// CONVERT_METRES_TO_FEET_INT(042D)
int32 ConvertMetresToFeetInt(int32 metres) {
    // The original uses the x87 `FISTP` (round-to-nearest) here.
    return static_cast<int32>(std::nearbyint(static_cast<float>(metres) * 3.333333f));
}

/// GET_POSITION_OF_ANALOGUE_STICKS(0494)
notsa::script::MultiRet<int32, int32, int32, int32> GetPositionOfAnalogueSticks(int32 pad) {
    const auto& state = CPad::GetPad(pad)->NewState;
    return notsa::script::return_multiple(
        static_cast<int32>(state.LeftStickX),
        static_cast<int32>(state.LeftStickY),
        static_cast<int32>(state.RightStickX),
        static_cast<int32>(state.RightStickY)
    );
}

/// IS_INT_VAR_EQUAL_TO_CONSTANT(04A3), IS_INT_LVAR_EQUAL_TO_CONSTANT(04A4), IS_INT_LVAR_EQUAL_TO_INT_VAR(07D6)
bool IsIntEqual(int32 lhs, int32 rhs) {
    return lhs == rhs;
}

//
// Pickups
//

/// CREATE_PICKUP(0213)
int32 CreatePickup(notsa::script::Model model, int32 pickupType, float x, float y, float z) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y) + 0.5f;
    }
    return CPickups::GenerateNewOne(posn, static_cast<uint32>(model.value), static_cast<ePickupType>(pickupType & 0xFF), 0u).num;
}

/// CREATE_PICKUP_WITH_AMMO(032B)
int32 CreatePickupWithAmmo(notsa::script::Model model, int32 pickupType, int32 ammo, float x, float y, float z) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y) + 0.5f;
    }
    return CPickups::GenerateNewOne(posn, static_cast<uint32>(model.value), static_cast<ePickupType>(pickupType & 0xFF), static_cast<uint32>(ammo)).num;
}

/// CREATE_PROTECTION_PICKUP(04A6)
int32 CreateProtectionPickup(float x, float y, float z, int32 revenueLimit, int32 revenueRate) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y) + 0.5f;
    }
    return CPickups::GenerateNewOne(posn, ModelIndices::MI_PICKUP_REVENUE, PICKUP_ASSET_REVENUE, static_cast<uint32>(revenueLimit), static_cast<uint32>(revenueRate)).num;
}

/// CREATE_LOCKED_PROPERTY_PICKUP(0517)
int32 CreateLockedPropertyPickup(float x, float y, float z, const char* message) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y) + 0.5f;
    }
    (void)TheText.Get(message);
    return CPickups::GenerateNewOne(posn, ModelIndices::MI_PICKUP_PROPERTY, PICKUP_PROPERTY_LOCKED, 0u, 0u, false, const_cast<char*>(message)).num;
}

/// CREATE_FORSALE_PROPERTY_PICKUP(0518)
int32 CreateForSalePropertyPickup(float x, float y, float z, int32 price, const char* message) {
    CVector posn{x, y, z};
    if (z <= -100.0f) {
        posn.z = CWorld::FindGroundZForCoord(x, y) + 0.5f;
    }
    (void)TheText.Get(message);
    return CPickups::GenerateNewOne(posn, ModelIndices::MI_PICKUP_PROPERTY_FORSALE, PICKUP_PROPERTY_FORSALE, static_cast<uint32>(price), 0u, false, const_cast<char*>(message)).num;
}

/// HAS_PICKUP_BEEN_COLLECTED(0214)
bool HasPickupBeenCollected(int32 pickup) {
    tPickupReference ref{};
    ref.num = pickup;
    return CPickups::IsPickUpPickedUp(ref);
}

/// REMOVE_PICKUP(0215)
void RemovePickup(int32 pickup) {
    tPickupReference ref{};
    ref.num = pickup;
    CPickups::RemovePickUp(ref);
}

/// UPDATE_PICKUP_MONEY_PER_DAY(094A)
void UpdatePickupMoneyPerDay(int32 pickup, int32 money) {
    tPickupReference ref{};
    ref.num = pickup;
    CPickups::UpdateMoneyPerDay(ref, static_cast<uint16>(money));
}

//
// Gangs & garages
//

/// SET_GANG_WEAPONS(0237)
void SetGangWeapons(int32 gang, int32 weapon1, int32 weapon2, int32 weapon3) {
    CGangs::SetGangWeapons(
        static_cast<int16>(gang & 0xFFFF),
        static_cast<eWeaponType>(weapon1),
        static_cast<eWeaponType>(weapon2),
        static_cast<eWeaponType>(weapon3)
    );
}

/// ACTIVATE_GARAGE(0299)
void ActivateGarage(const char* name) {
    const auto garageId = CGarages::GetGarageNumberByName(name);
    if (garageId >= 0) {
        CGarages::ActivateGarage(garageId);
    }
}

/// DEACTIVATE_GARAGE(02B9)
void DeactivateGarage(const char* name) {
    const auto garageId = CGarages::GetGarageNumberByName(name);
    if (garageId >= 0) {
        CGarages::DeActivateGarage(garageId);
    }
}

/// CLOSE_GARAGE(0361)
void CloseGarage(const char* name) {
    const auto garageId = CGarages::GetGarageNumberByName(name);
    if (garageId >= 0) {
        CGarages::GetGarage(garageId).CloseThisGarage();
    }
}

/// CHANGE_GARAGE_TYPE(02FA)
void ChangeGarageType(const char* name, int32 type) {
    const auto garageId = CGarages::GetGarageNumberByName(name);
    if (garageId >= 0) {
        CGarages::ChangeGarageType(garageId, static_cast<eGarageType>(type & 0xFF), 0);
    }
}

/// IS_GARAGE_OPEN(03B0)
bool IsGarageOpen(const char* name) {
    const auto garageId = CGarages::GetGarageNumberByName(name);
    return garageId >= 0 && CGarages::IsGarageOpen(garageId);
}

/// SET_GARAGE_RESPRAY_FREE(093A)
void SetGarageResprayFree(const char* name, bool state) {
    const auto garageId = CGarages::GetGarageNumberByName(name);
    if (garageId < 0) {
        return;
    }
    CGarages::GetGarage(garageId).m_bRespraysAlwaysFree = state;
}

//
// Audio
//

/// PLAY_MISSION_PASSED_TUNE(0394)
void PlayMissionPassedTune(int32 tune) {
    if (tune == 1 || tune == 2) {
        AudioEngine.PreloadBeatTrack(static_cast<int16>(tune + 10));
        AudioEngine.PlayPreloadedBeatTrack(true);
    }
}

/// HAS_MISSION_AUDIO_LOADED(03D0)
bool HasMissionAudioLoaded(int32 slot) {
    return AudioEngine.GetMissionAudioLoadingStatus(static_cast<uint8>(slot - 1)) == 1;
}

/// HAS_MISSION_AUDIO_FINISHED(03D2)
bool HasMissionAudioFinished(int32 slot) {
    return AudioEngine.IsMissionAudioSampleFinished(static_cast<uint8>(slot - 1));
}

/// CLEAR_MISSION_AUDIO(040D)
void ClearMissionAudio(int32 slot) {
    AudioEngine.ClearMissionAudio(static_cast<uint8>(slot - 1));
}

/// REPORT_MISSION_AUDIO_EVENT_AT_POSITION(097A)
void ReportMissionAudioEventAtPosition(float x, float y, float z, int32 eventId) {
    CVector posn{x, y, z};
    AudioEngine.ReportMissionAudioEvent(static_cast<uint16>(eventId & 0xFFFF), posn);
}

/// STOP_BEAT_TRACK(0955)
void StopBeatTrack() {
    AudioEngine.StopBeatTrack();
}

/// GET_BEAT_PROXIMITY(07B1)
auto GetBeatProximity(int32 offset) {
    const auto* beatInfo = AudioEngine.GetBeatInfo();
    if (!beatInfo || !beatInfo->IsBeatInfoPresent) {
        return notsa::script::return_multiple(-1, -1, -1);
    }

    if (offset == 0) {
        // Current beat - See the original assembly (Uses the beat before the current one if it's closer)
        const auto& current = beatInfo->BeatWindow[10];
        const auto& previous = beatInfo->BeatWindow[9];
        if (current.m_nTime != 0
            && beatInfo->BeatNumber != 0
            && std::abs(static_cast<int32>(previous.m_nTime)) < std::abs(static_cast<int32>(current.m_nTime))
        ) {
            return notsa::script::return_multiple(
                static_cast<int32>(previous.m_nTime),
                static_cast<int32>(previous.m_nKey),
                beatInfo->BeatNumber - 1
            );
        }
        return notsa::script::return_multiple(
            static_cast<int32>(current.m_nTime),
            static_cast<int32>(current.m_nKey),
            static_cast<int32>(beatInfo->BeatNumber)
        );
    }

    const auto idx = offset < 0 ? offset + 10 : offset + 9;
    const auto& beat = beatInfo->BeatWindow[idx];
    return notsa::script::return_multiple(
        static_cast<int32>(beat.m_nTime),
        static_cast<int32>(beat.m_nKey),
        static_cast<int32>(beatInfo->BeatNumber)
    );
}

//
// Save menu (The front-end handles the rest of the save flow)
//

/// ACTIVATE_SAVE_MENU(03D8)
void ActivateSaveMenu() {
    const auto player = FindPlayerPed();
    if (!player || player->bInVehicle) {
        return;
    }

    FrontEndMenuManager.m_bIsSaveDone = true;
    player->SetVelocity(CVector{});
    player->ResetTurnSpeed();
}

/// HAS_SAVE_GAME_FINISHED(03D9)
bool HasSaveGameFinished() {
    return !FrontEndMenuManager.m_bMenuActive && !FrontEndMenuManager.m_bIsSaveDone;
}

//
// Animations
//

/// REQUEST_ANIMATION(04ED)
void RequestAnimation(CRunningScript& S, const char* name) {
    const auto animBlockId = CAnimManager::GetAnimationBlockIndex(name);
    CStreaming::RequestModel(IFPToModelId(animBlockId), 4);
    CTheScripts::ScriptResourceManager.AddToResourceManager(animBlockId, RESOURCE_TYPE_ANIMATION, &S);
}

/// HAS_ANIMATION_LOADED(04EE)
bool HasAnimationLoaded(const char* name) {
    const auto animBlock = CAnimManager::GetAnimationBlock(name);
    return animBlock && animBlock->IsLoaded;
}

/// REMOVE_ANIMATION(04EF)
void RemoveAnimation(CRunningScript& S, const char* name) {
    const auto animBlockId = CAnimManager::GetAnimationBlockIndex(name);
    if (CTheScripts::ScriptResourceManager.RemoveFromResourceManager(animBlockId, RESOURCE_TYPE_ANIMATION, &S)) {
        CStreaming::SetMissionDoesntRequireModel(IFPToModelId(animBlockId));
    }
}

/// ATTACH_ANIMS_TO_MODEL(08E8)
void AttachAnimsToModel(notsa::script::Model model, const char* name) {
    const auto modelId = static_cast<int32>(model.value);
    CTheScripts::ScriptAttachAnimGroupToCharModel(modelId, name);
    CTheScripts::AddToListOfSpecialAnimGroupsAttachedToCharModels(modelId, name);
}

//
// Extra colours
//

/// SET_EXTRA_COLOURS(04F9)
void SetExtraColours(int32 colour, bool fade) {
    CTimeCycle::StartExtraColour(colour - 1, fade);
}

/// CLEAR_EXTRA_COLOURS(04FA)
void ClearExtraColours(bool withFade) {
    CTimeCycle::StopExtraColour(withFade);
}

//
// Misc entity checks
//

/// GET_ANGLE_BETWEEN_2D_VECTORS(05A4)
float GetAngleBetween2DVectors(float x1, float y1, float x2, float y2) {
    const auto dot  = x1 * x2 + y1 * y2;
    const auto len1 = std::sqrt(x1 * x1 + y1 * y1);
    const auto len2 = std::sqrt(x2 * x2 + y2 * y2);
    const auto denom = len1 * len2;
    if (denom == 0.0f) {
        return 0.0f;
    }
    // NOTE: Clamped to avoid NaNs caused by floating point error (The original doesn't clamp)
    return std::acos(std::clamp(dot / denom, -1.0f, 1.0f)) * (180.0f / PI);
}

//
// Tasks
//

/// TASK_STAND_STILL(05BA)
void TaskStandStill(CRunningScript& S, eScriptCommands opcode, CPed* ped, int32 time) {
    CTask* task;
    if (time == -1) {
        task = new CTaskSimpleStandStill(20000, false, false, 8.0f);
    } else if (time == -2) {
        task = new CTaskSimpleStandStill(999999, true, false, 8.0f);
    } else {
        task = new CTaskSimpleStandStill(time, false, false, 8.0f);
    }
    S.GivePedScriptedTask(ped, task, opcode);
}

/// TASK_GO_STRAIGHT_TO_COORD(05D3)
void TaskGoStraightToCoord(CRunningScript& S, eScriptCommands opcode, CPed* ped, CVector target, int32 speed, int32 time) {
    const auto moveState = static_cast<eMoveState>(speed);
    CTask* task;
    if (time == -2) {
        task = new CTaskComplexGoToPointAndStandStill(moveState, target, 0.5f, 2.0f, false, true);
    } else {
        task = new CTaskComplexGoToPointAndStandStillTimed(moveState, target, 0.5f, 2.0f, time == -1 ? 20000 : time);
    }
    S.GivePedScriptedTask(ped, task, opcode);
}

/// TASK_USE_MOBILE_PHONE(0729)
void TaskUseMobilePhone(CRunningScript& S, eScriptCommands opcode, CPed* ped, bool start) {
    if (start) {
        // TODO(re): `CTaskComplexUseMobilePhone` is an unimplemented stub (No ctor, no overrides => abstract),
        // so the phone task can't be created here yet. Once it's implemented the original does:
        // `GivePedScriptedTask(ped, new CTaskComplexUseMobilePhone(-1), opcode)`
        return;
    }

    if (!ped) {
        return;
    }

    auto* task = ped->GetTaskManager().FindTaskByType(TASK_PRIMARY_PRIMARY, TASK_COMPLEX_USE_MOBILE_PHONE);
    if (task && task->GetTaskType() == TASK_COMPLEX_USE_MOBILE_PHONE) {
        task->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
    }
}

/// TASK_DEAD(0762)
void TaskDead(CRunningScript& S, eScriptCommands opcode, CPed* ped) {
    S.GivePedScriptedTask(
        ped,
        new CTaskComplexDie(WEAPON_UNARMED, ANIM_GROUP_DEFAULT, static_cast<AnimationId>(0xF), 4.0f),
        opcode
    );
}

/// TASK_PLAY_ANIM(0605), TASK_PLAY_ANIM_NON_INTERRUPTABLE(0812), TASK_PLAY_ANIM_WITH_FLAGS(088A), TASK_PLAY_ANIM_SECONDARY(0A1A)
/// The amount and meaning of the parameters depends on the opcode, so `PlayAnimScriptCommand` handles them all
void TaskPlayAnim(CRunningScript& S, eScriptCommands opcode) {
    S.PlayAnimScriptCommand(opcode);
}

//
// Groups & decision makers
//

/// REMOVE_GROUP(0632)
void RemoveGroup(CRunningScript& S, int32 groupRef) {
    const auto groupId = CTheScripts::GetActualScriptThingIndex(groupRef, SCRIPT_THING_PED_GROUP);
    if (groupId >= 0 && groupId < (int32)std::size(CPedGroups::ms_activeGroups)) {
        if (CPedGroups::ms_activeGroups[groupId]) {
            const auto leader = CPedGroups::GetGroup(groupId).GetMembership().GetLeader();
            if (leader && leader->IsPlayer()) {
                return; // Can't remove the player's group
            }
        }
        CPedGroups::RemoveGroup(groupId);
    }

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(groupRef, MISSION_CLEANUP_ENTITY_TYPE_GROUP);
    }
}

/// REMOVE_DECISION_MAKER(065C)
void RemoveDecisionMaker(CRunningScript& S, int32 decisionMakerRef) {
    const auto decisionMakerId = CTheScripts::GetActualScriptThingIndex(decisionMakerRef, SCRIPT_THING_DECISION_MAKER);
    if (decisionMakerId >= 0
        && decisionMakerId < (int32)CDecisionMakerTypes::NUM_TYPES
        && CTheScripts::ScriptResourceManager.RemoveFromResourceManager(decisionMakerRef, RESOURCE_TYPE_DECISION_MAKER, &S)
    ) {
        CDecisionMakerTypes::GetInstance()->RemoveDecisionMaker(static_cast<eDecisionTypes>(decisionMakerId));
    }

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(decisionMakerRef, MISSION_CLEANUP_ENTITY_TYPE_DECISION_MAKER);
    }
}

/// DOES_DECISION_MAKER_EXIST(09F2)
bool DoesDecisionMakerExist(int32 decisionMakerRef) {
    const auto decisionMakerId = CTheScripts::GetActualScriptThingIndex(decisionMakerRef, SCRIPT_THING_DECISION_MAKER);
    return decisionMakerId >= 0
        && decisionMakerId < (int32)CDecisionMakerTypes::NUM_TYPES
        && CDecisionMakerTypes::m_IsActive[decisionMakerId];
}

/// SET_RELATIONSHIP(0746)
void SetRelationship(int32 relationship, int32 ofPedType, int32 toPedType) {
    CPedType::SetPedTypeAsAcquaintance(
        static_cast<AcquaintanceId>(relationship),
        static_cast<ePedType>(ofPedType),
        static_cast<int32>(CPedType::GetPedFlag(static_cast<ePedType>(toPedType)))
    );
}

//
// Messages & help
//

/// IS_MESSAGE_BEING_DISPLAYED(076F)
bool IsMessageBeingDisplayed() {
    return CMessages::BriefMessages[0].IsValid();
}

/// IS_HELP_MESSAGE_BEING_DISPLAYED(08FE)
bool IsHelpMessageBeingDisplayed() {
    return CHud::HelpMessageDisplayed();
}

/// IS_THIS_HELP_MESSAGE_BEING_DISPLAYED(0A2A)
bool IsThisHelpMessageBeingDisplayed(const char* gxtKey) {
    if (!CHud::HelpMessageDisplayed()) {
        return false;
    }

    GxtChar buffer[400];
    CMessages::StringCopy(buffer, TheText.Get(gxtKey), (uint16)std::size(buffer));
    CMessages::InsertPlayerControlKeysInString(buffer);
    return CMessages::StringCompare(buffer, CHud::m_pHelpMessageToPrint, static_cast<uint16>(CMessages::GetStringLength(buffer)));
}

/// FORCE_BIG_MESSAGE_AND_COUNTER(09EE)
void ForceBigMessageAndCounter(bool state) {
    CHud::bScriptForceDisplayWithCounters = state;
}

/// IS_MINIGAME_IN_PROGRESS(09BE)
bool IsMinigameInProgress() {
    return CTheScripts::bMiniGameInProgress;
}

//
// IPL, zones & tags
//

/// REQUEST_IPL(0776)
void RequestIpl(const char* name) {
    CIplStore::RequestIplAndIgnore(CIplStore::FindIplSlot(name));
}

/// NOTE: `CShopping::ms_shopLoaded` is private, so it's referenced by it's original address here
auto& ShopLoadedName = StaticRef<char[24]>(0xA9A7D8);

/// GET_LOADED_SHOP(07B0)
void GetLoadedShop(scm::StringRef dst) {
    char buffer[9]{};
    strncpy(buffer, ShopLoadedName, 8);
    MakeUpperCase(buffer);
    strncpy(dst.Data, buffer, std::min<size_t>(8, dst.Cap));
}

/// FIND_NUMBER_TAGS_TAGGED(08E1)
int32 FindNumberTagsTagged() {
    return CTagManager::ms_numTagged;
}

/// GET_TERRITORY_UNDER_CONTROL_PERCENTAGE(08E2)
int32 GetTerritoryUnderControlPercentage() {
    return static_cast<int32>(CGangWars::TerritoryUnderControlPercentage);
}

/// IS_GANG_WAR_FIGHTING_GOING_ON(0A03)
bool IsGangWarFightingGoingOn() {
    return CGangWars::GangWarFightingGoingOn();
}

/// GET_CURRENT_DAY_OF_WEEK(07D0)
int32 GetCurrentDayOfWeek() {
    return CClock::CurrentDay;
}

//
// Models
//

/// GET_MODEL_DIMENSIONS(07E4)
auto GetModelDimensions(notsa::script::Model model) {
    const auto modelInfo = CModelInfo::GetModelInfo(static_cast<int32>(model.value));
    if (!modelInfo) {
        return notsa::script::return_multiple(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    const auto& box = modelInfo->GetColModel()->GetBoundingBox();
    return notsa::script::return_multiple(
        static_cast<float>(box.m_vecMin.x), static_cast<float>(box.m_vecMin.y), static_cast<float>(box.m_vecMin.z),
        static_cast<float>(box.m_vecMax.x), static_cast<float>(box.m_vecMax.y), static_cast<float>(box.m_vecMax.z)
    );
}

//
// Script brains & streamed scripts
//

/// STREAM_SCRIPT(08A9)
void StreamScript(int32 scriptId) {
    const auto idx = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId & 0xFFFF));
    CStreaming::RequestModel(SCMToModelId(idx), 4);
}

/// HAS_STREAMED_SCRIPT_LOADED(08AB)
bool HasStreamedScriptLoaded(int32 scriptId) {
    const auto idx = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId & 0xFFFF));
    return CStreaming::IsModelLoaded(SCMToModelId(idx));
}

/// MARK_STREAMED_SCRIPT_AS_NO_LONGER_NEEDED(090F)
void MarkStreamedScriptAsNoLongerNeeded(int32 scriptId) {
    const auto idx = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId & 0xFFFF));
    CStreaming::SetMissionDoesntRequireModel(SCMToModelId(idx));
}

/// START_NEW_STREAMED_SCRIPT(0913)
void StartNewStreamedScript(CRunningScript& S, int32 scriptId) {
    const auto idx = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId & 0xFFFF));
    S.ReadParametersForNewlyStartedScript(CTheScripts::StreamedScripts.StartNewStreamedScript(idx));
}

/// GET_NUMBER_OF_INSTANCES_OF_STREAMED_SCRIPT(0926)
int32 GetNumberOfInstancesOfStreamedScript(int32 scriptId) {
    const auto idx = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId & 0xFFFF));
    if (idx < 0) {
        return 0;
    }
    return CTheScripts::StreamedScripts.m_aScripts[idx].m_NumberOfUsers;
}

/// ALLOCATE_STREAMED_SCRIPT_TO_RANDOM_PED(0928)
void AllocateStreamedScriptToRandomPed(int32 scriptId, int32 modelId, int32 priority) {
    const auto idx = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId & 0xFFFF));
    CTheScripts::ScriptsForBrains.AddNewScriptBrain(
        static_cast<int16>(idx),
        static_cast<int16>(modelId & 0xFFFF),
        static_cast<uint16>(priority & 0xFFFF),
        0,
        -1,
        -1.0f
    );
}

/// REGISTER_SCRIPT_BRAIN_FOR_CODE_USE(07D3)
void RegisterScriptBrainForCodeUse(int32 scriptId, const char* name) {
    const auto idx = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId & 0xFFFF));
    CTheScripts::ScriptsForBrains.AddNewStreamedScriptBrainForCodeUse(static_cast<int16>(idx), const_cast<char*>(name), 3);
}

/// REGISTER_ATTRACTOR_SCRIPT_BRAIN_FOR_CODE_USE(0884)
void RegisterAttractorScriptBrainForCodeUse(int32 scriptId, const char* name) {
    const auto idx = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId & 0xFFFF));
    CTheScripts::ScriptsForBrains.AddNewStreamedScriptBrainForCodeUse(static_cast<int16>(idx), const_cast<char*>(name), 5);
}

//
// Global variable bits
//

/// IS_GLOBAL_VAR_BIT_SET_CONST(08B4), IS_GLOBAL_VAR_BIT_SET_VAR(08B5), IS_GLOBAL_VAR_BIT_SET_LVAR(08B6)
void IsGlobalVarBitSet(CRunningScript& S) {
    const auto var = reinterpret_cast<uint32*>(S.GetPointerToScriptVariable(VAR_GLOBAL));

    S.CollectParameters(1);
    const auto bit = ScriptParams[0].iParam & 0x1F;

    S.UpdateCompareFlag((*var >> bit) & 1);
}

/// SET_GLOBAL_VAR_BIT_CONST(08BA), SET_GLOBAL_VAR_BIT_VAR(08BB), SET_GLOBAL_VAR_BIT_LVAR(08BC)
void SetGlobalVarBit(CRunningScript& S) {
    const auto var = reinterpret_cast<uint32*>(S.GetPointerToScriptVariable(VAR_GLOBAL));

    S.CollectParameters(1);
    const auto bit = ScriptParams[0].iParam & 0x1F;

    *var |= 1u << bit;
}

/// CLEAR_GLOBAL_VAR_BIT_CONST(08C0), CLEAR_GLOBAL_VAR_BIT_VAR(08C1), CLEAR_GLOBAL_VAR_BIT_LVAR(08C2)
void ClearGlobalVarBit(CRunningScript& S) {
    const auto var = reinterpret_cast<uint32*>(S.GetPointerToScriptVariable(VAR_GLOBAL));

    S.CollectParameters(1);
    const auto bit = ScriptParams[0].iParam & 0x1F;

    *var &= ~(1u << bit);
}

//
// Switch statements (The number of parameters is variable, so these must be read manually)
//

/// SWITCH_START(0871), SWITCH_CONTINUED(0872)
void Switch(CRunningScript& S, eScriptCommands opcode) {
    int32 numToRead;
    if (opcode == COMMAND_SWITCH_START) {
        S.CollectParameters(4);
        CTheScripts::ValueToCheckInSwitchStatement = ScriptParams[0].iParam;
        CTheScripts::NumberOfEntriesStillToReadForSwitch = static_cast<uint16>(ScriptParams[1].iParam * 2);
        CTheScripts::SwitchDefaultExists = ScriptParams[2].iParam != 0;
        CTheScripts::SwitchDefaultAddress = ScriptParams[3].iParam;
        numToRead = 14;
    } else {
        numToRead = 18;
    }

    S.CollectParameters(static_cast<int16>(numToRead));

    auto remaining = static_cast<int32>(CTheScripts::NumberOfEntriesStillToReadForSwitch);
    if (remaining > numToRead) {
        for (auto i = 0; i < numToRead; i += 2) {
            CTheScripts::AddToSwitchJumpTable(ScriptParams[i].iParam, ScriptParams[i + 1].iParam);
        }

        remaining -= numToRead;
        CTheScripts::NumberOfEntriesStillToReadForSwitch = static_cast<uint16>(remaining);
        if (remaining != 0) {
            return; // There are more entries left to read - The next `SWITCH_CONTINUED` handles them
        }
    } else {
        for (auto i = 0; i < remaining; i += 2) {
            CTheScripts::AddToSwitchJumpTable(ScriptParams[i].iParam, ScriptParams[i + 1].iParam);
        }
        CTheScripts::NumberOfEntriesStillToReadForSwitch = 0;
    }

    int32 offset = 0;
    CTheScripts::UseSwitchJumpTable(offset);
    S.UpdatePC(offset);
}

//
// Entry exits
//

/// SWITCH_ENTRY_EXIT(07FB)
void SwitchEntryExit(const char* name, bool state) {
    auto* pool = CEntryExitManager::GetPool();
    for (auto i = 0; i < (int32)pool->GetSize(); i++) {
        const auto entryExit = CEntryExitManager::GetInSlot(i);
        if (entryExit && !strncmp(entryExit->m_szName, name, sizeof(entryExit->m_szName))) {
            if (state) {
                entryExit->m_nFlags |= CEntryExit::ENABLE_ACCESS;
            } else {
                entryExit->m_nFlags &= ~CEntryExit::ENABLE_ACCESS;
            }
        }
    }
}

/// SET_BLIP_ENTRY_EXIT(08DC)
void SetBlipEntryExit(tBlipHandle blip, float x, float y, float radius) {
    const auto idx = CEntryExitManager::FindNearestEntryExit(CVector2D{x, y}, radius, -1);
    if (idx >= 0) {
        CRadar::SetBlipEntryExit(blip, CEntryExitManager::GetInSlot(idx));
    }
}

/// DISABLE_ALL_ENTRY_EXITS(08E7)
void DisableAllEntryExits(bool disable) {
    CEntryExitManager::ms_bDisabled = disable;
}

/// SET_NAMED_ENTRY_EXIT_FLAG(098E)
void SetNamedEntryExitFlag(const char* name, int32 flag, bool state) {
    auto* pool = CEntryExitManager::GetPool();
    for (auto i = 0; i < (int32)pool->GetSize(); i++) {
        const auto entryExit = CEntryExitManager::GetInSlot(i);
        if (entryExit && !strncmp(entryExit->m_szName, name, sizeof(entryExit->m_szName))) {
            if (state) {
                entryExit->m_nFlags |= static_cast<uint16>(flag);
            } else {
                entryExit->m_nFlags &= ~static_cast<uint16>(flag);
            }
        }
    }
}

/// SET_CLOSEST_ENTRY_EXIT_FLAG(09B4)
void SetClosestEntryExitFlag(float x, float y, float radius, int32 flag, bool state) {
    const auto idx = CEntryExitManager::FindNearestEntryExit(CVector2D{x, y}, radius, -1);
    if (idx < 0) {
        return;
    }

    const auto entryExit = CEntryExitManager::GetInSlot(idx);
    if (!entryExit) {
        return;
    }

    if (state) {
        entryExit->m_nFlags |= static_cast<uint16>(flag);
    } else {
        entryExit->m_nFlags &= ~static_cast<uint16>(flag);
    }
}

//
// Penalties & restart points
//

/// SWITCH_DEATH_PENALTIES(08DD)
void SwitchDeathPenalties(bool state) {
    CGameLogic::bPenaltyForDeathApplies = state;
}

/// SWITCH_ARREST_PENALTIES(08DE)
void SwitchArrestPenalties(bool state) {
    CGameLogic::bPenaltyForArrestApplies = state;
}

/// SET_EXTRA_HOSPITAL_RESTART_POINT(08DF)
void SetExtraHospitalRestartPoint(float x, float y, float z, float radius, float heading) {
    CRestart::ExtraHospitalRestartCoors   = CVector{x, y, z};
    CRestart::ExtraHospitalRestartRadius  = radius;
    CRestart::ExtraHospitalRestartHeading = heading;
}

//
// Menus
//

/// Returns `label`, unless it's `"DUMMY"`, in which case `nullptr` (an empty menu entry) is returned
const char* MenuLabelOrNullptr(const char* label) {
    return _stricmp(label, "DUMMY") == 0 ? nullptr : label;
}

/// CREATE_MENU(08D4)
int32 CreateMenu(const char* title, float x, float y, float width, int32 numColumns, bool interactive, bool background, int32 alignment) {
    return static_cast<int32>(CMenuSystem::CreateNewMenu(
        CMenuSystem::MENU_TYPE_DEFAULT,
        MenuLabelOrNullptr(title),
        SCREEN_STRETCH_X(x),
        SCREEN_STRETCH_Y(y),
        SCREEN_STRETCH_X(width),
        static_cast<uint8>(numColumns),
        interactive,
        background,
        static_cast<eFontAlignment>(alignment)
    ));
}

/// SET_MENU_COLUMN(08DB)
void SetMenuColumn(
    int32 menu,
    int32 column,
    const char* title,
    const char* row0, const char* row1, const char* row2, const char* row3,
    const char* row4, const char* row5, const char* row6, const char* row7,
    const char* row8, const char* row9, const char* row10, const char* row11
) {
    CMenuSystem::InsertMenu(
        static_cast<MenuId>(menu),
        static_cast<uint8>(column),
        MenuLabelOrNullptr(title),
        MenuLabelOrNullptr(row0), MenuLabelOrNullptr(row1), MenuLabelOrNullptr(row2), MenuLabelOrNullptr(row3),
        MenuLabelOrNullptr(row4), MenuLabelOrNullptr(row5), MenuLabelOrNullptr(row6), MenuLabelOrNullptr(row7),
        MenuLabelOrNullptr(row8), MenuLabelOrNullptr(row9), MenuLabelOrNullptr(row10), MenuLabelOrNullptr(row11)
    );
}

/// GET_MENU_ITEM_SELECTED(08D7)
int32 GetMenuItemSelected(int32 menu) {
    return CMenuSystem::CheckForSelected(static_cast<MenuId>(menu));
}

/// DELETE_MENU(08DA)
void DeleteMenu(int32 menu) {
    CMenuSystem::SwitchOffMenu(static_cast<MenuId>(menu));
}

/// SET_ACTIVE_MENU_ITEM(090E)
void SetActiveMenuItem(int32 menu, int32 row) {
    CMenuSystem::SetActiveMenuItem(static_cast<MenuId>(menu), static_cast<int8>(row));
}

/// SET_MENU_ITEM_WITH_NUMBER(08EE)
void SetMenuItemWithNumber(int32 menu, int32 column, int32 row, const char* label, int32 number) {
    CMenuSystem::InsertOneMenuItemWithNumber(
        static_cast<MenuId>(menu),
        static_cast<uint8>(column),
        static_cast<uint8>(row),
        label,
        number,
        -1
    );
}

//
// HUD colours & 3D markers
//

/// GET_HUD_COLOUR(0904)
auto GetHudColour(int32 colour) {
    const auto rgba = HudColour.GetRGB(static_cast<eHudColours>(colour & 0xFF));
    return notsa::script::return_multiple(
        static_cast<int32>(rgba.r),
        static_cast<int32>(rgba.g),
        static_cast<int32>(rgba.b),
        static_cast<int32>(rgba.a)
    );
}

/// CREATE_USER_3D_MARKER(0A40)
int32 CreateUser3dMarker(float x, float y, float z, int32 colour) {
    return C3dMarkers::User3dMarkerSet(x, y, z, static_cast<eHudColours>(colour & 0xFF));
}

/// REMOVE_USER_3D_MARKER(0A41)
void RemoveUser3dMarker(int32 slot) {
    C3dMarkers::User3dMarkerDelete(slot);
}

//
// Vehicles (misc)
//

/// HAS_TRAIN_DERAILED(0981)
bool HasTrainDerailed(int32 handle) {
    const auto vehicle = GetVehiclePool()->GetAtRef(handle);
    // Original reads bit 8 of `CTrain::m_nTrainFlags` (0x5B9 & 1)
    return vehicle && vehicle->IsTrain() && static_cast<CTrain*>(vehicle)->trainFlags.bNotOnARailRoad;
}

/// RANDOM_PASSENGER_SAY(09AB)
void RandomPassengerSay(CVehicle* vehicle, int32 phrase) {
    if (!vehicle) {
        return;
    }
    if (auto* passenger = vehicle->PickRandomPassenger()) {
        passenger->Say(static_cast<eGlobalSpeechContext>(phrase & 0xFFFF));
    }
}

//
// Blips (misc)
//

/// CHANGE_BLIP_COLOUR(0165)
void ChangeBlipColour(tBlipHandle blip, int32 colour) {
    CRadar::ChangeBlipColour(blip, static_cast<eBlipColour>(colour));
}

/// CHANGE_BLIP_SCALE(0168)
void ChangeBlipScale(tBlipHandle blip, int32 scale) {
    CRadar::ChangeBlipScale(blip, scale);
}

/// SET_BLIP_AS_FRIENDLY(07E0)
void SetBlipAsFriendly(tBlipHandle blip, bool friendly) {
    CRadar::SetBlipFriendly(blip, friendly);
}

//
// Camera, audio & explosions (misc)
//

/// GET_FADING_STATUS(016B)
bool GetFadingStatus() {
    return TheCamera.GetFading();
}

/// SET_WANTED_MULTIPLIER(03C7)
void SetWantedMultiplier(float multiplier) {
    FindPlayerWanted()->m_Multiplier = multiplier;
}

/// SET_RADIO_CHANNEL(041E)
void SetRadioChannel(int32 channel) {
    if (channel == -1) {
        return;
    }
    if (channel == 11) { // Channel 11 doesn't exist in the radio station list, it's remapped to 12
        channel = 12;
    }
    AudioEngine.RetuneRadio(static_cast<eRadioID>(channel + 1)); // Script channels are 0-based, the engine's are 1-based
}

/// ADD_ONE_OFF_SOUND(018C)
void AddOneOffSound(CVector posn, int32 soundId) {
    AudioEngine.ReportMissionAudioEvent(static_cast<uint16>(soundId), posn);
}

/// SET_MISSION_AUDIO_POSITION(03D7)
void SetMissionAudioPosition(int32 slot, CVector posn) {
    AudioEngine.SetMissionAudioPosition(static_cast<uint8>(slot - 1), posn);
}

/// ADD_EXPLOSION(020C)
void AddExplosion(CVector posn, int32 type) {
    CExplosion::AddExplosion(nullptr, nullptr, static_cast<eExplosionType>(type), posn, 0, true, -1.0f, false);
}

//
// World state
//

/// SET_LA_RIOTS(06C8)
void SetLaRiots(bool enabled) {
    gbLARiots = enabled;
}

/// DISPLAY_HUD(0826)
void DisplayHud(bool show) {
    CTheScripts::bDisplayHud = show;
}

/// SET_MINIGAME_IN_PROGRESS(09BD)
void SetMinigameInProgress(CRunningScript& S, bool inProgress) {
    S.m_IsTextBlockOverride                      = inProgress;
    CTheScripts::bMiniGameInProgress             = inProgress;
    CTheScripts::bDisplayNonMiniGameHelpMessages = !inProgress;
}

/// SET_TIME_SCALE(015D)
void SetTimeScale(float scale) {
    CTimer::SetTimeScale(scale);
}

//
// Math
//

/// GET_HEADING_FROM_VECTOR_2D(0604)
float GetHeadingFromVector2D(float x, float y) {
    float heading = RadiansToDegrees(CGeneral::GetATanOfXY(x, y)) - 90.0f;
    while (heading < 0.0f) {
        heading += 360.0f;
    }
    return heading;
}

//
// Cutscenes
//

/// LOAD_CUTSCENE(02E4)
void LoadCutscene(const char* name) {
    CCutsceneMgr::LoadCutsceneData(name);
}

/// START_CUTSCENE(02E7)
void StartCutscene() {
    CCutsceneMgr::StartCutscene();
}

/// HAS_CUTSCENE_FINISHED(02E9)
bool HasCutsceneFinished() {
    return CCutsceneMgr::HasCutsceneFinished();
}

/// CLEAR_CUTSCENE(02EA)
void ClearCutscene() {
    CCutsceneMgr::DeleteCutsceneData();
}

/// HAS_CUTSCENE_LOADED(06B9)
bool HasCutsceneLoaded() {
    return CCutsceneMgr::ms_cutsceneLoadStatus == CCutsceneMgr::LoadStatus::LOADED;
}

/// SKIP_CUTSCENE_END(0701)
void SkipCutsceneEnd(CRunningScript& S) {
    S.m_SceneSkipIP = 0;
}

/// SKIP_CUTSCENE_START_INTERNAL(0707)
void SkipCutsceneStartInternal(CRunningScript& S, int32 ip) {
    S.m_SceneSkipIP = ip;
}

/// IS_SKIP_CUTSCENE_BUTTON_PRESSED(08D0)
bool IsSkipCutsceneButtonPressed() {
    return CCutsceneMgr::IsCutsceneSkipButtonBeingPressed();
}

//
// Script (local) variable bits
//

/// IS_LOCAL_VAR_BIT_SET_CONST(08B7)
void IsLocalVarBitSet(CRunningScript& S) {
    const auto var = reinterpret_cast<uint32*>(S.GetPointerToScriptVariable(VAR_LOCAL));

    S.CollectParameters(1);
    const auto bit = ScriptParams[0].iParam & 0x1F;

    S.UpdateCompareFlag((*var >> bit) & 1);
}

/// SET_LOCAL_VAR_BIT_CONST(08BD)
void SetLocalVarBit(CRunningScript& S) {
    const auto var = reinterpret_cast<uint32*>(S.GetPointerToScriptVariable(VAR_LOCAL));

    S.CollectParameters(1);
    const auto bit = ScriptParams[0].iParam & 0x1F;

    *var |= 1u << bit;
}

/// CLEAR_LOCAL_VAR_BIT_CONST(08C3)
void ClearLocalVarBit(CRunningScript& S) {
    const auto var = reinterpret_cast<uint32*>(S.GetPointerToScriptVariable(VAR_LOCAL));

    S.CollectParameters(1);
    const auto bit = ScriptParams[0].iParam & 0x1F;

    *var &= ~(1u << bit);
}

//
// Mission, stats & death arrest
//

/// HAS_DEATHARREST_BEEN_EXECUTED(0112)
bool HasDeatharrestBeenExecuted(CRunningScript& S) {
    return S.m_DoneDeathArrest;
}

/// MISSION_HAS_FINISHED(00D8)
void MissionHasFinished(CRunningScript& S) {
    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.Process();
    }
}

/// REGISTER_MISSION_PASSED(0318)
void RegisterMissionPassed(const char* name) {
    char label[8]{};
    strncpy(label, name, sizeof(label));
    (void)TheText.Get(label); // Original calls this just for the side effects

    std::memcpy(CStats::LastMissionPassedName, label, sizeof(label));

    CStats::IncrementStat(STAT_MISSIONS_PASSED, 1.0f);
    CStats::IncrementStat(STAT_TOTAL_LEGITIMATE_KILLS, CStats::GetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT));
    CStats::SetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT, 0.0f);

    CTheScripts::LastMissionPassedTime = CTimer::GetTimeInMS();
}

//
// Vehicle recording & vehicles (misc)
//

/// STOP_PLAYBACK_RECORDED_CAR(05EC)
void StopPlaybackRecordedCar(CVehicle* vehicle) {
    if (vehicle) {
        CVehicleRecording::StopPlaybackRecordedCar(vehicle);
    }
}

/// SET_PLAYBACK_SPEED(06FD)
void SetPlaybackSpeed(CVehicle* vehicle, float speed) {
    if (vehicle) {
        CVehicleRecording::SetPlaybackSpeed(vehicle, speed);
    }
}

/// SET_CAN_BURST_CAR_TYRES(053F)
void SetCanBurstCarTyres(CVehicle& vehicle, bool state) {
    vehicle.vehicleFlags.bTyresDontBurst = !state;
}

/// SET_CAR_FORWARD_SPEED(04BA)
void SetCarForwardSpeed(CVehicle& vehicle, float speed) {
    // The script's speed is in units per second, while `m_vecMoveSpeed` is per-frame (60Hz)
    vehicle.m_vecMoveSpeed = vehicle.GetForwardVector() * (speed / 60.0f);

    // TODO(re): retail also writes `0.22f` to `CVehicle+0x84C` (some vehicle-specific field) when handling flag `0x2000000` is set and `CVehicle+0x590` is zero - the member hasn't been identified yet
}

/// PLACE_OBJECT_RELATIVE_TO_CAR(035C)
void PlaceObjectRelativeToCar(CObject* object, CVehicle* vehicle, CVector offset) {
    if (!object || !vehicle) {
        return;
    }
    CPhysical::PlacePhysicalRelativeToOtherPhysical(vehicle, object, offset);
}

//
// Script things (checkpoints, effect systems & fires)
//

/// CREATE_CHECKPOINT(06D5)
int32 CreateCheckpoint(CRunningScript& S, int32 type, CVector at, CVector pointTo, float radius) {
    const auto handle = static_cast<int32>(CTheScripts::AddScriptCheckpoint(at, pointTo, radius, static_cast<eCheckpointType>(type)));
    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.AddEntityToList(handle, MISSION_CLEANUP_ENTITY_TYPE_CHECKPOINT);
    }
    return handle;
}

/// DELETE_CHECKPOINT(06D6)
void DeleteCheckpoint(CRunningScript& S, int32 checkpoint) {
    if (CTheScripts::GetActualScriptThingIndex(checkpoint, SCRIPT_THING_CHECKPOINT) < 0) {
        return;
    }

    CTheScripts::RemoveScriptCheckpoint(checkpoint);
    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(checkpoint, MISSION_CLEANUP_ENTITY_TYPE_CHECKPOINT);
    }
}

/// KILL_FX_SYSTEM(0650)
void KillFxSystem(CRunningScript& S, int32 scriptEffect) {
    const auto idx = CTheScripts::GetActualScriptThingIndex(scriptEffect, SCRIPT_THING_EFFECT_SYSTEM);
    if (idx < 0) {
        return;
    }

    auto& fx = CTheScripts::ScriptEffectSystemArray[idx];
    if (!fx.m_pFxSystem) {
        return;
    }

    fx.m_pFxSystem->Kill();
    CTheScripts::RemoveScriptEffectSystem(scriptEffect);

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(scriptEffect, MISSION_CLEANUP_ENTITY_TYPE_PARTICLE);
    }
}

/// STOP_FX_SYSTEM(064E)
void StopFxSystem(int32 scriptEffect) {
    const auto idx = CTheScripts::GetActualScriptThingIndex(scriptEffect, SCRIPT_THING_EFFECT_SYSTEM);
    if (idx < 0) {
        return;
    }

    if (auto* fx = CTheScripts::ScriptEffectSystemArray[idx].m_pFxSystem) {
        fx->Stop();
    }
}

/// REMOVE_SCRIPT_FIRE(02D1)
void RemoveScriptFire(int32 fireThing) {
    const auto idx = CTheScripts::GetActualScriptThingIndex(fireThing, SCRIPT_THING_FIRE);
    if (idx >= 0 && idx < MAX_NUM_FIRES) {
        gFireManager.RemoveScriptFire(static_cast<uint16>(idx));
    }
}

//
// Script route (Used by `EXTEND_ROUTE` & `TASK_FOLLOW_POINT_ROUTE`)
//

//! Retail keeps this route in the global at `0xC18D50`
static CPointRoute s_ScriptRoute;

/// EXTEND_ROUTE(05D7)
void ExtendRoute(CVector point) {
    if (!s_ScriptRoute.IsFull()) {
        s_ScriptRoute.Add(point);
    }
}

/// FLUSH_ROUTE(05D6)
void FlushRoute() {
    s_ScriptRoute.Clear();
}

//
// Tasks (misc)
//

/// TASK_PAUSE(05B9)
void TaskPause(CRunningScript& S, int32 pedHandle, int32 time, eScriptCommands opcode) {
    S.GivePedScriptedTask(pedHandle, new CTaskSimplePause(time), opcode);
}

/// TASK_STAY_IN_SAME_PLACE(0638)
void TaskStayInSamePlace(int32 pedHandle, bool state) {
    if (pedHandle == -1) {
        // No ped given: The task is appended to the script's active task sequence
        CTaskSequences::AddTaskToActiveSequence(new CTaskSimpleSetStayInSamePlace(state));
        return;
    }
    if (auto* ped = GetPedPool()->GetAtRef(pedHandle)) {
        ped->SetStayInSamePlace(state);
    }
}

/// TASK_TOGGLE_DUCK(04EB)
void TaskToggleDuck(CRunningScript& S, int32 pedHandle, int32 state, eScriptCommands opcode) {
    S.GivePedScriptedTask(pedHandle, new CTaskSimpleDuckToggle(static_cast<CTaskSimpleDuckToggle::eMode>(state)), opcode);
}

/// TASK_ACHIEVE_HEADING(05D4)
void TaskAchieveHeading(CRunningScript& S, int32 pedHandle, float headingDegrees, eScriptCommands opcode) {
    S.GivePedScriptedTask(pedHandle, new CTaskSimpleAchieveHeading(DegreesToRadians(headingDegrees)), opcode);
}

/// TASK_FOLLOW_PATH_NODES_TO_COORD(05F5)
void TaskFollowPathNodesToCoord(CRunningScript& S, int32 pedHandle, CVector target, int32 moveState, int32 time, eScriptCommands opcode) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexFollowNodeRoute(
            static_cast<eMoveState>(moveState),
            target,
            0.5f,   // targetPtTolerance
            3.0f,   // slowDownDist
            2.0f,   // followNodeThresholdHeightChange
            true,   // keepNodesHeadingAwayFromTarget
            time == -2 ? -1 : (time == -1 ? 50000 : time),
            true    // useBlending
        ),
        opcode
    );
}

/// TASK_FOLLOW_POINT_ROUTE(05D8)
void TaskFollowPointRoute(CRunningScript& S, int32 pedHandle, int32 moveState, int32 mode, eScriptCommands opcode) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexFollowPointRoute(
            static_cast<eMoveState>(moveState),
            s_ScriptRoute,
            static_cast<CTaskComplexFollowPointRoute::Mode>(mode),
            0.5f,   // targetRadius
            5.0f,   // slowDownDist
            false,  // overshootTarget
            true,   // useBlending
            true    // standStillAfterMove
        ),
        opcode
    );
}

/// TASK_GO_TO_COORD_ANY_MEANS(0603)
void TaskGoToCoordAnyMeans(CRunningScript& S, int32 pedHandle, CVector target, int32 moveState, CVehicle* vehicle, eScriptCommands opcode) {
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexGoToPointAnyMeans(moveState, target, vehicle, 0.5f, -1),
        opcode
    );
}

}

void notsa::script::commands::game::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Game");

    REGISTER_COMMAND_HANDLER(COMMAND_SET_ALL_TAXIS_HAVE_NITRO, SetAllTaxisHaveNitro);
    REGISTER_COMMAND_HANDLER(COMMAND_ACTIVATE_PIMP_CHEAT, ActivatePimpCheat);
    REGISTER_COMMAND_HANDLER(COMMAND_ARE_ANY_CAR_CHEATS_ACTIVATED, AreAnyCarCheatsActivated);
    REGISTER_COMMAND_HANDLER(COMMAND_FAIL_CURRENT_MISSION, FailCurrentMission);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_GROUND_Z_FOR_3D_COORD, GetGroundZFor3DCoord);
    REGISTER_COMMAND_HANDLER(COMMAND_PLAYER_MADE_PROGRESS, PlayerMadeProgress);
    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_MISSION_GIVEN, RegisterMissionGiven);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_PED_DENSITY_MULTIPLIER, SetPedDensityMultiplier);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_JAPANESE_VERSION, IsJapaneseVersion);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_WIDESCREEN_ON_IN_OPTIONS, IsWidescreenOnInOptions);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_RID_OF_PLAYER_PROSTITUTE, GetRidOfPlayerProstitute);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_AREA_NAME, SetAreaName);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_RADIO_TO_PLAYERS_FAVOURITE_STATION, SetRadioToPlayersFavorite);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_DISABLE_MILITARY_ZONES, SetDisableMilitaryZones);
    REGISTER_COMMAND_HANDLER(COMMAND_INCREMENT_FLOAT_STAT_NO_MESSAGE, IncrementFloatStatNoMessage);
    REGISTER_COMMAND_HANDLER(COMMAND_INCREMENT_INT_STAT_NO_MESSAGE, IncrementFloatStatNoMessage);
    REGISTER_COMMAND_HANDLER(COMMAND_TAKE_PHOTO, TakePhoto);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_NO_RESPRAYS, SetNoResprays);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_RESPAWN_POINT_FOR_DURATION_OF_MISSION, SetRespawnPointForDurationOfMission);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CURRENT_LANGUAGE, GetCurrentLanguage);
    REGISTER_COMMAND_HANDLER(COMMAND_DO_WEAPON_STUFF_AT_START_OF_2P_GAME, DoWeaponStuffAtStartOf2PlayerGame);
    REGISTER_COMMAND_HANDLER(COMMAND_DISPLAY_RADAR, DisplayRadar);
    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_BEST_POSITION, RegisterBestPosition);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_EXPLOSION_IN_AREA, IsExplosionInArea);
    REGISTER_COMMAND_HANDLER(COMMAND_SHAKE_PAD, ShakePad);
    REGISTER_COMMAND_HANDLER(COMMAND_DRAW_SHADOW, DrawShadow);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_XBOX_VERSION, IsXboxVersion);
#if BUILD_XBOX
    //TODO:
    //REGISTER_COMMAND_HANDLER(COMMAND_IS_XBOX_PLAYER2_PRESSING_START, IsXboxPlayer2PressingStart);
    //REGISTER_COMMAND_HANDLER(COMMAND_FINISHED_WITH_XBOX_PLAYER2, FinishedWithXboxPlayer2);
#endif
    REGISTER_COMMAND_HANDLER(COMMAND_DO_DEBUG_STUFF, DoDebugStuff);

    // Onscreen timers/counters
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_ONSCREEN_COUNTER, ClearOnscreenCounter);
    REGISTER_COMMAND_HANDLER(COMMAND_DISPLAY_NTH_ONSCREEN_COUNTER_WITH_STRING, DisplayNthOnscreenCounterWithString);

    // Blips
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_BLIP, RemoveBlip);
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_BLIP_DISPLAY, ChangeBlipDisplay);
    REGISTER_COMMAND_HANDLER(COMMAND_DOES_BLIP_EXIST, DoesBlipExist);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_BLIP_FOR_COORD, AddBlipForCoord);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_SPRITE_BLIP_FOR_CONTACT_POINT, AddSpriteBlipForContactPoint);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_SHORT_RANGE_SPRITE_BLIP_FOR_COORD, AddShortRangeSpriteBlipForCoord);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_SHORT_RANGE_SPRITE_BLIP_FOR_CONTACT_POINT, AddShortRangeSpriteBlipForContactPoint);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_BLIP_ENTRY_EXIT, SetBlipEntryExit);

    // Camera, weather & world
    REGISTER_COMMAND_HANDLER(COMMAND_SET_FADING_COLOUR, SetFadingColour);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_WIDESCREEN, SwitchWidescreen);
    REGISTER_COMMAND_HANDLER(COMMAND_FORCE_WEATHER_NOW, ForceWeatherNow);
    REGISTER_COMMAND_HANDLER(COMMAND_RELEASE_WEATHER, ReleaseWeather);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_WORLD_PROCESSING, SwitchWorldProcessing);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_AREA, ClearArea);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_AREA_VISIBLE, SetAreaVisible);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_AREA_VISIBLE, GetAreaVisible);
    REGISTER_COMMAND_HANDLER(COMMAND_REQUEST_COLLISION, RequestCollision);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_EXTRA_COLOURS, SetExtraColours);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_EXTRA_COLOURS, ClearExtraColours);

    // Player & stats
    REGISTER_COMMAND_HANDLER(COMMAND_DECLARE_MISSION_FLAG, DeclareMissionFlag);
    REGISTER_COMMAND_HANDLER(COMMAND_STORE_WANTED_LEVEL, StoreWantedLevel);
    REGISTER_COMMAND_HANDLER(COMMAND_SQRT, Sqrt);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_PROGRESS_TOTAL, SetProgressTotal);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_PROGRESS_PERCENTAGE, GetProgressPercentage);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_TOTAL_NUMBER_OF_MISSIONS, SetTotalNumberOfMissions);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_MISSION_RESPECT_TOTAL, SetMissionRespectTotal);
    REGISTER_COMMAND_HANDLER(COMMAND_FIND_MAX_NUMBER_OF_GROUP_MEMBERS, FindMaxNumberOfGroupMembers);
    REGISTER_COMMAND_HANDLER(COMMAND_ARE_MEASUREMENTS_IN_METRES, AreMeasurementsInMetres);
    REGISTER_COMMAND_HANDLER(COMMAND_CONVERT_METRES_TO_FEET, ConvertMetresToFeet);
    REGISTER_COMMAND_HANDLER(COMMAND_CONVERT_METRES_TO_FEET_INT, ConvertMetresToFeetInt);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_POSITION_OF_ANALOGUE_STICKS, GetPositionOfAnalogueSticks);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_INT_VAR_EQUAL_TO_CONSTANT, IsIntEqual);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_INT_LVAR_EQUAL_TO_CONSTANT, IsIntEqual);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_INT_LVAR_EQUAL_TO_INT_VAR, IsIntEqual);

    // Pickups
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_PICKUP, CreatePickup);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_PICKUP_WITH_AMMO, CreatePickupWithAmmo);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_PROTECTION_PICKUP, CreateProtectionPickup);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_LOCKED_PROPERTY_PICKUP, CreateLockedPropertyPickup);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_FORSALE_PROPERTY_PICKUP, CreateForSalePropertyPickup);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_PICKUP_BEEN_COLLECTED, HasPickupBeenCollected);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_PICKUP, RemovePickup);
    REGISTER_COMMAND_HANDLER(COMMAND_UPDATE_PICKUP_MONEY_PER_DAY, UpdatePickupMoneyPerDay);

    // Gangs & garages
    REGISTER_COMMAND_HANDLER(COMMAND_SET_GANG_WEAPONS, SetGangWeapons);
    REGISTER_COMMAND_HANDLER(COMMAND_ACTIVATE_GARAGE, ActivateGarage);
    REGISTER_COMMAND_HANDLER(COMMAND_DEACTIVATE_GARAGE, DeactivateGarage);
    REGISTER_COMMAND_HANDLER(COMMAND_CLOSE_GARAGE, CloseGarage);
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_GARAGE_TYPE, ChangeGarageType);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_GARAGE_OPEN, IsGarageOpen);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_GARAGE_RESPRAY_FREE, SetGarageResprayFree);

    // Audio
    REGISTER_COMMAND_HANDLER(COMMAND_PLAY_MISSION_PASSED_TUNE, PlayMissionPassedTune);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_MISSION_AUDIO_LOADED, HasMissionAudioLoaded);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_MISSION_AUDIO_FINISHED, HasMissionAudioFinished);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_MISSION_AUDIO, ClearMissionAudio);
    REGISTER_COMMAND_HANDLER(COMMAND_REPORT_MISSION_AUDIO_EVENT_AT_POSITION, ReportMissionAudioEventAtPosition);
    REGISTER_COMMAND_HANDLER(COMMAND_STOP_BEAT_TRACK, StopBeatTrack);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_BEAT_PROXIMITY, GetBeatProximity);

    // Save menu
    REGISTER_COMMAND_HANDLER(COMMAND_ACTIVATE_SAVE_MENU, ActivateSaveMenu);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_SAVE_GAME_FINISHED, HasSaveGameFinished);

    // Animations
    REGISTER_COMMAND_HANDLER(COMMAND_REQUEST_ANIMATION, RequestAnimation);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_ANIMATION_LOADED, HasAnimationLoaded);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_ANIMATION, RemoveAnimation);
    REGISTER_COMMAND_HANDLER(COMMAND_ATTACH_ANIMS_TO_MODEL, AttachAnimsToModel);

    // Misc entity checks
    REGISTER_COMMAND_HANDLER(COMMAND_GET_ANGLE_BETWEEN_2D_VECTORS, GetAngleBetween2DVectors);

    // Tasks
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_STAND_STILL, TaskStandStill);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_GO_STRAIGHT_TO_COORD, TaskGoStraightToCoord);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_USE_MOBILE_PHONE, TaskUseMobilePhone);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_DEAD, TaskDead);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_PLAY_ANIM_NON_INTERRUPTABLE, TaskPlayAnim);

    // Groups & decision makers
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_GROUP, RemoveGroup);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_DECISION_MAKER, RemoveDecisionMaker);
    REGISTER_COMMAND_HANDLER(COMMAND_DOES_DECISION_MAKER_EXIST, DoesDecisionMakerExist);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_RELATIONSHIP, SetRelationship);

    // Messages & help
    REGISTER_COMMAND_HANDLER(COMMAND_IS_MESSAGE_BEING_DISPLAYED, IsMessageBeingDisplayed);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_HELP_MESSAGE_BEING_DISPLAYED, IsHelpMessageBeingDisplayed);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_THIS_HELP_MESSAGE_BEING_DISPLAYED, IsThisHelpMessageBeingDisplayed);
    REGISTER_COMMAND_HANDLER(COMMAND_FORCE_BIG_MESSAGE_AND_COUNTER, ForceBigMessageAndCounter);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_MINIGAME_IN_PROGRESS, IsMinigameInProgress);

    // IPL, zones, tags & gang wars
    REGISTER_COMMAND_HANDLER(COMMAND_REQUEST_IPL, RequestIpl);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_LOADED_SHOP, GetLoadedShop);
    REGISTER_COMMAND_HANDLER(COMMAND_FIND_NUMBER_TAGS_TAGGED, FindNumberTagsTagged);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_TERRITORY_UNDER_CONTROL_PERCENTAGE, GetTerritoryUnderControlPercentage);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_GANG_WAR_FIGHTING_GOING_ON, IsGangWarFightingGoingOn);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_CURRENT_DAY_OF_WEEK, GetCurrentDayOfWeek);

    // Models
    REGISTER_COMMAND_HANDLER(COMMAND_GET_MODEL_DIMENSIONS, GetModelDimensions);

    // Script brains & streamed scripts
    REGISTER_COMMAND_HANDLER(COMMAND_STREAM_SCRIPT, StreamScript);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_STREAMED_SCRIPT_LOADED, HasStreamedScriptLoaded);
    REGISTER_COMMAND_HANDLER(COMMAND_MARK_STREAMED_SCRIPT_AS_NO_LONGER_NEEDED, MarkStreamedScriptAsNoLongerNeeded);
    REGISTER_COMMAND_HANDLER(COMMAND_START_NEW_STREAMED_SCRIPT, StartNewStreamedScript);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_NUMBER_OF_INSTANCES_OF_STREAMED_SCRIPT, GetNumberOfInstancesOfStreamedScript);
    REGISTER_COMMAND_HANDLER(COMMAND_ALLOCATE_STREAMED_SCRIPT_TO_RANDOM_PED, AllocateStreamedScriptToRandomPed);
    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_SCRIPT_BRAIN_FOR_CODE_USE, RegisterScriptBrainForCodeUse);
    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_ATTRACTOR_SCRIPT_BRAIN_FOR_CODE_USE, RegisterAttractorScriptBrainForCodeUse);

    // Global variable bits
    REGISTER_COMMAND_HANDLER(COMMAND_IS_GLOBAL_VAR_BIT_SET_CONST, IsGlobalVarBitSet);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_GLOBAL_VAR_BIT_SET_VAR, IsGlobalVarBitSet);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_GLOBAL_VAR_BIT_SET_LVAR, IsGlobalVarBitSet);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_GLOBAL_VAR_BIT_CONST, SetGlobalVarBit);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_GLOBAL_VAR_BIT_VAR, SetGlobalVarBit);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_GLOBAL_VAR_BIT_LVAR, SetGlobalVarBit);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_GLOBAL_VAR_BIT_CONST, ClearGlobalVarBit);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_GLOBAL_VAR_BIT_VAR, ClearGlobalVarBit);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_GLOBAL_VAR_BIT_LVAR, ClearGlobalVarBit);

    // Switch statements
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_START, Switch);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_CONTINUED, Switch);

    // Entry exits
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_ENTRY_EXIT, SwitchEntryExit);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CLOSEST_ENTRY_EXIT_FLAG, SetClosestEntryExitFlag);
    REGISTER_COMMAND_HANDLER(COMMAND_DISABLE_ALL_ENTRY_EXITS, DisableAllEntryExits);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_NAMED_ENTRY_EXIT_FLAG, SetNamedEntryExitFlag);

    // Penalties & restart points
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_DEATH_PENALTIES, SwitchDeathPenalties);
    REGISTER_COMMAND_HANDLER(COMMAND_SWITCH_ARREST_PENALTIES, SwitchArrestPenalties);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_EXTRA_HOSPITAL_RESTART_POINT, SetExtraHospitalRestartPoint);

    // Menus
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_MENU, CreateMenu);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_MENU_COLUMN, SetMenuColumn);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_MENU_ITEM_SELECTED, GetMenuItemSelected);
    REGISTER_COMMAND_HANDLER(COMMAND_DELETE_MENU, DeleteMenu);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_ACTIVE_MENU_ITEM, SetActiveMenuItem);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_MENU_ITEM_WITH_NUMBER, SetMenuItemWithNumber);

    // HUD colours & 3D markers
    REGISTER_COMMAND_HANDLER(COMMAND_GET_HUD_COLOUR, GetHudColour);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_USER_3D_MARKER, CreateUser3dMarker);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_USER_3D_MARKER, RemoveUser3dMarker);

    // Vehicles (misc)
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_TRAIN_DERAILED, HasTrainDerailed);
    REGISTER_COMMAND_HANDLER(COMMAND_RANDOM_PASSENGER_SAY, RandomPassengerSay);

    // Blips (misc)
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_BLIP_COLOUR, ChangeBlipColour);
    REGISTER_COMMAND_HANDLER(COMMAND_CHANGE_BLIP_SCALE, ChangeBlipScale);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_BLIP_AS_FRIENDLY, SetBlipAsFriendly);

    // Camera, audio & explosions (misc)
    REGISTER_COMMAND_HANDLER(COMMAND_GET_FADING_STATUS, GetFadingStatus);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_WANTED_MULTIPLIER, SetWantedMultiplier);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_RADIO_CHANNEL, SetRadioChannel);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_ONE_OFF_SOUND, AddOneOffSound);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_MISSION_AUDIO_POSITION, SetMissionAudioPosition);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_EXPLOSION, AddExplosion);

    // World state
    REGISTER_COMMAND_HANDLER(COMMAND_SET_LA_RIOTS, SetLaRiots);
    REGISTER_COMMAND_HANDLER(COMMAND_DISPLAY_HUD, DisplayHud);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_MINIGAME_IN_PROGRESS, SetMinigameInProgress);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_TIME_SCALE, SetTimeScale);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_HEADING_FROM_VECTOR_2D, GetHeadingFromVector2D);

    // Cutscenes
    REGISTER_COMMAND_HANDLER(COMMAND_LOAD_CUTSCENE, LoadCutscene);
    REGISTER_COMMAND_HANDLER(COMMAND_START_CUTSCENE, StartCutscene);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_CUTSCENE_FINISHED, HasCutsceneFinished);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_CUTSCENE, ClearCutscene);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_CUTSCENE_LOADED, HasCutsceneLoaded);
    REGISTER_COMMAND_HANDLER(COMMAND_SKIP_CUTSCENE_END, SkipCutsceneEnd);
    REGISTER_COMMAND_HANDLER(COMMAND_SKIP_CUTSCENE_START_INTERNAL, SkipCutsceneStartInternal);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_SKIP_CUTSCENE_BUTTON_PRESSED, IsSkipCutsceneButtonPressed);

    // Local variable bits
    REGISTER_COMMAND_HANDLER(COMMAND_IS_LOCAL_VAR_BIT_SET_CONST, IsLocalVarBitSet);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_LOCAL_VAR_BIT_CONST, SetLocalVarBit);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_LOCAL_VAR_BIT_CONST, ClearLocalVarBit);

    // Mission, stats & death arrest
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_DEATHARREST_BEEN_EXECUTED, HasDeatharrestBeenExecuted);
    REGISTER_COMMAND_HANDLER(COMMAND_MISSION_HAS_FINISHED, MissionHasFinished);
    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_MISSION_PASSED, RegisterMissionPassed);

    // Vehicle recording & vehicles (misc)
    REGISTER_COMMAND_HANDLER(COMMAND_STOP_PLAYBACK_RECORDED_CAR, StopPlaybackRecordedCar);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_PLAYBACK_SPEED, SetPlaybackSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAN_BURST_CAR_TYRES, SetCanBurstCarTyres);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CAR_FORWARD_SPEED, SetCarForwardSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_PLACE_OBJECT_RELATIVE_TO_CAR, PlaceObjectRelativeToCar);

    // Script things (checkpoints, effect systems & fires)
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_CHECKPOINT, CreateCheckpoint);
    REGISTER_COMMAND_HANDLER(COMMAND_DELETE_CHECKPOINT, DeleteCheckpoint);
    REGISTER_COMMAND_HANDLER(COMMAND_KILL_FX_SYSTEM, KillFxSystem);
    REGISTER_COMMAND_HANDLER(COMMAND_STOP_FX_SYSTEM, StopFxSystem);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_SCRIPT_FIRE, RemoveScriptFire);

    // Script route
    REGISTER_COMMAND_HANDLER(COMMAND_EXTEND_ROUTE, ExtendRoute);
    REGISTER_COMMAND_HANDLER(COMMAND_FLUSH_ROUTE, FlushRoute);

    // Tasks (misc)
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_PLAY_ANIM, TaskPlayAnim);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_PAUSE, TaskPause);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_STAY_IN_SAME_PLACE, TaskStayInSamePlace);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_TOGGLE_DUCK, TaskToggleDuck);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_ACHIEVE_HEADING, TaskAchieveHeading);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_FOLLOW_PATH_NODES_TO_COORD, TaskFollowPathNodesToCoord);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_FOLLOW_POINT_ROUTE, TaskFollowPointRoute);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_GO_TO_COORD_ANY_MEANS, TaskGoToCoordAnyMeans);

    // -----------------------------[ NOP ]-----------------------------
    REGISTER_COMMAND_NOP(COMMAND_SAVE_INT_TO_DEBUG_FILE);
    REGISTER_COMMAND_NOP(COMMAND_SAVE_FLOAT_TO_DEBUG_FILE);
    REGISTER_COMMAND_NOP(COMMAND_SET_MENU_HEADER_ORIENTATION);
    REGISTER_COMMAND_NOP(COMMAND_IS_ICECREAM_JINGLE_ON);
    REGISTER_COMMAND_NOP(COMMAND_IS_SNIPER_BULLET_IN_AREA);
    REGISTER_COMMAND_NOP(COMMAND_SAVE_NEWLINE_TO_DEBUG_FILE);
    REGISTER_COMMAND_NOP(COMMAND_HAS_IMPORT_GARAGE_SLOT_BEEN_FILLED);
    REGISTER_COMMAND_NOP(COMMAND_NO_SPECIAL_CAMERA_FOR_THIS_GARAGE);
    REGISTER_COMMAND_NOP(COMMAND_GET_COLLECTABLE1S_COLLECTED);

    // -------------------------[ UNSUPPORTED ]-------------------------
    REGISTER_UNSUPPORTED_COMMAND_HANDLER(COMMAND_REGISTER_JUMP_DISTANCE, RegisterJumpDistance);
    REGISTER_UNSUPPORTED_COMMAND_HANDLER(COMMAND_REGISTER_JUMP_HEIGHT, RegisterJumpHeight);
    REGISTER_UNSUPPORTED_COMMAND_HANDLER(COMMAND_REGISTER_JUMP_SPINS, RegisterJumpSpins);
    REGISTER_UNSUPPORTED_COMMAND_HANDLER(COMMAND_REGISTER_JUMP_STUNT, RegisterJumpStunt);
    REGISTER_UNSUPPORTED_COMMAND_HANDLER(COMMAND_REGISTER_UNIQUE_JUMP_FOUND, RegisterUniqueJumpFound);
    REGISTER_UNSUPPORTED_COMMAND_HANDLER(COMMAND_SET_UNIQUE_JUMPS_TOTAL, SetUniqueJumpsTotal);
    REGISTER_UNSUPPORTED_COMMAND_HANDLER(COMMAND_REGISTER_PASSENGER_DROPPED_OFF_TAXI, RegisterPassengerDroppedOffTaxi);
    REGISTER_UNSUPPORTED_COMMAND_HANDLER(COMMAND_REGISTER_MONEY_MADE_TAXI, RegisterMoneyMadeTaxi);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_SWAT_REQUIRED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_FBI_REQUIRED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_ARMY_REQUIRED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CLEAR_NUMBER_OF_POWER_PILLS_EATEN);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_POWER_PILL);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_START_PACMAN_RACE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_START_PACMAN_RECORD);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_NUMBER_OF_POWER_PILLS_EATEN);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CLEAR_PACMAN);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_START_PACMAN_SCRAMBLE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_NUMBER_OF_POWER_PILLS_CARRIED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CLEAR_NUMBER_OF_POWER_PILLS_CARRIED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_MESSAGE_WAIT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COMEDY_CONTROLS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ACTIVATE_CRUSHER_CRANE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_ZONE_GROUP);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_START_DRUG_RUN);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_DRUG_RUN_BEEN_COMPLETED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_DRUG_PLANE_BEEN_SHOT_DOWN);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SAVE_PLAYER_FROM_FIRES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_INDUSTRIAL_PASSED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_COMMERCIAL_PASSED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SUBURBAN_PASSED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_NASTY_GAME);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_UNDRESS_CHAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_DRESS_CHAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_START_CHASE_SCENE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_STOP_CHASE_SCENE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_EXPLOSION_IN_ZONE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_START_DRUG_DROP_OFF);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_DROP_OFF_PLANE_BEEN_SHOT_DOWN);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_FIND_DROP_OFF_PLANE_COORDINATES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CREATE_FLOATING_PACKAGE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_AUDIO_STREAM);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_MOTION_BLUR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_2_REPEATED_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_2_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_3_REPEATED_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_3_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_4_REPEATED_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_4_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_5_REPEATED_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_5_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_6_REPEATED_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_6_PHONE_MESSAGES);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_PED_ROADS_ON_ANGLED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_PED_ROADS_OFF_ANGLED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_ROADS_ON_ANGLED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SWITCH_ROADS_OFF_ANGLED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_CRANE_LIFTING_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_REMOVE_ROUTE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_START_CATALINA_HELI);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CATALINA_HELI_TAKE_OFF);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_REMOVE_CATALINA_HELI);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_CATALINA_HELI_BEEN_SHOT_DOWN);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_REMOVE_ALL_PLAYER_WEAPONS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GRAB_CATALINA_HELI);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_ROTATING_GARAGE_DOOR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CATALINA_HELI_FLY_AWAY);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_PHONE_DISPLAYING_MESSAGE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_BLIP_FOR_PICKUP_OLD);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SPECIAL_0);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SPECIAL_1);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SPECIAL_2);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SPECIAL_3);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SPECIAL_4);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SPECIAL_5);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SPECIAL_6);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SPECIAL_7);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_AMMO_TO_PLAYER);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_AMMO_TO_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_PLAYER_STILL_ALIVE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ORDER_DRIVER_OUT_OF_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_PATROL_POINT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_PLAYER_IN_GANGZONE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_PLAYER_IN_ZONE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_PLAYER_INVINCIBLE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_PLAYER_GRAPHIC_TYPE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_PLAYER_BEEN_ARRESTED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_FAVOURITE_CAR_MODEL_FOR_CHAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CHANGE_CAR_LOCK);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SHAKE_CAM_WITH_POINT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_CAR_REMAP);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_CAR_JUST_SUNK);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_NO_COLLIDE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_CAR_DEAD_IN_AREA_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_CAR_DEAD_IN_AREA_3D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_TRAILER_ATTACHED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_CAR_ON_TRAILER);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_CAR_GOT_WEAPON);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_PARK);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_HAS_PARK_FINISHED);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_KILL_ALL_PASSENGERS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_BULLETPROOF);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_FLAMEPROOF);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_ROCKETPROOF);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_CARBOMB_ACTIVE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GIVE_CAR_ALARM);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_PLAYER_AMMO);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CAR_AMMO);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOAD_CAMERA_SPLINE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_MOVE_CAMERA_ALONG_SPLINE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_CAMERA_POSITION_ALONG_SPLINE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_RESTORE_PLAYER_AFTER_2P_GAME);
}
