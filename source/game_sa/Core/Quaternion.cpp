/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Quaternion.h"

void CQuaternion::InjectHooks() {
    RH_ScopedClass(CQuaternion);
    RH_ScopedCategory("Core");

    RH_ScopedOverloadedInstall(Get, "", 0x59C080, void (CQuaternion::*)(RwMatrix*) const);
    RH_ScopedOverloadedInstall(Get, "euler", 0x59C160, void (CQuaternion::*)(float*, float*, float*));
    RH_ScopedOverloadedInstall(Get, "axisAngle", 0x59C230, void (CQuaternion::*)(RwV3d*, float*));
    RH_ScopedInstall(Multiply, 0x59C270);
    RH_ScopedOverloadedInstall(Slerp, "halfTheta", 0x59C300, void (CQuaternion::*)(const CQuaternion&, const CQuaternion&, float, float, float));
    RH_ScopedOverloadedInstall(Set, "matrix", 0x59C3E0, void (CQuaternion::*)(const RwMatrix&));
    RH_ScopedOverloadedInstall(Set, "euler", 0x59C530, void (CQuaternion::*)(float, float, float));
    RH_ScopedOverloadedInstall(Set, "axisAngle", 0x59C600, void (CQuaternion::*)(RwV3d*, float));
    RH_ScopedOverloadedInstall(Slerp, "", 0x59C630, void (CQuaternion::*)(const CQuaternion&, const CQuaternion&, float));
    RH_ScopedInstall(Conjugate, 0x4D37D0);
    RH_ScopedInstall(Scale, 0x4CF9B0);
    RH_ScopedInstall(Copy, 0x4CF9E0);
}

// Quat to matrix
void CQuaternion::Get(RwMatrix* out) const {
    auto vecImag2 = imag + imag;
    auto x2x = vecImag2.x * imag.x;
    auto y2x = vecImag2.y * imag.x;
    auto z2x = vecImag2.z * imag.x;

    auto y2y = vecImag2.y * imag.y;
    auto z2y = vecImag2.z * imag.y;
    auto z2z = vecImag2.z * imag.z;

    auto x2r = vecImag2.x * real;
    auto y2r = vecImag2.y * real;
    auto z2r = vecImag2.z * real;

    CVector right{1.0F - (z2z + y2y), z2r + y2x, z2x - y2r}, up{y2x - z2r, 1.0F - (z2z + x2x), x2r + z2y}, at{y2r + z2x, z2y - x2r, 1.0F - (y2y + x2x)};
    RwV3dAssign(RwMatrixGetRight(out), &right);
    RwV3dAssign(RwMatrixGetUp(out), &up);
    RwV3dAssign(RwMatrixGetAt(out), &at);
}

// 0x59C160
// Quat to euler angles
void CQuaternion::Get(float* x, float* y, float* z) {
    RwMatrix m;
    Get(&m);

    // NOTE: The exe wraps negative angles by adding 2π (it doesn't use `LimitRadianAngle`)
    const auto wrap = [](float angle) { return angle < 0.0f ? angle + TWO_PI : angle; };

    *z = wrap(std::atan2(m.right.y, m.up.y));

    const auto s = std::sin(*z), c = std::cos(*z);

    *x = wrap(std::atan2(-m.at.y, m.right.y * s + m.up.y * c));
    *y = wrap(std::atan2(m.up.z * s - m.right.z * c, m.right.x * c - m.up.x * s));
}

// 0x59C230
// Quat to axis & angle
void CQuaternion::Get(RwV3d* axis, float* angle) {
    *angle = (float)std::acos((double)(2.0f * w)); // NOTE: The argument is doubled before `acos` (sic)

    const auto invSin = 1.0f / std::sin(*angle);
    axis->x = invSin * x;
    axis->y = invSin * y;
    axis->z = invSin * z;
}

// 0x59C270
// Stores result of quat multiplication
void CQuaternion::Multiply(const CQuaternion& a, const CQuaternion& b) {
    // NOTE: The vector part is stored first, and the real part is added to it afterwards
    // (so aliasing `this` with `a`/`b` gives the same (wrong) result as the exe)
    x = b.z * a.y - a.z * b.y;
    y = a.z * b.x - a.x * b.z;
    z = a.x * b.y - b.x * a.y;

    x += a.w * b.x + b.w * a.x;
    y += a.w * b.y + b.w * a.y;
    z += a.w * b.z + b.w * a.z;

    w = a.w * b.w - (a.x * b.x + a.y * b.y + a.z * b.z);
}

// 0x59C300
// Spherical linear interpolation
void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float halfTheta, float sinThetaInv, float t) {
    if (halfTheta == 0.0f) {
        *this = to;
        return;
    }

    float fromWeight, toWeight;
    if (halfTheta <= PI / 2.0f) {
        fromWeight = std::sin((1.0f - t) * halfTheta) * sinThetaInv;
        toWeight   = std::sin(halfTheta * t) * sinThetaInv;
    } else {
        // Use the supplementary angle (always interpolate the short way)
        const auto sup = PI - halfTheta;
        fromWeight = std::sin((1.0f - t) * sup) * sinThetaInv;
        toWeight   = -(std::sin(sup * t) * sinThetaInv);
    }

    x = fromWeight * from.x + toWeight * to.x;
    y = fromWeight * from.y + toWeight * to.y;
    z = fromWeight * from.z + toWeight * to.z;
    w = fromWeight * from.w + toWeight * to.w;
}

