/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include <extensions/utility.hpp>

/*
    https://gtamods.com/wiki/Decision_Maker
*/

enum eTaskType : int32;

enum eDecisionTypes {
    DEFAULT_DECISION_MAKER = 0,
    PLAYER_DECISION_MAKER = 1
};

enum eDecisionRelationship {
    DECISION_RELATIONSHIP_NEUTRAL = 0,
    DECISION_RELATIONSHIP_PLAYER = 1,
    DECISION_RELATIONSHIP_FRIEND = 2,
    DECISION_RELATIONSHIP_THREAT = 3
};

class CDecision {
public:
    constexpr static auto MAX_NUM_DECISIONS = 0x29; // 41
    constexpr static auto MAX_NUM_CHOICES   = 0x6;  // 6

public:
    notsa::mdarray<eTaskType, MAX_NUM_CHOICES>    m_Tasks;
    notsa::mdarray<uint8, MAX_NUM_CHOICES, 4>     m_Probs; // 4 different relationships : see eDecisionRelationship
    notsa::mdarray<bool, MAX_NUM_CHOICES, 2>      m_Bools; // 2 different types : see eDecisionTypes

public:
    static void InjectHooks();

    CDecision();
    CDecision(const CDecision&) = default;

    void SetDefault();
    void From(const CDecision& rhs);
    void Set(
        const notsa::mdarray<int32, MAX_NUM_CHOICES>&    tasks,
        const notsa::mdarray<float, MAX_NUM_CHOICES, 4>& probs,
        const notsa::mdarray<int32, MAX_NUM_CHOICES, 2>& bools,
        const notsa::mdarray<float, MAX_NUM_CHOICES, 6>& facialProbs //!< Unused by the exe
    );
    void Add(eTaskType task, const float* responseChances, const int32* flags);
    bool HasResponse() const;
    void MakeDecision(
        int32     relationship, //!< `eDecisionRelationship`
        bool      bInVehicle,
        eTaskType taskToAvoid1,
        eTaskType taskToAvoid2,
        eTaskType taskToAvoid3,
        eTaskType taskToSeek,
        int16&    taskType,
        int16&    facialTaskType
    );
};

VALIDATE_SIZE(CDecision, 0x3C);
