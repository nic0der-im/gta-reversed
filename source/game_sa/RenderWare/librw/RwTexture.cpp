#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

#include <cstring>
#include <string_view>

//! Textures and texture dictionaries, on top of librw.
//!
//! The game structs (`RwTexture`, `RwTexDictionary`, `RwRaster`) stay authoritative, because the
//! game reads their fields directly (`name`, `filterAddressing`, `raster->width`, ...). The TXD
//! readers parse the archive with librw — which knows the chunk layout and the native D3D9
//! texture format — convert every raster to the current platform and copy it into a game raster
//! created through `RwRasterCreate`, which is what registers the librw mirror the renderer
//! resolves through `GetLibrwRaster`.
//!
//! The readers mirror the retail ones:
//!   * `RwTexDictionaryGtaStreamRead` / `...1` / `...2` — `0x730FC0`, `0x731070`, `0x731150`
//!   * `RwTextureGtaStreamRead` — `0x730E60`
//!   * `RwTextureRead` — `0x7F3AC0`
//!   * `RwTextureCreate` / `RwTextureDestroy` — `0x7F37C0`, `0x7F3820`
//!   * `RwTexDictionaryFindNamedTexture` — `0x7F39F0`
//!   * `RwTextureStreamRead` — `0x8046E0`

namespace notsa::fakerw {

//! Absolute/relative stream movement, used by the librw bridge below. Defined in `RwStream.cpp`
//! because only that file knows the stream layout.
RwBool SeekRwStream(RwStream* stream, int32 offset, int32 whence);

} // namespace notsa::fakerw

namespace {

namespace fakerw = notsa::fakerw;

fakerw::PluginRegistry g_texturePlugins;
fakerw::PluginRegistry g_texDictionaryPlugins;

RwTexDictionary*     g_currentTexDictionary = nullptr;
RwTextureCallBackFind g_findCallBack        = nullptr;
RwTextureCallBackRead g_readCallBack        = nullptr;

//! `RwTexDictionaryGtaStreamRead1` reads half the textures and leaves the rest to
//! `RwTexDictionaryGtaStreamRead2`, which may get a different stream over the same model
//! buffer. These carry that state over (`DAT_008d6088` and `DAT_00c87fe8` in the retail code).
int32 g_remainingTextures = -1;
int32 g_resumePosition    = 0;

constexpr uint32 kTextureNameSize = rwTEXTUREBASENAMELENGTH;

//! RW versions the retail texture readers accept (3.4.0.0 .. 3.6.0.3).
constexpr RwUInt32 kMinLibraryVersion = 0x34000;
constexpr RwUInt32 kMaxLibraryVersion = 0x36003;

//! Default `filterAddressing` of a new texture: wrap/wrap with nearest filtering.
constexpr RwUInt32 kDefaultFilterAddressing = 0x1101;

//! Size of a RenderWare chunk header: type, length, library id.
constexpr int32 kChunkHeaderSize = 12;

//! Length of a NUL terminated string, clamped to `maxLength` (the RW name fields are fixed size
//! and an archive is not guaranteed to terminate them).
size_t BoundedStringLength(const char* text, size_t maxLength) {
    size_t length = 0;
    while (length < maxLength && text[length]) {
        length++;
    }
    return length;
}

//-----------------------------------------------------------------------------
// librw stream bridge
//-----------------------------------------------------------------------------

//! librw stream over one of the game's streams.
//!
//! The game owns the stream, so closing the wrapper must not close it. librw's readers rewind
//! over headers they already consumed, hence `seek` goes through `SeekRwStream`. `eof` reports a
//! short read, which is what librw's chunk reader checks after asking for a header.
class GameStream final : public rw::Stream {
public:
    explicit GameStream(RwStream& stream) : m_stream{stream} {}

    void close() override {}

    rw::uint32 write8(const void* data, rw::uint32 length) override {
        return RwStreamWrite(&m_stream, data, length) ? length : 0;
    }

    rw::uint32 read8(void* data, rw::uint32 length) override {
        if (length == 0) {
            return 0;
        }
        const RwUInt32 read = RwStreamRead(&m_stream, data, length);
        if (read != length) {
            m_shortRead = true;
        }
        return read;
    }

