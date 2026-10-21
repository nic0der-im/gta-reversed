#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

#include <cmath>

//! `RtAnim` animations and interpolators on the game's own structs (`rw/rtanim.h`).
//!
//! The layouts here are the game's (callbacks stored per interpolator, sub-interpolator links,
//! interpolated frames trailing the struct), not librw's, because `RpAnimBlend` and the ped
//! code read and write those fields directly.

namespace {

constexpr RwInt32 kMaxInterpolationSchemes = 16;

//! `rwID_RPANIMBLENDPLUGIN` (`Plugins/RpAnimBlendPlugin`). Its interpolated frames carry no
//! header, so the toolkit must not write the keyframe pair pointers into them.
constexpr RwUInt32 kAnimBlendPluginID = MAKECHUNKID(rwVENDORID_DEVELOPER, 0xFB);

RtAnimInterpolatorInfo* g_interpolationSchemes[kMaxInterpolationSchemes]{};

//! On 64 bit the interpolated frame header is two pointers, which is wider than the 32 bit
//! `maxInterpKeyFrameSize` that comes from the files (and that `RpAnimBlend` reads back).
constexpr RwInt32 kInterpFrameHeaderSlack = sizeof(void*) > 4 ? 16 : 0;

//! Bytes per interpolated frame that `RtAnimInterpolatorCreate` actually allocates.
RwInt32 InterpFrameStride(RwInt32 maxInterpKeyFrameSize) {
    return maxInterpKeyFrameSize + kInterpFrameHeaderSlack;
}

//! Root interpolator and accumulated node offset of a (possibly sub-)interpolator.
RtAnimInterpolator* GetRootInterpolator(RtAnimInterpolator* anim, RwInt32* offsetOut = nullptr) {
    RwInt32 offset = 0;
    while (anim->isSubInterpolator && anim->parentAnimation) {
        offset += anim->offsetInParent;
        anim = anim->parentAnimation;
    }
    if (offsetOut) {
        *offsetOut = offset;
    }
    return anim;
}

//! Interpolated frames of sub-interpolators live in their parent.
void* GetInterpFrame(RtAnimInterpolator* anim, RwInt32 nodeIndex) {
    RwInt32 offset = 0;
    RtAnimInterpolator* root = GetRootInterpolator(anim, &offset);
    return rtANIMGETINTERPFRAME(root, offset + nodeIndex);
}

//! Keyframe `index` of `animation`, clamped to the frame block.
RtAnimKeyFrameHeader* GetKeyFrame(const RtAnimAnimation* animation, RwInt32 index) {
    index = std::clamp(index, 0, std::max(animation->numFrames - 1, 0));
    return reinterpret_cast<RtAnimKeyFrameHeader*>(
        static_cast<RwUInt8*>(animation->pFrames) + index * animation->interpInfo->animKeyFrameSize
    );
}

//! Keyframe that follows `frame` in its node's (circular) list.
RtAnimKeyFrameHeader* FindNextKeyFrame(const RtAnimAnimation* animation, const RtAnimKeyFrameHeader* frame) {
    auto* frames = static_cast<RwUInt8*>(animation->pFrames);
    const RwInt32 stride = animation->interpInfo->animKeyFrameSize;

    for (RwInt32 i = 0; i < animation->numFrames; i++) {
        auto* candidate = reinterpret_cast<RtAnimKeyFrameHeader*>(frames + i * stride);
        if (candidate->prevFrame == frame) {
            return candidate;
        }
    }
    return const_cast<RtAnimKeyFrameHeader*>(frame);
}

//! Puts every node back at its initial keyframe pair and rewinds the cursor, exactly like the
//! retail `RtAnimInterpolatorSetCurrentAnim`: interpolate at time zero, then publish the pair in
//! the interpolated frame header and park the cursor past both initial keyframe blocks.
void ResetInterpFrames(RtAnimInterpolator* anim) {
    if (!anim->pCurrentAnim || anim->currentAnimKeyFrameSize <= 0) {
        return;
    }

    RwInt32 base = 0;
    RtAnimInterpolator* root = GetRootInterpolator(anim, &base);
    const RwInt32 totalNodes = root->numNodes;

    // `RpAnimBlend` stores its interpolated frames without a header, so it must not be touched
    // (see the note in `RenderWare/rw/rtanim.cpp`). Its animation is driven by the plugin.
    const bool storeHeaders = (RwUInt32)anim->pCurrentAnim->interpInfo->typeID != kAnimBlendPluginID;

    if (!storeHeaders) {
        // `RpAnimBlend` allocates its animation with zero frames and writes the interpolated
        // frames itself, so there is no keyframe block to read.
        for (RwInt32 i = 0; i < anim->numNodes; i++) {
            if (anim->keyFrameInterpolateCB) {
                anim->keyFrameInterpolateCB(GetInterpFrame(anim, i), nullptr, nullptr, 0.0f, anim->pCurrentAnim->customData);
            }
        }
        anim->pNextFrame  = nullptr;
        anim->currentTime = 0.0f;
        return;
    }

    const bool isRoot = root == anim;

    for (RwInt32 i = 0; i < anim->numNodes; i++) {
        auto* kf1 = GetKeyFrame(anim->pCurrentAnim, base + i);
        auto* kf2 = isRoot
            ? GetKeyFrame(anim->pCurrentAnim, totalNodes + i)
            : FindNextKeyFrame(anim->pCurrentAnim, kf1);

        auto* interp = static_cast<RtAnimInterpFrameHeader*>(GetInterpFrame(anim, i));
        if (anim->keyFrameInterpolateCB) {
            anim->keyFrameInterpolateCB(interp, kf1, kf2, 0.0f, anim->pCurrentAnim->customData);
        }

        interp->keyFrame1 = kf1;
        interp->keyFrame2 = kf2;
    }

    auto* frames = static_cast<RwUInt8*>(anim->pCurrentAnim->pFrames);
    const RwInt32 nextIndex = isRoot ? 2 * totalNodes : totalNodes;
    anim->pNextFrame  = nextIndex < anim->pCurrentAnim->numFrames
        ? frames + (size_t)nextIndex * anim->currentAnimKeyFrameSize
        : nullptr;
    anim->currentTime = 0.0f;
}

//! Moves the current pair of every node forward until it brackets `time`.
void AdvanceInterpFrames(RtAnimInterpolator* anim, RwReal time) {
    if (!anim->pCurrentAnim) {
        return;
    }

    auto* frames = static_cast<RwUInt8*>(anim->pCurrentAnim->pFrames);
    const RwInt32 stride = anim->currentAnimKeyFrameSize;
    if (stride <= 0) {
        return;
    }
    const RwInt32 lastIndex = anim->pCurrentAnim->numFrames - 1;

    auto* next = static_cast<RtAnimKeyFrameHeader*>(anim->pNextFrame);
    while (next) {
        const RwInt32 nextIndex = RwInt32((reinterpret_cast<RwUInt8*>(next) - frames) / stride);
        if (nextIndex <= 0 || nextIndex > lastIndex) {
            next = nullptr;
            break;
        }

        auto* prev = static_cast<RtAnimKeyFrameHeader*>(next->prevFrame);
        if (!prev || prev->time > time) {
            break;
        }

        RwInt32 node = -1;
        for (RwInt32 i = 0; i < anim->numNodes; i++) {
            auto* interp = static_cast<RtAnimInterpFrameHeader*>(GetInterpFrame(anim, i));
            if (interp->keyFrame2 == prev) {
                node = i;
                break;
            }
        }
        if (node < 0) {
            // A sub-interpolator only owns part of the animation: skip the others' frames.
            next = reinterpret_cast<RtAnimKeyFrameHeader*>(reinterpret_cast<RwUInt8*>(next) + stride);
            continue;
        }

        auto* interp = static_cast<RtAnimInterpFrameHeader*>(GetInterpFrame(anim, node));
        interp->keyFrame1 = prev;
        interp->keyFrame2 = next;

        next = reinterpret_cast<RtAnimKeyFrameHeader*>(reinterpret_cast<RwUInt8*>(next) + stride);
    }

    anim->pNextFrame = next;
}

void InterpolateAllFrames(RtAnimInterpolator* anim) {
    if (!anim->pCurrentAnim || !anim->keyFrameInterpolateCB) {
        return;
    }

    for (RwInt32 i = 0; i < anim->numNodes; i++) {
        auto* interp = static_cast<RtAnimInterpFrameHeader*>(GetInterpFrame(anim, i));
        if (!interp->keyFrame1 || !interp->keyFrame2) {
            continue;
        }
        anim->keyFrameInterpolateCB(
            interp,
            interp->keyFrame1,
            interp->keyFrame2,
            anim->currentTime,
            anim->pCurrentAnim->customData
        );
    }
}

void FireAnimCallBack(RtAnimInterpolator* anim) {
    if (anim->pAnimCallBack && anim->currentTime >= anim->animCallBackTime) {
        RtAnimCallBack callback = anim->pAnimCallBack;
        void* data = anim->pAnimCallBackData;
        anim->pAnimCallBack = nullptr; // One shot, like the retail toolkit.
        callback(anim, data);
    }
}

RwReal WrapTime(RwReal time, RwReal duration) {
    if (duration <= 0.0f) {
        return 0.0f;
    }
    RwReal wrapped = std::fmod(time, duration);
    if (wrapped < 0.0f) {
        wrapped += duration;
    }
    return wrapped;
}

} // namespace

