#include "StdInc.h"
#include "Interior_c.h"

#include "InteriorGroup_c.h"
#include "InteriorManager_c.h"
#include "Furniture_c.h"
#include "FurnitureManager_c.h"

void Interior_c::InjectHooks() {
    RH_ScopedClass(Interior_c);
    RH_ScopedCategory("Interior");

    //RH_ScopedInstall(Constructor, 0x5921D0, { .Reversed = false });
    //RH_ScopedInstall(Destructor, 0x591360, { .Reversed = false });

    RH_ScopedInstall(Bedroom_AddTableItem, 0x593F10);
    RH_ScopedInstall(FurnishBedroom, 0x593FC0, { .Reversed = false });
    RH_ScopedInstall(Kitchen_FurnishEdges, 0x596930, { .Reversed = false });
    RH_ScopedInstall(FurnishKitchen, 0x5970B0);
    RH_ScopedInstall(Lounge_AddTV, 0x597240);
    RH_ScopedInstall(Lounge_AddHifi, 0x597430);
    RH_ScopedInstall(Lounge_AddChairInfo, 0x5974E0);
    RH_ScopedInstall(Lounge_AddSofaInfo, 0x5975C0);
    RH_ScopedInstall(FurnishLounge, 0x597740, { .Reversed = false });
    RH_ScopedInstall(Office_PlaceEdgeFillers, 0x599210);
    RH_ScopedInstall(Office_PlaceDesk, 0x5993E0);
    RH_ScopedInstall(Office_PlaceEdgeDesks, 0x5995B0);
    RH_ScopedInstall(Office_FurnishEdges, 0x599770);
    RH_ScopedInstall(Office_PlaceDeskQuad, 0x599960);
    RH_ScopedInstall(Office_FurnishCenter, 0x599A30);
    RH_ScopedInstall(FurnishOffice, 0x599AF0);
    RH_ScopedInstall(Shop_Place3PieceUnit, 0x599BB0);
    RH_ScopedInstall(Shop_PlaceEdgeUnits, 0x599DC0);
    RH_ScopedInstall(Shop_PlaceCounter, 0x599EF0);
    RH_ScopedInstall(Shop_PlaceFixedUnits, 0x59A030);
    RH_ScopedInstall(Shop_FurnishCeiling, 0x59A130);
    RH_ScopedInstall(Shop_AddShelfInfo, 0x59A140);
    RH_ScopedInstall(Shop_FurnishEdges, 0x59A1B0);
    RH_ScopedInstall(GetBoundingBox, 0x593DB0);
    RH_ScopedInstall(Init, 0x593BF0);
    RH_ScopedInstall(ResetTiles, 0x593910);
    RH_ScopedInstall(PlaceObject, 0x5934E0);
    RH_ScopedInstall(GetFurnitureEntity, 0x5913B0);
    RH_ScopedInstall(IsPtInside, 0x5913E0);
    RH_ScopedInstall(CalcMatrix, 0x5914D0);
    RH_ScopedInstall(Furnish, 0x591590);
    RH_ScopedInstall(Unfurnish, 0x5915D0);
    RH_ScopedInstall(CheckTilesEmpty, 0x591680);
    RH_ScopedInstall(SetTilesStatus, 0x591700);
    RH_ScopedInstall(SetCornerTiles, 0x5917C0);
    RH_ScopedInstall(GetTileStatus, 0x5918E0);
    RH_ScopedInstall(GetNumEmptyTiles, 0x591920);
    RH_ScopedInstall(GetRandomTile, 0x591B20);
    RH_ScopedInstall(Shop_FurnishAisles, 0x59A590);
    RH_ScopedInstall(GetTileCentre, 0x591BD0);
    RH_ScopedInstall(AddGotoPt, 0x591D20);
    RH_ScopedInstall(AddInteriorInfo, 0x591E40);
    RH_ScopedInstall(AddPickups, 0x591F90);
    RH_ScopedInstall(Exit, 0x592230);
    RH_ScopedInstall(FindBoundingBox, 0x5922C0);
    RH_ScopedInstall(CalcExitPts, 0x5924A0);
    RH_ScopedInstall(IsVisible, 0x5929F0);
    RH_ScopedInstall(PlaceFurniture, 0x592AA0);
    RH_ScopedInstall(PlaceFurnitureOnWall, 0x593120);
    RH_ScopedInstall(PlaceFurnitureInCorner, 0x593340);
    RH_ScopedInstall(FindEmptyTiles, 0x591C50);
    RH_ScopedInstall(FurnishShop, 0x59A790);
}

// 0x593BF0
int32 Interior_c::Init(const CVector& pos) {
    CalcMatrix(const_cast<CVector*>(&pos));
    ResetTiles();

    auto* const entity = m_pGroup->GetEntity();
    if (!entity->m_matrix) {
        entity->AllocateMatrix();
        entity->m_placement.UpdateMatrix(entity->m_matrix);
    }
    const CVector interiorPos = entity->m_matrix->GetPosition();

    if (m_box->m_type != 0x63) { // 0x63 == Don't furnish
        // TODO(re): Identify the object pointed to by `0xBB3DAC` - Retail mixes its fields at `+0x8`, `+0x14` and `+0x18` into the seed
        static auto& s_UnkRngSeedSource = StaticRef<void*>(0xBB3DAC);

        int32 seed = (int32)interiorPos.z * (int32)interiorPos.y * (int32)interiorPos.x + m_box->m_seed;
        if (const auto p = s_UnkRngSeedSource) {
            const auto ReadFloatAt = [p](size_t offset) {
                return *reinterpret_cast<const float*>(reinterpret_cast<const uint8*>(p) + offset);
            };
            seed += (int32)ReadFloatAt(0x18) * (int32)ReadFloatAt(0x14) * (int32)ReadFloatAt(0x8);
        }
        srand(seed);
    }

    m_numGotoPoints      = 0;
    m_interiorInfosCount = 0;

    switch (m_box->m_type) {
    case 0: FurnishShop(0);    break;
    case 1: FurnishOffice();   break;
    case 2: FurnishLounge();   break;
    case 3: FurnishBedroom();  break;
    case 4: FurnishKitchen();  break;
    default: break;
    }

    CalcExitPts();

    if (!g_interiorMan.HasInteriorHadStealDataSetup(this)) {
        //! `g_interiorMan.m_InteriorIds` (Not writable through the manager's API)
        static auto& s_InteriorCount = StaticRef<size_t>(0xBB3914);
        static auto& s_InteriorIds   = StaticRef<std::array<int32, 64>>(0xBB3918);
        if (s_InteriorCount < s_InteriorIds.size()) {
            s_InteriorIds[s_InteriorCount++] = m_interiorId;
        }
    }

    if (m_box->m_type == 2 || m_box->m_type == 3) {
        AddPickups();
    }

    return 1;
}

// 0x592230
void Interior_c::Exit() {
    // Remove the pickups that `AddPickups` might've added around the interior
    CPickups::RemovePickUpsInArea(
        m_position.x - 50.0f, m_position.x + 50.0f,
        m_position.y - 50.0f, m_position.y + 50.0f,
        m_position.z - 50.0f, m_position.z + 50.0f
    );

    Unfurnish();
}

// 0x593F10
CObject* Interior_c::Bedroom_AddTableItem(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6, int32 a7) {
    float offsetX = (float)a5;
    float offsetY = (float)a6;

    if (a4 == 0 || a4 == 2) {
        offsetX += 0.5f;
    } else if (a4 == 1 || a4 == 3) {
        offsetY += 0.5f;
    }

    auto* const furniture = g_furnitureMan.GetFurniture(a2, a3, -1, m_box->m_status);
    return PlaceObject(1, furniture, offsetX + 0.5f, offsetY + 0.5f, 0.5f, (float)a7 * 90.0f); // `90.0f` is the constant at `0x85991C`
}

// 0x593FC0
// TODO(re): Not decoded - ~0x2960 bytes (0x593FC0-0x596930) of bedroom furniture placement.
//           Uses `Bedroom_AddTableItem`, `PlaceFurniture*` and `g_furnitureMan`; needs a dedicated pass.
void Interior_c::FurnishBedroom() {
    plugin::CallMethod<0x593FC0, Interior_c*>(this);
}

