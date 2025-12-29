#include "BasketballScene.h"
#include "HelloWorldScene.h"
#include "GameCore.h"
#include "SimplePhysics.h"
#include "AudioManager.h"
#include "SoundBank.h"
#include "CollisionSystem.h"
#include "InputSystem.h"
#include "AIController.h"
#include "ShootingSystem.h"
#include "EffectsManager.h"
#include "GameFeedback.h"
#include "GameFlow.h"
#include "MatchManager.h"
#include "GameIntegrator.h"
#include "Hoop.h"

USING_NS_CC;

Scene* BasketballScene::createScene() {
    return BasketballScene::create();
}

bool BasketballScene::init() {
    if (!Scene::init()) return false;
    
    // Initialize Systems
    GameCore::getInstance()->initPrimitives();
    CollisionSystem::getInstance()->reset();
    
    // Input System
    auto inputSystem = InputSystem::getInstance();
    if (inputSystem->getParent()) {
        inputSystem->removeFromParent();
    }
    addChild(inputSystem); // Add to scene to ensure update() is called
    
    // Init Managers
    GameFlow::getInstance()->reset();
    
    // Lights
    auto ambientLight = AmbientLight::create(Color3B(100, 100, 100));
    addChild(ambientLight);
    auto dirLight = DirectionLight::create(Vec3(-1, -2, -1), Color3B(200, 200, 200));
    addChild(dirLight);
    
    createCourt();
    createPlayer();
    createBall();
    setupCamera();
    
    // Init Rules & Match
    _gameRules = new GameRules(_player, _aiPlayer, _ball);
    MatchManager::getInstance()->init(_player, _aiPlayer, _ball);
    MatchManager::getInstance()->startMatch(); // Start with Jump Ball
    
    createUI();
    
    // Initialize Visual Managers
    EffectsManager::getInstance()->init(this);
    GameFeedback::getInstance()->init(this);
    
    // Audio
    AudioManager::getInstance()->setBackgroundMusicVolume(3.0f); 
    std::string musicPath = FileUtils::getInstance()->fullPathForFilename(SoundBank::BGM_GAME);
    if (!musicPath.empty()) {
        AudioManager::getInstance()->playBackgroundMusic(musicPath, true);
    }
    
    scheduleUpdate();
    
    return true;
}

