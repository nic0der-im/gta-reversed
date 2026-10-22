/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Population.h"
#include "Glass.h"
#include <PedPlacement.h>
#include <Attractors/PedAttractorPedPlacer.h>

#include <TaskTypes/TaskComplexWanderCop.h>
#include <TaskTypes/TaskSimpleStandStill.h>
#include <TaskTypes/TaskComplexWander.h>
#include <TaskTypes/TaskComplexDie.h>
#include <TaskTypes/TaskComplexKillPedOnFoot.h>
#include <TaskTypes/TaskComplexBeInCouple.h>
#include <TaskTypes/TaskComplexFollowLeaderInFormation.h>
#include <TaskTypes/TaskSimpleHoldEntity.h>
#include <TaskTypes/TaskComplexSunbathe.h>
#include <TaskTypes/TaskComplexWanderCriminal.h>

#include <SurfaceInfos_c.h>
#include <WeaponInfo.h>
#include <Scripts/TheScripts.h>
#include "Entity/Ped/PlayerPed.h"
#include "Wanted.h"

#include <Events/EventSexyPed.h>
#include "Events/EventAcquaintancePedHate.h"

//! Define this to have extra NOTSA_LOG_DEBUG's of CPopulation
#define EXTRA_DEBUG_LOGS

#ifdef EXTRA_DEBUG_LOGS
#define POP_LOG_DEBUG NOTSA_LOG_DEBUG
#else
#define POP_LOG_DEBUG(...)
#endif

//! Model ids of the objects spawned by `CreateBeachProp` (towels are 1-4, props are 6-10)
static auto& ms_nBeachPropModel1  = StaticRef<uint16>(0x8CD638); // TODO(re): original names unknown
static auto& ms_nBeachPropModel2  = StaticRef<uint16>(0x8CD67C);
static auto& ms_nBeachPropModel3  = StaticRef<uint16>(0x8CD680);
static auto& ms_nBeachPropModel4  = StaticRef<uint16>(0x8CD684);
static auto& ms_nBeachPropModel6  = StaticRef<uint16>(0x8CD688);
static auto& ms_nBeachPropModel7  = StaticRef<uint16>(0x8CD68C);
static auto& ms_nBeachPropModel8  = StaticRef<uint16>(0x8CD690);
static auto& ms_nBeachPropModel9  = StaticRef<uint16>(0x8CD694);
static auto& ms_nBeachPropModel10 = StaticRef<uint16>(0x8CD698);

// 0x610DB0
//! A random number of gang members to spawn (`1` to `8`, OG includes `5 * 2 / 5` style truncation)
static int32 GetRandomGangMemberCount() {
    return 1 + (int32)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * 4.0f);
}

// 0x62F2D0
//! Is the given weather type a "sunny" one?
static bool IsSunnyWeatherType(int32 weatherType) {
    return weatherType == 0
        || weatherType == 2
        || weatherType == 13
        || weatherType == 6
        || weatherType == 11
        || weatherType == 17;
}

// 0x632140
bool IsSunbathingWeather() {
    if (CClock::ms_nGameClockHours < 10 || CClock::ms_nGameClockHours > 17) {
        return false;
    }

    const auto weather = (int32)CWeather::NewWeatherType;
    switch (weather) {
    case 1: case 3: case 14: case 5: case 10: case 18:
        return true;
    default:
        return IsSunnyWeatherType(weather);
    }
}

// 0x44DA30
//! Jitter a position laterally within the (smallest) width of the two nodes it lies between
static void JitterPosBetweenNodes(CPathFind& pathFind, CNodeAddress a, CNodeAddress b, uint32 rnd, float* x, float* y) {
    if (a.m_wAreaId == (uint16)-1 || b.m_wAreaId == (uint16)-1) {
        return;
    }

    const CPathNode* nodeA = pathFind.m_pPathNodes[a.m_wAreaId];
    const CPathNode* nodeB = pathFind.m_pPathNodes[b.m_wAreaId];
    if (!nodeA || !nodeB) {
        return;
    }

    nodeA = &nodeA[a.m_wNodeId];
    nodeB = &nodeB[b.m_wNodeId];

    const uint8 width = std::min(nodeA->m_nPathWidth, nodeB->m_nPathWidth);

    *x += (float)(int32)((((int32)rnd & 0xF) - 7) * (uint32)width) * 0.00775f;
    *y += (float)(int32)(((((int32)rnd >> 4) & 0xF) - 7) * (uint32)width) * 0.00775f;
}

// 0x44E790
//! Find a random position on a path, between two connected nodes, around `(x, y)`
static bool FindSpawnPosBetweenNodes(
    float x, float y, float minD, float maxD, float minDClose, float maxDClose,
    CVector* outPos, CNodeAddress* outA, CNodeAddress* outB, float* outT,
    bool allowDeadEnds, RwMatrix* cullMatrix
) {
    auto& pathFind = ThePaths;

    const float maxDSq = (maxD + 30.0f) * (maxD + 30.0f);
    const int32 rnd    = (int32)((float)(rand() & 0xFFFF) * (1.0f / 32768.0f) * 15.0f);
    const auto  region = pathFind.FindRegionForCoors(CVector2D{ x, y });

    for (int32 attempt = 0; attempt < 300; attempt++) {
        if (!pathFind.m_pPathNodes[region] || !pathFind.m_anNumPedNodes[region]) {
            return false;
        }

        const auto* nodeA = &pathFind.m_pPathNodes[region][(rand() % pathFind.m_anNumPedNodes[region]) + pathFind.m_anNumVehicleNodes[region]];

        const CVector posA = nodeA->GetPosition();

        const float distSq = sq(posA.x - x) + sq(posA.y - y);
        if (distSq >= maxDSq) {
            continue;
        }
        if (rnd >= (int32)nodeA->m_nSpawnProbability) {
            continue;
        }

        for (auto l = 0u; l < nodeA->m_nNumLinks; l++) {
            const auto linkIdx = (int32)nodeA->m_wBaseLinkId + (int32)l;

            if (pathFind.m_pPathIntersections[region][linkIdx].m_bRoadCross) {
                continue;
            }

            const CNodeAddress addrB = pathFind.m_pNodeLinks[region][linkIdx];
            if (addrB.m_wAreaId >= 0x40) {
                continue;
            }

            const auto* nodeBBase = pathFind.m_pPathNodes[addrB.m_wAreaId];
            if (!nodeBBase) {
                continue;
            }
            const auto* nodeB = &nodeBBase[addrB.m_wNodeId];

            if (!((!nodeA->m_onDeadEnd && !nodeB->m_onDeadEnd) || allowDeadEnds)) {
                continue;
            }
            if (rnd >= (int32)nodeB->m_nSpawnProbability) {
                continue;
            }

            const CVector posB = nodeB->GetPosition();

            if (std::sqrt(distSq) >= maxD
                && std::sqrt((posA.x - x) * (posB.x - x) + (posA.y - y) * (posB.y - y)) >= maxD
            ) {
                continue;
            }

            for (int32 i = 0; i < 5; i++) {
                const float t = (float)(rand() & 0xFF) * (1.0f / 256.0f);
                *outT = t;

                CVector pos = lerp(posA, posB, t);
                const float dist = std::sqrt(sq(pos.x - x) + sq(pos.y - y));

                const bool visible = cullMatrix
                    ? TheCamera.IsSphereVisible(pos, 2.0f, cullMatrix)
                    : TheCamera.IsSphereVisible(pos, 2.0f);

                if (visible) {
                    if (!(minD < dist && dist < maxD)) {
                        continue;
                    }
                } else {
                    if (!(minDClose < dist && dist < maxDClose) || !(rand() & 1)) {
                        continue;
                    }
                }

                *outA   = nodeA->GetAddress();
                *outB   = nodeB->GetAddress();
                *outPos = pos;

                bool groundHit{};
                const float groundZ = CWorld::FindGroundZFor3DCoord(pos + CVector{ 0.0f, 0.0f, 2.0f }, &groundHit, nullptr);
                if (groundHit) {
                    if (std::abs(groundZ - pos.z) <= 3.0f) {
                        outPos->z = groundZ;
                        return true;
                    }
                    return false;
                }
            }
        }
    }

    return false;
}

// 0x6EABA0
//! Spawn a beach prop (towel, umbrella, ...) of the given type at `pos`
static CObject* CreateBeachProp(CVector* pos, int32 type) {
    if (CObject::nNoTempObjects > 149) {
        return nullptr;
    }

    bool useAlphaFlag{};
    switch (type) {
    case 5: // Any towel
        switch (rand() & 7) {
        case 1: case 7: type = 2; break;
        case 3: case 5: type = 4; break;
        default:        type = 3; break;
        }
        break;
    case 11: // Any prop
        switch (rand() & 7) {
        case 1: case 7: type = 8;  break;
        case 2: case 6: type = 9;  break;
        case 3: case 5: type = 10; break;
        default:        type = 7;  break;
        }
        if (CObject::nNoTempObjects > 144) {
            return nullptr;
        }
        break;
    }

    uint16 modelId{};
    switch (type) {
    case 1:  modelId = ms_nBeachPropModel1;  useAlphaFlag = false; break;
    case 2:  modelId = ms_nBeachPropModel2;  useAlphaFlag = false; break;
    case 3:  modelId = ms_nBeachPropModel3;  useAlphaFlag = false; break;
    case 4:  modelId = ms_nBeachPropModel4;  useAlphaFlag = false; break;
    case 6:  modelId = ms_nBeachPropModel6;  useAlphaFlag = true;  break;
    case 7:  modelId = ms_nBeachPropModel7;  useAlphaFlag = true;  break;
    case 8:  modelId = ms_nBeachPropModel8;  useAlphaFlag = true;  break;
    case 9:  modelId = ms_nBeachPropModel9;  useAlphaFlag = true;  break;
    case 10: modelId = ms_nBeachPropModel10; useAlphaFlag = true;  break;
    default: modelId = ms_nBeachPropModel1;  useAlphaFlag = false; break;
    }

    CObject* const obj = new CObject{ (int32)modelId, true };
    if (!obj) {
        return nullptr;
    }

    obj->SetPosn(*pos);
    if (obj->GetRwObject()) {
        obj->UpdateRwMatrix();
    }

    obj->m_nObjectType  = OBJECT_TEMPORARY;
    obj->m_nRemovalTime = CTimer::GetTimeInMS() + 43'200'000; // 12 hours
    // TODO(re): OG also calls the virtual at vtable+0x10 with `useAlphaFlag` here; that slot couldn't
    // be identified in this build's class hierarchy, so the flag isn't applied.
    (void)useAlphaFlag;

    CObject::nNoTempObjects++;

    CWorld::Add(obj);
    return obj;
}


