#include "StdInc.h"

#include "EventScanner.h"

#include "Acquaintance.h"
#include "InterestingEvents.h"
#include "ObjectScanner.h"
#include "PedStats.h"

#include "Tasks/TaskTypes/TaskSimpleGoTo.h"
#include "Tasks/TaskTypes/TaskComplexKillPedOnFoot.h"

#include "Events/EventAcquaintancePedDislike.h"
#include "Events/EventAcquaintancePedHate.h"
#include "Events/EventAcquaintancePedHateBadlyLit.h"
#include "Events/EventAcquaintancePedLike.h"
#include "Events/EventAcquaintancePedRespect.h"
#include "Events/EventAreaCodes.h"
#include "Events/EventDamage.h"
#include "Events/EventEscalator.h"
#include "Events/EventFireNearby.h"
#include "Events/EventInAir.h"
#include "Events/EventOnFire.h"
#include "Events/EventPedEnteredMyVehicle.h"
#include "Events/EventPotentialWalkIntoFire.h"
#include "Events/EventPotentialWalkIntoObject.h"
#include "Events/EventPotentialWalkIntoPed.h"
#include "Events/EventPotentialWalkIntoVehicle.h"
#include "Events/EventSeenCop.h"
#include "Events/EventSexyPed.h"

// 0x605300
CEventScanner::CEventScanner() {
    m_vehiclePotentialCollisionScanner.m_timer = {};
    m_objectPotentialCollisionScanner.m_timer  = {};

    m_attractorScanner.field_0        = true;
    m_attractorScanner.field_4        = {};
    m_attractorScanner.m_pEffectInUse = nullptr;
    m_attractorScanner.m_pEntityInUse = nullptr;
    m_attractorScanner.Clear();

    m_pedAcquaintanceScanner.m_timer                    = {};
    m_pedAcquaintanceScanner.m_bScanAllowedScriptPed    = true;
    m_pedAcquaintanceScanner.m_bScanAllowedInVehicle    = false;
    m_pedAcquaintanceScanner.m_bScanAllowedScriptedTask = false;

    m_sexyPedScanner.m_timer    = {};
    m_nearbyFireScanner.m_timer = {};

    // NOTE: The original subtracts the random value (So the very first scan happens right away), this keeps the repo's existing `+`
    m_nNextScanTime = CTimer::GetTimeInMS() + CGeneral::GetRandomNumberInRange(3000u); // Originally should be -3000.0f (float value)
}

void CEventScanner::Clear() {
    m_attractorScanner.Clear();
}

// 0x603720
void CVehiclePotentialCollisionScanner::ScanForVehiclePotentialCollisionEvents(const CPed& ped, CEntity** entities, int32 count) {
    UNUSED(entities);
    UNUSED(count);

    m_timer.StartIfNotAlready(500);
    m_timer.Reset();
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(500);

    auto* const intel = ped.GetIntelligence();
    auto* const task  = intel->GetTaskManager().GetSimplestActiveTask();
    if (!task || !CTask::IsGoToTask(task)) {
        return;
    }

    auto* const vehicle = intel->GetVehicleScanner().GetClosestVehicleInRange();
    if (!vehicle) {
        return;
    }

    const auto& col = *vehicle->GetColModel();
    const auto& mat = vehicle->GetMatrix();
    CVector     bbMinW;
    CVector     bbMaxW;
    bbMinW.FromMultiply(mat, col.m_boundBox.m_vecMin);
    bbMaxW.FromMultiply(mat, col.m_boundBox.m_vecMax);

    // Ped must sit within 0.5 of the bbox along the vehicle's up axis.
    // The second compare is `dot(-up, pedPos) - dot(-up, bbMinW)` (`0x858C1C` is -1).
    const auto& pedPos = ped.GetPosition();
    const auto& up     = mat.GetUp();
    if (up.Dot(pedPos) - up.Dot(bbMaxW) >= 0.5f) {
        return;
    }
    if (up.Dot(bbMinW) - up.Dot(pedPos) >= 0.5f) {
        return;
    }

    // 0x86C8DC is 5; trains (`m_nVehicleSubType == 6`) scan at twice that.
    const auto radius = vehicle->m_nVehicleSubType == VEHICLE_TYPE_TRAIN ? 10.0f : 5.0f;
    if (!ped.bHasJustLeftCar && (pedPos - vehicle->GetPosition()).SquaredMagnitude() >= sq(radius)) {
        return;
    }

    float losFraction{};
    if (CPedGeometryAnalyser::GetIsLineOfSightClear(ped, static_cast<CTaskSimpleGoTo*>(task)->m_vecTargetPoint, *vehicle, losFraction)) {
        return;
    }
    if (losFraction <= 0.5f) {
        return;
    }

    CEventPotentialWalkIntoVehicle event{ vehicle, static_cast<int32>(intel->GetMoveStateFromGoToTask()) };
    intel->GetEventGroup().Add(&event, false);
}

