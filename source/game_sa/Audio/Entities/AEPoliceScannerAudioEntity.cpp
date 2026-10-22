#include "StdInc.h"

#include "AEPoliceScannerAudioEntity.h"

#include "AEAudioHardware.h"

// 0x4E6E00
CAEPoliceScannerAudioEntity::~CAEPoliceScannerAudioEntity() {
    if (s_pPSControlling == this && s_nScannerPlaybackState != STATE_INITIAL) {
        s_bStoppingScanner = true;
        if (s_pSound) {
            s_pSound->StopSoundAndForget();
            s_pSound = nullptr;
        }
        FinishedPlayingScannerDialogue();
    }
}

// 0x5B9C30
void CAEPoliceScannerAudioEntity::StaticInitialise() {
    s_NextNewScannerDialogueTime = 0;
    s_bScannerDisabled           = false;
    s_nScannerPlaybackState      = STATE_INITIAL;
    s_pPSControlling             = nullptr;
    s_pCurrentSlots              = nullptr;
    s_fVolumeOffset              = 0.0f;
}

// 0x4E6E90
void CAEPoliceScannerAudioEntity::Reset() {
    StopScanner(true);
    FinishedPlayingScannerDialogue();
}

// 0x4E71E0
void CAEPoliceScannerAudioEntity::AddAudioEvent(eAudioEvents event, eCrimeType crimeType, const CVector& point) {
    if (event != AE_CRIME_COMMITTED) {
        return;
    }
    if (crimeType < CRIME_DAMAGED_PED || crimeType > CRIME_AIM_GUN) {
        return;
    }

    // Scanner phrase tables (retail `.data`; zero-filled natively until the exe data replay covers them)
    static auto& s_NoiseSoundIds      = StaticRef<int16[4]>(0x8C8160);
    static auto& s_CrimeSpeechIds     = StaticRef<int16[24]>(0x8C8168); //!< Indexed by `eCrimeType`
    static auto& s_ZoneNames          = StaticRef<char[194][8]>(0x8C8198); //!< Matched against `CZone::m_TextLabel`
    static auto& s_ZonePhraseIds      = StaticRef<int16[194]>(0x8C87A8);
    static auto& s_ZoneIsCity         = StaticRef<bool[194]>(0x8C8930);
    static auto& s_VehAudioTypePhrases = StaticRef<int16[46]>(0x8C89F8); //!< Indexed by `eAEVehicleAudioTypeForName`
    static auto& s_VehTypeNeedsRemap  = StaticRef<bool[46]>(0x8C8A54);
    static auto& s_RemapPhraseIds     = StaticRef<int16[0x7F]>(0x8C8A88); //!< Indexed by primary colour

    tScannerSlot first[NUM_POLICE_SCANNER_SLOTS];
    tScannerSlot second[NUM_POLICE_SCANNER_SLOTS];
    rng::fill(first, tScannerSlot{});  // Bank = SoundID = -1
    rng::fill(second, tScannerSlot{});

    // 1. Noise + crime code
    first[0] = { SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS, s_NoiseSoundIds[CAEAudioUtility::GetRandomNumberInRange(0, 3)] };
    first[1] = { SND_BANK_SCRIPT_SCANNER_NUMBERS, s_CrimeSpeechIds[crimeType] };

    // 2. Area (zone) - the scanner only reports zones it has a phrase for
    const auto* zone = CTheZones::FindSmallestZoneForPosition(point, true);
    if (!zone) {
        return;
    }

    int32 z = 0;
    for (; z < 194; z++) {
        if (std::memcmp(zone->m_TextLabel, s_ZoneNames[z], sizeof(zone->m_TextLabel)) == 0
            && s_ZonePhraseIds[z] >= 0
        ) {
            break;
        }
    }
    if (z == 194) {
        return;
    }

    // 3. Direction inside the zone (only for city zones)
    if (s_ZoneIsCity[z]) {
        const float minX = (float)zone->m_fX1, maxX = (float)zone->m_fX2;
        const float minY = (float)zone->m_fY1, maxY = (float)zone->m_fY2;
        const float cx   = (maxX - minX) * 0.5f + minX;
        const float cy   = (maxY - minY) * 0.5f + minY;
        const float rx   = (maxX - minX) * 0.25f;
        const float ry   = (maxY - minY) * 0.25f;

        if (point.y > cy + ry) {
            first[2].SoundID = 2; // North
        } else if (point.y < cy - ry) {
            first[2].SoundID = 3; // South
        }
        if (first[2].SoundID >= 0) {
            first[2].Bank = SND_BANK_SCRIPT_SCANNER_DIRECTIONS;
        }

        if (point.x > cx + rx) {
            first[3] = { SND_BANK_SCRIPT_SCANNER_DIRECTIONS, 1 }; // East
        } else if (point.x < cx - rx) {
            first[3] = { SND_BANK_SCRIPT_SCANNER_DIRECTIONS, 4 }; // West
        } else if (first[2].SoundID < 0) {
            first[3] = { SND_BANK_SCRIPT_SCANNER_DIRECTIONS, 0 }; // Centre
        }
    }
    first[4] = { SND_BANK_SCRIPT_SCANNER_AREAS, s_ZonePhraseIds[z] };

    // 4. Suspect description - only when the crime belongs to one of the players
    CPed* player = nullptr;
    for (int32 i = 0; i < 2; i++) {
        if (&FindPlayerWanted(i)->m_PoliceScannerAudioEntity == this) {
            player = FindPlayerPed(i);
        }
    }
    if (!player) {
        PlayPoliceScannerDialogue(first, second);
        return;
    }

    if (!player->bInVehicle) {
        if (player->GetIntelligence()->GetTaskSwim()) {
            second[4].SoundID = 2;
        } else if (player->GetIntelligence()->GetTaskJetPack()) {
            PlayPoliceScannerDialogue(first, second);
            return;
        } else {
            second[4].SoundID = 4;
        }
        second[0] = { SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS, 7 };
        second[4].Bank = SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS;
        PlayPoliceScannerDialogue(first, second);
        return;
    }

    CVehicle* veh = player->m_pVehicle;
    if (!veh) {
        PlayPoliceScannerDialogue(first, second);
        return;
    }

    //! Retail reads the (cached) `VehicleAudioTypeForName` of the vehicle
    const auto audioType = (int32)CAEVehicleAudioEntity::GetVehicleAudioSettings((int16)veh->m_nModelIndex).VehicleAudioTypeForName;
    if (audioType < 0 || audioType >= (int32)std::size(s_VehAudioTypePhrases)) {
        PlayPoliceScannerDialogue(first, second);
        return;
    }
    if (s_VehAudioTypePhrases[audioType] < 0) {
        PlayPoliceScannerDialogue(first, second);
        return;
    }

    second[0] = { SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS, 7 };
    second[1].Bank = SND_BANK_SCRIPT_SCANNER_INSTRUCTIONS;
    second[1].SoundID = notsa::contains(
        { AE_VAT_MOPED, AE_VAT_BIKE, AE_VAT_QUADBIKE, AE_VAT_MOWER, AE_VAT_BICYCLE },
        (eAEVehicleAudioTypeForName)audioType
    ) ? 3 : 1;
    if (audioType == AE_VAT_TRACTOR) {
        second[1].SoundID = 1;
    }

    if (s_VehTypeNeedsRemap[audioType]) {
        const auto remap = veh->GetRemapIndex();
        if (remap != -1) {
            second[2] = { SND_BANK_SCRIPT_SCANNER_COLOURS, 4 };
        } else {
            const uint8 colour = veh->m_nPrimaryColor;
            if (colour != 0 && colour < 0x7F && s_RemapPhraseIds[colour] >= 0) {
                second[2] = { SND_BANK_SCRIPT_SCANNER_COLOURS, s_RemapPhraseIds[colour] };
            }
        }
    }
    second[4] = { SND_BANK_SCRIPT_SCANNER_VEHICLES, s_VehAudioTypePhrases[audioType] };

    PlayPoliceScannerDialogue(first, second);
}

