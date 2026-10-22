/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"
#include "PathFind.h"

#include <reversiblebugfixes/Bugs.hpp>

// TODO: Move into the class itself
auto& s_pathsNeededPosn = StaticRef<CVector>(0x977B70);
auto& s_bLoadPathsNeeded = StaticRef<bool>(0x96F030);

// TODO: Remove this and use a stack array or smth..
auto& ToBeStreamed = StaticRef<std::array<bool, NUM_PATH_MAP_AREAS>>(0x96EFD0);

auto& XCoorGiven = StaticRef<std::array<float, 64>>(0x96EE80);
auto& YCoorGiven = StaticRef<std::array<float, 64>>(0x96ED80);
auto& ZCoorGiven = StaticRef<std::array<float, 64>>(0x96EC80);
auto& ConnectsToGiven = StaticRef<std::array<std::array<int8, 8>, 64>>(0x96EAC0);
auto& aDontWanderGiven = StaticRef<std::array<bool, 64>>(0x96EC40);

auto& aInteriorNodeLinkedToExterior = StaticRef<std::array<int32, NUM_PATH_INTERIOR_AREAS>>(0x96EA98);
auto& aExteriorNodeLinkedTo = StaticRef<std::array<CNodeAddress, NUM_PATH_INTERIOR_AREAS>>(0x977B7C);

auto& aNodesToBeCleared = StaticRef<std::array<CNodeAddress, 5000>>(0x972CD0);

void CPathFind::InjectHooks() {
    RH_ScopedClass(CPathFind);
    RH_ScopedCategoryGlobal();

    // Hooks commented out because most of the functions have no definitions
    // thus it's not possible to take their address
    // And I'm lazy to add stubs

    RH_ScopedInstall(AddNodeToNewInterior, 0x450E90);
    RH_ScopedInstall(FindNearestExteriorNodeToInteriorNode, 0x450F30);
    RH_ScopedInstall(ThisNodeHasToBeSwitchedOff, 0x44D3E0);
    RH_ScopedInstall(These2NodesAreAdjacent, 0x44D230);
    RH_ScopedInstall(FindRegionForCoors, 0x44D830);
    RH_ScopedInstall(FindYRegionForCoors, 0x44D8C0);
    RH_ScopedInstall(FindXRegionForCoors, 0x44D890);
    RH_ScopedInstall(AddInteriorLinkToExternalNode, 0x44DF30);
    RH_ScopedInstall(AddInteriorLink, 0x44DED0);
    RH_ScopedInstall(RemoveInteriorLinks, 0x44DF60);
    RH_ScopedInstall(MarkRegionsForCoors, 0x44DB60);
    RH_ScopedOverloadedInstall(FindStartPointOfRegion, "", 0x44D930, void(CPathFind::*)(size_t, size_t, float&, float&));
    RH_ScopedInstall(FindYCoorsForRegion, 0x44D910);
    RH_ScopedInstall(FindXCoorsForRegion, 0x44D8F0);
    RH_ScopedInstall(IsAreaNodesAvailable, 0x420AA0);
    RH_ScopedInstall(HaveRequestedNodesBeenLoaded, 0x450DB0);
    RH_ScopedInstall(MakeRequestForNodesToBeLoaded, 0x450D70);
    RH_ScopedInstall(UpdateStreaming, 0x450A60);
    RH_ScopedInstall(TakeWidthIntoAccountForWandering, 0x4509A0);
    RH_ScopedInstall(Shutdown, 0x450950);
    RH_ScopedOverloadedInstall(FindNodeCoorsForScript, "TwoNodes", 0x450780, CVector(CPathFind::*)(CNodeAddress, CNodeAddress, float&, bool*));
    RH_ScopedOverloadedInstall(FindNodeCoorsForScript, "LinkedNode", 0x4505E0, CVector(CPathFind::*)(CNodeAddress, bool*));
    RH_ScopedInstall(IsWaterNodeNearby, 0x450DE0);
    RH_ScopedInstall(CountNeighboursToBeSwitchedOff, 0x4504F0);
    RH_ScopedInstall(FindNodeOrientationForCarPlacement, 0x450320);
    //RH_ScopedInstall(FindNodePairClosestToCoors, 0x44FEE0);
    RH_ScopedInstall(FindNodeClosestToCoorsFavourDirection, 0x44FCE0);
    RH_ScopedInstall(RecordNodesClosestToCoors, 0x44FA30);
    RH_ScopedInstall(MarkRoadNodeAsDontWander, 0x450560);
    //RH_ScopedInstall(AddDynamicLinkBetween2Nodes, 0x4512D0);
    RH_ScopedOverloadedInstall(LoadPathFindData, "Area", 0x452F40, void(CPathFind::*)(int32));
    RH_ScopedOverloadedInstall(LoadPathFindData, "FromStream", 0x4529F0, void(CPathFind::*)(RwStream*, int32));
    RH_ScopedInstall(SwitchPedRoadsOffInArea, 0x452F00);
    RH_ScopedInstall(SwitchRoadsOffInArea, 0x452C80);
    RH_ScopedInstall(SwitchRoadsOffInAreaForOneRegion, 0x452820);
    RH_ScopedInstall(ComputeRoute, 0x452760);
    RH_ScopedInstall(CompleteNewInterior, 0x452270);
    RH_ScopedInstall(SwitchOffNodeAndNeighbours, 0x452160);
    RH_ScopedInstall(Find2NodesForCarCreation, 0x452090);
    //RH_ScopedInstall(TestCoorsCloseness, 0x452000);
    RH_ScopedInstall(FindNextNodeWandering, 0x451B70);
    RH_ScopedInstall(DoPathSearch, 0x4515D0, {.Reversed = false}); // Sometimes breaks `CTaskComplexFollowNodeRoute::ComputePathNodes` - To repro just walk around in groove st. 
    RH_ScopedInstall(FindParkingNodeInArea, 0x4513F0);
    RH_ScopedInstall(FindLinkBetweenNodes, 0x451350);
    RH_ScopedInstall(ReturnInteriorNodeIndex, 0x451300);
    RH_ScopedInstall(FindNthNodeClosestToCoors, 0x44F8C0);
    RH_ScopedInstall(FindNodeClosestInRegion, 0x44F2C0);
    RH_ScopedInstall(CalcDistToAnyConnectingLinks, 0x44F190);
    RH_ScopedInstall(CalcRoadDensity, 0x44EFC0);
    RH_ScopedInstall(TestForPedTrafficLight, 0x44D480);
    RH_ScopedInstall(UnMarkAllRoadNodesAsDontWander, 0x44D400);
    RH_ScopedInstall(TidyUpNodeSwitchesAfterMission, 0x44D3B0);
    RH_ScopedInstall(ThisNodeWillLeadIntoADeadEnd, 0x44D310);
    RH_ScopedInstall(AddNodeToList, 0x44D1E0);
    RH_ScopedInstall(RemoveNodeFromList, 0x44D1B0);
    RH_ScopedInstall(UnLoadPathFindData, 0x44D0F0);
    RH_ScopedInstall(Init, 0x44D080);
    RH_ScopedInstall(GetPathNode, 0x420AC0);
    RH_ScopedInstall(TestCrossesRoad, 0x44D790);
    RH_ScopedInstall(ReInit, 0x44E4E0);
    RH_ScopedInstall(RemoveInterior, 0x44E1A0);
    RH_ScopedInstall(AddDynamicLinkBetween2Nodes_For1Node, 0x44E000);
    RH_ScopedInstall(StartNewInterior, 0x44DE80);
    RH_ScopedInstall(LoadSceneForPathNodes, 0x44DE00);
    RH_ScopedInstall(AreNodesLoadedForArea, 0x44DD10);
    RH_ScopedInstall(ReleaseRequestedNodes, 0x44DD00);
    RH_ScopedInstall(SetPathsNeededAtPosition, 0x44DCD0);
    RH_ScopedInstall(SetLinksBridgeLights, 0x44D960);
    RH_ScopedInstall(Save, 0x5D34C0);
    RH_ScopedInstall(Load, 0x5D3500);
    RH_ScopedInstall(GeneratePedCreationCoors_Interior, 0x44ECA0);
    RH_ScopedInstall(FindNodeClosestToCoors, 0x44F460);
}

void CPathNode::InjectHooks() {
    RH_ScopedClass(CPathNode);
    RH_ScopedCategoryGlobal();
    RH_ScopedInstall(GetPosition, 0x420A10);
}

// 0x44D080
void CPathFind::Init() {
    ZoneScoped;

    static int32 NumTempExternalNodes = 0; // Unused
    m_nNumNodeSwitches                = 0;
    m_loadAreaRequestPending = false;

    for (auto i = 0u; i < NUM_TOTAL_PATH_NODE_AREAS; ++i) {
        m_pPathNodes[i] = nullptr;
        m_pNaviNodes[i] = nullptr;
        m_pNodeLinks[i] = nullptr;
        m_pLinkLengths[i] = nullptr;
        m_pPathIntersections[i] = nullptr;
        m_pNaviLinks[i] = nullptr; // BUG: Out of array bounds write, same as in original code
        m_aTempNodes[i] = nullptr;    // BUG: Out of array bounds write, same as in original code
    }

    rng::fill(m_interiorIDs, (uint32)-1);
}

// 0x44E4E0
void CPathFind::ReInit() {
    m_nNumNodeSwitches       = 0;
    m_loadAreaRequestPending = false;
}

// 0x44DD00
void CPathFind::MakeRequestForNodesToBeLoaded(float minX, float maxX, float minY, float maxY) {
    m_loadAreaRequestPending = true;
    m_loadAreaRequestMinX = minX;
    m_loadAreaRequestMaxX = maxX;
    m_loadAreaRequestMinY = minY;
    m_loadAreaRequestMaxY = maxY;
    UpdateStreaming(true);
}

// 0x44DD10
bool CPathFind::AreNodesLoadedForArea(float minX, float maxX, float minY, float maxY) {
    return IterAreasTouchingRect({ minX, minY, maxX, maxY }, [this](auto areaId) -> bool { return IsAreaLoaded(areaId); });
}

// 0x450DB0
bool CPathFind::HaveRequestedNodesBeenLoaded() {
    return AreNodesLoadedForArea(
        m_loadAreaRequestMinX,
        m_loadAreaRequestMaxX,
        m_loadAreaRequestMinY,
        m_loadAreaRequestMaxY
    );
}

// 0x450950
void CPathFind::Shutdown() {
    for (auto x = 0u; x < NUM_PATH_MAP_AREA_X; ++x) {
        for (auto y = 0u; y < NUM_PATH_MAP_AREA_Y; ++y) {
            auto relativeId = x + y * NUM_PATH_MAP_AREA_X;
            if (m_pPathNodes[relativeId]) {
                CStreaming::RemoveModel(DATToModelId(relativeId));
            }
        }
    }
}