// 0x607E30
void CEventScanner::ScanForEvents(CPed& ped) {
    if (CTimer::GetTimeInMS() <= m_nNextScanTime) {
        return;
    }

    auto* const intel = ped.GetIntelligence();

    // --- Per ped scanners (In the original's call order) ---
    m_vehiclePotentialCollisionScanner.ScanForVehiclePotentialCollisionEvents(ped, intel->GetVehicleEntities(), (int32)MAX_NUM_ENTITIES);
    ScanForPedPotentialWalkIntoPedEvents(ped, intel->GetPedScanner().GetClosestPedInRange());
    m_objectPotentialCollisionScanner.ScanForObjectPotentialCollisionEvents(ped);
    m_pedAcquaintanceScanner.ScanForAcquaintanceEvents(ped, intel->GetPedEntities(), (int32)MAX_NUM_ENTITIES);
    m_attractorScanner.ScanForAttractors(ped);
    m_nearbyFireScanner.ScanForNearbyFireEvents(ped);
    intel->m_mentalState.Process(&ped);

    // --- In the air ---
    if (!ped.bIsStanding && (ped.bIsInTheAir || CPedGeometryAnalyser::IsInAir(ped))) {
        CEventInAir event;
        ped.GetEventGroup().Add(&event, false);
    } else if (ped.bIsInTheAir) {
        auto* const currentEvent = intel->GetEventHandler().GetHistory().GetCurrentEvent();
        if (!currentEvent || currentEvent->GetEventType() != EVENT_IN_AIR) {
            ped.bIsInTheAir = false;
        }
    }

    // --- On fire ---
    if (ped.m_pFire) {
        CEventOnFire event;
        auto* const simplestTask = intel->GetTaskManager().GetSimplestActiveTask();
        if (!ped.IsPlayer() && simplestTask && !simplestTask->MakeAbortable(&ped, ABORT_PRIORITY_URGENT, &event)) {
            CWeapon::GenerateDamageEvent(&ped, ped.m_pFire->GetEntityStartedFire(), WEAPON_FLAMETHROWER, 5, PED_PIECE_TORSO, 0);
        } else {
            ped.GetEventGroup().Add(&event, false);
        }
    }

    // --- Sexy ped ---
    m_sexyPedScanner.ScanForSexyPedEvents(ped, intel->GetPedEntities(), (int32)MAX_NUM_ENTITIES);

    // --- Area codes (Only relevant when the ped is after somebody that comes from another area) ---
    CTask* killTask = intel->GetTaskManager().FindTaskByType(TASK_PRIMARY_DEFAULT, TASK_COMPLEX_KILL_PED_ON_FOOT);
    if (!killTask) {
        killTask = intel->GetTaskManager().FindTaskByType(TASK_PRIMARY_PRIMARY, TASK_COMPLEX_KILL_PED_ON_FOOT);
    }
    if (!killTask) {
        killTask = intel->GetTaskManager().FindTaskByType(TASK_PRIMARY_EVENT_RESPONSE_TEMP, TASK_COMPLEX_KILL_PED_ON_FOOT);
    }
    if (!killTask) {
        killTask = intel->GetTaskManager().FindTaskByType(TASK_PRIMARY_EVENT_RESPONSE_NONTEMP, TASK_COMPLEX_KILL_PED_ON_FOOT);
    }
    if (killTask) {
        CPed* const target = static_cast<CTaskComplexKillPedOnFoot*>(killTask)->m_target;
        if (target
            && target->m_pContactEntity
            && ped.m_pContactEntity
            && target->m_pContactEntity->GetAreaCode() != ped.m_pContactEntity->GetAreaCode()
        ) {
            CEventAreaCodes event{ target };
            ped.GetEventGroup().Add(&event, false);
        }
    }

    // --- Escalator ---
    if (ped.m_pContactEntity) {
        const auto modelId = ped.m_pContactEntity->m_nModelIndex;
        if (modelId == ModelIndices::MI_ESCALATORSTEP || modelId == ModelIndices::MI_ESCALATORSTEP8) {
            CEventEscalator event;
            ped.GetEventGroup().Add(&event, false);
        }
    }

    // --- Passenger in the player's boat ---
    if (ped.bInVehicle && ped.m_pVehicle && ped.m_pVehicle->m_nVehicleType == VEHICLE_TYPE_BOAT && !ped.IsPlayer()) {
        CPlayerPed* const player = FindPlayerPed(-1);
        // NOTE: The original compares against the player's `m_pContactEntity` (Offset 0x584), not `m_pVehicle`
        if (player && player->m_pContactEntity == ped.m_pVehicle) {
            CEventPedEnteredMyVehicle event{ player, ped.m_pVehicle, TARGET_DOOR_DRIVER };
            event.m_TaskId = TASK_COMPLEX_LEAVE_CAR_AND_FLEE; // 0x2C2, as the original writes it
            ped.GetEventGroup().Add(&event, false);
        }
    }

    // --- Dead ped walking (Health is negative, but it's still alive) ---
    if (ped.m_fHealth < 0.0f && ped.IsAlive() && m_sDeadPedWalkingTimer < CTimer::GetTimeInMS()) {
        CEventDamage event{ nullptr, 0, WEAPON_UNIDENTIFIED, PED_PIECE_TORSO, 0, false, false };
        if (!ped.GetEventGroup().HasEventOfType(&event)) {
            CWeapon::GenerateDamageEvent(&ped, nullptr, WEAPON_FALL, 10, PED_PIECE_TORSO, 0);
            m_sDeadPedWalkingTimer = CTimer::GetTimeInMS() + 2000;
        }
    }
}

