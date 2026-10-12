#include "StdInc.h"
#include "TaskComplexSunbathe.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleRunTimedAnim.h"
#include "TaskSimplePause.h"
#include "InterestingEvents.h"

namespace {
// Reproduces 0x407180: min + trunc((rand & 0xFFFF) * (max - min) / 32768)
int32 RandomIntOriginal(int32 min, int32 max) {
    return min + static_cast<int32>((static_cast<int64>(CGeneral::GetRandomNumber() & 0xFFFF) * (max - min)) / 32768);
}
}

void CTaskComplexSunbathe::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexSunbathe, 0x86e0ac, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x631F80);
    RH_ScopedInstall(Destructor, 0x632050);

    //RH_ScopedGlobalInstall(CanSunbathe, 0x632140, { .Reversed = false });
    //RH_ScopedGlobalInstall(ShouldLoadSunbatheAnims, 0x632190, { .Reversed = false });

    //RH_ScopedInstall(CreateSubTask, 0x638290, { .Reversed = false });

    RH_ScopedVMTInstall(Clone, 0x6366A0);
    RH_ScopedVMTInstall(GetTaskType, 0x632040);
    RH_ScopedVMTInstall(MakeAbortable, 0x6320F0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x6399F0);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x639CB0);
    RH_ScopedVMTInstall(ControlSubTask, 0x6381A0);

}

// 0x631f80
CTaskComplexSunbathe::CTaskComplexSunbathe(CObject* towel, bool bStartStanding) :
    m_pTowel{towel},
    m_bStartStanding{bStartStanding}
{
    if (m_pTowel) {
        CEntity::SafeRegisterRef(m_pTowel);
        m_pTowel->m_nObjectType = OBJECT_TYPE_DECORATION;
    }

    const auto LoadAnim = [](int32& idx, CAnimBlock*& blk, const char* name) {
        idx = CAnimManager::GetAnimationBlockIndex(name);
        blk = CAnimManager::GetAnimationBlock(name);
    };
    LoadAnim(m_BeachAnimBlockIndex, m_pBeachAnimBlock, "beach");
    LoadAnim(m_SunbatheAnimBlockIndex, m_pSunbatheAnimBlock, "sunbathe");

    // Refs added later in another function
}

CTaskComplexSunbathe::CTaskComplexSunbathe(const CTaskComplexSunbathe& o) :
    CTaskComplexSunbathe{ o.m_pTowel, o.m_bStartStanding }
{
}

// 0x632050
CTaskComplexSunbathe::~CTaskComplexSunbathe() {
    if (m_pTowel) {
        m_pTowel->m_nObjectType = OBJECT_TEMPORARY;
        CEntity::SafeCleanUpRef(m_pTowel);
    }

    const auto UnrefAnimBlock = [](bool& refed, int32 blkIdx) {
        if (refed) {
            CAnimManager::RemoveAnimBlockRef(blkIdx);
            refed = false;
        }
    };
    UnrefAnimBlock(m_bBeachAnimsReferenced, m_BeachAnimBlockIndex);
    UnrefAnimBlock(m_bSunbatheAnimsReferenced, m_SunbatheAnimBlockIndex);
}

// 0x632140
bool CTaskComplexSunbathe::CanSunbathe() {
    const auto hours = CClock::GetGameClockHours();
    if (hours < 10 || hours >= 18) { // NOTE: hours [10, 18)
        return false;
    }
    switch ((int16)CWeather::NewWeatherType) {
    case 1: case 3: case 0xE: case 5: case 0xA: case 0x12:
    case 0: case 2: case 0xD: case 6: case 0xB: case 0x11: // 0x62F2D0
        return true;
    }
    return false;
}

// 0x632190
bool CTaskComplexSunbathe::ShouldLoadSunbatheAnims() {
    const auto* const ped = CWorld::Players[CWorld::PlayerInFocus].m_pPed;
    if (ped->bInVehicle && ped->m_pVehicle) {
        return !(ped->m_pVehicle->m_vecMoveSpeed.SquaredMagnitude() > 0.04f);
    }
    return true;
}

