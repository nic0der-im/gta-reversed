/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Fx.h"
#include "Shadows.h"

static auto& TempVertexBuffer = StaticRef<std::array<RxObjSpace3DVertex, 4>>(0xC4D958);

// Holds the prim index used by `Fx_c::AddDebris` (index of the next prim to enable)
static auto& g_FxDebrisPrimIndex = StaticRef<uint32>(0xA9ADE4);

auto& g_fx = StaticRef<Fx_c>(0xA9AE00);

// `rand()` scaled into `[0, 1)` (The game uses MSVC's `rand()`, which returns `[0, 32767]`)
static float FxRand01() {
    return (float)CGeneral::GetRandomNumber() / 32768.0f;
}

void Fx_c::InjectHooks() {
    RH_ScopedClass(Fx_c);
    RH_ScopedCategory("Fx");

    // + RH_ScopedInstall(Constructor, 0x49E620);
    // + RH_ScopedInstall(Destructor, 0x49E630);
    // + RH_ScopedInstall(InitStaticSystems, 0x49E660);
    // + RH_ScopedInstall(ExitStaticSystems, 0x49E850);
    // + RH_ScopedInstall(InitEntitySystems, 0x49EA60);
    // + RH_ScopedInstall(ExitEntitySystems, 0x4A12D0);
    // + RH_ScopedInstall(Init, 0x49EA90);
    // + RH_ScopedInstall(Exit, 0x4A1320);
    // + RH_ScopedInstall(Reset, 0x49EAE0);
    // + RH_ScopedInstall(CreateEntityFx, 0x4A11E0);
    // + RH_ScopedInstall(DestroyEntityFx, 0x4A1280);
    // + RH_ScopedInstall(Update, 0x49E640);
    // + RH_ScopedInstall(Render, 0x49E650);
    RH_ScopedInstall(CreateMatFromVec, 0x49E950);
    // + RH_ScopedInstall(SetFxQuality, 0x49EA40);
    // + RH_ScopedInstall(GetFxQuality, 0x49EA50);
    RH_ScopedInstall(AddBlood, 0x49EB00);
    RH_ScopedInstall(AddWood, 0x49EE10);
    RH_ScopedInstall(AddSparks, 0x49F040);
    RH_ScopedInstall(AddTyreBurst, 0x49F300);
    RH_ScopedInstall(AddBulletImpact, 0x49F3D0);
    RH_ScopedInstall(AddPunchImpact, 0x49F670);
    RH_ScopedInstall(AddDebris, 0x49F750);
    RH_ScopedInstall(AddGlass, 0x49F970);
    RH_ScopedInstall(AddWheelSpray, 0x49FB30);
    RH_ScopedInstall(AddWheelGrass, 0x49FF20);
    RH_ScopedInstall(AddWheelGravel, 0x4A0170);
    RH_ScopedInstall(AddWheelMud, 0x4A03C0);
    RH_ScopedInstall(AddWheelSand, 0x4A0610);
    RH_ScopedInstall(AddWheelDust, 0x4A09C0);
    RH_ScopedInstall(TriggerWaterHydrant, 0x4A0D70);
    RH_ScopedInstall(TriggerGunshot, 0x4A0DE0);
    RH_ScopedInstall(TriggerTankFire, 0x4A0FA0);
    RH_ScopedInstall(TriggerWaterSplash, 0x4A1070);
    RH_ScopedInstall(TriggerBulletSplash, 0x4A10E0);
    RH_ScopedInstall(TriggerFootSplash, 0x4A1150);

    RH_ScopedGlobalInstall(RenderAddTri_, 0x4A1410);
    RH_ScopedGlobalInstall(RenderEnd, 0x4A1600);
    RH_ScopedGlobalInstall(RenderBegin, 0x4A13B0);
    RH_ScopedGlobalInstall(RotateVecIntoVec, 0x4A1660);
    RH_ScopedGlobalInstall(RotateVecAboutVec, 0x4A1780);
}

Fx_c* Fx_c::Constructor() { this->Fx_c::Fx_c(); return this; }
Fx_c* Fx_c::Destructor() { this->Fx_c::~Fx_c(); return this; }

