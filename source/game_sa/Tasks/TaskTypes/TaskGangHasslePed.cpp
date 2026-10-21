#include "StdInc.h"
#include "TaskGangHasslePed.h"
#include "SeekEntity/TaskComplexSeekEntityStandard.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexKillPedOnFoot.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskSimpleRunAnim.h"

void CTaskGangHasslePed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskGangHasslePed, 0x86FA00, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x65FED0);
    RH_ScopedInstall(Destructor, 0x65FF60);

    RH_ScopedVMTInstall(CreateNextSubTask, 0x6642C0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x664380);
    RH_ScopedVMTInstall(ControlSubTask, 0x65FFE0);
}

// 0x65FED0
CTaskGangHasslePed::CTaskGangHasslePed(CPed* ped, int32 a3, int32 a4, int32 a5) : CTaskComplex() {
    m_nTime = 0;
    m_nSomeRandomShit = 0;
    m_bFirstSubTaskInitialised = 0;
    m_bRefreshTime = 0;
    dword10 = a3;
    m_RndMin = a4;
    m_Ped = ped;
    m_RndMax = a5;
    m_bAnimBlockRefAdded = false;
    CEntity::SafeRegisterRef(m_Ped);
}

// 0x65FF60
CTaskGangHasslePed::~CTaskGangHasslePed() {
    CEntity::SafeCleanUpRef(m_Ped);

    if (m_bAnimBlockRefAdded) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_bAnimBlockRefAdded = false;
    }
}

// 0x6642C0
CTask* CTaskGangHasslePed::CreateNextSubTask(CPed* ped) {
    // NOTE: The RNG is drawn before checking the task type (Keep this order to preserve the RNG sequence)
    const auto radius = static_cast<float>(CGeneral::GetRandomNumber()) * RAND_MAX_FLOAT_RECIPROCAL * 2.0f + 3.0f; // [3, 5]

    if (m_pSubTask->GetTaskType() == TASK_COMPLEX_KILL_PED_ON_FOOT) { // If the kill task finished, we're done
        return nullptr;
    }

    return new CTaskComplexSeekEntityStandard{ m_Ped, 999'999, 1'000, radius, 2.0f, 2.0f, false, true };
}

// 0x664380
CTask* CTaskGangHasslePed::CreateFirstSubTask(CPed* ped) {
    if (!m_Ped) {
        return nullptr;
    }

    const auto radius = static_cast<float>(CGeneral::GetRandomNumber()) * RAND_MAX_FLOAT_RECIPROCAL * 2.0f + 3.0f; // [3, 5]

    // Both RNG draws are made before `m_nTime` is stored (And before the task is allocated)
    const auto waitTime = static_cast<int32>(static_cast<float>(CGeneral::GetRandomNumber()) * (1.0f / 32768.0f) * static_cast<float>(m_RndMax - m_RndMin));
    m_nTime                    = static_cast<int32>(CTimer::GetTimeInMS());
    m_nSomeRandomShit          = m_RndMin + waitTime;
    m_bFirstSubTaskInitialised = true;

    return new CTaskComplexSeekEntityStandard{ m_Ped, 999'999, 1'000, radius, 2.0f, 2.0f, false, true };
}

// 0x65FFE0
CTask* CTaskGangHasslePed::ControlSubTask(CPed* ped) {
    if (!m_Ped) {
        return nullptr;
    }

    // Once the wait window (seeded by `CreateFirstSubTask`) has expired, switch to killing (If aggressive)
    if (m_bFirstSubTaskInitialised) {
        if (m_bRefreshTime) {
            m_nTime        = static_cast<int32>(CTimer::GetTimeInMS());
            m_bRefreshTime = false;
        }

        // Unsigned comparison (As in the exe)
        if (CTimer::HasTimePointPassed(static_cast<uint32>(m_nTime) + static_cast<uint32>(m_nSomeRandomShit))) {
            if (dword10 == 2) { // Aggressive
                if (m_pSubTask->GetTaskType() != TASK_COMPLEX_KILL_PED_ON_FOOT) { // Don't restart the kill task if it's already running
                    return new CTaskComplexKillPedOnFoot{ m_Ped, -1, 0, 0, 0, 1 };
                }
            } else if (dword10 != 1) { // Not aggressive, but also not just "hassling" => We're done
                return nullptr;
            }
        }
    }

    // Make sure the anims are loaded (if they can/need to be)
    if (m_bAnimBlockRefAdded) {
        if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
            m_bAnimBlockRefAdded = false;
        }
    } else if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        const auto blk = CAnimManager::GetAnimationBlockIndex("gangs");
        if (CAnimManager::GetAnimBlocks()[blk].IsLoaded) {
            CAnimManager::AddAnimBlockRef(blk);
            m_bAnimBlockRefAdded = true;
        } else {
            CStreaming::RequestModel(IFPToModelId(blk), STREAMING_KEEP_IN_MEMORY);
        }
    }

    if (m_bAnimBlockRefAdded && !ped->IsPlayingHandSignal() && !ped->GetTaskManager().GetTaskSecondary(TASK_SECONDARY_PARTIAL_ANIM)) {
        const auto rnd = CGeneral::GetRandomNumberInRange<int32>(0, 200); // 0..199

        CTask* secondary{};
        if (rnd > 50 && rnd < 56) { // 5%
            secondary = new CTaskSimpleRunAnim{
                ANIM_GROUP_GANGS,
                // 0..7 (Truncated, not rounded) => ANIM_ID_PRTIAL_GNGTLKA..ANIM_ID_PRTIAL_GNGTLKH
                static_cast<AnimationId>(ANIM_ID_PRTIAL_GNGTLKA + static_cast<int32>(CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 8.0f)),
                4.0f,
                false
            };
        } else if (rnd == 100) { // 0.5%
            secondary = new CTaskComplexPlayHandSignalAnim{ ANIM_ID_UNDEFINED, 4.0f };
        }
        ped->GetTaskManager().SetTaskSecondary(secondary, TASK_SECONDARY_PARTIAL_ANIM); // nullptr clears the slot (As in the exe)
    }

    if (dword10 == 0) {
        ped->Say(CTX_GLOBAL_EYEING_PED);
    } else if (dword10 == 1 || dword10 == 2) {
        CTaskComplexGangLeader::DoGangAbuseSpeech(ped, m_Ped);
    }

    return m_pSubTask;
}