bool CPathFind::ThisNodeWillLeadIntoADeadEnd(CPathNode* startNode, CPathNode* endNode) {
    auto curr = startNode, prev = endNode;
    while (true) {
        CPathNode* next{}; // If node has no links (or neither links area is loaded) this will be nullptr
        for (auto& linked : GetNodeLinkedNodes(*curr)) {
            if (&linked == prev) { // Obviously don't count the previous node
                continue;
            }
            if (linked.m_nBehaviourType == 4u || linked.m_nBehaviourType > 10u) { // TODO: Enum?
                // I'm unsure what's happening here
                // I think, since this function isn't recursive, they just
                // consider having 2 appropriate links as a non-deadend
                if (next) {
                    return false;
                }
                next = &linked;
            }
        }
        if (!next) { 
            return true;
        }
        prev = curr;
        curr = next;
    }
}

// 0x44D3B0
void CPathFind::TidyUpNodeSwitchesAfterMission() {
    m_nNumNodeSwitches = std::min(54u, m_nNumNodeSwitches); // todo: magic number
}

// 0x44D400
void CPathFind::UnMarkAllRoadNodesAsDontWander() {
    for (auto i = 0u; i < NUM_PATH_MAP_AREAS; ++i) {
        if (!m_pPathNodes[i])
            continue;

        for (auto nodeInd = 0u; nodeInd < m_anNumVehicleNodes[i]; ++nodeInd) {
            m_pPathNodes[i][nodeInd].m_bDontWander = false;
        }
    }
}

// 0x44DD00
void CPathFind::ReleaseRequestedNodes() {
    m_loadAreaRequestPending = false;
}

/*!
* @brief Find intersection info between 2 nodes
* @addr notsa 100% inlined
*/
auto CPathFind::FindIntersection(const CNodeAddress& startNodeAddress, const CNodeAddress& targetNodeAddress) -> CPathIntersectionInfo* {
    // Make sure both nodes areas are loaded
    if (!AreNodeAreasLoaded({ targetNodeAddress, startNodeAddress })) {
        return nullptr;
    }

    const auto& startNode = *GetPathNode(startNodeAddress);
    const auto& nodeLinks = m_pNodeLinks[startNodeAddress.m_wAreaId];
    for (auto i = 0u; i < startNode.m_nNumLinks; i++) {
        const auto linkedNodeIdx = startNode.m_wBaseLinkId + i;
        if (nodeLinks[linkedNodeIdx] == targetNodeAddress) {
            return &m_pPathIntersections[startNodeAddress.m_wAreaId][linkedNodeIdx];
        }
    }

    return nullptr;
}

// 0x44D790
bool CPathFind::TestCrossesRoad(CNodeAddress startNodeAddress, CNodeAddress targetNodeAddress) {
    const auto intersect = FindIntersection(startNodeAddress, targetNodeAddress);
    return intersect && intersect->m_bRoadCross;
}

// 0x44ECA0
bool CPathFind::GeneratePedCreationCoors_Interior(float x, float y, CVector* outCoords, CNodeAddress* unused1, CNodeAddress* unused2, float* outOrientation) {
    const auto areaId = FindRegionForCoors({ x, y });

    for (auto attempt = 0; attempt < 200; attempt++) {
        if (!IsAreaLoaded(areaId) || !m_anNumPedNodes[areaId]) {
            continue;
        }

        // Pick a random *ped* node (They come right after the vehicle nodes)
        const auto nodeIdx = (int32)m_anNumVehicleNodes[areaId] + ((CGeneral::GetRandomNumber() >> 6) % (int32)m_anNumPedNodes[areaId]);
        const auto& node    = m_pPathNodes[areaId][nodeIdx];
        const auto  nodePos = node.GetPosition();

        if ((nodePos.x - x) * (nodePos.x - x) + (nodePos.y - y) * (nodePos.y - y) >= 2500.f) { // Not within 50 units
            continue;
        }
        if (nodePos.z <= 900.f) { // Only interior nodes
            continue;
        }

        for (auto linkNum = 0u; linkNum < node.m_nNumLinks; linkNum++) {
            const auto linkAddr = m_pNodeLinks[areaId][node.m_wBaseLinkId + linkNum];
            if (linkAddr.m_wAreaId >= NUM_PATH_MAP_AREAS || !IsAreaNodesAvailable(linkAddr)) { // Only linked to exterior nodes
                continue;
            }
            const auto linkedPos = GetPathNode(linkAddr)->GetPosition();

            for (auto i = 0; i < 5; i++) {
                const auto t = (float)(CGeneral::GetRandomNumber() & 0xFF) * (1.f / 256.f);
                *outOrientation = t;

                // Interpolate between the 2 nodes
                outCoords->x = nodePos.x * (1.f - t) + linkedPos.x * t;
                outCoords->y = nodePos.y * (1.f - t) + linkedPos.y * t;
                outCoords->z = nodePos.z * (1.f - t) + linkedPos.z * t;

                bool       found   = false;
                const auto groundZ = CWorld::FindGroundZFor3DCoord(*outCoords + CVector{ 0.f, 0.f, 2.f }, &found, nullptr);
                if (found && std::fabs(groundZ - outCoords->z) < 3.f) {
                    outCoords->z = groundZ;
                    return true;
                }
            }
        }
    }

    return false;
}

// 0x44D480
bool CPathFind::TestForPedTrafficLight(CNodeAddress startNodeAddress, CNodeAddress targetNodeAddress) {
    const auto intersect = FindIntersection(startNodeAddress, targetNodeAddress);
    return intersect && intersect->m_bPedTrafficLight;
}

// 0x4509A0
CVector CPathFind::TakeWidthIntoAccountForWandering(CNodeAddress nodeAddress, int16 randomSeed) {
    // Invalid area, or area not loaded
    if (!nodeAddress.IsValid() || !IsAreaNodesAvailable(nodeAddress)) {
        return {};
    }

    auto node         = GetPathNode(nodeAddress);
    auto basePosition = node->GetPosition();
    auto offsetX      = float(node->m_nPathWidth * ((randomSeed % 16) - 7)); // bottom 8 bits remapped to [-7 : +8]
    auto offsetY      = float(node->m_nPathWidth * (((randomSeed / 16) % 16) - 7)); // top 8 bits remapped to [-7 : +8]
    auto offset       = CVector{ offsetX * 0.00775f, offsetY * 0.00775f, 0.f };
    return basePosition + offset;
}

//  0x44F8C0
CNodeAddress CPathFind::FindNthNodeClosestToCoors(CVector pos, uint8 nodeType, float maxDistance, bool bLowTraffic, bool bUnkn, int32 nthNode, bool bBoatsOnly,
                                                  bool bIgnoreInterior, CNodeAddress* outNode) {
    // Clear the "Already returned by this call" mark of every candidate node
    for (auto areaId = 0u; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
        if (!m_pPathNodes[areaId]) {
            continue;
        }

        int32 start = nthNode, end = nthNode;
        if (nodeType == PATH_TYPE_VEH) {
            start = 0;
            end   = (int32)m_anNumVehicleNodes[areaId];
        } else if (nodeType == PATH_TYPE_PED) {
            start = (int32)m_anNumVehicleNodes[areaId];
            end   = (int32)m_anNumNodes[areaId];
        }

        for (auto nodeIdx = start; nodeIdx < end; nodeIdx++) {
            m_pPathNodes[areaId][nodeIdx].unk1 = false;
        }
    }

    // Find the closest node, marking each one found so it's skipped on the next iteration
    CNodeAddress found;
    while (true) {
        if (nthNode < 1) {
            return FindNodeClosestToCoors(pos, (ePathType)nodeType, maxDistance, bLowTraffic, bUnkn, 1, bBoatsOnly, 0);
        }

        found = FindNodeClosestToCoors(pos, (ePathType)nodeType, maxDistance, bLowTraffic, bUnkn, 1, bBoatsOnly, bIgnoreInterior);

        if (outNode) {
            *outNode = found;
        }

        if (found.m_wAreaId == (uint16)-1) { // No more nodes
            break;
        }

        m_pPathNodes[found.m_wAreaId][found.m_wNodeId].unk1 = true;
        nthNode--;
    }

    return found;
}

// 0x44FA30
void CPathFind::RecordNodesClosestToCoors(CVector pos, uint8 nodeType, int count, CNodeAddress* outAddresses, float maxDist, bool unk2, bool unk3, bool bBoatsOnly, bool unk6) {
    // Clear the "Already recorded" mark of the nodes of the given type, so that they can all be found again
    for (auto areaId = 0u; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
        if (!IsAreaLoaded(areaId)) {
            continue;
        }

        int32 start = nodeType, end = nodeType;
        if (nodeType == PATH_TYPE_VEH) {
            start = 0;
            end   = (int32)m_anNumVehicleNodes[areaId];
        } else if (nodeType == PATH_TYPE_PED) {
            start = (int32)m_anNumVehicleNodes[areaId];
            end   = (int32)m_anNumNodes[areaId];
        }

        for (auto nodeIdx = start; nodeIdx < end; nodeIdx++) {
            m_pPathNodes[areaId][nodeIdx].unk1 = false;
        }
    }

    for (; count > 0; count--) {
        const auto found = FindNodeClosestToCoors(pos, (ePathType)nodeType, maxDist, unk2, unk3, 1, bBoatsOnly, unk6);
        if (!found.IsAreaValid()) { // No more nodes
            return;
        }

        GetPathNode(found)->unk1 = true;
        *outAddresses++ = found;
    }
}

//! @notsa
//! @brief Direction code (`0..7`) of the normalized 2D vector `(dx, dy)`.
//! Computed inline by `CPathFind::FindNextNodeWandering` (0x451B70).
static uint8 GetWanderingDirCode(float dx, float dy) {
    if (dx < 0.f) {
        if (-dx <= 2.f * std::fabs(dy)) {
            if (dy <= -2.f * dx) {
                if (2.f * dx <= dy) {
                    return dy <= 0.f ? 5 : 7;
                }
                return 4;
            }
            return 0;
        }
        return 6;
    }
    if (dx <= 2.f * std::fabs(dy)) {
        if (dy <= 2.f * dx) {
            if (-2.f * dx <= dy) {
                return dy <= 0.f ? 3 : 1;
            }
            return 4;
        }
        return 0;
    }
    return 2;
}

