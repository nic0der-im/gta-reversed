#pragma once

#include "Base.h"

#include "rwplcore.h" // RwMatrix

#include "Vector.h"
#include "NodeAddress.h"
#include "InteriorInfo_t.h"
#include "List_c.h"
#include "ListItem_c.h"
#include "FurnitureEntity_c.h"

class CEntity;
class CObject;
class Furniture_c;
class InteriorGroup_c;

/*!
* @brief One of an interior's 16 "goto points" (Retail: 0x410, stride 0x10, count is `Interior_c::m_numGotoPoints`)
*/
struct InteriorGotoPoint {
    int8    m_tileX;  // 0x00
    int8    m_tileY;  // 0x01
    int8    m_pad[2]; // 0x02
    CVector m_pos;    // 0x04
};
VALIDATE_SIZE(InteriorGotoPoint, 0x10);

/*!
* @brief One of an interior's 4 exit-door path nodes (Retail: 0x510, stride 0x20)
*/
struct InteriorDoorPathNode {
    int16   m_pad0;       // 0x00 - Unused by the reversed functions
    int8    m_gotoPointA; // 0x02 - Index into the interior's goto points, or -1
    int8    m_gotoPointB; // 0x03 - Index into the interior's goto points, or -1
    CVector m_posIn;      // 0x04 - Position of the node inside the door
    int32   m_pad1;       // 0x10 - Unused by the reversed functions
    CVector m_posOut;     // 0x14 - Position of the node just outside the door (This one gets linked to the exterior paths)
};
VALIDATE_SIZE(InteriorDoorPathNode, 0x20);

class Interior_c : public ListItem_c<Interior_c> {
public:
    int32             m_interiorId;         // 0x8
    InteriorGroup_c*  m_pGroup;             // 0xC
    int32             m_areaCode;           // 0x10
    tEffectInterior*  m_box;                // 0x14
    RwMatrix          m_matrix;             // 0x18
    int32             field_58;             // 0x58
    TList_c<int>     m_list;                // 0x5C - TODO: Figure out type
    char              field_68[900];        // 0x68
    CNodeAddress      m_exteriorNodeAddress; // 0x3EC - The (exterior) node the interior's door was linked to
    CNodeAddress      m_nodeAddress;         // 0x3F0 - Node of the interior (Inside its door)
    CVector           m_exteriorNodePos;     // 0x3F4 - Position of `m_exteriorNodeAddress`
    CVector           m_position;            // 0x400
    int8              m_numGotoPoints;       // 0x40C
    int8              m_interiorInfosCount;  // 0x40D
    int16             _pad40E;               // 0x40E
    InteriorGotoPoint m_gotoPoints[16];      // 0x410
    InteriorDoorPathNode m_doorPathNodes[4]; // 0x510
    InteriorInfo_t    m_interiorInfos[16];   // 0x590
    int8              m_furnitureGroupId;   // 0x790
    int8              m_furnitureId;        // 0x791
    int8              field_792;            // 0x792
    int8              field_793;            // 0x793

public:
    static void InjectHooks();

    Interior_c() = default;
    ~Interior_c() = default; // 0x591360

    int32 Init(const CVector& pos);
    void Exit();

