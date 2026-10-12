#include "StdInc.h"

#include "EntityScanner.h"

void CEntityScanner::InjectHooks() {
    RH_ScopedClass(CEntityScanner);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Constructor, 0x5FF990);
    RH_ScopedInstall(Destructor, 0x603480);

    RH_ScopedInstall(Clear, 0x5FF9D0);
    RH_ScopedInstall(ScanForEntitiesInRange, 0x5FFA20);
}

// 0x5FF990
CEntityScanner::CEntityScanner() {
    m_timer = {};

    rng::fill(m_apEntities, nullptr);
    m_pClosestEntityInRange = nullptr;

    m_timer.SetPeriod(MAX_NUM_ENTITIES);
}

// 0x603480
CEntityScanner::~CEntityScanner() {
    Clear();
}

// 0x5FF9D0
void CEntityScanner::Clear() {
    for (auto& entity : m_apEntities) {
        CEntity::ClearReference(entity);
    }

    CEntity::ClearReference(m_pClosestEntityInRange);
}

// 0x5FFA20
void CEntityScanner::ScanForEntitiesInRange(const eRepeatSectorList sectorList, const CPed& ped) {
    if (!m_timer.Tick()) {
        return;
    }

    Clear();

    const auto isPlayer = FindPlayerPed() == &ped;

    const auto* const intel = ped.GetIntelligence();
    const auto range   = std::max(intel->m_fHearingRange, intel->m_fSeeingRange);
    const auto rangeSq = range * range;
    const auto& pos    = ped.GetPosition();

    std::array<float, MAX_NUM_ENTITIES> dists;
    dists.fill(FLT_MAX);
    int32 numEntities = 0;

    const auto ToSector = [](float v, float center) {
        return static_cast<int32>(std::floor((v - center) * 0.02f + 60.0f));
    };
    const auto minX = std::max(0, ToSector(pos.x, range));
    const auto minY = std::max(0, ToSector(pos.y, range));
    const auto maxX = std::min(0x77, ToSector(pos.x, -range));
    const auto maxY = std::min(0x77, ToSector(pos.y, -range));

    CWorld::AdvanceCurrentScanCode();
    // NOTE: Original also stamps the scanning ped's own scan code
    const_cast<CPed&>(ped).SetScanCode(CWorld::GetCurrentScanCode());

    const auto ProcessList = [&](auto& list) {
        for (CEntity* const entity : list) {
            if (entity->IsScanCodeCurrent()) {
                continue;
            }
            entity->SetCurrentScanCode();

            if (sectorList == REPEATSECTOR_PEDS && !isPlayer && static_cast<CPed*>(entity)->m_nPedState == PEDSTATE_DEAD) {
                continue;
            }

            const auto distSq = (entity->GetPosition() - pos).SquaredMagnitude();
            if (!(distSq < rangeSq)) {
                continue;
            }

            // Insert into the sorted (closest first) array
            size_t i = 0;
            for (; i < MAX_NUM_ENTITIES; i++) {
                if (!m_apEntities[i]) {
                    dists[i] = distSq;
                    numEntities++;
                    m_apEntities[i] = entity;
                    break;
                }
                if (distSq < dists[i]) {
                    // Shift everything from i onwards one slot further
                    for (auto j = (size_t)std::min(numEntities, (int32)MAX_NUM_ENTITIES - 1); j > i; j--) { // 0x5FFDFB
                        m_apEntities[j] = m_apEntities[j - 1];
                        dists[j]        = dists[j - 1];
                    }
                    dists[i]        = distSq;
                    m_apEntities[i] = entity;
                    if (numEntities < (int32)MAX_NUM_ENTITIES) {
                        numEntities++;
                    }
                    break;
                }
            }
        }
    };

    for (auto y = minY; y <= maxY; y++) {
        for (auto x = minX; x <= maxX; x++) {
            auto& rs = CWorld::GetRepeatSector(x, y);
            switch (sectorList) {
            case REPEATSECTOR_VEHICLES: ProcessList(rs.Vehicles); break;
            case REPEATSECTOR_PEDS:     ProcessList(rs.Peds);     break;
            case REPEATSECTOR_OBJECTS:  ProcessList(rs.Objects);  break;
            default:                    break; // NOTE: Original dereferences a null list here (crashes); unreachable with the 3-value enum
            }
        }
    }

    for (auto& entity : m_apEntities) {
        if (entity) {
            entity->RegisterReference(&entity);
        }
    }

    if (m_apEntities[0]) {
        m_pClosestEntityInRange = m_apEntities[0];
        m_pClosestEntityInRange->RegisterReference(&m_pClosestEntityInRange);
    }
}