// 0x49E660
void Fx_c::InitStaticSystems() {
    m_Blood          = g_fxMan.CreateFxSystem("prt_blood",            CVector{}, nullptr, true);
    m_BoatSplash     = g_fxMan.CreateFxSystem("prt_boatsplash",       CVector{}, nullptr, true);
    m_Bubble         = g_fxMan.CreateFxSystem("prt_bubble",           CVector{}, nullptr, true);
    m_Cardebris      = g_fxMan.CreateFxSystem("prt_cardebris",        CVector{}, nullptr, true);
    m_CollisionSmoke = g_fxMan.CreateFxSystem("prt_collisionsmoke",   CVector{}, nullptr, true);
    m_GunShell       = g_fxMan.CreateFxSystem("prt_gunshell",         CVector{}, nullptr, true);
    m_Sand           = g_fxMan.CreateFxSystem("prt_sand",             CVector{}, nullptr, true);
    m_Sand2          = g_fxMan.CreateFxSystem("prt_sand2",            CVector{}, nullptr, true);
    m_SmokeHuge      = g_fxMan.CreateFxSystem("prt_smoke_huge",       CVector{}, nullptr, true);
    m_SmokeII3expand = g_fxMan.CreateFxSystem("prt_smokeII_3_expand", CVector{}, nullptr, true);
    m_Spark          = g_fxMan.CreateFxSystem("prt_spark",            CVector{}, nullptr, true);
    m_Spark2         = g_fxMan.CreateFxSystem("prt_spark_2",          CVector{}, nullptr, true);
    m_Splash         = g_fxMan.CreateFxSystem("prt_splash",           CVector{}, nullptr, true);
    m_Wake           = g_fxMan.CreateFxSystem("prt_wake",             CVector{}, nullptr, true);
    m_WaterSplash    = g_fxMan.CreateFxSystem("prt_watersplash",      CVector{}, nullptr, true);
    m_WheelDirt      = g_fxMan.CreateFxSystem("prt_wheeldirt",        CVector{}, nullptr, true);
    m_Glass          = g_fxMan.CreateFxSystem("prt_glass",            CVector{}, nullptr, true);
}

// 0x49E850
void Fx_c::ExitStaticSystems() {
    g_fxMan.DestroyFxSystem(m_Blood);
    g_fxMan.DestroyFxSystem(m_BoatSplash);
    g_fxMan.DestroyFxSystem(m_Bubble);
    g_fxMan.DestroyFxSystem(m_Cardebris);
    g_fxMan.DestroyFxSystem(m_CollisionSmoke);
    g_fxMan.DestroyFxSystem(m_GunShell);
    g_fxMan.DestroyFxSystem(m_Sand);
    g_fxMan.DestroyFxSystem(m_Sand2);
    g_fxMan.DestroyFxSystem(m_SmokeHuge);
    g_fxMan.DestroyFxSystem(m_SmokeII3expand);
    g_fxMan.DestroyFxSystem(m_Spark);
    g_fxMan.DestroyFxSystem(m_Spark2);
    g_fxMan.DestroyFxSystem(m_Splash);
    g_fxMan.DestroyFxSystem(m_Wake);
    g_fxMan.DestroyFxSystem(m_WaterSplash);
    g_fxMan.DestroyFxSystem(m_WheelDirt);
    g_fxMan.DestroyFxSystem(m_Glass);
}

// 0x49EA60
void Fx_c::InitEntitySystems() {
    // NOP
}

// NOTSA
static void CreateFxWithinCameraRange(const char* name, const CVector& pos, float range) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) <= range) {
        if (auto* fxSystem = g_fxMan.CreateFxSystem(name, pos, nullptr, false)) {
            fxSystem->PlayAndKill();
        }
    }
}

// 0x4A12D0
void Fx_c::ExitEntitySystems() {
    for (auto it = m_FxEntities.GetHead(); it; it = m_FxEntities.GetNext(it)) {
        m_FxEntities.RemoveItem(it);
        g_fxMan.DestroyFxSystem(it->m_System);
        delete it;
    }
}

// 0x49EA90
void Fx_c::Init() {
    ZoneScoped;

    g_fxMan.Init();
    g_fxMan.LoadFxProject("models\\effects.fxp");
    g_fxMan.SetWindData(&CWeather::WindDir, &CWeather::Wind);
    InitStaticSystems();
    InitEntitySystems();
    m_Randomizer = 0;
}

// 0x4A1320
void Fx_c::Exit() {
    ExitEntitySystems();
    ExitStaticSystems();
    g_fxMan.Exit();
}

// 0x49EAE0
void Fx_c::Reset() {
    g_fxMan.DestroyAllFxSystems();
    InitStaticSystems();
    InitEntitySystems(); // NOTSA
}

// 0x4A11E0
void Fx_c::CreateEntityFx(CEntity* entity, const char* fxName, const CVector& pos, RwMatrix* transform) {
    // plugin::CallMethod<0x4A11E0, Fx_c*, CEntity*, char*, RwV3d*, RwMatrix*>(this, entity, fxName, pos transform);

    auto* particle = g_fxMan.CreateFxSystem(fxName, pos, transform, true);
    if (particle) {
        auto it = new FxEntitySystem();
        it->m_System = particle;
        it->m_Entity = entity;
        m_FxEntities.AddItem(it);
        it->m_System->Play();
    }
}

// 0x4A1280
void Fx_c::DestroyEntityFx(CEntity* entity) {
    // plugin::CallMethod<0x4A1280, Fx_c*, CEntity*>(this, entity);

    for (auto it = m_FxEntities.GetHead(); it; it = m_FxEntities.GetNext(it)) {
        if (it->m_Entity == entity) {
            m_FxEntities.RemoveItem(it);
            it->m_System->Kill();
            operator delete(it);
        }
    }
}

// 0x49E640
void Fx_c::Update(RwCamera* camera, float timeDelta) {
    ZoneScoped;

    g_fxMan.Update(camera, timeDelta);
}

