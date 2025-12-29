#ifndef __SHOOTING_SYSTEM_H__
#define __SHOOTING_SYSTEM_H__

#include "cocos2d.h"
#include "ShotCalculator.h"

class Player; // Forward declaration

class ShootingSystem : public cocos2d::Ref {
public:
    static ShootingSystem* create(Player* owner);
    bool init(Player* owner);
    
    // Input Handling
    void startShot();
    void update(float dt);
    void releaseShot();
    
    // State
    bool isCharging() const { return _isCharging; }
    float getChargePercent() const;
    float getCurrentChargeTime() const { return _currentChargeTime; }
    float getOptimalChargeTime() const { return _optimalChargeTime; }
    std::string getLastFeedback() const { return _lastFeedback; }
    
    // Visualization
    void drawTrajectory(cocos2d::DrawNode* debugNode);

private:
    Player* _owner;
    
    bool _isCharging;
    float _currentChargeTime;
    float _optimalChargeTime;
    
    std::string _lastFeedback;
    float _feedbackTimer;
    
    // Physics Helper
    cocos2d::Vec3 calculateVelocity(const cocos2d::Vec3& start, const cocos2d::Vec3& target, float flightTime);
};

#endif // __SHOOTING_SYSTEM_H__
