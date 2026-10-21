#include "StdInc.h"

#include "Ragdoll/IKChainManager.h"
#include "TaskSimpleUseGun.h"
#include "TaskSimpleGetUp.h"
#include "TaskSimpleDuck.h"
#include "TaskSimpleFight.h"

void CTaskSimpleUseGun::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleUseGun, 0x86D724, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

    //RH_ScopedInstall(Constructor, 0x61DE60);
    //RH_ScopedInstall(Destructor, 0x61DF30);

    RH_ScopedGlobalInstall(FinishGunAnimCB, 0x61F3A0);
    RH_ScopedGlobalInstall(RequirePistolWhip, 0x61E200);
    RH_ScopedInstall(Reset, 0x624DC0);
    RH_ScopedInstall(AimGun, 0x61ED10);
    RH_ScopedInstall(FireGun, 0x61EB10);
    RH_ScopedInstall(StartCountDown, 0x61E160);
    RH_ScopedInstall(ClearAnim, 0x61E190);
    RH_ScopedInstall(PlayerPassiveControlGun, 0x61E0A0);
    RH_ScopedInstall(ControlGun, 0x61E040);
    RH_ScopedInstall(AbortIK, 0x61DFA0);
    RH_ScopedInstall(ControlGunMove, 0x61E0C0);
    RH_ScopedInstall(RemoveStanceAnims, 0x61E8E0);
    RH_ScopedInstall(StartAnim, 0x624F30);
    RH_ScopedInstall(SetMoveAnim, 0x61E3F0);
    RH_ScopedVMTInstall(Clone, 0x622F20);
    RH_ScopedVMTInstall(GetTaskType, 0x61DF20);
    RH_ScopedVMTInstall(MakeAbortable, 0x624E30);
    RH_ScopedVMTInstall(ProcessPed, 0x62A380);
    RH_ScopedVMTInstall(SetPedPosition, 0x624ED0);
}

// 0x61DE60
CTaskSimpleUseGun::CTaskSimpleUseGun(CEntity* targetEntity, CVector vecTarget, eGunCommand firstCmd, uint16 burstLength, bool isAimImmediate) :
    m_NextCmd{ firstCmd },
    m_TargetPos{ vecTarget },
    m_TargetEntity{ targetEntity },
    m_BurstShots{ burstLength },
    m_IsAimImmediate{ isAimImmediate } {
}

// 0x61DF30
CTaskSimpleUseGun::~CTaskSimpleUseGun() {
    if (m_Anim) {
        m_Anim->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
    }
}

// 0x61E0C0
bool CTaskSimpleUseGun::ControlGunMove(const CVector2D& moveDir) {
    const auto maxDelta = CTimer::GetTimeStep() * 0.07f;
    for (size_t i = 0; i < 2; i++) {
        m_MoveCmd[i] = std::clamp(moveDir[i], m_MoveCmd[i] - maxDelta, m_MoveCmd[i] + maxDelta);
    }
    m_HasMoveControl = true;
    return true;
}

// 0x61F3A0
void CTaskSimpleUseGun::FinishGunAnimCB(CAnimBlendAssociation* anim, void* data) {
    const auto self = notsa::cast<CTaskSimpleUseGun>(static_cast<CTask*>(data));

    if (self->m_BurstShots > 0 && self->m_LastCmd == eGunCommand::RELOAD && self->m_NextCmd < eGunCommand::END_LEISURE) {
        switch (anim->GetAnimId()) {
        case ANIM_ID_RELOAD:
        case ANIM_ID_CROUCHRELOAD: {
            self->m_NextCmd = eGunCommand::RELOAD;
        }
        }
    }

    if (self->m_Anim == anim) {
        self->m_Anim = nullptr;
    }
}

// 0x61EB10
void CTaskSimpleUseGun::FireGun(CPed* ped, bool isLeftHand) {
    const auto DoFireGun = [&](CVector origin, CVector barrelPos) {
        ped->GetActiveWeapon().Fire(
            ped,
            &origin,
            &barrelPos,
            m_TargetEntity,
            m_TargetPos.x == 0.f || m_TargetPos.y == 0.f ? nullptr : &m_TargetPos,
            nullptr
        );
    };

    if (ped->bCalledPreRender) { // If pre-render was called, the bone positions are up-to-date
        CVector    barrelPos  = m_WeaponInfo->m_vecFireOffset;
        const auto handBoneId = isLeftHand ? BONE_L_HAND : BONE_R_HAND;
        CVector    origin     = ped->GetBonePosition(handBoneId);
        origin.z += barrelPos.z + 0.15f;
        ped->GetTransformedBonePosition(barrelPos, handBoneId);
        if (!ped->m_pedIK.bUseArm) {
            ped->m_pedIK.bGunReachedTarget = g_ikChainMan.IsFacingTarget(ped, isLeftHand ? eIKChainSlot::LEFT_ARM : eIKChainSlot::RIGHT_ARM);
        }
        DoFireGun(origin, barrelPos);
    } else { // Otherwise ped is probably not on screen anyways, so just do something good enough
        CVector origin = ped->GetPosition();
        origin.z += 0.7f;
        DoFireGun(origin, origin);
    }

    if (ped->GetActiveWeapon().GetState() == WEAPONSTATE_FIRING) {
        ped->DoGunFlash(250, isLeftHand);
    }
}

// 0x61E0A0
bool CTaskSimpleUseGun::PlayerPassiveControlGun() {
    m_IsInControl               = true;
    const auto IsCommandPassive = [](eGunCommand c) {
        return notsa::contains({ eGunCommand::UNKNOWN, eGunCommand::NONE, eGunCommand::AIM }, c);
    };
    if (IsCommandPassive(m_LastCmd) && IsCommandPassive(m_NextCmd)) {
        m_NextCmd = eGunCommand::END_LEISURE;
        return true;
    }
    return false;
}

