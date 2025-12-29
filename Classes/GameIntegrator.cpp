#include "GameIntegrator.h"
#include "PerformanceMonitor.h"
#include "GameFlow.h"
#include "MatchManager.h"
#include "BasketballScene.h"

USING_NS_CC;

GameIntegrator* GameIntegrator::_instance = nullptr;

GameIntegrator* GameIntegrator::getInstance() {
    if (!_instance) {
        _instance = new GameIntegrator();
    }
    return _instance;
}

void GameIntegrator::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

GameIntegrator::GameIntegrator() : _debugListener(nullptr) {
}

GameIntegrator::~GameIntegrator() {
}

void GameIntegrator::init(Scene* scene) {
    // Init Performance Monitor
    PerformanceMonitor::getInstance()->init(scene);
    
    // Global Keyboard Listener (F1, F2)
    _debugListener = EventListenerKeyboard::create();
    _debugListener->onKeyPressed = CC_CALLBACK_2(GameIntegrator::onKeyPressed, this);
    scene->getEventDispatcher()->addEventListenerWithSceneGraphPriority(_debugListener, scene);
}

void GameIntegrator::update(float dt) {
    PerformanceMonitor::getInstance()->update(dt);
}

void GameIntegrator::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event) {
    switch (keyCode) {
        case EventKeyboard::KeyCode::KEY_F1:
            PerformanceMonitor::getInstance()->toggleDebugInfo();
            break;
            
        case EventKeyboard::KeyCode::KEY_F2:
            // Soft Reset
            GameFlow::getInstance()->reset();
            MatchManager::getInstance()->reset();
            // Reload Scene
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, BasketballScene::createScene()));
            break;
            
        default:
            break;
    }
}
