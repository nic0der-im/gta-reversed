#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

#include <unordered_map>

//! Lights.
//!
//! The game struct is authoritative: colour, radius, cone angle, flags and the frame it hangs
//! off are all read straight from `RpLight`. librw, however, needs its own `rw::Light` in the
//! world for `rw::gl3::lightingCB()` to see it, and it takes a directional's direction from
//! `light->getFrame()->getLTM()->at`; the game only ever builds game frames. Every game light
//! therefore owns a mirror (a `rw::Light` plus a private `rw::Frame`) that is refreshed from the
//! game struct by every setter and once more when the light is added to a world
//! (`GetLibrwLight`, called from `RpWorldAddLight`).
//!
//! The light's frame in a stream is a *frame index* written by the clump/world reader right
//! before the light chunk (`ID_STRUCT` + index, RW 3.6); that reader owns it, `RpLightStreamRead`
//! only restores the light itself.
//!
//! `RpLightSetFrame`/`RpLightGetFrame` are macros over `rwObjectHasFrameSetFrame`/
//! `rwObjectGetParent` in the game headers, so they have no counterpart here: a frame installed
//! that way (or transformed after a setter, like the day/night sun) shows up in the mirror on the
//! next setter call — at most one frame later.

namespace {

notsa::fakerw::PluginRegistry g_lightPlugins;

struct LightMirror {
    rw::Light* light = nullptr;
    rw::Frame* frame = nullptr; //!< librw needs a frame to read the direction/position from.
};

std::unordered_map<RpLight*, LightMirror> g_lightMirrors;

//! librw's light types share the game's `RpLightType` values, but keep the mapping explicit.
int32 ToLibrwLightType(RwInt32 type) {
    switch (type) {
    case rpLIGHTDIRECTIONAL: return rw::Light::DIRECTIONAL;
    case rpLIGHTAMBIENT:     return rw::Light::AMBIENT;
    case rpLIGHTPOINT:       return rw::Light::POINT;
    case rpLIGHTSPOT:        return rw::Light::SPOT;
    case rpLIGHTSPOTSOFT:    return rw::Light::SOFTSPOT;
    default:
        NOTSA_LOG_WARN("Rw: unknown light type {}, treating it as directional", type);
        return rw::Light::DIRECTIONAL;
    }
}

LightMirror* FindMirror(const RpLight* light) {
    if (!light) {
        return nullptr;
    }
    const auto it = g_lightMirrors.find(const_cast<RpLight*>(light));
    return it != g_lightMirrors.end() ? &it->second : nullptr;
}

//! Copies the authoritative game state into the mirror.
void SyncMirror(RpLight* light, LightMirror& mirror) {
    mirror.light->radius        = light->radius;
    mirror.light->minusCosAngle = light->minusCosAngle;
    mirror.light->setColor(light->color.red, light->color.green, light->color.blue);
    mirror.light->color.alpha   = light->color.alpha;
    mirror.light->setFlags(rwObjectGetFlags(light));

    // The mirror frame always stays attached so a light without a game frame still has a valid
    // `getLTM()`; it just sits at the identity basis.
    if (auto* gameFrame = static_cast<RwFrame*>(rwObjectGetParent(light))) {
        const rw::Matrix matrix = notsa::fakerw::ToLibrw(*RwFrameGetLTM(gameFrame));
        mirror.frame->matrix = matrix;
        mirror.frame->ltm    = matrix;
    } else {
        mirror.frame->matrix.setIdentity();
        mirror.frame->ltm.setIdentity();
    }
}

void DestroyMirror(LightMirror& mirror) {
    if (mirror.light) {
        if (mirror.light->world) {
            mirror.light->world->removeLight(mirror.light);
        }
        mirror.light->destroy();
    }
    if (mirror.frame) {
        mirror.frame->destroy();
    }
}

} // namespace

//-----------------------------------------------------------------------------
// Lights
//-----------------------------------------------------------------------------

RpLight* RpLightCreate(RwInt32 type) {
    auto* light = static_cast<RpLight*>(notsa::fakerw::AllocObject(sizeof(RpLight), g_lightPlugins));

    rwObjectInitialize(&light->object.object, rwID_LIGHT, (RwUInt8)type);
    light->radius        = 1.0f;
    light->color         = { 1.0f, 1.0f, 1.0f, 1.0f };
    light->minusCosAngle = -1.0f;
    notsa::fakerw::ListInit(light->WorldSectorsInLight);

    g_lightPlugins.Construct(light, sizeof(RpLight));

    LightMirror mirror{ rw::Light::create(ToLibrwLightType(type)), rw::Frame::create() };
    if (mirror.light && mirror.frame) {
        mirror.light->setFrame(mirror.frame);
        SyncMirror(light, mirror);
        g_lightMirrors.emplace(light, mirror);
    } else {
        notsa::fakerw::WarnOnce("Rw: could not create a librw light mirror");
        if (mirror.light) {
            mirror.light->destroy();
        }
        if (mirror.frame) {
            mirror.frame->destroy();
        }
    }
    return light;
}

RwBool RpLightDestroy(RpLight* light) {
    if (!light) {
        return FALSE;
    }

    // Unlink the game-side world membership even when the caller skipped `RpWorldRemoveLight`,
    // or the world's light list would keep pointing at freed storage.
    if (light->inWorld.next) {
        notsa::fakerw::ListRemove(light->inWorld);
    }

    if (auto* mirror = FindMirror(light)) {
        DestroyMirror(*mirror);
        g_lightMirrors.erase(light);
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
        if (auto* mirror = FindMirror(light)) {
            SyncMirror(light, *mirror);
        }
    }
    return light;
}

