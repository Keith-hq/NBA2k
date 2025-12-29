#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"
#include "RigidBody.h"
#include "PlayerController.h"
#include "Basketball.h"

class ShootingSystem;
class DribbleSystem;
class DefenseSystem;

class Player : public cocos2d::Node {
public:
    static Player* create();
    virtual bool init() override;
    virtual ~Player();
    
    // Core Logic
    void update(float dt) override;
    void reset();
    
    // Jump
    void jump();
    bool canJump() const;

    // Controller
    void setController(PlayerController* controller);
    PlayerController* getController() const { return _controller; }
    
    // Ball Interaction
    void setBall(Basketball* ball) { _ball = ball; }
    void setPossession(bool hasBall);
    Basketball* getBall() const { return _ball; }
    bool hasBall() const { return _hasBall; }
    
    // Opponent
    void setOpponent(Player* opp) { _opponent = opp; }
    Player* getOpponent() const { return _opponent; }
    
    // Game Rules Flags
    void setMustClearBall(bool val) { _mustClearBall = val; }
    bool mustClearBall() const { return _mustClearBall; }
    
    // Systems
    ShootingSystem* getShootingSystem() const { return _shootingSystem; }
    DribbleSystem* getDribbleSystem() const { return _dribbleSystem; }
    DefenseSystem* getDefenseSystem() const { return _defenseSystem; }
    cocos2d::DrawNode* getTrajectoryNode() const { return _trajectoryNode; }
    cocos2d::DrawNode* getStaminaNode() const { return _staminaNode; }
    float getStamina() const { return _stamina; }
    
    // Physics
    RigidBody* getBody() const { return _body; }
    void setPosition3D(const cocos2d::Vec3& pos) override;
    cocos2d::Vec3 getPosition3D() const override;
    
    // State
    enum class State {
        IDLE,
        DRIBBLING,
        SHOOTING,
        RECOVERY,
        DEFENSING,
        CELEBRATING
    };
    State getState() const { return _state; }
    
    void celebrate();
    
    // Attributes
    void setStats(float speed, float shooting, float defense);
    float getShootingStat() const { return _shootingStat; }
    void setBodyColor(const cocos2d::Color3B& color);

    // Callbacks
    std::function<void(Player*)> onShoot;

    // Limbs Access
    cocos2d::Sprite3D* getModel() const { return _model; }

private:
    RigidBody* _body;
    PlayerController* _controller;
    Basketball* _ball;
    
    // Systems
    class AnimationPlayer* _animPlayer;
    ShootingSystem* _shootingSystem;
    DribbleSystem* _dribbleSystem;
    DefenseSystem* _defenseSystem;
    Player* _opponent;
    cocos2d::DrawNode* _trajectoryNode;
    
    // Visuals
    cocos2d::Node* _visualNode;
    cocos2d::Sprite3D* _model;
    cocos2d::DrawNode* _staminaNode;
    
    // State
    State _state;
    bool _hasBall;
    
    // Stats
    float _speedStat;
    float _shootingStat;
    float _defenseStat;
    
    // Gameplay
    float _shootChargeTime;
    bool _isChargingShot;
    float _recoveryTimer;
    float _celebrationTimer;
    float _pickupCooldown;
    bool _mustClearBall;
    float _stamina;
    
    // Helpers
    void handleMovement(float dt);
    void handleActions(float dt);
    void updateVisuals();
    void updateStaminaBar();
    void attemptShoot();
    void updateBallPosition();
    float calculateHitChance(float distance);
    
    // Constants
    const float HEIGHT = 1.8f; // 1.8m
    const float RADIUS = 0.3f;
};

#endif // __PLAYER_H__
