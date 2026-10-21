#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"
#include "pipelines/SAPipelines.h"

#include <cstring>
#include <string>

//! RenderWare's generic pipeline API (`Rx*`) on the native build.
//!
//! The game builds its object pipelines with RenderWare's generic pipeline API
//! (`RxPipelineCreate` + `RxLockedPipeAddFragment` + callbacks), but the native build renders
//! with the San Andreas pipelines from `pipelines/SAPipelines` instead: the `CCustom*Pipeline`
//! classes get their handle from `sapipelines::GetPipeline` and never construct an `RxPipeline`.
//!
//! This file therefore provides two things:
//!  - real handles for the generic API, so any pipeline built the RenderWare way (and destroyed
//!    again) is a valid, self-consistent object with a harmless generic node, and
//!  - the San Andreas node definitions (`RxNodeDefinitionGetD3D9*AllInOne`), which are the names
//!    the game looks nodes up by.
//!
//! Nothing here executes a node body: pipeline execution is only used by the D3D9 drivers, which
//! do not exist natively. `RxPipelineExecute` logs once and does not draw.

namespace {

//! Minimal, valid node definitions. `name` is what `RxPipelineFindNodeByName` matches, so the
//! names are the retail D3D9 ones.
RxNodeDefinition MakeNodeDefinition(const char* name) {
    RxNodeDefinition def{};
    def.name                     = const_cast<RwChar*>(name);
    def.nodeMethods              = {};
    def.io                       = {};
    def.pipelineNodePrivateDataSize = 0;
    def.editable                 = rxNODEDEFCONST;
    def.InputPipesCnt            = 0;
    return def;
}

RxNodeDefinition g_genericNodeDef     = MakeNodeDefinition("GenericNode");
RxNodeDefinition g_atomicNodeDef      = MakeNodeDefinition("D3D9AtomicAllInOneNode");
RxNodeDefinition g_worldSectorNodeDef = MakeNodeDefinition("D3D9WorldSectorAllInOneNode");
RxNodeDefinition g_skinAtomicNodeDef  = MakeNodeDefinition("D3D9SkinAtomicAllInOneNode");

//! Every generic pipeline owns exactly one node; it is allocated right after the pipeline.
constexpr uint32 kGenericNodeCount = 1;

RxPipelineNode* GenericNodes(RxPipeline* pipeline) {
    return reinterpret_cast<RxPipelineNode*>(pipeline + 1);
}

} // namespace

//-----------------------------------------------------------------------------
// Node definitions
//-----------------------------------------------------------------------------

RxNodeDefinition* RxNodeDefinitionGetD3D9AtomicAllInOne() {
    return &g_atomicNodeDef;
}

RxNodeDefinition* RxNodeDefinitionGetD3D9WorldSectorAllInOne() {
    return &g_worldSectorNodeDef;
}

RxNodeDefinition* RxNodeDefinitionGetD3D9SkinAtomicAllInOne() {
    return &g_skinAtomicNodeDef;
}

//-----------------------------------------------------------------------------
// Pipeline lifecycle
//-----------------------------------------------------------------------------

RwBool _rxPipelineOpen() {
    return TRUE;
}

RwBool _rxPipelineClose() {
    return TRUE;
}

RxPipeline* RxPipelineCreate() {
    constexpr size_t size = sizeof(RxPipeline) + kGenericNodeCount * sizeof(RxPipelineNode);

    auto* const pipeline = static_cast<RxPipeline*>(notsa::fakerw::Malloc(size));
    if (!pipeline) {
        return nullptr;
    }
    std::memset(pipeline, 0, size);

    pipeline->nodes        = GenericNodes(pipeline);
    pipeline->numNodes     = kGenericNodeCount;
    pipeline->entryPoint   = 0;
    pipeline->locked       = FALSE;
    pipeline->nodes->nodeDef = &g_genericNodeDef;

    return pipeline;
}

