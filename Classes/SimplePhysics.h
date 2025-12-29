#ifndef __SIMPLE_PHYSICS_H__
#define __SIMPLE_PHYSICS_H__

#include "cocos2d.h"

namespace SimplePhysics {
    // Game Constants
    constexpr float GRAVITY = -20.0f; // Increased gravity for snappier jumps
    constexpr float FLOOR_Y = 0.0f;
    constexpr float PLAYER_SPEED = 10.0f;
    constexpr float PLAYER_JUMP_SPEED = 4.5f; // Adjusted for new gravity (approx 0.5m height)
    constexpr float BALL_RADIUS = 0.25f;
    constexpr float CAMERA_DISTANCE = 15.0f;
    constexpr float CAMERA_HEIGHT = 10.0f;
    
    // Physics Masks
    constexpr int MASK_FLOOR = 1 << 0;
    constexpr int MASK_PLAYER = 1 << 1;
    constexpr int MASK_BALL = 1 << 2;
    constexpr int MASK_HOOP = 1 << 3;
    
    // Geometry Constants
    constexpr float COURT_WIDTH = 30.0f;   // Enlarged from 15.0f
    constexpr float COURT_LENGTH = 56.0f;  // Enlarged from 28.0f
    constexpr float HOOP_HEIGHT = 3.05f;
    
    // Hoop Position (Opposite side: -Z)
    constexpr float HOOP_Z = -COURT_LENGTH / 2.0f + 1.0f;
    
    // Physics Simulation Constants
    constexpr float FIXED_TIME_STEP = 1.0f / 60.0f;
    constexpr int SUB_STEPS = 4; // Higher sub-steps for better stability (especially fast ball)
    
    // Helper method declarations
    // Simple sphere-sphere collision check
    inline bool checkCollision(const cocos2d::Vec3& p1, float r1, const cocos2d::Vec3& p2, float r2) {
        return p1.distanceSquared(p2) < (r1 + r2) * (r1 + r2);
    }
    
    // Simple AABB collision check
    inline bool checkAABB(const cocos2d::Vec3& min1, const cocos2d::Vec3& max1, const cocos2d::Vec3& min2, const cocos2d::Vec3& max2) {
        return (min1.x <= max2.x && max1.x >= min2.x) &&
               (min1.y <= max2.y && max1.y >= min2.y) &&
               (min1.z <= max2.z && max1.z >= min2.z);
    }
}

#endif // __SIMPLE_PHYSICS_H__
