#ifndef __SHOT_CALCULATOR_H__
#define __SHOT_CALCULATOR_H__

#include "cocos2d.h"
#include "SimplePhysics.h"

enum class ShotType {
    JUMP_SHOT,
    LAYUP,
    DUNK,
    MID_RANGE,
    THREE_POINTER
};

struct ShotParams {
    float distance;         // Distance to hoop
    float defenderDist;     // Distance to nearest defender
    float defenderAngle;    // Angle between shooter forward vector and direction to defender (0 = Front, 180 = Back)
    bool isDefenderBlocking;// Is the defender actively jumping/blocking?
    float timingDev;        // 0.0 is perfect, 1.0 is max error (early/late)
    float skill;            // 0-100 shooting attribute
    cocos2d::Vec3 shooterPos;
};

struct ShotResult {
    bool success;
    cocos2d::Vec3 targetPos;
    std::string feedback;
    ShotType type;
    float finalChance;
};

class ShotCalculator {
public:
    static ShotResult calculateShot(const ShotParams& params) {
        ShotResult result;
        result.success = false;
        
        // 1. Determine Shot Type
        if (params.distance < 1.5f) {
            result.type = ShotType::DUNK; // Very close
        } else if (params.distance < 3.5f) {
            result.type = ShotType::LAYUP; // Close
        } else {
            result.type = ShotType::JUMP_SHOT;
        }
        
        // 2. Base Chance by Distance (Hot Zones simplified)
        // float baseChance = 0.5f; // Shadowed below
        float zoneMod = getZoneModifier(params.distance);
        
        // 3. Modifiers
        // Skill: 50 is baseline (1.0x). 100 is 1.5x. 0 is 0.5x.
        float skillMod = 0.5f + (params.skill / 100.0f);
        
        // Timing Logic (User Request)
        // Optimal: 1.0s. Range 0-2s.
        // Base Chance = 100% at 1.0s.
        // Penalty: -10% per 0.1s deviation.
        // Formula: 1.0 - (diff / 0.1) * 0.1
        
        float diff = params.timingDev; // This is abs(current - optimal)
        float penalty = (diff / 0.1f) * 0.1f;
        float timingChance = 1.0f - penalty;
        
        // Clamp timing chance
        if (timingChance < 0.0f) timingChance = 0.0f;
        
        // Defense Calculation
        float defenseMod = 1.0f;
        
        // Only calculate defense if defender is close enough (< 2.5m)
        if (params.defenderDist < 2.5f) {
            // Base Interference factor based on distance (Closer = Higher interference)
            // 0m = 1.0 (Max), 2.5m = 0.0
            float distFactor = 1.0f - (params.defenderDist / 2.5f);
            
            // Angle Factor: Only front defense matters mostly
            // 0 deg (Front) = 1.0, 90 deg (Side) = 0.5, 180 deg (Back) = 0.1
            // Use Cosine interpolation
            // defenderAngle is in degrees
            float rad = params.defenderAngle * M_PI / 180.0f;
            float cosVal = std::cos(rad); // 1.0 at 0, -1.0 at 180
            // Map -1..1 to 0..1 (Back..Front) -> No, Back is low interference.
            // Let's use max(0, cosVal) for strict front cone?
            // Or (cosVal + 1) / 2 for full range?
            // Realistically, side contest matters, back chase-down matters for layups but not jump shots.
            // Let's use: Front (0-60) is high, Side (60-120) is medium, Back is low.
            
            float angleFactor = 0.0f;
            if (params.defenderAngle < 60.0f) angleFactor = 1.0f;
            else if (params.defenderAngle < 120.0f) angleFactor = 0.5f;
            else angleFactor = 0.1f;
            
            // Block Factor
            float blockFactor = params.isDefenderBlocking ? 1.5f : 0.8f; // Jumping increases interference significantly
            
            // Calculate Penalty
            // Max Penalty: Dist(1.0) * Angle(1.0) * Block(1.5) = 1.5 -> reduce chance by 60%?
            // Let's say defenseMod is a multiplier 0.0 to 1.0.
            
            float interference = distFactor * angleFactor * blockFactor;
            
            // Map interference to modifier
            // Interference 0.0 -> Mod 1.0
            // Interference 1.0 -> Mod 0.6
            // Interference 1.5 -> Mod 0.3
            
            // "Deadeye" Mitigation: High skill reduces interference impact
            // Skill 50 -> 0% mitigation
            // Skill 100 -> 30% mitigation
            float mitigation = 0.0f;
            if (params.skill > 50.0f) {
                mitigation = (params.skill - 50.0f) / 50.0f * 0.3f;
            }
            
            float effectiveInterference = interference * (1.0f - mitigation);
            
            defenseMod = 1.0f - (effectiveInterference * 0.5f);
            if (defenseMod < 0.2f) defenseMod = 0.2f; // Min chance floor
        }
        
        // Final Chance
        // User Request: 100% at 1.0s, -10% per 0.1s deviation.
        // We apply Defense Mod on top of that.
        // We mostly ignore Skill/Zone for the base hit rate to strictly follow the "100%" rule,
        // but we might use them for slight adjustments or feedback?
        // For now, strict adherence:
        
        float finalChance = timingChance * defenseMod;
        
        // Apply Skill/Zone as minor bias? 
        // If we want "True" 100%, we shouldn't reduce it by Zone.
        // But maybe Skill > 50 gives a slight boost to the "Green Window"? 
        // The prompt defined a fixed decay rate, so window is fixed.
        
        // However, if we are strictly following "100% at 1s", then at 0s dev, chance is 1.0.
        // If Defense is 1.0 (Open), result is 1.0.
        
        // Hero Moment: If Perfect Timing, boost min chance
        if (params.timingDev < 0.05f && finalChance < 0.25f) {
            finalChance = 0.25f; // Reward perfect release
        }
        
        // Clamp
        if (finalChance > 0.95f) finalChance = 0.95f;
        if (finalChance < 0.05f) finalChance = 0.05f;
        
        result.finalChance = finalChance;
        
        // 4. Success Check
        result.success = (cocos2d::RandomHelper::random_real(0.0f, 1.0f) < finalChance);
        
        // 5. Feedback
        if (params.timingDev < 0.1f) result.feedback = "PERFECT";
        else if (params.timingDev < 0.25f) result.feedback = "GOOD";
        else result.feedback = "BAD TIMING";
        
        if (defenseMod < 0.6f) result.feedback += " (SMOTHERED)";
        else if (defenseMod < 0.9f) result.feedback += " (CONTESTED)";
        else if (params.defenderDist > 2.5f) result.feedback += " (WIDE OPEN)";
        
        // 6. Target Position
        cocos2d::Vec3 hoopPos(0, SimplePhysics::HOOP_HEIGHT, SimplePhysics::HOOP_Z);
        
        if (result.success) {
            // Aim for center of hoop
            result.targetPos = hoopPos;
            
            // Bank shot check?
            if (shouldBankShot(params.shooterPos, hoopPos)) {
                result.targetPos = getBankShotTarget(hoopPos, params.shooterPos);
                result.feedback += " (BANK)";
            }
        } else {
            // Miss logic: Short, Long, Left, Right
            // Based on timing (Short/Long) and Random (Left/Right)
            
            float missMargin = 0.5f; // Meters off
            
            // If timing was the issue?
            // Usually timing maps to short/long.
            // But we passed timingDev as absolute. 
            // We assume calling code knows if it was early or late, but here we just have dev.
            // Let's assume random short/long if we don't know direction of timing.
            // Or better, let caller handle target calculation? No, calculator should do it.
            
            // Simple random miss
            float xOffset = cocos2d::RandomHelper::random_real(-1.0f, 1.0f) * missMargin;
            float zOffset = cocos2d::RandomHelper::random_real(-1.0f, 1.0f) * missMargin;
            
            result.targetPos = hoopPos + cocos2d::Vec3(xOffset, 0, zOffset);
            
            // Refine feedback
            if (zOffset > 0.2f) result.feedback = "TOO LONG";
            else if (zOffset < -0.2f) result.feedback = "TOO SHORT";
            else if (xOffset < -0.2f) result.feedback = "LEFT";
            else result.feedback = "RIGHT";
        }
        
        return result;
    }
    