// 0x49E650
void Fx_c::Render(RwCamera* camera, bool heatHaze) {
    ZoneScoped;

    g_fxMan.Render(camera, heatHaze);
}

// 0x49E950
void Fx_c::CreateMatFromVec(RwMatrix* out, const CVector* origin, const CVector* direction) {
    out->right = { 1.0f, 0.0f, 0.0f };
    out->flags |= rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL;
    out->up = { 0.0f, 1.0f, 0.0f };
    out->at = { 0.0f, 0.0f, 1.0f };

    out->pos = *origin;
    out->at  = *direction;
    RwV3dNormalize(&out->at, &out->at);

    // `right = cross(at, (0, 0, 1))`
    out->right = { out->at.y, -out->at.x, 0.0f };

    // `up = cross(right, at)`
    out->up.x = out->right.y * out->at.z - out->right.z * out->at.y;
    out->up.y = out->right.z * out->at.x - out->right.x * out->at.z;
    out->up.z = out->right.x * out->at.y - out->right.y * out->at.x;

    RwMatrixUpdate(out);
}

// 0x49EA40
void Fx_c::SetFxQuality(FxQuality_e quality) {
    m_FxQuality = quality;
}

// 0x49EA50
FxQuality_e Fx_c::GetFxQuality() const {
    return m_FxQuality;
}

