#include "AIController.h"
#include "Player.h"
#include "Basketball.h"
#include "SimplePhysics.h"
#include "ScoreManager.h"

USING_NS_CC;

AIController::AIController(Player* opponent, Basketball* ball, AIBrain::Difficulty difficulty)
    : _opponent(opponent)
    , _ball(ball)
    , _moveInput(Vec2::ZERO)
    , _sprint(false)
    , _jump(false)
    , _shoot(false)
    , _steal(false)
    , _defend(false)
    , _crossover(false)
{
    _brain = new AIBrain(difficulty);
}

AIController::~AIController() {
    delete _brain;
}

void AIController::update(float dt) {
    if (!_player || !_opponent || !_ball) return;
    
    AIBrain::InputData input;
    input.selfPos = _player->getPosition3D();
    input.opponentPos = _opponent->getPosition3D();
    if (_opponent->getBody()) {
        input.opponentVel = _opponent->getBody()->getVelocity();
    } else {
        input.opponentVel = Vec3::ZERO;
    }
    input.ballPos = _ball->getPosition3D();
    input.hoopPos = Vec3(0, SimplePhysics::HOOP_HEIGHT, SimplePhysics::HOOP_Z); // Assuming hoop is at fixed Z
    
    input.hasBall = _player->hasBall();
    input.opponentHasBall = _opponent->hasBall();
    input.opponentIsShooting = (_opponent->getState() == Player::State::SHOOTING);
    input.needsClear = _player->mustClearBall();
    input.dt = dt;
    input.shotClock = ScoreManager::getInstance()->getShotClock();
    
    _brain->update(input);
    
    // Sync Output
    const AIBrain::OutputData& output = _brain->getOutput();
    _moveInput = output.moveDir;
    // Infinite stamina for AI: Sprint is always allowed if requested
    _sprint = output.sprint; 
    _jump = output.jump;
    _shoot = output.shoot;
    _steal = output.steal;
    _defend = output.defend;
    _crossover = output.crossover;
}

Vec2 AIController::getMoveInput() {
    return _moveInput;
}

bool AIController::isSprintPressed() {
    return _sprint;
}

bool AIController::isJumpPressed() {
    return _jump;
}

bool AIController::isShootPressed() {
    return _shoot;
}

bool AIController::isPassPressed() {
    return false; // AI doesn't pass in 1v1
}

bool AIController::isStealPressed() {
    return _steal;
}

bool AIController::isCrossoverPressed() {
    return _crossover;
}

bool AIController::isDefendPressed() {
    return _defend;
}
