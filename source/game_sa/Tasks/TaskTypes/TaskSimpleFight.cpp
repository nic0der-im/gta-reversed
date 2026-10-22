#include "StdInc.h"

#include "TaskSimpleFight.h"

#include "Game.h"
#include "Crime.h"
#include "Fx.h"
#include "RwHelper.h"

namespace {
//! Converts the frame counts used by `melee.dat` to seconds
constexpr float MELEE_FRAMES_TO_SECONDS = 1.f / 30.f;
}

// 0x61C470
CTaskSimpleFight::CTaskSimpleFight(CEntity* entity, int32 nCommand, uint32 nIdlePeriod) : CTaskSimple() {
    m_bIsFinished        = false;
    m_bIsInControl       = true;
    m_bAnimsReferenced   = false;
    m_nRequiredAnimGroup = ANIM_GROUP_MELEE_1;
    m_nIdleCounter       = 0;
    m_nContinueStrike    = 0;
    m_nChainCounter      = 0;
    m_pTargetEntity      = entity;
    m_pAnim              = nullptr;
    m_pIdleAnim          = nullptr;
    m_nComboSet          = 1;
    m_nCurrentMove       = static_cast<eFightAttackType>(-1);
    m_nNextCommand       = (uint8)nCommand;
    m_nLastCommand       = 0;

    if (entity) {
        entity->RegisterReference(&m_pTargetEntity);
    }

    m_nIdlePeriod = (uint16)std::min(nIdlePeriod, 60'000u);
}

// 0x61C530
CTaskSimpleFight::~CTaskSimpleFight() {
    if (m_pTargetEntity) {
        m_pTargetEntity->CleanUpOldReference(&m_pTargetEntity);
    }
    if (m_pAnim) {
        m_pAnim->SetDefaultDeleteCallback();
    }
    if (m_pIdleAnim) {
        m_pIdleAnim->SetDefaultDeleteCallback();
    }
    if (m_bAnimsReferenced && m_nRequiredAnimGroup != ANIM_GROUP_MELEE_1) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_nRequiredAnimGroup));
        m_bAnimsReferenced = false;
    }
}

// 0x61C5E0
bool CTaskSimpleFight::ControlFight(CEntity* entity, uint8 command) {
    m_bIsInControl = true;

    if (entity != m_pTargetEntity) {
        if (m_pTargetEntity) {
            m_pTargetEntity->CleanUpOldReference(&m_pTargetEntity);
        }
        m_pTargetEntity = entity;
        if (entity) {
            entity->RegisterReference(&m_pTargetEntity);
        }
    }

    // NOTE: Signed comparison (A command `>= 0x80` never replaces a smaller one)
    if ((int8)m_nNextCommand < (int8)command) {
        m_nNextCommand = command;
    }

    return true;
}

// 0x4ABDC0
bool CTaskSimpleFight::IsComboSet() {
    const auto idx = std::max<int32>(m_nComboSet - (int32)MELEE_COMBO_UNARMED_1, 0);
    return ((uint32)m_aComboData[idx].m_wFlags & (0x10u << (m_nNextCommand & 0x1F))) != 0;
}

// 0x4ABDF0
bool CTaskSimpleFight::IsHitComboSet() {
    // NOTE: Signed comparison (A command `>= 0x80` is considered negative)
    if ((int8)m_nNextCommand < 3) {
        const auto idx = std::max<int32>(m_nComboSet - (int32)MELEE_COMBO_UNARMED_1, 0);
        if ((uint32)m_aComboData[idx].m_wFlags & (0x1000u << (m_nNextCommand & 0x1F))) {
            return true;
        }
    }
    return false;
}

// 0x4ABDA0
AssocGroupId CTaskSimpleFight::GetComboAnimGroupID() {
    return m_aComboData[std::max<int32>(m_nComboSet - (int32)MELEE_COMBO_UNARMED_1, 0)].m_nAnimGroup;
}

