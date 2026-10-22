#include "StdInc.h"

#include "TaskSimpleGangDriveBy.h"
#include "EventDamage.h"
#include "General.h"
#include "PlayerInfo.h"
#include "PlayerPed.h"
#include "Timer.h"
#include "WeaponInfo.h"

/*!
 * @addr 0x621BE0
 * @brief Called when an animation of this task is deleted, so the task stops pointing at it.
 */
static void AnimDeleteCallback(CAnimBlendAssociation* anim, void* data) {
    auto* const task = static_cast<CTaskSimpleGangDriveBy*>(data);
    if (task->m_pAnimAssoc == anim) {
        task->m_pAnimAssoc = nullptr;
    }
}

/*!
 * @addr 0x627B20
 * @brief Makes sure the required animation block is streamed in and then plays/keeps the
 *        animation the current command asks for.
 *
 * @return If the ped is (now) playing the required animation properly
 *
 * @note Not a member of the class in the repo (retail has several such file-local helpers).
 */
static bool ProcessAnimCommand(CTaskSimpleGangDriveBy& task, CPed* ped) {
    // Stream in the block of the required animation group
    auto* animBlock = CAnimManager::GetAssocGroups()[task.m_nRequiredAnimGroup].m_AnimBlock;
    if (!animBlock) {
        animBlock = CAnimManager::GetAnimationBlock(CAnimManager::GetAnimBlockName(task.m_nRequiredAnimGroup));
    }
    if (!animBlock) { // Retail would dereference the null pointer here
        return false;
    }
    // Retail derives it from the block pointer; the equivalent public API looks it up by name
    const auto blockIdx = CAnimManager::GetAnimationBlockIndex(CAnimManager::GetAnimBlockName(task.m_nRequiredAnimGroup));
    if (!animBlock->IsLoaded) {
        CStreaming::RequestModel(IFPToModelId(blockIdx), STREAMING_KEEP_IN_MEMORY);
        return false;
    }
    if (!task.m_bAnimsReferenced) {
        CAnimManager::AddAnimBlockRef(blockIdx);
        task.m_bAnimsReferenced = true;
    }

    if (task.m_nRequiredAnimID == ANIM_ID_NO_ANIMATION_SET && !task.m_pAnimAssoc) {
        return false;
    }

    if (!task.m_pAnimAssoc) {
        // No animation yet - only start one once the target is in weapon range (and not between bursts)
        if (task.m_nAttackTimer > 0 || !task.m_bInWeaponRange) {
            return false;
        }
    } else if (task.m_pAnimAssoc->m_AnimId == (AnimationId)task.m_nRequiredAnimID) {
        // Already playing the right animation
        goto CheckAnimState;
    } else if (   task.m_nNextCommand != 1
               && task.m_nNextCommand != 2
               && task.m_nNextCommand != 3
    ) {
        goto CheckAnimState;
    } else if (task.m_nLastCommand >= 4) {
        goto CheckAnimState;
    }

    {
        const bool hadAnim = task.m_pAnimAssoc != nullptr;
        if (hadAnim) {
            task.m_pAnimAssoc->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
            task.m_pAnimAssoc->m_Flags &= ~ANIMATION_IS_PLAYING;
        }

        task.m_pAnimAssoc = CAnimManager::BlendAnimation(
            ped->GetRpClump(),
            task.m_nRequiredAnimGroup,
            (AnimationId)task.m_nRequiredAnimID,
            4.0f
        );
        task.m_pAnimAssoc->SetDeleteCallback(AnimDeleteCallback, &task);

        if (task.m_nNextCommand == 3 && task.m_nBurstShots <= 1 && task.m_pWeaponInfo) {
            task.m_nBurstShots = (int8)CGeneral::GetRandomNumberInRange(2, (int32)task.m_pWeaponInfo->m_nAmmoClip);
        }

        if (hadAnim) {
            // TODO(re): Retail syncs the new animation's time with the global float at `0x8D2E7C`
            //           (`SetCurrentTime(...)`), but that global's owner/name is not established.
            task.m_pAnimAssoc->m_Flags &= ~ANIMATION_IS_PLAYING;
            task.m_nLastCommand = 1;
            task.m_nNextCommand = 0;
            if (!ped->IsPlayer()) {
                task.m_nAttackTimer = 100;
            }
        } else {
            task.m_nLastCommand = task.m_nNextCommand;
            task.m_nNextCommand = 0;
        }
    }

CheckAnimState:
    const auto* const anim = task.m_pAnimAssoc;
    if (!anim) {
        return false;
    }
    if (anim->m_BlendAmount <= 0.9f) {
        return false;
    }
    if (anim->m_BlendDelta < 0.0f) {
        return false;
    }
    return true;
}