void CPopulation::InjectHooks() {
    RH_ScopedClass(CPopulation);
    RH_ScopedCategoryGlobal();

    RH_ScopedGlobalInstall(FindPedRaceFromName, 0x5B6D40);

    RH_ScopedGlobalInstall(LoadPedGroups, 0x5BCFE0);
    RH_ScopedGlobalInstall(LoadCarGroups, 0x5BD1A0);

    RH_ScopedGlobalInstall(DoesCarGroupHaveModelId, 0x406F50);
    RH_ScopedGlobalInstall(ManagePed, 0x611FC0);
    RH_ScopedGlobalInstall(FindNumberOfPedsWeCanPlaceOnBenches, 0x612240);
    RH_ScopedGlobalInstall(RemoveAllRandomPeds, 0x6122C0);
    RH_ScopedGlobalInstall(TestRoomForDummyObject, 0x612320);
    RH_ScopedGlobalInstall(TestSafeForRealObject, 0x6123A0);
    RH_ScopedGlobalInstall(AddPed, 0x612710);
    RH_ScopedGlobalInstall(AddDeadPedInFrontOfCar, 0x612CD0);
    RH_ScopedGlobalInstall(ChooseCivilianOccupation, 0x612F90);
    RH_ScopedGlobalInstall(ChooseCivilianCoupleOccupations, 0x613180);
    RH_ScopedGlobalInstall(ChooseCivilianOccupationForVehicle, 0x613260);
    RH_ScopedGlobalInstall(CreateWaitingCoppers, 0x6133F0);
    RH_ScopedGlobalInstall(AddPedInCar, 0x613A00);
    RH_ScopedGlobalInstall(PlaceMallPedsAsStationaryGroup, 0x613CD0);
    RH_ScopedGlobalInstall(PlaceCouple, 0x613D60);
    RH_ScopedGlobalInstall(AddPedAtAttractor, 0x614210);
    RH_ScopedGlobalInstall(FindDistanceToNearestPedOfType, 0x6143E0);
    RH_ScopedGlobalInstall(PickGangCar, 0x614490);
    RH_ScopedGlobalInstall(PickRiotRoadBlockCar, 0x6144B0);
    RH_ScopedGlobalInstall(ConvertToRealObject, 0x614580);
    RH_ScopedGlobalInstall(ConvertToDummyObject, 0x614670);
    RH_ScopedGlobalInstall(AddToPopulation, 0x614720);
    RH_ScopedGlobalInstall(GeneratePedsAtAttractors, 0x615970);
    RH_ScopedGlobalInstall(GeneratePedsAtStartOfGame, 0x615C90);
    RH_ScopedGlobalInstall(ManageObject, 0x615DC0);
    RH_ScopedGlobalInstall(ManageDummy, 0x616000);
    RH_ScopedGlobalInstall(ManageAllPopulation, 0x6160A0);
    RH_ScopedGlobalInstall(ManagePopulation, 0x616190);
    RH_ScopedGlobalInstall(RemovePedsIfThePoolGetsFull, 0x616300);
    RH_ScopedGlobalInstall(ConvertAllObjectsToDummyObjects, 0x616420);
    RH_ScopedGlobalInstall(FindPedMultiplierMotorway, 0x611B80);
    RH_ScopedGlobalInstall(FindCarMultiplierMotorway, 0x611B60);
    RH_ScopedGlobalInstall(IsCorrectTimeOfDayForEffect, 0x611B20);
    RH_ScopedGlobalInstall(RemoveSpecificDriverModelsForCar, 0x6119D0);
    RH_ScopedGlobalInstall(Initialise, 0x610E10);
    RH_ScopedGlobalInstall(Shutdown, 0x610EC0);
    RH_ScopedGlobalInstall(FindDummyDistForModel, 0x610ED0);
    RH_ScopedGlobalInstall(FindPedDensityMultiplierCullZone, 0x610F00);
    RH_ScopedGlobalInstall(RemovePed, 0x610F20);
    RH_ScopedGlobalInstall(ChoosePolicePedOccupation, 0x610F40);
    RH_ScopedGlobalInstall(ArePedStatsCompatible, 0x610F50);
    RH_ScopedGlobalInstall(PedMICanBeCreatedAtAttractor, 0x6110C0);
    RH_ScopedGlobalInstall(PedMICanBeCreatedAtThisAttractor, 0x6110E0);
    RH_ScopedGlobalInstall(PedMICanBeCreatedInInterior, 0x611450);
    RH_ScopedGlobalInstall(IsMale, 0x611470);
    RH_ScopedGlobalInstall(PopulateInterior, 0x616470);
    RH_ScopedGlobalInstall(IsFemale, 0x611490);
    RH_ScopedGlobalInstall(IsSkateable, 0x6114C0);
    RH_ScopedGlobalInstall(ChooseGangOccupation, 0x611550);
    RH_ScopedGlobalInstall(AddExistingPedInCar, 0x611560);
    RH_ScopedGlobalInstall(UpdatePedCount, 0x611570);
    RH_ScopedGlobalInstall(MoveCarsAndPedsOutOfAbandonedZones, 0x6116A0);
    RH_ScopedGlobalInstall(DealWithZoneChange, 0x6116B0);
    RH_ScopedGlobalInstall(PedCreationDistMultiplier, 0x6116C0);
    RH_ScopedGlobalInstall(CanSolicitPlayerOnFoot, 0x611780);
    RH_ScopedGlobalInstall(CanSolicitPlayerInCar, 0x611790);
    RH_ScopedGlobalInstall(CanJeerAtStripper, 0x6117B0);
    RH_ScopedGlobalInstall(PlaceGangMembers, 0x6117D0);
    RH_ScopedGlobalInstall(LoadSpecificDriverModelsForCar, 0x6117F0);
    RH_ScopedGlobalInstall(FindSpecificDriverModelForCar_ToUse, 0x611900);
    RH_ScopedGlobalInstall(IsSecurityGuard, 0x6114B0);
    RH_ScopedGlobalInstall(Update, 0x616650);

    RH_ScopedGlobalOverloadedInstall(IsSunbather, "ModelID", 0x611760, bool(*)(eModelID));
    //RH_ScopedGlobalOverloadedInstall(IsSunbather, "Ped", 0x611760, bool(*)(CPed*));
}

bool CanCameraSeeAPedHere(CVector pos) {
    if (TheCamera.IsSphereVisible({ pos, 2.f })) {
        if (sq(CPopulation::PedCreationDistMultiplier() * 42.5f) >= (FindPlayerPed()->GetPosition() - pos).SquaredMagnitude2D()) {
            return true;
        }
    }
    return false;
}

// 0x5B6D40
ePedRace CPopulation::FindPedRaceFromName(const char* modelName) {
    for (size_t i{}; i < 2; i++) {
        switch (toupper(modelName[i])) {
        case 0:   return RACE_DEFAULT; // Not handled properly originally
        case 'B': return RACE_BLACK;
        case 'H': return RACE_HISPANIC;
        case 'O':
        case 'I': return RACE_ORIENTAL;
        case 'W': return RACE_WHITE;
        }
    }
    return RACE_DEFAULT;
}

//! NOTSA - Unified function to read pedgrp.dat/cargrp.dat
void LoadGroup(const char* fileName, auto& outModelsInGroup, auto& outNumOfModelsPerGroup) {
    CFileMgr::ChangeDir("\\DATA\\");
    const auto file = CFileMgr::OpenFile(fileName, "r");
    CFileMgr::ChangeDir("\\");

    POP_LOG_DEBUG("Loading `{}`...", fileName);

    size_t currGrpIdx{}, lineno{1};
    for (;const auto l = CFileLoader::LoadLine(file); lineno++) { // Also replaces `,` with ` ` (space) (Important to know)
        uint16 npeds{};
        for (auto begin = l, end = l; *begin; begin = end) {
            begin = CFileLoader::FindFirstNonNullOrWS(begin);
            if (*begin == '#') {
                break;
            }
            end = CFileLoader::FindFirstNullOrWS(begin);
            if (begin == end) {
                break;
            }

#ifdef _DEBUG // See bottom of the outer loop for info
            if (currGrpIdx >= outModelsInGroup.size()) {
                POP_LOG_DEBUG("Data found past-the-end! This would crash the vanilla game! [Line: {}]", lineno);
                break;
            }
#endif

            // Originally this check was at the end of the loop
            // but we want to print a useful error message, so the
            // loop is let to do one more iteration before breaking
            // to see if there are any more models to be added
            if (npeds >= outModelsInGroup[currGrpIdx].size()) {
                POP_LOG_DEBUG("There are models to be added to the group, but there's no memory! [Group ID: {}; Line: {}]", currGrpIdx, lineno);
                break;
            }
            
            char modelName[256]{};
            strncpy_s(modelName, begin, end - begin);
            if (int32 pedModelIdx{ MODEL_INVALID }; CModelInfo::GetModelInfo(modelName, &pedModelIdx)) {
                assert(pedModelIdx != MODEL_PLAYER);
                outModelsInGroup[currGrpIdx][npeds++] = pedModelIdx;
            } else {
                NOTSA_LOG_DEBUG("Model ({}) doesn't exist! [Group ID: {}; Line: {}]", modelName, currGrpIdx, lineno);
            }
        }
        
        if (npeds == 0) {
            continue; // Blank line
        }

        //POP_LOG_DEBUG("Loaded ({}) models into the group ({})", outNumOfModelsPerGroup[currGrpIdx], currGrpIdx);

        // Only now set this
        outNumOfModelsPerGroup[currGrpIdx] = npeds;

        // Fill the rest (unused slots) with a value
        rng::fill(outModelsInGroup[currGrpIdx] | rng::views::drop(npeds), CPopulation::m_DefaultModelIDForUnusedSlot);
        assert(!notsa::contains(outModelsInGroup[currGrpIdx], 0));

        // Only increment this if it wasn't a blank line
        currGrpIdx++;
#ifndef _DEBUG // In debug mode we let the loop go to check for past-the-end data (and report it)
        if (currGrpIdx == outModelsInGroup.size()) {
            break; // No more data needed - This if kind of a bugfix too, as the original game would just keep going, and if there was anything after the last group it would just corrupt the memory :D
        }
#endif
    };

    if (currGrpIdx == outModelsInGroup.size()) {
        NOTSA_LOG_DEBUG("{} has been loaded successfully! [#Groups Loaded: {}]", fileName, currGrpIdx);
    } else {
        NOTSA_UNREACHABLE("Missing group data in {}! [#Groups Loaded: {}/{}]", fileName, currGrpIdx, outModelsInGroup.size());
    }

    CFileMgr::CloseFile(file);
}

// 0x5BCFE0
void CPopulation::LoadPedGroups() {
    LoadGroup("PEDGRP.DAT", m_PedGroups, m_nNumPedsInGroup);
}

// 0x5BD1A0
void CPopulation::LoadCarGroups() {
    LoadGroup("CARGRP.DAT", m_CarGroups, m_nNumCarsInGroup);
}

// 0x610E10
void CPopulation::Initialise() {
    ms_nNumCivMale = 0;
    ms_nNumCivFemale = 0;
    ms_nNumCop = 0;
    ms_nNumEmergency = 0;
    rng::fill(ms_nNumGang, 0);
    ms_nTotalCarPassengerPeds = 0;
    ms_nTotalCivPeds = 0;
    ms_nTotalGangPeds = 0;
    ms_nTotalPeds = 0;
    ms_nTotalMissionPeds = 0;
    m_CountDownToPedsAtStart = 5;
    bZoneChangeHasHappened = 0;
    PedDensityMultiplier = 1.0f;
    m_AllRandomPedsThisType = -1;
    MaxNumberOfPedsInUse = 25;
    NumberOfPedsInUseInterior = 40;
    bInPoliceStation = 0;
    m_bDontCreateRandomGangMembers = 0;
    m_bOnlyCreateRandomGangMembers = 0;
    m_bDontCreateRandomCops = 0;
    m_bMoreCarsAndFewerPeds = 0;
    LoadPedGroups();
    LoadCarGroups();
}

// 0x610EC0
void CPopulation::Shutdown() {
    /* nop */
}

// 0x610ED0
float CPopulation::FindDummyDistForModel(eModelID modelIndex) {
    return (modelIndex == ModelIndices::MI_SAMSITE || modelIndex == ModelIndices::MI_SAMSITE2)
        ? 750.f
        : 80.f;
}

// 0x610F00
float CPopulation::FindPedDensityMultiplierCullZone() {
    return CCullZones::FewerPeds()
        ? 0.6f
        : 1.f;
}

// 0x610F20
void CPopulation::RemovePed(CPed* ped) {
    CWorld::Remove(ped);
    delete ped;
}

// 0x610F40
eModelID CPopulation::ChoosePolicePedOccupation() {
    return (eModelID)COP_TYPE_CITYCOP; // See CCopPed::GetPedModelForCopType
}

// 0x610F50
bool CPopulation::ArePedStatsCompatible(ePedStats st1, ePedStats st2) {
    for (auto stype : { st1, st2 }) {
        switch (stype) {
        case ePedStats::PLAYER:
        case ePedStats::COP:
        case ePedStats::MEDIC:
        case ePedStats::FIREMAN:
        case ePedStats::TRAMP_MALE:
        case ePedStats::TRAMP_FEMALE:
        case ePedStats::TOURIST:
        case ePedStats::PROSTITUTE:
        case ePedStats::CRIMINAL:
        case ePedStats::BUSKER:
        case ePedStats::TAXIDRIVER:
        case ePedStats::PSYCHO:
        case ePedStats::STEWARD:
        case ePedStats::SPORTSFAN:
        case ePedStats::SHOPPER:
        case ePedStats::OLDSHOPPER:
        case ePedStats::BEACH_GUY:
        case ePedStats::BEACH_GIRL:
        case ePedStats::SKATER:
        case ePedStats::STD_MISSION:
        case ePedStats::COWARD:
        case ePedStats::GANG1:
        case ePedStats::GANG2:
        case ePedStats::GANG3:
        case ePedStats::GANG4:
        case ePedStats::GANG5:
        case ePedStats::GANG6:
        case ePedStats::GANG7:
            return false;
        }
        return true;
    }
    const auto IsOldGuyOrGirl = [](ePedStats st) {
        switch (st) {
        case ePedStats::OLD_GUY:
        case ePedStats::OLD_GIRL:
            return true;
        }
        return false;
    };
    return !IsOldGuyOrGirl(st1) || IsOldGuyOrGirl(st2);
}

// 0x6110C0
bool CPopulation::PedMICanBeCreatedAtAttractor(eModelID modelIndex) {
    switch (CModelInfo::GetPedModelInfo(modelIndex)->GetPedType()) {
    case PED_TYPE_DEALER:
    case PED_TYPE_MEDIC:
    case PED_TYPE_FIREMAN:
    case PED_TYPE_CRIMINAL:
    case PED_TYPE_BUM:
    case PED_TYPE_PROSTITUTE:
        return false;
    }
    return true;
}