void _rxPipelineDestroy(RxPipeline* pipeline) {
    if (!pipeline) {
        return;
    }

    // The San Andreas pipelines are singletons owned by `pipelines/SAPipelines`.
    notsa::native::sapipelines::Kind kind;
    if (notsa::native::sapipelines::GetPipelineKind(pipeline, kind)) {
        return;
    }

    notsa::fakerw::Free(pipeline);
}

RxPipeline* RxPipelineClone(RxPipeline* pipeline) {
    if (!pipeline) {
        return nullptr;
    }

    auto* const clone = RxPipelineCreate();
    if (!clone) {
        return nullptr;
    }

    clone->pluginId   = pipeline->pluginId;
    clone->pluginData = pipeline->pluginData;
    if (pipeline->nodes && pipeline->numNodes > 0) {
        clone->nodes[0].nodeDef = pipeline->nodes[0].nodeDef;
    }
    return clone;
}

RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data, RwBool heapReset) {
    UNUSED(heapReset);

    if (!pipeline || !pipeline->nodes || pipeline->numNodes == 0) {
        return nullptr;
    }

    RxPipelineNode* const node = &pipeline->nodes[pipeline->entryPoint];
    if (node->nodeDef && node->nodeDef->nodeMethods.nodeBody) {
        RxPipelineNodeParam params{ data, nullptr };
        node->nodeDef->nodeMethods.nodeBody(node, &params);
    } else {
        notsa::fakerw::Stub("RxPipelineExecute");
    }
    return pipeline;
}

//-----------------------------------------------------------------------------
// Locked pipeline editing
//-----------------------------------------------------------------------------

RxLockedPipe* RxPipelineLock(RxPipeline* pipeline) {
    if (!pipeline) {
        return nullptr;
    }
    pipeline->locked = TRUE;
    return pipeline;
}

RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline) {
    if (!pipeline) {
        return nullptr;
    }
    pipeline->locked = FALSE;
    return pipeline;
}

RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline, RwUInt32* firstIndex, RxNodeDefinition* nodeDef0, RxNodeDefinition* __va_end__) {
    UNUSED(__va_end__);

    if (!pipeline || !pipeline->nodes) {
        return nullptr;
    }

    if (firstIndex) {
        *firstIndex = 0;
    }
    // Only the first node definition matters: the fakerw pipeline has a single generic node,
    // and the game only looks that node up by name afterwards.
    if (nodeDef0) {
        pipeline->nodes[0].nodeDef = nodeDef0;
    }
    return pipeline;
}

RxPipeline* RxLockedPipeReplaceNode(RxLockedPipe* pipeline, RxPipelineNode* node, RxNodeDefinition* nodeDef) {
    if (!pipeline || !node) {
        return nullptr;
    }
    node->nodeDef = nodeDef;
    return pipeline;
}

RxPipeline* RxLockedPipeDeleteNode(RxLockedPipe* pipeline, RxPipelineNode* node) {
    UNUSED(node);
    // The generic node cannot be removed; deleting a node is a no-op for fakerw.
    return pipeline;
}

RxPipeline* RxLockedPipeSetEntryPoint(RxLockedPipe* pipeline, RxNodeInput in) {
    if (!pipeline || !pipeline->nodes) {
        return nullptr;
    }
    if (!in) {
        pipeline->entryPoint = 0;
    } else if (in >= pipeline->nodes && in < pipeline->nodes + pipeline->numNodes) {
        pipeline->entryPoint = static_cast<RwUInt32>(in - pipeline->nodes);
    }
    return pipeline;
}

RxPipelineNode* RxLockedPipeGetEntryPoint(RxLockedPipe* pipeline) {
    if (!pipeline || !pipeline->nodes || pipeline->entryPoint >= pipeline->numNodes) {
        return nullptr;
    }
    return &pipeline->nodes[pipeline->entryPoint];
}

RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RxNodeOutput out, RxNodeInput in) {
    UNUSED(out);
    UNUSED(in);
    // The single generic node has no outputs; the fragment stays linear.
    return pipeline;
}

