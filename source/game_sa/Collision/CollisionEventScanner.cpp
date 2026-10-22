#include "StdInc.h"

#include "CollisionEventScanner.h"

#include "EventAcquaintancePedHate.h"
#include "EventBuildingCollision.h"
#include "EventDamage.h"
#include "EventObjectCollision.h"
#include "EventPedCollisionWithPed.h"
#include "EventPedCollisionWithPlayer.h"
#include "EventPlayerCollisionWithPed.h"
#include "EventSoundQuiet.h"
#include "EventVehicleCollision.h"

namespace {

//! Thresholds used by the executable (`0x8D239C`..`0x8D23B0`).
constexpr float VEHICLE_IMPULSE_TRAIN_KILL   = 0.0001f; //!< squared speed
constexpr float VEHICLE_IMPULSE_WEAK         = 0.0025f; //!< squared speed
constexpr float OBJECT_INTENSITY_ATTACHED    = 2.0f;    // 0x8D23AC
constexpr float OBJECT_INTENSITY             = 1.0f;    // 0x8D23B0
constexpr float IMPACT_SOUND_MIN_INTENSITY   = 1.0f;    // 0x8D23A0
constexpr float IMPACT_SOUND_MIN_TURN_SPEED  = 0.04f;   // 0x8D23A4
constexpr float IMPACT_SOUND_LOUD_INTENSITY  = 3.0f;    // 0x8D239C
constexpr float IMPACT_DAMAGE_RATIO_MULT     = 10.0f;   // 0x8D23A8

constexpr float COL_PI = 3.1415927f;

//! Last time (in ms) each kind of impact sound was played.
auto& s_LastImpactSoundTime             = StaticRef<uint32>(0xC0B1B0);
auto& s_LastMovingObjectImpactSoundTime = StaticRef<uint32>(0xC0B1B4);

} // namespace

