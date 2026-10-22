#include "StdInc.h"

#include "AEPedAudioEntity.h"
#include "AEAudioHardware.h"
#include "AEAudioUtility.h"

#include "AECollisionAudioEntity.CollisionLookup.h"

void CAEPedAudioEntity::InjectHooks() {
    RH_ScopedClass(CAEPedAudioEntity);
    RH_ScopedCategory("Audio/Entities");

    //RH_ScopedInstall(Constructor, 0x5DE8D0, { .Reversed = false });

    RH_ScopedInstall(Initialise, 0x4E0E80);
    RH_ScopedInstall(StaticInitialise, 0x5B98A0);
    RH_ScopedInstall(Terminate, 0x4E1360);
    RH_ScopedInstall(AddAudioEvent, 0x4E2BB0);
    RH_ScopedInstall(TurnOnJetPack, 0x4E28A0);
    RH_ScopedInstall(TurnOffJetPack, 0x4E2A70);
    RH_ScopedInstall(StopJetPackSound, 0x4E1120);
    RH_ScopedInstall(UpdateJetPack, 0x4E0EE0);
    RH_ScopedInstall(PlayWindRush, 0x4E1170);
    RH_ScopedInstall(UpdateParameters, 0x4E1180);
    RH_ScopedInstall(HandleFootstepEvent, 0x4E13A0);
    RH_ScopedInstall(HandleSkateEvent, 0x4E17E0);
    RH_ScopedInstall(HandleLandingEvent, 0x4E18E0);
    RH_ScopedInstall(HandlePedSwing, 0x4E1A40);
    RH_ScopedInstall(HandlePedHit, 0x4E1CC0);
    RH_ScopedInstall(HandlePedJacked, 0x4E2350);
    RH_ScopedInstall(HandleSwimSplash, 0x4E26A0);
    RH_ScopedInstall(HandleSwimWake, 0x4E2790);
    RH_ScopedInstall(PlayShirtFlap, 0x4E2A90);
    RH_ScopedInstall(Service, 0x4E2EE0);
}

// 0x5DE8D0
CAEPedAudioEntity::CAEPedAudioEntity() : CAEAudioEntity() {
    m_pPed = nullptr;
    m_bCanAddEvent = false;

    m_JetPackSound0 = nullptr;
    m_JetPackSound1 = nullptr;
    m_JetPackSound2 = nullptr;
}

// (CEntity* entity)
// 0x4E0E80
void CAEPedAudioEntity::Initialise(CPed* ped) {
    m_pPed = ped;
    m_nSfxId = 0;
    m_LastSwimWakeTriggerTimeMs = 0;

    m_bJetPackPlaying = false;
    m_JetPackSound0 = nullptr;
    m_JetPackSound1 = nullptr;
    m_fVolume1 = -100.0f;
    m_fVolume2 = -100.0f;

    field_150 = nullptr;
    field_154 = -100.0f;
    field_158 = -100.0f;
    m_bCanAddEvent = true;
}

// 0x5B98A0
void CAEPedAudioEntity::StaticInitialise() {
    AEAudioHardware.LoadSoundBank(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC);
    AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_SWIMMING, SND_BANK_SLOT_SWIMMING);
}

// 0x4E1360
void CAEPedAudioEntity::Terminate() {
    m_bCanAddEvent = false;
    m_pPed   = nullptr;
    StopJetPackSound();
    AESoundManager.CancelSoundsOwnedByAudioEntity(this, true);
    if (m_sTwinLoopSoundEntity.IsActive()) {
        m_sTwinLoopSoundEntity.StopSoundAndForget();
    }
}

// 0x4E2BB0
void CAEPedAudioEntity::AddAudioEvent(eAudioEvents event, float volume, float speed, CPhysical* ped, eSurfaceType surfaceId, int32 a7, uint32 maxVol) {
    if (!m_bCanAddEvent)
        return;

    if (!m_pPed)
        return;

    switch (event) {
    case AE_PED_FOOTSTEP_LEFT:
    case AE_PED_FOOTSTEP_RIGHT:
        HandleFootstepEvent(event, volume, speed, surfaceId);
        break;
    case AE_PED_SKATE_LEFT:
    case AE_PED_SKATE_RIGHT:
        HandleSkateEvent(event, volume, speed);
        break;
    case AE_PED_LAND_ON_FEET_AFTER_FALL:
    case AE_PED_COLLAPSE_AFTER_FALL:
        HandleLandingEvent(event);
        break;
    case AE_PED_SWING:
        HandlePedSwing(event, a7, maxVol);
        break;
    case AE_PED_HIT_HIGH:
    case AE_PED_HIT_LOW:
    case AE_PED_HIT_GROUND:
    case AE_PED_HIT_GROUND_KICK:
    case AE_PED_HIT_HIGH_UNARMED:
    case AE_PED_HIT_LOW_UNARMED:
    case AE_PED_HIT_MARTIAL_PUNCH:
    case AE_PED_HIT_MARTIAL_KICK:
        HandlePedHit(event, ped, surfaceId, volume, maxVol);
        break;
    case AE_PED_JACKED_CAR_PUNCH:
    case AE_PED_JACKED_CAR_HEAD_BANG:
    case AE_PED_JACKED_CAR_KICK:
    case AE_PED_JACKED_BIKE:
    case AE_PED_JACKED_DOZER:
        HandlePedJacked(event);
        break;
    case AE_PED_SWIM_STROKE_SPLASH:
    case AE_PED_SWIM_DIVE_SPLASH:
        HandleSwimSplash(event);
        break;
    case AE_PED_SWIM_WAKE:
        HandleSwimWake(event);
        break;
    case AE_PED_CRUNCH: {
        if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_COLLISIONS, SND_BANK_SLOT_COLLISIONS)) {
            break;
        }

        volume += GetDefaultVolume(AE_PED_CRUNCH);

        if (AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_PED_CRUNCH, this)) {
            break;
        }
        AESoundManager.PlaySound({
            .BankSlotID        = SND_BANK_SLOT_COLLISIONS,
            .SoundID           = 29,
            .AudioEntity       = this,
            .Pos               = ped->GetPosition(),
            .Volume            = volume,
            .RollOffFactor     = 1.5f,
            .Speed             = speed,
            .Doppler           = 1.0f,
            .FrameDelay        = 0,
            .Flags             = SOUND_DEFAULT,
            .FrequencyVariance = 0.06f,
            .PlayTime          = 0,
            .EventID           = AE_PED_CRUNCH
        });

        if (AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_PED_KNOCK_DOWN, this) != 0) {
            break;
        }
        AESoundManager.PlaySound({
            .BankSlotID        = SND_BANK_SLOT_COLLISIONS,
            .SoundID           = (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(47, 49)),
            .AudioEntity       = this,
            .Pos               = ped->GetPosition(),
            .Volume            = volume,
            .RollOffFactor     = 1.5f,
            .Speed             = speed,
            .Doppler           = 1.0f,
            .FrameDelay        = 0,
            .Flags             = SOUND_DEFAULT,
            .FrequencyVariance = 0.06f,
            .PlayTime          = 0,
            .EventID           = AE_PED_KNOCK_DOWN
        });
        break;
    }
    default:
        break;
    }
}

