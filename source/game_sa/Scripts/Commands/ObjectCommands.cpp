#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

#include "Object.h"
#include "SetPieces.h"
#include "Fx.h"
#include "PointLights.h"
#include "CommandParser/Parser.hpp"

#include "AnimManager.h"
#include "AudioEngine.h"
#include "BreakManager_c.h"
#include "Camera.h"
#include "ClumpModelInfo.h"
#include "Entity/Ped/PlayerPed.h"
#include "Glass.h"
#include "ObjectData.h"
#include "PedIntelligence.h"
#include "Radar.h"
#include "TaskManager.h"
#include "TaskSimpleHoldEntity.h"
#include "TheScripts.h"
#include "Timer.h"
#include "Train.h"
#include "VisibilityPlugins.h"
#include "World.h"

using namespace notsa;
using namespace notsa::script;

/*!
* Various object commands
*/
namespace {
namespace Object {
CObject& CreateObject(CRunningScript& S, script::Model model, CVector posn) {
    const auto mi = CModelInfo::GetModelInfo(model);
    mi->m_nAlpha  = 255u;

    auto* object = CObject::Create(model, false);
    object->m_nObjectType = (S.m_IsExternal || S.m_ExternalType != -1) ? OBJECT_MISSION2 : OBJECT_MISSION;
    CWorld::PutToGroundIfTooLow(posn);
    posn.z += object->GetDistanceFromCentreOfMassToBaseOfModel();
    object->SetPosn(posn);
    object->SetOrientation(CVector{0.0f});
    object->UpdateRwMatrix();
    object->UpdateRwFrame();
    if (mi->AsLodAtomicModelInfoPtr()) {
        object->SetupBigBuilding();
    }

    CTheScripts::ClearSpaceForMissionEntity(posn, object);
    CWorld::Add(object);

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.AddEntityToList(*object);
    }
    return *object;
}

void RemoveObject(CRunningScript& S, CObject* object) {
    if (object) {
        CWorld::Remove(object);
        CWorld::RemoveReferencesToDeletedObject(object);
        delete object;
    }

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(static_cast<int32>(notsa::PtrToId(object)), MISSION_CLEANUP_ENTITY_TYPE_OBJECT);
    }
}

bool DoesObjectExists(CObject* object) {
    return object != nullptr;
}

void MarkObjectNoLongerNeeded(CRunningScript& S, CObject* object) {
    CTheScripts::CleanUpThisObject(object);

    if (object && S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(*object);
    }
}

void DoNotRemoveObject(CObject& object) {
    CTheScripts::MissionCleanUp.RemoveEntityFromList(object);
}

/// SET_OBJECT_AREA_VISIBLE(0566)
void SetObjectAreaVisible(CObject& object, eAreaCodes area) {
    object.SetAreaCode(area);
}

/// CONNECT_LODS(0827)
void ConnectLods(script::ScriptEntity<CObject> lod, script::ScriptEntity<CObject> object) {
    CTheScripts::ScriptConnectLodsFunction(lod.h, object.h);
    CTheScripts::AddToListOfConnectedLodObjects(lod.e, object.e);
}

CVector GetObjectCoordinates(CObject& object) {
    return object.GetPosition();
}

void SetObjectCoordinates(CObject& object, CVector posn) {
    object.SetPosn(posn);
}

CVector GetObjectVelocity(CObject& object) {
    return object.GetMoveSpeed() * 50.0f;
}

void SetObjectVelocity(CObject& object, CVector velocity) {
    object.SetVelocity(velocity / 50.0f);
}

float GetObjectHeading(CObject& object) {
    return FixAngleDegrees(RadiansToDegrees(object.GetHeading()));
}

void SetObjectHeading(CObject& object, float heading) {
    object.SetHeading(DegreesToRadians(FixAngleDegrees(heading)));
}

void SetObjectScale(CObject& object, float scale) {
    object.m_fScale = scale;
}

void SetObjectCollision(CObject& object, bool enable) {
    object.SetUsesCollision(enable);
}

bool DoesObjectHaveThisModel(CObject& object, script::Model model) {
    return object.m_nModelIndex == model;
}

CObject& CreateObjectNoOffset(CRunningScript& S, script::Model model, CVector posn) {
    const auto mi = CModelInfo::GetModelInfo(model);
    mi->m_nAlpha  = 255u;

    auto* object = CObject::Create(model, false);
    object->m_nObjectType = (S.m_IsExternal || S.m_ExternalType != -1) ? OBJECT_MISSION2 : OBJECT_MISSION;
    CWorld::PutToGroundIfTooLow(posn);
    object->SetPosn(posn);
    object->SetOrientation(CVector{0.0f});
    object->UpdateRwMatrix();
    object->UpdateRwFrame();
    if (mi->AsLodAtomicModelInfoPtr()) {
        object->SetupBigBuilding();
    }

    CTheScripts::ClearSpaceForMissionEntity(posn, object);
    CWorld::Add(object);

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.AddEntityToList(*object);
    }
    return *object;
}

bool IsObjectOnScreen(CObject& object) {
    const auto colModel = object.GetColModel();
    return colModel && TheCamera.IsSphereVisible(object.GetBoundCentre(), colModel->GetBoundRadius());
}

