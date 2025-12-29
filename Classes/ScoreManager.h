#ifndef __SCORE_MANAGER_H__
#define __SCORE_MANAGER_H__

#include "cocos2d.h"

class ScoreManager {
public:
    static ScoreManager* getInstance();
    static void destroyInstance();
    
    void reset();
    void update(float dt);
    
    // Score
    void addScore(bool isPlayer, int points);
    int getPlayerScore() const { return _playerScore; }
    int getAIScore() const { return _aiScore; }
    
    // Time
    float getGameTime() const { return _gameTime; }
    float getShotClock() const { return _shotClock; }
    void resetShotClock() { _shotClock = 24.0f; }
    
    int getCurrentQuarter() const { return _currentQuarter; }

    // Game State
    bool isGameOver() const;
    std::string getWinner() const;

    // Load State
    void loadState(int pScore, int aiScore, float time, int quarter);
    
private:
    ScoreManager();
    ~ScoreManager();
    
    static ScoreManager* _instance;
    
    int _playerScore;
    int _aiScore;
    
    float _gameTime;
    float _shotClock;
    int _currentQuarter;
    
    // 1v1 Settings
    const float QUARTER_TIME = 600.0f; // 10 minutes (effectively single period)
    const int MAX_QUARTERS = 1;        // Single period for 1v1
    const int WIN_SCORE = 21;          // Standard 1v1 score limit
};

#endif // __SCORE_MANAGER_H__