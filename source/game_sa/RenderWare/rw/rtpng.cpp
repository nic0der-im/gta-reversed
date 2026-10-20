#include "StdInc.h"

#include "rtpng.h"

RwImage *RtPNGImageWrite(RwImage *image, const RwChar *imageName) {
    return plugin::CallAndReturn<RwImage *, 0x7CF600, RwImage *, const RwChar *>(image, imageName);
}

RwImage *RtPNGImageRead(const RwChar *imageName) {
    return plugin::CallAndReturn<RwImage *, 0x7CF9B0, const RwChar *>(imageName);
}