// 0x5BD360
uint8 CTaskSimpleFight::GetHitLevel(const char* level) {
    switch (level[0]) {
    case 'H': return 0; // `eMeleeHitLevel::HIGH`
    case 'L': return 1; // `eMeleeHitLevel::LOW`
    case 'G': return 2; // `eMeleeHitLevel::GROUND`
    case 'B': return 3; // `eMeleeHitLevel::BEHIND`
    }

    // NOTE: Retail still compares multi-character literals (`'HL'`, `'LL'`, `'GL'`) against the single
    // character it read above - Those can never match, so the levels 4, 5 and 6 are unreachable.
    return 7; // `eMeleeHitLevel::NUM` (AKA "No level")
}

// 0x5BD3B0
int16 CTaskSimpleFight::GetHitSound(int32 id) {
    switch (id) {
    case 1: return 0x3D;
    case 2: return 0x3E;
    case 3: return 0x3F;
    case 4: return 0x40;
    case 5: return 0x41;
    case 6: return 0x42;
    case 7: return 0x43;
    case 8: return 0x44;
    default: return 0x3E; // Also the value for `2`
    }
}

// 0x5BEDC0
void CTaskSimpleFight::LoadMeleeData() {
    ZoneScoped;

    //> 0x5BEDE0 - Reset every combo to its defaults
    for (auto& combo : m_aComboData) {
        combo.m_nAnimGroup      = ANIM_GROUP_MELEE_1;
        combo.m_fRanges         = 1.5f;
        rng::fill(combo.m_fHit, 100.f);
        rng::fill(combo.m_fChain, 100.f);
        rng::fill(combo.m_fRadius, 1.f);
        combo.m_fGroundLoop     = 0.f;
        combo.m_fBlockLoopStart = 100.f;
        combo.m_fBlockLoopEnd   = 100.f;
        rng::fill(combo.m_nHitLevel, 7); // `eMeleeHitLevel::NUM`
        rng::fill(combo.m_nDamage, 0);
        rng::fill(combo.m_Hit, 0);
        rng::fill(combo.m_AltHit, 0);
        combo.m_wFlags          = 0;
    }

    //> 0x5BEE38 - Reset the per level hit offsets
    rng::fill(m_aHitOffset, CVector{0.f, 0.75f, 0.f});

    enum eSection {
        SECTION_NONE,
        SECTION_COMBO,
        SECTION_LEVELS,
    };

    //> 0x5BEF2E - Parse `DATA\melee.dat`
    eSection section     = SECTION_NONE;
    uint32   comboIdx    = 0;
    uint32   levelIdx    = 0;
    uint32   comboField  = 0;

    char fieldName[32];
    char fieldValue[32];

    const auto file = CFileMgr::OpenFile("DATA\\melee.dat", "rb");
    for (auto line = CFileLoader::LoadLine(file); line; line = CFileLoader::LoadLine(file)) {
        if (!line[0] || line[0] == '#') {
            continue;
        }

        const auto LineStartsWith = [line](const char* prefix) {
            return !strncmp(line, prefix, strlen(prefix));
        };

        if (LineStartsWith("END_MELEE_DATA")) {
            break;
        }

        if (section == SECTION_NONE) {
            if (LineStartsWith("START_COMBO")) {
                section = SECTION_COMBO;
            } else if (LineStartsWith("START_LEVELS")) {
                section = SECTION_LEVELS;
            }
            continue;
        }

        if (LineStartsWith("END_COMBO")) { //!< Yes, this also ends the levels section
            if (section == SECTION_COMBO) {
                ++comboIdx;
                comboField = 0;
            }
            section = SECTION_NONE;
            continue;
        }

        if (section == SECTION_LEVELS) {
            auto& offset = m_aHitOffset[levelIdx++];
            VERIFY(sscanf_s(
                line,
                "%s %f %f %f",
                SCANF_S_STR(fieldName), &offset.x, &offset.y, &offset.z
            ) == 4);
        } else { // SECTION_COMBO
            auto& combo = m_aComboData[comboIdx];

            switch (++comboField) {
            case 1: { // ANIMGROUP
                VERIFY(sscanf_s(
                    line,
                    "%s %s",
                    SCANF_S_STR(fieldName), SCANF_S_STR(fieldValue)
                ) == 2);

                // Retail does a case-sensitive search over the anim group names, and leaves the
                // default group in place if there's no group with that name.
                for (auto g = 0u; g < CAnimManager::GetAssocGroupDefs().size(); g++) {
                    if (!strcmp(CAnimManager::GetAnimGroupName((AssocGroupId)g), fieldValue)) {
                        combo.m_nAnimGroup = (AssocGroupId)g;
                        break;
                    }
                }
                break;
            }
            case 2: { // RANGES
                VERIFY(sscanf_s(
                    line,
                    "%s %f",
                    SCANF_S_STR(fieldName), &combo.m_fRanges
                ) == 2);
                break;
            }
            case 3: case 4: case 5: case 6: case 7: { // ATTACK1, ATTACK2, ATTACK3, AGROUND, AMOVING
                float hit, chain, radius;
                float groundLoop = 0.f; // Optional
                char  hitLevel[32];
                int32 damage, hitSound, altHitSound;

                VERIFY(sscanf_s(
                    line,
                    "%s %f %f %f %s %d %d %d %f",
                    SCANF_S_STR(fieldName), &hit, &chain, &radius, SCANF_S_STR(hitLevel), &damage, &hitSound, &altHitSound, &groundLoop
                ) >= 8);

                const auto move = comboField - 3; //!< 0-based index of the move this line describes

                combo.m_fHit[move]    = hit * MELEE_FRAMES_TO_SECONDS;
                combo.m_fChain[move]  = chain * MELEE_FRAMES_TO_SECONDS;
                combo.m_fRadius[move] = radius;
                combo.m_nHitLevel[move] = (int8)GetHitLevel(hitLevel);
                combo.m_nDamage[move]   = (uint8)damage;
                combo.m_Hit[move]       = GetHitSound(hitSound);
                combo.m_AltHit[move]    = GetHitSound(altHitSound);
                if (groundLoop > 0.f) {
                    combo.m_fGroundLoop = groundLoop * MELEE_FRAMES_TO_SECONDS;
                }
                break;
            }
            case 8: { // ABLOCK
                float loopStart, loopEnd;
                VERIFY(sscanf_s(
                    line,
                    "%s %f %f",
                    SCANF_S_STR(fieldName), &loopStart, &loopEnd
                ) == 3);
                combo.m_fBlockLoopStart = loopStart * MELEE_FRAMES_TO_SECONDS;
                combo.m_fBlockLoopEnd   = loopEnd * MELEE_FRAMES_TO_SECONDS;
                break;
            }
            case 9: { // FLAGS
                uint32 flags;
                VERIFY(sscanf_s(
                    line,
                    "%s %x",
                    SCANF_S_STR(fieldName), &flags
                ) == 2);
                combo.m_wFlags = (uint16)flags;
                break;
            }
            default:
                break; //!< Unknown field, ignore
            }
        }
    }
    CFileMgr::CloseFile(file);
}