// 0x4E6BC0
void CAEPoliceScannerAudioEntity::PrepSlots() {
    if (!s_pCurrentSlots) {
        return;
    }
    for (auto i = 0; i < NUM_POLICE_SCANNER_SLOTS; i++) {
        s_SlotState[i] = s_pCurrentSlots[i].IsActive();
    }
}

// 0x4E6CD0
void CAEPoliceScannerAudioEntity::LoadSlots() {
    if (!s_pCurrentSlots) {
        return;
    }

    bool canPlay = true;
    for (auto i = 0; i < NUM_POLICE_SCANNER_SLOTS; i++) {
        const auto slot = (eSoundBankSlot)(SND_BANK_SLOT_SCANNER_FIRST + i);
        auto& currentSlot = s_pCurrentSlots[i];

        if (s_SlotState[i]) {
            if (s_SlotState[i] == 2) {
                bool loaded = AEAudioHardware.IsSoundLoaded(currentSlot.Bank, currentSlot.SoundID, slot);
                if (loaded) {
                    s_SlotState[i] = 3;
                } else {
                    canPlay = false;
                }
            }
        } else if (currentSlot.IsActive()) {
            s_SlotState[i] = 1;
        } else {
            if (!CStreaming::IsVeryBusy()) {
                AEAudioHardware.LoadSound(currentSlot.Bank, currentSlot.SoundID, slot);
                s_SlotState[i] = 2;
            }
            canPlay = false;
        }
    }
    if (canPlay) {
        s_nScannerPlaybackState = FOUR;
    }
}