void BasketballScene::createCourt() {
    auto core = GameCore::getInstance();
    
    // Floor (Plane)
    auto floor = Sprite3D::create(core->getPrimitivePath("plane"));
    if(floor) {
        floor->setScaleX(SimplePhysics::COURT_WIDTH);
        floor->setScaleZ(SimplePhysics::COURT_LENGTH);
        floor->setColor(Color3B(139, 69, 19)); // Brown
        floor->setPosition3D(Vec3(0, 0, 0));
        floor->setCameraMask((unsigned short)CameraFlag::USER1);
        addChild(floor);
        
        // Add physics plane
        auto body = new RigidBody(ColliderType::PLANE, SimplePhysics::MASK_FLOOR, SimplePhysics::MASK_PLAYER | SimplePhysics::MASK_BALL);
        body->setPlane(Vec3::UNIT_Y, 0.0f);
        CollisionSystem::getInstance()->addBody(body);
    }
    
    // Hoop Post (Cylinder)
    auto post = Sprite3D::create(core->getPrimitivePath("cylinder"));
    if(post) {
        post->setScaleX(0.2f);
        post->setScaleY(SimplePhysics::HOOP_HEIGHT);
        post->setScaleZ(0.2f);
        // Move post behind the hoop (Hoop is at HOOP_Z)
        post->setPosition3D(Vec3(0, SimplePhysics::HOOP_HEIGHT/2, SimplePhysics::HOOP_Z - 0.8f));
        post->setCameraMask((unsigned short)CameraFlag::USER1);
        addChild(post);
        
        // Physics for post
        auto body = new RigidBody(ColliderType::CAPSULE, SimplePhysics::MASK_HOOP, SimplePhysics::MASK_PLAYER | SimplePhysics::MASK_BALL);
        body->setCapsule(0.2f, SimplePhysics::HOOP_HEIGHT);
        body->setPosition(post->getPosition3D());
        body->setStatic(true);
        CollisionSystem::getInstance()->addBody(body);
    }
    
    // Hoop (Board, Ring, Net)
    auto hoop = Hoop::create();
    if (hoop) {
        hoop->setPosition3D(Vec3(0, SimplePhysics::HOOP_HEIGHT, SimplePhysics::HOOP_Z));
        hoop->setCameraMask((unsigned short)CameraFlag::USER1);
        addChild(hoop);
    }
    
    // Draw 3-Point Line
    auto drawNode = DrawNode::create();
    drawNode->setCameraMask((unsigned short)CameraFlag::USER1);
    addChild(drawNode, 2); // Above floor
    
    Vec3 hoopPos(0, 0.05f, SimplePhysics::HOOP_Z); // Slightly above floor
    float radius = 7.24f; // Standard 3-point distance
    int segments = 50;
    
    // Draw Arc
    std::vector<Vec2> points;
    for (int i = 0; i <= segments; i++) {
        float angle = -CC_DEGREES_TO_RADIANS(90) + CC_DEGREES_TO_RADIANS(180) * ((float)i / segments);
        // Angle from -90 to 90? No, we want an arc around the hoop.
        // Hoop is at -Z end. We want the arc facing +Z.
        // So angle 0 is +Z? 
        // X = sin(angle) * radius
        // Z = cos(angle) * radius + HOOP_Z
        // If angle 0 is +Z, then we want range roughly -90 to 90.
        
        float rad = -3.14159f / 2.0f + 3.14159f * ((float)i / segments); // -PI/2 to PI/2
        float x = sin(rad) * radius;
        float z = cos(rad) * radius + SimplePhysics::HOOP_Z;
        
        // Clip to court width?
        // Court width 30, half width 15. 3pt radius 7.24. It fits.
        
        drawNode->drawPoint(Vec2(x, z), 5, Color4F::WHITE); // Draw on XZ plane logic? No DrawNode is 2D usually?
        // Wait, DrawNode in 3D: setPosition3D works?
        // DrawNode draws on XY plane of its node.
        // So we rotate DrawNode to lie on XZ floor.
    }
    
    // Correct way: DrawNode is 2D. Rotate -90 on X axis.
    drawNode->setRotation3D(Vec3(-90, 0, 0));
    drawNode->setPosition3D(Vec3(0, 0.05f, 0)); // Slightly above floor
    
    // Re-calculate points for XY plane (which will be mapped to XZ)
    // In XY plane of DrawNode:
    // X is World X
    // Y is World Z (because of rotation)
    // Hoop Pos in DrawNode Space: (0, HOOP_Z)
    
    Vec2 center(0, -SimplePhysics::HOOP_Z); // Wait, if World Z is negative, and we rotate X -90...
    // World (0, 0, -27) -> Rotated Local (0, 27)?
    // Let's keep it simple: Use DrawNode3D if available? No.
    // Use DrawNode and trace points.
    
    // Arc
    for (int i = 0; i <= segments; i++) {
        float rad = -3.14159f / 2.0f + 3.14159f * ((float)i / segments); // -90 to 90 degrees
        float x = sin(rad) * radius;
        float y = cos(rad) * radius + SimplePhysics::HOOP_Z; // Map Z to Y
        
        // Draw line segments
        if (i > 0) {
            float prevRad = -3.14159f / 2.0f + 3.14159f * ((float)(i-1) / segments);
            float prevX = sin(prevRad) * radius;
            float prevY = cos(prevRad) * radius + SimplePhysics::HOOP_Z;
            
            drawNode->drawLine(Vec2(prevX, -prevY), Vec2(x, -y), Color4F::WHITE); // Why -y?
            // If we rotate -90 on X:
            // Local Y+ points to World Z- ?
            // Local Z+ points to World Y+ ?
            // Let's verify rotation.
            // Default: X right, Y up, Z out.
            // Rot -90 X: Y points Into Screen (World -Z), Z points Up (World Y).
            // So Local Y+ = World Z-.
            // So World Z = -Local Y  => Local Y = -World Z.
        }
    }
    
    // Draw straight lines to baseline?
    // Usually 3pt line becomes straight near baseline.
    // For now, simple arc is fine.
}

