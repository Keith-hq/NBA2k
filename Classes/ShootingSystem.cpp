#include "ShootingSystem.h"
#include "Player.h"
#include "SimplePhysics.h"
#include "Basketball.h"
#include "AudioManager.h"
#include "SoundBank.h"
#include "GameFeedback.h"
#include <algorithm>
#include "base/CCDirector.h"
#include "2d/CCCamera.h"
#include "2d/CCScene.h"

USING_NS_CC;

ShootingSystem* ShootingSystem::create(Player* owner) {
    ShootingSystem* ret = new (std::nothrow) ShootingSystem();
    if (ret && ret->init(owner)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ShootingSystem::init(Player* owner) {
    _owner = owner;
    _isCharging = false;
    _currentChargeTime = 0.0f;
    _optimalChargeTime = 0.7f;
    _lastFeedback = "";
    _feedbackTimer = 0.0f;
    return true;
}

void ShootingSystem::startShot() {
    if (!_owner || !_owner->hasBall()) return;
    
    _isCharging = true;
    _currentChargeTime = 0.0f;
    _lastFeedback = "";
    _feedbackTimer = 0.0f;
    
    // Determine optimal time
    // User Request: Fixed 1.0 second optimal release time.
    _optimalChargeTime = 1.0f; 
}

void ShootingSystem::update(float dt) {
    if (_isCharging) {
        _currentChargeTime += dt;
        
        // Cap at 2.0s (User Request: 0-2s)
        if (_currentChargeTime > 2.0f) {
            _currentChargeTime = 2.0f;
            // Auto release? Or just hold at max?
            // Usually games auto-release or miss. Let's hold for now, releaseShot handles it.
        }
    }
    
    if (_feedbackTimer > 0) {
        _feedbackTimer -= dt;
        if (_feedbackTimer <= 0) {
            _lastFeedback = "";
        }
    }
}

float ShootingSystem::getChargePercent() const {
    if (_optimalChargeTime <= 0) return 0;
    return _currentChargeTime / _optimalChargeTime;
}

void ShootingSystem::releaseShot() {
    if (!_isCharging) return;
    _isCharging = false;
    
    if (!_owner || !_owner->hasBall()) return;
    
    // Collect Params
    ShotParams params;
    params.shooterPos = _owner->getPosition3D();
    
    Vec3 hoopPos(0, SimplePhysics::HOOP_HEIGHT, SimplePhysics::HOOP_Z);
    params.distance = params.shooterPos.distance(hoopPos);
    
    // Defender
    params.defenderDist = 10.0f; // Default Open
    params.defenderAngle = 180.0f; // Default Back (Safe)
    params.isDefenderBlocking = false;

    Player* opponent = _owner->getOpponent();
    if (opponent) {
        Vec3 oppPos = opponent->getPosition3D();
        params.defenderDist = params.shooterPos.distance(oppPos);
        
        // Calculate Angle: Angle between (Shooter->Hoop) and (Shooter->Defender)
        Vec3 toHoop = hoopPos - params.shooterPos;
        Vec3 toDefender = oppPos - params.shooterPos;
        
        // Ignore Y for angle (2D plane)
        Vec2 toHoop2D(toHoop.x, toHoop.z);
        Vec2 toDefender2D(toDefender.x, toDefender.z);
        
        if (toHoop2D.length() > 0.1f && toDefender2D.length() > 0.1f) {
            toHoop2D.normalize();
            toDefender2D.normalize();
            
            // Dot product for angle
            float dot = toHoop2D.dot(toDefender2D);
            // Clamp to -1..1
            if (dot > 1.0f) dot = 1.0f;
            if (dot < -1.0f) dot = -1.0f;
            
            float angleRad = acos(dot);
            params.defenderAngle = CC_RADIANS_TO_DEGREES(angleRad);
        }
        
        // Check Blocking: Defender is jumping and close?
        // Assume jumping if Y > 0.5f (SimplePhysics::FLOOR_Y is 0)
        bool isJumping = oppPos.y > 0.5f;
        params.isDefenderBlocking = isJumping;
    }
    
    params.skill = _owner->getShootingStat();
    params.timingDev = std::abs(_currentChargeTime - _optimalChargeTime);
    
    // Calculate Result
    ShotResult result = ShotCalculator::calculateShot(params);

    // --- 100% Accuracy Logic for Open Shots ---
    // User Request: 100% hit rate when unmanned and within range
    float openRangeThreshold = 9.0f; // 9 meters
    float openDefenseThreshold = 1.5f; // 1.5 meters clearance (Matches AI safe range)
    
    // Check if truly open (distance > threshold AND not actively blocking)
    if (params.distance < openRangeThreshold && 
        params.defenderDist > openDefenseThreshold && 
        !params.isDefenderBlocking) {
        
        // Force Perfect Shot
        result.success = true;
        result.finalChance = 100.0f;
        result.targetPos = hoopPos; // Dead center
        result.feedback = "PERFECT OPEN";
        
        // Update type for feedback
        if (params.distance > 7.24f) {
             result.type = ShotType::THREE_POINTER;
             result.feedback = "OPEN 3PT";
        } else {
             result.type = ShotType::MID_RANGE;
             result.feedback = "OPEN MID";
        }
        
        CCLOG("ShootingSystem: Auto-Success Triggered (Dist: %.2f, DefDist: %.2f)", params.distance, params.defenderDist);
    }

    // LAYUP LOGIC (User Request: W + Shoot + Close + Open = 100%)
    // Check if player is moving towards hoop in a small area without defender
    if (_owner->getController()) {
        Vec2 input = _owner->getController()->getMoveInput();
        
        // Check input magnitude (Moving)
        if (input.length() > 0.1f) {
            // Check Direction (Towards Hoop)
            // Hoop is at (0, HOOP_HEIGHT, HOOP_Z).
            // Vector from Player to Hoop (2D)
            Vec2 toHoop(hoopPos.x - params.shooterPos.x, hoopPos.z - params.shooterPos.z);
            toHoop.normalize();
            
            float alignment = input.getNormalized().dot(toHoop);
            
            // Conditions:
            // 1. Moving towards hoop (Alignment > 0.5)
            // 2. Very small area (Distance < 3.0m)
            // 3. No defender close (Defender Dist > 2.0m)
            if (params.distance < 3.0f && alignment > 0.5f && params.defenderDist > 2.0f) {
                // Force Success
                result.success = true;
                result.finalChance = 1.0f;
                result.feedback = "EASY LAYUP";
                result.type = ShotType::LAYUP;
                result.targetPos = hoopPos; // Perfect aim (Swish)
            }
        }
    }

    _lastFeedback = result.feedback;
    _feedbackTimer = 2.0f; // Show for 2 seconds
    
    // Execute Shot
    Basketball* ball = _owner->getBall();
    if (ball) {
        // _owner->setBall(nullptr); // Do NOT clear ball reference, just possession
        _owner->setPossession(false);
        ball->setOwner(nullptr);
        ball->setState(Basketball::State::FLYING);
        ball->setPosition3D(params.shooterPos + Vec3(0, 2.0f, 0)); // Start from head approx
        
        Vec3 velocity = calculateVelocity(ball->getPosition3D(), result.targetPos, 0);
        ball->setVelocity(velocity);
        
        // Notify
        if (_owner->onShoot) _owner->onShoot(_owner);
        
        AudioManager::getInstance()->playSpatialEffect(SoundBank::SFX_SHOOT, _owner->getPosition3D());

        CCLOG("Shot Released! Feedback: %s, Chance: %.2f", result.feedback.c_str(), result.finalChance);
        
        // Show Visual Feedback
        bool isGood = (result.finalChance > 50.0f); // Simplification
        GameFeedback::getInstance()->showShotResult(result.feedback, _owner->getPosition3D(), isGood);
    }
}

void ShootingSystem::drawTrajectory(cocos2d::DrawNode* debugNode) {
    if (!_isCharging || !debugNode) return;
    
    debugNode->clear();
    
    // Find Camera
    auto scene = Director::getInstance()->getRunningScene();
    if (!scene) return;
    
    Camera* cam = scene->getDefaultCamera();
    if (!cam) return;
    
    Vec3 startPos = _owner->getPosition3D() + Vec3(0, 2.0f, 0);
    Vec3 hoopPos(0, SimplePhysics::HOOP_HEIGHT, SimplePhysics::HOOP_Z);
    
    // Check for Bank Shot
    Vec3 target = hoopPos;
    if (ShotCalculator::shouldBankShot(startPos, hoopPos)) {
        target = ShotCalculator::getBankShotTarget(hoopPos, startPos);
    }
    
    Vec3 velocity = calculateVelocity(startPos, target, 0);
    
    // Draw Arc
    // User Request: Remove trajectory curve
    /*
    Vec3 current = startPos;
    Vec3 vel = velocity;
    float dt = 0.05f;
    float g = SimplePhysics::GRAVITY;
    
    // Color based on timing (Green = Good, Red = Bad)
    float dev = std::abs(_currentChargeTime - _optimalChargeTime);
    Color4F color = Color4F::GREEN;
    if (dev > 0.2f) color = Color4F::RED;
    else if (dev > 0.1f) color = Color4F::YELLOW;
    
    for (int i = 0; i < 30; i++) {
        Vec3 next = current + vel * dt;
        vel.y += g * dt;
        
        Vec2 p1 = cam->project(current);
        Vec2 p2 = cam->project(next);
        
        debugNode->drawLine(p1, p2, color);
        current = next;
        
        if (current.y < 0) break;
    }
    */
    
    // Draw Shot Meter (Screen Space)
    Vec3 headPos = _owner->getPosition3D() + Vec3(0, 2.8f, 0);
    Vec2 screenHead = cam->project(headPos);
    
    float width = 60.0f;
    Vec2 start = screenHead - Vec2(width/2, 0);
    Vec2 end = screenHead + Vec2(width/2, 0);
    
    // Background
    debugNode->drawSegment(start, end, 3.0f, Color4F::GRAY);
    
    // Progress
    float pct = getChargePercent(); // Returns current / 1.0
    // Show range 0.0 to 2.0 (1.0 is optimal)
    // pct represents seconds directly (since optimal is 1.0)
    // But we want to clamp visual bar to 2.0
    
    if (pct > 2.0f) pct = 2.0f;
    
    // Scale: Bar width represents 2.0 seconds
    float currentX = width * (pct / 2.0f);
    Vec2 currentPos = start + Vec2(currentX, 0);
    
    Color4F barColor = Color4F::YELLOW;
    // Green window: +/- 0.05s (very tight) or 0.1s?
    // User: 0.1s dev -> -10%. So < 0.1s dev is > 90% chance.
    if (std::abs(pct - 1.0f) < 0.1f) barColor = Color4F::GREEN;
    
    debugNode->drawSegment(start, currentPos, 3.0f, barColor);
    
    // Optimal Mark
    // At 1.0s (Center of 2.0s bar)
    float optX = width * 0.5f;
    Vec2 optPos = start + Vec2(optX, 0);
    debugNode->drawDot(optPos, 4.0f, Color4F::WHITE);
}

Vec3 ShootingSystem::calculateVelocity(const Vec3& startPos, const Vec3& targetPos, float flightTime) {
    // Arc Logic
    // Increased arc height for more realistic high-arcing shots
    float peakHeight = std::max(startPos.y, targetPos.y) + 3.5f; 
    float dist = startPos.distance(targetPos);
    if (dist < 3.0f) peakHeight = std::max(startPos.y, targetPos.y) + 1.0f; // Close shots slightly lower
    
    float g = -SimplePhysics::GRAVITY; // Positive magnitude 10
    float dy_up = peakHeight - startPos.y;
    float dy_down = peakHeight - targetPos.y;
    
    if (dy_up < 0) dy_up = 0.1f;
    
    float vy = sqrt(2 * g * dy_up);
    float t_up = vy / g;
    float t_down = 0.0f;
    if (dy_down > 0) t_down = sqrt(2 * dy_down / g); // peak is higher than target
    // If target is higher than peak (impossible with this logic), t_down is imaginary.
    // Our logic ensures peak >= target.
    
    float totalTime = t_up + t_down;
    if (totalTime <= 0.001f) totalTime = 1.0f;
    
    Vec2 distXZ(targetPos.x - startPos.x, targetPos.z - startPos.z);
    float v_h = distXZ.length() / totalTime;
    Vec2 dir = distXZ.getNormalized();
    
    return Vec3(dir.x * v_h, vy, dir.y * v_h);
}