// 0x6239F0
bool CTaskSimpleFight::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    switch (priority) {
    case ABORT_PRIORITY_URGENT:
    case ABORT_PRIORITY_IMMEDIATE: {
        // Only abort for events of a high enough priority (`60` being excluded separately)
        if (event && (event->GetEventPriority() <= 31 || event->GetEventPriority() == 60)) {
            return false;
        }

        if (m_pAnim) {
            if (priority == ABORT_PRIORITY_IMMEDIATE) {
                m_pAnim->SetBlendDelta(-1000.f);
            }
            m_pAnim->SetDefaultDeleteCallback();
            m_pAnim = nullptr;
        }

        if (m_pIdleAnim) {
            m_pIdleAnim->SetDefaultDeleteCallback();
            if (m_pIdleAnim->m_BlendAmount > 0.f && m_pIdleAnim->m_BlendDelta >= 0.f) {
                CAnimManager::BlendAnimation(
                    ped->GetRpClump(),
                    ped->m_nAnimGroup,
                    ANIM_ID_IDLE,
                    priority == ABORT_PRIORITY_IMMEDIATE ? 1000.f : 16.f
                );
            }
            m_pIdleAnim = nullptr;
        }

        if (ped && ped->IsPlayer()) {
            ped->GetPlayerData()->m_vecFightMovement = {0.f, 0.f};
            // TODO(re: `SetPlayerMoveAnim` (0x61C9B0, takes `ped->AsPlayer()`) is not reversed yet,
            // so the call at 0x623AFB can't be made here. It reblends the fighting-shuffle anims.)
        }

        m_bIsFinished = true;
        return true;
    }
    default: { // ABORT_PRIORITY_LEISURE (0) and anything else
        m_nNextCommand = 1; //!< AKA `eMeleeCommand::END_SLOW`
        return false;
    }
    }
}