// 0x49EB00
void Fx_c::AddBlood(const CVector& pos, const CVector& direction, int32 amount, float lightMult) {
    if (!CLocalisation::Blood())
        return;
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f)
        return;

    FxPrtMult_c prtMult(0.5f, 0.0f, 0.0f, 1.0f, 0.8f, 0.0f, 0.8f);

    for (int32 i = 0; i < amount; ++i) {
        prtMult.m_fSize = (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 0.3f + 0.7f;

        CVector velocity = direction * 1.5f;
        velocity.x += (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 2.0f - 1.0f;
        velocity.y += (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 2.0f - 1.0f;
        velocity.z += (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 2.0f - 1.0f;

        m_Blood->AddParticle(pos, velocity, 0.0f, prtMult, -1.0f, lightMult, 0.6f, false);
    }

    CVector shadowPos = pos + direction * 0.5f;
    shadowPos.x += (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 0.2f - 0.1f;
    shadowPos.y += (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 0.2f - 0.1f;
    shadowPos.z += 1.0f;

    switch (++m_Randomizer & 7) {
    case 5:
        CShadows::AddPermanentShadow(1, gpBloodPoolTex, &shadowPos, 0.1f, 0.0f, 0.0f, -0.1f, 0xFF, 200, 0, 0, 4.0f, (CGeneral::GetRandomNumber() & 0xFFF) + 2000, 1.0f);
        break;
    case 2:
        CShadows::AddPermanentShadow(1, gpBloodPoolTex, &shadowPos, 0.2f, 0.0f, 0.0f, -0.2f, 0xFF, 200, 0, 0, 4.0f, (CGeneral::GetRandomNumber() & 0xFFF) + 8000, 1.0f);
        break;
    }
}

// 0x49EE10
void Fx_c::AddWood(const CVector& pos, const CVector& direction, int32 amount, float lightMult) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f)
        return;

    FxPrtMult_c prtMult(0.5f, 0.25f, 0.0f, 1.0f, 0.3f, 0.0f, 1.0f);

    for (int32 i = 0; i < amount; ++i) {
        prtMult.m_Color.red   = (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 0.12f + 0.13f;
        prtMult.m_Color.green = (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 0.03f + 0.12f;
        prtMult.m_Color.blue  = (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 0.03f + 0.04f;
        prtMult.m_fSize       = (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 0.3f + 0.7f;

        CVector velocity = direction * 4.0f;
        velocity.x += (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 4.0f - 2.0f;
        velocity.y += (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 4.0f - 2.0f;
        velocity.z += (float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f * 4.0f - 2.0f;

        m_Blood->AddParticle(pos, velocity, 0.0f, prtMult, -1.0f, lightMult, 0.6f, false);
    }
}

// 0x49F040
void Fx_c::AddSparks(const CVector& pos, const CVector& direction, float force, int32 amount, CVector across, eSparkType sparksType, float spread, float life) {
    const float dist2 = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (dist2 > 22500.0f)
        return;
    if (dist2 > 225.0f && (CTimer::GetFrameCounter() & 1))
        return;

    FxPrtMult_c prtMult(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, life * 0.8f);

    across *= CTimer::GetTimeStep();

    FxSystem_c* system = sparksType ? m_Spark : m_Spark2;

    for (int32 i = 0; i < amount; ++i) {
        const float ratio = 1.0f - (float)i / (float)amount;

        const CVector velocity = {
            (direction.x + (FxRand01() * (spread - (-spread)) + (-spread))) * force,
            (direction.y + (FxRand01() * (spread - (-spread)) + (-spread))) * force,
            (direction.z + (FxRand01() * (spread - (-spread)) + (-spread))) * force,
        };

        system->AddParticle(pos - across * ratio, velocity, ratio * 0.05f, prtMult, -1.0f, 1.2f, 0.6f, false);
    }
}

// 0x49F300
void Fx_c::AddTyreBurst(const CVector& posn, const CVector& velocity) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), posn) > 625.0f)
        return;

    FxPrtMult_c prtMult(1.0f, 1.0f, 1.0f, 0.4f, 0.03f, 0.0f, 0.1f);

    for (int32 i = 0; i < 4; ++i)
        m_SmokeII3expand->AddParticle(posn, velocity, (float)i * 0.05f, prtMult, -1.0f, 1.2f, 0.6f, false);
}

// 0x49F3D0
void Fx_c::AddBulletImpact(const CVector& posn, const CVector& direction, int32 bulletFxType, int32 amount, float lightMult) {
    const int32 bulletFx = g_surfaceInfos.GetBulletFx((SurfaceId)bulletFxType);
    if (!bulletFx || DistanceBetweenPointsSquared(TheCamera.GetPosition(), posn) > 22500.0f)
        return;

    switch (bulletFx) {
    case 1: { // Metal
        AddSparks(posn, direction, 3.0f, amount, CVector{}, SPARK_PARTICLE_SPARK, 0.4f, 1.0f);

        FxPrtMult_c prtMult(1.0f, 1.0f, 1.0f, 0.15f, 0.4f, 0.0f, 0.075f);

        int32 numParticles = 2;
        if (amount > 7) {
            numParticles         = 1;
            prtMult.m_Color.alpha += prtMult.m_Color.alpha;
        }

        for (int32 i = 0; i < numParticles; ++i)
            m_SmokeII3expand->AddParticle(posn, direction, (float)i * 0.05f, prtMult, -1.0f, lightMult, 0.6f, false);
        break;
    }
    case 2:
    case 4: { // Dust / stone
        FxPrtMult_c prtMult(0.81f, 0.67f, 0.57f, 0.15f, 0.4f, 0.0f, 0.3f);

        if (bulletFx == 4) {
            prtMult.m_Color.red   = 0.6f;
            prtMult.m_Color.green = 0.6f;
            prtMult.m_Color.blue  = 0.6f;
        }

        int32 numParticles = 4;
        if (amount > 7) {
            numParticles         = 2;
            prtMult.m_Color.alpha += prtMult.m_Color.alpha;
        }

        for (int32 i = 0; i < numParticles; ++i)
            m_Sand->AddParticle(posn, direction * 0.3f, (float)i * 0.05f, prtMult, -1.0f, lightMult, 0.6f, false);
        break;
    }
    case 3: // Wood
        AddWood(posn, direction, 1, 1.0f);
        break;
    }
}

// 0x49F670
void Fx_c::AddPunchImpact(const CVector& pos, const CVector& velocity, int32 num) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f)
        return;

    FxPrtMult_c prtMult(1.0f, 1.0f, 1.0f, 0.4f, 0.1f, 0.0f, 0.1f);

    m_SmokeII3expand->AddParticle(pos, velocity, 0.00f, prtMult, -1.0f, 1.2f, 0.6f, false);
    m_SmokeII3expand->AddParticle(pos, velocity, 0.05f, prtMult, -1.0f, 1.2f, 0.6f, false);
}

// 0x49F750
void Fx_c::AddDebris(const CVector& pos, const RwRGBA& color, float scale, int32 amount) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f)
        return;

    FxPrtMult_c prtMult;
    prtMult.SetColor(color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, color.alpha / 255.0f);
    prtMult.m_fSize = scale;
    prtMult.m_Rot  = ((float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f + 1.0f) * 0.5f;
    prtMult.m_fLife = 0.2f;

    for (int32 i = 0; i < amount; ++i) {
        const CVector velocity = {
            FxRand01() * 10.0f - 5.0f,
            FxRand01() * 10.0f - 5.0f,
            FxRand01() * 3.0f + 2.0f,
        };

        m_Cardebris->EnablePrim(0, false);
        m_Cardebris->EnablePrim(1, false);
        m_Cardebris->EnablePrim(2, false);
        m_Cardebris->EnablePrim(3, false);
        m_Cardebris->EnablePrim(g_FxDebrisPrimIndex, true);

        m_Cardebris->AddParticle(pos, velocity, 0.0f, prtMult, -1.0f, 1.2f, 0.6f, false);

        g_FxDebrisPrimIndex = (g_FxDebrisPrimIndex + 1) & 3;
    }
}

// 0x49F970
void Fx_c::AddGlass(const CVector& pos, const RwRGBA& color, float scale, int32 amount) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f)
        return;

    FxPrtMult_c prtMult;
    prtMult.SetColor(color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, color.alpha / 255.0f);
    prtMult.m_fSize = scale;
    prtMult.m_Rot  = ((float)(CGeneral::GetRandomNumber() % 10'000) * 0.0001f + 1.0f) * 0.5f;
    prtMult.m_fLife = 0.2f;

    for (int32 i = 0; i < amount; ++i) {
        const CVector velocity = {
            FxRand01() * 10.0f - 5.0f,
            FxRand01() * 10.0f - 5.0f,
            FxRand01() * 3.0f + 2.0f,
        };

        m_Glass->AddParticle(pos, velocity, 0.0f, prtMult, -1.0f, 1.2f, 0.6f, false);
    }
}

// 0x49FB30
void Fx_c::AddWheelSpray(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, bool bInWater, float lightMult) {
    CVehicle* playerVehicle = FindPlayerVehicle(-1, false);

    const float dist2 = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (dist2 > 625.0f)
        return;
    if (dist2 > 400.0f) {
        if ((CTimer::GetFrameCounter() + vehicle->m_nModelIndex) & 3)
            return;
    } else if (!(dist2 <= 64.0f && playerVehicle) && ((CTimer::GetFrameCounter() + vehicle->m_nModelIndex) & 1)) {
        return;
    }

    const float speed = vehicle->m_vecMoveSpeed.Magnitude();
    if (speed <= 0.01f && !bWheelsSpinning)
        return;

    FxPrtMult_c prtMult(1.0f, 1.0f, 1.0f, 0.05f, 0.0f, 1.0f, 0.0f);

    const float mul = bWheelsSpinning ? 1.0f : std::min(speed * 2.0f, 1.0f);

    prtMult.m_Color.alpha = (mul + 1.0f) * (bInWater ? 0.20f : 0.15f);
    prtMult.m_fSize       = (mul + 1.0f) * 0.20f;
    prtMult.m_fLife       = 0.08f;

    const float v0 = (mul + 1.0f) * 10.0f;
    const float v  = (30.0f - v0) + FxRand01() * (2.0f * v0);

    const int32 count = std::max(1, (int32)(CTimer::GetTimeStep() * speed * mul));

    const CVector step = vehicle->m_vecMoveSpeed * CTimer::GetTimeStep();

    for (int32 i = 0; i < count; ++i) {
        const float t = (float)i / (float)count;

        CVector p = pos - step * t;
        p.z += 0.25f;

        CVector velocity = vehicle->m_vecMoveSpeed * v;
        velocity.z += (FxRand01() + 1.0f) * mul;

        m_BoatSplash->AddParticle(p, velocity, 0.0f, prtMult, -1.0f, lightMult, 0.6f, false);
    }
}

// 0x49FF20
void Fx_c::AddWheelGrass(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    if (vehicle->m_pDriver != FindPlayerPed(0) && vehicle->m_pDriver != FindPlayerPed(1))
        return;

    CVehicle* playerVehicle = FindPlayerVehicle(-1, false);

    const float dist2 = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (dist2 > 625.0f)
        return;
    if (dist2 > 400.0f) {
        if ((CTimer::GetFrameCounter() + vehicle->m_nModelIndex) & 3)
            return;
    } else if (!(dist2 <= 64.0f && playerVehicle) && ((CTimer::GetFrameCounter() + vehicle->m_nModelIndex) & 1)) {
        return;
    }

    FxPrtMult_c prtMult(0.03f, 0.09f, 0.03f, 1.0f, 0.0f, 0.0f, 0.05f);

    for (int32 i = 0; i < 3; ++i) {
        prtMult.m_fSize = FxRand01() * 0.03f + 0.03f;

        const CVector velocity = {
            FxRand01() * (vehicle->m_vecMoveSpeed.x * -1.5f),
            FxRand01() * (vehicle->m_vecMoveSpeed.y * -1.5f),
            FxRand01() * 1.5f + 2.0f,
        };

        CVector p = pos;
        p.x += FxRand01() * 0.4f - 0.2f;
        p.y += FxRand01() * 0.4f - 0.2f;

        m_WheelDirt->AddParticle(p, velocity, 0.0f, prtMult, -1.0f, lightMult, 0.6f, false);
    }
}

// 0x4A0170
void Fx_c::AddWheelGravel(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    if (vehicle->m_pDriver != FindPlayerPed(0) && vehicle->m_pDriver != FindPlayerPed(1))
        return;

    CVehicle* playerVehicle = FindPlayerVehicle(-1, false);

    const float dist2 = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (dist2 > 625.0f)
        return;
    if (dist2 > 400.0f) {
        if ((CTimer::GetFrameCounter() + vehicle->m_nModelIndex) & 3)
            return;
    } else if (!(dist2 <= 64.0f && playerVehicle) && ((CTimer::GetFrameCounter() + vehicle->m_nModelIndex) & 1)) {
        return;
    }

    FxPrtMult_c prtMult(0.25f, 0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 0.05f);

    for (int32 i = 0; i < 3; ++i) {
        prtMult.m_fSize = FxRand01() * 0.03f + 0.03f;

        const CVector velocity = {
            FxRand01() * (vehicle->m_vecMoveSpeed.x * -1.5f),
            FxRand01() * (vehicle->m_vecMoveSpeed.y * -1.5f),
            FxRand01() * 1.5f + 2.0f,
        };

        CVector p = pos;
        p.x += FxRand01() * 0.4f - 0.2f;
        p.y += FxRand01() * 0.4f - 0.2f;

        m_WheelDirt->AddParticle(p, velocity, 0.0f, prtMult, -1.0f, lightMult, 0.6f, false);
    }
}

// 0x4A03C0
void Fx_c::AddWheelMud(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    if (vehicle->m_pDriver != FindPlayerPed(0) && vehicle->m_pDriver != FindPlayerPed(1))
        return;

    CVehicle* playerVehicle = FindPlayerVehicle(-1, false);

    const float dist2 = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (dist2 > 625.0f)
        return;
    if (dist2 > 400.0f) {
        if ((CTimer::GetFrameCounter() + vehicle->m_nModelIndex) & 3)
            return;
    } else if (!(dist2 <= 64.0f && playerVehicle) && ((CTimer::GetFrameCounter() + vehicle->m_nModelIndex) & 1)) {
        return;
    }

    FxPrtMult_c prtMult(0.25f, 0.12f, 0.06f, 1.0f, 0.0f, 0.0f, 0.05f);

    for (int32 i = 0; i < 3; ++i) {
        prtMult.m_fSize = FxRand01() * 0.03f + 0.03f;

        const CVector velocity = {
            FxRand01() * (vehicle->m_vecMoveSpeed.x * -1.5f),
            FxRand01() * (vehicle->m_vecMoveSpeed.y * -1.5f),
            FxRand01() * 1.5f + 2.0f,
        };

        CVector p = pos;
        p.x += FxRand01() * 0.4f - 0.2f;
        p.y += FxRand01() * 0.4f - 0.2f;

        m_WheelDirt->AddParticle(p, velocity, 0.0f, prtMult, -1.0f, lightMult, 0.6f, false);
    }
}

// 0x4A0610
void Fx_c::AddWheelSand(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    CVehicle* playerVehicle = FindPlayerVehicle(-1, false);

    const float dist2 = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (dist2 > 625.0f)
        return;

    const uint32 band = CTimer::GetFrameCounter() + vehicle->m_nModelIndex;
    if (m_FxQuality < FX_QUALITY_MEDIUM) {
        if (m_FxQuality == FX_QUALITY_LOW) {
            if (band & 3)
                return;
            if (!(dist2 <= 64.0f && playerVehicle) && (band & 7))
                return;
        }
    } else {
        if (band & 1)
            return;
        if (!(dist2 <= 64.0f && playerVehicle) && (band & 3))
            return;
    }

    FxPrtMult_c prtMult(0.81f, 0.67f, 0.57f, 0.5f, 1.0f, 0.0f, 0.0f);

    const float gas = std::abs(vehicle->m_GasPedal);
    const float mul = bWheelsSpinning ? 1.0f : std::min(vehicle->m_vecMoveSpeed.Magnitude() * 2.0f, 1.0f);

    prtMult.m_fLife = (mul + 1.0f) * 0.1f;

    float size = mul * 0.9f + 0.1f;
    switch (vehicle->m_nVehicleSubType) {
    case VEHICLE_TYPE_BMX:  size *= 0.25f; break;
    case VEHICLE_TYPE_BIKE:
    case VEHICLE_TYPE_QUAD: size *= 0.5f;  break;
    default:                size *= 0.7f;  break;
    }
    prtMult.m_fSize = size;

    const CVector step  = vehicle->m_vecMoveSpeed * CTimer::GetTimeStep();
    const int32   count = std::max(1, (int32)(step.Magnitude() * mul));

    for (int32 i = 0; i < count; ++i) {
        const float t = 1.0f - (float)i / (float)count;
        const CVector p = pos - step * t;

        const CVector velocity = {
            FxRand01() * (-40.0f) * vehicle->m_vecMoveSpeed.x * gas,
            FxRand01() * (-40.0f) * vehicle->m_vecMoveSpeed.y * gas,
            FxRand01() * (mul + 0.6f) + 0.2f,
        };

        m_Sand->AddParticle(p, velocity, 0.0f, prtMult, -1.0f, lightMult, 1.4f, false);
    }
}

// 0x4A09C0
void Fx_c::AddWheelDust(CVehicle* vehicle, CVector pos, bool bWheelsSpinning, float lightMult) {
    CVehicle* playerVehicle = FindPlayerVehicle(-1, false);

    const float dist2 = DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos);
    if (dist2 > 625.0f)
        return;

    const uint32 band = CTimer::GetFrameCounter() + vehicle->m_nModelIndex;
    if (m_FxQuality < FX_QUALITY_MEDIUM) {
        if (m_FxQuality == FX_QUALITY_LOW) {
            if (band & 3)
                return;
            if (!(dist2 <= 64.0f && playerVehicle) && (band & 7))
                return;
        }
    } else {
        if (band & 1)
            return;
        if (!(dist2 <= 64.0f && playerVehicle) && (band & 3))
            return;
    }

    FxPrtMult_c prtMult(0.51f, 0.44f, 0.31f, 0.5f, 1.0f, 0.0f, 0.0f);

    const float gas = std::abs(vehicle->m_GasPedal);
    const float mul = bWheelsSpinning ? 1.0f : std::min(vehicle->m_vecMoveSpeed.Magnitude() * 2.0f, 1.0f);

    prtMult.m_fLife = mul * 0.05f + 0.1f;

    float size = mul * 0.9f + 0.1f;
    switch (vehicle->m_nVehicleSubType) {
    case VEHICLE_TYPE_BMX:  size *= 0.25f; break;
    case VEHICLE_TYPE_BIKE:
    case VEHICLE_TYPE_QUAD: size *= 0.5f;  break;
    default:                size *= 0.7f;  break;
    }
    prtMult.m_fSize = size;

    const CVector step  = vehicle->m_vecMoveSpeed * CTimer::GetTimeStep();
    const int32   count = std::max(1, (int32)(step.Magnitude() * mul));

    for (int32 i = 0; i < count; ++i) {
        const float t = 1.0f - (float)i / (float)count;
        const CVector p = pos - step * t;

        const CVector velocity = {
            FxRand01() * (-40.0f) * vehicle->m_vecMoveSpeed.x * gas,
            FxRand01() * (-40.0f) * vehicle->m_vecMoveSpeed.y * gas,
            FxRand01() * (mul + 0.6f) + 0.2f,
        };

        m_Sand->AddParticle(p, velocity, 0.0f, prtMult, -1.0f, lightMult, 1.4f, false);
    }
}

// 0x4A0D70
void Fx_c::TriggerWaterHydrant(const CVector& pos) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), pos) > 625.0f)
        return;

    if (FxSystem_c* system = g_fxMan.CreateFxSystem("water_hydrant", pos, nullptr, false))
        system->PlayAndKill();
}