// 0x4E28A0
void CAEPedAudioEntity::TurnOnJetPack() {
    if (!m_pPed)
        return;

    if (m_bJetPackPlaying || m_JetPackSound1 || m_JetPackSound0 || m_JetPackSound2)
        return;

    m_fVolume1 = -100.0f;
    m_fVolume2 = -100.0f;
    m_fVolume3 = +2.000f;
    m_JetPackSoundSpeedMult = 0.400f;

    m_bJetPackPlaying = true;

    const auto PlayJetPackSound = [&](eSoundBankSlot slot, eSoundID soundID) {
        return AESoundManager.PlaySound({
            .BankSlotID  = slot,
            .SoundID     = soundID,
            .AudioEntity = this,
            .Pos         = m_pPed->GetPosition(),
            .Volume      = -100.f,
            .Flags       = SOUND_REQUEST_UPDATES,
        });
    };
    m_JetPackSound0 = PlayJetPackSound(SND_BANK_SLOT_VEHICLE_GEN, 26);
    m_JetPackSound1 = PlayJetPackSound(SND_BANK_SLOT_WEAPON_GEN, 10);
    m_JetPackSound2 = PlayJetPackSound(SND_BANK_SLOT_FRONTEND_GAME, 0);
}

// 0x4E2A70
void CAEPedAudioEntity::TurnOffJetPack() {
    StopJetPackSound();
    m_bJetPackPlaying = false;
}

// 0x4E1120
void CAEPedAudioEntity::StopJetPackSound() {
    if (m_JetPackSound0) {
        m_JetPackSound0->StopSoundAndForget();
        m_JetPackSound0 = nullptr;
    }

    if (m_JetPackSound1) {
        m_JetPackSound1->StopSoundAndForget();
        m_JetPackSound1 = nullptr;
    }

    if (m_JetPackSound2) {
        m_JetPackSound2->StopSoundAndForget();
        m_JetPackSound2 = nullptr;
    }
}

// 0x4E0EE0
void CAEPedAudioEntity::UpdateJetPack(float thrustFwd, float thrustAngle) {
    if (!m_bJetPackPlaying || !m_JetPackSound1 || !m_JetPackSound0 || !m_JetPackSound2) {
        return;
    }

    if (thrustFwd <= 0.5f) { // flying
        m_fVolume1 = std::max(m_fVolume1 - 5.0f, -100.0f);
        m_fVolume2 = std::min(m_fVolume2 + 6.0f, -17.0f);
        m_fVolume3 = std::max(m_fVolume3 - 0.3f, 2.0f);
        m_JetPackSoundSpeedMult = std::max(m_JetPackSoundSpeedMult - 0.031f, 0.4f);
    } else { // idle
        m_fVolume1 = std::min(m_fVolume1 + 15.0f, -15.0f);
        m_fVolume2 = std::max(m_fVolume2 - 7.1f, -100.0f);
        m_fVolume3 = std::min(m_fVolume3 + 0.3f, 11.0f);
        m_JetPackSoundSpeedMult = std::min(m_JetPackSoundSpeedMult + 0.031f, 0.71f);
    }

    const auto angle = std::sin(thrustAngle);
    const float speed = angle < 0.0f ? -angle : angle; // maybe wrong

    // 0.0f == 0xB61384 (uninitialized)
    m_JetPackSound0->m_Volume = m_fVolume1 + 0.0f;
    m_JetPackSound0->m_Speed = speed * -0.07f + 1.0f;

    m_JetPackSound1->m_Volume = m_fVolume2 + 0.0f;
    m_JetPackSound1->m_Speed = 0.56f;

    m_JetPackSound2->m_Volume = m_fVolume3 + 0.0f;
    m_JetPackSound2->m_Speed = (speed / 5.0f + 1.0f) * m_JetPackSoundSpeedMult;
}

// 0x4E1170
void CAEPedAudioEntity::PlayWindRush(float, float) {
    // NOP
}

