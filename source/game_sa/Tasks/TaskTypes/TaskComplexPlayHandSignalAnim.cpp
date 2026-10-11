#include "StdInc.h"
#include "TaskComplexPlayHandSignalAnim.h"
#include "TaskSimpleStandStill.h"
#include "TaskSimplePlayHandSignalAnim.h"
#include "TaskComplexSequence.h"
#include "Ragdoll/IKChainManager.h"

void CTaskComplexPlayHandSignalAnim::InjectHooks() {
    RH_ScopedVirtualClass(CTaskComplexPlayHandSignalAnim, 0x86d5dc, 11);
    RH_ScopedCategory("Tasks/TaskTypes");

    RH_ScopedInstall(Constructor, 0x61B2B0);
    RH_ScopedInstall(Destructor, 0x61BDF0);

    RH_ScopedInstall(GetAnimIdForPed, 0x61B460);
    RH_ScopedInstall(CreateSubTask, 0x61B2F0);

    RH_ScopedVMTInstall(Clone, 0x61BA00);
    RH_ScopedVMTInstall(GetTaskType, 0x61B2E0);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x61B570);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x61B4F0);
    RH_ScopedVMTInstall(ControlSubTask, 0x61B580);
}

// 0x61B2B0
CTaskComplexPlayHandSignalAnim::CTaskComplexPlayHandSignalAnim(AnimationId animationId, float blendFactor) :
    m_animationId{animationId},
    m_AnimBlenDelta{blendFactor}
{
}

// 0x61BDF0
CTaskComplexPlayHandSignalAnim::~CTaskComplexPlayHandSignalAnim() {
    enum {
        RIGHT,
        LEFT,
    };
    enum {
        NONFAT,
        FAT
    };
    const eModelID handModels[2][2]{
        // nonfat           fat
        { MODEL_SHANDL, MODEL_FHANDL }, // left
        { MODEL_SHANDR, MODEL_FHANDR }  // right
    };

    // Remove hand model refs
    for (const auto i : { LEFT, RIGHT }) {
        CModelInfo::GetModelInfo(handModels[i][m_DoUseFatHands ? FAT : NONFAT])->RemoveRef();
    }

    // Deal with anim
    if (m_bAnimationLoaded) { // Remove anim ref
        CAnimManager::RemoveAnimBlockRef(ms_animBlock);
    } else if (ms_animBlock != -1 && !CAnimManager::GetAnimBlocks()[ms_animBlock].RefCnt) { 
        if (!rng::all_of(std::array{ LEFT, RIGHT }, [&, this](auto i) { // Unload anim block if not all of the models has refs
            return CModelInfo::GetModelInfo(handModels[i][m_DoUseFatHands ? FAT : NONFAT])->m_nRefCount != 0;
        })) {
            CStreaming::RemoveModel(IFPToModelId(ms_animBlock));        
        }
    }
}

// 0x61B460
AnimationId CTaskComplexPlayHandSignalAnim::GetAnimIdForPed(CPed* ped) {
    switch (ped->m_nPedType) {
    case PED_TYPE_GANG1:  return ANIM_ID_GSIGN1;
    case PED_TYPE_GANG2:  return ANIM_ID_GSIGN2;
    case PED_TYPE_GANG3:  return ANIM_ID_GSIGN3;
    case PED_TYPE_GANG5:  return ANIM_ID_GSIGN5;
    case PED_TYPE_GANG8:  return ANIM_ID_GSIGN4;
    default:              return ANIM_ID_UNDEFINED;
    }
}

// 0x61B2F0
CTask* CTaskComplexPlayHandSignalAnim::CreateSubTask(eTaskType taskType) {
    switch (taskType) {
    case TASK_SIMPLE_STAND_STILL:
        return new CTaskSimpleStandStill{ 0, true, false, 8.f };
    case TASK_COMPLEX_SEQUENCE: {
        auto* const seq = new CTaskComplexSequence{};
        seq->AddTask(new CTaskSimpleStandStill{ CGeneral::GetRandomNumberInRange<int32>(0, 1500), false, false, 8.f });
        seq->AddTask(new CTaskSimplePlayHandSignalAnim{ m_animationId, m_AnimBlenDelta, m_DoUseFatHands, false });
        return seq;
    }
    default:
        return nullptr;
    }
}

// 0x61BA00
CTask* CTaskComplexPlayHandSignalAnim::Clone() const {
    return new CTaskComplexPlayHandSignalAnim{ m_animationId, m_AnimBlenDelta };
}

// 0x61B570
CTask* CTaskComplexPlayHandSignalAnim::CreateNextSubTask(CPed* ped) {
    return nullptr;
}

// 0x61B4F0
CTask* CTaskComplexPlayHandSignalAnim::CreateFirstSubTask(CPed* ped) {
    if (g_ikChainMan.IsArmPointing(IK_ARM_LEFT, ped)) {
        return nullptr;
    }
    if (m_animationId == ANIM_ID_UNDEFINED) {
        m_animationId = GetAnimIdForPed(ped);
        if (m_animationId == ANIM_ID_UNDEFINED) {
            return nullptr;
        }
    }
    switch (ped->GetModelIndex()) {
    case MODEL_BALLAS2:
    case MODEL_FAM1:
    case MODEL_FAM3:
        m_DoUseFatHands = true;
        break;
    default:
        m_DoUseFatHands = false;
        break;
    }
    return CreateSubTask(TASK_SIMPLE_STAND_STILL);
}

// 0x61B580
CTask* CTaskComplexPlayHandSignalAnim::ControlSubTask(CPed* ped) {
    const auto handL = m_DoUseFatHands ? MODEL_FHANDL : MODEL_SHANDL;
    const auto handR = m_DoUseFatHands ? MODEL_FHANDR : MODEL_SHANDR;

    // NOTE: the original uses the same left/right flags in this order: L model -> bit0, R model -> bit1
    if (CStreaming::IsModelLoaded(handL)) {
        if (!m_bLeftHandLoaded) {
            CModelInfo::GetModelInfo(handL)->AddRef();
            m_bLeftHandLoaded = true;
        }
    } else {
        CStreaming::RequestModel(handL, STREAMING_KEEP_IN_MEMORY);
    }
    if (CStreaming::IsModelLoaded(handR)) {
        if (!m_bRightHandLoaded) {
            CModelInfo::GetModelInfo(handR)->AddRef();
            m_bRightHandLoaded = true;
        }
    } else {
        CStreaming::RequestModel(handR, STREAMING_KEEP_IN_MEMORY);
    }

    if (m_bLeftHandLoaded && m_bRightHandLoaded) {
        if (ms_animBlock == (uint32)-1) {
            ms_animBlock = CAnimManager::GetAnimationBlockIndex("ghands");
        }
        if (CStreaming::IsModelLoaded(IFPToModelId(ms_animBlock))) {
            if (!m_bAnimationLoaded) {
                CAnimManager::AddAnimBlockRef(ms_animBlock);
                m_bAnimationLoaded = true;
            }
        } else {
            CStreaming::RequestModel(IFPToModelId(ms_animBlock), STREAMING_KEEP_IN_MEMORY);
        }
    }

    if (m_bAnimationLoaded && m_pSubTask->GetTaskType() == TASK_SIMPLE_STAND_STILL) {
        return CreateSubTask(TASK_COMPLEX_SEQUENCE);
    }
    return m_pSubTask;
}