// 0x6053D0
void CEventScanner::ScanForEventsNow(const CPed& ped, bool bDontScan) {
    if (bDontScan) {
        return;
    }

    auto* scanner = &m_vehiclePotentialCollisionScanner;
    if (scanner->m_timer.m_bStarted) {
        scanner->m_timer.m_nStartTime = CTimer::GetTimeInMS();
        scanner->m_timer.m_nInterval = -1;
        scanner->m_timer.m_bStarted = true;
    }
    scanner->ScanForVehiclePotentialCollisionEvents(ped, ped.GetIntelligence()->GetVehicleEntities(), 16);
}

// 0x606580
// NOTE: The original's receiver is dead (It's called as a `__thiscall`, but no scanner state is used), so it's static here
void CEventScanner::ScanForPedPotentialWalkIntoPedEvents(CPed& ped, CEntity* closestPed) {
    if (!closestPed) {
        return;
    }
    if (!ped.m_bUsesCollision) {
        return;
    }

    auto* const intel = ped.GetIntelligence();

    auto* const task = intel->GetTaskManager().GetSimplestActiveTask();
    if (!task || !CTask::IsGoToTask(task)) {
        return;
    }
    if (intel->GetMoveStateFromGoToTask() == PEDMOVE_STILL) {
        return;
    }

    auto* const       goTo   = static_cast<CTaskSimpleGoTo*>(task);
    const auto        pedPos = ped.GetPosition();
    const auto&       pedFwd = ped.GetForward();

    auto  bestDistSq = sq(2.5f);
    CPed* bestPed    = nullptr;

    for (auto& other : intel->GetPedScanner().GetEntities<CPed>()) {
        if (!other.IsAlive() || !other.m_bUsesCollision) {
            continue;
        }

        const CVector dir = other.GetPosition() - pedPos;
        if (DotProduct(dir, pedFwd) <= 0.0f) { // Not in front of us
            continue;
        }

        CColSphere sphere;
        sphere.Set(0.7f, other.GetPosition());
        CVector hit1, hit2;
        if (!sphere.IntersectEdge(pedPos, goTo->m_vecTargetPoint, hit1, hit2)) { // Not in our way
            continue;
        }

        const auto distSq = dir.SquaredMagnitude();
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestPed    = &other;
        }
    }

    if (!bestPed) {
        return;
    }

    //! Both walking in the same direction? Then only report it when they're about to run into each other
    auto* const bestTask = bestPed->GetIntelligence()->GetTaskManager().GetSimplestActiveTask();
    if (bestTask && CTask::IsGoToTask(bestTask) && DotProduct(bestPed->GetForward(), pedFwd) >= 0.92388f) { // cos(22.5°)
        CVector selfVel  = ped.m_vecMoveSpeed * 50.0f;
        CVector otherVel = bestPed->m_vecMoveSpeed * 50.0f;
        selfVel.z  = 0.0f;
        otherVel.z = 0.0f;

        const auto lenSelf  = selfVel.Magnitude();
        const auto lenOther = otherVel.Magnitude() + 0.25f;
        if (bestDistSq > 1.0f && !approxEqual(lenOther, 0.0f, 0.01f) && lenSelf < lenOther) {
            return;
        }
    }

    CEventPotentialWalkIntoPed event{ bestPed, goTo->m_vecTargetPoint, intel->GetMoveStateFromGoToTask() };
    ped.GetEventGroup().Add(&event, false);
}

