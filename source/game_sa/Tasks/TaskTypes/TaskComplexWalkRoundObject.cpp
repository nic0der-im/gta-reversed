#include "StdInc.h"

#include "TaskComplexWalkRoundObject.h"
#include "TaskComplexFollowPointRoute.h"
#include "TaskComplexGoToPointAndStandStill.h"
#include "TaskSimpleAchieveHeading.h"
#include "TaskSimpleStandStill.h"
#include "PedGeometryAnalyser.h" // CPointRoute
#include "Enums/eMoveState.h"

void CTaskComplexWalkRoundObject::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexWalkRoundObject, 0x86F364, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x655020);
    RH_ScopedInstall(ComputeRoute, 0x6551D0);
    RH_ScopedInstall(CreateRouteTask, 0x655140);
    RH_ScopedInstall(CreateSubTask, 0x655290);

    RH_ScopedVMTInstall(CreateNextSubTask, 0x657220);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x657380);
    RH_ScopedVMTInstall(ControlSubTask, 0x6575F0);
}

// 0x655020
CTaskComplexWalkRoundObject::CTaskComplexWalkRoundObject(int32 moveState, const CVector& targetPoint, CEntity* object) : CTaskComplex() {
    m_moveState   = moveState;
    m_targetPoint = targetPoint;
    m_object      = object;
    field_24      = 0;
    field_28      = 0;
    field_2C      = 0;
    field_2D      = 0;

    CEntity::SafeRegisterRef(m_object);

    m_pointRoute = new CPointRoute();
}

CTaskComplexWalkRoundObject::~CTaskComplexWalkRoundObject() {
    CEntity::SafeCleanUpRef(m_object);

    delete m_pointRoute;
    // todo: m_pointRoute = nullptr;
}

CTaskComplexWalkRoundObject* CTaskComplexWalkRoundObject::Constructor(int32 moveState, const CVector& targetPoint, CEntity* object) {
    this->CTaskComplexWalkRoundObject::CTaskComplexWalkRoundObject(moveState, targetPoint, object);
    return this;
}

// 0x6551D0
float CTaskComplexWalkRoundObject::ComputeRoute(CPed* ped) {
    // NOTE: `CPedGeometryAnalyser::ms_fPedNominalRadius` is private, so it's accessed through its address here (Same storage)
    auto&      pedNominalRadius = StaticRef<float>(0x8D22B0);
    const auto savedRadius      = pedNominalRadius;

    pedNominalRadius = 0.7f;
    CPedGeometryAnalyser::ComputeRouteRoundEntityBoundingBox(*ped, *m_object, m_targetPoint, *m_pointRoute, 0);
    pedNominalRadius = savedRadius;

    std::array<CVector, 4> bbPlaneNormals{};
    std::array<float, 4>   bbPlaneDs{};
    CPedGeometryAnalyser::ComputeEntityBoundingBoxPlanes(ped->GetPosition().z, *m_object, bbPlaneNormals, bbPlaneDs);

    const eDirection pedSide = CPedGeometryAnalyser::ComputeEntityHitSide(*ped, *m_object);
    return bbPlaneNormals[+pedSide].Dot(ped->GetPosition()) + bbPlaneDs[+pedSide];
}

// 0x655290
CTask* CTaskComplexWalkRoundObject::CreateSubTask(eTaskType taskType, CPed* ped) {
    switch (taskType) {
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill(500, false, false, 8.f);
    case TASK_COMPLEX_FOLLOW_POINT_ROUTE:
        return CreateRouteTask(ped);
    default:
        return nullptr;
    }
}