// 0x629920
//! NOTE: Only the prologue up to `0x629B00` is established (See `.local/re/0x629920.md`). The rest of
//! the function (`0x629B9B..0x62A092`) does the animation/attack processing and depends on
//! `GetAvailableComboSet` (0x61C7F0), `SetPlayerMoveAnim` (0x61C9B0), `GetStrikeDamage` (0x61C740),
//! `GetRange` (0x61C1C0) and `FightSetUpCol` (0x623B10), none of which are reversed yet.
bool CTaskSimpleFight::ProcessPed(CPed* ped) {
    if (m_bIsFinished) { // 0x629927
        if (m_pIdleAnim) {
            m_pIdleAnim->SetDefaultDeleteCallback();
            if (m_pIdleAnim->m_BlendAmount > 0.f && m_pIdleAnim->m_BlendDelta >= 0.f) {
                CAnimManager::BlendAnimation(ped->GetRpClump(), ped->m_nAnimGroup, ANIM_ID_IDLE, 8.f);
            }
            m_pIdleAnim = nullptr;
        }
        return true;
    }

    // 0x629990 - Idle counter
    if (m_nComboSet != 0 && m_bIsInControl) {
        m_nIdleCounter = 0;
    } else {
        m_nIdleCounter += (uint16)(int32)(CTimer::GetTimeStep() * 0.02f * 1000.f);
    }

    // 0x6299BF
    if (!m_bIsInControl) {
        if ((int8)m_nNextCommand < 1) {
            return false;
        }
        if (m_pAnim && m_pAnim->GetAnimId() == ANIM_ID_FIGHT2IDLE) {
            return false;
        }
    }

    // 0x6299E2
    if (!m_pIdleAnim) {
        switch (m_nLastCommand) {
        case 1:    // `eMeleeCommand::END_SLOW`
        case 0x0F: // `eMeleeCommand::END_QUICK`
        case 0x10: // `eMeleeCommand::END_RUNAWAY`
        case 0x11: // `eMeleeCommand::END_SPRINTAWAY`
            m_bIsFinished = true;
            break;
        }
    }

    // TODO(re: 0x629A13-0x62A092 - Weapon combo set selection and the whole animation/attack
    // processing. Not established by the brief, and it's unreachable to implement without the
    // functions listed above.)
    return true;
}

void CTaskSimpleFight::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleFight, 0x86D684, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(ControlFight, 0x61C5E0);
    RH_ScopedInstall(IsComboSet, 0x4ABDC0);
    RH_ScopedInstall(IsHitComboSet, 0x4ABDF0);
    RH_ScopedInstall(GetComboAnimGroupID, 0x4ABDA0);
    RH_ScopedInstall(LoadMeleeData, 0x5BEDC0);
}

CTaskSimpleFight* CTaskSimpleFight::Constructor(CEntity* entity, int32 nCommand, uint32 nIdlePeriod) {
    this->CTaskSimpleFight::CTaskSimpleFight(entity, nCommand, nIdlePeriod);
    return this;
}

CTaskSimpleFight* CTaskSimpleFight::Destructor() {
    this->CTaskSimpleFight::~CTaskSimpleFight();
    return this;
}

