#include "StdInc.h"

#include "RoadBlocks.h"
#include "PedPlacement.h"
#include "TaskComplexWanderCop.h"
#include "TaskSimpleStandStill.h"
#include "CarCtrl.h"
#include <extensions/File.hpp>

void CRoadBlocks::InjectHooks() {
    RH_ScopedClass(CRoadBlocks);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x461100);
    RH_ScopedInstall(ClearScriptRoadBlocks, 0x460EC0);
    RH_ScopedInstall(ClearSpaceForRoadBlockObject, 0x461020);
    RH_ScopedInstall(CreateRoadBlockBetween2Points, 0x4619C0);
    RH_ScopedInstall(GenerateRoadBlockPedsForCar, 0x461170);
    RH_ScopedInstall(GenerateRoadBlocks, 0x4629E0);
    RH_ScopedInstall(GetRoadBlockNodeInfo, 0x460EE0);
    RH_ScopedInstall(RegisterScriptRoadBlock, 0x460DF0);
}

// 0x461100
void CRoadBlocks::Init() {
    rng::fill(InOrOut, true);
    GenerateDynamicRoadBlocks = false;

    if (notsa::File rbx("data\\paths\\roadblox.dat", "rb"); rbx) {
        rbx.Read(&NumRoadBlocks, sizeof(int32));
        assert(NumRoadBlocks <= MAX_ROADBLOCKS);
        rbx.Read(RoadBlockNodes.data(), RoadBlockNodes.size() * sizeof(CNodeAddress));
    } else {
        NOTSA_UNREACHABLE("roadblox.dat couldn't be opened!");
    }
    ClearScriptRoadBlocks();
}

// 0x460EC0
void CRoadBlocks::ClearScriptRoadBlocks() {
    for (auto& srb : aScriptRoadBlocks) {
        srb.IsActive = false;
    }
}

// 0x461020
// Returns true if cleared successfully.
bool CRoadBlocks::ClearSpaceForRoadBlockObject(CVector cornerA, CVector cornerB){
    int16 numEntities{};
    CEntity* entities[2]{};
    CWorld::FindObjectsIntersectingCube(
        cornerA,
        cornerB,
        &numEntities,
        std::size(entities),
        entities,
        false,
        true,
        true,
        true,
        false
    );

    if (numEntities > std::size(entities) || numEntities <= 0) {
        return numEntities <= 0;
    }

    const auto Remove = [](CEntity* e) {
        CWorld::Remove(e);
        delete e;
    };

    for (auto* entity : entities | rngv::take(numEntities)) {
        switch (entity->GetType()) {
        case ENTITY_TYPE_VEHICLE:
            if (auto* v = entity->AsVehicle(); !v->CanBeDeleted()) {
                return false;
            } else if (!v->vehicleFlags.bCreateRoadBlockPeds) {
                Remove(v);
            }
            break;
        case ENTITY_TYPE_PED:
            if (auto* p = entity->AsPed(); p->CanBeDeleted()) {
                Remove(p);
            } else {
                return false;
            }
            break;
        case ENTITY_TYPE_OBJECT:
            if (auto* o = entity->AsObject(); o->CanBeDeleted() && o->m_nObjectType != OBJECT_GAME) {
                Remove(o);
            } else {
                return false;
            }
            break;
        default:
            NOTSA_UNREACHABLE();
        }
    }

    return true;
}

