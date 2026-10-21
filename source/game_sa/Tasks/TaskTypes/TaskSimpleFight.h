/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskSimple.h"
class CAnimBlendAssociation;
class CEntity;
class CPed;
class CPlayerPed;

enum eFightAttackType : int8 {
    FIGHT_ATTACK_HIT_1 = 0,
    FIGHT_ATTACK_HIT_2 = 1,
    FIGHT_ATTACK_HIT_3 = 2,
    FIGHT_ATTACK_FIGHT_BLOCK = 3,
    FIGHT_ATTACK_FIGHTIDLE = 4,
};

class NOTSA_EXPORT_VTABLE CMeleeInfo {
public:
    AssocGroupId         m_nAnimGroup;      // 0x00
    float                m_fRanges;         // 0x04
    std::array<float, 5> m_fHit;            // 0x08 - AKA `FireTime`; When the move connects
    std::array<float, 5> m_fChain;          // 0x1C - AKA `ComboTime`; When the next move of the combo may start
    std::array<float, 5> m_fRadius;         // 0x30
    float                m_fGroundLoop;     // 0x44
    float                m_fBlockLoopStart; // 0x48 - AKA `ABlockHit`
    float                m_fBlockLoopEnd;   // 0x4C - AKA `ABlockChain`
    std::array<int8, 5>  m_nHitLevel;       // 0x50 - Per move (See `GetHitLevel`)
    std::array<uint8, 5> m_nDamage;         // 0x55 - Per move
    uint8                m_Padding[2];      // 0x5A - AKA `field_58` (Not used)
    std::array<int32, 5> m_Hit;             // 0x5C - Per move sound (See `GetHitSound`)
    std::array<int32, 5> m_AltHit;          // 0x70 - Per move alternative sound (See `GetHitSound`)
    uint16               m_wFlags;          // 0x84
};
VALIDATE_SIZE(CMeleeInfo, 0x88);

class NOTSA_EXPORT_VTABLE CTaskSimpleFight : public CTaskSimple {
public:
    bool                   m_bIsFinished;
    bool                   m_bIsInControl;
    bool                   m_bAnimsReferenced;
    AssocGroupId           m_nRequiredAnimGroup;
    uint16                 m_nIdlePeriod;
    uint16                 m_nIdleCounter;
    int8                   m_nContinueStrike;
    int8                   m_nChainCounter;
    CEntity*               m_pTargetEntity;
    CAnimBlendAssociation* m_pAnim;
    CAnimBlendAssociation* m_pIdleAnim;
    int8                   m_nComboSet;
    eFightAttackType       m_nCurrentMove;
    uint8                  m_nNextCommand;
    uint8                  m_nLastCommand;

    static inline auto& m_aComboData = StaticRef<std::array<CMeleeInfo, MELEE_COMBO_NUM - MELEE_COMBO_UNARMED_1>>(0xC170D0); // One for each combo in `melee.dat`, `MELEE_COMBO_UNARMED_1` being the first
    static inline auto& m_aHitOffset = StaticRef<std::array<CVector, 7>>(0xC177D0); // Per hit level offset (The `START_LEVELS` section of `melee.dat`)

public:
    static constexpr auto Type = eTaskType::TASK_SIMPLE_FIGHT;

    CTaskSimpleFight(CEntity* entity, int32 nCommand, uint32 nIdlePeriod = 10000);
    ~CTaskSimpleFight() override;

    eTaskType GetTaskType() const override { return Type; }
    CTask* Clone() const override { return new CTaskSimpleFight(m_pTargetEntity, m_nLastCommand, m_nIdlePeriod); }
    bool MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    bool ProcessPed(CPed* ped) override;

    static void LoadMeleeData();

    void BeHitWhileBlocking(CPed* ped1, CPed* ped2, int8, int8);
    void ChooseAttackAI(CPed* ped);
    void ChooseAttackPlayer(CPed* ped);
    bool ControlFight(CEntity* entity, uint8 command);

    void FightHitCar(CPed* ped, CVehicle* vehicle, CVector& posn1, CVector& posn2, int16, int8);
    void FightHitObj(CPed* ped, CObject* object, CVector& posn1, CVector& posn2, int16, int8);
    void FightHitPed(CPed* creator, CPed* victim, CVector& posn1, CVector& posn2, int16);
    void FightSetUpCol(float);
    void FightStrike(CPed* ped, CVector& posn);

    void FindTargetOnGround(CPed* ped);
    void FinishMeleeAnimCB(CAnimBlendAssociation*, void*);

    bool IsComboSet();
    bool IsHitComboSet();

    void GetAvailableComboSet(CPed* ped, int8);
    void GetComboType(char*);
    AssocGroupId GetComboAnimGroupID();
    static uint8 GetHitLevel(const char* level);
    static int16 GetHitSound(int32 id);
    void GetRange();
    void GetStrikeDamage(CPed* ped);

    void SetPlayerMoveAnim(CPlayerPed* player);
    void StartAnim(CPed* ped, int32);

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CTaskSimpleFight* Constructor(CEntity* entity, int32 nCommand, uint32 nIdlePeriod);
    CTaskSimpleFight* Destructor();

};
VALIDATE_SIZE(CTaskSimpleFight, 0x28);