// 0x596930
CObject* Interior_c::Kitchen_FurnishEdges() {
    const int32 width = m_box->m_width;
    const int32 depth = m_box->m_depth;

    auto* const wallFurniture = g_furnitureMan.GetFurniture(4, 7, m_furnitureId, m_box->m_status);

    int32 out1, out2;
    const bool bottomLeftPlaced  = PlaceFurniture(wallFurniture, 0,         depth - 1, 0.0f, 1, 1, &out1, &out2, 0) != nullptr;
    const bool bottomRightPlaced = PlaceFurniture(wallFurniture, width - 1, depth - 1, 0.0f, 1, 0, &out1, &out2, 0) != nullptr;

    //! Number of tiles already taken on either side by the doors/windows
    //! (`0x858B8C` is `0.5f`, not needed here - the ends are tile indices)
    const int32 leftEnd  = std::max<int32>(0, std::max<int32>(m_box->m_lDoorEnd,  m_box->m_lWindowEnd));
    const int32 rightEnd = std::max<int32>(0, std::max<int32>(m_box->m_rDoorEnd, m_box->m_rWindowEnd));

    int32 numLeftTiles, numRightTiles;
    if (rightEnd > 0) {
        numRightTiles = rightEnd;
        numLeftTiles  = leftEnd;
    } else if (leftEnd > 0) {
        numLeftTiles  = leftEnd;
        numRightTiles = 0;
    } else {
        //! No door/window on either side - skip a random amount of tiles
        numLeftTiles  = CGeneral::GetRandomNumberInRange(0, depth - 1);
        numRightTiles = CGeneral::GetRandomNumberInRange(0, depth - 1);
    }

    if (numLeftTiles > 0) {
        SetTilesStatus(0, 0, 1, numLeftTiles, 2, 0);
    }
    if (numRightTiles > 0) {
        SetTilesStatus(width - 1, 0, 1, numRightTiles, 2, 0);
    }

    PlaceFurniture(
        g_furnitureMan.GetFurniture(4, 0, m_furnitureId, m_box->m_status),
        0,
        numLeftTiles,
        0.0f, 1, 1,
        &out1, &out2, 0
    );
    PlaceFurniture(
        g_furnitureMan.GetFurniture(4, 2, m_furnitureId, m_box->m_status),
        width - 1,
        numRightTiles,
        0.0f, 1, 3,
        &out1, &out2, 0
    );

    //! Furniture along the bottom wall, starting at the (left) window/door start tile.
    //! NOTE: `PlaceFurnitureOnWall` only writes its out params when it actually placed something,
    //!       so a still-`-1` orientation means "nothing was placed" (Retail checks the call's leftover `eax`).
    int32 usedOrientation = -1, usedOffset = 0;
    PlaceFurnitureOnWall(
        4, 3, m_furnitureId,
        0.0f, 1, 0, m_box->m_tWindowStart, 0,
        &usedOrientation, &usedOffset,
        nullptr, nullptr, nullptr, nullptr
    );
    if (usedOrientation != -1) {
        // `1.5f` is the constant at `0x858CE8`
        AddInteriorInfo(5, (float)(usedOffset + 1), (float)depth - 1.5f, 2, nullptr);
    }

    //! Random wall pieces (orientation and offset both picked at random)
    PlaceFurnitureOnWall(4, 5, m_furnitureId, 0.0f, 1, -1, -1, 0, &usedOrientation, &usedOffset, nullptr, nullptr, nullptr, nullptr);
    PlaceFurnitureOnWall(4, 4, m_furnitureId, 0.0f, 1, -1, -1, 0, &usedOrientation, &usedOffset, nullptr, nullptr, nullptr, nullptr);
    PlaceFurnitureOnWall(4, 6, m_furnitureId, 0.0f, 1, -1, -1, 0, &usedOrientation, &usedOffset, nullptr, nullptr, nullptr, nullptr);

    auto* const randomTable = g_furnitureMan.GetFurniture(4, 10, -1, m_box->m_status);
    if (CGeneral::GetRandomNumber() < 0x3FFF && bottomLeftPlaced) {
        PlaceObject(1, randomTable, 0.5f, (float)depth - 0.5f, 1.05f, 45.0f); // `1.05f` == `0x3F866666`, `45.0f` == `0x42340000`
    } else if (bottomRightPlaced) {
        PlaceObject(1, randomTable, (float)width - 0.5f, (float)depth - 0.5f, 1.05f, 315.0f); // `315.0f` == `0x439D8000`
    }

    //! The edge pieces are collected first, and only a random subset of them gets used below
    float xs[32]{}, ys[32]{}, rotations[32]{}, used[32]{};
    int32   numPieces = 0;

    //! Left wall (bottom to top)
    for (int32 y = numLeftTiles + 1; y < depth - 1; y++) {
        if (PlaceFurniture(wallFurniture, 0, y, 0.0f, 1, 1, &out1, &out2, 0)) {
            xs[numPieces]        = 0.5f;
            ys[numPieces]        = (float)y + 0.5f;
            rotations[numPieces] = 90.0f; // `90.0f` == `0x42B40000`
            numPieces++;
        }
    }

    //! Back wall (left to right)
    for (int32 x = 1; x < width - 1; x++) {
        if (PlaceFurniture(wallFurniture, x, depth - 1, 0.0f, 1, 0, &out1, &out2, 0)) {
            xs[numPieces]        = (float)x + 0.5f;
            ys[numPieces]        = (float)(depth - 1) + 0.5f;
            rotations[numPieces] = 0.0f;
            numPieces++;
        }
    }

    //! Right wall (bottom to top)
    for (int32 y = numRightTiles + 1; y <= depth - 2; y++) {
        if (PlaceFurniture(wallFurniture, width - 1, y, 0.0f, 1, 3, &out1, &out2, 0)) {
            xs[numPieces]        = (float)(width - 1) + 0.5f;
            ys[numPieces]        = (float)y + 0.5f;
            rotations[numPieces] = 270.0f; // `270.0f` == `0x43870000`
            numPieces++;
        }
    }

    CObject* lastPlaced{};
    if (numPieces > 0) {
        //! `CGeneral::GetRandomNumber()` is normalised by `1 / 32768` (the constant at `0x858B14`)
        const auto RandomPieceIndex = [numPieces] {
            return (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * (float)numPieces);
        };

        const int32 pieceIdx = RandomPieceIndex();
        used[pieceIdx] = 1.0f; // `1.0f` == `0x3F800000`

        int32 otherPieceIdx = RandomPieceIndex();
        for (int32 tries = 0; used[otherPieceIdx] == 1.0f; tries++) {
            if (tries >= 30) {
                otherPieceIdx = -1;
                break;
            }
            otherPieceIdx = RandomPieceIndex();
        }

        //! Percentage threshold (0..100) below which an extra piece is placed - depends on the interior's wealth
        int32 threshold;
        if (m_box->m_status >= 75) {
            threshold = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 20.0f); // `20.0f` is the constant at `0x858BA4`
        } else if (m_box->m_status >= 50) {
            // TODO(re): 0x859CE4 - Assumed to be `-30.0f` (Inferred from the 0/20/50/100 threshold ladder; the constant lives in the packed game exe's uninitialized `.data`)
            threshold = 20 - (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * -30.0f);
        } else {
            threshold = 50 - (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * -50.0f); // `-50.0f` is the constant at `0x859B60`
        }

        for (int32 i = 0; i < numPieces; i++) {
            if (used[i] != 0.0f) { // Already used as one of the two special pieces below
                continue;
            }
            if ((int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 100.0f) >= threshold) { // `100.0f` is the constant at `0x858628`
                continue;
            }
            auto* const furniture = g_furnitureMan.GetFurniture(
                8,
                CGeneral::GetRandomNumber() < 0x3FFF ? 3 : 4,
                -1,
                m_box->m_status
            );
            if (!furniture) {
                continue;
            }
            PlaceObject(0, furniture, xs[i], ys[i], 1.05f, rotations[i]);
        }

        if (pieceIdx != -1) {
            lastPlaced = PlaceObject(1, g_furnitureMan.GetFurniture(4, 8, -1, m_box->m_status), xs[pieceIdx], ys[pieceIdx], 1.05f, rotations[pieceIdx]);
        }
        if (otherPieceIdx != -1) {
            lastPlaced = PlaceObject(1, g_furnitureMan.GetFurniture(4, 9, -1, m_box->m_status), xs[otherPieceIdx], ys[otherPieceIdx], 1.05f, rotations[otherPieceIdx]);
        }
    }

    return lastPlaced;
}

// 0x5970B0
void Interior_c::FurnishKitchen() {
    const int32 width = m_box->m_width;
    const int32 depth = m_box->m_depth;

    SetTilesStatus(m_box->m_door - 1, 0, 2, 1, 7, 0);

    for (int32 x = 1; x <= width - 2; x++) {
        SetTilesStatus(x, depth - 2, 1, 1, 3, 0);
        SetTilesStatus(x, 0, 1, 1, 3, 0);
    }
    for (int32 y = 0; y <= depth - 2; y++) {
        SetTilesStatus(1, y, 1, 1, 3, 0);
        SetTilesStatus(width - 2, y, 1, 1, 3, 0);
    }

    AddGotoPt(1, 1, 0.0f, 0.0f);
    AddGotoPt(1, depth - 2, 0.0f, 0.0f);
    AddGotoPt(width - 2, 1, 0.0f, 0.0f);
    AddGotoPt(width - 2, depth - 2, 0.0f, 0.0f);

    m_furnitureId = g_furnitureMan.GetRandomId(4, 0, m_box->m_status);
    Kitchen_FurnishEdges();

    auto* const furniture = g_furnitureMan.GetFurniture(8, 1, -1, m_box->m_status);

    int32 out1, out2;
    PlaceFurniture(
        furniture,
        (int32)((float)furniture->m_nWidthX * 0.5f - (float)width * 0.5f),
        (int32)((float)furniture->m_nWidthY * 0.5f - (float)depth * 0.5f),
        0.0f,
        0,
        0,
        &out1,
        &out2,
        0
    );
}

// 0x597240
CObject* Interior_c::Lounge_AddTV(int32 a2, int32 a3, int32 a4, int32 a5) {
    const int32 orientation = a2 & 3;

    const float width = (float)m_box->m_width;
    const float depth = (float)m_box->m_depth;

    float x1{}, y1{}, x2{}, y2{};
    switch (a2) {
    case 0:
        x1 = 0.5f;
        y1 = depth - 0.5f;
        x2 = 1.5f;
        y2 = depth - 0.5f;
        AddInteriorInfo(0, 1.0f, depth - 2.0f, -1, nullptr);
        break;
    case 1:
        x1 = 0.5f;
        y1 = 0.5f;
        x2 = 0.5f;
        y2 = 1.5f;
        AddInteriorInfo(0, 1.0f, 1.0f, -1, nullptr);
        break;
    case 2:
        x1 = width - 0.5f;
        y1 = 0.5f;
        x2 = width - 1.5f;
        y2 = 0.5f;
        AddInteriorInfo(0, width - 2.0f, 1.0f, -1, nullptr);
        break;
    case 3:
        x1 = width - 0.5f;
        y1 = depth - 0.5f;
        x2 = width - 0.5f;
        y2 = depth - 1.5f;
        AddInteriorInfo(0, width - 2.0f, depth - 2.0f, -1, nullptr);
        break;
    default: // Retail leaves these uninitialized
        break;
    }

    PlaceObject(
        1,
        g_furnitureMan.GetFurniture(2, 3, -1, m_box->m_status),
        x1, y1, 0.5f,
        (float)orientation * 90.0f + 45.0f // `45.0f` is the constant at `0x858CB4`
    );

    auto* const furniture2 = g_furnitureMan.GetFurniture(
        2,
        CGeneral::GetRandomNumber() < 0x3FFF ? 7 : 9,
        -1,
        m_box->m_status
    );

    return PlaceObject(1, furniture2, x2, y2, 0.5f, (float)orientation * 90.0f);
}

// 0x597430
CObject* Interior_c::Lounge_AddHifi(int32 a2, int32 a3, int32 a4, int32 a5) {
    float offsetX = (float)a3;
    float offsetY = (float)a4;

    if (a2 == 0 || a2 == 2) {
        offsetX += 0.5f;
    } else if (a2 == 1 || a2 == 3) {
        offsetY += 0.5f;
    }

    auto* const furniture = g_furnitureMan.GetFurniture(2, 8, -1, m_box->m_status);
    return PlaceObject(1, furniture, offsetX + 0.5f, offsetY + 0.5f, 0.5f, (float)(a2 & 3) * 90.0f);
}

