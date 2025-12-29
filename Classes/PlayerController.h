#ifndef __PLAYER_CONTROLLER_H__
#define __PLAYER_CONTROLLER_H__

#include "cocos2d.h"

class Player;

class PlayerController {
public:
    virtual ~PlayerController() {}

    // Input polling
    virtual cocos2d::Vec2 getMoveInput() = 0; // Normalized direction (x, z)
    virtual bool isSprintPressed() = 0;
    virtual bool isJumpPressed() = 0;    // Tap to jump
    virtual bool isShootPressed() = 0;   // Hold to charge/shoot
    virtual bool isPassPressed() = 0;
    virtual bool isStealPressed() = 0;
    virtual bool isCrossoverPressed() { return false; } // Default false
    virtual bool isDefendPressed() = 0;  // Hold to defend

    // Assignment
    void setTarget(Player* player) { _player = player; }
    Player* getTarget() const { return _player; }
    
    // Logic Update
    virtual void update(float dt) {}

protected:
    Player* _player = nullptr;
};

#endif // __PLAYER_CONTROLLER_H__
