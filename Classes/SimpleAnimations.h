#ifndef __SIMPLE_ANIMATIONS_H__
#define __SIMPLE_ANIMATIONS_H__

#include "cocos2d.h"
#include "Player.h"

class SimpleAnimations {
public:
    static void playIdle(Player* p) {
        // Breathing: Scale Chest slightly, move arms
        auto scaleUp = cocos2d::ScaleTo::create(1.0f, 1.0f, 1.02f, 1.0f);
        auto scaleDown = cocos2d::ScaleTo::create(1.0f, 1.0f, 1.0f, 1.0f);
        auto breath = cocos2d::Sequence::create(scaleUp, scaleDown, nullptr);
        p->getBodyMesh()->runAction(cocos2d::RepeatForever::create(breath));
        
        // Arms idle sway
        auto armLeft = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(1.0f, cocos2d::Vec3(5, 0, 0)),
            cocos2d::RotateTo::create(1.0f, cocos2d::Vec3(-5, 0, 0)),
            nullptr
        );
        p->getLeftArm()->runAction(cocos2d::RepeatForever::create(armLeft));
        
        auto armRight = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(1.0f, cocos2d::Vec3(5, 0, 0)),
            cocos2d::RotateTo::create(1.0f, cocos2d::Vec3(-5, 0, 0)),
            nullptr
        );
        p->getRightArm()->runAction(cocos2d::RepeatForever::create(armRight));
    }

    static void playRun(Player* p) {
        float speed = 0.3f;
        
        // Arms swing opposite
        auto armLeft = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(45, 0, 0)),
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(-45, 0, 0)),
            nullptr
        );
        p->getLeftArm()->runAction(cocos2d::RepeatForever::create(armLeft));
        
        auto armRight = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(-45, 0, 0)),
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(45, 0, 0)),
            nullptr
        );
        p->getRightArm()->runAction(cocos2d::RepeatForever::create(armRight));
        
        // Legs swing
        auto legLeft = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(-30, 0, 0)),
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(30, 0, 0)),
            nullptr
        );
        p->getLeftLeg()->runAction(cocos2d::RepeatForever::create(legLeft));
        
        auto legRight = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(30, 0, 0)),
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(-30, 0, 0)),
            nullptr
        );
        p->getRightLeg()->runAction(cocos2d::RepeatForever::create(legRight));
        
        // Body bob
        auto bob = cocos2d::Sequence::create(
            cocos2d::MoveBy::create(speed/2, cocos2d::Vec3(0, 0.1f, 0)),
            cocos2d::MoveBy::create(speed/2, cocos2d::Vec3(0, -0.1f, 0)),
            nullptr
        );
        p->getBodyMesh()->runAction(cocos2d::RepeatForever::create(bob));
    }

    static void playDribble(Player* p, bool isRightHand) {
        float speed = 0.3f; // Run speed
        float dribbleSpeed = 0.15f; 
        
        cocos2d::Node* dribbleArm = isRightHand ? p->getRightArm() : p->getLeftArm();
        cocos2d::Node* swingArm = isRightHand ? p->getLeftArm() : p->getRightArm();
        
        // Swing Arm: Swing like running (balance)
        auto armSwing = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(45, 0, 0)),
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(-45, 0, 0)),
            nullptr
        );
        swingArm->runAction(cocos2d::RepeatForever::create(armSwing));
        
        // Dribble Arm: Dribble motion (Fast Up/Down)
        auto armDribble = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(dribbleSpeed, cocos2d::Vec3(30, 0, 0)), // Push Down
            cocos2d::RotateTo::create(dribbleSpeed, cocos2d::Vec3(-10, 0, 0)), // Pull Up
            nullptr
        );
        dribbleArm->runAction(cocos2d::RepeatForever::create(armDribble));
        
        // Legs: Run motion
        auto legLeft = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(-30, 0, 0)),
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(30, 0, 0)),
            nullptr
        );
        p->getLeftLeg()->runAction(cocos2d::RepeatForever::create(legLeft));
        
        auto legRight = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(30, 0, 0)),
            cocos2d::RotateTo::create(speed, cocos2d::Vec3(-30, 0, 0)),
            nullptr
        );
        p->getRightLeg()->runAction(cocos2d::RepeatForever::create(legRight));
        
        // Body bob
        auto bob = cocos2d::Sequence::create(
            cocos2d::MoveBy::create(speed/2, cocos2d::Vec3(0, 0.1f, 0)),
            cocos2d::MoveBy::create(speed/2, cocos2d::Vec3(0, -0.1f, 0)),
            nullptr
        );
        p->getBodyMesh()->runAction(cocos2d::RepeatForever::create(bob));
    }

    static void playShoot(Player* p) {
        // Arms up
        float raiseTime = 0.2f;
        float holdTime = 0.1f;
        float releaseTime = 0.1f;
        
        // Both arms raise to shoot position (above head)
        // Angle: -150 degrees (up and slightly forward)
        auto raise = cocos2d::RotateTo::create(raiseTime, cocos2d::Vec3(-150, 0, 0));
        
        // Wrist flick (optional, hard with simple shapes)
        
        // Return to idle after delay
        auto seq = cocos2d::Sequence::create(
            raise,
            cocos2d::DelayTime::create(holdTime + releaseTime),
            cocos2d::CallFunc::create([p](){
                // Return to idle if still in shoot state
                // This is handled by state machine usually, but we can just let it hang or reset
            }),
            nullptr
        );
        
        p->getLeftArm()->runAction(seq);
        p->getRightArm()->runAction(seq->clone());
        
        // Small jump/lift
        auto jump = cocos2d::Sequence::create(
            cocos2d::MoveBy::create(raiseTime, cocos2d::Vec3(0, 0.2f, 0)),
            cocos2d::MoveBy::create(releaseTime, cocos2d::Vec3(0, -0.2f, 0)),
            nullptr
        );
        p->getBodyMesh()->runAction(jump);
    }
    
    static void playJump(Player* p) {
        // Legs tucked?
        p->getLeftLeg()->runAction(cocos2d::RotateTo::create(0.1f, cocos2d::Vec3(-45, 0, 0)));
        p->getRightLeg()->runAction(cocos2d::RotateTo::create(0.1f, cocos2d::Vec3(-20, 0, 0)));
        
        // Arms up for balance
        p->getLeftArm()->runAction(cocos2d::RotateTo::create(0.2f, cocos2d::Vec3(-90, 0, 0)));
        p->getRightArm()->runAction(cocos2d::RotateTo::create(0.2f, cocos2d::Vec3(-90, 0, 0)));
    }
    
    static void playCelebrate(Player* p) {
        // Jump joyfully
        auto jump = cocos2d::Sequence::create(
            cocos2d::MoveBy::create(0.3f, cocos2d::Vec3(0, 0.5f, 0)),
            cocos2d::MoveBy::create(0.3f, cocos2d::Vec3(0, -0.5f, 0)),
            nullptr
        );
        p->getBodyMesh()->runAction(cocos2d::Repeat::create(jump, 2));
        
        // Arms wave
        auto wave = cocos2d::Sequence::create(
            cocos2d::RotateTo::create(0.2f, cocos2d::Vec3(-170, 0, 0)),
            cocos2d::RotateTo::create(0.2f, cocos2d::Vec3(-130, 0, 0)),
            nullptr
        );
        p->getLeftArm()->runAction(cocos2d::Repeat::create(wave, 3));
        p->getRightArm()->runAction(cocos2d::Repeat::create(wave->clone(), 3));
    }
};

#endif // __SIMPLE_ANIMATIONS_H__