// 0x606890
void CObjectPotentialCollisionScanner::ScanForObjectPotentialCollisionEvents(CPed& ped) {
    m_timer.StartIfNotAlready(500);
    m_timer.Reset();
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(500);

    auto* const intel = ped.GetIntelligence();

    auto*           task      = intel->GetTaskManager().GetSimplestActiveTask();
    const eMoveState moveState = task && CTask::IsGoToTask(task)
        ? static_cast<CTaskSimpleGoTo*>(task)->m_moveState
        : PEDMOVE_STILL;

    if (ped.IsPlayer()) {
        return;
    }
    if (moveState == PEDMOVE_STILL) {
        return;
    }

    task = intel->GetTaskManager().GetSimplestActiveTask();
    if (!task || !CTask::IsGoToTask(task)) {
        return;
    }

    CObjectScanner scanner;
    scanner.ScanForObjectsInRange(ped);
    CObject* const object = scanner.GetClosestObjectInRange();
    if (!object) {
        return;
    }
    if (object->objectFlags.bIsBroken || object->objectFlags.bIsPickup) {
        return;
    }
    if (!object->m_bUsesCollision) {
        return;
    }

    const CVector diff = object->GetPosition() - ped.GetPosition();
    if (diff.SquaredMagnitude() >= sq(7.5f)) {
        return;
    }

    // The object must be within 1 unit (Vertically) of the ped (Note: The original uses `max.z` on both sides)
    const CVector centre  = object->GetBoundCentre();
    const auto    halfH   = object->GetColModel()->m_boundBox.m_vecMax.z;
    const auto    pedPosZ = ped.GetPosition().z;
    if (!(pedPosZ - 1.0f <= centre.z + halfH && centre.z - halfH <= pedPosZ + 1.0f)) {
        return;
    }

    float losFraction{};
    if (CPedGeometryAnalyser::GetIsLineOfSightClear(ped, static_cast<CTaskSimpleGoTo*>(task)->m_vecTargetPoint, *object, losFraction)) {
        return;
    }
    if (losFraction <= 0.5f) {
        return;
    }

    const CVector losStart = ped.GetPosition() - CVector{ 0.0f, 0.0f, 0.75f };
    if (CPedGeometryAnalyser::GetIsLineOfSightClear(losStart, static_cast<CTaskSimpleGoTo*>(task)->m_vecTargetPoint, *object)) {
        return;
    }

    CEventPotentialWalkIntoObject event{ object, static_cast<int32>(moveState) };
    ped.GetEventGroup().Add(&event, false);
}

