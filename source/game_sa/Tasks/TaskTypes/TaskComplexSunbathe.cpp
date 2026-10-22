#include "StdInc.h"
#include "TaskComplexSunbathe.h"

#include "TaskSimplePause.h"
#include "TaskSimpleRunAnim.h"
#include "TaskSimpleRunTimedAnim.h"
#include "Population.h"
#include "InterestingEvents.h"
#include "Streaming.h"
#include "AnimManager.h"
#include "AnimBlock.h"

void CTaskComplexSunbathe::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexSunbathe, 0x86e0ac, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x631F80);
    RH_ScopedInstall(Destructor, 0x632050);

    //RH_ScopedGlobalInstall(ShouldLoadSunbatheAnims, 0x632190, { .Reversed = false });

    RH_ScopedInstall(CreateSubTask, 0x638290);

    RH_ScopedVMTInstall(Clone, 0x6366A0, { .Reversed = false });
    RH_ScopedVMTInstall(GetTaskType, 0x632040, { .Reversed = false });
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

/*!
* @brief Only load the sunbathe animations if the player isn't (quickly) driving around
* @addr 0x632190
*/
static bool ShouldLoadSunbatheAnims() {
    if (const auto player = FindPlayerPed()) {
        if (player->bInVehicle && player->m_pVehicle) {
            const auto speed2 = player->m_pVehicle->m_vecMoveSpeed.SquaredMagnitude();
            // NaN-safe `speed2 > 0.2f²` (0.04f)
            if (speed2 >= 0.04f && speed2 != 0.04f) {
                return false;
            }
        }
    }
    return true;
}

// 0x6320F0
bool CTaskComplexSunbathe::MakeAbortable(CPed* ped, eAbortPriority priority, CEvent const* event) {
    if (event) {
        if (event->GetEventType() == EVENT_PED_COLLISION_WITH_PED) {
            return false;
        }
        if (event->GetEventType() == EVENT_PED_COLLISION_WITH_PLAYER) {
            return false;
        }
    }

    if (!m_pSubTask->MakeAbortable(ped, priority, event)) {
        return false;
    }

    m_bAborted = true;
    return true;
}

