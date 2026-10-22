#include "StdInc.h"

#include "AEScriptAudioEntity.h"
#include "AESoundManager.h"
#include "AEAudioUtility.h"
#include "AEAudioHardware.h"
#include "AEAmbienceTrackManager.h"
#include "AEVehicleAudioEntity.h"

// 0x5074D0
CAEScriptAudioEntity::CAEScriptAudioEntity() : CAEAudioEntity() {
    m_nLastTimeHornPlayed = 0;
    field_7E = 0;
    field_7C = 0;
    m_Volume = 0.0f;
    m_Speed = 1.0f;
    field_7D = 0;
    field_8C = 0.0f;
}

// 0x5B9B60
void CAEScriptAudioEntity::Initialise() {
    for (auto& link : wavLinks) {
        link.Init();
    }
}

// 0x4EC150
void CAEScriptAudioEntity::Reset() {
    for (auto i = 0; i < MISSION_AUDIO_COUNT; i++) {
        ClearMissionAudio(i);
    }
    field_7C = 0;
    m_Entity = nullptr;
    field_7D = 0;
    field_8C = 2.0f;
}

// 0x0
void CAEScriptAudioEntity::AddAudioEvent(int32) {
    /* Android NOP */
}

// 0x4EC100
CVector* CAEScriptAudioEntity::AttachMissionAudioToPhysical(uint8 sampleId, CPhysical* physical) {
    auto& link = wavLinks[sampleId];
    link.m_pEntity   = physical;
    link.m_vPosition = CVector{ -1000.0f, -1000.0f, -1000.0f };
    return &link.m_vPosition;
}

// 0x4EC040
void CAEScriptAudioEntity::ClearMissionAudio(uint8 sampleId) {
    if (sampleId >= MISSION_AUDIO_COUNT) {
        return;
    }

    AESoundManager.CancelSoundsInBankSlot(SND_BANK_SLOT_MISSION1 + sampleId, true);

    auto& link       = wavLinks[sampleId];
    link.m_pEntity   = nullptr;
    link.m_vPosition = CVector{ -1000.0f, -1000.0f, -1000.0f };
    link.m_Sound     = nullptr;
}

// 0x4EBFE0
bool CAEScriptAudioEntity::IsMissionAudioSampleFinished(uint8 sampleId) {
    if (sampleId >= MISSION_AUDIO_COUNT) {
        return true;
    }
    if (sampleId > 1) {
        return AESoundManager.AreSoundsPlayingInBankSlot(SND_BANK_SLOT_MISSION1 + sampleId) == 0;
    }
    return wavLinks[sampleId].m_Sound == nullptr;
}

// 0x4EBF60
int8 CAEScriptAudioEntity::GetMissionAudioLoadingStatus(uint8 sampleId) {
    if (sampleId >= MISSION_AUDIO_COUNT) {
        return 1;
    }

    const auto& link = wavLinks[sampleId];
    if (link.m_nBankId < 0) {
        return 1;
    }

    const auto slot = static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + sampleId);
    if (link.m_nBankSlotId < 0) {
        return AEAudioHardware.GetSoundBankLoadingStatus(static_cast<eSoundBank>(link.m_nBankId), slot);
    }
    return AEAudioHardware.GetSoundLoadingStatus(static_cast<eSoundBank>(link.m_nBankId), static_cast<eSoundID>(link.m_nBankSlotId), slot);
}

// 0x4EC020
int32 CAEScriptAudioEntity::GetMissionAudioEvent(uint8 sampleId) {
    return wavLinks[sampleId].m_nAudioEvent;
}

// 0x4EC0C0
void CAEScriptAudioEntity::SetMissionAudioPosition(uint8 sampleId, CVector& posn) {
    auto& link       = wavLinks[sampleId];
    link.m_vPosition = posn;
    link.m_pEntity   = nullptr;
}

// 0x4EC4D0
CVector* CAEScriptAudioEntity::GetMissionAudioPosition(uint8 sampleId) {
    auto& link = wavLinks[sampleId];
    if (link.m_pEntity) {
        return &link.m_pEntity->GetPosition();
    }
    if (link.m_vPosition != CVector{ -1000.0f, -1000.0f, -1000.0f }
        && link.m_vPosition != CVector{ 0.0f, 0.0f, 0.0f }
    ) {
        return &link.m_vPosition;
    }
    return nullptr;
}