// 0x4E1180
void CAEPedAudioEntity::UpdateParameters(CAESound* sound, int16 curPlayPos) {
    if (sound == m_JetPackSound1) {
        if (curPlayPos == -1) {
            m_JetPackSound1 = nullptr;
            return;
        }
        if (m_pPed) {
            sound->SetPosition(m_pPed->GetPosition());
        }
        return;
    }

    if (sound == m_JetPackSound0) {
        if (curPlayPos == -1) {
            m_JetPackSound0 = nullptr;
            return;
        }
        if (m_pPed) {
            sound->SetPosition(m_pPed->GetPosition());
        }
        return;
    }

    if (sound == m_JetPackSound2) {
        if (curPlayPos == -1) {
            m_JetPackSound2 = nullptr;
            return;
        }
        if (m_pPed) {
            sound->SetPosition(m_pPed->GetPosition());
        }
        return;
    }

    // shit
    if (sound == field_150) {
        if (curPlayPos == -1) {
            field_150 = nullptr;
        }
        return;
    }

    if (curPlayPos < 0)
        return;

    switch (sound->m_Event) {
    case AE_PED_SWING:
    case AE_PED_HIT_HIGH:
    case AE_PED_HIT_LOW:
    case AE_PED_HIT_GROUND:
    case AE_PED_HIT_GROUND_KICK:
    case AE_PED_HIT_HIGH_UNARMED:
    case AE_PED_HIT_LOW_UNARMED:
    case AE_PED_HIT_MARTIAL_PUNCH:
    case AE_PED_HIT_MARTIAL_KICK:
    case AE_PED_JACKED_CAR_PUNCH:
    case AE_PED_JACKED_CAR_HEAD_BANG:
    case AE_PED_JACKED_CAR_KICK:
    case AE_PED_JACKED_BIKE:
    case AE_PED_JACKED_DOZER:
        if (CTimer::GetTimeInMS() < (uint32)sound->m_ClientVariable)
            return;
        sound->m_Speed = 1.0f;
        return;
    case AE_PED_SWIM_WAKE: {
        const auto volume = GetDefaultVolume(AE_PED_SWIM_WAKE);

        if (CTimer::GetTimeInMS() <= m_LastSwimWakeTriggerTimeMs + 100) {
            if (sound->m_Volume >= volume) {
                return;
            }

            if (sound->m_Volume + 0.6f >= volume) {
                sound->m_Volume = volume;
                return;
            }
            sound->m_Volume += 0.6f;
        } else {
            auto vol = volume - 20.0f;
            if (sound->m_Volume <= vol) {
                sound->StopSoundAndForget();
                m_LastSwimWakeTriggerTimeMs = 0;
                return;
            }
            sound->m_Volume = std::max(sound->m_Volume - 0.6f, vol);
        }

        return;
    }
    default:
        return;
    }
}

// 0x4E13A0
void CAEPedAudioEntity::HandleFootstepEvent(eAudioEvents event, float volume, float speed, eSurfaceType forcedSurfaceType) {
    volume += GetDefaultVolume(event);

    if (m_pPed->bIsInTheAir) {
        return;
    }
    volume += (float)(CAEAudioUtility::GetRandomNumberInRange(-3, 3));

    const auto PlayFootstepSound = [&] (eSoundBank bank, eSoundBankSlot slot, eSoundID sfx, int16 playTime = 0) {
        if (AEAudioHardware.EnsureSoundBankIsLoaded(bank, slot)) {
            AESoundManager.PlaySound({
                .BankSlotID         = slot,
                .SoundID            = sfx,
                .AudioEntity        = this,
                .Pos                = m_pPed->GetPosition(),
                .Volume             = volume,
                .Speed              = speed,
                .Flags              = SOUND_START_PERCENTAGE,
                .FrequencyVariance  = 0.0588f,
                .PlayTime           = playTime,
                .RegisterWithEntity = m_pPed,
            });
        }
    };

    const auto PlayRandomGenericFootstepSound = [&] () {
        PlayFootstepSound(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(1, 5)));
    };

    if (FindPlayerPed(-1) != m_pPed) {
        volume -= 3.f;
        PlayRandomGenericFootstepSound();
    } else {
        const auto TryPlayPlayerFootstepSound = [&] (eSoundBank bank, eSoundBankSlot slot, eSoundID sfx, bool needCancelSoundsInSlot = false, int16 playTime = 0) {
            if (AEAudioHardware.IsSoundBankLoaded(bank, slot)) {
                PlayFootstepSound(bank, slot, sfx, playTime);
                return true;
            } else {
                if (needCancelSoundsInSlot) {
                    if (AESoundManager.AreSoundsPlayingInBankSlot(slot) - 1 <= 1) {
                        AESoundManager.CancelSoundsInBankSlot(slot, false);
                    }
                }
                AEAudioHardware.LoadSoundBank(bank, slot);
                PlayRandomGenericFootstepSound();
                return false;
            }
        };
        if (g_surfaceInfos.IsAudioWater(forcedSurfaceType) || g_surfaceInfos.IsAudioWater(m_pPed->m_nContactSurface)) { // 0x4E1703
            TryPlayPlayerFootstepSound(SND_BANK_GENRL_SWIMMING, SND_BANK_SLOT_SWIMMING, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), false, 50);
        } else if (g_surfaceInfos.IsAudioConcrete(m_pPed->m_nContactSurface)) { // 0x4E145E
            PlayRandomGenericFootstepSound();
        } else if (g_surfaceInfos.IsAudioGrass(m_pPed->m_nContactSurface) || g_surfaceInfos.IsAudioLongGrass(m_pPed->m_nContactSurface)) { // 0x4E16BA
            TryPlayPlayerFootstepSound(SND_BANK_FEET_GRASS, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else if (g_surfaceInfos.IsAudioSand(m_pPed->m_nContactSurface)) { // 0x4E14B7
            TryPlayPlayerFootstepSound(SND_BANK_FEET_SAND, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 3)), true);
        } else if (g_surfaceInfos.IsAudioGravel(m_pPed->m_nContactSurface)) { // 0x4E1531
            TryPlayPlayerFootstepSound(SND_BANK_FEET_GRAVEL, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else if (g_surfaceInfos.IsAudioWood(m_pPed->m_nContactSurface)) { // 0x4E157D
            TryPlayPlayerFootstepSound(SND_BANK_FEET_WOOD, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else if (g_surfaceInfos.IsAudioTile(m_pPed->m_nContactSurface)) { // 0x4E15DB
            TryPlayPlayerFootstepSound(SND_BANK_FEET_TILE, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else if (g_surfaceInfos.IsAudioMetal(m_pPed->m_nContactSurface)) { // 0x4E1636
            TryPlayPlayerFootstepSound(SND_BANK_FEET_METAL, SND_BANK_SLOT_FOOTSTEPS_PLAYER, (eSoundID)(CAEAudioUtility::GetRandomNumberInRange(0, 4)), true);
        } else { // 0x4E168F
            PlayRandomGenericFootstepSound();
        }
    }
}

// 0x4E17E0
void CAEPedAudioEntity::HandleSkateEvent(eAudioEvents event, float volume, float speed) {
    if (m_pPed->bIsInTheAir) {
        return;
    }
    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC)) {
        return;
    }
    AESoundManager.PlaySound({
        .BankSlotID         = SND_BANK_SLOT_FOOTSTEPS_GENERIC,
        .SoundID            = (eSoundID)(event == AE_PED_SKATE_LEFT ? 7 : 8),
        .AudioEntity        = this,
        .Pos                = m_pPed->GetPosition(),
        .Volume             = GetDefaultVolume(event) + (float)(CAEAudioUtility::GetRandomNumberInRange(-3, 3)) + volume,
        .Speed              = speed,
        .FrequencyVariance  = 0.0588f,
        .RegisterWithEntity = m_pPed,
    });
}

// 0x4E18E0
void CAEPedAudioEntity::HandleLandingEvent(eAudioEvents event) {
    if (m_pPed->bIsInTheAir) {
        return;
    }

    const auto PlayLandingSound = [&](eSoundBankSlot slot, eSoundID soundID, float volume, int16 playPos) {
        AESoundManager.PlaySound({
            .BankSlotID = slot,
            .SoundID            = soundID,
            .AudioEntity        = this,
            .Pos                = m_pPed->GetPosition(),
            .Volume             = volume,
            .Flags              = SOUND_START_PERCENTAGE,
            .FrequencyVariance  = 0.0588f,
            .PlayTime           = playPos,
            .RegisterWithEntity = m_pPed
        });
    };
    const auto volume = GetDefaultVolume(event);
    if (g_surfaceInfos.IsAudioWater(m_pPed->m_nContactSurface)) {
        if (AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_SWIMMING, SND_BANK_SLOT_SWIMMING)) {
            PlayLandingSound(
                SND_BANK_SLOT_SWIMMING,
                CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_SWIMMING_SWIM1, SND_GENRL_SWIMMING_SWIM5),
                std::max(0.f, volume),
                50
            );
        }
    } else {
        if (AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC)) {
            PlayLandingSound(
                SND_BANK_SLOT_FOOTSTEPS_GENERIC,
                event == AE_PED_LAND_ON_FEET_AFTER_FALL ? 6 : 0,
                volume,
                0
            );
        }
    }
}

