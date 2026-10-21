#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include <cstring>
#include <new>

#include <spdlog/spdlog.h>

#include "app/platform/native/ExeData.manual.h"
#include "app/platform/native/ExeGlobals.h"

#include "ControllerConfigManager.h"
#include "Pad.h"
#include "World.h"
#include "cHandlingDataMgr.h"

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
} // namespace

bool ApplyManualExeDataEntry(const ExeDataEntry& e) {
    switch (e.Kind) {
    case ExeDataKind::ObjectCtor:
        // 0x84D920: `mov ecx, 0x00B70198; jmp 0x00531EE0`. `CControllerConfigManager`'s
        // constructor is reversed (`ControllerConfigManager.cpp:80`), and its whole call tree is
        // in-repo code, so run it.
        if (e.Addr == 0xB70198) {
            ::new (static_cast<void*>(&ControlsManager)) CControllerConfigManager{};
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
