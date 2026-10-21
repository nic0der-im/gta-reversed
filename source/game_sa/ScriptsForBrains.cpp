#include "StdInc.h"

#include "ScriptsForBrains.h"

void CScriptsForBrains::InjectHooks() {
    RH_ScopedClass(CScriptsForBrains);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Init, 0x46A8C0);
    //RH_ScopedInstall(SwitchAllObjectBrainsWithThisID, 0x46A900, {.Reversed = false});
    RH_ScopedInstall(AddNewScriptBrain, 0x46A930);
    RH_ScopedInstall(AddNewStreamedScriptBrainForCodeUse, 0x46A9C0);
    RH_ScopedInstall(GetIndexOfScriptBrainWithThisName, 0x46AA30);
    RH_ScopedInstall(HasAttractorScriptBrainWithThisNameLoaded, 0x46AB20);
    RH_ScopedInstall(StartNewStreamedScriptBrain, 0x46B270);
    RH_ScopedInstall(StartAttractorScriptBrainWithThisName, 0x46B390);
    RH_ScopedInstall(StartOrRequestNewStreamedScriptBrain, 0x46CD80);
    //RH_ScopedInstall(StartOrRequestNewStreamedScriptBrainWithThisName, 0x46CED0, {.Reversed = false});
    RH_ScopedInstall(IsObjectWithinBrainActivationRange, 0x46B3D0);
    RH_ScopedInstall(CheckIfNewEntityNeedsScript, 0x46FF20);
    RH_ScopedInstall(MarkAttractorScriptBrainWithThisNameAsNoLongerNeeded, 0x46AAE0);
    RH_ScopedInstall(RequestAttractorScriptBrainWithThisName, 0x46AA80);
}


// 0x46A8C0
void CScriptsForBrains::Init() {
    for (auto& script : m_aScriptForBrains) {
        script = tScriptForBrains();
    }
}

// 0x46A930
// NOTE: The parameter names follow the header, but note that `attachType` is what's stored
//       as `m_TypeOfBrain` (And `Type` as `m_ObjectGroupingId`) - See the SCM handlers.
void CScriptsForBrains::AddNewScriptBrain(int16 ImgIndex, int16 Model, uint16 priority, int8 attachType, int8 Type, float Radius) {
    const auto slot = rng::find_if(m_aScriptForBrains, [](const tScriptForBrains& e) {
        return e.m_StreamedScriptIndex == -1;
    });
    if (slot == m_aScriptForBrains.end()) {
        return;
    }

    slot->m_StreamedScriptIndex         = ImgIndex;
    slot->m_PedModelOrPedGeneratorIndex = Model;
    slot->m_PercentageChance            = priority;
    slot->m_ObjectGroupingId            = Type;
    slot->m_TypeOfBrain                 = attachType;
    slot->m_bBrainActive                = true;
    slot->m_ObjectBrainActivationRadius = Radius > 0.0f ? Radius : 5.0f;
}

// 0x46A9C0
void CScriptsForBrains::AddNewStreamedScriptBrainForCodeUse(int16 a2, char* a3, int8 attachtype) {
    const auto slot = rng::find_if(m_aScriptForBrains, [](const tScriptForBrains& e) {
        return e.m_StreamedScriptIndex == -1;
    });
    if (slot == m_aScriptForBrains.end()) {
        return;
    }

    slot->m_StreamedScriptIndex = a2;
    //! Retail does an unbounded `strcpy` into this 8 byte field - Don't reproduce the overflow.
    strncpy(slot->m_ScriptName, a3, sizeof(slot->m_ScriptName) - 1);
    slot->m_ScriptName[sizeof(slot->m_ScriptName) - 1] = '\0';
    slot->m_TypeOfBrain                 = attachtype;
    slot->m_ObjectGroupingId            = -1;
    slot->m_bBrainActive                = true;
    slot->m_ObjectBrainActivationRadius = 5.0f;
}

// 0x46FF20
void CScriptsForBrains::CheckIfNewEntityNeedsScript(CEntity* entity, int8 attachType, void* /*unused*/) {
    // Early out if the entity already has (Or is waiting for) a script brain
    if (attachType == 0) {
        const auto ped = entity->AsPed();
        if (ped->bHasAScriptBrain || ped->bWaitingForScriptBrainToLoad) {
            return;
        }
    } else {
        if (entity->AsObject()->objectFlags.b0x100000_0x200000) {
            return;
        }
    }

    for (auto i = 0u; i < m_aScriptForBrains.size(); i++) {
        const auto& e = m_aScriptForBrains[i];
        if (e.m_TypeOfBrain != attachType || e.m_PedModelOrPedGeneratorIndex != (int16)entity->m_nModelIndex) {
            continue;
        }

        //! Not `CGeneral::GetRandomNumberInRange`, as its reciprocal is `1/32767`, unlike retail's `1/32768`
        const auto chance = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 100.0f);
        if (chance < e.m_PercentageChance) {
            StartOrRequestNewStreamedScriptBrain((uint8)i, entity, attachType, true);
            return;
        }
    }
}

