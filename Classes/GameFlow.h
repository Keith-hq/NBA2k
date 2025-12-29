#ifndef __GAME_FLOW_H__
#define __GAME_FLOW_H__

#include "cocos2d.h"

class GameFlow {
public:
    enum class State {
        READY,
        PLAYING,
        PAUSED,
        FINISHED
    };

    static GameFlow* getInstance();
    static void destroyInstance();

    void reset();
    void update(float dt);
    
    void changeState(State newState);
    State getState() const { return _currentState; }

    bool isPlaying() const { return _currentState == State::PLAYING; }
    bool isPaused() const { return _currentState == State::PAUSED; }

    // Callbacks
    std::function<void(State)> onStateChanged;

private:
    GameFlow();
    ~GameFlow();

    static GameFlow* _instance;
    State _currentState;
};

#endif // __GAME_FLOW_H__
