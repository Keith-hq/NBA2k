#include "MatchManager.h"
#include "GameFlow.h"
#include "ScoreManager.h"
#include "GameUI.h"
#include "GameFeedback.h"
#include "SimplePhysics.h"
#include "SaveSystem.h"

USING_NS_CC;

MatchManager* MatchManager::_instance = nullptr;

MatchManager* MatchManager::getInstance() {
    if (!_instance) {
        _instance = new MatchManager();
    }
    return _instance;
}

void MatchManager::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

MatchManager::MatchManager() 
    : _player(nullptr)
    , _aiPlayer(nullptr)
    , _ball(nullptr)
    , _isJumpBallActive(false)
    , _jumpBallTimer(0.0f)
{
}

MatchManager::~MatchManager() {
}

void MatchManager::init(Player* player, Player* aiPlayer, Basketball* ball) {
    _player = player;
    _aiPlayer = aiPlayer;
    _ball = ball;
    
    // Init Save System
    SaveSystem::getInstance()->init();
    
    // Check for saved game?
    // For now, we reset. If we want to support resume, we need a way to trigger it.
    // Let's Auto-Load if a match was in progress?
    auto& progress = SaveSystem::getInstance()->getMatchProgress();
    if (progress.hasSavedMatch) {
        // Resume
        ScoreManager::getInstance()->loadState(
            progress.playerScore, 
            progress.aiScore, 
            progress.timeRemaining, 
            progress.currentQuarter
        );
        CCLOG("Resumed match from save.");
    } else {
        reset();
    }
}

void MatchManager::reset() {
    _playerStats = PlayerStats();
    _aiStats = PlayerStats();
    _isJumpBallActive = false;
    _jumpBallTimer = 0.0f;
    ScoreManager::getInstance()->reset();
}

void MatchManager::update(float dt) {
    if (GameFlow::getInstance()->getState() != GameFlow::State::PLAYING) {
        if (GameFlow::getInstance()->getState() == GameFlow::State::READY) {
             if (_isJumpBallActive) {
                _jumpBallTimer += dt;
                // Wait 1 second then toss ball
                if (_jumpBallTimer > 1.0f && _ball->getPosition3D().y < 2.0f) {
                    // Toss ball up
                    _ball->setVelocity(Vec3(0, 10, 0)); // Upward force
                    _isJumpBallActive = false; // Ball is in air
                    GameFlow::getInstance()->changeState(GameFlow::State::PLAYING);
                    
                    // Show "Jump Ball!"
                    GameFeedback::getInstance()->showShotResult("JUMP BALL!", Vec3(0, 3, 0), true);
                }
             }
        }
        return;
    }

    // Check Win Condition via ScoreManager
    if (ScoreManager::getInstance()->isGameOver()) {
        endMatch();
    }
}

void MatchManager::startMatch() {
    // 1v1 Game Start Logic
    GameFlow::getInstance()->changeState(GameFlow::State::PLAYING);
    
    // Player starts with ball (Check Ball)
    startCheckBall(true);
}

void MatchManager::pauseMatch() {
    if (GameFlow::getInstance()->getState() == GameFlow::State::PLAYING) {
        GameFlow::getInstance()->changeState(GameFlow::State::PAUSED);
        GameUI::getInstance()->showPauseMenu(true);
    }
}

void MatchManager::resumeMatch() {
    if (GameFlow::getInstance()->getState() == GameFlow::State::PAUSED) {
        GameFlow::getInstance()->changeState(GameFlow::State::PLAYING);
        GameUI::getInstance()->showPauseMenu(false);
    }
}

void MatchManager::endMatch() {
    GameFlow::getInstance()->changeState(GameFlow::State::FINISHED);
    
    std::string winner = ScoreManager::getInstance()->getWinner();
    bool isPlayerWin = (winner == "Player"); 
    
    // Update Stats
    SaveSystem::getInstance()->updateStats(isPlayerWin, ScoreManager::getInstance()->getPlayerScore());
    SaveSystem::getInstance()->clearMatchProgress();
    
    GameUI::getInstance()->showGameOver(isPlayerWin);
}

