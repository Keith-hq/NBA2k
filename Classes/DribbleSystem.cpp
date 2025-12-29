#include "DribbleSystem.h"
#include "Player.h"
#include "Basketball.h"
#include "SimplePhysics.h"
#include "AudioManager.h"
#include "SoundBank.h"

USING_NS_CC;

DribbleSystem* DribbleSystem::create(Player* owner) {
    DribbleSystem* ret = new (std::nothrow) DribbleSystem();
    if (ret && ret->init(owner)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DribbleSystem::init(Player* owner) {
    _owner = owner;
    _isDribbling = false;
    _dribbleTimer = 0.0f;
    _dribbleInterval = 0.8f; // Standard dribble speed
    _isMovingDown = true;
    _handOffset = Vec3(0.5f, 1.0f, 0.5f); // Right hand side
    _crossoverCooldown = 0.0f;
    return true;
}

void DribbleSystem::startDribble() {
    if (!_owner || !_owner->hasBall()) return;
    _isDribbling = true;
    _isMovingDown = true;
    
    // Ensure ball state
    Basketball* ball = _owner->getBall();
    if (ball) {
        ball->setState(Basketball::State::DRIBBLING);
    }
}

void DribbleSystem::stopDribble() {
    _isDribbling = false;
    // Ball state might change to HELD or SHOOTING externally
}

void DribbleSystem::crossover() {
    if (_crossoverCooldown > 0) return;
    
    _handOffset.x = -_handOffset.x; // Switch hands
    _crossoverCooldown = 1.5f; // Cooldown
    
    // Play sound
    AudioManager::getInstance()->playSpatialEffect(SoundBank::SFX_DRIBBLE, _owner->getPosition3D());
    
    CCLOG("Crossover! Hand: %.1f", _handOffset.x);
}

void DribbleSystem::update(float dt) {
    if (_crossoverCooldown > 0) _crossoverCooldown -= dt;

    if (!_isDribbling || !_owner || !_owner->hasBall()) return;
    
    updateBallPhysics(dt);
}

void DribbleSystem::onMovement(const Vec3& velocity) {
    // Adjust dribble speed and offset based on movement
    float speed = Vec2(velocity.x, velocity.z).length();
    
    if (speed > 0.1f) {
        _dribbleInterval = 0.5f; // Faster dribble when running
        
        // Place ball slightly ahead
        Vec3 dir = velocity;
        dir.normalize();
        
        // If right hand dribble (simplified to always right for now)
        // Offset should be to the side + forward
        Vec3 right = Vec3(dir.z, 0, -dir.x); // Cross Y up
        // Actually, let's keep it simple: Relative to player rotation
        // But velocity is world space.
        
        // We'll calculate target hand pos in updateBallPhysics relative to player orientation
    } else {
        _dribbleInterval = 0.8f; // Slow idle dribble
    }
}

void DribbleSystem::updateBallPhysics(float dt) {
    Basketball* ball = _owner->getBall();
    if (!ball) return;
    
    // Calculate Hand Position (World Space)
    // Use _handOffset
    Mat4 transform = _owner->getNodeToParentTransform();
    Vec3 currentHandLocal = _handOffset;
    
    // If moving, push forward
    Vec3 vel = _owner->getBody() ? _owner->getBody()->getVelocity() : Vec3::ZERO;
    if (vel.lengthSquared() > 0.1f) {
        currentHandLocal.z += 0.5f; 
    }
    
    Vec3 handPos;
    transform.transformPoint(currentHandLocal, &handPos);
    
    // Vertical Motion (Bounce)
    float prevT = _dribbleTimer / _dribbleInterval;
    float t = _dribbleTimer / _dribbleInterval;
    _dribbleTimer += dt;
    
    // Check for bounce (passing 0.5)
    float nextT = _dribbleTimer / _dribbleInterval;
    if (prevT < 0.5f && nextT >= 0.5f) {
         AudioManager::getInstance()->playSpatialEffect(SoundBank::SFX_DRIBBLE, ball->getPosition3D());
    }

    if (_dribbleTimer > _dribbleInterval) _dribbleTimer -= _dribbleInterval;
    
    // Triangle wave or Sine wave for bounce?
    // t 0->0.5: Down. 0.5->1.0: Up.
    
    float floorY = SimplePhysics::BALL_RADIUS;
    float handY = handPos.y - SimplePhysics::BALL_RADIUS;
    
    float currentY;
    
    // Gravity based bounce approximation
    // Normalized time in cycle
    float cycleT = _dribbleTimer / _dribbleInterval;
    
    if (cycleT < 0.5f) {
        // Going Down (Accelerating)
        // 0 -> 1 (0 is hand, 1 is floor)
        float subT = cycleT * 2.0f; // 0 to 1
        float ease = subT * subT; // Quadratic ease in
        currentY = handY + (floorY - handY) * ease;
    } else {
        // Going Up (Decelerating)
        // 0 -> 1 (0 is floor, 1 is hand)
        float subT = (cycleT - 0.5f) * 2.0f; // 0 to 1
        float ease = 1.0f - (1.0f - subT) * (1.0f - subT); // Quadratic ease out
        currentY = floorY + (handY - floorY) * ease;
    }
    
    // Horizontal Position: Interpolate towards handPos x/z
    // Ball should lag slightly or lead?
    // For tight control, lock X/Z to handPos X/Z
    
    Vec3 ballPos = handPos;
    ballPos.y = currentY;
    
    ball->setPosition3D(ballPos);
    ball->setState(Basketball::State::DRIBBLING);
}

void DribbleSystem::loseBall(const Vec3& forceDirection) {
    if (!_owner || !_owner->hasBall()) return;
    
    stopDribble();
    
    Basketball* ball = _owner->getBall();
    if (ball) {
        // _owner->setBall(nullptr);
        _owner->setPossession(false);
        ball->setOwner(nullptr);
        ball->setState(Basketball::State::FLYING);
        
        // Fumble velocity
        Vec3 vel = forceDirection;
        vel.y += 3.0f; // Pop up
        
        // Add random horizontal noise to prevent perfect vertical bouncing
        float noiseX = cocos2d::RandomHelper::random_real(-1.0f, 1.0f);
        float noiseZ = cocos2d::RandomHelper::random_real(-1.0f, 1.0f);
        vel.x += noiseX;
        vel.z += noiseZ;
        
        ball->setVelocity(vel);
    }
}
