#include "RigidBody.h"
#include "SimplePhysics.h"

RigidBody::RigidBody(ColliderType type, int categoryMask, int collisionMask)
    : _type(type)
    , _categoryMask(categoryMask)
    , _collisionMask(collisionMask)
    , _position(cocos2d::Vec3::ZERO)
    , _previousPosition(cocos2d::Vec3::ZERO)
    , _velocity(cocos2d::Vec3::ZERO)
    , _force(cocos2d::Vec3::ZERO)
    , _mass(1.0f)
    , _invMass(1.0f)
    , _isKinematic(false)
    , _isStatic(false)
    , _radius(0.0f)
    , _height(0.0f)
    , _normal(cocos2d::Vec3::UNIT_Y)
    , _planeConstant(0.0f)
    , _userData(nullptr)
{
}

RigidBody::~RigidBody() {
}

void RigidBody::setSphere(float radius) {
    _radius = radius;
}

void RigidBody::setCapsule(float radius, float height) {
    _radius = radius;
    _height = height;
}

void RigidBody::setPlane(const cocos2d::Vec3& normal, float constant) {
    _normal = normal;
    _normal.normalize();
    _planeConstant = constant;
    _isStatic = true; // Planes are usually static
}

void RigidBody::setPosition(const cocos2d::Vec3& pos) {
    _position = pos;
    // Do NOT update previousPosition here, it's used for interpolation and updated in update()
}

cocos2d::Vec3 RigidBody::getInterpolatedPosition(float alpha) const {
    // alpha is 0..1, where 1 is current, 0 is previous
    return _previousPosition * (1.0f - alpha) + _position * alpha;
}

void RigidBody::setVelocity(const cocos2d::Vec3& vel) {
    _velocity = vel;
}

void RigidBody::applyForce(const cocos2d::Vec3& force) {
    _force += force;
}

void RigidBody::clearForces() {
    _force = cocos2d::Vec3::ZERO;
}

void RigidBody::setMass(float mass) {
    _mass = mass;
    if (_mass > 0.0001f) {
        _invMass = 1.0f / _mass;
    } else {
        _invMass = 0.0f;
    }
}

cocos2d::AABB RigidBody::getAABB() const {
    if (_type == ColliderType::SPHERE) {
        return cocos2d::AABB(
            _position - cocos2d::Vec3(_radius, _radius, _radius),
            _position + cocos2d::Vec3(_radius, _radius, _radius)
        );
    } else if (_type == ColliderType::CAPSULE) {
        float halfHeight = _height * 0.5f;
        return cocos2d::AABB(
            _position - cocos2d::Vec3(_radius, halfHeight + _radius, _radius),
            _position + cocos2d::Vec3(_radius, halfHeight + _radius, _radius)
        );
    } else if (_type == ColliderType::PLANE) {
        // Return a very large AABB for the plane
        return cocos2d::AABB(
            cocos2d::Vec3(-1000, -100, -1000),
            cocos2d::Vec3(1000, 100, 1000)
        );
    }
    return cocos2d::AABB();
}

void RigidBody::update(float dt) {
    if (_isStatic) return;

    _previousPosition = _position;

    if (!_isKinematic) {
        // Symplectic Euler
        // v += (f/m) * dt
        // x += v * dt
        
        cocos2d::Vec3 acceleration = _force * _invMass;
        _velocity += acceleration * dt;
    }
    
    _position += _velocity * dt;

    // Simple Floor Collision (Hardcoded for basketball court)
    // Assume Floor at Y = 0
    float floorY = SimplePhysics::FLOOR_Y;
    float bottomY = _position.y;
    
    // Calculate bottom based on collider type
    if (_type == ColliderType::SPHERE || _type == ColliderType::CAPSULE) {
        // For Capsule, origin is usually center, so bottom is pos.y - (halfHeight + radius)
        // But the game seems to use simpler physics where pos.y is the pivot? 
        // Let's assume position is the center of mass.
        
        // Wait, looking at SimplePhysics::checkCollision, it uses sphere-sphere.
        // Let's look at Player initialization or visual node. 
        // Usually 0 is feet.
        // If 0 is feet, then pos.y < 0 is penetration.
        // If 0 is center, then pos.y < radius is penetration.
        
        // Given SimplePhysics::PLAYER_JUMP_SPEED logic (y < 1.0f is ground),
        // it seems position is at feet level (0.0 when on ground).
        // Let's assume position.y IS the bottom for now, or very close to it.
        // Actually, let's use the radius to be safe if it's a ball.
        
        if (_type == ColliderType::SPHERE && _categoryMask == SimplePhysics::MASK_BALL) {
             // Ball center is its position
             if (_position.y < floorY + _radius) {
                 _position.y = floorY + _radius;
                 // Bounce
                 if (_velocity.y < 0) {
                     _velocity.y *= -0.7f; // Restitution
                     // Friction
                     _velocity.x *= 0.95f;
                     _velocity.z *= 0.95f;
                 }
             }
        } else if (_categoryMask == SimplePhysics::MASK_PLAYER) {
            // Player origin is likely at feet (0,0,0)
            if (_position.y < floorY) {
                _position.y = floorY;
                if (_velocity.y < 0) _velocity.y = 0;
            }
        }
    }
    
    clearForces();
}
