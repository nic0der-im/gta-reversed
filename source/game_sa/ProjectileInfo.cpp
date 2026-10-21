#include "StdInc.h"

#include "ProjectileInfo.h"

#include "Entity/Object/Projectile.h"
#include "Radar.h"
#include "World.h"
#include "Pools/Pools.h"
#include "Collision/Box.h"

void CProjectileInfo::InjectHooks() {
    RH_ScopedClass(CProjectileInfo);
    RH_ScopedCategoryGlobal();

    // Install("CProjectileInfo", "", , &CProjectileInfo::);
    RH_ScopedInstall(Initialise, 0x737B40);
    RH_ScopedInstall(Shutdown, 0x737BC0);
    RH_ScopedInstall(GetProjectileInfo, 0x737BF0);
    RH_ScopedInstall(RemoveNotAdd, 0x737C00);
    RH_ScopedInstall(AddProjectile, 0x737C80);
    RH_ScopedInstall(RemoveDetonatorProjectiles, 0x738860);
    RH_ScopedInstall(RemoveProjectile, 0x7388F0);
    RH_ScopedInstall(Update, 0x738B20);
    RH_ScopedInstall(IsProjectileInRange, 0x739860);
    RH_ScopedInstall(RemoveAllProjectiles, 0x7399B0);
    RH_ScopedInstall(RemoveIfThisIsAProjectile, 0x739A40);
    RH_ScopedInstall(RemoveFXSystem, 0x737B80);
}

// 0x737B40
void CProjectileInfo::Initialise() {
    for (auto& projectile : ms_apProjectile) {
        projectile = nullptr;
    }

    for (auto& info : gaProjectileInfo) {
        info.m_nWeaponType  = WEAPON_GRENADE; // Not 0 - `WEAPON_GRENADE` is the "unused slot" sentinel
        info.m_pCreator     = nullptr;
        info.m_nDestroyTime = 0;
        info.m_bActive      = false;
        info.m_pFxSystem    = nullptr;
    }
}

// 0x737BC0
void CProjectileInfo::Shutdown() {
    for (auto& info : gaProjectileInfo) {
        if (info.m_pFxSystem) {
            g_fxMan.DestroyFxSystem(info.m_pFxSystem);
            info.m_pFxSystem = nullptr;
        }
    }
}

// 0x737BF0
CProjectileInfo* CProjectileInfo::GetProjectileInfo(int32 infoId) {
    return &gaProjectileInfo[infoId];
}

// 0x737C00
void CProjectileInfo::RemoveNotAdd(CEntity* creator, eWeaponType weaponType, CVector posn) {
    eExplosionType explosionType;
    switch (weaponType) {
    case WEAPON_GRENADE:
    case WEAPON_REMOTE_SATCHEL_CHARGE:
        explosionType = EXPLOSION_GRENADE;
        break;
    case WEAPON_MOLOTOV:
        explosionType = EXPLOSION_MOLOTOV;
        break;
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS:
        explosionType = EXPLOSION_ROCKET;
        break;
    default:
        return;
    }

    CExplosion::AddExplosion(nullptr, creator, explosionType, posn, 0, true, -1.0f, false);
}