// 0x5974E0
void Interior_c::Lounge_AddChairInfo(int32 a2, int32 a3, CEntity* entityIgnoredCollision) {
    const int32 direction = (a2 - 2) & 3;

    switch (a2) {
    case 0:
        AddInteriorInfo(1, (float)a3 + 0.5f, (float)(m_box->m_depth - 1) - 1.0f, direction, entityIgnoredCollision);
        break;
    case 2:
    case 1:
        AddInteriorInfo(1, (float)a3 + 0.5f, 1.0f, direction, entityIgnoredCollision);
        break;
    case 3:
        AddInteriorInfo(1, (float)(m_box->m_width - 1) - 1.0f, (float)a3 + 0.5f, direction, entityIgnoredCollision);
        break;
    default:
        break;
    }
}

// 0x5975C0
void Interior_c::Lounge_AddSofaInfo(int32 sitType, int32 offsetX, CEntity* entityIgnoredCollision) {
    const int32 direction = (sitType - 2) & 3;
    const float x         = (float)offsetX;

    switch (sitType) {
    case 0: {
        const float y = (float)(m_box->m_depth - 1) - 1.0f;
        AddInteriorInfo(1, x + 0.5f, y, direction, entityIgnoredCollision);
        AddInteriorInfo(1, x + 1.5f, y, direction, entityIgnoredCollision);
        break;
    }
    case 2:
    case 1:
        AddInteriorInfo(1, x + 0.5f, 1.0f, direction, entityIgnoredCollision);
        AddInteriorInfo(1, x + 1.5f, 1.0f, direction, entityIgnoredCollision);
        break;
    case 3: {
        const float y = (float)(m_box->m_width - 1) - 1.0f;
        AddInteriorInfo(1, x + 0.5f, y, direction, entityIgnoredCollision);
        AddInteriorInfo(1, x + 1.5f, y, direction, entityIgnoredCollision);
        break;
    }
    default:
        break;
    }
}

// 0x597740
// TODO(re): Not decoded - ~0x1AD0 bytes (0x597740-0x599210) placing sofa/chairs/TV/hifi/table
//           through the `Lounge_*` helpers; needs a dedicated pass.
void Interior_c::FurnishLounge() {
    plugin::CallMethod<0x597740, Interior_c*>(this);
}

//! Defined below - Declared here so the shop/office furnishing code can use it
namespace { int32 RandomScaled(float scale); }

// 0x599210
int32 Interior_c::Office_PlaceEdgeFillers(int32 arg0, int32 a2, int32 a3, int32 a6, int32) {
    //! Whether the door furniture (Dedicated to this function, See `GetFurniture(1, 2, ...)`) was already placed
    static auto& s_DoorPlaced = StaticRef<int8>(0xBB3DC8); // TODO(re): Name this global properly

    const int32 chance = RandomScaled(100.0f); // `rand() * (1/32768) * 100`

    if (GetNumEmptyTiles(a2, a3, (a6 == 0 || a6 == 2) ? 1 : 2, 1) < 1) {
        return 1;
    }

    int32 x = a2,
          y = a3;
    switch (a6) {
    case 2: y += 1; break;
    case 0: y -= 1; break;
    case 3: x -= 1; break;
    case 1: x += 1; break;
    default: break;
    }

    const int32 dir = (a6 - 2) & 3;

    Furniture_c* furniture = nullptr;
    if (arg0 == -1) {
        if (chance > 90 && !s_DoorPlaced) {
            furniture = g_furnitureMan.GetFurniture(1, 2, -1, m_box->m_status);
            if (furniture) {
                AddInteriorInfo(7, (float)x, (float)y, dir, nullptr);
            }
            s_DoorPlaced = true;
        } else if (chance > 75) {
            a6        = CGeneral::GetRandomNumberInRange(0, 5); // Orientation
            furniture = g_furnitureMan.GetFurniture(8, 0, -1, m_box->m_status);
        } else if (chance > 25) {
            furniture = g_furnitureMan.GetFurniture(1, 3, -1, m_box->m_status);
            if (furniture) {
                AddInteriorInfo(7, (float)x, (float)y, dir, nullptr);
            }
        } else {
            return 1;
        }
    } else {
        furniture = g_furnitureMan.GetFurniture(1, arg0, -1, m_box->m_status);
    }

    int32 extentX, extentY;
    PlaceFurniture(furniture, a2, a3, 0.0f, 1, a6, &extentX, &extentY, 0);

    return extentX; //! Tile extent along X - Retail returns this (Can be `2` for wide furniture)
}

// 0x5993E0
int32 Interior_c::Office_PlaceDesk(int32 a3, int32 arg4, int32 offsetY, int32 a5, uint8 a6, int32 b) {
    const int32 dir = offsetY;

    int32 x = a3,
          y = arg4;
    switch (dir) {
    case 2: y += 1; break;
    case 1: x += 1; break;
    default: break;
    }

    auto* const desk = g_furnitureMan.GetFurniture(1, 0, b, m_box->m_status);

    int32 extentX, extentY;
    if (!PlaceFurniture(desk, x, y, 0.0f, 1, dir, &extentX, &extentY, 0)) {
        return 1;
    }

    // The tiles the second (quad) piece occupies (Retail keeps the pre-adjustment values for some cases)
    int32 tileX = x,
          tileY = y;
    switch (dir) {
    case 2:
        x += 1;
        break;
    case 0:
        tileX = x + 1;
        tileY = ++y;
        break;
    case 3:
        tileX = ++x;
        y += 1;
        break;
    case 1:
        tileY = y + 1;
        break;
    default:
        break;
    }

    float fx = (float)x,
          fy = (float)y;
    switch (dir) {
    case 2: fy += 0.5f; break;
    case 0: fy -= 0.5f; break;
    case 3: fx -= 0.5f; break;
    case 1: fx += 0.5f; break;
    default: break;
    }

    auto* const chair = g_furnitureMan.GetFurniture(1, 1, m_furnitureId, m_box->m_status);
    auto* const chairEntity = PlaceFurniture(chair, x, y, 0.0f, 1, dir, &extentX, &extentY, 1);

    AddInteriorInfo(6, fx, fy, (dir - 2) & 3, chairEntity);
    SetTilesStatus(tileX, tileY, 1, 1, 2, 1);

    return 2;
}

// 0x5995B0
int32 Interior_c::Office_PlaceEdgeDesks(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6) {
    const int32 rnd       = RandomScaled(100.0f);
    const int32 threshold = 30 - RandomScaled(-40.0f); // `0x85A708` == -40.0f

    const int32 numTiles = GetNumEmptyTiles(a3, a4, (a5 == 0 || a5 == 2) ? 1 : 2, 1);
    if (numTiles <= 1) {
        return 1;
    }

    int32 numDesks = numTiles / 2;
    if (numDesks >= 2 - RandomScaled(-2.0f)) {
        numDesks = 2 - RandomScaled(-2.0f);
    }

    if (rnd > threshold) {
        return 1;
    }

    const int32 dir = (a5 - 2) & 3;

    int32 placed = 0;
    for (; numDesks > 0; numDesks--) {
        switch (a6) {
        case 0: placed += Office_PlaceDesk(a3 + placed, a4 - 1,       dir, 70, 0, m_furnitureId); break;
        case 1: placed += Office_PlaceDesk(a3,          a4 + placed,  dir, 70, 0, m_furnitureId); break;
        case 2: placed += Office_PlaceDesk(a3 + placed, a4,           dir, 70, 0, m_furnitureId); break;
        case 3: placed += Office_PlaceDesk(a3 - 1,      a4 + placed,  dir, 70, 0, m_furnitureId); break;
        default: break;
        }
    }

    return placed + 1;
}

// 0x599770
void Interior_c::Office_FurnishEdges() {
    const int32 width3 = m_box->m_width - 3;
    const int32 depth3 = m_box->m_depth - 3;

    for (int32 i = 2; i <= width3; i++) {
        SetTilesStatus(i, depth3, 1, 1, 3, 0);
        SetTilesStatus(i, 2,      1, 1, 3, 0);
    }
    for (int32 i = 2; i <= depth3; i++) {
        SetTilesStatus(2,      i, 1, 1, 3, 0);
        SetTilesStatus(width3, i, 1, 1, 3, 0);
    }

    AddGotoPt(2,      2,       0.5f,  0.5f);
    AddGotoPt(2,      depth3,  0.5f, -0.5f);
    AddGotoPt(width3, 2,      -0.5f,  0.5f);
    AddGotoPt(width3, depth3, -0.5f, -0.5f);

    SetTilesStatus(m_box->m_door - 2, 0, 4, 2, 7, 0);

    const int32 width = m_box->m_width - 1;
    const int32 depth = m_box->m_depth - 1;

    for (int32 i = 1; i < width; i += Office_PlaceEdgeDesks(-1, i, 0, 2, 2)) {}
    for (int32 i = 1; i < width; i += Office_PlaceEdgeDesks(-1, i, depth, 0, 0)) {}
    for (int32 i = 1; i <= depth - 2; i += Office_PlaceEdgeDesks(-1, 0, i, 1, 1)) {}
    for (int32 i = 1; i <= depth - 2; i += Office_PlaceEdgeDesks(-1, width, i, 3, 3)) {}

    for (int32 i = 1; i < width; i += Office_PlaceEdgeFillers(-1, i, 0, 2, 2)) {}
    for (int32 i = 1; i < width; i += Office_PlaceEdgeFillers(-1, i, depth, 0, 0)) {}
    for (int32 i = 1; i <= depth - 2; i += Office_PlaceEdgeFillers(-1, 0, i, 1, 1)) {}
    for (int32 i = 1; i <= depth - 2; i += Office_PlaceEdgeFillers(-1, width, i, 3, 3)) {}
}

// 0x599960
int32 Interior_c::Office_PlaceDeskQuad(int32 a2, int32 a3, int32 a4, int32 a5) {
    Office_PlaceDesk(a3,     a4 - 2, 2, 70, 0, a5);
    Office_PlaceDesk(a3,     a4,     0, 70, 0, a5);
    Office_PlaceDesk(a3 - 2, a4,     0, 70, 0, a5);
    Office_PlaceDesk(a3 - 2, a4 - 2, 2, 70, 0, a5);

    SetTilesStatus(a3 - 3, a4 - 3, 6, 1, 3, 0);
    SetTilesStatus(a3 - 3, a4 + 2, 6, 1, 3, 0);
    SetTilesStatus(a3 - 3, a4 - 2, 1, 4, 3, 0);
    SetTilesStatus(a3 + 2, a4 - 2, 1, 4, 3, 0);

    return 6;
}

