#include "StdInc.h"

#include "VehicleSaveStructure.h"

void CVehicleSaveStructure::InjectHooks() {
    RH_ScopedClass(CVehicleSaveStructure);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Construct, 0x5D2330);
    RH_ScopedInstall(Extract, 0x5D23B0);
}

//! @notsa
void CVehicleSaveMatrix32::Construct(const CVehicle* v) {
    // Only the leading RenderWare matrix is stored (And it has the same layout in every build).
    // The (runtime-only) attachment data that follows it in the in-memory matrix isn't saved.
    std::memcpy(this, &*v->m_matrix, VEHICLE_SAVE_MATRIX_PAYLOAD_SIZE);
    std::memset(m_pad0x40, 0, sizeof(m_pad0x40));
}

//! @notsa
void CVehicleSaveMatrix32::Extract(CVehicle* v) const {
    CMatrix matrix; // Attachment data stays cleared - It was never part of the save either
    std::memcpy(&matrix, this, VEHICLE_SAVE_MATRIX_PAYLOAD_SIZE);
    v->SetMatrix(matrix);
}

// 0x5D2330
void CVehicleSaveStructure::Construct(CVehicle* v) {
    m_matrix.Construct(v);
    field_48 = 0; // Unused - Kept deterministic instead of leaving stack garbage in the save
    m_nPrimaryColor = v->m_nPrimaryColor;
    m_nSecondaryColor = v->m_nSecondaryColor;
    m_nTertiaryColor = v->m_nTertiaryColor;
    m_nQuaternaryColor = v->m_nQuaternaryColor;
    m_pad0x4D = 0;
    m_nAlarmState = v->m_nAlarmState;
    m_nMaxPassengers = v->m_nMaxPassengers;
    std::memset(m_pad0x51, 0, sizeof(m_pad0x51));
    m_fSteerAngle = v->m_fSteerAngle;
    m_fGasPedal = v->m_GasPedal;
    m_fBrakePedal = v->m_BrakePedal;
    m_nVehicleUpperFlags = v->m_nVehicleUpperFlags;
    m_nVehicleLowerFlags = v->m_nVehicleLowerFlags;
}

// 0x5D23B0
void CVehicleSaveStructure::Extract(CVehicle* v) {
    m_matrix.Extract(v);

    v->m_nPrimaryColor = m_nPrimaryColor;
    v->m_nSecondaryColor = m_nSecondaryColor;
    v->m_nTertiaryColor = m_nTertiaryColor;
    v->m_nQuaternaryColor = m_nQuaternaryColor;
    v->m_nAlarmState = m_nAlarmState;
    v->m_nMaxPassengers = m_nMaxPassengers;
    v->m_fSteerAngle = m_fSteerAngle;
    v->m_GasPedal = m_fGasPedal;
    v->m_BrakePedal = m_fBrakePedal;
    v->m_nVehicleUpperFlags = m_nVehicleUpperFlags;
    v->m_nVehicleLowerFlags = m_nVehicleLowerFlags;
}
