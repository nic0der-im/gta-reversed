#pragma once

#include "TaskSimple.h"
#include "TaskTimer.h"

class CAnimBlendAssociation;
class CEvent;
class CPed;

struct InteriorInfo_t;

/*!
* @brief Makes the ped sit down on (and get up from) a chair inside an interior.
*
* Used by `CTaskInteriorUseInfo` for `eInteriorInfoType::UNK_1` infos.
*/
class NOTSA_EXPORT_VTABLE CTaskInteriorSitInChair : public CTaskSimple {
public:
    static constexpr auto Type = eTaskType::TASK_INTERIOR_SIT_ON_CHAIR;

    static void InjectHooks();

    // 0x675C30
    CTaskInteriorSitInChair(int32 duration, InteriorInfo_t* interiorInfo, bool bDoInstantly);
    CTaskInteriorSitInChair(const CTaskInteriorSitInChair&);
    // 0x675C90
    ~CTaskInteriorSitInChair();

    // 0x675DD0
    static void FinishAnimCB(CAnimBlendAssociation*, void*);

    // 0x675CF0
    CTask*    Clone() const override { return new CTaskInteriorSitInChair{ *this }; }
    // 0x675C80
    eTaskType GetTaskType() const override { return Type; }
    // 0x675D60
    bool      MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, CEvent const* event = nullptr) override;
    // 0x676D30
    bool      ProcessPed(CPed* ped) override;

private:
    void StartAnim(CPed* ped, AnimationId animId, float blendDelta);
    void UpdatePedPos(CPed* ped);

private: // Wrappers for hooks
    // 0x675C30
    CTaskInteriorSitInChair* Constructor(int32 duration, InteriorInfo_t* interiorInfo, uint8 bDoInstantly) {
        this->CTaskInteriorSitInChair::CTaskInteriorSitInChair(duration, interiorInfo, bDoInstantly);
        return this;
    }

    // 0x675C90
    CTaskInteriorSitInChair* Destructor() {
        this->CTaskInteriorSitInChair::~CTaskInteriorSitInChair();
        return this;
    }

private:
    int32                  m_Duration;                     // 0x08
    InteriorInfo_t*        m_InteriorInfo;                 // 0x0C
    bool                   m_bDoInstantly;                 // 0x10
    CAnimBlendAssociation* m_Anim;                         // 0x14
    AnimationId            m_PrevAnimId{ ANIM_ID_UNDEFINED }; // 0x18
    CTaskTimer             m_TaskTimer;                    // 0x1C
    bool                   m_bUpdatePedPos;                // 0x28
    bool                   m_bTaskFinished;                // 0x29
    bool                   m_bTaskAborting;                // 0x2A
};
VALIDATE_SIZE(CTaskInteriorSitInChair, 0x2C);
