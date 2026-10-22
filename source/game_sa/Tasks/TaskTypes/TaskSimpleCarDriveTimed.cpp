#include "StdInc.h"

#include "TaskSimpleCarDriveTimed.h"

CTaskSimpleCarDriveTimed* CTaskSimpleCarDriveTimed::Constructor(CVehicle* vehicle, int32 nTime) {
    this->CTaskSimpleCarDriveTimed::CTaskSimpleCarDriveTimed(vehicle, nTime);
    return this;
}

// 0x5FF940
CTaskSimpleCarDriveTimed::CTaskSimpleCarDriveTimed(CVehicle* vehicle, int32 nTime) : CTaskSimpleCarDrive(vehicle, nullptr, false), m_nTimer() {
    m_nTime = nTime;
}

// 0x46F610
bool CTaskSimpleCarDriveTimed::ProcessPed(CPed* ped) {
    m_nTimer.StartIfNotAlready(m_nTime);

    if (m_nTimer.IsOutOfTime()) {
        return true;
    }

    return CTaskSimpleCarDrive::ProcessPed(ped);
}