// 0x4619C0
void CRoadBlocks::CreateRoadBlockBetween2Points(CVector a, CVector b, bool isGangRoadBlock) {
    const CVector rawDir = b - a;
    const float   dist   = rawDir.Magnitude();
    const CVector mid    = (a + b) * 0.5f;

    CVector dir = rawDir;
    dir.Normalise();

    CVector perp{ dir.y, -dir.x, 0.0f };
    perp.Normalise();

    // Have everything face the side of the road the player is on
    if ((FindPlayerCoors() - mid).Dot(perp) < 0.0f) {
        perp = -perp;
    }

    // Pick the model of the vehicle that's going to be used
    eModelID modelId;
    if (isGangRoadBlock) {
        modelId = CPopulation::PickRiotRoadBlockCar();
        if (modelId == MODEL_INVALID) {
            return;
        }
    } else {
        const auto wanted = FindPlayerWanted();
        if (wanted->AreArmyRequired()) {
            modelId = MODEL_BARRACKS;
        } else if (wanted->AreFbiRequired()) {
            modelId = MODEL_FBIRANCH;
        } else if (wanted->AreSwatRequired()) {
            modelId = MODEL_ENFORCER;
        } else {
            modelId = CStreaming::GetDefaultCopCarModel(false);
        }

        if (!CStreaming::IsModelLoaded(modelId)) {
            modelId = CStreaming::GetDefaultCopCarModel(false);
        }

        if (modelId == MODEL_COPBIKE) { // Not an automobile, so it can't be used
            return;
        }
    }

    // Space needed by a car, depending on how it's parked
    const auto& carBBox    = CModelInfo::GetModelInfo(modelId)->GetColModel()->m_boundBox;
    const auto  carBBoxMin = carBBox.m_vecMin;
    const auto  carBBoxMax = carBBox.m_vecMax;
    const float acrossGap  = (carBBoxMax.x - carBBoxMin.x) + 2.0f; // Parked across the road
    float       alongGap   = (carBBoxMax.y - carBBoxMin.y) + 0.2f; // Parked along the road
    if (isGangRoadBlock) {
        alongGap += 0.5f;
    }

    // Determine how many cars fit along the road, and how each of them is going to be parked
    std::array<bool, 5> acrossRoad{};
    float               remainingDist = dist;
    int32               numCars       = 0;
    for (int32 i = 0; i < (int32)(acrossRoad.size()); i++) {
        acrossRoad[i] = !(modelId == MODEL_BARRACKS || isGangRoadBlock) && (rand() & 1);
        const float gapNeeded = acrossRoad[i] ? acrossGap : alongGap;
        if (remainingDist < gapNeeded) {
            break;
        }
        remainingDist -= gapNeeded;
        numCars++;
    }

    // The cars are spaced evenly, the remaining space being split between them
    const float step = remainingDist / (float)(numCars + 1);

    if (numCars > 0) {
        const float advanceBetweenCars = alongGap * 0.5f + step;
        float       d                  = step;

        for (int32 i = 0; i < numCars; i++) {
            bool    onSide = false; // Whether it's on its side - Reused below to mean "was blown up"
            CMatrix mat;

            if (!acrossRoad[i]) {
                // Parked along the road
                d += alongGap * 0.5f;
                if (isGangRoadBlock) {
                    onSide = (rand() & 0xFF) < 0x40; // 25% chance
                }

                mat.GetForward() = dir;
                if (onSide) {
                    mat.GetRight() = CVector{ 0.0f, 0.0f, 1.0f };
                    mat.GetUp()    = CVector{ -dir.y, dir.x, 0.0f };
                } else {
                    mat.GetRight() = CVector{ dir.y, -dir.x, 0.0f };
                    mat.GetUp()    = CVector{ 0.0f, 0.0f, 1.0f };
                }

                if (rand() & 1) { // 50% chance of facing the other way
                    if (onSide) {
                        mat.RotateY(PI);
                    } else {
                        mat.RotateZ(PI);
                    }
                }

                mat.RotateZ(((float)(rand() & 0xFF) - 128.0f) * (isGangRoadBlock ? 0.006f : 0.003f));
            } else {
                // Parked across the road
                d += acrossGap * 0.5f;

                mat.GetForward() = perp;
                mat.GetRight()   = CVector{ perp.y, -perp.x, 0.0f };
                mat.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };

                mat.RotateZ(((float)(rand() & 0xFF) - 128.0f) * (isGangRoadBlock ? 0.004f : 0.002f));
            }

            mat.SetTranslateOnly({
                a.x + dir.x * d,
                a.y + dir.y * d,
                a.z + dir.z * d
            });
            mat.GetPosition().z = mat.GetPosition().z - (onSide ? carBBoxMin.x : carBBoxMin.z) + 0.3f;

            d += advanceBetweenCars;

            // The area occupied by the car, used to check whether it fits there
            const CVector cornerA = mat.InverseTransformVector(carBBoxMin) + mat.GetPosition();
            const CVector cornerB = mat.InverseTransformVector(carBBoxMax) + mat.GetPosition();
            if (!ClearSpaceForRoadBlockObject(cornerA, cornerB)) {
                continue;
            }

            if (!isGangRoadBlock || (!onSide && (rand() & 0xFF) >= 0x40)) {
                // Spawn the car
                auto* veh = new CAutomobile(modelId, RANDOM_VEHICLE, true);
#ifdef FIX_BUGS
                if (!veh) {
                    continue;
                }
#endif
                veh->SetStatus(STATUS_ABANDONED);

                mat.GetPosition().z = (veh->GetHeightAboveRoad() - 0.6f) + mat.GetPosition().z;

#ifdef FIX_BUGS
                // The original dereferences `m_matrix` unconditionally here (null-deref hazard)
                if (!veh->m_matrix) {
                    delete veh;
                    continue;
                }
#endif
                veh->GetMatrix() = mat;

                veh->PlaceOnRoadProperly();
                veh->SetIsStatic(false);
                veh->UpdateRwMatrix();

                veh->m_nDoorLock = CARLOCK_UNLOCKED;
                CCarCtrl::JoinCarWithRoadSystem(veh);

                onSide = false;

                // Reset the AI state, otherwise it'd drive off
                veh->m_autoPilot.m_nCarMission  = MISSION_NONE;
                veh->m_autoPilot.m_nTempAction  = TEMPACT_NONE;
                veh->m_autoPilot.m_nCurrentLane = 0;
                veh->m_autoPilot.m_nNextLane    = 0;
                veh->m_autoPilot.m_speed        = 0.0f;
                veh->m_autoPilot.m_nCruiseSpeed = 0;

                // Parked, and out of the way of the traffic
                veh->vehicleFlags.bNeverUseSmallerRemovalRange = true;
                veh->vehicleFlags.bIsLocked                    = false;
                veh->vehicleFlags.bEngineOn                    = false;

                if (!isGangRoadBlock) {
                    if (veh->UsesSiren() && (rand() & 1)) {
                        veh->vehicleFlags.bSirenOrAlarm = true;
                    }
                } else {
                    // 1/8 chance of only damaging it a bit, otherwise it gets blown up
                    if ((int32)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * 4.0f) == 0) {
                        veh->SetTotalDamage(true);
                    } else {
                        veh->BlowUpCarCutSceneNoExtras(true, true, true, true);
                        veh->m_nTimeWhenBlowedUp += 1'000'000;
                        onSide = true;
                    }
                }

                if (veh->GetMatrix().GetUp().z <= 0.94f) {
                    // Not standing upright, discard it
                    delete veh;
                } else {
                    CVisibilityPlugins::SetClumpAlpha(veh->GetRpClump(), 0); // Visible only as a collision
                    CWorld::Add(veh);
                    veh->vehicleFlags.bCreateRoadBlockPeds = true;
                    veh->m_nTimeTillWeNeedThisCar = CTimer::GetTimeInMS() + 7'000;
                    veh->m_nNumPedsForRoadBlock = numCars > 3 ? 1 : 2;

                    if (!acrossRoad[i]) {
                        // Peds are placed on the side of the car the player is on
                        const CVector delta = veh->GetPosition() - FindPlayerCoors();
                        veh->m_nPedsPositionForRoadBlock = (delta.Dot(veh->GetMatrix().GetRight()) < 0.0f) ? 0 : 1;
                    } else {
                        veh->m_nPedsPositionForRoadBlock = 2;

                        const auto node = CDamageManager::GetCarNodeIndexFromDoor(DOOR_LEFT_FRONT);
                        if (veh->m_aCarNodes[node]) {
                            veh->OpenDoor(nullptr, node, DOOR_LEFT_FRONT, 1.0f, true);
                        }

                        if (veh->m_nNumPedsForRoadBlock > 1) {
                            const auto node = CDamageManager::GetCarNodeIndexFromDoor(DOOR_RIGHT_FRONT);
                            if (veh->m_aCarNodes[node]) {
                                veh->OpenDoor(nullptr, node, DOOR_RIGHT_FRONT, 1.0f, true);
                            }
                        }
                    }

                    if (onSide) { // Was blown up
                        gFireManager.StartFire(veh, nullptr, 1.4f, 1, 60'000, 2);
                        if (veh->m_pFire) {
                            veh->m_pFire->SetRemovalDist(0x5C);
                        }
                    }
                }
            } else {
                // Spawn a wreck instead
                auto* obj = new CObject(ModelIndices::MI_ROADBLOCKFUCKEDCAR1, true);
#ifdef FIX_BUGS
                if (!obj) {
                    continue;
                }
                // The original dereferences `m_matrix` unconditionally here (null-deref hazard)
                if (!obj->m_matrix) {
                    delete obj;
                    continue;
                }
#endif
                obj->GetMatrix() = mat;
                obj->SetPosn(mat.GetPosition());
                obj->SetIsStatic(false);
                CObject::nNoTempObjects++;
                obj->m_nObjectType  = OBJECT_TEMPORARY;
                obj->m_nRemovalTime = CTimer::GetTimeInMS() + 600'000;
                CWorld::Add(obj);
                gFireManager.StartFire(obj, nullptr, 1.4f, 1, 60'000, 2);
                if (obj->m_pFire) {
                    obj->m_pFire->SetRemovalDist(0x5C);
                }
            }
        }
    }

    // Barriers are only used at cop road blocks
    if (!isGangRoadBlock) {
        const auto  barrierModelId = ModelIndices::MI_ROADWORKBARRIER1;
        const auto& barrierBBox    = CModelInfo::GetModelInfo(barrierModelId)->GetColModel()->m_boundBox;
        const auto  barrierMin     = barrierBBox.m_vecMin;
        const auto  barrierMax     = barrierBBox.m_vecMax;
        const float barrierWidth   = (barrierMax.x - barrierMin.x) + 0.5f;

        int32 numBarriers = (int32)(dist / barrierWidth);
        if (numBarriers > 8) {
            numBarriers = 8;
        }

        const float barrierStep = (dist - (float)numBarriers * barrierWidth) / (float)(numBarriers + 1);

        CObject::DeleteAllTempObjectsInArea(mid, dist * 0.5f);

        for (int32 i = 0; i < numBarriers; i++) {
            const float d = ((float)i + 0.5f) * barrierWidth + (float)(i + 1) * barrierStep;

            CMatrix mat;
            mat.SetUnity();
            mat.SetTranslate({ 0.0f, 0.0f, 0.0f });
            mat.GetRight()   = dir;
            mat.GetForward() = CVector{ dir.y, -dir.x, 0.0f };
            mat.GetUp()      = CVector{ 0.0f, 0.0f, 1.0f };
            mat.RotateZ(((float)(rand() & 0xFF) - 128.0f) * 0.003f);

            // Barriers are placed a bit to the side of the cars
            mat.SetTranslateOnly({
                a.x + dir.x * d + perp.x * 5.0f,
                a.y + dir.y * d + perp.y * 5.0f,
                a.z + dir.z * d + perp.z * 5.0f
            });

            // Affects the ground query and the final position, but not the matrix assigned below
            mat.GetPosition().x += (float)(rand() & 0xF) * 0.05f;
            mat.GetPosition().y += (float)(rand() & 0xF) * 0.05f;

            bool        groundFound{};
            const float groundZ = CWorld::FindGroundZFor3DCoord(mat.GetPosition() + CVector{ 0.0f, 0.0f, 2.0f }, &groundFound);
            mat.GetPosition().z = groundZ;
            if (!groundFound) {
                continue;
            }
            mat.GetPosition().z -= barrierMin.z;

            const CVector cornerA = mat.InverseTransformVector(barrierMin) + mat.GetPosition();
            const CVector cornerB = mat.InverseTransformVector(barrierMax) + mat.GetPosition();
            if (!ClearSpaceForRoadBlockObject(cornerA, cornerB)) {
                continue;
            }

            auto* obj = new CObject(barrierModelId, true);
#ifdef FIX_BUGS
            if (!obj) {
                continue;
            }
            // The original dereferences `m_matrix` unconditionally here (null-deref hazard)
            if (!obj->m_matrix) {
                delete obj;
                continue;
            }
#endif
            obj->GetMatrix() = mat;
            obj->SetPosn(mat.GetPosition());
            CObject::nNoTempObjects++;
            obj->m_nObjectType  = OBJECT_TEMPORARY;
            obj->m_nRemovalTime = CTimer::GetTimeInMS() + 600'000;
            CWorld::Add(obj);
        }
    }
}