/*!
* @brief Checks whether any vehicle/ped intersects the object's collision bound box, rotated/translated by `mat` and located at `pos`.
*/
bool DoesObjectCollideWhenMoved(CObject& object, const CMatrix& mat, const CVector& pos) {
    const auto colModel = object.GetColModel();
    if (!colModel) {
        return false;
    }

    const auto& box = colModel->m_boundBox;
    const CVector corners[] = {
        box.m_vecMin,
        box.m_vecMax,
        CVector{box.m_vecMax.x, box.m_vecMax.y, box.m_vecMin.z},
        CVector{box.m_vecMin.x, box.m_vecMax.y, box.m_vecMax.z}
    };

    //! Extents (in X/Y) of the box once transformed
    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;
    for (const auto& corner : corners) {
        const auto p = mat.TransformPoint(corner);
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }

    int16 numFound{};
    CWorld::FindObjectsIntersectingAngledCollisionBox(box, mat, pos, minX, minY, maxX, maxY, &numFound, 2, nullptr, false, true, true, false, false);
    return numFound > 0;
}

bool RotateObject(CObject& object, float targetHeading, float rotationSpeed, bool checkCollision) {
    auto currentHeading = RadiansToDegrees(object.GetHeading());
    if (currentHeading < 0.0f) {
        currentHeading += 360.0f;
    }

    bool finished = currentHeading == targetHeading;
    if (!finished) {
        auto clockwise        = targetHeading - currentHeading;
        auto counterClockwise = currentHeading - targetHeading;
        if (clockwise < 0.0f) {
            clockwise += 360.0f;
        }
        if (counterClockwise < 0.0f) {
            counterClockwise += 360.0f;
        }

        //! Rotate towards the target using the shortest way
        const auto newHeading = [&] {
            if (counterClockwise <= clockwise) {
                return rotationSpeed <= counterClockwise ? currentHeading - rotationSpeed : targetHeading;
            }
            return rotationSpeed <= clockwise ? currentHeading + rotationSpeed : targetHeading;
        }();

        if (checkCollision) {
            CMatrix mat;
            mat.SetRotateZ(DegreesToRadians(newHeading));
            mat.GetPosition() = object.GetPosition();
            if (DoesObjectCollideWhenMoved(object, mat, object.GetPosition())) {
                finished = true;
            }
        }

        if (!finished) {
            object.SetHeading(DegreesToRadians(newHeading));
            object.UpdateRwMatrix();
            object.UpdateRwFrame();
            finished = newHeading == targetHeading;
        }
    }

    return finished;
}

bool SlideObject(CObject& object, CVector targetPosn, CVector maxStep, bool checkCollision) {
    const auto posn = object.GetPosition();
    if (posn == targetPosn) {
        return true;
    }

    CVector newPosn = posn;
    for (auto i = 0u; i < 3u; i++) {
        const auto delta = posn[i] - targetPosn[i];
        if (delta < 0.0f) {
            newPosn[i] = -delta <= maxStep[i] ? targetPosn[i] : posn[i] + maxStep[i];
        } else {
            newPosn[i] = delta <= maxStep[i] ? targetPosn[i] : posn[i] - maxStep[i];
        }
    }

    if (checkCollision) {
        CMatrix mat = object.GetMatrix();
        mat.GetPosition() = newPosn;
        if (DoesObjectCollideWhenMoved(object, mat, newPosn)) {
            return true;
        }
    }

    object.Teleport(newPosn, false);
    return newPosn == targetPosn;
}

/*!
* @brief Finds the closest object with the given model around `pos` (only ones closer than `radius * 2` are considered).
* @param findLod Also search for a matching LOD object if no regular one was found.
*/
CObject* FindClosestObjectOfType(const CVector& pos, float radius, eModelID model, bool findLod) {
    CEntity*   entities[16];
    int16      numEntities{};
    const auto maxEntities = static_cast<int16>(std::size(entities));

    CWorld::FindObjectsOfTypeInRange(model, pos, radius, true, &numEntities, maxEntities, entities, true, false, false, true, true);
    if (numEntities == 0 && findLod) {
        CWorld::FindLodOfTypeInRange(model, pos, radius, true, &numEntities, maxEntities, entities);
    }

    CObject* closest{};
    auto     closestDist = radius + radius;
    for (int16 i = 0; i < numEntities; i++) {
        const auto dist = (entities[i]->GetPosition() - pos).Magnitude();
        if (dist < closestDist) {
            closest     = entities[i]->AsObject();
            closestDist = dist;
        }
    }
    return closest;
}

void SetVisibilityOfClosestObjectOfType(CVector posn, float radius, script::Model model, bool visible) {
    CWorld::PutToGroundIfTooLow(posn);

    const auto object = FindClosestObjectOfType(posn, radius, model, true);
    if (!object) {
        return;
    }

    if (!visible) {
        object->m_nFlags &= ~(1u | 0x80u); // bUsesCollision | bIsVisible
        CTheScripts::AddToInvisibilitySwapArray(object, false);
    } else {
        object->m_nFlags |= 1u | 0x80u;
        if (!object->GetIplIndex()) {
            for (auto& entity : CTheScripts::InvisibilitySettingArray) {
                if (entity == object) {
                    entity = nullptr;
                    break;
                }
            }
        }
    }
}

