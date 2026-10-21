#include "StdInc.h"

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

//! Matrix and vector math shared by the game and the frame/camera code.
//!
//! RenderWare matrices are stored as four rows (`right`, `up`, `at`, `pos`) and transform row
//! vectors (`v' = v * M`), which is why the multiplication below expands over the rows.

namespace {

constexpr RwReal kPi = 3.14159265358979323846f;

void SetIdentity(RwMatrix& m) {
    m.right = { 1.0f, 0.0f, 0.0f };
    m.up    = { 0.0f, 1.0f, 0.0f };
    m.at    = { 0.0f, 0.0f, 1.0f };
    m.pos   = { 0.0f, 0.0f, 0.0f };
}

void Multiply(RwMatrix& out, const RwMatrix& a, const RwMatrix& b) {
    RwMatrix tmp;

    const auto mulRow = [&b](RwV3d& dst, const RwV3d& row, RwReal posScale) {
        dst.x = row.x * b.right.x + row.y * b.up.x + row.z * b.at.x + posScale * b.pos.x;
        dst.y = row.x * b.right.y + row.y * b.up.y + row.z * b.at.y + posScale * b.pos.y;
        dst.z = row.x * b.right.z + row.y * b.up.z + row.z * b.at.z + posScale * b.pos.z;
    };

    mulRow(tmp.right, a.right, 0.0f);
    mulRow(tmp.up,    a.up,    0.0f);
    mulRow(tmp.at,    a.at,    0.0f);
    mulRow(tmp.pos,   a.pos,   1.0f);

    tmp.flags = a.flags & b.flags;
    if (tmp.flags & rwMATRIXINTERNALIDENTITY) {
        tmp.flags &= ~rwMATRIXINTERNALIDENTITY;
    }

    out = tmp;
}

void MakeRotation(RwMatrix& out, const RwV3d& axis, RwReal angleDegrees) {
    RwReal len = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    len = len != 0.0f ? 1.0f / std::sqrt(len) : 0.0f;

    const RwReal x = axis.x * len;
    const RwReal y = axis.y * len;
    const RwReal z = axis.z * len;

    const RwReal angle = angleDegrees * kPi / 180.0f;
    const RwReal s     = std::sin(angle);
    const RwReal c     = std::cos(angle);
    const RwReal t     = 1.0f - c;

    out.right = { c + x * x * t,     x * y * t + z * s, z * x * t - y * s };
    out.up    = { x * y * t - z * s, c + y * y * t,     y * z * t + x * s };
    out.at    = { z * x * t + y * s, y * z * t - x * s, c + z * z * t };
    out.pos   = { 0.0f, 0.0f, 0.0f };
    out.flags = rwMATRIXTYPEORTHONORMAL;
}

void ApplyCombine(RwMatrix& matrix, const RwMatrix& transform, RwOpCombineType op) {
    RwMatrix tmp;
    switch (op) {
    case rwCOMBINEREPLACE:
        matrix = transform;
        break;
    case rwCOMBINEPRECONCAT:
        Multiply(tmp, transform, matrix);
        matrix = tmp;
        break;
    case rwCOMBINEPOSTCONCAT:
        Multiply(tmp, matrix, transform);
        matrix = tmp;
        break;
    }
}

} // namespace

RwMatrix* RwMatrixCreate() {
    auto* matrix = static_cast<RwMatrix*>(notsa::fakerw::Malloc(sizeof(RwMatrix)));
    if (!matrix) {
        return nullptr;
    }
    std::memset(matrix, 0, sizeof(RwMatrix));
    SetIdentity(*matrix);
    matrix->flags = rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL;
    return matrix;
}

RwBool RwMatrixDestroy(RwMatrix* mpMat) {
    if (!mpMat) {
        return FALSE;
    }
    notsa::fakerw::Free(mpMat);
    return TRUE;
}

RwMatrix* RwMatrixUpdate(RwMatrix* matrix) {
    if (!matrix) {
        return nullptr;
    }
    if (matrix->flags & rwMATRIXINTERNALIDENTITY) {
        SetIdentity(*matrix);
        matrix->flags &= ~rwMATRIXINTERNALIDENTITY;
    }
    return matrix;
}

RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* MatrixIn1, const RwMatrix* matrixIn2) {
    if (!matrixOut || !MatrixIn1 || !matrixIn2) {
        return nullptr;
    }
    RwMatrix out;
    Multiply(out, *MatrixIn1, *matrixIn2);
    *matrixOut = out;
    return matrixOut;
}

RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn) {
    if (!matrixOut || !matrixIn) {
        return nullptr;
    }

    if (matrixIn->flags & rwMATRIXINTERNALIDENTITY) {
        *matrixOut = *matrixIn;
        SetIdentity(*matrixOut);
        matrixOut->flags &= ~rwMATRIXINTERNALIDENTITY;
        return matrixOut;
    }

    const auto& r = matrixIn->right;
    const auto& u = matrixIn->up;
    const auto& a = matrixIn->at;

    const RwReal det = r.x * (u.y * a.z - u.z * a.y)
                     - u.x * (r.y * a.z - r.z * a.y)
                     + a.x * (r.y * u.z - r.z * u.y);

    if (det == 0.0f) {
        return nullptr; // Singular; leave the output untouched like RenderWare does.
    }

    const RwReal invDet = 1.0f / det;

    RwMatrix out;
    out.right.x =  (u.y * a.z - u.z * a.y) * invDet;
    out.right.y = -(r.y * a.z - r.z * a.y) * invDet;
    out.right.z =  (r.y * u.z - r.z * u.y) * invDet;

    out.up.x = -(u.x * a.z - u.z * a.x) * invDet;
    out.up.y =  (r.x * a.z - r.z * a.x) * invDet;
    out.up.z = -(r.x * u.z - r.z * u.x) * invDet;

    out.at.x =  (u.x * a.y - u.y * a.x) * invDet;
    out.at.y = -(r.x * a.y - r.y * a.x) * invDet;
    out.at.z =  (r.x * u.y - r.y * u.x) * invDet;

    out.pos.x = -(matrixIn->pos.x * out.right.x + matrixIn->pos.y * out.up.x + matrixIn->pos.z * out.at.x);
    out.pos.y = -(matrixIn->pos.x * out.right.y + matrixIn->pos.y * out.up.y + matrixIn->pos.z * out.at.y);
    out.pos.z = -(matrixIn->pos.x * out.right.z + matrixIn->pos.y * out.up.z + matrixIn->pos.z * out.at.z);

    out.flags = matrixIn->flags & ~rwMATRIXINTERNALIDENTITY;
    *matrixOut = out;
    return matrixOut;
}

RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle, RwOpCombineType combineOp) {
    if (!matrix || !axis) {
        return nullptr;
    }
    RwMatrix rotation;
    MakeRotation(rotation, *axis, angle);
    ApplyCombine(*matrix, rotation, combineOp);
    return matrix;
}

RwMatrix* RwMatrixScale(RwMatrix* matrix, const RwV3d* scale, RwOpCombineType combineOp) {
    if (!matrix || !scale) {
        return nullptr;
    }

    RwMatrix scaling;
    SetIdentity(scaling);
    scaling.right.x = scale->x;
    scaling.up.y    = scale->y;
    scaling.at.z    = scale->z;
    scaling.flags   = rwMATRIXTYPENORMAL;

    ApplyCombine(*matrix, scaling, combineOp);
    return matrix;
}

RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation, RwOpCombineType combineOp) {
    if (!matrix || !translation) {
        return nullptr;
    }

    RwMatrix translationMatrix;
    SetIdentity(translationMatrix);
    translationMatrix.pos   = *translation;
    translationMatrix.flags = rwMATRIXTYPENORMAL;

    ApplyCombine(*matrix, translationMatrix, combineOp);
    return matrix;
}

RwMatrix* RwMatrixTransform(RwMatrix* matrix, const RwMatrix* transform, RwOpCombineType combineOp) {
    if (!matrix || !transform) {
        return nullptr;
    }
    ApplyCombine(*matrix, *transform, combineOp);
    return matrix;
}

RwV3d* RwV3dTransformPoint(RwV3d* pointOut, const RwV3d* pointIn, const RwMatrix* matrix) {
    if (!pointOut || !pointIn || !matrix) {
        return nullptr;
    }

    const RwV3d in = *pointIn;
    pointOut->x = in.x * matrix->right.x + in.y * matrix->up.x + in.z * matrix->at.x + matrix->pos.x;
    pointOut->y = in.x * matrix->right.y + in.y * matrix->up.y + in.z * matrix->at.y + matrix->pos.y;
    pointOut->z = in.x * matrix->right.z + in.y * matrix->up.z + in.z * matrix->at.z + matrix->pos.z;
    return pointOut;
}

RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix) {
    for (RwInt32 i = 0; i < numPoints; i++) {
        RwV3dTransformPoint(&pointsOut[i], &pointsIn[i], matrix);
    }
    return pointsOut;
}

RwV3d* RwV3dTransformVector(RwV3d* vectorOut, const RwV3d* vectorIn, const RwMatrix* matrix) {
    if (!vectorOut || !vectorIn || !matrix) {
        return nullptr;
    }

    const RwV3d in = *vectorIn;
    vectorOut->x = in.x * matrix->right.x + in.y * matrix->up.x + in.z * matrix->at.x;
    vectorOut->y = in.x * matrix->right.y + in.y * matrix->up.y + in.z * matrix->at.y;
    vectorOut->z = in.x * matrix->right.z + in.y * matrix->up.z + in.z * matrix->at.z;
    return vectorOut;
}

RwV3d* RwV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints, const RwMatrix* matrix) {
    for (RwInt32 i = 0; i < numPoints; i++) {
        RwV3dTransformVector(&vectorsOut[i], &vectorsIn[i], matrix);
    }
    return vectorsOut;
}

RwReal RwV3dLength(const RwV3d* in) {
    return std::sqrt(in->x * in->x + in->y * in->y + in->z * in->z);
}

RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in) {
    const RwReal len = RwV3dLength(in);
    if (len != 0.0f) {
        const RwReal inv = 1.0f / len;
        out->x = in->x * inv;
        out->y = in->y * inv;
        out->z = in->z * inv;
    } else {
        *out = { 0.0f, 0.0f, 0.0f };
    }
    return len;
}

RwReal RwV2dLength(const RwV2d* in) {
    return std::sqrt(in->x * in->x + in->y * in->y);
}

#endif // NOTSA_NATIVE