// 0x461170
void CRoadBlocks::GenerateRoadBlockPedsForCar(CVehicle* vehicle, int32 pedsPositionsType, ePedType pedType) {
    const auto Generate = [&](eModelID pedModel = MODEL_INVALID, eCopType copType = COP_TYPE_CITYCOP, bool isSpecialCop = false) {
        static constexpr auto PLACEMENTS = std::to_array<CVector>({
            { -1.5f, +1.9f, 0.0f },
            { -1.5f, -2.6f, 0.0f },
            { +1.5f, +1.9f, 0.0f },
            { +1.5f, -2.6f, 0.0f },
            { -1.5f,  0.0f, 0.0f },
            { +1.5f,  0.0f, 0.0f },
        });

        static constexpr auto SPECIAL_COP_PLACEMENTS = std::to_array<CVector>({
            {  0.0f, +3.2f, 0.0f },
            { +1.5f, -1.8f, 0.0f },
            {  0.0f, +3.2f, 0.0f },
            { -1.5f, -1.8f, 0.0f },
            { -1.5f,  0.0f, 0.0f },
            { +1.5f,  0.0f, 0.0f },
        });

        const auto placementIdx = 2 * pedsPositionsType;
        const auto radiusRatio  = vehicle->GetColModel()->GetBoundingSphere().m_fRadius
            / CModelInfo::GetModelInfo(CStreaming::GetDefaultCopCarModel(false))->GetColModel()->GetBoundingSphere().m_fRadius;

        for (auto i = 0u; i < vehicle->m_nNumPedsForRoadBlock; i++) {
            const auto offset = (isSpecialCop ? SPECIAL_COP_PLACEMENTS : PLACEMENTS)[placementIdx + i] * radiusRatio;
            const auto pos = vehicle->GetMatrix().TransformPoint(offset);

            auto* ped = [&]() -> CPed* {
                if (pedType != PED_TYPE_COP) { // 0x461560
                    return new CCivilianPed(pedType, pedModel);
                } else {
                    auto* p = new CCopPed(CStreaming::IsModelLoaded(pedModel) ? copType : COP_TYPE_CITYCOP);
                    if (copType == COP_TYPE_CITYCOP) {
                        p->SetCurrentWeapon(WEAPON_PISTOL);
                    }
                    return p;
                }
            }();

            ped->SetPosn(std::get<CVector>(CPedPlacement::FindZCoorForPed(pos)));
            ped->GetMatrix().SetRotateKeepPos({ 0.0f, 0.0f, -HALF_PI });

            if (pedType == PED_TYPE_COP) {
                auto* t = new CTaskComplexWanderCop(PEDMOVE_STILL, CGeneral::GetRandomNumberInRange(8ui8));
                t->m_nSubTaskCreatedTimer = {};
                t->m_nScanForStuffTimer   = {};
                ped->GetTaskManager().SetTask(t, TASK_PRIMARY_PRIMARY);
            }
            ped->GetTaskManager().SetTask(new CTaskSimpleStandStill(0, true), TASK_PRIMARY_DEFAULT);

            ped->bStayInSamePlace         = true;
            ped->bNotAllowedToDuck        = true;
            ped->m_nTimeTillWeNeedThisPed = CTimer::GetTimeInMS() + 10'000;
            ped->bCrouchWhenShooting      = !isSpecialCop || pedsPositionsType != 2;
            ped->bCullExtraFarAway        = true;
            CEntity::RegisterReference(ped->m_pVehicle = vehicle);
            CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);

            if (pedType != PED_TYPE_COP) {
                const auto weapon = CGangs::Gang[pedType - PED_TYPE_GANG1].GetRandomWeapon(false);
                if (weapon != WEAPON_UNARMED) {
                    ped->GiveDelayedWeapon(weapon, 25'001);
                    ped->SetCurrentWeapon(weapon);
                }
            }
            CWorld::Add(ped);
            ped->GetEventGroup().Add<CEventScriptCommand>({ TASK_PRIMARY_PRIMARY, new CTaskComplexKillPedOnFoot(FindPlayerPed()) });
        }
    };

    if (pedType == PED_TYPE_COP) {
        switch (vehicle->GetModelId()) {
        case MODEL_ENFORCER: Generate(MODEL_SWAT,    COP_TYPE_SWAT1,   true); break;
        case MODEL_BARRACKS: Generate(MODEL_ARMY,    COP_TYPE_ARMY,    true); break;
        case MODEL_FBIRANCH: Generate(MODEL_FBI,     COP_TYPE_FBI,     true); break;
        case MODEL_COPCARRU: Generate(MODEL_INVALID, COP_TYPE_CITYCOP, true); break;
        default:             Generate(MODEL_INVALID, COP_TYPE_CITYCOP, false); break;
        }
    } else if (IsPedTypeGang(pedType)) {
        for (auto i = 0; i < TOTAL_GANGS; i++) {
            if (!CPopCycle::m_pCurrZoneInfo->GangStrength[i]) {
                continue;
            }
            const auto pedModel = CGangs::ChooseGangPedModel((eGangID)i);
            if (pedModel == MODEL_INVALID) {
                continue;
            }
            Generate(pedModel);
            return;
        }
    } else {
        Generate();
    }
}

