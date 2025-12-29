#include "ScoreManager.h"
#include "SaveSystem.h"
#include "GameFeedback.h"

ScoreManager* ScoreManager::_instance = nullptr;

ScoreManager* ScoreManager::getInstance() {
    if (!_instance) {
        _instance = new ScoreManager();
    }
    return _instance;
}

void ScoreManager::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

ScoreManager::ScoreManager() {
    reset();
}

ScoreManager::~ScoreManager() {}

void ScoreManager::reset() {
    _playerScore = 0;
    _aiScore = 0;
    _gameTime = QUARTER_TIME;
    _shotClock = 24.0f;
    _currentQuarter = 1;
}

void ScoreManager::update(float dt) {
    if (isGameOver()) return;
    
    _gameTime -= dt;
    _shotClock -= dt;
    
    if (_shotClock < 0) _shotClock = 0; // Violation logic handled in GameRules
    
    if (_gameTime <= 0) {
        if (_currentQuarter < MAX_QUARTERS) {
            // End of Quarter
            _currentQuarter++;
            _gameTime = QUARTER_TIME;
            _shotClock = 24.0f; // Reset shot clock too? Usually inbound.
            
            // Auto Save
            SaveSystem::getInstance()->saveMatchProgress(_playerScore, _aiScore, _gameTime, _currentQuarter);
            
            // Visual Feedback
            if (GameFeedback::getInstance()) {
                GameFeedback::getInstance()->showShotResult("QUARTER END", cocos2d::Vec3(0, 5, 0), true);
            }
        } else {
            // End of Game (Game Over handled by isGameOver)
            _gameTime = 0;
        }
    }
}

void ScoreManager::loadState(int pScore, int aiScore, float time, int quarter) {
    _playerScore = pScore;
    _aiScore = aiScore;
    _gameTime = time;
    _currentQuarter = quarter;
}

void ScoreManager::addScore(bool isPlayer, int points) {
    if (isPlayer) {
        _playerScore += points;
    } else {
        _aiScore += points;
    }
}

bool ScoreManager::isGameOver() const {
    // Win by score limit OR time runs out in final period
    bool timeOver = (_currentQuarter >= MAX_QUARTERS && _gameTime <= 0);
    return timeOver || _playerScore >= WIN_SCORE || _aiScore >= WIN_SCORE;
}

std::string ScoreManager::getWinner() const {
    if (!isGameOver()) return "None";
    if (_playerScore > _aiScore) return "Player";
    if (_aiScore > _playerScore) return "AI";
    if (_playerScore == _aiScore) return "Draw"; // Time up
    return "None";
}