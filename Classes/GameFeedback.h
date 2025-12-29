#ifndef __GAME_FEEDBACK_H__
#define __GAME_FEEDBACK_H__

#include "cocos2d.h"
#include <string>

class GameFeedback {
public:
    static GameFeedback* getInstance();
    static void destroyInstance();

    void init(cocos2d::Scene* scene);
    
    // Feedback Methods
    void showShotResult(const std::string& text, const cocos2d::Vec3& position, bool isGood);
    void showCombo(int count);
    void showScore(int points, const cocos2d::Vec3& position);
    
private:
    GameFeedback();
    ~GameFeedback();
    
    static GameFeedback* _instance;
    cocos2d::Scene* _scene;
    
    // Combo Tracking
    int _currentCombo;
    float _comboTimer;
};

#endif // __GAME_FEEDBACK_H__
