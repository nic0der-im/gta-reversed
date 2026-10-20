/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "PluginBase.h"

int _NOINLINE_ plugin::GetBaseAddress() {
#ifdef NOTSA_NATIVE
    // The native executable has no preferred base address, and `GetGlobalAddress` is
    // only used to turn exe offsets into pointers (Which abort anyway, see `PluginBase.h`)
    return 0;
#else
    static int addr = reinterpret_cast<int>(GetModuleHandleA(NULL));
    return addr;
#endif
}

int _NOINLINE_ plugin::GetGlobalAddress(int address) {
    return GetBaseAddress() - 0x400000 + address;
}
