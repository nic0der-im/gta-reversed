#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"
#include "pipelines/SAPipelines.h"

#include <unordered_map>

//! The Rp world layer.
//!
//! These are real containers on the game structs (so pointers are valid, plugin data exists and
//! iterators work). The clump/atomic/geometry/mesh containers are filled by the DFF reader in
//! `RwDff.cpp`; what is still missing here is the streaming of worlds themselves.

namespace {

notsa::fakerw::PluginRegistry g_worldPlugins;
notsa::fakerw::PluginRegistry g_atomicPlugins;
notsa::fakerw::PluginRegistry g_clumpPlugins;
notsa::fakerw::PluginRegistry g_geometryPlugins;
notsa::fakerw::PluginRegistry g_materialPlugins;

std::unordered_map<RpWorld*, rw::World*> g_worldMirrors;

} // namespace

namespace notsa::fakerw {

//! One registry per object type. The game's plugin attach functions register their data and
//! stream callbacks here; `RwDff.cpp` dispatches the `rwID_EXTENSION` chunks through them.
PluginRegistry& MaterialPlugins() { return g_materialPlugins; }
PluginRegistry& GeometryPlugins() { return g_geometryPlugins; }
PluginRegistry& AtomicPlugins()    { return g_atomicPlugins; }
PluginRegistry& ClumpPlugins()     { return g_clumpPlugins; }

rw::World* GetLibrwWorld(RpWorld* world) {
    if (!world) {
        return nullptr;
    }
    const auto it = g_worldMirrors.find(world);
    return it != g_worldMirrors.end() ? it->second : nullptr;
}

RpWorld* GetGameWorld(rw::World* world) {
    if (!world) {
        return nullptr;
    }
    for (const auto& [gameWorld, mirror] : g_worldMirrors) {
        if (mirror == world) {
            return gameWorld;
        }
    }
    return nullptr;
}

} // namespace notsa::fakerw



//-----------------------------------------------------------------------------
// Plugin attach
//-----------------------------------------------------------------------------

RwBool RpWorldPluginAttach() {
    // librw's world is part of the engine; nothing to attach.
    return TRUE;
}

RwBool RpMatFXPluginAttach() {
    static bool attached = false;
    if (!attached) {
        rw::registerMatFXPlugin();
        attached = true;
    }
    return TRUE;
}

RwBool RpUVAnimPluginAttach() {
    static bool attached = false;
    if (!attached) {
        rw::registerUVAnimPlugin();
        attached = true;
    }
    return TRUE;
}

