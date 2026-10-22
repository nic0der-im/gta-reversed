/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "Train.h"

#include "Buoyancy.h"
#include "CarCtrl.h"
#include "Pipelines/CustomBuilding/CustomBuildingDNPipeline.h"
#include "Tasks/TaskTypes/SeekEntity/TaskComplexSeekEntity.h"
#include "Tasks/TaskTypes/TaskComplexEnterCar.h"
#include "Tasks/TaskTypes/TaskComplexEnterCarAsDriver.h"
#include "Tasks/TaskTypes/TaskComplexEnterCarAsPassenger.h"
#include "Tasks/TaskTypes/TaskComplexLeaveCarAndWander.h"

CVector CTrain::aStationCoors[6] = { // 0x8D48F8
    CVector{ 1741.0f, -1954.0f, 15.0f },
    CVector{ 1297.0f, -1898.0f, 3.0f  },
    CVector{ -1945.0f, 128.0f,  29.0f },
    CVector{ 1434.0f,  2632.0f, 13.0f },
    CVector{ 2783.0f,  1758.0f, 12.0f },
    CVector{ 2865.0f,  1281.0f, 12.0  }
};

auto& pTrackNodes = StaticRef<CTrainNode*[4]>(0xC38024);
auto& NumTrackNodes = StaticRef<std::array<int32, 4>>(0xC38014);
auto& arrTotalTrackLength = StaticRef<std::array<float, 4>>(0xC37FEC);
auto& StationDist = StaticRef<std::array<float, 6>>(0xC38034);

//! Train composition per `trainType` (0x8D44F8): zero-terminated model IDs, engine first and then the carriages.
//! Every entry is 0x40 bytes long, of which only the leading model IDs are used.
auto& GenTrainConfigs = StaticRef<std::array<std::array<int32, 16>, 10>>(0x8D44F8);

//! Last `GenTrain_TrainConfig` that was used (address 0xC38064, unnamed in the original)
auto& GenTrain_LastConfig = StaticRef<uint32>(0xC38064);

//! Set when the player was within 60m of a station during the last (3 second) check (address 0xC38068, unnamed in the original)
auto& GenTrain_PlayerNearStation = StaticRef<bool>(0xC38068);

void CTrain::InjectHooks() {
    RH_ScopedVirtualClass(CTrain, 0x872370, 66);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6F6030);
    RH_ScopedInstall(InitTrains, 0x6F7440);
    RH_ScopedInstall(ReadAndInterpretTrackFile, 0x6F55D0);
    RH_ScopedInstall(Shutdown, 0x6F58D0);
    RH_ScopedInstall(UpdateTrains, 0x6F5900);
    RH_ScopedInstall(FindCoorsFromPositionOnTrack, 0x6F59E0);
    RH_ScopedInstall(FindMaximumSpeedToStopAtStations, 0x6F5BA0);
    RH_ScopedInstall(FindNumCarriagesPulled, 0x6F5CD0);
    RH_ScopedInstall(OpenTrainDoor, 0x6F5D80);
    RH_ScopedInstall(AddPassenger, 0x6F5D90);
    RH_ScopedInstall(RemovePassenger, 0x6F5DA0);
    RH_ScopedInstall(DisableRandomTrains, 0x6F5DB0);
    RH_ScopedInstall(RemoveOneMissionTrain, 0x6F5DC0);
    RH_ScopedInstall(ReleaseOneMissionTrain, 0x6F5DF0);
    RH_ScopedInstall(SetTrainSpeed, 0x6F5E20);
    RH_ScopedInstall(SetTrainCruiseSpeed, 0x6F5E50);
    RH_ScopedInstall(FindCaboose, 0x6F5E70);
    RH_ScopedInstall(FindEngine, 0x6F5E90);
    RH_ScopedInstall(FindCarriage, 0x6F5EB0);
    RH_ScopedInstall(FindSideStationIsOn, 0x6F5EF0);
    RH_ScopedInstall(FindNextStationPositionInDirection, 0x6F5F00);
    RH_ScopedInstall(IsInTunnel, 0x6F6320);
    RH_ScopedInstall(RemoveRandomPassenger, 0x6F6850);
    RH_ScopedInstall(RemoveMissionTrains, 0x6F6A20);
    RH_ScopedInstall(RemoveAllTrains, 0x6F6AA0);
    RH_ScopedInstall(ReleaseMissionTrains, 0x6F6B60);
    RH_ScopedInstall(FindClosestTrackNode, 0x6F6BD0);
    RH_ScopedInstall(FindPositionOnTrackFromCoors, 0x6F6CC0);
    RH_ScopedInstall(FindNearestTrain, 0x6F7090);
    RH_ScopedInstall(SetNewTrainPosition, 0x6F7140);
    RH_ScopedInstall(IsNextStationAllowed, 0x6F7260);
    RH_ScopedInstall(SkipToNextAllowedStation, 0x6F72F0);
    RH_ScopedInstall(CreateMissionTrain, 0x6F7550);
    RH_ScopedInstall(DoTrainGenerationAndRemoval, 0x6F7900);
    RH_ScopedInstall(AddNearbyPedAsRandomPassenger, 0x6F8170);
    RH_ScopedVMTInstall(SetModelIndex, 0x6F5530);
    RH_ScopedVMTInstall(ProcessControl, 0x6F86A0);

    RH_ScopedGlobalInstall(ProcessTrainAnnouncements, 0x6F5910);
    RH_ScopedGlobalInstall(PlayAnnouncement, 0x6F5920);
    RH_ScopedGlobalInstall(MarkSurroundingEntitiesForCollisionWithTrain, 0x6F6640);
    RH_ScopedGlobalInstall(TrainHitStuff<CPtrListSingleLink<CPhysical*>>, 0x6F5CF0);
}

// 0x6F6030
CTrain::CTrain(int32 modelIndex, eVehicleCreatedBy createdBy) : CVehicle(createdBy) {
    m_nVehicleSubType = VEHICLE_TYPE_TRAIN;
    m_nVehicleType    = VEHICLE_TYPE_TRAIN;

    const auto mi = CModelInfo::GetModelInfo(modelIndex)->AsVehicleModelInfoPtr();
    m_pHandlingData           = gHandlingDataMgr.GetVehiclePointer(mi->m_nHandlingId);
    m_nHandlingFlagsIntValue  = m_pHandlingData->m_nHandlingFlags;

    CVehicle::SetModelIndex(modelIndex);
    SetupModelNodes();

    m_aDoors = {}; // Zeroed by the original (both doors are fully set up below anyway)

    if (m_nModelIndex == MODEL_STREAKC) {
        m_aDoors[DOOR_LEFT_FRONT].Init(1.25f, 0.25f, DOOR_AXIS_NEG_Y, DOOR_AXIS_Z, DOOR_EXTRA_BASED);
        m_aDoors[DOOR_RIGHT_FRONT].Init(1.25f, 0.25f, DOOR_AXIS_NEG_Y, DOOR_AXIS_Z, DOOR_EXTRA_BASED);
    } else {
        m_aDoors[DOOR_LEFT_FRONT].Init(TWO_PI / -5.0f, 0.0f, DOOR_AXIS_NEG_Y, DOOR_AXIS_Z, DOOR_EXTRA_BASED);
        m_aDoors[DOOR_RIGHT_FRONT].Init(TWO_PI / +5.0f, 0.0f, DOOR_AXIS_NEG_Y, DOOR_AXIS_Z, DOOR_EXTRA_BASED);
    }

    // Only these flags are (re)written by the ctor - `bIsFrontCarriage`/`bIsLastCarriage`/`bClockwiseDirection`
    // are set up by `CreateMissionTrain`
    trainFlags.b01                       = true;
    trainFlags.bStoppedAtStation         = false;
    trainFlags.bPassengersCanEnterAndLeave = false;
    trainFlags.bMissionTrain             = false;
    trainFlags.bStopsAtStations          = true;
    trainFlags.bForceSlowDown            = true;

    m_nPassengersGenerationState = 0;
    m_nNumPassengersToLeave      = CGeneral::GetRandomNumber() & 3;
    m_nNumPassengersToEnter      = (CGeneral::GetRandomNumber() & 3) + 1;
    m_pTemporaryPassenger        = nullptr;
    m_nMaxPassengers             = 5;

    physicalFlags.bDisableSimpleCollision = true;
    SetUsesCollision(true);

    m_nTimeWhenCreated = CTimer::GetTimeInMS();
    field_5C8          = 0;
    m_nTrackId         = 0;
    m_fCurrentRailDistance       = 0.0f;
    m_fTrainSpeed                = 0.0f;
    m_nTimeWhenStoppedAtStation  = 0;

    mi->ChooseVehicleColour(m_nPrimaryColor, m_nSecondaryColor, m_nTertiaryColor, m_nQuaternaryColor, 1);

    m_fMass          = m_pHandlingData->m_fMass;
    m_fTurnMass      = m_pHandlingData->m_fTurnMass;
    m_vecCentreOfMass = m_pHandlingData->m_vecCentreOfMass;
    m_fElasticity    = 0.05f;
    m_fBuoyancyConstant = m_pHandlingData->m_fBuoyancyConstant;
    m_fAirResistance = GetDefaultAirResistance();

    physicalFlags.bCollidable           = false;
    physicalFlags.bDisableCollisionForce = true;
    m_bTunnelTransition                 = true;
    m_pPrevCarriage                     = nullptr;
    m_pNextCarriage                     = nullptr;
    SetStatus(STATUS_TRAIN_MOVING);
    m_autoPilot.m_speed = 0.0f;
    m_autoPilot.SetCruiseSpeed(0);
    m_vehicleAudio.Initialise(this);
}