CTaskSimpleGangDriveBy::CTaskSimpleGangDriveBy(CEntity* target, const CVector* targetPos, float abortRange, int8 frequencyPercentage, eDrivebyStyle drivebyStyle, bool seatRHS) {
    m_bSeatRHS             = seatRHS;
    m_nDrivebyStyle        = drivebyStyle;
    m_fAbortRange          = abortRange;
    m_pTargetEntity        = target;
    m_nFrequencyPercentage = frequencyPercentage;
    m_bIsFinished          = false;
    m_bAnimsReferenced     = false;
    m_bInRangeToShoot      = false;
    m_bInWeaponRange       = false;
    m_bReachedAbortRange   = false;
    m_bFromScriptCommand   = false;
    m_nBurstShots          = -1;
    m_nFakeShootDirn       = -1;
    m_nAttackTimer         = -1;
    m_nLastCommand         = 0;
    m_nNextCommand         = 1;
    m_nLOSCheckTime        = 0;
    m_nLOSBlocked          = true;
    m_pAnimAssoc           = nullptr;
    m_nRequiredAnimID      = ANIM_ID_NO_ANIMATION_SET;
    m_nRequiredAnimGroup   = ANIM_GROUP_DEFAULT;
    m_pWeaponInfo          = nullptr;
    CEntity::SafeRegisterRef(m_pTargetEntity);
    if (targetPos) {
        m_vecCoords = *targetPos;
    }
}

CTaskSimpleGangDriveBy::~CTaskSimpleGangDriveBy()
{
    if (m_bAnimsReferenced)
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex(m_nRequiredAnimGroup));

    if (m_pAnimAssoc)
        m_pAnimAssoc->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);

    CEntity::SafeCleanUpRef(m_pTargetEntity);
}

// 0x6236D0
CTask* CTaskSimpleGangDriveBy::Clone() const {
    auto* const clone = new CTaskSimpleGangDriveBy(
        m_pTargetEntity,
        &m_vecCoords,
        m_fAbortRange,
        m_nFrequencyPercentage,
        m_nDrivebyStyle,
        m_bSeatRHS
    );
    clone->m_bFromScriptCommand = m_bFromScriptCommand;
    return clone;
}

// 0x62D290
bool CTaskSimpleGangDriveBy::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event)
{
    if (priority != ABORT_PRIORITY_URGENT && priority != ABORT_PRIORITY_IMMEDIATE) {
        m_nNextCommand = 6;
        return false;
    }

    // Only a fatal damage event can interrupt a drive-by
    if (event && event->GetEventType() == EVENT_DAMAGE) {
        const auto* const damageEvent = static_cast<const CEventDamage*>(event);
        if (!damageEvent->m_damageResponse.m_bHealthZero || !damageEvent->m_bAddToEventGroup) {
            return false;
        }
    }

    m_nNextCommand = 7;
    if (ProcessAnimCommand(*this, ped)) {
        m_nLastCommand = m_nNextCommand;

        if (priority == ABORT_PRIORITY_IMMEDIATE) {
            m_pAnimAssoc->m_BlendDelta = -1000.0f;
        } else {
            m_pAnimAssoc->m_Flags |= ANIMATION_IS_PLAYING;
        }
    }

    if (auto* const playerData = ped->GetPlayerData()) {
        playerData->m_bFreeAiming = true;
    }

    if (ped->IsPlayer()) {
        static_cast<CPlayerPed*>(ped)->GetPlayerInfoForThisPlayerPed()->m_bCanDoDriveBy = true;
    }

    // Fade the animation out (unless it's already playing) and detach from it
    if (m_pAnimAssoc && !m_pAnimAssoc->IsPlaying()) {
        if (   m_pAnimAssoc->m_BlendDelta > 0.0f
            || (m_pAnimAssoc->m_BlendAmount > 0.0f && m_pAnimAssoc->m_BlendDelta == 0.0f)
        ) {
            m_pAnimAssoc->m_BlendDelta = -8.0f;
        }
    }

    if (m_pAnimAssoc) {
        m_pAnimAssoc->SetDeleteCallback(CDefaultAnimCallback::DefaultAnimCB, nullptr);
        m_pAnimAssoc = nullptr;
    }

    m_bIsFinished = true;
    return true;
}

// 0x62D3B0
bool CTaskSimpleGangDriveBy::ProcessPed(CPed* ped)
{
    // TODO(re): The retail body is ~0x600 bytes long and drives a command table, calling the
    //           not-yet-reversed helpers 0x621960 (player aiming) and 0x627600 (AI aiming).
    //           Only the entry checks are implemented for now.
    const auto weaponType = ped->GetActiveWeapon().m_Type;
    const auto weaponInfo = CWeaponInfo::GetWeaponInfo(weaponType, ped->GetWeaponSkill());
    if (weaponInfo && weaponInfo->m_nWeaponFire != WEAPON_FIRE_INSTANT_HIT) {
        return true;
    }
    if (ped->m_pVehicle && !ped->m_pVehicle->CanPedLeanOut(ped)) {
        return true;
    }
    if (m_bIsFinished) {
        return true;
    }
    if (!m_nLOSCheckTime) {
        m_nLOSCheckTime = CTimer::GetTimeInMS() + CGeneral::GetRandomNumberInRange(0, 1000);
    }
    if (!m_pWeaponInfo) {
        m_pWeaponInfo = weaponInfo;
    }

    // TODO(re): 0x62D458 onwards: abort checks (`m_pPedFlags` bit 8, health), vehicle window
    //           flags, weapon model, the `ProcessAnimCommand`/`FireGun` state machine and the
    //           final `SetPedPosition` fix-up.

    return m_bIsFinished;
}

