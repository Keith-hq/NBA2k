#ifndef __DEFENSE_SYSTEM_H__
#define __DEFENSE_SYSTEM_H__

#include "cocos2d.h"

class Player;

class DefenseSystem : public cocos2d::Ref {
public:
    static DefenseSystem* create(Player* owner);
    bool init(Player* owner);
    
    void update(float dt);
    
    // Actions
    void enterStance();
    void exitStance();
    void attemptSteal();
    void attemptBlock();
    void onOpponentCrossover();
    
    // State
    bool isStance() const { return _isStance; }
    bool isStunned() const { return _stunTimer > 0; }
    bool canSteal() const { return _stealCooldown <= 0; }
    
private:
    Player* _owner;
    
    bool _isStance;
    float _stunTimer;
    float _stealCooldown;
    float _blockCooldown;
    
    // Constants
    const float STEAL_RANGE = 2.0f;
    const float BLOCK_RANGE = 2.5f;
    const float STANCE_SPEED_MODIFIER = 0.6f;
    
    bool checkStealSuccess(Player* target);
};

#endif // __DEFENSE_SYSTEM_H__
