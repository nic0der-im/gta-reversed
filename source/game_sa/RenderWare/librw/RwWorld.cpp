#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"
#include "pipelines/SAPipelines.h"

#include <unordered_map>

//! The Rp world layer.
//!
//! These are real containers on the game structs (so pointers are valid, plugin data exists and
//! iterators work), but nothing is fed to librw yet: DFF/TXD streaming and rendering are later
//! slices. Functions that would only matter for rendering log `Rw: stub`.

namespace {

notsa::fakerw::PluginRegistry g_worldPlugins;
notsa::fakerw::PluginRegistry g_atomicPlugins;
notsa::fakerw::PluginRegistry g_clumpPlugins;
notsa::fakerw::PluginRegistry g_geometryPlugins;
notsa::fakerw::PluginRegistry g_lightPlugins;
notsa::fakerw::PluginRegistry g_materialPlugins;

std::unordered_map<RpWorld*, rw::World*> g_worldMirrors;

rw::BBox ToLibrw(const RwBBox& box) {
    return { notsa::fakerw::ToLibrw(box.inf), notsa::fakerw::ToLibrw(box.sup) };
}

} // namespace

//-----------------------------------------------------------------------------
// Plugin attach
//-----------------------------------------------------------------------------

RwBool RpWorldPluginAttach() {
    // librw's world is part of the engine; nothing to attach.
    return TRUE;
}

RwBool RpSkinPluginAttach() {
    static bool attached = false;
    if (!attached) {
        rw::registerSkinPlugin();
        attached = true;
    }
    return TRUE;
}

RwBool RpHAnimPluginAttach() {
    static bool attached = false;
    if (!attached) {
        rw::registerHAnimPlugin();
        attached = true;
    }
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

RwBool RtAnimInitialize() {
    // librw needs no separate animation initialisation.
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

    auto librwBox = ToLibrw(world->boundingBox);
    if (auto* librwWorld = rw::World::create(&librwBox)) {
        g_worldMirrors.emplace(world, librwWorld);
    }

    g_worldPlugins.Construct(world, sizeof(RpWorld));
    return world;
}

RwBool RpWorldDestroy(RpWorld* world) {
    if (!world) {
        return FALSE;
    }

    if (const auto it = g_worldMirrors.find(world); it != g_worldMirrors.end()) {
        it->second->destroy();
        g_worldMirrors.erase(it);
    }

    notsa::fakerw::FreeObject(world, sizeof(RpWorld), g_worldPlugins);
    return TRUE;
}

RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera) {
    // The camera is presented through its own librw mirror; the world camera list is only used
    // by the (not yet implemented) world renderer.
    UNUSED(camera);
    notsa::fakerw::Stub("RpWorldAddCamera");
    return world;
}

RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera) {
    UNUSED(camera);
    notsa::fakerw::Stub("RpWorldRemoveCamera");
    return world;
}

RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light) {
    UNUSED(light);
    notsa::fakerw::Stub("RpWorldAddLight");
    return world;
}

RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light) {
    UNUSED(light);
    notsa::fakerw::Stub("RpWorldRemoveLight");
    return world;
}

//-----------------------------------------------------------------------------
// Lights
//-----------------------------------------------------------------------------

RpLight* RpLightCreate(RwInt32 type) {
    auto* light = static_cast<RpLight*>(notsa::fakerw::AllocObject(sizeof(RpLight), g_lightPlugins));
    if (!light) {
        return nullptr;
    }

    rwObjectInitialize(&light->object.object, rwID_LIGHT, (RwUInt8)type);
    light->radius        = 1.0f;
    light->color         = { 1.0f, 1.0f, 1.0f, 1.0f };
    light->minusCosAngle = -1.0f;
    notsa::fakerw::ListInit(light->WorldSectorsInLight);

    g_lightPlugins.Construct(light, sizeof(RpLight));
    return light;
}

RwBool RpLightDestroy(RpLight* light) {
    if (!light) {
        return FALSE;
    }
    if (light->object.object.parent) {
        _rwObjectHasFrameSetFrame(light, nullptr);
    }
    notsa::fakerw::FreeObject(light, sizeof(RpLight), g_lightPlugins);
    return TRUE;
}

RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color) {
    if (light && color) {
        light->color = *color;
    }
    return light;
}

RpLight* RpLightSetRadius(RpLight* light, RwReal radius) {
    if (light) {
        light->radius = radius;
    }
    return light;
}

//-----------------------------------------------------------------------------
// Materials
//-----------------------------------------------------------------------------

RpMaterial* RpMaterialCreate() {
    auto* material = static_cast<RpMaterial*>(notsa::fakerw::AllocObject(sizeof(RpMaterial), g_materialPlugins));
    if (!material) {
        return nullptr;
    }

    material->texture      = nullptr;
    material->color        = { 255, 255, 255, 255 };
    material->pipeline     = nullptr;
    material->surfaceProps = { 1.0f, 1.0f, 1.0f };
    material->refCount     = 0;

    g_materialPlugins.Construct(material, sizeof(RpMaterial));
    return material;
}

