/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskTimer.h"

struct C2dEffect;
struct C2dEffectPedAttractor;

class CEntity;
class CPed;

class CAttractorScanner {
public:
    char       field_0;
    char       _pad[3];
    CTaskTimer field_4;
    C2dEffect* m_pEffectInUse;      //!< +0x10
    CEntity*   m_pEntityInUse;      //!< +0x14 - The entity the effect above belongs to (Null for scripted effects)
    CEntity*   m_apEntities[10];    //!< +0x18 - Indexed by `ePedAttractorType`
    C2dEffect* m_apEffects[10];     //!< +0x40 - Indexed by `ePedAttractorType`
    float      m_aDistances[10];    //!< +0x68 - Squared distance to the effect (Indexed by `ePedAttractorType`)

public:
    void Clear();

    //! Scan the world (And the scripted 2d effects) around the ped for attractors and report the closest one
    void ScanForAttractors(CPed& ped);

private:
    //! Process a single 2d effect of an entity (Passed by the sector sweep, `entity` might be null for scripted effects)
    void AddAttractor(C2dEffect& effect, CEntity* entity, CPed& ped);

    //! Store the closest attractor found by `AddAttractor` into the out params (Null when none were found)
    void GetClosestAttractor(C2dEffect** outEffect, CEntity** outEntity) const;

    //! Find the ped a scripted attractor effect belongs to (Null if none)
    static CPed* GetScriptedAttractorPed(const C2dEffectPedAttractor& effect);
};

VALIDATE_SIZE(CAttractorScanner, 0x90);
