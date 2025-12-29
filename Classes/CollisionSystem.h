#ifndef __COLLISION_SYSTEM_H__
#define __COLLISION_SYSTEM_H__

#include "RigidBody.h"
#include <vector>
#include <utility>

class CollisionSystem {
public:
    static CollisionSystem* getInstance();

    void reset();

    void addBody(RigidBody* body);
    void removeBody(RigidBody* body);
    
    void update(float dt);
    
    // Get alpha for interpolation (0.0 - 1.0)
    float getAlpha() const;

    // Check if a point is inside a trigger (for Hoop)
    bool checkTrigger(const cocos2d::Vec3& point, const cocos2d::AABB& triggerBox);

private:
    CollisionSystem();
    ~CollisionSystem();
    
    static CollisionSystem* _instance;
    std::vector<RigidBody*> _bodies;
    
    float _accumulator;
    
    struct Manifold {
        RigidBody* a;
        RigidBody* b;
        cocos2d::Vec3 normal;
        float depth;
    };
    
    // Cache for temporal coherence
    std::vector<std::pair<RigidBody*, RigidBody*>> _cachedPairs;

    void fixedUpdate(float dt);
    void applyForces(float dt);
    void enforceBoundaries();
    void broadPhase(std::vector<std::pair<RigidBody*, RigidBody*>>& pairs);
    void narrowPhase(std::vector<std::pair<RigidBody*, RigidBody*>>& pairs, std::vector<Manifold>& manifolds);
    void resolveCollisions(const std::vector<Manifold>& manifolds);
    
    // Detection primitives
    bool detectSpherePlane(RigidBody* sphere, RigidBody* plane, Manifold& m);
    bool detectSphereSphere(RigidBody* s1, RigidBody* s2, Manifold& m);
    bool detectSphereCapsule(RigidBody* sphere, RigidBody* capsule, Manifold& m);
    bool detectCapsulePlane(RigidBody* capsule, RigidBody* plane, Manifold& m);
};

#endif // __COLLISION_SYSTEM_H__