// 0x61E8E0
void CTaskSimpleUseGun::RemoveStanceAnims(CPed* ped, float x) {
    if (m_Anim && !(m_Anim->m_Flags & ANIMATION_IS_PLAYING) && m_Anim->GetTimeProgress() < 1.f) {
        if (m_WeaponInfo && m_WeaponInfo->GetAnimLoopStart() == m_Anim->GetCurrentTime()) {
            m_Anim->SetCurrentTime(m_WeaponInfo->GetAnimLoopEnd());
        }
        m_Anim->SetFlag(ANIMATION_IS_PLAYING);
    }

    bool pedHasStanceAnims = false;
    for (const auto animId : {
        ANIM_ID_GUN_STAND,
        ANIM_ID_GUNMOVE_FWD,
        ANIM_ID_GUNMOVE_L,
        ANIM_ID_GUNMOVE_BWD,
        ANIM_ID_GUNMOVE_R,
    }) {
        if (const auto a = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), animId)) {
            a->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
            pedHasStanceAnims = true;
        }
    }

    ped->SetMoveState(PEDMOVE_STILL);
    ped->m_nSwimmingMoveState = PEDMOVE_STILL;

    if (!pedHasStanceAnims) {
        return;
    }

    const auto DoBlendAnimAndStart = [ped](AnimationId animId) {
        const auto animWalk = CAnimManager::BlendAnimation(ped->GetRpClump(), ped->m_nAnimGroup, animId);
        animWalk->SetFlag(ANIMATION_IS_PLAYING);
    };

    if (ped->IsPlayer()) {
        if (const auto pad = ped->AsPlayer()->GetPadFromPlayer()) {
            if (std::abs(pad->GetPedWalkUpDown()) > 50 || std::abs(pad->GetPedWalkLeftRight()) > 50) { // NOTSA: They didn't use abs ;)
                DoBlendAnimAndStart(ANIM_ID_WALK);

                ped->SetMoveState(PEDMOVE_WALK);
                ped->m_nSwimmingMoveState = PEDMOVE_WALK;

                if (const auto pd = ped->GetPlayerData()) {
                    pd->m_fMoveBlendRatio = 1.f;
                }
            }
        }
    }

    DoBlendAnimAndStart(ANIM_ID_IDLE);

    if (m_NextCmd == eGunCommand::END_LEISURE) {
        switch (ped->m_nAnimGroup) {
        case ANIM_GROUP_PLAYERROCKET:
        case ANIM_GROUP_PLAYERROCKETF:
        case ANIM_GROUP_PLAYERROCKETM:
            break;
        default:
            CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, ANIM_ID_GUN_2_IDLE, 8.f);
        }
    }
}

// 0x61E200
bool CTaskSimpleUseGun::RequirePistolWhip(CPed* ped, CEntity* targetEntity) {
    if (const auto tUseGun = ped->GetIntelligence()->GetTaskUseGun()) {
        if (tUseGun->GetLastGunCommand() == eGunCommand::PISTOLWHIP) {
            return false;
        }
    }
    const auto IsPistolWhipRequiredForPed = [
        &,
        &pedPos = ped->GetPosition()
    ](CPed& otherPed) {
        if (!otherPed.IsAlive()) { // Ped is dead :D
            return false;
        }
        const auto forPedHeadPos = otherPed.GetBonePosition(BONE_HEAD);
        if (pedPos.z >= forPedHeadPos.z || pedPos.z + 1.5f <= forPedHeadPos.z) { // Ped is below/above us
            return false;
        }
        if (otherPed.bFallenDown && !notsa::dyn_cast_if_present<CTaskSimpleGetUp>(ped->GetTaskManager().GetSimplestActiveTask())) { // Fell, but hasn't got up yet?
            return false;
        }
        const auto pedToOther = otherPed.GetPosition() - ped->GetPosition();
        if (pedToOther.SquaredMagnitude2D() >= 1.f) {
            return false;
        }
        const auto pedToOtherDot = ped->GetForward().Dot(pedToOther);
        if (pedToOtherDot <= 0.f) { // Is behind?
            return false;
        }
        if (std::abs(ped->GetRight().Dot(pedToOther)) / pedToOtherDot >= 1.f / SQRT_3) { // 1/sqrt3 = 0.57730001
            return false;
        }
        return true;
    };
    return targetEntity && targetEntity->GetIsTypePed()
        ? IsPistolWhipRequiredForPed(*targetEntity->AsPed())
        : rng::any_of(ped->GetIntelligence()->GetPedScanner().GetEntities<CPed>(), IsPistolWhipRequiredForPed);
}

// 0x61E040
bool CTaskSimpleUseGun::ControlGun(CPed* ped, CEntity* target, eGunCommand cmd) {
    m_IsInControl = true;
    if (target != m_TargetEntity) { // New target?
        m_TargetEntity  = target;
        m_IsArmIKInUse  = false;
        m_IsLookIKInUse = false;
    }
    m_NextCmd = std::max(cmd, m_NextCmd);
    return true;
}

// 0x61DFA0
void CTaskSimpleUseGun::AbortIK(CPed* ped) {
    if (m_IsArmIKInUse) {
        g_ikChainMan.AbortPointArmIfPointing(eIKArm::IK_ARM_RIGHT, ped);
        g_ikChainMan.AbortPointArmIfPointing(eIKArm::IK_ARM_LEFT, ped);
        m_IsArmIKInUse = false;
    }
    if (m_IsLookIKInUse) {
        g_ikChainMan.AbortLookAtIfLooking(ped);
        m_IsLookIKInUse = false;
    }
    ped->m_pedIK.bEverythingUsed = false;
}