RxPipeline* RxLockedPipeDeletePath(RxLockedPipe* pipeline, RxNodeOutput out, RxNodeInput in) {
    UNUSED(out);
    UNUSED(in);
    return pipeline;
}

RxPipeline* RxPipelineInsertDebugNode(RxPipeline* pipeline, RxPipelineNode* before, RxPipelineNode* after, RxNodeDefinition* debugNode) {
    UNUSED(before);
    UNUSED(after);
    UNUSED(debugNode);
    return pipeline;
}

//-----------------------------------------------------------------------------
// Node lookup
//-----------------------------------------------------------------------------

RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name, RxPipelineNode* start, RwInt32* nodeIndex) {
    if (!pipeline || !pipeline->nodes || pipeline->numNodes == 0) {
        return nullptr;
    }

    uint32 begin = 0;
    if (start && start >= pipeline->nodes && start < pipeline->nodes + pipeline->numNodes) {
        begin = static_cast<uint32>(start - pipeline->nodes) + 1;
    }

    if (name) {
        for (uint32 i = begin; i < pipeline->numNodes; i++) {
            RxPipelineNode* const node = &pipeline->nodes[i];
            if (node->nodeDef && node->nodeDef->name && std::strcmp(node->nodeDef->name, name) == 0) {
                if (nodeIndex) {
                    *nodeIndex = static_cast<RwInt32>(i);
                }
                return node;
            }
        }

        // Missing node: the generic node is still a valid (inert) handle, which is enough for
        // setups that only attach callbacks to it.
        notsa::fakerw::WarnOnce(
            ("Rx: node '" + std::string(name) + "' not found; using the generic node").c_str()
        );
    }

    // No name means "the first node from `begin`".
    if (begin >= pipeline->numNodes) {
        return nullptr;
    }
    if (nodeIndex) {
        *nodeIndex = static_cast<RwInt32>(begin);
    }
    return &pipeline->nodes[begin];
}

RxPipelineNode* RxPipelineFindNodeByIndex(RxPipeline* pipeline, RwUInt32 nodeIndex) {
    if (!pipeline || !pipeline->nodes || nodeIndex >= pipeline->numNodes) {
        return nullptr;
    }
    return &pipeline->nodes[nodeIndex];
}

RxNodeDefinition* RxPipelineNodeCloneDefinition(RxPipelineNode* node, RxClusterDefinition* cluster2add) {
    UNUSED(cluster2add);
    if (!node) {
        return nullptr;
    }
    return node->nodeDef ? node->nodeDef : &g_genericNodeDef;
}

RxPipeline* RxPipelineNodeRequestCluster(RxPipeline* pipeline, RxPipelineNode* node, RxClusterDefinition* clusterDef) {
    UNUSED(node);
    UNUSED(clusterDef);
    // Clusters are only used by packet execution, which fakerw does not run.
    return pipeline;
}

RxPipeline* RxPipelineNodeReplaceCluster(RxPipeline* pipeline, RxPipelineNode* node, RxClusterDefinition* oldClusterDef, RxClusterDefinition* newClusterDef) {
    UNUSED(node);
    UNUSED(oldClusterDef);
    UNUSED(newClusterDef);
    return pipeline;
}

void* RxPipelineNodeGetInitData(RxPipelineNode* node) {
    return node ? node->initializationData : nullptr;
}

void* RxPipelineNodeCreateInitData(RxPipelineNode* node, RwUInt32 size) {
    if (!node) {
        return nullptr;
    }
    if (node->initializationData && node->initializationDataSize >= size) {
        return node->initializationData;
    }
    notsa::fakerw::Free(node->initializationData);
    node->initializationData     = notsa::fakerw::Malloc(size);
    node->initializationDataSize = size;
    if (node->initializationData) {
        std::memset(node->initializationData, 0, size);
    }
    return node->initializationData;
}

#endif // NOTSA_NATIVE
