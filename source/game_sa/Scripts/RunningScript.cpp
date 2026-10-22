#include "StdInc.h"

#include "RunningScript.h"
#include "TheScripts.h"
#include "CarGenerator.h"
#include "Hud.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#ifdef NOTSA_NATIVE
#include "app/platform/native/Unreversed.h"
#endif

static notsa::log_ptr logger;

//static auto logger = NOTSA_MAKE_LOGGER("script");

//! Define it to dump out all commands that don't have a custom handler (that is, they aren't reversed)
//! Makes compilation slow, so don't enable unless necessary!
//#define DUMP_CUSTOM_COMMAND_HANDLERS_TO_FILE

#ifdef DUMP_CUSTOM_COMMAND_HANDLERS_TO_FILE
#include <fstream>
#endif

#include "CommandParser/Parser.hpp"
#include "CommandParser/LUTGenerator.hpp"
#include "reversiblehooks/ReversibleHook/ScriptCommandHook.h"

#include "Commands/Commands.hpp"
#include "Tasks/TaskTypes/TaskSimpleHoldEntity.h"
#include "Tasks/TaskTypes/TaskSimpleRunNamedAnim.h"
#include "Tasks/TaskTypes/TaskSimpleAffectSecondaryBehaviour.h"
#include "Tasks/TaskTypes/TaskSimpleFinishBrain.h"
#ifdef NOTSA_WITH_CLEO_SCRIPT_COMMANDS // TODO: Add premake/cmake option for this define
#include "Commands/CLEO/Commands.hpp"
#include "Commands/CLEO/Extensions/Commands.hpp"
#endif

// https://library.sannybuilder.com/#/sa

//! Holds all custom command handlers (or null for commands with no custom handler)
static inline std::array<notsa::script::CommandHandlerFunction, (size_t)(COMMAND_HIGHEST_ID_TO_HOOK) + 1> s_CustomCommandHandlerTable{};

std::array<std::array<char, COMMANDS_CHAR_BUFFER_SIZE>, COMMANDS_CHAR_BUFFERS_COUNT> CRunningScript::ScriptArgCharBuffers        = {};
uint8                                                                                CRunningScript::ScriptArgCharNextFreeBuffer = 0;

//! Position the given ped is located at (A ped inside a vehicle is located by it's vehicle's position)
static CVector GetScriptPedPosition(CPed* ped) {
    return ped->IsInVehicle()
        ? ped->m_pVehicle->GetPosition()
        : ped->GetPosition();
}