// 0x4E1A40
void CAEPedAudioEntity::HandlePedSwing(eAudioEvents event, int32 a3, uint32 volume) {
    // NOTE: `volume` is really the sound's duration in milliseconds

    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN, true)) {
        return;
    }

    const auto& weapon     = m_pPed->GetActiveWeapon();
    const auto  baseVolume = GetDefaultVolume(event);

    eSoundID soundId      = (eSoundID)-1;
    float    volumeOffset = 0.f;
    float    speed        = 1.f;

    switch (weapon.m_Type) {
    case WEAPON_UNARMED:
    case WEAPON_BRASSKNUCKLE:
    case WEAPON_CHAINSAW:
    case WEAPON_FLOWERS:
    case WEAPON_GRENADE:
    case WEAPON_TEARGAS:
    case WEAPON_MOLOTOV:
    case WEAPON_PISTOL:
    case WEAPON_PISTOL_SILENCED:
    case WEAPON_DESERT_EAGLE:
    case WEAPON_SHOTGUN:
    case WEAPON_SAWNOFF_SHOTGUN:
    case WEAPON_SPAS12_SHOTGUN:
    case WEAPON_MICRO_UZI:
    case WEAPON_MP5:
    case WEAPON_AK47:
    case WEAPON_M4:
    case WEAPON_TEC9:
    case WEAPON_COUNTRYRIFLE:
    case WEAPON_SNIPERRIFLE:
    case WEAPON_FLAMETHROWER:
    case WEAPON_MINIGUN:
    case WEAPON_REMOTE_SATCHEL_CHARGE:
    case WEAPON_DETONATOR:
    case WEAPON_SPRAYCAN:
    case WEAPON_EXTINGUISHER:
    case WEAPON_CAMERA:
    case WEAPON_NIGHTVISION:
    case WEAPON_INFRARED:
        soundId      = SND_GENRL_WEAPONS_WEAPON_SWIPE_FIST;
        volumeOffset = -6.f;
        speed        = 0.84f;
        break;
    case WEAPON_GOLFCLUB:
    case WEAPON_CANE:
        soundId = SND_GENRL_WEAPONS_CANE_SWISH;
        break;
    case WEAPON_NIGHTSTICK:
    case WEAPON_POOL_CUE:
        soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE;
        speed   = 0.84f;
        break;
    case WEAPON_KNIFE:
    case WEAPON_KATANA:
        soundId      = SND_GENRL_WEAPONS_WEAPON_SWIPE_METAL;
        volumeOffset = -3.f;
        break;
    case WEAPON_BASEBALLBAT:
    case WEAPON_SHOVEL:
        soundId = SND_GENRL_WEAPONS_WEAPON_SWIPE;
        speed   = 0.67f;
        break;
    case WEAPON_DILDO1:
    case WEAPON_DILDO2:
    case WEAPON_VIBE1:
    case WEAPON_VIBE2:
        soundId      = SND_GENRL_WEAPONS_WEAPON_SWIPE;
        volumeOffset = -6.f;
        speed        = 0.84f;
        break;
    default: // Rocket launchers & co. have no swing sound
        break;
    }

    // The previously played event overrides the weapon's sound
    switch (a3) {
    case AE_PED_HIT_GROUND_KICK:
    case AE_PED_HIT_HIGH_UNARMED:
    case AE_PED_HIT_LOW_UNARMED:
        soundId      = SND_GENRL_WEAPONS_WEAPON_SWIPE_FIST;
        volumeOffset = -6.f;
        speed        = 0.84f;
        break;
    case AE_PED_HIT_MARTIAL_PUNCH:
    case AE_PED_HIT_MARTIAL_KICK:
        soundId      = SND_GENRL_WEAPONS_WEAPON_SWIPE_FIST;
        volumeOffset = -2.f;
        speed        = 0.84f;
        break;
    default:
        if (soundId < 0) {
            return;
        }
        break;
    }

    m_tempSound.Initialise(
        SND_BANK_SLOT_WEAPON_GEN,
        soundId,
        this,
        m_pPed->GetPosition(),
        baseVolume + volumeOffset,
        1.f,   // rollOff
        speed,
        1.f,   // doppler
        0,     // frameDelay
        SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY,
        0.04f, // speedVariance
        0      // playTime
    );
    m_tempSound.RegisterWithPhysicalEntity(m_pPed);

    if (volume) { // Keep the current sound position until the event is over
        m_tempSound.m_Speed          = 0.f;
        m_tempSound.m_SpeedVariance  = 0.f;
        m_tempSound.m_ClientVariable = (float)(CTimer::GetTimeInMS() + volume);
        m_tempSound.m_Event          = event;
        m_tempSound.SetFlags(SOUND_REQUEST_UPDATES, true);
    }

    AESoundManager.RequestNewSound(&m_tempSound);
}

