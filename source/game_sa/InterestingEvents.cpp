#include "StdInc.h"

#include "InterestingEvents.h"

auto& g_InterestingEvents = StaticRef<CInterestingEvents>(0xC0B058);

void CInterestingEvents::InjectHooks() {
    RH_ScopedClass(CInterestingEvents);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Constructor, 0x6023A0);
    RH_ScopedInstall(Destructor, 0x856880);
    RH_ScopedInstall(Add, 0x602590);
    RH_ScopedInstall(ScanForNearbyEntities, 0x605A30);
    RH_ScopedInstall(GetInterestingEvent, 0x6028A0);
    RH_ScopedInstall(InvalidateEvent, 0x602960);
    RH_ScopedInstall(InvalidateNonVisibleEvents, 0x6029C0);
}

// 0x6023A0
CInterestingEvents::CInterestingEvents()
{
    // The original does a read-modify-write on the whole flag byte, so any higher bits
    // survive a re-construction in place: Clear `m_b1`, set `m_b2`, `m_b4` and `m_b8`
    m_nFlags = static_cast<uint8>((m_nFlags & 0xFE) | 0x0E);

    /* Everything missing from here is initialized in the header */

    const auto SetOptions = [=](auto index, auto priority, auto delay, uint32 end = 0) {
        m_nPriorities[index] = priority;
        m_nDelays[index]     = delay;
        m_nEndsOfTime[index] = end;
    };

    SetOptions(INTERESTING_EVENT_0,     5,  2000);
    SetOptions(PEDS_CHATTING,           1,  5000);
    SetOptions(INTERESTING_EVENT_2,     1,  5000);
    SetOptions(INTERESTING_EVENT_3,     1,  5000);
    SetOptions(INTERESTING_EVENT_4,     2,  3000);
    SetOptions(INTERESTING_EVENT_5,     2,  3000);
    SetOptions(INTERESTING_EVENT_6,     2,  3000);
    SetOptions(INTERESTING_EVENT_7,     2,  3000);
    SetOptions(INTERESTING_EVENT_8,     4,  3000);
    SetOptions(INTERESTING_EVENT_9,     4,  3000);
    SetOptions(INTERESTING_EVENT_10,    5,  6000);
    SetOptions(INTERESTING_EVENT_11,    6,  6000);
    SetOptions(INTERESTING_EVENT_12,    6,  8000);
    SetOptions(INTERESTING_EVENT_13,    6,  5000);
    SetOptions(INTERESTING_EVENT_14,    5,  6000);
    SetOptions(INTERESTING_EVENT_15,    9,  6000);
    SetOptions(INTERESTING_EVENT_16,    9,  6000);
    SetOptions(VEHICLE_DAMAGE,          8,  6000);
    SetOptions(INTERESTING_EVENT_18,    7,  6000);
    SetOptions(INTERESTING_EVENT_19,    6,  5000);
    SetOptions(INTERESTING_EVENT_20,    7,  6000);
    SetOptions(INTERESTING_EVENT_21,    8,  8000);
    SetOptions(INTERESTING_EVENT_22,    9,  5000);
    SetOptions(GANG_ATTACKING_PED,      9,  6000);
    SetOptions(GANG_FIGHT,              9,  6000);
    SetOptions(INTERESTING_EVENT_25,    9,  6000);
    SetOptions(ZELDICK_OCCUPATION,      9,  8000);
    SetOptions(EVENT_ATTRACTOR,         10, 4000);
    SetOptions(INTERESTING_EVENT_28,    10, 4000);
}

CInterestingEvents* CInterestingEvents::Constructor() {
    this->CInterestingEvents::CInterestingEvents();
    return this;
}

// 0x856880
CInterestingEvents::~CInterestingEvents() {
    // The original walks the global object by absolute address and never reads `this`.
    for (auto& event : g_InterestingEvents.m_Events) {
        CEntity::ClearReference(event.entity);
    }
}

CInterestingEvents* CInterestingEvents::Destructor() {
    this->CInterestingEvents::~CInterestingEvents();
    return this;
}