    void seek(rw::int32 offset, rw::int32 whence = 1) override {
        m_shortRead = false;
        fakerw::SeekRwStream(&m_stream, offset, whence);
    }

    rw::uint32 tell() override { return (rw::uint32)m_stream.position; }
    bool eof() override { return m_shortRead; }

private:
    RwStream& m_stream;
    bool      m_shortRead = false;
};

//-----------------------------------------------------------------------------
// Texture dictionary lists
//-----------------------------------------------------------------------------

RwTexture* FirstTexture(RwTexDictionary* dict) {
    if (!dict || fakerw::ListEmpty(dict->texturesInDict)) {
        return nullptr;
    }
    return rwLLLinkGetData(dict->texturesInDict.link.next, RwTexture, lInDictionary);
}

RwTexture* NextTexture(RwTexture* texture) {
    RwLLLink* next = texture->lInDictionary.next;
    auto*     dict = texture->dict;
    if (!dict || next == &dict->texturesInDict.link) {
        return nullptr;
    }
    return rwLLLinkGetData(next, RwTexture, lInDictionary);
}

//! Case-insensitive comparison of RW texture names (`RwTexDictionaryFindNamedTexture`, `0x7F39F0`
//! uppercases both sides as it walks them). Names never exceed `kTextureNameSize` characters.
bool NamesEqual(const RwChar* a, const RwChar* b) {
    for (uint32 i = 0; i < kTextureNameSize; i++) {
        const char left  = a[i];
        const char right = b[i];

        const char leftUpper  = left  >= 'a' && left  <= 'z' ? (char)(left  - ('a' - 'A')) : left;
        const char rightUpper = right >= 'a' && right <= 'z' ? (char)(right - ('a' - 'A')) : right;
        if (leftUpper != rightUpper) {
            return false;
        }
        if (!left || !right) {
            return true;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
// Raster copying
//-----------------------------------------------------------------------------

//! The GL3-specific raster data of a raster in the current platform. librw's
//! `GETGL3RASTEREXT` is a macro that expands to `PLUGINOFFSET`, so it cannot be called through
//! the namespace; this is its expansion.
rw::gl3::Gl3Raster* GetGl3RasterExt(rw::Raster* raster) {
    return reinterpret_cast<rw::gl3::Gl3Raster*>(reinterpret_cast<char*>(raster) + rw::gl3::nativeRasterOffset);
}

//! GL internal formats librw's GL3 backend uses for S3TC textures (`allocateDXT`).
int32 DxtFromInternalFormat(rw::int32 internalFormat) {
    switch (internalFormat) {
    case 0x83F0: // GL_COMPRESSED_RGB_S3TC_DXT1_EXT
    case 0x83F1: // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        return 1;
    case 0x83F2: // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        return 3;
    case 0x83F3: // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
        return 5;
    default:
        return 0;
    }
}

//! Byte size of one mip level, computed the way librw's GL3 `getLevelSize` does: stride and height
//! halve per level, but never below the 4x4 block size of a compressed raster.
size_t LevelSize(const rw::Raster& raster, int32 level, bool compressed) {
    const int32 minDim = compressed ? 4 : 1;
    int32       width  = raster.originalWidth;
    int32       height = raster.originalHeight;
    int32       stride = raster.originalStride;
    for (int32 i = 0; i < level; i++) {
        if (width > minDim) {
            width  /= 2;
            stride /= 2;
        }
        if (height > minDim) {
            height /= 2;
        }
    }
    return (size_t)stride * (size_t)height;
}

//! Copies `source` (already converted to the current platform) into a game raster.
//!
//! `RwRasterCreate` is the only entry point that registers a librw mirror for a game raster, so
//! the loaded texture cannot simply be adopted: its levels are copied into a raster created here
//! (compressed ones get librw's DXT allocator, which the plain create path cannot reach).
RwRaster* CopyRasterToGameRaster(rw::Raster* source, std::string_view name) {
    if (!source) {
        return nullptr;
    }

    if (source->platform != rw::platform) {
        NOTSA_LOG_WARN("Rw: texture '{}' is in platform {}, which this layer cannot read", name, source->platform);
        return nullptr;
    }

    auto*      ext        = GetGl3RasterExt(source);
    const bool compressed = ext->isCompressed != 0;
    const int32 levels    = source->getNumLevels();
    const int32 format    = source->format & (rwRASTERFORMATPIXELFORMATMASK | rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP);

    RwRaster* raster = nullptr;
    if (compressed) {
        const int32 dxt = DxtFromInternalFormat(ext->internalFormat);
        if (!dxt) {
            NOTSA_LOG_WARN("Rw: texture '{}' is compressed with the unknown GL format 0x{:X}", name, ext->internalFormat);
            return nullptr;
        }

        // Storage is allocated below through librw's DXT path, so create the mirror without any.
        raster = RwRasterCreate(source->originalWidth, source->originalHeight, 16,
                                rwRASTERTYPETEXTURE | format | rwRASTERDONTALLOCATE);
        if (!raster) {
            return nullptr;
        }
        rw::gl3::allocateDXT(fakerw::GetLibrwRaster(raster), dxt, levels, ext->hasAlpha);
    } else {
        raster = RwRasterCreate(source->originalWidth, source->originalHeight, source->depth,
                                rwRASTERTYPETEXTURE | format);
        if (!raster) {
            NOTSA_LOG_WARN("Rw: texture '{}' uses the raster format 0x{:X}, which has no GL mapping", name, format);
            return nullptr;
        }
    }

    auto*       target    = fakerw::GetLibrwRaster(raster);
    const int32 numLevels = std::min(levels, target->getNumLevels());
    for (int32 level = 0; level < numLevels; level++) {
        const size_t     size      = LevelSize(*source, level, compressed);
        const rw::uint8* srcPixels = source->lock(level, rw::Raster::LOCKREAD);
        rw::uint8*       dstPixels = target->lock(level, rw::Raster::LOCKWRITE | rw::Raster::LOCKNOFETCH);
        if (srcPixels && dstPixels) {
            std::memcpy(dstPixels, srcPixels, size);
        }
        target->unlock(level);
        source->unlock(level);
    }

    raster->width  = target->originalWidth;
    raster->height = target->originalHeight;
    raster->depth  = target->depth;
    raster->stride = target->originalStride;
    raster->cType  = (RwUInt8)(raster->cType & ~rwRASTERDONTALLOCATE);
    return raster;
}

//-----------------------------------------------------------------------------
// librw texture -> game texture
//-----------------------------------------------------------------------------

//! Builds the game texture for a texture librw just read, converting its raster to the current
//! platform first. The librw texture itself is left untouched (the caller destroys it).
RwTexture* CreateTextureFromLibrw(rw::Texture* source) {
    if (!source) {
        return nullptr;
    }

    if (source->raster) {
        source->raster = rw::Raster::convertTexToCurrentPlatform(source->raster);
    }

    auto* texture = static_cast<RwTexture*>(fakerw::AllocObject(sizeof(RwTexture), g_texturePlugins));
    if (!texture) {
        return nullptr;
    }

    const std::string_view name{source->name, BoundedStringLength(source->name, kTextureNameSize)};
    texture->raster = CopyRasterToGameRaster(source->raster, name);

    std::memcpy(texture->name, source->name, kTextureNameSize);
    std::memcpy(texture->mask, source->mask, kTextureNameSize);
    texture->name[kTextureNameSize - 1] = '\0';
    texture->mask[kTextureNameSize - 1] = '\0';

    // `RwTextureGtaStreamRead` (0x730E60) bumps the two cheapest filter modes, so textures read
    // from an archive are filtered rather than point sampled.
    RwUInt32 filterAddressing = source->filterAddressing;
    switch (filterAddressing & rwTEXTUREFILTERMODEMASK) {
    case rwFILTERNEAREST:
        filterAddressing = (filterAddressing & ~rwTEXTUREFILTERMODEMASK) | rwFILTERLINEAR;
        break;
    case rwFILTERMIPNEAREST:
        filterAddressing = (filterAddressing & ~rwTEXTUREFILTERMODEMASK) | rwFILTERMIPLINEAR;
        break;
    default:
        break;
    }
    texture->filterAddressing = filterAddressing;

    texture->dict     = nullptr;
    texture->refCount = 1; // `RwTextureCreate` (0x7F37C0) creates with one reference

    g_texturePlugins.Construct(texture, sizeof(RwTexture));
    return texture;
}

//-----------------------------------------------------------------------------
// TXD reading
//-----------------------------------------------------------------------------

//! Reads the texture dictionary struct: the texture count and the device id (which the retail
//! readers ignore).
bool ReadDictionaryHeader(RwStream* stream, uint32& textureCount) {
    RwUInt32 length = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, nullptr)) {
        NOTSA_LOG_WARN("Rw: a texture dictionary has no header chunk");
        return false;
    }
    if (length != 4) {
        NOTSA_LOG_WARN("Rw: a texture dictionary header is {} bytes, expected 4", length);
        return false;
    }

    uint16 header[2]{}; // texture count, device id
    if (RwStreamRead(stream, header, sizeof(header)) != sizeof(header)) {
        return false;
    }
    textureCount = header[0];
    return true;
}

bool AppendTextures(RwStream* stream, RwTexDictionary* dict, uint32 count) {
    for (uint32 i = 0; i < count; i++) {
        RwTexture* texture = RwTextureGtaStreamRead(stream);
        if (!texture) {
            return false;
        }
        RwTexDictionaryAddTexture(dict, texture);
    }
    return true;
}

//! Reads one `rwID_STRING`/`rwID_UNICODESTRING` chunk into `out`, skipping any other chunk like
//! `_rwStringStreamRead` (0x80A510) does. 16-bit strings keep the low byte of each character.
bool ReadTextureString(RwStream* stream, char* out, size_t outSize) {
    if (!stream || !out || outSize < 2) {
        return false;
    }
    std::memset(out, 0, outSize);

    RwChunkHeaderInfo header{};
    while (RwStreamReadChunkHeaderInfo(stream, &header)) {
        if (header.type != rwID_STRING && header.type != rwID_UNICODESTRING) {
            if (!RwStreamSkip(stream, header.length)) {
                return false;
            }
            continue;
        }

        const bool wide     = header.type == rwID_UNICODESTRING;
        const size_t step   = wide ? 2 : 1;
        size_t       stored = 0;
        RwUInt32     read   = 0;
        char         buffer[128];
        while (read < header.length) {
            const RwUInt32 chunk = std::min(header.length - read, (RwUInt32)sizeof(buffer));
            if (RwStreamRead(stream, buffer, chunk) != chunk) {
                return false;
            }
            read += chunk;

            for (RwUInt32 i = 0; i + step <= chunk; i += step) {
                const char character = buffer[i];
                if (character == '\0') {
                    return true;
                }
                if (stored + 1 < outSize) {
                    out[stored++] = character;
                }
            }
        }
        return true;
    }
    return false;
}

} // namespace

//-----------------------------------------------------------------------------
// Textures
//-----------------------------------------------------------------------------

RwTexture* RwTextureCreate(RwRaster* raster) {
    auto* texture = static_cast<RwTexture*>(fakerw::AllocObject(sizeof(RwTexture), g_texturePlugins));
    if (!texture) {
        return nullptr;
    }

    texture->raster           = raster;
    texture->dict             = nullptr;
    texture->filterAddressing = kDefaultFilterAddressing;
    texture->refCount         = 1;

    g_texturePlugins.Construct(texture, sizeof(RwTexture));
    return texture;
}

RwBool RwTextureDestroy(RwTexture* texture) {
    if (!texture) {
        return FALSE;
    }

    // Drops one reference and only frees the texture on the last one (0x7F3820).
    if (--texture->refCount > 0) {
        return TRUE;
    }

    if (texture->dict) {
        RwTexDictionaryRemoveTexture(texture);
    }

    if (texture->raster) {
        RwRasterDestroy(texture->raster);
        texture->raster = nullptr;
    }

    fakerw::FreeObject(texture, sizeof(RwTexture), g_texturePlugins);
    return TRUE;
}

RwTexture* RwTextureRead(const RwChar* name, const RwChar* maskName) {
    if (!name) {
        return nullptr;
    }

    // Already loaded? `RwTextureRead` (0x7F3AC0) first asks the find callback (the game's
    // `CTxdStore::TxdStoreFindCB` walks the current dictionary and its parents) and takes another
    // reference on a hit.
    if (g_findCallBack) {
        if (RwTexture* found = g_findCallBack(name)) {
            found->refCount++;
            return found;
        }
    }

    // Otherwise the read callback (the game's `CTxdStore::TxdStoreLoadCB`) would load it; it never
    // does, textures arrive through the streamed dictionaries.
    RwTexture* texture = g_readCallBack ? g_readCallBack(name, maskName) : nullptr;
    if (!texture) {
        if (maskName) {
            NOTSA_LOG_WARN("Rw: texture '{}' (mask '{}') is not in any loaded texture dictionary", name, maskName);
        } else {
            NOTSA_LOG_WARN("Rw: texture '{}' is not in any loaded texture dictionary", name);
        }
        return nullptr;
    }

    if (g_currentTexDictionary) {
        RwTexDictionaryAddTexture(g_currentTexDictionary, texture);
    }
    return texture;
}

//! Reads one native texture chunk (`rwID_TEXTURENATIVE`) — `RwTextureGtaStreamRead`, 0x730E60.
RwTexture* RwTextureGtaStreamRead(RwStream* stream) {
    if (!stream) {
        return nullptr;
    }

    if (!RwStreamFindChunk(stream, rwID_TEXTURENATIVE, nullptr, nullptr)) {
        NOTSA_LOG_WARN("Rw: a texture dictionary chunk does not start with a texture");
        return nullptr;
    }

    // The texture's struct starts with its platform id. librw dispatches on it but asserts on
    // platforms it does not know, so reject those first (the game's archives are D3D9).
    RwUInt32 structLength = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &structLength, nullptr) || structLength < sizeof(RwUInt32)) {
        NOTSA_LOG_WARN("Rw: a texture chunk has no struct");
        return nullptr;
    }

    RwUInt32 platform = 0;
    const bool readPlatform = RwStreamRead(stream, &platform, sizeof(platform)) == sizeof(platform);
    // Back to just before the struct header, which is where librw's reader expects to start.
    const bool rewound = fakerw::SeekRwStream(stream, -(int32)(sizeof(platform) + kChunkHeaderSize), 1);
    if (!readPlatform || !rewound) {
        return nullptr;
    }
    if (platform != rw::PLATFORM_D3D9) {
        NOTSA_LOG_WARN("Rw: texture platform {} is not supported (the archives must be D3D9)", platform);
        return nullptr;
    }

    GameStream   bridge{*stream};
    rw::Texture* loaded = rw::Texture::streamReadNative(&bridge);
    if (!loaded) {
        NOTSA_LOG_WARN("Rw: failed to read a texture from a texture dictionary");
        return nullptr;
    }

    RwTexture* texture = CreateTextureFromLibrw(loaded);
    loaded->destroy(); // releases the librw texture; the game texture owns the copied raster
    return texture;
}