//! @notsa
//! @brief Volume offset (in dB) applied to a collision sound played on the given surface.
//! TODO(re): 0x4DA510 - This is an unnamed helper that lives in `AECollisionAudioEntity.cpp` (Its only caller is `HandlePedHit`).
//!           Inlined here so this file doesn't have to call unreversed code - Should be moved there once that file is reversed.
//!
//! The original computes `20 * log10(paramD * 0.01)` (As `20 * log2(x) * log10(2)`, without clamping).
static float GetSurfaceVolumeOffsetDb(eSurfaceType surface) {
    return 20.f * std::log10f((float)gCollisionLookup[surface].ParamD * 0.01f);
}

// 0x4E1CC0
void CAEPedAudioEntity::HandlePedHit(eAudioEvents event, CPhysical* physical, uint8 surfaceId, float volume, uint32 maxVol) {
    // NOTE: `maxVol` is really the sound's duration in milliseconds

    const auto& weapon     = m_pPed->GetActiveWeapon();
    const auto  baseVolume = GetDefaultVolume(event) + volume;

    float    offsetA   = 0.f;    // Volume offset of the weapon's sound ("A")
    float    offsetB   = -6.f;   // Volume offset of the event's sound ("B")
    auto     bankSlotB = SND_BANK_SLOT_WEAPON_GEN;
    float    varianceA = 0.0588f;
    eSoundID soundIdA  = (eSoundID)-1;
    eSoundID soundIdB  = (eSoundID)-1;
    bool     playA     = true;   // Cleared by the paths that only play sound "B"

    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN, true)) {
        return;
    }

    // 1. Sound "A" - Based on the weapon used
    switch (weapon.m_Type) {
    case WEAPON_UNARMED:
    case WEAPON_NIGHTVISION:
    case WEAPON_INFRARED:
        soundIdA = SND_GENRL_WEAPONS_KNUCKLE;
        break;
    case WEAPON_BRASSKNUCKLE:
    case WEAPON_GRENADE:
    case WEAPON_TEARGAS:
    case WEAPON_MOLOTOV:
    case WEAPON_PISTOL:
    case WEAPON_PISTOL_SILENCED:
    case WEAPON_DESERT_EAGLE:
    case WEAPON_SHOTGUN:
    case WEAPON_SAWNOFF_SHOTGUN:
    case WEAPON_SPAS12_SHOTGUN:
    case WEAPON_MICRO_UZI:
    case WEAPON_MP5:
    case WEAPON_AK47:
    case WEAPON_M4:
    case WEAPON_TEC9:
    case WEAPON_COUNTRYRIFLE:
    case WEAPON_SNIPERRIFLE:
    case WEAPON_FLAMETHROWER:
    case WEAPON_MINIGUN:
    case WEAPON_REMOTE_SATCHEL_CHARGE:
    case WEAPON_DETONATOR:
    case WEAPON_SPRAYCAN:
    case WEAPON_EXTINGUISHER:
    case WEAPON_CAMERA:
        soundIdA = SND_GENRL_WEAPONS_BRASS_KNUCKLES;
        break;
    case WEAPON_GOLFCLUB:
        soundIdA = SND_GENRL_WEAPONS_CLUB_STRIKE;
        break;
    case WEAPON_NIGHTSTICK:
        soundIdA = SND_GENRL_WEAPONS_BAT_STRIKE;
        break;
    case WEAPON_KNIFE:
        soundIdA = SND_GENRL_WEAPONS_KNIFE;
        varianceA = 0.03f;
        break;
    case WEAPON_BASEBALLBAT:
        soundIdA = SND_GENRL_WEAPONS_BASEBALL_BAT;
        break;
    case WEAPON_SHOVEL:
        soundIdA = SND_GENRL_WEAPONS_SHOVEL;
        break;
    case WEAPON_POOL_CUE:
        soundIdA = SND_GENRL_WEAPONS_POOLCUE;
        break;
    case WEAPON_KATANA:
        soundIdA = SND_GENRL_WEAPONS_KATANA_SWORD;
        break;
    case WEAPON_DILDO1:
    case WEAPON_DILDO2:
    case WEAPON_VIBE1:
    case WEAPON_VIBE2:
        soundIdA = SND_GENRL_WEAPONS_DILDO;
        break;
    case WEAPON_FLOWERS:
        soundIdA  = SND_GENRL_WEAPONS_FLOWER_STRIKE;
        varianceA = 0.07f;
        break;
    case WEAPON_CANE:
        soundIdA = SND_GENRL_WEAPONS_CANE_STRIKE;
        break;
    default: // Chainsaw, explosives & co. have no impact sound
        break;
    }

    // 2. Sound "B" - Based on the event (And the target, if it's a ped)
    const auto RandomBoxFaceSound = [] {
        return CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_WEAPONS_BOX_FACE1, SND_GENRL_WEAPONS_BOX_FACE4);
    };
    const auto RandomBodyPunchSound = [] {
        return CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_WEAPONS_BODY_PUNCH1, SND_GENRL_WEAPONS_BODY_PUNCH4);
    };
    const auto RandomBulletHitSound = [] {
        return CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_BULLET_HITS_BULPED1L, SND_GENRL_BULLET_HITS_BULPED3L);
    };
    const auto RandomMartialStrikeSound = [] {
        return CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_WEAPONS_MARTIAL_STRIKE1, SND_GENRL_WEAPONS_MARTIAL_STRIKE4);
    };

    const auto ReportCollision = [&] {
        AudioEngine.ReportCollision(
            m_pPed,
            physical,
            SURFACE_PED,
            (eSurfaceType)surfaceId,
            m_pPed->GetPosition(),
            nullptr,
            1.f,   // Impact
            1.f,   // Impact
            true,  // Play only one shot collision sound
            false
        );
    };

    if (!physical || !physical->GetIsTypePed()) {
        switch (event) {
        case AE_PED_HIT_HIGH:
        case AE_PED_HIT_LOW:
        case AE_PED_HIT_GROUND:
            offsetA = GetSurfaceVolumeOffsetDb((eSurfaceType)surfaceId);
            ReportCollision();
            break;
        case AE_PED_HIT_GROUND_KICK:
        case AE_PED_HIT_MARTIAL_KICK:
            ReportCollision();
            return; // No sound is played
        case AE_PED_HIT_HIGH_UNARMED:
        case AE_PED_HIT_LOW_UNARMED:
        case AE_PED_HIT_MARTIAL_PUNCH:
            soundIdA = SND_GENRL_WEAPONS_KNUCKLE;
            offsetA  = GetSurfaceVolumeOffsetDb((eSurfaceType)surfaceId);
            ReportCollision();
            break;
        default:
            break;
        }
    } else {
        switch (event) {
        case AE_PED_HIT_HIGH:
        case AE_PED_HIT_LOW:
            switch (weapon.m_Type) {
            case WEAPON_BRASSKNUCKLE:
                offsetA  = 1.f;
                soundIdB = event == AE_PED_HIT_HIGH ? RandomBoxFaceSound() : RandomBodyPunchSound();
                break;
            case WEAPON_GOLFCLUB:
            case WEAPON_NIGHTSTICK:
            case WEAPON_SHOVEL:
            case WEAPON_POOL_CUE:
            case WEAPON_DILDO1:
            case WEAPON_DILDO2:
            case WEAPON_VIBE1:
            case WEAPON_VIBE2:
            default: // Unarmed, the cane, and everything else outside of the original's jump table
                offsetA  = -15.f;
                soundIdB = event == AE_PED_HIT_HIGH ? RandomBoxFaceSound() : RandomBodyPunchSound();
                break;
            case WEAPON_KNIFE:
            case WEAPON_KATANA:
                soundIdB = SND_GENRL_WEAPONS_STAB;
                offsetB  = -9.f;
                playA    = false;
                break;
            case WEAPON_BASEBALLBAT:
                offsetA  = -10.f;
                soundIdB = event == AE_PED_HIT_HIGH ? RandomBoxFaceSound() : RandomBodyPunchSound();
                break;
            case WEAPON_CHAINSAW:
                if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_BULLET_HITS, SND_BANK_SLOT_BULLET_HITS)) {
                    return;
                }
                bankSlotB = SND_BANK_SLOT_BULLET_HITS;
                soundIdB  = RandomBulletHitSound();
                playA     = false;
                break;
            case WEAPON_FLOWERS: // No "B" sound
                break;
            }
            break;
        case AE_PED_HIT_GROUND:
            switch (weapon.m_Type) {
            case WEAPON_KNIFE:
            case WEAPON_KATANA:
                soundIdB = SND_GENRL_WEAPONS_STAB;
                offsetB  = -9.f;
                playA    = false;
                break;
            case WEAPON_BASEBALLBAT:
                soundIdB = SND_GENRL_WEAPONS_STOMP1;
                offsetA  = -10.f;
                break;
            case WEAPON_SHOVEL:
            case WEAPON_POOL_CUE:
            case WEAPON_DILDO1:
            case WEAPON_DILDO2:
            case WEAPON_VIBE1:
            case WEAPON_VIBE2:
                soundIdB = SND_GENRL_WEAPONS_STOMP1;
                offsetA  = -15.f;
                break;
            case WEAPON_CHAINSAW:
                if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_BULLET_HITS, SND_BANK_SLOT_BULLET_HITS)) {
                    return;
                }
                bankSlotB = SND_BANK_SLOT_BULLET_HITS;
                soundIdB  = RandomBulletHitSound();
                playA     = false;
                break;
            case WEAPON_FLOWERS:
                break;
            default: // Unarmed, blunt and everything else above the cane
                soundIdB = SND_GENRL_WEAPONS_STOMP1;
                offsetA  = -15.f;
                break;
            }
            break;
        case AE_PED_HIT_GROUND_KICK:
            soundIdB = SND_GENRL_WEAPONS_STOMP1;
            playA    = false;
            break;
        case AE_PED_HIT_HIGH_UNARMED:
        case AE_PED_HIT_MARTIAL_PUNCH:
            soundIdA = SND_GENRL_WEAPONS_KNUCKLE;
            soundIdB = RandomBoxFaceSound();
            break;
        case AE_PED_HIT_LOW_UNARMED:
            soundIdA = SND_GENRL_WEAPONS_KNUCKLE;
            soundIdB = RandomBodyPunchSound();
            break;
        case AE_PED_HIT_MARTIAL_KICK:
            soundIdB = RandomMartialStrikeSound();
            playA    = false;
            break;
        default:
            break;
        }
    }

    const auto PlaySound = [&](eSoundBankSlot slot, eSoundID soundId, float volume, float speedVariance) {
        m_tempSound.Initialise(
            slot,
            soundId,
            this,
            m_pPed->GetPosition(),
            volume,
            1.f, // rollOff
            1.f, // speed
            1.f, // doppler
            0,   // frameDelay
            SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY,
            speedVariance,
            0    // playTime
        );
        m_tempSound.RegisterWithPhysicalEntity(m_pPed);

        if (maxVol) { // Keep the current sound position until the event is over
            m_tempSound.m_Speed          = 0.f;
            m_tempSound.m_SpeedVariance  = 0.f;
            m_tempSound.m_ClientVariable = (float)(CTimer::GetTimeInMS() + maxVol);
            m_tempSound.m_Event          = event;
            m_tempSound.SetFlags(SOUND_REQUEST_UPDATES, true);
        }

        AESoundManager.RequestNewSound(&m_tempSound);
    };

    if (playA && soundIdA >= 0) {
        PlaySound(SND_BANK_SLOT_WEAPON_GEN, soundIdA, baseVolume + offsetA, varianceA);
    }
    if (soundIdB >= 0) {
        PlaySound(bankSlotB, soundIdB, baseVolume + offsetB, 0.0588f);
    }
}