// 0x657220
CTask* CTaskComplexWalkRoundObject::CreateNextSubTask(CPed* ped) {
    if (!m_object) {
        return nullptr;
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL:
        return CreateRouteTask(ped);

    case TASK_SIMPLE_ACHIEVE_HEADING: {
        if (!m_pointRoute->IsEmpty()) {
            return CreateRouteTask(ped);
        }

        // The ped is on the same side of the object as the target point - Just go there
        CVector closestSurfacePt;
        CPedGeometryAnalyser::ComputeClosestSurfacePoint(*ped, *m_object, closestSurfacePt);
        if (CPedGeometryAnalyser::ComputeEntityHitSide(closestSurfacePt, *m_object)
            != CPedGeometryAnalyser::ComputeEntityHitSide(m_targetPoint, *m_object)
        ) {
            return new CTaskComplexGoToPointAndStandStill{ PEDMOVE_WALK, closestSurfacePt, 0.5f, 2.f };
        }

        // Walk around the object
        CVector targetSurfacePt;
        m_pointRoute->AddUnlessFull(closestSurfacePt);
        CPedGeometryAnalyser::ComputeClosestSurfacePoint(m_targetPoint, *m_object, targetSurfacePt);
        m_pointRoute->AddUnlessFull(targetSurfacePt);
        return CreateRouteTask(ped);
    }

    default:
        return nullptr;
    }
}

// 0x657380
CTask* CTaskComplexWalkRoundObject::CreateFirstSubTask(CPed* ped) {
    if (!m_object) {
        return nullptr;
    }

    m_pointRoute->Clear();

    const float routeVal = ComputeRoute(ped);
    if (m_pointRoute->IsEmpty()) {
        return nullptr;
    }

    field_24 = CTimer::GetTimeInMS();
    field_28 = m_moveState == PEDMOVE_WALK ? 8000 : 4000;
    field_2C = true;

    m_objectPos     = m_object->GetPosition();
    const CMatrix& objMat = m_object->GetMatrix();
    m_objectForward = objMat.GetForward();
    m_objectRight   = objMat.GetRight();

    CVector     toFirstPoint   = (*m_pointRoute)[0] - ped->GetPosition();
    const float firstPointDist = toFirstPoint.NormaliseAndMag();

    const float distThreshold = m_moveState == PEDMOVE_WALK
        ? 2.f
        : (m_moveState == PEDMOVE_RUN ? 4.f : 6.f);

    if (firstPointDist > distThreshold
        && routeVal > distThreshold
        && toFirstPoint.Dot(ped->GetMatrix().GetForward()) > 0.f
    ) {
        return CreateRouteTask(ped);
    }

    // The route is too short - Just turn to face the first point of the route instead
    const float heading = CGeneral::LimitRadianAngle(
        CGeneral::GetRadianAngleBetweenPoints(
            (*m_pointRoute)[0].x - ped->GetPosition().x,
            (*m_pointRoute)[0].y - ped->GetPosition().y,
            0.f,
            0.f
        )
    );
    return new CTaskSimpleAchieveHeading{ heading, 1.f, 0.2f };
}

// 0x6575F0
CTask* CTaskComplexWalkRoundObject::ControlSubTask(CPed* ped) {
    const auto abortAndRestart = [&]() -> CTask* {
        if (m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_URGENT, nullptr)) {
            return CreateSubTask(TASK_SIMPLE_STAND_STILL, ped);
        }
        return m_pSubTask;
    };

    if (!m_object) {
        return abortAndRestart();
    }

    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_FOLLOW_POINT_ROUTE && field_2C) {
        if (field_2D) {
            field_24 = CTimer::GetTimeInMS();
            field_2D  = false;
        }
        if (CTimer::GetTimeInMS() >= field_24 + field_28) {
            return abortAndRestart();
        }
    }

    if (ped->GetIntelligence()->m_AnotherStaticCounter > 30) {
        return abortAndRestart();
    }

    if (m_pSubTask->GetTaskType() == TASK_SIMPLE_STAND_STILL) {
        return m_pSubTask;
    }

    // The object has to be (more or less) where it was when the route was computed
    const CVector  objPos = m_object->GetPosition();
    const CMatrix& objMat = m_object->GetMatrix();
    if ((m_objectPos - objPos).SquaredMagnitude() > 0.0625f // `0.25f * 0.25f`
        || m_objectForward.Dot(objMat.GetForward()) < 0.9f
        || m_objectRight.Dot(objMat.GetRight()) < 0.9f
    ) {
        return abortAndRestart();
    }

    return m_pSubTask;
}

// 0x655140
CTask* CTaskComplexWalkRoundObject::CreateRouteTask(CPed*) const {
    if (m_pointRoute->IsEmpty()) {
        return nullptr;
    }
    return new CTaskComplexFollowPointRoute{ (eMoveState)m_moveState, *m_pointRoute };
}
