#include "GameRules.h"
#include "ScoreManager.h"
#include "SimplePhysics.h"
#include "AudioManager.h"
#include "SoundBank.h"
#include "EffectsManager.h"
#include "GameFeedback.h"
#include "MatchManager.h"

USING_NS_CC;

GameRules::GameRules(Player* player, Player* aiPlayer, Basketball* ball)
    : _player(player)
    , _aiPlayer(aiPlayer)
    , _ball(ball)
    , _currentOffense(nullptr)
    , _lastViolation(Violation::NONE)
    , _possessionTimer(0.0f)
    , _needsToClearBall(false)
{
    ScoreManager::getInstance()->reset();
    if (_ball) {
        _prevBallPos = _ball->getPosition3D();
    }
}

GameRules::~GameRules() {}

void GameRules::update(float dt) {
    auto sm = ScoreManager::getInstance();
    sm->update(dt);
    
    if (sm->isGameOver()) {
        _lastViolation = Violation::GAME_OVER;
        return;
    }

    // Detect Possession Change
    Player* holder = nullptr;
    if (_player && _player->hasBall()) holder = _player;
    else if (_aiPlayer && _aiPlayer->hasBall()) holder = _aiPlayer;
    
    if (holder && holder != _currentOffense) {
        onPossessionChange(holder);
    } else if (!holder && _currentOffense) {
        // Ball is loose?
        // We keep _currentOffense as "last known offense" until someone else picks it up
        // This allows shot clock to continue? Or should it stop?
        // NBA Rules: Shot clock continues on loose ball if caused by offense?
        // But resets on rim hit.
    }
    
    checkOutOfBounds();
    checkShotClock();
    checkTraveling(dt);
    checkClearBall();
    checkWinCondition();
    checkGoal();
    
    if (_ball) {
        _prevBallPos = _ball->getPosition3D();
    }
}

void GameRules::checkGoal() {
    if (!_ball) return;
    
    Vec3 currPos = _ball->getPosition3D();
    
    // Hoop: (0, HOOP_HEIGHT, HOOP_Z)
    float hoopY = SimplePhysics::HOOP_HEIGHT;
    Vec3 hoopCenter(0, hoopY, SimplePhysics::HOOP_Z);
    float hoopRadius = 0.3f; // Slightly generous
    
    // Check if passed through hoop plane from top to bottom
    if (_prevBallPos.y >= hoopY && currPos.y < hoopY) {
        // Check X/Z distance
        float distXZ = Vec2(currPos.x - hoopCenter.x, currPos.z - hoopCenter.z).length();
        
        if (distXZ < hoopRadius) {
            
            // Check Clear Ball Rule
            if (_needsToClearBall) {
                 CCLOG("GOAL INVALID: Did not clear ball!");
                 GameFeedback::getInstance()->showShotResult("MUST CLEAR BALL!", Vec3(0, 3, 0), true);
                 // No points.
                 // Treat as turnover? Or just continue play?
                 // In streetball, if you score without clearing, it doesn't count, and opponent gets ball usually.
                 // Let's treat it as a violation/turnover.
                 
                 bool isPlayerViolation = (_currentOffense == _player);
                 MatchManager::getInstance()->handleViolation(isPlayerViolation, "DID NOT CLEAR");
                 
                 _ball->setVelocity(Vec3::ZERO);
                 _ball->setOwner(nullptr);
                 _ball->setState(Basketball::State::NONE);
                 return;
            }

            // GOAL!
            CCLOG("GOAL SCRORED!");
            AudioManager::getInstance()->playSpatialEffect(SoundBank::SFX_SWISH, hoopCenter);
            
            // Determine points
            int points = calculateShotPoints(_shotPos);
            
            bool isPlayer = (_currentOffense == _player);
            
            // Delegate to MatchManager
            MatchManager::getInstance()->handleGoal(isPlayer, points);
            
            // Reset ball state immediately to prevent double triggering or weird physics during celebration
            _ball->setVelocity(Vec3::ZERO);
            _ball->setOwner(nullptr);
            _ball->setState(Basketball::State::NONE); 
            // MatchManager will reposition it shortly
        }
    }
}

void GameRules::checkOutOfBounds() {
    if (!_ball) return;
    
    Vec3 pos = _ball->getPosition3D();
    float halfWidth = SimplePhysics::COURT_WIDTH / 2.0f;
    float halfLength = SimplePhysics::COURT_LENGTH / 2.0f;
    
    // Tolerance
    float tolerance = 0.5f;
    
    bool out = (pos.x < -halfWidth - tolerance || pos.x > halfWidth + tolerance ||
                pos.z < -halfLength - tolerance || pos.z > halfLength + tolerance);
                
    if (out) {
        // Only trigger if not already handled (simple check)
        if (_lastViolation == Violation::OUT_OF_BOUNDS) return;

        _lastViolation = Violation::OUT_OF_BOUNDS;
        
        // Determine violator (whoever touched it last or current offense)
        // Simplified: Assume current offense caused it
        bool isPlayerViolation = (_currentOffense == _player);
        
        MatchManager::getInstance()->handleViolation(isPlayerViolation, "OUT OF BOUNDS");
        
        // Stop ball
        _ball->setVelocity(Vec3::ZERO);
        _ball->setState(Basketball::State::NONE);
    }
}