void SetUsesCollisionOfClosestObjectOfType(CVector posn, float radius, script::Model model, bool state) {
    CWorld::PutToGroundIfTooLow(posn);

    if (const auto object = FindClosestObjectOfType(posn, radius, model, false)) {
        object->m_bUsesCollision = state;
    }
}

void SetCharUsesCollisionClosestObjectOfType(CVector posn, float radius, script::Model model, bool state, CPed& ped) {
    CWorld::PutToGroundIfTooLow(posn);

    if (const auto object = FindClosestObjectOfType(posn, radius, model, false)) {
        ped.m_pEntityIgnoredCollision = state ? nullptr : object;
    }
}

void AddToObjectVelocity(CObject& object, CVector velocity) {
    object.m_vecMoveSpeed += velocity * 0.02f;
}

void SetObjectDynamic(CObject& object, bool dynamic) {
    if (dynamic) {
        if (!object.m_bIsStatic) {
            return;
        }
        object.SetIsStatic(false);
        object.AddToMovingList();
    } else {
        if (object.m_bIsStatic) {
            return;
        }
        object.SetIsStatic(true);
        object.RemoveFromMovingList();
    }
}

CVector GetOffsetFromObjectInWorldCoords(CObject& object, CVector offset) {
    return object.GetMatrix().TransformVector(offset) + object.GetPosition();
}

void SetObjectRotation(CObject& object, float rotX, float rotY, float rotZ) {
    CWorld::Remove(&object);
    object.SetOrientation(CVector{DegreesToRadians(rotX), DegreesToRadians(rotY), DegreesToRadians(rotZ)});
    object.UpdateRwMatrix();
    object.UpdateRwFrame();
    CWorld::Add(&object);
}

void SetObjectRecordsCollisions(CObject& object, bool recordsCollisions) {
    object.physicalFlags.bCanBeCollidedWith = recordsCollisions;
}

bool HasObjectCollidedWithAnything(CObject& object) {
    return object.m_nNumEntitiesCollided != 0;
}

void LocateObject2D(CRunningScript& S, eScriptCommands command) {
    S.LocateObjectCommand(command);
}

void LocateObject3D(CRunningScript& S, eScriptCommands command) {
    S.LocateObjectCommand(command);
}

void IsObjectInArea3D(CRunningScript& S, eScriptCommands command) {
    S.ObjectInAreaCheckCommand(command);
}

void FreezeObjectPosition(CObject& object, bool freeze) {
    object.physicalFlags.bDisableCollisionForce = freeze;
    object.physicalFlags.bDontApplySpeed        = freeze;
}

CVector GetObjectRotationVelocity(CObject& object) {
    return object.GetMatrix().InverseTransformVector(object.m_vecTurnSpeed) * 50.0f;
}

uint32 CreateFxSystemOnObjectWithDirection(CRunningScript& S, const char* name, CObject& object, CVector offset, CVector direction, bool ignoreBoundingChecks) {
    CWorld::PutToGroundIfTooLow(offset);

    RwMatrix transform;
    g_fx.CreateMatFromVec(&transform, &offset, &direction);

    if (!object.GetRwObject()) {
        object.CreateRwObject();
    }

    // TODO(re): The decompiled case re-tests `direction.x` before choosing `ignoreBoundingChecks` in the no-model-matrix branch - read as the script flag.
    FxSystem_c* fxSystem;
    if (const auto objectMatrix = object.GetModellingMatrix()) {
        fxSystem = g_fxMan.CreateFxSystem(name, transform, objectMatrix, ignoreBoundingChecks);
    } else {
        fxSystem = g_fxMan.CreateFxSystem(name, offset, nullptr, ignoreBoundingChecks);
    }

    const auto fxSystemId = CTheScripts::AddScriptEffectSystem(fxSystem);
    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.AddEntityToList(fxSystemId, MISSION_CLEANUP_ENTITY_TYPE_PARTICLE);
    }
    return fxSystemId;
}

void AttachObjectToEntity(CObject& object, CPhysical* attachTo, CVector offset, CVector rotation) {
    object.AttachEntityToEntity(attachTo, offset, CVector{DegreesToRadians(rotation.x), DegreesToRadians(rotation.y), DegreesToRadians(rotation.z)});
}

void AttachObjectToObject(CObject& object, CObject* attachTo, CVector offset, CVector rotation) {
    AttachObjectToEntity(object, attachTo, offset, rotation);
}

void AttachObjectToChar(CObject& object, CPed* ped, CVector offset, CVector rotation) {
    AttachObjectToEntity(object, ped, offset, rotation);
}

void DetachObject(CObject* object, float pitch, float heading, float strength, bool applyTurnForce) {
    if (object && object->m_pAttachedTo) {
        object->DettachEntityFromEntity(DegreesToRadians(pitch), DegreesToRadians(-heading), DegreesToRadians(strength), applyTurnForce);
    }
}

bool IsObjectAttached(CObject* object) {
    return object && object->m_pAttachedTo;
}

void TaskPickUpObject(CRunningScript& S, eScriptCommands command) {
    S.ScriptTaskPickUpObject(command);
}

void TaskPickUpSecondObject(CRunningScript& S, eScriptCommands command) {
    S.ScriptTaskPickUpObject(command);
}

