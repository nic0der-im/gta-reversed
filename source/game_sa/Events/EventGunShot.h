#pragma once

#include "EventEditableResponse.h"
#include "Ped.h"

class NOTSA_EXPORT_VTABLE CEventGunShot : public CEventEditableResponse {
public:
    CEntity* m_firedBy;
    CVector  m_startPoint;
    CVector  m_endPoint;
    bool     m_bHasNoSound;

#ifdef NOTSA_NATIVE
    static inline float ms_fGunShotSenseRangeForRiot2 = -1.0f; // File-backed default in the executable's `.data`
#else
    static inline auto& ms_fGunShotSenseRangeForRiot2 = StaticRef<float>(0x8A625C);
#endif
    static void InjectHooks();

    CEventGunShot(CEntity* entity, CVector startPoint, CVector endPoint, bool bHasNoSound);
    ~CEventGunShot() override;

    eEventType GetEventType() const override { return EVENT_SHOT_FIRED; }
    int32 GetEventPriority() const override { return 35; }
    int32 GetLifeTime() override { return 0; }
    bool AffectsPed(CPed* ped) override;
    bool IsCriminalEvent() override;
    void ReportCriminalEvent(CPed* ped) override { } // empty
    CEntity* GetSourceEntity() const override { return m_firedBy; }
    bool TakesPriorityOver(const CEvent& refEvent) override;
    float GetLocalSoundLevel() override { return m_bHasNoSound ? 0.0f : 160.0f; }
    bool CanBeInterruptedBySameEvent() override { return true; }
    CEventEditableResponse* CloneEditable() const noexcept override;

    CEventGunShot* Constructor(CEntity* entity, CVector startPoint, CVector endPoint, bool bHasNoSound);
};

VALIDATE_SIZE(CEventGunShot, 0x34);