void MatchManager::handleGoal(bool isPlayerScored, int points) {
    // Record Score
    ScoreManager::getInstance()->addScore(isPlayerScored, points);
    recordPoint(isPlayerScored, points);
    
    // Feedback
    // Determine position for feedback
    Vec3 hoopCenter(0, SimplePhysics::HOOP_HEIGHT, SimplePhysics::HOOP_Z);
    GameFeedback::getInstance()->showScore(points, hoopCenter);
    
    if (isPlayerScored) {
        if (_player) _player->celebrate();
    } else {
        if (_aiPlayer) _aiPlayer->celebrate();
    }
    
    // Reset Shot Clock
    ScoreManager::getInstance()->resetShotClock();
    
    // Check Win
    if (ScoreManager::getInstance()->isGameOver()) {
        endMatch();
        return;
    }
    
    // Exchange Possession (Loser's Ball)
    // If Player scored, AI gets ball.
    bool nextIsPlayerBall = !isPlayerScored;
    
    // Delay slightly before check ball?
    // For now, immediate transition or maybe a small delay via GameFlow/Action
    // We can use a delayed callback
    cocos2d::Director::getInstance()->getScheduler()->schedule([this, nextIsPlayerBall](float dt){
        startCheckBall(nextIsPlayerBall);
    }, this, 2.0f, 0, 0.0f, false, "check_ball_delay");
}

void MatchManager::handleViolation(bool isPlayerViolation, const std::string& violationName) {
    CCLOG("Violation: %s by %s", violationName.c_str(), isPlayerViolation ? "Player" : "AI");
    
    GameFeedback::getInstance()->showShotResult(violationName, Vec3(0, 3, 0), true);
    
    ScoreManager::getInstance()->resetShotClock();
    
    // Turnover: Possession goes to non-violator
    bool nextIsPlayerBall = !isPlayerViolation;
    
    startCheckBall(nextIsPlayerBall);
}

void MatchManager::startCheckBall(bool isPlayerBall) {
    if (!_player || !_aiPlayer || !_ball) return;
    
    // Reset Positions for 1v1 Check Ball
    // Attacker at top of key (0, 0, 6) or (0, 0, 5) ?
    // Defender inside?
    // Let's assume standard "Check Ball" at 3pt line top (approx z=7)
    
    float checkZ = 7.0f;
    float defendZ = 6.0f; // Defender slightly inside
    
    if (isPlayerBall) {
        _player->setPosition3D(Vec3(0, 0, checkZ));
        _player->setRotation3D(Vec3(0, 180, 0)); // Face hoop
        
        _aiPlayer->setPosition3D(Vec3(0, 0, defendZ - 2.0f)); // Defender giving space? Or tight?
        // Usually defender checks ball then passes back.
        // Simplified: Attacker starts with ball at top.
        _aiPlayer->setPosition3D(Vec3(0, 0, 5.0f));
        _aiPlayer->setRotation3D(Vec3(0, 0, 0)); // Face attacker
        
        // Give Ball
        _ball->setPosition3D(Vec3(0, 1, checkZ));
        _ball->setVelocity(Vec3::ZERO);
        _player->setBall(_ball);
        _player->setPossession(true);
        _aiPlayer->setPossession(false);
        
    } else {
        _aiPlayer->setPosition3D(Vec3(0, 0, checkZ));
        _aiPlayer->setRotation3D(Vec3(0, 180, 0));
        
        _player->setPosition3D(Vec3(0, 0, 5.0f));
        _player->setRotation3D(Vec3(0, 0, 0));
        
        // Give Ball
        _ball->setPosition3D(Vec3(0, 1, checkZ));
        _ball->setVelocity(Vec3::ZERO);
        _aiPlayer->setBall(_ball);
        _aiPlayer->setPossession(true);
        _player->setPossession(false);
    }
    
    _ball->setState(Basketball::State::HELD);
    
    GameFeedback::getInstance()->showShotResult("CHECK BALL", Vec3(0, 3, 0), true);
}

void MatchManager::triggerJumpBall() {
    if (!_player || !_aiPlayer || !_ball) return;
    
    _isJumpBallActive = true;
    _jumpBallTimer = 0.0f;
    
    // Reset positions
    _player->setPosition3D(Vec3(0, 0, 2));
    _player->setRotation3D(Vec3(0, 180, 0)); // Face center
    
    _aiPlayer->setPosition3D(Vec3(0, 0, -2));
    _aiPlayer->setRotation3D(Vec3(0, 0, 0)); // Face center
    
    _ball->setPosition3D(Vec3(0, 1, 0));
    _ball->setVelocity(Vec3::ZERO);
    _ball->setState(Basketball::State::NONE);
}

void MatchManager::recordPoint(bool isPlayer, int points) {
    if (isPlayer) _playerStats.points += points;
    else _aiStats.points += points;
}

void MatchManager::recordRebound(bool isPlayer) {
    if (isPlayer) _playerStats.rebounds++;
    else _aiStats.rebounds++;
    
    // Feedback
    Vec3 pos = isPlayer ? _player->getPosition3D() : _aiPlayer->getPosition3D();
    GameFeedback::getInstance()->showShotResult("REBOUND", pos, true);
}

void MatchManager::recordAssist(bool isPlayer) {
    if (isPlayer) _playerStats.assists++;
    else _aiStats.assists++;
}