// 0x4EC6D0
void CAEScriptAudioEntity::PlayMissionBankSound(uint8 sampleId, CVector& posn, CPhysical* physical, int16 sfxId, uint8 linkId, uint8 a7, float volume, float maxDistance, float speed) {
    if (linkId < 2 || linkId >= MISSION_AUDIO_COUNT) { // Only the mission banks (Slots 2, 3) can be used
        return;
    }

    if (a7 && AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(sampleId, this) != 0) {
        return;
    }

    const auto bankSlot = static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + linkId); // 0x1A + linkId
    const auto& link    = wavLinks[linkId];
    if (!AEAudioHardware.IsSoundBankLoaded(static_cast<eSoundBank>(link.m_nBankId), bankSlot)) {
        return;
    }

    // Resolve position (`-1000` and `0,0,0` mean "no position" -> play as a front-end sound)
    CVector pos;
    bool    bFrontEnd = false;
    if (physical) {
        pos = physical->GetPosition();
    } else if (posn == CVector{-1000.f, -1000.f, -1000.f} || posn == CVector{0.f, 0.f, 0.f}) {
        pos       = CVector{0.f, 1.f, 0.f};
        bFrontEnd = true;
    } else {
        pos       = posn;
        bFrontEnd = posn == CVector{-1.f, 0.f, 0.f} || posn == CVector{1.f, 0.f, 0.f};
    }

    m_tempSound.Initialise(
        bankSlot,
        static_cast<eSoundID>(sfxId),
        this,
        pos,
        GetDefaultVolume(static_cast<eAudioEvents>(sampleId)) + volume,
        maxDistance, // rollOff
        speed,
        1.f, // doppler
        0,   // frameDelay
        SOUND_IS_CANCELLABLE | SOUND_REQUEST_UPDATES, // 0x6 (Set directly after `Initialise` by the original)
        0.f, // speedVariance
        0    // playTime
    );
    m_tempSound.SetFlags(SOUND_FRONT_END, bFrontEnd);
    if (physical) {
        m_tempSound.SetFlags(SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY, true);
        m_tempSound.RegisterWithPhysicalEntity(physical);
    }
    m_tempSound.m_Event = static_cast<eAudioEvents>(sampleId);
    AESoundManager.RequestNewSound(&m_tempSound);
}

// event eAudioEvents
// 0x4EC550
void CAEScriptAudioEntity::PlayResidentSoundEvent(eSoundBankSlot slot, eSoundBank bank, eSoundID sfx, eAudioEvents event, CVector& posn, CPhysical* physical, float vol, float speed, int16 playPosn, float maxDistance) {
    if (!AEAudioHardware.IsSoundBankLoaded(bank, slot)) {
        return;
    }

    bool bFrontend = false;
    const auto volume = GetDefaultVolume(static_cast<eAudioEvents>(event)) + vol;
    CVector pos = [&] {
        if (physical) {
            return physical->GetPosition();
        } else if (posn == -1000.0f || posn.IsZero()) {
            bFrontend = true;
            return CVector{0.0f, 1.0f, 0.0f};
        } else {
            return posn;
        }
    }();

    AESoundManager.PlaySound({
        .BankSlotID         = slot,
        .SoundID            = sfx,
        .AudioEntity        = this,
        .Pos                = pos,
        .Volume             = volume,
        .RollOffFactor      = maxDistance,
        .Speed              = speed,
        .Doppler            = 1.0f,
        .FrameDelay         = 0,
        .Flags              = SOUND_START_PERCENTAGE | SOUND_REQUEST_UPDATES | SOUND_IS_CANCELLABLE | (bFrontend ? SOUND_FRONT_END : 0u),
        .FrequencyVariance  = 0.0f,
        .PlayTime           = playPosn,
        .RegisterWithEntity = physical,
        .EventID            = (eAudioEvents)(event),
    });
}