// 0x4A0DE0
void Fx_c::TriggerGunshot(CEntity* entity, const CVector& origin, const CVector& direction, bool doGunflash) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), origin) > 625.0f)
        return;

    RwMatrix* tempMatrix = nullptr;
    CVector   fxPos;
    RwMatrix* fxMat;

    if (!entity) {
        tempMatrix = g_fxMan.FxRwMatrixCreate();
        CreateMatFromVec(tempMatrix, &origin, &direction);
        fxPos = CVector{ 0.0f, 0.0f, 0.0f };
        fxMat = tempMatrix;
    } else {
        const CVector offset = origin - entity->GetPosition();

        if (!entity->m_matrix) {
            entity->AllocateMatrix();
            entity->m_placement.UpdateMatrix(entity->m_matrix);
        }

        // World -> entity-local space (the transpose of the rotation part)
        fxPos.x = offset.Dot(entity->m_matrix->GetRight());
        fxPos.y = offset.Dot(entity->m_matrix->GetUp());
        fxPos.z = offset.Dot(entity->m_matrix->GetForward());

        if (!entity->GetRwObject())
            return;

        fxMat = &reinterpret_cast<RwFrame*>(entity->GetRwObject()->parent)->modelling;
    }

    if (!fxMat)
        return;

    if (doGunflash) {
        if (FxSystem_c* system = g_fxMan.CreateFxSystem("gunflash", fxPos, fxMat, false)) {
            if (!entity)
                system->CopyParentMatrix();
            system->PlayAndKill();
        }
    }

    if (FxSystem_c* system = g_fxMan.CreateFxSystem("gunsmoke", fxPos, fxMat, false)) {
        if (!entity)
            system->CopyParentMatrix();
        system->PlayAndKill();
    }

    if (tempMatrix)
        g_fxMan.FxRwMatrixDestroy(tempMatrix);
}