// 0x59C3E0
// Quat from matrix (Shepperd's method)
void CQuaternion::Set(const RwMatrix& m) {
    float k;

    const auto trace = m.up.y + m.right.x + m.at.z;
    if (trace >= 0.0f) {
        k = std::sqrt(trace + 1.0f);
        w = k * 0.5f;
        k = 0.5f / k;
        x = (m.up.z - m.at.y) * k;
        y = (m.at.x - m.right.z) * k;
        z = (m.right.y - m.up.x) * k;
        return;
    }

    const auto dx = (m.right.x - m.up.y) - m.at.z;
    if (dx >= 0.0f) {
        k = std::sqrt(dx + 1.0f);
        x = k * 0.5f;
        k = 0.5f / k;
        y = (m.up.x + m.right.y) * k;
        z = (m.at.x + m.right.z) * k;
        w = (m.up.z - m.at.y) * k;
        return;
    }

    const auto dy = (m.up.y - m.right.x) - m.at.z;
    if (dy >= 0.0f) {
        k = std::sqrt(dy + 1.0f);
        y = k * 0.5f;
        k = 0.5f / k;
        w = (m.at.x - m.right.z) * k;
        x = (m.up.x - m.right.y) * k;
        z = (m.at.y + m.up.z) * k;
        return;
    }

    k = std::sqrt((m.at.z - (m.up.y + m.right.x)) + 1.0f);
    z = k * 0.5f;
    k = 0.5f / k;
    w = (m.right.y - m.up.x) * k;
    x = (m.at.x + m.right.z) * k;
    y = (m.at.y + m.up.z) * k;
}

// 0x59C530
// Quat from euler angles
void CQuaternion::Set(float x, float y, float z) {
    // NOTE: The component mapping is cyclic (rotation about X -> quat Y etc.)
    const auto c1 = std::cos(x * 0.5f), s1 = std::sin(x * 0.5f);
    const auto c2 = std::cos(y * 0.5f), s2 = std::sin(y * 0.5f);
    const auto c3 = std::cos(z * 0.5f), s3 = std::sin(z * 0.5f);

    this->w = c1 * c2 * c3 + s1 * s2 * s3;
    this->x = c1 * c2 * s3 - s1 * s2 * c3;
    this->y = s1 * c2 * c3 + c1 * s2 * s3;
    this->z = c1 * s2 * c3 - s1 * c2 * s3;
}

// 0x59C600
// Quat from axis & angle
void CQuaternion::Set(RwV3d* axis, float angle) {
    const auto half = angle * 0.5f;
    const auto s    = std::sin(half);

    x = s * axis->x;
    y = s * axis->y;
    z = s * axis->z;
    w = std::cos(half);
}

// 0x59C630
// Spherical linear interpolation
void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float t) {
    // Same as `CAnimBlendNode::CalcTheta` (0x4D00E0) inlined
    auto dot = from.w * to.w + from.z * to.z + from.y * to.y + from.x * to.x;
    if (dot > 1.0f) {
        dot = 1.0f;
    }

    const auto halfTheta = (float)std::acos((double)dot);
    // NOTE: The exe leaves this slot unwritten when `halfTheta == 0`, but it's never read on that path
    // (reading it would be UB in C++, so it's zeroed here instead)
    float sinThetaInv = 0.0f;
    if (halfTheta != 0.0f) {
        sinThetaInv = 1.0f / std::sin(halfTheta);
    }

    Slerp(from, to, halfTheta, sinThetaInv, t);
}

// 0x4D37D0
// Conjugate of a quat
void CQuaternion::Conjugate() {
    x = -x;
    y = -y;
    z = -z;
}

// Squared length of a quat
float CQuaternion::GetLengthSquared() const {
    // Originally NOP.
    return sq(x) + sq(y) + sq(z) + sq(w);
}

// 0x4CF9B0
// Multiplies quat by a floating point value
void CQuaternion::Scale(float multiplier) {
    x *= multiplier;
    y *= multiplier;
    z *= multiplier;
    w *= multiplier;
}

// 0x4CF9E0
// Copies value from other quat
void CQuaternion::Copy(const CQuaternion& from) {
    x = from.x;
    y = from.y;
    z = from.z;
    w = from.w;
}

// Gets a dot product for quats
float CQuaternion::Dot(const CQuaternion& rhs) {
    return this->w * rhs.w + this->z * rhs.z + this->y * rhs.y + this->x * rhs.x;
}

// Normalises a quat
void CQuaternion::Normalise() {
    const auto sqMag = GetLengthSquared();
    if (sqMag == 0.f) {
        w = 1.0;
    } else {
        *this = *this / std::sqrt(sqMag);
    }
}