// 0x4EC270
void CAEScriptAudioEntity::PlayLoadedMissionAudio(uint8 slotId) {
    if (slotId >= MISSION_AUDIO_COUNT) {
        return;
    }

    auto& link = wavLinks[slotId];
    if (link.m_nBankId < 0 || link.m_nBankSlotId < 0) {
        return;
    }
    if (GetMissionAudioLoadingStatus(slotId) != 1) { // 1 = Loaded
        return;
    }

    float volume;
    if (link.m_nAudioEvent == 0xFFFF) {
        volume = -100.f;
    } else {
        volume = GetDefaultVolume(static_cast<eAudioEvents>(link.m_nAudioEvent));
    }

    // Sounds in the first two slots are ducked/compressed
    const bool bDuckable = slotId < 2;
    if (bDuckable && volume == -128.f) { // "No volume set" -> default
        volume = 6.f;
    }

    // Resolve position (No position -> front-end)
    CVector pos;
    bool    bFrontEnd = false;
    if (link.m_pEntity) {
        pos = link.m_pEntity->GetPosition();
    } else if (link.m_vPosition == CVector{-1000.f, -1000.f, -1000.f} || link.m_vPosition == CVector{0.f, 0.f, 0.f}) {
        pos       = CVector{0.f, 1.f, 0.f};
        bFrontEnd = true;
    } else {
        pos = link.m_vPosition;
    }

    m_tempSound.Initialise(
        static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + slotId),
        static_cast<eSoundID>(static_cast<uint16>(link.m_nBankSlotId)),
        this,
        pos,
        volume,
        2.f, // rollOff
        1.f, // speed
        1.f, // doppler
        0,   // frameDelay
        SOUND_IS_CANCELLABLE | SOUND_REQUEST_UPDATES | SOUND_PLAY_PHYSICALLY, // 0xE
        0.f, // speedVariance
        0    // playTime
    );
    m_tempSound.SetFlags(SOUND_FRONT_END, bFrontEnd);
    m_tempSound.SetFlags(SOUND_IS_DUCKABLE, bDuckable);
    m_tempSound.SetFlags(SOUND_IS_COMPRESSABLE, bDuckable);
    m_tempSound.SetFlags(SOUND_SMOOTH_DUCKING, bDuckable);

    link.m_Sound = AESoundManager.RequestNewSound(&m_tempSound);
}

// 0x4EC190
void CAEScriptAudioEntity::PreloadMissionAudio(uint8 slotId, int32 sampleId) {
    if (slotId >= MISSION_AUDIO_COUNT) {
        return;
    }
    if (!IsMissionAudioSampleFinished(slotId)) {
        return;
    }

    auto& link = wavLinks[slotId];

    eSoundBankS32 bankId;
    int32        soundId = sampleId; // Out param, written back into the slot's event below
    if (!CAEAudioUtility::GetBankAndSoundFromScriptSlotAudioEvent(static_cast<eAudioEvents>(sampleId), bankId, soundId, slotId)) {
        return;
    }

    const auto slot = static_cast<eSoundBankSlot>(SND_BANK_SLOT_MISSION1 + slotId);
    if (link.m_nBankSlotId < 0) { // Negative sound id means the whole bank was requested
        AEAudioHardware.LoadSoundBank(static_cast<eSoundBank>(link.m_nBankId), slot);
    } else {
        AEAudioHardware.LoadSound(static_cast<eSoundBank>(link.m_nBankId), static_cast<eSoundID>(static_cast<uint16>(link.m_nBankSlotId)), slot);
    }

    link.m_nAudioEvent = soundId;
    link.m_pEntity     = nullptr;
    link.m_vPosition   = CVector{-1000.f, -1000.f, -1000.f};
}