RwBool RpAnisotPluginAttach() {
    static bool attached = false;
    if (!attached) {
        rw::registerAnisotropyPlugin();
        attached = true;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// World
//-----------------------------------------------------------------------------

RpWorld* RpWorldCreate(RwBBox* boundingBox) {
    auto* world = static_cast<RpWorld*>(notsa::fakerw::AllocObject(sizeof(RpWorld), g_worldPlugins));
    if (!world) {
        return nullptr;
    }

    rwObjectInitialize(&world->object, rwID_WORLD, 0);
    world->renderOrder = rpWORLDRENDERNARENDERORDER;
    if (boundingBox) {
        world->boundingBox = *boundingBox;
    }
    notsa::fakerw::ListInit(world->clumpList);
    notsa::fakerw::ListInit(world->lightList);
    notsa::fakerw::ListInit(world->directionalLightList);

    auto librwBox = notsa::fakerw::ToLibrw(world->boundingBox);
    if (auto* librwWorld = rw::World::create(&librwBox)) {
        g_worldMirrors.emplace(world, librwWorld);
    }

    g_worldPlugins.Construct(world, sizeof(RpWorld));
    return world;
}

namespace {

//! Unlinks every light of `world` from the world lists, both the game ones and the librw ones,
//! so a light that outlives its world never points into freed storage.
void DetachLights(RpWorld* world) {
    auto* librwWorld = notsa::fakerw::GetLibrwWorld(world);
    const auto DetachList = [librwWorld](RwLinkList& list) {
        for (RwLLLink* link = list.link.next; link != &list.link;) {
            RwLLLink* next = link->next;
            auto* light = rwLLLinkGetData(link, RpLight, inWorld);
            if (auto* librwLight = notsa::fakerw::GetLibrwLight(light); librwLight && librwLight->world == librwWorld) {
                librwLight->world->removeLight(librwLight);
            }
            notsa::fakerw::ListRemove(light->inWorld);
            link = next;
        }
    };

    DetachList(world->directionalLightList);
    DetachList(world->lightList);
}

} // namespace

RwBool RpWorldDestroy(RpWorld* world) {
    if (!world) {
        return FALSE;
    }

    DetachLights(world);

    if (const auto it = g_worldMirrors.find(world); it != g_worldMirrors.end()) {
        it->second->destroy();
        g_worldMirrors.erase(it);
    }

    notsa::fakerw::FreeObject(world, sizeof(RpWorld), g_worldPlugins);
    return TRUE;
}

RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera) {
    if (!world || !camera) {
        return world;
    }

    auto* librwWorld  = notsa::fakerw::GetLibrwWorld(world);
    auto* librwCamera = notsa::fakerw::GetLibrwCamera(camera);
    if (!librwWorld || !librwCamera) {
        return world;
    }

    // `Camera::beginUpdate` copies this into `engine->currentWorld`, which is the world the SA
    // pipelines ask `rw::gl3::lightingCB()` to enumerate the lights of.
    if (librwCamera->world != librwWorld) {
        if (librwCamera->world) {
            librwCamera->world->removeCamera(librwCamera);
        }
        librwWorld->addCamera(librwCamera);
    }
    return world;
}

RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera) {
    if (!world || !camera) {
        return world;
    }

    auto* librwWorld  = notsa::fakerw::GetLibrwWorld(world);
    auto* librwCamera = notsa::fakerw::GetLibrwCamera(camera);
    if (librwWorld && librwCamera && librwCamera->world == librwWorld) {
        librwWorld->removeCamera(librwCamera);
    }
    return world;
}

RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light) {
    if (!world || !light) {
        return world;
    }

    // Game-side membership: retail keeps ambient/directional lights (no position, so no world
    // sector tie) in `directionalLightList` and positioned ones in `lightList`. `inWorld`
    // belongs to one of the two lists, so its link doubles as the membership marker.
    if (!light->inWorld.next) {
        auto& list = rwObjectGetSubType(light) < rpLIGHTPOSITIONINGSTART
            ? world->directionalLightList
            : world->lightList;
        notsa::fakerw::ListAppend(list, light->inWorld);
    }

    if (auto* librwWorld = notsa::fakerw::GetLibrwWorld(world)) {
        // Refreshes the mirror from the game struct first: the game rotates the light's frame
        // before adding it, and `lightingCB()` wants the resulting direction.
        if (auto* librwLight = notsa::fakerw::GetLibrwLight(light)) {
            if (librwLight->world != librwWorld) {
                if (librwLight->world) {
                    librwLight->world->removeLight(librwLight);
                }
                librwWorld->addLight(librwLight);
            }
        }
    }
    return world;
}

RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light) {
    if (!world || !light) {
        return world;
    }

    if (light->inWorld.next) {
        notsa::fakerw::ListRemove(light->inWorld);
    }

    if (auto* librwWorld = notsa::fakerw::GetLibrwWorld(world)) {
        if (auto* librwLight = notsa::fakerw::GetLibrwLight(light)) {
            if (librwLight->world == librwWorld) {
                librwWorld->removeLight(librwLight);
            }
        }
    }
    return world;
}

//-----------------------------------------------------------------------------
// Materials
//-----------------------------------------------------------------------------
//
// `RpMaterialCreate/Destroy/SetTexture` and the material list live in `RwDff.cpp` with the
// stream reader that builds them.

