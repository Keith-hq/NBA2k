#include "CollisionSystem.h"
#include "SimplePhysics.h"
#include "PerformanceMonitor.h"
#include <algorithm>
#include <cmath>

CollisionSystem* CollisionSystem::_instance = nullptr;

CollisionSystem* CollisionSystem::getInstance() {
    if (!_instance) {
        _instance = new CollisionSystem();
    }
    return _instance;
}

CollisionSystem::CollisionSystem() : _accumulator(0.0f) {}

CollisionSystem::~CollisionSystem() {}

void CollisionSystem::reset() {
    _bodies.clear();
    _cachedPairs.clear();
    _accumulator = 0.0f;
}

void CollisionSystem::addBody(RigidBody* body) {
    if (std::find(_bodies.begin(), _bodies.end(), body) == _bodies.end()) {
        _bodies.push_back(body);
    }
}

void CollisionSystem::removeBody(RigidBody* body) {
    auto it = std::find(_bodies.begin(), _bodies.end(), body);
    if (it != _bodies.end()) {
        _bodies.erase(it);
    }
}

void CollisionSystem::update(float dt) {
    _accumulator += dt;
    
    // Clamp accumulator to avoid spiral of death (e.g. if game hangs)
    if (_accumulator > 0.2f) _accumulator = 0.2f;
    
    int checks = 0;
    while (_accumulator >= SimplePhysics::FIXED_TIME_STEP) {
        fixedUpdate(SimplePhysics::FIXED_TIME_STEP);
        _accumulator -= SimplePhysics::FIXED_TIME_STEP;
        checks++;
    }
    
    // Record Metrics
    if (PerformanceMonitor::getInstance()->isDebugVisible()) {
        PerformanceMonitor::getInstance()->recordCollisionChecks(_bodies.size() * _bodies.size() * checks); // Approximate
        PerformanceMonitor::getInstance()->recordEntityCount(_bodies.size());
    }
}

float CollisionSystem::getAlpha() const {
    return _accumulator / SimplePhysics::FIXED_TIME_STEP;
}

void CollisionSystem::fixedUpdate(float dt) {
    // Sub-stepping for stability
    float subDt = dt / SimplePhysics::SUB_STEPS;
    
    for (int step = 0; step < SimplePhysics::SUB_STEPS; ++step) {
        applyForces(subDt);
        
        // Update positions
        for (auto body : _bodies) {
            body->update(subDt);
        }
        
        enforceBoundaries();
        
        std::vector<std::pair<RigidBody*, RigidBody*>> pairs;
        broadPhase(pairs);
        
        std::vector<Manifold> manifolds;
        narrowPhase(pairs, manifolds);
        
        resolveCollisions(manifolds);
    }
}

void CollisionSystem::enforceBoundaries() {
    float halfWidth = SimplePhysics::COURT_WIDTH / 2.0f;
    float halfLength = SimplePhysics::COURT_LENGTH / 2.0f;

    for (auto body : _bodies) {
        if (body->isStatic()) continue;

        cocos2d::Vec3 pos = body->getPosition();
        cocos2d::Vec3 vel = body->getVelocity();
        float radius = body->getRadius();

        bool modified = false;

        // Check X (Width)
        if (pos.x < -halfWidth + radius) {
            pos.x = -halfWidth + radius;
            if (vel.x < 0) vel.x *= -body->getMaterial().restitution;
            modified = true;
        } else if (pos.x > halfWidth - radius) {
            pos.x = halfWidth - radius;
            if (vel.x > 0) vel.x *= -body->getMaterial().restitution;
            modified = true;
        }

        // Check Z (Length)
        if (pos.z < -halfLength + radius) {
            pos.z = -halfLength + radius;
            if (vel.z < 0) vel.z *= -body->getMaterial().restitution;
            modified = true;
        } else if (pos.z > halfLength - radius) {
            pos.z = halfLength - radius;
            if (vel.z > 0) vel.z *= -body->getMaterial().restitution;
            modified = true;
        }

        if (modified) {
            body->setPosition(pos);
            body->setVelocity(vel);
        }
    }
}

void CollisionSystem::applyForces(float dt) {
    for (auto body : _bodies) {
        if (body->isStatic() || body->isKinematic()) continue;
        
        // Gravity
        cocos2d::Vec3 gravity(0, SimplePhysics::GRAVITY, 0);
        body->applyForce(gravity * body->getMass());
        
        // Air Resistance Removed for precise ballistic calculation
        // if (body->getCategoryMask() & SimplePhysics::MASK_BALL) { ... }
    }
}