// 0x61ED10
void CTaskSimpleUseGun::AimGun(CPed* ped) {
    const auto ikBlendTime = m_IsAimImmediate ? 0 : 250;
    const auto ikSpeed     = m_IsAimImmediate ? 1.f : 0.5f;

    const auto isAimingWithArm = ped->bIsDucking || !m_WeaponInfo->flags.bAimWithArm;
    if (isAimingWithArm) {
        if (!ped->m_pedIK.bEverythingUsed) {
            AbortIK(ped);
        }
    }
    ped->m_pedIK.bUseArm         = !isAimingWithArm;
    ped->m_pedIK.bEverythingUsed = isAimingWithArm;

    const auto DoLookAt = [this, ped](eBoneTag lookAtBone, CEntity* lookAtEntity, CVector* lookAtPos) {
        g_ikChainMan.LookAt(
            "TaskUseGun",
            ped,
            lookAtEntity,
            9'999'999,
            lookAtBone,
            lookAtPos,
            true,
            0.25f,
            250,
            3,
            false
        );
        m_IsLookIKInUse = true;
    };

    const auto DoPointArm = [&, this, ped](eIKArm arm, eBoneTag armBone = BONE_UNKNOWN, CEntity* lookAtEntity = nullptr, CVector* lookAtPos = nullptr) {
        g_ikChainMan.PointArm(
            "CTaskSimpleUseGun",
            arm,
            ped,
            lookAtEntity,
            armBone,
            lookAtPos,
            ikSpeed,
            ikBlendTime,
            100.f
        );
        m_IsArmIKInUse = true;
    };

    if (m_TargetEntity) { // If there's a target entity, aim at them
        const auto [lookAtPos, lookAtBone] = GetAimLookAtInfo(); // 0x61EDB8

        if (!ped->IsPlayer()) {
            ped->Say(CTX_GLOBAL_TARGET);
        }

        if (ped->m_pedIK.bUseArm) { // Removed redudant `if` check from here
            if (!m_IsLookIKInUse) {
                if (m_Anim->GetBlendAmount() > 0.98f) {
                    DoLookAt(lookAtBone, m_TargetEntity, nullptr); // 0x61EF0B
                }
            }
             
            if (!m_IsArmIKInUse) { // 0x61EF0B
                DoPointArm(eIKArm::IK_ARM_RIGHT, lookAtBone, m_TargetEntity, nullptr);
                if (m_WeaponInfo->flags.bTwinPistol && !ped->bLeftArmBlocked) {
                    DoPointArm(eIKArm::IK_ARM_LEFT, lookAtBone, m_TargetEntity, nullptr);
                } 
            }

            ped->m_pedIK.RotateTorsoForArm(m_TargetEntity->GetPosition());
        } else {
            ped->m_pedIK.PointGunAtPosition(lookAtPos, m_Anim->GetBlendAmount());
        }
    } else if (ped->m_pedIK.bUseArm) { // 0x61EFB4 - Process player free-aiming
        const auto pd = ped->GetPlayerData();
        if (pd && pd->m_bFreeAiming && CVector2D{ m_TargetPos }.IsZero() && notsa::contains({ MODE_AIMWEAPON, MODE_AIMWEAPON_ATTACHED }, TheCamera.m_aCams[0].m_nMode)) { // Aim to in front of us
            CVector origin, target;
            TheCamera.Find3rdPersonCamTargetVector(20.f, ped->GetPosition() + CVector{0.f, 0.f, 0.7f}, origin, target);

            if (m_Anim->GetBlendAmount() > 0.98f) {
                DoLookAt(BONE_UNKNOWN, nullptr, &target);
            }

            DoPointArm(eIKArm::IK_ARM_RIGHT, BONE_UNKNOWN, nullptr, &target);
            if (m_WeaponInfo->flags.bTwinPistol && !ped->bLeftArmBlocked) {
                DoPointArm(eIKArm::IK_ARM_LEFT, BONE_UNKNOWN, nullptr, &target);
            }

            ped->m_pedIK.RotateTorsoForArm(target);
        } else if (!m_IsArmIKInUse || !m_IsLookIKInUse) { // 0x61F144 - Neither pointing with arm, or looking at anything -> Just look somewhere...
            if (m_TargetPos.x == 0.f || m_TargetPos.y == 0.f) { // TODO/NOTE: Bug? Perhaps they meant &&?
                CVector target{0.f, 2.f, 0.f};

                if (m_Anim->GetBlendAmount() > 0.98f) {
                    DoLookAt(BONE_UNKNOWN, nullptr, &target);
                }

                DoPointArm(eIKArm::IK_ARM_RIGHT, BONE_R_UPPER_ARM, ped, &target);
                if (m_WeaponInfo->flags.bTwinPistol && !ped->bLeftArmBlocked) {
                    DoPointArm(eIKArm::IK_ARM_LEFT, BONE_L_UPPER_ARM, ped, &target);
                }
            } else { // 0x61F172 - Either arm/look IK is in use, meaning there's a target
                if (m_Anim->GetBlendAmount() > 0.98f) {
                    DoLookAt(BONE_UNKNOWN, nullptr, &m_TargetPos);
                }

                DoPointArm(eIKArm::IK_ARM_RIGHT, BONE_UNKNOWN, nullptr, &m_TargetPos);
                if (m_WeaponInfo->flags.bTwinPistol && !ped->bLeftArmBlocked) {
                    DoPointArm(eIKArm::IK_ARM_LEFT, BONE_UNKNOWN, nullptr, &m_TargetPos);
                }
            }
        }
    } else if (m_TargetPos.x == 0.f || m_TargetPos.y == 0.f) { // 0x61F30E
        ped->m_pedIK.PointGunInDirection(
            ped->m_fCurrentRotation,
            ped->IsPlayer() ? ped->GetPlayerData()->m_fLookPitch : 0.f,
            false,
            m_Anim->GetBlendAmount()
        );
    } else { // 0x61F305
        ped->m_pedIK.PointGunAtPosition(m_TargetPos, m_Anim->GetBlendAmount());
    }

    if (m_WeaponInfo->flags.bTwinPistol && ped->bLeftArmBlocked) {
        g_ikChainMan.AbortPointArmIfPointing(eIKArm::IK_ARM_RIGHT, ped);
    }
}

// 0x61E190
void CTaskSimpleUseGun::ClearAnim(CPed* ped) {
    if (m_Anim) {
        if (m_Anim->GetBlendAmount() > 0.f && m_Anim->GetBlendDelta() >= 0.f) {
            m_Anim->SetBlendDelta(-8.f);
        }
        m_Anim->SetDefaultDeleteCallback();
        m_Anim = nullptr;
    }
    if (m_LastCmd < eGunCommand::END_LEISURE) {
        m_LastCmd = eGunCommand::NONE;
    }
    AbortIK(ped);
}

// 0x624E30
bool CTaskSimpleUseGun::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    if (priority == ABORT_PRIORITY_LEISURE) {
        m_NextCmd = eGunCommand::END_LEISURE;
        return false;
    }
    RemoveStanceAnims(ped, -4.f);
    if (priority == ABORT_PRIORITY_IMMEDIATE) {
        CAnimManager::BlendAnimation(ped->GetRpClump(), ped->m_nAnimGroup, ANIM_ID_IDLE, 1000.f);
    }
    if (m_Anim) {
        if (priority == ABORT_PRIORITY_IMMEDIATE) {
            m_Anim->SetBlendDelta(-1000.f);
        }
        m_Anim->SetDefaultDeleteCallback();
        m_Anim = nullptr;
    }
    m_IsFinished = true;
    AbortIK(ped);
    if (const auto pd = ped->GetPlayerData()) {
        pd->m_fAttackButtonCounter = 0.f;
    }
    return true;
}