void DropObject(CPed& ped, bool addEventSoundQuiet) {
    if (const auto holdTask = ped.GetIntelligence()->GetTaskHold(false)) {
        holdTask->DropEntity(&ped, addEventSoundQuiet);
    }
}

void DropSecondObject(CPed& ped, bool addEventSoundQuiet) {
    // The second object is kept in the secondary task at index 0 (See `ScriptTaskPickUpObject`)
    const auto task = ped.GetTaskManager().GetTaskSecondary(TASK_SECONDARY_ATTACK);
    if (task && task->GetTaskType() == TASK_SIMPLE_HOLD_ENTITY) {
        const auto holdTask = static_cast<CTaskSimpleHoldEntity*>(task);
        holdTask->DropEntity(&ped, addEventSoundQuiet);
        holdTask->MakeAbortable(&ped, ABORT_PRIORITY_URGENT, nullptr);
    }
}

bool IsCharHoldingObject(CPed* ped, CObject* object) {
    const auto IsHolding = [object](CPed& pedToCheck) {
        const auto holdTask = pedToCheck.GetIntelligence()->GetTaskHold(false);
        if (holdTask && holdTask->SetPedPosition(&pedToCheck)) {
            return object ? holdTask->m_pEntityToHold == object : holdTask->m_pEntityToHold != nullptr;
        }
        return false;
    };

    if (ped) {
        return IsHolding(*ped);
    }

    for (auto& otherPed : GetPedPool()->GetAllValid()) {
        if (IsHolding(otherPed)) {
            return true;
        }
    }
    return false;
}

void SetObjectVisible(CObject& object, bool visible) {
    object.m_bIsVisible = visible;
}

bool PlayObjectAnim(CObject& object, const char* animName, const char* ifpName, float frameDelta, bool lockF, bool loop) {
    const auto animHierarchy = CAnimManager::GetAnimation(animName, CAnimManager::GetAnimationBlock(ifpName));

    int32 flags{};
    if (lockF) {
        flags |= ANIMATION_IS_LOOPED;
    }
    if (!loop) {
        flags |= ANIMATION_IS_FINISH_AUTO_REMOVE;
    }

    const auto clump = object.GetRwObject() && RwObjectGetType(object.GetRwObject()) == rpCLUMP
        ? object.GetRpClump()
        : nullptr;
    if (!clump) {
        return false;
    }

    if (!RpAnimBlendClumpIsInitialized(clump)) {
        RpAnimBlendClumpInit(clump);
    }
    CAnimManager::BlendAnimation(clump, animHierarchy, flags, frameDelta);
    return true;
}

float GetRopeHeightForObject(CObject& object) {
    return object.GetRopeHeight();
}

void SetRopeHeightForObject(CObject& object, float height) {
    object.SetRopeHeight(height);
}

auto GrabEntityOnRopeForObject(CObject& object) {
    int32 vehicleHandle = -1;
    int32 pedHandle     = -1;
    int32 objectHandle  = -1;

    if (const auto entity = object.GetObjectCarriedWithRope()) {
        switch (entity->GetType()) {
        case ENTITY_TYPE_VEHICLE:
            vehicleHandle = GetVehiclePool()->GetRef(entity->AsVehicle());
            break;
        case ENTITY_TYPE_PED:
            pedHandle = GetPedPool()->GetRef(entity->AsPed());
            break;
        case ENTITY_TYPE_OBJECT:
            objectHandle = GetObjectPool()->GetRef(entity->AsObject());
            break;
        }
    }
    return return_multiple(vehicleHandle, pedHandle, objectHandle);
}

void ReleaseEntityFromRopeForObject(CObject& object) {
    object.ReleaseObjectCarriedWithRope();
}

void SetObjectCollisionDamageEffect(CObject& object, bool state) {
    if (state) {
        if (object.m_nSpecialColResponseCase) {
            object.m_nColDamageEffect        = object.m_nSpecialColResponseCase;
            object.m_nSpecialColResponseCase = 0;
        }
    } else if (object.m_nColDamageEffect) {
        object.m_nSpecialColResponseCase = object.m_nColDamageEffect;
        object.m_nColDamageEffect        = 0;
    }
}

CVector GetLevelDesignCoordsForObject(CObject& object, int32 nth) {
    CVector coords;
    object.FindTriggerPointCoors(&coords, nth);
    return coords;
}

void SetObjectCoordinatesAndVelocity(CObject& object, CVector posn) {
    CWorld::PutToGroundIfTooLow(posn);

    const auto velocity = (posn - object.GetPosition()) / CTimer::GetTimeStep();
    object.m_vecMoveSpeed = CVector{
        std::clamp(velocity.x, -1.0f, 1.0f),
        std::clamp(velocity.y, -1.0f, 1.0f),
        std::clamp(velocity.z, -1.0f, 1.0f)
    };

    MarkSurroundingEntitiesForCollisionWithTrain(posn, 25.0f, &object, false);
    object.Teleport(posn, false);
    CTheScripts::ClearSpaceForMissionEntity(posn, &object);
}

void LockDoor(CObject& object, bool state) {
    if (state) {
        object.LockDoor();
    } else {
        object.physicalFlags.bDisableCollisionForce = false;
        object.physicalFlags.bCollidable            = false;
        object.m_vecTurnSpeed                       = CVector{};
        object.SetIsStatic(true);
    }
}