// 0x6110E0
bool CPopulation::PedMICanBeCreatedAtThisAttractor(eModelID modelId, const char* attrName) {
    if (!attrName) {
        return true;
    }

    const auto NameIsAnyOf = [&](auto... anyOfValues) {
        return (... || (_stricmp(attrName, anyOfValues) == 0));
    };

    const auto pedType = CModelInfo::GetPedModelInfo(modelId)->GetPedType();

    if (NameIsAnyOf("COPSIT", "COPLOOK", "BROWSE")) {
        return pedType == PED_TYPE_COP;
    }

    if (pedType == PED_TYPE_COP) {
        return false;
    }

    if (NameIsAnyOf("DANCER")) {
        switch (modelId) {
        case MODEL_BFYRI:
        case MODEL_BMYRI:
        case MODEL_BMYST:
        case MODEL_HFYRI:
        case MODEL_HMYRI:
        case MODEL_OFYST:
        case MODEL_OMOST:
        case MODEL_OMYRI:
        case MODEL_OMYST:
        case MODEL_WFYRI:
        case MODEL_WFYST:
        case MODEL_WMYRI:
        case MODEL_WMYST:
            return true;
        }
        return false;
    }

    if (NameIsAnyOf("BARGUY", "PEDROUL", "PEDCARD", "PEDSLOT")) {
        switch (modelId) {
        case MODEL_WMYCON:
        case MODEL_HMOGAR:
        case MODEL_WMYMECH:
        case MODEL_SBFYST:
        case MODEL_WMYSGRD:
        case MODEL_SWMYHP1:
        case MODEL_SWMYHP2:
        case MODEL_SWMOTR1:
        case MODEL_SBMOTR2SBMOTR2:
        case MODEL_SWMOTR2:
        case MODEL_SBMYTR3:
        case MODEL_SWMOTR3:
        case MODEL_SBMYST:
        case MODEL_WMYCONB:
        case MODEL_SOMYST:
        case MODEL_SBFOST:
        case MODEL_SOFOST:
        case MODEL_SOFYST:
        case MODEL_SOMOST:
        case MODEL_SWMOTR5:
        case MODEL_SWFOST:
        case MODEL_SWFYST:
        case MODEL_SWMOST:
        case MODEL_SWMOTR4:
            return false; // TODO/BUG: Is this correct? Shouldn't it return true?
        }
        return true;
    }

    if (NameIsAnyOf("STRIPW")) {
        switch (modelId) {
        case MODEL_VWFYST1:
        case MODEL_VBFYST2:
        case MODEL_VHFYST3:
        case MODEL_SBFYSTR:
        case MODEL_SWFYSTR:
            return true;
        }
        return false;
    }

    if (NameIsAnyOf("STRIPM")) {
        return pedType == PED_TYPE_CIVFEMALE;
    }

    return false;
}

// 0x611450
bool CPopulation::PedMICanBeCreatedInInterior(eModelID modelIndex) {
    switch (CModelInfo::GetPedModelInfo(modelIndex)->GetPedType()) {
    case PED_TYPE_COP:
    case PED_TYPE_GANG1:
    case PED_TYPE_GANG2:
    case PED_TYPE_GANG3:
    case PED_TYPE_GANG4:
    case PED_TYPE_GANG5:
    case PED_TYPE_GANG6:
    case PED_TYPE_GANG7:
    case PED_TYPE_GANG8:
    case PED_TYPE_GANG9:
    case PED_TYPE_GANG10:
    case PED_TYPE_DEALER:
    case PED_TYPE_MEDIC:
    case PED_TYPE_FIREMAN:
    case PED_TYPE_CRIMINAL:
    case PED_TYPE_BUM:
    case PED_TYPE_PROSTITUTE:
        return false;
    }
    return true;
}

// 0x611470
bool CPopulation::IsMale(eModelID modelIndex) {
    return CModelInfo::GetPedModelInfo(modelIndex)->GetPedType() == PED_TYPE_CIVMALE;
}

// 0x611490
bool CPopulation::IsFemale(eModelID modelIndex) {
    return CModelInfo::GetPedModelInfo(modelIndex)->GetPedType() == PED_TYPE_CIVFEMALE;
}

// 0x6114B0
bool CPopulation::IsSecurityGuard(ePedType pedType) {
    return false;
}

// 0x6114C0
bool CPopulation::IsSkateable(const CVector& point) {
    CColPoint cp{};
    CEntity* hitEntity{};
    CWorld::ProcessVerticalLine(point + CVector{ 0.f, 0.f, 2.f }, point.z - 2.f, cp, hitEntity, true);
    return hitEntity && g_surfaceInfos.IsSkateable(cp.m_nSurfaceTypeB);
}

// 0x611550
eModelID CPopulation::ChooseGangOccupation(eGangID gangId) {
    return CGangs::ChooseGangPedModel(gangId);
}

// 0x611560 (Unused)
CPed* CPopulation::AddExistingPedInCar(CPed* ped, CVehicle* vehicle) {
    NOTSA_UNREACHABLE(); // Does nothing (At least not what the name suggests)
}

// 0x611570
void CPopulation::UpdatePedCount(CPed* ped, bool pedAddedOrRemoved) {
    if (pedAddedOrRemoved != ped->bHasBeenAddedToPopulation) {
        return;
    }

    ped->bHasBeenAddedToPopulation = !pedAddedOrRemoved;

    const auto DoOp = [&](auto& value) {
        if (pedAddedOrRemoved) {
            value--;
        } else {
            value++;
        }
    };

    switch (ped->m_nPedType) {
    case PED_TYPE_CIVMALE:
    case PED_TYPE_CRIMINAL:
        DoOp(ms_nNumCivMale);
        break;
    case PED_TYPE_CIVFEMALE:
    case PED_TYPE_PROSTITUTE:
        DoOp(ms_nNumCivFemale);
        break;
    case PED_TYPE_COP:
        DoOp(ms_nNumCop);
        break;
    case PED_TYPE_GANG1:
    case PED_TYPE_GANG2:
    case PED_TYPE_GANG3:
    case PED_TYPE_GANG4:
    case PED_TYPE_GANG5:
    case PED_TYPE_GANG6:
    case PED_TYPE_GANG7:
    case PED_TYPE_GANG8:
    case PED_TYPE_GANG9:
    case PED_TYPE_GANG10:
#ifdef FIX_BUGS
        DoOp(ms_nNumGang[ped->m_nPedType - PED_TYPE_GANG1]);
#else
        DoOp(ms_nNumGang[ped->m_nPedType]);
#endif
        break;
    case PED_TYPE_DEALER:
        DoOp(ms_nNumDealers);
        break;
    case PED_TYPE_MEDIC:
    case PED_TYPE_FIREMAN:
        DoOp(ms_nNumEmergency);
        break;
    }
}

// 0x6116A0
void CPopulation::MoveCarsAndPedsOutOfAbandonedZones() {
    /* nop */
}

// 0x6116B0
void CPopulation::DealWithZoneChange(eLevelName arg0, eLevelName arg1, bool arg2) {
    /* nop */
}

// 0x6116C0
float CPopulation::PedCreationDistMultiplier() {
    if (const auto veh = FindPlayerVehicle()) {
        return std::clamp(veh->m_vecMoveSpeed.Magnitude2D() - 0.1f + 1.f, 1.f, 1.5f);
    }
    return 1.f;
}

// 0x611760
bool CPopulation::IsSunbather(eModelID modelIndex) {
    switch (CModelInfo::GetPedModelInfo(modelIndex)->GetPedStatType()) {
    case ePedStats::BEACH_GUY:
    case ePedStats::BEACH_GIRL:
        return true;
    }
    return false;
}

bool CPopulation::IsSunbather(CPed* ped) {
    return IsSunbather(ped->GetModelId());
}

// 0x611780
bool CPopulation::CanSolicitPlayerOnFoot(eModelID modelIndex) {
    return false;
}

// 0x611790
bool CPopulation::CanSolicitPlayerInCar(eModelID modelIndex) {
    return CModelInfo::GetPedModelInfo(modelIndex)->GetPedType() == PED_TYPE_PROSTITUTE;
}

// 0x6117B0
bool CPopulation::CanJeerAtStripper(eModelID modelIndex) {
    return CModelInfo::GetPedModelInfo(modelIndex)->GetPedType() == PED_TYPE_CIVMALE;
}

// 0x6117D0
void CPopulation::PlaceGangMembers(ePedType pedType, uint32 numOfPeds, const CVector& posn) {
    CPedGroupPlacer{}.PlaceGroup(pedType, numOfPeds, posn, ePedGroupDefaultTaskAllocatorType::RANDOM);
}

//! NOTSA - Helper
void ProcessPossibleDriverModelsOfCar(eModelID carModelIndex, auto&& ProcessModel) {
    if (carModelIndex == MODEL_FREEWAY) {
        ProcessModel(MODEL_BIKERA);
        ProcessModel(MODEL_BIKERB);
    } else {
        const auto model = [&] {
            switch (carModelIndex) {
            case MODEL_STRETCH:     return MODEL_WMYCH;
            case MODEL_TAXI:
            case MODEL_CABBIE:      return (eModelID)(CStreaming::GetDefaultCabDriverModel());
            case MODEL_MRWHOOP:     return MODEL_WMOICE;
            case MODEL_SECURICA:    return MODEL_WMYSGRD;
            case MODEL_PIZZABOY:    return MODEL_WMYPIZZ;
            case MODEL_FREEWAY:     return MODEL_BIKERA;
            case MODEL_BMX:         return MODEL_WMYBMX;
            default:                return MODEL_INVALID;
        }}();
        if (model != MODEL_INVALID) {
            ProcessModel(model);
        }
    }
}

// 0x6117F0
void CPopulation::LoadSpecificDriverModelsForCar(eModelID carModelIndex) {
    ProcessPossibleDriverModelsOfCar(carModelIndex, [&](eModelID model) {
        CStreaming::RequestModel(model, STREAMING_KEEP_IN_MEMORY | STREAMING_GAME_REQUIRED);
    });
}

// 0x6119D0
void CPopulation::RemoveSpecificDriverModelsForCar(eModelID carModelIndex) {
    ProcessPossibleDriverModelsOfCar(carModelIndex, CStreaming::SetModelAndItsTxdDeletable);
}

// 0x611900
eModelID CPopulation::FindSpecificDriverModelForCar_ToUse(eModelID carModelIndex) {
    switch (carModelIndex) {
    case MODEL_STRETCH:  return MODEL_WMYCH;
    case MODEL_TAXI:
    case MODEL_CABBIE:   return (eModelID)(CStreaming::GetDefaultCabDriverModel());
    case MODEL_MRWHOOP:  return MODEL_WMOICE;
    case MODEL_SECURICA: return MODEL_WMYSGRD;
    case MODEL_PIZZABOY: return MODEL_WMYPIZZ;
    case MODEL_BMX:      return MODEL_WMYBMX;
    case MODEL_FREEWAY: {
        switch (CGeneral::GetRandomNumberInRange(0, 3)) {
        case 0:  return MODEL_BIKERA;
        case 1:  return MODEL_BIKERB;
        default: return MODEL_INVALID;
        }
    }
    }
    return MODEL_INVALID;
}

// 0x611B20
bool CPopulation::IsCorrectTimeOfDayForEffect(const C2dEffectPedAttractor& fx) {
    switch (fx.m_nAttractorType) {
    case PED_ATTRACTOR_PIZZA:
    case PED_ATTRACTOR_SHELTER:
    case PED_ATTRACTOR_TRIGGER_SCRIPT:
    case PED_ATTRACTOR_LOOK_AT:
    case PED_ATTRACTOR_SCRIPTED:
    case PED_ATTRACTOR_PARK:
    case PED_ATTRACTOR_STEP:
        return true;
    }
    return CClock::GetIsTimeInRange(9, 20); // 9 is correct, because the function uses `>=` not `>`
}

// 0x611B60
float CPopulation::FindCarMultiplierMotorway() {
    return m_bMoreCarsAndFewerPeds
        ? 1.7f
        : 1.f;
}

// 0x611B80
float CPopulation::FindPedMultiplierMotorway() {
    return 1.f;
}

// 0x611FC0
void CPopulation::ManagePed(CPed* ped, const CVector& playerPosn) {
    if (ped->IsPlayer()) {
        return;
    }

    if (!ped->CanBeDeleted()) {
        return;
    }

    if (ped->bInVehicle) {
        return;
    }

    if (ped->m_pAttachedTo && ped->m_pAttachedTo->GetIsTypeVehicle()) {
        return;
    }

    // If pead is dead, possibly fade them out
    if (ped->IsStateDead()) {
        const auto delta = CTimer::GetTimeInMS() - ped->m_nDeathTimeMS;
        if (   delta > 30'000
            || CDarkel::FrenzyOnGoing() && delta > 15'000
            || CGangWars::GangWarFightingGoingOn() && delta > 8'000
        ) {
            ped->bFadeOut = true;
        }
    }

    // If we've faded the ped completely out, remove it
    if (ped->bFadeOut && CVisibilityPlugins::GetClumpAlpha(ped->GetRpClump()) == 0) {
        RemovePed(ped);
        return;
    }

    const auto pedsRemovalDist = [&] {
        const auto GetDist = [&] {
            return (ped->GetPosition() - playerPosn).Magnitude2D() * ped->m_fRemovalDistMultiplier;
        };
        if (IsPedTypeGang(ped->m_nPedType)) {
            return GetDist() - 30.f;
        } else if (ped->bDeadPedInFrontOfCar && ped->m_VehDeadInFrontOf) { // Never true, because `field_590` is always 0
            return 0.f;
        } else {
            return GetDist();
        }
    }() / PedCreationDistMultiplier(); // Simplify the conditions below by dividing here

    if (TheCamera.m_fGenerationDistMultiplier * (ped->bCullExtraFarAway ? 65.f : 54.5f) <= pedsRemovalDist) {
        if (ped->GetIsOnScreen()) {
            ped->bFadeOut = true;
        } else {
            RemovePed(ped);
        }
    } else if (pedsRemovalDist <= 25.f || ped->GetIsOnScreen()) { // From here on I did some truth table magic, and was able to remove some of the redudant code (So some of the code is missing)
        ped->m_nTimeTillWeNeedThisPed = CTimer::GetTimeInMS() + (ped->m_nPedType == PED_TYPE_COP ? 10'000 : 4'000);
    } else if (CTimer::GetTimeInMS() > ped->m_nTimeTillWeNeedThisPed) { // Ped not needed anymore
        const auto& activeCam = TheCamera.GetActiveCamera();
        switch (activeCam.m_nMode) {
        case MODE_SNIPER:
        case MODE_SNIPER_RUNABOUT:
        case MODE_CAMERA:
            return;
        }
        if (activeCam.m_bLookingLeft || activeCam.m_bLookingRight || activeCam.m_bLookingBehind) {
            return;
        }
        RemovePed(ped);
    }
}

