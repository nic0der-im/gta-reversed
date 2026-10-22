#include "StdInc.h"

#include "AttractorScanner.h"

#include "Scripts/Scripted2dEffects.h"

// 0x5FFF90
void CAttractorScanner::Clear()
{
    for (auto i = 0; i < (int32)std::size(m_apEntities); i++) {
        m_apEntities[i] = nullptr;
        m_apEffects[i]  = nullptr;
        m_aDistances[i] = (i == PED_ATTRACTOR_SHELTER || i == PED_ATTRACTOR_SCRIPTED) ? 225.f : 25.f;
    }
}

// 0x6060A0
void CAttractorScanner::ScanForAttractors(CPed& ped)
{
    if (!field_0 || ped.bInVehicle) {
        return;
    }

    //! Using an attractor right now? Only postpone the next scan
    if (const auto* const activeTask = ped.GetTaskManager().GetActiveTask(); activeTask && activeTask->GetTaskType() == TASK_COMPLEX_USE_EFFECT) {
        field_4.m_nStartTime = CTimer::GetTimeInMS();
        field_4.m_nInterval  = 3000;
        field_4.m_bStarted   = true;
        return;
    }

    field_4.StartIfNotAlready(1500);
    field_4.Reset();
    if (!field_4.IsOutOfTime()) {
        return;
    }
    field_4.Start(1500);

    Clear();

    // --- Sector sweep ---
    const auto pos = ped.GetPosition();
    const auto minX = std::max(CWorld::GetSectorX(pos.x - 15.f), 0);
    const auto minY = std::max(CWorld::GetSectorY(pos.y - 15.f), 0);
    const auto maxX = std::min(CWorld::GetSectorX(pos.x + 15.f), MAX_SECTORS_X - 1);
    const auto maxY = std::min(CWorld::GetSectorY(pos.y + 15.f), MAX_SECTORS_Y - 1);

    // Iterates the entities of a sector list and adds their attractor effects, if any
    const auto ScanList = [this, &ped](const auto& list) {
        // NOTE: The original only ever sets this for objects, but keeps using it for the other entity types as well
        bool enableDisabledAttractors = false;
        for (auto* const entityRaw : list) {
            CEntity* const entity = entityRaw;
            if (entity->GetType() == ENTITY_TYPE_OBJECT) {
                const auto& object = static_cast<const CObject&>(*entity);
                enableDisabledAttractors = object.objectFlags.bEnableDisabledAttractors;
                // Only static (Or non-streamed) objects that haven't exploded are considered
                if (!(entity->m_bIsStatic || entity->m_bDontStream) || object.objectFlags.bIsExploded) {
                    continue;
                }
            }

            const auto* const modelInfo = entity->GetModelInfo();
            for (int32 i = 0; i < modelInfo->m_n2dfxCount; i++) {
                C2dEffect* const effect = modelInfo->Get2dEffect(i);
                if (effect->m_Type != EFFECT_ATTRACTOR) {
                    continue;
                }
                if ((effect->pedAttractor.m_nFlags & 1) && !enableDisabledAttractors) { // Disabled attractor
                    continue;
                }
                AddAttractor(*effect, entity, ped);
            }
        }
    };

    for (int32 y = minY; y <= maxY; y++) {
        for (int32 x = minX; x <= maxX; x++) {
            ScanList(CWorld::GetSector(x, y).Buildings);
            ScanList(CWorld::GetRepeatSector(x & 0xF, y & 0xF).Objects);
        }
    }

    // --- Scripted (Opcode created) 2d effects ---
    for (auto i = 0; i < NUM_SCRIPTED_2D_EFFECTS; i++) {
        if (!CScripted2dEffects::ms_activated[i]) {
            continue;
        }

        const auto& userList = CScripted2dEffects::ms_userLists[i];
        if (userList.m_bUseList) {
            bool isApplicable = false;
            for (const auto userType : userList.m_UserTypes) {
                if (userType == (int32)ped.m_nModelIndex) {
                    isApplicable = true;
                    break;
                }
            }
            if (!isApplicable) { // Check the ped types too (0x5FE960)
                for (auto k = 0; k < (int32)std::size(userList.m_UserTypesByPedType); k++) {
                    if (userList.m_UserTypes[k] == -2 && userList.m_UserTypesByPedType[k] == (int32)ped.m_nPedType) {
                        isApplicable = true;
                        break;
                    }
                }
            }
            if (!isApplicable) {
                continue;
            }
        }

        AddAttractor(CScripted2dEffects::ms_effects[i], nullptr, ped);
    }

    // --- Report the newly found attractor (If different from the last one reported) ---
    C2dEffect* effect{};
    CEntity*   entity{};
    GetClosestAttractor(&effect, &entity);
    if (!effect || (effect == m_pEffectInUse && entity == m_pEntityInUse)) {
        return;
    }

    // Scripted effects can only be used by the ped they belong to
    if (effect->pedAttractor.m_nAttractorType == PED_ATTRACTOR_SCRIPTED) {
        if (GetScriptedAttractorPed(*C2dEffect::Cast<C2dEffectPedAttractor>(effect)) != &ped) {
            return;
        }
    }

    CEventAttractor event{ C2dEffect::Cast<C2dEffectPedAttractor>(effect), entity, false };
    if (ped.GetEventGroup().Add(&event, false)) {
        m_pEffectInUse = effect;
        m_pEntityInUse = entity;
    }
}

