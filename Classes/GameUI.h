#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "HUD.h"

class GameUI : public cocos2d::Layer {
public:
    static GameUI* getInstance();
    static GameUI* create();
    virtual bool init() override;
    virtual void update(float dt) override;
    
    // Lifecycle
    GameUI();
    virtual ~GameUI();

    HUD* getHUD() const { return _hud; }

    // Menus
    void showPauseMenu(bool show);
    void showGameOver(bool isWin);

private:
    static GameUI* _instance;

    HUD* _hud;
    cocos2d::LayerColor* _pauseLayer;
    cocos2d::LayerColor* _gameOverLayer;
    cocos2d::Label* _resultLabel;

    void createPauseMenu();
    void createGameOverScreen();
};

#endif // __GAME_UI_H__