// 0x612240
int32 CPopulation::FindNumberOfPedsWeCanPlaceOnBenches() {
    const int32 base = CGame::CanSeeOutSideFromCurrArea()
        ? (int32)(std::floor(std::min((float)(MaxNumberOfPedsInUse), CPopCycle::m_NumOther_Peds)) * PedDensityMultiplier * FindPedDensityMultiplierCullZone())
        : (int32)(NumberOfPedsInUseInterior);
    return base - (int32)(ms_nNumCivMale) - (int32)(ms_nNumCivFemale) + 2;
}

// 0x6122C0
void CPopulation::RemoveAllRandomPeds() {
    for (auto& ped : GetPedPool()->GetAllValid()) {
        if (ped.CanBeDeleted()) {
            RemovePed(&ped);
        }
    }
}

// 0x612320
bool CPopulation::TestRoomForDummyObject(CObject* object) {
    int16 ncolliding{};
    CWorld::FindObjectsKindaColliding(
        object->GetBoundCentre(),
        object->GetModelInfo()->GetColModel()->GetBoundRadius(),
        false,
        &ncolliding,
        2,              // TODO: Why 2?
        nullptr,
        false,
        true,
        true,
        false,
        false
    );
    return ncolliding == 0;
}

// 0x6123A0
bool CPopulation::TestSafeForRealObject(CDummyObject* obj) {
    const auto objCM  = obj->GetColModel();
    const auto objMat = obj->GetMatrix();
    return CWorld::IterateSectorsOverlappedByRect(
        CRect{ obj->GetBoundCentre(), objCM->GetBoundRadius()},
        [&](int32 x, int32 y) {
            for (auto* const entity : CWorld::GetRepeatSector(x, y).Vehicles) {
                if (CCollision::ProcessColModels(
                    objMat, *objCM,
                    entity->GetMatrix(), *entity->GetColModel(),
                    CWorld::m_aTempColPts,
                    nullptr,
                    nullptr,
                    false
                ) > 0) {
                    return false;
                }
            }
            return true;
        }
    );
}

// 0x612710
CPed* CPopulation::AddPed(ePedType pedType, eModelID modelIndex, const CVector& createAtPos, bool makeWander) {
    const auto GiveAndSetPedWeapon = [](CPed* ped, eWeaponType wtype, uint32 ammo = 25001) { // 0x612970
        ped->GiveDelayedWeapon(wtype, ammo);
        ped->SetCurrentWeapon(wtype);
    };

    // Create the ped
    const auto ped = [&]() -> CPed* {
        switch (pedType) {
        case PED_TYPE_CIVMALE:
        case PED_TYPE_CIVFEMALE: { // 0x61274E
            const auto ped = new CCivilianPed(pedType, modelIndex);

            if (CCheat::IsAnyActive({ CHEAT_EVERYONE_ARMED, CHEAT_PEDS_ATTACK_OTHER_WITH_GOLFCLUB })) {
                GiveAndSetPedWeapon(ped, CGeneral::RandomChoiceFromList({ WEAPON_PISTOL, WEAPON_BASEBALLBAT, WEAPON_SHOTGUN, WEAPON_M4, WEAPON_RLAUNCHER }));
            }

            return ped;
        }
        case PED_TYPE_GANG1:
        case PED_TYPE_GANG2:
        case PED_TYPE_GANG3:
        case PED_TYPE_GANG4:
        case PED_TYPE_GANG5:
        case PED_TYPE_GANG6:
        case PED_TYPE_GANG7:
        case PED_TYPE_GANG8:
        case PED_TYPE_GANG9:
        case PED_TYPE_GANG10: { // 0x6128C4
            const auto ped = new CCivilianPed(pedType, modelIndex);

            if (CCheat::IsActive(CHEAT_NINJA_THEME) && pedType == PED_TYPE_GANG7) {
                GiveAndSetPedWeapon(ped, WEAPON_KATANA);
            } else if (CGeneral::RandomBool(33)) { // Give random weapon
                if (const auto wtype = CGangs::Gang[GetGangOfPedType(pedType)].GetRandomWeapon(); wtype != WEAPON_UNARMED) {
                    GiveAndSetPedWeapon(ped, wtype);
                }
            }

            return ped;
        }
        case PED_TYPE_DEALER:
        case PED_TYPE_CRIMINAL:
        case PED_TYPE_PROSTITUTE:
        case PED_TYPE_SPECIAL:
        case PED_TYPE_MISSION1:
        case PED_TYPE_MISSION2:
        case PED_TYPE_MISSION3:
        case PED_TYPE_MISSION4:
        case PED_TYPE_MISSION5:
        case PED_TYPE_MISSION6:
        case PED_TYPE_MISSION7:
        case PED_TYPE_MISSION8: // 0x612992
            return new CCivilianPed(pedType, modelIndex);
        case PED_TYPE_MEDIC: // 0x612848
        case PED_TYPE_FIREMAN: // 0x612886
            return new CEmergencyPed(pedType, modelIndex);
        case PED_TYPE_COP: // 0x61280C
            return new CCopPed(modelIndex);
        default:
            NOTSA_UNREACHABLE();
        }
    }();

    // 0x6129D9

    ped->SetPosn(createAtPos);
    ped->SetOrientation(0.f, 0.f, 0.f);

    CWorld::Add(ped);

    if (!makeWander) {
        return ped;
    }

    auto& tmgr = ped->GetTaskManager();

    // Set it's default tasks
    if (pedType == PED_TYPE_COP) {
        tmgr.SetTask(
            CTaskComplexWander::GetWanderTaskByPedType(ped),
            TASK_PRIMARY_PRIMARY
        );
        tmgr.SetTask(
            new CTaskSimpleStandStill{ 0, true },
            TASK_PRIMARY_DEFAULT
        );
    } else {
        tmgr.SetTask(
            CTaskComplexWander::GetWanderTaskByPedType(ped),
            TASK_PRIMARY_DEFAULT
        );
    }

    if (CCheat::IsActive(CHEAT_SLUT_MAGNET) && pedType == PED_TYPE_PROSTITUTE) {
        ped->GetEventGroup().Add(CEventSexyPed{ FindPlayerPed(), TASK_COMPLEX_GANG_HASSLE_PED });
        GiveAndSetPedWeapon(ped, CGeneral::RandomChoiceFromList({ WEAPON_DILDO1, WEAPON_DILDO2, WEAPON_VIBE1, WEAPON_VIBE2 }), 1);
    }

    if (CCheat::IsActive(CHEAT_PEDS_ATTACK_OTHER_WITH_GOLFCLUB)) {
        auto& pedsc = ped->GetIntelligence()->GetPedScanner();
        pedsc.ScanForPedsInRange(*ped);
        if (const auto closest = pedsc.GetClosestPedInRange()) { // Not what they did, but better (though possibly hacky)
            ped->GetEventGroup().Add(CEventAcquaintancePedHate{ closest, TASK_COMPLEX_KILL_PED_ON_FOOT });
        }
    }

    return ped;
}

// 0x612CD0 - Unused
CPed* CPopulation::AddDeadPedInFrontOfCar(const CVector& createPedAt, CVehicle* vehicle) {
    if (CanCameraSeeAPedHere(createPedAt)) {
        return nullptr;
    }

    bool bGroundHit{};
    const auto groundZ = std::max(CWorld::FindGroundZFor3DCoord(CVector{ createPedAt, createPedAt.z + 1.f }, &bGroundHit, nullptr) + 1.f, createPedAt.z);
    if (!bGroundHit) {
        return nullptr;
    }

    if (!CModelInfo::GetModelInfo(MODEL_MALE01)->GetRwObject()) {
        NOTSA_LOG_DEBUG("Didn't create ped, because `MODEL_MALE01` has no RW object!");
        return nullptr;
    }

    const auto ped = AddPed(PED_TYPE_CIVMALE, MODEL_MALE01, CVector{ createPedAt, groundZ }, false);

    ped->GetEventGroup().Add(
        CEventScriptCommand{
            TASK_PRIMARY_DEFAULT,
            new CTaskComplexDie{ WEAPON_UNARMED, ANIM_GROUP_DEFAULT, ANIM_ID_KO_SHOT_FRONT_0}
        }
    );

    ped->m_nMoneyCount = 0;
    ped->bDeadPedInFrontOfCar = true;
    CEntity::ChangeEntityReference(ped->m_VehDeadInFrontOf, vehicle);

    // Check if it's colliding with anything...
    if (   !CPedPlacement::IsPositionClearForPed(createPedAt, 2.f, {vehicle, ped})
        || CCollision::ProcessColModels(ped->GetMatrix(), *ped->GetColModel(), vehicle->GetMatrix(), *vehicle->GetColModel(), CWorld::m_aTempColPts, nullptr, nullptr, false) > 0
    ) {
        RemovePed(ped);
        return nullptr;
    }

    CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);

    return ped;
}

// 0x612F90
eModelID CPopulation::ChooseCivilianOccupation(
    bool         mustBeMale,
    bool         mustBeFemale,
    AssocGroupId mustUseThisAnimGroup,
    eModelID     mustNotBeThisModel,
    ePedStats    mustBeCompatibleWithThisPedStat,
    bool         bOnlyOnFoots,
    bool         doTestForUsedOccupations,
    bool         isAtAttractor,
    const char*  attractorScriptName
) {
    UNUSED(attractorScriptName);

    const size_t maxModelsToCheck = [&] {
        // We do some extra math here, so in case the size of the array is ever changed, it scales automagically!
        const auto nMaxModels = CStreaming::ms_pedsLoaded.size();
        if (doTestForUsedOccupations) {
            return nMaxModels * 7 / 8; // 7
        }
        if (!CGame::CanSeeOutSideFromCurrArea() && NumberOfPedsInUseInterior > 20) {
            return nMaxModels * 5 / 8; // 5
        }
        return nMaxModels * 3 / 8; // 3
    }();

    for (size_t i{}; i < maxModelsToCheck; i++) {
        const auto modelId = CStreaming::ms_pedsLoaded[i];
        if (modelId == MODEL_INVALID) {
            continue;
        }
        if (!CStreaming::IsModelLoaded(modelId)) { // So why the fuck is it in the `ms_pedsLoaded` array if it's not loaded?
            continue;
        }
        const auto mi = CModelInfo::GetPedModelInfo(modelId);
        if (mi->m_nRefCount != i) { // TODO/BUG: Why?
            continue;
        }
        if (mustNotBeThisModel == modelId) {
            continue;
        }
        if (CGame::CanSeeOutSideFromCurrArea() && !CPopCycle::PedIsAcceptableInCurrentZone(modelId)) {
            continue;
        }
        if (bOnlyOnFoots && (mi->m_nCarsCanDriveMask & 0x1000) == 0) {
            continue;
        }
        if (mustBeMale && mi->GetPedType() != PED_TYPE_CIVMALE) {
            continue;
        }
        if (mustBeFemale && mi->GetPedType() != PED_TYPE_CIVFEMALE) {
            continue;
        }
        if (mustUseThisAnimGroup != ANIM_GROUP_NONE && mi->m_nAnimType != mustUseThisAnimGroup) {
            continue;
        }
        if (isAtAttractor && !PedMICanBeCreatedAtAttractor(modelId)) {
            continue;
        }
        if (!CGame::CanSeeOutSideFromCurrArea() && !PedMICanBeCreatedInInterior(modelId)) {
            continue;
        }
        if (mustBeCompatibleWithThisPedStat != ePedStats::NONE && !ArePedStatsCompatible(mi->GetPedStatType(), mustBeCompatibleWithThisPedStat)) {
            continue;
        }
        if (CWeather::Rain >= 0.1f && IsSunbather(modelId)) {
            continue;
        }
        return modelId;
    }
    return doTestForUsedOccupations
        ? MODEL_INVALID
        : MODEL_MALE01;
}

// 0x613180
void CPopulation::ChooseCivilianCoupleOccupations(eModelID& husbandOccupation, eModelID& wifeyOccupation) {
    wifeyOccupation = husbandOccupation = MODEL_INVALID;
    
    const auto [husbandMustBeMale, wifeMustBeFemale] = []() -> std::pair<bool, bool> {
        if (CWeather::WeatherRegion != WEATHER_REGION_SF || (rand() & 16) == 0) {
            return { true, false };
        } else if (rand() & 32) {
            return { true, true };
        } else {
            return { false, false };
        }
    }();

    // TODO: Check this out... I'm not sure what to do here
    // You see, `ChooseCivilianOccupation` breaks if both `mustBeMale` and `mustBeFemale` are `true`... So I'm quite sure they just did an oopsie here.
//#ifdef FIX_BUGS
  //  if ((husbandOccupation = ChooseCivilianOccupation(husbandMustBeMale, false)) != MODEL_INVALID) {
  //      if ((wifeyOccupation = ChooseCivilianOccupation(false, wifeMustBeFemale)) != MODEL_INVALID) {
//#else
    if ((husbandOccupation = ChooseCivilianOccupation(husbandMustBeMale, wifeMustBeFemale)) != MODEL_INVALID) {
        if ((wifeyOccupation = ChooseCivilianOccupation(wifeMustBeFemale, husbandMustBeMale)) != MODEL_INVALID) {
//#endif // FIX_BUGS
            const auto IsSkater = [](eModelID model) {
                return CModelInfo::GetPedModelInfo(model)->GetPedStatType() == ePedStats::SKATER;
            };
            if (IsSkater(husbandOccupation) == IsSkater(wifeyOccupation)) { // Either they are both skaters, or they both aren't
                return; // All good
            }       
        }
    }

    // Fail
    wifeyOccupation = husbandOccupation = MODEL_INVALID;
}