void CollisionSystem::broadPhase(std::vector<std::pair<RigidBody*, RigidBody*>>& pairs) {
    // Optimization: Skip Static-Static collisions
    // And Distance Check
    
    size_t count = _bodies.size();
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            RigidBody* a = _bodies[i];
            RigidBody* b = _bodies[j];
            
            // Skip if both static
            if (a->isStatic() && b->isStatic()) continue;
            
            // Filter masks
            if ((a->getCategoryMask() & b->getCollisionMask()) == 0 ||
                (b->getCategoryMask() & a->getCollisionMask()) == 0) continue;
            
            // AABB Check
            if (a->getAABB().intersects(b->getAABB())) {
                pairs.push_back({a, b});
            }
        }
    }
}

void CollisionSystem::narrowPhase(std::vector<std::pair<RigidBody*, RigidBody*>>& pairs, std::vector<Manifold>& manifolds) {
    for (auto& pair : pairs) {
        RigidBody* a = pair.first;
        RigidBody* b = pair.second;
        
        Manifold m;
        m.a = a;
        m.b = b;
        
        bool collided = false;
        
        // Dispatch
        if (a->getType() == ColliderType::SPHERE && b->getType() == ColliderType::SPHERE) {
            collided = detectSphereSphere(a, b, m);
        } else if (a->getType() == ColliderType::SPHERE && b->getType() == ColliderType::PLANE) {
            collided = detectSpherePlane(a, b, m);
        } else if (a->getType() == ColliderType::PLANE && b->getType() == ColliderType::SPHERE) {
            collided = detectSpherePlane(b, a, m); // Swap
            m.normal = -m.normal;
            std::swap(m.a, m.b);
        } else if (a->getType() == ColliderType::SPHERE && b->getType() == ColliderType::CAPSULE) {
            collided = detectSphereCapsule(a, b, m);
        } else if (a->getType() == ColliderType::CAPSULE && b->getType() == ColliderType::SPHERE) {
            collided = detectSphereCapsule(b, a, m); // Swap
            m.normal = -m.normal;
            std::swap(m.a, m.b);
        } else if (a->getType() == ColliderType::CAPSULE && b->getType() == ColliderType::PLANE) {
            collided = detectCapsulePlane(a, b, m);
        } else if (a->getType() == ColliderType::PLANE && b->getType() == ColliderType::CAPSULE) {
            collided = detectCapsulePlane(b, a, m); // Swap
            m.normal = -m.normal;
            std::swap(m.a, m.b);
        }
        
        if (collided) {
            manifolds.push_back(m);
        }
    }
}

bool CollisionSystem::detectSphereSphere(RigidBody* s1, RigidBody* s2, Manifold& m) {
    cocos2d::Vec3 d = s2->getPosition() - s1->getPosition();
    float distSq = d.lengthSquared();
    float radiusSum = s1->getRadius() + s2->getRadius();
    
    if (distSq < radiusSum * radiusSum) {
        float dist = sqrt(distSq);
        if (dist < 0.0001f) {
            m.normal = cocos2d::Vec3::UNIT_Y;
            m.depth = radiusSum;
        } else {
            m.normal = d * (1.0f / dist);
            m.depth = radiusSum - dist;
        }
        return true;
    }
    return false;
}

bool CollisionSystem::detectSpherePlane(RigidBody* sphere, RigidBody* plane, Manifold& m) {
    // Plane: n.p + d = 0
    // Dist = n.p + d
    float dist = plane->getNormal().dot(sphere->getPosition()) + plane->getPlaneConstant();
    
    // CCD: Check if we passed through the plane
    // Currently only simple static check
    if (dist < sphere->getRadius()) {
        m.normal = plane->getNormal();
        m.depth = sphere->getRadius() - dist;
        return true;
    }
    return false;
}