// 0x4E2350
void CAEPedAudioEntity::HandlePedJacked(eAudioEvents event) {
    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN, true)) {
        return;
    }

    const auto volume = GetDefaultVolume(event);

    uint32 endTimeShort = CTimer::GetTimeInMS();
    uint32 endTimeLong  = endTimeShort;

    eSoundID soundId1   = (eSoundID)-1;
    eSoundID soundId2   = (eSoundID)-1;
    bool     playFirst  = false;

    if ((uint32)(event - AE_PED_JACKED_CAR_PUNCH) <= AE_PED_JACKED_DOZER - AE_PED_JACKED_CAR_PUNCH) {
        switch (event) {
        case AE_PED_JACKED_CAR_PUNCH:
            soundId1 = SND_GENRL_WEAPONS_KNUCKLE;
            soundId2 = CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_WEAPONS_BOX_FACE1, SND_GENRL_WEAPONS_BOX_FACE4);
            endTimeShort += 500;
            endTimeLong  += 2133;
            playFirst    = true;
            break;
        case AE_PED_JACKED_CAR_HEAD_BANG:
            soundId1 = SND_GENRL_WEAPONS_DILDO;
            soundId2 = CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_WEAPONS_BODY_PUNCH1, SND_GENRL_WEAPONS_BODY_PUNCH4);
            endTimeShort += 933;
            endTimeLong  += 2633;
            playFirst    = true;
            break;
        case AE_PED_JACKED_CAR_KICK:
            soundId2 = CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_WEAPONS_BOX_FACE1, SND_GENRL_WEAPONS_BOX_FACE4);
            endTimeShort += 900;
            endTimeLong  += 2800;
            break;
        case AE_PED_JACKED_BIKE:
            soundId1 = SND_GENRL_WEAPONS_KNUCKLE;
            soundId2 = CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_WEAPONS_BOX_FACE1, SND_GENRL_WEAPONS_BOX_FACE4);
            endTimeShort += 33;
            endTimeLong  += 366;
            playFirst    = true;
            break;
        case AE_PED_JACKED_DOZER:
            soundId2 = CAEAudioUtility::GetRandomNumberInRange<eSoundID>(SND_GENRL_WEAPONS_BODY_PUNCH1, SND_GENRL_WEAPONS_BODY_PUNCH4);
            endTimeShort += 266;
            endTimeLong  += 833;
            break;
        }
    }

    const auto PlaySound = [&](eSoundBankSlot slot, eSoundID soundId, uint32 endTime) {
        m_tempSound.Initialise(
            slot,
            soundId,
            this,
            m_pPed->GetPosition(),
            volume,
            2.f, // rollOff
            0.f, // speed
            1.f, // doppler
            0,   // frameDelay
            SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY | SOUND_REQUEST_UPDATES,
            0.f, // speedVariance
            0    // playTime
        );
        m_tempSound.RegisterWithPhysicalEntity(m_pPed);
        m_tempSound.m_Event          = event;
        m_tempSound.m_ClientVariable = (float)endTime;
        AESoundManager.RequestNewSound(&m_tempSound);
    };

    if (playFirst) {
        PlaySound(SND_BANK_SLOT_WEAPON_GEN, soundId1, endTimeShort);
    }
    if (soundId2 >= 0) {
        PlaySound(SND_BANK_SLOT_WEAPON_GEN, soundId2, endTimeShort);
    }

    // The (generic) impact sound, played for every event
    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_FEET_GENERIC, SND_BANK_SLOT_FOOTSTEPS_GENERIC)) {
        return;
    }
    PlaySound(SND_BANK_SLOT_FOOTSTEPS_GENERIC, (eSoundID)0, endTimeLong); // TODO(re): Name the sound ID (Unknown, it's the first one in the bank)
}