// 0x4629E0
void CRoadBlocks::GenerateRoadBlocks() {
    ZoneScoped;

    if (FindPlayerWanted()->m_ChanceOnRoadBlock && FindPlayerVehicle()) {
        if (!GenerateDynamicRoadBlocks) {
            rng::fill(InOrOut, true);
            GenerateDynamicRoadBlocks = true;
        }

        const auto counter1      = MAX_ROADBLOCKS * (CTimer::GetFrameCounter() % 16 + 1);
        const auto rbsToGenerate = std::min<size_t>((size_t)NumRoadBlocks, ((counter1 % 16) + counter1) / 16);
        auto       counter2      = MAX_ROADBLOCKS * (CTimer::GetFrameCounter() % 16) / 16;

        for (; counter2 < rbsToGenerate; counter2++) {
            const auto& mrbNode = RoadBlockNodes[counter2];
            if (!ThePaths.IsAreaLoaded(mrbNode)) {
                continue;
            }
            const auto& mainNode = ThePaths.GetPathNode(mrbNode);
            const auto  playerPos = FindPlayerCoors();
            if (std::abs(playerPos.x - mainNode->GetPosition().x) >= 90.0f ||
                std::abs(playerPos.y - mainNode->GetPosition().y) >= 90.0f ||
                DistanceBetweenPoints2D(playerPos, mainNode->GetPosition()) >= 90.0f)
            {
                InOrOut[counter2] = false;
                continue;
            }

            if (InOrOut[counter2]) {
                continue;
            }
            InOrOut[counter2] = true;

            if (CGeneral::GetRandomNumberInRange(128u) >= FindPlayerWanted()->m_ChanceOnRoadBlock) {
                continue;
            }

            float mrbWidth{};
            CVector mrbDir{};
            if (!GetRoadBlockNodeInfo(mrbNode, mrbWidth, mrbDir)) {
                continue;
            }

            if (mainNode->m_nPathWidth) {
                const auto width = mainNode->m_nPathWidth / 16.0f;
                CreateRoadBlockBetween2Points(
                    mainNode->GetPosition() + mrbDir * (mrbWidth / 2.f + width),
                    mainNode->GetPosition() + mrbDir * width,
                    false
                );
                CreateRoadBlockBetween2Points(
                    mainNode->GetPosition() - mrbDir * width,
                    mainNode->GetPosition() - mrbDir * (mrbWidth / 2.f + width),
                    false
                );
                continue;
            }

            for (auto&& [i, nodeAddr] : rngv::enumerate(RoadBlockNodes)) {
                if (counter2 == i || InOrOut[i] || !ThePaths.IsAreaLoaded(nodeAddr.m_wAreaId)) {
                    continue;
                }
                const auto& node = ThePaths.GetPathNode(nodeAddr);

                if (std::abs(mainNode->GetPosition().x - node->GetPosition().x) >= 30.0f ||
                    std::abs(mainNode->GetPosition().y - node->GetPosition().y) >= 30.0f)
                {
                    continue;
                }

                float   width{};
                CVector dir{}; 
                if (!GetRoadBlockNodeInfo(nodeAddr, width, dir)) {
                    continue;
                }

                if (mrbWidth != width || dir.Dot(mrbDir) <= 0.7f) {
                    continue;
                }

                [[maybe_unused]] CColPoint col{};
                [[maybe_unused]] CEntity*  colEntity{};
                if (CWorld::ProcessLineOfSight(
                    mainNode->GetPosition() + CVector{0.0f, 0.0f, 1.0f},
                    node->GetPosition() + CVector{0.0f, 0.0f, 1.0f},
                    col,
                    colEntity,
                    true,
                    false,
                    false,
                    false,
                    false,
                    false,
                    false,
                    false
                )) {
                    continue;
                }

                const auto dirFromMain = (node->GetPosition() - mainNode->GetPosition()).Normalized();
                CreateRoadBlockBetween2Points(
                    node->GetPosition()     + dirFromMain * (mrbWidth / 2.0f),
                    mainNode->GetPosition() - dirFromMain * (mrbWidth / 2.0f),
                    false
                );
                InOrOut[i] = true;

                if (i == NumRoadBlocks) {
                    CreateRoadBlockBetween2Points(
                        mainNode->GetPosition() - dirFromMain * (mrbWidth / 2.0f),
                        mainNode->GetPosition() + dirFromMain * (mrbWidth / 2.0f),
                        false
                    );
                    break;
                }
            }
        }
    } else {
        GenerateDynamicRoadBlocks = false;
    }

    if (auto& srb = aScriptRoadBlocks[CTimer::GetFrameCounter() % MAX_SCRIPT_ROADBLOCKS]; srb.IsActive) {
        const auto c = CVector::Centroid({ srb.CornerA, srb.CornerB });

        if (DistanceBetweenPoints(FindPlayerCoors(), c) >= 90.0f) {
            srb.IsSafeToCreate = true;
        } else if (srb.IsSafeToCreate) {
            CreateRoadBlockBetween2Points(srb.CornerA, srb.CornerB, srb.IsGangRoadBlock);
            srb.IsSafeToCreate = false;
        }
    }
}

