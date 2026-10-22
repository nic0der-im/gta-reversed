#include "StdInc.h"

#include "TaskComplexFollowPointRoute.h"
#include "TaskComplexWalkRoundCar.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleStandStill.h"

void CTaskComplexWalkRoundCar::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWalkRoundCar, 0x86f308, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x6541B0);
    RH_ScopedInstall(Destructor, 0x656B00);

    RH_ScopedInstall(SetNewVehicle, 0x654290);
    RH_ScopedInstall(CreateRouteTask, 0x6542E0);
    RH_ScopedInstall(ComputeRouteRoundSmallCar, 0x6544F0);
    RH_ScopedInstall(GoingForDoor, 0x654720);
    RH_ScopedInstall(ComputeRouteRoundBigCar, 0x656BB0);
    RH_ScopedInstall(ComputeRoute, 0x657B80);

    RH_ScopedVMTInstall(Clone, 0x655B00);
    RH_ScopedVMTInstall(GetTaskType, 0x654280);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x656B70);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x658200);
    RH_ScopedVMTInstall(ControlSubTask, 0x654370);
}

// 0x6541B0
CTaskComplexWalkRoundCar::CTaskComplexWalkRoundCar(eMoveState moveState, CVector const& targetPt, CVehicle* vehicle, bool isPedGoingForCarDoor, uint8 forceThisDirectionRoundCar) :
    m_MoveState{(uint8)moveState},
    m_bIsPedGoingForCarDoor{isPedGoingForCarDoor},
    m_ForceThisDirectionRoundCar{forceThisDirectionRoundCar},
    m_TargetPt{targetPt},
    m_Veh{vehicle},
    m_Route{new CPointRoute{}},
    m_EnterCarStartTime{-1}
{
    CEntity::SafeRegisterRef(m_Veh);
}

CTaskComplexWalkRoundCar::CTaskComplexWalkRoundCar(const CTaskComplexWalkRoundCar& o) :
    CTaskComplexWalkRoundCar{
        (eMoveState)o.m_MoveState,
        o.m_TargetPt,
        o.m_Veh,
        o.m_bIsPedGoingForCarDoor,
        o.m_ForceThisDirectionRoundCar
    }
{
}

// 0x656B00
CTaskComplexWalkRoundCar::~CTaskComplexWalkRoundCar() {
    CEntity::SafeCleanUpRef(m_Veh);
    delete m_Route;
}


// 0x654290
void CTaskComplexWalkRoundCar::SetNewVehicle(CVehicle * vehicle, uint8 forceThisDirectionRoundCar) {
    if (notsa::IsFixBugs()) {
        if (vehicle == m_Veh) {
            return;
        }
    }
    CEntity::ChangeEntityReference(m_Veh, vehicle);
    m_ForceThisDirectionRoundCar    = forceThisDirectionRoundCar;
    m_bFirstSubTaskNeedsToBeCreated = true;
    m_Route->Clear();
}

// 0x6542E0
CTask* CTaskComplexWalkRoundCar::CreateRouteTask(CPed*) const {
    if (m_Route->IsEmpty()) {
        return nullptr;
    }
    return new CTaskComplexFollowPointRoute{ (eMoveState)m_MoveState, *m_Route };
}