// 0x451B70
void CPathFind::FindNextNodeWandering(uint8 nodeType, CVector vecPos, CNodeAddress* originAddress, CNodeAddress* targetAddress, uint8 dir, uint8* outDir) {
    // Only the area ID of the input address is looked at
    CNodeAddress savedOrigin;
    if (originAddress) {
        savedOrigin = *originAddress;
    }

    CNodeAddress origin;
    if (savedOrigin.m_wAreaId == (uint16)-1) {
        origin = FindNodeClosestToCoors(vecPos, (ePathType)nodeType, 999999.88f, 0, 0, 0, 0, 0);
    } else {
        if (!IsAreaLoaded(savedOrigin.m_wAreaId)) {
            targetAddress->m_wAreaId = (uint16)-1;
            *outDir                  = 0;
            return;
        }

        const auto& hint      = *GetPathNode(savedOrigin);
        const auto  hintRange = std::max((float)hint.m_nPathWidth * 0.125f, 7.f);
        if (hintRange < (vecPos - hint.GetPosition()).Magnitude()) {
            origin = FindNodeClosestToCoors(vecPos, (ePathType)nodeType, 999999.88f, 0, 0, 0, 0, 0);
        } else {
            origin = savedOrigin;
        }
    }

    bool foundTarget = false;

    if (origin.m_wAreaId == (uint16)-1 || !IsAreaLoaded(origin.m_wAreaId)) {
        targetAddress->m_wAreaId = (uint16)-1;
        origin.m_wAreaId         = (uint16)-1; // Note: `origin` ends up as the target below
    } else {
        const auto& node    = *GetPathNode(origin);
        const auto  nodePos = node.GetPosition();

        targetAddress->m_wAreaId = (uint16)-1;

        const auto angle = (float)dir * (PI / 4.f);
        const auto sinA  = std::sin(angle);
        const auto cosA  = std::cos(angle);

        float bestDot = -999999.f;
        for (auto linkNum = 0u; linkNum < node.m_nNumLinks; linkNum++) {
            const auto linkAddr = m_pNodeLinks[node.m_wAreaId][node.m_wBaseLinkId + linkNum];
            if (!IsAreaNodesAvailable(linkAddr)) {
                continue;
            }

            const auto& linked = *GetPathNode(linkAddr);
            if (!node.m_isSwitchedOff && linked.m_isSwitchedOff) {
                continue;
            }
            if (!node.m_bDontWander && linked.m_bDontWander) {
                continue;
            }

            const auto linkedPos = linked.GetPosition();
            float      dx        = linkedPos.x - nodePos.x;
            float      dy        = linkedPos.y - nodePos.y;
            const auto invLen    = 1.f / std::sqrt(dx * dx + dy * dy);
            dx *= invLen;
            dy *= invLen;

            const auto dot = dx * sinA + dy * cosA;
            if (bestDot <= dot) { // On ties the last link wins
                *targetAddress = linkAddr;
                bestDot        = dot;
                *outDir        = GetWanderingDirCode(dx, dy);
            }
        }

        if (originAddress) { // Note: The original dereferences this unconditionally when a node was found
            *originAddress = origin;
        }
        foundTarget = targetAddress->m_wAreaId != (uint16)-1;
    }

    if (!foundTarget) {
        *outDir        = 0;
        *targetAddress = origin; // Couldn't find anything, so stay where we are
    }

    // If we'd walk back to where we came from, use the first loaded link of that node instead
    if (*targetAddress == savedOrigin && savedOrigin.m_wAreaId != (uint16)-1 && IsAreaLoaded(savedOrigin.m_wAreaId)) {
        const auto& node = *GetPathNode(savedOrigin);
        for (auto linkNum = 0u; linkNum < node.m_nNumLinks; linkNum++) {
            const auto linkAddr = m_pNodeLinks[node.m_wAreaId][node.m_wBaseLinkId + linkNum];
            if (IsAreaNodesAvailable(linkAddr)) {
                *targetAddress = linkAddr;
                break;
            }
        }
    }
}

// 0x4515D0
void CPathFind::DoPathSearch(
    ePathType pathType,
    CVector originPos,
    CNodeAddress originAddrAddrHint, // If invalid/area not loaded the closest node to `originPos` is used.
    CVector targetPos,
    CNodeAddress* outResultNodes,
    int16& outNodesCount,
    int32 maxNodesToFind,
    float* outDistance,
    float maxSearchDistance,
    CNodeAddress* targetNodeAddrHint, // If null/invalid/area not loaded the closest node to `targetPos` is used.
    float maxSearchDepth,
    bool sameLaneOnly,
    CNodeAddress forbiddenNodeAddr,
    bool bAllowWaterNodeTransitions,
    bool forBoats
) {
    // Moved this up here, as it's set in every return path
    outNodesCount = 0;

    const auto ResolveNode = [&, this](CVector nodePosn, CNodeAddress* addr) {
        if (addr && addr->IsValid()) {
            // In case area is not loaded we still fall-back to
            // finding the closest node, as that will yield the
            // node closest in a loaded area
            if (IsAreaNodesAvailable(*addr)) {
                return GetPathNode(*addr);
            }
        }
        const auto foundAddr = FindNodeClosestToCoors(
            nodePosn,
            pathType,
            maxSearchDistance,
            false,
            false,
            false,
            forBoats,
            false
        );
        return foundAddr.IsValid()
            ? GetPathNode(foundAddr)
            : nullptr;
    };

    // Resolve addresses to use. Dont use `originAddrAddr` or `targetNodeAddr` after this point
    CPathNode *origin, *target{};
    if (   !(origin = ResolveNode(originPos, &originAddrAddrHint))
        || !(target = ResolveNode(targetPos, targetNodeAddrHint))
    ) {
    fail:
        outNodesCount = 0;
        if (outDistance) {
            *outDistance = 100'000.f;
        }
        return;
    }

    // Check if the 2 nodes ended up being the same
    if (*origin == *target) {
        outNodesCount = 0;
        if (outDistance) {
            *outDistance = 0.f;
        }
        return;
    }

    // Check if flood fill values match, if not, fail
    if (origin->m_nFloodFill != target->m_nFloodFill) {
        goto fail;
    }

    rng::fill(m_pathFindHashTable, nullptr);
    m_totalNumNodesInPathFindHashTable = 0u;

    AddNodeToList(target, 0);

    size_t numNodesToBeCleared{};
    const auto AddNodeToBeCleared = [&](const CPathNode& node) {
        if (numNodesToBeCleared < std::size(aNodesToBeCleared)) {
            aNodesToBeCleared[numNodesToBeCleared++] = node.GetAddress();
        }
    };
    AddNodeToBeCleared(*target);

    size_t iterDepth{};
    bool finished{};
    while (true) {
        // Dijkstra's algorithm (probably)

        // Find distances
        for (auto node = m_pathFindHashTable[iterDepth % std::size(m_pathFindHashTable)]; node; node = node->m_next) {
            if (*node == *origin) {
                finished = true;
            }

            for (auto linkNum = 0u; linkNum < node->m_nNumLinks; linkNum++) {
                const auto linkIdx    = node->m_wBaseLinkId + linkNum;
                const auto linkedAddr = m_pNodeLinks[node->m_wAreaId][linkIdx];

                if (!IsAreaNodesAvailable(linkedAddr)) {
                    continue;
                }

                auto& linked = *GetPathNode(linkedAddr);

                // Omitted the bool variable and instead used `continue`s

                if (sameLaneOnly) { // 0x451814
                    const auto& naviLinkAddr = m_pNaviLinks[node->m_wAreaId][linkIdx];
                    if (IsAreaLoaded(naviLinkAddr.m_wAreaId)) {
                        const auto& naviLink = GetCarPathLink(naviLinkAddr);
                        if (naviLink.m_attachedTo == linked.GetAddress()) {
                            if (!naviLink.m_numSameDirLanes) {
                                continue;
                            }
                        } else if (!naviLink.m_numOppositeDirLanes) {
                            continue;
                        }
                    }
                }

                if (forbiddenNodeAddr == linked.GetAddress()) {
                    continue;
                }

                // 0x451885
                if (node->m_bWaterNode != linked.m_bWaterNode && !bAllowWaterNodeTransitions) {
                    continue;
                }

                // 0x4518BD
                const auto distToOriginFromLinked = node->m_totalDistFromOrigin + m_pLinkLengths[node->m_wAreaId][linkIdx];

                // If this new route we found is better than the previous re-insert node into hashtable
                if (distToOriginFromLinked < linked.m_totalDistFromOrigin) {
                    if (linked.m_totalDistFromOrigin != SHRT_MAX - 1) { // Why the fuck they used this instead of `SHRT_MAX`?
                        RemoveNodeFromList(&linked);
                    } else {
                        AddNodeToBeCleared(linked);
                    }
                    AddNodeToList(&linked, distToOriginFromLinked);
                }
            }

            // We've visited this node, so remove it
            RemoveNodeFromList(node);
        }

        // No more nodes? Well, too sad.
        if (!m_totalNumNodesInPathFindHashTable) {
            break;
        }

        // Hit limit?
        if (++iterDepth > maxSearchDepth) {
            break;
        }

        if (numNodesToBeCleared >= std::size(aNodesToBeCleared) - 50) {
            break;
        }

        if (!finished) { // Inverted 0x4519C0 
            continue;
        }

        if (outDistance) {
            *outDistance = origin->m_totalDistFromOrigin;
        }
        if (outResultNodes) { // Weird check really, because below it isn't checked :D
            outResultNodes[outNodesCount++] = origin->GetAddress();
        }

        for (auto node = origin; node == target || outNodesCount < maxNodesToFind; outNodesCount++) {
            for (auto linkNum = 0u; linkNum < node->m_nNumLinks; linkNum++) {
                const auto linkedAddr = m_pNodeLinks[node->m_wAreaId][linkNum];
                const auto linkIdx    = node->m_wBaseLinkId + linkNum;
                if (!IsAreaNodesAvailable(linkedAddr)) {
                    continue;
                }
                const auto linked = GetPathNode(linkedAddr);
                if (const auto dist = node->m_totalDistFromOrigin - m_pLinkLengths[node->m_wAreaId][linkIdx]; dist == linked->m_totalDistFromOrigin) {
                    outResultNodes[outNodesCount++] = linkedAddr;
                    node = linked;
                    break;
                }
            }
        }
        break;
    }
    for (auto& addr : aNodesToBeCleared | rng::views::take(numNodesToBeCleared)) {
        GetPathNode(addr)->m_totalDistFromOrigin = SHRT_MAX - 1;
    }
}

// 0x452760
void CPathFind::ComputeRoute(uint8 nodeType, const CVector& vecStart, const CVector& vecEnd, const CNodeAddress& startAddress, CNodeRoute* route) {
    CNodeAddress nodes[8]; // Note: The original only initializes the area IDs, the node IDs are never read

    int16 numNodes = 0;

    // 0x8A5F44 - The "no forbidden node" address (Note: The node ID is `0`, not `-1`)
    static const CNodeAddress NoForbiddenNode{ (uint16)-1, 0 };

    DoPathSearch(
        (ePathType)nodeType,
        vecStart,
        startAddress,
        vecEnd,
        nodes,
        numNodes,
        8,
        nullptr,
        999999.88f,
        nullptr,
        999999.88f,
        false,
        NoForbiddenNode,
        false,
        false
    );

    route->Clear();
    for (int16 i = 0; i < numNodes; i++) {
        if (!route->IsFull()) {
            route->Add(nodes[i]);
        }
    }
}

// 0x44D960
void CPathFind::SetLinksBridgeLights(float fXMin, float fXMax, float fYMin, float fYMax, bool value) {
    const auto areaRect = CRect{ {fXMin, fYMin}, {fXMax, fYMax} };
    for (auto areaId = 0u; areaId < NUM_PATH_MAP_AREAS; areaId++) {
        if (!IsAreaLoaded(areaId)) {
            continue;
        }

        for (auto n = 0u; n < m_anNumCarPathLinks[areaId]; n++) {
            auto& node = GetCarPathLink({ areaId, n });
            if (areaRect.IsPointInside(node.GetNodeCoors())) {
                node.m_bridgeLights = value;
            }
        }
    }
}