// 0x460EE0
bool CRoadBlocks::GetRoadBlockNodeInfo(CNodeAddress nodeAddress, float& outWidth, CVector& outDir) {
    auto* const node = ThePaths.GetPathNode(nodeAddress);
    assert(node);

    assert(node->m_nNumLinks >= 2);
    const auto naviLinkAddrA = ThePaths.GetNaviLink(nodeAddress.m_wAreaId, node->m_wBaseLinkId + 0),
               naviLinkAddrB = ThePaths.GetNaviLink(nodeAddress.m_wAreaId, node->m_wBaseLinkId + 1);
    if (!ThePaths.IsAreaLoaded(naviLinkAddrA.m_wAreaId) || !ThePaths.IsAreaLoaded(naviLinkAddrB.m_wAreaId)) {
        return false;
    }

    const auto &naviLinkA = ThePaths.GetCarPathLink(naviLinkAddrA),
               &naviLinkB = ThePaths.GetCarPathLink(naviLinkAddrB);

    const auto maxNumLanes = std::max(
        naviLinkA.m_numOppositeDirLanes + naviLinkA.m_numSameDirLanes,
        naviLinkB.m_numOppositeDirLanes + naviLinkB.m_numSameDirLanes
    );

    outWidth = ((float)maxNumLanes + 1.f) * 5.f;
    outDir   = CVector{ (naviLinkB.GetNodeCoors() - naviLinkA.GetNodeCoors()).GetPerpRight(), 0.f }.Normalized();

    return true;
}

// 0x460DF0
void CRoadBlocks::RegisterScriptRoadBlock(CVector cornerA, CVector cornerB, bool isGangRoadBlock) {
    auto free = rng::find_if(aScriptRoadBlocks, [](const auto& srb) { return !srb.IsActive; });
    if (free == aScriptRoadBlocks.end()) {
        // No free script roadblock found
        return;
    }

    free->CornerA         = cornerA;
    free->CornerB         = cornerB;
    free->IsActive        = true;
    free->IsSafeToCreate  = true;
    free->IsGangRoadBlock = isGangRoadBlock;
}