void CRunningScript::InjectHooks() {
    logger = NOTSA_MAKE_LOGGER("script");

    InjectCustomCommandHooks();

    RH_ScopedClass(CRunningScript);
    RH_ScopedCategory("Scripts");

    RH_ScopedInstall(Init, 0x4648E0);
    RH_ScopedInstall(GetCorrectPedModelIndexForEmergencyServiceType, 0x464F50);

    RH_ScopedInstall(PlayAnimScriptCommand, 0x470150);
    RH_ScopedInstall(LocateCarCommand, 0x487A20);
    RH_ScopedInstall(LocateCharCommand, 0x486D80);
    RH_ScopedInstall(LocateObjectCommand, 0x487D10);
    RH_ScopedInstall(LocateCharCarCommand, 0x487420);
    RH_ScopedInstall(LocateCharCharCommand, 0x4870F0);
    RH_ScopedInstall(LocateCharObjectCommand, 0x487720);
    RH_ScopedInstall(CarInAreaCheckCommand, 0x488EC0);
    RH_ScopedInstall(CharInAreaCheckCommand, 0x488B50);
    RH_ScopedInstall(ObjectInAreaCheckCommand, 0x489150);
    RH_ScopedInstall(CharInAngledAreaCheckCommand, 0x487F60);
    RH_ScopedInstall(FlameInAngledAreaCheckCommand, 0x488780);
    RH_ScopedInstall(ObjectInAngledAreaCheckCommand, 0x4883F0);
    RH_ScopedInstall(CollectParameters, 0x464080, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(CollectNextParameterWithoutIncreasingPC, 0x464250, { .StackArgumentsToPreserve = 0, .PreserveRegisters = true });
    RH_ScopedInstall(StoreParameters, 0x464370, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(ReadArrayInformation, 0x463CF0, { .StackArgumentsToPreserve = 3, .PreserveRegisters = true });
    RH_ScopedInstall(ReadParametersForNewlyStartedScript, 0x464500, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(ReadTextLabelFromScript, 0x463D50, { .StackArgumentsToPreserve = 2, .PreserveRegisters = true });
    RH_ScopedInstall(GetIndexOfGlobalVariable, 0x464700, { .StackArgumentsToPreserve = 0, .PreserveRegisters = true });
    RH_ScopedInstall(GetPadState, 0x485B10);
    RH_ScopedInstall(GetPointerToLocalVariable, 0x463CA0, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(GetPointerToLocalArrayElement, 0x463CC0, { .StackArgumentsToPreserve = 3, .PreserveRegisters = true });
    RH_ScopedInstall(GetPointerToScriptVariable, 0x464790, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(DoDeathArrestCheck, 0x485A50);
    RH_ScopedInstall(SetCharCoordinates, 0x464DC0);
    RH_ScopedInstall(AddScriptToList, 0x464C00, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(RemoveScriptFromList, 0x464BD0, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(ShutdownThisScript, 0x465AA0);
    RH_ScopedInstall(IsPedDead, 0x464D70);
    RH_ScopedInstall(ThisIsAValidRandomPed, 0x489490);
    RH_ScopedInstall(ScriptTaskPickUpObject, 0x46AF50);
    RH_ScopedInstall(UpdateCompareFlag, 0x4859D0, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(UpdatePC, 0x464DA0, { .StackArgumentsToPreserve = 1, .PreserveRegisters = true });
    RH_ScopedInstall(ProcessOneCommand, 0x469EB0);
    RH_ScopedInstall(Process, 0x469F00);
    RH_ScopedOverloadedInstall(GivePedScriptedTask, "OG", 0x465C20, void(CRunningScript::*)(int32, CTask*, int32));
}

//! Register our custom script command handlers
void CRunningScript::InjectCustomCommandHooks() {
    // Uncommenting any call will prevent it from being hooked, so
    // feel free to do so when debugging (Just don't forget to undo the changes!)

    namespace c = ::notsa::script::commands;
    c::basic::RegisterHandlers();
    c::camera::RegisterHandlers();
    c::character::RegisterHandlers();
    c::clock::RegisterHandlers();
    c::comparasion::RegisterHandlers();
    c::game::RegisterHandlers();
    c::generic::RegisterHandlers();
    c::math::RegisterHandlers();
    c::mission::RegisterHandlers();
    c::object::RegisterHandlers();
    c::pad::RegisterHandlers();
    c::ped::RegisterHandlers();
    c::player::RegisterHandlers();
    c::script::RegisterHandlers();
    c::sequence::RegisterHandlers();
    c::text::RegisterHandlers();
    c::unused::RegisterHandlers();
    c::utility::RegisterHandlers();
    c::vehicle::RegisterHandlers();
    c::zone::RegisterHandlers();
    c::stat::RegisterHandlers();
    c::conversation::RegisterHandlers();

#ifdef NOTSA_WITH_CLEO_SCRIPT_COMMANDS
    cleo::audiostream::RegisterHandlers();
    cleo::character::RegisterHandlers();
    cleo::dynamiclibrary::RegisterHandlers();
    cleo::fs::RegisterHandlers();
    cleo::game::RegisterHandlers();
    cleo::generic::RegisterHandlers();
    cleo::memory::RegisterHandlers();
    cleo::pad::RegisterHandlers();
    cleo::script::RegisterHandlers();
    cleo::vehicle::RegisterHandlers();
    cleo::world::RegisterHandlers();

    cleo::extensions::cleoplus::RegisterHandlers();
    cleo::extensions::clipboard::RegisterHandlers();
    cleo::extensions::fs::RegisterHandlers();
    cleo::extensions::imgui::RegisterHandlers();
    cleo::extensions::intoperations::RegisterHandlers();
#endif

#ifdef DUMP_CUSTOM_COMMAND_HANDLERS_TO_FILE
    auto reversed{0}, total{0};
    std::ofstream ofsrev{ "reversed_script_command_handlers.txt" }, ofsnotrev{ "NOT_reversed_script_command_handlers.txt" };
    for (auto&& [idx, handler] : rngv::enumerate(s_CustomCommandHandlerTable)) {
        const auto id = (eScriptCommands)(idx);
        ++total;
        if (handler) ++reversed;
        (handler ? ofsrev : ofsnotrev) << ::notsa::script::GetScriptCommandName(id) << '\n';
    }
    NOTSA_LOG_DEBUG("Script cmds dumped! Find them in `<GTA Directory>/Scripts`!");
    NOTSA_LOG_DEBUG("Script cmds reverse progress: {}/{} ({:.2f}% done)", reversed, total, 100.0f * ((float)reversed / (float)total));
#endif
}

// 0x4648E0
void CRunningScript::Init() {
    SetName("noname");
    rng::fill(m_IPStack, nullptr);
    rng::fill(m_LocalVars, tScriptParam{ 0 });
    m_BaseIP                          = nullptr;
    m_pPrev                           = nullptr;
    m_pNext                           = nullptr;
    m_IP                              = nullptr;
    m_StackDepth                      = 0;
    m_WakeTime                        = 0;
    m_IsActive                        = false;
    m_CondResult                      = false;
    m_UsesMissionCleanup              = false;
    m_IsExternal                      = false;
    m_IsTextBlockOverride             = false;
    m_ExternalType                    = -1;
    m_AndOrState                      = 0;
    m_NotFlag                         = false;
    m_DoneDeathArrest                 = false;
    m_SceneSkipIP                     = 0;
    m_ThisMustBeTheOnlyMissionRunning = false;
    m_IsDeathArrestCheckEnabled       = true;
}

/*!
 * Adds script to list
 * @addr 0x464C00
 */
void CRunningScript::AddScriptToList(CRunningScript** queueList) {
    m_pNext = *queueList;
    m_pPrev = nullptr;
    if (*queueList)
        (*queueList)->m_pPrev = this;
    *queueList = this;
}

/*!
 * Removes script from list
 * @addr 0x464BD0
 */
void CRunningScript::RemoveScriptFromList(CRunningScript** queueList) {
    if (m_pPrev)
        m_pPrev->m_pNext = m_pNext;
    else
        *queueList = m_pNext;

    if (m_pNext)
        m_pNext->m_pPrev = m_pPrev;
}

/*!
 * Terminates a script
 * @addr 0x465AA0
 */
void CRunningScript::ShutdownThisScript() {
    m_IsActive = false;

    if (m_IsExternal) {
        const auto idx = CTheScripts::StreamedScripts.GetStreamedScriptWithThisStartAddress(m_BaseIP);
        CTheScripts::StreamedScripts.m_aScripts[idx].m_NumberOfUsers--;
    }

    // The entity this script is attached to is stored in the first local variable
    // (Or in the mission's `LocalVariablesForCurrentMission`, if this is the mission script)
    const int32 entityRef = (m_ThisMustBeTheOnlyMissionRunning
        ? CTheScripts::LocalVariablesForCurrentMission[0]
        : m_LocalVars[0]
    ).iParam;

    switch (m_ExternalType) {
    case 0:
    case 2:
    case 3:
    case 5: {
        CPed* ped = GetPedPool()->GetAtRef(entityRef);
        if (!ped) {
            break;
        }
        ped->bHasAScriptBrain = false;
        if (m_ExternalType == 5) {
            CScriptedBrainTaskStore::SetTask(ped, new CTaskSimpleFinishBrain{});
        }
        break;
    }
    case 1:
    case 4: {
        if (CObject* object = GetObjectPool()->GetAtRef(entityRef)) {
            object->objectFlags.b0x100000_0x200000 = 1; // b0x100000 (Waiting)
        }
        break;
    }
    }
}

// 0x465C20
void CRunningScript::GivePedScriptedTask(int32 pedHandle, CTask* task, int32 opcode) {
    if (pedHandle == -1) {
        CTaskSequences::AddTaskToActiveSequence(task);
        return;
    }

    CPed* ped = GetPedPool()->GetAtRef(pedHandle);
    assert(ped);
    CPedGroup* pedGroup = CPedGroups::GetPedsGroup(ped);

    CPed* otherPed = nullptr;
    if (m_ExternalType == 5 || m_ExternalType == 2 || !m_ExternalType || m_ExternalType == 3) {
        auto* localVariable = reinterpret_cast<int32*>(GetPointerToLocalVariable(0));
        otherPed = GetPedPool()->GetAtRef(*localVariable);
    }

    if (ped->bHasAScriptBrain && otherPed != ped) {
        delete task;
    } else if (otherPed && m_ExternalType == 5) {
        if (CScriptedBrainTaskStore::SetTask(ped, task)) {
            const int32 slot = CPedScriptedTaskRecord::GetVacantSlot();
            CPedScriptedTaskRecord::ms_scriptedTasks[slot].SetAsAttractorScriptTask(ped, opcode, task);
        }
    } else if (!pedGroup || ped->IsPlayer()) {
        CEventScriptCommand eventScriptCommand(TASK_PRIMARY_PRIMARY, task, false);
        auto* event = static_cast<CEventScriptCommand*>(ped->GetEventGroup().Add(&eventScriptCommand, false));
        if (event) {
            const int32 slot = CPedScriptedTaskRecord::GetVacantSlot();
            CPedScriptedTaskRecord::ms_scriptedTasks[slot].Set(ped, opcode, event);
        }
    } else {
        pedGroup->GetIntelligence().SetScriptCommandTask(ped, *task);
        CTask* scriptedTask = pedGroup->GetIntelligence().GetTaskScriptCommand(ped);
        const int32 slot = CPedScriptedTaskRecord::GetVacantSlot();
        CPedScriptedTaskRecord::ms_scriptedTasks[slot].SetAsGroupTask(ped, opcode, scriptedTask);
        delete task;
    }
}

void CRunningScript::GivePedScriptedTask(CPed* ped, CTask* task, int32 opcode) {
    GivePedScriptedTask(GetPedPool()->GetRef(ped), task, opcode); // Must do it like this, otherwise unhooking of the original `GivePedScriptedTask` will do nothing
}

/*!
 * @addr 0x470150
 * @brief Starts a `TASK_PLAY_ANIM*`-family task on a ped (The amount of parameters collected depends on the opcode)
 */
void CRunningScript::PlayAnimScriptCommand(int32 commandId) {
    CollectParameters(1);
    const int32 pedRef = ScriptParams[0].iParam;

    char animName[24];
    char animGroup[16];
    ReadTextLabelFromScript(animName, 0x18);
    ReadTextLabelFromScript(animGroup, 0x10);

    bool dontInterrupt = true;
    bool offsetPed     = false;
    switch (commandId) {
    case COMMAND_TASK_PLAY_ANIM_WITH_FLAGS: // 0x88A
        CollectParameters(8);
        dontInterrupt = ScriptParams[6].iParam != 0;
        offsetPed     = ScriptParams[7].iParam != 0;
        break;
    case COMMAND_TASK_PLAY_ANIM: // 0x605
        CollectParameters(6);
        break;
    case COMMAND_TASK_PLAY_ANIM_NON_INTERRUPTABLE: // 0x812
        dontInterrupt = false;
        CollectParameters(6);
        break;
    case COMMAND_TASK_PLAY_ANIM_SECONDARY: // 0xA1A
        CollectParameters(6);
        break;
    }

    // NOTE: For any other opcode no extra parameters are collected, so the values below are whatever was left in `ScriptParams`
    const float blendDelta = ScriptParams[0].fParam;
    const int32 endTime    = ScriptParams[5].iParam;

    uint32 animFlags = ANIMATION_IS_PARTIAL; // 0x10
    if (ScriptParams[1].iParam != 0 || (endTime > 0 && ScriptParams[4].iParam == 0)) {
        animFlags = ANIMATION_IS_PARTIAL | ANIMATION_IS_LOOPED; // 0x12
    }
    if (ScriptParams[2].iParam != 0) {
        animFlags |= ANIMATION_CAN_EXTRACT_VELOCITY; // 0x40
    }
    if (ScriptParams[3].iParam != 0) {
        animFlags |= ANIMATION_CAN_EXTRACT_X_VELOCITY; // 0x80
    }
    if (ScriptParams[4].iParam == 0) {
        animFlags |= ANIMATION_IS_FINISH_AUTO_REMOVE; // 0x08
    }
    if (commandId == COMMAND_TASK_PLAY_ANIM_SECONDARY) {
        animFlags |= ANIMATION_DONT_ADD_TO_PARTIAL_BLEND; // 0x400
    }

    CTask* task = new CTaskSimpleRunNamedAnim(
        animName,
        animGroup,
        animFlags,
        blendDelta,
        static_cast<uint32>(endTime < 1 ? -1 : endTime), // `-1` => No end time
        !dontInterrupt,
        CTaskSequences::ms_iActiveSequence >= 0, // Run in sequence (If one is active)
        offsetPed,
        false // Hold last frame
    );

    if (commandId == COMMAND_TASK_PLAY_ANIM_SECONDARY) {
        task = new CTaskSimpleAffectSecondaryBehaviour(true, TASK_SECONDARY_PARTIAL_ANIM, task);
    }

    GivePedScriptedTask(pedRef, task, commandId);
}

/*!
 * @addr 0x487A20
 * @brief Is a car within a box (Of size `rx, ry[, rz]`) around a point
 */
void CRunningScript::LocateCarCommand(int32 commandId) {
    const bool is3D = commandId >= COMMAND_LOCATE_CAR_3D && commandId <= COMMAND_LOCATE_STOPPED_CAR_3D;
    CollectParameters(is3D ? 8 : 6);

    CVehicle* veh = GetVehiclePool()->GetAtRef(ScriptParams[0].iParam);

    // "Locate stopped car" variants require the car to be standing still
    const bool isMoving = (commandId == COMMAND_LOCATE_STOPPED_CAR_2D || commandId == COMMAND_LOCATE_STOPPED_CAR_3D)
        && veh->m_fMovingSpeed > std::min(CTimer::GetTimeStep(), CTimer::GetOldTimeStep()) * 0.01f;

    const float x  = ScriptParams[1].fParam;
    const float y  = ScriptParams[2].fParam;
    const float z  = is3D ? ScriptParams[3].fParam : 0.f;
    const float rx = ScriptParams[is3D ? 4 : 3].fParam;
    const float ry = ScriptParams[is3D ? 5 : 4].fParam;
    const float rz = is3D ? ScriptParams[6].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 7 : 5].fParam;

    bool result = false;
    if (!isMoving) {
        const CVector& pos = veh->GetPosition();
        result =    x - rx <= pos.x && pos.x <= x + rx
                 && y - ry <= pos.y && pos.y <= y + ry
                 && (!is3D || (z - rz <= pos.z && pos.z <= z + rz));
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(CVector2D{x - rx, y - ry}, CVector2D{x + rx, y + ry}, is3D ? z : MAP_Z_LOW_LIMIT);
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x - rx, y - ry, x + rx, y + ry);
    }
}

/*!
 * @addr 0x486D80
 * @brief Is a char within a box (Of size `rx, ry[, rz]`) around a point
 */
void CRunningScript::LocateCharCommand(int32 commandId) {
    const bool is3D = commandId >= COMMAND_LOCATE_CHAR_ANY_MEANS_3D && commandId <= COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_3D;
    CollectParameters(is3D ? 8 : 6);

    CPed* ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);
    const CVector pos = GetScriptPedPosition(ped);

    bool mustBeStopped = false;
    switch (commandId) {
    case COMMAND_LOCATE_STOPPED_CHAR_ANY_MEANS_2D:
    case COMMAND_LOCATE_STOPPED_CHAR_ON_FOOT_2D:
    case COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_2D:
    case COMMAND_LOCATE_STOPPED_CHAR_ANY_MEANS_3D:
    case COMMAND_LOCATE_STOPPED_CHAR_ON_FOOT_3D:
    case COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_3D:
        mustBeStopped = true;
        break;
    }

    const float x  = ScriptParams[1].fParam;
    const float y  = ScriptParams[2].fParam;
    const float z  = is3D ? ScriptParams[3].fParam : 0.f;
    const float rx = ScriptParams[is3D ? 4 : 3].fParam;
    const float ry = ScriptParams[is3D ? 5 : 4].fParam;
    const float rz = is3D ? ScriptParams[6].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 7 : 5].fParam;

    bool result = false;
    if (!mustBeStopped || CTheScripts::IsPedStopped(ped)) {
        if (   x - rx <= pos.x && pos.x <= x + rx
            && y - ry <= pos.y && pos.y <= y + ry
            && (!is3D || (z - rz <= pos.z && pos.z <= z + rz))) {
            switch (commandId) {
            // On foot
            case COMMAND_LOCATE_CHAR_ON_FOOT_2D:
            case COMMAND_LOCATE_STOPPED_CHAR_ON_FOOT_2D:
            case COMMAND_LOCATE_CHAR_ON_FOOT_3D:
            case COMMAND_LOCATE_STOPPED_CHAR_ON_FOOT_3D:
                result = !ped->bInVehicle;
                break;
            // In a vehicle
            case COMMAND_LOCATE_CHAR_IN_CAR_2D:
            case COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_2D:
            case COMMAND_LOCATE_CHAR_IN_CAR_3D:
            case COMMAND_LOCATE_STOPPED_CHAR_IN_CAR_3D:
                result = ped->bInVehicle;
                break;
            // Any means
            case COMMAND_LOCATE_CHAR_ANY_MEANS_2D:
            case COMMAND_LOCATE_STOPPED_CHAR_ANY_MEANS_2D:
            case COMMAND_LOCATE_CHAR_ANY_MEANS_3D:
            case COMMAND_LOCATE_STOPPED_CHAR_ANY_MEANS_3D:
                result = true;
                break;
            }
        }
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(CVector2D{x - rx, y - ry}, CVector2D{x + rx, y + ry}, is3D ? z : MAP_Z_LOW_LIMIT);
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x - rx, y - ry, x + rx, y + ry);
    }
}

/*!
 * @addr 0x487D10
 * @brief Is an object within a box (Of size `rx, ry[, rz]`) around a point
 */
void CRunningScript::LocateObjectCommand(int32 commandId) {
    const bool is3D = commandId == COMMAND_IS_OBJECT_IN_AREA_3D; // This is the only opcode that passes 8 parameters
    CollectParameters(is3D ? 8 : 6);

    CObject* obj = GetObjectPool()->GetAtRef(ScriptParams[0].iParam);

    const float x  = ScriptParams[1].fParam;
    const float y  = ScriptParams[2].fParam;
    const float z  = is3D ? ScriptParams[3].fParam : 0.f;
    const float rx = ScriptParams[is3D ? 4 : 3].fParam;
    const float ry = ScriptParams[is3D ? 5 : 4].fParam;
    const float rz = is3D ? ScriptParams[6].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 7 : 5].fParam;

    const CVector& pos = obj->GetPosition();
    const bool result =    x - rx <= pos.x && pos.x <= x + rx
                        && y - ry <= pos.y && pos.y <= y + ry
                        && (!is3D || (z - rz <= pos.z && pos.z <= z + rz));

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(CVector2D{x - rx, y - ry}, CVector2D{x + rx, y + ry}, is3D ? z : MAP_Z_LOW_LIMIT);
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x - rx, y - ry, x + rx, y + ry);
    }
}

