#ifndef __GAME_DATA_H__
#define __GAME_DATA_H__

#include <string>
#include "cocos2d.h"

namespace GameData {

    struct Settings {
        float masterVolume;
        float sfxVolume;
        float musicVolume;
        bool vibrationEnabled;

        Settings() 
            : masterVolume(1.0f)
            , sfxVolume(1.0f)
            , musicVolume(1.0f)
            , vibrationEnabled(true) 
        {}
    };

    struct Stats {
        int totalWins;
        int totalLosses;
        int highScore;
        int totalPointsScored;
        int totalGamesPlayed;

        Stats() 
            : totalWins(0)
            , totalLosses(0)
            , highScore(0)
            , totalPointsScored(0)
            , totalGamesPlayed(0) 
        {}
    };

    struct MatchProgress {
        bool hasSavedMatch;
        int playerScore;
        int aiScore;
        float timeRemaining;
        int currentQuarter;
        // Could add player positions, ball position if we want deep resume

        MatchProgress() 
            : hasSavedMatch(false)
            , playerScore(0)
            , aiScore(0)
            , timeRemaining(0.0f)
            , currentQuarter(1) 
        {}
    };

    struct SaveFile {
        Settings settings;
        Stats stats;
        MatchProgress currentMatch;
        std::string hash; // For integrity check

        SaveFile() {}
    };

}

#endif // __GAME_DATA_H__