// 0x5FFFD0
void CAttractorScanner::AddAttractor(C2dEffect& effect, CEntity* entity, CPed& ped)
{
    auto* const fx = C2dEffect::Cast<C2dEffectPedAttractor>(&effect);
    const auto  slot = (int32)fx->m_nAttractorType;
    if (slot < 0 || slot >= (int32)std::size(m_apEntities)) { // NOTE: Not in the original (Which would index out of bounds)
        return;
    }

    // Shelter attractors are only of interest when it rains, every other one only when it doesn't
    if ((slot == PED_ATTRACTOR_SHELTER) != (CWeather::Rain >= 0.2f)) {
        return;
    }

    const CVector pos = [&] {
        if (entity) {
            return entity->TransformFromObjectSpace(effect.m_Pos);
        }
        return effect.m_Pos;
    }();

    const auto distSq = (ped.GetPosition() - pos).SquaredMagnitude();
    if (distSq >= m_aDistances[slot]) {
        return;
    }

    // Scripted effects have their own radius - Don't consider them when far away from it
    // NOTE: The original indexes the radii array with `GetIndex()` (Which is -1 for non-scripted effects)
    if (slot == PED_ATTRACTOR_SCRIPTED) {
        if (const auto idx = CScripted2dEffects::IndexOfEffect(fx)) {
            const auto radius = CScripted2dEffects::ms_radii[*idx];
            if (radius >= 0.f && radius * radius <= distSq) {
                return;
            }
        }
    }

    if (!GetPedAttractorManager()->HasEmptySlot(fx, entity)) {
        return;
    }

    CMatrix mat;
    if (entity) {
        mat = entity->GetMatrix();
    } else {
        mat.SetScale(1.f);
    }

    if (!CPedAttractorManager::IsApproachable(fx, mat, 0, &ped)) {
        return;
    }

    m_apEntities[slot] = entity;
    m_aDistances[slot] = distSq;
    m_apEffects[slot]  = &effect;
}

// 0x600180
void CAttractorScanner::GetClosestAttractor(C2dEffect** outEffect, CEntity** outEntity) const
{
    *outEffect = nullptr;
    *outEntity = nullptr;

    // Shelter always wins
    if (m_apEntities[PED_ATTRACTOR_SHELTER]) {
        *outEffect = m_apEffects[PED_ATTRACTOR_SHELTER];
        *outEntity = m_apEntities[PED_ATTRACTOR_SHELTER];
        return;
    }

    auto bestDist = FLT_MAX;
    for (auto i = 0; i < (int32)std::size(m_apEffects); i++) {
        if (m_aDistances[i] >= bestDist || !m_apEffects[i]) {
            continue;
        }
        bestDist   = m_aDistances[i];
        *outEffect = m_apEffects[i];
        *outEntity = m_apEntities[i];
    }
}

// 0x603570
CPed* CAttractorScanner::GetScriptedAttractorPed(const C2dEffectPedAttractor& effect)
{
    CPed* bestPed    = nullptr;
    auto  bestDistSq = FLT_MAX;

    // NOTE: Iterates the pool in reverse (As the original does)
    auto* const pool = GetPedPool();
    for (auto i = (int32)pool->GetSize(); i-- > 0;) {
        CPed* const ped = pool->GetAt(i);
        if (!ped) {
            continue;
        }

        if (const auto* const activeTask = ped->GetTaskManager().GetActiveTask(); activeTask && activeTask->GetTaskType() == TASK_COMPLEX_USE_EFFECT) {
            continue;
        }

        const auto distSq = (ped->GetPosition() - effect.m_Pos).SquaredMagnitude();
        if (distSq >= bestDistSq) {
            continue;
        }

        const auto idx = CScripted2dEffects::IndexOfEffect(&effect);
        if (!idx) { // Not a scripted effect
            continue;
        }
        const auto& userList = CScripted2dEffects::ms_userLists[*idx];
        if (userList.m_bUseList) {
            bool isApplicable = false;
            for (const auto userType : userList.m_UserTypes) {
                if (userType == (int32)ped->m_nModelIndex) {
                    isApplicable = true;
                    break;
                }
            }
            if (!isApplicable) {
                for (auto k = 0; k < (int32)std::size(userList.m_UserTypesByPedType); k++) {
                    if (userList.m_UserTypes[k] == -2 && userList.m_UserTypesByPedType[k] == (int32)ped->m_nPedType) {
                        isApplicable = true;
                        break;
                    }
                }
            }
            if (!isApplicable) {
                continue;
            }
        }

        CMatrix mat;
        mat.SetScale(1.f);
        if (CPedAttractorManager::IsApproachable(const_cast<C2dEffectPedAttractor*>(&effect), mat, 0, ped)) {
            bestDistSq = distSq;
            bestPed    = ped;
        }
    }

    return bestPed;
}