//-----------------------------------------------------------------------------
// Interpolation scheme registry
//-----------------------------------------------------------------------------

void RtAnimAnimationFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    // Native allocations do not use free lists.
    UNUSED(blockSize);
    UNUSED(numBlocksToPrealloc);
}

RwBool RtAnimInitialize() {
    // Runs before the plugin attach functions, which register their schemes.
    for (auto*& scheme : g_interpolationSchemes) {
        scheme = nullptr;
    }
    return TRUE;
}

RwBool RtAnimRegisterInterpolationScheme(RtAnimInterpolatorInfo* interpolatorInfo) {
    if (!interpolatorInfo) {
        return FALSE;
    }

    for (auto*& slot : g_interpolationSchemes) {
        if (!slot) {
            slot = interpolatorInfo;
            return TRUE;
        }
    }

    NOTSA_LOG_ERR("Rw: no room for interpolation scheme {}", interpolatorInfo->typeID);
    return FALSE;
}

RtAnimInterpolatorInfo* RtAnimGetInterpolatorInfo(RwInt32 typeID) {
    for (auto* scheme : g_interpolationSchemes) {
        if (scheme && scheme->typeID == typeID) {
            return scheme;
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
// RtAnimAnimation
//-----------------------------------------------------------------------------

RtAnimAnimation* RtAnimAnimationCreate(RwInt32 typeID, RwInt32 numFrames, RwInt32 flags, RwReal duration) {
    RtAnimInterpolatorInfo* interpInfo = RtAnimGetInterpolatorInfo(typeID);
    if (!interpInfo || numFrames < 0) {
        return nullptr;
    }

    const size_t headerSize = sizeof(RtAnimAnimation);
    const size_t framesSize = (size_t)numFrames * interpInfo->animKeyFrameSize;
    const size_t size       = headerSize + framesSize + interpInfo->customDataSize;

    auto* animation = static_cast<RtAnimAnimation*>(notsa::fakerw::Malloc(size));
    if (!animation) {
        return nullptr;
    }
    std::memset(animation, 0, size);

    animation->interpInfo = interpInfo;
    animation->numFrames  = numFrames;
    animation->flags      = flags;
    animation->duration   = duration;
    animation->pFrames    = reinterpret_cast<RwUInt8*>(animation) + headerSize;
    animation->customData = interpInfo->customDataSize > 0
        ? static_cast<RwUInt8*>(animation->pFrames) + framesSize
        : nullptr;

    return animation;
}

RwBool RtAnimAnimationDestroy(RtAnimAnimation* animation) {
    if (!animation) {
        return FALSE;
    }
    notsa::fakerw::Free(animation);
    return TRUE;
}

RwUInt32 RtAnimAnimationGetNumNodes(const RtAnimAnimation* animation) {
    if (!animation || !animation->pFrames || animation->numFrames <= 0 || !animation->interpInfo) {
        return 0;
    }

    auto* first = static_cast<RtAnimKeyFrameHeader*>(animation->pFrames);
    auto* frame = first;
    RwUInt32 numNodes = 0;

    while (frame && frame->prevFrame != first && numNodes < (RwUInt32)animation->numFrames) {
        numNodes++;
        frame = reinterpret_cast<RtAnimKeyFrameHeader*>(
            reinterpret_cast<RwUInt8*>(frame) + animation->interpInfo->animKeyFrameSize
        );
    }
    return numNodes;
}

RtAnimAnimation* RtAnimAnimationStreamRead(RwStream* stream) {
    if (!stream) {
        return nullptr;
    }

    RwInt32 version = 0;
    RwInt32 typeID = 0;
    RwInt32 numFrames = 0;
    RwInt32 flags = 0;
    RwReal  duration = 0.0f;

    if (RwStreamRead(stream, &version, sizeof(version)) != sizeof(version)
        || version != rtANIMSTREAMCURRENTVERSION
    ) {
        NOTSA_LOG_ERR("Rw: unsupported animation stream version 0x{:X}", (RwUInt32)version);
        return nullptr;
    }

    if (RwStreamRead(stream, &typeID, sizeof(typeID)) != sizeof(typeID)
        || RwStreamRead(stream, &numFrames, sizeof(numFrames)) != sizeof(numFrames)
        || RwStreamRead(stream, &flags, sizeof(flags)) != sizeof(flags)
        || RwStreamRead(stream, &duration, sizeof(duration)) != sizeof(duration)
    ) {
        return nullptr;
    }

    RtAnimAnimation* animation = RtAnimAnimationCreate(typeID, numFrames, flags, duration);
    if (!animation) {
        return nullptr;
    }

    if (!animation->interpInfo->keyFrameStreamReadCB
        || !animation->interpInfo->keyFrameStreamReadCB(stream, animation)
    ) {
        RtAnimAnimationDestroy(animation);
        return nullptr;
    }
    return animation;
}

RwBool RtAnimAnimationStreamWrite(const RtAnimAnimation* animation, RwStream* stream) {
    if (!animation || !stream) {
        return FALSE;
    }

    const RwInt32 version = rtANIMSTREAMCURRENTVERSION;
    const RwInt32 typeID  = animation->interpInfo->typeID;

    if (!RwStreamWrite(stream, &version, sizeof(version))
        || !RwStreamWrite(stream, &typeID, sizeof(typeID))
        || !RwStreamWrite(stream, &animation->numFrames, sizeof(animation->numFrames))
        || !RwStreamWrite(stream, &animation->flags, sizeof(animation->flags))
        || !RwStreamWrite(stream, &animation->duration, sizeof(animation->duration))
    ) {
        return FALSE;
    }

    return animation->interpInfo->keyFrameStreamWriteCB
        ? animation->interpInfo->keyFrameStreamWriteCB(animation, stream)
        : FALSE;
}

RwInt32 RtAnimAnimationStreamGetSize(const RtAnimAnimation* animation) {
    if (!animation) {
        return 0;
    }

    const RwInt32 keyFramesSize = animation->interpInfo->keyFrameStreamGetSizeCB
        ? animation->interpInfo->keyFrameStreamGetSizeCB(animation)
        : 0;

    // version, typeID, numFrames, flags, duration
    return 5 * (RwInt32)sizeof(RwInt32) + keyFramesSize;
}

namespace {

//! Writes the 12 byte RenderWare chunk header. `_rwStreamWriteVersionedChunkHeader` is not part
//! of the native layer, and nothing in the game writes animation files, so this is the only
//! writer that needs it.
bool WriteChunkHeader(RwStream* stream, RwUInt32 type, RwUInt32 length) {
    struct {
        RwUInt32 type;
        RwUInt32 length;
        RwUInt32 libraryID;
    } header{ type, length, 0x36003 }; // RenderWare 3.6.0.3
    return RwStreamWrite(stream, &header, sizeof(header)) != nullptr;
}

} // namespace

RtAnimAnimation* RtAnimAnimationRead(const RwChar* filename) {
    if (!filename) {
        return nullptr;
    }

    RwStream* stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);
    if (!stream) {
        NOTSA_LOG_ERR("Rw: failed to open animation file '{}'", filename);
        return nullptr;
    }

    RtAnimAnimation* animation = nullptr;
    if (RwStreamFindChunk(stream, rwID_ANIMANIMATION, nullptr, nullptr)) {
        animation = RtAnimAnimationStreamRead(stream);
    }

    RwStreamClose(stream, nullptr);
    return animation;
}

RwBool RtAnimAnimationWrite(const RtAnimAnimation* animation, const RwChar* filename) {
    if (!animation || !filename) {
        return FALSE;
    }

    RwStream* stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, filename);
    if (!stream) {
        return FALSE;
    }

    const RwBool ok = WriteChunkHeader(stream, rwID_ANIMANIMATION, RtAnimAnimationStreamGetSize(animation))
        && RtAnimAnimationStreamWrite(animation, stream);
    RwStreamClose(stream, nullptr);
    return ok;
}

RwBool RtAnimAnimationMakeDelta(RtAnimAnimation* animation, RwInt32 numNodes, RwReal time) {
    if (!animation || !animation->pFrames || numNodes <= 0 || time < 0.0f) {
        return FALSE;
    }
    if (!animation->interpInfo->keyFrameMulRecipCB) {
        return FALSE;
    }

    RtAnimInterpolator* interp = RtAnimInterpolatorCreate(numNodes, animation->interpInfo->interpKeyFrameSize);
    if (!interp) {
        return FALSE;
    }

    if (!RtAnimInterpolatorSetCurrentAnim(interp, animation)
        || !RtAnimInterpolatorSetCurrentTime(interp, time)
    ) {
        RtAnimInterpolatorDestroy(interp);
        return FALSE;
    }

    for (RwInt32 node = 0; node < numNodes; node++) {
        auto* start = static_cast<RtAnimInterpFrameHeader*>(GetInterpFrame(interp, node));

        auto* initial = GetKeyFrame(animation, node);
        animation->interpInfo->keyFrameMulRecipCB(initial, start);

        auto* frame = FindNextKeyFrame(animation, initial);
        for (RwInt32 guard = 0; frame != initial && guard < animation->numFrames; guard++) {
            animation->interpInfo->keyFrameMulRecipCB(frame, start);
            auto* next = FindNextKeyFrame(animation, frame);
            if (next == initial) {
                break;
            }
            frame = next;
        }
    }

    animation->duration = std::max(0.0f, animation->duration - time);
    RtAnimInterpolatorDestroy(interp);
    return TRUE;
}

//-----------------------------------------------------------------------------
// RtAnimInterpolator
//-----------------------------------------------------------------------------

RtAnimInterpolator* RtAnimInterpolatorCreate(RwInt32 numNodes, RwInt32 maxInterpKeyFrameSize) {
    if (numNodes <= 0 || maxInterpKeyFrameSize <= 0) {
        return nullptr;
    }

    const size_t size = sizeof(RtAnimInterpolator)
                      + (size_t)numNodes * InterpFrameStride(maxInterpKeyFrameSize);
    auto* anim = static_cast<RtAnimInterpolator*>(notsa::fakerw::Malloc(size));
    if (!anim) {
        return nullptr;
    }
    std::memset(anim, 0, size);

    anim->maxInterpKeyFrameSize      = maxInterpKeyFrameSize;
    anim->currentInterpKeyFrameSize  = maxInterpKeyFrameSize;
    anim->currentAnimKeyFrameSize    = 0;
    anim->numNodes                   = numNodes;
    anim->isSubInterpolator          = FALSE;
    anim->offsetInParent             = 0;
    anim->parentAnimation            = nullptr;
    return anim;
}

void RtAnimInterpolatorDestroy(RtAnimInterpolator* anim) {
    if (anim) {
        notsa::fakerw::Free(anim);
    }
}

RwBool RtAnimInterpolatorSetCurrentAnim(RtAnimInterpolator* animI, RtAnimAnimation* anim) {
    if (!animI || !anim || !anim->interpInfo || !anim->pFrames) {
        return FALSE;
    }

    RtAnimInterpolatorInfo* interpInfo = anim->interpInfo;
    if (interpInfo->interpKeyFrameSize > InterpFrameStride(animI->maxInterpKeyFrameSize)) {
        NOTSA_LOG_ERR("Rw: interpolation frame of scheme {} does not fit", interpInfo->typeID);
        return FALSE;
    }

    animI->pCurrentAnim                   = anim;
    animI->currentInterpKeyFrameSize      = interpInfo->interpKeyFrameSize;
    animI->currentAnimKeyFrameSize        = interpInfo->animKeyFrameSize;
    animI->keyFrameApplyCB                = interpInfo->keyFrameApplyCB;
    animI->keyFrameBlendCB                = interpInfo->keyFrameBlendCB;
    animI->keyFrameInterpolateCB          = interpInfo->keyFrameInterpolateCB;
    animI->keyFrameAddCB                  = interpInfo->keyFrameAddCB;

    ResetInterpFrames(animI);
    return TRUE;
}

RwBool RtAnimInterpolatorSetKeyFrameCallBacks(RtAnimInterpolator* anim, RwInt32 keyFrameTypeID) {
    if (!anim) {
        return FALSE;
    }

    RtAnimInterpolatorInfo* interpInfo = RtAnimGetInterpolatorInfo(keyFrameTypeID);
    if (!interpInfo) {
        return FALSE;
    }
    if (interpInfo->interpKeyFrameSize > InterpFrameStride(anim->maxInterpKeyFrameSize)) {
        return FALSE;
    }

    anim->keyFrameApplyCB           = interpInfo->keyFrameApplyCB;
    anim->keyFrameBlendCB           = interpInfo->keyFrameBlendCB;
    anim->keyFrameInterpolateCB     = interpInfo->keyFrameInterpolateCB;
    anim->keyFrameAddCB             = interpInfo->keyFrameAddCB;
    anim->currentInterpKeyFrameSize = interpInfo->interpKeyFrameSize;
    anim->currentAnimKeyFrameSize   = interpInfo->animKeyFrameSize;

    if (anim->pCurrentAnim && (RwUInt32)anim->pCurrentAnim->interpInfo->typeID != kAnimBlendPluginID) {
        InterpolateAllFrames(anim);
    }
    return TRUE;
}

void RtAnimInterpolatorSetAnimLoopCallBack(RtAnimInterpolator* anim, RtAnimCallBack callBack, void* data) {
    if (!anim) {
        return;
    }
    anim->pAnimLoopCallBack     = callBack;
    anim->pAnimLoopCallBackData = data;
}

void RtAnimInterpolatorSetAnimCallBack(RtAnimInterpolator* anim, RtAnimCallBack callBack, RwReal time, void* data) {
    if (!anim) {
        return;
    }
    anim->pAnimCallBack     = callBack;
    anim->animCallBackTime  = time;
    anim->pAnimCallBackData = data;
}

RwBool RtAnimInterpolatorCopy(RtAnimInterpolator* outAnim, RtAnimInterpolator* inAnim) {
    if (!outAnim || !inAnim) {
        return FALSE;
    }

    const RwInt32 numNodes = std::min(outAnim->numNodes, inAnim->numNodes);

    outAnim->pCurrentAnim              = inAnim->pCurrentAnim;
    outAnim->currentTime               = inAnim->currentTime;
    outAnim->pNextFrame                = inAnim->pNextFrame;
    outAnim->pAnimCallBack             = inAnim->pAnimCallBack;
    outAnim->pAnimCallBackData         = inAnim->pAnimCallBackData;
    outAnim->animCallBackTime          = inAnim->animCallBackTime;
    outAnim->pAnimLoopCallBack         = inAnim->pAnimLoopCallBack;
    outAnim->pAnimLoopCallBackData     = inAnim->pAnimLoopCallBackData;
    outAnim->currentInterpKeyFrameSize = inAnim->currentInterpKeyFrameSize;
    outAnim->currentAnimKeyFrameSize   = inAnim->currentAnimKeyFrameSize;
    outAnim->keyFrameApplyCB           = inAnim->keyFrameApplyCB;
    outAnim->keyFrameBlendCB           = inAnim->keyFrameBlendCB;
    outAnim->keyFrameInterpolateCB     = inAnim->keyFrameInterpolateCB;
    outAnim->keyFrameAddCB             = inAnim->keyFrameAddCB;

    const RwInt32 size = std::min({
        outAnim->currentInterpKeyFrameSize,
        inAnim->currentInterpKeyFrameSize,
        InterpFrameStride(outAnim->maxInterpKeyFrameSize)
    });
    for (RwInt32 i = 0; i < numNodes; i++) {
        std::memcpy(GetInterpFrame(outAnim, i), GetInterpFrame(inAnim, i), size);
    }
    return TRUE;
}

RwBool RtAnimInterpolatorSubAnimTime(RtAnimInterpolator* anim, RwReal time) {
    if (!anim || !anim->pCurrentAnim) {
        return FALSE;
    }
    if (time <= 0.0f) {
        return TRUE;
    }

    RwReal newTime = anim->currentTime - time;
    if (newTime < 0.0f) {
        if (anim->pAnimLoopCallBack) {
            anim->pAnimLoopCallBack(anim, anim->pAnimLoopCallBackData);
        }
        newTime = WrapTime(newTime, anim->pCurrentAnim->duration);
    }

    // Going backwards needs a fresh pair scan from the start.
    ResetInterpFrames(anim);
    anim->currentTime = newTime;
    AdvanceInterpFrames(anim, newTime);
    InterpolateAllFrames(anim);
    return TRUE;
}

RwBool RtAnimInterpolatorAddAnimTime(RtAnimInterpolator* anim, RwReal time) {
    if (!anim || !anim->pCurrentAnim) {
        return FALSE;
    }
    if (time <= 0.0f) {
        return TRUE;
    }

    const RwReal duration = anim->pCurrentAnim->duration;
    RwReal newTime = anim->currentTime + time;

    if (newTime > duration) {
        if (anim->pAnimLoopCallBack) {
            anim->pAnimLoopCallBack(anim, anim->pAnimLoopCallBackData);
        }
        newTime = WrapTime(newTime, duration);
        ResetInterpFrames(anim);
    }

    anim->currentTime = newTime;
    AdvanceInterpFrames(anim, newTime);
    InterpolateAllFrames(anim);
    FireAnimCallBack(anim);
    return TRUE;
}

RwBool RtAnimInterpolatorSetCurrentTime(RtAnimInterpolator* anim, RwReal time) {
    if (!anim || !anim->pCurrentAnim) {
        return FALSE;
    }

    const RwReal duration = anim->pCurrentAnim->duration;
    if (time < 0.0f) {
        time = 0.0f;
    } else if (time > duration) {
        time = duration;
    }

    if (time < anim->currentTime) {
        ResetInterpFrames(anim);
    }

    anim->currentTime = time;
    AdvanceInterpFrames(anim, time);
    InterpolateAllFrames(anim);
    FireAnimCallBack(anim);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Combining interpolators
//-----------------------------------------------------------------------------

RwBool RtAnimInterpolatorBlend(
    RtAnimInterpolator* outAnim,
    RtAnimInterpolator* inAnim1,
    RtAnimInterpolator* inAnim2,
    RwReal              alpha
) {
    if (!outAnim || !inAnim1 || !inAnim2 || !outAnim->keyFrameBlendCB) {
        return FALSE;
    }

    const RwInt32 numNodes = std::min({ outAnim->numNodes, inAnim1->numNodes, inAnim2->numNodes });
    for (RwInt32 i = 0; i < numNodes; i++) {
        outAnim->keyFrameBlendCB(
            GetInterpFrame(outAnim, i),
            GetInterpFrame(inAnim1, i),
            GetInterpFrame(inAnim2, i),
            alpha
        );
    }
    return TRUE;
}

RwBool RtAnimInterpolatorAddTogether(
    RtAnimInterpolator* outAnim,
    RtAnimInterpolator* inAnim1,
    RtAnimInterpolator* inAnim2
) {
    if (!outAnim || !inAnim1 || !inAnim2 || !outAnim->keyFrameAddCB) {
        return FALSE;
    }

    const RwInt32 numNodes = std::min({ outAnim->numNodes, inAnim1->numNodes, inAnim2->numNodes });
    for (RwInt32 i = 0; i < numNodes; i++) {
        outAnim->keyFrameAddCB(
            GetInterpFrame(outAnim, i),
            GetInterpFrame(inAnim1, i),
            GetInterpFrame(inAnim2, i)
        );
    }
    return TRUE;
}

RtAnimInterpolator* RtAnimInterpolatorCreateSubInterpolator(
    RtAnimInterpolator* parentAnim,
    RwInt32             startNode,
    RwInt32             numNodes,
    RwInt32             maxInterpKeyFrameSize
) {
    if (!parentAnim || startNode < 0 || numNodes <= 0 || startNode + numNodes > parentAnim->numNodes) {
        return nullptr;
    }

    RtAnimInterpolator* subAnim = RtAnimInterpolatorCreate(numNodes, maxInterpKeyFrameSize);
    if (!subAnim) {
        return nullptr;
    }

    subAnim->isSubInterpolator        = TRUE;
    subAnim->offsetInParent           = startNode;
    subAnim->parentAnimation          = parentAnim;
    subAnim->pCurrentAnim             = parentAnim->pCurrentAnim;
    subAnim->currentInterpKeyFrameSize = parentAnim->currentInterpKeyFrameSize;
    subAnim->currentAnimKeyFrameSize   = parentAnim->currentAnimKeyFrameSize;
    subAnim->keyFrameApplyCB           = parentAnim->keyFrameApplyCB;
    subAnim->keyFrameBlendCB           = parentAnim->keyFrameBlendCB;
    subAnim->keyFrameInterpolateCB     = parentAnim->keyFrameInterpolateCB;
    subAnim->keyFrameAddCB             = parentAnim->keyFrameAddCB;
    return subAnim;
}

RwBool RtAnimInterpolatorBlendSubInterpolator(
    RtAnimInterpolator* outAnim,
    RtAnimInterpolator* inAnim1,
    RtAnimInterpolator* inAnim2,
    RwReal              alpha
) {
    if (!outAnim || !inAnim1 || !inAnim2 || !outAnim->keyFrameBlendCB) {
        return FALSE;
    }

    const RwInt32 numNodes = std::min({ outAnim->numNodes, inAnim1->numNodes, inAnim2->numNodes });
    for (RwInt32 i = 0; i < numNodes; i++) {
        outAnim->keyFrameBlendCB(
            GetInterpFrame(outAnim, i),
            GetInterpFrame(inAnim1, i),
            GetInterpFrame(inAnim2, i),
            alpha
        );
    }
    return TRUE;
}

RwBool RtAnimInterpolatorAddSubInterpolator(
    RtAnimInterpolator* outAnim,
    RtAnimInterpolator* mainAnim,
    RtAnimInterpolator* subAnim
) {
    if (!outAnim || !mainAnim || !subAnim || !outAnim->keyFrameAddCB) {
        return FALSE;
    }

    const RwInt32 numNodes = std::min({ outAnim->numNodes, mainAnim->numNodes, subAnim->numNodes });
    for (RwInt32 i = 0; i < numNodes; i++) {
        outAnim->keyFrameAddCB(
            GetInterpFrame(outAnim, i),
            GetInterpFrame(mainAnim, i),
            GetInterpFrame(subAnim, i)
        );
    }
    return TRUE;
}

#endif // NOTSA_NATIVE