// 0x599A30
int32 Interior_c::Office_FurnishCenter() {
    const int32 width  = m_box->m_width - 6;
    const int32 depth  = m_box->m_depth - 6;
    const int32 numX   = width / 6;
    const int32 numY   = depth / 6;
    const int32 offsetX = (width - numX * 6) / 2 + 6;
    const int32 offsetY = (depth - numY * 6) / 2 + 6;

    if (width <= 0 || depth <= 0 || numX <= 0 || numY <= 0) {
        return 0; // NOTE: Retail's return value here is unspecified (All callers ignore it)
    }

    for (int32 i = 0, x = offsetX; i < numX; i++, x += 6) {
        for (int32 j = 0, y = offsetY; j < numY; j++, y += 6) {
            Office_PlaceDeskQuad(-1, x, y, m_furnitureId);
        }
    }

    return 0; // NOTE: Retail's return value here is unspecified (All callers ignore it)
}

// 0x599AF0
void Interior_c::FurnishOffice() {
    SetTilesStatus(0,                   0,                   2, 2, 2, 0);
    SetTilesStatus(0,                   m_box->m_depth - 2,  2, 2, 2, 0);
    SetTilesStatus(m_box->m_width - 2,  0,                   2, 2, 2, 0);
    SetTilesStatus(m_box->m_width - 2,  m_box->m_depth - 2,  2, 2, 2, 0);

    m_furnitureId = g_furnitureMan.GetRandomId(1, 0, m_box->m_status);
    field_792     = g_furnitureMan.GetRandomId(1, 1, m_box->m_status);

    Office_FurnishEdges();
    Office_FurnishCenter();
    Shop_FurnishCeiling(); // Tail call in retail
}

// 0x599BB0
int8 Interior_c::Shop_Place3PieceUnit(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6) {
    const auto status = m_box->m_status;

    Furniture_c* first;
    Furniture_c* second;
    if (a5 == 2 || a5 == 3) {
        first  = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2 + 1, -1, status);
        second = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2, first->m_nId, status);
    } else {
        first  = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2, -1, status);
        second = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2 + 1, first->m_nId, status);
    }
    auto* const third = g_furnitureMan.GetFurniture(m_furnitureGroupId, a2 + 2, first->m_nId, status);

    int32 extentX, extentY; // Outputs (Unused here)
    if (a5 == 0 || a5 == 2) { // Along the X axis
        PlaceFurniture(first, a3, a4, 0.0f, 1, a5, &extentX, &extentY, 0);

        int32 x = a3 + a5;
        for (int32 i = 0, numPieces = a6 - 2; i < numPieces; i++) {
            PlaceFurniture(third, x, a4, 0.0f, 1, a5, &extentX, &extentY, 0);
            x += a5;
        }

        PlaceFurniture(second, x, a4, 0.0f, 1, a5, &extentX, &extentY, 0);
    } else { // Along the Y axis
        PlaceFurniture(first, a3, a4, 0.0f, 1, a5, &extentX, &extentY, 0);

        int32 y = a4 + a2;
        for (int32 i = 0, numPieces = a6 - 2; i < numPieces; i++) {
            PlaceFurniture(third, a3, y, 0.0f, 1, a5, &extentX, &extentY, 0);
            y += a2;
        }

        PlaceFurniture(second, a3, y, 0.0f, 1, a5, &extentX, &extentY, 0);
    }

    return 1;
}

// 0x599DC0
int32 Interior_c::Shop_PlaceEdgeUnits(int32 a2, int32 a3, int32 a4, int32 a5) {
    //! Never written anywhere in retail (Always `0`)
    static auto& s_ShopStyle = StaticRef<int32>(0xBB3DE4); // TODO(re): Name

    const int32 numTiles = GetNumEmptyTiles(a3, a4, (a5 == 0 || a5 == 2) ? 1 : 2, 1);
    if (numTiles <= 1) {
        return 1;
    }

    int32 numPieces = 2 - RandomScaled(-3.0f); // `0x859024` == -3.0f
    if (numTiles == 3) {
        numPieces = numTiles;
    } else {
        const int32 diff = numTiles - numPieces;
        if (diff < 0) {
            numPieces = numTiles;
        } else if (diff == 1) {
            numPieces--;
        }
    }

    if (a2 != -1) {
        Shop_Place3PieceUnit(a2, a3, a4, a5, numPieces);
    } else if (s_ShopStyle > 50) {
        Shop_Place3PieceUnit(0, a3, a4, a5, numPieces);
    } else if (s_ShopStyle > 25) {
        Shop_Place3PieceUnit(3, a3, a4, a5, numPieces);
    } else if (s_ShopStyle > 10) {
        Shop_Place3PieceUnit(6, a3, a4, a5, numPieces);
    } else {
        Shop_Place3PieceUnit(9, a3, a4, a5, numPieces);
    }

    return numPieces;
}

// 0x599EF0
int32 Interior_c::Shop_PlaceCounter(uint8 a2) {
    auto* const counterA = g_furnitureMan.GetFurniture(0, 12, -1, m_box->m_status);
    auto* const counterB = g_furnitureMan.GetFurniture(0, 13, -1, m_box->m_status);

    const int32 orientation = RandomScaled(4.0f); // `0x858B90` == 4.0f

    int32 extentX, extentY;
    if (a2) {
        PlaceFurniture(counterA, m_box->m_door - 5, 1, 0.0f, 1, 0, &extentX, &extentY, 0);
        SetTilesStatus(m_box->m_door - 6, 0, extentX + 1, 1, 2, 0);
        PlaceFurniture(counterB, m_box->m_door + 1, 0, 0.0f, 1, orientation, &extentX, &extentY, 1);
        return m_box->m_door - 3;
    } else {
        PlaceFurniture(counterA, m_box->m_door + 2, 1, 0.0f, 1, 0, &extentX, &extentY, 0);
        SetTilesStatus(m_box->m_door + 2, 0, extentX + 1, 1, 2, 0);
        PlaceFurniture(counterB, m_box->m_door - 2, 0, 0.0f, 1, orientation, &extentX, &extentY, 1);
        return m_box->m_door + 4;
    }
}

// 0x59A030
void Interior_c::Shop_PlaceFixedUnits() {
    if (m_box->m_door == -1) {
        return;
    }

    const int32 door = m_box->m_door;
    SetTilesStatus(door - 1, 0, 2, 1, 7, 0);

    const int32 width = m_box->m_width - door - 2;
    const int32 depth = door - 2;

    int32 x = 0; //! NOTE: Retail reads this uninitialized when neither branch runs - We zero it
    if (width >= 6) {
        if (depth >= 6) {
            x = Shop_PlaceCounter(CGeneral::GetRandomNumber() < 0x3FFF);
        } else {
            x = Shop_PlaceCounter(false);
        }
    } else if (depth >= 6) {
        x = Shop_PlaceCounter(true);
    }

    AddInteriorInfo(9,  (float)x, 2.0f, 0, nullptr);
    AddInteriorInfo(10, (float)x, 0.0f, 2, nullptr);
}

// 0x59A130
void Interior_c::Shop_FurnishCeiling() {
    // NOTE: Empty in retail
}

// 0x59A140
void Interior_c::Shop_AddShelfInfo(int32 a2, int32 a3, int32 a5) {
    //! Shared with `Shop_FurnishEdges` (Retail: `0x8D0948`)
    static auto& s_ShelfCounter = StaticRef<int32>(0x8D0948); // TODO(re): Name

    if (s_ShelfCounter > 1 && RandomScaled(100.0f) > 60.0f) {
        AddInteriorInfo(8, (float)a2, (float)a3, a5, nullptr);
        s_ShelfCounter = 1; // Retail: `= 0` followed by `++`
        return;
    }

    s_ShelfCounter++;
}

// 0x59A1B0
void Interior_c::Shop_FurnishEdges() {
    //! Shared with `Shop_AddShelfInfo` (Retail: `0x8D0948`)
    static auto& s_ShelfCounter = StaticRef<int32>(0x8D0948); // TODO(re): Name

    //! Retail inlines this 4 times (Same code as the helper at `0x59A0E0`)
    const auto RandomShopStyle = [] {
        const int32 rnd = RandomScaled(100.0f);
        return rnd > 50 ? 0 : rnd > 25 ? 3 : rnd <= 10 ? 9 : 6;
    };

    const int32 width = m_box->m_width;
    const int32 depth = m_box->m_depth;

    for (int32 x = 1; x < width - 1; x += Shop_PlaceEdgeUnits(RandomShopStyle(), x, depth - 1, 0)) {}

    for (int32 y = 1; y <= depth - 2; y += Shop_PlaceEdgeUnits(RandomShopStyle(), 0, y, 1)) {}
    for (int32 y = 1; y <= depth - 2; y += Shop_PlaceEdgeUnits(RandomShopStyle(), width - 1, y, 3)) {}

    // Shelves along the top wall
    for (int32 x = 1; x <= width - 2; x++) {
        if (m_box->m_tDoorStart == -1 || x < m_box->m_tDoorStart || x > m_box->m_tDoorEnd) {
            if (s_ShelfCounter > 1 && RandomScaled(100.0f) > 60.0f) {
                AddInteriorInfo(8, (float)x, (float)(depth - 2), 2, nullptr);
                s_ShelfCounter = 0;
            }
            s_ShelfCounter++;
        }
        if (GetTileStatus(x, 1) == 0) {
            SetTilesStatus(x, 1, 1, 1, 2, 0);
        }
    }

    // Shelves along the left/right walls
    for (int32 y = 2; y <= depth - 3; y++) {
        if (m_box->m_lDoorStart == -1 || y < m_box->m_lDoorStart || y > m_box->m_lDoorEnd) {
            if (s_ShelfCounter > 1 && RandomScaled(100.0f) > 60.0f) {
                AddInteriorInfo(8, 1.0f, (float)y, 3, nullptr);
                s_ShelfCounter = 0;
            }
            s_ShelfCounter++;
        }
        if (m_box->m_rDoorStart == -1 || y < m_box->m_rDoorStart || y > m_box->m_rDoorEnd) {
            if (s_ShelfCounter > 1 && RandomScaled(100.0f) > 60.0f) {
                AddInteriorInfo(8, (float)(width - 2), (float)y, 1, nullptr);
                s_ShelfCounter = 0;
            }
            s_ShelfCounter++;
        }
    }

    // Remaining floor tiles
    for (int32 x = 2; x <= width - 3; x++) {
        if (GetTileStatus(x, 2) == 0) {
            SetTilesStatus(x, 2, 1, 1, 2, 0);
        }
        SetTilesStatus(x, depth - 3, 1, 1, 3, 0);
    }
    for (int32 y = 3; y <= depth - 4; y++) {
        SetTilesStatus(2,          y, 1, 1, 3, 0);
        SetTilesStatus(width - 3,  y, 1, 1, 3, 0);
    }
    SetTilesStatus(3, 3, width - 6, 1, 3, 0);
}