// 0x737C80
bool CProjectileInfo::AddProjectile(CEntity* creator, eWeaponType projectileType, CVector origin, float force, const CVector* dir, CEntity* target) {
    int32   destroyTime           = 0; // Note: The original leaves this uninitialized for unhandled weapon types
    float   speed                 = 0.0f;
    float   elasticity            = 0.75f;
    bool    useGravity            = true;
    bool    useGrenadeObjectData  = false;
    CVector velocity{};
    CMatrix matrix;

    // Creator's heading - From the matrix if it has one, else from the placement
    const auto getHeading = [creator]() -> float {
        if (creator->m_matrix) {
            const auto& forward = creator->GetMatrix().GetForward();
            return std::atan2(-forward.x, forward.y);
        }
        return creator->m_placement.m_fHeading;
    };

    switch (projectileType) {
    case WEAPON_GRENADE:
    case WEAPON_REMOTE_SATCHEL_CHARGE: {
        destroyTime = CTimer::GetTimeInMS() + 2000;
        speed       = force == 0.0f ? 0.0f : force * 0.22f + 0.15f;
        if (projectileType == WEAPON_REMOTE_SATCHEL_CHARGE) {
            speed *= 0.5f;
        }

        auto heading = getHeading();
        if (creator->GetType() == ENTITY_TYPE_VEHICLE) {
            heading = CGeneral::LimitRadianAngle(heading + PI);
        }

        matrix.SetTranslate({});
        matrix.RotateZ(heading);
        matrix.GetPosition() += origin;

        velocity = CVector{
            std::sin(heading) * speed * -1.0f,
            std::cos(heading) * speed,
            (force + 1.0f) * 0.4f * speed
        };

        if (creator->m_nModelIndex == MODEL_SENTINEL) { // Sentinels throw their speed along
            velocity += static_cast<CPhysical*>(creator)->m_vecMoveSpeed;
        }

        useGrenadeObjectData = true;
        elasticity           = projectileType == WEAPON_REMOTE_SATCHEL_CHARGE ? 0.03f : 0.5f;
        break;
    }
    case WEAPON_TEARGAS: {
        destroyTime = CTimer::GetTimeInMS() + 20000;
        speed       = force == 0.0f ? 0.0f : force * 0.22f + 0.15f;

        const auto heading = getHeading(); // Note: No PI flip for vehicles here

        matrix.SetTranslate({});
        matrix.RotateZ(heading);
        matrix.GetPosition() += origin;

        velocity = CVector{
            std::sin(heading) * speed * -1.0f,
            std::cos(heading) * speed,
            (force + 1.0f) * 0.4f * speed
        };

        useGrenadeObjectData = true;
        elasticity           = 0.5f;
        break;
    }
    case WEAPON_MOLOTOV: {
        destroyTime = CTimer::GetTimeInMS() + 2000;
        speed       = force * 0.22f + 0.15f;
        if (speed < 0.2f) {
            speed = 0.2f;
        }

        const auto heading = getHeading();

        matrix.SetTranslate({});
        matrix.RotateZ(heading);
        matrix.GetPosition() += origin;

        velocity = CVector{
            std::sin(heading) * speed * -1.0f,
            std::cos(heading) * speed,
            (force * 0.2f + 0.4f) * speed
        };

        elasticity = 0.5f;
        break;
    }
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS: {
        if (projectileType == WEAPON_ROCKET) {
            destroyTime = CTimer::GetTimeInMS() + 3000;
            speed       = 0.4f;
        } else {
            destroyTime = CTimer::GetTimeInMS() + 10000;
            speed       = 0.2f;
        }

        if (creator->GetType() == ENTITY_TYPE_VEHICLE) {
            matrix               = creator->GetMatrix();
            matrix.GetPosition() = origin;
            speed += static_cast<CPhysical*>(creator)->m_vecMoveSpeed.Magnitude();
        } else if (creator->GetType() == ENTITY_TYPE_PED && static_cast<CPed*>(creator)->IsPlayer()) {
            // Launch along the active camera's view
            const auto& cam = TheCamera.GetActiveCam();
            matrix.GetForward()  = cam.m_vecFront;
            matrix.GetUp()       = cam.m_vecUp;
            matrix.GetRight()    = CrossProduct(cam.m_vecUp, cam.m_vecFront);
            matrix.GetPosition() = origin;
        } else if (dir) {
            matrix.GetForward()  = *dir;
            matrix.GetRight()    = creator->GetMatrix().GetRight();
            matrix.GetUp()       = CrossProduct(matrix.GetRight(), matrix.GetForward());
            matrix.GetPosition() = origin;
        } else {
            matrix = creator->GetMatrix();
        }

        velocity   = matrix.TransformVector(CVector{ 0.0f, speed, 0.0f }); // Forward * speed
        useGravity = false;
        break;
    }
    case WEAPON_FREEFALL_BOMB:
    case WEAPON_FLARE: {
        if (projectileType == WEAPON_FREEFALL_BOMB) {
            destroyTime = CTimer::GetTimeInMS() + 2000000;
        } else {
            CStreaming::RequestModel(ModelIndices::MI_FLARE, 0);
            destroyTime = CTimer::GetTimeInMS() + 10000;
        }

        if (creator->GetType() < ENTITY_TYPE_VEHICLE || creator->GetType() > ENTITY_TYPE_OBJECT) {
            velocity = CVector{};
        } else {
            velocity = static_cast<CPhysical*>(creator)->m_vecMoveSpeed;
        }

        matrix               = creator->GetMatrix();
        matrix.GetPosition() = origin;
        break;
    }
    default:
        break;
    }

    // Find a free slot
    int32 slot = -1;
    for (auto i = 0; i < (int32)gaProjectileInfo.size(); i++) {
        if (!gaProjectileInfo[i].m_bActive) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        return false;
    }

    switch (projectileType) {
    case WEAPON_GRENADE:
    case WEAPON_TEARGAS:
    case WEAPON_MOLOTOV:
    case WEAPON_REMOTE_SATCHEL_CHARGE: {
        auto* projectile = new CProjectile(CWeaponInfo::GetWeaponInfo(projectileType, eWeaponSkill::STD)->m_nModelId1);
        ms_apProjectile[slot] = projectile;
        if (projectile) {
            const auto colModel = CModelInfo::GetModelInfo(projectile->m_nModelIndex)->GetColModel();
            auto* colData       = colModel->m_pColData;
            if (!colData) {
                colModel->AllocateData(1, 0, 0, 0, 0, false);
                colModel->GetData()->m_pSpheres->Set(colModel->m_boundSphere.m_fRadius * 0.75f, colModel->m_boundSphere.m_vecCenter, SURFACE_GIRDER, 0, tColLighting(0xFF));
            } else if (colData->m_nNumSpheres == 0 && !colData->m_pSpheres) {
                colData->m_nNumSpheres = 1;
                colData->m_pSpheres    = static_cast<CColSphere*>(CMemoryMgr::Malloc(sizeof(CColSphere), 0));
                colData->m_pSpheres->Set(colModel->m_boundSphere.m_fRadius * 0.75f, colModel->m_boundSphere.m_vecCenter, SURFACE_GIRDER, 0, tColLighting(0xFF));
            }
        }
        break;
    }
    case WEAPON_ROCKET:
    case WEAPON_ROCKET_HS:
    case WEAPON_FREEFALL_BOMB:
        ms_apProjectile[slot] = new CProjectile(CWeaponInfo::GetWeaponInfo(projectileType, eWeaponSkill::STD)->m_nModelId1);
        break;
    case WEAPON_FLARE:
        if ((ms_apProjectile[slot] = new CProjectile(ModelIndices::MI_FLARE))) {
            ms_apProjectile[slot]->m_fAirResistance = 0.9f;
        }
        break;
    default:
        break;
    }

    auto* projectile = ms_apProjectile[slot];
    if (!projectile) {
        return false;
    }

    auto& info = gaProjectileInfo[slot];

    info.m_nWeaponType = projectileType;
    info.m_pCreator    = creator;
    CEntity::RegisterReference(info.m_pCreator);

    projectile->SetMatrix(matrix);
    projectile->m_vecMoveSpeed = velocity;
    projectile->physicalFlags.bApplyGravity = useGravity;

    info.m_nDestroyTime       = destroyTime;
    projectile->m_fElasticity = elasticity;

    if (useGrenadeObjectData) {
        projectile->m_pObjectInfo = &CObjectData::ms_aObjectInfo[4]; // COL_SPECIAL_RESPONSE_GRENADE
    }

    info.m_pVictim = target;
    if (target) {
        CEntity::RegisterReference(info.m_pVictim);
    }

    info.m_bActive = true;

    CWorld::Add(projectile);
    CEntity::RegisterReference(ms_apProjectile[slot]);
    info.m_vecLastPosn = projectile->GetPosition();

    if (projectileType == WEAPON_TEARGAS) {
        info.m_pFxSystem = g_fxMan.CreateFxSystem("teargasAD", CVector{}, projectile->GetModellingMatrix(), false);
        if (info.m_pFxSystem) {
            info.m_pFxSystem->Play();
        }
    }

    projectile->m_pEntityIgnoredCollision = creator;
    projectile->physicalFlags.bCanBeCollidedWith = true;

    if (creator
        && (creator->GetType() == ENTITY_TYPE_VEHICLE || creator->GetType() == ENTITY_TYPE_PED || creator->GetType() == ENTITY_TYPE_OBJECT)
        && !static_cast<CPhysical*>(creator)->m_pEntityIgnoredCollision
    ) {
        static_cast<CPhysical*>(creator)->m_pEntityIgnoredCollision = creator;
    }

    if (projectileType == WEAPON_ROCKET_HS) {
        // Note: The original passes a 5th argument ("CODEPRJ") to `CRadar::SetEntityBlip`;
        //       the repo's 4-argument declaration is used here instead.
        const auto blip = CRadar::SetEntityBlip(BLIP_OBJECT, GetObjectPool()->GetRef(projectile), 0xFF0000FF, BLIP_DISPLAY_BLIPONLY);
        CRadar::ChangeBlipScale(blip, 1);

        const auto byPlayer = creator == FindPlayerPed(-1) || creator == FindPlayerVehicle(-1, false);
        CRadar::ChangeBlipColour(blip, static_cast<eBlipColour>(byPlayer ? 0xFFFFFFFF : 0xFF0000FF));
    }

    AudioEngine.ReportWeaponEvent(0x94, projectileType, projectile);
    return true;
}