void AllocateStreamedScriptToObject(int32 scriptId, script::Model model, int32 priority, float radius, int32 groupingId) {
    const auto streamedScriptIndex = CTheScripts::StreamedScripts.GetProperIndexFromIndexUsedByScript(static_cast<int16>(scriptId));
    CTheScripts::ScriptsForBrains.AddNewScriptBrain(streamedScriptIndex, static_cast<int16>(model.value), static_cast<uint16>(priority), 1, static_cast<int8>(groupingId), radius);
}

bool HasObjectBeenUprooted(CObject& object) {
    return !object.GetIsStatic();
}

bool IsObjectWithinBrainActivationRange(CObject* object) {
    if (!CWorld::Players[CWorld::PlayerInFocus].m_pPed || !object) {
        return false;
    }
    return CTheScripts::ScriptsForBrains.IsObjectWithinBrainActivationRange(object, FindPlayerCentreOfWorld(CWorld::PlayerInFocus));
}

void ReportMissionAudioEventAtObject(CObject* object, uint16 soundId) {
    AudioEngine.ReportMissionAudioEvent(soundId, object);
}

void AttachMissionAudioToObject(uint8 slotId, CObject* object) {
    AudioEngine.AttachMissionAudioToObject(slotId - 1, object);
}

int32 GetObjectModel(CObject& object) {
    return object.m_nModelIndex;
}

void RemoveObjectElegantly(CRunningScript& S, script::ScriptEntity<CObject> object) {
    CTheScripts::CleanUpThisObject(object.e);

    if (object.e) {
        if (object.e->GetRwObject() && RwObjectGetType(object.e->GetRwObject()) == rpCLUMP) {
            RpClumpForAllAtomics(object.e->GetRpClump(), CClumpModelInfo::SetAtomicRendererCB, reinterpret_cast<void*>(CVisibilityPlugins::RenderFadingClumpCB));
        }
        object.e->objectFlags.bFadingIn = true;
    }

    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.RemoveEntityFromList(object.h, MISSION_CLEANUP_ENTITY_TYPE_OBJECT);
    }
}

void SetObjectProofs(CObject& object, bool bulletProof, bool fireProof, bool explosionProof, bool collisionProof, bool meleeProof) {
    object.physicalFlags.bBulletProof    = bulletProof;
    object.physicalFlags.bFireProof      = fireProof;
    object.physicalFlags.bExplosionProof = explosionProof;
    object.physicalFlags.bCollisionProof = collisionProof;
    object.physicalFlags.bMeleeProof     = meleeProof;
}

tBlipHandle AddBlipForObject(script::ScriptEntity<CObject> object) {
    const auto blip = CRadar::SetEntityBlip(BLIP_OBJECT, object.h, 6, BLIP_DISPLAY_BOTH);
    CRadar::ChangeBlipScale(blip, 3);
    return blip;
}

/// SET_OBJECT_HEALTH(071F)
void SetObjectHealth(CObject* object, int32 health) {
    if (object) {
        object->m_fHealth = static_cast<float>(health);
    }
}

/// SET_OBJECT_ONLY_DAMAGED_BY_PLAYER(0875)
void SetObjectOnlyDamagedByPlayer(CObject* object, bool state) {
    if (object) {
        object->physicalFlags.bInvulnerable = state;
    }
}

/// MAKE_OBJECT_TARGETTABLE(035D)
void MakeObjectTargettable(CObject* object, bool state) {
    if (!object) {
        return;
    }
    object->SetObjectTargettable(state);

    // Stop the player from locking-on to an object that's no longer targettable
    if (!state) {
        if (const auto ped = FindPlayerPed()) {
            if (ped->m_pTargetedObject == object) {
                CEntity::ClearReference(ped->m_pTargetedObject);
            }
        }
    }
}

/// HAS_OBJECT_BEEN_DAMAGED(0366)
bool HasObjectBeenDamaged(const CObject* object) {
    // Retail (0x46A2F0): `m_bRenderDamaged || !m_bIsVisible`
    return object && (object->m_bRenderDamaged || !object->m_bIsVisible);
}

/// BREAK_OBJECT(0723)
void BreakObject(CObject* object, int32 intensity) {
    if (!object) {
        return;
    }
    if (CGlass::IsObjectGlass(object)) {
        CGlass::WindowRespondsToCollision(object, 100000.f, CVector{}, object->GetPosition(), false);
        object->objectFlags.bIsBroken = true;
    } else if (object->m_pObjectInfo) {
        auto breakVelocity = object->m_pObjectInfo->m_vecBreakVelocity;
        g_breakMan.Add(object, &breakVelocity, object->m_pObjectInfo->m_fBreakVelocityRand, intensity != 0);
    }
}

