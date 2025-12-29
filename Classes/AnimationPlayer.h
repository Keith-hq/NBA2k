#ifndef __ANIMATION_PLAYER_H__
#define __ANIMATION_PLAYER_H__

#include "cocos2d.h"
#include <string>

class Player;

class AnimationPlayer : public cocos2d::Ref {
public:
    enum class AnimState {
        NONE,
        IDLE,
        RUN,
        DRIBBLE,
        SHOOT,
        JUMP,
        DEFEND,
        CELEBRATE
    };

    static AnimationPlayer* create(Player* owner);
    bool init(Player* owner);

    void update(float dt);
    void playState(AnimState state);
    
    // IK / Procedural
    void setRunSpeed(float speed);
    void setLookAt(const cocos2d::Vec3& target);

private:
    Player* _owner;
    AnimState _currentState;
    float _stateTime;
    
    // Internal helpers
    void playIdle();
    void playRun();
    void playDribble();
    void playShoot();
    void playJump();
    void playDefend();
    void playCelebrate();
    
    void stopAllParts();
};

#endif // __ANIMATION_PLAYER_H__