// 0x4E26A0
void CAEPedAudioEntity::HandleSwimSplash(eAudioEvents event) {
    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_SWIMMING, SND_BANK_SLOT_SWIMMING)) {
        return;
    }
    if (++m_nSfxId > 4) {
        m_nSfxId = 0;
    }
    AESoundManager.PlaySound({
        .BankSlotID         = SND_BANK_SLOT_SWIMMING,
        .SoundID            = (eSoundID)(m_nSfxId),
        .AudioEntity        = this,
        .Pos                = m_pPed->GetPosition(),
        .Volume             = GetDefaultVolume(event),
        .FrequencyVariance  = 0.0588f,
        .RegisterWithEntity = m_pPed,
    });
}

// 0x4E2790
void CAEPedAudioEntity::HandleSwimWake(eAudioEvents event) {
    if (!AEAudioHardware.EnsureSoundBankIsLoaded(SND_BANK_GENRL_COLLISIONS, SND_BANK_SLOT_COLLISIONS, true)) {
        return;
    }
    if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntityAndPhysical(event, this, m_pPed)) {
        AESoundManager.PlaySound({
            .BankSlotID         = SND_BANK_SLOT_COLLISIONS,
            .SoundID            = 3,
            .AudioEntity        = this,
            .Pos                = m_pPed->GetPosition(),
            .Volume             = GetDefaultVolume(event) - 20.0f,
            .RollOffFactor      = 1.f,
            .Speed              = 0.75f,
            .Doppler            = 1.f,
            .FrameDelay         = 0,
            .Flags              = SOUND_REQUEST_UPDATES | SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY,
            .RegisterWithEntity = m_pPed,
            .EventID            = event,
        });
    }
    m_LastSwimWakeTriggerTimeMs = CTimer::GetTimeInMS();
}

