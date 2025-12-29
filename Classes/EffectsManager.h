#ifndef __EFFECTS_MANAGER_H__
#define __EFFECTS_MANAGER_H__

#include "cocos2d.h"

class EffectsManager {
public:
    static EffectsManager* getInstance();
    static void destroyInstance();

    void init(cocos2d::Scene* scene);
    
    // Particles
    void playGoalEffect(const cocos2d::Vec3& position);
    void playRimHitEffect(const cocos2d::Vec3& position);
    void playSweatEffect(const cocos2d::Vec3& position);
    
    // Screen Effects
    void shakeScreen(float intensity, float duration);
    
    void update(float dt);

private:
    EffectsManager();
    ~EffectsManager();
    
    static EffectsManager* _instance;
    cocos2d::Scene* _scene;
    
    // Shake State
    float _shakeTimer;
    float _shakeIntensity;
    cocos2d::Vec3 _originalCameraPos;
};

#endif // __EFFECTS_MANAGER_H__
