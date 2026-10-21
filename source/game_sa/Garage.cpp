#include "StdInc.h"

#include "Garage.h"
#include "Garages.h"

void CGarage::InjectHooks() {
    RH_ScopedClass(CGarage);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(BuildRotatedDoorMatrix, 0x4479F0);
    RH_ScopedInstall(TidyUpGarageClose, 0x449D10);
    RH_ScopedInstall(TidyUpGarage, 0x449C50);
    RH_ScopedInstall(StoreAndRemoveCarsForThisHideOut, 0x449900);
    RH_ScopedInstall(EntityHasASphereWayOutsideGarage, 0x449050);
    RH_ScopedInstall(RemoveCarsBlockingDoorNotInside, 0x449690);
    RH_ScopedInstall(IsEntityTouching3D, 0x448EE0);
    RH_ScopedInstall(IsEntityEntirelyOutside, 0x448D30);
    RH_ScopedInstall(IsStaticPlayerCarEntirelyInside, 0x44A830);
    RH_ScopedInstall(IsEntityEntirelyInside3D, 0x448BE0);
    RH_ScopedOverloadedInstall(IsPointInsideGarage, "CVector", 0x448740, bool(CGarage::*)(CVector));
    RH_ScopedInstall(PlayerArrestedOrDied, 0x4486C0);
    RH_ScopedInstall(OpenThisGarage, 0x447D50);
    RH_ScopedInstall(CloseThisGarage, 0x447D70);
    RH_ScopedInstall(InitDoorsAtStart, 0x447600);
    RH_ScopedOverloadedInstall(IsPointInsideGarage, "CVector-float", 0x4487D0, bool(CGarage::*)(CVector, float));
    RH_ScopedInstall(Update, 0x44AA50);
    RH_ScopedInstall(IsGarageEmpty, 0x44A9C0);
    RH_ScopedInstall(FindDoorsWithGarage, 0x449FF0);
}

// 0x4479F0
void CGarage::BuildRotatedDoorMatrix(CEntity* entity, float fDoorPosition) {
    const auto fAngle = fDoorPosition * -HALF_PI;
    const auto fSin = sin(fAngle);
    const auto fCos = cos(fAngle);
    CMatrix& matrix = entity->GetMatrix();

    const auto& vecForward = matrix.GetForward();
    matrix.GetUp() = CVector(-fSin * vecForward.y, fSin * vecForward.x, fCos);
    matrix.GetRight() = CrossProduct(vecForward, matrix.GetUp());
}

// 0x449D10
void CGarage::TidyUpGarageClose() {
    const auto pool = GetVehiclePool();
    for (auto i = static_cast<int32>(pool->GetSize()) - 1; i != 0; i--) { // Note: Slot 0 is never visited
        if (pool->IsFreeSlotAtIndex(i)) {
            continue;
        }

        auto* vehicle = pool->GetAt(i);
        if (!vehicle) {
            continue;
        }

        if (vehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE && vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
            continue;
        }

        if (vehicle->GetStatus() != STATUS_WRECKED) {
            continue;
        }

        if (!IsEntityTouching3D(vehicle)) {
            continue;
        }

        if (m_nDoorState != GARAGE_DOOR_CLOSED) {
            // The whole car must fit inside the garage
            const auto colData = vehicle->GetColData();
            bool anySphereOutside = false;
            for (auto s = 0; s < colData->m_nNumSpheres; s++) {
                const auto& sphere = colData->m_pSpheres[s];
                if (!IsPointInsideGarage(vehicle->GetMatrix().TransformPoint(sphere.m_vecCenter), sphere.m_fRadius)) {
                    anySphereOutside = true;
                    break;
                }
            }
            if (anySphereOutside) {
                continue;
            }
        }

        CWorld::Remove(vehicle);
        delete vehicle;
        return; // At most one car per call
    }
}

// 0x449C50
void CGarage::TidyUpGarage() {
    const auto pool = GetVehiclePool();
    for (auto i = static_cast<int32>(pool->GetSize()) - 1; i != 0; i--) { // Note: Slot 0 is never visited
        if (pool->IsFreeSlotAtIndex(i)) {
            continue;
        }

        auto* vehicle = pool->GetAt(i);
        if (!vehicle) {
            continue;
        }

        if (vehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE && vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
            continue;
        }

        if (!IsPointInsideGarage(vehicle->GetPosition())) {
            continue;
        }

        // Wrecked or on its roof
        if (vehicle->GetStatus() == STATUS_WRECKED || vehicle->GetMatrix().GetUp().z < 0.0f) {
            CWorld::Remove(vehicle);
            delete vehicle;
        }
    }
}

