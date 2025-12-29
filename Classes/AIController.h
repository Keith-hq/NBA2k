#ifndef __AI_CONTROLLER_H__
#define __AI_CONTROLLER_H__

#include "PlayerController.h"
#include "AIBrain.h"

class Basketball;

class AIController : public PlayerController {
public:
    AIController(Player* opponent, Basketball* ball, AIBrain::Difficulty difficulty);
    virtual ~AIController();

    // Overrides
    virtual void update(float dt) override;
    
    virtual cocos2d::Vec2 getMoveInput() override;
    virtual bool isSprintPressed() override;
    virtual bool isJumpPressed() override;
    virtual bool isShootPressed() override;
    virtual bool isPassPressed() override;
    virtual bool isStealPressed() override;
    virtual bool isCrossoverPressed() override;
    virtual bool isDefendPressed() override;

private:
    AIBrain* _brain;
    Player* _opponent;
    Basketball* _ball;
    
    // Cache inputs
    cocos2d::Vec2 _moveInput;
    bool _sprint;
    bool _jump;
    bool _shoot;
    bool _steal;
    bool _defend;
    bool _crossover;
};

#endif // __AI_CONTROLLER_H__