// 0x607D80
void CPedAcquaintanceScanner::ScanForAcquaintanceEvents(CPed& ped, CEntity** entities, int32 count) {
    m_timer.StartIfNotAlready(500);
    m_timer.Reset();
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(500);

    if (!CanScanAcquaintanceEvents(ped)) {
        return;
    }

    // NOTE: The out values are never read by this function (All work happens in `FindAcquaintanceEventTarget`)
    CPed* targetPed{};
    int32 acquaintanceId{ -1 };
    FindAcquaintanceEventTarget(ped, -1, entities, count, &targetPed, &acquaintanceId);
}

// 0x603A30
bool CPedAcquaintanceScanner::CanScanAcquaintanceEvents(CPed& ped) const {
    if (!ped.IsAlive()) {
        return false;
    }

    // Script peds are only scanned when explicitly allowed to
    if (ped.IsCreatedByMission() && !m_bScanAllowedScriptPed) {
        const auto status = CPedScriptedTaskRecord::GetStatus(&ped);
        if ((status == eScriptedTaskStatus::EVENT_ASSOCIATED || !m_bScanAllowedScriptedTask)
            && !(ped.bInVehicle && m_bScanAllowedInVehicle)
        ) {
            return false;
        }
    }

    // Already hating somebody? Then only look for a new target if the ped can inform its respected friends about it
    auto* const currentEvent = ped.GetEventHandler().GetHistory().GetCurrentEvent();
    if (!currentEvent || currentEvent->GetEventType() != EVENT_ACQUAINTANCE_PED_HATE) {
        return true;
    }
    return ped.GetIntelligence()->m_nDmNumPedsToScan != 0
        && static_cast<CEventEditableResponse*>(currentEvent)->ComputeResponseTaskOfType(&ped, TASK_SIMPLE_INFORM_RESPECTED_FRIENDS)
        && ped.GetIntelligence()->FindRespectedFriendInInformRange();
}

// 0x603AF0
//! Can the ped (Scanner) treat `other` as a riot target? (Only used when riots are active)
static bool IsRiotAcquaintanceTarget(CPed& ped, CPed& other) {
    const auto pedType = (int32)ped.m_nPedType;
    if (pedType == PED_TYPE_COP || pedType == PED_TYPE_DEALER || pedType == PED_TYPE_MEDIC) {
        return false;
    }
    if (ped.IsPlayer() || ped.IsCreatedByMission()) {
        return false;
    }

    const auto& intel = *ped.GetIntelligence();
    if (other.IsPlayer()) {
        return !intel.Respects(&other);
    }

    if (const auto* const group = CPedGroups::GetPedsGroup(&other)) {
        if (CPed* const leader = group->GetMembership().GetLeader()) {
            if (leader->IsPlayer()) {
                return !intel.Respects(leader);
            }
        }
    }

    // Ped's of the same gang don't attack each other
    if (IsPedTypeGang(ped.m_nPedType) && IsPedTypeGang(other.m_nPedType) && ped.m_nPedType == other.m_nPedType) {
        return false;
    }

    return true;
}