//! Reads a texture dictionary — `RwTexDictionaryGtaStreamRead`, 0x730FC0.
RwTexDictionary* RwTexDictionaryGtaStreamRead(RwStream* stream) {
    uint32 textureCount = 0;
    if (!stream || !ReadDictionaryHeader(stream, textureCount)) {
        return nullptr;
    }

    RwTexDictionary* dict = RwTexDictionaryCreate();
    if (!dict) {
        return nullptr;
    }

    if (!AppendTextures(stream, dict, textureCount)) {
        RwTexDictionaryDestroy(dict);
        return nullptr;
    }
    return dict;
}

//! Starts reading a texture dictionary for the streaming loader — `0x731070`.
//! Reads the dictionary header plus the first half of the textures, then remembers where the
//! remaining ones start so `RwTexDictionaryGtaStreamRead2` can continue from a buffer that
//! covers the same model file.
RwTexDictionary* RwTexDictionaryGtaStreamRead1(RwStream* stream) {
    g_remainingTextures = -1;
    g_resumePosition    = 0;

    uint32 textureCount = 0;
    if (!stream || !ReadDictionaryHeader(stream, textureCount)) {
        return nullptr;
    }

    RwTexDictionary* dict = RwTexDictionaryCreate();
    if (!dict) {
        return nullptr;
    }

    const uint32 firstHalf = textureCount - textureCount / 2;
    if (!AppendTextures(stream, dict, firstHalf)) {
        RwTexDictionaryDestroy(dict);
        return nullptr;
    }

    g_resumePosition    = stream->position;
    g_remainingTextures = (int32)(textureCount - firstHalf);
    return dict;
}

