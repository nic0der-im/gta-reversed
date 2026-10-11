#include "StdInc.h"

#include "ScriptsForBrains.h"

void CScriptsForBrains::InjectHooks() {
    RH_ScopedClass(CScriptsForBrains);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Init, 0x46A8C0);
    //RH_ScopedInstall(SwitchAllObjectBrainsWithThisID, 0x46A900, {.Reversed = false});
    //RH_ScopedInstall(AddNewScriptBrain, 0x46A930, {.Reversed = false});
    //RH_ScopedInstall(AddNewStreamedScriptBrainForCodeUse, 0x46A9C0, {.Reversed = false});
    RH_ScopedInstall(GetIndexOfScriptBrainWithThisName, 0x46AA30);
    RH_ScopedInstall(HasAttractorScriptBrainWithThisNameLoaded, 0x46AB20);
    //RH_ScopedInstall(StartNewStreamedScriptBrain, 0x46B270, {.Reversed = false});
    RH_ScopedInstall(StartAttractorScriptBrainWithThisName, 0x46B390);
    RH_ScopedInstall(StartOrRequestNewStreamedScriptBrain, 0x46CD80);
    //RH_ScopedInstall(StartOrRequestNewStreamedScriptBrainWithThisName, 0x46CED0, {.Reversed = false});
    RH_ScopedInstall(IsObjectWithinBrainActivationRange, 0x46B3D0);
}


// 0x46A8C0
void CScriptsForBrains::Init() {
    for (auto& script : m_aScriptForBrains) {
        script = tScriptForBrains();
    }
}

void CScriptsForBrains::CheckIfNewEntityNeedsScript(CEntity* entity, int8 attachType, void* unused) {
    plugin::CallMethod<0x46FF20, CScriptsForBrains*, CEntity*, int8, void*>(this, entity, attachType, unused);
}

void CScriptsForBrains::MarkAttractorScriptBrainWithThisNameAsNoLongerNeeded(const char* name) {
    plugin::CallMethod<0x46AAE0, CScriptsForBrains*, const char*>(this, name);
}

void CScriptsForBrains::RequestAttractorScriptBrainWithThisName(const char* name) {
    plugin::CallMethod<0x46AA80, CScriptsForBrains*, const char*>(this, name);
}

void CScriptsForBrains::StartNewStreamedScriptBrain(uint8 index, CEntity* entity, bool bHasAScriptBrain) {
    plugin::CallMethodAndReturn<void, 0x46B270, CScriptsForBrains*, uint8, CEntity*, uint8>(this, index, entity, bHasAScriptBrain);
}

void CScriptsForBrains::StartOrRequestNewStreamedScriptBrain(uint8 index, CEntity* entity, int8 attachType, bool bAddToWaitingArray) {
    // Flag masks as used by the original (CPed: dword @ 0x474, CObject: m_nObjectFlags)
    constexpr uint32 OBJ_WAITING_BRAIN = 0x100000;
    constexpr uint32 OBJ_HAS_BRAIN     = 0x200000;

    if (bAddToWaitingArray) {
        if (!m_aScriptForBrains[index].m_bBrainActive) {
            return;
        }
        switch (attachType) {
        case 0:
        case 3: // Ped
            if (entity->AsPed()->bHasAScriptBrain || entity->AsPed()->bWaitingForScriptBrainToLoad) {
                return;
            }
            break;
        case 1:
        case 4: // Object
            if (entity->AsObject()->m_nObjectFlags & (OBJ_WAITING_BRAIN | OBJ_HAS_BRAIN)) {
                return;
            }
            break;
        }
    }

    if (attachType == 1) {
        auto* const obj = entity->AsObject();
        if (bAddToWaitingArray) {
            obj->m_nStreamedScriptBrainToLoad = index;
            obj->m_nObjectFlags = (obj->m_nObjectFlags & ~OBJ_HAS_BRAIN) | OBJ_WAITING_BRAIN;
            CTheScripts::AddToWaitingForScriptBrainArray(obj, index);
        } else {
            obj->m_nObjectFlags = (obj->m_nObjectFlags & ~OBJ_WAITING_BRAIN) | OBJ_HAS_BRAIN;
        }
    }

    const auto scriptModelId = SCMToModelId(m_aScriptForBrains[index].m_StreamedScriptIndex);
    if (CStreaming::IsModelLoaded(scriptModelId)) {
        StartNewStreamedScriptBrain(index, entity, false);
        return;
    }

    CStreaming::RequestModel(scriptModelId, STREAMING_MISSION_REQUIRED);
    if ((attachType == 0 || (attachType > 2 && attachType < 5)) && bAddToWaitingArray) {
        auto* const ped = entity->AsPed();
        ped->m_StreamedScriptBrainToLoad = index;
        ped->bWaitingForScriptBrainToLoad = true;
        CTheScripts::AddToWaitingForScriptBrainArray(ped, index);
    }
}

bool CScriptsForBrains::HasAttractorScriptBrainWithThisNameLoaded(const char* name) {
    if (const auto idx = GetIndexOfScriptBrainWithThisName(name, 5); idx >= 0) {
        return CStreaming::IsModelLoaded(SCMToModelId(m_aScriptForBrains[idx].m_StreamedScriptIndex));
    }
    return false;
}

bool CScriptsForBrains::IsObjectWithinBrainActivationRange(CObject* entity, const CVector& point) {
    const auto& brain = m_aScriptForBrains[entity->m_nStreamedScriptBrainToLoad];
    if (brain.m_TypeOfBrain != 1) {
        return false;
    }
    return DistanceBetweenPoints(point, entity->GetPosition()) < brain.m_ObjectBrainActivationRadius;
}

int16 CScriptsForBrains::GetIndexOfScriptBrainWithThisName(const char* name, int8 type) {
    const auto it = rng::find_if(m_aScriptForBrains, [=](tScriptForBrains& script) {
        return script.m_TypeOfBrain == type && !_stricmp(script.m_ScriptName, name);
    });
    return it != m_aScriptForBrains.end()
        ? rng::distance(m_aScriptForBrains.begin(), it)
        : -1;
}

void CScriptsForBrains::StartAttractorScriptBrainWithThisName(const char* name, CPed* ped, bool bHasAScriptBrain) {
    if (!ped->bWaitingForScriptBrainToLoad && !ped->bHasAScriptBrain) {
        if (const auto idx = GetIndexOfScriptBrainWithThisName(name, 5); idx >= 0) {
            StartNewStreamedScriptBrain(static_cast<uint8>(idx), ped, bHasAScriptBrain);
        }
    }
}