// 0x6042B0
//! Does the ped's decision maker have any response for the given event types?
//! NOTE: This belongs to `CDecisionMakerTypes` (`DecisionMakerTypes.cpp`), but as it's not implemented there
//!       (And this file isn't allowed to edit it), a local copy lives here.
static bool HasAnyResponseForEventTypes(CPed& ped, std::span<const eEventType> eventTypes) {
    auto* const dmTypes = CDecisionMakerTypes::GetInstance();
    const auto  dmType  = ped.GetIntelligence()->m_nDecisionMakerType;

    for (const auto eventType : eventTypes) {
        const auto eventIdx = dmTypes->m_EventIndices[+eventType];

        const CDecisionMaker* decisionMaker;
        if (dmType == -2) { // Player
            decisionMaker = &dmTypes->m_DefaultPlayerPedDecisionMaker;
        } else if (dmType == -1) { // One of the defaults
            decisionMaker = ped.IsCreatedByMission()
                ? &dmTypes->m_DefaultMissionPedDecisionMaker
                : &dmTypes->m_DefaultRandomPedDecisionMaker;
        } else {
            decisionMaker = &dmTypes->m_DecisionMakers[dmType];
        }

        if (decisionMaker->m_aDecisions[eventIdx].HasResponse()) {
            return true;
        }
    }
    return false;
}

// 0x607A90
void CPedAcquaintanceScanner::FindAcquaintanceEventTarget(CPed& ped, int32 filter, CEntity** entities, int32 count, CPed** outPed, int32* outAcquaintanceId) {
    *outPed = nullptr;

    std::array<CPed*, MAX_NUM_ENTITIES> candidates;
    int32 numCandidates{};

    // NOTE: The original passes these 4 event types along with a 5th, uninitialised stack slot
    const eEventType eventTypes[] = {
        EVENT_ACQUAINTANCE_PED_HATE,    // 0x24
        EVENT_ACQUAINTANCE_PED_DISLIKE, // 0x25
        EVENT_ACQUAINTANCE_PED_RESPECT, // 0x27
        eEventType(0x28),               // Not an actual event type (Kept as the original has it)
    };

    for (int32 i = 0; i < count; i++) {
        CPed* const other = static_cast<CPed*>(entities[i]);
        if (!other || !other->IsAlive()) {
            continue;
        }

        // Only peds that are in front of us (Or that are in a vehicle) are of interest
        CVector dir = other->GetPosition() - ped.GetPosition();
        dir.Normalise();
        if (DotProduct(dir, ped.GetForward()) <= ms_fThresholdDotProduct && !ped.bInVehicle && !other->bInVehicle) {
            continue;
        }

        bool isAcquaintance = filter == -1 && other->m_nPedType == PED_TYPE_COP;
        if (!isAcquaintance) {
            for (auto id = 4; id >= 0; id--) { // 0 - Respect, 4 - Hate
                if (filter != -1 && filter != id) {
                    continue;
                }
                if (ped.m_acquaintance.GetAcquaintances((AcquaintanceId)id) & CPedType::GetPedFlag(other->m_nPedType)) {
                    isAcquaintance = true;
                    break;
                }
            }
            if (!isAcquaintance && (!CGameLogic::LaRiotsActiveHere() || !IsRiotAcquaintanceTarget(ped, *other))) {
                continue;
            }
        }

        // Non-cop peds must have a response for the acquaintance events
        if (other->m_nPedType != PED_TYPE_COP) {
            const bool isRiotTarget = CGameLogic::LaRiotsActiveHere() && IsRiotAcquaintanceTarget(ped, *other);
            if (!isRiotTarget && !HasAnyResponseForEventTypes(ped, eventTypes)) {
                continue;
            }
        }

        if (numCandidates >= (int32)std::size(candidates)) { // NOTE: Not in the original (Which would overflow the stack array)
            break;
        }
        candidates[numCandidates++] = other;
    }

    int32 currentAcquaintanceId = -1;
    for (int32 i = 0; i < numCandidates; i++) {
        if (currentAcquaintanceId == 4) { // Once a hate event has been created nobody else is checked
            continue;
        }

        CPed* const candidate = candidates[i];

        // Only peds we can actually see (And reach) are considered
        if (!CPedGeometryAnalyser::CanPedTargetPed(ped, *candidate, !(ped.bInVehicle || candidate->bInVehicle))) {
            continue;
        }

        currentAcquaintanceId = ProcessAcquaintanceCandidate(ped, filter, currentAcquaintanceId, *candidate, outPed, outAcquaintanceId);
    }
}