namespace detail {
// NOTSA
CVector GetPosnBetweenNodesForScript(CPathNode* nodeA, CVector2D dir) {
    return nodeA->GetPosition() + CVector{dir.GetPerpLeft() * ((float)nodeA->m_nPathWidth / 16.f + 2.7f)};
}
};

// 0x4505E0
CVector CPathFind::FindNodeCoorsForScript(CNodeAddress address, bool* bFound) {
    const auto SetFound = [&](bool found) {
        if (bFound) {
            *bFound = found;
        }
    };
    if (!address.IsValid() || !IsAreaNodesAvailable(address)) {
        SetFound(false);
        return {};
    } else {
        SetFound(true);

        const auto node = GetPathNode(address);
        const auto nodePos = node->GetPosition();
 
        // If this node has a link return some kind of position between this and the first link
        if (node->m_nPathWidth && node->m_nNumLinks) {
            if (const auto firstLink = m_pNodeLinks[node->m_wBaseLinkId]) {
                if (const auto firstLinkedNode = GetPathNode(*firstLink)) {
                    auto dir = CVector2D{ firstLinkedNode->GetPosition() - nodePos }.Normalized();

                    // By negating here we invert the direction
                    dir = dir.x >= 0 ? dir : -dir;

                    return detail::GetPosnBetweenNodesForScript(node, dir);
                }
            }
        }

        // Otherwise just return this node's position
        return nodePos;
    }
}

// 0x450780
CVector CPathFind::FindNodeCoorsForScript(CNodeAddress nodeAddrA, CNodeAddress nodeAddrB, float& outHeadingDeg, bool* outFound) {
    const auto SetFound = [&](bool found) {
        if (outFound) {
            *outFound = found;
        }
    };
    if (nodeAddrA.IsValid() && nodeAddrB.IsValid() && AreNodeAreasLoaded({ nodeAddrA, nodeAddrB })) { // Inverted
        SetFound(true);

        const auto nodeA = GetPathNode(nodeAddrA);
        const auto posA  = nodeA->GetPosition();
        const auto dir   = CVector2D{ GetPathNode(nodeAddrB)->GetPosition() - posA }.Normalized();

        outHeadingDeg = RWRAD2DEG(dir.Heading());

        return nodeA->m_nPathWidth
            ? detail::GetPosnBetweenNodesForScript(nodeA, dir)
            : posA;
    } else {
        SetFound(false);
        return {};
    }
}

// 0x450560
void CPathFind::MarkRoadNodeAsDontWander(float x, float y, float z) {
    CVector pos = {x, y, z};
    auto node = FindNodeClosestToCoors(pos, PATH_TYPE_VEH, 999999.88f, 0, 0, 0, 0, 0);
    if (node.IsValid()) {
        m_pPathNodes[node.m_wAreaId][node.m_wNodeId].m_bDontWander = true;
    }
}

// 0x452820
void CPathFind::SwitchRoadsOffInAreaForOneRegion(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, bool bSwitchOff, bool bCars, int areaId, bool bBackToOriginal) {
    assert(areaId >= 0 && areaId < NUM_PATH_MAP_AREAS);

    if (!IsAreaLoaded(areaId)) {
        return;
    }

    auto start = bCars ? 0 : m_anNumVehicleNodes[areaId];
    auto end   = bCars ? m_anNumVehicleNodes[areaId] : m_anNumNodes[areaId];

    for (auto nodeIdx = start; nodeIdx < end; ++nodeIdx) {
        CPathNode& node     = m_pPathNodes[areaId][nodeIdx];
        const auto position = node.GetPosition();

        if (position.x < xMin || position.x > xMax || position.y < yMin || position.y > yMax || position.z < zMin || position.z > zMax) {
            continue;
        }
        if (!ThisNodeHasToBeSwitchedOff(&node) || node.m_isSwitchedOff == (bBackToOriginal ? node.m_isSwitchedOffOriginal : bSwitchOff)) {
            continue;
        }
        CPathNode* next1{};
        CPathNode* next2{};

        SwitchOffNodeAndNeighbours(&node, next1, &next2, bSwitchOff, bBackToOriginal);

        for (auto iter = next1; iter;) {
            SwitchOffNodeAndNeighbours(iter, iter, nullptr, bSwitchOff, bBackToOriginal);
        }
        for (auto iter = next2; iter;) {
            SwitchOffNodeAndNeighbours(iter, iter, nullptr, bSwitchOff, bBackToOriginal);
        }
    }
}

// NOTSA
CPathNode* CPathFind::GetPathNode(CNodeAddress address) {
    assert(address.IsValid());
    assert(IsAreaNodesAvailable(address));
    return &m_pPathNodes[address.m_wAreaId][address.m_wNodeId];
}

// notsa
CCarPathLinkAddress CPathFind::GetNaviLink(uint16 area, uint16 linkId) const {
    assert(area < NUM_PATH_MAP_AREAS);
    assert(linkId < m_anNumAddresses[area]);
    return m_pNaviLinks[area][linkId];
}

// NOTSA
bool CPathFind::FindNodeCoorsForScript(CVector& outPos, CNodeAddress addr) {
    bool valid{};
    outPos = FindNodeCoorsForScript(addr, &valid);
    return valid;
}

// 0x452F40
void CPathFind::LoadPathFindData(int32 areaId) {
    CTimer::Suspend();
    sprintf_s(gString, "data\\paths\\nodes%d.dat", areaId);
    auto* stream = RwStreamOpen(RwStreamType::rwSTREAMFILENAME, RwStreamAccessType::rwSTREAMREAD, gString);
    LoadPathFindData(stream, areaId);
    CTimer::Resume();
}

// 0x4529F0
void CPathFind::LoadPathFindData(RwStream* stream, int32 areaId) {
    RwStreamRead(stream, &m_anNumNodes[areaId],        sizeof(m_anNumNodes[areaId]));
    RwStreamRead(stream, &m_anNumVehicleNodes[areaId], sizeof(m_anNumVehicleNodes[areaId]));
    RwStreamRead(stream, &m_anNumPedNodes[areaId],     sizeof(m_anNumPedNodes[areaId]));
    RwStreamRead(stream, &m_anNumCarPathLinks[areaId], sizeof(m_anNumCarPathLinks[areaId]));
    RwStreamRead(stream, &m_anNumAddresses[areaId],    sizeof(m_anNumAddresses[areaId]));

    assert(m_anNumNodes[areaId] == m_anNumVehicleNodes[areaId] + m_anNumPedNodes[areaId]);

    auto numNodes = m_anNumNodes[areaId];
    if (numNodes) {
        m_pPathNodes[areaId] = new CPathNode[numNodes];
#ifdef NOTSA_NATIVE
        // The file has the 32 bit layout, while `CPathNode` is bigger on native (64 bit) builds - read through a mirror.
        std::vector<CPathNodeDisk> diskNodes(numNodes);
        RwStreamRead(stream, diskNodes.data(), sizeof(CPathNodeDisk) * numNodes);
        for (auto i = 0u; i < numNodes; ++i) {
            CPathNodeDiskToNode(diskNodes[i], m_pPathNodes[areaId][i]);
        }
#else
        RwStreamRead(stream, m_pPathNodes[areaId], sizeof(CPathNode) * numNodes);
#endif
    } else {
        m_pPathNodes[areaId] = new CPathNode[1];
    }

    auto numCarPathLinks = m_anNumCarPathLinks[areaId];
    if (numCarPathLinks) {
        m_pNaviNodes[areaId] = new CCarPathLink[numCarPathLinks];
        RwStreamRead(stream, m_pNaviNodes[areaId], sizeof(CCarPathLink) * numCarPathLinks);
    } else {
        m_pNaviNodes[areaId] = nullptr;
    }

    auto numAddresses = m_anNumAddresses[areaId];
    if (numAddresses) {
        auto numToAdd = numAddresses + NUM_DYNAMIC_LINKS_PER_AREA * 12;
        m_pNodeLinks[areaId] = new CNodeAddress[numToAdd];
        m_pNaviLinks[areaId] = new CCarPathLinkAddress[numAddresses];
        m_pLinkLengths[areaId] = new uint8[numToAdd];
        m_pPathIntersections[areaId] = new CPathIntersectionInfo[numToAdd];
        RwStreamRead(stream, m_pNodeLinks[areaId], sizeof(CNodeAddress) * numToAdd);
        RwStreamRead(stream, m_pNaviLinks[areaId], sizeof(CCarPathLinkAddress) * numAddresses);
        RwStreamRead(stream, m_pLinkLengths[areaId], sizeof(uint8) * numToAdd);
        RwStreamRead(stream, m_pPathIntersections[areaId], sizeof(CPathIntersectionInfo) * numToAdd);
    } else {
        m_pNodeLinks[areaId] = nullptr;
        m_pNaviLinks[areaId] = nullptr;
        m_pLinkLengths[areaId] = nullptr;
        m_pPathIntersections[areaId] = nullptr;
    }

    for (auto i = 0u; i < numNodes; ++i) {
        auto& node = m_pPathNodes[areaId][i];
        node.m_isSwitchedOffOriginal = node.m_isSwitchedOff;
    }

    for (auto i = 0u; i < m_nNumNodeSwitches; ++i) {
        auto& area = m_aNodeSwitches[i];
        SwitchRoadsOffInAreaForOneRegion(area.xMin, area.xMax, area.yMin, area.yMax, area.zMin, area.zMax, area.isOff, area.isCars, areaId, false);
    }
    for (auto i = 0u; i < NUM_DYNAMIC_LINKS_PER_AREA; ++i) {
        rng::fill(m_aDynamicLinksBaseIds[i], -1);
        rng::fill(m_aDynamicLinksIds[i], -1);
    }
}

// 0x44D0F0
void CPathFind::UnLoadPathFindData(int32 index) {
    delete[] m_pPathNodes[index];
    delete[] m_pNaviNodes[index];
    delete[] m_pNodeLinks[index];
    delete[] m_pNaviLinks[index];
    delete[] m_pLinkLengths[index];
    delete[] m_pPathIntersections[index];

    m_pPathNodes[index] = nullptr;
    m_pNaviNodes[index] = nullptr;
    m_pNodeLinks[index] = nullptr;
    m_pNaviLinks[index] = nullptr;
    m_pLinkLengths[index] = nullptr;
    m_pPathIntersections[index] = nullptr;
}

// 0x44DE00
void CPathFind::LoadSceneForPathNodes(CVector point) {
    rng::fill(ToBeStreamed, false);
    MarkRegionsForCoors(point, 350.f);
    for (const auto [areaId, load] : rngv::enumerate(ToBeStreamed)) {
        if (load) {
            CStreaming::RequestModel(DATToModelId((int32)areaId), STREAMING_DEFAULT);
        }
    }
}