// 0x4E6DB0
void CAEPoliceScannerAudioEntity::EnableScanner() {
    s_bScannerDisabled = false;
}

// 0x4E71B0
void CAEPoliceScannerAudioEntity::DisableScanner(bool a1, bool bStopSound) {
    s_bScannerDisabled = true;
    if (a1 && s_nScannerPlaybackState != STATE_INITIAL) {
        if (s_pPSControlling) {
            StopScanner(bStopSound);
        }
    }
}

// 0x4E6DC0
void CAEPoliceScannerAudioEntity::StopScanner(bool bStopSound) {
    if (s_nScannerPlaybackState == STATE_INITIAL)
        return;

    s_bStoppingScanner = true;
    if (bStopSound) {
        if (s_pSound) {
            s_pSound->StopSoundAndForget();
            s_pSound = nullptr;
        }
        FinishedPlayingScannerDialogue();
    }
}

// 0x4E6C30
void CAEPoliceScannerAudioEntity::FinishedPlayingScannerDialogue() {
    constexpr uint32 NEW_DIALOGUE_DELAY_MS = 10'000; // 0x8C8140

    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_NOISE_STOP);
    s_nScannerPlaybackState      = STATE_INITIAL;
    s_pPSControlling             = nullptr;
    s_pCurrentSlots              = nullptr;
    s_bStoppingScanner           = false;
    s_NextNewScannerDialogueTime = CTimer::GetTimeInMS() + NEW_DIALOGUE_DELAY_MS;
    s_fVolumeOffset              = 0.0f;

    rng::fill(s_SlotState, 1);

    rng::fill(s_ScannerSlotFirst, tScannerSlot{});
    rng::fill(s_ScannerSlotSecond, tScannerSlot{});
}