//! Finishes a dictionary started by `RwTexDictionaryGtaStreamRead1` — `0x731150`.
RwTexDictionary* RwTexDictionaryGtaStreamRead2(RwStream* stream, RwTexDictionary* dict) {
    if (!stream || !dict) {
        return nullptr;
    }

    // The finishing read gets its own stream over the start of the model buffer.
    const int32 toSkip = g_resumePosition - stream->position;
    if (toSkip > 0 && !RwStreamSkip(stream, (RwUInt32)toSkip)) {
        NOTSA_LOG_WARN("Rw: failed to resume a texture dictionary at offset {}", g_resumePosition);
        RwTexDictionaryDestroy(dict);
        return nullptr;
    }

    const int32 remaining = g_remainingTextures;
    g_remainingTextures = -1;
    if (!AppendTextures(stream, dict, (uint32)std::max(remaining, 0))) {
        RwTexDictionaryDestroy(dict);
        return nullptr;
    }
    return dict;
}

//! Reads a texture dictionary in the standard RW 3.6 layout, which is what the GTA readers above
//! handle: a 4-byte dictionary struct followed by the native texture chunks.
RwTexDictionary* RwTexDictionaryStreamRead(RwStream* stream) {
    return RwTexDictionaryGtaStreamRead(stream);
}

