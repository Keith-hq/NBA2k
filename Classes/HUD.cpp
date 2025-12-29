#include "HUD.h"

USING_NS_CC;
using namespace cocos2d::ui;

HUD* HUD::create() {
    HUD* ret = new (std::nothrow) HUD();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool HUD::init() {
    if (!Layer::init()) return false;

    setupUI();
    
    // Listen for window resize if needed, usually Scene handles this or layout relative
    // For now we assume fixed design resolution or relative layout
    
    return true;
}

void HUD::setupUI() {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // Score Board Panel (Top Center-ish)
    auto topPanel = Layout::create();
    topPanel->setContentSize(Size(visibleSize.width, 100));
    topPanel->setPosition(Vec2(origin.x, origin.y + visibleSize.height - 100));
    topPanel->setBackGroundColorType(Layout::BackGroundColorType::NONE);
    addChild(topPanel);

    // Player Score (Left)
    _playerScoreText = Text::create("P1: 00", "fonts/Marker Felt.ttf", 36);
    _playerScoreText->setPosition(Vec2(visibleSize.width * 0.2f, 50));
    _playerScoreText->setColor(Color3B::WHITE);
    _playerScoreText->enableOutline(Color4B::BLACK, 2);
    topPanel->addChild(_playerScoreText);

    // AI Score (Right)
    _aiScoreText = Text::create("CPU: 00", "fonts/Marker Felt.ttf", 36);
    _aiScoreText->setPosition(Vec2(visibleSize.width * 0.8f, 50));
    _aiScoreText->setColor(Color3B::WHITE);
    _aiScoreText->enableOutline(Color4B::BLACK, 2);
    topPanel->addChild(_aiScoreText);

    // Game Time (Center Top)
    _gameTimeText = Text::create("05:00", "fonts/arial.ttf", 30);
    _gameTimeText->setPosition(Vec2(visibleSize.width * 0.5f, 75));
    _gameTimeText->setColor(Color3B::WHITE);
    _gameTimeText->enableOutline(Color4B::BLACK, 1);
    topPanel->addChild(_gameTimeText);

    // Shot Clock (Center Bottom)
    _shotClockText = Text::create("24", "fonts/arial.ttf", 48);
    _shotClockText->setPosition(Vec2(visibleSize.width * 0.5f, 30));
    _shotClockText->setColor(Color3B::YELLOW);
    _shotClockText->enableOutline(Color4B::BLACK, 2);
    topPanel->addChild(_shotClockText);

    // Feedback Text (Center Screen, slightly up)
    _feedbackText = Text::create("", "fonts/Marker Felt.ttf", 48);
    _feedbackText->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.7f));
    _feedbackText->setColor(Color3B::GREEN);
    _feedbackText->enableOutline(Color4B::BLACK, 2);
    _feedbackText->setVisible(false);
    addChild(_feedbackText);
}

void HUD::updateScore(int playerScore, int aiScore) {
    if (_playerScoreText) _playerScoreText->setString(StringUtils::format("P1: %d", playerScore));
    if (_aiScoreText) _aiScoreText->setString(StringUtils::format("CPU: %d", aiScore));
}

void HUD::updateTime(float gameTime) {
    int minutes = (int)(gameTime / 60);
    int seconds = (int)gameTime % 60;
    if (_gameTimeText) _gameTimeText->setString(StringUtils::format("%02d:%02d", minutes, seconds));
}

void HUD::updateShotClock(float shotClock) {
    if (_shotClockText) {
        _shotClockText->setString(StringUtils::format("%d", (int)ceil(shotClock)));
        if (shotClock <= 5.0f) {
            _shotClockText->setColor(Color3B::RED);
        } else {
            _shotClockText->setColor(Color3B::YELLOW);
        }
    }
}

 

void HUD::showFeedback(const std::string& text) {
    if (_feedbackText) {
        _feedbackText->setString(text);
        _feedbackText->setVisible(true);
        _feedbackText->setOpacity(255);
        _feedbackText->stopAllActions();
        _feedbackText->runAction(Sequence::create(
            DelayTime::create(1.0f),
            FadeOut::create(1.0f),
            Hide::create(),
            nullptr
        ));
    }
}

void HUD::onResize(const cocos2d::Size& size) {
    // Re-layout if needed. For now setupUI uses Director::getInstance()->getVisibleSize()
    // but that only runs on init.
    // Real implementation would update positions here.
    removeAllChildren();
    setupUI();
}
