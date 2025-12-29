#ifndef __BASKETBALL_H__
#define __BASKETBALL_H__

#include "cocos2d.h"
#include "RigidBody.h"

class Player;

class Basketball : public cocos2d::Node {
public:
    static Basketball* create();
    virtual bool init() override;
    
    // Core Logic
    void update(float dt) override;
    
    // Physics
    RigidBody* getBody() const { return _body; }
    void setPosition3D(const cocos2d::Vec3& pos) override;
    cocos2d::Vec3 getPosition3D() const override;
    void setVelocity(const cocos2d::Vec3& vel);
    cocos2d::Vec3 getVelocity() const;
    
    // Actions
    void throwAt(const cocos2d::Vec3& target, float forceFactor = 1.0f);
    void dribble(const cocos2d::Vec3& handPos);
    void hold(const cocos2d::Vec3& holdPos);
    
    // State
    enum class State {
        NONE,
        HELD,
        DRIBBLING,
        FLYING,
        ON_GROUND,
        SHOOTING
    };
    
    State getState() const { return _state; }
    void setState(State state);
    
    void setOwner(Player* player) { _owner = player; }
    Player* getOwner() const { return _owner; }

    // Visuals
    void updateRotation(float dt);

private:
    RigidBody* _body;
    cocos2d::Sprite3D* _visual;
    
    State _state;
    Player* _owner;
    
    float _dribbleTimer;
    bool _dribbleDown; // True if ball is moving down during dribble
    
    // Constants
    const float RADIUS = 0.25f; // ~25cm
};

#endif // __BASKETBALL_H__