// 0x738860
void CProjectileInfo::RemoveDetonatorProjectiles() {
    for (auto i = 0; i < (int32)gaProjectileInfo.size(); i++) {
        auto& info       = gaProjectileInfo[i];
        auto* projectile = ms_apProjectile[i];

        if (!info.m_bActive) {
            continue;
        }
        if (static_cast<eWeaponType>(info.m_nWeaponType) != WEAPON_REMOTE_SATCHEL_CHARGE) {
            continue;
        }

        CExplosion::AddExplosion(nullptr, info.m_pCreator, EXPLOSION_GRENADE, projectile->GetPosition(), 0, true, -1.0f, false);

        info.m_bActive = false;
        if (info.m_pFxSystem) {
            info.m_pFxSystem->Kill();
            info.m_pFxSystem = nullptr;
        }

        projectile->m_bRemoveFromWorld = true; // Removed by `CWorld` during its next update
    }
}

// 0x7388F0
void CProjectileInfo::RemoveProjectile(CProjectileInfo* info, CProjectile* projectile) {
    const auto weaponType = static_cast<eWeaponType>(info->m_nWeaponType);
    const auto creator    = info->m_pCreator;
    const auto pos        = projectile->GetPosition();

    switch (weaponType) {
    case WEAPON_GRENADE:
        CExplosion::AddExplosion(nullptr, creator, EXPLOSION_GRENADE, pos, 0, true, -1.0f, false);
        break;
    case WEAPON_MOLOTOV:
        CExplosion::AddExplosion(nullptr, creator, EXPLOSION_MOLOTOV, pos, 0, true, -1.0f, false);
        AudioEngine.ReportObjectDestruction(projectile);
        break;
    case WEAPON_ROCKET: {
        // The vehicle's driver gets the credit, not the vehicle itself
        auto* explosionCreator = creator;
        if (creator && creator->GetType() == ENTITY_TYPE_VEHICLE) {
            explosionCreator = static_cast<CVehicle*>(creator)->m_pDriver;
        }
        CExplosion::AddExplosion(nullptr, explosionCreator, EXPLOSION_ROCKET, pos, 0, true, -1.0f, false);
        break;
    }
    case WEAPON_ROCKET_HS: {
        const auto byPlayer = creator == FindPlayerPed(-1);
        CExplosion::AddExplosion(nullptr, creator, byPlayer ? EXPLOSION_ROCKET : EXPLOSION_WEAK_ROCKET, pos, 0, true, -1.0f, false);
        break;
    }
    case WEAPON_FREEFALL_BOMB:
        CExplosion::AddExplosion(nullptr, creator, EXPLOSION_GRENADE, pos, 0, true, -1.0f, false);
        break;
    default: // Teargas, satchel & flare do not explode
        break;
    }

    info->m_bActive = false;
    if (info->m_pFxSystem) {
        info->m_pFxSystem->Kill();
        info->m_pFxSystem = nullptr;
    }

    CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(projectile));
    CWorld::Remove(projectile);
    delete projectile;
}