void GameRules::checkShotClock() {
    if (ScoreManager::getInstance()->getShotClock() <= 0) {
        if (_lastViolation == Violation::SHOT_CLOCK) return;

        _lastViolation = Violation::SHOT_CLOCK;
        
        bool isPlayerViolation = (_currentOffense == _player);
        MatchManager::getInstance()->handleViolation(isPlayerViolation, "SHOT CLOCK");
    }
}

void GameRules::checkTraveling(float dt) {
    // Simplified: "Holding ball moving > 3s"
    Player* holder = nullptr;
    if (_player->hasBall()) holder = _player;
    else if (_aiPlayer->hasBall()) holder = _aiPlayer;
    
    if (holder) {
        // If Dribbling, no traveling
        if (holder->getState() == Player::State::DRIBBLING) {
            _possessionTimer = 0.0f;
            return;
        }

        Vec3 vel = holder->getBody()->getVelocity();
        // Check horizontal movement
        float speed = Vec2(vel.x, vel.z).length();
        
        if (speed > 0.5f) {
            _possessionTimer += dt;
            if (_possessionTimer > TRAVELING_LIMIT) {
                if (_lastViolation == Violation::TRAVELING) return;

                _lastViolation = Violation::TRAVELING;
                _possessionTimer = 0.0f;
                
                bool isPlayerViolation = (holder == _player);
                MatchManager::getInstance()->handleViolation(isPlayerViolation, "TRAVELING");
                
                holder->setPossession(false);
            }
        } else {
            _possessionTimer = 0.0f;
        }
    } else {
        _possessionTimer = 0.0f;
    }
}

void GameRules::checkWinCondition() {
    if (ScoreManager::getInstance()->isGameOver()) {
        // Handle Game Over
        CCLOG("Game Over! Winner: %s", ScoreManager::getInstance()->getWinner().c_str());
    }
}

void GameRules::onBallShot(Player* shooter) {
    _currentOffense = shooter;
    if (shooter) {
        _shotPos = shooter->getPosition3D();
    }
}

void GameRules::onBallHitRim() {
    ScoreManager::getInstance()->resetShotClock();
}

void GameRules::onPossessionChange(Player* newOwner) {
    if (_currentOffense != newOwner) {
        ScoreManager::getInstance()->resetShotClock();
        _lastViolation = Violation::NONE; // Clear violation on possession change
        
        // Check if inside 3pt line
        // Hoop Pos: (0, 0, HOOP_Z)
        Vec3 hoopPos(0, 0, SimplePhysics::HOOP_Z);
        Vec3 playerPos = newOwner->getPosition3D();
        playerPos.y = 0;
        
        float dist = playerPos.distance(hoopPos);
        if (dist < THREE_POINT_DIST) {
            _needsToClearBall = true;
            if (newOwner) newOwner->setMustClearBall(true);
            CCLOG("Possession Change Inside 3PT: Needs Clear");
            GameFeedback::getInstance()->showShotResult("CLEAR BALL!", Vec3(0, 3, 0), false);
        } else {
            _needsToClearBall = false;
            if (newOwner) newOwner->setMustClearBall(false);
        }
    }
    _currentOffense = newOwner;
}

void GameRules::checkClearBall() {
    if (_needsToClearBall && _currentOffense) {
        // Ensure flag is consistent
        _currentOffense->setMustClearBall(true);

        Vec3 hoopPos(0, 0, SimplePhysics::HOOP_Z);
        Vec3 playerPos = _currentOffense->getPosition3D();
        playerPos.y = 0;
        
        float dist = playerPos.distance(hoopPos);
        if (dist > THREE_POINT_DIST) {
            _needsToClearBall = false;
            _currentOffense->setMustClearBall(false);
            CCLOG("Ball Cleared!");
            GameFeedback::getInstance()->showShotResult("CLEARED", Vec3(0, 3, 0), false);
            AudioManager::getInstance()->playEffect(SoundBank::SFX_WHISTLE); // Or some positive sound
        }
    }
}

int GameRules::calculateShotPoints(const Vec3& shootPos) {
    // Hoop Pos: (0, HOOP_HEIGHT, HOOP_Z)
    // HOOP_Z is negative.
    Vec3 hoopPos(0, 0, SimplePhysics::HOOP_Z);
    Vec3 floorPos(shootPos.x, 0, shootPos.z);
    
    float dist = floorPos.distance(hoopPos);
    
    if (dist > THREE_POINT_DIST) return 3;
    return 2;
}