// 0x450DE0
bool CPathFind::IsWaterNodeNearby(CVector position, float radius) {
    for (auto areaId = 0u; areaId < NUM_PATH_MAP_AREAS; areaId++) {
        for (const auto& node : GetPathNodesInArea(areaId, PATH_TYPE_VEH)) {
            if (node.m_bWaterNode) {
                if ((node.GetPosition() - position).SquaredMagnitude() <= sq(radius)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// 0x44EFC0
float CPathFind::CalcRoadDensity(float x, float y) {
    float density = 0.f;
    for (auto areaId = 0u; areaId < NUM_PATH_MAP_AREAS; areaId++) {
        if (!m_pPathNodes[areaId]) {
            continue;
        }

        const auto numNodes = (int32)m_anNumVehicleNodes[areaId];
        if (numNodes <= 0) {
            continue;
        }

        for (auto nodeIdx = 0; nodeIdx < numNodes; nodeIdx++) {
            const CPathNode& node    = m_pPathNodes[areaId][nodeIdx];
            const CVector    nodePos = node.GetPosition();

            // Only consider nodes that are close enough on both axes
            if (std::abs(nodePos.x - x) >= 80.f || std::abs(nodePos.y - y) >= 80.f) {
                continue;
            }

            for (auto linkIdx = 0u; linkIdx < node.m_nNumLinks; linkIdx++) {
                const auto linkId = node.m_wBaseLinkId + linkIdx;

                const CNodeAddress linkedAddr = m_pNodeLinks[areaId][linkId];
                if (!m_pPathNodes[linkedAddr.m_wAreaId]) {
                    continue;
                }
                const CVector linkedPos = m_pPathNodes[linkedAddr.m_wAreaId][linkedAddr.m_wNodeId].GetPosition();
                const float   dist      = std::sqrt(sq(nodePos.x - linkedPos.x) + sq(nodePos.y - linkedPos.y));

                // The navi node's area has to be loaded too
                const CCarPathLinkAddress naviAddr = m_pNaviLinks[areaId][linkId];
                if (!m_pPathNodes[naviAddr.m_wAreaId]) {
                    continue;
                }
                const CCarPathLink& navi = m_pNaviNodes[naviAddr.m_wAreaId][naviAddr.m_wCarPathLinkId];
                density += (float)navi.m_numSameDirLanes * dist + (float)navi.m_numOppositeDirLanes * dist;
            }
        }
    }
    return density * 0.0004f;
}

// 0x44F190
float CPathFind::CalcDistToAnyConnectingLinks(CPathNode* node, CVector pos) {
    float minDistSq = 999999.9f;
    for (auto linkIdx = 0u; linkIdx < node->m_nNumLinks; linkIdx++) {
        const CNodeAddress linkedAddr = m_pNodeLinks[node->m_wAreaId][node->m_wBaseLinkId + linkIdx];
        if (!m_pPathNodes[linkedAddr.m_wAreaId]) {
            continue;
        }

        const float distSq = CCollision::DistToLineSqr(
            node->GetPosition(),
            m_pPathNodes[linkedAddr.m_wAreaId][linkedAddr.m_wNodeId].GetPosition(),
            pos
        );
        if (distSq <= minDistSq) {
            minDistSq = distSq;
        }
    }
    return std::sqrt(minDistSq);
}

// 0x44F2C0
void CPathFind::FindNodeClosestInRegion(CNodeAddress* outAddress, uint16 areaId, CVector pos, uint8 nodeType, float* outDist, bool bLowTraffic, bool bUnkn, bool bBoats, bool bUnused) {
    if (!m_pPathNodes[areaId]) {
        return;
    }

    int32 start, end;
    switch (nodeType) {
    case PATH_TYPE_VEH:
        start = 0;
        end   = (int32)m_anNumVehicleNodes[areaId];
        break;
    case PATH_TYPE_PED:
        start = (int32)m_anNumVehicleNodes[areaId];
        end   = (int32)m_anNumNodes[areaId];
        break;
    default:
        start = end = nodeType;
        break;
    }

    for (auto nodeIdx = start; nodeIdx < end; nodeIdx++) {
        CPathNode& node = m_pPathNodes[areaId][nodeIdx];

        if (bLowTraffic && node.m_isSwitchedOff) {
            continue;
        }
        if (bUnkn && node.unk1) {
            continue;
        }
        if (bBoats != node.m_bWaterNode) {
            continue;
        }

        const CVector nodePos   = node.GetPosition();
        const float   roughDist = (std::abs(nodePos.x - pos.x) + std::abs(nodePos.y - pos.y) + std::abs(nodePos.z - pos.z) * 3.f) * 0.3f;
        if (roughDist >= *outDist) {
            continue;
        }

        const float dist = CalcDistToAnyConnectingLinks(&node, pos) * 0.2f + roughDist;
        if (dist < *outDist) {
            *outDist              = dist;
            outAddress->m_wAreaId = areaId;
            outAddress->m_wNodeId = (uint16)nodeIdx;
        }
    }
}

// 0x44F460
CNodeAddress CPathFind::FindNodeClosestToCoors(
    CVector pos,
    ePathType nodeType,
    float maxDistance,
    uint16 unk2,
    int32 unk3,
    uint16 unk4,
    uint16 bBoatsOnly,
    int32 unk6
) {
    CNodeAddress found; // `.m_wAreaId` is `-1` (Not found) by default
    float        bestDist = maxDistance;

    const auto xRegion = (int32)FindXRegionForCoors(pos.x);
    const auto yRegion = (int32)FindYRegionForCoors(pos.y);

    // Distance from `pos` to the closest border of the region it's in
    float borderDist = pos.x - ((float)xRegion * 750.f - 3000.f);
    borderDist = std::min(borderDist, ((float)(xRegion + 1) * 750.f - 3000.f) - pos.x);
    borderDist = std::min(borderDist, pos.y - ((float)yRegion * 750.f - 3000.f));
    borderDist = std::min(borderDist, ((float)(yRegion + 1) * 750.f - 3000.f) - pos.y);

    FindNodeClosestInRegion(
        &found,
        (uint16)(xRegion + yRegion * NUM_PATH_MAP_AREA_X),
        pos,
        (uint8)nodeType,
        &bestDist,
        unk2,
        unk4,
        bBoatsOnly,
        unk6
    );

    // Expand the search in rings around the original region (At most 4) until the closest node found is farther than the next ring's border
    if (borderDist < bestDist) {
        const int32 dYX = yRegion - xRegion;
        int32       lo  = xRegion;     // `xRegion - ring + 1` for ring `n`
        int32       hi  = xRegion + 1; // `xRegion + ring` for ring `n`
        int32       top = yRegion;

        for (int32 ring = 1; ring < 5; ring++) {
            top--; // `yRegion - ring`
            const int32 bottom = dYX + hi;

            // Left column
            if (lo - 1 >= 0 && lo - 1 < NUM_PATH_MAP_AREA_X) {
                for (auto y = top; y <= bottom; y++) {
                    if (y >= 0 && y < NUM_PATH_MAP_AREA_Y) {
                        FindNodeClosestInRegion(&found, (uint16)(lo - 1 + y * NUM_PATH_MAP_AREA_X), pos, (uint8)nodeType, &bestDist, unk2, unk4, bBoatsOnly, unk6);
                    }
                }
            }

            // Right column
            if (hi >= 0 && hi < NUM_PATH_MAP_AREA_X) {
                for (auto y = top; y <= bottom; y++) {
                    if (y >= 0 && y < NUM_PATH_MAP_AREA_Y) {
                        FindNodeClosestInRegion(&found, (uint16)(hi + y * NUM_PATH_MAP_AREA_X), pos, (uint8)nodeType, &bestDist, unk2, unk4, bBoatsOnly, unk6);
                    }
                }
            }

            // Top row
            if (top >= 0 && top < NUM_PATH_MAP_AREA_Y) {
                for (auto x = lo; x < hi; x++) {
                    if (x >= 0 && x < NUM_PATH_MAP_AREA_X) {
                        FindNodeClosestInRegion(&found, (uint16)(x + top * NUM_PATH_MAP_AREA_X), pos, (uint8)nodeType, &bestDist, unk2, unk4, bBoatsOnly, unk6);
                    }
                }
            }

            // Bottom row
            if (bottom >= 0 && bottom < NUM_PATH_MAP_AREA_Y) {
                for (auto x = lo; x < hi; x++) {
                    if (x >= 0 && x < NUM_PATH_MAP_AREA_X) {
                        FindNodeClosestInRegion(&found, (uint16)(x + bottom * NUM_PATH_MAP_AREA_X), pos, (uint8)nodeType, &bestDist, unk2, unk4, bBoatsOnly, unk6);
                    }
                }
            }

            borderDist += 750.f;
            if (bestDist < borderDist) {
                break;
            }
            lo--;
            hi++;
        }
    }

    // Also search the interior areas
    if (!unk6) {
        for (auto areaId = NUM_PATH_MAP_AREAS; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
            FindNodeClosestInRegion(&found, (uint16)areaId, pos, (uint8)nodeType, &bestDist, unk2, unk4, bBoatsOnly, 0);
        }
    }

    return found;
}

// 0x450A60
void CPathFind::UpdateStreaming(bool bForceStreaming) {
    ZoneScoped;

    // The time thingy I think is some kind of `% 512`, not sure yet, will have to figure it out.
    if (!s_bLoadPathsNeeded && !bForceStreaming && (CTimer::GetTimeInMS() ^ CTimer::GetPreviousTimeInMS()) < 512) {
        return;
    }

    rng::fill(ToBeStreamed, false);
    std::array<bool, NUM_PATH_MAP_AREAS> ToBeStreamedForScript{};

    // Mark areas around the player
    if (FindPlayerPed()) {
        MarkRegionsForCoors(FindPlayerCoors(), 350.f);
    }

    // Mark areas requested by `SetPathsNeededAtPosition`
    if (s_bLoadPathsNeeded) {
        MarkRegionsForCoors(s_pathsNeededPosn, 300.f);
        s_bLoadPathsNeeded = false;
    }

    // Mark paths around some specific mission vehicles
    for (const auto& veh : GetVehiclePool()->GetAllValid()) {
        if (!veh.IsMissionVehicle()) {
            continue;
        }

        switch (veh.m_nVehicleSubType) {
        case VEHICLE_TYPE_HELI:
        case VEHICLE_TYPE_PLANE:
        case VEHICLE_TYPE_BOAT:
        case VEHICLE_TYPE_TRAIN:
        case VEHICLE_TYPE_FPLANE:
            break;
        default:
            MarkRegionsForCoors(veh.GetPosition(), 300.f);
        }
    }

    // Mark areas inside load request rect
    if (m_loadAreaRequestPending) {
        IterAreasTouchingRect(
            { m_loadAreaRequestMinX, m_loadAreaRequestMinY, m_loadAreaRequestMaxX, m_loadAreaRequestMaxY },
            [&, this](auto areaId) {
                ToBeStreamed[areaId] = ToBeStreamedForScript[areaId] = true;
                return true;
            }
        );
    }

    // Load/unload areas as per `ToBeStreamed`
    for (const auto [areaId, shouldBeLoaded] : rngv::enumerate(ToBeStreamed)) {
        if (shouldBeLoaded) {
            if (!IsAreaLoaded(areaId)) {
                CStreaming::RequestModel(
                    DATToModelId(areaId),
                    ToBeStreamedForScript[areaId]
                        ? STREAMING_MISSION_REQUIRED
                        : STREAMING_KEEP_IN_MEMORY
                );
                NOTSA_LOG_DEBUG("Requested area: {}", (int)areaId);
            }
        } else if (IsAreaLoaded(areaId)) {
            CStreaming::RemoveModel(DATToModelId(areaId));
            NOTSA_LOG_DEBUG("Removed area: {}", (int)areaId);
        }
    }
}

// 0x44DE80
void CPathFind::StartNewInterior(int32 interiorNum) {
    InteriorIDBeingBuilt = interiorNum;
    bInteriorBeingBuilt = true;
    NumNodesGiven = 0;
    NumLinksToExteriorNodes = 0;

    // BUG: Possible endless loop if 8 interiors are loaded i think
    NewInteriorSlot = 0;
    while (m_interiorIDs[NewInteriorSlot] != (uint32)-1) {
        NewInteriorSlot++;
        assert(NewInteriorSlot < 8);
    }
}

// 0x450E90
CNodeAddress CPathFind::AddNodeToNewInterior(
    float x,
    float y,
    float z,
    bool bDontWander,
    int8 con0,
    int8 con1,
    int8 con2,
    int8 con3,
    int8 con4,
    int8 con5
) {
    const auto idx = NumNodesGiven++;
    XCoorGiven[idx] = x;
    YCoorGiven[idx] = y;
    ZCoorGiven[idx] = z;
    aDontWanderGiven[idx] = bDontWander;
    rng::copy(std::array{ con0, con1, con2, con3, con4, con5 }, ConnectsToGiven[idx].begin());
    return { (uint16)(NUM_PATH_MAP_AREAS + NewInteriorSlot), (uint16)idx };
}

// 0x451300 unused
CNodeAddress CPathFind::ReturnInteriorNodeIndex(int32 unkn, uint32 intId, int16 nodeId) {
    for (auto i = 0; i < NUM_PATH_INTERIOR_AREAS; ++i) {
        if (m_interiorIDs[i] == intId) {
            return CNodeAddress(NUM_PATH_MAP_AREAS + i, nodeId);
        }
    }
    return {};
}

// 0x451350
CCarPathLinkAddress CPathFind::FindLinkBetweenNodes(CNodeAddress nodeAddrA, CNodeAddress nodeAddrB) {
    if (AreNodeAreasLoaded({ nodeAddrA, nodeAddrB })) {
        const auto nodeA = GetPathNode(nodeAddrA);
        for (auto i = 0u; i < nodeA->m_nNumLinks; i++) {
            const auto linkIdx = nodeA->m_wBaseLinkId + i;
            if (m_pNodeLinks[nodeA->m_wAreaId][linkIdx] == nodeAddrB) {
                return m_pNaviLinks[nodeA->m_wAreaId][linkIdx];
            }
        }
    }
    return {};
}

// 0x4513F0
CVector CPathFind::FindParkingNodeInArea(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
    // 0x977B9C - Which found node to return, so successive calls cycle through all of them
    static int32 s_NodeIndex = 0;

    CVector firstFound{}, indexedFound{};
    bool    haveFirst   = false;
    bool    haveIndexed = false;
    int32   numFound    = 0;

    for (auto areaId = 0u; areaId < NUM_PATH_MAP_AREAS; areaId++) {
        if (!IsAreaLoaded(areaId) || (int32)m_anNumVehicleNodes[areaId] <= 0) {
            continue;
        }

        for (auto nodeIdx = 0; nodeIdx < (int32)m_anNumVehicleNodes[areaId]; nodeIdx++) {
            const auto& node = m_pPathNodes[areaId][nodeIdx];
            const auto  pos  = node.GetPosition();

            if (pos.x <= minX || pos.x >= maxX) {
                continue;
            }
            if (pos.y <= minY || pos.y >= maxY) {
                continue;
            }
            if (pos.z <= minZ || pos.z >= maxZ) {
                continue;
            }
            if (node.m_nBehaviourType != 2) { // Not a parking node
                continue;
            }

            if (numFound == 0) {
                firstFound = pos;
                haveFirst  = true;
            }
            if (numFound == s_NodeIndex) {
                indexedFound = pos;
                haveIndexed  = true;
            }
            numFound++;
        }
    }

    if (numFound <= ++s_NodeIndex) {
        s_NodeIndex = 0;
    }

    if (!haveFirst) {
        return {};
    }

    return haveIndexed ? indexedFound : firstFound;
}

// 0x450F30
CNodeAddress CPathFind::FindNearestExteriorNodeToInteriorNode(int32 interiorId) {
    return FindNodeClosestToCoors(
        { XCoorGiven[interiorId], YCoorGiven[interiorId], ZCoorGiven[interiorId] },
        ePathType::PATH_TYPE_PED,
        3.f,
        0,
        0,
        0,
        0,
        true
    );
}

// 0x44E000
void CPathFind::AddDynamicLinkBetween2Nodes_For1Node(CNodeAddress first, CNodeAddress second) {
    assert(IsAreaNodesAvailable(first));

    auto& firstPathInfo = m_pPathNodes[first.m_wAreaId][first.m_wNodeId];
    auto numAddresses = m_anNumAddresses[first.m_wAreaId];

    uint32 firstLinkId;
    if (static_cast<uint32>(firstPathInfo.m_wBaseLinkId) >= numAddresses)
        firstLinkId = firstPathInfo.m_wBaseLinkId;
    else {
        auto* nodeLink = &m_pNodeLinks[first.m_wAreaId][numAddresses];
        auto linkCounter = 0u;
        while (!nodeLink->IsValid()) {
            nodeLink += 12; // No clue why we jump 12 objects each time (Search: MAGIC_NUM_12)
            ++linkCounter;
        }

        firstLinkId = numAddresses + 12 * linkCounter;
        for (auto i = 0u; i < firstPathInfo.m_nNumLinks; ++i) {
            m_pNodeLinks[first.m_wAreaId][firstLinkId + i] = m_pNodeLinks[first.m_wAreaId][firstPathInfo.m_wBaseLinkId + i];
            m_pLinkLengths[first.m_wAreaId][firstLinkId + i] = m_pLinkLengths[first.m_wAreaId][firstPathInfo.m_wBaseLinkId + i];
            m_pPathIntersections[first.m_wAreaId][firstLinkId + i] = m_pPathIntersections[first.m_wAreaId][firstPathInfo.m_wBaseLinkId + i];
        }

        if (first.m_wAreaId < NUM_PATH_MAP_AREAS) {
            auto& linkInfo = m_aDynamicLinksBaseIds[first.m_wAreaId];
            for (auto i = 0u; i < NUM_DYNAMIC_LINKS_PER_AREA; ++i) {
                if (linkInfo[i] == -1) {
                    m_aDynamicLinksBaseIds[first.m_wAreaId][i] = firstPathInfo.m_wBaseLinkId;
                    m_aDynamicLinksIds[first.m_wAreaId][i] = firstLinkId;
                    break;
                }
            }
        }
    }

    m_pNodeLinks[first.m_wAreaId][firstLinkId + firstPathInfo.m_nNumLinks] = second;
    m_pLinkLengths[first.m_wAreaId][firstLinkId + firstPathInfo.m_nNumLinks] = 5;
    m_pPathIntersections[first.m_wAreaId][firstLinkId + firstPathInfo.m_nNumLinks].Clear();
    firstPathInfo.m_nNumLinks++;
    firstPathInfo.m_wBaseLinkId = firstLinkId;
}

// 0x44D230
bool CPathFind::These2NodesAreAdjacent(CNodeAddress nodeAddress1, CNodeAddress nodeAddress2) {
    const auto node1 = GetPathNode(nodeAddress1);
    for (auto i = 0u; i < node1->m_nNumLinks; i++) {
        if (m_pNodeLinks[node1->m_wAreaId][node1->m_wBaseLinkId + i] == nodeAddress2) {
            return true;
        }
    }
    return false;
}

// 0x44FCE0
CNodeAddress CPathFind::FindNodeClosestToCoorsFavourDirection(CVector pos, ePathType nodeType, CVector2D dir) {
    dir = dir.Normalized(); // In-place normalize
    
    CNodeAddress closest{};
    float        scoreOfClosest{std::numeric_limits<float>::max()};
    for (auto areaId{ 0u }; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
        for (const auto& node : GetPathNodesInArea(areaId, nodeType)) { // NOTE: Function takes care of checking whenever the area is loaded
            const auto playerToNodeDirection = node.GetPosition() - pos;

            const auto dotScore = (abs(playerToNodeDirection) * CVector { 1.f, 1.f, 3.f }).ComponentwiseSum();
            if (dotScore >= scoreOfClosest) {
                continue;
            }

            const auto score = dotScore - (dir.Dot(CVector2D{ playerToNodeDirection }.Normalized()) - 1.f) * 20.f;
            if (score > scoreOfClosest) {
                continue;
            }

            scoreOfClosest = score;
            closest = node.GetAddress();
        }
    }
    return closest;
}

// 0x5D34C0
bool CPathFind::Save() {
    CGenericGameStorage::SaveDataToWorkBuffer(m_nNumNodeSwitches);
    for (auto& area : std::span{ m_aNodeSwitches, m_nNumNodeSwitches }) {
        CGenericGameStorage::SaveDataToWorkBuffer(area);
    }
    return true;
}

// 0x5D3500
bool CPathFind::Load() {
    CGenericGameStorage::LoadDataFromWorkBuffer(m_nNumNodeSwitches);
    for (auto& area : std::span{ m_aNodeSwitches, m_nNumNodeSwitches }) {
        CGenericGameStorage::LoadDataFromWorkBuffer(area);
    }
    return true;
}

bool CPathFind::AreNodeAreasLoaded(const std::initializer_list<CNodeAddress>& addrs) const {
    return rng::all_of(addrs, [this](auto&& addr) { return IsAreaNodesAvailable(addr); });
}

// 0x44DCD0
void CPathFind::SetPathsNeededAtPosition(const CVector& posn) {
    s_pathsNeededPosn = posn;
    s_bLoadPathsNeeded = true;
}

namespace detail {
constexpr size_t RegionValueOf(float p, size_t nareas) {
    return std::clamp((uint32)((p + 3000.f) / (6000.f / (float)nareas)), 0u, (uint32)nareas - 1);
}
}; // namespace detail

size_t CPathFind::FindXRegionForCoors(float x) const {
    return detail::RegionValueOf(x, NUM_PATH_MAP_AREA_X);
}

size_t CPathFind::FindYRegionForCoors(float y) const {
    return detail::RegionValueOf(y, NUM_PATH_MAP_AREA_Y);
}

// 0x44DB60
void CPathFind::MarkRegionsForCoors(CVector pos, float radius) {
    // HACK: Since the below function isnt `static` (TODO...) we gotta use the class instance here...
    ThePaths.IterAreasTouchingRect(
        { pos, radius },
        [](auto areaId) {
            ToBeStreamed[areaId] = true;
            return true;
        }
    );
}

// 0x44D3E0 - Moved to CPathNode
bool CPathFind::ThisNodeHasToBeSwitchedOff(CPathNode* node) {
    return node->HasToBeSwitchedOff();
}

// 0x4504F0
// This function is only called from `SwitchOffNodeAndNeighbours` but when unhooked
// it doesn't spoil `eax` which makes the former crash
// so hopefully the `__asm mov eax, this` fixes it
// If not just lock both :D
size_t CPathFind::CountNeighboursToBeSwitchedOff(const CPathNode& node) {
    const auto ret = (size_t)rng::count_if(GetNodeLinkedNodes(node), &CPathNode::HasToBeSwitchedOff);
#ifndef NOTSA_NATIVE // `eax` is only preserved because of the x86 calling convention of the exe
    __asm mov eax, this // It has to be `this`
#endif
    return ret;
}

// 0x450320
float CPathFind::FindNodeOrientationForCarPlacement(CNodeAddress address) {
    if (!IsAreaNodesAvailable(address)) {
        return 0.f;
    }

    const auto& node     = *GetPathNode(address);
    const auto  numLinks = (int32)node.m_nNumLinks;
    if (!numLinks) {
        return 0.f;
    }

    // Find the first link with lanes going in the direction of the passed node (The last link is never tested by the original)
    int32 linkNum = 0;
    for (; linkNum < numLinks - 1; linkNum++) {
        const auto naviAddr = m_pNaviLinks[address.m_wAreaId][node.m_wBaseLinkId + linkNum];
        if (!IsAreaLoaded(naviAddr.m_wAreaId)) {
            continue;
        }

        const auto& navi = m_pNaviNodes[naviAddr.m_wAreaId][naviAddr.m_wCarPathLinkId];
        // If the navi node is attached to the passed node the lanes go in its direction, otherwise against it
        const auto numLanes = navi.m_attachedTo == address
            ? navi.m_numSameDirLanes
            : navi.m_numOppositeDirLanes;
        if (numLanes) {
            break;
        }
    }

    const auto targetAddr = m_pNodeLinks[address.m_wAreaId][node.m_wBaseLinkId + linkNum];
    if (!IsAreaNodesAvailable(targetAddr)) {
        return 0.f;
    }

    const CVector2D dir = GetPathNode(targetAddr)->GetPosition() - node.GetPosition(); // Z is dropped, as in the original
    return RWRAD2DEG(dir.Normalized().Heading());
}

// 0x452090
void CPathFind::Find2NodesForCarCreation(CVector pos, CNodeAddress* outAddress1, CNodeAddress* outAddress2, bool bLowTraffic) {
    CNodeAddress nodes[4]; // Note: The original only initializes the area IDs of these

    RecordNodesClosestToCoors(pos, PATH_TYPE_VEH, (int32)std::size(nodes), nodes, 999999.88f, bLowTraffic, false, false, true);

    if (!nodes[0].IsAreaValid()) { // No nodes found
        outAddress1->m_wAreaId = (uint16)-1;
        outAddress2->m_wAreaId = (uint16)-1;
        return;
    }

    *outAddress1 = nodes[0];

    // Pick the first node that isn't directly connected to the first one
    for (auto i = 1u; i < std::size(nodes); i++) {
        if (nodes[i].IsAreaValid() && !These2NodesAreAdjacent(nodes[0], nodes[i])) {
            *outAddress2 = nodes[i];
            return;
        }
    }
}

// 0x452160
void CPathFind::SwitchOffNodeAndNeighbours(CPathNode* node, CPathNode*& outNext1, CPathNode** outNext2, bool bWhatToSwitchTo, bool bBackToOriginal) {
    node->m_isSwitchedOff = bBackToOriginal ? node->m_isSwitchedOffOriginal : bWhatToSwitchTo;
   
    outNext1 = nullptr;
    if (outNext2) {
        *outNext2 = nullptr;
    }

    if (CountNeighboursToBeSwitchedOff(*node) > 2) {
        return;
    }

    for (auto& linked : GetNodeLinkedNodes(*node)) {
        if (!linked.HasToBeSwitchedOff()) {
            continue;
        }
        if (linked.m_isSwitchedOff == bWhatToSwitchTo) {
            continue;
        }
        if (CountNeighboursToBeSwitchedOff(*node) > 2) {
            continue;
        }
        if (!outNext1) {
            outNext1 = &linked;
        }
#ifdef FIX_BUGS // Above it was checked whenever it's set so I assume this was a bug
        else if (outNext2) // Don't get confused, `outNext2` is a ptr to a ptr
#else
        else
#endif
        {
            *outNext2 = &linked;
        }
    }
}

/*!
* @addr 0x450FB0
* @brief Adds a link from the node at index `fromNodeIdx` to the node at `toNodeIdx` of the interior currently being built.
*
* Retail also computes the link's length from the (decompressed) distance between both nodes (Truncated, clamped to `[1, 255]`),
* and bumps the link counter of the node the link originates from.
*/
static void SetNewInteriorNodeLink(CPathFind& paths, uint16 areaId, int32 fromNodeIdx, int32 toNodeIdx, int32& nextLinkId) {
    paths.m_pNodeLinks[areaId][nextLinkId] = CNodeAddress{ areaId, (uint16)toNodeIdx };

    const auto& fromNode = paths.m_pPathNodes[areaId][fromNodeIdx];
    const auto& toNode   = paths.m_pPathNodes[areaId][toNodeIdx];

    // Retail clamps the distance to 255 *before* converting it to an int, and bumps lengths below 1 to 1
    const auto dist = std::min((toNode.GetPosition() - fromNode.GetPosition()).Magnitude(), 255.f);
    paths.m_pLinkLengths[areaId][nextLinkId] = (uint8)std::max(1, (int32)dist);
    paths.m_pPathIntersections[areaId][nextLinkId].Clear();

    paths.m_pPathNodes[areaId][fromNodeIdx].m_nNumLinks++;

    nextLinkId++;
}

// 0x452270
void CPathFind::CompleteNewInterior(CNodeAddress* outAddress) {
    if (outAddress) {
        outAddress->ResetAreaId(); // Retail only invalidates the area id of the out address
    }

    if (!NumNodesGiven) {
        bInteriorBeingBuilt = false;
        return;
    }

    const auto intSlot  = NewInteriorSlot;
    const auto areaId   = (uint16)(NUM_PATH_MAP_AREAS + intSlot);
    const auto numNodes = (int32)NumNodesGiven;

    // Every node of the new interior shares the same flood fill group.
    // Retail uses `100 + interiorSlot`, unless a node of this interior was already linked to an exterior node.
    uint8 floodFillGroup = (uint8)(100 + intSlot);
    if (NumLinksToExteriorNodes > 0) {
        const auto& extNodeAddr = aExteriorNodeLinkedTo[0];
        floodFillGroup = m_pPathNodes[extNodeAddr.m_wAreaId][extNodeAddr.m_wNodeId].m_nFloodFill;
    }

    m_interiorIDs[intSlot] = (uint32)InteriorIDBeingBuilt;

    // Create the nodes
    m_pPathNodes[areaId] = static_cast<CPathNode*>(CMemoryMgr::Malloc(sizeof(CPathNode) * numNodes));
    for (auto i = 0; i < numNodes; i++) {
        auto& node = m_pPathNodes[areaId][i];

        node.m_next = nullptr; // notsa: Retail leaves these uninitialized (These nodes aren't added to the hash table)
        node.m_prev = nullptr;

        node.m_vPos                = CompressedLargeVector{ CVector{ XCoorGiven[i], YCoorGiven[i], ZCoorGiven[i] } };
        node.m_wNodeId             = (uint16)i;
        node.m_wAreaId             = areaId;
        node.m_nPathWidth          = 0;
        node.m_nFloodFill          = floodFillGroup;

        node.m_nNumLinks           = 0;
        node.m_onDeadEnd           = false;
        node.m_isSwitchedOff       = true;
        node.m_bRoadBlocks         = false;
        node.m_bWaterNode          = false;

        node.m_isSwitchedOffOriginal = true;
        node.unk1                  = false;
        node.m_bDontWander         = aDontWanderGiven[i];
        node.unk2                  = true;
        node.m_nSpeedLimit         = 0;
        node.unk3                  = false; // notsa: Retail doesn't initialize these 2 (Unused)
        node.unk4                  = false;

        node.m_nSpawnProbability   = 0xF;
        node.m_nBehaviourType      = 0;

        node.m_totalDistFromOrigin = SHRT_MAX - 1;
    }

    // Make the links bidirectional (Retail only requires them to be specified one way)
    for (auto i = 0; i < numNodes; i++) {
        for (auto k = 0u; k < 6; k++) {
            const auto linkedNodeIdx = ConnectsToGiven[i][k];
            if (linkedNodeIdx < 0) {
                continue;
            }

            const auto reverseLinks = std::span{ ConnectsToGiven[linkedNodeIdx] }.first(6);
            if (rng::contains(reverseLinks, (int8)i)) {
                continue;
            }
            if (const auto freeSlot = rng::find(reverseLinks, (int8)-1); freeSlot != reverseLinks.end()) {
                *freeSlot = (int8)i;
            }
        }
    }

    // Count the resulting links (Each of them takes up an address)
    int32 numAddresses = 0;
    for (auto i = 0; i < numNodes; i++) {
        for (auto k = 0u; k < 6; k++) {
            if (ConnectsToGiven[i][k] >= 0) {
                numAddresses++;
            }
        }
    }

    // Allocate the links (Plus the extra addresses dynamic links can use)
    const auto numLinkSlots = numAddresses + NUM_DYNAMIC_LINKS_PER_AREA * 12;
    m_pNodeLinks[areaId]         = static_cast<CNodeAddress*>(CMemoryMgr::Malloc(sizeof(CNodeAddress) * numLinkSlots));
    m_pLinkLengths[areaId]       = static_cast<uint8*>(CMemoryMgr::Malloc(numLinkSlots));
    m_pPathIntersections[areaId] = static_cast<CPathIntersectionInfo*>(CMemoryMgr::Malloc(sizeof(CPathIntersectionInfo) * numLinkSlots));

    // Create the links of every node
    int32 nextLinkId = 0;
    for (auto i = 0; i < numNodes; i++) {
        m_pPathNodes[areaId][i].m_wBaseLinkId = (int16)nextLinkId;
        m_pPathNodes[areaId][i].m_nNumLinks   = 0;
        for (auto k = 0u; k < 6; k++) {
            const auto linkedNodeIdx = ConnectsToGiven[i][k];
            if (linkedNodeIdx < 0) {
                continue;
            }
            SetNewInteriorNodeLink(*this, areaId, i, linkedNodeIdx, nextLinkId);
        }
    }

    // Invalidate the extra addresses (Only the area id is written, as in retail)
    for (auto i = numAddresses; i < numLinkSlots; i++) {
        m_pNodeLinks[areaId][i].m_wAreaId = (uint16)-1;
    }

    m_anNumNodes[areaId]          = (uint32)numNodes;
    m_anNumVehicleNodes[areaId]   = 0;
    m_anNumPedNodes[areaId]       = (uint32)numNodes;
    m_anNumCarPathLinks[areaId]   = 0;
    m_anNumAddresses[areaId]      = (uint32)numAddresses;

    // Link the interior nodes that were linked to exterior nodes while the interior was being built
    for (auto i = 0; i < NumLinksToExteriorNodes; i++) {
        const CNodeAddress intNodeAddr{ areaId, (uint16)aInteriorNodeLinkedToExterior[i] };
        const auto         extNodeAddr = aExteriorNodeLinkedTo[i];
        AddDynamicLinkBetween2Nodes_For1Node(intNodeAddr, extNodeAddr);
        AddDynamicLinkBetween2Nodes_For1Node(extNodeAddr, intNodeAddr);
    }

    bInteriorBeingBuilt = false;
}

// 0x44DF30
void CPathFind::AddInteriorLinkToExternalNode(int32 interiorNodeIdx, CNodeAddress externalNodeAddr) {
    const auto idx = NumLinksToExteriorNodes++;
    aInteriorNodeLinkedToExterior[idx] = interiorNodeIdx;
    aExteriorNodeLinkedTo[idx] = externalNodeAddr;
}

// 0x44DF60
void CPathFind::RemoveInteriorLinks(uint32 intIdx) {
    // Remove every link (In both directions) that points to the given interior node
    for (auto i = 0u; i < NumNodesGiven; i++) {
        for (auto k = 0u; k < 6; k++) {
            if (i == intIdx || (int32)ConnectsToGiven[i][k] == (int32)intIdx) {
                ConnectsToGiven[i][k] = -1;
            }
        }
    }
}

// 0x44E1A0
void CPathFind::RemoveInterior(uint32 intId) {
    for (auto intSlot = 0u; intSlot < NUM_PATH_INTERIOR_AREAS; intSlot++) {
        const auto intSlotAreaId = NUM_PATH_MAP_AREAS + intSlot;

        if (m_interiorIDs[intSlot] != intId) {
            continue;
        }

        for (auto areaId = 0u; areaId < NUM_TOTAL_PATH_NODE_AREAS; areaId++) {
            for (auto& node : GetPathNodesInArea(areaId, PATH_TYPE_PED)) {
                // I assume this checks if the link is an interior link?
                if ((int32)node.m_wBaseLinkId < (int32)m_anNumAddresses[areaId]) {
                    continue;
                }

                // Remove all link of this node that point to a node int the current interior
                bool foundNodeFromOtherInt{}, foundNodeFromThisInt{};
                (void)rng::remove_if(GetNodeLinkedNodes(node, false), [&](CPathNode& linkedNode) {
                    if (linkedNode.m_wAreaId == intSlotAreaId) {
                        node.m_nNumLinks--;
                        foundNodeFromOtherInt = true;
                        return true;
                    } else if (linkedNode.m_wAreaId >= NUM_PATH_MAP_AREAS) {
                        foundNodeFromThisInt = true;
                    }
                    return false;
                });

                // If we found a linked node and there was no other node from another interior
                if (foundNodeFromOtherInt || !foundNodeFromThisInt) {
                    continue;
                }

                // TODO: Magic number `12` (Search: MAGIC_NUM_12)
                // Null out all links of this node
                rng::fill(std::span{ &m_pNodeLinks[node.m_wAreaId][node.m_wBaseLinkId], 12 }, CNodeAddress{});

                // Delete dynamic link of this area
                // Honestly, this doesn't make much sense... As in, I don't think these are dynamic areas? We'll see.. TODO
                const auto& dynLinks  = m_aDynamicLinksIds[intSlot];
                const auto  dynLinkIt = rng::find(dynLinks, (int32)node.m_wBaseLinkId);
                if (dynLinkIt != rng::end(dynLinks)) {
                    const auto dynLinkIdx = rng::distance(rng::begin(dynLinks), dynLinkIt);
                    node.m_wBaseLinkId = m_aDynamicLinksBaseIds[intSlot][dynLinkIdx];
                    m_aDynamicLinksBaseIds[intSlot][dynLinkIdx] = -1;
                    m_aDynamicLinksIds[intSlot][dynLinkIdx] = -1;
                }
            }
        }

        // Finally, unload area and related data
        const auto FreeAndNull = [](auto& ptr) {
            CMemoryMgr::Free(ptr);
            ptr = nullptr;
        };
        FreeAndNull(m_pPathIntersections[intSlotAreaId]);
        FreeAndNull(m_pLinkLengths[intSlotAreaId]);
        FreeAndNull(m_pPathNodes[intSlotAreaId]);
        FreeAndNull(m_pLinkLengths[intSlotAreaId]);

        m_anNumAddresses[intSlotAreaId] = 0;
        m_anNumCarPathLinks[intSlotAreaId] = 0;
        m_anNumPedNodes[intSlotAreaId] = 0;
        m_anNumVehicleNodes[intSlotAreaId] = 0;
        m_anNumNodes[intSlotAreaId] = 0;
    }
}

// 0x44D930
void CPathFind::FindStartPointOfRegion(size_t x, size_t y, float& outX, float& outY) {
    const auto pos = FindStartPointOfRegion(x, y);
    outX = pos.x;
    outY = pos.y;
}

// notsa
CVector2D CPathFind::FindStartPointOfRegion(size_t x, size_t y) {
    return { FindXCoorsForRegion(x), FindYCoorsForRegion(y) };
}

namespace detail {
constexpr auto GetCoorsOfRegion(size_t p, size_t nareas) {
    return (6000.f / (float)nareas) * (float)p - 3000.f;
}
};

// 0x44D8F0
float CPathFind::FindXCoorsForRegion(size_t x) {
    return detail::GetCoorsOfRegion(x, NUM_PATH_MAP_AREA_X);
}

// 0x44D910
float CPathFind::FindYCoorsForRegion(size_t y) {
    return detail::GetCoorsOfRegion(y, NUM_PATH_MAP_AREA_Y);
}

// 0x44DED0
void CPathFind::AddInteriorLink(int32 intNodeA, int32 intNodeB) {
    const auto AddLink = [](int32 intIdx, int32 linkTo) {
        const auto it = rng::find(ConnectsToGiven[intIdx], -1); 
        assert(!(*it >= 0)); // NOTE: Original code did a `while (*it >= 0), if anything goes bad use `>= 0` for `rng::find`
        *it = linkTo;
    };
    AddLink(intNodeA, intNodeB);
    AddLink(intNodeB, intNodeA);
}

// notsa
std::span<CPathNode> CPathFind::GetPathNodesInArea(size_t areaId, ePathType ptype) const {
    if (const auto allNodes = m_pPathNodes[areaId]) {
        const auto numVehNodes = m_anNumVehicleNodes[areaId];
        switch (ptype) {
        case ePathType::PATH_TYPE_VEH: // Vehicles, boats, race tracks
            return std::span{ allNodes, m_anNumVehicleNodes[areaId] };
        case ePathType::PATH_TYPE_PED: // Peds only
            assert(m_anNumPedNodes[areaId] == m_anNumNodes[areaId] - numVehNodes); // Pirulax: I'm assuming this is true, so if this doesnt assert for a long time remove it
            return std::span{ allNodes + numVehNodes, m_anNumPedNodes[areaId] };
        case ePathType::PATH_TYPE_ALL: // All of the above
            return std::span{ allNodes, m_anNumNodes[areaId] };
        default:
            NOTSA_UNREACHABLE("Invalid pathType: {}", (int)ptype);
        }
    }
    return {}; // Area not loaded, return nothing.. Perhaps assert here instead?
}

// 0x44D1B0
void CPathFind::RemoveNodeFromList(CPathNode* node) {
    node->m_prev->m_next = node->m_next;
    if (node->m_next) {
        node->m_next->m_prev = node->m_prev;
    }

    m_totalNumNodesInPathFindHashTable--;
}

void CPathFind::AddNodeToList(CPathNode* node, int32 distFromOrigin) {
    // Insert the node as the head into it's bucket

    auto& head = m_pathFindHashTable[distFromOrigin % std::size(m_pathFindHashTable)];

    node->m_next = head;

    // Make this node's `next` point to the head in the hash table
    // I guess this works as long as you only access the `m_prev` variable
    // as that's at offset 0
    // This is a really bad hack to avoid having to do special handling
    // for the head in `RemoveNodeFromList`...
    node->m_prev = reinterpret_cast<CPathNode*>(&head);

    if (head) {
        head->m_prev = node;
    }

    head = node;

    assert(distFromOrigin <= std::numeric_limits<decltype(node->m_totalDistFromOrigin)>::max()); // Prevent bugs from overflow
    node->m_totalDistFromOrigin = (int16)distFromOrigin;

    m_totalNumNodesInPathFindHashTable++;
}

// 0x452C80
void CPathFind::SwitchRoadsOffInArea(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, bool bSwitchOff, bool bCars, bool bBackToOriginal) {
    for (auto areaId = 0u; areaId < NUM_PATH_MAP_AREAS; ++areaId) {
        SwitchRoadsOffInAreaForOneRegion(xMin, xMax, yMin, yMax, zMin, zMax, bSwitchOff, bCars, areaId, bBackToOriginal);
    }

    for (auto i = 0u; i < m_nNumNodeSwitches; ++i) {
        auto* pArea = &m_aNodeSwitches[i];

        if (notsa::bugfixes::CPathFind_SwitchRoadsOffInArea_StrayAreas) {
            // some missions create both types of switches at the same area, so we store them separately
            if (pArea->isCars != bCars) {
                continue;
            }

            // avoid creating stray areas, potentially leaving no space for important areas later in game
            // ideally, the script would use SWITCH_ROADS_BACK_TO_ORIGINAL or SWITCH_PED_ROADS_BACK_TO_ORIGINAL but that's not always the case
            // so we consider toggling the same area as "back to original"
            if (pArea->xMin == xMin && pArea->yMin == yMin && pArea->zMin == zMin && pArea->xMax == xMax && pArea->yMax == yMax && pArea->zMax == zMax && pArea->isOff != bSwitchOff) {
                bBackToOriginal = true;
            }
        }

        // If the existing area is completely inside the area we are switching off, remove it
        if (pArea->xMin < xMin || pArea->yMin < yMin || pArea->zMin < zMin || pArea->xMax > xMax || pArea->yMax > yMax || pArea->zMax > zMax) {
            continue;
        }

        for (auto j = i; j < m_nNumNodeSwitches - 1; ++j) {
            if (notsa::bugfixes::CPathFind_SwitchRoadsOffInArea_StrayAreas) {
                m_aNodeSwitches[j] = m_aNodeSwitches[j + 1];
            } else {
                // R* bug, they messed up with the index
                m_aNodeSwitches[i] = m_aNodeSwitches[i + 1];
            }
        }

        --m_nNumNodeSwitches;
        --i;
    }

    if (!bBackToOriginal && m_nNumNodeSwitches < NUM_PATH_MAP_AREAS) {
        auto& area  = m_aNodeSwitches[m_nNumNodeSwitches];
        area.xMin   = xMin;
        area.xMax   = xMax;
        area.yMin   = yMin;
        area.yMax   = yMax;
        area.zMin   = zMin;
        area.zMax   = zMax;
        area.isOff  = bSwitchOff;
        area.isCars = bCars;
        m_nNumNodeSwitches++;
    }
}

// 0x452F00
void CPathFind::SwitchPedRoadsOffInArea(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, bool bSwitchOff, bool bBackToOriginal) {
    SwitchRoadsOffInArea(xMin, xMax, yMin, yMax, zMin, zMax, bSwitchOff, false, bBackToOriginal);
}
