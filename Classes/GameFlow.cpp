#include "GameFlow.h"

USING_NS_CC;

GameFlow* GameFlow::_instance = nullptr;

GameFlow* GameFlow::getInstance() {
    if (!_instance) {
        _instance = new GameFlow();
    }
    return _instance;
}

void GameFlow::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

GameFlow::GameFlow() : _currentState(State::READY) {
}

GameFlow::~GameFlow() {
}

void GameFlow::reset() {
    _currentState = State::READY;
}

void GameFlow::update(float dt) {
    // Basic state machine logic could go here
    // But mostly it just holds state for other systems to check
}

void GameFlow::changeState(State newState) {
    if (_currentState == newState) return;

    _currentState = newState;
    
    if (onStateChanged) {
        onStateChanged(_currentState);
    }
    
    CCLOG("GameFlow State Changed to: %d", (int)_currentState);
}
