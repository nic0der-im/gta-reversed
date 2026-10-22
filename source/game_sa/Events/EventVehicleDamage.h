#pragma once

#include "EventEditableResponse.h"

class CEntity;
class CVehicle;
enum eWeaponType : uint32;

class NOTSA_EXPORT_VTABLE CEventVehicleDamage : public CEventEditableResponse {
public:
    CVehicle*   m_vehicle;
    CEntity*    m_attacker;
    eWeaponType m_weaponType;

public:
    static void InjectHooks();

    CEventVehicleDamage(CVehicle* vehicle, CEntity* attacker, eWeaponType weaponType);
    ~CEventVehicleDamage() override;

    // Retail slot is the shared purecall at 0x82263A (no per-class body to hook).
    // 41 matches the weapon subclass, which is the vtable installed for a direct
    // `CEventVehicleDamage` used as a weapon-damage event. Collision overrides this.
    eEventType GetEventType() const override { return EVENT_VEHICLE_DAMAGE_WEAPON; }
    int32 GetEventPriority() const override { return 49; }
    int32 GetLifeTime() override { return 0; }
    bool AffectsPed(CPed* ped) override;
    bool IsCriminalEvent() override;
    void ReportCriminalEvent(CPed* ped) override;
    CEntity* GetSourceEntity() const override;
    float GetLocalSoundLevel() override { return 55.0f; }
    CEventEditableResponse* CloneEditable() const noexcept override;

private:
    CEventVehicleDamage* Constructor(CVehicle* vehicle, CEntity* attacker, eWeaponType weaponType);
};

VALIDATE_SIZE(CEventVehicleDamage, 0x20);