/*!
 * @addr 0x487420
 * @brief Is a char within a box (Of size `rx, ry[, rz]`) around a car
 */
void CRunningScript::LocateCharCarCommand(int32 commandId) {
    const bool is3D = commandId >= COMMAND_LOCATE_CHAR_ANY_MEANS_CAR_3D && commandId <= COMMAND_LOCATE_CHAR_IN_CAR_CAR_3D;
    CollectParameters(is3D ? 6 : 5);

    CPed*     ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);
    CVehicle* veh = GetVehiclePool()->GetAtRef(ScriptParams[1].iParam);

    const CVector  pos    = GetScriptPedPosition(ped);
    const CVector& vehPos = veh->GetPosition();

    const float rx = ScriptParams[2].fParam;
    const float ry = ScriptParams[3].fParam;
    const float rz = is3D ? ScriptParams[4].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 5 : 4].fParam;

    bool result = false;
    if (   vehPos.x - rx <= pos.x && pos.x <= vehPos.x + rx
        && vehPos.y - ry <= pos.y && pos.y <= vehPos.y + ry
        && (!is3D || (vehPos.z - rz <= pos.z && pos.z <= vehPos.z + rz))) {
        switch (commandId) {
        // On foot
        case COMMAND_LOCATE_CHAR_ON_FOOT_CAR_2D:
        case COMMAND_LOCATE_CHAR_ON_FOOT_CAR_3D:
            result = !ped->bInVehicle;
            break;
        // In a vehicle
        case COMMAND_LOCATE_CHAR_IN_CAR_CAR_2D:
        case COMMAND_LOCATE_CHAR_IN_CAR_CAR_3D:
            result = ped->bInVehicle;
            break;
        // Any means
        case COMMAND_LOCATE_CHAR_ANY_MEANS_CAR_2D:
        case COMMAND_LOCATE_CHAR_ANY_MEANS_CAR_3D:
            result = true;
            break;
        }
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(
            CVector2D{vehPos.x - rx, vehPos.y - ry},
            CVector2D{vehPos.x + rx, vehPos.y + ry},
            is3D ? vehPos.z : MAP_Z_LOW_LIMIT
        );
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(vehPos.x - rx, vehPos.y - ry, vehPos.x + rx, vehPos.y + ry);
    }
}

/*!
 * @addr 0x4870F0
 * @brief Is a char within a box (Of size `rx, ry[, rz]`) around another char
 */
void CRunningScript::LocateCharCharCommand(int32 commandId) {
    const bool is3D = commandId >= COMMAND_LOCATE_CHAR_ANY_MEANS_CHAR_3D && commandId <= COMMAND_LOCATE_CHAR_IN_CAR_CHAR_3D;
    CollectParameters(is3D ? 6 : 5);

    CPed* ped      = GetPedPool()->GetAtRef(ScriptParams[0].iParam);
    CPed* otherPed = GetPedPool()->GetAtRef(ScriptParams[1].iParam);

    const CVector pos      = GetScriptPedPosition(ped);
    const CVector otherPos = GetScriptPedPosition(otherPed);

    const float rx = ScriptParams[2].fParam;
    const float ry = ScriptParams[3].fParam;
    const float rz = is3D ? ScriptParams[4].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 5 : 4].fParam;

    bool result = false;
    if (   otherPos.x - rx <= pos.x && pos.x <= otherPos.x + rx
        && otherPos.y - ry <= pos.y && pos.y <= otherPos.y + ry
        && (!is3D || (otherPos.z - rz <= pos.z && pos.z <= otherPos.z + rz))) {
        switch (commandId) {
        // On foot
        case COMMAND_LOCATE_CHAR_ON_FOOT_CHAR_2D:
        case COMMAND_LOCATE_CHAR_ON_FOOT_CHAR_3D:
            result = !ped->bInVehicle;
            break;
        // In a vehicle
        case COMMAND_LOCATE_CHAR_IN_CAR_CHAR_2D:
        case COMMAND_LOCATE_CHAR_IN_CAR_CHAR_3D:
            result = ped->bInVehicle;
            break;
        // Any means
        case COMMAND_LOCATE_CHAR_ANY_MEANS_CHAR_2D:
        case COMMAND_LOCATE_CHAR_ANY_MEANS_CHAR_3D:
            result = true;
            break;
        }
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(
            CVector2D{otherPos.x - rx, otherPos.y - ry},
            CVector2D{otherPos.x + rx, otherPos.y + ry},
            is3D ? otherPos.z : MAP_Z_LOW_LIMIT
        );
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(otherPos.x - rx, otherPos.y - ry, otherPos.x + rx, otherPos.y + ry);
    }
}

/*!
 * @addr 0x487720
 * @brief Is a char within a box (Of size `rx, ry[, rz]`) around an object
 */
