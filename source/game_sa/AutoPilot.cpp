#include "StdInc.h"

#include "AutoPilot.h"
#include "Curves.h"

// 0x6D5E20
CAutoPilot::CAutoPilot() : m_aPathFindNodesInfo() {
    _smthNext = 1;
    _smthCurr = 1;

    m_nCarCtrlFlags = 0;

    field_C = 0;
    m_nSpeedScaleFactor = 1000;
    m_nNextLane = 0;
    m_nCurrentLane = 0;
    m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    m_nCarMission = eCarMission::MISSION_NONE;
    m_nTempAction = TEMPACT_NONE;
    SetCruiseSpeed(10);
    m_speed = 10.0F;
    m_nPathFindNodesCount = 0;
    m_TargetEntity = nullptr;

    m_nTimeToStartMission = CTimer::GetTimeInMS();
    m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
    m_LastUpdateTimeMs = 0;

    m_nStraightLineDistance = 20;
    m_ucTempActionMode = 0;
    m_ucCarMissionModeCounter = 0;
    field_41 = 0;
    m_SpeedMult = 1.0f;
    m_ucHeliSpeedMult = 0;
    movementFlags.bIsStopped = false;
    movementFlags.bIsParked = false;
    field_4A = 0;
    m_ucCarFollowDist = 10;
    m_ucHeliTargetDist2 = 10;
    field_50 = CGeneral::GetRandomNumber() % 8 + 2;
    m_vehicleRecordingId = -1;
    m_bPlaneDogfightSomething = false;
    m_ObstructingEntity = nullptr;
    m_fMaxTrafficSpeed = 0.0F;
}

// 0x41B980
void CAutoPilot::ModifySpeed(float target) {
    // `!(target > 0.01f)` rather than `target <= 0.01f`: the original's `FCOM`/`TEST AH,0x41` also
    // clamps when the comparison is unordered (NaN).
    if (!(target > 0.01f)) {
        target = 0.01f;
    }

    // Scaled time elapsed under the *previous* speed scale factor
    const float scaledTime = (float)(CTimer::GetTimeInMS() - (uint32)field_C) / (float)m_nSpeedScaleFactor;
    m_speed = target;

    if (!ThePaths.m_pPathNodes[m_nCurrentPathNodeInfo.m_wAreaId] || !ThePaths.m_pPathNodes[m_nNextPathNodeInfo.m_wAreaId]) {
        return;
    }

    const CCarPathLink& curr = ThePaths.GetCarPathLink(m_nCurrentPathNodeInfo);
    const CCarPathLink& next = ThePaths.GetCarPathLink(m_nNextPathNodeInfo);

    const float currLaneOffset = (curr.OneWayLaneOffset() + (float)m_nCurrentLane) * 5.4f;
    const float nextLaneOffset = (next.OneWayLaneOffset() + (float)m_nNextLane) * 5.4f;

    const float currDirX = (float)curr.m_dir.x * (float)_smthCurr;
    const float currDirY = (float)curr.m_dir.y * (float)_smthCurr;
    const float nextDirX = (float)next.m_dir.x * (float)_smthNext;
    const float nextDirY = (float)next.m_dir.y * (float)_smthNext;

    const float speedScale = CCurves::CalcSpeedScaleFactor(
        CVector{(float)curr.m_posn.x + currLaneOffset * currDirX, (float)curr.m_posn.y - currLaneOffset * currDirY, 0.0f},
        CVector{(float)next.m_posn.x + nextLaneOffset * nextDirX, (float)next.m_posn.y - nextLaneOffset * nextDirY, 0.0f},
        currDirX, currDirY, nextDirX, nextDirY
    );

    m_nSpeedScaleFactor = (uint32)(1000.0f / m_speed * speedScale);
    field_C             = (int32)((float)CTimer::GetTimeInMS() - (float)m_nSpeedScaleFactor * scaledTime);
}

// 0x41B950
void CAutoPilot::RemoveOnePathNode() {
    --m_nPathFindNodesCount;
    for (int32 i = 0; i < (int16)m_nPathFindNodesCount; i++) {
        m_aPathFindNodesInfo[i] = m_aPathFindNodesInfo[i + 1];
    }
}

void CAutoPilot::SetCarMission(eCarMission carMission, uint32 timeOffsetMs) {
    m_nCarMission = carMission;
    m_nTimeToStartMission = CTimer::GetTimeInMS() + timeOffsetMs;
}

// notsa
void CAutoPilot::SetTempAction(eAutoPilotTempAction action, uint32 durMs) {
    m_nTempAction = action;
    m_nTempActionTime = CTimer::GetTimeInMS() + durMs;
}
