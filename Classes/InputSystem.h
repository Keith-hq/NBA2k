#ifndef __INPUT_SYSTEM_H__
#define __INPUT_SYSTEM_H__

#include "cocos2d.h"
#include <map>
#include <vector>

enum class GameKey {
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    JUMP,    // Space
    SHOOT,   // Mouse Left
    PASS,    // Mouse Right
    SPRINT,  // Shift
    DEFEND,  // Alt or Auto
    STEAL,   // F
    CROSSOVER // E
};

class InputSystem : public cocos2d::Node {
public:
    static InputSystem* getInstance();
    
    virtual bool init() override;
    
    // Check current state
    bool isKeyPressed(GameKey key);
    bool isKeyDown(GameKey key); // Just pressed this frame
    bool isKeyUp(GameKey key);   // Just released this frame
    
    // Mouse
    cocos2d::Vec2 getMousePosition() const { return _mousePosition; }
    cocos2d::Vec2 getMouseDelta() const { return _mouseDelta; }
    
    // Update called by scene
    void update(float dt) override;
    
    // Setup listeners
    void setupInputListeners();

private:
    InputSystem();
    ~InputSystem();
    
    static InputSystem* _instance;
    
    std::map<cocos2d::EventKeyboard::KeyCode, bool> _keyState;
    std::map<GameKey, bool> _gameKeyState;
    std::map<GameKey, bool> _lastGameKeyState;
    
    cocos2d::Vec2 _mousePosition;
    cocos2d::Vec2 _mouseDelta;
    bool _mouseLeftDown;
    bool _mouseRightDown;
    
    // Key mapping
    GameKey mapKey(cocos2d::EventKeyboard::KeyCode keyCode);
    void updateGameKeys();
};

#endif // __INPUT_SYSTEM_H__