// 0x6544F0
float CTaskComplexWalkRoundCar::ComputeRouteRoundSmallCar(CPed* ped) {
    std::array<CVector, 4> bbCorners{};
    std::array<CVector, 4> bbPlaneNormals{};
    std::array<float, 4>   bbPlaneDs{};

    const float pedZ = ped->GetPosition().z;
    CPedGeometryAnalyser::ComputeEntityBoundingBoxCorners(pedZ, *m_Veh, bbCorners);
    CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanes(pedZ, *m_Veh, bbPlaneNormals, bbPlaneDs);

    // NOTE: `ComputeEntityHitSide` never returns `-1` (It falls back to `FORWARD`), so there's nothing to check for here
    const eDirection pedSide = CPedGeometryAnalyser::ComputeEntityHitSide(*ped, *m_Veh);
    const float      result  = bbPlaneNormals[+pedSide].Dot(ped->GetPosition()) + bbPlaneDs[+pedSide];

    // When going for a car door, the target point must not be inside the car's bounding box
    if (m_bIsPedGoingForCarDoor) {
        std::array<float, 4> planeDists{};
        int32                side = 0;
        for (; side < (int32)planeDists.size(); side++) {
            planeDists[side] = bbPlaneNormals[side].Dot(m_TargetPt) + bbPlaneDs[side];
            if (planeDists[side] >= 0.f) {
                break;
            }
        }
        if (side == (int32)planeDists.size()) { // The target point is inside of the box - Push it out
            const eDirection targetSide = CPedGeometryAnalyser::ComputeEntityHitSide(m_TargetPt, *m_Veh);
            const float      pushOut    = -planeDists[+targetSide] + 0.05f;
            m_TargetPt += bbPlaneNormals[+targetSide] * pushOut;
        }
    }

    const int32 dir = CPedGeometryAnalyser::ComputeRouteRoundEntityBoundingBox(
        *ped,
        *m_Veh,
        m_TargetPt,
        *m_Route,
        (int8)(m_ForceThisDirectionRoundCar << 4) >> 4 // Sign-extend the 4-bit field (See `ComputeRouteRoundEntityBoundingBox`'s docs)
    );
    m_DirectionGoingRoundCar = (uint8)dir & 0xF;

    return result;
}

// 0x656BB0
float CTaskComplexWalkRoundCar::ComputeRouteRoundBigCar(CPed* ped) {
    // NOTE: `CPedGeometryAnalyser::ms_fPedNominalRadius` is private, so it's accessed through its address here (Same storage)
    auto&      pedNominalRadius = StaticRef<float>(0x8D22B0);
    const auto savedRadius      = pedNominalRadius;

    CVector pedSurfacePt{};
    CVector targetSurfacePt{};

    const eDirection pedSide    = CPedGeometryAnalyser::ComputeEntityHitSide(*ped, *m_Veh);
    const eDirection targetSide = CPedGeometryAnalyser::ComputeEntityHitSide(m_TargetPt, *m_Veh);

    pedNominalRadius = 0.7f;
    CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_Veh, pedSurfacePt);
    CPedGeometryAnalyser::ComputeClosestSurfacePoint(m_TargetPt, *m_Veh, targetSurfacePt);
    pedNominalRadius = savedRadius;

    if (pedSide == targetSide) {
        m_Route->AddUnlessFull(pedSurfacePt);
    } else {
        // Compute the route for the closest surface points, then shift it so it's relative to the original positions
        const CVector savedPedPos = ped->GetPosition();
        const CVector savedTarget = m_TargetPt;

        ped->SetPosn(pedSurfacePt);
        m_TargetPt = targetSurfacePt;
        ComputeRouteRoundSmallCar(ped);
        m_TargetPt = savedTarget;
        ped->SetPosn(savedPedPos);

        const CPointRoute snapshot = *m_Route;
        m_Route->Clear();
        m_Route->AddUnlessFull(pedSurfacePt);
        for (const auto& point : snapshot.GetAll()) {
            m_Route->AddUnlessFull(point);
        }

        pedNominalRadius = 0.7f;
        CPedGeometryAnalyser::ComputeClosestSurfacePoint(m_TargetPt, *m_Veh, targetSurfacePt);
        pedNominalRadius = savedRadius;
    }

    m_Route->AddUnlessFull(targetSurfacePt);

    return 0.f;
}

// 0x657B80
float CTaskComplexWalkRoundCar::ComputeRoute(CPed* ped) {
    return m_Veh->vehicleFlags.bIsBig
        ? ComputeRouteRoundBigCar(ped)
        : ComputeRouteRoundSmallCar(ped);
}

// 0x656B70
CTask* CTaskComplexWalkRoundCar::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_ACHIEVE_HEADING:
        return CreateRouteTask(ped);
    }
    return nullptr;
}