// 0x4A0FA0
void Fx_c::TriggerTankFire(const CVector& origin, const CVector& direction) {
    if (DistanceBetweenPointsSquared(TheCamera.GetPosition(), origin) > 625.0f)
        return;

    RwMatrix* matrix = g_fxMan.FxRwMatrixCreate();
    CreateMatFromVec(matrix, &origin, &direction);

    if (FxSystem_c* system = g_fxMan.CreateFxSystem("tank_fire", CVector{ 0.0f, 0.0f, 0.0f }, matrix, false)) {
        system->CopyParentMatrix();
        system->PlayAndKill();
    }

    g_fxMan.FxRwMatrixDestroy(matrix);
}

// 0x4A1070
void Fx_c::TriggerWaterSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splash_big", pos, 625.0f);
}

// 0x4A10E0
void Fx_c::TriggerBulletSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splash", pos, 625.0f);
}

// 0x4A1150
void Fx_c::TriggerFootSplash(const CVector& pos) {
    CreateFxWithinCameraRange("water_splsh_sml", pos, 625.0f);
}

// see RwIm3DTransformFlags
// 0x4A13B0
void RenderBegin(RwRaster* newRaster, RwMatrix* transform, uint32 transformRenderFlags) {
    g_fx.m_pTransformLTM = transform;
    g_fx.m_nVerticesCount = 0;
    g_fx.m_nVerticesCount2 = 0;
    g_fx.m_pRasterToRender = newRaster;
    g_fx.m_nTransformRenderFlags = transformRenderFlags;
    g_fx.m_pVerts = TempBufferVertices.m_3d;

    // And maybe update raster on RW if the same isnt already set...
    RwRaster* currRaster{};
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &currRaster);
    if (currRaster != newRaster)
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(newRaster));
}