void BasketballScene::createPlayer() {
    // Human Player
    _player = Player::create();
    _player->setPosition3D(Vec3(0, 1.0f, 10.0f)); // Start at opposite side (+Z)
    _player->setRotation3D(Vec3(0, 180.0f, 0)); // Face -Z (Hoop)
    _player->setCameraMask((unsigned short)CameraFlag::USER1);
    _player->setBodyColor(Color3B::BLUE);
    
    _player->onShoot = [this](Player* p) {
        if (_gameRules) _gameRules->onBallShot(p);
    };
    
    addChild(_player);
    if (_player->getTrajectoryNode()) {
        addChild(_player->getTrajectoryNode(), 100); // Overlay
    }
    if (_player->getStaminaNode()) {
        addChild(_player->getStaminaNode(), 110); // Overlay above trajectory
    }
    
    _playerController = new HumanController();
    _player->setController(_playerController);

    // AI Player
    _aiPlayer = Player::create();
    _aiPlayer->setPosition3D(Vec3(0, 1.0f, 0.0f)); // Start near hoop
    _aiPlayer->setRotation3D(Vec3(0, 0, 0)); // Face +Z (Player)
    _aiPlayer->setCameraMask((unsigned short)CameraFlag::USER1);
    _aiPlayer->setBodyColor(Color3B::RED);
    // User Request: Reduce AI speed to make it easier to beat
    _aiPlayer->setStats(35.0f, 50.0f, 50.0f); // Speed 35 (was default 50)
    _aiPlayer->onShoot = [this](Player* p) {
        if (_gameRules) _gameRules->onBallShot(p);
    };
    addChild(_aiPlayer);
    if (_aiPlayer->getTrajectoryNode()) {
        addChild(_aiPlayer->getTrajectoryNode(), 100); // Overlay
    }
    
    // Link Opponents
    _player->setOpponent(_aiPlayer);
    _aiPlayer->setOpponent(_player);
}

void BasketballScene::createBall() {
    _ball = Basketball::create();
    _ball->setPosition3D(Vec3(2.0f, 5.0f, 10.0f)); // Drop near player
    _ball->setCameraMask((unsigned short)CameraFlag::USER1);
    // Ensure ball starts in a state where it can land and be picked up
    _ball->setState(Basketball::State::FLYING);
    addChild(_ball);
    
    // Link ball to player if close?
    if (_player) {
        _player->setBall(_ball); 
        // Game starts with player holding ball
        _player->setPossession(true);
        _ball->setOwner(_player);
        _ball->setState(Basketball::State::HELD);
    }

    if (_aiPlayer) {
        _aiPlayer->setBall(_ball);
        // AI does not have ball initially
        _aiPlayer->setPossession(false);
        
        // Create AI Controller
        _aiController = new AIController(_player, _ball, AIBrain::Difficulty::NORMAL);
        _aiController->setTarget(_aiPlayer);
        _aiPlayer->setController(_aiController);
    }
}

void BasketballScene::setupCamera() {
    _camera = Camera::createPerspective(60, (float)Director::getInstance()->getWinSize().width / Director::getInstance()->getWinSize().height, 1.0f, 1000.0f);
    _camera->setCameraFlag(CameraFlag::USER1);
    addChild(_camera);
}

void BasketballScene::update(float dt) {
    // Global Integrator
    GameIntegrator::getInstance()->update(dt);
    
    // Global Flow Update
    GameFlow::getInstance()->update(dt);
    MatchManager::getInstance()->update(dt);

    // Update Systems
    // InputSystem update is handled by Scene Graph (scheduleUpdate)
    CollisionSystem::getInstance()->update(dt);
    
    // Update Effects
    EffectsManager::getInstance()->update(dt);
    
    // Update Rules
    if (_gameRules) {
        _gameRules->update(dt);
    }
    
    updateUI();
    
    // Update Camera
    if (_playerController) {
        _playerController->updateCamera(_camera, dt);
    }
}

void BasketballScene::createUI() {
    _gameUI = GameUI::create();
    if (_gameUI) {
        addChild(_gameUI, 10); // UI on top
    }

    // Add Back Button
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto closeItem = MenuItemImage::create(
                                           "CloseNormal.png",
                                           "CloseSelected.png",
                                           [](Ref* sender){
                                               auto scene = HelloWorld::createScene();
                                               Director::getInstance()->replaceScene(TransitionFade::create(0.5f, scene));
                                           });

    if (closeItem) {
        float x = origin.x + visibleSize.width - closeItem->getContentSize().width/2 - 10;
        float y = origin.y + visibleSize.height - closeItem->getContentSize().height/2 - 10;
        closeItem->setPosition(Vec2(x, y));
        
        auto menu = Menu::create(closeItem, nullptr);
        menu->setPosition(Vec2::ZERO);
        addChild(menu, 20); // Above GameUI
    }
}

void BasketballScene::updateUI() {
    // _gameUI updates itself via scheduleUpdate
}
