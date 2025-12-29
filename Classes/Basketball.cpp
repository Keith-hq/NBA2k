#include "Basketball.h"
#include "GameCore.h"
#include "CollisionSystem.h"
#include "SimplePhysics.h"
#include "AudioManager.h"
#include "SoundBank.h"
#include "EffectsManager.h"

USING_NS_CC;

Basketball* Basketball::create() {
    Basketball* pRet = new (std::nothrow) Basketball();
    if (pRet && pRet->init()) {
        pRet->autorelease();
        return pRet;
    }
    else {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool Basketball::init() {
    if (!Node::init()) return false;
    
    _state = State::NONE;
    _owner = nullptr;
    _dribbleTimer = 0.0f;
    _dribbleDown = true;
    
    // Visuals
    _visual = Sprite3D::create("basketball.c3b");
    if (_visual) {
        // Auto-scale logic
        // _visual->forceNodeToParentTransformLayout(); // Removed as it causes compilation error
        
        // Get AABB to determine raw size
        auto aabb = _visual->getAABB();
        Vec3 size = aabb._max - aabb._min;
        float maxDim = std::max(size.x, std::max(size.y, size.z));
        
        if (maxDim > 0.0f) {
            float targetSize = RADIUS * 2.0f; // 0.5 meters
            float scale = targetSize / maxDim;
            _visual->setScale(scale);
            CCLOG("Basketball model auto-scaled. Raw Size: %.2f, Scale: %.4f", maxDim, scale);
        } else {
            // Fallback if AABB is zero (empty mesh?)
            _visual->setScale(RADIUS * 2);
            CCLOG("Basketball model AABB is zero. Using default scale.");
        }
        
        // _visual->setColor(Color3B(255, 140, 0)); // Use texture colors
        addChild(_visual);
    } else {
        CCLOG("Failed to load basketball.c3b! Fallback to sphere.");
        _visual = Sprite3D::create(GameCore::getInstance()->getPrimitivePath("sphere"));
        if (_visual) {
            _visual->setScale(RADIUS * 2);
            _visual->setColor(Color3B(255, 140, 0)); // Orange
            addChild(_visual);
        }
    }
    
    // Physics
    _body = new RigidBody(ColliderType::SPHERE, SimplePhysics::MASK_BALL, SimplePhysics::MASK_FLOOR | SimplePhysics::MASK_PLAYER | SimplePhysics::MASK_HOOP);
    _body->setSphere(RADIUS);
    _body->setMass(0.6f); // 0.6kg
    _body->setMaterial(SimplePhysicsMaterial(0.8f, 0.5f)); // High bounce, medium friction
    _body->setUserData(this);
    
    CollisionSystem::getInstance()->addBody(_body);

    _body->onCollision = [this](RigidBody* other, float impulse) {
        if (_state == State::HELD || _state == State::DRIBBLING) return;
        
        // Threshold to avoid sliding sounds
        if (impulse < 1.0f) return;
        
        int mask = other->getCategoryMask();
        if (mask & SimplePhysics::MASK_FLOOR) {
            AudioManager::getInstance()->playSpatialEffect(SoundBank::SFX_DRIBBLE, getPosition3D());
        } else if (mask & SimplePhysics::MASK_HOOP) {
            AudioManager::getInstance()->playSpatialEffect(SoundBank::SFX_RIM_HIT, getPosition3D());
            EffectsManager::getInstance()->playRimHitEffect(getPosition3D());
        }
    };
    
    scheduleUpdate();
    
    return true;
}

void Basketball::setPosition3D(const Vec3& pos) {
    Node::setPosition3D(pos);
    if (_body) {
        _body->setPosition(pos);
    }
}

Vec3 Basketball::getPosition3D() const {
    if (_body) return _body->getPosition();
    return Node::getPosition3D();
}

void Basketball::setVelocity(const Vec3& vel) {
    if (_body) {
        _body->setVelocity(vel);
    }
}

Vec3 Basketball::getVelocity() const {
    if (_body) {
        return _body->getVelocity();
    }
    return Vec3::ZERO;
}

void Basketball::setState(State state) {
    _state = state;
    
    if (_state == State::HELD || _state == State::DRIBBLING) {
        if (_body) _body->setKinematic(true);
    } else {
        if (_body) _body->setKinematic(false);
    }
}

void Basketball::update(float dt) {
    if (_state == State::FLYING || _state == State::ON_GROUND) {
        if (_body) {
            float alpha = CollisionSystem::getInstance()->getAlpha();
            Node::setPosition3D(_body->getInterpolatedPosition(alpha));
            
            // Check if on ground
            if (_body->getPosition().y <= RADIUS + 0.05f && _body->getVelocity().lengthSquared() < 0.1f) {
                _state = State::ON_GROUND;
            }
            
            updateRotation(dt);
        }
    }
}

void Basketball::updateRotation(float dt) {
    if (_state == State::FLYING || _state == State::ON_GROUND) {
        // Rotate based on velocity (simplified)
        Vec3 vel = _body->getVelocity();
        if (vel.lengthSquared() > 0.1f) {
            Vec3 axis(vel.z, 0, -vel.x);
            axis.normalize();
            float speed = vel.length();
            float angle = speed * dt * 360.0f; // Arbitrary rotation speed
            
            // Apply rotation (cumulative)
            // Node::rotate is not 3D in same way.
            // Simplified: Just rotate visual node if we had one separate.
            // But we are rotating the whole node.
            // Let's skip complex rolling rotation for now.
        }
    }
}

void Basketball::throwAt(const Vec3& target, float forceFactor) {
    setState(State::FLYING);
    _owner = nullptr;
    
    Vec3 startPos = getPosition3D();
    
    // Physics projectile calculation
    // d = v*t
    // h = vy*t + 0.5*g*t^2
    
    // Simplified arc:
    // Aim for a peak height
    float peakHeight = 4.0f;
    if (target.y > peakHeight) peakHeight = target.y + 1.0f;
    if (startPos.y > peakHeight) peakHeight = startPos.y + 1.0f;
    
    float g = -SimplePhysics::GRAVITY; // Gravity is negative in Physics, so -g is positive magnitude
    
    float dy = target.y - startPos.y;
    float h = peakHeight - startPos.y;
    
    // vy = sqrt(2 * g * h)
    float vy = sqrt(2 * g * h);
    
    // Time to peak
    float t_up = vy / g;
    
    // Time from peak to target
    // y_peak = peakHeight. Target y = target.y
    // dy_down = peakHeight - target.y
    // dy_down = 0.5 * g * t_down^2
    float t_down = sqrt(2 * (peakHeight - target.y) / g);
    
    float totalTime = t_up + t_down;
    
    // Horizontal distance
    Vec2 distXZ(target.x - startPos.x, target.z - startPos.z);
    float d = distXZ.length();
    
    float vh = d / totalTime;
    
    Vec2 dirXZ = distXZ.getNormalized();
    
    Vec3 velocity(dirXZ.x * vh, vy, dirXZ.y * vh);
    
    // Add some randomness/error based on forceFactor?
    // forceFactor 1.0 = perfect.
    
    if (_body) {
        _body->setVelocity(velocity);
    }
}

void Basketball::dribble(const Vec3& handPos) {
    if (_state != State::DRIBBLING) return;
    
    // Procedural Dribble Animation
    // Move ball between hand and floor
    
    float floorY = RADIUS;
    float handY = handPos.y - RADIUS;
    
    float speed = 5.0f; // Dribble speed
    
    Vec3 currentPos = getPosition3D();
    Vec3 targetPos = handPos;
    targetPos.y = floorY; // Default target floor
    
    if (_dribbleDown) {
        // Moving down
        currentPos.y -= speed * 0.016f; // dt approx
        if (currentPos.y <= floorY) {
            currentPos.y = floorY;
            _dribbleDown = false;
        }
    } else {
        // Moving up
        currentPos.y += speed * 0.016f;
        if (currentPos.y >= handY) {
            currentPos.y = handY;
            _dribbleDown = true;
        }
    }
    
    // Horizontal follow hand immediately
    currentPos.x = handPos.x;
    currentPos.z = handPos.z; // + offset?
    
    setPosition3D(currentPos);
}

void Basketball::hold(const Vec3& holdPos) {
    if (_state != State::HELD && _state != State::SHOOTING) return;
    
    setPosition3D(holdPos);
    if (_body) _body->setVelocity(Vec3::ZERO);
}