// 0x593DB0
bool Interior_c::GetBoundingBox(FurnitureEntity_c* entity, CVector* a3) {
    switch (m_box->m_type) {
    case 0:
    case 1:
    case 6:
        break;
    default:
        return false;
    }

    static auto& s_PedNominalRadius = StaticRef<float>(0x8D22B0); // `CPedGeometryAnalyser::ms_fPedNominalRadius`

    const int32 tileX = entity->m_tileX;
    const int32 tileY = entity->m_tileY;

    std::array<int32, 30 * 30> grid{};
    grid[tileX * 30 + tileY] = 1;

    int32 minX = tileX, maxX = tileX;
    int32 minY = tileY, maxY = tileY;
    FindBoundingBox(tileX, tileY, &minX, &maxX, &minY, &maxY, grid.data());

    const float radius = s_PedNominalRadius;
    GetTileCentre((float)minX - 0.5f - radius, (float)maxY + radius + 0.5f, &a3[0]);
    GetTileCentre((float)minX - 0.5f - radius, (float)minY - 0.5f - radius, &a3[1]);
    GetTileCentre((float)maxX + radius + 0.5f, (float)minY - 0.5f - radius, &a3[2]);
    GetTileCentre((float)maxX + radius + 0.5f, (float)maxY + radius + 0.5f, &a3[3]);

    return true;
}

// 0x593910
void Interior_c::ResetTiles() {
    const auto* const b = m_box;

    std::memset(field_68, 0, sizeof(field_68)); // 30 * 30 tile statuses

    const int32 width = b->m_width;
    const int32 depth = b->m_depth;

    //! Marks `[start, end)` along the `x` column (`y` varies)
    const auto MarkVertical = [this, width, depth](int32 start, int32 end, int32 x, char value) {
        if (start == -1 || width <= 0) {
            return;
        }
        const int32 len = end - start;
        if (len <= 0 || start < 0 || x < 0 || start + len > depth) {
            return;
        }
        char* const first = &field_68[x * 30 + start];
        for (int32 i = 0; i < len; i++) {
            if (!first[i]) {
                first[i] = value;
            }
        }
    };

    //! Marks `[start, end)` along the `y` row (`x` varies)
    const auto MarkHorizontal = [this, width, depth](int32 start, int32 end, int32 y, char value) {
        if (start == -1 || depth <= 0) {
            return;
        }
        const int32 len = end - start;
        if (len <= 0 || start < 0 || y < 0 || start + len > width) {
            return;
        }
        char* first = &field_68[start * 30 + y];
        for (int32 i = 0; i < len; i++, first += 30) {
            if (!*first) {
                *first = value;
            }
        }
    };

    MarkVertical(b->m_lDoorStart, b->m_lDoorEnd, 0, 8);         // Left door
    MarkVertical(b->m_rDoorStart, b->m_rDoorEnd, width - 1, 8); // Right door
    MarkHorizontal(b->m_tDoorStart, b->m_tDoorEnd, depth - 1, 8); // Top door

    MarkVertical(b->m_lWindowStart, b->m_lWindowEnd, 0, 9);         // Left window
    MarkVertical(b->m_rWindowStart, b->m_rWindowEnd, width - 1, 9); // Right window
    MarkHorizontal(b->m_tWindowStart, b->m_tWindowEnd, depth - 1, 9); // Top window

    for (int32 i = 0; i < 3; i++) {
        const int32 left   = b->m_noGoLeft[i];
        const int32 bottom = b->m_noGoBottom[i];
        if (left == -1 || bottom == -1) {
            continue;
        }
        const int32 noGoWidth = b->m_noGoWidth[i];
        const int32 noGoDepth = b->m_noGoDepth[i];
        if (left < 0 || bottom < 0) {
            continue;
        }
        if (left + noGoWidth > width) {
            continue;
        }
        if (bottom + noGoDepth > depth) {
            continue;
        }
        if (noGoWidth <= 0) {
            continue;
        }
        char* row = &field_68[left * 30 + bottom];
        for (int32 x = 0; x < noGoWidth; x++, row += 30) {
            for (int32 y = 0; y < noGoDepth; y++) {
                if (!row[y]) {
                    row[y] = 0xB;
                }
            }
        }
    }
}

// 0x5934E0
CObject* Interior_c::PlaceObject(uint8 isStealable, Furniture_c* furniture, float offsetX, float offsetY, float offsetZ, float rotationZ) {
    //! Pool of recyclable furniture entities (Only referenced by `Interior_c`)
    static auto& s_FurnitureEntityPool = StaticRef<TList_c<FurnitureEntity_c>>(0xBAD3EC);

    //! `g_interiorMan.m_Objects` / `m_ObjectCount` (Not writable through the manager's API)
    static auto& s_ObjectCount = StaticRef<int32>(0xBB3A18);
    static auto& s_Objects     = StaticRef<std::array<InteriorObject, 32>>(0xBB3A1C);

    const auto* const modelInfo = CModelInfo::GetModelInfo((int32)furniture->m_nModelId);
    const auto&       bbMin     = modelInfo->GetColModel()->m_boundBox.m_vecMin;

    const CVector pos{
        offsetX - (float)m_box->m_width  * 0.5f,
        offsetY - (float)m_box->m_depth  * 0.5f,
        offsetZ - (float)m_box->m_height * 0.5f - bbMin.z
    };

    if (s_FurnitureEntityPool.GetNumItems() <= 0) {
        return nullptr;
    }

    // Object matrix = interior matrix * (unity rotated by `rotationZ` and translated by `pos`)
    CMatrix localMat{};
    localMat.SetUnity();
    localMat.RotateZ(DEG_TO_RAD * rotationZ);
    localMat.GetPosition() = pos;

    CMatrix objectMat = CMatrix{ &m_matrix, false } * localMat;

    auto* const furnitureEntity = s_FurnitureEntityPool.RemoveHead();
    if (!furnitureEntity) {
        return nullptr;
    }

    auto* const object = new CObject(furniture->m_nModelId, false);
    furnitureEntity->m_entity = object;

    object->SetMatrix(objectMat);
    object->SetAreaCode(static_cast<eAreaCodes>(m_areaCode));
    object->m_nFlags         |= 0x10000; // See `CEntity`'s flags union
    object->m_nObjectType     = OBJECT_TYPE_DECORATION;
    object->SetIsStatic(true);           // Virtual call in retail (`vtable[0x10]`)
    CWorld::Add(object);

    furnitureEntity->m_tileX = (uint16)offsetX;
    furnitureEntity->m_tileY = (uint16)offsetY;

    reinterpret_cast<TList_c<FurnitureEntity_c>*>(&m_list)->AddItem(furnitureEntity);

    if (isStealable) {
        object->objectFlags.bIsLiftable = true;

        if (!g_interiorMan.HasInteriorHadStealDataSetup(this)) { // First time an object of this interior is placed - Store its data so it can be restored later
            auto& info = s_Objects[s_ObjectCount];
            info.entity     = object;
            info.modelId    = furniture->m_nModelId;
            info.interiorId = m_interiorId;
            info.pos        = pos;
            info.wasStolen  = false;
            ++s_ObjectCount;
        } else {
            int32 idx = g_interiorMan.FindStealableObjectId(m_interiorId, furniture->m_nModelId, pos);
            if (idx >= 0 && s_Objects[idx].wasStolen) {
                // It was already stolen, so don't add it (Retail deletes the object and recycles the entity)
                CWorld::Remove(object);
                delete object;

                furnitureEntity->m_entity = nullptr;
                reinterpret_cast<TList_c<FurnitureEntity_c>*>(&m_list)->RemoveItem(furnitureEntity);
                s_FurnitureEntityPool.AddItem(furnitureEntity);
                return nullptr;
            }

            idx = g_interiorMan.FindStealableObjectId(m_interiorId, furniture->m_nModelId, pos);
            if (idx >= 0) {
                s_Objects[idx].entity = object;
            }
        }
    }

    return object;
}

namespace {

//! The interior's tiles are a `30x30` grid (Stored in `Interior_c::field_68`), indexed as `x * 30 + y`
constexpr int32 TILE_GRID_WIDTH = 30;

/*!
* @return `(int32)(rand() * (1 / 32768) * scale)` - Retail's `fild`/`fmul`/`ftol` sequence using the `1 / 32768` constant.
* @note The result is truncated towards zero (Meaning it can be negative if `scale` is).
*/
int32 RandomScaled(const float scale) {
    return static_cast<int32>(static_cast<float>(CGeneral::GetRandomNumber()) * (1.0f / 32768.0f) * scale);
}

/*!
* @brief Get the list of the interior's furniture entities
* @note `m_list` is still declared as a `TList_c<int>` (See `Interior_c.h`), but retail uses it as a list of `FurnitureEntity_c` (Same as `PlaceObject`)
*/
TList_c<FurnitureEntity_c>& GetFurnitureEntities(Interior_c& interior) {
    return *reinterpret_cast<TList_c<FurnitureEntity_c>*>(&interior.m_list);
}

} // namespace

// 0x5913B0
FurnitureEntity_c* Interior_c::GetFurnitureEntity(const CEntity& entity) {
    auto&      list = GetFurnitureEntities(*this);
    for (auto* furniture = list.GetHead(); furniture; furniture = list.GetNext(furniture)) {
        if (furniture->m_entity == &entity) {
            return furniture;
        }
    }
    return nullptr;
}

// 0x5913E0
bool Interior_c::IsPtInside(const CVector& pt, CVector bias) {
    const CVector relPos = pt - m_position;

    // NOTE: The box's extents (`m_width`/`m_depth`/`m_height`) are full sizes (Not half ones)
    if (std::fabs(m_matrix.right.x * relPos.x + m_matrix.right.y * relPos.y + m_matrix.right.z * relPos.z) > (float)m_box->m_width * 0.5f + bias.x) {
        return false;
    }
    if (std::fabs(m_matrix.up.x * relPos.x + m_matrix.up.y * relPos.y + m_matrix.up.z * relPos.z) > (float)m_box->m_depth * 0.5f + bias.y) {
        return false;
    }
    return std::fabs(m_matrix.at.x * relPos.x + m_matrix.at.y * relPos.y + m_matrix.at.z * relPos.z) <= (float)m_box->m_height * 0.5f + bias.z;
}