void CRunningScript::LocateCharObjectCommand(int32 commandId) {
    const bool is3D = commandId >= COMMAND_LOCATE_CHAR_ANY_MEANS_OBJECT_3D && commandId <= COMMAND_LOCATE_CHAR_IN_CAR_OBJECT_3D;
    CollectParameters(is3D ? 6 : 5);

    CPed*    ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);
    CObject* obj = GetObjectPool()->GetAtRef(ScriptParams[1].iParam);

    const CVector  pos    = GetScriptPedPosition(ped);
    const CVector& objPos = obj->GetPosition();

    const float rx = ScriptParams[2].fParam;
    const float ry = ScriptParams[3].fParam;
    const float rz = is3D ? ScriptParams[4].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 5 : 4].fParam;

    bool result = false;
    if (   objPos.x - rx <= pos.x && pos.x <= objPos.x + rx
        && objPos.y - ry <= pos.y && pos.y <= objPos.y + ry
        && (!is3D || (objPos.z - rz <= pos.z && pos.z <= objPos.z + rz))) {
        switch (commandId) {
        // On foot
        case COMMAND_LOCATE_CHAR_ON_FOOT_OBJECT_2D:
        case COMMAND_LOCATE_CHAR_ON_FOOT_OBJECT_3D:
            result = !ped->bInVehicle;
            break;
        // In a vehicle
        case COMMAND_LOCATE_CHAR_IN_CAR_OBJECT_2D:
        case COMMAND_LOCATE_CHAR_IN_CAR_OBJECT_3D:
            result = ped->bInVehicle;
            break;
        // Any means
        case COMMAND_LOCATE_CHAR_ANY_MEANS_OBJECT_2D:
        case COMMAND_LOCATE_CHAR_ANY_MEANS_OBJECT_3D:
            result = true;
            break;
        }
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(
            CVector2D{objPos.x - rx, objPos.y - ry},
            CVector2D{objPos.x + rx, objPos.y + ry},
            is3D ? objPos.z : MAP_Z_LOW_LIMIT
        );
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(objPos.x - rx, objPos.y - ry, objPos.x + rx, objPos.y + ry);
    }
}

/*!
 * @addr 0x488EC0
 * @brief Is a car within an axis-aligned box (`x1, y1[, z1]` - `x2, y2[, z2]`)
 */
void CRunningScript::CarInAreaCheckCommand(int32 commandId) {
    const bool is3D = commandId == COMMAND_IS_CAR_IN_AREA_3D || commandId == COMMAND_IS_CAR_STOPPED_IN_AREA_3D;
    CollectParameters(is3D ? 8 : 6);

    CVehicle* veh = GetVehiclePool()->GetAtRef(ScriptParams[0].iParam);

    // "Is stopped car in area" variants require the car to be standing still
    const bool isMoving = (commandId == COMMAND_IS_CAR_STOPPED_IN_AREA_2D || commandId == COMMAND_IS_CAR_STOPPED_IN_AREA_3D)
        && !CTheScripts::IsVehicleStopped(veh);

    float x1 = ScriptParams[1].fParam;
    float y1 = ScriptParams[2].fParam;
    float z1 = is3D ? ScriptParams[3].fParam : 0.f;
    float x2 = ScriptParams[is3D ? 4 : 3].fParam;
    float y2 = ScriptParams[is3D ? 5 : 4].fParam;
    float z2 = is3D ? ScriptParams[6].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 7 : 5].fParam;

    // Normalize the box's corners
    if (is3D && z2 < z1) {
        std::swap(z1, z2);
    }
    if (x2 < x1) {
        std::swap(x1, x2);
    }
    if (y2 < y1) {
        std::swap(y1, y2);
    }

    bool result = false;
    if (!isMoving) {
        const CVector& pos = veh->GetPosition();
        result =    x1 <= pos.x && pos.x <= x2
                 && y1 <= pos.y && pos.y <= y2
                 && (!is3D || (z1 <= pos.z && pos.z <= z2));
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(CVector2D{x1, y1}, CVector2D{x2, y2}, is3D ? (z1 + z2) * 0.5f : MAP_Z_LOW_LIMIT);
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x1, y1, x2, y2);
    }
}

/*!
 * @addr 0x488B50
 * @brief Is a char within an axis-aligned box (`x1, y1[, z1]` - `x2, y2[, z2]`)
 */
void CRunningScript::CharInAreaCheckCommand(int32 commandId) {
    const bool is3D = commandId == COMMAND_IS_CHAR_IN_AREA_3D
        || (commandId >= COMMAND_IS_CHAR_IN_AREA_ON_FOOT_3D && commandId <= COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_3D);
    CollectParameters(is3D ? 8 : 6);

    CPed* ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);
    const CVector pos = GetScriptPedPosition(ped);

    bool mustBeStopped = false;
    switch (commandId) {
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_ON_FOOT_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_3D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_ON_FOOT_3D:
    case COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_3D:
        mustBeStopped = true;
        break;
    }

    float x1 = ScriptParams[1].fParam;
    float y1 = ScriptParams[2].fParam;
    float z1 = is3D ? ScriptParams[3].fParam : 0.f;
    float x2 = ScriptParams[is3D ? 4 : 3].fParam;
    float y2 = ScriptParams[is3D ? 5 : 4].fParam;
    float z2 = is3D ? ScriptParams[6].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 7 : 5].fParam;

    // Normalize the box's corners
    if (is3D && z2 < z1) {
        std::swap(z1, z2);
    }
    if (x2 < x1) {
        std::swap(x1, x2);
    }
    if (y2 < y1) {
        std::swap(y1, y2);
    }

    bool result = false;
    if ((!mustBeStopped || CTheScripts::IsPedStopped(ped))
     && x1 <= pos.x && pos.x <= x2
     && y1 <= pos.y && pos.y <= y2
     && (!is3D || (z1 <= pos.z && pos.z <= z2))) {
        switch (commandId) {
        // On foot
        case COMMAND_IS_CHAR_IN_AREA_ON_FOOT_2D:
        case COMMAND_IS_CHAR_STOPPED_IN_AREA_ON_FOOT_2D:
        case COMMAND_IS_CHAR_IN_AREA_ON_FOOT_3D:
        case COMMAND_IS_CHAR_STOPPED_IN_AREA_ON_FOOT_3D:
            result = !ped->bInVehicle;
            break;
        // In a vehicle
        case COMMAND_IS_CHAR_IN_AREA_IN_CAR_2D:
        case COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_2D:
        case COMMAND_IS_CHAR_IN_AREA_IN_CAR_3D:
        case COMMAND_IS_CHAR_STOPPED_IN_AREA_IN_CAR_3D:
            result = ped->bInVehicle;
            break;
        // Any means
        case COMMAND_IS_CHAR_IN_AREA_2D:
        case COMMAND_IS_CHAR_IN_AREA_3D:
        case COMMAND_IS_CHAR_STOPPED_IN_AREA_2D:
        case COMMAND_IS_CHAR_STOPPED_IN_AREA_3D:
            result = true;
            break;
        }
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(CVector2D{x1, y1}, CVector2D{x2, y2}, is3D ? (z1 + z2) * 0.5f : MAP_Z_LOW_LIMIT);
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x1, y1, x2, y2);
    }
}

/*!
 * @addr 0x489150
 * @brief Is an object within an axis-aligned box (`x1, y1[, z1]` - `x2, y2[, z2]`)
 */
void CRunningScript::ObjectInAreaCheckCommand(int32 commandId) {
    const bool is3D = commandId == COMMAND_IS_OBJECT_IN_AREA_3D;
    CollectParameters(is3D ? 8 : 6);

    CObject* obj = GetObjectPool()->GetAtRef(ScriptParams[0].iParam);

    float x1 = ScriptParams[1].fParam;
    float y1 = ScriptParams[2].fParam;
    float z1 = is3D ? ScriptParams[3].fParam : 0.f;
    float x2 = ScriptParams[is3D ? 4 : 3].fParam;
    float y2 = ScriptParams[is3D ? 5 : 4].fParam;
    float z2 = is3D ? ScriptParams[6].fParam : 0.f;
    const float highlight = ScriptParams[is3D ? 7 : 5].fParam;

    // Normalize the box's corners
    if (is3D && z2 < z1) {
        std::swap(z1, z2);
    }
    if (x2 < x1) {
        std::swap(x1, x2);
    }
    if (y2 < y1) {
        std::swap(y1, y2);
    }

    const CVector& pos = obj->GetPosition();
    const bool result =    x1 <= pos.x && pos.x <= x2
                        && y1 <= pos.y && pos.y <= y2
                        && (!is3D || (z1 <= pos.z && pos.z <= z2));

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        HighlightImportantArea(CVector2D{x1, y1}, CVector2D{x2, y2}, is3D ? (z1 + z2) * 0.5f : MAP_Z_LOW_LIMIT);
    }

    if (CTheScripts::DbgFlag && !is3D) {
        CTheScripts::DrawDebugSquare(x1, y1, x2, y2);
    }
}

//! The (2D) area used by the "angled area" commands, which is a parallelogram going from `a` to `b`, extruded sideways by `width`
namespace {
struct ScriptAngledArea {
    CVector2D A{};     //!< Start of the area's spine
    CVector2D B{};     //!< End of the area's spine
    CVector2D ASide{}; //!< `A` offset sideways by `width`
    CVector2D BSide{}; //!< `B` offset sideways by `width`
    CVector2D Side{};  //!< `ASide` relative to `A`
    float     Z1{};    //!< Minimum Z (3D only)
    float     Z2{};    //!< Maximum Z (3D only)

