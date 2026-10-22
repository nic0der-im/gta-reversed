#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include <algorithm>
#include <cstring>
#include <new>

#include <spdlog/spdlog.h>

#include "app/platform/native/ExeData.manual.h"
#include "app/platform/native/ExeGlobals.h"

#include "Animation/AnimBlendAssocGroup.h"
#include "Animation/AnimManager.h"
#include "Audio/AESmoothFadeThread.h"
#include "Audio/Entities/AEWeatherAudioEntity.h"
#include "Audio/Entities/AETwinLoopSoundEntity.h"
#include "Audio/Hardware/AEAudioHardware.h"
#include "Audio/Managers/AESoundManager.h"
#include "AudioEngine.h"
#include "Buoyancy.h"
#include "Camera.h"
#include "Collision/ColModel.h"
#include "Collision/CollisionData.h"
#include "Collision/TempColModels.h"
#include "ControllerConfigManager.h"
#include "Core/MatrixLinkList.h"
#include "Core/Matrix.h"
#include "Core/OctTreeBase.h"
#include "CutsceneMgr.h"
#include "Directory.h"
#include "Entity/Vehicle/Vehicle.h"
#include "Escalator.h"
#include "Escalators.h"
#include "Explosion.h"
#include "FireManager.h"
#include "Fx/Fx.h"
#include "Fx/FxManager.h"
#include "Game.h"
#include "GameLogic.h"
#include "Gangs.h"
#include "Garages.h"
#include "HandShaker.h"
#include "HudColours.h"
#include "IdleCam.h"
#include "Interior/FurnitureManager_c.h"
#include "Interior/FurnitureSubGroup_c.h"
#include "Interior/Furniture_c.h"
#include "Interior/InteriorManager_c.h"
#include "InterestingEvents.h"
#include "LoadMonitor.h"
#include "Memory/MemoryHeap.h"
#include "MemoryMgr.h"
#include "MenuManager.h"
#include "Models/ModelInfo.h"
#include "Pad.h"
#include "PedClothesDesc.h"
#include "PedGroup.h"
#include "PedGroups.h"
#include "Plant/ProcObjectMan.h"
#include "PlantMgr.h"
#include "PlayerInfo.h"
#include "Ragdoll/BoneNodeManager.h"
#include "Ragdoll/IKChainManager.h"
#include "Replay.h"
#include "RGBA.h"
#include "Shopping.h"
#include "Tasks/Allocators/PedGroup/PedGroupDefaultTaskAllocators.h"
#include "Tasks/PedScriptedTaskRecord.h"
#include "Tasks/TaskSequences.h"
#include "Tasks/TaskTypes/TaskComplexFollowLeaderInFormation.h"
#include "Text/Text.h"
#include "VehicleAnimGroupData.h"
#include "WaterCannon.h"
#include "WaterCannons.h"
#include "WaterCreatureManager_c.h"
#include "Weather.h"
#include "Weapon.h"
#include "World.h"
#include "cHandlingDataMgr.h"

//! Defined in `Clothes.cpp`, bound to `StaticRef<CPedClothesDesc>(0xBC1C78)`.
extern CPedClothesDesc& PlayerClothes;

// The following globals are defined at file scope in their own `.cpp` without a header
// declaration, so the executable's constructor rows reach them through these declarations.
extern CColModel&              ms_ClimbColModel;    // TaskSimpleClimb.cpp, 0xC19518
extern CColModel&              ms_StandUpColModel;  // TaskSimpleClimb.cpp, 0xC19548
extern CColModel&              ms_VaultColModel;    // TaskSimpleClimb.cpp, 0xC19578
extern CColModel&              ms_FindEdgeColModel; // TaskSimpleClimb.cpp, 0xC195A8
extern CDirectory&             playerImg;           // ClothesBuilder.cpp, 0xBC12C0
extern CAETwinLoopSoundEntity& m_sRainSoundL;       // AEWeatherAudioEntity.cpp, 0xB6BB18
extern CAETwinLoopSoundEntity& m_sRainSoundR;       // AEWeatherAudioEntity.cpp, 0xB6BBC0

