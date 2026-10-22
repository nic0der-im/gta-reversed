#include "StdInc.h"

#include "EventScanner.h"

#include "Tasks/TaskTypes/TaskSimpleGoTo.h"
#include "Events/EventPotentialWalkIntoVehicle.h"

// 0x605300
CEventScanner::CEventScanner() {
    m_nNextScanTime = CTimer::GetTimeInMS() + CGeneral::GetRandomNumberInRange(3000u); // Originally should be -3000.0f (float value)
}

void CEventScanner::Clear() {
    m_attractorScanner.Clear();
}

// 0x607E30
void CEventScanner::ScanForEvents(CPed& ped) {
    plugin::CallMethod<0x607E30, CEventScanner*, CPed&>(this, ped);
}

// 0x6053D0
void CEventScanner::ScanForEventsNow(const CPed& ped, bool bDontScan) {
    if (bDontScan)
        return;

    auto scanner = &m_vehiclePotentialCollisionScanner;
    if (scanner->m_timer.m_bStarted) { // todo: inlined?
        scanner->m_timer.m_nStartTime = CTimer::GetTimeInMS();
        scanner->m_timer.m_nInterval = -1;
        scanner->m_timer.m_bStarted = true;
    }
    scanner->ScanForVehiclePotentialCollisionEvents(ped, ped.GetIntelligence()->GetVehicleEntities(), 16);
}

// 0x603720
void CVehiclePotentialCollisionScanner::ScanForVehiclePotentialCollisionEvents(const CPed& ped, CEntity** entities, int32 count) {
    UNUSED(entities, count); // These arguments are unused by the original

    m_timer.StartIfNotAlready(500);
    m_timer.Reset();
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(500);

    const auto intel = ped.GetIntelligence();

    const auto task = intel->GetTaskManager().GetSimplestActiveTask();
    if (!task || !CTask::IsGoToTask(task)) {
        return;
    }

    const auto veh = intel->GetVehicleScanner().GetClosestVehicleInRange();
    if (!veh) {
        return;
    }

    const auto& vehMat = veh->GetMatrix();
    const auto& col    = *veh->GetColModel();

    const auto pedPos = ped.GetPosition();
    const auto dist2D = pedPos - veh->GetPosition();

    CVector bbMinW, bbMaxW;
    bbMinW.FromMultiply(vehMat, col.m_boundBox.m_vecMin);
    bbMaxW.FromMultiply(vehMat, col.m_boundBox.m_vecMax);

    // The ped must be within the vehicle's bounding box along it's `up` axis (0.5 units of tolerance)
    const auto up = vehMat.GetUp();
    if (DotProduct(up, pedPos) - DotProduct(up, bbMaxW) >= 0.5f) {
        return;
    }
    if (DotProduct(up, bbMinW) - DotProduct(up, pedPos) >= 0.5f) {
        return;
    }

    const auto maxDist = veh->m_nVehicleSubType == VEHICLE_TYPE_TRAIN ? 10.f : 5.f;
    if (!ped.bHasJustLeftCar && dist2D.SquaredMagnitude() >= sq(maxDist)) {
        return;
    }

    float losFraction{};
    if (CPedGeometryAnalyser::GetIsLineOfSightClear(ped, static_cast<CTaskSimpleGoTo*>(task)->m_vecTargetPoint, *veh, losFraction)) {
        return;
    }
    if (losFraction <= 0.5f) {
        return;
    }

    CEventPotentialWalkIntoVehicle event{ veh, intel->GetMoveStateFromGoToTask() };
    intel->GetEventGroup().Add(&event, false);
}
