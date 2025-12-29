#ifndef __BASKETBALL_SCENE_H__
#define __BASKETBALL_SCENE_H__

#include "cocos2d.h"
#include "Player.h"
#include "Basketball.h"
#include "HumanController.h"
#include "AIController.h"
#include "GameRules.h"
#include "ScoreManager.h"
#include "GameUI.h"

class BasketballScene : public cocos2d::Scene {
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    
    CREATE_FUNC(BasketballScene);
    
    virtual void update(float dt) override;
    
private:
    void createCourt();
    void createPlayer();
    void createBall();
    void setupCamera();
    
    Player* _player;
    Player* _aiPlayer;
    Basketball* _ball;
    HumanController* _playerController;
    AIController* _aiController;
    cocos2d::Camera* _camera;
    
    GameRules* _gameRules;
    
    // UI
    GameUI* _gameUI;
    
    void createUI();
    void updateUI();
};

#endif // __BASKETBALL_SCENE_H__
