#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

//! Device-independent images. Used by screenshots and by code that builds textures from raw
//! pixels; the pixels live in the game allocator.

namespace {

RwInt32 ImageStride(RwInt32 width, RwInt32 depth) {
    return (width * depth + 7) / 8;
}

void FillLibrwImage(rw::Image& dst, const RwImage& src) {
    dst.flags   = src.flags;
    dst.width   = src.width;
    dst.height  = src.height;
    dst.depth   = src.depth;
    dst.bpp     = src.depth / 8;
    dst.stride  = src.stride;
    dst.pixels  = src.cpPixels;
    dst.palette = reinterpret_cast<rw::uint8*>(src.palette);
}

} // namespace

RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth) {
    auto* image = static_cast<RwImage*>(notsa::fakerw::Malloc(sizeof(RwImage)));
    if (!image) {
        return nullptr;
    }

    std::memset(image, 0, sizeof(RwImage));
    image->width  = width;
    image->height = height;
    image->depth  = depth;
    image->stride = ImageStride(width, depth);
    return image;
}

RwBool RwImageDestroy(RwImage* image) {
    if (!image) {
        return FALSE;
    }
    RwImageFreePixels(image);
    notsa::fakerw::Free(image);
    return TRUE;
}

RwImage* RwImageAllocatePixels(RwImage* image) {
    if (!image) {
        return nullptr;
    }

    if (!image->cpPixels) {
        const size_t size = (size_t)image->stride * (size_t)image->height;
        image->cpPixels = static_cast<RwUInt8*>(notsa::fakerw::Malloc(size));
        if (!image->cpPixels) {
            return nullptr;
        }
        std::memset(image->cpPixels, 0, size);
    }
    return image;
}

RwImage* RwImageFreePixels(RwImage* image) {
    if (!image) {
        return nullptr;
    }
    if (image->cpPixels) {
        notsa::fakerw::Free(image->cpPixels);
        image->cpPixels = nullptr;
    }
    return image;
}

RwImage* RwImageFindRasterFormat(
    RwImage* ipImage,
    RwInt32  nRasterType,
    RwInt32* npWidth,
    RwInt32* npHeight,
    RwInt32* npDepth,
    RwInt32* npFormat
) {
    if (!ipImage) {
        return nullptr;
    }

    rw::Image source{};
    FillLibrwImage(source, *ipImage);

    RwInt32 width = 0, height = 0, depth = 0, format = 0;
    if (!rw::Raster::imageFindRasterFormat(&source, nRasterType, &width, &height, &depth, &format)) {
        return nullptr;
    }

    if (npWidth)  *npWidth  = width;
    if (npHeight) *npHeight = height;
    if (npDepth)  *npDepth  = depth;
    if (npFormat) *npFormat = format;
    return ipImage;
}

RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster) {
    auto* librwRaster = notsa::fakerw::GetLibrwRaster(raster);
    if (!image || !librwRaster) {
        return nullptr;
    }

    auto* source = librwRaster->toImage();
    if (!source) {
        return nullptr;
    }

    image->flags   = 0;
    image->width   = source->width;
    image->height  = source->height;
    image->depth   = source->depth;
    image->stride  = source->stride;

    RwImageFreePixels(image);
    const size_t size = (size_t)image->stride * (size_t)image->height;
    image->cpPixels = static_cast<RwUInt8*>(notsa::fakerw::Malloc(size));
    if (image->cpPixels) {
        std::memcpy(image->cpPixels, source->pixels, size);
    }

    source->destroy();
    return image;
}

#endif // NOTSA_NATIVE