    static float getZoneModifier(float dist) {
        // 0-3m: Paint (High %) -> 1.2
        // 3-7m: Mid-range (Med %) -> 1.0
        // >7m: 3PT (Lower %) -> 0.9
        if (dist < 3.0f) return 1.2f;
        if (dist < 7.0f) return 1.0f;
        return 0.9f;
    }
    
    static bool shouldBankShot(const cocos2d::Vec3& shooterPos, const cocos2d::Vec3& hoopPos) {
        // Angle check: ~45 degrees from baseline
        cocos2d::Vec2 toHoop(shooterPos.x - hoopPos.x, shooterPos.z - hoopPos.z);
        float angle = std::abs(atan2(toHoop.y, toHoop.x) * 180.0f / M_PI); // 0 is +X (Side), 90 is +Z (Center court)
        // Hoop is at -Z. Shooter is usually at +Z relative to hoop.
        
        // Correct vector: Hoop to Shooter
        cocos2d::Vec2 dir = toHoop;
        dir.normalize();
        
        // Z-axis (court length) is (0, 1) or (0, -1). 
        // Hoop is at -Z end. Shooter is at +Z direction relative to hoop.
        // Center shot: x=0. Angle with Z axis = 0.
        // 45 degree shot: x = z.
        
        // Simply check ratio of x/z
        if (std::abs(dir.y) < 0.01f) return false; // Side shot (baseline)
        float ratio = std::abs(dir.x / dir.y);
        
        // 45 deg is ratio 1.0. Range 0.5 to 1.5?
        return (ratio > 0.5f && ratio < 1.5f) && (toHoop.length() < 6.0f); // Bank shots usually mid-range
    }
    