    ScriptAngledArea(float ax, float ay, float bx, float by, float z1, float z2, float width) :
        A{ ax, ay },
        B{ bx, by },
        Z1{ std::min(z1, z2) },
        Z2{ std::max(z1, z2) }
    {
        // The area is centered on the spine, so it's offset sideways by the angle's perpendicular
        float angle = CGeneral::GetRadianAngleBetweenPoints(ax, ay, bx, by) + HALF_PI;
        while (angle < 0.f) {
            angle += TWO_PI;
        }
        while (angle > TWO_PI) {
            angle -= TWO_PI;
        }
        const float s = std::sin(angle), c = std::cos(angle);

        ASide = { ax + s * width, ay - c * width };
        BSide = { bx + s * width, by - c * width };
        Side  = ASide - A;
    }

    //! Check if the given position is within the area (Bounds are inclusive)
    bool IsPointInside(const CVector& pos, bool is3D) const {
        if (is3D && (pos.z < Z1 || pos.z > Z2)) {
            return false;
        }

        const CVector2D spine = B - A;
        const float     spineLen = spine.Magnitude();

        CVector2D uSpine = spine;
        uSpine.Normalise();

        const float alongSpine = (pos.x - A.x) * uSpine.x + (pos.y - A.y) * uSpine.y;
        if (alongSpine < 0.f || alongSpine > spineLen) {
            return false;
        }

        const float sideLen = Side.Magnitude(); // Must be computed before normalising
        CVector2D uSide = Side;
        uSide.Normalise();

        const float alongSide = (pos.x - A.x) * uSide.x + (pos.y - A.y) * uSide.y;
        return alongSide >= 0.f && alongSide <= sideLen;
    }

    float GetHighlightZ(bool is3D) const {
        return is3D ? (Z1 + Z2) * 0.5f : MAP_Z_LOW_LIMIT;
    }

    void Highlight(uint32 markerId, bool is3D) const {
        CTheScripts::HighlightImportantAngledArea(
            markerId,
            A.x, A.y,
            B.x, B.y,
            BSide.x, BSide.y,
            ASide.x, ASide.y,
            GetHighlightZ(is3D)
        );
    }
};
} // namespace

/*!
 * @addr 0x487F60
 * @brief Is a char within an angled (parallelogram) area
 */
void CRunningScript::CharInAngledAreaCheckCommand(int32 commandId) {
    const bool is3D = commandId >= COMMAND_IS_CHAR_IN_ANGLED_AREA_3D && commandId <= COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_3D;
    CollectParameters(is3D ? 9 : 7);

    CPed* ped = GetPedPool()->GetAtRef(ScriptParams[0].iParam);

    bool mustBeStopped = false;
    switch (commandId) {
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_ON_FOOT_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_2D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_3D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_ON_FOOT_3D:
    case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_3D:
        mustBeStopped = true;
        break;
    }

    const ScriptAngledArea area{
        ScriptParams[1].fParam,                        // a.x
        ScriptParams[2].fParam,                        // a.y
        ScriptParams[is3D ? 4 : 3].fParam,             // b.x
        ScriptParams[is3D ? 5 : 4].fParam,             // b.y
        is3D ? ScriptParams[3].fParam : 0.f,           // z1
        is3D ? ScriptParams[6].fParam : 0.f,           // z2
        ScriptParams[is3D ? 7 : 5].fParam              // width
    };
    const float highlight = ScriptParams[is3D ? 8 : 6].fParam;

    bool result = false;
    if ((!mustBeStopped || CTheScripts::IsPedStopped(ped)) && area.IsPointInside(GetScriptPedPosition(ped), is3D)) {
        switch (commandId) {
        // On foot
        case COMMAND_IS_CHAR_IN_ANGLED_AREA_ON_FOOT_2D:
        case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_ON_FOOT_2D:
        case COMMAND_IS_CHAR_IN_ANGLED_AREA_ON_FOOT_3D:
        case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_ON_FOOT_3D:
            result = !ped->bInVehicle;
            break;
        // In a vehicle
        case COMMAND_IS_CHAR_IN_ANGLED_AREA_IN_CAR_2D:
        case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_2D:
        case COMMAND_IS_CHAR_IN_ANGLED_AREA_IN_CAR_3D:
        case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_IN_CAR_3D:
            result = ped->bInVehicle;
            break;
        // Any means
        case COMMAND_IS_CHAR_IN_ANGLED_AREA_2D:
        case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_2D:
        case COMMAND_IS_CHAR_IN_ANGLED_AREA_3D:
        case COMMAND_IS_CHAR_STOPPED_IN_ANGLED_AREA_3D:
            result = true;
            break;
        }
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        area.Highlight(GetHighlightMarkerId(), is3D);
    }

    // NOTE: The original draws a debug square here (`CTheScripts::DrawDebugAngledSquare`) if `CTheScripts::DbgFlag` is set
}

/*!
 * @addr 0x488780
 * @brief Is any flamethrower flame within an angled (parallelogram) area
 * @note Unlike the other "angled area" commands this one has no entity argument, so the parameters start at index 0
 */
void CRunningScript::FlameInAngledAreaCheckCommand(int32 commandId) {
    const bool is3D = commandId == COMMAND_IS_FLAME_IN_ANGLED_AREA_3D;
    CollectParameters(is3D ? 8 : 6);

    const ScriptAngledArea area{
        ScriptParams[0].fParam,                        // a.x
        ScriptParams[1].fParam,                        // a.y
        ScriptParams[is3D ? 3 : 2].fParam,             // b.x
        ScriptParams[is3D ? 4 : 3].fParam,             // b.y
        is3D ? ScriptParams[2].fParam : 0.f,           // z1
        is3D ? ScriptParams[5].fParam : 0.f,           // z2
        ScriptParams[is3D ? 6 : 4].fParam              // width
    };
    const float highlight = ScriptParams[is3D ? 7 : 5].fParam;

    bool result = false;
    for (auto shotId = 0u; shotId < 100u && !result; shotId++) { // 100 == `MAX_SHOT_INFOS`
        CVector pos;
        if (!CShotInfo::GetFlameThrowerShotPosn(shotId, pos)) {
            continue;
        }
        result = area.IsPointInside(pos, is3D);
    }

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        area.Highlight(GetHighlightMarkerId(), is3D);
    }
}

/*!
 * @addr 0x4883F0
 * @brief Is an object within an angled (parallelogram) area
 */
void CRunningScript::ObjectInAngledAreaCheckCommand(int32 commandId) {
    const bool is3D = commandId == COMMAND_IS_OBJECT_IN_ANGLED_AREA_3D;
    CollectParameters(is3D ? 9 : 7);

    CObject* obj = GetObjectPool()->GetAtRef(ScriptParams[0].iParam);

    const ScriptAngledArea area{
        ScriptParams[1].fParam,                        // a.x
        ScriptParams[2].fParam,                        // a.y
        ScriptParams[is3D ? 4 : 3].fParam,             // b.x
        ScriptParams[is3D ? 5 : 4].fParam,             // b.y
        is3D ? ScriptParams[3].fParam : 0.f,           // z1
        is3D ? ScriptParams[6].fParam : 0.f,           // z2
        ScriptParams[is3D ? 7 : 5].fParam              // width
    };
    const float highlight = ScriptParams[is3D ? 8 : 6].fParam;

    const bool result = area.IsPointInside(obj->GetPosition(), is3D);

    UpdateCompareFlag(result);

    if (highlight != 0.f) {
        area.Highlight(GetHighlightMarkerId(), is3D);
    }

    // NOTE: The original draws a debug square here (`CTheScripts::DrawDebugAngledSquare`) if `CTheScripts::DbgFlag` is set
}

// 0x464D70
bool CRunningScript::IsPedDead(CPed* ped) const {
    ePedState pedState = ped->m_nPedState;
    return pedState == PEDSTATE_DEAD || pedState == PEDSTATE_DIE || pedState == PEDSTATE_DIE_BY_STEALTH;
}

// 0x489490
bool CRunningScript::ThisIsAValidRandomPed(ePedType pedType, bool civilian, bool gang, bool criminal) {
    switch (pedType) {
    case PED_TYPE_CIVMALE:
    case PED_TYPE_CIVFEMALE:
        return civilian;
    case PED_TYPE_GANG1:
    case PED_TYPE_GANG2:
    case PED_TYPE_GANG3:
    case PED_TYPE_GANG4:
    case PED_TYPE_GANG5:
    case PED_TYPE_GANG6:
    case PED_TYPE_GANG7:
    case PED_TYPE_GANG8:
    case PED_TYPE_GANG9:
        return gang;
    case PED_TYPE_CRIMINAL:
    case PED_TYPE_PROSTITUTE:
        return criminal;
    default:
        return false;
    }
}

// 0x485A50
void CRunningScript::DoDeathArrestCheck() {
    if (!m_IsDeathArrestCheckEnabled) {
        return;
    }

    if (!CTheScripts::IsPlayerOnAMission()) {
        return;
    }

    if (const auto& pi = FindPlayerInfo(); !pi.IsRestartingAfterDeath() && !pi.IsRestartingAfterArrest()) {
        return;
    }

    CMessages::ClearSmallMessagesOnly();
    memset(&CTheScripts::ScriptSpace[CTheScripts::OnAMissionFlag], 0, sizeof(uint32));
    ResetIP();

    m_DoneDeathArrest = true;
    m_WakeTime        = 0;
}

// 0x464F50
void CRunningScript::GetCorrectPedModelIndexForEmergencyServiceType(ePedType pedType, uint32* typeSpecificModelId) {
    switch (*typeSpecificModelId) {
    case MODEL_LAPD1:
    case MODEL_SFPD1:
    case MODEL_LVPD1:
    case MODEL_LAPDM1:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_CITYCOP;
        }
        break;
    case MODEL_CSHER:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_CSHER;
        }
        break;
    case MODEL_SWAT:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_SWAT1;
        }
        break;
    case MODEL_FBI:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_FBI;
        }
        break;
    case MODEL_ARMY:
        if (pedType == PED_TYPE_COP) {
            *typeSpecificModelId = COP_TYPE_ARMY;
        }
        break;
    default:
        return;
    }
}

