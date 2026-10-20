#include "StdInc.h"

#include "rtquat.h"

RwBool RtQuatConvertFromMatrix(RtQuat* qpQuat, const RwMatrix* const mpMatrix) {
    return plugin::CallAndReturn<RwBool, 0x7EB5C0, RtQuat*, const RwMatrix* const>(qpQuat, mpMatrix);
}

RtQuat* RtQuatRotate(RtQuat* quat, const RwV3d* axis, RwReal angle, RwOpCombineType combineOp) {
    return plugin::CallAndReturn<RtQuat*, 0x7EB7C0, RtQuat*, const RwV3d*, RwReal, RwOpCombineType>(quat, axis, angle, combineOp);
}

const RtQuat* RtQuatQueryRotate(const RtQuat* quat, RwV3d* unitAxis, RwReal* angle) {
    return plugin::CallAndReturn<const RtQuat*, 0x7EBA80, const RtQuat*, RwV3d*, RwReal*>(quat, unitAxis, angle);
}

RwV3d* RtQuatTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, const RwInt32 numPoints, const RtQuat* quat) {
    return plugin::CallAndReturn<RwV3d*, 0x7EBBB0, RwV3d*, const RwV3d*, const RwInt32, const RtQuat*>(vectorsOut, vectorsIn, numPoints, quat);
}

RwReal RtQuatModulus(RtQuat* q) {
    return plugin::CallAndReturn<RwReal, 0x7EBD10, RtQuat*>(q);
}