// 0x449900
void CGarage::StoreAndRemoveCarsForThisHideOut(CStoredCar* storedCars, int32 maxSlot) {
    maxSlot = std::min<int32>(maxSlot, NUM_GARAGE_STORED_CARS);

    for (auto i = 0; i < NUM_GARAGE_STORED_CARS; i++)
        storedCars[i].Clear();

    auto pool = GetVehiclePool();
    auto storedCarIdx{0u};
    for (auto i = pool->GetSize(); i; i--) {
        if (auto vehicle = pool->GetAt(i - 1)) {
            if (IsPointInsideGarage(vehicle->GetPosition()) && vehicle->GetCreatedBy() != MISSION_VEHICLE) {
                if (storedCarIdx < static_cast<uint32>(maxSlot) && !EntityHasASphereWayOutsideGarage(vehicle, 1.0f)) {
                    storedCars[storedCarIdx++].StoreCar(vehicle);
                }

                FindPlayerInfo().CancelPlayerEnteringCars(vehicle);
                CWorld::Remove(vehicle);
                delete vehicle;
            }
        }
    }

    // Clear slots with no vehicles in it
    for (auto i = storedCarIdx; i < NUM_GARAGE_STORED_CARS; i++)
        storedCars[i].Clear();
}

// 0x449050
bool CGarage::EntityHasASphereWayOutsideGarage(CEntity* entity, float radius) {
    const auto colData = entity->GetColData();
    if (colData->m_nNumSpheres == 0) {
        return false;
    }

    for (auto i = 0; i < colData->m_nNumSpheres; i++) {
        const auto& sphere = colData->m_pSpheres[i];
        if (!IsPointInsideGarage(entity->GetMatrix().TransformPoint(sphere.m_vecCenter), radius + sphere.m_fRadius)) {
            return true;
        }
    }
    return false;
}

// 0x449690
void CGarage::RemoveCarsBlockingDoorNotInside() {
    const auto pool = GetVehiclePool();
    for (auto i = static_cast<int32>(pool->GetSize()) - 1; i > 0; i--) { // Note: Slot 0 is never visited
        if (pool->IsFreeSlotAtIndex(i)) {
            continue;
        }

        auto* vehicle = pool->GetAt(i);
        if (!vehicle) {
            continue;
        }

        if (!IsEntityTouching3D(vehicle)) {
            continue;
        }
        if (IsPointInsideGarage(vehicle->GetPosition())) { // Its center is inside, so it's not blocking the door
            continue;
        }
        if (vehicle->vehicleFlags.bIsLocked) { // Script-locked vehicles are never removed
            continue;
        }
        if (!vehicle->CanBeDeleted()) {
            continue;
        }

        CWorld::Remove(vehicle);
        delete vehicle;
        return; // At most one car per call
    }
}

// 0x448EE0
bool CGarage::IsEntityTouching3D(CEntity* entity) {
    const auto radius = CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetColModel()->m_boundSphere.m_fRadius;

    const auto& pos = entity->GetPosition();
    if (m_fLeftCoord - radius > pos.x || pos.x > radius + m_fRightCoord) {
        return false;
    }
    if (m_fFrontCoord - radius > pos.y || pos.y > radius + m_fBackCoord) {
        return false;
    }
    if (m_vPosn.z - radius > pos.z || pos.z > radius + m_fTopZ) {
        return false;
    }

    // Any single sphere overlapping the garage counts as touching
    const auto colData = entity->GetColData();
    for (auto i = 0; i < colData->m_nNumSpheres; i++) {
        const auto& sphere = colData->m_pSpheres[i];
        if (IsPointInsideGarage(entity->GetMatrix().TransformPoint(sphere.m_vecCenter), sphere.m_fRadius)) {
            return true;
        }
    }
    return false;
}

// 0x448D30
bool CGarage::IsEntityEntirelyOutside(CEntity* entity, float radius) {
    const auto& pos = entity->GetPosition();
    if (m_fLeftCoord - radius < pos.x && pos.x < radius + m_fRightCoord
     && m_fFrontCoord - radius < pos.y && pos.y < radius + m_fBackCoord
    ) {
        return false; // Horizontally inside (No Z test here!)
    }

    const auto colData = entity->GetColData();
    if (colData && colData->m_nNumSpheres > 0) {
        for (auto i = 0; i < colData->m_nNumSpheres; i++) {
            const auto& sphere = colData->m_pSpheres[i];
            if (IsPointInsideGarage(entity->GetMatrix().TransformPoint(sphere.m_vecCenter), radius + sphere.m_fRadius)) {
                return false;
            }
        }
    }
    return true;
}