// 0x5914D0
void Interior_c::CalcMatrix(CVector* translation) {
    m_matrix.right = { 1.0f, 0.0f, 0.0f };
    m_matrix.up    = { 0.0f, 1.0f, 0.0f };
    m_matrix.at    = { 0.0f, 0.0f, 1.0f };
    m_matrix.pos   = { 0.0f, 0.0f, 0.0f };
    m_matrix.flags |= static_cast<RwUInt32>(rwMATRIXTYPEORTHONORMAL) | static_cast<RwUInt32>(rwMATRIXINTERNALIDENTITY);

    const CVector zAxis{ 0.0f, 0.0f, 1.0f };
    RwMatrixRotate(&m_matrix, &zAxis, m_box->m_rot, rwCOMBINEREPLACE);
    RwMatrixTranslate(&m_matrix, translation, rwCOMBINEPOSTCONCAT);

    // The interior's matrix is relative to its group's entity's frame
    if (auto* const entity = m_pGroup->GetEntity()) {
        if (auto* const rwObject = entity->GetRwObject()) {
            RwMatrixMultiply(&m_matrix, &m_matrix, RwFrameGetMatrix(static_cast<RwFrame*>(rwObject->parent)));
        }
    }
}

// 0x591590
void Interior_c::Furnish() {
    switch (m_box->m_type) {
    case 1:
        FurnishOffice();
        break;
    case 2:
        FurnishLounge();
        break;
    case 3:
        FurnishBedroom();
        break;
    case 4:
        FurnishKitchen();
        break;
    default: // NOTE: Also 0
        FurnishShop(m_box->m_type);
        break;
    }
}

// 0x5915D0
void Interior_c::Unfurnish() {
    //! Pool of recyclable furniture entities (Same as the one `PlaceObject` uses)
    static auto& s_FurnitureEntityPool = StaticRef<TList_c<FurnitureEntity_c>>(0xBAD3EC);

    auto& furnitureList = GetFurnitureEntities(*this);
    for (auto* furniture = furnitureList.GetHead(); furniture;) {
        auto* const next = furniture->m_pNext;

        auto* const ped  = FindPlayerPed();
        auto* const held = ped ? ped->GetEntityThatThisPedIsHolding() : nullptr;
        if (held && held == furniture->m_entity && held->GetIsTypeObject() && held->AsObject()->objectFlags.bIsLiftable) {
            // The player is holding it right now, so let it linger instead of deleting it under him
            const auto object = held->AsObject();
            CObject::nNoTempObjects++;
            object->m_nObjectType  = OBJECT_TEMPORARY;
            object->m_nRemovalTime = CTimer::GetTimeInMS() + 99'999'999;
        } else {
            CWorld::Remove(furniture->m_entity);
            delete furniture->m_entity; // NOTE: Retail checks for null here (But deleting a null pointer is a no-op anyways)
        }

        furniture->m_entity = nullptr;
        furnitureList.RemoveItem(furniture);
        s_FurnitureEntityPool.AddItem(furniture); // Recycle it
        furniture = next;
    }
}

// 0x591680
int8 Interior_c::CheckTilesEmpty(int32 x, int32 y, int32 width, int32 height, uint8 a5) {
    if (x < 0 || y < 0) {
        return false;
    }
    if (x + width > m_box->m_width || y + height > m_box->m_depth) {
        return false;
    }

    for (auto* row = &field_68[x * TILE_GRID_WIDTH + y]; width > 0; width--, row += TILE_GRID_WIDTH) {
        for (auto i = 0; i < height; i++) {
            const auto status = static_cast<uint8>(row[i]);
            if (status == 0) {
                continue;
            }
            if (a5 == 0 || status != 9) {
                return false;
            }
        }
    }
    return true;
}

// 0x591700
void Interior_c::SetTilesStatus(int32 x, int32 y, int32 width, int32 height, int32 status, int8 a6) {
    if (x < 0 || y < 0) {
        return;
    }
    if (x + width > m_box->m_width || y + height > m_box->m_depth) {
        return;
    }

    const auto newStatus = static_cast<int8>(status);

    for (auto* row = &field_68[x * TILE_GRID_WIDTH + y]; width > 0; width--, row += TILE_GRID_WIDTH) {
        for (auto i = 0; i < height; i++) {
            auto& tile = row[i];
            if (tile == 9 && newStatus == 5) {
                tile = 10;
                continue;
            }
            if (a6) {
                if (tile == 5 || tile == 7 || tile == 8) { // Occupied, keep it
                    continue;
                }
                tile = newStatus;
            } else {
                if (tile == 3) {
                    if (newStatus == 3) {
                        return;
                    }
                    if (newStatus == 4) {
                        tile = newStatus;
                    }
                    continue;
                }
                if (tile != 0) { // Already occupied, keep it
                    continue;
                }
                tile = newStatus;
            }
        }
    }
}

// 0x5917C0
void Interior_c::SetCornerTiles(int32 corner, int32 len, int32 status, uint8 a6) {
    switch (corner) {
    case 0:
        SetTilesStatus(0, m_box->m_depth - 1, len, 1, status, a6);
        SetTilesStatus(0, m_box->m_depth - len, 1, len, status, a6);
        break;
    case 1:
        SetTilesStatus(0, 0, len, 1, status, a6);
        SetTilesStatus(0, 0, 1, len, status, a6);
        break;
    case 2:
        SetTilesStatus(m_box->m_width - len, 0, len, 1, status, a6);
        SetTilesStatus(m_box->m_width - 1, 0, 1, len, status, a6);
        break;
    case 3:
        SetTilesStatus(m_box->m_width - len, m_box->m_depth - 1, len, 1, status, a6);
        SetTilesStatus(m_box->m_width - 1, m_box->m_depth - len, 1, len, status, a6);
        break;
    }
}

// 0x5918E0
int32 Interior_c::GetTileStatus(int32 x, int32 y) {
    if (x < 0 || y < 0 || x >= m_box->m_width || y >= m_box->m_depth) {
        return 1; // NOTE: Out of bounds tiles are reported as `1`
    }
    return static_cast<uint8>(field_68[x * TILE_GRID_WIDTH + y]);
}

// 0x591920
int32 Interior_c::GetNumEmptyTiles(int32 x, int32 y, int32 dir, int32 numTiles) {
    //! `dir` selects the scan direction: `0`/`3` -> `-y`, `1`/`2` -> `+y`
    const int32 step = (dir == 0 || dir == 3) ? -1 : 1;
    int32 numRows = 0;

    if (dir == 1 || dir == 3) {
        // Scan strips of `numTiles` along `y`, moving along `x`
        for (;;) {
            for (int32 i = 0, ty = y; i < numTiles; i++, ty++) {
                if (x >= m_box->m_width || ty >= m_box->m_depth || x < 0 || ty < 0 || field_68[x * TILE_GRID_WIDTH + ty]) {
                    return numRows;
                }
            }
            numRows++;
            x += step;
        }
    } else {
        // Scan strips of `numTiles` along `x`, moving along `y`
        for (;;) {
            for (int32 i = 0, tx = x; i < numTiles; i++, tx++) {
                if (tx >= m_box->m_width || y >= m_box->m_depth || tx < 0 || y < 0 || field_68[tx * TILE_GRID_WIDTH + y]) {
                    return numRows;
                }
            }
            numRows++;
            y += step;
        }
    }
}

// 0x591B20
int32 Interior_c::GetRandomTile(int32 status, int32* outX, int32* outY) {
    int32 x, y;
    do {
        x = RandomScaled(static_cast<float>(m_box->m_width));
        y = RandomScaled(static_cast<float>(m_box->m_depth));
    } while (GetTileStatus(x, y) != status);

    *outX = x;
    *outY = y;

    return y; // NOTE: Retail returns the `y` coordinate (Most likely unused)
}

// 0x59A590
void Interior_c::Shop_FurnishAisles() {
    const int32 width    = m_box->m_width - 6;
    const int32 depth    = m_box->m_depth - 7;
    const int32 depthTop = m_box->m_depth - 3;

    if (width <= 0 || depth <= 0) {
        return;
    }

    AddGotoPt(2, 3,        -0.5f, -0.5f);
    AddGotoPt(2, depthTop, -0.5f,  0.5f);

    //! Retail inlines this 4 times (Same code as the helper at `0x59A0E0`)
    const auto RandomShopStyle = [] {
        const int32 rnd = RandomScaled(100.0f);
        return rnd > 50 ? 0 : rnd > 25 ? 3 : rnd <= 10 ? 9 : 6;
    };

    int32 x = 0;
    for (; x < width; x++) {
        const int32 style = RandomShopStyle();

        switch (x % 4) {
        case 0:
            if (x != width - 1) {
                for (int32 i = 0, y = 4; i < depth; i++) {
                    y += Shop_PlaceEdgeUnits(style, x + 3, y, 3);
                }
            }
            break;
        case 1:
            for (int32 i = 0, y = 4; i < depth; i++) {
                y += Shop_PlaceEdgeUnits(style, x + 3, y, 1);
            }
            break;
        case 2:
            for (int32 i = 0; i < depth; i++) {
                Shop_AddShelfInfo(x + 3, i + 4, 3);
            }
            break;
        case 3:
            SetTilesStatus(x + 3, 4, 1, depth, 3, 0);
            AddGotoPt(x + 3, 3,        -0.5f, -0.5f);
            AddGotoPt(x + 3, depthTop, -0.5f,  0.5f);
            break;
        default:
            break;
        }
    }

    AddGotoPt(x + 3, 3,        0.5f, -0.5f);
    AddGotoPt(x + 3, depthTop, 0.5f,  0.5f);
}

// 0x591BD0
CVector* Interior_c::GetTileCentre(float offsetX, float offsetY, CVector* pointsIn) {
    pointsIn->x = offsetX + 0.5f - (float)m_box->m_width  * 0.5f;
    pointsIn->y = offsetY + 0.5f - (float)m_box->m_depth  * 0.5f;
    pointsIn->z = -(float)m_box->m_height * 0.5f;

    RwV3dTransformPoints(pointsIn, pointsIn, 1, &m_matrix);

    return pointsIn;
}

// 0x591D20
void Interior_c::AddGotoPt(int32 tileX, int32 tileY, float offsetX, float offsetY) {
    if (m_numGotoPoints >= 16) {
        return;
    }

    // NOTE: Tiles of status `3` (Or `7` if not in bounds - `GetTileStatus` returns `1` there) are the valid ones
    const bool inBounds = tileX >= 0 && tileY >= 0 && tileX < m_box->m_width && tileY < m_box->m_depth;
    if (!(inBounds && field_68[tileX * TILE_GRID_WIDTH + tileY] == 3) && GetTileStatus(tileX, tileY) != 7) {
        return;
    }

    CVector centre;
    GetTileCentre((float)tileX + offsetX, (float)tileY + offsetY, &centre);

    auto& gotoPoint    = m_gotoPoints[m_numGotoPoints];
    gotoPoint.m_tileX  = static_cast<int8>(tileX);
    gotoPoint.m_tileY  = static_cast<int8>(tileY);
    gotoPoint.m_pos    = centre;

    // Mark the tile as a goto point
    if (inBounds) {
        auto& tile = field_68[tileX * TILE_GRID_WIDTH + tileY];
        if (tile == 3 || tile == 0) {
            tile = 4;
        }
    }

    m_numGotoPoints++;
}

