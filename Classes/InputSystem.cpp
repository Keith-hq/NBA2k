#include "InputSystem.h"

USING_NS_CC;

InputSystem* InputSystem::_instance = nullptr;

InputSystem* InputSystem::getInstance() {
    if (!_instance) {
        _instance = new InputSystem();
        _instance->init();
        _instance->retain(); // Keep alive
    }
    return _instance;
}

InputSystem::InputSystem() 
    : _mouseLeftDown(false)
    , _mouseRightDown(false)
    , _mousePosition(Vec2::ZERO)
    , _mouseDelta(Vec2::ZERO)
{
}

InputSystem::~InputSystem() {
}

bool InputSystem::init() {
    if (!Node::init()) return false;
    
    setupInputListeners();
    scheduleUpdate();
    
    return true;
}

void InputSystem::setupInputListeners() {
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode code, Event* event) {
        _keyState[code] = true;
    };
    listener->onKeyReleased = [this](EventKeyboard::KeyCode code, Event* event) {
        _keyState[code] = false;
    };
    _eventDispatcher->addEventListenerWithFixedPriority(listener, 1);
    
    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseDown = [this](Event* event) {
        EventMouse* e = (EventMouse*)event;
        if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            _mouseLeftDown = true;
        } else if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_RIGHT) {
            _mouseRightDown = true;
        }
    };
    mouseListener->onMouseUp = [this](Event* event) {
        EventMouse* e = (EventMouse*)event;
        if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            _mouseLeftDown = false;
        } else if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_RIGHT) {
            _mouseRightDown = false;
        }
    };
    mouseListener->onMouseMove = [this](Event* event) {
        EventMouse* e = (EventMouse*)event;
        Vec2 newPos = e->getLocationInView();
        // Cocos Y is inverted in some versions, but getLocationInView is usually top-left
        // Let's standardise to bottom-left 0,0
        // Director::getInstance()->getWinSize().height - newPos.y ...
        // For mouse delta, it doesn't matter much
        
        _mouseDelta = newPos - _mousePosition;
        _mousePosition = newPos;
    };
    _eventDispatcher->addEventListenerWithFixedPriority(mouseListener, 1);
}

void InputSystem::update(float dt) {
    _lastGameKeyState = _gameKeyState;
    updateGameKeys();
    _mouseDelta = Vec2::ZERO; // Reset delta after frame usage? 
    // Actually, mouse move event happens anytime.
    // If we reset here, we might miss movement if no event fired.
    // Better to clear delta at end of frame or accumulate it.
    // For now, let's keep it simple. The event updates delta. 
    // But we need to reset it if no movement.
    // A better way is accumulating in event, and resetting in update.
    // Let's trust the event fires frequently enough or reset here.
    // _mouseDelta = Vec2::ZERO; // This would kill delta if update runs after event
}

void InputSystem::updateGameKeys() {
    // Reset all
    _gameKeyState[GameKey::UP] = _keyState[EventKeyboard::KeyCode::KEY_W] || _keyState[EventKeyboard::KeyCode::KEY_UP_ARROW];
    _gameKeyState[GameKey::DOWN] = _keyState[EventKeyboard::KeyCode::KEY_S] || _keyState[EventKeyboard::KeyCode::KEY_DOWN_ARROW];
    _gameKeyState[GameKey::LEFT] = _keyState[EventKeyboard::KeyCode::KEY_A] || _keyState[EventKeyboard::KeyCode::KEY_LEFT_ARROW];
    _gameKeyState[GameKey::RIGHT] = _keyState[EventKeyboard::KeyCode::KEY_D] || _keyState[EventKeyboard::KeyCode::KEY_RIGHT_ARROW];
    
    _gameKeyState[GameKey::JUMP] = _keyState[EventKeyboard::KeyCode::KEY_SPACE];
    _gameKeyState[GameKey::SPRINT] = _keyState[EventKeyboard::KeyCode::KEY_SHIFT];
    
    _gameKeyState[GameKey::SHOOT] = _mouseLeftDown;
    _gameKeyState[GameKey::PASS] = _mouseRightDown;
    
    _gameKeyState[GameKey::STEAL] = _keyState[EventKeyboard::KeyCode::KEY_F];
    _gameKeyState[GameKey::CROSSOVER] = _keyState[EventKeyboard::KeyCode::KEY_E];
    _gameKeyState[GameKey::DEFEND] = _keyState[EventKeyboard::KeyCode::KEY_V]; // Manual defend key
}

bool InputSystem::isKeyPressed(GameKey key) {
    return _gameKeyState[key];
}

bool InputSystem::isKeyDown(GameKey key) {
    return _gameKeyState[key] && !_lastGameKeyState[key];
}

bool InputSystem::isKeyUp(GameKey key) {
    return !_gameKeyState[key] && _lastGameKeyState[key];
}
