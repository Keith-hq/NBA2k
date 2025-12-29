#include "Player.h"
#include "GameCore.h"
#include "SimplePhysics.h"
#include "CollisionSystem.h"
#include "AnimationPlayer.h"
#include "ShootingSystem.h"
#include "DribbleSystem.h"
#include "DefenseSystem.h"
#include "2d/CCCamera.h"
#include "2d/CCScene.h"

USING_NS_CC;

Player* Player::create() {
    Player* pRet = new (std::nothrow) Player();
    if (pRet && pRet->init()) {
        pRet->autorelease();
        return pRet;
    }
    else {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

Player::~Player() {
    CC_SAFE_RELEASE(_animPlayer);
    CC_SAFE_RELEASE(_shootingSystem);
    CC_SAFE_RELEASE(_dribbleSystem);
    CC_SAFE_RELEASE(_defenseSystem);
    CC_SAFE_RELEASE(_trajectoryNode);
    CC_SAFE_RELEASE(_staminaNode);
    if (_body) delete _body;
}

bool Player::init() {
    if (!Node::init()) return false;
    
    _controller = nullptr;
    _ball = nullptr;
    _hasBall = false;
    _opponent = nullptr;
    _state = State::IDLE;
    
    _speedStat = 50.0f;
    _shootingStat = 50.0f;
    _defenseStat = 50.0f;
    
    _shootChargeTime = 0.0f;
    _isChargingShot = false;
    _pickupCooldown = 0.0f;
    _mustClearBall = false;
    _stamina = 1.0f;
    
    // Visuals
    setCascadeColorEnabled(true);
    _visualNode = Node::create();
    _visualNode->setCascadeColorEnabled(true);
    addChild(_visualNode);
    
    _trajectoryNode = DrawNode::create();
    if (_trajectoryNode) {
        _trajectoryNode->retain(); // Keep it alive, will be added to Scene later
    }
    
    _staminaNode = DrawNode::create();
    if (_staminaNode) {
        _staminaNode->retain();
        _staminaNode->setIsolated(true);
        _staminaNode->setCameraMask((unsigned short)CameraFlag::DEFAULT);
    }
    
    // Load Model
    _model = Sprite3D::create("models/player/skeleton/character.c3b");
    if (_model) {
        _model->setTexture("models/player/textures/1.png");
        
        // Auto-scale to match player height (1.8m)
        auto aabb = _model->getAABB();
        Vec3 size = aabb._max - aabb._min;
        if (size.y > 0.1f) {
            float scale = HEIGHT / size.y;
            _model->setScale(scale);
            CCLOG("Player model auto-scaled. Raw Height: %.2f, Scale: %.4f", size.y, scale);
        } else {
            _model->setScale(3.0f); // Fallback
        }

        _model->setPosition3D(Vec3(0, 0, 0));
        _model->setRotation3D(Vec3(0, 180, 0)); // Fix rotation if needed
        _model->setCascadeColorEnabled(true);
        _visualNode->addChild(_model);
    }
    
    // Animation Player
    _animPlayer = AnimationPlayer::create(this);
    if (_animPlayer) {
        _animPlayer->retain(); // Keep it alive
    }
    
    // Shooting System
    _shootingSystem = ShootingSystem::create(this);
    if (_shootingSystem) {
        _shootingSystem->retain();
    }

    // Dribble System
    _dribbleSystem = DribbleSystem::create(this);
    if (_dribbleSystem) {
        _dribbleSystem->retain();
    }

    // Defense System
    _defenseSystem = DefenseSystem::create(this);
    if (_defenseSystem) {
        _defenseSystem->retain();
    }
    
    // Physics
    _body = new RigidBody(ColliderType::CAPSULE, SimplePhysics::MASK_PLAYER, SimplePhysics::MASK_FLOOR | SimplePhysics::MASK_PLAYER | SimplePhysics::MASK_BALL);
    _body->setCapsule(RADIUS, HEIGHT);
    _body->setMass(80.0f); // 80kg
    _body->setMaterial(SimplePhysicsMaterial(0.0f, 0.2f)); // No bounce, low friction
    _body->setUserData(this);
    
    CollisionSystem::getInstance()->addBody(_body);
    
    scheduleUpdate();
    
    return true;
}

void Player::reset() {
    CCLOG("Player::reset() called");
    _state = State::IDLE;
    _hasBall = false;
    _shootChargeTime = 0.0f;
    _isChargingShot = false;
    _pickupCooldown = 0.0f;
    _mustClearBall = false;
    _stamina = 1.0f;
    _recoveryTimer = 0.0f;
    _celebrationTimer = 0.0f;
    
    // Reset Systems
    if (_shootingSystem) {
        // _shootingSystem->reset(); // Implement if needed
    }
    if (_dribbleSystem) {
        _dribbleSystem->stopDribble();
    }
    if (_defenseSystem) {
        _defenseSystem->exitStance();
    }
    
    // Reset Visuals
    if (_animPlayer) {
        _animPlayer->playState(AnimationPlayer::AnimState::IDLE);
    }
    
    // Reset Physics Velocity
    if (_body) {
        _body->setVelocity(Vec3::ZERO);
    }
    
    if (_trajectoryNode) _trajectoryNode->clear();
    if (_staminaNode) _staminaNode->clear();
}

void Player::setStats(float speed, float shooting, float defense) {
    _speedStat = speed;
    _shootingStat = shooting;
    _defenseStat = defense;
}

void Player::setPossession(bool hasBall) {
    _hasBall = hasBall;
    if (!hasBall) {
        _pickupCooldown = 1.0f; // 1 second cooldown after losing ball
    }
}

void Player::setBodyColor(const Color3B& color) {
    if (_model) {
        _model->setColor(color);
    }
}

void Player::setController(PlayerController* controller) {
    _controller = controller;
    if (_controller) {
        _controller->setTarget(this);
    }
}

void Player::setPosition3D(const Vec3& pos) {
    Node::setPosition3D(pos);
    if (_body) {
        _body->setPosition(pos);
    }
}

Vec3 Player::getPosition3D() const {
    if (_body) return _body->getPosition();
    return Node::getPosition3D();
}

bool Player::canJump() const {
    // Cannot jump if busy
    if (_state == State::SHOOTING || _state == State::CELEBRATING) return false;
    if (_defenseSystem && _defenseSystem->isStunned()) return false;
    
    // Ground check (allow small tolerance)
    float y = _body ? _body->getPosition().y : getPosition3D().y;
    return y <= 0.1f;
}

void Player::jump() {
    if (!canJump()) return;
    
    if (_body) {
        Vec3 vel = _body->getVelocity();
        vel.y = SimplePhysics::PLAYER_JUMP_SPEED;
        _body->setVelocity(vel);
        
        // Play animation
        if (_animPlayer) {
            _animPlayer->playJump();
        }
    }
}

void Player::update(float dt) {
    if (_body) {
        // Sync visual with physics (Interpolated)
        float alpha = CollisionSystem::getInstance()->getAlpha();
        Node::setPosition3D(_body->getInterpolatedPosition(alpha));
    }
    
    // Update Systems
    if (_shootingSystem) {
        _shootingSystem->update(dt);
    }
    if (_dribbleSystem) {
        _dribbleSystem->update(dt);
    }
    if (_defenseSystem) {
        _defenseSystem->update(dt);
    }
    
    handleMovement(dt);
    handleActions(dt);
    updateBallPosition();
    
    if (_controller) {
        _controller->update(dt);
    }
    
    updateVisuals();
}

void Player::celebrate() {
    _state = State::CELEBRATING;
    _celebrationTimer = 2.0f; // Celebrate for 2 seconds
    
    // Stop movement
    if (_body) {
        Vec3 vel = _body->getVelocity();
        vel.x = 0;
        vel.z = 0;
        _body->setVelocity(vel);
    }
}

void Player::handleMovement(float dt) {
    if (_state == State::CELEBRATING) {
        _celebrationTimer -= dt;
        if (_celebrationTimer <= 0) {
            _state = State::IDLE;
        }
        return; // Skip other updates? Or allow movement?
        // Usually celebrate locks movement.
    }

    if (_pickupCooldown > 0) {
        _pickupCooldown -= dt;
    }

    if (!_controller) return;

    // Check if grounded for input control (User Request: Silence input in air)
    float currentY = _body ? _body->getPosition().y : getPosition3D().y;
    bool isGrounded = (currentY <= 0.15f);

    if (!isGrounded) {
        // In air: No input response
        
        // Fix state if needed
        if (_state == State::DRIBBLING) {
            _state = State::IDLE;
        }
        
        // Continue stamina regen
        _stamina += 0.25f * dt;
        if (_stamina > 1.0f) _stamina = 1.0f;
        
        return;
    }
    
    // Check Stun (Ankle Breaker)
    bool isStunned = (_defenseSystem && _defenseSystem->isStunned());

    // User Request: Stop moving/dribbling when shooting
    if (_state == State::SHOOTING) {
        if (_body) {
             Vec3 vel = _body->getVelocity();
             vel.x = 0;
             vel.z = 0;
             _body->setVelocity(vel);
        }
        // Ensure dribble is stopped
        if (_dribbleSystem && _dribbleSystem->isDribbling()) {
            _dribbleSystem->stopDribble();
        }
        return;
    }

    if (_state == State::RECOVERY) {
        _recoveryTimer -= dt;
        if (_recoveryTimer <= 0) {
            _state = State::IDLE;
        } else {
             // Stop movement
             if (_body) {
                 Vec3 vel = _body->getVelocity();
                 vel.x = 0;
                 vel.z = 0;
                 _body->setVelocity(vel);
             }
             return;
        }
    }
    
    Vec2 input = _controller->getMoveInput();
    bool isSprint = _controller->isSprintPressed();
    if (isStunned) {
        isSprint = false;
    }

    if (_controller->isJumpPressed()) {
        jump();
    }
    
    if (isSprint && input.length() > 0.1f && _stamina > 0.0f) {
        _stamina -= 0.5f * dt;
    } else {
        _stamina += 0.25f * dt;
    }
    if (_stamina < 0.0f) _stamina = 0.0f;
    if (_stamina > 1.0f) _stamina = 1.0f;
    if (_stamina <= 0.0f) {
        isSprint = false;
    }
    
    float speed = SimplePhysics::PLAYER_SPEED * (_speedStat / 50.0f);
    if (isStunned) {
        speed *= 0.4f;
    }
    if (isSprint) speed *= 1.5f;
    
    if (input.length() > 0.1f) {
        // Move
        Vec3 dir(input.x, 0, input.y);
        dir.normalize();
        
        // Update physics velocity directly for responsiveness (arcade feel)
        Vec3 currentVel = _body->getVelocity();
        Vec3 targetVel = dir * speed;
        targetVel.y = currentVel.y; // Preserve gravity
        
        _body->setVelocity(targetVel);
        
        // Rotate visual
        if (_state == State::DEFENSING && _opponent) {
             // Face opponent
             Vec3 target = _opponent->getPosition3D();
             Vec3 myPos = getPosition3D();
             Vec3 dir = myPos - target; // Inverted to correct model orientation
             dir.y = 0; // Ignore height difference
             if (dir.lengthSquared() > 0.01f) {
                 dir.normalize();
                 float angle = atan2(dir.x, dir.z);
                 _visualNode->setRotation3D(Vec3(0, CC_RADIANS_TO_DEGREES(angle), 0));
             }
        } else {
             // Face movement direction
             float angle = atan2(dir.x, dir.z);
             _visualNode->setRotation3D(Vec3(0, CC_RADIANS_TO_DEGREES(angle), 0));
        }
        
        if (_hasBall) {
            _state = State::DRIBBLING;
        } else {
            // Check defense
            if (_controller->isDefendPressed()) {
                _state = State::DEFENSING;
                if (_defenseSystem) _defenseSystem->enterStance();
            } else {
                _state = State::IDLE; 
                if (_defenseSystem) _defenseSystem->exitStance();
            }
        }
    } else {
        // Stop
        Vec3 vel = _body->getVelocity();
        vel.x = 0;
        vel.z = 0;
        _body->setVelocity(vel);
        
        if (_state == State::DRIBBLING && !_hasBall) _state = State::IDLE;
        if (_state == State::DEFENSING) {
             // Keep stance if button held even if not moving?
             if (_controller->isDefendPressed()) {
                 if (_defenseSystem) _defenseSystem->enterStance();
             } else {
                 _state = State::IDLE;
                 if (_defenseSystem) _defenseSystem->exitStance();
             }
        }
    }
    
    // Speed Modifier for Stance
    if (_state == State::DEFENSING) {
         if (_opponent) {
              // Face opponent even if standing still
              Vec3 target = _opponent->getPosition3D();
              Vec3 myPos = getPosition3D();
              Vec3 dir = myPos - target; // Inverted
              dir.y = 0;
              if (dir.lengthSquared() > 0.01f) {
                  dir.normalize();
                  float angle = atan2(dir.x, dir.z);
                  _visualNode->setRotation3D(Vec3(0, CC_RADIANS_TO_DEGREES(angle), 0));
              }
          }

         if (_defenseSystem && _defenseSystem->isStance()) {
             Vec3 vel = _body->getVelocity();
             vel.x *= 0.6f;
             vel.z *= 0.6f;
             _body->setVelocity(vel);
         }
    }
    
    // Jump
    if (_controller->isJumpPressed()) {
        // Strict ground check to prevent double jump or moon jumps
        // Must be very close to ground AND not moving up significantly
        float yPos = _body->getPosition().y;
        float yVel = _body->getVelocity().y;
        
        // Check if grounded (Capsule center is at HEIGHT/2 = 0.9m when on floor)
        // We allow some tolerance (1.2f)
        if (yPos < 1.2f && std::abs(yVel) < 0.5f) { 
            Vec3 vel = _body->getVelocity();
            vel.y = SimplePhysics::PLAYER_JUMP_SPEED;
            _body->setVelocity(vel);
            
            CCLOG("Player Jumped! VelY: %.2f", vel.y);

            // If defending, attempt block
            if (!_hasBall && _defenseSystem) {
                _defenseSystem->attemptBlock();
            }
        }
    }
}

void Player::handleActions(float dt) {
    if (!_controller) return;

    // Check if grounded (User Request: Silence input in air)
    float currentY = _body ? _body->getPosition().y : getPosition3D().y;
    if (currentY > 0.15f) return;
    
    // Shoot
    if (_hasBall && _controller->isShootPressed()) {
        if (_state != State::SHOOTING) {
             _state = State::SHOOTING;
             if (_dribbleSystem) _dribbleSystem->stopDribble();
             if (_shootingSystem) _shootingSystem->startShot();
        }
    } else if (_state == State::SHOOTING && !_controller->isShootPressed()) {
        // Release shot
        if (_shootingSystem) _shootingSystem->releaseShot();
        
        // Enter Recovery State (Follow Through)
        _state = State::RECOVERY;
        _recoveryTimer = 0.5f; // 0.5s recovery time
    }
    
    // Steal
    if (!_hasBall && _controller->isStealPressed()) {
        if (_defenseSystem) _defenseSystem->attemptSteal();
    }
    
    // Crossover
    if (_hasBall && _controller->isCrossoverPressed()) {
        if (_dribbleSystem) {
             _dribbleSystem->crossover();
             
             // Trick Opponent
             if (_opponent && _opponent->getDefenseSystem()) {
                 _opponent->getDefenseSystem()->onOpponentCrossover();
             }
        }
    }
}

void Player::attemptShoot() {
    // Deprecated. Handled by ShootingSystem.
}

float Player::calculateHitChance(float distance) {
    return 0.0f; // Deprecated
}

void Player::updateVisuals() {
    if (!_body || !_visualNode) return;
    
    // Sync position
    Vec3 pos = _body->getPosition();
    
    // Simplified Foot IK: Feet always touching ground
    // Visual node is at 0,0,0 relative to parent? No, we setPosition3D on Player node.
    // Player node follows physics body.
    
    // If we are in air, we want the visual to look like it's jumping.
    // But for "Foot IK", usually means adjusting leg length to touch ground.
    // Here we just ensure the Player node is correct.
    
    // Rotate visual node to face velocity or target
    // We already handle rotation in handleMovement or controller.
    
    // Update Animation State
    if (_animPlayer) {
        _animPlayer->update(Director::getInstance()->getDeltaTime());
        
        if (_state == State::CELEBRATING) {
             _animPlayer->playState(AnimationPlayer::AnimState::CELEBRATE);
        } else if (_state == State::SHOOTING) {
            _animPlayer->playState(AnimationPlayer::AnimState::SHOOT);
            if (_shootingSystem) _shootingSystem->drawTrajectory(_trajectoryNode);
        } else if (_state == State::RECOVERY) {
             _animPlayer->playState(AnimationPlayer::AnimState::SHOOT);
        } else if (_state == State::DEFENSING) {
            _animPlayer->playState(AnimationPlayer::AnimState::DEFEND);
        } else {
            // Check movement
            _trajectoryNode->clear(); // Clear if not shooting
            Vec3 vel = _body->getVelocity();
            float speed = Vec2(vel.x, vel.z).length();
            
            if (pos.y > 0.2f) { // In air (threshold adjusted for 0.5m jump height)
                _animPlayer->playState(AnimationPlayer::AnimState::JUMP);
            } else if (_state == State::DRIBBLING) {
                if (speed > 0.1f) {
                    _animPlayer->playState(AnimationPlayer::AnimState::DRIBBLE);
                } else {
                    _animPlayer->playState(AnimationPlayer::AnimState::IDLE);
                }
            } else if (speed > 0.5f) {
                _animPlayer->playState(AnimationPlayer::AnimState::RUN);
            } else {
                _animPlayer->playState(AnimationPlayer::AnimState::IDLE);
            }
        }
    }
    
    updateStaminaBar();
}

void Player::updateBallPosition() {
    if (_hasBall && _ball) {
        // Calculate hand position (front of player)
        // Fix: Get rotation from 3D Y-axis
        float angle = CC_DEGREES_TO_RADIANS(_visualNode->getRotation3D().y);
        
        // Base offset: in front and slightly to right (right hand dribble)
        Vec3 offset(sin(angle) * 0.5f + cos(angle) * 0.3f, 0, cos(angle) * 0.5f - sin(angle) * 0.3f);
        Vec3 handPos = getPosition3D() + offset;
        
        if (_state == State::DRIBBLING) {
            // Dynamic height for dribbling
            // Bounce frequency: 2.0 bounces per second? 
            // Use system time or internal timer? We don't have a timer passed here, use Director time
            float time = Director::getInstance()->getTotalFrames() * Director::getInstance()->getAnimationInterval();
            float bounceHeight = 0.8f + abs(sin(time * 10.0f)) * 0.8f; // 0.8 to 1.6m
            
            handPos.y = bounceHeight;

            if (_ball->getState() != Basketball::State::DRIBBLING) _ball->setState(Basketball::State::DRIBBLING);
            _ball->dribble(handPos);
        } else if (_state == State::SHOOTING) {
            // Hold ball above head
            Vec3 shootPos = getPosition3D() + Vec3(0, HEIGHT + 0.2f, 0);
            if (_ball->getState() != Basketball::State::SHOOTING) _ball->setState(Basketball::State::SHOOTING);
            _ball->hold(shootPos);
        } else {
            // Hold ball
            if (_ball->getState() != Basketball::State::HELD) _ball->setState(Basketball::State::HELD);
            _ball->hold(handPos);
        }
    } else if (!_hasBall && _ball) {
        // Check if we can pickup ball
        if (_pickupCooldown > 0) return;

        if (_ball->getState() == Basketball::State::ON_GROUND || _ball->getState() == Basketball::State::FLYING) {
            Vec3 playerPos = getPosition3D();
            Vec3 ballPos = _ball->getPosition3D();
            float distXZ = Vec2(playerPos.x - ballPos.x, playerPos.z - ballPos.z).length();
            
            bool canPickup = false;
            
            // Physics-based contact check
            // Player Radius (0.3) + Ball Radius (0.25) = 0.55m
            // Allow small margin for arm reach -> 0.8m max
            float contactDist = 0.8f;
            
            if (_ball->getState() == Basketball::State::ON_GROUND) {
                // Must be close to touch
                if (distXZ < contactDist && abs(playerPos.y - ballPos.y) < 2.0f) {
                    canPickup = true;
                }
            } else {
                // FLYING (Shot, Rebound, Pass)
                // Strict contact required
                bool isFalling = _ball->getVelocity().y < 0;
                
                // If rising (Shot), radius must be tiny (Direct collision)
                float reqDist = isFalling ? contactDist : 0.4f; 
                
                if (distXZ < reqDist && abs(playerPos.y - ballPos.y) < 2.0f) {
                    canPickup = true;
                }
            }

            if (canPickup) {
                _hasBall = true;
                _ball->setOwner(this);
                _state = State::DRIBBLING;
                CCLOG("Ball Picked Up!");
            }
        }
    }
}

void Player::updateStaminaBar() {
    if (!_staminaNode) return;
    Scene* scene = Director::getInstance()->getRunningScene();
    if (!scene) return;
    Camera* cam = nullptr;
    // Prefer the same camera flag as player (USER1). Fallback to default.
    const auto& cams = scene->getCameras();
    for (auto* c : cams) {
        if (c->getCameraFlag() == CameraFlag::USER1) { cam = c; break; }
    }
    if (!cam) cam = scene->getDefaultCamera();
    if (!cam) return;
    
    Vec3 headWorld = getPosition3D() + Vec3(0, HEIGHT + 0.3f, 0);
    Vec2 screenPos = cam->projectGL(headWorld);
    
    _staminaNode->clear();
    
    float barWidth = 80.0f;
    float barHeight = 10.0f;
    float yOffset = 40.0f;
    
    Vec2 origin(screenPos.x - barWidth * 0.5f, screenPos.y + yOffset);
    Vec2 dest(origin.x + barWidth, origin.y + barHeight);
    
    _staminaNode->drawSolidRect(origin, dest, Color4F(0.2f, 0.2f, 0.2f, 0.6f));
    
    float pct = _stamina;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    Vec2 fillDest(origin.x + barWidth * pct, origin.y + barHeight);
    _staminaNode->drawSolidRect(origin, fillDest, Color4F(0.2f, 0.8f, 0.2f, 0.9f));
    
    _staminaNode->drawRect(origin, dest, Color4F(0, 0, 0, 1.0f));
}
