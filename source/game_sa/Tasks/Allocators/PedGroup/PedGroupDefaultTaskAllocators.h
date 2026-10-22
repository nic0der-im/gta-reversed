#pragma once

#include <reversiblehooks/ReversibleHooks.h>

#include "./PedGroupDefaultTaskAllocator.h"
#include "./PedGroupDefaultTaskAllocatorType.h"
#include "./PedGroupDefaultTaskAllocatorFollowAnyMeans.h"
#include "./PedGroupDefaultTaskAllocatorFollowLimited.h"
#include "./PedGroupDefaultTaskAllocatorStandStill.h"
#include "./PedGroupDefaultTaskAllocatorChat.h"
#include "./PedGroupDefaultTaskAllocatorSitInLeaderCar.h"
#include "./PedGroupDefaultTaskAllocatorRandom.h"

class CPedGroupDefaultTaskAllocators {
public:
    //! The allocator singletons, in executable address order.
    //! The executable constructs them from its CRT initializer table (`0x851270`..`0x851330`:
    //! `mov ecx, 0x00C099xx; call 0x5F6450`..), and those constructors only store the vtable.
    //! Natively they are constructed by `ExeData.manual.cpp` (`kCtorRows`), so without that row the
    //! first `Get()` + virtual call would jump through a null vtable.
    static inline auto& ms_FollowAnyMeansAllocator = StaticRef<CPedGroupDefaultTaskAllocatorFollowAnyMeans>(0xC09908);
    static inline auto& ms_FollowLimitedAllocator  = StaticRef<CPedGroupDefaultTaskAllocatorFollowLimited>(0xC0990C);
    static inline auto& ms_StandStillAllocator     = StaticRef<CPedGroupDefaultTaskAllocatorStandStill>(0xC09910);
    static inline auto& ms_ChatAllocator           = StaticRef<CPedGroupDefaultTaskAllocatorChat>(0xC09914);
    static inline auto& ms_RandomAllocator         = StaticRef<CPedGroupDefaultTaskAllocatorRandom>(0xC09918);
    static inline auto& ms_SitInLeaderCarAllocator = StaticRef<CPedGroupDefaultTaskAllocatorSitInLeaderCar>(0xC0991C);

public:
    static const CPedGroupDefaultTaskAllocator& Get(ePedGroupDefaultTaskAllocatorType type) {
        using enum ePedGroupDefaultTaskAllocatorType;
        switch (type) {
        case FOLLOW_ANY_MEANS:  return ms_FollowAnyMeansAllocator;
        case FOLLOW_LIMITED:    return ms_FollowLimitedAllocator;
        case STAND_STILL:       return ms_StandStillAllocator;
        case CHAT:              return ms_ChatAllocator;
        case SIT_IN_LEADER_CAR: return ms_SitInLeaderCarAllocator;
        case RANDOM:            return ms_RandomAllocator;
        default:                NOTSA_UNREACHABLE();
        }
    }

public:
    static void InjectHooks() {
        RH_ScopedClass(CPedGroupDefaultTaskAllocators);
        RH_ScopedCategory("Tasks/Allocators/PedGroup");

        //RH_ScopedInstall(Get, 0x5F7200, { .State = HS::RedirectToGTA, .Locked = true }); // Spoils <ecx>, can't hook
    }
};