// 0x6F5530
void CTrain::SetModelIndex(uint32 index) {
    CVehicle::SetModelIndex(index);
    SetupModelNodes();
}

void CTrain::SetupModelNodes() {
    std::ranges::fill(m_aTrainNodes, nullptr);
    CClumpModelInfo::FillFrameArray(GetRpClump(), m_aTrainNodes.data());
}

// 0x6F7440
void CTrain::InitTrains() {
    ZoneScoped;

    bDisableRandomTrains = false;
    GenTrain_Status      = 0;

    constexpr const char* filenames[] {
        "data\\paths\\tracks.dat",
        "data\\paths\\tracks3.dat",
        "data\\paths\\tracks2.dat",
        "data\\paths\\tracks4.dat",
    };
    for (auto i = 0u; i < std::size(pTrackNodes); ++i) {
        if (!pTrackNodes[i]) {
            ReadAndInterpretTrackFile(filenames[i], &pTrackNodes[i], &NumTrackNodes[i], &arrTotalTrackLength[i], (int32)i);
        }
    }

    // NOTE: The node index is always taken from track 0 - the track the closest node belongs to is ignored.
    for (auto i = 0u; i < std::size(aStationCoors); ++i) {
        int32 trackId;
        const auto nodeIdx = FindClosestTrackNode(aStationCoors[i], &trackId);
        StationDist[i] = pTrackNodes[0][nodeIdx].GetDistanceFromStart();
    }
}

// 0x6F55D0
void CTrain::ReadAndInterpretTrackFile(const char* filename, CTrainNode** nodes, int32* lineCount, float* totalDist, int32 skipStations) {
    if (!*nodes) {
        auto* const fileContents = new char[0xB530];
        CFileMgr::LoadFile(filename, reinterpret_cast<uint8*>(fileContents), 0xB530, "rb");

        int32 pos = 0;
        const auto ReadLine = [&] {
            int32 len = 0;
            while (fileContents[pos] != '\n') {
                gString[len++] = fileContents[pos++];
            }
            gString[len] = '\0';
            ++pos;
        };

        ReadLine();
        if (std::strncmp(gString, "processed", 10) == 0) { // Already pre-processed file: the node count is on the second line
            ReadLine();
        }

        std::sscanf(gString, "%d", lineCount);

        *nodes = new CTrainNode[*lineCount];

        int32 numStations = 0;
        for (int32 i = 0; i < *lineCount; ++i) {
            ReadLine();

            float x, y, z;
            int32 isStation{};
            std::sscanf(gString, "%f %f %f %d", &x, &y, &z, &isStation);

            (*nodes)[i].SetX(x);
            (*nodes)[i].SetY(y);
            (*nodes)[i].SetZ(z);

            if (!skipStations && isStation) {
                aStationCoors[numStations++] = CVector{ x, y, z };
            }
        }

        delete[] fileContents;
    }

    if (*lineCount <= 0) {
        *totalDist = 0.0f;
        return;
    }

    // Compute the distance from the start of the track for every node (Z is ignored)
    float accumulatedDist = 0.0f;
    for (int32 i = 0; i < *lineCount; ++i) {
        (*nodes)[i].SetDistanceFromStart(accumulatedDist);

        const auto& node = (*nodes)[i];
        const auto& next = (*nodes)[(i + 1) % *lineCount];
        accumulatedDist += (node.GetPosn() - next.GetPosn()).Magnitude2D();
    }
    *totalDist = accumulatedDist;
}

// 0x6F58D0
void CTrain::Shutdown() {
    for (auto node : pTrackNodes) {
        delete node;
        node = nullptr;
    }
}

// 0x6F5900
void CTrain::UpdateTrains() {
    ZoneScoped;

    // NOP
}

// 0x6F5910
void ProcessTrainAnnouncements() {
    // NOP
}

// 0x6F5920
void PlayAnnouncement(uint8 arg0, uint8 arg1) {
    // NOP
}

// 0x6F59E0
void CTrain::FindCoorsFromPositionOnTrack(float railDistance, int32 trackId, CVector* outCoors) {
    const auto numNodes = NumTrackNodes[trackId];
    if (numNodes <= 0) {
        return;
    }

    const auto nodes = pTrackNodes[trackId];
    for (int32 i = 0;; ++i) {
        const int32 nextIdx = (i + 1) % numNodes;

        const auto distBehind = railDistance - nodes[i].GetDistanceFromStart();
        const auto distAhead  = nodes[nextIdx].GetDistanceFromStart() - railDistance;
        if (distBehind >= 0.0f && distAhead >= 0.0f) {
            const auto invDist = 1.0f / (distAhead + distBehind);
            *outCoors = (nodes[i].GetPosn() * distAhead + nodes[nextIdx].GetPosn() * distBehind) * invDist;
            return;
        }

        if (i + 1 >= numNodes) { // Distance not on this track - leave `outCoors` untouched
            return;
        }
    }
}

// 0x6F5BA0
bool CTrain::FindMaximumSpeedToStopAtStations(float* speed) {
    *speed = 50.0f;
    if (m_nTrackId != 0) { // Only track 0 has stations
        return false;
    }

    const auto trackLength     = arrTotalTrackLength[0];
    const auto halfTrackLength = trackLength * 0.5f;
    const auto clockwise       = trainFlags.bClockwiseDirection;

    float minDist = 10000.0f;
    for (const auto stationDist : StationDist) {
        float dist = stationDist - m_fCurrentRailDistance;
        dist += clockwise ? 40.0f : -40.0f;

        while (dist > halfTrackLength) {
            dist -= trackLength;
        }
        while (dist < -halfTrackLength) {
            dist += trackLength;
        }

        // Only consider stations ahead of us
        if (clockwise ? dist > 0.0f : dist < 0.0f) {
            dist = std::abs(dist);
            if (dist < minDist) {
                minDist = dist;
            }
        }
    }

    if (minDist >= 500.0f) {
        *speed = 100'000.0f;
    } else {
        *speed = (1.0f - (500.0f - minDist) * 0.002f) * 50.0f;
    }

    return minDist < 5.0f;
}

// 0x6F5CD0
uint32 CTrain::FindNumCarriagesPulled() {
    uint32 num;
    CTrain* carriage = m_pNextCarriage;
    for (num = 0; carriage; ++num) {
        carriage = carriage->m_pNextCarriage;
    }
    return num;
}

// 0x6F5D80
void CTrain::OpenTrainDoor(float state) {
    // NOP
}

// 0x6F5D90
void CTrain::AddPassenger(CPed* ped) {
    // NOP
}

// 0x6F5DA0
void CTrain::RemovePassenger(CPed* ped) {
    // NOP
}

// 0x6F5DB0
void CTrain::DisableRandomTrains(bool disable) {
    bDisableRandomTrains = disable;
}

// 0x6F5DC0
void CTrain::RemoveOneMissionTrain(CTrain* train) {
    if (!train)
        return;

    CTrain* next;
    CTrain* _train = train;
    do {
        next = _train->m_pNextCarriage;
        CWorld::Remove(_train);
        delete _train;
        _train = next;
    } while (next);
}

// 0x6F5DF0
void CTrain::ReleaseOneMissionTrain(CTrain* train) {
    for (auto* head = train; head; head = head->m_pNextCarriage) {
        head->trainFlags.bMissionTrain = false;
    }
}

// 0x6F5E20
void CTrain::SetTrainSpeed(CTrain* train, float speed) {
    train->m_fTrainSpeed = speed / 50.0f;
    if (!train->trainFlags.bClockwiseDirection) {
        train->m_fTrainSpeed = -train->m_fTrainSpeed;
    }
}

// 0x6F5E50
void CTrain::SetTrainCruiseSpeed(CTrain* train, float speed) {
    train->m_autoPilot.SetCruiseSpeed((uint8)speed);
}

// 0x6F5E70
CTrain* CTrain::FindCaboose(CTrain* train) {
    CTrain* carriage = train->m_pNextCarriage;
    if (!carriage) {
        return train;
    }
    while (carriage->m_pNextCarriage) {
        carriage = carriage->m_pNextCarriage;
    }
    return carriage;
}

// 0x6F5E90
CTrain* CTrain::FindEngine(CTrain* train) {
    CTrain* carriage = train->m_pPrevCarriage;
    if (!carriage) {
        return train;
    }
    while (carriage->m_pPrevCarriage) {
        carriage = carriage->m_pPrevCarriage;
    }
    return carriage;
}

// 0x6F5EB0
CTrain* CTrain::FindCarriage(CTrain* train, uint8 carriage) {
    if (!carriage) {
        return train;
    }
    for (uint8 i = 0; i < carriage; ++i) {
        train = train->m_pNextCarriage;
        if (!train) {
            return nullptr;
        }
    }
    return train;
}

// 0x6F5EF0
bool CTrain::FindSideStationIsOn() const {
    return trainFlags.bClockwiseDirection; // ?
}

