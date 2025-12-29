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

    // Shot Meter (Bottom Center)
    _shotMeterPanel = Layout::create();
    _shotMeterPanel->setContentSize(Size(300, 40));
    _shotMeterPanel->setAnchorPoint(Vec2(0.5f, 0.0f));
    _shotMeterPanel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 100));
    addChild(_shotMeterPanel);
    
    // Background
    auto meterBg = Layout::create();
    meterBg->setContentSize(Size(300, 20));
    meterBg->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    meterBg->setBackGroundColor(Color3B::GRAY);
    meterBg->setPosition(Vec2(0, 10));
    _shotMeterPanel->addChild(meterBg);

    // Bar
    _shotBar = LoadingBar::create("sliderProgress.png"); // Using default or solid color
    // Since we might not have assets, let's use a LayerColor masked or scaled
    // But LoadingBar is standard. If image missing, it won't show.
    // Let's try to create a simple texture or use LayerColor for bar.
    // Actually, let's use a scaled Sprite for the bar content if LoadingBar fails, 
    // but for now let's assume we can use a simple white pixel stretched.
    
    // Fallback: Use Layout as bar
    // But LoadingBar is nicer.
    // Let's stick to simple primitives if assets unknown.
    // "sliderProgress.png" is standard in Cocos tests but maybe not here.
    // We'll create a white sprite manually.
    
    auto whiteTexture = new Texture2D();
    const unsigned char pixels[] = {255, 255, 255, 255};
    Image* image = new Image();
    image->initWithRawData(pixels, 4, 1, 1, 32); // 1x1 white pixel
    whiteTexture->initWithImage(image);
    image->release();
    
    // Create sprite from texture
    auto barSprite = Sprite::createWithTexture(whiteTexture);
    barSprite->setTextureRect(Rect(0,0,300,20));
    barSprite->setColor(Color3B::YELLOW);
    
    // We can't put Sprite into LoadingBar directly easily without file.
    // So let's just use a ProgressBar implemented with scaling LayerColor or Sprite.
    
    _shotBar = LoadingBar::create();
    _shotBar->setScale9Enabled(true);
    _shotBar->setContentSize(Size(300, 20));
    _shotBar->setDirection(LoadingBar::Direction::LEFT);
    _shotBar->setPercent(0);
    _shotBar->setPosition(Vec2(150, 20));
    // LoadingBar needs a texture to show anything.
    // If we don't have one, it's invisible.
    
    // Alternative: Use a Layout for the bar and change its width.
    auto barLayout = Layout::create();
    barLayout->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    barLayout->setBackGroundColor(Color3B::YELLOW);
    barLayout->setContentSize(Size(0, 20));
    barLayout->setPosition(Vec2(0, 10));
    barLayout->setName("BarLayout");
    _shotMeterPanel->addChild(barLayout);

    // Marker for optimal release
    _optimalMarker = Sprite::create();
    _optimalMarker->setTextureRect(Rect(0,0,4,26));
    _optimalMarker->setColor(Color3B::WHITE);
    _optimalMarker->setPosition(Vec2(200, 20)); // Default pos
    _shotMeterPanel->addChild(_optimalMarker);

    _shotMeterPanel->setVisible(false);
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

void HUD::updateShotMeter(float currentCharge, float optimalCharge) {
    if (!_shotMeterPanel->isVisible()) return;

    auto bar = _shotMeterPanel->getChildByName<Layout*>("BarLayout");
    if (bar) {
        // User Request: 0-2s range, 1.0s optimal.
        // We assume optimalCharge is passed as 1.0f.
        // We set max range to 2.0f.
        
        float maxRange = 2.0f; 
        if (optimalCharge > 0.1f) {
             // If optimal varies, maybe scale? But request was fixed 1.0s.
             // We'll stick to 2.0f fixed range for consistency.
        }
        
        float pct = currentCharge / maxRange;
        if (pct > 1.0f) pct = 1.0f;
        
        float width = 300.0f * pct;
        bar->setContentSize(Size(width, 20));
        
        // Color
        float diff = std::abs(currentCharge - optimalCharge);
        // Green window < 0.1s
        if (diff < 0.1f) bar->setBackGroundColor(Color3B::GREEN);
        else if (diff < 0.25f) bar->setBackGroundColor(Color3B::YELLOW);
        else bar->setBackGroundColor(Color3B::ORANGE);
    }
    
    // Position marker
    if (_optimalMarker) {
        // Marker should be at optimalCharge position
        float maxRange = 2.0f;
        float optPct = optimalCharge / maxRange;
        if (optPct > 1.0f) optPct = 1.0f;
        
        float markerX = 300.0f * optPct;
        _optimalMarker->setPosition(Vec2(markerX, 10));
    }
}


void HUD::showShotMeter(bool visible) {
    _shotMeterPanel->setVisible(visible);
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
