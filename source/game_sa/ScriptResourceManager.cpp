#include "StdInc.h"

#include "ScriptResourceManager.h"

void CScriptResourceManager::InjectHooks() {
    RH_ScopedClass(CScriptResourceManager);
    RH_ScopedCategory("Scripts");

    RH_ScopedInstall(Initialise, 0x470480);
    RH_ScopedInstall(AddToResourceManager, 0x4704B0);
    RH_ScopedInstall(RemoveFromResourceManager, 0x470510);
    RH_ScopedInstall(HasResourceBeenRequested, 0x470620);
    //RH_ScopedInstall(Load, 0x0, { .Reversed = false });
    //RH_ScopedInstall(Save, 0x0, { .Reversed = false });
}

// 0x470480
void CScriptResourceManager::Initialise() {
    m_aScriptResources.fill(tScriptResource());
}

// 0x4704B0
void CScriptResourceManager::AddToResourceManager(int32 modelId, eScriptResourceType type, CRunningScript* script) {
    tScriptResource* freeSlot = nullptr;
    for (auto& res : m_aScriptResources) {
        if (res.m_nModelId == modelId && res.m_nType == type && res.m_pThread == script) {
            return; // Already added
        }
        if (res.m_nType == RESOURCE_TYPE_DEFAULT) {
            freeSlot = &res; // NOTE: last free slot is used
        }
    }
    if (freeSlot) {
        freeSlot->m_nModelId = modelId;
        freeSlot->m_nType    = type;
        freeSlot->m_pThread  = script;
    }
}

// 0x470510
bool CScriptResourceManager::RemoveFromResourceManager(int32 modelId, eScriptResourceType type, CRunningScript* script) {
    tScriptResource* found = nullptr;
    bool usedByOthers = false;
    for (auto& res : m_aScriptResources) {
        if (res.m_nModelId != modelId || res.m_nType != type) {
            continue;
        }
        if (res.m_pThread == script) {
            found = &res; // NOTE: last match is removed
        } else {
            usedByOthers = true;
        }
    }
    if (found) {
        found->m_nModelId = -1;
        found->m_nType    = RESOURCE_TYPE_DEFAULT;
        found->m_pThread  = nullptr;
    }
    return !usedByOthers;
}

// 0x470620
bool CScriptResourceManager::HasResourceBeenRequested(int32 modelId, eScriptResourceType type) {
    return rng::any_of(m_aScriptResources, [&](auto& res) {
        return res.m_nModelId == modelId && res.m_nType == type;
    });
}

// 0x0
bool CScriptResourceManager::Load() {
    assert(false);
    return true;
}

// 0x0
bool CScriptResourceManager::Save() {
    assert(false);
    return true;
}