// Returns state of pad button
// 0x485B10
int16 CRunningScript::GetPadState(uint16 playerIndex, eButtonId buttonId) {
    const auto* pad = CPad::GetPad(playerIndex);
    switch (buttonId) {
    case BUTTON_LEFT_STICK_X:    return pad->NewState.LeftStickX;
    case BUTTON_LEFT_STICK_Y:    return pad->NewState.LeftStickY;
    case BUTTON_RIGHT_STICK_X:   return pad->NewState.RightStickX;
    case BUTTON_RIGHT_STICK_Y:   return pad->NewState.RightStickY;
    case BUTTON_LEFT_SHOULDER1:  return pad->NewState.LeftShoulder1;
    case BUTTON_LEFT_SHOULDER2:  return pad->NewState.LeftShoulder2;
    case BUTTON_RIGHT_SHOULDER1: return pad->NewState.RightShoulder1;
    case BUTTON_RIGHT_SHOULDER2: return pad->NewState.RightShoulder2;
    case BUTTON_DPAD_UP:         return pad->NewState.DPadUp;
    case BUTTON_DPAD_DOWN:       return pad->NewState.DPadDown;
    case BUTTON_DPAD_LEFT:       return pad->NewState.DPadLeft;
    case BUTTON_DPAD_RIGHT:      return pad->NewState.DPadRight;
    case BUTTON_START:           return pad->NewState.Start;
    case BUTTON_SELECT:          return pad->NewState.Select;
    case BUTTON_SQUARE:          return pad->NewState.ButtonSquare;
    case BUTTON_TRIANGLE:        return pad->NewState.ButtonTriangle;
    case BUTTON_CROSS:           return pad->NewState.ButtonCross;
    case BUTTON_CIRCLE:          return pad->NewState.ButtonCircle;
    case BUTTON_LEFTSHOCK:       return pad->NewState.ShockButtonL;
    case BUTTON_RIGHTSHOCK:      return pad->NewState.ShockButtonR;
    default:                     return OR_CONTINUE;
    }
}

/*!
 * @addr 0x46AF50
 * @brief Makes a ped pick up (Hold) an object, optionally playing an animation
 */
void CRunningScript::ScriptTaskPickUpObject(int32 commandId) {
    CollectParameters(7);

    const int32 pedRef = ScriptParams[0].iParam;
    CObject*    object = GetObjectPool()->GetAtRef(ScriptParams[1].iParam);

    const CVector posn{
        ScriptParams[2].fParam,
        ScriptParams[3].fParam,
        ScriptParams[4].fParam
    };
    const uint8 boneFrameId = (uint8)(ScriptParams[5].iParam);
    const uint8 boneFlags   = (uint8)(ScriptParams[6].iParam);

    char animName[24];
    char animGroup[16];
    ReadTextLabelFromScript(animName, 0x18);
    ReadTextLabelFromScript(animGroup, 0x10);

    // If either of the labels is "NULL" no animation is played
    const bool noAnim = !std::strncmp(animName, "NULL", 5) || !std::strncmp(animGroup, "NULL", 5);

    CollectParameters(1);
    const eAnimationFlags animFlags = ScriptParams[0].iParam == 0
        ? static_cast<eAnimationFlags>(ANIMATION_IS_FINISH_AUTO_REMOVE | ANIMATION_IS_PARTIAL) // 0x18
        : ANIMATION_IS_PARTIAL;                                                                // 0x10

    CTask* task = noAnim
        ? static_cast<CTask*>(new CTaskSimpleHoldEntity(object, &posn, boneFrameId, boneFlags, AnimationId::ANIM_ID_NO_ANIMATION_SET, ANIM_GROUP_DEFAULT, false))
        : static_cast<CTask*>(new CTaskSimpleHoldEntity(object, &posn, boneFrameId, boneFlags, animName, animGroup, animFlags));

    if (pedRef == -1) { // No ped, so add the task to the active task sequence
        if (commandId == COMMAND_TASK_PICK_UP_OBJECT) {
            task = new CTaskSimpleAffectSecondaryBehaviour(true, TASK_SECONDARY_PARTIAL_ANIM, task);
        }
        CTaskSequences::AddTaskToActiveSequence(task);
    } else {
        CPed* ped = GetPedPool()->GetAtRef(pedRef);
        ped->GetTaskManager().SetTaskSecondary(
            task,
            commandId == COMMAND_TASK_PICK_UP_OBJECT
                ? TASK_SECONDARY_PARTIAL_ANIM
                : TASK_SECONDARY_ATTACK
        );
        CPedScriptedTaskRecord::ms_scriptedTasks[CPedScriptedTaskRecord::GetVacantSlot()].Set(ped, commandId, task);
    }
}

// 0x464DC0
void CRunningScript::SetCharCoordinates(CPed& ped, CVector posn, bool warpGang, bool offset) {
    CWorld::PutToGroundIfTooLow(posn);

    CVehicle* vehicle = ped.GetVehicleIfInOne();
    if (vehicle) {
        posn.z += vehicle->GetDistanceFromCentreOfMassToBaseOfModel();
        vehicle->Teleport(posn, false);
        CTheScripts::ClearSpaceForMissionEntity(posn, vehicle);
    } else {
        posn.z += offset ? ped.GetDistanceFromCentreOfMassToBaseOfModel() : 0.0f;
        CTheScripts::ClearSpaceForMissionEntity(posn, &ped);
        auto* group = CPedGroups::GetPedsGroup(&ped);
        if (group && group->GetMembership().IsLeader(&ped) && warpGang) {
            group->Teleport(posn);
        } else {
            ped.Teleport(posn, false);
        }
    }
}

// 0x463CA0
tScriptParam* CRunningScript::GetPointerToLocalVariable(int32 loc) {
    return &GetLocal<tScriptParam>(loc);
}

/*!
 * @addr 0x463CC0
 * @brief Returns pointer to a local script variable.
 *
 * @param arrayBaseOffset          The offset of the array
 * @param index                    Index of the variable inside the array
 * @param arrayEntriesSizeAsParams Size of 1 variable in the array (In terms of `tScriptParam`'s - So for a regular `int` (or float, etc) variable this will be `1`, for long strings it's `4` and for short one's it's `2`)
 */
tScriptParam* CRunningScript::GetPointerToLocalArrayElement(int32 arrayBaseOffset, uint16 index, uint8 arrayEntriesSizeAsParams) {
    return &GetArrayLocal<tScriptParam>(arrayBaseOffset, index, arrayEntriesSizeAsParams);
}

/*!
 * Returns pointer to script variable of any type.
 * @addr 0x464790
 */
tScriptParam* CRunningScript::GetPointerToScriptVariable(eScriptVariableType) {
    uint8  arrElemSize;
    uint16 arrVarOffset;
    int32  arrElemIdx;

    int8 type = CTheScripts::Read1ByteFromScript(m_IP);
    switch (type) {
    case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
    case SCRIPT_PARAM_GLOBAL_SHORT_STRING_VARIABLE:
    case SCRIPT_PARAM_GLOBAL_LONG_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        return reinterpret_cast<tScriptParam*>(&CTheScripts::ScriptSpace[index]);
    }
    case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
    case SCRIPT_PARAM_LOCAL_SHORT_STRING_VARIABLE:
    case SCRIPT_PARAM_LOCAL_LONG_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        return GetPointerToLocalVariable(index);
    }

    case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
    case SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY:
    case SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY:
        ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
        if (type == SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY)
            return reinterpret_cast<tScriptParam*>(&CTheScripts::ScriptSpace[LONG_STRING_SIZE * arrElemIdx + arrVarOffset]);
        else if (type == SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY)
            return reinterpret_cast<tScriptParam*>(&CTheScripts::ScriptSpace[SHORT_STRING_SIZE * arrElemIdx + arrVarOffset]);
        else // SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY
            return reinterpret_cast<tScriptParam*>(&CTheScripts::ScriptSpace[4 * arrElemIdx + arrVarOffset]);

    case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
    case SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY:
    case SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY:
        ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
        if (type == SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY)
            arrElemSize = 4;
        else if (type == SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY)
            arrElemSize = 2;
        else // SCRIPT_PARAM_LOCAL_NUMBER_ARRAY
            arrElemSize = 1;
        return GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, arrElemSize);

    default:
        NOTSA_UNREACHABLE();
    }
}

/*!
 * Returns offset of global variable
 * @addr 0x464700
 */
uint16 CRunningScript::GetIndexOfGlobalVariable() {
    switch (const auto t = GetAtIPAs<uint8>()) {
    case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        return GetAtIPAs<uint16>();
    case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY: {
        uint16 base;
        int32  idx;
        ReadArrayInformation(true, &base, &idx);
        return base + sizeof(tScriptParam) * idx;
    }
    default:
        NOTSA_UNREACHABLE();
    }
}