// 0x613260
eModelID CPopulation::ChooseCivilianOccupationForVehicle(bool mustBeMale, CVehicle* vehicle) {
    const auto vehClass = vehicle->GetVehicleModelInfo()->m_nVehicleClass;
    for (size_t i{}; i < 2; i++) { // In 0th iteration we check if the model is inside the vehicle already, in the second we dont
        for (size_t k{}; k < 4; k++) {
            for (auto pedModelId : CStreaming::ms_pedsLoaded) {
                if (pedModelId == MODEL_INVALID) {
                    continue;
                }
                if (!CStreaming::IsModelLoaded(pedModelId)) {
                    continue;
                }
                const auto mi = CModelInfo::GetPedModelInfo(pedModelId);
                if (k != 4 && mi->m_nRefCount != k) {
                    continue;
                }
                if (!CCheat::IsAnyActive({
                    CHEAT_ELVIS_IS_EVERYWHERE,
                    CHEAT_PEDS_ATTACK_YOU_WITH_ROCKETS,
                    CHEAT_BEACH_PARTY,
                    CHEAT_GANGMEMBERS_EVERYWHERE,
                    CHEAT_NINJA_THEME,
                    CHEAT_SLUT_MAGNET,
                    CHEAT_FUNHOUSE_THEME,
                    CHEAT_COUNTRY_TRAFFIC,
                })) {
                    if (!CPopCycle::PedIsAcceptableInCurrentZone(pedModelId)) {
                        continue;
                    }
                    if (!mi->CanPedDriveVehicleClass(vehClass)) {
                        continue;
                    }
                }
                if (i != 0 || !vehicle->IsPedOfModelInside(pedModelId)) {
                    return pedModelId;
                }
            }
        }
    }
    return MODEL_MALE01;
}

// 0x6133F0
void CPopulation::CreateWaitingCoppers(CVector createAt, float createaWithHeading) {
    constexpr uint32 NUM_COPS_FOR_WANTED_LEVEL[7]{ 0, 1, 2, 4, 5, 6, 7 };
    constexpr uint32 NUM_CARS_FOR_WANTED_LEVEL[7]{ 0, 0, 0, 1, 1, 2, 2 };

    const auto plyrWantedLvl = FindPlayerWanted()->GetWantedLevel();
    assert(plyrWantedLvl <= eWantedLevel::WANTED_LEVEL_6);
    
    createAt.z += 1.f;

    // Create cop cars
    if (auto numOfCars = NUM_CARS_FOR_WANTED_LEVEL[+plyrWantedLvl]) {
        CNodeAddress createPosNodes[3]{};
        createPosNodes[0] = ThePaths.FindNthNodeClosestToCoors(
            createAt,
            0,
            30.f,
            false,
            false,
            3,
            false,
            true,
            &createPosNodes[1]
        );

        const auto copCarModel = CStreaming::GetDefaultCopCarModel();

        for (const auto nodeAddr : createPosNodes) {
            if (!nodeAddr.IsValid()) {
                continue;
            }

            const auto veh = new CAutomobile{ copCarModel, RANDOM_VEHICLE, true };

            // Set vehicle's position to the node's 
            veh->SetPosn(ThePaths.GetPathNode(nodeAddr)->GetPosition());
            veh->SetStatus(STATUS_ABANDONED);

            // Adjust vehicle to be pointing at the creation coords
            auto& vehMat = veh->GetMatrix();
            const auto createPosToVehDir2D = (CVector2D{createAt} - veh->GetPosition2D()).Normalized();
            vehMat.GetForward() = CVector{ createPosToVehDir2D };
            vehMat.GetRight() = CVector{ createPosToVehDir2D.GetPerpLeft() };

            veh->PlaceOnRoadProperly();
            veh->SetIsStatic(false);

            // Now, update the RW matrix too
            if (veh->GetRwObject()) {
                vehMat.UpdateRwMatrix(RwFrameGetMatrix(RpClumpGetFrame(veh->GetRpClump())));
            }

            CCarCtrl::JoinCarWithRoadSystem(veh);
            CWorld::Add(veh);

            if (!--numOfCars) {
                break;
            }
        }
    }

    // Create cop peds
    if (auto numOfCopPeds = NUM_COPS_FOR_WANTED_LEVEL[+plyrWantedLvl]) {
        for (int32 i{}; i < 20; i++) { // int32 angleOffset = 0; angleOffset > -20; angleOffset--
            const auto heading   = CGeneral::GetRandomNumberInRange(0.f, (float)(i) * 0.4f) - (float)(i) * 0.2f + createaWithHeading;
            auto       copPedPos = CVector{ CVector2D{createAt} + CVector2D{sin(heading), cos(heading)} *CGeneral::GetRandomNumberInRange(8.f, 10.f), createAt.z };

            if (!CWorld::GetIsLineOfSightClear(createAt, copPedPos, true, true, false, true)) {
                continue;
            }

            int16 numColliding{};
            CEntity* entitiesInRange[1];
            CWorld::FindObjectsInRange(copPedPos, 1.f, true, &numColliding, (int16)std::size(entitiesInRange), entitiesInRange, false, true, true, false, false);
            if (numColliding) {
                continue;
            }

            bool bGroundHit{};
            copPedPos.z = CWorld::FindGroundZFor3DCoord(copPedPos, &bGroundHit, nullptr);
            if (!bGroundHit) {
                continue;
            }

            const auto ped = new CCopPed{ 0 };

            ped->SetPosn(copPedPos);
            ped->m_fAimingRotation = ped->m_fCurrentRotation = CVector2D{ createAt - copPedPos }.Heading();
            ped->SetHeading(ped->m_fCurrentRotation);

            CWorld::Add(ped);

            if (plyrWantedLvl > eWantedLevel::WANTED_LEVEL_1) {
                ped->GiveWeapon(WEAPON_PISTOL, 30000, true);
                ped->SetCurrentWeapon(WEAPON_PISTOL);
            }

            ped->GetEventGroup().Add(
                CEventScriptCommand{
                    TASK_PRIMARY_PRIMARY,
                    new CTaskComplexKillPedOnFoot{FindPlayerPed(), -1, 0, 0, 0, 2, true, true}
                }
            );
            ped->GetIntelligence()->Process();

            if (!--numOfCopPeds) {
                break;
            }
        }
    }
}

// 0x613A00
CPed* CPopulation::AddPedInCar(
    CVehicle* veh,
    bool      addAsDriver,
    int32     carRating,
    int32     seatNumber,
    bool      mustBeMale,
    bool      isCriminal
) {
    // Pick a model and ped type to use (TODO: Could probably just get the model type, and then resolve the ped type from the model)
    const auto pedModel = [&]() -> eModelID {
        if (addAsDriver) {
            const auto driverModel = FindSpecificDriverModelForCar_ToUse(veh->GetModelId());
            if (driverModel != MODEL_INVALID && CStreaming::IsModelLoaded(driverModel)) {
                return driverModel;
            }
        }

        const auto FixIfInvalid = [&](eModelID model, bool checkRWObj = false) {
            return model != MODEL_INVALID && (!checkRWObj || CModelInfo::GetPedModelInfo(model)->GetRwObject())
                ? model
                : MODEL_MALE01;
        };

        const auto TranslateCopType = [&](eCopType ctype) {
            return CCopPed::GetPedModelForCopType(ctype);
        };

        switch (veh->GetModelId()) {
        case MODEL_FIRETRUK:
            return FixIfInvalid(CStreaming::GetDefaultFiremanModel());
        case MODEL_AMBULAN:
            return FixIfInvalid(CStreaming::GetDefaultMedicModel());
        case MODEL_ENFORCER:
            return TranslateCopType(COP_TYPE_SWAT1);
        case MODEL_PREDATOR:
        case MODEL_POLMAV:
        case MODEL_COPCARLA:
        case MODEL_COPCARSF:
        case MODEL_COPCARVG:
        case MODEL_COPCARRU:
            return TranslateCopType(COP_TYPE_CITYCOP);
        case MODEL_RHINO:
        case MODEL_BARRACKS:
            return TranslateCopType(COP_TYPE_ARMY);
        case MODEL_FBIRANCH:
            return TranslateCopType(COP_TYPE_FBI);
        case MODEL_COPBIKE:
            return TranslateCopType(COP_TYPE_LAPDM1);
        case MODEL_STREAKC:
            return FixIfInvalid(ChooseCivilianOccupation());
        default: {
            const auto gangPedTypeForRating = [carRating] {
                switch (carRating) {
                case 14: return PED_TYPE_GANG1;
                case 15: return PED_TYPE_GANG2;
                case 16: return PED_TYPE_GANG3;
                case 17: return PED_TYPE_GANG4;
                case 18: return PED_TYPE_GANG5;
                case 19: return PED_TYPE_GANG6;
                case 20: return PED_TYPE_GANG7;
                case 21: return PED_TYPE_GANG8;
                case 22: return PED_TYPE_GANG9;
                case 23: return PED_TYPE_GANG10;
                default: return PED_TYPE_NONE;
                }
            }();
            if (gangPedTypeForRating != PED_TYPE_NONE) {
                return FixIfInvalid(CGangs::ChooseGangPedModel(GetGangOfPedType(gangPedTypeForRating)), true);
            }
            return FixIfInvalid(ChooseCivilianOccupationForVehicle(mustBeMale, veh), true);
        }
        }
    }();

    const auto ped = AddPed(
        CModelInfo::GetPedModelInfo(pedModel)->GetPedType(),
        pedModel,
        veh->GetPosition(),
        false
    );

    CCarEnterExit::SetPedInCarDirect(
        ped,
        veh,
        seatNumber >= 0 ? CCarEnterExit::ComputeTargetDoorToEnterAsPassenger(veh, seatNumber) : 0,
        addAsDriver
    );

    if (isCriminal) {
        UpdatePedCount(ped, false);
        ped->m_nPedType = PED_TYPE_CRIMINAL;
        UpdatePedCount(ped, true);
    }

    return ped;
}

// 0x613CD0
void CPopulation::PlaceMallPedsAsStationaryGroup(const CVector& posn) {
    if (CGame::currArea == eAreaCodes::AREA_CODE_4) {
        CPedGroupPlacer{}.PlaceGroup(
            CModelInfo::GetPedModelInfo(ChooseCivilianOccupation())->GetPedType(),
            CGeneral::GetRandomNumberInRange(1, 5),
            posn,
            ePedGroupDefaultTaskAllocatorType::STAND_STILL
        );
    }        
}

// 0x613D60
void CPopulation::PlaceCouple(ePedType husbandPedType, eModelID husbandModelId, ePedType wifeyPedType, eModelID wifeyModelId, CVector placeAt) {
    if (CGameLogic::LaRiotsActiveHere()) {
        return;
    }

    if (husbandPedType != PED_TYPE_CIVMALE || wifeyPedType != PED_TYPE_CIVFEMALE) {
        return;
    }

    if (CanCameraSeeAPedHere(placeAt)) {
        return;
    }

    if (!CPedPlacement::IsPositionClearForPed(placeAt, CModelInfo::GetPedModelInfo(husbandModelId)->GetColModel()->GetBoundRadius())) {
        return;
    }

    const auto GetSetGroundZ = [](CVector& posn) {
        bool bGroundHit{};
        posn.z = std::max(CWorld::FindGroundZFor3DCoord(posn, &bGroundHit, nullptr) + 1.f, posn.z);
        return bGroundHit;
    };

    if (!GetSetGroundZ(placeAt)) {
        return;
    }

    const auto CreatePed = [&](ePedType ptype, eModelID model) -> CPed* {
        if (!CModelInfo::GetPedModelInfo(model)->GetRwObject()) {
            return nullptr;
        }
        return AddPed(ptype, model, placeAt, true);
    };

    const auto husb = CreatePed(PED_TYPE_CIVMALE, husbandModelId);
    if (!husb) {
        return;
    }
    CVisibilityPlugins::SetClumpAlpha(husb->GetRpClump(), 0);

    const auto wifey = CreatePed(PED_TYPE_CIVFEMALE, wifeyModelId);
    if (!wifey) {
        return; // No need to delete `husband`, as he's been added to the world already (and he'll get deleted eventually)
    }

    // If they both walk slow (or there's no real difference in their walk speed) => We're done
    const auto husbWalkSpeed = husb->GetWalkAnimSpeed(), wifeyWalkSpeed = wifey->GetWalkAnimSpeed();
    if (husbWalkSpeed < 0.75f || wifeyWalkSpeed < 0.75f || std::abs(husbWalkSpeed - wifeyWalkSpeed) > 0.45f) {
        return;
    }

    // 0x614028
    const auto wifeyIsLeader = wifeyWalkSpeed >= husbWalkSpeed; // Whoever is faster is the leader
    wifey->GetTaskManager().SetTask(new CTaskComplexBeInCouple{ husb, wifeyIsLeader }, TASK_PRIMARY_PRIMARY);
    husb->GetTaskManager().SetTask(new CTaskComplexBeInCouple{ wifey, !wifeyIsLeader }, TASK_PRIMARY_PRIMARY);

    // Update husband position
    auto husbNewPos = husb->GetPosition() + CVector{ CTaskComplexFollowLeaderInFormation::ms_offsets.Offsets[4] };
    if (GetSetGroundZ(husbNewPos)) {
        husb->SetPosn(husbNewPos);
    }

    if (CPedPlacement::IsPositionClearForPed(
        husbNewPos,
        wifey->GetColModel()->GetBoundRadius(),
        { husb, wifey }
    )) {
        CVisibilityPlugins::SetClumpAlpha(wifey->GetRpClump(), 0); // All good
    }  else { // Blocked by something
        RemovePed(wifey);
        RemovePed(husb);
    }
}