// 0x638290
CTask* CTaskComplexSunbathe::CreateSubTask(eTaskType taskType, CPed* ped) {
    if (m_pSubTask && m_pSubTask->GetTaskType() == taskType) {
        return m_pSubTask;
    }
    switch (taskType) {
    case TASK_SIMPLE_SUNBATHE: { // 0x1a2
        const auto timeLeft = m_BathingTimer.m_bStarted // 0x62EB80
            ? m_BathingTimer.m_nInterval - (int32)CTimer::GetTimeInMS() + (int32)m_BathingTimer.m_nStartTime
            : 0;
        int32 durationMs;
        if (!m_pSunbatheAnimBlock->IsLoaded || (m_SunbatherType != SUNBATHER_MALE_1 && m_SunbatherType != SUNBATHER_FEMALE_1)) {
            durationMs = timeLeft + 1000;
        } else {
            durationMs = std::min(timeLeft + 1000, 12000);
            durationMs = RandomIntOriginal(3000, durationMs);
        }
        return new CTaskSimpleRunTimedAnim{
            ANIM_GROUP_BEACH,
            (AnimationId)(m_SunbatherType + BEACH_PARKSIT_M_LOOP),
            4.0f,
            -4.0f,
            (uint32)durationMs,
            TASK_SIMPLE_SUNBATHE,
            "sunbathe",
            true
        };
    }
    case TASK_SIMPLE_START_SUNBATHING: // 0x1ac
        return new CTaskSimpleRunAnim{
            ANIM_GROUP_SUNBATHE,
            (AnimationId)(m_SunbatherType + SUNBATHE_PARKSIT_M_IN),
            4.0f,
            TASK_SIMPLE_START_SUNBATHING,
            "start sunbathing",
            true
        };
    case TASK_SIMPLE_IDLE_SUNBATHING: { // 0x1ad
        int32 animId;
        if (m_SunbatherType == SUNBATHER_MALE_1) {
            animId = RandomIntOriginal(0, 3) + SUNBATHE_PARKSIT_M_IDLEA;
        } else if (m_SunbatherType == SUNBATHER_FEMALE_1) {
            animId = RandomIntOriginal(0, 3) + SUNBATHE_PARKSIT_W_IDLEA;
        } else {
            animId = TASK_SIMPLE_IDLE_SUNBATHING; // BUG: Original code uses the task type as the anim id here
        }
        return new CTaskSimpleRunAnim{
            ANIM_GROUP_SUNBATHE,
            (AnimationId)animId,
            4.0f,
            TASK_SIMPLE_IDLE_SUNBATHING,
            "idle sunbathing",
            true
        };
    }
    case TASK_SIMPLE_STOP_SUNBATHING: // 0x1ae
        return new CTaskSimpleRunAnim{
            ANIM_GROUP_SUNBATHE,
            (AnimationId)(m_SunbatherType + SUNBATHE_PARKSIT_M_OUT),
            4.0f,
            TASK_SIMPLE_STOP_SUNBATHING,
            "stop sunbathing",
            false
        };
    case TASK_SIMPLE_PAUSE: // 0xca
        if ((!m_pSunbatheAnimBlock->IsLoaded && ShouldLoadSunbatheAnims()) || !m_pBeachAnimBlock->IsLoaded) {
            return new CTaskSimplePause{10000};
        }
        return new CTaskSimplePause{RandomIntOriginal(1000, 5000)};
    default: // Includes TASK_FINISHED (0x516)
        return nullptr;
    }
}

// 0x6320F0
bool CTaskComplexSunbathe::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (event && (event->GetEventType() == EVENT_PED_COLLISION_WITH_PED || event->GetEventType() == EVENT_PED_COLLISION_WITH_PLAYER)) {
        return false;
    }
    if (!m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }
    m_bAborted = true;
    return true;
}