// 0x464080
void CRunningScript::CollectParameters(int16 count) {
    uint16 arrVarOffset;
    int32  arrElemIdx;

    for (auto i = 0; i < count; i++) {
        switch (CTheScripts::Read1ByteFromScript(m_IP)) {
        case SCRIPT_PARAM_STATIC_INT_32BITS:
            ScriptParams[i].iParam = CTheScripts::Read4BytesFromScript(m_IP);
            break;
        case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            ScriptParams[i].iParam = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[index]);
            break;
        }
        case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            ScriptParams[i] = *GetPointerToLocalVariable(index);
            break;
        }
        case SCRIPT_PARAM_STATIC_INT_8BITS:
            ScriptParams[i].iParam = CTheScripts::Read1ByteFromScript(m_IP);
            break;
        case SCRIPT_PARAM_STATIC_INT_16BITS:
            ScriptParams[i].iParam = CTheScripts::Read2BytesFromScript(m_IP);
            break;
        case SCRIPT_PARAM_STATIC_FLOAT:
            ScriptParams[i].fParam = CTheScripts::ReadFloatFromScript(m_IP);
            break;
        case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            ScriptParams[i].iParam = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[arrVarOffset + 4 * arrElemIdx]);
            break;
        case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            ScriptParams[i] = *GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 1);
            break;
        }
    }
}

/*!
 * Collects parameter and returns it.
 * @addr 0x464250
 */
int32 CRunningScript::CollectNextParameterWithoutIncreasingPC() {
    uint16 arrVarOffset;
    int32  arrElemIdx;
    uint8* ip = m_IP;
    int32  result = -1;

    switch (CTheScripts::Read1ByteFromScript(m_IP)) {
    case SCRIPT_PARAM_STATIC_INT_32BITS:
    case SCRIPT_PARAM_STATIC_FLOAT:
        result = CTheScripts::Read4BytesFromScript(m_IP);
        break;
    case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        result = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[index]);
        break;
    }
    case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        result = GetPointerToLocalVariable(index)->iParam;
        break;
    }
    case SCRIPT_PARAM_STATIC_INT_8BITS:
        result = CTheScripts::Read1ByteFromScript(m_IP);
        break;
    case SCRIPT_PARAM_STATIC_INT_16BITS:
        result = CTheScripts::Read2BytesFromScript(m_IP);
        break;
    case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
        ReadArrayInformation(false, &arrVarOffset, &arrElemIdx);
        result = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[arrVarOffset + 4 * arrElemIdx]);
        break;
    case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
        ReadArrayInformation(false, &arrVarOffset, &arrElemIdx);
        result = GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 1)->iParam;
        break;
    }

    m_IP = ip;
    return result;
}

/*!
 * @addr 0x464370
 */
void CRunningScript::StoreParameters(int16 count) {
    uint16 arrVarOffset;
    int32  arrElemIdx;

    for (auto i = 0; i < count; i++) {
        switch (CTheScripts::Read1ByteFromScript(m_IP)) {
        case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[index]) = ScriptParams[i].iParam;
            break;
        }
        case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            *GetPointerToLocalVariable(index) = ScriptParams[i];
            break;
        }
        case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[arrVarOffset + 4 * arrElemIdx]) = ScriptParams[i].iParam;
            break;
        case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            *GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 1) = ScriptParams[i];
            break;
        }
    }
}

// Reads array var base offset and element index from index variable.
// 0x463CF0
void CRunningScript::ReadArrayInformation(int32 updateIP, uint16* outArrayBase, int32* outArrayIndex) {
    const auto op = GetAtIPAs<scm::ArrayAccess>(updateIP);
    *outArrayIndex = op.IdxVarIsGlobal
        ? GetGlobal<int32>(op.IdxVarLoc)
        : GetLocal<int32>(op.IdxVarLoc);
    *outArrayBase = op.ArrayBase;
}

// Collects parameters and puts them to local variables of new script
// 0x464500
void CRunningScript::ReadParametersForNewlyStartedScript(CRunningScript* newScript) {
    uint16 arrVarOffset;
    int32  arrElemIdx;
    int8   type = CTheScripts::Read1ByteFromScript(m_IP);

    for (int i = 0; type != SCRIPT_PARAM_END_OF_ARGUMENTS; type = CTheScripts::Read1ByteFromScript(m_IP), i++) {
        switch (type) {
        case SCRIPT_PARAM_STATIC_INT_32BITS:
            newScript->m_LocalVars[i].iParam = CTheScripts::Read4BytesFromScript(m_IP);
            break;
        case SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            newScript->m_LocalVars[i].iParam = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[index]);
            break;
        }
        case SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE:
        {
            uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
            newScript->m_LocalVars[i] = *GetPointerToLocalVariable(index);
            break;
        }
        case SCRIPT_PARAM_STATIC_INT_8BITS:
            newScript->m_LocalVars[i].iParam = CTheScripts::Read1ByteFromScript(m_IP);
            break;
        case SCRIPT_PARAM_STATIC_INT_16BITS:
            newScript->m_LocalVars[i].iParam = CTheScripts::Read2BytesFromScript(m_IP);
            break;
        case SCRIPT_PARAM_STATIC_FLOAT:
            newScript->m_LocalVars[i].fParam = CTheScripts::ReadFloatFromScript(m_IP);
            break;
        case SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            newScript->m_LocalVars[i].iParam = *reinterpret_cast<int32*>(&CTheScripts::ScriptSpace[arrVarOffset + 4 * arrElemIdx]);
            break;
        case SCRIPT_PARAM_LOCAL_NUMBER_ARRAY:
            ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
            newScript->m_LocalVars[i] = *GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 1);
            break;
        default:
            break;
        }
    }
}

// Collects string parameter
// 0x463D50
void CRunningScript::ReadTextLabelFromScript(char* buffer, uint8 nBufferLength) {
    uint16 arrVarOffset;
    int32  arrElemIdx;

    int8 type = CTheScripts::Read1ByteFromScript(m_IP);
    switch (type) {
    case SCRIPT_PARAM_STATIC_SHORT_STRING:
        for (auto i = 0; i < SHORT_STRING_SIZE; i++)
            buffer[i] = CTheScripts::Read1ByteFromScript(m_IP);
        break;

    case SCRIPT_PARAM_GLOBAL_SHORT_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        strncpy_s(buffer, SHORT_STRING_SIZE, (char*)&CTheScripts::ScriptSpace[index], SHORT_STRING_SIZE);
        break;
    }

    case SCRIPT_PARAM_LOCAL_SHORT_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        strncpy_s(buffer, SHORT_STRING_SIZE, (char*) GetPointerToLocalVariable(index), SHORT_STRING_SIZE);
        break;
    }

    case SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY:
    case SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY:
        ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
        if (type == SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY)
            strncpy_s(buffer, SHORT_STRING_SIZE, (char*) & CTheScripts::ScriptSpace[SHORT_STRING_SIZE * arrElemIdx + arrVarOffset], SHORT_STRING_SIZE);
        else
            strncpy_s(buffer, SHORT_STRING_SIZE, (char*) & CTheScripts::ScriptSpace[LONG_STRING_SIZE * arrElemIdx + arrVarOffset], std::min<uint8>(nBufferLength, LONG_STRING_SIZE));
        break;

    case SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY:
    case SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY:
        ReadArrayInformation(true, &arrVarOffset, &arrElemIdx);
        if (type == SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY)
            strncpy_s(buffer, SHORT_STRING_SIZE, (char*) GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 2), SHORT_STRING_SIZE);
        else {
            const auto bufferLength = std::min<uint8>(nBufferLength, LONG_STRING_SIZE);
            strncpy_s(buffer, bufferLength, (char*)GetPointerToLocalArrayElement(arrVarOffset, arrElemIdx, 4), bufferLength);
        }
        break;

    case SCRIPT_PARAM_STATIC_PASCAL_STRING:
    {
        int16 nStringLen = CTheScripts::Read1ByteFromScript(m_IP); // sign extension. max size = 127, not 255
        for (auto i = 0; i < nStringLen; i++)
            buffer[i] = CTheScripts::Read1ByteFromScript(m_IP);

        if (nStringLen < nBufferLength)
            memset(&buffer[(uint8)nStringLen], 0, (uint8)(nBufferLength - nStringLen));
        break;
    }

    case SCRIPT_PARAM_STATIC_LONG_STRING:
        // slightly changed code: original code is a bit messy and calls Read1ByteFromScript
        // in a loop and does some additional checks to ensure that buffer can hold the data
        strncpy_s(buffer, LONG_STRING_SIZE, (char*)m_IP, std::min<uint8>(nBufferLength, LONG_STRING_SIZE));
        m_IP += LONG_STRING_SIZE;
        break;

    case SCRIPT_PARAM_GLOBAL_LONG_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        strncpy_s(buffer, LONG_STRING_SIZE, (char*) & CTheScripts::ScriptSpace[index], std::min<uint8>(nBufferLength, LONG_STRING_SIZE));
        break;
    }

    case SCRIPT_PARAM_LOCAL_LONG_STRING_VARIABLE:
    {
        uint16 index = CTheScripts::Read2BytesFromScript(m_IP);
        strncpy_s(buffer, LONG_STRING_SIZE, (char*) GetPointerToLocalVariable(index), std::min<uint8>(nBufferLength, LONG_STRING_SIZE));
        break;
    }

    default:
        break;
    }
}

// Updates comparement flag, used in conditional commands
// 0x4859D0
void CRunningScript::UpdateCompareFlag(bool state) {
    if (m_NotFlag)
        state = !state;

    if (m_AndOrState == ANDOR_NONE) {
        m_CondResult = state;
        return;
    }

    if (m_AndOrState >= ANDS_1 && m_AndOrState <= ANDS_8) {
        m_CondResult &= state;
        if (m_AndOrState == ANDS_1)
            m_AndOrState = ANDOR_NONE;
        else
            m_AndOrState--;

        return;
    }

    if (m_AndOrState >= ORS_1 && m_AndOrState <= ORS_8) {
        m_CondResult |= state;
        if (m_AndOrState == ORS_1)
            m_AndOrState = ANDOR_NONE;
        else
            m_AndOrState--;

        return;
    }
}

