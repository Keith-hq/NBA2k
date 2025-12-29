#ifndef __SAVE_SYSTEM_H__
#define __SAVE_SYSTEM_H__

#include "cocos2d.h"
#include "GameData.h"
#include "json/document.h"

class SaveSystem {
public:
    static SaveSystem* getInstance();
    static void destroyInstance();

    // Init loads data from disk
    void init();

    // Save/Load
    void save();
    void load();
    
    // Accessors
    GameData::Settings& getSettings() { return _data.settings; }
    GameData::Stats& getStats() { return _data.stats; }
    GameData::MatchProgress& getMatchProgress() { return _data.currentMatch; }
    
    // Helpers
    void saveSettings(float master, float sfx, float music);
    void updateStats(bool isWin, int score);
    void saveMatchProgress(int pScore, int aiScore, float time, int quarter);
    void clearMatchProgress();

private:
    SaveSystem();
    ~SaveSystem();

    static SaveSystem* _instance;
    GameData::SaveFile _data;
    std::string _filePath;
    const std::string FILE_NAME = "save_data.dat";
    const std::string SECRET_KEY = "NBA2K_LITE_SECRET";

    // Internal
    std::string encrypt(const std::string& input);
    std::string decrypt(const std::string& input);
    std::string calculateHash(const std::string& content);
    
    // JSON Serialization
    std::string serialize(const GameData::SaveFile& data);
    bool deserialize(const std::string& json, GameData::SaveFile& outData);
};

#endif // __SAVE_SYSTEM_H__