// 0x4ECCF0
// TODO(re): The switch below transcribes the binary's dispatch for the events that could be decoded
//   mechanically (case values taken from the compare tree + jump tables; body per case, see
//   `.local/re/0x4ECCF0.casemap.md`). The remaining events are still unimplemented and currently play
//   nothing - their bodies need the extra branch logic (field_7D / `AreSoundsOfThisEventPlaying...`
//   guards, garage/explosion sub-entities, ambience/frontend events) plus this function's
//   `volume`/`speed` arguments, which those bodies forward:
    //   - AE_CRANE_WINCH_MOVE, AE_SCRIPT_WAREHOUSE_DOOR_SLIDE_START, AE_SCRIPT_WAREHOUSE_DOOR_SLIDE_STOP,
    //   - AE_SCRIPT_CLAXON_START, AE_SCRIPT_BONNET_DENT, AE_SCRIPT_BASKETBALL_BOUNCE,
    //   - AE_SCRIPT_BASKETBALL_SCORE, AE_SCRIPT_POOL_BREAK, AE_SCRIPT_POOL_BALL_HIT_BALL,
    //   - AE_SCRIPT_POOL_HIT_CUSHION, AE_SCRIPT_POOL_BALL_POT, AE_SCRIPT_CRANE_ENTER,
    //   - AE_SCRIPT_CRANE_MOVE_START, AE_SCRIPT_CRANE_MOVE_STOP, AE_SCRIPT_CRANE_EXIT,
    //   - AE_SCRIPT_CRANE_SMASH_PORTACABIN, AE_SCRIPT_CONTAINER_COLLISION, AE_SCRIPT_KEYPAD_BEEP,
    //   - AE_SCRIPT_SHOOTING_RANGE_TARGET_SHATTER, AE_SCRIPT_SHOOTING_RANGE_TARGET_DROP,
    //   - AE_SCRIPT_SHOOTING_RANGE_TARGET_MOVE_STOP, AE_SCRIPT_SHUTTER_DOOR_START,
    //   - AE_SCRIPT_SHUTTER_DOOR_STOP, AE_SCRIPT_FREEFALL_START, AE_SCRIPT_FREEFALL_STOP,
    //   - AE_SCRIPT_PARACHUTE_OPEN, AE_SCRIPT_PARACHUTE_COLLAPSE, AE_SCRIPT_DUAL_SHOOT,
    //   - AE_SCRIPT_DUAL_MENU_SELECT, AE_SCRIPT_DUAL_PICKUP_LIGHT, AE_SCRIPT_AMMUNATION_BUY_WEAPON,
    //   - AE_SCRIPT_AMMUNATION_BUY_WEAPON_DENIED, AE_SCRIPT_SHOP_BUY, AE_SCRIPT_SHOP_BUY_DENIED,
    //   - AE_SCRIPT_RACE_321, AE_SCRIPT_RACE_GO, AE_SCRIPT_PART_MISSION_COMPLETE, AE_SCRIPT_GOGO_ENEMY_FIRE,
    //   - AE_SCRIPT_GOGO_TRACK_START, AE_SCRIPT_GOGO_TRACK_STOP, AE_SCRIPT_GOGO_ACCEPT,
    //   - AE_SCRIPT_DUAL_TRACK_START, AE_SCRIPT_DUAL_TRACK_STOP, AE_SCRIPT_BEE_ZAP, AE_SCRIPT_BEE_SELECT,
    //   - AE_SCRIPT_BEE_TRACK_START, AE_SCRIPT_BEE_TRACK_STOP, AE_SCRIPT_BEE_GAME_OVER,
    //   - AE_SCRIPT_FREEZER_CLOSE, AE_SCRIPT_ROULETTE_ADD_CASH, AE_SCRIPT_ROULETTE_REMOVE_CASH,
    //   - AE_SCRIPT_ROULETTE_NO_CASH, AE_SCRIPT_BANDIT_WHEEL_STOP, AE_SCRIPT_BANDIT_WHEEL_START,
    //   - AE_SCRIPT_BANDIT_PAYOUT, AE_SCRIPT_OFFICE_FIRE_ALARM_START, AE_SCRIPT_OFFICE_FIRE_ALARM_STOP,
    //   - AE_SCRIPT_OFFICE_FIRE_COUGHING_START, AE_SCRIPT_OFFICE_FIRE_COUGHING_STOP,
    //   - AE_SCRIPT_BIKE_GANG_WHEEL_SPIN, AE_SCRIPT_AWARD_TRACK_START, AE_SCRIPT_AWARD_TRACK_STOP,
    //   - AE_SCRIPT_MOLOTOV, AE_SCRIPT_MESH_GATE_OPEN_START