// 0x614210
bool CPopulation::AddPedAtAttractor(eModelID modelIndex, C2dEffectPedAttractor* attractor, CVector posn, CEntity* entity, int32 decisionMakerType) {
    if (FindDistanceToNearestPed(posn) <= 0.015f) {
        return false;
    }

    if (!GetPedAttractorManager()->HasQueueTailArrivedAtSlot(attractor, entity)) {
        return false;
    }

    const auto ped = AddPed(CModelInfo::GetPedModelInfo(modelIndex)->GetPedType(), modelIndex, posn, false);
    if (!ped) {
        return false;
    }
    
    ped->SetCharCreatedBy(PED_GAME);
    ped->GetIntelligence()->SetPedDecisionMakerType(decisionMakerType == -1 ? 2 : decisionMakerType);
    ped->GetTaskManager().SetTask(CTaskComplexWander::GetWanderTaskByPedType(ped), TASK_PRIMARY_DEFAULT);

    CPedAttractorPedPlacer::PlacePedAtEffect(*attractor, entity, ped, 0.02f);
    ped->bUseAttractorInstantly = true;

    ped->GetEventGroup().Add(CEventAttractor{ attractor, ped, true, TASK_COMPLEX_USE_ATTRACTOR });

    ped->GetIntelligence()->ProcessEventHandler();
    ped->GetIntelligence()->Process();

    return true;
}

// 0x6143E0
// NOTSA: Added option to use `ePedType::NONE` as valid value to ignore the ped type (this way you can get the distance of the nearest ped of any type)
float CPopulation::FindDistanceToNearestPedOfType(ePedType pedType, CVector posn) {
    float closest3DSq = sq(10'000'000.f);
    for (CPed& ped : GetPedPool()->GetAllValid()) {
        if (pedType != PED_TYPE_NONE /*notsa*/ && ped.m_nPedType != pedType) {
            continue;
        }
        closest3DSq = std::min(closest3DSq, (ped.GetPosition() - posn).SquaredMagnitude());
    }
    return std::sqrt(closest3DSq);

    /* Which is more readable? Really trying to love `ranges`, but man... (Above is also twice as fast :D, because the transform isn't invoked every time)
    * 
    auto peds = GetPedPool()->GetAllValid()
        | rng::views::filter([&](CPed& ped) { return ped.m_nPedType == pedType; })
        | rng::views::transform([&](CPed& ped) { return (ped.GetPosition() - pos).SquaredMagnitude(); });
    return rng::empty(peds)
        ? 10'000'000.f
        : std::sqrt(rng::min(peds));

    *
    ** there's also the 3rd option: **
    * 
    return std::sqrt(notsa::min_default(
          GetPedPool()->GetAllValid()
        | rng::views::filter([&](CPed& ped) { return ped.m_nPedType == pedType; })
        | rng::views::transform([&](CPed& ped) { return (ped.GetPosition() - pos).SquaredMagnitude(); }),
        sq(10'000'000.f)
    ));
    */
}

float CPopulation::FindDistanceToNearestPed(CVector pos) {
    return FindDistanceToNearestPedOfType(PED_TYPE_NONE, pos);
}

// 0x614490
eModelID CPopulation::PickGangCar(eGangID forGang) {
    return (eModelID)m_LoadedGangCars[(size_t)(forGang)].PickRandomCar(false, false);
}

// 0x6144B0
eModelID CPopulation::PickRiotRoadBlockCar() {
    // First try gang cars
    const auto baseIdx = CGeneral::GetRandomNumberInRange<size_t>(0, TOTAL_GANGS);
    for (size_t i{}; i < TOTAL_GANGS; i++) {
        const auto model = PickGangCar((eGangID)(baseIdx + i));
        if (model != MODEL_INVALID) {
            return model;
        }
    }

    // Try appropriate/inappropriate cars
    for (auto grp : { &m_AppropriateLoadedCars, &m_InAppropriateLoadedCars }) {
        for (auto i{ grp->CountMembers() }; i --> 0;) { // TODO: Use `grp->GetAllModels()`
            const auto model = (eModelID)grp->GetMember(i);
            if (model == MODEL_INVALID) {
                continue;
            }
            switch (CModelInfo::GetVehicleModelInfo(model)->m_nVehicleClass) {
            case VEHICLE_CLASS_BIG:
            case VEHICLE_CLASS_MOPED:
            case VEHICLE_CLASS_MOTORBIKE:
                continue;
            }
            return model;
        }
    }

    // Nothing suits
    return CStreaming::GetDefaultCopCarModel();
}

// 0x614580
void CPopulation::ConvertToRealObject(CDummyObject* dummyObject) {
    if (!CPopulation::TestSafeForRealObject(dummyObject)) {
        return;
    }

    auto* obj = dummyObject->CreateObject();
    if (!obj) {
        return;
    }

    CWorld::Remove(dummyObject);
    dummyObject->SetIsVisible(false);
    dummyObject->ResolveReferences();

    obj->SetRelatedDummy(dummyObject);
    CWorld::Add(obj);

    if (!CGlass::IsObjectGlass(obj) || obj->GetModelInfo()->IsGlassType2()) {
        if (obj->m_nModelIndex == ModelIndices::MI_BUOY || obj->physicalFlags.bAttachedToEntity) {
            obj->SetIsStatic(false);
            obj->m_vecMoveSpeed.Set(0.0F, 0.0F, -0.001F);
            obj->physicalFlags.bTouchingWater = true;
            obj->AddToMovingList();
        }
    } else {
        obj->SetIsVisible(false);
    }
}

// 0x614670
void CPopulation::ConvertToDummyObject(CObject* object) {
    auto* dummy = object->m_pDummyObject;
    if (dummy) {
        if (!CPopulation::TestRoomForDummyObject(object)) {
            return;
        }
        dummy->SetIsVisible(true);
        dummy->UpdateFromObject(object);
    }

    if (object->GetIsTypeObject()) {
        auto* mi = object->GetModelInfo()->AsAtomicModelInfoPtr();
        if (mi && mi->IsGlassType1()) {
            if (dummy) {
                dummy->SetIsVisible(false);
            } else {
                assert(false && "FIX_BUGS: dummy == nullptr");
            }
        }
    }

    CWorld::Remove(object);
    delete object;

    if (dummy) {
        CWorld::Add(dummy);
    }
}