// 0x44A830
bool CGarage::IsStaticPlayerCarEntirelyInside() {
    auto* vehicle = FindPlayerVehicle(-1, false);
    if (!vehicle) {
        return false;
    }

    if (vehicle->m_nVehicleType != VEHICLE_TYPE_AUTOMOBILE && vehicle->m_nVehicleType != VEHICLE_TYPE_BIKE) {
        return false;
    }

    if (FindPlayerPed()->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)) {
        return false;
    }

    const auto& pos = vehicle->GetPosition();
    if (pos.x < m_fLeftCoord || pos.x > m_fRightCoord) {
        return false;
    }
    if (pos.y < m_fFrontCoord || pos.y > m_fBackCoord) {
        return false;
    }

    // Must be stationary
    const auto& speed = vehicle->m_vecMoveSpeed;
    if (std::abs(speed.x) > 0.01f || std::abs(speed.y) > 0.01f || std::abs(speed.z) > 0.01f) {
        return false;
    }
    if (speed.SquaredMagnitude() > 0.0001f) {
        return false;
    }

    return IsEntityEntirelyInside3D(vehicle, 0.0f);
}

// 0x448BE0
bool CGarage::IsEntityEntirelyInside3D(CEntity* entity, float radius) {
    const auto& pos = entity->GetPosition();
    if (m_fLeftCoord - radius > pos.x || pos.x > radius + m_fRightCoord) {
        return false;
    }
    if (m_fFrontCoord - radius > pos.y || pos.y > radius + m_fBackCoord) {
        return false;
    }
    if (m_vPosn.z - radius > pos.z || pos.z > radius + m_fTopZ) {
        return false;
    }

    const auto colData = entity->GetColData();
    if (!colData || colData->m_nNumSpheres == 0) {
        return true;
    }

    for (auto i = 0; i < colData->m_nNumSpheres; i++) {
        const auto& sphere = colData->m_pSpheres[i];
        if (!IsPointInsideGarage(entity->GetMatrix().TransformPoint(sphere.m_vecCenter), radius - sphere.m_fRadius)) {
            return false;
        }
    }
    return true;
}

// 0x448740
bool CGarage::IsPointInsideGarage(CVector point) {
    if (point.z < m_vPosn.z || point.z > m_fTopZ) {
        return false;
    }

    const auto projA = (point.y - m_vPosn.y) * m_vDirectionA.y + (point.x - m_vPosn.x) * m_vDirectionA.x;
    if (projA < 0.0f || projA > m_fWidth) {
        return false;
    }

    const auto projB = (point.y - m_vPosn.y) * m_vDirectionB.y + (point.x - m_vPosn.x) * m_vDirectionB.x;
    if (projB < 0.0f || projB > m_fHeight) {
        return false;
    }

    return true;
}

// 0x4486C0
eGarageDoorState CGarage::PlayerArrestedOrDied() {
    // Note: The binary returns whatever the door state was *before* the changes below
    //       (AL is only ever loaded for the comparisons), and callers ignore it.
    const auto previousDoorState = m_nDoorState;

    switch (m_nType) {
    case BOMBSHOP_TIMED:
    case BOMBSHOP_ENGINE:
    case BOMBSHOP_REMOTE:
    case PAYNSPRAY:
    case static_cast<eGarageType>(13): // 13 is not a named `eGarageType` - Behaves like the bomb shops
        if (m_nDoorState == GARAGE_DOOR_CLOSED || m_nDoorState == GARAGE_DOOR_CLOSING || m_nDoorState == GARAGE_DOOR_OPENING) {
            m_nDoorState = GARAGE_DOOR_OPENING;
        }
        break;
    case INVALID:
        break;
    default: // Every other valid type (1, 6..12, 14..45)
        if (m_nDoorState != GARAGE_DOOR_CLOSED && m_nDoorState <= GARAGE_DOOR_OPENING) {
            m_nDoorState = GARAGE_DOOR_CLOSING;
        }
        break;
    }

    return previousDoorState;
}

// 0x447D50
void CGarage::OpenThisGarage() {
  if ( m_nDoorState == GARAGE_DOOR_CLOSED
    || m_nDoorState == GARAGE_DOOR_CLOSING
    || m_nDoorState == GARAGE_DOOR_CLOSED_DROPPED_CAR)
  {
    m_nDoorState = GARAGE_DOOR_OPENING;
  }
}

// 0x447D70
void CGarage::CloseThisGarage() {
    if (m_nDoorState == GARAGE_DOOR_OPEN || m_nDoorState == GARAGE_DOOR_OPENING)
        m_nDoorState = GARAGE_DOOR_CLOSING;
}

