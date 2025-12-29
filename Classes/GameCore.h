#ifndef __GAME_CORE_H__
#define __GAME_CORE_H__

#include "cocos2d.h"
#include <string>

class GameCore {
public:
    static GameCore* getInstance();
    
    // Resource Management: Generate and get path to primitives
    void initPrimitives();
    std::string getPrimitivePath(const std::string& name);
    
    // Game Management
    void resetGame();
    void addScore(int points);
    int getScore() const { return _score; }
    
    // Config Management
    void loadConfig();
    bool isDebug() const { return _debugMode; }
    
private:
    GameCore();
    ~GameCore();
    
    static GameCore* _instance;
    int _score;
    bool _debugMode;
    std::string _writablePath;
    
    void generateObjFile(const std::string& filename, const std::string& content);
    void generateCube();
    void generatePlane();
    void generateCylinder();
    void generateSphere();
    void generateCapsule();
};

#endif // __GAME_CORE_H__