// Melee data - Written by `CTaskSimpleFight::LoadMeleeData` (0x5BEDC0)
static inline auto& g_MeleeAttackTime = StaticRef<float[2]>(0xC17738);     //!< Indexed by ped ducking; Time into the attack anim at which the hit occurs
static inline auto& g_MeleeAttackPointIdx = StaticRef<uint8[2]>(0xC17780); //!< Indexed by ped ducking; Which of `g_MeleeAttackPointOffsets` to use
static inline auto& g_MeleeAttackPointOffsets = StaticRef<CVector[2]>(0xC177D0); //!< Indexed by `g_MeleeAttackPointIdx`; Attack point offset (In ped space)

// 0x62A380
bool CTaskSimpleUseGun::ProcessPed(CPed* ped) {
    if (m_IsLookIKInUse && !g_ikChainMan.IsLooking(ped)) {
        m_IsLookIKInUse = false;
    }
    if (m_IsArmIKInUse && !IKChainManager_c::IsArmPointing(eIKArm::IK_ARM_RIGHT, ped)) {
        m_IsArmIKInUse = false;
    }
    m_IsFiringGunRightHandThisFrame = false;
    m_IsFiringGunLeftHandThisFrame = false;

    // `LAB_0062af27`
    const auto AbortIKAndReturnTrue = [this, ped] {
        AbortIK(ped);
        if (const auto pd = ped->GetPlayerData()) {
            pd->m_fAttackButtonCounter = 0.f;
        }
        return true;
    };

    if (!m_WeaponInfo) { // 0x62A3DA
        if (const auto pd = ped->GetPlayerData(); pd && pd->m_nChosenWeapon != ped->m_nActiveWeaponSlot) {
            return false;
        }

        m_WeaponInfo = &ped->GetActiveWeapon().GetWeaponInfo(ped);
        if (m_WeaponInfo->m_nWeaponFire == WEAPON_FIRE_MELEE || m_WeaponInfo->flags.bThrow) {
            m_IsFinished = true;
            m_WeaponInfo = nullptr;
            return AbortIKAndReturnTrue();
        }
        m_MoveCmd = { 0.f, 0.f };
    } else if (m_WeaponInfo != &ped->GetActiveWeapon().GetWeaponInfo(ped)) { // 0x62A44D
        MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr);
    }

    if (!m_WeaponInfo->flags.b1stPerson || !ped->GetPlayerData()) { // 0x62A488
        ped->bTestForBlockedPositions = true;
    }

    if (!m_IsFinished) { // 0x62A4A8
        if (const auto pd = ped->GetPlayerData(); pd && pd->m_fAttackButtonCounter > 0.f) {
            pd->m_fAttackButtonCounter *= std::pow(0.96f, CTimer::GetTimeStep());
        }

        if (m_IsInControl) { // 0x62A4E4
            m_CountDownFrames = 0xFF;

            const bool isDucking     = ped->bIsDucking;
            const bool isArmBlocked  = isDucking // 0x62A4F3
                ? ped->bDuckRightArmBlocked
                : ped->bRightArmBlocked && !m_WeaponInfo->flags.bAimWithArm;

            // Whether the LOS being blocked means we should skip the anim block below (`LAB_0062a56d`)
            bool isLosBlocked = false;
            if (!isArmBlocked) { // 0x62A5D0
                if (m_NextCmd == eGunCommand::PISTOLWHIP && m_LastCmd <= eGunCommand::FIREBURST) {
                    if (m_Anim) {
                        m_Anim->SetDefaultDeleteCallback();
                    }
                    m_Anim = nullptr;
                    AbortIK(ped);
                }
            } else if (m_LastCmd != eGunCommand::PISTOLWHIP && (m_NextCmd != eGunCommand::PISTOLWHIP || m_Anim)) { // 0x62A532
                if (m_Anim) { // 0x62A623
                    m_Anim->SetBlendDelta(-4.f);
                    m_Anim->SetFlag(ANIMATION_IS_PLAYING, false);
                    m_Anim->SetFlag(ANIMATION_IS_BLEND_AUTO_REMOVE);
                    isLosBlocked = true;
                } else if (m_NextCmd >= eGunCommand::AIM && m_NextCmd <= eGunCommand::FIREBURST) { // 0x62A60D
                    isLosBlocked = true;
                }

                if (isLosBlocked) { // 0x62A638
                    AbortIK(ped);
                    m_IsLOSBlocked = true;
                    if (m_LastCmd == eGunCommand::FIREBURST) {
                        m_LastCmd = eGunCommand::AIM;
                    }
                }
            }

            if (!isLosBlocked) { // `LAB_0062a54d`
                if (!m_Anim) {
                    StartAnim(ped);
                    if (!m_Anim) {
                        AbortIK(ped);
                    }
                } else {
                    switch (m_LastCmd) {
                    case eGunCommand::RELOAD: { // 0x62A657
                        if (m_Anim->GetAnimId() != ANIM_ID_RELOAD && m_Anim->GetAnimId() != ANIM_ID_CROUCHRELOAD) {
                            if (m_Anim->GetBlendDelta() >= 0.f) {
                                m_Anim->SetBlendDelta(-4.f);
                            }
                        }
                        break;
                    }
                    case eGunCommand::PISTOLWHIP: { // 0x62A67C
                        const auto
                            animBlendAmount = m_Anim->GetBlendAmount(),
                            animTime        = m_Anim->GetCurrentTime(),
                            animTimeStep    = m_Anim->m_TimeStep;

                        // The hit occurs `<One time step>` after `g_MeleeAttackTime`
                        const bool isTimeToAttack = animBlendAmount > 0.9f
                            && m_Anim->GetBlendDelta() >= 0.f
                            && animTime > g_MeleeAttackTime[isDucking]
                            && animTime - animTimeStep < g_MeleeAttackTime[isDucking];

                        if (isTimeToAttack) { // 0x62A6E9
                            CVector attackPoint = ped->GetMatrix().TransformPoint(g_MeleeAttackPointOffsets[g_MeleeAttackPointIdx[isDucking]]);

                            // A temporary fight task is set up to describe the punch
                            CTaskSimpleFight fightTask(m_TargetEntity, 0xB, 20'000);
                            fightTask.m_pAnim        = m_Anim;
                            fightTask.m_nComboSet    = 0x10; // NOTE: Retail's value, a combo set index into `m_aComboData`
                            fightTask.m_nCurrentMove = (eFightAttackType)isDucking;
                            fightTask.m_nLastCommand = 0xB;

                            // NOTE: 0x6240B0 is an unidentified `CTaskSimpleFight` member (Its only caller is this function),
                            //       so it's called into the game. Args are `(CPed*, CVector* attackPoint)`.
                            plugin::CallMethod<0x6240B0, CTaskSimpleFight*, CPed*, CVector*>(&fightTask, ped, &attackPoint);

                            fightTask.m_pAnim = nullptr; // Don't let the dtor touch our anim
                        }

                        if (m_TargetEntity) { // 0x62A771
                            const auto targetDir = m_TargetEntity->GetPosition() - ped->GetPosition();
                            ped->m_fAimingRotation = std::atan2(-targetDir.x, targetDir.y);
                        }
                        break;
                    }
                    case eGunCommand::AIM:
                    case eGunCommand::FIRE:
                    case eGunCommand::FIREBURST: { // 0x62A7F7
                        if (m_Anim->GetBlendDelta() >= 0.f) {
                            const auto
                                animLoopStart = m_WeaponInfo->GetAnimLoopStart(isDucking),
                                animLoopEnd   = m_WeaponInfo->GetAnimLoopEnd(isDucking),
                                animLoopFire  = isDucking ? m_WeaponInfo->m_fAnimLoop2Fire : m_WeaponInfo->m_fAnimLoopFire,
                                animLoopFireMid = (animLoopEnd - animLoopStart) * 0.5f + animLoopFire;

                            const auto
                                animTime     = m_Anim->GetCurrentTime(),
                                animTimeStep = m_Anim->m_TimeStep;

                            // The fire event occurs once the anim time passes `threshold` (And at most one time step after it)
                            const auto IsInFireTimeWindow = [&](float threshold) {
                                return animTime > threshold && animTime - animTimeStep <= threshold;
                            };

                            if (m_LastCmd == eGunCommand::FIRE || m_LastCmd == eGunCommand::FIREBURST) { // 0x62A855
                                if (m_Anim->GetBlendAmount() < 0.99f || ped->GetActiveWeapon().GetState() == WEAPONSTATE_RELOADING) {
                                    if (m_Anim->IsPlaying() && animTime >= animLoopStart && animTime - animTimeStep < animLoopStart) { // 0x62A89A
                                        m_Anim->SetFlag(ANIMATION_IS_PLAYING, false);
                                        m_Anim->SetCurrentTime(animLoopStart);
                                    }
                                } else if (!m_Anim->IsPlaying() && animTime == animLoopStart) { // 0x62A880
                                    m_Anim->SetFlag(ANIMATION_IS_PLAYING);
                                }
                            }

                            if (!m_WeaponInfo->flags.bContinuosFire) { // 0x62A99A
                                if (m_Anim->IsPlaying() && IsInFireTimeWindow(animLoopFire) && (m_LastCmd == eGunCommand::FIRE || m_LastCmd == eGunCommand::FIREBURST)) {
                                    m_IsFiringGunRightHandThisFrame = true;
                                    m_HasFiredGun = true;
                                    if (m_BurstShots > 0) {
                                        m_BurstShots--;
                                    }
                                }

                                if (m_WeaponInfo->flags.bTwinPistol && !ped->bLeftArmBlocked
                                    && m_Anim->IsPlaying() && IsInFireTimeWindow(animLoopFireMid)
                                    && (m_LastCmd == eGunCommand::FIRE || m_LastCmd == eGunCommand::FIREBURST)
                                ) {
                                    m_IsFiringGunLeftHandThisFrame = true;
                                    m_HasFiredGun = true;
                                    if (m_BurstShots > 0) {
                                        m_BurstShots--;
                                    }
                                }
                            } else if ((m_LastCmd == eGunCommand::FIRE || m_LastCmd == eGunCommand::FIREBURST) // 0x62A8F0
                                    && animTime > animLoopStart
                                    && animTime < animLoopEnd
                                    && m_Anim->IsPlaying()
                            ) {
                                if (!m_HasFiredGun // 0x62A932
                                    || (m_LastCmd == eGunCommand::FIREBURST && m_BurstShots > 0)
                                    || m_NextCmd == eGunCommand::FIRE
                                    || m_NextCmd == eGunCommand::FIREBURST
                                ) {
                                    m_IsFiringGunRightHandThisFrame = true;
                                    m_HasFiredGun = true;
                                    if (m_LastCmd < m_NextCmd) {
                                        m_LastCmd = m_NextCmd;
                                    }
                                    m_NextCmd = eGunCommand::NONE;
                                    if (m_LastCmd == eGunCommand::FIREBURST && m_BurstShots > 0) {
                                        m_BurstShots--;
                                    } else {
                                        m_BurstShots = 0;
                                    }
                                } else {
                                    m_Anim->SetFlag(ANIMATION_IS_PLAYING, false);
                                    m_Anim->SetBlendDelta(-4.f);
                                }
                            }

                            const bool isAnimPlaying = m_Anim->IsPlaying(); // 0x62AA48
                            if (isAnimPlaying
                                || (m_LastCmd == eGunCommand::AIM && m_NextCmd <= eGunCommand::AIM)
                                || (m_Anim->GetHier()->GetTotalTime() <= animTime && m_Anim->GetBlendDelta() < 0.f)
                            ) {
                                if (m_LastCmd == eGunCommand::AIM && isAnimPlaying) { // 0x62AB35
                                    if ((animTime >= animLoopStart && animTime - animTimeStep < animLoopStart)
                                        || animTime + animTimeStep >= animLoopStart
                                    ) {
                                        m_Anim->SetFlag(ANIMATION_IS_PLAYING, false);
                                        m_Anim->SetCurrentTime(animLoopStart);
                                    }
                                }
                            } else { // 0x62AA8E
                                if (m_LastCmd < eGunCommand::PISTOLWHIP && m_NextCmd < eGunCommand::PISTOLWHIP) {
                                    if (m_Anim->GetBlendAmount() > 0.f
                                        && m_Anim->GetBlendDelta() >= 0.f
                                        && ped->GetActiveWeapon().GetState() != WEAPONSTATE_RELOADING
                                    ) {
                                        m_Anim->SetFlag(ANIMATION_IS_PLAYING);
                                    }
                                } else if (!m_WeaponInfo->flags.bExpands) {
                                    m_Anim->SetBlendDelta(-4.f);
                                } else {
                                    m_Anim->SetFlag(ANIMATION_IS_PLAYING);
                                    if (m_Anim->GetCurrentTime() != animLoopStart) {
                                        m_Anim->SetCurrentTime(animLoopEnd);
                                    }
                                }

                                const auto nextCmd = m_NextCmd; // 0x62AB05
                                if (nextCmd == eGunCommand::FIRE || nextCmd == eGunCommand::FIREBURST) {
                                    m_LastCmd = nextCmd;
                                    m_NextCmd = eGunCommand::NONE;
                                    if (nextCmd == eGunCommand::FIREBURST) {
                                        m_BurstShots = m_BurstLength;
                                    }
                                } else if (m_LastCmd == eGunCommand::AIM && nextCmd != eGunCommand::AIM) {
                                    m_LastCmd = eGunCommand::NONE;
                                }
                            }

                            if (animTime > m_WeaponInfo->m_fBreakoutTime && m_NextCmd == eGunCommand::END_NOW) { // 0x62AC36
                                m_IsFinished = true;
                                m_Anim->SetBlendDelta(m_Anim->HasFlag(ANIMATION_DONT_ADD_TO_PARTIAL_BLEND) ? -1.f : -4.f);
                            }
                        }
                        break;
                    }
                    default: { // 0x62AC6E
                        if (m_LastCmd == eGunCommand::NONE && m_Anim->GetBlendAmount() > 0.f) {
                            if (m_Anim->GetBlendDelta() >= 0.f) {
                                m_Anim->SetBlendDelta(-4.f);
                            }
                        }
                        break;
                    }
                    }

                    // 0x62AC99
                    m_SkipAim = false;
                    if (ped->m_pedIK.bUseArm) {
                        // A target ped with no health left is considered dead
                        const bool isAimTargetDead = m_TargetEntity && m_TargetEntity->GetIsTypePed()
                            && m_TargetEntity->AsPed()->m_fHealth <= 0.f;

                        const auto aimPos = GetAimTargetPosition(ped);

                        if (aimPos.x != 0.f || aimPos.y != 0.f) { // 0x62ADB6
                            CVector aimDir = aimPos - ped->GetPosition();
                            aimDir.Normalise();

                            auto aimAngle = std::atan2(-aimDir.x, aimDir.y) - ped->m_fCurrentRotation;
                            if (aimAngle > 3.1415927f) {
                                aimAngle -= 6.2831855f;
                            } else if (aimAngle < -3.1415927f) {
                                aimAngle += 6.2831855f;
                            }

                            // The aim cone is widened by 40 deg. if the target is dead
                            constexpr auto DEG_40 = 0.69813174f;
                            if (aimAngle > 2.0071287f - isAimTargetDead * DEG_40
                                || aimAngle < -2.268928f + isAimTargetDead * DEG_40
                            ) {
                                m_SkipAim = true;
                            }

                            if (isAimTargetDead && aimDir.Dot(ped->GetUpVector()) < -0.8f) { // 0x62AE89
                                m_SkipAim = true;
                            }
                        }

                        if (!ped->bIsDucking && ped->bRightArmBlocked) { // 0x62AEB6
                            m_SkipAim = true;
                        }
                    }

                    if (m_LastCmd != eGunCommand::NONE && m_LastCmd < eGunCommand::RELOAD && !m_SkipAim) { // 0x62AECF
                        AimGun(ped);
                    } else {
                        AbortIK(ped);
                    }
                }
            }

            // `LAB_0062a56d`
            if (!m_WeaponInfo->flags.bAimWithArm || ped->bIsDucking || m_HasMoveControl) { // 0x62A56D
                SetMoveAnim(ped);
            }
            if (m_NextCmd < eGunCommand::END_LEISURE
                && (m_NextCmd != eGunCommand::RELOAD || (m_LastCmd != eGunCommand::FIRE && m_LastCmd != eGunCommand::FIREBURST))
            ) {
                m_NextCmd = eGunCommand::NONE;
            }
            m_IsInControl = false;
            return false;
        }

        // Not in control - Count down until we're done (0x62AEFA)
        m_HasFiredGun = false;
        m_MoveCmd = { 0.f, 0.f };
        const auto prevCountDownFrames = m_CountDownFrames;
        m_CountDownFrames--;
        if (prevCountDownFrames) {
            return false;
        }
        m_IsFinished = true;
    }

    RemoveStanceAnims(ped, -4.f);
    return AbortIKAndReturnTrue();
}