// 0x447600
void CGarage::InitDoorsAtStart() {
    m_nFlags      = (m_nFlags & 0x39) | 0x40; // Clear inactive/used-respray/always-free, set door-closed
    m_nDoorState  = GARAGE_DOOR_CLOSED;
    m_nTimeToOpen = 0;

    switch (m_nType) {
    case BOMBSHOP_TIMED:
    case BOMBSHOP_ENGINE:
    case BOMBSHOP_REMOTE:
    case PAYNSPRAY:
        m_nDoorState    = GARAGE_DOOR_OPEN;
        m_fDoorPosition = 1.0f;
        return;
    case INVALID:
    case static_cast<eGarageType>(13): // 13 is not a named `eGarageType` - The door position is left untouched
        return;
    default: // Every other valid type (1, 6..12, 14..45)
        m_nDoorState    = GARAGE_DOOR_CLOSED;
        m_fDoorPosition = 0.0f;
        return;
    }
}

// 0x4487D0
bool CGarage::IsPointInsideGarage(CVector point, float radius) {
    if (point.z < m_vPosn.z - radius || point.z > m_fTopZ + radius) {
        return false;
    }

    const auto projA = (point.y - m_vPosn.y) * m_vDirectionA.y + (point.x - m_vPosn.x) * m_vDirectionA.x;
    if (projA < -radius || projA > m_fWidth + radius) {
        return false;
    }

    const auto projB = (point.y - m_vPosn.y) * m_vDirectionB.y + (point.x - m_vPosn.x) * m_vDirectionB.x;
    if (projB < -radius || projB > m_fHeight + radius) {
        return false;
    }

    return true;
}

// 0x44AA50
void CGarage::Update(int32 garageId) {
    // Only the identity, the callee/global inventory and the shape of the state machine could be
    // recovered from the original (see `.local/re/0x44AA50.md`); the per-state constants were never
    // transcribed, so only the dispatch below is established.
    //
    // The original's body is a `switch (m_nDoorState)` that, per state and per `m_nType`, drives:
    //   - the door animation: `SlideDoorOpen()` 0x44A660 / `SlideDoorClosed()` 0x44A750
    //     (`m_fDoorPosition`, `m_nTimeToOpen`, `CAEDoorAudioEntity` events), neither of which is implemented yet;
    //   - the geometry predicates: `IsPlayerOutsideGarage()` 0x448E50, `IsEntityEntirelyOutside`,
    //     `IsEntityEntirelyInside3D`, `IsEntityTouching3D`, `IsPointInsideGarage`, `IsStaticPlayerCarEntirelyInside`,
    //     `IsAnyOtherCarTouchingGarage()` 0x449100, `ThrowCarsNearDoorOutOfGarage()` 0x449220,
    //     `IsAnyCarBlockingDoor()` 0x4494F0, `IsGarageEmpty()`, `CalcDistToGarageRectangleSquared()` 0x447D80;
    //   - the safehouse/impound interplay: `FindSafeHouseIndexForGarageType()`, `CountCarsInHideoutGarage()`,
    //     `CGarages::aCarsInSafeHouse`, `FindDoorsWithGarage()`, `RightModTypeForThisGarage()` 0x447720;
    //   - the player feedback: `CGarages::TriggerMessage()`, `CHud::SetHelpMessage()`, `CText::Get()`,
    //     `CAudioEngine::ReportFrontendAudioEvent()`, `CStats::IncrementStat()`, `CGeneral::GetRandomNumberInRange()`;
    //   - the door particles: `FxPrtMult_c` + `FxSystem_c::AddParticle()`;
    //   - the camera bookkeeping: `TheCamera.m_pToGarageWeAreIn` (+ `...ForHackAvoidFirstPerson`),
    //     `CGarages::LastGaragePlayerWasIn`, `CGarages::LastTimeHelpMessage`, `CGarages::bCamShouldBeOutside`;
    //   - the door trigger: `CPad::GetPad(0)`'s controller state.
    switch (m_nDoorState) {
    case GARAGE_DOOR_CLOSED:
        // TODO(re): missing the `m_nTimeToOpen`/`CTimer::GetTimeInMS()` thresholds that decide when to call
        //           `SlideDoorOpen()`, the per-`m_nType` open conditions (`BOMBSHOP_*`/`PAYNSPRAY`/`SAFEHOUSE_*`/
        //           `IMPOUND_*`/script types), the `CGarages::TriggerMessage` message tags and the
        //           `CHud::SetHelpMessage`/`CAudioEngine::ReportFrontendAudioEvent` event ids.
        break;
    case GARAGE_DOOR_OPEN:
        // TODO(re): missing the respray/bomb-shop completion path (the `CStats::IncrementStat` ids,
        //           `CVehicle::GetVehicleAppearance`, and the `CGarages::NoResprays`/`RespraysAreFree`/
        //           `BombsAreFree` price gates), the `SlideDoorClosed()` trigger and the spray particles
        //           (`FxPrtMult_c` construction values + `FxSystem_c::AddParticle` arguments).
        break;
    case GARAGE_DOOR_CLOSING:
        // TODO(re): missing the "door fully closed" condition (the `m_fDoorPosition` comparison done by
        //           `SlideDoorClosed()`), the `GARAGE_DOOR_CLOSED`/`GARAGE_DOOR_CLOSED_DROPPED_CAR` transition
        //           rules and the `m_nTimeToOpen` reset timing.
        break;
    case GARAGE_DOOR_OPENING:
        // TODO(re): missing the "door fully open" condition (the `m_fDoorPosition` comparison done by
        //           `SlideDoorOpen()`), the `GARAGE_DOOR_WAITING_PLAYER_TO_EXIT` transition and the
        //           `IsPlayerOutsideGarage()`/`IsAnyCarBlockingDoor()` re-close conditions.
        break;
    case GARAGE_DOOR_WAITING_PLAYER_TO_EXIT:
        // TODO(re): missing the `IsPlayerOutsideGarage()` radius, the `m_nTimeToOpen` wait time before
        //           closing again, and the `CGarages::TriggerMessage`/audio feedback used while waiting.
        break;
    case GARAGE_DOOR_CLOSED_DROPPED_CAR:
        // TODO(re): missing the per-`m_nType` re-open rules (`STAY_OPEN_WITH_CAR_INSIDE`,
        //           `CLOSE_WITH_CAR_DONT_OPEN_AGAIN`, `SCRIPT_ONLY_OPEN`, ...) and the
        //           `m_pTargetCar` handling.
        break;
    default:
        break;
    }

    // TODO(re): the original also runs the camera/`LastGaragePlayerWasIn` bookkeeping for `garageId`
    //           outside of the state switch, plus the periodic (`CTimer::m_FrameCounter`) `TidyUp*` work.
    (void)garageId;
}