void CAEScriptAudioEntity::ProcessMissionAudioEvent(eAudioEvents eventId, CVector& posn, CPhysical* physical, float volume, float speed) {
    switch (eventId) {
        case AE_SCRIPT_FIRST:
            CAEVehicleAudioEntity::DisableHelicoptors();
            break;
        case AE_SCRIPT_ENABLE_HELI_AUDIO:
            CAEVehicleAudioEntity::EnableHelicoptors();
            break;
        case AE_SCRIPT_CEILING_VENT_LAND:
            PlayResidentSoundEvent(static_cast<eSoundBankSlot>(2), static_cast<eSoundBank>(39), 64, eventId, posn, physical, 0.0f, 0.79f, 35, 1.0f);
            break;
        case AE_SCRIPT_CLAXON_STOP:
            AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_CLAXON_START, this);
            break;
        case AE_SCRIPT_BLAST_DOOR_SLIDE_STOP:
            AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_BLAST_DOOR_SLIDE_START, this);
            break;
        case AE_SCRIPT_POOL_HIT_WHITE:
            PlayMissionBankSound(eventId, posn, physical, 10, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_POOL_CHALK_CUE:
            PlayMissionBankSound(eventId, posn, physical, 7, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_VIDEO_POKER_PAYOUT:
            PlayMissionBankSound(eventId, posn, physical, 1, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_VIDEO_POKER_BUTTON:
            PlayMissionBankSound(eventId, posn, physical, 0, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_DUAL_THRUST:
            PlayMissionBankSound(eventId, posn, physical, 0, 3, 1, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_DUAL_EXPLOSION_SHORT:
            PlayMissionBankSound(eventId, posn, physical, 2, 3, 1, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_DUAL_EXPLOSION_LONG:
            PlayMissionBankSound(eventId, posn, physical, 1, 3, 1, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_DUAL_MENU_DESELECT:
        case AE_SCRIPT_BEE_PICKUP:
            PlayMissionBankSound(eventId, posn, physical, 4, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_DUAL_GAME_OVER:
            PlayMissionBankSound(eventId, posn, physical, 3, 3, 1, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_DUAL_PICKUP_DARK:
            PlayMissionBankSound(eventId, posn, physical, 6, 3, 1, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_DUAL_TOUCH_DARK:
            PlayMissionBankSound(eventId, posn, physical, 9, 3, 1, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_WHEEL_OF_FORTUNE_CLACKER:
            PlayResidentSoundEvent(static_cast<eSoundBankSlot>(2), static_cast<eSoundBank>(39), 19, eventId, posn, physical, 0.0f, 1.0f, 70, 1.0f);
            break;
        case AE_SCRIPT_BLAST_DOOR_SLIDE_START:
            PlayMissionBankSound(eventId, posn, physical, 0, 2, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_KEYPAD_FAIL:
            PlayMissionBankSound(eventId, posn, physical, 1, 2, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_SHOOTING_RANGE_TARGET_MOVE_START:
            PlayMissionBankSound(eventId, posn, physical, 0, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_DUAL_TOUCH_LIGHT:
            PlayMissionBankSound(eventId, posn, physical, 10, 3, 1, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_GOGO_PLAYER_FIRE:
            PlayMissionBankSound(eventId, posn, physical, 5, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_BASKETBALL_HIT_HOOP:
        case AE_SCRIPT_GOGO_EXPLOSION:
        case AE_SCRIPT_BANDIT_INSERT_COIN:
            PlayMissionBankSound(eventId, posn, physical, 3, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_GOGO_SELECT:
            PlayMissionBankSound(eventId, posn, physical, 6, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_GOGO_DECLINE:
        case AE_SCRIPT_BEE_ACCEPT:
            PlayMissionBankSound(eventId, posn, physical, 1, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_GOGO_GAME_OVER:
            PlayMissionBankSound(eventId, posn, physical, 4, 3, 1, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_BEE_DROP:
            PlayMissionBankSound(eventId, posn, physical, 2, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_BEE_DECLINE:
            PlayMissionBankSound(eventId, posn, physical, 1, 3, 0, 0.0f, 2.0f, 0.79f);
            break;
        case AE_SCRIPT_ROULETTE_SPIN:
            PlayMissionBankSound(eventId, posn, physical, 0, 3, 0, 0.0f, 2.0f, 1.0f);
            break;
        case AE_SCRIPT_BIKE_PACKER_CLUNK:
            PlayResidentSoundEvent(static_cast<eSoundBankSlot>(2), static_cast<eSoundBank>(39), 65, eventId, posn, physical, 0.0f, 1.0f, 16, 1.0f);
            break;
        case AE_SCRIPT_FREEZER_OPEN:
            PlayMissionBankSound(eventId, posn, physical, 2, 2, 1, 0.0f, 3.0f, 1.0f);
            break;
        case AE_SCRIPT_MEAT_TRACK_START:
            PlayMissionBankSound(eventId, posn, physical, 3, 2, 1, 0.0f, 3.0f, 1.0f);
            break;
        case AE_SCRIPT_MEAT_TRACK_STOP:
            AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_MEAT_TRACK_START, this);
            break;
        case AE_SCRIPT_KEYPAD_PASS:
            PlayMissionBankSound(eventId, posn, physical, 2, 2, 0, 0.0f, 2.0f, 1.0f);
            break;
    default:
        break;
    }
}

// 0x4EE960
void CAEScriptAudioEntity::ReportMissionAudioEvent(eAudioEvents eventId, CPhysical* physical, float volume, float speed) {
    CVector posn{ -1000.0f, -1000.0f, -1000.0f };
    ProcessMissionAudioEvent(eventId, posn, physical, volume, speed);
}

// 0x4EE940
void CAEScriptAudioEntity::ReportMissionAudioEvent(eAudioEvents eventId, CVector& posn) {
    ProcessMissionAudioEvent(eventId, posn, nullptr);
}

// 0x4EC970
void CAEScriptAudioEntity::UpdateParameters(CAESound* sound, int16 playTime) {
    if (!sound) {
        return;
    }

    CVector noPosn{ -1000.f, -1000.f, -1000.f };

    // NOTE: Faithfully replicates the original, where the `switch` is in the `else` of the
    //       "is this slot's sound" check, meaning it runs once per non-matching slot.
    for (auto i = 0; i < MISSION_AUDIO_COUNT; i++) {
        auto& link = wavLinks[i];
        if (sound != link.m_Sound) {
            switch (sound->m_Event) {
            case AE_SCRIPT_GYM_BIKE_START:
            case AE_SCRIPT_GYM_RUNNING_MACHINE_START: {
                const auto targetVolume = GetDefaultVolume(static_cast<eAudioEvents>(sound->m_Event));
                if (field_8C == 1.f) {
                    if (sound->m_Volume < targetVolume) {
                        sound->m_Volume = std::min(sound->m_Volume + 0.1f, targetVolume);
                    }
                } else if (field_8C == 2.f) {
                    const auto fadedVolume = targetVolume - 18.f;
                    if (sound->m_Volume <= fadedVolume) {
                        sound->StopSoundAndForget();
                    } else {
                        sound->m_Volume = std::max(sound->m_Volume - 0.1f, fadedVolume);
                    }
                }
                break;
            }
            case AE_SCRIPT_ROULETTE_SPIN: {
                if (m_nLastTimeHornPlayed + 0x1194 < CTimer::GetTimeInMS()) {
                    if (sound->m_Volume <= -40.f) {
                        sound->StopSoundAndForget();
                        m_nLastTimeHornPlayed = 0;
                    } else {
                        sound->m_Volume -= 0.1f;
                    }
                    if (sound->m_Speed > 0.f) {
                        sound->m_Speed = std::max(sound->m_Speed - 0.001f, 0.f);
                    }
                }
                if (m_nLastTimeHornPlayed + 0x12C0 < CTimer::GetTimeInMS() && field_7C == 0) {
                    PlayMissionBankSound(
                        static_cast<uint8>(AE_SCRIPT_ROULETTE_BALL_BOUNCING),
                        noPosn,
                        nullptr,
                        static_cast<int16>(CAEAudioUtility::GetRandomNumberInRange(1, 3)),
                        3,
                        0,
                        0.f,
                        2.f,
                        1.f
                    );
                    field_7C = 1;
                }
                break;
            }
            case AE_CRANE_WINCH_MOVE: {
                if (m_nLastTimeHornPlayed + 300 < CTimer::GetTimeInMS()) {
                    sound->StopSoundAndForget();
                    m_nLastTimeHornPlayed = 0;
                    PlayResidentSoundEvent(
                        SND_BANK_SLOT_PLAYER_ENGINE_P,
                        SND_BANK_GENRL_CRANE_P,
                        2,
                        AE_SCRIPT_CRANE_MOVE_STOP,
                        sound->m_CurrPos,
                        static_cast<CPhysical*>(sound->m_PhysicalEntity.Get()),
                        -12.f,
                        1.f,
                        0,
                        2.5f
                    );
                } else {
                    sound->m_Volume = m_Volume;
                    sound->m_Speed  = m_Speed;
                }
                break;
            }
            case AE_SCRIPT_DUAL_THRUST:
            case AE_SCRIPT_BEE_BUZZ:
            case AE_SCRIPT_TEMPEST_SHIELD_GLOW:
                if (m_nLastTimeHornPlayed + 300 < CTimer::GetTimeInMS()) {
                    sound->StopSoundAndForget();
                    m_nLastTimeHornPlayed = 0;
                }
                break;
            case AE_SCRIPT_SWEETS_HORN:
                if (m_nLastTimeHornPlayed + 500 < CTimer::GetTimeInMS()) {
                    sound->StopSoundAndForget();
                    m_nLastTimeHornPlayed = 0;
                }
                break;
            default:
                break;
            }
            continue;
        }

        if (playTime == -1) {
            link.m_Sound = nullptr;
            return;
        }
        if (link.m_pEntity) {
            sound->SetPosition(link.m_pEntity->GetPosition());
        }
    }
}

// 0x4EC900
void CAEScriptAudioEntity::Service() {
    CVector posn = {-1000.0f, -1000.0f, -1000.0f};
    if (!m_Physical)
        return;
    if (AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_CRANE_ENTER, this) != 0)
        return;

    PlayResidentSoundEvent(SND_BANK_SLOT_PLAYER_ENGINE_P, SND_BANK_GENRL_CRANE_P, 0, AE_SCRIPT_CRANE_ENTER, posn, m_Physical, 0.0f, 1.0f, 0, 2.5f);
}

void CAEScriptAudioEntity::InjectHooks() {
    RH_ScopedVirtualClass(CAEScriptAudioEntity, 0x862E58, 1);
    RH_ScopedCategory("Audio/Entities");

    RH_ScopedInstall(Constructor, 0x5074D0);
    RH_ScopedInstall(Initialise, 0x5B9B60);
    RH_ScopedInstall(Service, 0x4EC900);
    RH_ScopedInstall(Reset, 0x4EC150);
    RH_ScopedInstall(GetMissionAudioLoadingStatus, 0x4EBF60);
    RH_ScopedInstall(IsMissionAudioSampleFinished, 0x4EBFE0);
    RH_ScopedInstall(GetMissionAudioEvent, 0x4EC020);
    RH_ScopedInstall(ClearMissionAudio, 0x4EC040);
    RH_ScopedInstall(SetMissionAudioPosition, 0x4EC0C0);
    RH_ScopedInstall(AttachMissionAudioToPhysical, 0x4EC100);
    RH_ScopedInstall(PreloadMissionAudio, 0x4EC190);
    RH_ScopedInstall(PlayLoadedMissionAudio, 0x4EC270);
    RH_ScopedInstall(GetMissionAudioPosition, 0x4EC4D0);
    RH_ScopedInstall(PlayResidentSoundEvent, 0x4EC550);
    RH_ScopedInstall(PlayMissionBankSound, 0x4EC6D0);
    RH_ScopedInstall(ProcessMissionAudioEvent, 0x4ECCF0);
    RH_ScopedOverloadedInstall(ReportMissionAudioEvent, "1", 0x4EE960, void (CAEScriptAudioEntity::*)(eAudioEvents, CPhysical*, float, float));
    RH_ScopedOverloadedInstall(ReportMissionAudioEvent, "2", 0x4EE940, void (CAEScriptAudioEntity::*)(eAudioEvents, CVector&));
    RH_ScopedVMTInstall(UpdateParameters, 0x4EC970);
}

CAEScriptAudioEntity* CAEScriptAudioEntity::Constructor() {
    this->CAEScriptAudioEntity::CAEScriptAudioEntity();
    return this;
}