namespace notsa::native {
namespace {
//! `FurnitureSubGroup_c.cpp` declares `g_furniturePool` `static`, so the row gets its own
//! reference: same address and type, hence the same `ExeGlobals` storage.
static auto& g_furniturePool = StaticRef<std::array<Furniture_c, 256>>(0xBAE1F8);

//! `CVehicle::DoBladeCollision` declares these `static` inside the function, so they are never
//! constructed unless it runs. The executable constructs them during static initialization, and
//! their constructors only clear them, so the rows construct them at startup too.
static auto& s_TestBladeCol     = StaticRef<CColModel>(0xC1CD38);
static auto& s_TestBladeColData = StaticRef<CCollisionData>(0xC1CD68);

//! `CAnimManager::ms_aAnimations`, `FurnitureManager_c::g_subGroupStore` and
//! `CEscalators::aEscalators` are private, so the rows cannot name them. These references have
//! the declared types, so they resolve to the same `ExeGlobals` storage.
static auto& ms_Animations   = StaticRef<std::array<CAnimBlendHierarchy, 2500>>(0xB4EA40);
static auto& g_subGroupStore = StaticRef<std::array<FurnitureSubGroup_c, 128>>(0xBAD3F8);
static auto& aEscalators     = StaticRef<std::array<CEscalator, NUM_ESCALATORS>>(0xC6E9A8);

/*!
* @brief Clear a static array whose executable element constructor only writes zeros.
*
* The native storage starts out zeroed, so this is a no-op in practice; it is spelled out so the
* table row has an owner and because the executable really does clear the region.
*/
void ClearArray(const ExeDataEntry& e) {
    ExeGlobals::ZeroFill(e.Addr, static_cast<size_t>(e.Size) * e.Count);
}

/*!
* @brief One row of the initializer table whose native element layout differs from the executable's.
*
* The executable writes element `i` at `Addr + i * ExeStride`; the native type stores element `i` at
* `Addr + i * sizeof(element)` in `ExeGlobals` storage, which is a different offset as soon as a
* pointer widens. `WriteElement` maps one executable element write onto the native members, and
* `ClearElements` maps a clear of the whole row onto the native type.
*/
struct NativeLayoutRow {
    std::uintptr_t Addr;      //!< Executable address of element 0.
    std::size_t    ExeStride; //!< Element size in the executable.
    std::size_t    ExeSize;   //!< Bytes the executable occupies (element count x `ExeStride`).
    //! Apply the executable's write of `size` bytes at `offset` inside element `index`.
    void (*WriteElement)(std::uint32_t index, std::size_t offset, const void* data, std::size_t size);
    //! Bring the whole row to the executable's initial state (rows the executable clears).
    void (*ClearElements)();
};

//! Read the little-endian integer payload of one table write.
std::int32_t ReadInt32(const void* data, std::size_t size) {
    std::int32_t value{};
    std::memcpy(&value, data, std::min(size, sizeof(value)));
    return value;
}

// 0x851680: `CPedScriptedTaskRecord::ms_scriptedTasks` (128 x 0x14), element constructor 0x608330
// stores `m_opcode` = -1 at offset 0x0 and `m_status` = FLUSHED at offset 0x10. Natively the three
// pointers widen the element to 0x28, so the executable's offsets do not match the member offsets.
void WriteScriptedTask(std::uint32_t index, std::size_t offset, const void* data, std::size_t size) {
    auto& record = CPedScriptedTaskRecord::ms_scriptedTasks[index];
    switch (offset) {
    case 0x00:
        record.m_opcode = ReadInt32(data, size);
        return;
    case 0x10:
        record.m_status = static_cast<eScriptedTaskStatus>(ReadInt32(data, size));
        return;
    default:
        spdlog::warn("ExeData: no `CPedScriptedTaskRecordData` member at executable offset 0x{:X}", offset);
        return;
    }
}

// 0x850CE0: `CGangs::Gang` (10 x 0x10 `CGangInfo`), element constructor 0x5DE520 stores
// `m_nPedModelOverride` = -1 at offset 0x0.
void WriteGang(std::uint32_t index, std::size_t offset, const void* data, std::size_t size) {
    if (offset != 0 || size != 1) {
        spdlog::warn("ExeData: unexpected write of {} byte(s) at `CGangInfo` offset 0x{:X}", size, offset);
        return;
    }
    CGangs::Gang[index].m_nPedModelOverride = static_cast<std::int8_t>(*static_cast<const std::uint8_t*>(data));
}

// 0x84E9E0: `CWorld::ms_aRepeatSectors` (256 x 0xC). Element constructor 0x563190 only constructs
// the three inline `CPtrListDoubleLink`s (0x0043E3F0), each of which clears itself, so zeroing the
// native element is the equivalent.
void ClearRepeatSectors() {
    for (auto& row : CWorld::ms_aRepeatSectors) {
        for (auto& sector : row) {
            std::memset(static_cast<void*>(&sector), 0, sizeof(sector));
        }
    }
}

// 0x84FF30: `PlayerClothes`, cleared as two zero fills (0xBC1C78: 40 bytes, 0xBC1CA0: 72 bytes).
// `CPedClothesDesc` has the same size in both layouts, but the rows address it in two pieces, so
// the clear goes through the typed object.
void ClearPlayerClothes() {
    std::memset(static_cast<void*>(&PlayerClothes), 0, sizeof(PlayerClothes));
}

constexpr NativeLayoutRow kNativeLayoutRows[] = {
    // 0x84E9E0 `_eh_vector_constructor_iterator(ctor 0x563190 dtor 0x5631B0 256 x 0xC)`
    {0xB992B8, 0x0C, 0x0C * 256, nullptr, &ClearRepeatSectors},

    // 0x84FF30 zero fill of `CPedClothesDesc` (0x78 bytes in both layouts).
    {0xBC1C78, sizeof(CPedClothesDesc), sizeof(CPedClothesDesc), nullptr, &ClearPlayerClothes},

    // 0x850CE0 `_eh_vector_constructor_iterator(ctor 0x5DE520 dtor 0x5DE540 10 x 0x10)`
    {0xC091F0, 0x10, 0x10 * 10, &WriteGang, nullptr},

    // 0x851680 `_eh_vector_constructor_iterator(ctor 0x608330 dtor 0x6082E0 128 x 0x14)`
    {0xC0B1E8, 0x14, 0x14 * 128, &WriteScriptedTask, nullptr},
};

//! Find the row whose executable range contains `[addr, addr + size)`.
const NativeLayoutRow* FindNativeLayoutRow(std::uintptr_t addr, std::size_t size) {
    if (size == 0) {
        return nullptr;
    }
    for (const auto& row : kNativeLayoutRows) {
        if (addr < row.Addr) {
            continue;
        }
        const auto offset = addr - row.Addr;
        if (offset < row.ExeSize && size <= row.ExeSize - offset) {
            return &row;
        }
    }
    return nullptr;
}

/*!
* @brief Reproduce one of the executable's constructor rows on the native storage.
*
* The executable's CRT static initializer table constructs a global (or an array of globals) in
* place. Native storage starts out zero-filled, which is not enough for objects with a vtable or
* a non-trivial member: `CModelInfo::Initialise` dereferenced a null vtable on the never
* constructed `CModelInfo::ms_atomicModelInfoStore.m_aObjects[0]`. Every class here has its
* constructor reversed in the repo, so the row is replayed by running that constructor.
*/
template <typename T>
void Construct(T& obj) {
    ::new (static_cast<void*>(&obj)) T{};
}

//! Replay a `_eh_vector_constructor_iterator` row: construct every element in place.
template <typename Container>
void ConstructEach(Container& objects) {
    for (auto& obj : objects) {
        Construct(obj);
    }
}

//! Maps the executable's global to the native construction that replays its constructor row.
struct CtorRow {
    std::uintptr_t Addr; //!< Address of the global in the executable.
    void (*Apply)();     //!< Constructs the native object(s) in place.
};

// clang-format off
//! `ObjectCtor`/`VectorCtor` rows whose constructor is reversed, in executable row order.
//! Rows not listed here have no reversed constructor yet (see `docs/NativePort.md`) or are
//! applied by a dedicated handler below.
constexpr CtorRow kCtorRows[] = {
    // `CTempColModels` (`Collision/TempColModels.h`): 13 standalone models plus the 20-element
    // cutscene array, all constructed with `CColModel::CColModel` (0x40FB60).
    {0x968A00, [] { Construct(CTempColModels::ms_colModelBBox); }},             // 0x849470
    {0x968A30, [] { ConstructEach(CTempColModels::ms_colModelCutObj); }},       // 0x849490 20 x 0x30
    {0x968DF0, [] { Construct(CTempColModels::ms_colModelPed1); }},             // 0x8494C0
    {0x968E20, [] { Construct(CTempColModels::ms_colModelPed2); }},             // 0x8494E0
    {0x968E50, [] { Construct(CTempColModels::ms_colModelDoor1); }},            // 0x849500
    {0x968E80, [] { Construct(CTempColModels::ms_colModelBumper1); }},          // 0x849520
    {0x968EB0, [] { Construct(CTempColModels::ms_colModelPanel1); }},           // 0x849540
    {0x968EE0, [] { Construct(CTempColModels::ms_colModelBonnet1); }},          // 0x849560
    {0x968F10, [] { Construct(CTempColModels::ms_colModelBoot1); }},            // 0x849580
    {0x968F40, [] { Construct(CTempColModels::ms_colModelWheel1); }},           // 0x8495A0
    {0x968F70, [] { Construct(CTempColModels::ms_colModelBodyPart1); }},        // 0x8495C0
    {0x968FA0, [] { Construct(CTempColModels::ms_colModelBodyPart2); }},        // 0x8495E0
    {0x968FD0, [] { Construct(CTempColModels::ms_colModelWeapon); }},           // 0x849600

    // `CGameLogic::SavedWeapons` (`CWeapon() = default`), 13 x 0x1C.
    {0x96A9B8, [] { ConstructEach(CGameLogic::SavedWeapons); }},                // 0x849CE0
    // `CGarages::aGarages` (`CGarage() = default`), 50 x 0xD8.
    {0x96C048, [] { ConstructEach(CGarages::aGarages); }},                      // 0x849E90

    // `CReplay::PlayerInfo`: a single `CPlayerInfo` (constructor 0x571920).
    {0xA430B0, [] { Construct(CReplay::PlayerInfo); }},                         // 0x84A540

    // `CShopping::gStoredClothesState` (`CPedClothesDesc`).
    {0xA9A810, [] { Construct(gStoredClothesState); }},                         // 0x84B2A0
    // `g_fx` (`Fx_c() = default`) and `g_fxMan` (`FxManager_c`).
    {0xA9AE00, [] { Construct(g_fx); }},                                        // 0x84B5D0
    {0xA9AE80, [] { Construct(g_fxMan); }},                                     // 0x84B650

    // `CModelInfo::ms_*Store`: the executable's store constructor constructs every element in
    // place (`_eh_vector_constructor_iterator`), then zeroes the count. The element constructors
    // are reversed and set the vtable, so each element must be constructed individually.
    {0xAAE950, [] { ConstructEach(CModelInfo::ms_atomicModelInfoStore.m_aObjects); }},        // 0x84BBF0
    {0xB1BF58, [] { ConstructEach(CModelInfo::ms_damageAtomicModelInfoStore.m_aObjects); }},  // 0x84BC10
    {0xB1C934, [] { ConstructEach(CModelInfo::ms_lodAtomicModelInfoStore.m_aObjects); }},     // 0x84BC30
    {0xB1C960, [] { ConstructEach(CModelInfo::ms_timeModelInfoStore.m_aObjects); }},          // 0x84BC50
    {0xB1E128, [] { ConstructEach(CModelInfo::ms_lodTimeModelInfoStore.m_aObjects); }},       // 0x84BC70
    {0xB1E158, [] { ConstructEach(CModelInfo::ms_weaponModelInfoStore.m_aObjects); }},        // 0x84BC90
    {0xB1E958, [] { ConstructEach(CModelInfo::ms_clumpModelInfoStore.m_aObjects); }},         // 0x84BCB0
    {0xB1F650, [] { ConstructEach(CModelInfo::ms_vehicleModelInfoStore.m_aObjects); }},       // 0x84BCD0
    {0xB478F8, [] { ConstructEach(CModelInfo::ms_pedModelInfoStore.m_aObjects); }},           // 0x84BCF0

    // `CAnimManager::ms_aAnimations`, 2500 x 0x18.
    {0xB4EA40, [] { ConstructEach(ms_Animations); }},                           // 0x84BFD0

    // Audio managers and entities. Only the objects are built here; audio output is out of the
    // native port's scope, but their constructors are reversed and part of the executable's
    // static initialization.
    {0xB5F8B8, [] { Construct(AEAudioHardware); }},                             // 0x84C270
    {0xB608D0, [] { Construct(AESmoothFadeThread); }},                          // 0x84C290
    {0xB62CB0, [] { Construct(AESoundManager); }},                              // 0x84CD20
    {0xB6BB18, [] { Construct(m_sRainSoundL); }},                               // 0x84D4D0
    {0xB6BBC0, [] { Construct(m_sRainSoundR); }},                               // 0x84D4F0
    {0xC81360, [] { Construct(CWeather::m_WeatherAudioEntity); }},              // 0x855510
    {0xC888D0, [] { Construct(CExplosion::m_ExplosionAudioEntity); }},          // 0x8559E0

    // `CHandShaker::gHandShaker`, 6 x 0x94 (element constructor 0x517740).
    {0xB6ECA0, [] { ConstructEach(gHandShaker); }},                             // 0x84D6D0

    {0xB6F028, [] { Construct(TheCamera); }},                                   // 0x84D700
    {0xB6FDA0, [] { Construct(gIdleCam); }},                                    // 0x84D720
    {0xB71F80, [] { Construct(gFireManager); }},                                // 0x84DE70
    {0xB72978, [] { Construct(g_LoadMonitor); }},                               // 0x84E010
    {0xB74240, [] { Construct(gDummyMatrix); }},                                // 0x84E2E0
    {0xB74288, [] { Construct(gMatrixList); }},                                 // 0x84E300
    // `CWorld::Players`, 2 x 0x190.
    {0xB7CD98, [] { ConstructEach(CWorld::Players); }},                         // 0x84E980
    {0xBA6748, [] { Construct(FrontEndMenuManager); }},                         // 0x84EE40
    {0xBAB22C, [] { Construct(HudColour); }},                                   // 0x84F1F0

    // Interiors. `g_furniturePool` is declared `static` in `FurnitureSubGroup_c.cpp`.
    {0xBAB380, [] { Construct(g_furnitureMan); }},                              // 0x84F300
    {0xBAD3F8, [] { ConstructEach(g_subGroupStore); }},                         // 0x84F320 128 x 0x1C
    {0xBAE1F8, [] { ConstructEach(g_furniturePool); }},                         // 0x84F350 256 x 0x14
    {0xBAF670, [] { Construct(g_interiorMan); }},                               // 0x84F740

    {0xBB7CB0, [] { Construct(g_procObjMan); }},                                // 0x84FDC0
    {0xBC1290, [] { Construct(gOctTreeBase); }},                                // 0x84FE20
    {0xBC12C0, [] { Construct(playerImg); }},                                   // 0x84FE40
    {0xBC4020, [] { Construct(CCutsceneMgr::ms_cutsceneAssociations); }},       // 0x850040
    {0xC03A44, [] { Construct(CPlantMgr::m_AmbientColor); }},                   // 0x850910

    // `CPedGroupDefaultTaskAllocators::ms_*Allocator`: vtable-only singletons. Their constructors
    // (`0x5F6450`, `0x5F6480`, `0x5F64B0`, `0x5F64E0`, `0x5F6510`, `0x5F6540`) only store the
    // vtable, so the native default constructor reproduces them. Never constructed, the first
    // `CPedGroupIntelligence::SetDefaultTaskAllocator` call (from `CPlayerPed::SetInitialState`)
    // jumped through a null vtable.
    {0xC09908, [] { Construct(CPedGroupDefaultTaskAllocators::ms_FollowAnyMeansAllocator); }}, // 0x851270
    {0xC0990C, [] { Construct(CPedGroupDefaultTaskAllocators::ms_FollowLimitedAllocator); }},  // 0x851290
    {0xC09910, [] { Construct(CPedGroupDefaultTaskAllocators::ms_StandStillAllocator); }},     // 0x8512B0
    {0xC09914, [] { Construct(CPedGroupDefaultTaskAllocators::ms_ChatAllocator); }},           // 0x8512D0
    {0xC09918, [] { Construct(CPedGroupDefaultTaskAllocators::ms_RandomAllocator); }},         // 0x8512F0
    {0xC0991C, [] { Construct(CPedGroupDefaultTaskAllocators::ms_SitInLeaderCarAllocator); }}, // 0x851310

    // `CPedGroups::ms_groups`, 8 x 0x2D4.
    {0xC09920, [] { ConstructEach(CPedGroups::ms_groups); }},                   // 0x851330
    {0xC0B058, [] { Construct(g_InterestingEvents); }},                         // 0x8514E0
    {0xC10820, [] { Construct(g_boneNodeMan); }},                               // 0x851A70
    {0xC15448, [] { Construct(g_ikChainMan); }},                                // 0x851B50

    // `CGame::col1[0]`. The executable only constructs the first element here; the object that
    // follows it (`0x00C17854`) is a standalone `CCollisionData` row, see below.
    {0xC17824, [] { Construct(col1[0]); }},                                     // 0x851FF0
    // `CTaskSequences::ms_taskSequence`, 64 x 0x40.
    {0xC178F0, [] { ConstructEach(CTaskSequences::ms_taskSequence); }},         // 0x8520E0

    {0xC19518, [] { Construct(ms_ClimbColModel); }},                            // 0x852A10
    {0xC19548, [] { Construct(ms_StandUpColModel); }},                          // 0x852A30
    {0xC19578, [] { Construct(ms_VaultColModel); }},                            // 0x852A50
    {0xC195A8, [] { Construct(ms_FindEdgeColModel); }},                         // 0x852A70
    {0xC1B340, [] { Construct(TheText); }},                                     // 0x853460
    {0xC1C890, [] { Construct(mod_Buoyancy); }},                                // 0x853880

    // `CVehicle` collision models.
    {0xC1CC78, [] { ConstructEach(CVehicle::m_aSpecialColModel); }},            // 0x853DC0 4 x 0x30
    {0xC1CD38, [] { Construct(s_TestBladeCol); }},                              // 0x853DF0
    {0xC1CD68, [] { Construct(s_TestBladeColData); }},                          // 0x853E10

    // `CVehicleAnimGroupData::m_vehicleAnimGroups`, 30 x 0x94.
    {0xC1CDC0, [] { ConstructEach(CVehicleAnimGroupData::m_vehicleAnimGroups); }}, // 0x853E20
    {0xC1DF30, [] { Construct(g_waterCreatureMan); }},                          // 0x853F10

    // `CEscalators::aEscalators`, 32 x 0x150.
    {0xC6E9A8, [] { ConstructEach(aEscalators); }},                             // 0x854D50
    // `CWaterCannons::aCannons`, 3 x 0x3CC.
    {0xC80740, [] { ConstructEach(CWaterCannons::aCannons); }},                 // 0x855420

    // The three heaps are separate rows in the executable, one per element.
    {0xC87B40, [] { Construct(g_Heaps[0]); }},                                  // 0x855670
    {0xC87B88, [] { Construct(g_Heaps[1]); }},                                  // 0x855680
    {0xC87BD0, [] { Construct(g_Heaps[2]); }},                                  // 0x855690

    {0xC8A7DC, [] { Construct(CWeapon::ms_PelletTestCol); }},                   // 0x855C40
};
// clang-format on

//! Find the constructor row for the executable global at `addr`.
const CtorRow* FindCtorRow(std::uintptr_t addr) {
    for (const auto& row : kCtorRows) {
        if (row.Addr == addr) {
            return &row;
        }
    }
    return nullptr;
}
} // namespace

bool ApplyManualExeDataPattern(std::uintptr_t addr, const void* data, std::size_t size) {
    const auto* const row = FindNativeLayoutRow(addr, size);
    if (!row || !row->WriteElement) {
        return false;
    }
    const auto offset = addr - row->Addr;
    row->WriteElement(
        static_cast<std::uint32_t>(offset / row->ExeStride),
        offset % row->ExeStride,
        data,
        size
    );
    return true;
}

bool ApplyManualExeDataZeroFill(std::uintptr_t addr, std::size_t size) {
    const auto* const row = FindNativeLayoutRow(addr, size);
    if (!row || !row->ClearElements) {
        return false;
    }
    row->ClearElements();
    return true;
}

bool ApplyManualExeDataEntry(const ExeDataEntry& e) {
    switch (e.Kind) {
    case ExeDataKind::ObjectCtor:
        // Rows whose class constructor is reversed: run it on the native storage.
        if (const auto* const row = FindCtorRow(e.Addr)) {
            row->Apply();
            return true;
        }

        // 0x852010: `mov ecx, 0x00C17854; call 0x0040F030`. The `CCollisionData` that follows
        // `CGame::col1` (0x00C17824, one 0x30-byte `CColModel`): the collision setup at 0x0061D5F0
        // stores its address in `col1.m_pColData` (0x00C17850) and points its `m_pSpheres` at the
        // inline `CColSphere` further along (0x00C17884). `CCollisionData::CCollisionData`
        // (0x40F030) only clears its 0x30 bytes, and the repo declares the whole region as
        // `std::array<CColModel, 2>`, whose second element overlays this object, so the row clears
        // that element instead of a second, overlapping `StaticRef<CCollisionData>`.
        if (e.Addr == 0xC17854) {
            std::memset(static_cast<void*>(&col1[1]), 0, sizeof(col1[1]));
            return true;
        }

        // 0x84D920: `mov ecx, 0x00B70198; jmp 0x00531EE0`. `CControllerConfigManager`'s
        // constructor is reversed (`ControllerConfigManager.cpp:80`), and its whole call tree is
        // in-repo code, so run it.
        if (e.Addr == 0xB70198) {
            ::new (static_cast<void*>(&ControlsManager)) CControllerConfigManager{};;
            return true;
        }

        // 0x84D5D0: `mov ecx, 0x00B6BC90; call 0x00507670`. `CAudioEngine`'s default constructor is
        // compiler-generated (`AudioEngine.h:42`) and all it does is construct its audio entity
        // members. Those are abstract-ish objects whose only initial state is their vtable, so the
        // zero-filled storage is not enough: leaving it unconstructed makes every
        // `CAEAudioEntity::UpdateParameters` call jump through a null vtable
        // (`CAESound::UpdateParameters` faulted at `ldr x8, [x8]` with `x8 == 0`).
        if (e.Addr == 0xB6BC90) {
            ::new (static_cast<void*>(&AudioEngine)) CAudioEngine{};
            return true;
        }

        // 0x852F10: `mov ecx, 0x00C196E8; call 0x00694850`.
        // `CTaskComplexFollowLeaderInFormation::ms_offsets`: the constructor (not reversed) stores
        // the eight standing formation offsets, the eight moving ones and the scale. Zeroed,
        // every `CPedGroupDefaultTaskAllocatorRandom` follower would target the leader's own
        // position, and `Scale` would stay 0.
        if (e.Addr == 0xC196E8) {
            const CVector2D STANDING[TOTAL_PED_GROUP_MEMBERS]{
                {-1.f,  1.f}, { 1.f,  1.f}, { 0.f,  1.f}, {-1.f,  0.f},
                { 1.f,  0.f}, {-1.f, -1.f}, { 0.f, -1.f}, { 1.f, -1.f},
            };
            const CVector2D MOVING[TOTAL_PED_GROUP_MEMBERS]{
                {-1.f,   0.f}, { 1.f,   0.f}, { 0.f, -1.25f}, {-1.25f, -1.25f},
                { 1.25f, -1.25f}, { 0.f, -2.5f}, {-1.f, -2.5f}, { 1.f, -2.5f},
            };
            auto& offsets = CTaskComplexFollowLeaderInFormation::ms_offsets;
            for (auto i = 0u; i < offsets.Offsets.size(); i++) {
                offsets.Offsets[i]       = STANDING[i];
                offsets.MovingOffsets[i] = MOVING[i];
            }
            offsets.Scale = 1.f;
            return true;
        }
        break;

    case ExeDataKind::VectorCtor:
        // Rows whose element constructor is reversed: construct every element in place.
        if (const auto* const row = FindCtorRow(e.Addr)) {
            row->Apply();
            return true;
        }

        // 0x84E1F0: `CPad::Pads` (0x00B73458, 2 x 0x134), element constructor 0x00541D80
        // = `CPad::CPad`, which is reversed (`Pad.cpp:125`).
        if (e.Addr == 0xB73458) {
            for (auto& pad : StaticRef<CPad[MAX_PADS]>(0xB73458)) {
                ::new (static_cast<void*>(&pad)) CPad{};
            }
            return true;
        }

        // 0x84E9B0: `CWorld::ms_aSectors` (8 x 0x3840). Element constructor 0x00564040 only
        // zeroes the first two dwords of the sector.
        if (e.Addr == 0xB7D0B8) {
            ClearArray(e);
            return true;
        }

        // 0x84E9E0: `CWorld::ms_aRepeatSectors` (256 x 0xC). Element constructor 0x00563190 only
        // constructs three inline `CPtrListSingleLink`s (0x0043E3F0), each of which clears itself.
        // The zero fill below is redirected to the native array by `ClearRepeatSectors`, so the
        // executable's 0xC stride never addresses the widened elements.
        if (e.Addr == 0xB992B8) {
            ClearArray(e);
            return true;
        }

        // 0x84EA10: `CWorld::ms_aLodPtrLists` (4 x 0x384). Element constructor 0x0043E3F0 only
        // clears the list head.
        if (e.Addr == 0xB99EB8) {
            ClearArray(e);
            return true;
        }
        break;

    case ExeDataKind::Unhandled:
        // 0x8543D0: after constructing 0xD2 objects at 0x00C2BA08, the initializer clears
        // `gHandlingDataMgr` (0x00C2B9C8, 0xC624 bytes). The clear is the net effect and is all
        // that is ported; the surrounding loop builds objects that the clear then wipes.
        if (e.Addr == 0x8543D0) {
            // `rep stosd` of 0x3189 dwords, i.e. exactly `sizeof(cHandlingDataMgr)` in the
            // 32-bit layout; the typed reference is used so a layout change shows up here.
            constexpr size_t EXE_BYTES = 0xC624;
            if (sizeof(cHandlingDataMgr) != EXE_BYTES) {
                spdlog::warn(
                    "ExeData: `cHandlingDataMgr` is {} bytes natively, {} in the executable",
                    sizeof(cHandlingDataMgr), EXE_BYTES
                );
            }
            std::memset(static_cast<void*>(&gHandlingDataMgr), 0, EXE_BYTES);
            return true;
        }
        break;

    default:
        break;
    }
    return false;
}
} // namespace notsa::native

#endif // NOTSA_NATIVE