// Sets instruction pointer, used in GOTO-like commands
// 0x464DA0
void CRunningScript::UpdatePC(int32 newIP) {
    m_IP = newIP >= 0
        ? &CTheScripts::ScriptSpace[newIP]
        : m_BaseIP + std::abs(newIP);
}

// 0x469EB0, inlined
OpcodeResult CRunningScript::ProcessOneCommand() {
    ++CTheScripts::CommandsExecuted;

    const auto op = GetAtIPAs<scm::Instruction>();

    // Check if IP is valid pre-return
    notsa::ScopeGuard guardIP{[this]() {
        const auto next{ GetAtIPAs<scm::Instruction>(false) };
        VERIFY(next.Command <= COMMAND_HIGHEST_VANILLA_ID);
    }};

#ifdef NOTSA_SCRIPT_TRACING
    // snprintf is faster (in debug at least) - Gotta stick to it for now
    char msg[4096];
    sprintf_s(msg, "[%s][IP: 0x%X + 0x%X]: %s [0x%X]", m_szName, LOG_PTR(m_pBaseIP), LOG_PTR(m_IP - m_pBaseIP), notsa::script::GetScriptCommandName((eScriptCommands)op.Command).data(), (size_t)op.Command);
    SPDLOG_LOGGER_TRACE(logger, msg);
    //SPDLOG_LOGGER_TRACE(logger, "[{}][IP: {:#x} + {:#x}]: {} [{:#x}]", BaseFilename, LOG_PTR(m_pBaseIP), LOG_PTR(m_IP - m_pBaseIP), notsa::script::GetScriptCommandName((eScriptCommands)op.command), (size_t)op.command);
#endif
    
    m_NotFlag = op.NotFlag;

    if (const auto handler = CustomCommandHandlerOf((eScriptCommands)(op.Command))) {
        return std::invoke(handler, this);
    } else {
#ifdef NOTSA_NATIVE
        // There's no executable to fall back to: report the missing handler and abort
        notsa::native::UnreversedScriptCommand(op.Command, notsa::script::GetScriptCommandName((eScriptCommands)(op.Command)).data());
#else
        return std::invoke(s_OriginalCommandHandlerTable[(size_t)op.Command / 100], this, (eScriptCommands)(op.Command));
#endif
    }
}

// 0x469F00
OpcodeResult CRunningScript::Process() {
    if (m_SceneSkipIP && CCutsceneMgr::IsCutsceneSkipButtonBeingPressed()) {
        CHud::m_BigMessage[1][0] = 0;
        UpdatePC(std::exchange(m_SceneSkipIP, 0));
        m_WakeTime = 0;
    }

    if (m_UsesMissionCleanup) {
        DoDeathArrestCheck();
    }

    if (m_ThisMustBeTheOnlyMissionRunning && CTheScripts::FailCurrentMission == 1) {
        if (m_StackDepth > 0) {
            ResetIP();
        }
    }

    CTheScripts::ReinitialiseSwitchStatementData();

    if (CTimer::GetTimeInMS() >= (uint32)m_WakeTime) {
        while (ProcessOneCommand() == OR_CONTINUE)
            ; // Process commands
    }

    return OR_CONTINUE;
}

uint32 CRunningScript::GetHighlightMarkerId() const {
    return static_cast<uint32>(CTheScripts::GetScriptIndexFromPointer(const_cast<CRunningScript*>(this)) * CTheScripts::SCRIPT_SPACE_SIZE)
         + static_cast<uint32>(m_IP - CTheScripts::ScriptSpace.data());
}

void CRunningScript::HighlightImportantArea(CVector2D from, CVector2D to, float z) {
    CTheScripts::HighlightImportantArea(GetHighlightMarkerId(), from.x, from.y, to.x, to.y, z);
}

void CRunningScript::HighlightImportantArea(CRect area, float z) {
    HighlightImportantArea(area.GetTopLeft(), area.GetBottomRight(), z);
}

void CRunningScript::HighlightImportantArea(CVector from, CVector to) {
    HighlightImportantArea(CVector2D{ from }, CVector2D{ to }, (from.z + to.z) / 2.f);
}

void CRunningScript::HighlightImportantAngledArea(uint32 id, CVector2D a, CVector2D b, CVector2D c, CVector2D d) {
    NOTSA_UNREACHABLE(); // Fuck this, we dont need it!
}

notsa::script::CommandHandlerFunction& CRunningScript::CustomCommandHandlerOf(eScriptCommands command) {
    return s_CustomCommandHandlerTable[(size_t)(command)];
}

void CRunningScript::ResetIP() {
    assert(m_StackDepth > 0); // Original bug...
    do {
        m_IP = std::exchange(m_IPStack[--m_StackDepth], nullptr); // NOTSA: Also clear the stack, we don't need it anymore
    } while (m_StackDepth);
}

//! Address `CTheScripts::ScriptSpace` has in the original (32 bit) game - Pointers into it are stored in save games relative to it
static constexpr uint32 s_ScriptSpaceSaveAddr = 0xA49960;

//! Address `ScriptsArray` (The array all running scripts live in) has in the original (32 bit) game
static constexpr uint32 s_ScriptsArraySaveAddr = 0xA8B430;

//! Convert a pointer into the script space to the address stored in save games
static uint32 ToSaveGameAddr(const uint8* p) {
    if (!p) {
        return 0;
    }
    VERIFY(CTheScripts::ScriptSpace.data() <= p && p < CTheScripts::ScriptSpace.data() + CTheScripts::SCRIPT_SPACE_SIZE);
    return s_ScriptSpaceSaveAddr + static_cast<uint32>(p - CTheScripts::ScriptSpace.data());
}

//! Convert an address loaded from a save game to a pointer into our script space
static uint8* FromSaveGameAddr(uint32 addr) {
    if (!addr) {
        return nullptr;
    }
    VERIFY(addr >= s_ScriptSpaceSaveAddr && addr < s_ScriptSpaceSaveAddr + CTheScripts::SCRIPT_SPACE_SIZE);
    return &CTheScripts::ScriptSpace[addr - s_ScriptSpaceSaveAddr];
}

//! Convert a pointer to a script in `ScriptsArray` to the address stored in save games
static uint32 ToSaveGameScriptAddr(const CRunningScript* script) {
    if (!script) {
        return 0;
    }
    return s_ScriptsArraySaveAddr + static_cast<uint32>(CTheScripts::GetScriptIndexFromPointer(const_cast<CRunningScript*>(script)) * sizeof(CRunningScriptSave32));
}

void CRunningScriptSave32::Construct(const CRunningScript& script) {
    // NOTE: These are not read back on load (The script list is re-linked from the live scripts),
    //       but they're still written (as the original game did) so the saved image stays the same
    m_pNext = ToSaveGameScriptAddr(script.m_pNext);
    m_pPrev = ToSaveGameScriptAddr(script.m_pPrev);

    rng::copy(script.m_szName, m_szName);

    m_BaseIP = ToSaveGameAddr(script.m_BaseIP);
    m_IP     = ToSaveGameAddr(script.m_IP);
    rng::transform(script.m_IPStack, m_IPStack, ToSaveGameAddr);

    m_StackDepth = script.m_StackDepth;
    rng::copy(script.m_LocalVars, m_LocalVars);

    m_IsActive                        = script.m_IsActive;
    m_CondResult                      = script.m_CondResult;
    m_UsesMissionCleanup              = script.m_UsesMissionCleanup;
    m_IsExternal                      = script.m_IsExternal;
    m_IsTextBlockOverride             = script.m_IsTextBlockOverride;
    m_ExternalType                    = script.m_ExternalType;
    m_WakeTime                        = script.m_WakeTime;
    m_AndOrState                      = script.m_AndOrState;
    m_NotFlag                         = script.m_NotFlag;
    m_IsDeathArrestCheckEnabled       = script.m_IsDeathArrestCheckEnabled;
    m_DoneDeathArrest                 = script.m_DoneDeathArrest;
    m_SceneSkipIP                     = script.m_SceneSkipIP;
    m_ThisMustBeTheOnlyMissionRunning = script.m_ThisMustBeTheOnlyMissionRunning;
}

void CRunningScriptSave32::Extract(CRunningScript& script) const {
    // NOTE: `script.m_pNext` and `script.m_pPrev` are intentionally left as they are

    rng::copy(m_szName, script.m_szName);

    script.m_BaseIP = FromSaveGameAddr(m_BaseIP);
    script.m_IP     = FromSaveGameAddr(m_IP);
    rng::transform(m_IPStack, script.m_IPStack.begin(), FromSaveGameAddr);

    script.m_StackDepth = m_StackDepth;
    rng::copy(m_LocalVars, script.m_LocalVars.begin());

    script.m_IsActive                        = m_IsActive;
    script.m_CondResult                      = m_CondResult;
    script.m_UsesMissionCleanup              = m_UsesMissionCleanup;
    script.m_IsExternal                      = m_IsExternal;
    script.m_IsTextBlockOverride             = m_IsTextBlockOverride;
    script.m_ExternalType                    = m_ExternalType;
    script.m_WakeTime                        = m_WakeTime;
    script.m_AndOrState                      = m_AndOrState;
    script.m_NotFlag                         = m_NotFlag;
    script.m_IsDeathArrestCheckEnabled       = m_IsDeathArrestCheckEnabled;
    script.m_DoneDeathArrest                 = m_DoneDeathArrest;
    script.m_SceneSkipIP                     = m_SceneSkipIP;
    script.m_ThisMustBeTheOnlyMissionRunning = m_ThisMustBeTheOnlyMissionRunning;
}