/// IS_CLOSEST_OBJECT_OF_TYPE_SMASHED_OR_DAMAGED(0716)
bool IsClosestObjectOfTypeSmashedOrDamaged(float x, float y, float z, float radius, int32 modelId, bool smashed, bool damaged) {
    CVector point{x, y, z};
    if (z <= -100.f) {
        point.z = CWorld::FindGroundZForCoord(x, y);
    }
    if (modelId < 0) {
        modelId = CTheScripts::UsedObjectArray[-modelId].nModelIndex;
    }

    bool result = false;
    if (const auto entity = CWorld::FindNearestObjectOfType(modelId, point, radius, false, false, false, false, true, true)) {
        if (entity->GetType() == ENTITY_TYPE_OBJECT) {
            const auto object = static_cast<CObject*>(entity);
            if (smashed && object->objectFlags.bIsBroken) {
                result = true;
            }
            if (damaged && HasObjectBeenDamaged(object)) {
                result = true;
            }
        }
    }
    return result;
}
} // namespace Object

namespace Model {
void LoadModel(CRunningScript* S, script::Model model) {
    CStreaming::RequestModel(model, STREAMING_MISSION_REQUIRED | STREAMING_KEEP_IN_MEMORY);
    CTheScripts::ScriptResourceManager.AddToResourceManager(model, RESOURCE_TYPE_MODEL_OR_SPECIAL_CHAR, S);
}

bool HasModelLoaded(script::Model model) {
    return CStreaming::IsModelLoaded(model);
}

void MarkModelNotNeeded(CRunningScript* S, script::Model model) {
    if (CTheScripts::ScriptResourceManager.RemoveFromResourceManager(model, RESOURCE_TYPE_MODEL_OR_SPECIAL_CHAR, S)) {
        CStreaming::SetMissionDoesntRequireModel(model);
    }
}

void LoadAllModelsNow() {
    CTimer::Suspend();
    CStreaming::LoadAllRequestedModels(false);
    CTimer::Resume();
}

bool IsModelAvailable(eModelID model) {
    return CModelInfo::GetModelInfo(model) != nullptr;
}
} // namespace Model

namespace Fx {
void DrawLight(CVector pos, CVector color) {
    color /= 255.0f;
    CPointLights::AddLight(ePointLightType::PLTYPE_POINTLIGHT, pos, {}, 12.0f, color.x, color.y, color.z, 0, false, nullptr);
}

void AddBigGunFlash(CVector origin, CVector target) {
    CPointLights::AddLight(ePointLightType::PLTYPE_POINTLIGHT, origin, {}, 5.0f, 1.0f, 0.8f, 0.0f, 0, false, nullptr);
    g_fx.TriggerGunshot(nullptr, origin, (target - origin).Normalized(), true);
}
} // namespace Fx

namespace Attractor {
void EnableDisabledAttractorOnObject(CObject& object, bool enabled) {
    object.objectFlags.bEnableDisabledAttractors = enabled; // todo: rename obj->objectFlags.b0x1000000
}
} // namespace Attractor

namespace Animation {
void SetObjectAnimSpeed(CObject& obj, const char* animName, float speed) {
    if (const auto anim = RpAnimBlendClumpGetAssociation(obj.GetRpClump(), animName)) {
        anim->m_Speed = speed;
    }
}

bool IsObjectPlayingAnim(CObject& obj, const char* animName) {
    auto* const clump = RwObjectGetType(obj.GetRwObject()) == rpCLUMP 
        ? obj.GetRpClump() 
        : nullptr;
    return clump != nullptr
        && RpAnimBlendClumpIsInitialized(clump)
        && RpAnimBlendClumpGetAssociation(clump, animName) != nullptr;
}

auto GetObjectAnimCurrentTime(CObject& obj, const char* animName) {
    if (const auto anim = RpAnimBlendClumpGetAssociation(obj.GetRpClump(), animName)) {
        return anim->m_CurrentTime / anim->m_BlendHier->m_fTotalTime;
    }
    return 0.f;
}

auto SetObjectAnimCurrentTime(CObject& obj, const char* animName, float progress) {
    if (const auto anim = RpAnimBlendClumpGetAssociation(obj.GetRpClump(), animName)) {
        anim->SetCurrentTime(anim->m_Speed * progress);
    }
}
} // namespace Animation
};