// 0x591E40
bool Interior_c::AddInteriorInfo(int32 actionType, float offsetX, float offsetY, int32 direction, CEntity* entityIgnoredCollision) {
    if (m_interiorInfosCount >= 16) {
        return false;
    }

    CVector pos;
    GetTileCentre(offsetX, offsetY, &pos);
    pos.z += 0.8f;

    CVector dir{ 0.0f, 0.0f, 0.0f };
    switch (direction) {
    case 3: dir.x = -1.0f; break;
    case 1: dir.x =  1.0f; break;
    case 2: dir.y =  1.0f; break;
    case 0: dir.y = -1.0f; break;
    default: break; // NOTE: `-1` means no direction at all (The vector stays zero)
    }
    if (direction != -1) { // NOTE: `-1` also skips the transform
        RwV3dTransformVectors(&dir, &dir, 1, &m_matrix);
    }

    auto& info = m_interiorInfos[m_interiorInfosCount];
    info.Type                   = static_cast<eInteriorInfoType>(actionType);
    info.IsInUse                = false;
    info.Pos                    = pos;
    info.Dir                    = dir;
    info.EntityIgnoredCollision = entityIgnoredCollision;

    m_interiorInfosCount++;

    return true;
}

// 0x591F90
void Interior_c::AddPickups() {
    //! Time at which a pickup was last added (Retail: Global at `0xBB3DC4`)
    static auto& s_LastPickupTimeMs = StaticRef<uint32>(0xBB3DC4);

    if (CTimer::GetTimeInMS() - s_LastPickupTimeMs < 180'000) { // 3 minutes
        return;
    }

    for (auto attempt = 0, numAdded = 0; attempt < 100 && numAdded < 1; attempt++) {
        const auto x = RandomScaled(static_cast<float>(m_box->m_width - 1));
        const auto y = RandomScaled(static_cast<float>(m_box->m_depth - 1));
        if (x < 0 || y < 0 || x >= m_box->m_width || y >= m_box->m_depth) {
            continue;
        }

        // Only pick tiles that are either empty, a goto point, or already a placed-object one
        const auto tileStatus = static_cast<uint8>(field_68[x * TILE_GRID_WIDTH + y]);
        if (tileStatus != 0 && tileStatus != 3 && tileStatus != 4) {
            continue;
        }

        CVector pos;
        GetTileCentre((float)x, (float)y, &pos);

        if (RandomScaled(100.0f) < 75) {
            // Money
            CPickups::GenerateNewOne(pos, ModelIndices::MI_MONEY, PICKUP_MONEY, 10 - RandomScaled(-40.0f));
        } else {
            // A weapon
            pos.z += 0.5f;

            const auto rnd = RandomScaled(100.0f);
            const auto weapon = rnd < 40 ? WEAPON_BASEBALLBAT
                              : rnd < 80 ? WEAPON_PISTOL
                              : rnd < 90 ? WEAPON_KNIFE
                                         : WEAPON_SHOTGUN;

            CPickups::GenerateNewOne_WeaponType(pos, weapon, PICKUP_ONCE, 3 - RandomScaled(-15.0f), false, nullptr);
        }

        numAdded++;
    }
}

// 0x5922C0
void Interior_c::FindBoundingBox(int32 x, int32 y, int32* minX, int32* maxX, int32* minY, int32* maxY, int32* visited) {
    // NOTE: Retail tail-calls itself for `y - 1`, so we loop instead
    for (;;) {
        // (x - 1, y)
        if (x > 0 && x - 1 < m_box->m_width && y < m_box->m_depth && y >= 0) {
            const auto idx = (x - 1) * TILE_GRID_WIDTH + y;
            if (field_68[idx] == 5 && !visited[idx]) {
                visited[idx] = 1;
                if (x - 1 < *minX) {
                    *minX = x - 1;
                }
                FindBoundingBox(x - 1, y, minX, maxX, minY, maxY, visited);
            }
        }

        // (x, y + 1)
        if (y < 29 && x < m_box->m_width && y + 1 < m_box->m_depth && x >= 0) {
            const auto idx = x * TILE_GRID_WIDTH + y + 1;
            if (field_68[idx] == 5 && !visited[idx]) {
                visited[idx] = 1;
                if (y + 1 > *maxY) {
                    *maxY = y + 1;
                }
                FindBoundingBox(x, y + 1, minX, maxX, minY, maxY, visited);
            }
        }

        // (x + 1, y)
        if (x < 29 && x + 1 < m_box->m_width && y < m_box->m_depth) {
            if (y < 0) {
                return;
            }
            const auto idx = (x + 1) * TILE_GRID_WIDTH + y;
            if (field_68[idx] == 5 && !visited[idx]) {
                visited[idx] = 1;
                if (x + 1 > *maxX) {
                    *maxX = x + 1;
                }
                FindBoundingBox(x + 1, y, minX, maxX, minY, maxY, visited);
            }
        }

        // (x, y - 1)
        if (y <= 0) {
            return;
        }
        if (x >= m_box->m_width || y - 1 >= m_box->m_depth) {
            return;
        }
        const auto idx = x * TILE_GRID_WIDTH + y - 1;
        if (field_68[idx] != 5 || visited[idx]) {
            return;
        }
        visited[idx] = 1;
        if (y - 1 < *minY) {
            *minY = y - 1;
        }
        y--;
    }
}

// 0x5924A0
void Interior_c::CalcExitPts() {
    const auto  box        = m_box;
    const int32 numGotoPts = m_numGotoPoints;

    //! Main door (Node 0)
    if (box->m_door > 0) {
        auto&       node = m_doorPathNodes[0];
        const float f    = (float)box->m_door - 0.5f;

        if (numGotoPts > 2) {
            int32 i = 0;
            while (i < numGotoPts && (float)m_gotoPoints[i].m_tileX <= f) {
                i += 2;
            }
            if (i == 0) {
                node.m_gotoPointA = 0;
                node.m_gotoPointB = -1;
            } else if (i == numGotoPts) {
                node.m_gotoPointA = numGotoPts - 2;
                node.m_gotoPointB = -1;
            } else {
                node.m_gotoPointA = i;
                node.m_gotoPointB = i - 2;
            }
            GetTileCentre(f, (float)m_gotoPoints[0].m_tileY - 0.25f, &node.m_posIn);
        } else {
            node.m_gotoPointA = -1;
            node.m_gotoPointB = -1;
            GetTileCentre(f, 0.0f, &node.m_posIn);
        }
        GetTileCentre(f, -0.25f, &node.m_posOut);
    }

    //! Left door (Node 1)
    if (box->m_lDoorStart > 0) {
        auto&       node = m_doorPathNodes[1];
        const float f    = (float)(box->m_lDoorEnd - box->m_lDoorStart) * 0.5f + (float)box->m_lDoorStart - 0.5f;

        if (numGotoPts > 2) {
            if ((float)m_gotoPoints[0].m_tileY > f) {
                node.m_gotoPointA = 0;
                node.m_gotoPointB = -1;
            } else if ((float)m_gotoPoints[1].m_tileY >= f) {
                node.m_gotoPointA = 1;
                node.m_gotoPointB = -1;
            } else {
                node.m_gotoPointA = 0;
                node.m_gotoPointB = 1;
            }
            GetTileCentre((float)m_gotoPoints[0].m_tileX - 0.25f, f, &node.m_posIn);
        } else {
            node.m_gotoPointA = -1;
            node.m_gotoPointB = -1;
            GetTileCentre(0.0f, f, &node.m_posIn);
        }
        GetTileCentre(-0.25f, f, &node.m_posOut);
    }

    //! Top door (Node 2)
    if (box->m_tDoorStart > 0) {
        auto&       node = m_doorPathNodes[2];
        const float f    = (float)(box->m_tDoorEnd - box->m_tDoorStart) * 0.5f + (float)box->m_tDoorStart - 0.5f;

        if (numGotoPts > 2) {
            int32 i = 1;
            while (i < numGotoPts && (float)m_gotoPoints[i].m_tileX <= f) {
                i += 2;
            }
            if (i == 0) {
                node.m_gotoPointA = 1;
                node.m_gotoPointB = -1;
            } else if (i == numGotoPts) {
                node.m_gotoPointA = numGotoPts - 1;
                node.m_gotoPointB = -1;
            } else {
                node.m_gotoPointA = i;
                node.m_gotoPointB = i - 2;
            }
            GetTileCentre(f, (float)m_gotoPoints[1].m_tileY + 0.25f, &node.m_posIn);
        } else {
            node.m_gotoPointA = -1;
            node.m_gotoPointB = -1;
            GetTileCentre(f, (float)(box->m_depth - 1), &node.m_posIn);
        }
        GetTileCentre(f, (float)(box->m_depth - 1) + 0.25f, &node.m_posOut);
    }

    //! Right door (Node 3)
    if (box->m_rDoorStart > 0) {
        auto&       node = m_doorPathNodes[3];
        const float f    = (float)(box->m_rDoorEnd - box->m_rDoorStart) * 0.5f + (float)box->m_rDoorStart - 0.5f;

        if (numGotoPts > 2) {
            if ((float)m_gotoPoints[numGotoPts - 2].m_tileY > f) {
                node.m_gotoPointA = numGotoPts - 2;
                node.m_gotoPointB = -1;
            } else if ((float)m_gotoPoints[numGotoPts - 1].m_tileY >= f) {
                node.m_gotoPointA = numGotoPts - 1;
                node.m_gotoPointB = -1;
            } else {
                node.m_gotoPointA = numGotoPts - 1;
                node.m_gotoPointB = numGotoPts - 2;
            }
            GetTileCentre(f, (float)m_gotoPoints[numGotoPts - 1].m_tileX + 0.25f, &node.m_posIn);
        } else {
            node.m_gotoPointA = -1;
            node.m_gotoPointB = -1;
            GetTileCentre(f, (float)(box->m_width - 1), &node.m_posIn);
        }
        GetTileCentre((float)(box->m_width - 1) + 0.25f, f, &node.m_posOut);
    }
}