// 0x627CC0
void CTaskSimpleGangDriveBy::FireGun(CPed* shooter) {
    // Muzzle position, in world space
    CVector muzzlePos = m_pWeaponInfo->m_vecFireOffset;
    const auto hierarchy = GetAnimHierarchyFromSkinClump(shooter->GetRpClump());
    RwV3dTransformPoints(&muzzlePos, &muzzlePos, 1, &RpHAnimHierarchyGetMatrixArray(hierarchy)[RpHAnimIDGetIndex(hierarchy, shooter->m_apBones[PED_NODE_RIGHT_HAND]->BoneTag)]);

    const auto veh      = shooter->m_pVehicle;
    const auto colModel = veh->GetModelInfo()->GetColModel();

    // Muzzle position, relative to the vehicle's position
    const auto muzzleOffset = muzzlePos - veh->GetPosition();

    // Aim direction
    CVector aimDir;
    if (shooter->IsPlayer()) {
        // Aim towards where the camera is pointing
        const auto& cam = TheCamera.GetActiveCam();
        float distance = 20.0f;
        if (cam.m_nMode == MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
            distance = (shooter->GetPosition() - cam.m_vecSource).Magnitude() + 20.0f;
        } else if (cam.m_nMode != MODE_AIMWEAPON_FROMCAR) {
            return;
        }
        CVector camSource; // Unused, but has to be passed anyhow
        TheCamera.Find3rdPersonCamTargetVector(distance, muzzlePos, camSource, aimDir);
        aimDir -= muzzlePos;
        shooter->GetPlayerData()->m_fAttackButtonCounter = 0.0f;
    } else if (const auto target = m_pTargetEntity) {
        aimDir = target->GetMatrix().TransformPoint(m_vecCoords) - muzzlePos;
    } else {
        aimDir = m_vecCoords - muzzlePos;
    }
    aimDir.Normalise();

    // Move the target point onto the outer face of the vehicle's bounding box (The face we're shooting towards)
    CVector targetPos = muzzlePos;
    const auto IsPlayingAnim = [this](auto... animIds) {
        return m_pAnimAssoc && ((m_pAnimAssoc->m_AnimId == animIds) || ...);
    };
    const auto& bbox   = colModel->m_boundBox;
    const auto& vehRgt = veh->GetRight();
    const auto& vehFwd = veh->GetForward();
    if (IsPlayingAnim(ANIM_ID_DRIVEBYLHS, ANIM_ID_DRIVEBYTOP_RHS) || m_nFakeShootDirn == 1) {
        const auto d = -DotProduct(aimDir, vehRgt);
        if (d > 0.1f) {
            targetPos = muzzlePos + aimDir * ((bbox.m_vecMin.x - DotProduct(muzzleOffset, vehRgt) - 0.2f) / d);
        }
    } else if (IsPlayingAnim(ANIM_ID_DRIVEBYRHS, ANIM_ID_DRIVEBYTOP_LHS) || m_nFakeShootDirn == 3) {
        const auto d = DotProduct(aimDir, vehRgt);
        if (d > 0.1f) {
            targetPos = muzzlePos + aimDir * ((bbox.m_vecMax.x - DotProduct(muzzleOffset, vehRgt) + 0.2f) / d);
        }
    } else if (IsPlayingAnim(ANIM_ID_DRIVEBYLHS_BWD, ANIM_ID_DRIVEBYRHS_BWD) || m_nFakeShootDirn == 2) {
        const auto d = -DotProduct(aimDir, vehFwd);
        if (d > 0.1f) {
            targetPos = muzzlePos + aimDir * ((bbox.m_vecMin.y - DotProduct(muzzleOffset, vehFwd) - 0.2f) / d);
        }
    } else {
        const auto d = DotProduct(aimDir, vehFwd);
        if (d > 0.1f) {
            targetPos = muzzlePos + aimDir * ((bbox.m_vecMax.y - DotProduct(muzzleOffset, vehFwd) + 0.2f) / d);
        }
    }

    shooter->m_pedIK.bGunReachedTarget = true;

    auto& weapon = shooter->GetActiveWeapon();
    if (m_vecCoords.IsZero()) {
        weapon.Fire(shooter, &muzzlePos, &muzzlePos, m_pTargetEntity, &targetPos, nullptr);
    } else {
        auto targetCoords = m_vecCoords;
        if (m_pTargetEntity) {
            targetCoords = m_pTargetEntity->GetMatrix().TransformPoint(m_vecCoords);
        }
        weapon.Fire(shooter, &muzzlePos, &muzzlePos, m_pTargetEntity, &targetCoords, &targetPos);
    }

    shooter->DoGunFlash(250, false);
}
