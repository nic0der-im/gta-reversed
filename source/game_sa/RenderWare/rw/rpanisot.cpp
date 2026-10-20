#include "StdInc.h"

#include "rpanisot.h"

RwInt8 RpAnisotGetMaxSupportedMaxAnisotropy() {
    return plugin::CallAndReturn<RwInt8, 0x748F20>();
}

RwTexture* RpAnisotTextureSetMaxAnisotropy(RwTexture* tex, RwInt8 val) {
    return plugin::CallAndReturn<RwTexture*, 0x748F30, RwTexture*, RwInt8>(tex, val);
}

RwInt8 RpAnisotTextureGetMaxAnisotropy(RwTexture* tex) {
    return plugin::CallAndReturn<RwInt8, 0x748F50, RwTexture*>(tex);
}

RwBool RpAnisotPluginAttach() {
    return plugin::CallAndReturn<RwBool, 0x748F70>();
}
