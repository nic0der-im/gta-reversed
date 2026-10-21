#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

//! Frames: the transform hierarchy.
//!
//! The game only reads the frame's matrices (`modelling`, `ltm`) and its object list, so frames
//! are implemented on the game struct itself; librw frames are only used to mirror the camera
//! (see `RwCamera.cpp`).
//!
//! RenderWare syncs frames lazily through a dirty list; there are very few frames during
//! startup, so every query recomputes the hierarchy from its root instead.

namespace {

notsa::fakerw::PluginRegistry g_framePlugins;

void SetIdentity(RwMatrix& m) {
    m.right = { 1.0f, 0.0f, 0.0f };
    m.up    = { 0.0f, 1.0f, 0.0f };
    m.at    = { 0.0f, 0.0f, 1.0f };
    m.pos   = { 0.0f, 0.0f, 0.0f };
    m.flags = rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL;
}

RwFrame* FindRoot(RwFrame* frame) {
    while (frame && frame->object.parent) {
        frame = static_cast<RwFrame*>(frame->object.parent);
    }
    return frame;
}

void UpdateLTM(RwFrame* frame, const RwMatrix* parentLTM) {
    if (parentLTM) {
        RwMatrixMultiply(&frame->ltm, &frame->modelling, parentLTM);
    } else {
        frame->ltm = frame->modelling;
        RwMatrixUpdate(&frame->ltm);
    }

    for (RwFrame* child = frame->child; child; child = child->next) {
        UpdateLTM(child, &frame->ltm);
    }
}

void UpdateObjects(RwFrame* frame) {
    for (RwLLLink* link = frame->objectList.link.next; link != &frame->objectList.link; link = link->next) {
        auto* object = static_cast<RwObjectHasFrame*>(rwLLLinkGetData(link, RwObjectHasFrame, lFrame));
        if (object->sync) {
            object->sync(object);
        }
    }
}

void UpdateHierarchy(RwFrame* frame) {
    RwFrame* root = FindRoot(frame);
    UpdateLTM(root, nullptr);
}

void SetSubtreeRoot(RwFrame* frame, RwFrame* root) {
    frame->root = root;
    for (RwFrame* child = frame->child; child; child = child->next) {
        SetSubtreeRoot(child, root);
    }
}

//! Clone of `src` and of its whole subtree, as built by `_rwFrameCloneAndLinkClones`.
//!
//! While the link is up every original points at its clone through `root`, and every clone
//! points at the root of the clone tree. That is how RenderWare maps the objects of an original
//! clump onto their clones (`RpClumpClone` walks `RpAtomicGetFrame(atomic)->root`);
//! `_rwFramePurgeClone` restores the originals' half of the link.
RwFrame* CloneFrame(RwFrame* src, RwFrame* cloneRoot) {
    RwFrame* const frame = RwFrameCreate();
    if (!frame) {
        return nullptr;
    }

    if (!cloneRoot) {
        cloneRoot = frame;
    }

    frame->object.type         = src->object.type;
    frame->object.subType      = src->object.subType;
    frame->object.flags        = src->object.flags;
    frame->object.privateFlags = src->object.privateFlags;

    frame->modelling = src->modelling;

    frame->root = cloneRoot;
    src->root   = frame; // Remember the clone until `_rwFramePurgeClone`.

    // RenderWare prepends the cloned children, so the clone's child list comes out reversed;
    // nothing reads that order (hierarchy indices live in the plugin data).
    for (RwFrame* child = src->child; child; child = child->next) {
        RwFrame* const clone = CloneFrame(child, cloneRoot);
        if (!clone) {
            RwFrameDestroyHierarchy(frame);
            return nullptr;
        }

        clone->next          = frame->child;
        frame->child         = clone;
        clone->object.parent = frame;
    }

    g_framePlugins.Copy(frame, src, sizeof(RwFrame));
    return frame;
}

} // namespace

RwFrame* RwFrameCreate() {
    auto* frame = static_cast<RwFrame*>(notsa::fakerw::AllocObject(sizeof(RwFrame), g_framePlugins));
    if (!frame) {
        return nullptr;
    }

    frame->object.type      = 0;
    frame->object.subType   = 0;
    frame->object.flags     = 0;
    frame->object.privateFlags = 0;
    frame->object.parent    = nullptr;

    SetIdentity(frame->modelling);
    frame->ltm = frame->modelling;

    notsa::fakerw::ListInit(frame->objectList);

    frame->child = nullptr;
    frame->next  = nullptr;
    frame->root  = frame;

    g_framePlugins.Construct(frame, sizeof(RwFrame));
    return frame;
}

