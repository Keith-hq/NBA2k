#ifndef __PERFORMANCE_MONITOR_H__
#define __PERFORMANCE_MONITOR_H__

#include "cocos2d.h"

class PerformanceMonitor {
public:
    static PerformanceMonitor* getInstance();
    static void destroyInstance();

    void init(cocos2d::Scene* scene);
    void update(float dt);
    
    // Toggles
    void toggleDebugInfo();
    bool isDebugVisible() const { return _isDebugVisible; }

    // Metrics tracking
    void recordDrawCall(int count) { _drawCalls = count; } // Usually pulled from renderer
    void recordCollisionChecks(int count) { _collisionChecks = count; }
    void recordEntityCount(int count) { _entityCount = count; }

private:
    PerformanceMonitor();
    ~PerformanceMonitor();

    static PerformanceMonitor* _instance;
    
    cocos2d::Label* _debugLabel;
    bool _isDebugVisible;
    
    // Metrics
    int _drawCalls;
    int _collisionChecks;
    int _entityCount;
    float _fpsTimer;
    int _frameCount;
    float _currentFPS;
    
    void updateLabel();
};

#endif // __PERFORMANCE_MONITOR_H__
