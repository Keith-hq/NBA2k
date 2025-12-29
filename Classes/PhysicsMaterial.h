#ifndef __PHYSICS_MATERIAL_H__
#define __PHYSICS_MATERIAL_H__

struct SimplePhysicsMaterial {
    float restitution; // Bounciness (0.0 - 1.0)
    float friction;    // Friction coefficient (0.0 - 1.0)
    
    SimplePhysicsMaterial(float r = 0.5f, float f = 0.5f) 
        : restitution(r), friction(f) {}
};

#endif // __PHYSICS_MATERIAL_H__
