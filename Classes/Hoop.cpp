#include "Hoop.h"
#include "GameCore.h"
#include "CollisionSystem.h"
#include "SimplePhysics.h"
#include "RigidBody.h"

USING_NS_CC;

Hoop* Hoop::create() {
    Hoop* ret = new (std::nothrow) Hoop();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

Hoop::~Hoop() {
    for (auto body : _physicsBodies) {
        CollisionSystem::getInstance()->removeBody(body);
        delete body;
    }
    _physicsBodies.clear();
}

bool Hoop::init() {
    if (!Node::init()) return false;
    
    createBackboard();
    createRim();
    createNet();
    
    initPhysics();
    
    return true;
}

void Hoop::createBackboard() {
    auto core = GameCore::getInstance();
    
    // Visual
    auto board = Sprite3D::create(core->getPrimitivePath("cube"));
    if (board) {
        board->setScaleX(BACKBOARD_WIDTH);
        board->setScaleY(BACKBOARD_HEIGHT);
        board->setScaleZ(BACKBOARD_THICKNESS);
        
        // Position relative to Hoop Node (0,0,0 is center of hoop ring?)
        // Standard: Rim is 15cm above bottom of board.
        // Hoop Center is (0, HOOP_HEIGHT, HOOP_Z).
        // Let's say Hoop Node is at (0, HOOP_HEIGHT, HOOP_Z).
        // Board is behind the rim.
        // Distance from face of board to center of rim = 15cm + radius? No.
        // Standard: 15cm from board to ring nearest point.
        // Ring Radius ~0.35. Center is 0.35 + 0.15 = 0.5m from board.
        
        float zOffset = -0.5f; // Board is behind
        float yOffset = 0.3f; // Board center is higher than rim
        
        board->setPosition3D(Vec3(0, yOffset, zOffset));
        board->setColor(Color3B(220, 220, 220)); // White/Glass
        board->setOpacity(200); // Semi-transparent
        addChild(board);
        
        // Target Box (Red Square)
        auto target = Sprite3D::create(core->getPrimitivePath("cube"));
        if (target) {
            target->setScaleX(0.59f);
            target->setScaleY(0.45f);
            target->setScaleZ(BACKBOARD_THICKNESS + 0.01f);
            target->setPosition3D(Vec3(0, yOffset - 0.15f, zOffset));
            target->setColor(Color3B::RED);
            addChild(target);
        }
    }
}

void Hoop::createRim() {
    auto core = GameCore::getInstance();
    
    // Visual Ring (Octagon approximation with cylinders)
    int segments = 8;
    float radius = 0.35f;
    float tubeRadius = 0.03f;
    
    for (int i = 0; i < segments; ++i) {
        float angle = i * (2 * M_PI / segments);
        float nextAngle = (i + 1) * (2 * M_PI / segments);
        
        // Midpoint
        float midAngle = (angle + nextAngle) / 2.0f;
        float x = cos(midAngle) * radius;
        float z = sin(midAngle) * radius; // Z is horizontal here too
        
        // Length of segment
        float segmentLength = 2 * radius * sin(M_PI / segments);
        
        auto seg = Sprite3D::create(core->getPrimitivePath("cylinder"));
        if (seg) {
            seg->setScaleX(tubeRadius); // Thickness
            seg->setScaleZ(tubeRadius); // Thickness
            seg->setScaleY(segmentLength); // Length
            
            seg->setPosition3D(Vec3(x, 0, z));
            
            // Rotation: Cylinder is Y-up. We need to lay it flat and rotate around Y
            // First rotate 90 X to lay flat (along Z)
            // Then rotate Y to face tangent
            
            // Actually, Cylinder Y axis is the length.
            // We want the length to be tangent to the circle.
            // Tangent angle is midAngle + 90 deg.
            
            float deg = CC_RADIANS_TO_DEGREES(midAngle);
            seg->setRotation3D(Vec3(90, -deg, 0)); 
            
            seg->setColor(Color3B(255, 69, 0)); // Orange/Red
            addChild(seg);
        }
    }
    
    // Connector to backboard
    auto connector = Sprite3D::create(core->getPrimitivePath("cube"));
    if (connector) {
        connector->setScaleX(0.1f);
        connector->setScaleY(0.05f);
        connector->setScaleZ(0.5f); // Length to board
        connector->setPosition3D(Vec3(0, 0, -0.25f));
        connector->setColor(Color3B(255, 69, 0));
        addChild(connector);
    }
}

void Hoop::createNet() {
    auto core = GameCore::getInstance();
    
    // Net Visual (Cone/Cylinder)
    // Tapered cylinder
    // Since we only have basic primitives, maybe just a white cylinder with transparency
    
    auto net = Sprite3D::create(core->getPrimitivePath("cylinder"));
    if (net) {
        float topRadius = 0.35f;
        float bottomRadius = 0.2f;
        float height = 0.4f;
        
        // We can't easily taper a cylinder primitive without shader/mesh mod.
        // Just use a cylinder.
        
        net->setScaleX(topRadius);
        net->setScaleZ(topRadius);
        net->setScaleY(height);
        net->setPosition3D(Vec3(0, -height/2, 0));
        net->setColor(Color3B::WHITE);
        net->setOpacity(100); // Transparent
        
        // Wireframe look?
        // simple texture would help, but we don't have one.
        
        addChild(net);
    }
}

void Hoop::initPhysics() {
    // 1. Rim Physics (8 spheres)
    int segments = 8;
    float radius = 0.35f;
    float sphereRadius = 0.05f; // Thin collider
    
    for (int i = 0; i < segments; ++i) {
        float angle = i * (2 * M_PI / segments);
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        
        auto body = new RigidBody(ColliderType::SPHERE, SimplePhysics::MASK_HOOP, SimplePhysics::MASK_BALL);
        body->setSphere(sphereRadius);
        body->setMass(0.0f); // Static
        body->setStatic(true);
        body->setMaterial(SimplePhysicsMaterial(0.5f, 0.5f));
        
        // Position is relative to World in Physics Engine!
        // We need to update this if Hoop moves (it doesn't).
        // Hoop is at (0, HOOP_HEIGHT, HOOP_Z).
        
        Vec3 worldPos = Vec3(0, SimplePhysics::HOOP_HEIGHT, SimplePhysics::HOOP_Z) + Vec3(x, 0, z);
        body->setPosition(worldPos);
        
        CollisionSystem::getInstance()->addBody(body);
        _physicsBodies.push_back(body);
    }
    
    // 2. Backboard Physics (Approximation with spheres)
    // Board is at Z = HOOP_Z - 0.5f
    // Size 1.8 x 1.05
    // Grid of spheres
    
    float boardZ = SimplePhysics::HOOP_Z - 0.5f;
    float boardY = SimplePhysics::HOOP_HEIGHT + 0.3f;
    
    int cols = 3;
    int rows = 2;
    float colSpacing = 0.5f;
    float rowSpacing = 0.4f;
    float colliderRadius = 0.3f;
    
    for (int c = -1; c <= 1; ++c) {
        for (int r = -1; r <= 1; ++r) {
            if (r == 0) continue; // Skip center row to reduce count? No.
            
            float x = c * colSpacing;
            float y = boardY + r * rowSpacing; // Relative to board center
            
            auto body = new RigidBody(ColliderType::SPHERE, SimplePhysics::MASK_HOOP, SimplePhysics::MASK_BALL);
            body->setSphere(colliderRadius);
            body->setStatic(true);
            body->setMaterial(SimplePhysicsMaterial(0.3f, 0.5f)); // Less bounce
            
            // Offset Z slightly to align front of sphere with board face
            // Board face is at boardZ + thickness/2.
            // Sphere surface should be there.
            // Center = FaceZ - Radius
            float z = boardZ - colliderRadius + 0.05f; 
            
            body->setPosition(Vec3(x, y, z));
            
            CollisionSystem::getInstance()->addBody(body);
            _physicsBodies.push_back(body);
        }
    }
}