bool CollisionSystem::detectSphereCapsule(RigidBody* sphere, RigidBody* capsule, Manifold& m) {
    // Segment of capsule
    cocos2d::Vec3 p = capsule->getPosition();
    cocos2d::Vec3 top = p + cocos2d::Vec3(0, capsule->getHeight()/2, 0);
    cocos2d::Vec3 bottom = p - cocos2d::Vec3(0, capsule->getHeight()/2, 0);
    
    // Closest point on segment to sphere center
    cocos2d::Vec3 sPos = sphere->getPosition();
    cocos2d::Vec3 segment = top - bottom;
    cocos2d::Vec3 pointToSphere = sPos - bottom;
    float t = pointToSphere.dot(segment) / segment.lengthSquared();
    t = std::max(0.0f, std::min(1.0f, t));
    
    cocos2d::Vec3 closest = bottom + segment * t;
    
    cocos2d::Vec3 d = sPos - closest;
    float distSq = d.lengthSquared();
    float radiusSum = sphere->getRadius() + capsule->getRadius();
    
    if (distSq < radiusSum * radiusSum) {
        float dist = sqrt(distSq);
        if (dist < 0.0001f) {
            m.normal = cocos2d::Vec3::UNIT_X; // Arbitrary
            m.depth = radiusSum;
        } else {
            m.normal = d / dist;
            m.depth = radiusSum - dist;
        }
        return true;
    }
    return false;
}

bool CollisionSystem::detectCapsulePlane(RigidBody* capsule, RigidBody* plane, Manifold& m) {
    // Check top and bottom sphere of capsule
    float halfHeight = capsule->getHeight() / 2;
    cocos2d::Vec3 pos = capsule->getPosition();
    cocos2d::Vec3 bottom = pos - cocos2d::Vec3(0, halfHeight, 0);
    cocos2d::Vec3 top = pos + cocos2d::Vec3(0, halfHeight, 0);
    
    float distBottom = plane->getNormal().dot(bottom) + plane->getPlaneConstant();
    float distTop = plane->getNormal().dot(top) + plane->getPlaneConstant();
    
    float radius = capsule->getRadius();
    
    bool hitBottom = distBottom < radius;
    // We prioritize the deepest penetration
    
    if (hitBottom) {
        m.normal = plane->getNormal();
        m.depth = radius - distBottom;
        return true;
    }
    
    // Usually capsules stay upright, so checking bottom is often enough for floor
    return false;
}

void CollisionSystem::resolveCollisions(const std::vector<Manifold>& manifolds) {
    for (const auto& m : manifolds) {
        RigidBody* a = m.a;
        RigidBody* b = m.b;
        
        // 1. Positional Correction (prevent sinking)
        const float percent = 0.8f; // Penetration percentage to correct
        const float slop = 0.01f;   // Penetration allowance
        cocos2d::Vec3 correction = m.normal * (std::max(m.depth - slop, 0.0f) / (a->getInvMass() + b->getInvMass())) * percent;
        
        if (!a->isStatic()) a->setPosition(a->getPosition() - correction * a->getInvMass());
        if (!b->isStatic()) b->setPosition(b->getPosition() + correction * b->getInvMass());
        
        // 2. Velocity Resolution
        cocos2d::Vec3 rv = b->getVelocity() - a->getVelocity();
        float velAlongNormal = rv.dot(m.normal);
        
        if (velAlongNormal > 0) continue; // Moving away
        
        float e = std::min(a->getMaterial().restitution, b->getMaterial().restitution);
        
        float j = -(1 + e) * velAlongNormal;
        j /= a->getInvMass() + b->getInvMass();
        
        cocos2d::Vec3 impulse = m.normal * j;
        
        if (!a->isStatic()) a->setVelocity(a->getVelocity() - impulse * a->getInvMass());
        if (!b->isStatic()) b->setVelocity(b->getVelocity() + impulse * b->getInvMass());
        
        // Callback
        if (a->onCollision) a->onCollision(b, std::abs(j));
        if (b->onCollision) b->onCollision(a, std::abs(j));

        // 3. Friction
        cocos2d::Vec3 tangent = rv - m.normal * rv.dot(m.normal);
        if (tangent.lengthSquared() > 0.0001f) {
            tangent.normalize();
            float jt = -rv.dot(tangent);
            jt /= a->getInvMass() + b->getInvMass();
            
            float mu = std::sqrt(a->getMaterial().friction * b->getMaterial().friction);
            
            // Coulomb's Law
            cocos2d::Vec3 frictionImpulse;
            if (std::abs(jt) < j * mu) {
                frictionImpulse = tangent * jt;
            } else {
                frictionImpulse = tangent * (-j * mu);
            }
            
            if (!a->isStatic()) a->setVelocity(a->getVelocity() - frictionImpulse * a->getInvMass());
            if (!b->isStatic()) b->setVelocity(b->getVelocity() + frictionImpulse * b->getInvMass());
        }
    }
}

bool CollisionSystem::checkTrigger(const cocos2d::Vec3& point, const cocos2d::AABB& triggerBox) {
    return triggerBox.containPoint(point);
}