void notsa::script::commands::object::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Object");

    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_HEALTH, Object::SetObjectHealth);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_ONLY_DAMAGED_BY_PLAYER, Object::SetObjectOnlyDamagedByPlayer);
    REGISTER_COMMAND_HANDLER(COMMAND_MAKE_OBJECT_TARGETTABLE, Object::MakeObjectTargettable);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_OBJECT_BEEN_DAMAGED, Object::HasObjectBeenDamaged);
    REGISTER_COMMAND_HANDLER(COMMAND_BREAK_OBJECT, Object::BreakObject);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CLOSEST_OBJECT_OF_TYPE_SMASHED_OR_DAMAGED, Object::IsClosestObjectOfTypeSmashedOrDamaged);

    using namespace Object;
    using namespace Model;
    using namespace Fx;
    using namespace Attractor;
    using namespace Animation;

    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_OBJECT, CreateObject);
    REGISTER_COMMAND_HANDLER(COMMAND_DELETE_OBJECT, RemoveObject);
    REGISTER_COMMAND_HANDLER(COMMAND_DOES_OBJECT_EXIST, DoesObjectExists);
    REGISTER_COMMAND_HANDLER(COMMAND_MARK_OBJECT_AS_NO_LONGER_NEEDED, MarkObjectNoLongerNeeded);
    REGISTER_COMMAND_HANDLER(COMMAND_DONT_REMOVE_OBJECT, DoNotRemoveObject);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OBJECT_COORDINATES, GetObjectCoordinates);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_COORDINATES, SetObjectCoordinates);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OBJECT_VELOCITY, GetObjectVelocity);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_VELOCITY, SetObjectVelocity);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OBJECT_HEADING, GetObjectHeading);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_HEADING, SetObjectHeading);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_SCALE, SetObjectScale);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_COLLISION, SetObjectCollision);
    REGISTER_COMMAND_HANDLER(COMMAND_DOES_OBJECT_HAVE_THIS_MODEL, DoesObjectHaveThisModel);

    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_OBJECT_NO_OFFSET, CreateObjectNoOffset);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_OBJECT_ON_SCREEN, IsObjectOnScreen);
    REGISTER_COMMAND_HANDLER(COMMAND_ROTATE_OBJECT, RotateObject);
    REGISTER_COMMAND_HANDLER(COMMAND_SLIDE_OBJECT, SlideObject);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_VISIBILITY_OF_CLOSEST_OBJECT_OF_TYPE, SetVisibilityOfClosestObjectOfType);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_TO_OBJECT_VELOCITY, AddToObjectVelocity);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_DYNAMIC, SetObjectDynamic);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OFFSET_FROM_OBJECT_IN_WORLD_COORDS, GetOffsetFromObjectInWorldCoords);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_ROTATION, SetObjectRotation);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_RECORDS_COLLISIONS, SetObjectRecordsCollisions);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_OBJECT_COLLIDED_WITH_ANYTHING, HasObjectCollidedWithAnything);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCATE_OBJECT_2D, LocateObject2D);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCATE_OBJECT_3D, LocateObject3D);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_OBJECT_IN_AREA_3D, IsObjectInArea3D);
    REGISTER_COMMAND_HANDLER(COMMAND_FREEZE_OBJECT_POSITION, FreezeObjectPosition);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OBJECT_ROTATION_VELOCITY, GetObjectRotationVelocity);
    REGISTER_COMMAND_HANDLER(COMMAND_CREATE_FX_SYSTEM_ON_OBJECT_WITH_DIRECTION, CreateFxSystemOnObjectWithDirection);
    REGISTER_COMMAND_HANDLER(COMMAND_ATTACH_OBJECT_TO_OBJECT, AttachObjectToObject);
    REGISTER_COMMAND_HANDLER(COMMAND_ATTACH_OBJECT_TO_CHAR, AttachObjectToChar);
    REGISTER_COMMAND_HANDLER(COMMAND_DETACH_OBJECT, DetachObject);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_OBJECT_ATTACHED, IsObjectAttached);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_PICK_UP_OBJECT, TaskPickUpObject);
    REGISTER_COMMAND_HANDLER(COMMAND_TASK_PICK_UP_SECOND_OBJECT, TaskPickUpSecondObject);
    REGISTER_COMMAND_HANDLER(COMMAND_DROP_OBJECT, DropObject);
    REGISTER_COMMAND_HANDLER(COMMAND_DROP_SECOND_OBJECT, DropSecondObject);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_CHAR_HOLDING_OBJECT, IsCharHoldingObject);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_VISIBLE, SetObjectVisible);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_AREA_VISIBLE, SetObjectAreaVisible);
    REGISTER_COMMAND_HANDLER(COMMAND_CONNECT_LODS, ConnectLods);
    REGISTER_COMMAND_HANDLER(COMMAND_PLAY_OBJECT_ANIM, PlayObjectAnim);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_ROPE_HEIGHT_FOR_OBJECT, GetRopeHeightForObject);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_ROPE_HEIGHT_FOR_OBJECT, SetRopeHeightForObject);
    REGISTER_COMMAND_HANDLER(COMMAND_GRAB_ENTITY_ON_ROPE_FOR_OBJECT, GrabEntityOnRopeForObject);
    REGISTER_COMMAND_HANDLER(COMMAND_RELEASE_ENTITY_FROM_ROPE_FOR_OBJECT, ReleaseEntityFromRopeForObject);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_COLLISION_DAMAGE_EFFECT, SetObjectCollisionDamageEffect);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_LEVEL_DESIGN_COORDS_FOR_OBJECT, GetLevelDesignCoordsForObject);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_COORDINATES_AND_VELOCITY, SetObjectCoordinatesAndVelocity);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_USES_COLLISION_OF_CLOSEST_OBJECT_OF_TYPE, SetUsesCollisionOfClosestObjectOfType);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_CHAR_USES_COLLISION_CLOSEST_OBJECT_OF_TYPE, SetCharUsesCollisionClosestObjectOfType);
    REGISTER_COMMAND_HANDLER(COMMAND_LOCK_DOOR, LockDoor);
    REGISTER_COMMAND_HANDLER(COMMAND_ALLOCATE_STREAMED_SCRIPT_TO_OBJECT, AllocateStreamedScriptToObject);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_OBJECT_BEEN_UPROOTED, HasObjectBeenUprooted);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_OBJECT_WITHIN_BRAIN_ACTIVATION_RANGE, IsObjectWithinBrainActivationRange);
    REGISTER_COMMAND_HANDLER(COMMAND_REPORT_MISSION_AUDIO_EVENT_AT_OBJECT, ReportMissionAudioEventAtObject);
    REGISTER_COMMAND_HANDLER(COMMAND_ATTACH_MISSION_AUDIO_TO_OBJECT, AttachMissionAudioToObject);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OBJECT_MODEL, GetObjectModel);
    REGISTER_COMMAND_HANDLER(COMMAND_REMOVE_OBJECT_ELEGANTLY, RemoveObjectElegantly);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_PROOFS, SetObjectProofs);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_BLIP_FOR_OBJECT, AddBlipForObject);

    REGISTER_COMMAND_HANDLER(COMMAND_REQUEST_MODEL, LoadModel);
    REGISTER_COMMAND_HANDLER(COMMAND_HAS_MODEL_LOADED, HasModelLoaded);
    REGISTER_COMMAND_HANDLER(COMMAND_MARK_MODEL_AS_NO_LONGER_NEEDED, MarkModelNotNeeded);
    REGISTER_COMMAND_HANDLER(COMMAND_LOAD_ALL_MODELS_NOW, LoadAllModelsNow);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_MODEL_AVAILABLE, IsModelAvailable);

    REGISTER_COMMAND_HANDLER(COMMAND_ADD_SET_PIECE, CSetPieces::AddOne);
    REGISTER_COMMAND_HANDLER(COMMAND_DRAW_LIGHT, DrawLight);
    REGISTER_COMMAND_HANDLER(COMMAND_ADD_BIG_GUN_FLASH, AddBigGunFlash);

    REGISTER_COMMAND_HANDLER(COMMAND_ENABLE_DISABLED_ATTRACTORS_ON_OBJECT, EnableDisabledAttractorOnObject);

    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_ANIM_SPEED, SetObjectAnimSpeed);
    REGISTER_COMMAND_HANDLER(COMMAND_IS_OBJECT_PLAYING_ANIM, IsObjectPlayingAnim);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_OBJECT_ANIM_CURRENT_TIME, GetObjectAnimCurrentTime);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_OBJECT_ANIM_CURRENT_TIME, SetObjectAnimCurrentTime);

    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_FLASH_OBJECT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_NO_OBJ);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_WAIT_ON_FOOT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FLEE_ON_FOOT_TILL_SAFE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GUARD_SPOT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GUARD_AREA);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_WAIT_IN_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_KILL_CHAR_ON_FOOT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_KILL_PLAYER_ON_FOOT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_KILL_CHAR_ANY_MEANS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_KILL_PLAYER_ANY_MEANS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FLEE_CHAR_ON_FOOT_TILL_SAFE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FLEE_PLAYER_ON_FOOT_TILL_SAFE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FLEE_CHAR_ON_FOOT_ALWAYS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FLEE_PLAYER_ON_FOOT_ALWAYS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GOTO_CHAR_ON_FOOT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GOTO_PLAYER_ON_FOOT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_LEAVE_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_ENTER_CAR_AS_PASSENGER);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_ENTER_CAR_AS_DRIVER);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FOLLOW_CAR_IN_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FIRE_AT_OBJECT_FROM_VEHICLE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_DESTROY_OBJECT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_DESTROY_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GOTO_AREA_ON_FOOT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GOTO_AREA_IN_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FOLLOW_CAR_ON_FOOT_WITH_OFFSET);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GUARD_ATTACK);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_FOLLOW_ROUTE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GOTO_COORD_ON_FOOT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GOTO_COORD_IN_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_RUN_TO_AREA);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_RUN_TO_COORD);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_PEDS_IN_AREA_TO_COLL);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_PEDS_IN_VEHICLE_TO_COLL);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CLEAR_COLL);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_COLL_IN_CARS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_ANY_MEANS_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_ON_FOOT_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_IN_CAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_STOPPED_COLL_ANY_MEANS_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_STOPPED_COLL_ON_FOOT_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_STOPPED_COLL_IN_CAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_ANY_MEANS_CHAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_ON_FOOT_CHAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_IN_CAR_CHAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_ANY_MEANS_CAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_ON_FOOT_CAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_IN_CAR_CAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_ANY_MEANS_PLAYER_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_ON_FOOT_PLAYER_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOCATE_COLL_IN_CAR_PLAYER_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_COLL_IN_AREA_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_COLL_IN_AREA_ON_FOOT_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_COLL_IN_AREA_IN_CAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_COLL_STOPPED_IN_AREA_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_COLL_STOPPED_IN_AREA_ON_FOOT_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_COLL_STOPPED_IN_AREA_IN_CAR_2D);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_NUMBER_OF_PEDS_IN_COLL);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_GOTO_AREA_ANY_MEANS);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_PARTICLE_EFFECT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_SPRITE_BLIP_FOR_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_SPRITE_BLIP_FOR_CHAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_SPRITE_BLIP_FOR_OBJECT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_DROP_MINE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_DROP_NAUTICAL_MINE);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_LOAD_SPECIAL_MODEL);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_CREATE_CUTSCENE_HEAD);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_CUTSCENE_HEAD_ANIM);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_CATCH_TRAIN);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_SET_COLL_OBJ_STEAL_ANY_CAR);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_ADD_MOVING_PARTICLE_EFFECT);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_REMOVE_PARTICLE_EFFECTS_IN_AREA);
    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_IS_COLLISION_IN_MEMORY);
}