bool CGarage::IsHideOut() const {
    switch (m_nType) {
    case eGarageType::SAFEHOUSE_GANTON:
    case eGarageType::SAFEHOUSE_SANTAMARIA:
    case eGarageType::SAGEHOUSE_ROCKSHORE:
    case eGarageType::SAFEHOUSE_FORTCARSON:
    case eGarageType::SAFEHOUSE_VERDANTMEADOWS:
    case eGarageType::SAFEHOUSE_DILLIMORE:
    case eGarageType::SAFEHOUSE_PRICKLEPINE:
    case eGarageType::SAFEHOUSE_WHITEWOOD:
    case eGarageType::SAFEHOUSE_PALOMINOCREEK:
    case eGarageType::SAFEHOUSE_REDSANDSWEST:
    case eGarageType::SAFEHOUSE_ELCORONA:
    case eGarageType::SAFEHOUSE_MULHOLLAND:
    case eGarageType::SAFEHOUSE_CALTONHEIGHTS:
    case eGarageType::SAFEHOUSE_PARADISO:
    case eGarageType::SAFEHOUSE_DOHERTY:
    case eGarageType::SAFEHOUSE_HASHBURY:
    case eGarageType::HANGAR_ABANDONED_AIRPORT:
        return true;
    default:
        return false;
    }
}

// 0x44A9C0
bool CGarage::IsGarageEmpty() {
    const CVector cornerA{ m_fLeftCoord, m_fFrontCoord, m_vPosn.z };
    const CVector cornerB{ m_fRightCoord, m_fBackCoord, m_fTopZ };

    int16 outCount{};
    CEntity* outEntities[16];
    CWorld::FindObjectsIntersectingCube(cornerA, cornerB, &outCount, static_cast<int16>(std::size(outEntities)), outEntities, false, true, true, false, false);

    for (auto i = 0; i < outCount; i++) {
        if (IsEntityTouching3D(outEntities[i])) {
            return false;
        }
    }
    return true;
}

/*
void CGarage::CenterCarInGarage(CEntity* entity) {
    auto vehicle = FindPlayerVehicle();
    if (IsAnyOtherCarTouchingGarage(vehicle))
        return;

    auto player = FindPlayerPed();
    if (IsAnyOtherPedTouchingGarage(player))
        return;

    auto pos = entity->GetPosition();

    const auto halfX = (m_fRightCoord + m_fLeftCoord) * 0.5f;
    const auto halfY = (m_fBackCoord + m_fFrontCoord) * 0.5f;
    CVector p1{
        halfX - pos.x,
        halfY - pos.y,
        pos.z - pos.z
    };

    auto dist = p1.Magnitude();
    if (dist >= 0.4f) {
        auto x = halfX - pos.x * 0.4f / dist + pos.x;
        auto y = 0.4f / dist * halfY - pos.y + pos.y;
    } else {
        auto x = halfX;
        auto y = halfY;
    }

    if (!IsEntityEntirelyInside3D(entity, 0.3f))
        entity->SetPosn(entity->GetPosition());
}
*/

