#ifndef __GAME_INTEGRATOR_H__
#define __GAME_INTEGRATOR_H__

#include "cocos2d.h"

class GameIntegrator {
public:
    static GameIntegrator* getInstance();
    static void destroyInstance();

    // Setup listeners and global logic
    void init(cocos2d::Scene* scene);
    
    // Called by scene update
    void update(float dt);

private:
    GameIntegrator();
    ~GameIntegrator();

    static GameIntegrator* _instance;
    cocos2d::EventListenerKeyboard* _debugListener;
    
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
};

#endif // __GAME_INTEGRATOR_H__