// 0x6F5F00
void CTrain::FindNextStationPositionInDirection(bool clockwiseDirection, float distance, float* distanceToStation, int32* numStations) {
    int32 stationIdx = 0;
    while (stationIdx < (int32)std::size(StationDist) && distance >= StationDist[stationIdx]) {
        ++stationIdx;
    }
    if (stationIdx >= (int32)std::size(StationDist)) {
        stationIdx = 0;
    }

    int32 nextStation = stationIdx;
    if (!clockwiseDirection) {
        nextStation = stationIdx - 1;
        if (nextStation < 0) {
            nextStation = stationIdx + 5;
        }
    }

    // If we're already at that station, skip to the next one in the travel direction
    if (std::abs(distance - StationDist[nextStation]) < 100.0f) {
        nextStation += clockwiseDirection ? 1 : -1;
        if (nextStation < 0) {
            nextStation += 6;
        }
        if (nextStation > 5) {
            nextStation = 0;
        }
    }

    *numStations       = nextStation;
    *distanceToStation = StationDist[nextStation];
}

// 0x6F6320
bool CTrain::IsInTunnel() const {
    const auto& pos = GetPosition();
    CColPoint colPoint{};
    CEntity* colEntity{};
    return CWorld::ProcessVerticalLine(pos, pos.z + 100.0f, colPoint, colEntity, true);
}

// 0x6F6640
void MarkSurroundingEntitiesForCollisionWithTrain(CVector pos, float radius, CEntity* entity, bool bOnlyVehicles) {
    int32 startSectorX = std::max(CWorld::GetSectorX(pos.x - radius), 0);
    int32 startSectorY = std::max(CWorld::GetSectorY(pos.y - radius), 0);
    int32 endSectorX   = std::min(CWorld::GetSectorX(pos.x + radius), MAX_SECTORS_X - 1);
    int32 endSectorY   = std::min(CWorld::GetSectorY(pos.y + radius), MAX_SECTORS_Y - 1);

    CWorld::AdvanceCurrentScanCode();

    for (int32 sectorY = startSectorY; sectorY <= endSectorY; ++sectorY) {
        for (int32 sectorX = startSectorX; sectorX <= endSectorX; ++sectorX) {
            auto& repeatSector = CWorld::GetRepeatSector(sectorX, sectorY);
            TrainHitStuff(repeatSector.Vehicles, entity);
            if (!bOnlyVehicles) {
                TrainHitStuff(repeatSector.Peds, entity);
                TrainHitStuff(repeatSector.Objects, entity);
            }
        }
    }
}

// 0x6F5CF0
template<typename PtrListType>
void TrainHitStuff(PtrListType& ptrList, CEntity* entity) {
    for (CPhysical* const physical : ptrList) {
        if (physical == entity) {
            continue;
        }

        physical->physicalFlags.bProcessCollisionEvenIfStationary = true;

        if (physical->GetType() != ENTITY_TYPE_OBJECT || !physical->GetIsStatic()) {
            continue;
        }

        if (physical->m_nModelIndex != ModelIndices::MI_OBJECTFORMAGNOCRANE1
            && physical->m_nModelIndex != ModelIndices::MI_OBJECTFORMAGNOCRANE2
            && physical->m_nModelIndex != ModelIndices::MI_OBJECTFORMAGNOCRANE3
        ) {
            continue;
        }

        // Wake the crane object up so the train can collide with it
        physical->SetIsStatic(false);
        physical->AddToMovingList();
        physical->m_nFakePhysics = 0;
    }
}

// 0x6F6850
void CTrain::RemoveRandomPassenger() {
    if (CReplay::Mode == MODE_PLAYBACK) {
        return;
    }

    const auto player = FindPlayerPed();
    if (player->m_pVehicle != this) {
        return;
    }

    // Don't interfere with the player while he's entering the train
    if (const auto task = player->GetTaskManager().GetTaskPrimary(TASK_PRIMARY_DEFAULT)) {
        const auto taskType = task->GetTaskType();
        if ((taskType == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER || taskType == TASK_COMPLEX_ENTER_CAR_AS_DRIVER)
            && static_cast<CTaskComplexEnterCar*>(task)->GetTargetCar() == this
        ) {
            return;
        }
    }

    const auto clockwise = trainFlags.bClockwiseDirection;
    if (player->bInVehicle && player->m_pVehicle == this) {
        if (clockwise ? m_pDriver == player : m_pDriver != player) {
            return;
        }
    }

    if (m_nNumPassengersToLeave == 0) {
        return;
    }

    if (m_pTemporaryPassenger) {
        if (m_pTemporaryPassenger->bInVehicle) {
            return;
        }
        CEntity::ClearReference(m_pTemporaryPassenger);
    }

    int32 carRating = CGeneral::GetRandomNumberInRange(0, 12);
    if (carRating == 12) {
        carRating = 11;
    }
    if ((CGeneral::GetRandomNumber() & 0xFFFF) < 100) {
        carRating = 25;
    }

    auto* const passenger = CPopulation::AddPedInCar(this, !clockwise, carRating, 0, false, false);
    if (!passenger) {
        return;
    }

    passenger->bJustGotOffTrain = true;
    --m_nNumPassengersToLeave;
    m_pTemporaryPassenger = passenger;
    CEntity::RegisterReference(m_pTemporaryPassenger);

    passenger->GetTaskManager().SetTask(
        new CTaskComplexLeaveCarAndWander(this, TARGET_DOOR_FRONT_LEFT, 0, true),
        TASK_PRIMARY_PRIMARY,
        false
    );
}

// 0x6F6A20
void CTrain::RemoveMissionTrains() {
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (vehicle.IsTrain() &&
            &vehicle != FindPlayerVehicle() &&
            vehicle.AsTrain()->trainFlags.bMissionTrain
        ) {
            CWorld::Remove(&vehicle);
            delete &vehicle;
        }
    }
}

// 0x6F6AA0
void CTrain::RemoveAllTrains() {
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (!vehicle.IsTrain()) {
            continue;
        }

        auto* const train = vehicle.AsTrain();

        // The player's train (or a part of it) is never removed
        bool isPlayers = false;
        for (CTrain* carriage = train; carriage; carriage = carriage->m_pPrevCarriage) {
            if (carriage == FindPlayerVehicle()) {
                isPlayers = true;
            }
        }
        for (CTrain* carriage = train; carriage; carriage = carriage->m_pNextCarriage) {
            if (carriage == FindPlayerVehicle()) {
                isPlayers = true;
            }
        }

        if (!isPlayers) {
            CWorld::Remove(train);
            delete train;
        }
    }
}

// 0x6F6B60
void CTrain::ReleaseMissionTrains() {
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (vehicle.IsTrain() && &vehicle != FindPlayerVehicle()) {
            vehicle.AsTrain()->trainFlags.bMissionTrain = false;
        }
    }
}

// 0x6F6BD0
int32 CTrain::FindClosestTrackNode(CVector posn, int32* outTrackId) {
    float minDist = 99999.9f;
    // NOTE: Uninitialised in the original (it's only ever read when at least one track was loaded)
    int32 closestNode{};

    for (int32 trackId = 0; trackId < (int32)std::size(pTrackNodes); ++trackId) {
        for (int32 nodeIdx = 0; nodeIdx < NumTrackNodes[trackId]; ++nodeIdx) {
            const auto dist = (posn - pTrackNodes[trackId][nodeIdx].GetPosn()).Magnitude();
            if (dist < minDist) {
                *outTrackId = trackId;
                closestNode = nodeIdx;
                minDist     = dist;
            }
        }
    }

    return closestNode;
}

// 0x6F6CC0
void CTrain::FindPositionOnTrackFromCoors() {
    const auto trackId  = (uint8)m_nTrackId;
    const auto numNodes = NumTrackNodes[trackId];
    if (numNodes < 1) {
        return;
    }

    const auto nodes = pTrackNodes[trackId];
    const auto pos   = GetPosition();

    for (int32 i = 0; i < numNodes; ++i) {
        const int32 nextIdx = (i + 1) % numNodes;

        const auto p0 = nodes[i].GetPosn();
        const auto p1 = nodes[nextIdx].GetPosn();

        // Project the position on the (2D) segment between the two nodes
        const auto segLen2 = (p0.x - p1.x) * (p0.x - p1.x) + (p0.y - p1.y) * (p0.y - p1.y);
        const auto t       = ((pos.x - p0.x) * (p1.x - p0.x) + (pos.y - p0.y) * (p1.y - p0.y)) / segLen2;
        if (t <= 0.001f || t >= 1.001f) {
            continue;
        }

        const auto distX = (p1.x - p0.x) * t + p0.x - pos.x;
        const auto distY = (p1.y - p0.y) * t + p0.y - pos.y;
        if (std::sqrt(distX * distX + distY * distY) >= 3.0f) {
            // Too far from the track to snap to it - only interpolate the surface brightness
            const auto balance = CCustomBuildingDNPipeline::m_fDNBalanceParam;
            const auto SampleLighting = [balance](tColLighting lighting) {
                return lighting.night * (1.0f / 30.0f) * balance
                     + (1.0f - balance) * lighting.day * (1.0f / 30.0f);
            };
            const auto lighting0 = SampleLighting(nodes[i].GetLightingFromCollision());
            const auto lighting1 = SampleLighting(nodes[nextIdx].GetLightingFromCollision());

            m_fContactSurfaceBrightness = (lighting1 - lighting0) * t + lighting0;
            continue;
        }

        // Snap to the track
        float railDist = (nodes[nextIdx].GetDistanceFromStart() - nodes[i].GetDistanceFromStart()) * t + nodes[i].GetDistanceFromStart();
        m_fCurrentRailDistance = railDist;

        const auto* const colModel = CModelInfo::GetModelInfo(m_nModelIndex)->GetColModel();
        const auto&       bbox     = colModel->GetBoundingBox();
        railDist -= (bbox.m_vecMax.y - bbox.m_vecMin.y) * 0.5f;
        m_fCurrentRailDistance = railDist;

        if (railDist < 0.0f) {
            m_fCurrentRailDistance = railDist + arrTotalTrackLength[trackId];
        }

        const auto speed = m_vecMoveSpeed.Magnitude();
        m_fTrainSpeed = speed;

        const auto forward = (p1.x - p0.x) * m_vecMoveSpeed.x + (p1.y - p0.y) * m_vecMoveSpeed.y > 0.0f;
        if (trainFlags.bClockwiseDirection == forward) {
            m_fTrainSpeed = -speed;
        }
        return;
    }
}