// 0x6399F0
CTask* CTaskComplexSunbathe::CreateNextSubTask(CPed* ped) {
    if (ped->m_nPedType == PED_TYPE_PROSTITUTE || ped->m_nPedType == PED_TYPE_CIVFEMALE) {
        g_InterestingEvents.Add(CInterestingEvents::INTERESTING_EVENT_2, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_PAUSE: // 0xCA
        if (!m_bBathing) {
            if (m_pSunbatheAnimBlock->IsLoaded && m_pBeachAnimBlock->IsLoaded) {
                return CreateSubTask(TASK_SIMPLE_START_SUNBATHING, ped);
            }
        } else if (ShouldLoadSunbatheAnims() && m_pSunbatheAnimBlock->IsLoaded) {
            return CreateSubTask(TASK_SIMPLE_STOP_SUNBATHING, ped);
        }
        break;
    case TASK_SIMPLE_SUNBATHE: // 0x1A2
        if (!m_pSunbatheAnimBlock->IsLoaded) {
            return CreateSubTask(TASK_SIMPLE_PAUSE, ped);
        }
        if (CanSunbathe() && !m_BathingTimer.IsOutOfTime() && (m_SunbatherType == SUNBATHER_MALE_1 || m_SunbatherType == SUNBATHER_FEMALE_1)) {
            return CreateSubTask(TASK_SIMPLE_IDLE_SUNBATHING, ped);
        }
        return CreateSubTask(TASK_SIMPLE_STOP_SUNBATHING, ped);
    case TASK_SIMPLE_START_SUNBATHING: // 0x1AC
        if (m_pBeachAnimBlock->IsLoaded) {
            m_BathingTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_BathingTimer.m_nInterval  = RandomIntOriginal(20000, 100000);
            m_BathingTimer.m_bStarted   = true;
            const auto task = CreateSubTask(TASK_SIMPLE_SUNBATHE, ped);
            m_bBathing = true;
            return task;
        }
        break;
    case TASK_SIMPLE_IDLE_SUNBATHING: // 0x1AD
        return CreateSubTask(m_pBeachAnimBlock->IsLoaded ? TASK_SIMPLE_SUNBATHE : TASK_FINISHED, ped);
    case TASK_SIMPLE_STOP_SUNBATHING: { // 0x1AE
        const auto task = CreateSubTask(TASK_FINISHED, ped);
        m_bBathing = false;
        return task;
    }
    default:
        return nullptr;
    }
    return CreateSubTask(TASK_FINISHED, ped);
}

// 0x639CB0
CTask* CTaskComplexSunbathe::CreateFirstSubTask(CPed* ped) {
    CGeneral::GetRandomNumber(); // NOTE: Result discarded (but advances the RNG state)

    if (ped->m_nPedType == PED_TYPE_CIVFEMALE) {
        m_SunbatherType = static_cast<eSunbatherType>(RandomIntOriginal(2, 5));
    } else if (CPopulation::IsSunbather((eModelID)ped->m_nModelIndex)) {
        m_SunbatherType = (eSunbatherType)(CGeneral::GetRandomNumber() < 0x3FFF);
    } else {
        m_SunbatherType = SUNBATHER_MALE_2;
    }

    if (!m_pBeachAnimBlock->IsLoaded) {
        CStreaming::RequestModel(IFPToModelId(m_BeachAnimBlockIndex), STREAMING_KEEP_IN_MEMORY);
    }
    if (!m_pSunbatheAnimBlock->IsLoaded) {
        CStreaming::RequestModel(IFPToModelId(m_SunbatheAnimBlockIndex), STREAMING_KEEP_IN_MEMORY);
    }

    m_bBathing = false;

    const auto shouldLoad = ShouldLoadSunbatheAnims();
    const auto beachLoaded = m_pBeachAnimBlock->IsLoaded;

    const auto StartBathing = [&] { // 0x639DCC
        m_BathingTimer.m_nStartTime = CTimer::GetTimeInMS();
        m_BathingTimer.m_nInterval  = RandomIntOriginal(20000, 100000);
        m_BathingTimer.m_bStarted   = true;
        m_bBathing = true;
        return CreateSubTask(TASK_SIMPLE_SUNBATHE, ped);
    };

    if (m_bStartStanding) {
        if (shouldLoad) {
            if (!m_pTowel && m_pSunbatheAnimBlock->IsLoaded) {
                return CreateSubTask(TASK_SIMPLE_START_SUNBATHING, ped);
            }
            return CreateSubTask(TASK_SIMPLE_PAUSE, ped); // 0x639DBA
        }
    } else if (shouldLoad) {
        // -> 0x639DA5
        return beachLoaded ? StartBathing() : CreateSubTask(TASK_SIMPLE_PAUSE, ped);
    }
    // 0x639D9B
    return beachLoaded ? StartBathing() : nullptr;
}

// 0x6381A0
CTask* CTaskComplexSunbathe::ControlSubTask(CPed* ped) {
    if (m_bAborted) {
        return nullptr;
    }

    if (!m_bBeachAnimsReferenced) {
        if (m_pBeachAnimBlock->IsLoaded) {
            CAnimManager::AddAnimBlockRef(m_BeachAnimBlockIndex);
            m_bBeachAnimsReferenced = true;
        } else {
            CStreaming::RequestModel(IFPToModelId(m_BeachAnimBlockIndex), STREAMING_KEEP_IN_MEMORY);
        }
    }

    if (!m_bSunbatheAnimsReferenced && ShouldLoadSunbatheAnims()) {
        if (m_pSunbatheAnimBlock->IsLoaded) {
            CAnimManager::AddAnimBlockRef(m_SunbatheAnimBlockIndex);
            m_bSunbatheAnimsReferenced = true;
        } else {
            CStreaming::RequestModel(IFPToModelId(m_SunbatheAnimBlockIndex), STREAMING_KEEP_IN_MEMORY);
        }
    }

    if (m_bBathing) {
        ped->m_pedIK.bSlopePitch = true; // 0x528 |= 8
        ped->bFallenDown = true;         // 0x474 |= 0x100000
    }

    if (!CanSunbathe()) {
        if (m_BathingTimer.m_bStarted) {
            m_BathingTimer.m_bStopped = true;
            m_BathingTimer.m_nInterval -= CTimer::GetTimeInMS() - m_BathingTimer.m_nStartTime;
        }
        if (m_pSubTask->GetTaskType() == TASK_SIMPLE_SUNBATHE) {
            m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
        }
    }
    return m_pSubTask;
}
