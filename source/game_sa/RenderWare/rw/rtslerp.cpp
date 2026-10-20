#include "StdInc.h"

#include "rtslerp.h"

void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache) {
    plugin::Call<0x7EC220, RtQuat*, RtQuat*, RtQuatSlerpCache*>(qpFrom, qpTo, sCache);
}