// 0x606BA0
//! Create (And add to the appropriate event group) the event matching the given acquaintance type.
//! @param ped  The ped that's scanning
//! @param other The ped the event is about
static bool CreateAcquaintanceEvent(CPed& ped, int32 acquaintanceId, CPed& other) {
    switch (acquaintanceId) {
    case 0: { // Respect
        CEventAcquaintancePedRespect event{ &other };
        return ped.GetEventGroup().Add(&event, false) != nullptr;
    }
    case 1: { // Like
        CEventAcquaintancePedLike event{ &other };
        return ped.GetEventGroup().Add(&event, false) != nullptr;
    }
    case 2: { // Seen cop
        if (other.m_nPedType != PED_TYPE_COP) {
            return false;
        }
        CEventSeenCop event{ &other };
        return ped.GetEventGroup().Add(&event, false) != nullptr;
    }
    case 3: { // Dislike
        CEventAcquaintancePedDislike event{ &other };
        return ped.GetEventGroup().Add(&event, false) != nullptr;
    }
    case 4: { // Hate
        const auto visibility = ped.GetIntelligence()->CanSeeEntityWithLights(&other, 0);
        if (visibility == 0.0f) {
            return false;
        }
        if (visibility < 0.0f) { // Can't see them properly (Too dark)
            CEventAcquaintancePedHateBadlyLit event{ &other, (int32)CTimer::GetTimeInMS(), other.GetPosition() };
            return ped.GetEventGroup().Add(&event, false) != nullptr;
        }

        CEventAcquaintancePedHate event{ &other };
        if (!CCheat::IsActive(CHEAT_HAVE_ABOUNTY_ON_YOUR_HEAD) && (!CGameLogic::LaRiotsActiveHere() || !IsRiotAcquaintanceTarget(ped, other))) {
            return ped.GetEventGroup().Add(&event, false) != nullptr;
        }

        // Riots (Or the "everybody attacks player" cheat) - Tell the whole group about it
        if (auto* const group = CPedGroups::GetPedsGroup(&ped)) {
            event.m_TaskId = TASK_GROUP_KILL_PLAYER_BASIC;
            return group->GetIntelligence().AddEvent(&event);
        }
        event.m_TaskId = TASK_COMPLEX_KILL_PED_ON_FOOT;
        return ped.GetEventGroup().Add(&event, false) != nullptr;
    }
    default:
        return false;
    }
}

// 0x607560
int32 CPedAcquaintanceScanner::ProcessAcquaintanceCandidate(CPed& ped, int32 filter, int32 currentAcquaintanceId, CPed& candidate, CPed** outPed, int32* outAcquaintanceId) {
    const auto visibility = ped.GetIntelligence()->CanSeeEntityWithLights(&candidate, 0);

    for (auto id = 4; id >= 0; id--) { // 0 - Respect, 4 - Hate
        if (id == currentAcquaintanceId) {
            return -1;
        }

        bool isAcquaintance;
        if (filter == -1) {
            // Cops are always accepted as "seen" when no specific type was requested
            isAcquaintance = id == 2 && candidate.m_nPedType == PED_TYPE_COP;
            if (!isAcquaintance) {
                isAcquaintance = (ped.m_acquaintance.GetAcquaintances((AcquaintanceId)id) & CPedType::GetPedFlag(candidate.m_nPedType)) != 0
                              || (CGameLogic::LaRiotsActiveHere() && IsRiotAcquaintanceTarget(ped, candidate));
            }
        } else if (filter == id) {
            isAcquaintance = (ped.m_acquaintance.GetAcquaintances((AcquaintanceId)id) & CPedType::GetPedFlag(candidate.m_nPedType)) != 0
                          || (CGameLogic::LaRiotsActiveHere() && IsRiotAcquaintanceTarget(ped, candidate));
        } else {
            continue; // Not the requested type
        }
        if (!isAcquaintance) {
            continue;
        }

        // The ped must at least be somewhat visible to us
        if (visibility > 0.0f || (id == 4 && visibility != 0.0f)) {
            *outPed             = &candidate;
            *outAcquaintanceId  = id;

            if (filter != -1) {
                return filter;
            }

            const bool added = CreateAcquaintanceEvent(ped, id, candidate);

            // Hate targets that we can't see properly are re-checked more often
            m_timer.m_nStartTime = CTimer::GetTimeInMS();
            m_timer.m_nInterval  = (id == 4 && visibility < 0.0f) ? 200 : 3000;
            m_timer.m_bStarted   = true;

            if (added) {
                return id;
            }
        }
    }

    return -1;
}