// Wrapper for original func
// 0x4A1410
void RenderAddTri_(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float u1, float v1,
    float u2, float v2,
    float u3, float v3,
    int32 r1, int32 g1, int32 b1, int32 a1,
    int32 r2, int32 g2, int32 b2, int32 a2,
    int32 r3, int32 g3, int32 b3, int32 a3
) {
    RenderAddTri(
        { x1, y1, z1 },
        { x2, y2, z2 },
        { x3, y3, z3 },
        { u1, v1 },
        { u2, v2 },
        { u3, v3 },
        CRGBA().FromInt32(r1, g1, b1, a1),
        CRGBA().FromInt32(r2, g2, b2, a2),
        CRGBA().FromInt32(r3, g3, b3, a3)
    );
}

// TODO: I honestly think this should be a class method...
// Although originally it wasnt.
// NOTE: Method signature changed to use CVector + RwTexCoords instead of raw values for convenience.
void RenderAddTri(CVector pos1, CVector pos2, CVector pos3, RwTexCoords coord1, RwTexCoords coord2, RwTexCoords coord3, const CRGBA& color1, const CRGBA& color2, const CRGBA& color3) {
    const auto GetVertex = [](unsigned i) {
        return &g_fx.m_pVerts[i];
    };

    const CVector pos[] = { pos1, pos2, pos3 };
    const RwRGBA color[] = {
        { color1.ToRwRGBA() },
        { color2.ToRwRGBA() },
        { color3.ToRwRGBA() },
    };
    for (unsigned i = 0; i < 3; i++) {
        RxObjSpace3DVertexSetPos(GetVertex(i), &pos[i]);
        RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color[i]);
    }

    if (g_fx.m_pRasterToRender) {
        const RwTexCoords uvs[] = { coord1, coord2, coord3 };
        for (unsigned i = 0; i < 3; i++) {
            RxObjSpace3DVertexSetU(GetVertex(i), uvs[i].u);
            RxObjSpace3DVertexSetV(GetVertex(i), uvs[i].v);
        }
    }

    g_fx.m_pVerts += 3;
    g_fx.m_nVerticesCount2 += 3;
    g_fx.m_nVerticesCount++;

    if (g_fx.m_nVerticesCount2 >= TOTAL_TEMP_BUFFER_3DVERTICES - 3 || g_fx.m_nVerticesCount >= TOTAL_TEMP_BUFFER_INDICES - 1) {
        RenderEnd(); // Render vertices to free up vertex buffer
    }
}