// 0x6F7090
CTrain* CTrain::FindNearestTrain(CVector posn, bool mustBeMainTrain) {
    CTrain* nearestTrain = nullptr;
    float   nearestDist  = 1.0e7f;

    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (!vehicle.IsTrain()) {
            continue;
        }

        auto* const train = vehicle.AsTrain();
        const auto& pos   = train->GetPosition();

        // 2D distance - the Z coordinate is ignored
        const auto dist = std::sqrt((pos.x - posn.x) * (pos.x - posn.x) + (pos.y - posn.y) * (pos.y - posn.y));
        if (dist < nearestDist && (!mustBeMainTrain || train->trainFlags.bIsFrontCarriage)) {
            nearestTrain = train;
            nearestDist  = dist;
        }
    }

    return nearestTrain;
}

// 0x6F7140
void CTrain::SetNewTrainPosition(CTrain* train, CVector posn) {
    train->SetPosn(posn);
    train->FindPositionOnTrackFromCoors();
}

// 0x6F7260
bool CTrain::IsNextStationAllowed(CTrain* train) {
    auto* const engine = FindEngine(train);

    float railDist;
    int32 stationIdx;
    FindNextStationPositionInDirection(engine->trainFlags.bClockwiseDirection, engine->m_fCurrentRailDistance, &railDist, &stationIdx);

    const auto level = CTheZones::GetLevelFromPosition(aStationCoors[stationIdx]);
    return CStats::GetStatValue(STAT_CITY_UNLOCKED) + 1.0f >= (float)level;
}

// 0x6F72F0
void CTrain::SkipToNextAllowedStation(CTrain* train) {
    auto* const engine = FindEngine(train);

    float railDist = engine->m_fCurrentRailDistance;
    int32 stationIdx;
    do {
        FindNextStationPositionInDirection(engine->trainFlags.bClockwiseDirection, railDist, &railDist, &stationIdx);
        const auto level = CTheZones::GetLevelFromPosition(aStationCoors[stationIdx]);
        if (CStats::GetStatValue(STAT_CITY_UNLOCKED) + 1.0f >= (float)level) {
            break;
        }
    } while (true);

    if (engine->trainFlags.bClockwiseDirection) {
        railDist -= 20.0f;
        engine->m_fTrainSpeed = 0.1f;
    } else {
        railDist += 20.0f;
        engine->m_fTrainSpeed = -0.1f;
    }
    engine->m_fCurrentRailDistance = railDist;

    CStreaming::LoadScene(aStationCoors[stationIdx]);
    CStreaming::LoadAllRequestedModels(false);

    // Advance the game time by (roughly) the time it takes the train to reach that station
    const auto& trainPos = engine->GetPosition();
    const auto  stationPos = aStationCoors[stationIdx];
    CGameLogic::PassTime(
        (uint32)(
            std::sqrt((stationPos.x - trainPos.x) * (stationPos.x - trainPos.x)
                    + (stationPos.y - trainPos.y) * (stationPos.y - trainPos.y)) * 0.05f + 23.0f
        )
    );
}

// 0x6F7550
void CTrain::CreateMissionTrain(CVector posn, bool clockwiseDirection, uint32 trainType, CTrain** outFirstCarriage, CTrain** outLastCarriage, int32 nodeIndex, int32 trackId, bool isMissionTrain) {
    if (nodeIndex < 0) {
        nodeIndex = FindClosestTrackNode(posn, &trackId);
    }

    const auto& config = GenTrainConfigs[trainType];

    float railDist = pTrackNodes[trackId][nodeIndex].GetDistanceFromStart();

    const auto GetCarriageLength = [](const CTrain* train) {
        const auto& bbox = CModelInfo::GetModelInfo(train->m_nModelIndex)->GetColModel()->GetBoundingBox();
        return bbox.m_vecMax.y - bbox.m_vecMin.y;
    };

    std::array<CTrain*, 16> created{};
    size_t       numCreated{};
    CTrain*      prev{};
    for (const auto modelId : config) {
        if (!modelId) {
            break;
        }

        auto* const train = new CTrain(modelId, PERMANENT_VEHICLE);
        train->GetMatrix().SetTranslate(CVector{ 0.0f, 0.0f, 0.0f });
        train->m_nNodeIndex = (int16)nodeIndex;
        train->SetStatus(STATUS_ABANDONED);
        train->vehicleFlags.bIsLocked = true; // (0x428 |= 0x08)
        train->m_fCurrentRailDistance = railDist;
        train->trainFlags.bMissionTrain      = isMissionTrain;
        train->trainFlags.bClockwiseDirection = clockwiseDirection;
        train->m_nTrackId = (int8)trackId;

        if (isMissionTrain) {
            train->trainFlags.bStopsAtStations = false;

            if (!prev) {
                train->GetPosition() = posn;
                train->FindPositionOnTrackFromCoors();
                train->m_fLength = 0.0f;
            } else if (!clockwiseDirection) {
                const auto len = GetCarriageLength(prev);
                train->m_fLength = len;
                railDist += len;
            } else {
                const auto len = GetCarriageLength(train);
                train->m_fLength = -len;
                railDist -= len;
            }
        } else if (!prev) {
            train->m_fLength = 0.0f;
        } else if (!clockwiseDirection) {
            const auto len = GetCarriageLength(prev);
            train->m_fLength = len;
            railDist += len;
        } else {
            const auto len = GetCarriageLength(train);
            train->m_fLength = -len;
            railDist -= len;
        }

        created[numCreated++] = train;

        if (!prev) {
            train->trainFlags.bIsFrontCarriage = true;
            if (outFirstCarriage) {
                *outFirstCarriage = train;
            }
        } else {
            train->trainFlags.bIsFrontCarriage = false;
            train->vehicleFlags.bFadeOut       = true; // (0x42A |= 0x02)
            prev->trainFlags.bIsLastCarriage   = false;
        }

        train->trainFlags.bIsLastCarriage = true;
        if (outLastCarriage) {
            *outLastCarriage = train;
        }

        train->m_pPrevCarriage = prev;
        if (prev) {
            CEntity::RegisterReference(train->m_pPrevCarriage);
        }
        train->m_pNextCarriage = nullptr;
        if (prev) {
            prev->m_pNextCarriage = train;
            CEntity::RegisterReference(prev->m_pNextCarriage);
        }

        train->ProcessControl();

        prev = train;
    }

    if (!numCreated) {
        // TODO(re): The original dereferences the (never assigned) out-first-carriage pointer here;
        //           none of the train configs in the table is empty, so this is unreachable.
        return;
    }

    // Re-insert every carriage (in reverse order) so the world sees their final positions
    for (auto i = (int32)numCreated; i-- > 0;) {
        CWorld::Remove(created[i]);
        CWorld::Add(created[i]);
    }

    auto* const firstCarriage = created[0];

    if (firstCarriage->m_nModelIndex != MODEL_TRAM) {
        CPopulation::AddPedInCar(firstCarriage, true, -1, 0, false, false);
        if (firstCarriage->m_pDriver) {
            firstCarriage->m_pDriver->GetIntelligence()->SetPedDecisionMakerType(6);
        }
    }

    // The whole train is a "streak" train as soon as one of its carriages is a streak
    bool isStreak = false;
    for (CTrain* carriage = firstCarriage; carriage; carriage = carriage->m_pNextCarriage) {
        if (carriage->m_nModelIndex == MODEL_STREAK) {
            isStreak = true;
            break;
        }
    }
    for (CTrain* carriage = firstCarriage; carriage; carriage = carriage->m_pNextCarriage) {
        carriage->trainFlags.bIsStreakModel = isStreak;
    }
}