// 0x624ED0
bool CTaskSimpleUseGun::SetPedPosition(CPed* ped) {
    if (m_WeaponInfo == &ped->GetActiveWeapon().GetWeaponInfo(ped)) {
        if (m_IsFiringGunRightHandThisFrame) { // Right hand
            FireGun(ped, false);
            m_IsFiringGunRightHandThisFrame = false;
        }
        if (m_IsFiringGunLeftHandThisFrame) { // Left hand
            FireGun(ped, true);
            m_IsFiringGunLeftHandThisFrame = false;
        }
    }
    return false;
}

// notsa
CVector CTaskSimpleUseGun::GetAimTargetPosition(CPed* ped) const {
    if (m_TargetEntity) {
        if (m_TargetEntity->GetIsTypePed()) {
            return m_TargetEntity->AsPed()->GetBonePosition(BONE_SPINE1);
        }
        return m_TargetEntity->GetPosition();
    }
    if (ped->IsPlayer() && TheCamera.m_aCams[0].m_nMode == MODE_AIMWEAPON) {
        CVector src, target;
        TheCamera.Find3rdPersonCamTargetVector(20.f, ped->GetPosition() + CVector{0.f, 0.f, 0.7f}, src, target);
        return target;
    }
    return m_TargetPos;
}

// notsa
std::pair<CVector, eBoneTag> CTaskSimpleUseGun::GetAimLookAtInfo() const {
    if (m_TargetEntity->GetIsTypePed()) {
        const auto targetPed = m_TargetEntity->AsPed();
        if (const auto pd = targetPed->GetPlayerData()) {
            CVector ret = pd->m_vecTargetBoneOffset;
            targetPed->GetTransformedBonePosition(ret, (eBoneTag)pd->m_nTargetBone);
            return {ret, (eBoneTag)pd->m_nTargetBone};
        }
    }
    return {m_TargetEntity->GetPosition(), BONE_SPINE1};
}