// 0x61D5F0
void CTaskSimpleFight::FightSetUpCol(float radius) {
    auto& col = col1[0];
    if (!col.m_pColData) {
        // NOTE: The vanilla code points the model's data at a *static*, shared single-sphere
        // `CCollisionData` (0xC17854, its sphere array at 0xC17884) and never initializes that
        // sphere, so in practice only the model's bound box is meaningful.
        col.AllocateData(1, 0, 0, 0, 0, false);
        col.m_pColData->m_pSpheres[0] = CColSphere{};
    }

    col.m_boundBox.m_vecMin    = CVector{ -radius, -radius, -radius };
    col.m_boundBox.m_vecMax    = CVector{  radius,  radius,  radius };
    col.m_boundSphere.m_vecCenter = CVector{};
    col.m_boundSphere.m_fRadius   = radius;
}

// 0x61D0B0
void CTaskSimpleFight::FightHitCar(CPed* ped, CVehicle* vehicle, CVector& posn1, CVector& posn2, int16 damageComponent, int8 surfaceType) {
    // TODO(re: 0x61C740 `GetStrikeDamage` is not reversed (and the declaration in the header says it
    // returns `void`, while the exe returns the damage through ST0), so only the base per-move damage
    // read out of `melee.dat` is used here.
    const auto strikeDamage = (float)m_aComboData[
        std::max<int32>(m_nComboSet - (int32)MELEE_COMBO_UNARMED_1, 0)
    ].m_nDamage[m_nCurrentMove];

    const auto weaponType = ped->GetActiveWeapon().m_Type;
    const auto handling   = vehicle->m_pHandlingData;

    if (weaponType == WEAPON_CHAINSAW) {
        // A chainsaw also throws sparks off the attacker's right hand
        const auto hierarchy = GetAnimHierarchyFromSkinClump(ped->GetRpClump());
        const auto& handMat  = RpHAnimHierarchyGetMatrixArray(hierarchy)[RpHAnimIDGetIndex(hierarchy, BONE_R_HAND)];
        g_fx.AddSparks(
            posn1,
            { handMat.up.x, handMat.up.y, handMat.up.z },
            5.f, 32, {}, SPARK_PARTICLE_SPARK, 0.3f, 1.f
        );
        vehicle->VehicleDamage(
            handling->m_fMass * strikeDamage * 0.00075f,
            (eVehicleCollisionComponent)damageComponent, ped, &posn1, nullptr, WEAPON_UNARMED
        );
    } else {
        vehicle->VehicleDamage(
            handling->m_fMass * strikeDamage * 0.01f,
            (eVehicleCollisionComponent)damageComponent, ped, &posn1, nullptr, WEAPON_UNARMED
        );
    }

    CCrime::ReportCrime(CRIME_HIT_CAR, vehicle, ped);

    // TODO(re: Every occupant (the driver, then `m_nMaxPassengers` passengers) is meant to receive a
    // `CEventVehicleDamage(vehicle, ped, WEAPON_BASEBALLBAT)` on their event group here, but the repo's
    // `CEventVehicleDamage` is abstract (it doesn't override the pure virtual of
    // `CEventEditableResponse`), so it cannot be constructed yet. That belongs to
    // `source/game_sa/Events/EventVehicleDamage.cpp`, which this task doesn't own.

    if (vehicle->m_fHealth < strikeDamage) {
        vehicle->m_nLastWeaponDamageType = (uint8)weaponType;
        vehicle->m_pLastDamageEntity     = ped;
        CEntity::SafeRegisterRef(vehicle->m_pLastDamageEntity);
    }

    // TODO(re: The vanilla body also fires two audio events - `0x9B` on `ped->m_weaponAudio` when the
    // active weapon is a chainsaw, and a per-(combo, move) event looked up around 0xC16F0C for
    // `CAEPedAudioEntity::AddAudioEvent`. Neither the `0x9B` enum member nor that lookup table's
    // layout is established; audio is out of the scope this task targets, so both are omitted.)
}
