#include "DefenseSystem.h"
#include "Player.h"
#include "DribbleSystem.h"
#include "ShootingSystem.h"
#include "SimplePhysics.h"
#include "ScoreManager.h"
#include "GameFeedback.h"

USING_NS_CC;

DefenseSystem* DefenseSystem::create(Player* owner) {
    DefenseSystem* ret = new (std::nothrow) DefenseSystem();
    if (ret && ret->init(owner)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DefenseSystem::init(Player* owner) {
    _owner = owner;
    _isStance = false;
    _stunTimer = 0.0f;
    _stealCooldown = 0.0f;
    _blockCooldown = 0.0f;
    return true;
}

void DefenseSystem::update(float dt) {
    if (_stunTimer > 0) {
        _stunTimer -= dt;
        if (_stunTimer <= 0) {
            _stunTimer = 0;
            // Recover from stun
        }
    }
    if (_stealCooldown > 0) _stealCooldown -= dt;
    if (_blockCooldown > 0) _blockCooldown -= dt;
    
    // Auto-exit stance if moving too fast? No, stance limits speed.
}

void DefenseSystem::enterStance() {
    _isStance = true;
    // Apply speed modifier logic should be in Player::handleMovement
}

void DefenseSystem::exitStance() {
    _isStance = false;
}

void DefenseSystem::attemptSteal() {
    if (_stealCooldown > 0) return;
    
    _stealCooldown = 2.0f; // 2 seconds cooldown
    
    Player* opponent = _owner->getOpponent();
    if (!opponent || !opponent->hasBall()) return;
    
    // Cannot steal if opponent is shooting (Foul or Block territory)
    if (opponent->getState() == Player::State::SHOOTING) return;
    
    float dist = _owner->getPosition3D().distance(opponent->getPosition3D());
    
    if (dist < STEAL_RANGE) {
        if (checkStealSuccess(opponent)) {
            // User Request: Remove magnetic hands. 
            // Steal should knock the ball loose, not teleport it.
            
            Basketball* ball = opponent->getBall();
            if (ball) {
                // 1. Remove from opponent
                opponent->setPossession(false);
                if (opponent->getDribbleSystem()) {
                    opponent->getDribbleSystem()->stopDribble();
                }
                
                // 2. Knock ball loose (Physics Impulse)
                // Calculate direction from Opponent to Owner (Stealer)
                Vec3 dir = _owner->getPosition3D() - opponent->getPosition3D();
                dir.normalize();
                
                // Add some upward force and randomness
                Vec3 impulse = dir * 2.0f + Vec3(0, 3.0f, 0); // Pop up
                
                // Apply to ball
                if (ball->getBody()) {
                     ball->getBody()->setVelocity(impulse);
                }
                ball->setState(Basketball::State::FLYING); // Or Loose
                ball->setOwner(nullptr);
                
                // 3. Reset Shot Clock? Maybe not on loose ball until possession established
                // ScoreManager::getInstance()->resetShotClock();
                
                CCLOG("Steal Successful! Ball knocked loose!");
                GameFeedback::getInstance()->showShotResult("STEAL!", _owner->getPosition3D() + Vec3(0,2,0), false);
            }
        } else {
            CCLOG("Steal Failed!");
        }
    }
}

bool DefenseSystem::checkStealSuccess(Player* target) {
    // Base chance 20%
    float chance = 0.2f;
    
    // Bonus for Stance
    if (_isStance) chance += 0.3f;
    
    // Stats (Placeholder)
    float defStat = 0.5f; // _owner->getDefenseStat();
    float ballHandleStat = 0.5f; // target->getBallHandleStat();
    
    chance += (defStat - ballHandleStat);
    
    // Angle: If behind, higher chance
    Vec3 toTarget = target->getPosition3D() - _owner->getPosition3D();
    toTarget.normalize();
    
    // Target forward
    // Vec3 targetFwd = ... needs rotation
    
    return RandomHelper::random_real(0.0f, 1.0f) < chance;
}

void DefenseSystem::attemptBlock() {
    if (_blockCooldown > 0) return;
    _blockCooldown = 1.0f;
    
    // Jump animation
    // _owner->jump(); // Handled by controller?
    // DefenseSystem shouldn't control movement, just logic.
    
    Player* opponent = _owner->getOpponent();
    if (!opponent || !opponent->hasBall()) return;
    
    // Check if opponent is shooting
    if (opponent->getState() == Player::State::SHOOTING) {
        float dist = _owner->getPosition3D().distance(opponent->getPosition3D());
        if (dist < BLOCK_RANGE) {
            // Chance to block
            // Needs to be in front (Angle check)
            Vec3 toShooter = opponent->getPosition3D() - _owner->getPosition3D();
            toShooter.normalize();
            
            // Owner's forward vector
            // We assume owner is facing the shooter if they are defending properly
            // But let's check the angle between Owner->Shooter and Owner->Forward
            // Actually, simpler: Check if Owner is "in front" of Shooter.
            // Shooter is shooting towards Hoop. Owner should be between Shooter and Hoop, or close.
            // Let's use the dot product of (Shooter->Hoop) and (Shooter->Owner).
            
            Vec3 hoopPos(0, SimplePhysics::HOOP_HEIGHT, SimplePhysics::HOOP_Z);
            Vec3 shooterToHoop = hoopPos - opponent->getPosition3D();
            Vec3 shooterToOwner = _owner->getPosition3D() - opponent->getPosition3D();
            
            shooterToHoop.y = 0; shooterToOwner.y = 0; // 2D check
            if (shooterToHoop.length() > 0.1f && shooterToOwner.length() > 0.1f) {
                shooterToHoop.normalize();
                shooterToOwner.normalize();
                
                float dot = shooterToHoop.dot(shooterToOwner);
                // dot > 0.5 means Owner is roughly in the direction of the shot (Front)
                
                if (dot > 0.5f) {
                    // Base Block Chance (Hard Block / Steal)
                    // Drastically reduced to allow "Contested Shots" instead of automatic turnovers
                    float blockChance = 0.0f;
                    
                    // Distance factor
                    // Drastically reduced block chance to prevent "stealing" the ball during shots.
                    // Realism: Clean blocks are rare (1-2%). Most defense is just "Contest" (handled by ShotCalculator).
                    if (dist < 0.8f) blockChance = 0.01f; // 1% chance for stuff block
                    else blockChance = 0.0f; // No blocks from > 0.8m, only contest
                    
                    // Attribute modifier (Placeholder)
                    // blockChance += _owner->getBlockStat() * 0.01f;
                    
                    // Random Roll
                    if (cocos2d::RandomHelper::random_real(0.0f, 1.0f) < blockChance) {
                        // Successful Block!
                        CCLOG("Blocked by %s!", _owner->getName().c_str());
                        
                        ShootingSystem* shootSys = opponent->getShootingSystem();
                        if (shootSys && shootSys->isCharging()) {
                            // Force loose ball
                            opponent->getDribbleSystem()->loseBall(Vec3(0, 3, 0)); // Pop up higher
                            
                            // Feedback
                            GameFeedback::getInstance()->showShotResult("BLOCKED!", _owner->getPosition3D(), false);
                        }
                    }
                }
            }
        }
    }
}

void DefenseSystem::onOpponentCrossover() {
    // Check if we get fooled (Ankle Breaker)
    // Base chance 40%
    float chance = 0.4f;
    
    // If in stance, harder to fool? Or easier because committed?
    // Let's say if in stance, slightly harder to fool (better defense)
    if (_isStance) chance -= 0.1f;
    
    // Random check
    if (cocos2d::RandomHelper::random_real(0.0f, 1.0f) < chance) {
        // Ankle Broken!
        _stunTimer = 1.0f; // Stunned for 1 second
        
        // Visual effect? (e.g., fall or freeze)
        CCLOG("Ankles Broken! %s is stunned.", _owner->getName().c_str());
        
        // Force exit stance
        if (_isStance) exitStance();
    }
}
