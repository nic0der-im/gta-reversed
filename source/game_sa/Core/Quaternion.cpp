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

// Quat to euler angles
void CQuaternion::Get(float* x, float* y, float* z) {
    plugin::CallMethod<0x59C160, CQuaternion*, float*, float*, float*>(this, x, y, z);
}

// Quat to axis & angle
void CQuaternion::Get(RwV3d* axis, float* angle) {
    plugin::CallMethod<0x59C230, CQuaternion*, RwV3d*, float*>(this, axis, angle);
}

// Stores result of quat multiplication
void CQuaternion::Multiply(const CQuaternion& a, const CQuaternion& b) {
    plugin::CallMethod<0x59C270, CQuaternion*, const CQuaternion&, const CQuaternion&>(this, a, b);
}

// Spherical linear interpolation
void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float halftheta, float sintheta_inv, float t) {
    plugin::CallMethod<0x59C300, CQuaternion*, const CQuaternion&, const CQuaternion&, float, float, float>(this, from, to, halftheta, sintheta_inv, t);
}

// Quat from matrix
void CQuaternion::Set(const RwMatrix& m) {
    plugin::CallMethod<0x59C3E0, CQuaternion*, const RwMatrix&>(this, m);
}

// Quat from euler angles
void CQuaternion::Set(float x, float y, float z) {
    plugin::CallMethod<0x59C530, CQuaternion*, float, float, float>(this, x, y, z);
}

// Quat from axis & angle
void CQuaternion::Set(RwV3d* axis, float angle) {
    plugin::CallMethod<0x59C600, CQuaternion*, RwV3d*, float>(this, axis, angle);
}

// Spherical linear interpolation
void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float t) {
    plugin::CallMethod<0x59C630, CQuaternion*, const CQuaternion&, const CQuaternion&, float>(this, from, to, t);
}

// Conjugate of a quat
void CQuaternion::Conjugate() {
    plugin::CallMethod<0x4D37D0, CQuaternion*>(this);
}

// Squared length of a quat
float CQuaternion::GetLengthSquared() const {
    // Originally NOP.
    return sq(x) + sq(y) + sq(z) + sq(w);
}

// Multiplies quat by a floating point value
void CQuaternion::Scale(float multiplier) {
    plugin::CallMethod<0x4CF9B0, CQuaternion*, float>(this, multiplier);
}

// Copies value from other quat
void CQuaternion::Copy(const CQuaternion& from) {
    plugin::CallMethod<0x4CF9E0, CQuaternion*, const CQuaternion&>(this, from);
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
