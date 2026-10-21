#include "PedToPlayerConversations.h"

#include "Conversations.h"
#include "PedClothesDesc.h"
#include "Ragdoll/IKChainManager.h"
#include "Tasks/TaskTypes/TaskComplexKillPedOnFoot.h"
#include "Tasks/TaskTypes/TaskSimpleStandStill.h"

void CPedToPlayerConversations::InjectHooks() {
    RH_ScopedClass(CPedToPlayerConversations);
    RH_ScopedCategory("Conversations");
    RH_ScopedInstall(Clear, 0x43AAE0);
    RH_ScopedInstall(Update, 0x43B0F0);
    RH_ScopedInstall(EndConversation, 0x43AB10);
}

// 0x43AAE0
void CPedToPlayerConversations::Clear() {
    ZoneScoped;

    if (m_State != eP2pState::INACTIVE) {
        CAEPedSpeechAudioEntity::ReleasePlayerConversation();
        m_State = eP2pState::INACTIVE;
    }

    m_pPed                         = nullptr;
    m_TimeOfLastPlayerConversation = 0;
}

// 0x43B0F0
void CPedToPlayerConversations::Update() {
    ZoneScoped;

    const auto player = FindPlayerPed();

    if (auto* const vehicle = FindPlayerVehicle(-1, false)) {
        m_pLastPlayerVehicle = vehicle;
        CEntity::RegisterReference(m_pLastPlayerVehicle);
    }

    //! Have the given ped open a conversation with the player (If possible)
    //! @returns If the conversation was opened
    const auto PedOpensConversation = [&](CPed* ped, bool positiveOpening, eGlobalSpeechContext ctx) {
        if (!CAEPedSpeechAudioEntity::RequestPlayerConversation(ped)) {
            return false;
        }

        m_bPositiveOpening = positiveOpening;
        ped->Say(ctx, 0, 1.0f, true, false, false);

        m_State = eP2pState::PEDHASOPENED;
        m_pPed  = ped;
        CEntity::RegisterReference(m_pPed);

        m_TimeOfLastPlayerConversation = m_StartTime = CTimer::GetTimeInMS();

        ped->DisablePedSpeech(false);
        g_ikChainMan.LookAt("Ped2Pl_Conversation",  ped,    player, 100'000, BONE_HEAD, nullptr, false, 0.25f, 500, 8, false);
        g_ikChainMan.LookAt("Ped2Pl_ConversationP", player, ped,    100'000, BONE_HEAD, nullptr, false, 0.25f, 500, 8, false);
        return true;
    };

    //! Make the ped attack the player
    const auto PedAttacksPlayer = [&] {
        auto* const sequence = new CTaskComplexSequence();
        sequence->AddTask(new CTaskSimpleStandStill(2000, false, false, 8.0f));
        sequence->AddTask(new CTaskComplexKillPedOnFoot(player, -1, 0, 0, 0, 0));

        CEventScriptCommand event(TASK_PRIMARY_PRIMARY, sequence, false); // Deletes `sequence` when destructed
        m_pPed->GetEventGroup().Add(&event, false);
    };

    //! The stats the peds comment on are randomized (Per ped)
    const auto RandomizedStat = [](CPed* ped, eStats stat, uint16 mask, float bias) {
        return static_cast<int32>(CStats::GetStatValue(stat) + (float)(ped->m_nRandomSeed & mask) - bias);
    };

    switch (m_State) {
    case eP2pState::INACTIVE: {
        if (CTimer::GetTimeInMS() <= m_TimeOfLastPlayerConversation + 30'000 // Don't talk too often
            || FindPlayerVehicle(-1, false)                                  // Nor while in a vehicle
            || player->GetWantedLevel() != eWantedLevel::WANTED_CLEAN        // Nor while wanted
            || CConversations::IsConversationGoingOn()                       // Nor over other conversations
            || !player->PedIsReadyForConversation(true)                      // Nor while busy
        ) {
            break;
        }

        // Look for a ped the player could talk to (A few peds per frame)
        const auto pool     = GetPedPool();
        const auto poolSize = static_cast<int32>(pool->GetSize());
        for (auto i = 0; i < 8; i++) {
            if (++m_ScanIndex >= poolSize) {
                m_ScanIndex = 0;
            }

            CPed* const ped = pool->GetAt(m_ScanIndex);
            if (!ped
                || !ped->IsCreatedBy(PED_GAME) // Only ambient peds
                || !ped->PedIsReadyForConversation(true)
                || ped->bHasAScriptBrain // Script driven peds don't chat
                || !ped->GetIsOnScreen()
            ) {
                continue;
            }

            const auto pedPos    = ped->GetPosition();
            const auto playerPos = player->GetPosition();
            if (!(DotProduct(ped->GetForwardVector(), playerPos - pedPos) > 0.0f)       // The ped has to face the player
                || !(DotProduct(player->GetForwardVector(), pedPos - playerPos) > 0.0f) // And the player, the ped
                || (pedPos - FindPlayerCoors()).Magnitude() >= 7.0f                     // And they have to be close to each other
            ) {
                continue;
            }

            // Cops and most gang members talk about other things than the rest
            const auto pedType = ped->m_nPedType;
            m_Topic = (pedType == PED_TYPE_COP || (pedType > PED_TYPE_GANG1 && pedType <= PED_TYPE_GANG9))
                ? CGeneral::GetRandomNumberInRange<int32>(8, 10)
                : CGeneral::GetRandomNumberInRange<int32>(0, 7);

            if (!ped->GetSpeechAE().WillPedChatAboutTopic(static_cast<int16>(m_Topic))) {
                continue;
            }

            switch (m_Topic) {
            case 0: { // Car
                if (!m_pLastPlayerVehicle || (m_pLastPlayerVehicle->GetPosition() - pedPos).Magnitude2D() >= 20.0f) {
                    break;
                }
                switch (CModelInfo::GetVehicleModelInfo(m_pLastPlayerVehicle->m_nModelIndex)->m_nVehicleClass) {
                case VEHICLE_CLASS_POORFAMILY:
                case VEHICLE_CLASS_WORKER:
                case VEHICLE_CLASS_BIG:
                case VEHICLE_CLASS_TAXI:
                    if (PedOpensConversation(ped, false, CTX_GLOBAL_CONV_DISL_CAR)) {
                        return;
                    }
                    break;
                case VEHICLE_CLASS_RICHFAMILY:
                case VEHICLE_CLASS_EXECUTIVE:
                case VEHICLE_CLASS_LEISUREBOAT:
                    if (m_pLastPlayerVehicle->m_fHealth > 800.0f && PedOpensConversation(ped, true, CTX_GLOBAL_CONV_LIKE_CAR)) {
                        return;
                    }
                    break;
                }
                break;
            }
            case 1: { // Clothes
                const auto clothesRespect = RandomizedStat(ped, STAT_CLOTHES_RESPECT, 0x1FF, 256.0f);
                if (clothesRespect > 500) {
                    if (PedOpensConversation(ped, true, CTX_GLOBAL_CONV_LIKE_CLOTHES)) return;
                } else if (clothesRespect < 150) {
                    if (PedOpensConversation(ped, false, CTX_GLOBAL_CONV_DISL_CLOTHES)) return;
                }
                break;
            }
            case 2: { // Hair
                const auto clothesRespect = RandomizedStat(ped, STAT_CLOTHES_RESPECT, 0x1FF, 256.0f);
                if (!player->GetClothesDesc()->HasVisibleNewHairCut(0)) {
                    break;
                }
                if (clothesRespect > 500) {
                    if (PedOpensConversation(ped, true, CTX_GLOBAL_CONV_LIKE_HAIR)) return;
                } else if (clothesRespect < 150 || CStats::GetStatValue(STAT_HAIRDRESSING_BUDGET) < 5.0f) {
                    if (PedOpensConversation(ped, false, CTX_GLOBAL_CONV_DISL_HAIR)) return;
                }
                break;
            }
            case 3: { // Physique
                const auto muscle = RandomizedStat(ped, STAT_MUSCLE, 0xFF, 128.0f);
                const auto fat    = RandomizedStat(ped, STAT_FAT,    0xFF, 128.0f);
                if (muscle > 400 && fat < 250) {
                    if (PedOpensConversation(ped, true, CTX_GLOBAL_CONV_LIKE_PHYS)) return;
                } else if (muscle - fat < 200 && fat > 500) {
                    if (PedOpensConversation(ped, false, CTX_GLOBAL_CONV_DISL_PHYS)) return;
                }
                break;
            }
            case 4: { // Shoes
                const auto clothesRespect = RandomizedStat(ped, STAT_CLOTHES_RESPECT, 0x3FF, 512.0f);
                if (clothesRespect > 700) {
                    if (PedOpensConversation(ped, true, CTX_GLOBAL_CONV_LIKE_SHOES)) return;
                } else if (clothesRespect < 300) {
                    if (PedOpensConversation(ped, false, CTX_GLOBAL_CONV_DISL_SHOES)) return;
                }
                break;
            }
            case 5: { // Smell
                const auto sexAppeal = RandomizedStat(ped, STAT_SEX_APPEAL, 0x1FF, 256.0f);
                if (sexAppeal > 700) {
                    if (PedOpensConversation(ped, true, CTX_GLOBAL_CONV_LIKE_SMELL)) return;
                } else if (sexAppeal < 300) {
                    if (PedOpensConversation(ped, false, CTX_GLOBAL_CONV_DISL_SMELL)) return;
                }
                break;
            }
            case 6: { // Tattoo
                if (!player->GetClothesDesc()->HasVisibleTattoo()) {
                    break;
                }
                const auto rating = CGeneral::GetRandomNumberInRange<int32>(0, 1000);
                if (rating > 700) {
                    if (PedOpensConversation(ped, true, CTX_GLOBAL_CONV_LIKE_TATTOO)) return;
                } else if (rating < 300) {
                    if (PedOpensConversation(ped, false, CTX_GLOBAL_CONV_DISL_TATTOO)) return;
                }
                break;
            }
            case 7: { // Weather
                if (CWeather::Wind >= 0.1f) {
                    if (CWeather::Wind > 0.5f && PedOpensConversation(ped, false, CTX_GLOBAL_CONV_DISL_WEATHER)) {
                        return;
                    }
                } else if (CClock::ms_nGameClockHours > 6 && CClock::ms_nGameClockHours < 20) {
                    if (PedOpensConversation(ped, true, CTX_GLOBAL_CONV_LIKE_WEATHER)) return;
                }
                break;
            }
            case 8: // Where are you from
                if (PedOpensConversation(ped, true, CTX_GLOBAL_WHERE_YOU_FROM)) return;
                break;
            case 9: // Gangbang
                if (PedOpensConversation(ped, true, CTX_GLOBAL_GANGBANG)) return;
                break;
            }

            // Rarely, the ped just comments on the player looking at them
            if ((rand() & 0xFFF) == 3) {
                ped->Say(CTX_GLOBAL_EYEING_PLAYER, 0, 1.0f, false, false, false);
                return;
            }
        }
        break;
    }
    case eP2pState::PEDHASOPENED: {
        if (!m_pPed) {
            m_State = eP2pState::INACTIVE;
            CAEPedSpeechAudioEntity::ReleasePlayerConversation();
            if (g_ikChainMan.IsLooking(player)) {
                g_ikChainMan.AbortLookAt(player, 250);
            }
            break;
        }

        if (CTimer::GetTimeInMS() > m_StartTime + 4000) { // The player took too long to reply
            m_State = eP2pState::INACTIVE;
            CAEPedSpeechAudioEntity::ReleasePlayerConversation();
            m_pPed->EnablePedSpeech();
            if (g_ikChainMan.IsLooking(player)) {
                g_ikChainMan.AbortLookAt(player, 250);
            }
            if (g_ikChainMan.IsLooking(m_pPed)) {
                g_ikChainMan.AbortLookAt(m_pPed, 250);
                if (m_bPositiveOpening) {
                    m_pPed->Say(CTX_GLOBAL_CONV_IGNORED, 0, 1.0f, false, false, false);
                }
            }
            return;
        }

        if (CPad::GetPad()->ConversationYesJustDown()) {
            if (m_bPositiveOpening) {
                switch (m_Topic) {
                case 0: player->Say(CTX_GLOBAL_LIKE_CAR_REPLY,     0, 1.0f, true, false, false); break;
                case 1: player->Say(CTX_GLOBAL_LIKE_CLOTHES_REPLY, 0, 1.0f, true, false, false); break;
                case 2: player->Say(CTX_GLOBAL_LIKE_HAIR_REPLY,    0, 1.0f, true, false, false); break;
                case 3: player->Say(CTX_GLOBAL_LIKE_PHYS_REPLY,    0, 1.0f, true, false, false); break;
                case 4: player->Say(CTX_GLOBAL_LIKE_SHOES_REPLY,   0, 1.0f, true, false, false); break;
                case 5: player->Say(CTX_GLOBAL_LIKE_SMELL_REPLY,   0, 1.0f, true, false, false); break;
                case 6: player->Say(CTX_GLOBAL_LIKE_TATTOO_REPLY,  0, 1.0f, true, false, false); break;
                case 7: player->Say(CTX_GLOBAL_WEATHER_LIKE_REPLY, 0, 1.0f, true, false, false); break;
                case 8:
                case 9:
                    player->Say(
                        m_Topic == 8 ? CTX_GLOBAL_WHERE_YOU_FROM_POS : CTX_GLOBAL_GANGBANG_YES,
                        0, 1.0f, true, false, false
                    );
                    if (CGeneral::GetRandomNumberInRange<int32>(0, 100) < 75) {
                        PedAttacksPlayer();
                    }
                    EndConversation();
                    return;
                }
            } else {
                player->Say(
                    IsPedTypeFemale(m_pPed->m_nPedType) ? CTX_GLOBAL_LIKE_DISMISS_FEMALE : CTX_GLOBAL_LIKE_DISMISS_MALE,
                    0, 1.0f, true, false, false
                );
            }
            m_State     = eP2pState::WAITINGTOFINISH;
            m_StartTime = CTimer::GetTimeInMS();
            return;
        }

        if (CPad::GetPad()->ConversationNoJustDown()) {
            if (!m_bPositiveOpening) {
                player->Say(
                    IsPedTypeFemale(m_pPed->m_nPedType) ? CTX_GLOBAL_LIKE_NEGATIVE_FEMALE : CTX_GLOBAL_LIKE_NEGATIVE_MALE,
                    0, 1.0f, true, false, false
                );
                if (m_pPed->m_nPedType != PED_TYPE_GANG1 && CGeneral::GetRandomNumberInRange<int32>(0, 100) < 40) {
                    PedAttacksPlayer();
                }
                m_StartTime = CTimer::GetTimeInMS();
                m_State     = eP2pState::WAITINGTOFINISH;
                return;
            }
            switch (m_Topic) {
            case 8:
                player->Say(CTX_GLOBAL_WHERE_YOU_FROM_NEG, 0, 1.0f, true, false, false);
                EndConversation();
                return;
            case 9:
                player->Say(CTX_GLOBAL_GANGBANG_NO, 0, 1.0f, true, false, false);
                EndConversation();
                return;
            default:
                player->Say(
                    IsPedTypeFemale(m_pPed->m_nPedType) ? CTX_GLOBAL_LIKE_DISMISS_FEMALE : CTX_GLOBAL_LIKE_DISMISS_MALE,
                    0, 1.0f, true, false, false
                );
                m_StartTime = CTimer::GetTimeInMS();
                m_State     = eP2pState::WAITINGFORFINALWORD;
                return;
            }
        }
        break;
    }
    case eP2pState::WAITINGFORFINALWORD: {
        if (!m_pPed) {
            m_State = eP2pState::INACTIVE;
            CAEPedSpeechAudioEntity::ReleasePlayerConversation();
            if (g_ikChainMan.IsLooking(player)) {
                g_ikChainMan.AbortLookAt(player, 250);
            }
            break;
        }

        if (CTimer::GetTimeInMS() <= m_StartTime + 3000) {
            break;
        }

        if (m_pPed->GetPedTalking()) { // Wait for the ped to finish talking
            break;
        }

        m_pPed->Say(CTX_GLOBAL_LIKE_DISMISS_REPLY, 0, 1.0f, true, false, false);
        m_StartTime = CTimer::GetTimeInMS();
        m_State     = eP2pState::WAITINGTOFINISH;
        return;
    }
    case eP2pState::WAITINGTOFINISH:
        if (CTimer::GetTimeInMS() <= m_StartTime + 2500 && m_pPed) {
            return;
        }
        EndConversation();
        break;
    }
}

// 0x43AB10
void CPedToPlayerConversations::EndConversation() {
    m_State           = eP2pState::INACTIVE;

    CAEPedSpeechAudioEntity::ReleasePlayerConversation();

    if (m_pPed) {
        m_pPed->EnablePedSpeech();
    }

    const auto player = FindPlayerPed(-1);
    if (g_ikChainMan.IsLooking(player)) {
        g_ikChainMan.AbortLookAt(player, 250);
    }

    if (m_pPed && g_ikChainMan.IsLooking(m_pPed)) {
        return g_ikChainMan.AbortLookAt(m_pPed, 250);
    }
}
