#ifndef __DRIBBLE_SYSTEM_H__
#define __DRIBBLE_SYSTEM_H__

#include "cocos2d.h"

class Player;

class DribbleSystem : public cocos2d::Ref {
public:
    static DribbleSystem* create(Player* owner);
    bool init(Player* owner);
    
    void update(float dt);
    
    // Actions
    void startDribble();
    void stopDribble();
    void crossover();
    void loseBall(const cocos2d::Vec3& forceDirection);
    
    // Logic
    bool isDribbling() const { return _isDribbling; }
    void onMovement(const cocos2d::Vec3& velocity);
    const cocos2d::Vec3& getHandOffset() const { return _handOffset; }
    
private:
    Player* _owner;
    bool _isDribbling;
    float _crossoverCooldown;
    
    // Rhythm
    float _dribbleTimer;
    float _dribbleInterval; // Time for one down-up cycle
    bool _isMovingDown;
    
    // Hand Position Logic
    cocos2d::Vec3 _handOffset;
    cocos2d::Vec3 _targetHandPos;
    
    void updateBallPhysics(float dt);
};

#endif // __DRIBBLE_SYSTEM_H__