RwBool RwFrameDestroy(RwFrame* frame) {
    if (!frame) {
        return FALSE;
    }

    // Detach from the parent before the frame disappears.
    if (frame->object.parent) {
        RwFrameRemoveChild(frame);
    }

    // Destroy the subtree (RenderWare requires it to be empty, but the clone paths rely on this).
    while (frame->child) {
        RwFrameDestroy(frame->child);
    }

    while (!notsa::fakerw::ListEmpty(frame->objectList)) {
        auto* link = frame->objectList.link.next;
        notsa::fakerw::ListRemove(*link);
    }

    notsa::fakerw::FreeObject(frame, sizeof(RwFrame), g_framePlugins);
    return TRUE;
}

RwBool RwFrameDestroyHierarchy(RwFrame* frame) {
    if (!frame) {
        return FALSE;
    }

    // RenderWare requires the caller to hand over a whole hierarchy; detach it from whatever it
    // hangs from so the parent's child list never keeps a dangling pointer.
    if (frame->object.parent) {
        RwFrameRemoveChild(frame);
    }

    // Children first: `frame` must not be released while they still point at it.
    while (frame->child) {
        RwFrameDestroyHierarchy(frame->child);
    }

    while (!notsa::fakerw::ListEmpty(frame->objectList)) {
        auto* const object = static_cast<RwObjectHasFrame*>(
            rwLLLinkGetData(frame->objectList.link.next, RwObjectHasFrame, lFrame)
        );
        object->object.parent = nullptr;
        notsa::fakerw::ListRemove(object->lFrame);
    }

    notsa::fakerw::FreeObject(frame, sizeof(RwFrame), g_framePlugins);
    return TRUE;
}

RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child) {
    if (!parent || !child || parent == child) {
        return nullptr;
    }

    if (child->object.parent == parent) {
        return child;
    }

    if (child->object.parent) {
        RwFrameRemoveChild(child);
    }

    child->object.parent = parent;
    child->next          = parent->child;
    parent->child        = child;

    SetSubtreeRoot(child, parent->root ? parent->root : FindRoot(parent));
    return child;
}

RwFrame* RwFrameRemoveChild(RwFrame* child) {
    if (!child || !child->object.parent) {
        return child;
    }

    auto* parent = static_cast<RwFrame*>(child->object.parent);
    if (parent->child == child) {
        parent->child = child->next;
    } else {
        for (RwFrame* sibling = parent->child; sibling; sibling = sibling->next) {
            if (sibling->next == child) {
                sibling->next = child->next;
                break;
            }
        }
    }

    child->next          = nullptr;
    child->object.parent = nullptr;
    SetSubtreeRoot(child, child);
    return child;
}

RwFrame* RwFrameUpdateObjects(RwFrame* frame) {
    if (!frame) {
        return nullptr;
    }
    UpdateHierarchy(frame);
    UpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameGetRoot(const RwFrame* frame) {
    if (!frame) {
        return nullptr;
    }
    // `root` is the top of the tree; it only points elsewhere while a clone link is up.
    return frame->root ? frame->root : const_cast<RwFrame*>(frame);
}

RwMatrix* RwFrameGetLTM(RwFrame* frame) {
    if (!frame) {
        return nullptr;
    }
    UpdateHierarchy(frame);
    return &frame->ltm;
}

RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, RwReal angle, RwOpCombineType combine) {
    if (!frame) {
        return nullptr;
    }
    RwMatrixRotate(&frame->modelling, axis, angle, combine);
    UpdateHierarchy(frame);
    return frame;
}

RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* v, RwOpCombineType combine) {
    if (!frame) {
        return nullptr;
    }
    RwMatrixTranslate(&frame->modelling, v, combine);
    UpdateHierarchy(frame);
    return frame;
}

RwFrame* RwFrameScale(RwFrame* frame, const RwV3d* v, RwOpCombineType combine) {
    if (!frame) {
        return nullptr;
    }
    RwMatrixScale(&frame->modelling, v, combine);
    UpdateHierarchy(frame);
    return frame;
}

RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* m, RwOpCombineType combine) {
    if (!frame) {
        return nullptr;
    }
    RwMatrixTransform(&frame->modelling, m, combine);
    UpdateHierarchy(frame);
    return frame;
}

RwFrame* RwFrameSetIdentity(RwFrame* frame) {
    if (!frame) {
        return nullptr;
    }
    SetIdentity(frame->modelling);
    UpdateHierarchy(frame);
    return frame;
}