RwBool RpMaterialDestroy(RpMaterial* material) {
    if (!material) {
        return FALSE;
    }

    if (material->refCount > 0) {
        material->refCount--;
        return TRUE;
    }

    notsa::fakerw::FreeObject(material, sizeof(RpMaterial), g_materialPlugins);
    return TRUE;
}

RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture) {
    if (material) {
        material->texture = texture;
    }
    return material;
}

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

RpGeometry* RpGeometryCreate(RwInt32 numVert, RwInt32 numTriangles, RwUInt32 format) {
    auto* geometry = static_cast<RpGeometry*>(notsa::fakerw::AllocObject(sizeof(RpGeometry), g_geometryPlugins));
    if (!geometry) {
        return nullptr;
    }

    rwObjectInitialize(&geometry->object, rwID_GEOMETRY, 0);
    geometry->flags          = format;
    geometry->numVertices    = numVert;
    geometry->numTriangles   = numTriangles;
    geometry->numMorphTargets = 1;
    geometry->refCount       = 0;

    if (numTriangles > 0) {
        geometry->triangles = static_cast<RpTriangle*>(
            notsa::fakerw::Malloc(sizeof(RpTriangle) * (size_t)numTriangles)
        );
        if (geometry->triangles) {
            std::memset(geometry->triangles, 0, sizeof(RpTriangle) * (size_t)numTriangles);
        }
    }

    geometry->morphTarget = static_cast<RpMorphTarget*>(notsa::fakerw::Malloc(sizeof(RpMorphTarget)));
    if (geometry->morphTarget) {
        std::memset(geometry->morphTarget, 0, sizeof(RpMorphTarget));
        geometry->morphTarget->parentGeom = geometry;

        if (numVert > 0) {
            geometry->morphTarget->verts = static_cast<RwV3d*>(
                notsa::fakerw::Malloc(sizeof(RwV3d) * (size_t)numVert)
            );
            geometry->morphTarget->normals = static_cast<RwV3d*>(
                notsa::fakerw::Malloc(sizeof(RwV3d) * (size_t)numVert)
            );
            if (geometry->morphTarget->verts) {
                std::memset(geometry->morphTarget->verts, 0, sizeof(RwV3d) * (size_t)numVert);
            }
            if (geometry->morphTarget->normals) {
                std::memset(geometry->morphTarget->normals, 0, sizeof(RwV3d) * (size_t)numVert);
            }
        }
    }

    g_geometryPlugins.Construct(geometry, sizeof(RpGeometry));
    return geometry;
}

RwBool RpGeometryDestroy(RpGeometry* geometry) {
    if (!geometry) {
        return FALSE;
    }

    // The SA pipelines cache GL buffers per geometry; drop them before the struct is freed.
    notsa::native::sapipelines::ForgetGeometry(geometry);

    // Plugins must be destructed before the storage is released.
    g_geometryPlugins.Destruct(geometry, sizeof(RpGeometry));

    if (geometry->morphTarget) {
        notsa::fakerw::Free(geometry->morphTarget->verts);
        notsa::fakerw::Free(geometry->morphTarget->normals);
        notsa::fakerw::Free(geometry->morphTarget);
    }
    notsa::fakerw::Free(geometry->triangles);

    notsa::fakerw::Free(geometry);
    return TRUE;
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* data) {
    if (!geometry || !fpCallBack) {
        return geometry;
    }

    for (RwInt32 i = 0; i < geometry->matList.numMaterials; i++) {
        if (!fpCallBack(geometry->matList.materials[i], data)) {
            return nullptr;
        }
    }
    return geometry;
}

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

    rwObjectInitialize(&atomic->object.object, rwID_ATOMIC, 0);
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
    if (atomic->geometry && atomic->geometry->refCount > 0) {
        atomic->geometry->refCount--;
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
    UNUSED(flags);
    if (atomic) {
        atomic->geometry = geometry;
        if (geometry) {
            geometry->refCount++;
        }
    }
    return atomic;
}

RpAtomic* RpAtomicClone(RpAtomic* atomic) {
    if (!atomic) {
        return nullptr;
    }

    auto* clone = RpAtomicCreate();
    if (!clone) {
        return nullptr;
    }

    clone->geometry       = atomic->geometry;
    clone->boundingSphere = atomic->boundingSphere;
    clone->worldBoundingSphere = atomic->worldBoundingSphere;
    clone->renderCallBack = atomic->renderCallBack;
    if (clone->geometry) {
        clone->geometry->refCount++;
    }

    g_atomicPlugins.Copy(clone, atomic, sizeof(RpAtomic));
    return clone;
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

    rwObjectInitialize(&clump->object, rwID_CLUMP, 0);
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
