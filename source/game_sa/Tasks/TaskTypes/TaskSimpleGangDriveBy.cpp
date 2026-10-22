#include "StdInc.h"

#include "TaskSimpleGangDriveBy.h"

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

CTask* CTaskSimpleGangDriveBy::Clone() const {
    return plugin::CallMethodAndReturn<CTask*, 0x6236D0, const CTask*>(this);
}

bool CTaskSimpleGangDriveBy::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event)
{
    return plugin::CallMethodAndReturn<bool, 0x62D290, CTask*, CPed*, int32, const CEvent*>(this, ped, priority, event);
}

bool CTaskSimpleGangDriveBy::ProcessPed(CPed* ped)
{
    return plugin::CallMethodAndReturn<bool, 0x62D3B0, CTask*, CPed*>(this, ped);
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