RwFrame* RwFrameOrthoNormalize(RwFrame* frame) {
    if (!frame) {
        return nullptr;
    }

    auto& m = frame->modelling;

    RwV3dNormalize(&m.right, &m.right);

    const RwReal dot = m.right.x * m.up.x + m.right.y * m.up.y + m.right.z * m.up.z;
    m.up.x -= m.right.x * dot;
    m.up.y -= m.right.y * dot;
    m.up.z -= m.right.z * dot;
    RwV3dNormalize(&m.up, &m.up);

    m.at.x = m.right.y * m.up.z - m.right.z * m.up.y;
    m.at.y = m.right.z * m.up.x - m.right.x * m.up.z;
    m.at.z = m.right.x * m.up.y - m.right.y * m.up.x;

    m.flags &= ~rwMATRIXINTERNALIDENTITY;
    m.flags |= rwMATRIXTYPEORTHONORMAL;

    UpdateHierarchy(frame);
    return frame;
}

RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callBack, void* data) {
    if (!frame || !callBack) {
        return frame;
    }

    for (RwFrame* child = frame->child; child;) {
        RwFrame* next = child->next;
        if (!callBack(child, data)) {
            return nullptr;
        }
        child = next;
    }
    return frame;
}

RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callBack, void* data) {
    if (!frame || !callBack) {
        return frame;
    }

    for (RwLLLink* link = frame->objectList.link.next; link != &frame->objectList.link; link = link->next) {
        auto* object = rwLLLinkGetData(link, RwObjectHasFrame, lFrame);
        if (!callBack(&object->object, data)) {
            return nullptr;
        }
    }
    return frame;
}

RwInt32 RwFrameCount(RwFrame* frame) {
    if (!frame) {
        return 0;
    }

    RwInt32 count = 1;
    for (RwFrame* child = frame->child; child; child = child->next) {
        count += RwFrameCount(child);
    }
    return count;
}

RwInt32 RwFrameRegisterPlugin(
    RwInt32                   size,
    RwUInt32                  pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor  destructCB,
    RwPluginObjectCopy        copyCB
) {
    return g_framePlugins.Register(sizeof(RwFrame), size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RwFrameRegisterPluginStream(
    RwUInt32                         pluginID,
    RwPluginDataChunkReadCallBack    readCB,
    RwPluginDataChunkWriteCallBack   writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB
) {
    return g_framePlugins.RegisterStream(pluginID, readCB, writeCB, getSizeCB);
}

void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame) {
    if (!object) {
        return;
    }

    auto* hasFrame = static_cast<RwObjectHasFrame*>(object);

    if (hasFrame->object.parent) {
        notsa::fakerw::ListRemove(hasFrame->lFrame);
        hasFrame->object.parent = nullptr;
    }

    if (frame) {
        hasFrame->object.parent = frame;
        notsa::fakerw::ListAppend(frame->objectList, hasFrame->lFrame);
    }

    if (hasFrame->sync) {
        hasFrame->sync(hasFrame);
    }
}

void _rwObjectHasFrameReleaseFrame(void* object) {
    _rwObjectHasFrameSetFrame(object, nullptr);
}

RwFrame* _rwFrameCloneAndLinkClones(RwFrame* root) {
    if (!root) {
        return nullptr;
    }

    RwFrame* const clone = CloneFrame(root, nullptr);
    if (!clone) {
        // `CloneFrame` links as it goes; drop the links of the frames it already visited.
        RwFrame* const hierarchyRoot = FindRoot(root);
        SetSubtreeRoot(hierarchyRoot, hierarchyRoot);
        return nullptr;
    }

    // The clone tree starts clean: nothing in it is dirty and its LTMs follow its own matrices.
    clone->object.privateFlags &= ~(rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ);
    UpdateLTM(clone, nullptr);
    return clone;
}

RwFrame* _rwFramePurgeClone(RwFrame* root) {
    if (!root) {
        return nullptr;
    }

    // Every original in the hierarchy still points at its clone; the hierarchy root is what they
    // all must point at again. The clone tree itself stays alive and belongs to the caller, which
    // is what `RpClumpClone` adopts as the frame hierarchy of the cloned clump.
    RwFrame* const hierarchyRoot = FindRoot(root);
    SetSubtreeRoot(hierarchyRoot, hierarchyRoot);
    return root;
}

RwFrame* RwFrameCloneHierarchy(RwFrame* root) {
    RwFrame* const clone = _rwFrameCloneAndLinkClones(root);
    if (clone) {
        _rwFramePurgeClone(root);
    }
    return clone;
}

notsa::fakerw::PluginRegistry& notsa::fakerw::FramePlugins() {
    return g_framePlugins;
}

#endif // NOTSA_NATIVE
