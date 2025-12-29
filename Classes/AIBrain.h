#ifndef __AI_BRAIN_H__
#define __AI_BRAIN_H__

#include "cocos2d.h"

class AIBrain {
public:
    enum class Difficulty {
        EASY,
        NORMAL,
        HARD
    };

    enum class State {
        OFFENSE,   // Has ball
        DEFENSE,   // Opponent has ball
        TRANSITION // Loose ball
    };

    struct InputData {
        cocos2d::Vec3 selfPos;
        cocos2d::Vec3 opponentPos;
        cocos2d::Vec3 opponentVel; // Added velocity
        cocos2d::Vec3 ballPos;
        cocos2d::Vec3 hoopPos;
        bool hasBall;
        bool opponentHasBall;
        bool opponentIsShooting; 
        bool needsClear; // Added for 3-point clear rule
        float dt;
        float shotClock; // Added shot clock awareness
    };

    struct OutputData {
        cocos2d::Vec2 moveDir;
        bool sprint;
        bool jump;
        bool shoot;
        bool steal;
        bool defend;
        bool crossover; // Added for advanced dribbling
        
        OutputData() {
            moveDir = cocos2d::Vec2::ZERO;
            sprint = false;
            jump = false;
            shoot = false;
            steal = false;
            defend = false;
            crossover = false;
        }
    };

    AIBrain(Difficulty difficulty);
    ~AIBrain();

    void update(const InputData& input);
    const OutputData& getOutput() const { return _output; }
    State getState() const { return _state; }

private:
    Difficulty _difficulty;
    State _state;
    OutputData _output;
    
    float _reactionTimer;
    float _reactionInterval;
    float _stealTimer; // Cooldown for steal attempts
    bool _hasJumpedForShot; // To prevent multiple jumps per shot
    
    float _defenseReactionTimer; // Reaction delay for defense (block/contest)
    bool _isReactingToShot;      // Whether we are currently reacting to a shot

    bool _isShooting;
    float _shotTimer;
    
    bool _firstOffenseFrame; // To force immediate reaction on possession gain

    // Internal Logic
    void updateState(const InputData& input);
    void processOffense(const InputData& input);
    void processDefense(const InputData& input);
    void processTransition(const InputData& input);
    
    // Helpers
    cocos2d::Vec2 getDirectionTo(const cocos2d::Vec3& from, const cocos2d::Vec3& to);
    float getDistance(const cocos2d::Vec3& a, const cocos2d::Vec3& b);
    float getDistance2D(const cocos2d::Vec3& a, const cocos2d::Vec3& b);
};

#endif // __AI_BRAIN_H__