// 0x6F7900
void CTrain::DoTrainGenerationAndRemoval() {
    // Refresh whether the player is near one of the stations (only every 3 seconds)
    if (CTimer::GetTimeInMS() / 3000 != CTimer::GetPreviousTimeInMS() / 3000) {
        GenTrain_PlayerNearStation = false;
        for (const auto& stationCoors : aStationCoors) {
            const auto playerCoors = FindPlayerCoors();
            const auto dist        = std::sqrt((stationCoors.y - playerCoors.y) * (stationCoors.y - playerCoors.y)
                                             + (stationCoors.x - playerCoors.x) * (stationCoors.x - playerCoors.x));
            if (dist < 60.0f) {
                GenTrain_PlayerNearStation = true;
            }
        }
    }

    const auto interval = GenTrain_PlayerNearStation ? 1u : 950u; // Check more often when near a station
    const auto minDist  = GenTrain_PlayerNearStation ? 100.0f : 70.0f;

    if (CTimer::GetTimeInMS() / interval == CTimer::GetPreviousTimeInMS() / interval) {
        return;
    }

    // Remove ambient (non-mission) trains that ended up next to the player
    bool anyRandomTrain = false;
    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (vehicle.m_nVehicleSubType != VEHICLE_TYPE_TRAIN) {
            continue;
        }

        auto* const frontCarriage = vehicle.AsTrain();
        if (!frontCarriage->trainFlags.bIsFrontCarriage || frontCarriage->trainFlags.bMissionTrain) {
            continue;
        }

        anyRandomTrain = true;

        const auto& cameraPos = TheCamera.GetPosition();
        bool        removeTrain = true;
        for (CTrain* carriage = frontCarriage; carriage; carriage = carriage->m_pNextCarriage) {
            const auto& trainPos = carriage->GetPosition();
            const auto  dist     = std::sqrt((trainPos.y - cameraPos.y) * (trainPos.y - cameraPos.y)
                                           + (trainPos.x - cameraPos.x) * (trainPos.x - cameraPos.x));
            if (carriage == FindPlayerVehicle(-1, false) || dist < 220.0f) {
                removeTrain = false;
            }
        }

        if (removeTrain) {
            for (CTrain* carriage = frontCarriage; carriage;) {
                auto* const next = carriage->m_pNextCarriage;
                CWorld::Remove(carriage);
                delete carriage;
                carriage = next;
            }
        }
    }

    if (bDisableRandomTrains) {
        return;
    }

    if (GenTrain_Status != 0) {
        if (GenTrain_Status != 1) {
            return;
        }

        // Wait until every model of the selected configuration is streamed in
        const auto& config = GenTrainConfigs[GenTrain_TrainConfig];
        bool        allLoaded = true;
        for (const auto modelId : config) {
            if (!modelId) {
                break;
            }
            if (CStreaming::ms_aInfoForModel[modelId].m_LoadState != LOADSTATE_LOADED) {
                CStreaming::RequestModel(modelId, 8);
                allLoaded = false;
            }
        }
        if (!allLoaded) {
            return;
        }

        const auto& cameraPos = TheCamera.GetPosition();
        const auto& node      = pTrackNodes[GenTrain_Track][GenTrain_GenerationNode];
        const auto  nodePos   = node.GetPosn();
        const auto  nodeDist  = std::sqrt((nodePos.y - cameraPos.y) * (nodePos.y - cameraPos.y)
                                        + (nodePos.x - cameraPos.x) * (nodePos.x - cameraPos.x));
        if (nodeDist > 60.0f) {
            CTrain* firstCarriage{};
            CreateMissionTrain(
                { 0.0f, 0.0f, 0.0f },
                GenTrain_Direction,
                GenTrain_TrainConfig,
                &firstCarriage,
                nullptr,
                GenTrain_GenerationNode,
                GenTrain_Track,
                false
            );

            int32 speed;
            if (GenTrain_Track == 0) {
                speed = CGeneral::GetRandomNumber() % 30 + 15;
                const auto distToStation = std::sqrt((node.GetX() - 2222.0f) * (node.GetX() - 2222.0f)
                                                   + (node.GetY() - -1750.0f) * (node.GetY() - -1750.0f));
                if (distToStation < 300.0f) {
                    speed /= 2;
                }
            } else {
                speed = CGeneral::GetRandomNumber() % 7 + 7;
            }

            float maxSpeed;
            firstCarriage->FindMaximumSpeedToStopAtStations(&maxSpeed);
            if ((float)speed < maxSpeed) {
                maxSpeed = (float)speed;
            }

            SetTrainSpeed(firstCarriage, maxSpeed);
            firstCarriage->m_autoPilot.m_nCruiseSpeed = (uint8)speed;
        }

        for (const auto modelId : config) {
            if (!modelId) {
                break;
            }
            CStreaming::SetModelIsDeletable(modelId);
            CStreaming::SetModelTxdIsDeletable(modelId);
        }
        GenTrain_Status = 0;
        return;
    }

    if (anyRandomTrain) {
        return; // Only one ambient train at a time
    }

    // Look for a node to spawn the train at
    const auto& cameraPos = TheCamera.GetPosition();

    GenTrain_Track          = 0;
    GenTrain_GenerationNode = CGeneral::GetRandomNumber() % NumTrackNodes[0];

    auto      nodePos  = pTrackNodes[0][GenTrain_GenerationNode].GetPosn();
    auto      nodeDist = std::sqrt((nodePos.y - cameraPos.y) * (nodePos.y - cameraPos.y)
                                 + (nodePos.x - cameraPos.x) * (nodePos.x - cameraPos.x));

    if (nodeDist < minDist) {
        // Spawn anyway if the player is on top of a tunnel while the node is inside of it
        const auto playerCoors = FindPlayerCoors();
        bool       nodeFarEnough = false;
        if (nodePos.z + 6.0f <= playerCoors.z) {
            const auto playerAttribs = CCullZones::FindTunnelAttributesForCoors(playerCoors);
            if (!(playerAttribs & (TUNNEL_TRANSITION | TUNNEL))) {
                if (CCullZones::FindTunnelAttributesForCoors(nodePos) & TUNNEL) {
                    nodeFarEnough = true;
                }
            }
        }

        if (!nodeFarEnough) {
            GenTrain_Direction = CGeneral::GetRandomNumber() & 1;

            uint32 prevNode = GenTrain_GenerationNode;
            while (nodeDist < 170.0f) {
                prevNode = GenTrain_GenerationNode;
                if (!GenTrain_Direction) {
                    GenTrain_GenerationNode = (GenTrain_GenerationNode + 1) % NumTrackNodes[GenTrain_Track];
                } else {
                    GenTrain_GenerationNode = GenTrain_GenerationNode
                        ? GenTrain_GenerationNode - 1
                        : NumTrackNodes[GenTrain_Track] - 1;
                }

                nodePos  = pTrackNodes[GenTrain_Track][GenTrain_GenerationNode].GetPosn();
                nodeDist = std::sqrt((nodePos.y - cameraPos.y) * (nodePos.y - cameraPos.y)
                                   + (nodePos.x - cameraPos.x) * (nodePos.x - cameraPos.x));
            }

            GenTrain_TrainConfig = (GenTrain_LastConfig + 1) & 7;
            GenTrain_Status      = 1;
            GenTrain_LastConfig  = GenTrain_TrainConfig;

            if (nodeDist > 220.0f) {
                GenTrain_GenerationNode = prevNode; // Too far away - retry from the original node
                return;
            }
        }
    }

    // Not spawned on the main track: try track 1 (only in San Fierro weather region)
    if (GenTrain_Status == 0 && CWeather::WeatherRegion == WEATHER_REGION_SF) {
        GenTrain_Track          = 1;
        GenTrain_GenerationNode = CGeneral::GetRandomNumber() % NumTrackNodes[1];

        const auto otherNodePos = pTrackNodes[1][GenTrain_GenerationNode].GetPosn();
        nodeDist = std::sqrt((otherNodePos.y - cameraPos.y) * (otherNodePos.y - cameraPos.y)
                           + (otherNodePos.x - cameraPos.x) * (otherNodePos.x - cameraPos.x));

        if (nodeDist < minDist) {
            GenTrain_Direction = CGeneral::GetRandomNumber() & 1;

            while (nodeDist < 170.0f) {
                if (!GenTrain_Direction) {
                    GenTrain_GenerationNode = (GenTrain_GenerationNode + 1) % NumTrackNodes[GenTrain_Track];
                } else {
                    GenTrain_GenerationNode = GenTrain_GenerationNode
                        ? GenTrain_GenerationNode - 1
                        : NumTrackNodes[GenTrain_Track] - 1;
                }

                const auto newNodePos = pTrackNodes[GenTrain_Track][GenTrain_GenerationNode].GetPosn();
                nodeDist = std::sqrt((newNodePos.y - cameraPos.y) * (newNodePos.y - cameraPos.y)
                                   + (newNodePos.x - cameraPos.x) * (newNodePos.x - cameraPos.x));
            }

            if (nodeDist < 220.0f) {
                GenTrain_Status      = 1;
                GenTrain_TrainConfig = (CGeneral::GetRandomNumber() & 1) + 8; // Tram configurations
            }
        }
    }
}