// 0x46AA80
void CScriptsForBrains::RequestAttractorScriptBrainWithThisName(const char* name) {
    const auto idx = GetIndexOfScriptBrainWithThisName(name, 5);
    if (idx < 0) {
        return;
    }

    const auto scriptIndex = m_aScriptForBrains[idx].m_StreamedScriptIndex;
    const auto modelId     = SCMToModelId(scriptIndex);
    if (!CStreaming::IsModelLoaded(modelId)) {
        CStreaming::RequestModel(modelId, STREAMING_PRIORITY_REQUEST);
    }

    CTheScripts::StreamedScripts.m_aScripts[scriptIndex].m_NumberOfUsers++;
}

// 0x46AAE0
void CScriptsForBrains::MarkAttractorScriptBrainWithThisNameAsNoLongerNeeded(const char* name) {
    const auto idx = GetIndexOfScriptBrainWithThisName(name, 5);
    if (idx < 0) {
        return;
    }

    auto& numUsers = CTheScripts::StreamedScripts.m_aScripts[m_aScriptForBrains[idx].m_StreamedScriptIndex].m_NumberOfUsers;
    if (numUsers) {
        numUsers--;
    }
}

// 0x46B3D0
bool CScriptsForBrains::IsObjectWithinBrainActivationRange(CObject* entity, const CVector& point) {
    const auto& e = m_aScriptForBrains[entity->m_nStreamedScriptBrainToLoad];
    if (e.m_TypeOfBrain != 1) {
        return false;
    }
    return (point - entity->GetPosition()).Magnitude() < e.m_ObjectBrainActivationRadius;
}

// 0x46B270
void CScriptsForBrains::StartNewStreamedScriptBrain(uint8 index, CEntity* entity, bool bHasAScriptBrain) {
    const auto& e = m_aScriptForBrains[index];

    auto* const script = CTheScripts::StreamedScripts.StartNewStreamedScript(e.m_StreamedScriptIndex);
    script->m_ExternalType = e.m_TypeOfBrain;

    switch (e.m_TypeOfBrain) {
    case 0: // Ped
    case 3: // Ped (For code use)
    case 5: // Ped (Attractor)
    {
        auto* const ped = entity->AsPed();

        script->m_LocalVars[0].uParam = CPools::GetPedRef(ped);

        ped->bHasAScriptBrain = true;
        if (e.m_TypeOfBrain == 5) {
            script->m_LocalVars[1].uParam = bHasAScriptBrain;
        }
        ped->bWaitingForScriptBrainToLoad = false;

        CTheScripts::RemoveFromWaitingForScriptBrainArray(entity, ped->m_StreamedScriptBrainToLoad);
        ped->m_StreamedScriptBrainToLoad = -1;
        break;
    }
    case 1: // Object
    case 4: // Object
    {
        auto* const obj = entity->AsObject();

        script->m_LocalVars[0].uParam = CPools::GetObjectRef(obj);

        obj->objectFlags.b0x100000_0x200000 |= 3;
        break;
    }
    }
}

// 0x46CD80
void CScriptsForBrains::StartOrRequestNewStreamedScriptBrain(uint8 index, CEntity* entity, int8 attachType, bool bAddToWaitingArray) {
    if (bAddToWaitingArray) {
        if (!m_aScriptForBrains[index].m_bBrainActive) {
            return;
        }

        switch (attachType) {
        case 0:
        case 3:
            if (entity->AsPed()->bHasAScriptBrain || entity->AsPed()->bWaitingForScriptBrainToLoad) {
                return;
            }
            break;
        case 1:
        case 4:
            if (entity->AsObject()->objectFlags.b0x100000_0x200000) {
                return;
            }
            break;
        }
    }

    if (attachType == 1) {
        auto* const obj = entity->AsObject();
        if (bAddToWaitingArray) {
            obj->objectFlags.b0x100000_0x200000 = 1; // b0x100000 (Waiting)
            obj->m_nStreamedScriptBrainToLoad   = index;
            CTheScripts::AddToWaitingForScriptBrainArray(entity, index);
        } else {
            obj->objectFlags.b0x100000_0x200000 = 2; // b0x200000 (Has)
        }
    }

    const auto modelId = SCMToModelId(m_aScriptForBrains[index].m_StreamedScriptIndex);
    if (CStreaming::IsModelLoaded(modelId)) {
        StartNewStreamedScriptBrain(index, entity, false);
        return;
    }

    CStreaming::RequestModel(modelId, STREAMING_MISSION_REQUIRED);

    // Only peds (And type 4) are added to the waiting array
    if (attachType != 0 && (attachType <= 2 || attachType > 4)) {
        return;
    }
    if (!bAddToWaitingArray) {
        return;
    }

    auto* const ped = entity->AsPed();

    ped->bWaitingForScriptBrainToLoad = true;
    ped->m_StreamedScriptBrainToLoad   = index;
    CTheScripts::AddToWaitingForScriptBrainArray(entity, index);
}

bool CScriptsForBrains::HasAttractorScriptBrainWithThisNameLoaded(const char* name) {
    if (const auto idx = GetIndexOfScriptBrainWithThisName(name, 5); idx >= 0) {
        return CStreaming::IsModelLoaded(SCMToModelId(m_aScriptForBrains[idx].m_StreamedScriptIndex));
    }
    return false;
}

bool CScriptsForBrains::IsObjectWithinBrainActivationRange(CObject* entity, const CVector& point) {
    NOTSA_UNREACHABLE();
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