// 0x603E70
void CNearbyFireScanner::ScanForNearbyFireEvents(CPed& ped) {
    m_timer.StartIfNotAlready(0);
    m_timer.Reset();
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(100);

    auto* const intel = ped.GetIntelligence();

    auto* const task = intel->GetTaskManager().GetSimplestActiveTask();
    const eMoveState moveState = task && CTask::IsGoToTask(task)
        ? static_cast<CTaskSimpleGoTo*>(task)->m_moveState
        : PEDMOVE_STILL;

    auto* const activeTask = intel->GetTaskManager().GetActiveTask();

    const CVector& pos  = ped.GetPosition();
    CFire* const   fire = gFireManager.FindNearestFire(pos, false, false);

    auto distSq = 0.0f;
    auto dz     = 0.0f;
    if (fire) {
        const CVector diff = fire->GetPosition() - pos;
        dz     = diff.z;
        distSq = diff.SquaredMagnitude();
        if (distSq < sq(20.0f) && std::abs(dz) < 2.0f) {
            CEventFireNearby event{ fire->GetPosition() };
            ped.GetEventGroup().Add(&event, false);
        }
    }

    if (activeTask
        && activeTask->GetTaskType() != TASK_COMPLEX_WALK_ROUND_FIRE
        && task && CTask::IsGoToTask(task)
        && fire
        && distSq < sq(4.0f)
        && std::abs(dz) < 2.0f
    ) {
        CEventPotentialWalkIntoFire event{ fire->GetPosition(), fire->GetStrength(), moveState };
        ped.GetEventGroup().Add(&event, false);
    }
}

// 0x603BF0
void CSexyPedScanner::ScanForSexyPedEvents(CPed& ped, CEntity** entities, int32 count) {
    m_timer.StartIfNotAlready(500);
    m_timer.Reset();
    if (!m_timer.IsOutOfTime()) {
        return;
    }
    m_timer.Start(500);

    if (!ped.IsCreatedByMission() && ped.bInVehicle) {
        return;
    }

    const CVector& pos = ped.GetPosition();

    auto  bestDistSq = 1e10f;
    CPed* bestPed    = nullptr;

    for (int32 i = 0; i < count; i++) {
        CPed* const other = static_cast<CPed*>(entities[i]);
        if (!other) {
            continue;
        }
        if (other->m_nPedType != PED_TYPE_CIVFEMALE) {
            continue;
        }
        if (ped.m_pStats->m_nSexiness >= other->m_pStats->m_nSexiness) {
            continue;
        }
        if (other->bInVehicle) {
            continue;
        }

        const CVector dir    = other->GetPosition() - pos;
        const auto    distSq = dir.SquaredMagnitude();
        if (distSq >= sq(100.0f)) {
            continue;
        }

        if (ped.GetPlayerData()) {
            g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_9, other);
        }

        if (distSq < bestDistSq
            && DotProduct(dir, ped.GetForward()) > 0.0f
            && CWorld::GetIsLineOfSightClear(pos, other->GetPosition(), true, false, false, true, false, false, false)
        ) {
            bestDistSq = distSq;
            bestPed    = other;
        }
    }

    if (bestPed) {
        CEventSexyPed event{ bestPed };
        ped.GetEventGroup().Add(&event, false);

        // Nothing else is scanned for a while
        m_timer.m_nStartTime = CTimer::GetTimeInMS();
        m_timer.m_nInterval  = 3000;
        m_timer.m_bStarted   = true;
    }
}