// 0x4E2A90
void CAEPedAudioEntity::PlayShirtFlap(float volume, float speed) {
    if (m_sTwinLoopSoundEntity.IsActive()) {
        m_sTwinLoopSoundEntity.UpdateTwinLoopSound(m_pPed->GetPosition(), volume, speed);
    } else {
        m_sTwinLoopSoundEntity.Initialise(SND_BANK_SLOT_WEAPON_GEN, 19, 20, this, 200, 1000, -1, -1);
        m_sTwinLoopSoundEntity.PlayTwinLoopSound(m_pPed->GetPosition(), volume, speed, 2.0f, 1.0f, SOUND_DEFAULT);
    }
}

// 0x4E2EE0
void CAEPedAudioEntity::Service() {
    float volume         = -100.f; // dB
    float speedFac       = 1.f;    // Multiplier passed to `PlayShirtFlap`
    float animSkyDive    = 0.f;    // "FALL_SkyDive_accel" blend amount
    float animParaDecel  = 0.f;    // "Para_decel" blend amount
    bool  onFoot         = true;
    bool  usingParachute = false;

    if (!m_pPed->IsPlayer()) {
        return;
    }

    if ((m_pPed->m_pAttachedTo && m_pPed->m_pAttachedTo->GetIsTypeVehicle()) || m_pPed->bInVehicle) {
        onFoot = false;
    }

    // In a bike (Or an open top automobile) while driving
    if (m_pPed->m_nPedState == PEDSTATE_DRIVING) {
        if (CVehicle* veh = m_pPed->m_pVehicle) {
            if (veh->IsSubBike() || (veh->IsSubAutomobile() && veh->IsOpenTopCar())) {
                const float speed = veh->m_vecMoveSpeed.Magnitude();
                if (speed != 0.f) {
                    const float baseVolume = veh->m_vehicleAudio.m_AuSettings.IsBMX() ? -17.f : -10.f;
                    speedFac = std::clamp(speed, 0.f, 1.f);
                    volume   = CAEAudioUtility::AudioLog10(speedFac) * 20.f + baseVolume;
                    speedFac = 0.4f * speedFac + 0.8f;
                }
            }
        }
    }

    if (m_pPed->GetIntelligence()) {
        usingParachute = m_pPed->GetIntelligence()->GetUsingParachute();
    }

    // On foot
    if (onFoot) {
        float speedRatio = m_pPed->m_vecMoveSpeed.Magnitude() / 0.7f;
        float baseVolume;
        if (!usingParachute) {
            baseVolume = -16.f;
            speedRatio = speedRatio >= 0.5f ? (speedRatio - 0.5f) / 0.5f : 0.f;
        } else {
            baseVolume = -7.f;
            if (const auto anim = RpAnimBlendClumpGetAssociation(m_pPed->GetRpClump(), "FALL_SkyDive_accel")) {
                animSkyDive = std::clamp(anim->m_BlendAmount, 0.f, 1.f);
            }
            if (const auto anim = RpAnimBlendClumpGetAssociation(m_pPed->GetRpClump(), "Para_decel")) {
                animParaDecel = std::clamp(anim->m_BlendAmount, 0.f, 1.f);
            }
        }
        speedRatio = std::clamp(speedRatio, 0.f, 1.f);
        volume     = CAEAudioUtility::AudioLog10(speedRatio) * 20.f + baseVolume;
        speedFac   = 0.8f * speedRatio + 0.8f;
    }

    // Smoothly move `field_158` towards the target volume (At most `+5`/`-2` dB per call)
    const float flapSpeedBase = 0.5f * animSkyDive;
    const float targetVolume  = -6.f * animParaDecel + 4.f * animSkyDive + volume;
    if (targetVolume <= field_158) {
        if (targetVolume != field_158) {
            field_158 = std::max(targetVolume, field_158 - 2.f);
        }
    } else {
        field_158 = std::min(targetVolume, field_158 + 5.f);
    }

    // Snap below -20dB (Bypasses the rate limit above)
    if (targetVolume < -20.f && field_158 < -20.f) {
        field_158 = targetVolume;
    }

    if (field_158 > -100.f) {
        PlayShirtFlap(field_158, (flapSpeedBase + 1.f) * speedFac);
    } else {
        m_sTwinLoopSoundEntity.StopSoundAndForget();
    }
}