// 0x638290
CTask* CTaskComplexSunbathe::CreateSubTask(eTaskType taskType, CPed* ped) {
    if (m_pSubTask && m_pSubTask->GetTaskType() == taskType) {
        return m_pSubTask;
    }

    switch (taskType) {
    case TASK_SIMPLE_IDLE_SUNBATHING: { // 0x1AD
        AnimationId animId;
        if (m_SunbatherType == SUNBATHER_MALE_1) {
            animId = static_cast<AnimationId>(ANIM_ID_PARKSIT_M_IDLEA + CGeneral::GetRandomNumberInRange(0, 3));
        } else if (m_SunbatherType == SUNBATHER_FEMALE_1) {
            animId = static_cast<AnimationId>(ANIM_ID_PARKSIT_W_IDLEA + CGeneral::GetRandomNumberInRange(0, 3));
        } else {
            animId = static_cast<AnimationId>(TASK_SIMPLE_IDLE_SUNBATHING);
        }
        return new CTaskSimpleRunAnim{ ANIM_GROUP_SUNBATHE, animId, 4.0f, TASK_SIMPLE_IDLE_SUNBATHING, "SunbatheIdle", true };
    }
    case TASK_SIMPLE_PAUSE: { // 0xCA
        if ((!m_pSunbatheAnimBlock->IsLoaded && ShouldLoadSunbatheAnims()) || !m_pBeachAnimBlock->IsLoaded) {
            return new CTaskSimplePause{ 10'000 };
        }
        return new CTaskSimplePause{ CGeneral::GetRandomNumberInRange(1'000, 5'000) };
    }
    case TASK_SIMPLE_SUNBATHE: { // 0x1A2
        const auto sunbatherType = static_cast<int32>(m_SunbatherType);
        int32 duration;
        if (!m_pSunbatheAnimBlock->IsLoaded || (sunbatherType != SUNBATHER_MALE_1 && sunbatherType != SUNBATHER_FEMALE_1)) {
            duration = static_cast<int32>(m_BathingTimer.m_bStarted
                ? (m_BathingTimer.m_nStartTime + m_BathingTimer.m_nInterval - CTimer::GetTimeInMS())
                : 0
            ) + 1'000;
        } else {
            const auto timeLeft = static_cast<int32>(m_BathingTimer.m_bStarted
                ? (m_BathingTimer.m_nStartTime + m_BathingTimer.m_nInterval - CTimer::GetTimeInMS())
                : 0
            );
            duration = CGeneral::GetRandomNumberInRange(3'000, timeLeft + 1'000 < 12'001 ? timeLeft + 1'000 : 12'000);
        }
        return new CTaskSimpleRunTimedAnim{
            ANIM_GROUP_BEACH,
            static_cast<AnimationId>(ANIM_ID_PARKSIT_M_LOOP + sunbatherType),
            4.0f,
            -4.0f,
            static_cast<uint32>(duration),
            TASK_SIMPLE_SUNBATHE,
            "Sunbathe",
            true
        };
    }
    case TASK_SIMPLE_START_SUNBATHING: { // 0x1AC
        const auto sunbatherType = static_cast<int32>(m_SunbatherType);
        return new CTaskSimpleRunAnim{
            ANIM_GROUP_SUNBATHE,
            static_cast<AnimationId>(ANIM_ID_PARKSIT_M_IN + sunbatherType),
            4.0f,
            TASK_SIMPLE_START_SUNBATHING,
            "SunbatheStart",
            true
        };
    }
    case TASK_SIMPLE_STOP_SUNBATHING: { // 0x1AE
        const auto sunbatherType = static_cast<int32>(m_SunbatherType);
        return new CTaskSimpleRunAnim{
            ANIM_GROUP_SUNBATHE,
            static_cast<AnimationId>(ANIM_ID_PARKSIT_M_OUT + sunbatherType),
            4.0f,
            TASK_SIMPLE_STOP_SUNBATHING,
            "SunbatheStop",
            false
        };
    }
    case TASK_FINISHED: // 0x516
        return nullptr;
    default:
        return nullptr;
    }
}

// 0x6399F0
CTask* CTaskComplexSunbathe::CreateNextSubTask(CPed* ped) {
    if (ped->m_nPedType == PED_TYPE_PROSTITUTE || ped->m_nPedType == PED_TYPE_CIVFEMALE) {
        g_InterestingEvents.Add(CInterestingEvents::EType::INTERESTING_EVENT_2, ped);
    }

    switch (m_pSubTask->GetTaskType()) {
    case TASK_SIMPLE_PAUSE: // 0xCA
        if (!m_bBathing) {
            if (m_pSunbatheAnimBlock->IsLoaded && m_pBeachAnimBlock->IsLoaded) {
                return CreateSubTask(TASK_SIMPLE_START_SUNBATHING, ped); // 0x1AC
            }
        } else {
            if (ShouldLoadSunbatheAnims() && m_pSunbatheAnimBlock->IsLoaded) {
                return CreateSubTask(TASK_SIMPLE_STOP_SUNBATHING, ped); // 0x1AE
            }
        }
        break;
    case TASK_SIMPLE_SUNBATHE: // 0x1A2
        if (!m_pSunbatheAnimBlock->IsLoaded) {
            return CreateSubTask(TASK_SIMPLE_PAUSE, ped); // 0xCA
        }
        if (IsSunbathingWeather()
            && !m_BathingTimer.IsOutOfTime()
            && (m_SunbatherType == SUNBATHER_MALE_1 || m_SunbatherType == SUNBATHER_FEMALE_1)
        ) {
            return CreateSubTask(TASK_SIMPLE_IDLE_SUNBATHING, ped); // 0x1AD
        }
        return CreateSubTask(TASK_SIMPLE_STOP_SUNBATHING, ped); // 0x1AE
    case TASK_SIMPLE_START_SUNBATHING: // 0x1AC
        if (m_pBeachAnimBlock->IsLoaded) {
            m_BathingTimer.m_nStartTime = CTimer::GetTimeInMS();
            m_BathingTimer.m_nInterval  = CGeneral::GetRandomNumberInRange(20'000, 100'000);
            m_BathingTimer.m_bStarted   = true;

            CTask* task = CreateSubTask(TASK_SIMPLE_SUNBATHE, ped); // 0x1A2
            m_bBathing = true;
            return task;
        }
        break;
    case TASK_SIMPLE_IDLE_SUNBATHING: // 0x1AD
        if (!m_pBeachAnimBlock->IsLoaded) {
            return CreateSubTask(TASK_FINISHED, ped); // 0x516
        }
        return CreateSubTask(TASK_SIMPLE_SUNBATHE, ped); // 0x1A2
    case TASK_SIMPLE_STOP_SUNBATHING: { // 0x1AE
        CTask* task = CreateSubTask(TASK_FINISHED, ped); // 0x516
        m_bBathing = false;
        return task;
    }
    default:
        return nullptr;
    }
    return CreateSubTask(TASK_FINISHED, ped); // 0x516
}

// 0x639CB0
CTask* CTaskComplexSunbathe::CreateFirstSubTask(CPed* ped) {
    rand(); // Discarded, but still advances the generator

    if (ped->m_nPedType == PED_TYPE_CIVFEMALE) {
        m_SunbatherType = static_cast<eSunbatherType>(2 - (int32)((float)(rand() & 0xFFFF) * 3.05176e-05f * -3.0f));
    } else if (CPopulation::IsSunbather(static_cast<eModelID>(ped->m_nModelIndex))) {
        m_SunbatherType = static_cast<eSunbatherType>(rand() < 0x3FFF);
    } else {
        m_SunbatherType = SUNBATHER_MALE_2;
    }

    if (!m_pBeachAnimBlock->IsLoaded) {
        CStreaming::RequestModel(m_BeachAnimBlockIndex + 0x63E7, 8);
    }
    if (!m_pSunbatheAnimBlock->IsLoaded) {
        CStreaming::RequestModel(m_SunbatheAnimBlockIndex + 0x63E7, 8);
    }

    m_bBathing = false;
    const bool shouldLoadSunbatheAnims = ShouldLoadSunbatheAnims();

    if (m_bStartStanding && shouldLoadSunbatheAnims) {
        if (!m_pTowel && m_pSunbatheAnimBlock->IsLoaded) {
            return CreateSubTask(TASK_SIMPLE_START_SUNBATHING, ped); // 0x1AC
        }
        return CreateSubTask(TASK_SIMPLE_PAUSE, ped); // 0xCA
    }
    if (!m_bStartStanding && shouldLoadSunbatheAnims) {
        if (!m_pBeachAnimBlock->IsLoaded) {
            return CreateSubTask(TASK_SIMPLE_PAUSE, ped); // 0xCA
        }
    } else if (!m_pBeachAnimBlock->IsLoaded) {
        return nullptr;
    }

    rand(); // Discarded, but still advances the generator
    m_BathingTimer.m_nStartTime = CTimer::GetTimeInMS();
    m_BathingTimer.m_nInterval  = 20'000 - (int32)((float)(rand() & 0xFFFF) * 3.05176e-05f * -80'000.0f);
    m_BathingTimer.m_bStarted   = true;
    m_bBathing                  = true;
    return CreateSubTask(TASK_SIMPLE_SUNBATHE, ped); // 0x1A2
}

// 0x6381A0
CTask* CTaskComplexSunbathe::ControlSubTask(CPed* ped) {
    if (m_bAborted) {
        return nullptr;
    }

    if (!m_bBeachAnimsReferenced) {
        if (!m_pBeachAnimBlock->IsLoaded) {
            CStreaming::RequestModel(m_BeachAnimBlockIndex + 0x63E7, 8);
        } else {
            CAnimManager::AddAnimBlockRef(m_BeachAnimBlockIndex);
            m_bBeachAnimsReferenced = true;
        }
    }

    if (!m_bSunbatheAnimsReferenced) {
        if (ShouldLoadSunbatheAnims()) {
            if (!m_pSunbatheAnimBlock->IsLoaded) {
                CStreaming::RequestModel(m_SunbatheAnimBlockIndex + 0x63E7, 8);
            } else {
                CAnimManager::AddAnimBlockRef(m_SunbatheAnimBlockIndex);
                m_bSunbatheAnimsReferenced = true;
            }
        }
    }

    if (m_bBathing) {
        ped->m_pedIK.m_nFlags |= 8;          // `bSlopePitch` - Reused as "lying down"
        ped->bFallenDown = true; // 0x474, bit 0x100000
    }

    if (!IsSunbathingWeather()) {
        if (m_BathingTimer.m_bStarted) {
            m_BathingTimer.m_bStopped = true;
            m_BathingTimer.m_nInterval -= CTimer::GetTimeInMS() - m_BathingTimer.m_nStartTime;
        }

        if (m_pSubTask->GetTaskType() == TASK_SIMPLE_SUNBATHE) { // 0x1A2
            m_pSubTask->MakeAbortable(ped, ABORT_PRIORITY_LEISURE, nullptr);
        }
    }

    return m_pSubTask;
}
