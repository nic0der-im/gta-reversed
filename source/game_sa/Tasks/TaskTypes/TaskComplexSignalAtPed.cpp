#include "StdInc.h"
#include "TaskComplexSignalAtPed.h"
#include "TaskComplexGangLeader.h"
#include "TaskComplexTurnToFaceEntityOrCoord.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimpleDoHandSignal.h"
#include "TaskSimpleRunAnim.h"
#include "Ragdoll/IKChainManager.h"

namespace {
// 0x407180: min + trunc((rand() & 0xFFFF) * (max - min) / 32768), exclusive at the top
int32 RandomIntOriginal(int32 min, int32 max) {
    return min + static_cast<int32>((static_cast<int64>(CGeneral::GetRandomNumber() & 0xFFFF) * (max - min)) >> 15);
}
}

void CTaskComplexSignalAtPed::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexSignalAtPed, 0x86fa8c, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x660A30);
    RH_ScopedInstall(Destructor, 0x660AB0);

    RH_ScopedVMTInstall(Clone, 0x662140);
    RH_ScopedVMTInstall(GetTaskType, 0x660AA0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x660B30);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x660CC0);
    RH_ScopedVMTInstall(ControlSubTask, 0x660D80);
}

// 0x660A30
CTaskComplexSignalAtPed::CTaskComplexSignalAtPed(CPed* pedToSignalAt, int32 unused1, bool playAnimAtEnd) :
    m_pedToSignalAt{pedToSignalAt},
    m_playAnimAtEnd{playAnimAtEnd}
{
    CEntity::SafeRegisterRef(m_pedToSignalAt);
}

CTaskComplexSignalAtPed::CTaskComplexSignalAtPed(const CTaskComplexSignalAtPed& o) :
    CTaskComplexSignalAtPed{o.m_pedToSignalAt, o.m_initialPause, o.m_playAnimAtEnd}
{
}

// 0x660AB0
CTaskComplexSignalAtPed::~CTaskComplexSignalAtPed() {
    CEntity::SafeCleanUpRef(m_pedToSignalAt);
}

// 0x660B30
CTask* CTaskComplexSignalAtPed::CreateNextSubTask(CPed* ped) {
    switch (m_pSubTask->GetTaskType()) {
    case TASK_COMPLEX_TURN_TO_FACE_ENTITY: { // 0x398
        const auto pause = m_initialPause == -1
            ? RandomIntOriginal(0, 2000)
            : m_initialPause;
        return new CTaskSimpleStandStill{ pause, false, false, 8.f };
    }
    case TASK_SIMPLE_STAND_STILL: // 0xCB
        return new CTaskSimpleDoHandSignal{};
    case TASK_SIMPLE_DO_HAND_SIGNAL: // 0x4BE
        if (m_areAnimsReferenced && m_playAnimAtEnd) {
            return new CTaskSimpleRunAnim{
                ANIM_GROUP_GANGS,
                (AnimationId)(ANIM_ID_PRTIAL_GNGTLKA + RandomIntOriginal(0, 8)),
                4.f,
                false
            };
        }
        return nullptr;
    default:
        return nullptr;
    }
}

// 0x660CC0
CTask* CTaskComplexSignalAtPed::CreateFirstSubTask(CPed* ped) {
    if (!m_pedToSignalAt || !CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        return nullptr;
    }
    ped->StopPlayingHandSignal();
    g_ikChainMan.LookAt(
        "TaskSignalAtPed",
        ped,
        m_pedToSignalAt,
        5000,
        BONE_HEAD,
        nullptr,
        true,
        0.15f,
        500,
        3,
        false
    );
    return new CTaskComplexTurnToFaceEntityOrCoord{ m_pedToSignalAt, 0.5f, 0.2f };
}

// 0x660D80
CTask* CTaskComplexSignalAtPed::ControlSubTask(CPed* ped) {
    // Make sure gang anims are loaded (if they can/need to be)
    if (!m_areAnimsReferenced) {
        if (CTaskComplexGangLeader::ShouldLoadGangAnims()) {
            const auto blk = CAnimManager::GetAnimationBlockIndex("gangs");
            if (CAnimManager::GetAnimBlocks()[blk].IsLoaded) {
                CAnimManager::AddAnimBlockRef(blk);
                m_areAnimsReferenced = true;
            } else {
                CStreaming::RequestModel(IFPToModelId(blk), STREAMING_KEEP_IN_MEMORY);
            }
        }
    } else if (!CTaskComplexGangLeader::ShouldLoadGangAnims()) {
        CAnimManager::RemoveAnimBlockRef(CAnimManager::GetAnimationBlockIndex("gangs"));
        m_areAnimsReferenced = false;
    }

    if (m_pedToSignalAt) {
        const auto delta = m_pedToSignalAt->GetPosition() - ped->GetPosition();
        ped->m_fAimingRotation = CGeneral::LimitRadianAngle(CGeneral::GetRadianAngleBetweenPoints(delta.x, delta.y, 0.f, 0.f));
    }

    return m_pSubTask;
}
