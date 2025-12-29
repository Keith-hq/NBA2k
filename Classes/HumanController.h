#ifndef __HUMAN_CONTROLLER_H__
#define __HUMAN_CONTROLLER_H__

#include "PlayerController.h"
#include "InputSystem.h"

class HumanController : public PlayerController {
public:
    HumanController();
    virtual ~HumanController();

    // Input polling implementation
    virtual cocos2d::Vec2 getMoveInput() override;
    virtual bool isSprintPressed() override;
    virtual bool isJumpPressed() override;
    virtual bool isShootPressed() override;
    virtual bool isPassPressed() override;
    virtual bool isStealPressed() override;
    virtual bool isCrossoverPressed() override;
    virtual bool isDefendPressed() override;

    // Camera control
    void updateCamera(cocos2d::Camera* camera, float dt);

private:
    InputSystem* _input;
    
    // Smooth input
    cocos2d::Vec2 _currentInput;
    
    // Camera state
    float _cameraAngleY;
    float _cameraAngleX;
    float _cameraDistance;
};

#endif // __HUMAN_CONTROLLER_H__
