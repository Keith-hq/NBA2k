#ifndef __GAME_RULES_H__
#define __GAME_RULES_H__

#include "cocos2d.h"
#include "Player.h"
#include "Basketball.h"

class GameRules {
public:
    GameRules(Player* player, Player* aiPlayer, Basketball* ball);
    ~GameRules();
    
    void update(float dt);
    
    // Events
    void onBallShot(Player* shooter);
    void onBallHitRim();
    void onPossessionChange(Player* newOwner);
    
    enum class Violation {
        NONE,
        OUT_OF_BOUNDS,
        TRAVELING,
        SHOT_CLOCK,
        GAME_OVER
    };
    
    Violation getLastViolation() const { return _lastViolation; }
    void clearViolation() { _lastViolation = Violation::NONE; }
    
    // Checkers
    int calculateShotPoints(const cocos2d::Vec3& shootPos);
    
    // Clear Ball Rule
    bool needsToClearBall() const { return _needsToClearBall; }
    
private:
    Player* _player;
    Player* _aiPlayer;
    Basketball* _ball;
    
    Player* _currentOffense;
    
    Violation _lastViolation;
    float _possessionTimer; // For traveling check
    cocos2d::Vec3 _prevBallPos;
    cocos2d::Vec3 _shotPos;
    bool _needsToClearBall;
    
    void checkOutOfBounds();
    void checkShotClock();
    void checkTraveling(float dt);
    void checkWinCondition();
    void checkGoal();
    void checkClearBall();
    
    // Constants
    const float THREE_POINT_DIST = 7.24f;
    const float TRAVELING_LIMIT = 3.0f; // Simplified rule: holding ball moving > 3s
};

#endif // __GAME_RULES_H__