// 0x5929F0
bool Interior_c::IsVisible() {
    //! NOTE: These are globals written at runtime by the camera code (No owning class known)
    static auto& s_CameraPosition = StaticRef<CVector>(0xB6F02C);
    static auto& s_CameraEntity   = StaticRef<CEntity*>(0xB6F03C);

    const CVector camPos = s_CameraEntity ? s_CameraEntity->GetPosition() : s_CameraPosition;

    if (IsPtInside(camPos, { 20.0f, 20.0f, 0.0f })) {
        return true;
    }
    if (m_box->m_door <= 0) {
        return false;
    }
    return CVector::DistSqr(camPos, m_position) < 100.0f; // `100.0f` is the constant at `0x858628`
}

// 0x592AA0
CEntity* Interior_c::PlaceFurniture(Furniture_c* a1, int32 a2, int32 a3, float a4, int32 a5, int32 a6, int32* a7, int32* a8, uint8 a9) {
    //! Pool of recyclable furniture entities (Only referenced by `Interior_c`)
    static auto& s_FurnitureEntityPool = StaticRef<TList_c<FurnitureEntity_c>>(0xBAD3EC);
    auto&       furnitureEntities     = *reinterpret_cast<TList_c<FurnitureEntity_c>*>(&m_list);

    //! Retail treats `a4` as a raw integer (it's passed as-is to `CheckTilesEmpty` and stored in `m_tileY`)
    const int32 a4Bits = std::bit_cast<int32>(a4);

    const int32 widthX = a1->m_nWidthX;
    const int32 widthY = a1->m_nWidthY;

    //! For the rotated orientations (1/3) the furniture's dimensions are swapped
    const bool  isRotated    = (a6 == 1 || a6 == 3);
    const int32 sizeAcrossX  = isRotated ? widthY : widthX; // L20
    const int32 sizeAlongY   = isRotated ? widthX : widthY; // L1C

    if (s_FurnitureEntityPool.GetNumItems() <= 0) {
        *a7 = 0;
        *a8 = 0;
        return nullptr;
    }

    if (a5 == 1 && a9 == 0) {
        if (!CheckTilesEmpty(a4Bits, a3, sizeAcrossX, sizeAlongY, a1->m_bIsTall)) {
            *a7 = 0;
            *a8 = 0;
            return nullptr;
        }
    }

    //! Rotation about the furniture's own centre (Only for furniture that may be rotated)
    CMatrix localRot{};
    localRot.SetUnity();
    if (a1->m_nMaxAng > 0) {
        const float centerY = 0.5f - (float)widthY * 0.5f;
        const float centerX = 0.5f - (float)widthX * 0.5f;

        CMatrix toCenter{};
        toCenter.SetUnity();
        toCenter.SetTranslate(CVector{ -centerX, -centerY, 0.0f });

        CMatrix rotation{};
        rotation.SetUnity();
        rotation.RotateZ(DEG_TO_RAD * (float)CGeneral::GetRandomNumberInRange(-(int32)a1->m_nMaxAng, (int32)a1->m_nMaxAng));

        CMatrix fromCenter{};
        fromCenter.SetUnity();
        fromCenter.SetTranslate(CVector{ centerX, centerY, 0.0f });

        localRot = (toCenter * rotation) * fromCenter;
    }

    //! Tile offset induced by the orientation
    int32 offsetAlongX = 0; // L10
    int32 offsetAlongY = 0; // L18
    switch (a6) {
    case 1:
        offsetAlongX = sizeAcrossX - 1;
        break;
    case 2:
        offsetAlongX = sizeAcrossX - 1;
        offsetAlongY = sizeAlongY - 1;
        break;
    case 3:
        offsetAlongY = sizeAlongY - 1;
        break;
    default:
        break;
    }

    CMatrix tileMatrix{};
    tileMatrix.SetUnity();
    tileMatrix.RotateZ((float)a6 * 1.5707964f); // `PI / 2` is the constant at `0x858FE4`

    if (a5 == 0 || a5 == 1 || a5 == 2) {
        tileMatrix.GetPosition() = CVector{
            -(float)m_box->m_width * 0.5f + (float)offsetAlongX + (float)a2 + 0.5f,
            -(float)m_box->m_depth * 0.5f + (float)offsetAlongY + (float)a3 + 0.5f,
            a5 == 0
                ?  (float)m_box->m_height * 0.5f - a4
                : -(float)m_box->m_height * 0.5f + a4
        };
    }

    CMatrix objectMat = (CMatrix{ &m_matrix, false } * tileMatrix) * localRot;

    auto* const furnitureEntity = s_FurnitureEntityPool.RemoveHead();
    if (!furnitureEntity) {
        return nullptr;
    }

    auto* const entity = new CEntity();
    furnitureEntity->m_entity = entity;

    entity->SetModelIndexNoCreate(a1->m_nModelId); // Virtual call in retail (`vtable[0x18]`)
    entity->SetMatrix(objectMat);
    entity->SetAreaCode(static_cast<eAreaCodes>(m_areaCode));
    entity->m_nFlags |= 0x10000;  // See `CEntity`'s flags union
    entity->m_nFlags |= 0x400000; // `m_bIsTempBuilding`
    CWorld::Add(entity);

    furnitureEntity->m_tileX = (uint16)a3;
    furnitureEntity->m_tileY = (uint16)a4Bits;
    furnitureEntities.AddItem(furnitureEntity);

    if (a5 == 1) {
        SetTilesStatus(a5, a4Bits, sizeAcrossX, sizeAlongY, a1->m_bCanSteal ? 6 : 5, 1);
    }

    *a7 = sizeAcrossX;
    *a8 = sizeAlongY;

    return entity;
}

// 0x593120
void Interior_c::PlaceFurnitureOnWall(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 furnitureId, float a5, int32 a6, int32 a7, int32 a8, int32 a9, int32* a10,
                                      int32* a11, int32* a12, int32* a13, int32* a14, int32* a15) {
    auto* const furniture = g_furnitureMan.GetFurniture(furnitureGroupId, furnitureSubgroupId, furnitureId, m_box->m_status);

    //! `a7` is the orientation (0..3, or -1 for random) and `a8` the offset along the wall (or -1 for random)
    const int32 maxTries = (a7 == -1 && a8 == -1) ? 100 : 1;
    if (!furniture || maxTries <= 0) {
        return;
    }

    int32 usedOrientation = a7;
    int32 usedOffset      = a8;

    for (int32 tries = 0; tries < maxTries; tries++) {
        int32 orientation = a7;
        if (a7 == -1) {
            orientation = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 4.0f);
        }
        int32 offset = a8;

        int32 x, y;
        if (orientation == 1 || orientation == 3) {
            if (a8 == -1) {
                offset = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * (float)(m_box->m_depth - furniture->m_nWidthX));
            }
            if (orientation == 1) {
                x = a9;
                y = offset;
            } else {
                x = m_box->m_width - furniture->m_nWidthY - a9;
                y = offset;
            }
        } else {
            if (a8 == -1) {
                offset = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * (float)(m_box->m_width - furniture->m_nWidthX));
            }
            if (orientation == 2) {
                x = offset;
                y = a9;
            } else {
                x = offset;
                y = m_box->m_depth - furniture->m_nWidthY - a9;
            }
        }

        int32 out1, out2;
        if (PlaceFurniture(furniture, x, y, a5, a6, orientation, &out1, &out2, 0)) {
            usedOrientation = orientation;
            usedOffset      = offset;

            if (a10) *a10 = usedOrientation;
            if (a11) *a11 = usedOffset;
            if (a12) *a12 = x;
            if (a13) *a13 = y;
            if (a14) *a14 = out1;
            if (a15) *a15 = out2;
            return;
        }
    }
}

// 0x593340
void Interior_c::PlaceFurnitureInCorner(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 id, float a4, int32 a5, int32 a6, int32 a2,
                                        int32* a9, int32* a10, int32* a11, int32* a12, int32* a13) {
    auto* const furniture = g_furnitureMan.GetFurniture(furnitureGroupId, furnitureSubgroupId, id, m_box->m_status);

    //! `a2` is used as the offset along the walls; when it's `-1` the orientation is randomized
    const int32 maxTries = (a2 == -1) ? 20 : 1;
    if (!furniture || maxTries <= 0) {
        return;
    }

    for (int32 tries = 0; tries < maxTries; tries++) {
        int32 orientation = a6;
        if (a2 == -1) {
            orientation = (int32)((float)CGeneral::GetRandomNumber() * (1.0f / 32768.0f) * 4.0f);
        }

        //! Retail initializes both with `a6`, but every reachable case overwrites them
        int32 x = a6;
        int32 y = a6;
        switch (orientation) {
        case 1:
            x = a2;
            y = 0;
            break;
        case 3:
            x = m_box->m_width - furniture->m_nWidthY - a2;
            y = m_box->m_depth - furniture->m_nWidthX;
            break;
        case 2:
            x = m_box->m_width - furniture->m_nWidthX;
            y = a2;
            break;
        case 0:
            x = 0;
            y = m_box->m_depth - furniture->m_nWidthY - a2;
            break;
        default:
            break;
        }

        int32 out1, out2;
        if (PlaceFurniture(furniture, x, y, a4, a5, orientation, &out1, &out2, 0)) {
            if (a9)  *a9  = orientation;
            if (a10) *a10 = x;
            if (a11) *a11 = y;
            if (a12) *a12 = out1;
            if (a13) *a13 = out2;
            return;
        }
    }
}

// 0x591C50
bool Interior_c::FindEmptyTiles(int32 width, int32 height, int32* outX, int32* outY) {
    for (auto attempt = 0; attempt < 100; attempt++) {
        const auto x = RandomScaled(static_cast<float>(m_box->m_width - width));
        const auto y = RandomScaled(static_cast<float>(m_box->m_depth - height));
        if (CheckTilesEmpty(x, y, width, height, 1)) {
            *outX = x;
            *outY = y;
            return true;
        }
    }
    return false;
}

// 0x59A790
void Interior_c::FurnishShop(int32 a2) {
    m_furnitureGroupId = static_cast<int8>(a2);

    if (m_box->m_door - 1 < 6 && m_box->m_width - m_box->m_door < 6) {
        return;
    }

    SetTilesStatus(0,                   0,                   1, 1, 2, 0);
    SetTilesStatus(0,                   m_box->m_depth - 1,  1, 1, 2, 0);
    SetTilesStatus(m_box->m_width - 1,  0,                   1, 1, 2, 0);
    SetTilesStatus(m_box->m_width - 1,  m_box->m_depth - 1,  1, 1, 2, 0);

    Shop_PlaceFixedUnits();
    Shop_FurnishEdges();
    Shop_FurnishAisles();
}