// 0x5D3020
void CSaveGarage::CopyGarageIntoSaveGarage(Const CGarage& g) {
    m_nType         = g.m_nType;
    m_nDoorState    = g.m_nDoorState;
    m_nFlags        = g.m_nFlags;
    m_vPosn         = g.m_vPosn;
    m_vDirectionA   = g.m_vDirectionA;
    m_vDirectionB   = g.m_vDirectionB;
    m_fTopZ         = g.m_fTopZ;
    m_fWidth        = g.m_fWidth;
    m_fHeight       = g.m_fHeight;
    m_fLeftCoord    = g.m_fLeftCoord;
    m_fRightCoord   = g.m_fRightCoord;
    m_fFrontCoord   = g.m_fFrontCoord;
    m_fBackCoord    = g.m_fBackCoord;
    m_fDoorPosition = g.m_fDoorPosition;
    m_nTimeToOpen   = g.m_nTimeToOpen;
    m_nOriginalType = g.m_nOriginalType;
    strcpy_s(m_anName, g.m_anName);
}

// 0x5D30C0
void CSaveGarage::CopyGarageOutOfSaveGarage(CGarage& g) const {
    g.m_nType         = m_nType;
    g.m_nDoorState    = m_nDoorState;
    g.m_nFlags        = m_nFlags;
    g.m_vPosn         = m_vPosn;
    g.m_vDirectionA   = m_vDirectionA;
    g.m_vDirectionB   = m_vDirectionB;
    g.m_fTopZ         = m_fTopZ;
    g.m_fWidth        = m_fWidth;
    g.m_fHeight       = m_fHeight;
    g.m_fLeftCoord    = m_fLeftCoord;
    g.m_fRightCoord   = m_fRightCoord;
    g.m_fFrontCoord   = m_fFrontCoord;
    g.m_fBackCoord    = m_fBackCoord;
    g.m_fDoorPosition = m_fDoorPosition;
    g.m_nTimeToOpen   = m_nTimeToOpen;
    g.m_nOriginalType = m_nOriginalType;
    g.m_pTargetCar    = nullptr;
    strcpy_s(g.m_anName, m_anName);
}

// todo move
// 0x449760
void CStoredCar::StoreCar(CVehicle* vehicle) {
    m_wModelIndex = vehicle->m_nModelIndex;
    m_vPosn       = vehicle->GetPosition();

    // Pack the vehicle's forward vector into 3 signed bytes (Scaled by 100 and truncated)
    const auto& forward = vehicle->GetMatrix().GetForward();
    m_nPackedForwardX = static_cast<uint8>(static_cast<int8>(static_cast<int32>(forward.x * 100.0f)));
    m_nPackedForwardY = static_cast<uint8>(static_cast<int8>(static_cast<int32>(forward.y * 100.0f)));
    m_nPackedForwardZ = static_cast<uint8>(static_cast<int8>(static_cast<int32>(forward.z * 100.0f)));

    m_nPrimaryColor    = vehicle->m_nPrimaryColor;
    m_nSecondaryColor  = vehicle->m_nSecondaryColor;
    m_nTertiaryColor   = vehicle->m_nTertiaryColor;
    m_nQuaternaryColor = vehicle->m_nQuaternaryColor;
    m_nRadioStation    = static_cast<uint8>(vehicle->m_vehicleAudio.m_AuSettings.RadioStation);
    m_nHandlingFlags   = vehicle->m_nHandlingFlagsIntValue;
    m_anCompsToUse[0]  = vehicle->m_anExtras[0];
    m_anCompsToUse[1]  = vehicle->m_anExtras[1];

    m_nStoredCarFlags = 0; // Original does a 16-bit store here, which also zeroes the padding byte
    _pad0             = 0;

    const auto physicalFlags = vehicle->m_nPhysicalFlags;
    if (physicalFlags & 0x40000) {  // bBulletProof
        m_nStoredCarFlags |= 0x01;
    }
    if (physicalFlags & 0x80000) {  // bFireProof
        m_nStoredCarFlags |= 0x02;
    }
    if (physicalFlags & 0x800000) { // bExplosionProof
        m_nStoredCarFlags |= 0x04;
    }
    if (physicalFlags & 0x100000) { // bCollisionProof
        m_nStoredCarFlags |= 0x08;
    }
    if (physicalFlags & 0x200000) { // bMeleeProof
        m_nStoredCarFlags |= 0x10;
    }
    if (vehicle->vehicleFlags.bUpgradedStereo) {
        m_nStoredCarFlags |= 0x20;
    }
    if (vehicle->m_nHandlingFlagsIntValue & VEHICLE_HANDLING_HYDRAULIC_INST) {
        m_nStoredCarFlags |= 0x40;
    }
    if (vehicle->m_nHandlingFlagsIntValue & VEHICLE_HANDLING_NOS_INST) {
        m_nStoredCarFlags |= 0x80;
    }

    if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE || vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        m_nBombType = vehicle->m_nBombOnBoard & 7;
    }

    for (auto i = 0; i < CVehicle::NUM_VEHICLE_UPGRADES; i++) {
        m_awCarMods[i] = vehicle->m_anUpgrades[i];
    }

    m_nPaintJob    = static_cast<uint8>(vehicle->GetRemapIndex());
    m_nNitroBoosts = vehicle->m_nNitroBoosts;
}