//! Reads a generic `rwID_TEXTURE` chunk — `RwTextureStreamRead`, 0x8046E0. The texture is
//! resolved through `RwTextureRead` (find callback, then read callback), exactly like the retail
//! reader; the platform raster chunk that follows belongs to RW's texture plugin stream, which
//! this layer does not have. SA archives store their textures as `rwID_TEXTURENATIVE` chunks
//! instead, which `RwTextureGtaStreamRead` handles.
RwTexture* RwTextureStreamRead(RwStream* stream) {
    if (!stream) {
        return nullptr;
    }

    RwUInt32 length  = 0;
    RwUInt32 version = 0;
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, &version)) {
        return nullptr;
    }
    if (version < kMinLibraryVersion || version > kMaxLibraryVersion) {
        NOTSA_LOG_WARN("Rw: texture chunk has the unsupported RW version 0x{:X}", version);
        return nullptr;
    }
    if (length != sizeof(RwUInt32)) {
        NOTSA_LOG_WARN("Rw: texture chunk struct is {} bytes, expected 4", length);
        return nullptr;
    }

    RwUInt32 filterAddressing = 0;
    if (RwStreamRead(stream, &filterAddressing, sizeof(filterAddressing)) != sizeof(filterAddressing)) {
        return nullptr;
    }
    if ((filterAddressing & rwTEXTUREADDRESSINGVMASK) == 0) {
        // V addressing unset: use U's, like `Texture::streamRead` in librw.
        filterAddressing |= (filterAddressing & rwTEXTUREADDRESSINGUMASK) << 4;
    }

    char name[kTextureNameSize]{};
    char mask[kTextureNameSize]{};
    if (!ReadTextureString(stream, name, sizeof(name)) || !ReadTextureString(stream, mask, sizeof(mask))) {
        return nullptr;
    }

    RwTexture* texture = RwTextureRead(name, mask);
    if (!texture) {
        return nullptr;
    }

    if (texture->refCount == 1) {
        texture->filterAddressing = filterAddressing & 0xFFFF;
    }

    fakerw::WarnOnce("Rw: RwTextureStreamRead does not read the platform raster chunk");
    return texture;
}

