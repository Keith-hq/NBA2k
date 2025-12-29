#include "GameFeedback.h"

USING_NS_CC;

GameFeedback* GameFeedback::_instance = nullptr;

GameFeedback* GameFeedback::getInstance() {
    if (!_instance) {
        _instance = new GameFeedback();
    }
    return _instance;
}

void GameFeedback::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

GameFeedback::GameFeedback() 
    : _scene(nullptr)
    , _currentCombo(0)
    , _comboTimer(0.0f)
{
}

GameFeedback::~GameFeedback() {
}

void GameFeedback::init(Scene* scene) {
    _scene = scene;
}

void GameFeedback::showShotResult(const std::string& text, const Vec3& position, bool isGood) {
    if (!_scene || text.empty()) return;
    
    auto camera = _scene->getDefaultCamera();
    if (!camera) return;
    
    Vec2 screenPos = camera->project(position + Vec3(0, 1.0f, 0)); // Above player
    
    auto label = Label::createWithTTF(text, "fonts/Marker Felt.ttf", 24);
    if (!label) label = Label::createWithSystemFont(text, "Arial", 24);
    
    label->setPosition(screenPos);
    label->setColor(isGood ? Color3B::GREEN : Color3B::RED);
    label->enableOutline(Color4B::BLACK, 2);
    _scene->addChild(label, 2000); // UI Layer
    
    // Animation
    auto move = MoveBy::create(1.0f, Vec2(0, 50));
    auto fade = FadeOut::create(1.0f);
    auto spawn = Spawn::create(move, fade, nullptr);
    auto seq = Sequence::create(spawn, RemoveSelf::create(), nullptr);
    
    label->runAction(seq);
}

void GameFeedback::showScore(int points, const Vec3& position) {
    if (!_scene) return;
    
    auto camera = _scene->getDefaultCamera();
    if (!camera) return;
    
    Vec2 screenPos = camera->project(position);
    
    std::string text = "+" + std::to_string(points);
    auto label = Label::createWithTTF(text, "fonts/Marker Felt.ttf", 48);
    if (!label) label = Label::createWithSystemFont(text, "Arial", 48);
    
    label->setPosition(screenPos);
    label->setColor(Color3B::YELLOW);
    label->enableOutline(Color4B::BLACK, 3);
    _scene->addChild(label, 2000);
    
    // Animation: Scale up then fade
    label->setScale(0.0f);
    auto scale = ScaleTo::create(0.2f, 1.2f);
    auto bounce = ScaleTo::create(0.1f, 1.0f);
    auto delay = DelayTime::create(0.5f);
    auto fade = FadeOut::create(0.5f);
    auto move = MoveBy::create(0.5f, Vec2(0, 50));
    auto spawn = Spawn::create(fade, move, nullptr);
    auto seq = Sequence::create(scale, bounce, delay, spawn, RemoveSelf::create(), nullptr);
    
    label->runAction(seq);
}

void GameFeedback::showCombo(int count) {
    if (!_scene || count < 2) return;
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 center = visibleSize / 2;
    
    std::string text = std::to_string(count) + "x COMBO!";
    auto label = Label::createWithTTF(text, "fonts/Marker Felt.ttf", 64);
    if (!label) label = Label::createWithSystemFont(text, "Arial", 64);
    
    label->setPosition(center + Vec2(0, 100)); // Slightly above center
    label->setColor(Color3B::ORANGE);
    label->enableOutline(Color4B::RED, 4);
    label->setRotation(-10.0f);
    _scene->addChild(label, 2000);
    
    // Animation
    label->setScale(2.0f);
    label->setOpacity(0);
    
    auto fadeIn = FadeIn::create(0.1f);
    auto scaleDown = ScaleTo::create(0.2f, 1.0f);
    auto shake = Sequence::create(
        MoveBy::create(0.05f, Vec2(-10, 0)),
        MoveBy::create(0.05f, Vec2(20, 0)),
        MoveBy::create(0.05f, Vec2(-10, 0)),
        nullptr
    );
    auto delay = DelayTime::create(1.0f);
    auto fadeOut = FadeOut::create(0.5f);
    auto seq = Sequence::create(
        Spawn::create(fadeIn, scaleDown, nullptr),
        shake,
        delay,
        fadeOut,
        RemoveSelf::create(),
        nullptr
    );
    
    label->runAction(seq);
}
