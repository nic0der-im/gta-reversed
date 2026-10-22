#include "StdInc.h"

#include "EventVehicleDamage.h"

#include "PedType.h"

void CEventVehicleDamage::InjectHooks()
{
    RH_ScopedVirtualClass(CEventVehicleDamage, 0x85B588, 17);
    RH_ScopedCategory("Events");

    RH_ScopedInstall(Constructor, 0x4B18D0);
    RH_ScopedVMTInstall(AffectsPed, 0x4B1A00);
    RH_ScopedVMTInstall(IsCriminalEvent, 0x4B1A90);
    RH_ScopedVMTInstall(ReportCriminalEvent, 0x4B50B0);
    RH_ScopedVMTInstall(GetSourceEntity, 0x4B1A70);
}

// 0x4B18D0
CEventVehicleDamage::CEventVehicleDamage(CVehicle* vehicle, CEntity* attacker, eWeaponType weaponType)
{
    m_attacker = attacker;
    m_vehicle = vehicle;
    m_weaponType = weaponType;
    CEntity::SafeRegisterRef(m_vehicle);
    CEntity::SafeRegisterRef(m_attacker);
}

CEventVehicleDamage::~CEventVehicleDamage()
{
    CEntity::SafeCleanUpRef(m_vehicle);
    CEntity::SafeCleanUpRef(m_attacker);
}

CEventVehicleDamage* CEventVehicleDamage::Constructor(CVehicle* vehicle, CEntity* attacker, eWeaponType weaponType)
{
    this->CEventVehicleDamage::CEventVehicleDamage(vehicle, attacker, weaponType);
    return this;
}

// 0x82263A retail purecall (shared; not hooked). Subclasses replace this.
CEventEditableResponse* CEventVehicleDamage::CloneEditable() const noexcept
{
    return new CEventVehicleDamage(m_vehicle, m_attacker, m_weaponType);
}

// 0x4B1A00
bool CEventVehicleDamage::AffectsPed(CPed* ped)
{
    if (!ped->IsAlive() || !m_attacker) {
        return false;
    }
    // Passengers of a vehicle that already has a driver ignore the event.
    // The driver, and anyone whose vehicle pointer is the damaged car, still receive it.
    if (ped->IsInVehicle() && ped->m_pVehicle->IsPassenger(ped) && ped->m_pVehicle->m_pDriver) {
        return false;
    }
    return m_vehicle && ped->m_pVehicle == m_vehicle;
}

// 0x4B1A90
bool CEventVehicleDamage::IsCriminalEvent()
{
    if (m_attacker) {
        switch (m_attacker->GetType()) {
        case ENTITY_TYPE_PED:
            return m_attacker->AsPed()->IsPlayer();
        case ENTITY_TYPE_VEHICLE:
            return m_attacker->AsVehicle()->m_pDriver == FindPlayerPed();
        }
    }
    return false;
}

// 0x4B50B0
void CEventVehicleDamage::ReportCriminalEvent(CPed* ped)
{
    if (IsCriminalEvent() && m_attacker) {
        bool bPoliceDontReallyCare = CPedType::PoliceDontCareAboutCrimesAgainstPedType(ped->m_nPedType);
        if (ped->m_nPedType == PED_TYPE_COP) 
            FindPlayerWanted()->RegisterCrime(eCrimeType::CRIME_VEHICLE_DAMAGE, m_attacker->GetPosition(), (uint32)ped, bPoliceDontReallyCare);
    }
}

// 0x4B1A70
CEntity* CEventVehicleDamage::GetSourceEntity() const
{
    if (m_attacker && m_attacker->GetIsTypeVehicle()) {
        CVehicle* vehicle = m_attacker->AsVehicle();
        if (vehicle->m_pDriver)
            return vehicle->m_pDriver;
    }
    return m_attacker;
}