// 0x738B20
void CProjectileInfo::Update() {
    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    const auto ClampSpeedTo9_9 = [](CProjectile* projectile) {
        if (const auto speed = projectile->m_vecMoveSpeed.Magnitude(); speed > 9.9f) {
            projectile->m_vecMoveSpeed *= 9.9f / speed;
        }
    };

    for (auto i = 0; i < (int32)gaProjectileInfo.size(); i++) {
        auto* projectile = ms_apProjectile[i];
        auto& info       = gaProjectileInfo[i];

        if (!info.m_bActive) {
            continue;
        }

        // (1) Submerged projectiles stop their FX
        if (projectile->physicalFlags.bSubmergedInWater && info.m_pFxSystem) {
            info.m_pFxSystem->Kill();
            info.m_pFxSystem = nullptr;
        }

        // (2) Drop the creator reference if the ped is gone
        if (info.m_pCreator
            && info.m_pCreator->GetType() == ENTITY_TYPE_PED
            && !static_cast<CPed*>(info.m_pCreator)->IsPointerValid()
        ) {
            info.m_pCreator = nullptr;
        }

        const auto weaponType = static_cast<eWeaponType>(info.m_nWeaponType);

        // (3) Grenade/Satchel/Teargas
        if (weaponType == WEAPON_REMOTE_SATCHEL_CHARGE || weaponType == WEAPON_GRENADE || weaponType == WEAPON_TEARGAS) {
            // Once it comes to rest it stops bouncing
            const auto& speed = projectile->m_vecMoveSpeed;
            if (projectile->m_fElasticity > 0.1f
                && std::abs(speed.x) < 0.05f
                && std::abs(speed.y) < 0.05f
                && std::abs(speed.z) < 0.05f
            ) {
                projectile->m_fElasticity = 0.03f;
            }

            // Teargas chokes nearby peds during the last 17500ms of its 20000ms lifetime
            if (weaponType == WEAPON_TEARGAS
                && info.m_nDestroyTime - 17500 < CTimer::GetTimeInMS()
                && CGeneral::GetRandomNumberInRange<int32>(0, 100) < 10
            ) {
                const auto& pos = projectile->GetPosition();
                CWorld::SetPedsChoking(pos.x, pos.y, pos.z, 6.0f, info.m_pCreator);
            }
        }

        // (4) Rockets emit a smoke trail
        if (weaponType == WEAPON_ROCKET || weaponType == WEAPON_ROCKET_HS) {
            const FxPrtMult_c particleMat{ 0.3f, 0.3f, 0.3f, 0.3f, 0.5f, 1.0f, 0.08f };

            const auto delta  = projectile->m_vecMoveSpeed * CTimer::GetTimeStep();
            auto       amount = static_cast<int32>(std::sqrt(delta.SquaredMagnitude()));
            if (amount < 1) {
                amount = 1;
            }

            if (info.m_pFxSystem) {
                for (auto k = 0; k < amount; k++) {
                    const auto t   = 1.0f - static_cast<float>(k) / static_cast<float>(amount); // 1 at the tail, 0 at the head
                    const auto pos = projectile->GetPosition() - delta * t;

                    CVector randomDir{
                        static_cast<float>(CGeneral::GetRandomNumber()) * 3.051851e-05f * 2.0f - 1.0f,
                        static_cast<float>(CGeneral::GetRandomNumber()) * 3.051851e-05f * 2.0f - 1.0f,
                        static_cast<float>(CGeneral::GetRandomNumber()) * 3.051851e-05f * 2.0f - 1.0f
                    };
                    randomDir.Normalise();

                    auto speedDir = projectile->m_vecMoveSpeed;
                    speedDir.Normalise();

                    info.m_pFxSystem->AddParticle(pos, CrossProduct(speedDir, randomDir) * 1.5f, 0.0f, particleMat, -1.0f, 1.2f, 0.6f, false);
                }
            }

            // TODO(re): the original additionally draws two random values per particle (a 0.25..0.5
            //           grey shade and a 0.08..0.12 lifetime); the transcription did not establish
            //           where they end up (`AddParticle` is called with the fixed `particleMat` above).
        }

        // (5) Lifetime / movement / collision
        if (info.m_nDestroyTime != 0 && info.m_nDestroyTime < CTimer::GetTimeInMS()) {
            // --- Expired ---
            if (weaponType == WEAPON_REMOTE_SATCHEL_CHARGE) {
                // Satchels only despawn once the player lost the detonator
                if (info.m_pCreator
                    && info.m_pCreator->GetType() == ENTITY_TYPE_PED
                    && static_cast<CPed*>(info.m_pCreator)->IsPlayer()
                ) {
                    auto* const ped    = static_cast<CPed*>(info.m_pCreator);
                    const auto& weapon = ped->m_aWeapons[ped->GetWeaponSlot(WEAPON_DETONATOR)];
                    if (weapon.m_Type != WEAPON_DETONATOR || weapon.m_TotalAmmo == 0) {
                        info.m_nDestroyTime = 0; // Never expire again
                    }
                }
            } else {
                RemoveProjectile(&info, projectile);
            }
        } else {
            // --- Still alive ---

            // Any single sphere/collision test blocking the way from the previous position to the
            // current one detonates the projectile, unless it hit its creator or another missile.
            const auto CheckLineOfSight = [&] {
                if (!projectile->physicalFlags.bOnSolidSurface) {
                    CWorld::pIgnoreEntity    = info.m_pCreator;
                    projectile->m_bUsesCollision = false;
                    const auto isClear = CWorld::GetIsLineOfSightClear(info.m_vecLastPosn, projectile->GetPosition(), true, true, true, true, false, false, false);
                    CWorld::pIgnoreEntity    = nullptr;
                    projectile->m_bUsesCollision = true;
                    projectile->m_pEntityIgnoredCollision = info.m_pCreator;
                    if (isClear) {
                        return;
                    }
                }

                auto* const hit = projectile->m_nNumEntitiesCollided ? projectile->m_apCollidedEntities[0] : nullptr;
                if (!hit || (hit != info.m_pCreator && hit->m_nModelIndex != MODEL_MISSILE)) {
                    RemoveProjectile(&info, projectile);
                }
            };

            switch (weaponType) {
            case WEAPON_ROCKET: {
                // Accelerate along the facing
                projectile->m_vecMoveSpeed += projectile->GetMatrix().GetForward() * (CTimer::GetTimeStep() * 0.008f);
                ClampSpeedTo9_9(projectile);
                CheckLineOfSight();
                break;
            }
            case WEAPON_FLARE: {
                CWorld::pIgnoreEntity = info.m_pCreator;
                projectile->m_bUsesCollision = false;
                const auto isClear = CWorld::GetIsLineOfSightClear(info.m_vecLastPosn, projectile->GetPosition(), true, true, true, true, false, false, false);
                projectile->m_bUsesCollision = true;
                CWorld::pIgnoreEntity = nullptr;

                if (!isClear) { // Hit something: freeze at the last clear position
                    projectile->m_vecMoveSpeed = CVector{};
                    projectile->SetPosn(info.m_vecLastPosn);
                }
                break;
            }
            case WEAPON_MOLOTOV:
            case WEAPON_FREEFALL_BOMB: {
                const auto pos = projectile->GetPosition();

                CWorld::pIgnoreEntity = info.m_pCreator;
                projectile->m_bUsesCollision = false;

                // Ignore collisions while still within 2 units of the thrower
                bool ignoreCollisions = false;
                if (info.m_pCreator) {
                    ignoreCollisions = (info.m_vecLastPosn - info.m_pCreator->GetPosition()).SquaredMagnitude() < 2.0f;
                }

                if (!ignoreCollisions) {
                    const auto shouldExplode = projectile->physicalFlags.bOnSolidSurface
                        || !CWorld::GetIsLineOfSightClear(info.m_vecLastPosn, pos, true, true, true, true, false, false, false);
                    if (shouldExplode) {
                        RemoveProjectile(&info, projectile);

                        // NOTE: The original keeps writing to the (just deleted) projectile here;
                        //       only the world state is restored before leaving.
                        CWorld::pIgnoreEntity = nullptr;
                        break;
                    }
                }

                CWorld::pIgnoreEntity = nullptr;
                projectile->m_bUsesCollision = true;
                break;
            }
            case WEAPON_ROCKET_HS: {
                if (info.m_pVictim) {
                    const auto byPlayer = info.m_pCreator == FindPlayerPed(-1) || info.m_pCreator == FindPlayerVehicle(-1, false);

                    if (info.m_pVictim == FindPlayerVehicle(-1, false)) {
                        AudioEngine.ReportFrontendAudioEvent(static_cast<eAudioEvents>(0x65), 0.0f, 1.0f); // Lock tone
                    }

                    const auto forward = projectile->GetMatrix().GetForward();
                    const auto lookAt  = projectile->GetPosition() + forward;

                    const auto victimScore = CWeapon::EvaluateTargetForHeatSeekingMissile(info.m_pVictim, lookAt, forward, 0.6f, true, nullptr);

                    // A flare is preferred if it scores higher
                    CProjectile* bestFlare      = nullptr;
                    float        bestFlareScore = 0.0f;
                    for (auto j = 0; j < (int32)gaProjectileInfo.size(); j++) {
                        if (static_cast<eWeaponType>(gaProjectileInfo[j].m_nWeaponType) != WEAPON_FLARE || !gaProjectileInfo[j].m_bActive) {
                            continue;
                        }
                        if (const auto score = CWeapon::EvaluateTargetForHeatSeekingMissile(ms_apProjectile[j], lookAt, forward, 0.6f, true, nullptr); bestFlareScore <= score) {
                            bestFlareScore = score;
                            bestFlare      = ms_apProjectile[j];
                        }
                    }

                    auto* targetEntity = (bestFlare && bestFlareScore > victimScore) ? static_cast<CEntity*>(bestFlare) : info.m_pVictim;

                    const auto isPlaneTarget = targetEntity->GetType() == ENTITY_TYPE_VEHICLE
                        && byPlayer
                        && static_cast<CVehicle*>(targetEntity)->m_nVehicleSubType == VEHICLE_TYPE_PLANE;

                    const auto distance = (projectile->GetPosition() - targetEntity->GetPosition()).Magnitude();
                    const auto lead     = std::min(distance, isPlaneTarget ? 1.5f : 50.0f);

                    const auto aimPoint = targetEntity->GetPosition() + static_cast<CPhysical*>(targetEntity)->m_vecMoveSpeed * lead;
                    const auto projPos  = isPlaneTarget
                        ? projectile->GetPosition()
                        : projectile->GetPosition() + projectile->m_vecMoveSpeed * 100.0f;

                    CVector steer = aimPoint - projPos;
                    steer.Normalise();

                    // Remove the component opposing the current velocity ("turn towards, never reverse")
                    if (const auto dot = steer.Dot(projectile->m_vecMoveSpeed); dot < 0.0f) {
                        steer -= projectile->m_vecMoveSpeed * dot;
                    }
                    steer.Normalise();

                    auto turnRate = byPlayer ? 0.0117f : 0.009f;
                    auto damping  = 1.0f;
                    if (projectile->m_vecMoveSpeed.Magnitude() > 0.8f) {
                        turnRate *= 1.2f;
                    }
                    if (isPlaneTarget) {
                        damping  = std::pow(0.95f, CTimer::GetTimeStep());
                        turnRate = 0.15f;
                    }

                    const auto previousSpeed = projectile->m_vecMoveSpeed;

                    projectile->m_vecMoveSpeed *= damping;
                    projectile->m_vecMoveSpeed += steer * (turnRate * CTimer::GetTimeStep());
                    ClampSpeedTo9_9(projectile);

                    // The pre-update speed is remembered as the missile's facing
                    projectile->GetMatrix().GetForward() = previousSpeed;
                }
                CheckLineOfSight();
                break;
            }
            default: // Grenades, teargas & satchels get no movement help
                break;
            }
        }

        if (!info.m_bActive) {
            continue; // Removed above - Don't touch the (already deleted) projectile anymore
        }

        info.m_vecLastPosn = projectile->GetPosition();
    }
}

