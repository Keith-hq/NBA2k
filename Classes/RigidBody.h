#ifndef __RIGID_BODY_H__
#define __RIGID_BODY_H__

#include "cocos2d.h"
#include "PhysicsMaterial.h"

enum class ColliderType {
    SPHERE,
    CAPSULE,
    PLANE
};

class RigidBody {
public:
    RigidBody(ColliderType type, int categoryMask, int collisionMask);
    ~RigidBody();

    // Shape properties
    void setSphere(float radius);
    void setCapsule(float radius, float height);
    void setPlane(const cocos2d::Vec3& normal, float constant); // Plane equation: normal.dot(p) + constant = 0

    // Physics State
    void setPosition(const cocos2d::Vec3& pos);
    cocos2d::Vec3 getPosition() const { return _position; }
    cocos2d::Vec3 getInterpolatedPosition(float alpha) const;
    
    void setVelocity(const cocos2d::Vec3& vel);
    cocos2d::Vec3 getVelocity() const { return _velocity; }
    
    void applyForce(const cocos2d::Vec3& force);
    void clearForces();

    // Material
    void setMaterial(const SimplePhysicsMaterial& material) { _material = material; }
    const SimplePhysicsMaterial& getMaterial() const { return _material; }

    // Properties
    void setMass(float mass);
    float getMass() const { return _mass; }
    float getInvMass() const { return _invMass; }
    
    void setKinematic(bool kinematic) { _isKinematic = kinematic; }
    bool isKinematic() const { return _isKinematic; }
    
    void setStatic(bool isStatic) { _isStatic = isStatic; }
    bool isStatic() const { return _isStatic; }

    // Collision
    ColliderType getType() const { return _type; }
    int getCategoryMask() const { return _categoryMask; }
    int getCollisionMask() const { return _collisionMask; }
    cocos2d::AABB getAABB() const;
    
    // Specific shape data getters
    float getRadius() const { return _radius; }
    float getHeight() const { return _height; }
    cocos2d::Vec3 getNormal() const { return _normal; }
    float getPlaneConstant() const { return _planeConstant; }
    
    // User Data (e.g., binding to Sprite3D)
    void setUserData(void* data) { _userData = data; }
    void* getUserData() const { return _userData; }

    std::function<void(RigidBody* other, float impulse)> onCollision;

    void update(float dt);

private:
    ColliderType _type;
    int _categoryMask;
    int _collisionMask;
    
    cocos2d::Vec3 _position;
    cocos2d::Vec3 _previousPosition;
    cocos2d::Vec3 _velocity;
    cocos2d::Vec3 _force;
    
    float _mass;
    float _invMass;
    SimplePhysicsMaterial _material;
    
    bool _isKinematic;
    bool _isStatic;
    
    // Shape data
    float _radius;
    float _height; // For Capsule (total height)
    cocos2d::Vec3 _normal; // For Plane
    float _planeConstant; // For Plane
    
    void* _userData;
};

#endif // __RIGID_BODY_H__