// 0x604500
void CCollisionEventScanner::ScanForCollisionEvents(CPed* victim, CEventGroup* eventGroup) {
    // Retail clears the flag on every exit path.
    m_bAlreadyHitByCar = false;

    CEntity* const hit = victim->m_pDamageEntity;
    if (!hit || victim->m_fDamageIntensity == 0.0f) {
        return;
    }

    // Passed to the collision events as their "move state": the simplest active task's type when
    // that task is a go-to task, otherwise 1 (`CTask::IsGoToTask` is inlined in the executable).
    const auto task = victim->GetTaskManager().GetSimplestActiveTask();
    const int16 moveState = task && CTask::IsGoToTask(task) ? static_cast<int16>(task->GetTaskType()) : int16{1};

    const CVector& impactVelocity = victim->m_vecLastCollisionImpactVelocity;
    const CVector& impactPosn     = victim->m_vecLastCollisionPosn;

    switch (hit->GetType()) {
    case ENTITY_TYPE_BUILDING: {
        CEventBuildingCollision event{
            static_cast<int16>(victim->m_nPieceType),
            victim->m_fDamageIntensity,
            hit->AsBuilding(),
            impactVelocity,
            impactPosn,
            moveState
        };
        eventGroup->Add(&event, false);
        break;
    }
    case ENTITY_TYPE_VEHICLE: {
        if (m_bAlreadyHitByCar) {
            break;
        }

        CVehicle* const vehicle = hit->AsVehicle();

        // The "impulse" is the vehicle's *squared* speed.
        const float impulse = vehicle->m_vecMoveSpeed.SquaredMagnitude();

        // Trains running over a ped kill it outright. Retail reads `CVehicle + 0x594` (which holds
        // a `eVehicleType` value) and `m_nPhysicalFlags & PHYSICAL_DISABLE_COLLISION_FORCE`.
        if (vehicle->m_nVehicleType == VEHICLE_TYPE_TRAIN
            && (vehicle->m_nPhysicalFlags & PHYSICAL_DISABLE_COLLISION_FORCE)
            && victim->m_bIsStuck
            && victim->bIsStanding
            && !victim->m_pAttachedTo
            && impulse > VEHICLE_IMPULSE_TRAIN_KILL
        ) {
            victim->KillPedWithCar(vehicle, 9.0f, false);
        }

        if (impulse <= VEHICLE_IMPULSE_WEAK) {
            // Retail tests `victim->m_pAttachedTo` here (it only needs "attached to something").
            if (!victim->m_pAttachedTo || victim->m_pAttachedTo->GetType() != ENTITY_TYPE_VEHICLE) {
                if (!victim->IsPlayer()) {
                    CEventVehicleCollision event{
                        static_cast<int16>(victim->m_nPieceType),
                        victim->m_fDamageIntensity,
                        vehicle,
                        impactVelocity,
                        impactPosn,
                        static_cast<int8>(victim->GetIntelligence()->GetMoveStateFromGoToTask()),
                        VEHICLE_EVADE_NONE
                    };
                    eventGroup->Add(&event, false);
                }
            } else {
                CPedDamageResponseCalculator dmgRespCalc{
                    vehicle,
                    victim->m_fDamageIntensity,
                    WEAPON_RAMMEDBYCAR,
                    PED_PIECE_TORSO,
                    false
                };
                CEventDamage dmgEvent{
                    vehicle,
                    CTimer::GetTimeInMS(),
                    WEAPON_RAMMEDBYCAR,
                    PED_PIECE_TORSO,
                    static_cast<uint8>(CPedGeometryAnalyser::ComputePedHitSide(*victim, *vehicle)),
                    false,
                    // Retail passes `(m_nPedFlags >> 8) & 0xFFFFFF01` here; the only flag with a
                    // named counterpart is bit 8.
                    victim->bInVehicle
                };
                dmgEvent.ComputeDamageResponseIfAffectsPed(victim, dmgRespCalc, true);
                eventGroup->Add(&dmgEvent, false);
            }
        } else { // Strong impact
            float speed = victim->m_fDamageIntensity;
            if (victim->bIsStanding) {
                const float alongAnimShift = impactVelocity.x * victim->m_vecAnimMovingShift.x
                                           + impactVelocity.y * victim->m_vecAnimMovingShift.y;
                if (alongAnimShift < 0.0f) {
                    speed = std::max(0.0f, alongAnimShift * victim->m_fMass + speed);
                }
            }

            if (!victim->IsPlayer()) {
                if (victim->m_pAttachedTo && victim->m_pAttachedTo->GetType() == ENTITY_TYPE_VEHICLE) {
                    CPedDamageResponseCalculator dmgRespCalc{
                        vehicle,
                        victim->m_fDamageIntensity,
                        WEAPON_RAMMEDBYCAR,
                        PED_PIECE_TORSO,
                        false
                    };
                    CEventDamage dmgEvent{
                        vehicle,
                        CTimer::GetTimeInMS(),
                        WEAPON_RAMMEDBYCAR,
                        PED_PIECE_TORSO,
                        static_cast<uint8>(CPedGeometryAnalyser::ComputePedHitSide(*victim, *vehicle)),
                        false,
                        victim->bInVehicle
                    };
                    dmgEvent.ComputeDamageResponseIfAffectsPed(victim, dmgRespCalc, true);
                    eventGroup->Add(&dmgEvent, false);
                } else {
                    victim->KillPedWithCar(vehicle, victim->m_fDamageIntensity, false);
                }
            } else {
                speed = std::min(speed, 20.0f);

                // Head-on test: the ped's heading relative to the vehicle, compared against the
                // angle of the vehicle's collision box.
                const CColModel& colModel = *vehicle->GetColModel();
                const CVector    boxCentre = (colModel.m_boundBox.m_vecMin + colModel.m_boundBox.m_vecMax) * 2.0f;
                const CVector    boxCentreWorld = vehicle->GetMatrix().TransformPoint(boxCentre);

                const CVector victimToBox = boxCentreWorld - victim->GetPosition();
                const float   boxRelAngle = CGeneral::LimitRadianAngle(
                    victim->GetHeading() - std::atan2(-victimToBox.x, victimToBox.y)
                );

                const float boxAngle = std::atan2(
                    colModel.m_boundBox.m_vecMax.x - colModel.m_boundBox.m_vecMin.x,
                    colModel.m_boundBox.m_vecMax.y - colModel.m_boundBox.m_vecMin.y
                );

                const CVector victimDir = (victim->GetPosition() - vehicle->GetPosition()).Normalized();

                // TODO(re): retail also dots the vehicle's right axis with the victim's up axis
                // inside this branch, but that result is never used.
                const float dot = [&]{
                    if (std::abs(boxRelAngle) < boxAngle || COL_PI - boxAngle < std::abs(boxRelAngle)) {
                        return DotProduct(victimDir, vehicle->m_vecMoveSpeed);
                    }
                    if (boxRelAngle <= 0.0f) {
                        return DotProduct(vehicle->GetMatrix().GetRight(), vehicle->m_vecMoveSpeed);
                    }
                    return DotProduct(-vehicle->GetMatrix().GetRight(), vehicle->m_vecMoveSpeed);
                }();

                if (dot > 0.1f) {
                    victim->KillPedWithCar(vehicle, speed, false);
                }
            }
        }
        break;
    }
    case ENTITY_TYPE_PED: {
        CPed* const other = hit->AsPed();

        // The event's move states: the go-to state of the ped the event is created for.
        const auto otherMoveState = static_cast<eMoveState>(other->GetIntelligence()->GetMoveStateFromGoToTask());

        // The event is always created for the *other* ped.
        if (!victim->IsPlayer()) {
            if (!other->IsPlayer()) {
                CEventPedCollisionWithPed event{
                    static_cast<int16>(victim->m_nPieceType),
                    victim->m_fDamageIntensity,
                    other,
                    impactVelocity,
                    impactPosn,
                    static_cast<eMoveState>(moveState),
                    otherMoveState
                };
                eventGroup->Add(&event, false);
            } else {
                CEventPedCollisionWithPlayer event{
                    static_cast<int16>(victim->m_nPieceType),
                    victim->m_fDamageIntensity,
                    other,
                    impactVelocity,
                    impactPosn,
                    static_cast<eMoveState>(moveState),
                    otherMoveState
                };
                eventGroup->Add(&event, false);

                // Retail bumps the victim's anger-at-player counter (0x421050), which is rate
                // limited by `CMentalState::m_AngerTimer` (3 seconds per increment).
                victim->GetIntelligence()->IncrementAngerAtPlayer(1);

                victim->Say(CTX_GLOBAL_BUMP, 0, 1.0f, false, false, false);

                if (victim->m_acquaintance.GetAcquaintances(4) & CPedType::GetPedFlag(other->m_nPedType)) {
                    CEventAcquaintancePedHate hateEvent{other};
                    eventGroup->Add(&hateEvent, false);
                }
            }
        } else {
            CEventPlayerCollisionWithPed event{
                static_cast<int16>(victim->m_nPieceType),
                victim->m_fDamageIntensity,
                other,
                impactVelocity,
                impactPosn,
                static_cast<eMoveState>(moveState),
                otherMoveState
            };
            eventGroup->Add(&event, false);

            other->Say(CTX_GLOBAL_BUMP, 0, 1.0f, false, false, false);
        }

        // The other ped only reacts once, so mirror the collision back at it when it has no
        // collision of its own recorded. Retail delivers the mirrored events (and the anger bump)
        // to the *other* ped, not to the ped being scanned.
        if (!other->m_pDamageEntity) {
            const CVector negImpactVelocity = -impactVelocity;
            CEventGroup& otherEventGroup = other->GetEventGroup();

            if (!other->IsPlayer()) {
                if (!victim->IsPlayer()) {
                    CEventPedCollisionWithPed event{
                        static_cast<int16>(victim->m_nPieceType),
                        victim->m_fDamageIntensity,
                        victim,
                        negImpactVelocity,
                        impactPosn,
                        static_cast<eMoveState>(moveState),
                        otherMoveState
                    };
                    otherEventGroup.Add(&event, false);
                } else {
                    CEventPedCollisionWithPlayer event{
                        static_cast<int16>(victim->m_nPieceType),
                        victim->m_fDamageIntensity,
                        victim,
                        negImpactVelocity,
                        impactPosn,
                        static_cast<eMoveState>(moveState),
                        otherMoveState
                    };
                    otherEventGroup.Add(&event, false);

                    // Retail bumps the struck ped's anger-at-player counter (0x421050, see above).
                    other->GetIntelligence()->IncrementAngerAtPlayer(1);

                    if (other->m_acquaintance.GetAcquaintances(4) & CPedType::GetPedFlag(victim->m_nPedType)) {
                        CEventAcquaintancePedHate hateEvent{victim};
                        otherEventGroup.Add(&hateEvent, false);
                    }
                }
            } else {
                CEventPlayerCollisionWithPed event{
                    static_cast<int16>(victim->m_nPieceType),
                    victim->m_fDamageIntensity,
                    victim,
                    negImpactVelocity,
                    impactPosn,
                    static_cast<eMoveState>(moveState),
                    otherMoveState
                };
                otherEventGroup.Add(&event, false);
            }
        }
        break;
    }
    case ENTITY_TYPE_OBJECT: {
        CObject* const object = hit->AsObject();

        float intensity = victim->m_fDamageIntensity;
        if (victim->bIsStanding && !object->GetIsStatic()) {
            const float alongAnimShift = impactVelocity.x * victim->m_vecAnimMovingShift.x
                                       + impactVelocity.y * victim->m_vecAnimMovingShift.y;
            if (alongAnimShift < 0.0f) {
                intensity = std::max(0.0f, alongAnimShift * victim->m_fMass + intensity);
            }
        }

        const float threshold = victim->m_pAttachedTo ? OBJECT_INTENSITY_ATTACHED : OBJECT_INTENSITY;

        if (intensity <= threshold
            || object->GetIsStatic()
            || !victim->bIsStanding
            || hit == victim->m_pContactEntity
            || (object->m_pAttachedTo && object == object->m_pAttachedTo)
        ) {
            CEventObjectCollision event{
                static_cast<int16>(victim->m_nPieceType),
                victim->m_fDamageIntensity,
                object,
                impactVelocity,
                impactPosn,
                moveState
            };
            eventGroup->Add(&event, false);
        } else {
            CWeapon::GenerateDamageEvent(
                victim,
                hit,
                WEAPON_FALL,
                static_cast<int32>(intensity / threshold * IMPACT_DAMAGE_RATIO_MULT),
                PED_PIECE_TORSO,
                static_cast<uint8>(victim->GetLocalDirection(-impactVelocity))
            );

            victim->m_pEntityIgnoredCollision = hit;
        }
        break;
    }
    }

    // Impact sound, only when the ped wears a balaclava and the collision was with a building or an
    // object.
    if (!victim->GetPlayerData()) {
        return;
    }
    const auto hitType = hit->GetType();
    if (hitType != ENTITY_TYPE_BUILDING && hitType != ENTITY_TYPE_OBJECT) {
        return;
    }
    if (!victim->GetClothesDesc()->GetIsWearingBalaclava()) {
        return;
    }

    float volume;
    if (hit->GetIsTypePhysical() && (hit->AsPhysical()->m_nPhysicalFlags & PHYSICAL_DISABLE_MOVE_FORCE)) {
        if (std::abs(hit->AsPhysical()->m_vecTurnSpeed.z) <= IMPACT_SOUND_MIN_TURN_SPEED
            || CTimer::GetTimeInMS() <= s_LastMovingObjectImpactSoundTime + 2000
        ) {
            return;
        }
        volume = 40.0f;
        s_LastMovingObjectImpactSoundTime = CTimer::GetTimeInMS();
    } else {
        if (victim->m_fDamageIntensity <= IMPACT_SOUND_MIN_INTENSITY
            || CTimer::GetTimeInMS() <= s_LastImpactSoundTime + 1000
        ) {
            return;
        }
        volume = victim->m_fDamageIntensity <= IMPACT_SOUND_LOUD_INTENSITY ? 30.0f : 40.0f;
        s_LastImpactSoundTime = CTimer::GetTimeInMS();
    }

    if (volume <= 0.0f) {
        return;
    }

    CEventSoundQuiet event{victim, volume, std::numeric_limits<uint32>::max(), CVector{}};
    GetEventGlobalGroup()->Add(&event, false);
}