    CObject* Bedroom_AddTableItem(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6, int32 a7);
    void FurnishBedroom();
    CObject* Kitchen_FurnishEdges();
    void FurnishKitchen();
    CObject* Lounge_AddTV(int32 a2, int32 a3, int32 a4, int32 a5);
    CObject* Lounge_AddHifi(int32 a2, int32 a3, int32 a4, int32 a5);
    void Lounge_AddChairInfo(int32 a2, int32 a3, CEntity* entityIgnoredCollision);
    void Lounge_AddSofaInfo(int32 sitType, int32 offsetX, CEntity* entityIgnoredCollision);
    void FurnishLounge();
    int32 Office_PlaceEdgeFillers(int32 arg0, int32 a2, int32 a3, int32 a6, int32);
    int32 Office_PlaceDesk(int32 a3, int32 arg4, int32 offsetY, int32 a5, uint8 a6, int32 b);
    int32 Office_PlaceEdgeDesks(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6);
    void Office_FurnishEdges();
    int32 Office_PlaceDeskQuad(int32 a2, int32 a3, int32 a4, int32 a5);
    int32 Office_FurnishCenter();
    void FurnishOffice();
    int8 Shop_Place3PieceUnit(int32 a2, int32 a3, int32 a4, int32 a5, int32 a6);
    int32 Shop_PlaceEdgeUnits(int32 a2, int32 a3, int32 a4, int32 a5);
    int32 Shop_PlaceCounter(uint8 a2);
    void Shop_PlaceFixedUnits();
    void Shop_FurnishCeiling();
    void Shop_AddShelfInfo(int32 a2, int32 a3, int32 a5);
    void Shop_FurnishEdges();
    bool GetBoundingBox(FurnitureEntity_c* entity, CVector* a3);
    void ResetTiles();
    CObject* PlaceObject(uint8 isStealable, Furniture_c* furniture, float offsetX, float offsetY, float offsetZ, float rotationZ);
    FurnitureEntity_c* GetFurnitureEntity(const CEntity& entity);
    bool IsPtInside(const CVector& pt, CVector bias = {});
    void CalcMatrix(CVector* translation);
    void Furnish();
    void Unfurnish();
    int8 CheckTilesEmpty(int32 a1, int32 a2, int32 a3, int32 a4, uint8 a5);
    void SetTilesStatus(int32 a, int32 b, int32 a3, int32 a4, int32 a5, int8 a6);
    void SetCornerTiles(int32 a4, int32 a3, int32 a5, uint8 a6);
    int32 GetTileStatus(int32 x, int32 y);
    int32 GetNumEmptyTiles(int32 a2, int32 a3, int32 a4, int32 a5);
    int32 GetRandomTile(int32 a2, int32* a3, int32* a4);
    void Shop_FurnishAisles();
    CVector* GetTileCentre(float offsetX, float offsetY, CVector* pointsIn);
    void AddGotoPt(int32 a, int32 b, float a3, float a4);
    bool AddInteriorInfo(int32 actionType, float offsetX, float offsetY, int32 direction, CEntity* entityIgnoredCollision);
    void AddPickups();
    void FindBoundingBox(int32, int32, int32*, int32*, int32*, int32*, int32*);
    void CalcExitPts();
    bool IsVisible();
    CEntity* PlaceFurniture(Furniture_c* a1, int32 a2, int32 a3, float a4, int32 a5, int32 a6, int32* a7, int32* a8, uint8 a9);
    void PlaceFurnitureOnWall(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 furnitureId, float a5, int32 a6, int32 a7, int32 a8, int32 a9, int32* a10, int32* a11,
                              int32* a12, int32* a13, int32* a14, int32* a15);
    void PlaceFurnitureInCorner(int32 furnitureGroupId, int32 furnitureSubgroupId, int32 id, float a4, int32 a5, int32 a6, int32 a2, int32* a9, int32* a10, int32* a11, int32* a12,
                                int32* a13);
    bool FindEmptyTiles(int32 a3, int32 a4, int32* arg8, int32* a5);
    void FurnishShop(int32 a2);

    auto GetNodeAddress() const { return m_nodeAddress; }
};
VALIDATE_OFFSET(Interior_c, m_exteriorNodeAddress, 0x3EC);
VALIDATE_OFFSET(Interior_c, m_nodeAddress, 0x3F0);
VALIDATE_OFFSET(Interior_c, m_exteriorNodePos, 0x3F4);
VALIDATE_OFFSET(Interior_c, m_numGotoPoints, 0x40C);
VALIDATE_OFFSET(Interior_c, m_gotoPoints, 0x410);
VALIDATE_OFFSET(Interior_c, m_doorPathNodes, 0x510);
VALIDATE_OFFSET(Interior_c, m_interiorInfos, 0x590);
VALIDATE_SIZE(Interior_c, 0x794);