// 0x6F8170
void CTrain::AddNearbyPedAsRandomPassenger() {
    const auto player = FindPlayerPed();
    if (player->m_pVehicle == this) {
        return;
    }

    // Don't interfere with the player while he's entering the train
    if (const auto task = player->GetTaskManager().GetTaskPrimary(TASK_PRIMARY_DEFAULT)) {
        const auto taskType = task->GetTaskType();
        if ((taskType == TASK_COMPLEX_ENTER_CAR_AS_PASSENGER || taskType == TASK_COMPLEX_ENTER_CAR_AS_DRIVER)
            && static_cast<CTaskComplexEnterCar*>(task)->GetTargetCar() == this
        ) {
            return;
        }
    }

    const auto clockwise = trainFlags.bClockwiseDirection;
    if (m_nNumPassengersToLeave == m_nNumPassengersToEnter) {
        return;
    }

    if (m_pTemporaryPassenger) {
        auto* const tempPassenger = m_pTemporaryPassenger;
        const bool  justGotOffTrain = tempPassenger->bJustGotOffTrain;
        const bool  inVehicle       = tempPassenger->bInVehicle;
        if (justGotOffTrain == inVehicle) { // Wait until the ped either left the train or got in
            return;
        }
        if (tempPassenger->GetIntelligence()->FindTaskByType(TASK_COMPLEX_LEAVE_CAR_AND_WANDER)) {
            return;
        }
        CEntity::ClearReference(m_pTemporaryPassenger);
        if (!justGotOffTrain && inVehicle && tempPassenger->m_pVehicle == this) {
            CPopulation::RemovePed(tempPassenger);
        }
        m_pTemporaryPassenger = nullptr;
    }

    const auto& trainPos = GetPosition();
    const auto& trainRight = GetRight();

    CPed*     bestPed{};
    float     bestDistSq   = 999999.0f;
    CVector   bestPedDelta{};
    for (auto& ped : GetPedPool()->GetAllValid()) {
        if (ped.GetCreatedBy() != PED_GAME
            || CPedGroups::GetPedsGroup(&ped)
            || ped.GetPlayerData()
            || ped.m_nPedType == PED_TYPE_COP
            || ped.bInVehicle
            || ped.bJustGotOffTrain
            || ped.m_nPedState == PEDSTATE_DIE
            || ped.m_nPedState == PEDSTATE_DEAD
            || ped.m_nPedState == PEDSTATE_DIE_BY_STEALTH
            || ped.GetIntelligence()->FindTaskByType(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER)
        ) {
            continue;
        }

        const auto& pedPos  = ped.GetPosition();
        const CVector delta = pedPos - trainPos;

        const auto distSq = delta.SquaredMagnitude();
        if (distSq > 625.0f) { // 25m
            continue;
        }

        // The ped has to be on the side the train is heading toward
        const auto side = DotProduct(pedPos, trainRight) - DotProduct(trainPos, trainRight);
        if (clockwise ? side < 0.0f : side > 0.0f) {
            continue;
        }

        if (distSq < bestDistSq) {
            bestPed      = &ped;
            bestDistSq   = distSq;
            bestPedDelta = delta;
        }
    }

    if (!bestPed) {
        return;
    }

    { // NOTE: The results are unused, but the original still calls it here
        CVector doorPos;
        int32   doorId{};
        CCarEnterExit::GetNearestCarDoor(bestPed, this, doorPos, doorId);
    }

    // Offset (in the train's local space) of the position the ped should seek
    CMatrix invMatrix;
    Invert(GetMatrix(), invMatrix);
    CVector offset = invMatrix.TransformPoint(bestPedDelta);
    if (clockwise) {
        offset += trainRight;
    } else {
        offset -= trainRight;
    }

    const auto seekTask = new CTaskComplexSeekEntity<CEntitySeekPosCalculatorXYOffset>(
        this,
        50'000,
        1'000,
        1.0f,
        2.0f,
        2.0f,
        true,
        true
    );
    seekTask->GetSeekPosCalculator().SetOffset(offset);
    seekTask->SetMoveState(PEDMOVE_SPRINT);

    const auto sequence = new CTaskComplexSequence{};
    sequence->AddTask(seekTask);
    sequence->AddTask(clockwise
        ? static_cast<CTask*>(new CTaskComplexEnterCarAsPassenger(this, 0, false))
        : static_cast<CTask*>(new CTaskComplexEnterCarAsDriver(this))
    );
    bestPed->GetTaskManager().SetTask(sequence, TASK_PRIMARY_PRIMARY, false);

    ++m_nNumPassengersToLeave;
    m_pTemporaryPassenger = bestPed;
    CEntity::RegisterReference(m_pTemporaryPassenger);
}