// 0x602590
void CInterestingEvents::Add(CInterestingEvents::EType type, CEntity* entity) {
    if (!m_b1 || !entity)
        return;

    const CVector camPos = CCamera::GetActiveCamera().m_vecSource;

    if (m_CurrentFrameCounter != CTimer::GetFrameCounter()) {
        m_CurrentFrameCounter = CTimer::GetFrameCounter();

        CPlayerPed* player = FindPlayerPed(-1);
        const CVector& playerPos = player->GetPosition();
        vec148 = playerPos - camPos;
        vec148.z = 0.f;
        if (vec148.NormaliseAndMag() == 0.f) {
            vec148 = player->GetMatrix().GetForward();
        }
        m_vecCenter = (vec148 * m_fRadius) + playerPos;
    }

    const CVector& pos = entity->GetPosition();
    const float    dx  = m_vecCenter.x - pos.x;
    const float    dy  = m_vecCenter.y - pos.y;
    if (dy * dy + dx * dx > m_fRadius * m_fRadius)
        return;

    // Only creatures in front of the player (relative to `vec148`) are interesting
    if (m_b2 && vec148.Dot(pos) - vec148.Dot(camPos) < 0.f)
        return;

    if (!CWorld::GetIsLineOfSightClear(camPos, pos, true, false, false, false, false, true, false))
        return;

    const uint32 now         = CTimer::GetTimeInMS();
    const bool   endsExpired = now > m_nEndsOfTime[type];

    TInterestingEvent* slot = nullptr;
    for (int32 i = 0; i < MAX_INTERESTING_EVENTS; i++) {
        TInterestingEvent& event = m_Events[i];
        if (!event.entity) {
            event.type   = 0;
            event.entity = nullptr;
            slot         = &event;
            break;
        }
        if (event.type == 0) {
            slot = &event;
            break;
        }
        if (!endsExpired || m_nInterestingEvent == i)
            continue;
        if (m_nPriorities[type] >= m_nPriorities[event.type] || now > event.time + static_cast<uint32>(m_nDelays[event.type])) {
            slot = &event;
            break;
        }
    }
    if (!slot)
        return;

    if (slot->entity)
        slot->entity->CleanUpOldReference(&slot->entity);
    slot->type   = type;
    slot->entity = entity;
    slot->time   = now;
    entity->RegisterReference(&slot->entity);
    m_nEndsOfTime[type] = m_b8 ? now + (m_nDelays[type] >> 1) : now;
}

// 0x605A30
void CInterestingEvents::ScanForNearbyEntities() {
    // NB: The original reads the flag and passes `this` from the global object (0xC0B058),
    // never from the object the method was invoked on.
    if (!g_InterestingEvents.m_b1)
        return;

    const auto now = CTimer::GetTimeInMS();
    if (now - m_nLastScanTimeUpdate < 500) { // Unsigned compare, as in the original
        return;
    }
    m_nLastScanTimeUpdate = now;

    CPlayerPed* const player = FindPlayerPed(-1);
    if (m_CurrentFrameCounter != CTimer::GetFrameCounter()) {
        m_CurrentFrameCounter = CTimer::GetFrameCounter();

        const CVector  camPos    = CCamera::GetActiveCamera().m_vecSource;
        const CVector& playerPos = player->GetPosition();
        vec148 = playerPos - camPos;
        vec148.z = 0.f;
        if (vec148.NormaliseAndMag() == 0.f)
            vec148 = player->GetMatrix().GetForward();
        m_vecCenter = (vec148 * m_fRadius) + playerPos;
    }

    // `(pos * 0.02f + 60.f)` transformed into a sector index - In float, then floored as a double (as the original)
    const auto SectorIndex = [](float pos) {
        return static_cast<int32>(std::floor(static_cast<double>(pos * 0.02f + 60.f)));
    };

    const int32 startSectorX = std::max(SectorIndex(m_vecCenter.x - m_fRadius), 0);
    const int32 startSectorY = std::max(SectorIndex(m_vecCenter.y - m_fRadius), 0);
    const int32 endSectorX   = std::min(SectorIndex(m_vecCenter.x + m_fRadius), MAX_SECTORS_X - 1);
    const int32 endSectorY   = std::min(SectorIndex(m_vecCenter.y + m_fRadius), MAX_SECTORS_Y - 1);

    CWorld::AdvanceCurrentScanCode();
    player->SetCurrentScanCode();

    for (int32 sectorY = startSectorY; sectorY <= endSectorY; ++sectorY) {
        for (int32 sectorX = startSectorX; sectorX <= endSectorX; ++sectorX) {
            auto& rs = CWorld::GetRepeatSector(sectorX, sectorY);

            for (auto* const ped : rs.Peds) {
                if (ped->IsScanCodeCurrent())
                    continue;

                ped->SetCurrentScanCode(); // Before the state check, so dead peds aren't re-examined either

                if (ped->m_nPedState == PEDSTATE_DEAD)
                    continue;

                // The vehicle is more interesting than the ped driving it
                CEntity* entity = ped;
                if (ped->bInVehicle && ped->m_pVehicle) {
                    entity = ped->m_pVehicle;
                }

                switch (ped->m_nPedType) {
                case PED_TYPE_COP:
                    g_InterestingEvents.Add(INTERESTING_EVENT_5, entity);
                    break;
                case PED_TYPE_CRIMINAL:
                    g_InterestingEvents.Add(INTERESTING_EVENT_6, entity);
                    break;
                case PED_TYPE_PROSTITUTE:
                    g_InterestingEvents.Add(INTERESTING_EVENT_4, entity);
                    break;
                default:
                    if (IsPedTypeGang(ped->m_nPedType)) { // PED_TYPE_GANG1 .. PED_TYPE_GANG10
                        g_InterestingEvents.Add(INTERESTING_EVENT_7, entity);
                    }
                    break;
                }
            }

            for (auto* const vehicle : rs.Vehicles) {
                if (vehicle->IsScanCodeCurrent())
                    continue;

                vehicle->SetCurrentScanCode();

                if (vehicle->physicalFlags.bRenderScorched)
                    continue;

                if (!vehicle->m_pDriver)
                    continue;

                const auto style = vehicle->m_autoPilot.m_nCarDrivingStyle;
                if (style == DRIVING_STYLE_STOP_FOR_CARS)
                    continue;

                if (style == DRIVING_STYLE_DRIVINGMODE_AVOIDCARS_STOPFORPEDS_OBEYLIGHTS)
                    continue;

                g_InterestingEvents.Add(INTERESTING_EVENT_14, vehicle);
            }
        }
    }
}