RwInt32 RpMaterialRegisterPlugin(
    RwInt32                   size,
    RwUInt32                  pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor  destructCB,
    RwPluginObjectCopy        copyCB
) {
    return g_materialPlugins.Register(sizeof(RpMaterial), size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpMaterialRegisterPluginStream(
    RwUInt32                         pluginID,
    RwPluginDataChunkReadCallBack    readCB,
    RwPluginDataChunkWriteCallBack   writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB
) {
    return g_materialPlugins.RegisterStream(pluginID, readCB, writeCB, getSizeCB);
}

//-----------------------------------------------------------------------------
// Geometry
//-----------------------------------------------------------------------------
//
// `RpGeometryCreate/Destroy` and the rest of the geometry machinery live in `RwDff.cpp` next
// to the stream reader that fills them.

RwInt32 RpGeometryRegisterPlugin(
    RwInt32                   size,
    RwUInt32                  pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor  destructCB,
    RwPluginObjectCopy        copyCB
) {
    return g_geometryPlugins.Register(sizeof(RpGeometry), size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpGeometryRegisterPluginStream(
    RwUInt32                         pluginID,
    RwPluginDataChunkReadCallBack    readCB,
    RwPluginDataChunkWriteCallBack   writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB
) {
    return g_geometryPlugins.RegisterStream(pluginID, readCB, writeCB, getSizeCB);
}

//-----------------------------------------------------------------------------
// Atomics
//-----------------------------------------------------------------------------

RpAtomic* RpAtomicCreate(void) {
    auto* atomic = static_cast<RpAtomic*>(notsa::fakerw::AllocObject(sizeof(RpAtomic), g_atomicPlugins));
    if (!atomic) {
        return nullptr;
    }

    rwObjectInitialize(&atomic->object.object, rpATOMIC, 0);
    atomic->object.sync       = nullptr;
    atomic->geometry          = nullptr;
    atomic->clump             = nullptr;
    atomic->pipeline          = nullptr;
    atomic->renderCallBack    = AtomicDefaultRenderCallBack;
    notsa::fakerw::ListInit(atomic->llWorldSectorsInAtomic);

    g_atomicPlugins.Construct(atomic, sizeof(RpAtomic));
    return atomic;
}

RwBool RpAtomicDestroy(RpAtomic* atomic) {
    if (!atomic) {
        return FALSE;
    }

    if (atomic->object.object.parent) {
        _rwObjectHasFrameSetFrame(atomic, nullptr);
    }
    if (atomic->clump) {
        RpClumpRemoveAtomic(atomic->clump, atomic);
    }
    if (atomic->geometry) {
        RpGeometryDestroy(atomic->geometry);
        atomic->geometry = nullptr;
    }

    notsa::fakerw::FreeObject(atomic, sizeof(RpAtomic), g_atomicPlugins);
    return TRUE;
}

RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame) {
    if (atomic) {
        _rwObjectHasFrameSetFrame(atomic, frame);
    }
    return atomic;
}

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic) {
    if (!atomic) {
        return atomic;
    }

    notsa::native::sapipelines::Kind kind;
    if (notsa::native::sapipelines::GetPipelineKind(atomic->pipeline, kind)) {
        notsa::native::sapipelines::RenderAtomic(atomic);
    } else if (atomic->pipeline) {
        // A pipeline built through the generic `Rx*` API: nothing renders it natively.
        notsa::fakerw::Stub("AtomicRenderCallBack(generic pipeline)");
    } else {
        // No pipeline at all (the atomics librw's own renderer would draw in B3).
        notsa::fakerw::Stub("AtomicRenderCallBack(no pipeline)");
    }
    return atomic;
}

RwBool RpAtomicInstance(RpAtomic* atomic) {
    if (!atomic) {
        return FALSE;
    }

    notsa::native::sapipelines::Kind kind;
    if (notsa::native::sapipelines::GetPipelineKind(atomic->pipeline, kind)) {
        notsa::native::sapipelines::InstanceAtomic(atomic);
    }
    return TRUE;
}

RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, RwUInt32 flags) {
    if (!atomic) {
        return nullptr;
    }

    if (atomic->geometry && atomic->geometry != geometry) {
        RpGeometryDestroy(atomic->geometry);
    }

    atomic->geometry = geometry;
    if (geometry) {
        geometry->refCount++;

        if (!(flags & rpATOMICSAMEBOUNDINGSPHERE) && geometry->morphTarget) {
            atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
            atomic->object.object.privateFlags |= rpATOMICPRIVATEWORLDBOUNDDIRTY;
        }
    }
    return atomic;
}

RwInt32 RpAtomicRegisterPlugin(
    RwInt32                   size,
    RwUInt32                  pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor  destructCB,
    RwPluginObjectCopy        copyCB
) {
    return g_atomicPlugins.Register(sizeof(RpAtomic), size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpAtomicRegisterPluginStream(
    RwUInt32                         pluginID,
    RwPluginDataChunkReadCallBack    readCB,
    RwPluginDataChunkWriteCallBack   writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB
) {
    return g_atomicPlugins.RegisterStream(pluginID, readCB, writeCB, getSizeCB);
}

//-----------------------------------------------------------------------------
// Clumps
//-----------------------------------------------------------------------------

RpClump* RpClumpCreate(void) {
    auto* clump = static_cast<RpClump*>(notsa::fakerw::AllocObject(sizeof(RpClump), g_clumpPlugins));
    if (!clump) {
        return nullptr;
    }

    rwObjectInitialize(&clump->object, rpCLUMP, 0);
    notsa::fakerw::ListInit(clump->atomicList);
    notsa::fakerw::ListInit(clump->lightList);
    notsa::fakerw::ListInit(clump->cameraList);

    g_clumpPlugins.Construct(clump, sizeof(RpClump));
    return clump;
}

RwBool RpClumpDestroy(RpClump* clump) {
    if (!clump) {
        return FALSE;
    }

    while (!notsa::fakerw::ListEmpty(clump->atomicList)) {
        auto* atomic = rwLLLinkGetData(clump->atomicList.link.next, RpAtomic, inClumpLink);
        RpClumpRemoveAtomic(clump, atomic);
        RpAtomicDestroy(atomic);
    }

    // The clump owns its frame hierarchy (retail `RpClumpDestroy`, 0x74A310).
    if (auto* frame = static_cast<RwFrame*>(clump->object.parent)) {
        clump->object.parent = nullptr;
        RwFrameDestroyHierarchy(frame);
    }

    notsa::fakerw::FreeObject(clump, sizeof(RpClump), g_clumpPlugins);
    return TRUE;
}

RpClump* RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic) {
    if (!clump || !atomic) {
        return nullptr;
    }

    if (atomic->clump == clump) {
        return clump;
    }
    if (atomic->clump) {
        RpClumpRemoveAtomic(atomic->clump, atomic);
    }

    atomic->clump = clump;
    notsa::fakerw::ListAppend(clump->atomicList, atomic->inClumpLink);
    return clump;
}

RpClump* RpClumpRemoveAtomic(RpClump* clump, RpAtomic* atomic) {
    if (!clump || !atomic) {
        return nullptr;
    }

    notsa::fakerw::ListRemove(atomic->inClumpLink);
    atomic->clump = nullptr;
    return clump;
}

RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* data) {
    if (!clump || !callback) {
        return clump;
    }

    for (RwLLLink* link = clump->atomicList.link.next; link != &clump->atomicList.link;) {
        RwLLLink* next = link->next;
        auto* atomic = rwLLLinkGetData(link, RpAtomic, inClumpLink);
        if (!callback(atomic, data)) {
            return nullptr;
        }
        link = next;
    }
    return clump;
}

RpClump* RpClumpRender(RpClump* clump) {
    notsa::fakerw::Stub("RpClumpRender");
    return clump;
}

RwInt32 RpClumpRegisterPlugin(
    RwInt32                   size,
    RwUInt32                  pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor  destructCB,
    RwPluginObjectCopy        copyCB
) {
    return g_clumpPlugins.Register(sizeof(RpClump), size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpClumpRegisterPluginStream(
    RwUInt32                         pluginID,
    RwPluginDataChunkReadCallBack    readCB,
    RwPluginDataChunkWriteCallBack   writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB
) {
    return g_clumpPlugins.RegisterStream(pluginID, readCB, writeCB, getSizeCB);
}

#endif // NOTSA_NATIVE