// 0x614720
bool CPopulation::AddToPopulation(float minDist, float maxDist, float minDistClose, float maxDistClose) {
    if (CGangWars::DontCreateCivilians() || CCheat::m_aCheatsActive[CHEAT_REDUCED_TRAFFIC]) {
        return false;
    }

    const auto  playerIdx    = CWorld::PlayerInFocus;
    const auto& player       = CWorld::Players[playerIdx];
    const auto& playerPos    = FindPlayerCentreOfWorld(playerIdx);
    const auto  playerWanted = player.m_pPed->GetWanted();

    // Force a cop to spawn instead of a civilian if the player is wanted, but there aren't enough cops/cars around
    bool forceCop{};
    if (playerWanted->m_WantedLevel > eWantedLevel::WANTED_LEVEL_2
        && (int32)ms_nNumCop < (int32)playerWanted->m_MaxCopsInPursuit
        && !CGangWars::GangWarFightingGoingOn()
        && !player.m_pPed->bInVehicle
        && (   CCarCtrl::NumLawEnforcerCars >= (uint32)playerWanted->m_MaxCopCarsInPursuit
            || (float)(int32)player.m_nCarDensityForCurrentZone * CCarCtrl::CarDensityMultiplier <= (float)CCarCtrl::NumRandomCars
            || CCarCtrl::MaxNumberOfCarsInUse <= CCarCtrl::NumAmbulancesOnDuty + CCarCtrl::NumFireTrucksOnDuty + CCarCtrl::NumParkedCars + CCarCtrl::NumMissionCars + CCarCtrl::NumRandomCars + CCarCtrl::NumLawEnforcerCars
           )
    ) {
        forceCop = true;
        minDist  = PedCreationDistMultiplier() * 42.5f;
        maxDist  = PedCreationDistMultiplier() * 50.5f;
    }

    if (CGameLogic::LaRiotsActiveHere() && PedDensityMultiplier > 0.1f && (int32)ms_nNumCop < 1) {
        forceCop = true;
    }

    int32 numPeds{(int32)MaxNumberOfPedsInUse};
    CDarkel::FrenzyOnGoing(); // Result unused
    if (const float pedCap = CPopCycle::m_NumOther_Peds + CPopCycle::m_NumCops_Peds + CPopCycle::m_NumGangs_Peds + CPopCycle::m_NumDealers_Peds; pedCap <= (float)numPeds) {
        numPeds = (int32)pedCap;
    }
    if (CGame::currArea != 0) {
        numPeds = (int32)NumberOfPedsInUseInterior;
    }

    const bool belowTarget = (float)(int32)ms_nTotalPeds < (CCullZones::FewerPeds() ? 0.6f : 1.f) * PedDensityMultiplier * (float)numPeds;

    ePedType pedType{PED_TYPE_CIVMALE};
    eModelID model{MODEL_INVALID};
    eModelID spouseModel{MODEL_INVALID}; // Husband
    eModelID wifeyModel{MODEL_INVALID};  // Wife

    if (belowTarget && !forceCop) {
        // Pick a random ped
        const bool highUp = playerPos.z > 950.f;
        if (!CPopCycle::FindNewPedType(pedType, model, highUp, highUp)) {
            return belowTarget;
        }

        if ((pedType == PED_TYPE_CIVMALE || pedType == PED_TYPE_CIVFEMALE) && CGeneral::GetRandomNumberInRange(0.f, 1.f) > 0.9f) {
            ChooseCivilianCoupleOccupations(spouseModel, wifeyModel);
            if (spouseModel == MODEL_INVALID || wifeyModel == MODEL_INVALID) {
                return belowTarget;
            }
            pedType = CModelInfo::GetPedModelInfo(spouseModel)->m_nPedType;
        }

        if (m_AllRandomPedsThisType > 0) {
            pedType = (ePedType)m_AllRandomPedsThisType;
        }

        numPeds = IsPedTypeGang(pedType) ? GetRandomGangMemberCount() : 1;
    } else if (forceCop) {
        pedType = PED_TYPE_COP;
        model   = (eModelID)COP_TYPE_CITYCOP; // For cops this is the cop type, not a model id (See `ChoosePolicePedOccupation`)
        numPeds = 1;
    } else {
        return belowTarget;
    }

    // Gangs are spawned further away from the player
    float minD = minDist, maxD = maxDist;
    if (IsPedTypeGang(pedType)) {
        minD += 30.f;
        maxD += 30.f;
    }

    const bool isWantedCop = pedType == PED_TYPE_COP && FindPlayerWanted(-1)->m_WantedLevel > eWantedLevel::WANTED_CLEAN;

    CVector     spawnPos;
    CNodeAddress nodeA, nodeB;
    float       unusedRandFactor{};
    if (!FindSpawnPosBetweenNodes(
            playerPos.x, playerPos.y, minD, maxD, minDistClose, maxDistClose, &spawnPos, &nodeA, &nodeB, &unusedRandFactor, isWantedCop, nullptr
        )
    ) {
        return belowTarget;
    }

    // Spawn probability is that of the two nodes the position was interpolated between
    const uint32 nodeSpawnProbability = std::min<uint32>(
        ThePaths.m_pPathNodes[nodeA.m_wAreaId][nodeA.m_wNodeId].m_nSpawnProbability,
        ThePaths.m_pPathNodes[nodeB.m_wAreaId][nodeB.m_wNodeId].m_nSpawnProbability
    );
    if ((uint32)(rand() & 0xF) > nodeSpawnProbability) {
        return belowTarget;
    }

    // Jitter the position within the width of the path
    JitterPosBetweenNodes(ThePaths, nodeA, nodeB, rand(), &spawnPos.x, &spawnPos.y);

    if (IsPedTypeGang(pedType)) {
        PlaceGangMembers(pedType, numPeds, spawnPos);
        return belowTarget;
    }

    if (spouseModel >= 0 && wifeyModel >= 0) {
        // Couple
        CVector couplePos = spawnPos;
        if (CGameLogic::LaRiotsActiveHere()) {
            return belowTarget;
        }

        if (TheCamera.IsSphereVisible(couplePos, 1.5f)) {
            if ((FindPlayerPed(-1)->GetPosition() - couplePos).Magnitude2D() < PedCreationDistMultiplier() * 42.5f) {
                return belowTarget;
            }
        }

        if (!CPedPlacement::IsPositionClearForPed(couplePos, CModelInfo::GetPedModelInfo(spouseModel)->GetColModel()->GetBoundRadius())) {
            return belowTarget;
        }

        bool groundHit{};
        const float groundZ = CWorld::FindGroundZFor3DCoord(couplePos + CVector{0.f, 0.f, 1.f}, &groundHit, nullptr);
        if (!groundHit) {
            return belowTarget;
        }
        const float spawnZ = std::max(couplePos.z, groundZ + 1.f);

        if (!CModelInfo::GetPedModelInfo(spouseModel)->GetRwObject()) {
            return belowTarget;
        }
        CPed* const husb = AddPed(PED_TYPE_CIVMALE, spouseModel, {couplePos.x, couplePos.y, spawnZ}, true);
        if (!husb) {
            return belowTarget;
        }
        CVisibilityPlugins::SetClumpAlpha(husb->GetRpClump(), 0);

        if (!CModelInfo::GetPedModelInfo(wifeyModel)->GetRwObject()) {
            return belowTarget;
        }
        CPed* const wifey = AddPed(PED_TYPE_CIVFEMALE, wifeyModel, {couplePos.x, couplePos.y, spawnZ}, true);
        if (!wifey) {
            return belowTarget;
        }

        // If they both walk slow (or there's no real difference in their walk speed) => We're done
        const auto wifeyWalkSpeed = wifey->GetWalkAnimSpeed(), husbWalkSpeed = husb->GetWalkAnimSpeed();
        if (wifeyWalkSpeed >= 0.75f && husbWalkSpeed >= 0.75f && std::abs(wifeyWalkSpeed - husbWalkSpeed) <= 0.45f) {
            if (wifeyWalkSpeed <= husbWalkSpeed) { // The male is at least as fast => He's the leader
                wifey->GetTaskManager().SetTask(new CTaskComplexBeInCouple{husb, false}, TASK_PRIMARY_PRIMARY);
                husb->GetTaskManager().SetTask(new CTaskComplexBeInCouple{wifey, true}, TASK_PRIMARY_PRIMARY);
            } else { // The wife is faster => She's the leader
                husb->GetTaskManager().SetTask(new CTaskComplexBeInCouple{wifey, false}, TASK_PRIMARY_PRIMARY);
                wifey->GetTaskManager().SetTask(new CTaskComplexBeInCouple{husb, true}, TASK_PRIMARY_PRIMARY);
            }

            // Move the wife next to the husband
            CVector wifeyNewPos = husb->GetPosition() + CVector{CTaskComplexFollowLeaderInFormation::ms_offsets.Offsets[4]};
            bool wifeyGroundHit{};
            const float wifeyGroundZ = CWorld::FindGroundZFor3DCoord(wifeyNewPos + CVector{0.f, 0.f, 1.f}, &wifeyGroundHit, nullptr);
            if (!wifeyGroundHit) {
                RemovePed(husb);
                RemovePed(wifey);
            } else {
                wifeyNewPos.z = std::max(wifeyNewPos.z, wifeyGroundZ + 1.f);
                wifey->SetPosn(wifeyNewPos);

                // Only the couple itself may be in the way
                CEntity* entsInTheWay[3]{};
                CPedPlacement::IsPositionClearForPed(wifeyNewPos, CModelInfo::GetPedModelInfo(wifeyModel)->GetColModel()->GetBoundRadius(), 3, entsInTheWay, true, true, true);
                for (const auto ent : entsInTheWay) {
                    if (ent && ent != husb && ent != wifey) {
                        RemovePed(husb);
                        RemovePed(wifey);
                        return belowTarget;
                    }
                }
                CVisibilityPlugins::SetClumpAlpha(wifey->GetRpClump(), 0);
            }
        }
        return belowTarget;
    }

    // Single ped(s)
    if (numPeds <= 0) {
        return belowTarget;
    }

    const auto IsWeaponModelLoaded = [](eWeaponType weapon) {
        return CStreaming::IsModelLoaded(CWeaponInfo::GetWeaponInfo(weapon, eWeaponSkill::STD)->m_nModelId1);
    };

    int32 pedIdx{};
    CPed* firstPed{};
    while (true) {
        CPed* const nearPed = firstPed;

        if (pedType == PED_TYPE_COP) {
            // Special cop types require their model (and weapons) to be loaded
            if (model == (eModelID)COP_TYPE_FBI) {
                if (!CStreaming::IsModelLoaded(MODEL_FBI) || !IsWeaponModelLoaded(WEAPON_MP5)) {
                    return belowTarget;
                }
            } else if (model == (eModelID)COP_TYPE_SWAT1) {
                if (!CStreaming::IsModelLoaded(MODEL_SWAT) || !IsWeaponModelLoaded(WEAPON_MICRO_UZI)) {
                    return belowTarget;
                }
            } else if (model == (eModelID)COP_TYPE_ARMY) {
                if (!CStreaming::IsModelLoaded(MODEL_ARMY) || !IsWeaponModelLoaded(WEAPON_MP5) || !IsWeaponModelLoaded(WEAPON_GRENADE)) {
                    return belowTarget;
                }
            }
        } else if (!CModelInfo::GetModelInfo(model)->GetRwObject()) {
            return belowTarget;
        }

        spawnPos.z += 0.7f;
        if (pedIdx < numPeds && rand() != 0 && nearPed) {
            const float maxOff = ((float)pedIdx + 1.f) * 0.5f;
            const float minOff = (float)pedIdx * 0.5f;
            const float offX = CGeneral::GetRandomNumberInRange(minOff, maxOff);
            const float offY = CGeneral::GetRandomNumberInRange(minOff, maxOff);
            const auto  nearPos = nearPed->GetPosition();
            spawnPos.x = nearPos.x + ((rand() & 1) ? -offX : offX);
            spawnPos.y = nearPos.y + ((rand() & 1) ? -offY : offY);
        }

        if (!CPedPlacement::IsPositionClearForPed(spawnPos, -1.f, -1, nullptr, true, true, true)) {
            return belowTarget;
        }

        const int32 nextIdx = pedIdx + 1;
        if (nextIdx < numPeds) {
            bool groundHit{};
            const float groundZ = CWorld::FindGroundZFor3DCoord({spawnPos.x, spawnPos.y, spawnPos.z + 2.f}, &groundHit, nullptr);
            if (!groundHit) {
                return belowTarget;
            }
            if (spawnPos.z <= groundZ + 0.7f) {
                spawnPos.z = groundZ + 0.7f;
            }
        }

        bool ok = true;
        if (TheCamera.IsSphereVisible(spawnPos, 2.f) && (spawnPos - playerPos).Magnitude2D() < PedCreationDistMultiplier() * 42.5f) {
            ok = false;
        } else if (CModelInfo::GetPedModelInfo(model)->m_nStatType == ePedStats::SKATER) {
            ok = IsSkateable(spawnPos);
        }

        if (IsSunbather(model) && (CClock::ms_nGameClockHours < 8 || CClock::ms_nGameClockHours > 19)) {
            ok = false;
        }

        if (pedType == PED_TYPE_DEALER) {
            if (!ok) {
                return belowTarget;
            }
            if (FindDistanceToNearestPedOfType(PED_TYPE_DEALER, spawnPos) < 20.f) {
                return belowTarget;
            }
        }

        if (!ok) {
            return belowTarget;
        }

        // No gang peds in the stadium's parking lot
        if (IsPedTypeGang(pedType) && pedType != PED_TYPE_GANG2
            && spawnPos.x > 2400.f && spawnPos.x < 2540.f
            && spawnPos.y > -1730.f && spawnPos.y < -1624.f
        ) {
            return belowTarget;
        }

        CPed* const ped = AddPed(pedType, model, spawnPos, true);

        // Try to make the ped sunbathe (Only successful if on a beach-like surface)
        enum class eSunbatheResult { DefaultTasks, NoTask, TaskSet };
        const auto TrySunbathe = [&]() -> eSunbatheResult {
            CColPoint colPoint{};
            CEntity*  hitEnt{};
            CWorld::ProcessVerticalLine({spawnPos.x, spawnPos.y, spawnPos.z + 2.f}, spawnPos.z - 2.f, colPoint, hitEnt, true, false, false, false, false, false, nullptr);
            if (!hitEnt) {
                return eSunbatheResult::DefaultTasks;
            }

            const auto surface = colPoint.m_nSurfaceTypeB;
            if (!(g_surfaceInfos.IsBeach(surface) || surface == SURFACE_CONCRETE_BEACH || surface == SURFACE_PARKGRASS || CCheat::m_aCheatsActive[CHEAT_BEACH_PARTY])) {
                return eSunbatheResult::DefaultTasks;
            }
            if (!IsSunbathingWeather() && !CCheat::m_aCheatsActive[CHEAT_BEACH_PARTY]) {
                return eSunbatheResult::DefaultTasks;
            }

            // Only the ped itself may be in the way
            bool     free{true};
            CEntity* entsInTheWay[2]{};
            if (CPedPlacement::IsPositionClearForPed(spawnPos, 3.f, 2, entsInTheWay, true, true, true)) {
                for (const auto ent : entsInTheWay) {
                    free = free && (!ent || ent == ped);
                }
            }

            const bool onSand = surface != SURFACE_PARKGRASS;
            if (!free) {
                return eSunbatheResult::DefaultTasks;
            }

            // The ground must be flat enough for the ped to lie on
            const auto pedPos = ped->GetPosition();
            CColPoint  groundColPoint{};
            CEntity*   groundEnt{};
            if (!CWorld::ProcessVerticalLine({pedPos.x, pedPos.y, pedPos.z + 10.f}, -10.f, groundColPoint, groundEnt, true, false, false, false, true, false, nullptr)) {
                return eSunbatheResult::NoTask;
            }
            if (groundColPoint.m_vecNormal.z <= 0.95f) {
                return eSunbatheResult::NoTask;
            }

            const float heading = CGeneral::GetRandomNumberInRange(0.f, 1.f) * 6.2831855f;
            ped->m_fAimingRotation  = heading;
            ped->m_fCurrentRotation = heading;
            ped->SetHeading(heading);

            CObject* towel{};
            if (onSand) {
                CVector towelPos{pedPos.x, pedPos.y, groundColPoint.m_vecPoint.z + 0.04f};
                towel = CreateBeachProp(&towelPos, 11);
                if (towel) {
                    towel->SetHeading(heading);

                    // Align the towel with the ground
                    auto& towelMat = towel->GetMatrix();
                    towelMat.GetUp()      = groundColPoint.m_vecNormal;
                    towelMat.GetRight()   = CrossProduct(towelMat.GetUp(), towelMat.GetForward());
                    towelMat.GetForward() = CrossProduct(towelMat.GetUp(), towelMat.GetRight());
                    towel->UpdateRwMatrix();
                    towel->UpdateRwFrame();

                    // Maybe create some more beach props next to the towel
                    if ((rand() & 3) == 0) {
                        CVector propPos = towelPos;
                        propPos += towelMat.GetRight()   * CGeneral::GetRandomNumberInRange(-0.5f, 0.5f);
                        propPos += towelMat.GetForward() * CGeneral::GetRandomNumberInRange(-1.f, 1.f);
                        CreateBeachProp(&propPos, 6);
                    }
                }
            }

            ped->GetTaskManager().SetTask(new CTaskComplexSunbathe(towel, (rand() & 3) == 0), TASK_PRIMARY_PRIMARY);
            return eSunbatheResult::TaskSet;
        };

        if (!IsSunbather(model) || (rand() & 3) == 0 || TrySunbathe() == eSunbatheResult::DefaultTasks) {
            if (!CGameLogic::LaRiotsActiveHere()) {
                CTheScripts::ScriptsForBrains.CheckIfNewEntityNeedsScript(ped, 0, nullptr);
            } else if ((rand() & 3) == 0) {
                // Rioting peds carry an object around
                if (pedType != PED_TYPE_COP) {
                    auto* const obj = new CObject(ModelIndices::MI_TELLY, true);
                    CWorld::Add(obj);
                    ped->GetTaskManager().SetTask(CTaskComplexWander::GetWanderTaskByPedType(ped), TASK_PRIMARY_DEFAULT);
                    ped->GetTaskManager().SetTaskSecondary(
                        new CTaskSimpleHoldEntity(obj, CVector{0.f, 0.45f, 0.35f}, 1, 1, ANIM_ID_CRRY_PRTIAL, ANIM_GROUP_CARRY, false),
                        TASK_SECONDARY_PARTIAL_ANIM
                    );
                }
            } else {
                ped->GetTaskManager().SetTask(new CTaskComplexWanderCriminal(PEDMOVE_RUN, (uint8)CGeneral::GetRandomNumberInRange(0, 8), true), TASK_PRIMARY_DEFAULT);
            }
        }

        if (!firstPed) {
            firstPed = ped;
        }
        CVisibilityPlugins::SetClumpAlpha(ped->GetRpClump(), 0);

        pedIdx = nextIdx;
        if (nextIdx > numPeds || numPeds < 2 || nextIdx >= numPeds) {
            return belowTarget;
        }
    }
}