// 0x6028A0
TInterestingEvent* CInterestingEvents::GetInterestingEvent() {
    const auto now = CTimer::GetTimeInMS();

    // Return the previously picked event for as long as it's still fresh
    if (m_b4 && m_nInterestingEvent != -1) {
        TInterestingEvent& cached = m_Events[m_nInterestingEvent];
        if (cached.entity && now < cached.time + static_cast<uint32>(m_nDelays[cached.type])) {
            return &cached;
        }
    }

    uint8 bestPriority = 0;
    int8  best         = -1;
    for (int32 i = 0; i < MAX_INTERESTING_EVENTS; i++) {
        TInterestingEvent& event = m_Events[i];
        if (!event.entity)
            continue;

        if (now >= event.time + static_cast<uint32>(m_nDelays[event.type])) // Expired
            continue;

        // Strictly higher priority wins, otherwise there's a rare random chance of picking it anyway.
        // The original calls `_rand` directly (Not `CGeneral::GetRandomNumber`), which is the same function as `rand`.
        if (m_nPriorities[event.type] > bestPriority || (rand() & 0xFFFF) < 0x80) {
            bestPriority = m_nPriorities[event.type];
            best         = i;
        }
    }
    m_nInterestingEvent = best;

    return best == -1 ? nullptr : &m_Events[best];
}

// 0x602960
void CInterestingEvents::InvalidateEvent(const TInterestingEvent* event) {
    for (auto index = 0; index < MAX_INTERESTING_EVENTS; index++) {
        TInterestingEvent* tevent = &m_Events[index];
        if (tevent != event)
            continue;

        tevent->time = 0;
        CEntity::ClearReference(tevent->entity);
        if (m_nInterestingEvent == index) {
            m_nInterestingEvent = -1;
        }
    }
}

// 0x6029C0
void CInterestingEvents::InvalidateNonVisibleEvents() {
    const auto& camPos = CCamera::GetActiveCamera().m_vecSource;
    for (auto i = 0; i < MAX_INTERESTING_EVENTS; i++) {
        TInterestingEvent& event = m_Events[i];
        if (!event.entity)
            continue;

        CVector pos = event.entity->GetPosition();
        if (CWorld::GetIsLineOfSightClear(camPos, pos, true, false, false, false, false, true, false))
            continue;

        event.time = 0;
        CEntity::ClearReference(event.entity);
        if (m_nInterestingEvent == i) {
            m_nInterestingEvent = -1;
        }
    }
}