// 0x624DC0
void CTaskSimpleUseGun::Reset(CPed* ped, CEntity* targetEntity, CVector targetPos, eGunCommand, int16 burstLength) {
    AbortIK(ped);

    m_TargetPos       = targetPos;
    m_BurstLength     = burstLength;
    m_BurstShots      = 0;
    m_CountDownFrames = -1;
    m_IsLOSBlocked    = false;
    m_IsArmIKInUse    = false;
    m_IsLookIKInUse   = false;
    m_TargetEntity    = targetEntity;
}

// 0x61E3F0
void CTaskSimpleUseGun::SetMoveAnim(CPed* ped) {
    const auto
        animGunStand   = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_GUN_STAND),
        animGunMoveFwd = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_GUNMOVE_FWD),
        animGunMoveL   = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_GUNMOVE_L),
        animGunMoveBwd = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_GUNMOVE_BWD),
        animGunMoveR   = RpAnimBlendClumpGetAssociation(ped->GetRpClump(), ANIM_ID_GUNMOVE_R);

    // 0x61E444
    if (ped->bIsDucking) {
        if (const auto tDuck = ped->GetIntelligence()->GetTaskDuck()) {
            switch (m_LastCmd) {
            case eGunCommand::FIRE:
            case eGunCommand::FIREBURST:
            case eGunCommand::RELOAD:
                tDuck->ForceStopMove();
            }
        }
        m_MoveCmd = {0.f, 0.f};
        m_HasMoveControl = false;
        return;
    }

    // 0x61E4A5
    const auto isInMoveControl = m_HasMoveControl && (m_LastCmd != eGunCommand::FIRE || m_WeaponInfo->flags.bMoveFire);
    const auto moveCmdDist = isInMoveControl
        ? m_MoveCmd.Magnitude()
        : 0.f;
    const auto moveCmdAbsSum = isInMoveControl
        ? std::abs(m_MoveCmd.x) + std::abs(m_MoveCmd.y)
        : 0.f;

    const auto DoBlendAnim = [ped](AnimationId animId, float blendDelta) {
        return CAnimManager::BlendAnimation(ped->GetRpClump(), ANIM_GROUP_DEFAULT, animId, blendDelta);
    };

    // 0x61E505
    if (moveCmdAbsSum < 0.1f) {
        const auto moveAnim = m_WeaponInfo->flags.bAimWithArm
            ? ped->IsGangster() || ped->IsCop()
                ? DoBlendAnim(ANIM_ID_GANG_GUNSTAND, 4.f) // Why is this used for cops? (0x61E529, inverted)
                : DoBlendAnim(ANIM_ID_IDLE, 8.f)
            : DoBlendAnim(ANIM_ID_GUN_STAND, 8.f);

        const auto resetPlayTime = moveAnim && moveAnim->GetBlendAmount() > 0.95f;
        for (const auto anim : { animGunMoveBwd, animGunMoveL, animGunMoveR, animGunMoveBwd }) {
            if (anim) {
                anim->SetFlag(ANIMATION_IS_PLAYING, false);
                if (resetPlayTime) {
                    anim->SetCurrentTime(0.f);
                }
            }
        }

        if (!notsa::IsFixBugs()) {
            m_MoveCmd = { 0.f, 0.f }; // BUG: At high FPS, m_MoveCmd can never accumulate past the 0.1 threshold, preventing movement while aiming
        }
        m_HasMoveControl = false;

        return; // 0x61E5F3
    }

    // 0x61E5F6 - Blend in new move anim according to the move command
    if (m_WeaponInfo->flags.bAimWithArm) {
        const auto moveAnim = DoBlendAnim(
            m_MoveCmd.x > 0.75f
                ? ANIM_ID_GUNMOVE_R
                : m_MoveCmd.x <= -0.75f
                    ? ANIM_ID_GUNMOVE_L
                    : m_MoveCmd.y <= 0.f
                        ? ANIM_ID_GUNMOVE_FWD
                        : ANIM_ID_GUNMOVE_BWD,
            8.f
        );
        moveAnim->SetFlag(ANIMATION_IS_PLAYING, true);
        ped->SetMoveState(PEDMOVE_NONE);
        return; // 0x61E675
    }

    //
    // Handle anim transition defined by the current move command
    //

    // 0x61E67C - Calculate lerp progress
    float t = 1.f;
    if (const auto aGS = animGunStand) {
        // 0x61E682 - Adjust blend amount
        if (aGS->GetBlendDelta() >= 0.f) {
            aGS->SetBlendDelta(0.f);
            aGS->SetBlendAmount(std::max(0.f, aGS->GetBlendAmount() - CTimer::GetTimeStep() * 0.16f));
        }

        // Calculate lerp t
        t = 1.f - std::clamp(CTimer::GetTimeStep() * aGS->GetBlendDelta() * 0.02f + aGS->GetBlendAmount(), 0.f, 1.f);
    }

    // Process transition from one gun move to another
    const auto DoTransitionGunMoveAnim = [&, ped, this](CAnimBlendAssociation* fromAnim, CAnimBlendAssociation* toAnim, AnimationId toAnimId, float toAnimBlendAmnt) {
        // If previous anim exists, blend it out
        if (fromAnim) {
            fromAnim->SetBlendAmount(0.f);
            fromAnim->SetBlendDelta(0.f);
        }

        // If new anim doesn't exists, create it
        if (!toAnim) {
            toAnim = CAnimManager::AddAnimation(ped->GetRpClump(),ANIM_GROUP_DEFAULT, toAnimId);
        }

        // Set params of the anim we're transitioning to
        toAnim->SetBlendDelta(0.f);
        toAnim->SetBlendAmount(toAnimBlendAmnt);
        toAnim->SetFlag(ANIMATION_IS_PLAYING, true);
        toAnim->SetSpeed(m_WeaponInfo->m_fMoveSpeed * moveCmdDist);
    };

    const auto moveCmdLerp = m_MoveCmd * (1.f / moveCmdAbsSum) * t;

    // 0x61E73F - Process X axis (left, right)
    if (moveCmdLerp.x < 0.f) { // right => left
        DoTransitionGunMoveAnim(animGunMoveR, animGunMoveL, ANIM_ID_GUNMOVE_L, std::abs(moveCmdLerp.x));
    } else if (moveCmdLerp.x > 0.f) { // left => right
        DoTransitionGunMoveAnim(animGunMoveL, animGunMoveR, ANIM_ID_GUNMOVE_R, std::abs(moveCmdLerp.x));
    }

    // 0x61E7E5 - Process Y axis (fwd, bwd)
    if (moveCmdLerp.y < 0.f) { // bwd => fwd
        DoTransitionGunMoveAnim(animGunMoveBwd, animGunMoveFwd, ANIM_ID_GUNMOVE_FWD, std::abs(moveCmdLerp.y));
    } else if (moveCmdLerp.y > 0.f) { // fwd => bwd
        DoTransitionGunMoveAnim(animGunMoveFwd, animGunMoveBwd, ANIM_ID_GUNMOVE_BWD, std::abs(moveCmdLerp.y));
    }

    // 0x61E88B
    if (m_LastCmd == eGunCommand::AIM) {
        if (m_Anim && m_Anim->GetBlendAmount() > 0.f && m_Anim->GetBlendDelta() >= 0.f && !m_WeaponInfo->flags.bMoveAim) {
            m_Anim->SetBlendDelta(-4.f);
        }
    }

    m_HasMoveControl = false;
}

