#include "PerformanceMonitor.h"

USING_NS_CC;

PerformanceMonitor* PerformanceMonitor::_instance = nullptr;

PerformanceMonitor* PerformanceMonitor::getInstance() {
    if (!_instance) {
        _instance = new PerformanceMonitor();
    }
    return _instance;
}

void PerformanceMonitor::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

PerformanceMonitor::PerformanceMonitor() 
    : _debugLabel(nullptr)
    , _isDebugVisible(false)
    , _drawCalls(0)
    , _collisionChecks(0)
    , _entityCount(0)
    , _fpsTimer(0.0f)
    , _frameCount(0)
    , _currentFPS(60.0f)
{
}

PerformanceMonitor::~PerformanceMonitor() {
}

void PerformanceMonitor::init(Scene* scene) {
    if (!scene) return;

    _debugLabel = Label::createWithTTF("Debug Info", "fonts/Marker Felt.ttf", 24);
    if (!_debugLabel) _debugLabel = Label::createWithSystemFont("Debug Info", "Arial", 24);
    
    _debugLabel->setPosition(Vec2(100, Director::getInstance()->getVisibleSize().height - 50));
    _debugLabel->setColor(Color3B::GREEN);
    _debugLabel->setVisible(_isDebugVisible);
    
    scene->addChild(_debugLabel, 999); // Topmost
}

void PerformanceMonitor::toggleDebugInfo() {
    _isDebugVisible = !_isDebugVisible;
    if (_debugLabel) {
        _debugLabel->setVisible(_isDebugVisible);
    }
    
    // Also toggle Cocos built-in stats
    Director::getInstance()->setDisplayStats(_isDebugVisible);
}

void PerformanceMonitor::update(float dt) {
    if (!_isDebugVisible) return;

    _fpsTimer += dt;
    _frameCount++;
    
    if (_fpsTimer >= 0.5f) {
        _currentFPS = _frameCount / _fpsTimer;
        _frameCount = 0;
        _fpsTimer = 0.0f;
        updateLabel();
    }
}

void PerformanceMonitor::updateLabel() {
    if (!_debugLabel) return;
    
    // Get Draw Calls from Renderer directly
    _drawCalls = (int)Director::getInstance()->getRenderer()->getDrawnBatches();
    
    std::string info = StringUtils::format(
        "FPS: %.1f\nEntities: %d\nCollisions: %d\nDraw Calls: %d", 
        _currentFPS,
        _entityCount,
        _collisionChecks,
        _drawCalls
    );
    
    _debugLabel->setString(info);
}
