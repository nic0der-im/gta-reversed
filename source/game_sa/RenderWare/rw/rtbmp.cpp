#include "StdInc.h"

#include "rtbmp.h"

RwImage* RtBMPImageWrite(RwImage* image, const RwChar* imageName) {
    return plugin::CallAndReturn<RwImage*, 0x7CE990, RwImage*, const RwChar*>(image, imageName);
}

RwImage* RtBMPImageRead(const RwChar* imageName) {
    return plugin::CallAndReturn<RwImage*, 0x7CDF60, const RwChar*>(imageName);
}