// 0x615970
int32 CPopulation::GeneratePedsAtAttractors(
    CVector pos,
    float   minRadius,
    float   maxRadius,
    float   minRadiusClose,
    float   maxRadiusClose,
    int32   decisionMaker,
    int32   numPedsToCreate
) {
    ZoneScoped;

    if (!numPedsToCreate) {
        return 0;
    }

    CEntity* entitiesInRng[512];
    int16    numEntitiesInRng{};
    CWorld::FindObjectsInRange(pos, maxRadius, false, &numEntitiesInRng, (int16)std::size(entitiesInRng), entitiesInRng, true, false, false, true, false);
    if (!numEntitiesInRng) {
        return 0;
    }

    const auto IsEffectInRadius = [&](CVector effectPos) {
        const auto EffInRange = [
            effDistSq = (effectPos - pos).SquaredMagnitude()
        ](float min, float max) {
            return effDistSq >= sq(min) && effDistSq <= sq(max);
        };
        return TheCamera.IsSphereVisible(effectPos, 2.f)
            ? EffInRange(minRadius, maxRadius)
            : EffInRange(minRadiusClose, maxRadiusClose);
    };

    int32 numPedsCreated{};
    for (size_t i{}; i < 12; i++) {
        for (int16 o{}; o < numEntitiesInRng; o++) {
            const auto ent = entitiesInRng[o];
            assert(ent);
            if (!ent->GetRwObject()) {
                continue;
            }
            if (!ent->IsInCurrentArea()) {
                continue;
            }
            auto* const attractor = notsa::cast_if_present<C2dEffectPedAttractor>(ent->GetRandom2dEffect(EFFECT_ATTRACTOR, true));
            if (!attractor || !IsCorrectTimeOfDayForEffect(*attractor)) {
                continue;
            }
            if (attractor->m_nFlags & 1) {
                if (!ent->GetIsTypeObject()) {
                    continue;
                }
                if (!ent->AsObject()->objectFlags.bEnableDisabledAttractors) {
                    continue;
                }
            }

            const auto effectPosWS = ent->GetMatrix().TransformPoint(attractor->m_Pos); // ws = world space
            if (!IsEffectInRadius(effectPosWS)) {
                continue;
            }

            const auto usePoliceModel = bInPoliceStation
                && CGeneral::RandomBool(70.f)
                && PedMICanBeCreatedAtThisAttractor(CStreaming::GetDefaultCopModel(), attractor->m_szScriptName);

            const auto model = usePoliceModel
                ? CStreaming::GetDefaultCopModel()
                : ChooseCivilianOccupation(
                    false,
                    false,
                    ANIM_GROUP_NONE,
                    MODEL_INVALID,
                    ePedStats::NONE,
                    true,
                    true,
                    true,
                    attractor->m_szScriptName
                );

            if (usePoliceModel) {
                decisionMaker = 1; // TODO: Shouldn't this be local to this iteration instead? Right now this will presist into all futher iterations...
            }

            switch (model) {
            case MODEL_INVALID:
            case MODEL_MALE01:
                continue;
            }

            if (!AddPedAtAttractor(model, attractor, effectPosWS, ent, decisionMaker)) {
                continue;
            }

            numPedsCreated++;

            if (decisionMaker == -1) {
                break;
            }

            if (numPedsCreated == numPedsToCreate) {
                break;
            }
        }
    }

    return numPedsCreated;
}

// 0x615C90
void CPopulation::GeneratePedsAtStartOfGame() {
    ZoneScoped;

    const auto minRadius = 10.f, maxRadius = 50.5f * PedCreationDistMultiplier();
    
    for (int32 i = 100; i --> 0;) { // "down to" operator in use
        UpdatePedCounts();
        ms_nTotalPeds -= ms_nTotalCarPassengerPeds; // NOTE/TODO: I wonder why this part isnt in `UpdatePedCounts()`

        AddToPopulation(minRadius, maxRadius, minRadius, maxRadius);
    }

    GeneratePedsAtAttractors(FindPlayerCentreOfWorld(), minRadius, maxRadius, minRadius, maxRadius, -1, 1);
}

// 0x615DC0
void CPopulation::ManageObject(CObject* object, const CVector& posn) {
    if (!object->CanBeDeleted()) {
        return;
    }

    const auto model = object->m_nModelIndex;
    const auto dist  = (object->GetPosition() - posn).Magnitude();

    const auto RemoveObject = [object] {
        CWorld::Remove(object);
        delete object; // The exe calls the deleting destructor through the vtable
    };

    if (object->m_nObjectType != OBJECT_TEMPORARY) {
        // Not a temporary object - Convert it to a dummy object once it's far enough away
        const auto maxDist = FindDummyDistForModel(object->GetModelId());
        if (dist <= maxDist) {
            return;
        }
        const auto dummyDist = object->m_pDummyObject
            ? (object->m_pDummyObject->GetPosition() - posn).Magnitude()
            : 100'000.f;
        if (dummyDist <= maxDist) {
            return;
        }
        ConvertToDummyObject(object);
        return;
    }

    // Temporary object - Remove it once it's too far away, or no longer visible
    if (model == ModelIndices::MI_ROADWORKBARRIER1
        || model == ModelIndices::MI_ROADBLOCKFUCKEDCAR1
        || model == ModelIndices::MI_ROADBLOCKFUCKEDCAR2
        || model == ModelIndices::MI_BEACHBALL
    ) {
        if (dist < 120.f) {
            return;
        }
        RemoveObject();
        return;
    }

    if (model >= ModelIndices::MI_BEACHTOWEL01 && model <= ModelIndices::MI_BEACHTOWEL04) {
        if (dist <= 64.5f) {
            if (dist <= 35.f) {
                return;
            }
            if (object->GetIsOnScreen()) {
                return;
            }
        }
        RemoveObject();
        return;
    }

    if (dist > 54.5f) {
        RemoveObject();
        return;
    }
    if (dist > 25.f && !object->GetIsOnScreen()) {
        RemoveObject();
        return;
    }
    if (CTimer::GetTimeInMS() > object->m_nRemovalTime) {
        RemoveObject();
        return;
    }

    const auto rwObject = object->GetRwObject();
    if (!rwObject) {
        return;
    }
    if (RwObjectGetType(rwObject) != rpCLUMP) {
        return;
    }
    if (!object->objectFlags.bFadingIn) {
        return;
    }
    if (CVisibilityPlugins::GetClumpAlpha(object->GetRpClump()) == 0) {
        RemoveObject();
        return;
    }
    if (!object->IsVisible()) {
        RemoveObject();
        return;
    }
}

// 0x616000
void CPopulation::ManageDummy(CDummy* dummy, const CVector& posn) {
    if (!dummy->IsInCurrentArea() || !dummy->GetIsVisible()) {
        return;
    }
    if ((posn - dummy->GetPosition()).SquaredMagnitude() >= sq(FindDummyDistForModel(dummy->GetModelId()))) {
        return;
    }
    ConvertToRealObject(static_cast<CDummyObject*>(dummy));
}

// 0x6160A0
void CPopulation::ManageAllPopulation() {
    const auto objPlyrIsHolding = [] {
        const auto holdEntityTask = notsa::dyn_cast_if_present<CTaskSimpleHoldEntity>(FindPlayerPed()->GetIntelligence()->GetTaskHold(false));
        return holdEntityTask
            ? holdEntityTask->GetHeldEntity()
            : nullptr;
    }();

    const auto& center = FindPlayerCentreOfWorld();

    for (auto& obj : GetObjectPool()->GetAllValid()) {
        if (&obj != objPlyrIsHolding) {
            ManageObject(&obj, center);
        }
    }

    for (auto& dummy : GetDummyPool()->GetAllValid()) {
        ManageDummy(&dummy, center);
    }
}

// 0x616190
void CPopulation::ManagePopulation() {
    ZoneScoped;
    constexpr auto framePopulation = 32;
    const auto& centre = FindPlayerCentreOfWorld();
    const uint32 batch = CTimer::GetFrameCounter() % framePopulation;
    
    {
        ZoneScopedN("Manage Objects");
        auto* pool = GetObjectPool();
        const auto poolSize = pool->GetSize();
        const auto startIdx = (poolSize * batch) / framePopulation;
        const auto endIdx   = (poolSize * (batch + 1)) / framePopulation;
        for (auto i = startIdx; i < endIdx; ++i) {
            if (auto* obj = pool->GetAt(i)) {
                ManageObject(obj, centre);
            }
        }
    }
    
    {
        ZoneScopedN("Manage Dummies");
        auto* pool = GetDummyPool();
        const auto poolSize = pool->GetSize();
        const auto startIdx = (poolSize * batch) / framePopulation;
        const auto endIdx   = (poolSize * (batch + 1)) / framePopulation;
        for (auto i = startIdx; i < endIdx; ++i) {
            if (auto* dummy = pool->GetAt(i)) {
                ManageDummy(dummy, centre);
            }
        }
    }
    
    {
        ZoneScopedN("Manage Peds");
        auto* pool = GetPedPool();
        for (auto& ped : GetPedPool()->GetAllValid()) {
            ManagePed(&ped, centre);
        }
    }
}

// 0x616300
void CPopulation::RemovePedsIfThePoolGetsFull() {
    ZoneScoped;

    if (CTimer::GetFrameCounter() % 8 != 5) {
        return;
    }
    if (GetPedPool()->GetNoOfFreeSpaces() >= 8) {
        return;
    }
    const auto closest = rng::min( // It's guaranteed there to be a ped
        GetPedPool()->GetAllValid<CPed*>(),
        {},
        [campos = TheCamera.GetPosition()](CPed* ped) { return (ped->GetPosition() - campos).SquaredMagnitude(); }
    );
    RemovePed(closest);
}

// 0x616420
void CPopulation::ConvertAllObjectsToDummyObjects() {
    for (auto& obj : GetObjectPool()->GetAllValid()) {
        if (obj.m_nObjectType != OBJECT_GAME) {
            continue;
        }
        ConvertToDummyObject(&obj);
    }
}

// 0x616470
void CPopulation::PopulateInterior(int32 numPedsToCreate, CVector pos) {
    if (pos.z < 900.f) {
        return; // Not in interior
    }

    RemoveAllRandomPeds();
    CColStore::LoadCollision(pos, true);
    CStreaming::LoadAllRequestedModels(false);

    if (!numPedsToCreate) {
        return;
    }

    numPedsToCreate -= GeneratePedsAtAttractors(pos, 0.f, 150.f, 0.f, 150.f, 7, numPedsToCreate * 19 / 20); // 19 / 20 = 0.95
    
    for (size_t i{}; numPedsToCreate && i < 25; i++) {
        float   orientation{};
        CVector nodePos{};
        if (!ThePaths.GeneratePedCreationCoors_Interior(pos.x, pos.y, &nodePos, nullptr, nullptr, &orientation)) {
            continue;
        }

        ePedType ptype{};
        eModelID pmodel{};
        if (!CPopCycle::FindNewPedType(ptype, pmodel, true, true)) {
            continue;
        }

        pos.z += 0.9f; // NOTE/BUG: So they just keep adding it up here?

        const auto ped = AddPed(ptype, pmodel, pos, true);

        numPedsToCreate--;

        ped->GetIntelligence()->SetPedDecisionMakerType(7);

        if (ped->m_nAnimGroup == CAnimManager::GetAnimationGroupIdByName("jogger")) { // TODO: Move `GetAnimationGroupId` out the loop?
            ped->m_nAnimGroup = CAnimManager::GetAnimationGroupIdByName("man");
        }
    }
}

// 0x616650
void CPopulation::Update(bool generatePeds) {
    ZoneScoped;

    CurrentWorldZone = [] {
        switch (CWeather::WeatherRegion) {
        case WEATHER_REGION_DEFAULT:
        case WEATHER_REGION_LA:
        case WEATHER_REGION_DESERT:
            return 0;
        case WEATHER_REGION_SF:
            return 1;
        case WEATHER_REGION_LV:
            return 2;
        default:
            NOTSA_UNREACHABLE();
        }
    }();

    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    ManagePopulation();
    RemovePedsIfThePoolGetsFull();

    if (m_CountDownToPedsAtStart) {
        if (--m_CountDownToPedsAtStart == 0) {
            GeneratePedsAtStartOfGame();
        }
        return;
    }

    UpdatePedCounts();

    if (CCutsceneMgr::IsCutsceneProcessing() || !generatePeds) {
        return;
    }

    const auto pcdm = PedCreationDistMultiplier();
    const auto gdm  = TheCamera.m_fGenerationDistMultiplier;
    const float dists[]{
        pcdm * gdm * 42.5f,
        pcdm * gdm * 50.5f,
        pcdm * 25.f - 10.f,
        pcdm * 25.f
    };

    if (AddToPopulation(dists[0], dists[1], dists[2], dists[3])) {
        GeneratePedsAtAttractors(
            FindPlayerCentreOfWorld(),
            dists[0], dists[1],
            dists[2], dists[3],
            CGame::CanSeeOutSideFromCurrArea() ? -1 : 7,
            true
        );
    }
}

bool CPopulation::DoesCarGroupHaveModelId(int32 carGroupId, int32 modelId) {
    return notsa::contains(GetModelsInCarGroup(carGroupId), modelId);
}

// NOTSA
uint32 CPopulation::CalculateTotalNumGangPeds() {
    return notsa::accumulate(ms_nNumGang, 0u);
}

// NOTSA - Moved here for reuseability
void CPopulation::UpdatePedCounts() {
    ZoneScoped;

    ms_nTotalGangPeds = CalculateTotalNumGangPeds();
    ms_nTotalCivPeds = ms_nNumCivMale + ms_nNumCivFemale;
    ms_nTotalPeds = ms_nTotalCivPeds + ms_nTotalGangPeds + ms_nNumCop + ms_nNumEmergency;
}