// 0x4E6F60
void CAEPoliceScannerAudioEntity::PlayLoadedDialogue() {
    // Start the search at the slot that's currently being spoken (if any)
    int32 i = 0;
    while (i < NUM_POLICE_SCANNER_SLOTS && s_SlotState[i] != FIVE) {
        i++;
    }
    if (i == NUM_POLICE_SCANNER_SLOTS) {
        i = 0; // Nothing being spoken - search from the beginning
    }

    // Find the first slot that finished loading (state THREE) and play it
    while (i < NUM_POLICE_SCANNER_SLOTS && s_SlotState[i] != THREE) {
        i++;
    }

    if (i < NUM_POLICE_SCANNER_SLOTS) {
        const float volume = GetDefaultVolume(AE_CRIME_COMMITTED) + s_fVolumeOffset;

        CAESound sound;
        sound.Initialise(
            (eSoundBankSlot)(SND_BANK_SLOT_SCANNER_FIRST + i),
            s_pCurrentSlots[i].SoundID,
            this,
            { 0.0f, 1.0f, 0.0f },
            volume,
            1.0f,
            1.0f,
            1.0f,
            0,
            SOUND_DEFAULT,
            0.0f,
            0
        );
        sound.m_ClientVariable = (float)i;
        sound.m_Flags          = SOUND_FRONT_END | SOUND_IS_CANCELLABLE | SOUND_REQUEST_UPDATES | SOUND_IS_DUCKABLE;
        sound.m_Event          = AE_CRIME_COMMITTED;

        if ((s_pSound = AESoundManager.RequestNewSound(&sound))) {
            s_SlotState[i]          = FIVE;
            s_nScannerPlaybackState = SEVEN;
        }
        return;
    }

    // No loaded slot left: switch over to the second (noise/click) section
    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_CLICK, s_fVolumeOffset); // NOTE: Click volume (`0xB61D54`) is 0.0f
    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_NOISE_STOP);

    if (s_nSectionPlaying) {
        FinishedPlayingScannerDialogue();
        return;
    }

    s_pCurrentSlots   = s_ScannerSlotSecond;
    s_nSectionPlaying = 1;
    for (auto k = 0; k < NUM_POLICE_SCANNER_SLOTS; k++) {
        s_SlotState[k] = s_ScannerSlotSecond[k].IsActive();
    }
    s_nPlaybackStartTime    = 0;
    s_nAbortPlaybackTime    = CTimer::GetTimeInMS() + 5000u; // 0x8C8148
    s_nScannerPlaybackState = TWO;
}

// 0x4E6B60
void CAEPoliceScannerAudioEntity::PopulateScannerDialogueLists(const tScannerSlot* first, const tScannerSlot* second) {
    assert(first && second);
    if (s_nScannerPlaybackState == STATE_INITIAL) {
        for (auto slotIndex = 0; slotIndex < NUM_POLICE_SCANNER_SLOTS; slotIndex++) {
            s_ScannerSlotFirst[slotIndex] = first[slotIndex];
            s_ScannerSlotSecond[slotIndex] = second[slotIndex];
        }
    }
}

// inlined
// 0x4E6C00
bool CAEPoliceScannerAudioEntity::CanWePlayNewScannerDialogue() {
    if (s_nScannerPlaybackState != STATE_INITIAL)
        return false;

    if (CTimer::GetTimeInMS() < s_NextNewScannerDialogueTime)
        return false;

    if (TheCamera.m_bWideScreenOn)
        return false;

    if (s_bScannerDisabled)
        return false;

    return true;
}

// 0x4E6ED0
void CAEPoliceScannerAudioEntity::PlayPoliceScannerDialogue(tScannerSlot* first, tScannerSlot* second) {
    assert(first && second);
    if (CanWePlayNewScannerDialogue()) { // todo: maybe little bit wrong
        PopulateScannerDialogueLists(first, second);
        s_pPSControlling = this;
        s_nSectionPlaying = 0;
        s_pCurrentSlots = s_ScannerSlotFirst;
        PrepSlots();
        s_nScannerPlaybackState = TWO;
        s_nPlaybackStartTime = CTimer::GetTimeInMS() + 2000; // todo: gSpeechContextLookup[365][6]
        s_nAbortPlaybackTime = CTimer::GetTimeInMS() + 5000; // todo: gSpeechContextLookup[366][0]
    }
}

// 0x4E7590
void CAEPoliceScannerAudioEntity::UpdateParameters(CAESound* sound, int16 curPlayPos) {
    if (curPlayPos == -1) {
        s_pSound = nullptr;
        if (s_bStoppingScanner) {
            if (s_nScannerPlaybackState != STATE_INITIAL) {
                s_bStoppingScanner = true; // V1048 [CWE-1164] The 's_bStoppingScanner' variable was assigned the same value
                FinishedPlayingScannerDialogue();
            }
            return;
        }
        PlayLoadedDialogue();
        return;
    }

    if (s_bStoppingScanner) {
        sound->m_Volume = sound->m_Volume - 6.0f; // todo: *(float*)&gSpeechContextLookup[366][4]
        return;
    }

    if (sound->m_Length > 0 && curPlayPos > sound->m_Length - 40 && sound->m_BankSlot != 37) { // todo: -40 should be replaced with by gSpeechContextLookup[366][2]
        sound->SetFlags(eSoundEnvironment::SOUND_REQUEST_UPDATES, false);
        s_pSound = nullptr;
        PlayLoadedDialogue();
        return;
    }
}