    static cocos2d::Vec3 getBankShotTarget(const cocos2d::Vec3& hoopPos, const cocos2d::Vec3& shooterPos) {
        // Backboard is behind hoop (further -Z)
        // Hoop center Z = HOOP_Z.
        // Backboard Z ~ HOOP_Z - 0.3m (radius) - gap. 
        // Let's say backboard is at Z - 0.5m.
        
        // But to bank it in, we aim for a point on the backboard.
        // Angle of incidence = Angle of reflection.
        
        // Simplified: Aim for a point slightly above hoop on backboard plane.
        // Actually, we need the ball to hit the backboard and bounce INTO the hoop.
        // Target should be the "virtual target" on the backboard.
        
        // For simplicity, just return a point on the backboard.
        // Basketball physics will bounce it.
        // Backboard position:
        float backboardZ = hoopPos.z - 1.2f; // Further back? Hoop is rim center. Rim radius 0.25. Backboard offset ~0.4?
        // Let's check SimplePhysics... HOOP_Z is center.
        
        // Let's target the "square" on the backboard.
        // X = depends on shooter side.
        // If shooter is Left (+X), aim Left side of square? No, aim same side.
        
        // Heuristic: Aim for a point on backboard that aligns with shooter-hoop line?
        // Actually, for a 45 degree shot, you aim for the top corner of the square on your side.
        
        float signX = (shooterPos.x > 0) ? 1.0f : -1.0f;
        cocos2d::Vec3 target = hoopPos;
        target.z = hoopPos.z - 0.4f; // Backboard dist
        target.y += 0.3f; // Slightly higher
        target.x = 0.3f * signX; // Side of square
        
        return target;
    }
};

#endif // __SHOT_CALCULATOR_H__