// 0x447E40
CVehicle* CStoredCar::RestoreCar() {
    CStreaming::RequestModel(m_wModelIndex, 8);
    for (auto i = 0; i < CVehicle::NUM_VEHICLE_UPGRADES; i++) {
        if (m_awCarMods[i] != -1) {
            CStreaming::RequestVehicleUpgrade(m_awCarMods[i], 0);
        }
    }

    if (CStreaming::ms_aInfoForModel[m_wModelIndex].m_LoadState != LOADSTATE_LOADED) {
        return nullptr;
    }

    for (auto i = 0; i < CVehicle::NUM_VEHICLE_UPGRADES; i++) {
        if (m_awCarMods[i] != -1 && !CStreaming::HasVehicleUpgradeLoaded(m_awCarMods[i])) {
            return nullptr;
        }
    }

    // Consumed by the vehicle's constructor
    CVehicleModelInfo::ms_compsToUse[0] = static_cast<int8>(m_anCompsToUse[0]);
    CVehicleModelInfo::ms_compsToUse[1] = static_cast<int8>(m_anCompsToUse[1]);

    CVehicle* vehicle{};
    switch (CModelInfo::GetModelInfo(m_wModelIndex)->AsVehicleModelInfoPtr()->m_nVehicleType) {
    case VEHICLE_TYPE_MTRUCK:
        vehicle = new CMonsterTruck(m_wModelIndex, RANDOM_VEHICLE);
        break;
    case VEHICLE_TYPE_QUAD:
        vehicle = new CQuadBike(m_wModelIndex, RANDOM_VEHICLE);
        break;
    case VEHICLE_TYPE_HELI:
        vehicle = new CHeli(m_wModelIndex, RANDOM_VEHICLE);
        break;
    case VEHICLE_TYPE_PLANE:
        vehicle = new CPlane(m_wModelIndex, RANDOM_VEHICLE);
        break;
    case VEHICLE_TYPE_BOAT:
        vehicle = new CBoat(m_wModelIndex, RANDOM_VEHICLE);
        break;
    case VEHICLE_TYPE_BIKE:
        vehicle = new CBike(m_wModelIndex, RANDOM_VEHICLE);
        if (vehicle) {
            static_cast<CBike*>(vehicle)->bikeFlags.bOnSideStand = true;
        }
        break;
    case VEHICLE_TYPE_BMX:
        vehicle = new CBmx(m_wModelIndex, RANDOM_VEHICLE);
        if (vehicle) {
            static_cast<CBmx*>(vehicle)->bikeFlags.bOnSideStand = true;
        }
        break;
    case VEHICLE_TYPE_TRAILER:
        vehicle = new CTrailer(m_wModelIndex, RANDOM_VEHICLE);
        break;
    default: // Also handles VEHICLE_TYPE_AUTOMOBILE and any unknown type
        vehicle = new CAutomobile(m_wModelIndex, RANDOM_VEHICLE, true);
        break;
    }

    if (!vehicle) {
        return nullptr;
    }

    vehicle->SetPosn(m_vPosn); // Writes the placement position if there's no matrix

    // Rebuild a (crude) orientation from the packed forward vector
    const auto forward = CVector{
        static_cast<float>(static_cast<int8>(m_nPackedForwardX)) * 0.01f,
        static_cast<float>(static_cast<int8>(m_nPackedForwardY)) * 0.01f,
        static_cast<float>(static_cast<int8>(m_nPackedForwardZ)) * 0.01f
    };
    {
        auto& matrix = vehicle->GetMatrix();
        matrix.GetForward()  = forward;
        matrix.GetRight()    = CVector{ forward.y, -forward.x, 0.0f };
        matrix.GetUp()       = CVector{ 0.0f, 0.0f, 1.0f };
        matrix.GetPosition() = m_vPosn;
    }

    vehicle->SetStatus(STATUS_ABANDONED);
    vehicle->m_pDriver = nullptr;
    vehicle->m_vehicleAudio.m_AuSettings.RadioStation = static_cast<eRadioID>(m_nRadioStation);
    vehicle->m_nHandlingFlagsIntValue                 = static_cast<eVehicleHandlingFlags>(m_nHandlingFlags);

    vehicle->vehicleFlags.bFreebies = false;            // 0x428 bit 7
    vehicle->vehicleFlags.bHasBeenOwnedByPlayer = true;  // 0x42A bit 1
    vehicle->m_nDoorLock = CARLOCK_UNLOCKED;             // 0x4F8 = 1

    if (vehicle->m_nVehicleType == VEHICLE_TYPE_AUTOMOBILE) {
        vehicle->m_nBombOnBoard = m_nBombType & 7;
    }

    // Stored car flags -> vehicle state
    if (m_nStoredCarFlags & 0x01) {
        vehicle->physicalFlags.bBulletProof = true;
    }
    if (m_nStoredCarFlags & 0x02) {
        vehicle->physicalFlags.bFireProof = true;
    }
    if (m_nStoredCarFlags & 0x04) {
        vehicle->physicalFlags.bExplosionProof = true;
    }
    if (m_nStoredCarFlags & 0x08) {
        vehicle->physicalFlags.bCollisionProof = true;
    }
    if (m_nStoredCarFlags & 0x10) {
        vehicle->physicalFlags.bMeleeProof = true;
    }
    if (m_nStoredCarFlags & 0x20) {
        vehicle->vehicleFlags.bUpgradedStereo = true;
        // The original also sets a byte at `CVehicle + 0x1BE` (inside `CAEVehicleAudioEntity`)
        // TODO(re): identify the `CAEVehicleAudioEntity` member at +0x4E and set it by name.
    }
    if (m_nStoredCarFlags & 0x40) {
        vehicle->m_nHandlingFlagsIntValue = static_cast<eVehicleHandlingFlags>(vehicle->m_nHandlingFlagsIntValue | VEHICLE_HANDLING_HYDRAULIC_INST);
    }
    if (m_nStoredCarFlags & 0x80) {
        vehicle->m_nHandlingFlagsIntValue = static_cast<eVehicleHandlingFlags>(vehicle->m_nHandlingFlagsIntValue | VEHICLE_HANDLING_NOS_INST);
    }

    memcpy(vehicle->m_anUpgrades.data(), m_awCarMods, sizeof(m_awCarMods));
    vehicle->SetupUpgradesAfterLoad();
    vehicle->SetRemap(m_nPaintJob);
    vehicle->vehicleFlags.bEngineOn = false; // 0x428 bit 4
    vehicle->m_nNitroBoosts = m_nNitroBoosts;
    vehicle->m_nPrimaryColor    = m_nPrimaryColor;
    vehicle->m_nSecondaryColor  = m_nSecondaryColor;
    vehicle->m_nTertiaryColor   = m_nTertiaryColor;
    vehicle->m_nQuaternaryColor = m_nQuaternaryColor;
    vehicle->vehicleFlags.bDontSetColourWhenRemapping = true; // 0x42F bit 2

    return vehicle;
}