// 0x4A1600
void RenderEnd() {
    if (!g_fx.m_nVerticesCount)
        return;

    if (RwIm3DTransform(TempBufferVertices.m_3d, 3 * g_fx.m_nVerticesCount, g_fx.m_pTransformLTM, g_fx.m_nTransformRenderFlags)) {
        RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
        RwIm3DEnd();
    }

    g_fx.m_pVerts = TempBufferVertices.m_3d;
    g_fx.m_nVerticesCount2 = 0;
    g_fx.m_nVerticesCount = 0;
}

// 0x4A1660
void RotateVecIntoVec(RwV3d& vecRes, const RwV3d& vec, const RwV3d& vecAlign) {
    const CVector up = vecAlign;
    const auto ref = CVector{ 3.f, 4.f, 5.f }.Normalized();

    RwV3d right;
    RwV3dCrossProduct(&right, &up, &ref);
    RwV3dNormalize(&right, &right);

    RwV3d at;
    RwV3dCrossProduct(&at, &up, &right);

    auto* m = g_fxMan.FxRwMatrixCreate();
    m->right = right;
    m->up    = up;
    m->at    = at;
    m->pos   = { 0.0f, 0.0f, 0.0f };

    RwMatrixUpdate(m);
    RwV3dTransformVectors(&vecRes, &vec, 1, m);
    g_fxMan.FxRwMatrixDestroy(m);
}

// 0x4A1780
void RotateVecAboutVec(RwV3d& vecRes, const RwV3d& vec, const RwV3d& axis, float angle) {
    const float x = axis.x, y = axis.y, z = axis.z;

    const float s = CMaths::GetSinFast(angle);
    const float c = CMaths::GetCosFast(angle);
    const float t = 1.0f - c;

    vecRes.x = (t*x*x + c)   * vec.x + (t*x*y - s*z) * vec.y + (t*x*z + s*y) * vec.z;
    vecRes.y = (t*x*y + s*z) * vec.x + (t*y*y + c)   * vec.y + (t*y*z - s*x) * vec.z;
    vecRes.z = (t*x*z - s*y) * vec.x + (t*y*z + s*x) * vec.y + (t*z*z + c)   * vec.z;
}
