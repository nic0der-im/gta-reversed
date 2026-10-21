#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include <algorithm>
#include <cstring>
#include <new>

#include <spdlog/spdlog.h>

#include "app/platform/native/ExeData.manual.h"
#include "app/platform/native/ExeGlobals.h"

#include "AudioEngine.h"
#include "ControllerConfigManager.h"
#include "Gangs.h"
#include "Pad.h"
#include "PedClothesDesc.h"
#include "Tasks/PedScriptedTaskRecord.h"
#include "World.h"
#include "cHandlingDataMgr.h"

//! Defined in `Clothes.cpp`, bound to `StaticRef<CPedClothesDesc>(0xBC1C78)`.
extern CPedClothesDesc& PlayerClothes;

namespace notsa::native {
namespace {
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
        break;

    case ExeDataKind::VectorCtor:
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

        // `CWorld::Players` (0x00B7CD98) stays unapplied: element constructor 0x00571920 does
        // more than clear (it constructs `m_PlayerData` through 0x0056F210), and our
        // `CPlayerInfo::CPlayerInfo` still forwards to it (`PlayerInfo.cpp:43`). Reverse that
        // constructor first. See `docs/NativePort.md`.
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