// 0x449FF0
void CGarage::FindDoorsWithGarage(CObject** ppFirstDoor, CObject** ppSecondDoor) {
    *ppFirstDoor  = nullptr;
    *ppSecondDoor = nullptr;

    // The garage index is derived from `this` (Which is an element of `CGarages::aGarages`)
    const auto myId = static_cast<int8>(this - &CGarages::aGarages[0]);

    // Center of the garage
    const auto centreX = m_vPosn.x + m_vDirectionA.x * (m_fWidth * 0.5f) + m_vDirectionB.x * (m_fHeight * 0.5f);
    const auto centreY = m_vPosn.y + m_vDirectionA.y * (m_fWidth * 0.5f) + m_vDirectionB.y * (m_fHeight * 0.5f);

    float bestDistance1 = 99999.9f;
    float bestDistance2 = 99999.9f;

    const auto pool = GetObjectPool();
    for (auto i = static_cast<int32>(pool->GetSize()) - 1; i > 0; i--) { // Note: Slot 0 is never visited
        if (pool->IsFreeSlotAtIndex(i)) {
            continue;
        }

        auto* door = pool->GetAt(i);
        if (!door) {
            continue;
        }

        if (door->m_nGarageDoorGarageIndex != myId) {
            continue;
        }

        const auto& pos = door->GetPosition();
        const auto dx = centreX - pos.x;
        const auto dy = centreY - pos.y;
        const auto distance = std::sqrt(dx * dx + dy * dy); // 2D distance

        if (!*ppFirstDoor) {
            *ppFirstDoor  = door;
            bestDistance1 = distance;
        } else if (distance < bestDistance1) {
            *ppSecondDoor = *ppFirstDoor;
            bestDistance2 = bestDistance1;

            *ppFirstDoor  = door;
            bestDistance1 = distance;
        } else if (!*ppSecondDoor || distance < bestDistance2) {
            *ppSecondDoor = door;
            bestDistance2 = distance;
        }
    }
}
