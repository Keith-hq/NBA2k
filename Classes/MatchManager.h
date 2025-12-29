#ifndef __MATCH_MANAGER_H__
#define __MATCH_MANAGER_H__

#include "cocos2d.h"
#include "Player.h"
#include "Basketball.h"

struct PlayerStats {
    int points;
    int rebounds;
    int assists;
    
    PlayerStats() : points(0), rebounds(0), assists(0) {}
};

class MatchManager {
public:
    static MatchManager* getInstance();
    static void destroyInstance();

    void init(Player* player, Player* aiPlayer, Basketball* ball);
    void reset();
    void update(float dt);

    // Match Actions
    void startMatch();
    void pauseMatch();
    void resumeMatch();
    void endMatch();
    
    void triggerJumpBall();
    
    // Gameplay Events
    void handleGoal(bool isPlayerScored, int points);
    void handleViolation(bool isPlayerViolation, const std::string& violationName);
    void startCheckBall(bool isPlayerBall);

    // Stats
    void recordPoint(bool isPlayer, int points);
    void recordRebound(bool isPlayer);
    void recordAssist(bool isPlayer);
    
    const PlayerStats& getPlayerStats() const { return _playerStats; }
    const PlayerStats& getAIStats() const { return _aiStats; }

private:
    MatchManager();
    ~MatchManager();

    static MatchManager* _instance;

    Player* _player;
    Player* _aiPlayer;
    Basketball* _ball;

    PlayerStats _playerStats;
    PlayerStats _aiStats;

    bool _isJumpBallActive;
    float _jumpBallTimer;
};

#endif // __MATCH_MANAGER_H__
