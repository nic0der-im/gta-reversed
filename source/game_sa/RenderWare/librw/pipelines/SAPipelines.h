#pragma once

//! San Andreas specific render pipelines on librw's GL3 backend.
//!
//! The retail game does not use RenderWare's generic object pipeline for most of its
//! geometry: buildings, cars and peds are drawn by custom `RxPipeline`s (`CustomBuilding`,
//! `CustomBuildingDN`, `CustomCarEnvMap`, skin) that drive D3D9 directly. This module
//! reimplements those four pipelines on top of librw's GL3 backend.
//!
//! Each pipeline owns:
//!  - GLSL 330 shader programs (`rw::gl3::Shader`) built from librw's lighting/fog headers,
//!  - the vertex/index buffers of every geometry that uses it (`rw::gl3::InstanceDataHeader`),
//!  - the uniforms that drive it (day/night balance, env-map matrix, wet-road level, vehicle
//!    paint colours, specular).
//!
//! Integration (plan B3): call `RegisterSAPipelines()` once the librw GL context exists (right
//! after `RwEngineStart()`), store `GetPipeline(...)` into the game's `ObjPipeline` globals
//! (`CCustomBuildingPipeline::ObjPipeline`, ...), and forward `RpAtomicInstance` /
//! `RpAtomicRender` to `InstanceAtomic` / `RenderAtomic`. Geometry destruction must call
//! `ForgetGeometry` so the cached GL buffers are released.
//!
//! No GL state is touched outside `InstanceAtomic`/`RenderAtomic` (and registration).

#ifdef NOTSA_NATIVE

#include "RenderWare.h"

#include <cstdint>

namespace notsa::native::sapipelines {

enum class Kind : uint8_t {
    Building,         //!< `CCustomBuildingPipeline`
    BuildingDayNight, //!< `CCustomBuildingDNPipeline` (day/night vertex colours)
    Vehicle,          //!< `CCustomCarEnvMapPipeline` (env map + specular)
    Ped,              //!< skinned peds with specular
};

//! Creates the shaders, registers the uniforms and builds the four pipelines.
//! Requires a current librw GL3 context; safe to call more than once.
void RegisterSAPipelines();

//! Releases the shaders and every cached geometry buffer.
void ShutdownSAPipelines();

//! Opaque handle to assign to `RpAtomic::pipeline` (the game treats `RxPipeline` opaquely).
RxPipeline* GetPipeline(Kind kind);

//! Maps an atomic's pipeline handle back to our kind. False when it is not one of ours.
bool GetPipelineKind(const RxPipeline* pipeline, Kind& kind);

//! Builds/updates the GL buffers for `atomic`'s geometry.
void InstanceAtomic(RpAtomic* atomic, bool reinstance = false);

//! Draws `atomic` with its pipeline. Instances it first when needed.
void RenderAtomic(RpAtomic* atomic);

//! Drops the cached GL buffers of a geometry that is about to be destroyed.
void ForgetGeometry(RpGeometry* geometry);

//! Day/night balance used by the building pipelines. Reads/writes the game's own
//! `CCustomBuildingDNPipeline::m_fDNBalanceParam`, so no duplicated state is kept.
void  SetDNBalanceParam(float balance);
float GetDNBalanceParam();

//! Camera-space to texture-space matrix used by the vehicle reflection pass.
void SetEnvMapMatrix(const RwMatrix& matrix);
//! Column-major 4x4, GL layout.
void SetEnvMapMatrix(const float matrix[16]);

//! Reads/writes `CWeather::WetRoads`; scales the vehicle reflection/specular intensity.
void  SetWetRoadLevel(float level);
float GetWetRoadLevel();

//! Vehicle paint indices (as `CVehicleModelInfo::SetVehicleColour` stores them).
void SetVehicleColours(uint8_t prim, uint8_t sec, uint8_t tert, uint8_t quat);

} // namespace notsa::native::sapipelines

#endif // NOTSA_NATIVE
