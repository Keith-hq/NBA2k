#ifndef __HOOP_H__
#define __HOOP_H__

#include "cocos2d.h"
#include <vector>

class RigidBody;

class Hoop : public cocos2d::Node {
public:
    static Hoop* create();
    virtual bool init() override;
    virtual ~Hoop();
    
    // Physics
    void initPhysics();
    
private:
    std::vector<RigidBody*> _physicsBodies;
    
    void createBackboard();
    void createRim();
    void createNet();
    
    // Constants
    const float RIM_RADIUS = 0.23f; // 45cm diameter / 2
    const float RIM_THICKNESS = 0.02f;
    const float BACKBOARD_WIDTH = 1.8f;
    const float BACKBOARD_HEIGHT = 1.05f;
    const float BACKBOARD_THICKNESS = 0.05f;
};

#endif // __HOOP_H__