RwTexture* RwTextureSetName(RwTexture* texture, const RwChar* name) {
    if (!texture) {
        return nullptr;
    }
    std::strncpy(texture->name, name ? name : "", sizeof(texture->name) - 1);
    texture->name[sizeof(texture->name) - 1] = '\0';
    return texture;
}

RwTexture* RwTextureSetRaster(RwTexture* texture, RwRaster* raster) {
    if (texture) {
        texture->raster = raster;
    }
    return texture;
}

RwBool RwTextureSetMipmapping(RwBool enable) {
    UNUSED(enable);
    return TRUE; // The GL backend decides this per texture
}

RwBool RwTextureSetAutoMipmapping(RwBool enable) {
    UNUSED(enable);
    return TRUE;
}

RwBool RwTextureSetFindCallBack(RwTextureCallBackFind callBack) {
    g_findCallBack = callBack;
    return TRUE;
}

RwTextureCallBackFind RwTextureGetFindCallBack() {
    return g_findCallBack;
}

RwBool RwTextureSetReadCallBack(RwTextureCallBackRead callBack) {
    g_readCallBack = callBack;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Texture dictionaries
//-----------------------------------------------------------------------------

RwTexDictionary* RwTexDictionaryCreate() {
    auto* dict = static_cast<RwTexDictionary*>(fakerw::AllocObject(sizeof(RwTexDictionary), g_texDictionaryPlugins));
    if (!dict) {
        return nullptr;
    }

    rwObjectInitialize(&dict->object, rwID_TEXDICTIONARY, 0);
    fakerw::ListInit(dict->texturesInDict);
    dict->lInInstance.next = nullptr;
    dict->lInInstance.prev = nullptr;

    g_texDictionaryPlugins.Construct(dict, sizeof(RwTexDictionary));
    return dict;
}

RwBool RwTexDictionaryDestroy(RwTexDictionary* dict) {
    if (!dict) {
        return FALSE;
    }

    while (!fakerw::ListEmpty(dict->texturesInDict)) {
        auto* texture = FirstTexture(dict);
        // Detach first, so a texture that is still referenced keeps a valid `dict` pointer and
        // does not stay in a dictionary that is about to disappear.
        fakerw::ListRemove(texture->lInDictionary);
        texture->dict = nullptr;
        RwTextureDestroy(texture);
    }

    if (g_currentTexDictionary == dict) {
        g_currentTexDictionary = nullptr;
    }

    fakerw::FreeObject(dict, sizeof(RwTexDictionary), g_texDictionaryPlugins);
    return TRUE;
}

RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dict, RwTexture* texture) {
    if (!dict || !texture) {
        return nullptr;
    }

    // `RwTexDictionaryAddTexture` (0x7F3980) always unlinks before appending, which also moves a
    // texture that already belongs to this dictionary to the end.
    if (texture->dict) {
        fakerw::ListRemove(texture->lInDictionary);
    }

    texture->dict = dict;
    fakerw::ListAppend(dict->texturesInDict, texture->lInDictionary);
    return texture;
}

RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture) {
    if (!texture || !texture->dict) {
        return nullptr;
    }

    fakerw::ListRemove(texture->lInDictionary);
    texture->dict = nullptr;
    return texture;
}

RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* dict, const RwChar* name) {
    if (!dict || !name) {
        return nullptr;
    }

    for (RwTexture* texture = FirstTexture(dict); texture; texture = NextTexture(texture)) {
        if (NamesEqual(texture->name, name)) {
            return texture;
        }
    }
    return nullptr;
}

const RwTexDictionary* RwTexDictionaryForAllTextures(
    const RwTexDictionary* dict,
    RwTextureCallBack      fpCallBack,
    void*                  data
) {
    if (!dict || !fpCallBack) {
        return dict;
    }

    auto* mutableDict = const_cast<RwTexDictionary*>(dict);
    for (RwTexture* texture = FirstTexture(mutableDict); texture;) {
        // The callback may remove the texture from the dictionary.
        RwTexture* next = NextTexture(texture);
        if (!fpCallBack(texture, data)) {
            break;
        }
        texture = next;
    }
    return dict;
}

RwTexDictionary* RwTexDictionaryGetCurrent() {
    return g_currentTexDictionary;
}

RwTexDictionary* RwTexDictionarySetCurrent(RwTexDictionary* dict) {
    RwTexDictionary* previous = g_currentTexDictionary;
    g_currentTexDictionary = dict;
    return previous;
}

RwInt32 RwTexDictionaryRegisterPlugin(
    RwInt32                   size,
    RwUInt32                  pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor  destructCB,
    RwPluginObjectCopy        copyCB
) {
    return g_texDictionaryPlugins.Register(sizeof(RwTexDictionary), size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RwTexDictionaryRegisterPluginStream(
    RwUInt32                         pluginID,
    RwPluginDataChunkReadCallBack    readCB,
    RwPluginDataChunkWriteCallBack   writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB
) {
    return g_texDictionaryPlugins.RegisterStream(pluginID, readCB, writeCB, getSizeCB);
}

RwTexture* GetFirstTexture(RwTexDictionary* dict) {
    return FirstTexture(dict);
}

#endif // NOTSA_NATIVE