// 0x624F30
void CTaskSimpleUseGun::StartAnim(CPed* ped) {
    const auto tDuck = ped->bIsDucking
        ? ped->GetIntelligence()->GetTaskDuck()
        : nullptr;

    if (m_Anim) {
        if (m_NextCmd == eGunCommand::END_NOW && m_LastCmd <= eGunCommand::FIREBURST) {
            if (m_Anim->GetBlendDelta() > -8.f && m_Anim->GetBlendAmount() > 0.f) {
                m_Anim->SetBlendDelta(-8.f);
            }
        }
        m_Anim->SetDefaultDeleteCallback();
        m_Anim = nullptr;
    }

    switch (m_NextCmd) {                      
    case eGunCommand::AIM:
    case eGunCommand::FIRE:
    case eGunCommand::FIREBURST: { // 0x624FC1
        if (m_NextCmd == eGunCommand::AIM) {
            if (!m_MoveCmd.IsZero() && !m_WeaponInfo->flags.bMoveFire || tDuck && tDuck->IsTaskInUseByOtherTasks()) { // 0x624FC3 - Check if we can fire right now
                switch (m_LastCmd) {
                case eGunCommand::FIRE:
                case eGunCommand::FIREBURST:
                    m_LastCmd = eGunCommand::AIM;
                }
                return; // Nope, we can't fire right now, so continue aiming
            }
        } else if (tDuck && tDuck->StopFireGun()) {
            return;
        }

        if (m_NextCmd == eGunCommand::FIREBURST) {
            m_BurstShots = m_BurstLength;
        }

        m_Anim = CAnimManager::BlendAnimation( // 0x62503A
            ped->GetRpClump(),
            m_WeaponInfo->m_eAnimGroup,
            ped->bIsDucking && m_WeaponInfo->flags.bCrouchFire
                ? ANIM_ID_CROUCHFIRE
                : ANIM_ID_FIRE
        );

        if (m_LastCmd == eGunCommand::RELOAD) {
            if (m_WeaponInfo->flags.bReload2Start) {
                m_Anim->SetCurrentTime(m_WeaponInfo->GetAnimLoopStart(ped->bIsDucking));
                m_Anim->SetFlag(ANIMATION_IS_PLAYING, false);
            }
        }

        m_Anim->SetFinishCallback(FinishGunAnimCB, this);

        break;
    }
    case eGunCommand::RELOAD: { // 0x6250E5
        if (tDuck && tDuck->StopFireGun()) {
            return;
        }

        if (m_LastCmd != eGunCommand::RELOAD) {
            m_BurstShots = m_WeaponInfo->flags.bTwinPistol ? 2 : 1;
        }

        if (m_BurstShots > 0) { // 0x625196 - Inverted
            m_Anim = CAnimManager::BlendAnimation( // 0x62511C
                ped->GetRpClump(),
                m_WeaponInfo->m_eAnimGroup,
                ped->bIsDucking && m_WeaponInfo->flags.bCrouchFire
                    ? ANIM_ID_CROUCHRELOAD
                    : ANIM_ID_RELOAD
            );
            m_Anim->Start();
            m_Anim->SetFinishCallback(FinishGunAnimCB, this);

            m_BurstShots--;
        } else {
            m_NextCmd = eGunCommand::NONE;
        }

        break;
    }
    case eGunCommand::PISTOLWHIP: {
        if (tDuck && tDuck->StopFireGun()) {
            return;
        }
        m_Anim = CAnimManager::BlendAnimation( // 0x6251DC
            ped->GetRpClump(),
            CTaskSimpleFight::m_aComboData[12].m_nAnimGroup,
            ped->bIsDucking
                ? ANIM_ID_FIGHT_2
                : ANIM_ID_FIGHT_1
        );
        m_Anim->SetFinishCallback(FinishGunAnimCB, this);
        break;
    }
    case eGunCommand::NONE:
    case eGunCommand::END_LEISURE: { // 0x62520A [Moved here for readability]
        RemoveStanceAnims(ped, 4.f);
        m_IsFinished = true;
        break;
    }
    case eGunCommand::END_NOW: { // 0x625214
        RemoveStanceAnims(ped, 8.f);
        m_IsFinished = true;
        break;
    }
    }

    m_LastCmd = std::exchange(m_NextCmd, eGunCommand::NONE);
}

// 0x0x61E160
void CTaskSimpleUseGun::StartCountDown(uint8 numIdleFrames, char isMax) {
    m_CountDownFrames = isMax ? std::min(numIdleFrames, m_CountDownFrames) : numIdleFrames;
    m_HasFiredGun     = false;
}
