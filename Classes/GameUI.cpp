#include "GameUI.h"
#include "GameCore.h"
#include "BasketballScene.h"
#include "ScoreManager.h"
#include "MatchManager.h"
#include "GameFlow.h"

USING_NS_CC;
using namespace cocos2d::ui;

GameUI* GameUI::_instance = nullptr;

GameUI* GameUI::getInstance() {
    return _instance;
}

GameUI::GameUI() 
    : _hud(nullptr)
    , _pauseLayer(nullptr)
    , _gameOverLayer(nullptr)
    , _resultLabel(nullptr)
{
}

GameUI::~GameUI() {
    if (_instance == this) {
        _instance = nullptr;
    }
}

GameUI* GameUI::create() {
    GameUI* pRet = new (std::nothrow) GameUI();
    if (pRet && pRet->init()) {
        pRet->autorelease();
        return pRet;
    }
    else {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool GameUI::init() {
    if (!Layer::init()) return false;
    
    _instance = this;
    
    // Create HUD
    _hud = HUD::create();
    if (_hud) {
        addChild(_hud, 0);
    }
    
    createPauseMenu();
    createGameOverScreen();
    
    scheduleUpdate();
    
    return true;
}

void GameUI::update(float dt) {
    if (_hud) {
        ScoreManager* sm = ScoreManager::getInstance();
        _hud->updateScore(sm->getPlayerScore(), sm->getAIScore());
        _hud->updateTime(sm->getGameTime());
        _hud->updateShotClock(sm->getShotClock());
        
        // Shot Meter syncing is done via ShootingSystem usually, 
        // or we can pull it if accessible. 
        // For now HUD has methods that ShootingSystem calls directly or via GameUI.
    }
}

void GameUI::createPauseMenu() {
    _pauseLayer = LayerColor::create(Color4B(0, 0, 0, 150));
    _pauseLayer->setVisible(false);
    this->addChild(_pauseLayer, 100);
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 center = Vec2(visibleSize.width / 2, visibleSize.height / 2);
    
    auto label = Label::createWithTTF("PAUSED", "fonts/Marker Felt.ttf", 64);
    if (!label) label = Label::createWithSystemFont("PAUSED", "Arial", 64);
    label->setPosition(center + Vec2(0, 100));
    _pauseLayer->addChild(label);
    
    // Resume Button
    auto resumeBtn = Button::create();
    if (resumeBtn) {
        resumeBtn->ignoreContentAdaptWithSize(false);
        resumeBtn->setContentSize(Size(200, 50));
        resumeBtn->setTitleText("RESUME");
        resumeBtn->setTitleFontSize(32);
        resumeBtn->setTitleFontName("fonts/Marker Felt.ttf");
        resumeBtn->setPosition(center);
        resumeBtn->addClickEventListener([](Ref* sender) {
            MatchManager::getInstance()->resumeMatch();
        });
        _pauseLayer->addChild(resumeBtn);
    }
    
    // Quit Button
    auto quitBtn = Button::create();
    if (quitBtn) {
        quitBtn->ignoreContentAdaptWithSize(false);
        quitBtn->setContentSize(Size(200, 50));
        quitBtn->setTitleText("QUIT");
        quitBtn->setTitleFontSize(32);
        quitBtn->setTitleFontName("fonts/Marker Felt.ttf");
        quitBtn->setPosition(center - Vec2(0, 80));
        quitBtn->addClickEventListener([](Ref* sender) {
            Director::getInstance()->end();
        });
        _pauseLayer->addChild(quitBtn);
    }
}

void GameUI::createGameOverScreen() {
    _gameOverLayer = LayerColor::create(Color4B(0, 0, 0, 200));
    _gameOverLayer->setVisible(false);
    this->addChild(_gameOverLayer, 100);
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 center = Vec2(visibleSize.width / 2, visibleSize.height / 2);
    
    _resultLabel = Label::createWithTTF("GAME OVER", "fonts/Marker Felt.ttf", 72);
    if (!_resultLabel) _resultLabel = Label::createWithSystemFont("GAME OVER", "Arial", 72);
    _resultLabel->setPosition(center + Vec2(0, 100));
    _gameOverLayer->addChild(_resultLabel);
    
    // Restart Button
    auto restartBtn = Button::create();
    if (restartBtn) {
        restartBtn->ignoreContentAdaptWithSize(false);
        restartBtn->setContentSize(Size(200, 50));
        restartBtn->setTitleText("RESTART");
        restartBtn->setTitleFontSize(32);
        restartBtn->setTitleFontName("fonts/Marker Felt.ttf");
        restartBtn->setPosition(center);
        restartBtn->addClickEventListener([](Ref* sender) {
            GameFlow::getInstance()->reset();
            MatchManager::getInstance()->reset();
            // Reload Scene
            auto scene = BasketballScene::createScene();
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, scene));
        });
        _gameOverLayer->addChild(restartBtn);
    }
}

void GameUI::showPauseMenu(bool show) {
    if (_pauseLayer) {
        _pauseLayer->setVisible(show);
    }
}

void GameUI::showGameOver(bool isWin) {
    if (_gameOverLayer) {
        _gameOverLayer->setVisible(true);
        if (_resultLabel) {
            _resultLabel->setString(isWin ? "YOU WIN!" : "YOU LOSE!");
            _resultLabel->setColor(isWin ? Color3B::GREEN : Color3B::RED);
        }
    }
}