// 0x739860
bool CProjectileInfo::IsProjectileInRange(float x1, float x2, float y1, float y2, float z1, float z2, bool bDestroy) {
    const CBox bb{
        CVector{ x1, y1, z1 },
        CVector{ x2, y2, z2 }
    };
    bool found = false;
    for (auto&& [info, proj] : rngv::zip(gaProjectileInfo, ms_apProjectile)) {
        if (!info.m_bActive) {
            continue;
        }

        if (!IsWeaponTypeProjectile(static_cast<eWeaponType>(info.m_nWeaponType))) {
            continue;
        }

        if (!bb.IsPointInside(proj->GetPosition())) {
            continue;
        }

        found = true;
        if (bDestroy) {
            info.m_bActive = false;
            info.RemoveFXSystem(false);
            CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(proj));
            CWorld::Remove(proj);
            delete proj;
        }
    }
    return found;
}

// 0x7399B0
void CProjectileInfo::RemoveAllProjectiles() {
    for (auto i = 0; i < (int32)gaProjectileInfo.size(); i++) {
        auto& info       = gaProjectileInfo[i];
        auto* projectile = ms_apProjectile[i];

        if (!info.m_bActive) {
            continue;
        }

        info.m_bActive = false;
        if (info.m_pFxSystem) {
            g_fxMan.DestroyFxSystem(info.m_pFxSystem);
            info.m_pFxSystem = nullptr;
        }

        CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(projectile));
        CWorld::Remove(projectile);
        delete projectile;
    }
}

// 0x739A40
bool CProjectileInfo::RemoveIfThisIsAProjectile(CObject* object) {
    int32 i = 0;
    while (!(ms_apProjectile[i] == object && gaProjectileInfo[i].m_bActive)) {
        if (++i > (int32)gaProjectileInfo.size() - 1) {
            return false;
        }
    }

    gaProjectileInfo[i].m_bActive = false;
    if (gaProjectileInfo[i].m_pFxSystem) {
        gaProjectileInfo[i].m_pFxSystem->Kill();
        gaProjectileInfo[i].m_pFxSystem = nullptr;
    }

    CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(ms_apProjectile[i]));
    CWorld::Remove(ms_apProjectile[i]);
    delete ms_apProjectile[i];
    ms_apProjectile[i] = nullptr; // Unlike `RemoveProjectile` the slot is cleared
    return true;
}

// 0x737B80
void CProjectileInfo::RemoveFXSystem(bool bInstantly) {
    if (!m_pFxSystem) {
        return;
    }

    if (bInstantly) {
        g_fxMan.DestroyFxSystem(m_pFxSystem);
    } else {
        m_pFxSystem->Kill();
    }
    m_pFxSystem = nullptr;
}