// 0x658200
CTask* CTaskComplexWalkRoundCar::CreateFirstSubTask(CPed* ped) {
    m_Route->Clear();

    const float routeVal = m_Veh->vehicleFlags.bIsBig
        ? ComputeRouteRoundBigCar(ped)
        : ComputeRouteRoundSmallCar(ped);

    if (m_Route->IsEmpty()) {
        return nullptr;
    }

    int32 interval = m_MoveState == PEDMOVE_WALK ? 20'000 : 15'000;
    if (m_Veh->m_pVehicleBeingTowed || m_Veh->m_pTowingVehicle) {
        interval = (int32)((float)interval * 4.f);
    }
    m_Timer.Start(interval);

    const CMatrix& vehMat = m_Veh->GetMatrix();
    m_VehPos      = vehMat.GetPosition();
    m_VehMatFwd   = vehMat.GetForward();
    m_VehMatRight = vehMat.GetRight();

    CVector     toFirstPoint   = (*m_Route)[0] - ped->GetPosition();
    const float firstPointDist = toFirstPoint.NormaliseAndMag();

    const float distThreshold = m_MoveState == PEDMOVE_WALK
        ? 2.f
        : (m_MoveState == PEDMOVE_RUN ? 4.f : 6.f);

    if (ped->IsPlayer()) {
        if (CTask* const enterCarTask = ped->GetIntelligence()->m_TaskMgr.FindTaskByType(TASK_PRIMARY_PRIMARY, TASK_COMPLEX_ENTER_CAR_AS_DRIVER)) {
            // NOTE: `CTaskComplexEnterCar::m_EnterCarStartTime` is protected, so it's read through its offset (0x4C)
            m_EnterCarStartTime = *(int32*)((char*)enterCarTask + 0x4C);
        }
    }

    if (!m_Veh->vehicleFlags.bIsBig
        && firstPointDist > distThreshold
        && routeVal > distThreshold
        && toFirstPoint.Dot(ped->GetMatrix().GetForward()) > 0.f
    ) {
        return CreateRouteTask(ped);
    }

    // The route is too short - Just turn to face the first point of the route instead
    CTaskSimpleStandStill{ 0, false, false, 8.f }.ProcessPed(ped);
    const float heading = CGeneral::LimitRadianAngle(
        CGeneral::GetRadianAngleBetweenPoints(
            (*m_Route)[0].x - ped->GetPosition().x,
            (*m_Route)[0].y - ped->GetPosition().y,
            0.f,
            0.f
        )
    );
    return new CTaskSimpleAchieveHeading{ heading, 1.f, 0.1f };
}

// 0x654370
CTask* CTaskComplexWalkRoundCar::ControlSubTask(CPed* ped) {
    if (m_bFirstSubTaskNeedsToBeCreated) {
        m_bFirstSubTaskNeedsToBeCreated = false;
        return CreateFirstSubTask(ped);
    }

    eTaskType newType     = TASK_NONE;
    bool      quitCarEnter = false;
    if (ped->IsPlayer() && m_Veh && m_EnterCarStartTime != -1) {
        if (CCarEnterExit::IsPlayerToQuitCarEnter(ped, m_Veh, m_EnterCarStartTime, m_pSubTask)
            && m_pSubTask->GetTaskType() == TASK_COMPLEX_FOLLOW_POINT_ROUTE
        ) {
            quitCarEnter = true;
            newType      = TASK_FINISHED;
        }
    }
    if (!quitCarEnter
        && m_pSubTask->GetTaskType() == TASK_COMPLEX_FOLLOW_POINT_ROUTE
        && m_Timer.IsOutOfTime()
    ) {
        newType = TASK_FINISHED;
    }

    // Keep going as-is while the vehicle hasn't moved (And we haven't decided to stop)
    if (m_Veh && newType == TASK_NONE) {
        const CMatrix& vehMat = m_Veh->GetMatrix();
        if (   (m_VehPos - m_Veh->GetPosition()).SquaredMagnitude() <= 0.0625f // `0.25f * 0.25f`
            && vehMat.GetForward().Dot(m_VehMatFwd) >= 0.9f
            && vehMat.GetRight().Dot(m_VehMatRight) >= 0.9f
        ) {
            return m_pSubTask;
        }
    }

    if (!m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
        return m_pSubTask;
    }

    if (quitCarEnter) {
        // Also abort the player's enter car task (Iff that's what they're doing)
        if (CTask* const primaryTask = ped->GetIntelligence()->m_TaskMgr.GetTaskPrimary(TASK_PRIMARY_PRIMARY); primaryTask && primaryTask->GetTaskType() == TASK_COMPLEX_ENTER_CAR_AS_DRIVER) {
            primaryTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
        }
    }

    return nullptr;
}