// 0x4E7630
void CAEPoliceScannerAudioEntity::Service() {
    static constexpr uint32 startDelay = 300;   // 0x8C8154
    static constexpr float noiseVolume = -6.0f; // 0x8C8158
    static constexpr float clickVolume = +0.0f; // 0xB61D54

    bool finishPlaying;
    if (TheCamera.m_bWideScreenOn && s_nScannerPlaybackState != STATE_INITIAL) {
        finishPlaying = true;
        s_bStoppingScanner = true;
    } else {
        finishPlaying = s_bStoppingScanner;
    }

    switch (s_nScannerPlaybackState) {
    case TWO:
        if (CTimer::GetTimeInMS() > s_nAbortPlaybackTime || finishPlaying) {
            FinishedPlayingScannerDialogue();
            break;
        }

        LoadSlots();
        break;
    case FOUR:
        if (finishPlaying) {
            FinishedPlayingScannerDialogue();
            break;
        }

        if (CTimer::GetTimeInMS() >= s_nPlaybackStartTime) {
            s_fVolumeOffset = CAEVehicleAudioEntity::s_pVehicleAudioSettingsForRadio ? 0.0f : -8.0f; // todo: -8 is gSpeechContextLookup[367][2]
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_CLICK,       s_fVolumeOffset + clickVolume, 1.0f);
            AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SCANNER_NOISE_START, s_fVolumeOffset + noiseVolume, 1.0f); // todo: noiseVolume is gSpeechContextLookup[367][0]
            s_nPlaybackStartTime = startDelay + CTimer::GetTimeInMS(); // todo: startDelay is gSpeechContextLookup[366][6]
            s_nScannerPlaybackState = FIVE;
        }
        break;
    case FIVE:
        if (finishPlaying) {
            FinishedPlayingScannerDialogue();
            break;
        }

        if (CTimer::GetTimeInMS() >= s_nPlaybackStartTime && s_pPSControlling) {
            s_nScannerPlaybackState = SEVEN;
            s_pPSControlling->PlayLoadedDialogue();
        }
        break;
    default:
        return;
    }
}

void CAEPoliceScannerAudioEntity::InjectHooks() {
    RH_ScopedVirtualClass(CAEPoliceScannerAudioEntity, 0x85F368, 1);
    RH_ScopedCategory("Audio/Entities");

    RH_ScopedInstall(Constructor, 0x56DA00);
    RH_ScopedInstall(Destructor, 0x4E6E00);
    RH_ScopedInstall(StaticInitialise, 0x5B9C30);
    RH_ScopedInstall(Reset, 0x4E6E90);
    RH_ScopedInstall(AddAudioEvent, 0x4E71E0);
    RH_ScopedInstall(PrepSlots, 0x4E6BC0);
    RH_ScopedInstall(LoadSlots, 0x4E6CD0, { .Reversed = false });
    RH_ScopedInstall(EnableScanner, 0x4E6DB0);
    RH_ScopedInstall(DisableScanner, 0x4E71B0);
    RH_ScopedInstall(StopScanner, 0x4E6DC0);
    RH_ScopedInstall(FinishedPlayingScannerDialogue, 0x4E6C30);
    RH_ScopedInstall(PlayLoadedDialogue, 0x4E6F60);
    RH_ScopedInstall(PopulateScannerDialogueLists, 0x4E6B60);
    RH_ScopedInstall(CanWePlayNewScannerDialogue, 0x4E6C00);
    RH_ScopedInstall(PlayPoliceScannerDialogue, 0x4E6ED0);
    RH_ScopedVMTInstall(UpdateParameters, 0x4E7590);
    RH_ScopedInstall(Service, 0x4E7630, { .Reversed = false });
}

CAEPoliceScannerAudioEntity* CAEPoliceScannerAudioEntity::Constructor() {
    this->CAEPoliceScannerAudioEntity::CAEPoliceScannerAudioEntity();
    return this;
}

CAEPoliceScannerAudioEntity* CAEPoliceScannerAudioEntity::Destructor() {
    this->CAEPoliceScannerAudioEntity::~CAEPoliceScannerAudioEntity();
    return this;
}