// 0x6F86A0
void CTrain::ProcessControl() {
    vehicleFlags.bWarnedPeds = 0;
    m_vehicleAudio.Service();
    if (gbModelViewer) {
        return;
    }

    CVector vecOldTrainPosition = GetPosition();
    float fOldTrainHeading = GetHeading();

    const float& fTotalTrackLength = arrTotalTrackLength[m_nTrackId];
    CTrainNode* trainNodes = pTrackNodes[m_nTrackId];
    auto numTrackNodes = NumTrackNodes[m_nTrackId];

    if (trainFlags.bNotOnARailRoad == 0) {
        if (!trainFlags.bIsFrontCarriage) {
            if (m_pPrevCarriage) {
                m_fTrainSpeed = m_pPrevCarriage->m_fTrainSpeed;
                m_fCurrentRailDistance = m_pPrevCarriage->m_fCurrentRailDistance + m_fLength;
            } else {
                m_fTrainSpeed *= std::pow(0.9900000095367432f, CTimer::GetTimeStep());
                m_fCurrentRailDistance += m_fTrainSpeed * CTimer::GetTimeStep();
            }

            if (trainFlags.b01 && trainFlags.bStoppedAtStation && m_nModelIndex == MODEL_STREAKC && !trainFlags.bMissionTrain) {
                CPlayerPed* localPlayer = FindPlayerPed();
                if (m_nPassengersGenerationState == TRAIN_PASSENGERS_QUERY_NUM_PASSENGERS_TO_LEAVE) {
                    if (localPlayer->m_pVehicle == this) {
                        m_nNumPassengersToLeave = 0;
                    } else {
                        m_nNumPassengersToLeave = (CGeneral::GetRandomNumber() & 3) + 1; // [1, 4]
                    }
                    m_nPassengersGenerationState = TRAIN_PASSENGERS_TELL_PASSENGERS_TO_LEAVE;
                }

                if (m_nPassengersGenerationState == TRAIN_PASSENGERS_TELL_PASSENGERS_TO_LEAVE) {
                    RemoveRandomPassenger();
                    if (m_nNumPassengersToLeave == 0) {
                        m_nPassengersGenerationState = TRAIN_PASSENGERS_QUERY_NUM_PASSENGERS_TO_ENTER;
                    }
                }

                if (m_nPassengersGenerationState == TRAIN_PASSENGERS_QUERY_NUM_PASSENGERS_TO_ENTER) {
                    if (localPlayer->m_pVehicle == this) {
                        m_nNumPassengersToEnter = 0;
                    } else {
                        m_nNumPassengersToEnter = CGeneral::GetRandomNumber() % 4 + 1; // rand(1, 4)
                    }
                    m_nPassengersGenerationState = TRAIN_PASSENGERS_TELL_PASSENGERS_TO_ENTER;
                }

                if (m_nPassengersGenerationState == TRAIN_PASSENGERS_TELL_PASSENGERS_TO_ENTER) {
                    if (trainFlags.bPassengersCanEnterAndLeave) {
                        AddNearbyPedAsRandomPassenger();
                        if (m_nNumPassengersToLeave == m_nNumPassengersToEnter) {
                            m_nPassengersGenerationState = TRAIN_PASSENGERS_GENERATION_FINISHED;
                        }
                    }
                }
            }
        } else {
            CPad* pad = CPad::GetPad();
            if (m_pDriver && m_pDriver->IsPlayer()) {
                pad = m_pDriver->AsPlayer()->GetPadFromPlayer();
            }

            uint32 numCarriagesPulled = FindNumCarriagesPulled();
            if (!trainFlags.bClockwiseDirection) {
                m_fTrainSpeed = -m_fTrainSpeed;
            }

            if (GetStatus()) {
                bool bIsStreakModel = trainFlags.bIsStreakModel;
                auto fStopAtStationSpeed = static_cast<float>(m_autoPilot.m_nCruiseSpeed);

                uint32 timeAtStation = CTimer::GetTimeInMS() - m_nTimeWhenStoppedAtStation;
                if (timeAtStation >= (bIsStreakModel ? 20'000u : 10'000u)) {
                    if (timeAtStation >= (bIsStreakModel ? 28'000u : 18'000u)) {
                        if (timeAtStation >= (bIsStreakModel ? 32'000u : 22'000u)) {
                            if (trainFlags.bStopsAtStations) {
                                float maxTrainSpeed = 0.0f;
                                if (FindMaximumSpeedToStopAtStations(&maxTrainSpeed)) {
                                    fStopAtStationSpeed = 0.0f;
                                    m_nTimeWhenStoppedAtStation = CTimer::GetTimeInMS();
                                } else {
                                    if (fStopAtStationSpeed >= maxTrainSpeed) {
                                        fStopAtStationSpeed = maxTrainSpeed;
                                    }
                                }
                            }
                        } else if (trainFlags.bStoppedAtStation) {
                            CTrain* trainCarriage = this;
                            do {
                                trainFlags.bStoppedAtStation = false;
                                trainCarriage->m_nPassengersGenerationState = TRAIN_PASSENGERS_GENERATION_FINISHED;
                                trainCarriage = trainCarriage->m_pNextCarriage;
                            } while (trainCarriage);
                        }
                    } else {
                        fStopAtStationSpeed = 0.0f;
                        if (trainFlags.bStoppedAtStation) {
                            CTrain* trainCarriage = this;
                            do {
                                trainFlags.bPassengersCanEnterAndLeave = false;
                                trainCarriage->m_nPassengersGenerationState = TRAIN_PASSENGERS_GENERATION_FINISHED;
                                trainCarriage = trainCarriage->m_pNextCarriage;
                            } while (trainCarriage);
                        }
                    }
                } else {
                    fStopAtStationSpeed = 0.0f;
                    if (!trainFlags.bStoppedAtStation) {
                        CTrain* trainCarriage = this;
                        do {
                            trainFlags.bStoppedAtStation = true;
                            trainFlags.bPassengersCanEnterAndLeave = true;
                            trainCarriage->m_nPassengersGenerationState = TRAIN_PASSENGERS_QUERY_NUM_PASSENGERS_TO_LEAVE;
                            trainCarriage = trainCarriage->m_pNextCarriage;
                        } while (trainCarriage);
                    }
                }

                fStopAtStationSpeed = fStopAtStationSpeed / 50.0f - m_fTrainSpeed;
                if (fStopAtStationSpeed > 0.0f) {
                    m_fTrainGas = fStopAtStationSpeed * 30.0f;
                    if (m_fTrainGas >= 1.0f) {
                        m_fTrainGas = 1.0f;
                    }

                    m_fTrainGas *= 255.0f;
                    m_fTrainBrake = 0.0f;
                } else {
                    float fTrainSpeed = fStopAtStationSpeed * -30.0f;
                    m_fTrainGas = 0.0f;
                    if (fTrainSpeed >= 1.0f) {
                        fTrainSpeed = 1.0f;
                    }
                    m_fTrainBrake = fTrainSpeed * 255.0f;
                }
            } else {
                float fTrainSpeed = m_fTrainSpeed;
                if (fTrainSpeed < 0.0f) {
                    fTrainSpeed = -fTrainSpeed;
                }

                if (fTrainSpeed < 0.001f) {
                    m_fTrainBrake = 0.0f;
                    m_fTrainGas = static_cast<float>(pad->GetAccelerate() - pad->GetBrake());
                } else {
                    if (m_fTrainSpeed > 0.0f) {
                        m_fTrainBrake = static_cast<float>(pad->GetBrake());
                        m_fTrainGas = static_cast<float>(pad->GetAccelerate());
                    } else {
                        m_fTrainGas = static_cast<float>(-pad->GetBrake());
                        m_fTrainBrake = static_cast<float>(pad->GetAccelerate());
                    }
                }
            }

            if (trainFlags.bForceSlowDown) {
                const CVector& vecPoint = GetPosition();
                CVector vecDistance{};
                if (CGameLogic::CalcDistanceToForbiddenTrainCrossing(vecPoint, m_vecMoveSpeed, true, vecDistance) < 230.0f) {
                    if (DotProduct(GetForwardVector(), vecDistance) <= 0.0f) {
                        m_fTrainGas = std::max(0.0f, m_fTrainGas);
                    } else {
                        m_fTrainGas = std::min(0.0f, m_fTrainGas);
                    }

                    if (CGameLogic::CalcDistanceToForbiddenTrainCrossing(vecPoint, m_vecMoveSpeed, false, vecDistance) < 230.0f) {
                        m_fTrainBrake = 512.0f;
                    }
                }
            }

            numCarriagesPulled += 3;

            m_fTrainSpeed += m_fTrainGas / 256.0f * CTimer::GetTimeStep() * 0.002f / float(numCarriagesPulled);

            if (m_fTrainBrake != 0.0f) {
                float fTrainSpeed = m_fTrainSpeed;
                if (m_fTrainSpeed < 0.0f) {
                    fTrainSpeed = -fTrainSpeed;
                }
                float fBreak = m_fTrainBrake / 256.0f * CTimer::GetTimeStep() * 0.006f / float(numCarriagesPulled);
                if (fTrainSpeed >= fBreak) {
                    if (m_fTrainSpeed < 0.0f) {
                        m_fTrainSpeed += fBreak;
                    } else {
                        m_fTrainSpeed -= fBreak;
                    }
                } else {
                    m_fTrainSpeed = 0.0f;
                }
            }

            m_fTrainSpeed *= pow(0.999750018119812f, CTimer::GetTimeStep());
            if (!trainFlags.bClockwiseDirection) {
                m_fTrainSpeed = -m_fTrainSpeed;
            }

            m_fCurrentRailDistance += CTimer::GetTimeStep() * m_fTrainSpeed;

            if (GetStatus() == STATUS_PLAYER) {

                float fTheTrainSpeed = m_fTrainSpeed;
                if (fTheTrainSpeed < 0.0f) {
                    fTheTrainSpeed = -fTheTrainSpeed;
                }
                if (fTheTrainSpeed > 1.0f * 0.95f) {
                    CPad::GetPad()->StartShake(300, 70, 0);
                    TheCamera.CamShake(0.1f, GetPosition());
                }

                fTheTrainSpeed = m_fTrainSpeed;
                if (fTheTrainSpeed < 0.0f) {
                    fTheTrainSpeed = -fTheTrainSpeed;
                }

                if (fTheTrainSpeed > 1.0f) {
                    int32 nNodeIndex = m_nNodeIndex;
                    int32 previousNodeIndex = nNodeIndex - 1;
                    if (previousNodeIndex < 0) {
                        previousNodeIndex = numTrackNodes;
                    }

                    int32 previousNodeIndex2 = previousNodeIndex - 1;
                    if (previousNodeIndex2 < 0) {
                        previousNodeIndex2 = numTrackNodes;
                    }

                    CTrainNode* pCurrentTrainNode = &trainNodes[m_nNodeIndex];
                    CTrainNode* pPreviousTrainNode = &trainNodes[previousNodeIndex];
                    CTrainNode* pPreviousTrainNode2 = &trainNodes[previousNodeIndex2];

                    CVector vecDifference1 = pCurrentTrainNode->GetPosn() - pPreviousTrainNode->GetPosn();
                    CVector vecDifference2 = pPreviousTrainNode->GetPosn() - pPreviousTrainNode2->GetPosn();
                    vecDifference1.Normalise();
                    vecDifference2.Normalise();

                    if (DotProduct(vecDifference1, vecDifference2) < 0.996f) {
                        CTrain* carriage = this;
                        bool bIsInTunnel = false;
                        while (!bIsInTunnel) {
                            bIsInTunnel = carriage->IsInTunnel();
                            carriage = carriage->m_pNextCarriage;
                            if (!carriage) {
                                if (!bIsInTunnel) {
                                    CTrain* theTrainCarriage = this;
                                    do {
                                        trainFlags.bNotOnARailRoad = true;
                                        theTrainCarriage->physicalFlags.bDisableCollisionForce = false;
                                        theTrainCarriage->physicalFlags.bDisableSimpleCollision = false;
                                        theTrainCarriage->SetIsStatic(false);
                                        theTrainCarriage = theTrainCarriage->m_pNextCarriage;
                                    } while (theTrainCarriage);

                                    CPhysical::ProcessControl();
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (m_fCurrentRailDistance < 0.0f) {
            do {
                m_fCurrentRailDistance += fTotalTrackLength;
            } while (m_fCurrentRailDistance < 0.0f);
        }

        if (m_fCurrentRailDistance >= fTotalTrackLength) {
            do {
                m_fCurrentRailDistance -= fTotalTrackLength;
            } while (m_fCurrentRailDistance >= fTotalTrackLength);
        }

        float fNextNodeTrackLength = 0.0f;
        int32 nextNodeIndex = m_nNodeIndex + 1;
        if (nextNodeIndex < numTrackNodes) {
            CTrainNode* nextTrainNode = &trainNodes[nextNodeIndex];
            fNextNodeTrackLength = nextTrainNode->GetDistanceFromStart();
        } else {
            fNextNodeTrackLength = fTotalTrackLength;
            nextNodeIndex = 0;
        }

        CTrainNode* theTrainNode = &trainNodes[m_nNodeIndex];
        float fCurrentNodeTrackLength = theTrainNode->GetDistanceFromStart();
        while (m_fCurrentRailDistance < fCurrentNodeTrackLength || fNextNodeTrackLength < m_fCurrentRailDistance) {
            int32 newNodeIndex = m_nNodeIndex - 1; // previous node
            if (fCurrentNodeTrackLength <= m_fCurrentRailDistance) {
                newNodeIndex = m_nNodeIndex + 1; // next node
            }
            m_nNodeIndex = newNodeIndex % numTrackNodes;
            m_vehicleAudio.AddAudioEvent(AE_TRAIN_CLACK, 0.0f);

            theTrainNode = &trainNodes[m_nNodeIndex];
            fCurrentNodeTrackLength = theTrainNode->GetDistanceFromStart();

            nextNodeIndex = m_nNodeIndex + 1;
            if (nextNodeIndex < numTrackNodes) {
                CTrainNode* nextTrainNode = &trainNodes[nextNodeIndex];
                fNextNodeTrackLength = nextTrainNode->GetDistanceFromStart();
            } else {
                fNextNodeTrackLength = fTotalTrackLength;
                nextNodeIndex = 0;
            }
        }

        CTrainNode* nextTrainNode = &trainNodes[nextNodeIndex];
        fNextNodeTrackLength = nextTrainNode->GetDistanceFromStart();

        float fTrackNodeDifference = fNextNodeTrackLength - fCurrentNodeTrackLength;
        if (fTrackNodeDifference < 0.0f) {
            fTrackNodeDifference += fTotalTrackLength;
        }

        float fTheDistance = (m_fCurrentRailDistance - fCurrentNodeTrackLength) / fTrackNodeDifference;
        CVector vecPosition1 = theTrainNode->GetPosn() * (1.0f - fTheDistance) + nextTrainNode->GetPosn() * fTheDistance;

        CColModel* vehicleColModel = CModelInfo::GetModelInfo(m_nModelIndex)->GetColModel();
        const CBoundingBox& bbox = vehicleColModel->GetBoundingBox();
        float fTotalCurrentRailDistance = bbox.GetLength() + m_fCurrentRailDistance;
        if (fTotalCurrentRailDistance > fTotalTrackLength) {
            fTotalCurrentRailDistance -= fTotalTrackLength;
        }

        nextNodeIndex = m_nNodeIndex + 1;
        if (nextNodeIndex < numTrackNodes) {
            fNextNodeTrackLength = trainNodes[nextNodeIndex].GetDistanceFromStart();
        } else {
            fNextNodeTrackLength = fTotalTrackLength;
            nextNodeIndex = 0;
        }

        int32 trainNodeIndex = m_nNodeIndex;
        while (fTotalCurrentRailDistance < fCurrentNodeTrackLength || fTotalCurrentRailDistance > fNextNodeTrackLength) {
            trainNodeIndex = (trainNodeIndex + 1) % numTrackNodes;

            theTrainNode = &trainNodes[trainNodeIndex];
            fCurrentNodeTrackLength = theTrainNode->GetDistanceFromStart();

            nextNodeIndex = trainNodeIndex + 1;
            if (nextNodeIndex < numTrackNodes) {
                fNextNodeTrackLength = trainNodes[nextNodeIndex].GetDistanceFromStart();
            } else {
                fNextNodeTrackLength = fTotalTrackLength;
                nextNodeIndex = 0;
            }
        }

        fNextNodeTrackLength = trainNodes[nextNodeIndex].GetDistanceFromStart();

        fTrackNodeDifference = fNextNodeTrackLength - fCurrentNodeTrackLength;
        if (fTrackNodeDifference < 0.0f) {
            fTrackNodeDifference += fTotalTrackLength;
        }

        fTheDistance = (fTotalCurrentRailDistance - fCurrentNodeTrackLength) / fTrackNodeDifference;
        CVector vecPosition2 = theTrainNode->GetPosn() * (1.0f - fTheDistance) + trainNodes[nextNodeIndex].GetPosn() * fTheDistance;

        {
            CVector& vecVehiclePosition = GetPosition();
            vecVehiclePosition = (vecPosition1 + vecPosition2) / 2.0f;
            vecVehiclePosition.z += m_pHandlingData->m_fSuspensionLowerLimit - bbox.m_vecMin.z;
        }

        GetForward() = vecPosition2 - vecPosition1;
        GetForward().Normalise();
        if (!trainFlags.bClockwiseDirection) {
            GetForward() *= -1.0f;
        }

        CVector vecTemp(0.0f, 0.0f, 1.0f);
        CrossProduct(&GetRight(), &GetForward(), &vecTemp);
        GetRight().Normalise();
        CrossProduct(&GetUp(), &GetRight(), &GetForward());

        auto fTrainNodeLighting     = theTrainNode->GetLightingFromCollision().GetCurrentLighting();
        auto fTrainNextNodeLighting = trainNodes[nextNodeIndex].GetLightingFromCollision().GetCurrentLighting();

        fTrainNodeLighting += (fTrainNextNodeLighting - fTrainNodeLighting) * fTheDistance;
        m_fContactSurfaceBrightness = fTrainNodeLighting;
        m_vecMoveSpeed = (1.0f / CTimer::GetTimeStep()) * (GetPosition() - vecOldTrainPosition);

        float fNewTrainHeading = GetHeading();
        float fHeading = fNewTrainHeading - fOldTrainHeading;
        if (fHeading <= PI) {
            if (fHeading < -PI) {
                fHeading += TWO_PI;
            }
        } else {
            fHeading -= TWO_PI;
        }

        m_vecTurnSpeed = CVector(0.0f, 0.0f, fHeading / CTimer::GetTimeStep());

        if (trainFlags.bNotOnARailRoad) {
            m_vecMoveSpeed *= -1.0f;
            m_vecTurnSpeed *= -1.0f;

            ApplyMoveSpeed();

            m_vecMoveSpeed *= -1.0f;
            m_vecTurnSpeed *= -1.0f;

            CPhysical::ProcessControl();
        } else {
            m_vecMoveSpeed.x = std::clamp(m_vecMoveSpeed.x, -2.0f, 2.0f);
            m_vecMoveSpeed.y = std::clamp(m_vecMoveSpeed.y, -2.0f, 2.0f);
            m_vecMoveSpeed.z = std::clamp(m_vecMoveSpeed.z, -2.0f, 2.0f);

            m_vecTurnSpeed.x = std::clamp(m_vecTurnSpeed.x, -0.1f, 0.1f);
            m_vecTurnSpeed.y = std::clamp(m_vecTurnSpeed.y, -0.1f, 0.1f);
            m_vecTurnSpeed.z = std::clamp(m_vecTurnSpeed.z, -0.1f, 0.1f);
        }

        UpdateRwMatrix();
        UpdateRwFrame();
        RemoveAndAdd();

        SetIsStuck(false);
        SetWasPostponed(false);
        SetIsInSafePosition(true);

        m_fMovingSpeed = DistanceBetweenPoints(GetPosition(), vecOldTrainPosition);

        if (trainFlags.bIsFrontCarriage || trainFlags.bIsLastCarriage) {
            CVector vecPoint = bbox.m_vecMax.y * GetForward();
            vecPoint += GetPosition();
            vecPoint += CTimer::GetTimeStep() * m_vecMoveSpeed;

            MarkSurroundingEntitiesForCollisionWithTrain(vecPoint, 3.0f, this, false);
        }

        if (!vehicleFlags.bWarnedPeds) {
            CCarCtrl::ScanForPedDanger(this);
        }
        return;
    } else {
        if (!GetIsStuck()) {
            float fMaxForce = 0.003f;
            float fMaxTorque = 0.0009f;
            float fMaxMovingSpeed = 0.005f;

            if (GetStatus() != STATUS_PLAYER) {
                fMaxForce = 0.006f;
                fMaxTorque = 0.0015f;
                fMaxMovingSpeed = 0.015f;
            }

            float fMaxForceTimeStep  = (fMaxForce  * CTimer::GetTimeStep()) * (fMaxForce * CTimer::GetTimeStep());
            float fMaxTorqueTimeStep = (fMaxTorque * CTimer::GetTimeStep()) * (fMaxTorque * CTimer::GetTimeStep());

            m_vecForce  = (m_vecForce  + m_vecMoveSpeed) / 2.0f;
            m_vecTorque = (m_vecTorque + m_vecTurnSpeed) / 2.0f;

            if (m_vecForce.SquaredMagnitude() > fMaxForceTimeStep ||
                m_vecTorque.SquaredMagnitude() > fMaxTorqueTimeStep ||
                m_fMovingSpeed >= fMaxMovingSpeed ||
                m_fDamageIntensity > 0.0f && m_pDamageEntity != nullptr && m_pDamageEntity->GetIsTypePed()
            ) {
                m_nFakePhysics = 0;
            } else {
                m_nFakePhysics += 1;
                if (m_nFakePhysics > 10 /*&& !plugin::Call<0x424100>()*/) {
                    // if (m_nFakePhysics > 10) { // OG redundant check
                        m_nFakePhysics = 10;
                    // }

                    ResetMoveSpeed();
                    ResetTurnSpeed();
                    SkipPhysics();
                    return;
                }
            }
        }

        CPhysical::ProcessControl();

        CVector vecMoveForce{}, vecTurnForce{};
        if (mod_Buoyancy.ProcessBuoyancy(this, m_fBuoyancyConstant, &vecMoveForce, &vecTurnForce)) {
            physicalFlags.bTouchingWater = true;

            float fTimeStep = 0.01f;
            if (CTimer::GetTimeStep() >= 0.01f) {
                fTimeStep = CTimer::GetTimeStep();
            }

            float fSpeedFactor = 1.0f - vecOldTrainPosition.z / (fTimeStep * m_fMass * 0.008f) * 0.05f;
            fSpeedFactor = std::pow(fSpeedFactor, CTimer::GetTimeStep());

            m_vecMoveSpeed *= fSpeedFactor;
            m_vecTurnSpeed *= fSpeedFactor;
            ApplyMoveForce(vecOldTrainPosition.x, vecOldTrainPosition.y, vecOldTrainPosition.z);
            ApplyTurnForce(vecTurnForce, vecMoveForce);
        }
    }
}