RpLight* RpLightSetRadius(RpLight* light, RwReal radius) {
    if (light) {
        light->radius = radius;
        if (auto* mirror = FindMirror(light)) {
            SyncMirror(light, *mirror);
        }
    }
    return light;
}

// 0x751AE0
RwReal RpLightGetConeAngle(const RpLight* light) {
    return light ? std::acos(-light->minusCosAngle) : 0.0f;
}

RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle) {
    if (light) {
        light->minusCosAngle = -std::cos(angle);
        if (auto* mirror = FindMirror(light)) {
            SyncMirror(light, *mirror);
        }
    }
    return light;
}

RpWorld* RpLightGetWorld(const RpLight* light) {
    const auto* mirror = FindMirror(light);
    if (!mirror || !mirror->light->world) {
        return nullptr;
    }
    return notsa::fakerw::GetGameWorld(mirror->light->world);
}

RpLight* RpLightForAllWorldSectors(RpLight* light, RpWorldSectorCallBack callback, void* data) {
    if (!light || !callback) {
        return light;
    }

    // World sectors (and therefore light ties) do not exist natively, so the list is empty.
    for (RwLLLink* link = light->WorldSectorsInLight.link.next; link != &light->WorldSectorsInLight.link;) {
        RwLLLink* next = link->next;
        auto* tie = rwLLLinkGetData(link, RpLightTie, lightInWorldSector);
        if (!callback(tie->sect, data)) {
            return light;
        }
        link = next;
    }
    return light;
}

RwBool RpLightValidatePlugins(const RpLight* light) {
    // No light plugin data is registered natively, so there is nothing to validate.
    UNUSED(light);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Streaming
//-----------------------------------------------------------------------------

RpLightChunkInfo* _rpLightChunkInfoRead(RwStream* stream, RpLightChunkInfo* lightChunkInfo, RwInt32* bytesRead) {
    constexpr RwUInt32 kStructHeaderSize = sizeof(RwUInt32) * 3;

    if (!stream || !lightChunkInfo) {
        return nullptr;
    }

    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, nullptr)) {
        NOTSA_LOG_ERR("Rw: light chunk has no struct");
        return nullptr;
    }
    if (length < sizeof(RpLightChunkInfo)) {
        NOTSA_LOG_ERR("Rw: light struct is {} bytes, expected at least {}", length, sizeof(RpLightChunkInfo));
        return nullptr;
    }
    if (RwStreamRead(stream, lightChunkInfo, sizeof(RpLightChunkInfo)) != sizeof(RpLightChunkInfo)) {
        return nullptr;
    }
    // Newer writers may append fields; step over them so the caller lands on the next chunk.
    if (length > sizeof(RpLightChunkInfo) && !RwStreamSkip(stream, length - (RwUInt32)sizeof(RpLightChunkInfo))) {
        return nullptr;
    }

    if (bytesRead) {
        *bytesRead = (RwInt32)(kStructHeaderSize + length);
    }
    return lightChunkInfo;
}

RpLight* RpLightStreamRead(RwStream* stream) {
    if (!stream) {
        return nullptr;
    }

    RwUInt32 length  = 0;
    RwUInt32 version = 0;
    if (!RwStreamFindChunk(stream, rwID_LIGHT, &length, &version)) {
        NOTSA_LOG_ERR("Rw: stream has no light chunk");
        return nullptr;
    }

    RpLightChunkInfo info{};
    RwInt32 headerBytes = 0;
    if (!_rpLightChunkInfoRead(stream, &info, &headerBytes)) {
        return nullptr;
    }

    auto* light = RpLightCreate((RwInt32)(info.typeAndFlags >> 16));
    light->radius   = info.radius;
    light->color.red   = info.red;
    light->color.green = info.green;
    light->color.blue  = info.blue;
    light->color.alpha = 1.0f;
    if (version >= 0x30300) {
        light->minusCosAngle = info.minusCosAngle;
    } else {
        // Before 3.3 the field held the tangent of the cone angle.
        light->minusCosAngle = -1.0f / std::sqrt(info.minusCosAngle * info.minusCosAngle + 1.0f);
    }
    rwObjectSetFlags(light, info.typeAndFlags & 0xFF);

    if (auto* mirror = FindMirror(light)) {
        SyncMirror(light, *mirror);
    }

    // The light chunk is the struct plus whatever the plugins added; the frame index lives in
    // the enclosing clump/world reader, so skip any trailing bytes to land on the next chunk.
    if (const RwUInt32 consumed = (RwUInt32)headerBytes; length > consumed && !RwStreamSkip(stream, length - consumed)) {
        NOTSA_LOG_ERR("Rw: could not skip the light chunk trailer");
    }

    return light;
}

//-----------------------------------------------------------------------------
// Accessors
//-----------------------------------------------------------------------------

namespace notsa::fakerw {

rw::Light* GetLibrwLight(RpLight* light) {
    auto* mirror = FindMirror(light);
    if (!mirror) {
        return nullptr;
    }
    SyncMirror(light, *mirror);
    return mirror->light;
}

} // namespace notsa::fakerw

#endif // NOTSA_NATIVE
